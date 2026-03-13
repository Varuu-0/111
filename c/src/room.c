#include <stdlib.h>
#include <string.h>

#include "room.h"


Room *room_create(int id, const char *name, int width, int height)
{
    Room *r = malloc(sizeof *r);
    if (r == NULL) {
        return NULL;
    }

    /* clamp dimensions to at least 1 */
    if (width < 1) {
        width = 1;
    }
    if (height < 1) {
        height = 1;
    }

    r->id = id;
    r->width = width;
    r->height = height;

    if (name != NULL) {
        size_t len = strlen(name);
        r->name = malloc(len + 1);
        if (r->name == NULL) {
            free(r);
            return NULL;
        }
        memcpy(r->name, name, len + 1);
    } else {
        r->name = NULL;
    }

    r->floor_grid = NULL;
    r->portals = NULL;
    r->portal_count = 0;
    r->treasures = NULL;
    r->treasure_count = 0;
    r->pushables = NULL;
    r->pushable_count = 0;

    return r;
}

void room_destroy(Room *r) {
    if (r == NULL) return;

    free(r->floor_grid);

    if (r->portals != NULL) {
        for (int i = 0; i < r->portal_count; i++) free(r->portals[i].name);
        free(r->portals);
    }

    if (r->treasures != NULL) {
        for (int i = 0; i < r->treasure_count; i++) free(r->treasures[i].name);
        free(r->treasures);
    }

    if (r->pushables != NULL) {
        for (int i = 0; i < r->pushable_count; i++) free(r->pushables[i].name);
        free(r->pushables);
    }

    free(r->name);
    free(r);
}

int room_get_width(const Room *r) {
    if (r == NULL) return 0;
    return r->width;
}

int room_get_height(const Room *r) {
    if (r == NULL) return 0;
    return r->height;
}

Status room_set_floor_grid(Room *r, bool *floor_grid) {
    if (r == NULL) return INVALID_ARGUMENT;
    free(r->floor_grid);
    r->floor_grid = floor_grid;
    return OK;
}

Status room_set_portals(Room *r, Portal *portals, int portal_count) {
    if (r == NULL) return INVALID_ARGUMENT;
    if (portal_count > 0 && portals == NULL) return INVALID_ARGUMENT;

    if (r->portals != NULL) {
        for (int i = 0; i < r->portal_count; i++) free(r->portals[i].name);
        free(r->portals);
    }
    r->portals = portals;
    r->portal_count = portal_count;
    return OK;
}

Status room_set_treasures(Room *r, Treasure *treasures, int treasure_count) {
    if (r == NULL) return INVALID_ARGUMENT;
    if (treasure_count > 0 && treasures == NULL) return INVALID_ARGUMENT;

    if (r->treasures != NULL) {
        for (int i = 0; i < r->treasure_count; i++) free(r->treasures[i].name);
        free(r->treasures);
    }
    r->treasures = treasures;
    r->treasure_count = treasure_count;
    return OK;
}

Status room_place_treasure(Room *r, const Treasure *treasure) {
    if (r == NULL || treasure == NULL) return INVALID_ARGUMENT;

    int old_count = r->treasure_count;
    int new_count = old_count + 1;

    Treasure *new_array = malloc(sizeof(*new_array) * new_count);
    if (new_array == NULL) return NO_MEMORY;

    for (int i = 0; i < old_count; i++) new_array[i] = r->treasures[i];

    new_array[old_count] = *treasure;
    if (treasure->name != NULL) {
        size_t len = strlen(treasure->name);
        new_array[old_count].name = malloc(len + 1);
        if (new_array[old_count].name == NULL) {
            free(new_array);
            return NO_MEMORY;
        }
        memcpy(new_array[old_count].name, treasure->name, len + 1);
    }

    free(r->treasures);
    r->treasures = new_array;
    r->treasure_count = new_count;

    return OK;
}

