#include "game_engine.h"
#include "world_loader.h"
#include "player.h"
#include "graph.h"
#include "room.h"
#include <stdlib.h>
#include <string.h>

/* --- Private Helpers --- */
static Status get_initial_player_pos(const Room *first_room, int *x_out, int *y_out) {
    if (!first_room) return INTERNAL_ERROR;
    return room_get_start_position(first_room, x_out, y_out);
}

static void calc_next_pos(int old_x, int old_y, Direction dir, int *next_x, int *next_y) {
    *next_x = old_x;
    *next_y = old_y;
    if (dir == DIR_NORTH) (*next_y)--;
    if (dir == DIR_SOUTH) (*next_y)++;
    if (dir == DIR_EAST) (*next_x)++;
    if (dir == DIR_WEST) (*next_x)--;
}

static Status handle_pushable(GameEngine *eng, Room *room, int pushable_id,
                              Direction dir, int next_x, int next_y) {

    Status push_status = room_try_push(room, pushable_id, dir);

    /* If the room says it's impassable, the move is blocked */
    if (push_status == ROOM_IMPASSABLE) {
        return ROOM_IMPASSABLE;
    }

    /* Any other non-OK status should propagate as an error */
    if (push_status != OK) {
        return push_status;
    }

    /* Push succeeded → move the player into the old pushable position */
    if (player_set_position(eng->player, next_x, next_y) != OK) {
        return INTERNAL_ERROR;
    }

    return OK;
}

static Status handle_treasure(GameEngine *eng, Room *room, int treasure_id) {
    if (treasure_id >= 0) {
        Treasure *picked_up = NULL;
        if (room_pick_up_treasure(room, treasure_id, &picked_up) == OK && picked_up != NULL) {
            player_try_collect(eng->player, picked_up);
        }
    }
    return OK;
}

static Status handle_portal(GameEngine *eng, int target_room_id) {
    if (target_room_id == -1) return ROOM_NO_PORTAL;
    
    Room target_key = { .id = target_room_id };
    if (!graph_get_payload(eng->graph, &target_key)) return GE_NO_SUCH_ROOM;
    
    if (player_move_to_room(eng->player, target_room_id) != OK) return INTERNAL_ERROR;
    return OK;
}

/* --- Public API --- */

Status game_engine_create(const char *config_file_path, GameEngine **engine_out) {
    if (!config_file_path || !engine_out) return INVALID_ARGUMENT;
    *engine_out = NULL;

    GameEngine *eng = calloc(1, sizeof(GameEngine));
    if (!eng) return NO_MEMORY;

    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;

    Status st = loader_load_world(config_file_path, &graph, &first_room, &num_rooms, &charset);
    if (st != OK) {
        free(eng);
        return st;
    }

    int init_x = 0;
    int init_y = 0;
    st = get_initial_player_pos(first_room, &init_x, &init_y);
    if (st != OK) {
        graph_destroy(graph);
        free(eng);
        return INTERNAL_ERROR;
    }
    
    st = player_create(first_room->id, init_x, init_y, &eng->player);
    if (st != OK) {
        graph_destroy(graph);
        free(eng);
        return st;
    }

    eng->graph = graph;
    eng->charset = charset;
    eng->room_count = num_rooms;
    eng->initial_room_id = first_room->id;
    eng->initial_player_x = init_x;
    eng->initial_player_y = init_y;

    *engine_out = eng;
    return OK;
}

void game_engine_destroy(GameEngine *eng) {
    if (!eng) return;
    player_destroy(eng->player);
    graph_destroy(eng->graph);
    free(eng);
}

const Player *game_engine_get_player(const GameEngine *eng) {
    if (!eng) return NULL;
    return eng->player;
}

Status game_engine_move_player(GameEngine *eng, Direction dir) {
    if (!eng || !eng->player || !eng->graph) return INVALID_ARGUMENT;
    if (dir < 0 || dir > DIR_WEST) return INVALID_ARGUMENT;

    int old_x = 0;
    int old_y = 0; 
    if (player_get_position(eng->player, &old_x, &old_y) != OK) return INTERNAL_ERROR;

    int next_x = 0;
    int next_y = 0;
    calc_next_pos(old_x, old_y, dir, &next_x, &next_y);

    int current_room_id = player_get_room(eng->player);
    Room search_key = { .id = current_room_id };
    Room *current_room = (Room *)graph_get_payload(eng->graph, &search_key);
    if (!current_room) return GE_NO_SUCH_ROOM;

    int out_id = -1;
    RoomTileType tile_type = room_classify_tile(current_room, next_x, next_y, &out_id);

    if (tile_type == ROOM_TILE_INVALID || tile_type == ROOM_TILE_WALL) {
        return ROOM_IMPASSABLE;
    } 

    if (tile_type == ROOM_TILE_PUSHABLE) {
        return handle_pushable(eng, current_room, out_id, dir, next_x, next_y);
    }
    
    if (tile_type == ROOM_TILE_TREASURE) {
        /* Move player onto the treasure tile first (so this is logged as a MOVE),
        then pick up the treasure. Treat failures as internal errors. */
        if (player_set_position(eng->player, next_x, next_y) != OK) {
            return INTERNAL_ERROR;
        }
        if (handle_treasure(eng, current_room, out_id) != OK) {
            return INTERNAL_ERROR;
        }
        return OK;
    }
        
    if (tile_type == ROOM_TILE_FLOOR) {
        if (player_set_position(eng->player, next_x, next_y) != OK) return INTERNAL_ERROR;
        return OK;
    } 
    
    if (tile_type == ROOM_TILE_PORTAL) {
        return handle_portal(eng, out_id);
    }
    
    return INTERNAL_ERROR;
}

