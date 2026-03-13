#include <check.h>
#include <stdlib.h>
#include <stdbool.h>

#include "world_loader.h"
#include "graph.h"
#include "room.h"
#include "datagen.h"
#include "types.h"

/* ============================================================
 * Test Implementations
 * ============================================================ */

/* A. Argument and config validation */

START_TEST(test_loader_load_world_invalid_arguments)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = -1;
    Charset charset;

    // Test each output pointer being NULL
    ck_assert_int_eq(loader_load_world(NULL, &graph, &first_room, &num_rooms, &charset), INVALID_ARGUMENT);
    ck_assert_int_eq(loader_load_world("../assets/starter.ini", NULL, &first_room, &num_rooms, &charset), INVALID_ARGUMENT);
    ck_assert_int_eq(loader_load_world("../assets/starter.ini", &graph, NULL, &num_rooms, &charset), INVALID_ARGUMENT);
    ck_assert_int_eq(loader_load_world("../assets/starter.ini", &graph, &first_room, NULL, &charset), INVALID_ARGUMENT);
    ck_assert_int_eq(loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, NULL), INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_loader_load_world_bad_config)
{
    const char *bad_path = "../assets/does_not_exist.ini";
    Graph *graph = (Graph *)0x1;
    Room *first_room = (Room *)0x1;
    int num_rooms = 123;
    Charset charset;
    memset(&charset, 0xAA, sizeof(Charset));

    Status status = loader_load_world(bad_path, &graph, &first_room, &num_rooms, &charset);

    ck_assert_int_eq(status, WL_ERR_CONFIG);
    ck_assert_ptr_eq(graph, NULL);
    ck_assert_ptr_eq(first_room, NULL);
    ck_assert_int_eq(num_rooms, 0);
    ck_assert_int_eq(charset.wall, '\0');
}
END_TEST

/* B. Basic success contract (starter.ini) */

START_TEST(test_loader_load_world_success_basic)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = -1;
    Charset charset;

    Status status = loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &charset);

    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(graph);
    ck_assert_ptr_nonnull(first_room);
    ck_assert_int_eq(num_rooms, 3);
    ck_assert_int_eq(graph_size(graph), 3);
    ck_assert(graph_contains(graph, first_room));

    graph_destroy(graph);
}
END_TEST

START_TEST(test_loader_load_world_rooms_have_valid_dimensions)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = -1;
    Charset charset;
    Status status = loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &charset);
    ck_assert_int_eq(status, OK);

    const void * const *payloads = NULL;
    int payload_count = 0;
    graph_get_all_payloads(graph, &payloads, &payload_count);
    
    for (int i = 0; i < payload_count; i++) {
        const Room *room = (const Room *)payloads[i];
        ck_assert_ptr_nonnull(room);
        ck_assert_int_gt(room_get_width(room), 0);
        ck_assert_int_gt(room_get_height(room), 0);
    }
    graph_destroy(graph);
}
END_TEST

/* C. Charset handling */

START_TEST(test_loader_load_world_charset_matches_datagen)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = -1;
    Charset loaded_charset;

    Status status = loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &loaded_charset);
    ck_assert_int_eq(status, OK);
    graph_destroy(graph);

    int dg_status = start_datagen("../assets/starter.ini");
    ck_assert_int_eq(dg_status, DG_OK);
    const DG_Charset *dg_cs = dg_get_charset();
    ck_assert_ptr_nonnull(dg_cs);

    ck_assert_int_eq(loaded_charset.wall, dg_cs->wall);
    ck_assert_int_eq(loaded_charset.floor, dg_cs->floor);
    ck_assert_int_eq(loaded_charset.player, dg_cs->player);
    ck_assert_int_eq(loaded_charset.pushable, dg_cs->pushable);
    ck_assert_int_eq(loaded_charset.treasure, dg_cs->treasure);
    ck_assert_int_eq(loaded_charset.portal, dg_cs->portal);
    
    stop_datagen();
}
END_TEST

/* D. Graph connectivity vs portals */

START_TEST(test_loader_load_world_graph_matches_room_portals)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    Status status = loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &charset);
    ck_assert_int_eq(status, OK);

    const void * const *payloads = NULL;
    int payload_count = 0;
    graph_get_all_payloads(graph, &payloads, &payload_count);

    for (int i = 0; i < payload_count; i++) {
        const Room *src_room = (const Room *)payloads[i];
        for (int p = 0; p < src_room->portal_count; p++) {
            int target_id = src_room->portals[p].target_room_id;
            if (target_id < 0) continue;

            Room search_key = {.id = target_id};
            const Room *dst_room = graph_get_payload(graph, &search_key);
            
            ck_assert_msg(dst_room != NULL, "Portal destination room ID %d not found in graph", target_id);
            ck_assert_msg(graph_has_edge(graph, src_room, dst_room), "Graph edge missing for portal from room %d to %d", src_room->id, target_id);
        }
    }
    graph_destroy(graph);
}
END_TEST