int room_get_treasure_at(const Room *r, int x, int y) {
    if (r == NULL) return -1;
    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].x == x && r->treasures[i].y == y && !r->treasures[i].collected) {
            return r->treasures[i].id;
        }
    }
    return -1;
}

int room_get_portal_destination(const Room *r, int x, int y) {
    if (r == NULL) return -1;
    for (int i = 0; i < r->portal_count; i++) {
        if (r->portals[i].x == x && r->portals[i].y == y) {
            return r->portals[i].target_room_id;
        }
    }
    return -1;
}

bool room_is_walkable(const Room *r, int x, int y) {
    if (r == NULL) return false;
    if (x < 0 || y < 0 || x >= r->width || y >= r->height) return false;

    int dummy_idx = -1;
    if (room_has_pushable_at(r, x, y, &dummy_idx)) return false; 

    if (r->floor_grid != NULL) return r->floor_grid[y * r->width + x];

    if (x == 0 || y == 0 || x == r->width - 1 || y == r->height - 1) return false;

    return true;
}

RoomTileType room_classify_tile(const Room *r, int x, int y, int *out_id) {
    if (r == NULL || x < 0 || y < 0 || x >= r->width || y >= r->height) return ROOM_TILE_INVALID;

    int treasure_id = room_get_treasure_at(r, x, y);
    if (treasure_id != -1) {
        if (out_id != NULL) *out_id = treasure_id;
        return ROOM_TILE_TREASURE;
    }

    int dest_room_id = room_get_portal_destination(r, x, y);
    if (dest_room_id != -1) {
        if (out_id != NULL) *out_id = dest_room_id;
        return ROOM_TILE_PORTAL;
    }

    int pushable_idx = -1;
    if (room_has_pushable_at(r, x, y, &pushable_idx)) {
        if (out_id != NULL) *out_id = pushable_idx;
        return ROOM_TILE_PUSHABLE;
    }

    if (room_is_walkable(r, x, y)) return ROOM_TILE_FLOOR;

    return ROOM_TILE_WALL;
}

/* Helper functions for rendering to reduce cognitive complexity */
static void render_base_grid(const Room *r, const Charset *charset, char *buffer, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            if (r->floor_grid != NULL) {
                buffer[index] = (char)(r->floor_grid[index] ? charset->floor : charset->wall);
            } else {
                bool is_perimeter = (x == 0 || y == 0 || x == width - 1 || y == height - 1);
                buffer[index] = (char)(is_perimeter ? charset->wall : charset->floor);
            }
        }
    }
}

static void render_treasures(const Room *r, const Charset *charset, char *buffer, int width, int height) {
    if (r->treasures == NULL) return;
    for (int i = 0; i < r->treasure_count; i++) {
        const Treasure *t = &r->treasures[i];
        if (t->collected) continue;
        if (t->x >= 0 && t->y >= 0 && t->x < width && t->y < height) {
            buffer[t->y * width + t->x] = charset->treasure;
        }
    }
}

static void render_portals(const Room *r, const Charset *charset, char *buffer, int width, int height) {
    if (r->portals == NULL) return;
    for (int i = 0; i < r->portal_count; i++) {
        const Portal *p = &r->portals[i];
        if (p->x >= 0 && p->y >= 0 && p->x < width && p->y < height) {
            buffer[p->y * width + p->x] = charset->portal;
        }
    }
}

static void render_pushables(const Room *r, const Charset *charset, char *buffer, int width, int height) {
    if (r->pushables == NULL) return;
    for (int i = 0; i < r->pushable_count; i++) {
        const Pushable *p = &r->pushables[i];
        if (p->x >= 0 && p->y >= 0 && p->x < width && p->y < height) {
            buffer[p->y * width + p->x] = charset->pushable;
        }
    }
}

