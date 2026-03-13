#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "world_loader.h"
#include "room.h"
#include "graph.h"
#include "datagen.h"
#include "types.h"

/* ======================= Internal Helpers ======================= */

// Graph callback to compare two Room pointers by their ID.
static int room_compare(const void *a, const void *b) {
    const Room *ra = (const Room *)a;
    const Room *rb = (const Room *)b;
    if (ra == NULL && rb == NULL) return 0;
    if (ra == NULL) return -1;
    if (rb == NULL) return 1;
    return ra->id - rb->id;
}

// Graph callback to destroy a Room payload.
static void room_destroy_payload(void *payload) {
    if (payload) {
        room_destroy((Room *)payload);
    }
}

// Deep-copies all data from a datagen room (dg_room) into a newly created room.
static Status copy_room_data(Room *room, const DG_Room *dg_room){
    // 1. Copy floor grid
    if (dg_room->floor_grid) {
        size_t grid_size = (size_t)dg_room->width * (size_t)dg_room->height * sizeof(bool);
        bool *grid_copy = malloc(grid_size);
        if (!grid_copy) return NO_MEMORY;
        memcpy(grid_copy, dg_room->floor_grid, grid_size);
        if (room_set_floor_grid(room, grid_copy) != OK) {
            free(grid_copy);
            return INTERNAL_ERROR;
        }
    }

    // 2. Copy portals
    if (dg_room->portal_count > 0) {
        Portal *portals_copy = malloc((size_t)dg_room->portal_count * sizeof(Portal));
        if (!portals_copy) return NO_MEMORY;
        for (int i = 0; i < dg_room->portal_count; i++) {
            portals_copy[i].id = dg_room->portals[i].id;
            portals_copy[i].name = NULL; // No names in A1 datagen
            portals_copy[i].x = dg_room->portals[i].x;
            portals_copy[i].y = dg_room->portals[i].y;
            portals_copy[i].target_room_id = dg_room->portals[i].neighbor_id;
            portals_copy[i].gated = false; // Default to ungated
            portals_copy[i].required_switch_id = -1; // No switch required
        }
        if (room_set_portals(room, portals_copy, dg_room->portal_count) != OK) {
            free(portals_copy);
            return INTERNAL_ERROR;
        }
    }

    // 3. Copy treasures
    if (dg_room->treasure_count > 0) {
        Treasure *treasures_copy = malloc((size_t)dg_room->treasure_count * sizeof(Treasure));
        if (!treasures_copy) return NO_MEMORY;
        for (int i = 0; i < dg_room->treasure_count; i++) {
            treasures_copy[i].id = dg_room->treasures[i].global_id;
            treasures_copy[i].name = NULL; // Names not used in A1 datagen
            treasures_copy[i].x = dg_room->treasures[i].x;
            treasures_copy[i].y = dg_room->treasures[i].y;
            treasures_copy[i].collected = false;
        }
        if (room_set_treasures(room, treasures_copy, dg_room->treasure_count) != OK) {
            free(treasures_copy);
            return INTERNAL_ERROR;
        }
    }

    return OK;
}


// Iterates through all rooms to connect graph edges based on portal destinations.
static Status connect_graph_edges(Graph *graph) {
    const void * const *payloads = NULL;
    int payload_count = 0;
    if (graph_get_all_payloads(graph, &payloads, &payload_count) != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }

    for (int i = 0; i < payload_count; i++) {
        const Room *src_room = (const Room *)payloads[i];
        for (int p = 0; p < src_room->portal_count; p++) {
            int target_id = src_room->portals[p].target_room_id;
            if (target_id < 0) continue;

            // Find the destination room in the payload list
            const Room *dst_room = NULL;
            for (int j = 0; j < payload_count; j++) {
                const Room *candidate = (const Room *)payloads[j];
                if (candidate->id == target_id) {
                    dst_room = candidate;
                    break;
                }
            }

            if (dst_room && graph_connect(graph, src_room, dst_room) == GRAPH_STATUS_NO_MEMORY) {
                return NO_MEMORY;
            }
        }
    }
    return OK;
}


/* ======================= Public API ======================= */

Status loader_load_world(const char *config_file,
                         Graph **graph_out,
                         Room **first_room_out,
                         int *num_rooms_out,
                         Charset *charset_out) {
    if (!config_file || !graph_out || !first_room_out || !num_rooms_out || !charset_out) {
        return INVALID_ARGUMENT;
    }

    *graph_out = NULL;
    *first_room_out = NULL;
    *num_rooms_out = 0;
    memset(charset_out, 0, sizeof(Charset));

    if (start_datagen(config_file) != DG_OK) {
        return WL_ERR_CONFIG;
    }

    const DG_Charset *dg_cs = dg_get_charset();
    if (!dg_cs) {
        stop_datagen();
        return WL_ERR_DATAGEN;
    }
    
    charset_out->wall = dg_cs->wall;
    charset_out->floor = dg_cs->floor;
    charset_out->player = dg_cs->player;
    charset_out->treasure = dg_cs->treasure;
    charset_out->portal = dg_cs->portal;
    charset_out->pushable = dg_cs->pushable;

    Graph *graph = NULL;
    if (graph_create(room_compare, room_destroy_payload, &graph) != GRAPH_STATUS_OK) {
        stop_datagen();
        return NO_MEMORY;
    }

    int room_count = 0;
    while (has_more_rooms()) {
        DG_Room dg_room = get_next_room();
        Room *room = room_create(dg_room.id, NULL, dg_room.width, dg_room.height);
        if (!room || copy_room_data(room, &dg_room) != OK || graph_insert(graph, room) != GRAPH_STATUS_OK) {
            room_destroy(room); // Safe on NULL
            graph_destroy(graph);
            stop_datagen();
            return NO_MEMORY;
        }
        if (!*first_room_out) {
            *first_room_out = room;
        }
        room_count++;
    }

    if (connect_graph_edges(graph) != OK) {
        graph_destroy(graph);
        stop_datagen();
        return INTERNAL_ERROR;
    }
    
    *graph_out = graph;
    *num_rooms_out = room_count;
    stop_datagen();

    return OK;
}