START_TEST(test_loader_load_world_graph_is_connected)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    Status status = loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &charset);
    ck_assert_int_eq(status, OK);
    
    ck_assert_msg(graph_is_connected(graph), "World graph from starter.ini should be connected");
    
    graph_destroy(graph);
}
END_TEST

/* E. Room content consistency and ownership */

START_TEST(test_loader_load_world_room_arrays_match_counts)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &charset);

    const void * const *payloads = NULL;
    int count = 0;
    graph_get_all_payloads(graph, &payloads, &count);

    for (int i = 0; i < count; i++) {
        const Room *r = (const Room *)payloads[i];
        if (r->portal_count > 0) ck_assert_ptr_nonnull(r->portals);
        else ck_assert_ptr_null(r->portals);
        
        if (r->treasure_count > 0) ck_assert_ptr_nonnull(r->treasures);
        else ck_assert_ptr_null(r->treasures);
    }
    graph_destroy(graph);
}
END_TEST

START_TEST(test_loader_load_world_floorgrid_safe_to_query)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &charset);

    const void * const *payloads = NULL;
    int count = 0;
    graph_get_all_payloads(graph, &payloads, &count);

    for (int i = 0; i < count; i++) {
        const Room *r = (const Room *)payloads[i];
        if (r->floor_grid) {
            room_is_walkable(r, 0, 0);
            room_is_walkable(r, room_get_width(r) - 1, room_get_height(r) - 1);
        }
    }
    graph_destroy(graph);
}
END_TEST

START_TEST(test_loader_load_world_first_room_is_in_graph)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &charset);
    
    ck_assert_ptr_nonnull(first_room);
    ck_assert(graph_contains(graph, first_room));
    
    graph_destroy(graph);
}
END_TEST

/* F. Memory / lifecycle sanity */

START_TEST(test_loader_load_world_multiple_calls_independent)
{
    Graph *g1 = NULL, *g2 = NULL;
    Room *r1 = NULL, *r2 = NULL;
    int n1 = 0, n2 = 0;
    Charset cs1, cs2;

    Status st1 = loader_load_world("../assets/starter.ini", &g1, &r1, &n1, &cs1);
    ck_assert_int_eq(st1, OK);
    ck_assert_int_eq(n1, 3);
    
    Status st2 = loader_load_world("../assets/starter.ini", &g2, &r2, &n2, &cs2);
    ck_assert_int_eq(st2, OK);
    ck_assert_int_eq(n2, 3);

    graph_destroy(g1);
    graph_destroy(g2);
}
END_TEST

START_TEST(test_loader_load_world_graph_destroy_frees_rooms)
{
    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;
    loader_load_world("../assets/starter.ini", &graph, &first_room, &num_rooms, &charset);
    graph_destroy(graph);
}
END_TEST

/* ============================================================
 * Suite
 * ============================================================ */

Suite *world_loader_suite(void)
{
    Suite *s = suite_create("world_loader");
    TCase *tc_core = tcase_create("core");

    tcase_add_test(tc_core, test_loader_load_world_invalid_arguments);
    tcase_add_test(tc_core, test_loader_load_world_bad_config);
    tcase_add_test(tc_core, test_loader_load_world_success_basic);
    tcase_add_test(tc_core, test_loader_load_world_rooms_have_valid_dimensions);
    tcase_add_test(tc_core, test_loader_load_world_charset_matches_datagen);
    tcase_add_test(tc_core, test_loader_load_world_graph_matches_room_portals);
    tcase_add_test(tc_core, test_loader_load_world_graph_is_connected);
    tcase_add_test(tc_core, test_loader_load_world_room_arrays_match_counts);
    tcase_add_test(tc_core, test_loader_load_world_floorgrid_safe_to_query);
    tcase_add_test(tc_core, test_loader_load_world_first_room_is_in_graph);
    tcase_add_test(tc_core, test_loader_load_world_multiple_calls_independent);
    tcase_add_test(tc_core, test_loader_load_world_graph_destroy_frees_rooms);

    suite_add_tcase(s, tc_core);
    return s;
}