Status room_render(const Room *r, const Charset *charset, char *buffer, int buffer_width, int buffer_height) {
    if (r == NULL || charset == NULL || buffer == NULL) return INVALID_ARGUMENT;
    if (buffer_width != r->width || buffer_height != r->height) return INVALID_ARGUMENT;

    render_base_grid(r, charset, buffer, r->width, r->height);
    render_treasures(r, charset, buffer, r->width, r->height);
    render_portals(r, charset, buffer, r->width, r->height);
    render_pushables(r, charset, buffer, r->width, r->height);

    return OK;
}

Status room_get_start_position(const Room *r, int *x_out, int *y_out) {
    if (r == NULL || x_out == NULL || y_out == NULL) return INVALID_ARGUMENT;

    if (r->portals != NULL && r->portal_count > 0) {
        *x_out = r->portals[0].x;
        *y_out = r->portals[0].y;
        return OK;
    }

    if (r->width > 2 && r->height > 2) {
        for (int y = 1; y < r->height - 1; y++) {
            for (int x = 1; x < r->width - 1; x++) {
                if (room_is_walkable(r, x, y)) {
                    *x_out = x;
                    *y_out = y;
                    return OK;
                }
            }
        }
    }
    return ROOM_NOT_FOUND;
}

int room_get_id(const Room *r) {
    if (r == NULL) return -1;
    return r->id;
}

Status room_pick_up_treasure(Room *r, int treasure_id, Treasure **treasure_out) {
    if (r == NULL) return INVALID_ARGUMENT;
    if (treasure_out == NULL) return NULL_POINTER;
    if (treasure_id < 0) return ROOM_NOT_FOUND;

    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].id == treasure_id) {
            if (r->treasures[i].collected) return ROOM_NOT_FOUND;

            r->treasures[i].collected = true;
            *treasure_out = &r->treasures[i];
            r->treasures[i].x = -1;
            r->treasures[i].y = -1;
            return OK;
        }
    }
    return ROOM_NOT_FOUND;
}

void destroy_treasure(Treasure *t) {
    if (t == NULL) return;
    free(t->name);
}

bool room_has_pushable_at(const Room *r, int x, int y, int *pushable_idx_out) {
    if (r == NULL) return false;
    for (int i = 0; i < r->pushable_count; i++) {
        if (r->pushables[i].x == x && r->pushables[i].y == y) {
            if (pushable_idx_out != NULL) *pushable_idx_out = i;
            return true;
        }
    }
    return false;
}

Status room_try_push(Room *r, int pushable_idx, Direction dir) {
    if (r == NULL) return INVALID_ARGUMENT;
    if (pushable_idx < 0 || pushable_idx >= r->pushable_count) return INVALID_ARGUMENT;

    int cur_x = r->pushables[pushable_idx].x;
    int cur_y = r->pushables[pushable_idx].y;

    int target_x = cur_x;
    int target_y = cur_y;

    switch (dir) {
        case DIR_NORTH: target_y--; break;
        case DIR_SOUTH: target_y++; break;
        case DIR_EAST:  target_x++; break;
        case DIR_WEST:  target_x--; break;
        default: return INVALID_ARGUMENT;
    }

    /* bounds check BEFORE classification */
    if (target_x < 0 || target_y < 0 ||
        target_x >= r->width || target_y >= r->height) {
        return ROOM_IMPASSABLE;
    }

    int next_id = -1;
    RoomTileType tile_type = room_classify_tile(r, target_x, target_y, &next_id);

    /* only allow push onto empty floor (not portals, treasures, other pushables) */
    if (tile_type != ROOM_TILE_FLOOR) {
        return ROOM_IMPASSABLE;
    }

    /* ensure no other pushable occupies the destination */
    int other_idx = -1;
    if (room_has_pushable_at(r, target_x, target_y, &other_idx)) return ROOM_IMPASSABLE;

    r->pushables[pushable_idx].x = target_x;
    r->pushables[pushable_idx].y = target_y;

    return OK;
}