Status game_engine_get_room_count(const GameEngine *eng, int *count_out) {
    if (!eng) return INVALID_ARGUMENT;
    if (!count_out) return NULL_POINTER;
    
    *count_out = eng->room_count;
    if (*count_out == 0 && eng->graph != NULL) {
        *count_out = graph_size(eng->graph);
    }
    return OK;
}

Status game_engine_get_room_dimensions(const GameEngine *eng, int *width_out, int *height_out) {
    if (!eng) return INVALID_ARGUMENT;
    if (!width_out || !height_out) return NULL_POINTER;
    
    *width_out = 0;
    *height_out = 0;
    
    if (!eng->player || !eng->graph) return INTERNAL_ERROR;
    
    int current_room_id = player_get_room(eng->player);
    Room search_key = { .id = current_room_id };
    const Room *current_room = graph_get_payload(eng->graph, &search_key);
    if (!current_room) return GE_NO_SUCH_ROOM;
    
    *width_out = room_get_width(current_room);
    *height_out = room_get_height(current_room);
    if (*width_out <= 0 || *height_out <= 0) return INTERNAL_ERROR;
    
    return OK;
}

Status game_engine_reset(GameEngine *eng) {
    if (!eng) return INVALID_ARGUMENT;
    if (!eng->player) return INTERNAL_ERROR;
    return player_reset_to_start(eng->player, eng->initial_room_id, eng->initial_player_x, eng->initial_player_y);
}

Status game_engine_render_current_room(const GameEngine *eng, char **str_out) {
    if (!eng) return INVALID_ARGUMENT;
    if (!str_out) return NULL_POINTER;
    *str_out = NULL;

    if (!eng->player) return INTERNAL_ERROR;

    int current_room_id = player_get_room(eng->player);
    Status st = game_engine_render_room(eng, current_room_id, str_out);
    if (st != OK) return st;

    int width = 0;
    int height = 0;
    game_engine_get_room_dimensions(eng, &width, &height);

    int player_x = 0; 
    int player_y = 0;
    player_get_position(eng->player, &player_x, &player_y);
    
    if (player_x < 0 || player_y < 0 || player_x >= width || player_y >= height) {
        free(*str_out);
        *str_out = NULL;
        return INTERNAL_ERROR;
    }
    
    size_t char_index = (size_t)player_y * ((size_t)width + 1) + (size_t)player_x;
    
    if (char_index < strlen(*str_out)) {
        (*str_out)[char_index] = eng->charset.player;
    } else {
        free(*str_out);
        *str_out = NULL;
        return INTERNAL_ERROR;
    }

    return OK;
}

Status game_engine_render_room(const GameEngine *eng, int room_id, char **str_out) {
    if (!eng) return INVALID_ARGUMENT;
    if (!str_out) return NULL_POINTER;
    *str_out = NULL;
    if (!eng->graph) return INTERNAL_ERROR;

    Room search_key = { .id = room_id };
    const Room *room = graph_get_payload(eng->graph, &search_key);
    if (!room) return GE_NO_SUCH_ROOM;

    int width = room_get_width(room);
    int height = room_get_height(room);
    char *raw_buffer = malloc((size_t)width * (size_t)height);
    if (!raw_buffer) return NO_MEMORY;

    Status st = room_render(room, &eng->charset, raw_buffer, width, height);
    if (st != OK) {
        free(raw_buffer);
        return INTERNAL_ERROR;
    }

    size_t final_size = ((size_t)width * (size_t)height) + (size_t)height + 1;
    char *final_str = malloc(final_size);
    if (!final_str) {
        free(raw_buffer);
        return NO_MEMORY;
    }

    char *current_pos = final_str;
    for (int y = 0; y < height; y++) {
        memcpy(current_pos, raw_buffer + ((size_t)y * (size_t)width), (size_t)width);
        current_pos += width;
        *current_pos = '\n';
        current_pos++;
    }
    *current_pos = '\0';

    free(raw_buffer);
    *str_out = final_str;
    return OK;
}

Status game_engine_get_room_ids(const GameEngine *eng, int **ids_out, int *count_out) {
    if (!eng) return INVALID_ARGUMENT;
    if (!ids_out || !count_out) return NULL_POINTER;
    *ids_out = NULL;
    *count_out = 0;

    if (!eng->graph) return INTERNAL_ERROR;

    const void * const *payloads = NULL;
    int payload_count = 0;
    
    if (graph_get_all_payloads(eng->graph, &payloads, &payload_count) != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }

    if (payload_count == 0) return OK;
    
    int *ids = malloc(sizeof(int) * payload_count);
    if (!ids) return NO_MEMORY;

    for (int i = 0; i < payload_count; i++) {
        const Room *room = (const Room *)payloads[i];
        if (!room) {
            free(ids);
            return INTERNAL_ERROR;
        }
        ids[i] = room->id;
    }

    *ids_out = ids;
    *count_out = payload_count;
    return OK;
}

void game_engine_free_string(void *ptr) { 
    if (ptr) free(ptr); 
}

Status game_engine_get_room_by_id(const GameEngine *eng, int room_id, const Room **room_out) {
    if (!eng) return INVALID_ARGUMENT;
    if (!room_out) return NULL_POINTER;
    *room_out = NULL;

    if (!eng->graph) return INTERNAL_ERROR;

    Room search_key = { .id = room_id };
    const Room *room = graph_get_payload(eng->graph, &search_key);
    
    if (!room) return GE_NO_SUCH_ROOM;

    *room_out = room;
    return OK;
}
