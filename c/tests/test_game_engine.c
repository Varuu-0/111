#include <check.h>
#include <stdlib.h>
#include "game_engine.h"
#include "player.h"

// --- Test Fixture ---
// This setup creates a fresh game engine for every single test.
static GameEngine *eng = NULL;

static void setup_engine(void) {
    // Use the known-good config file. All tests depend on this loading correctly.
    Status st = game_engine_create("../assets/starter.ini", &eng);
    ck_assert_int_eq(st, OK);
    ck_assert_ptr_nonnull(eng);
}

static void teardown_engine(void) {
    game_engine_destroy(eng);
    eng = NULL;
}

// --- Test Cases ---

// 1. Test Creation and Destruction
START_TEST(test_engine_create_success) {
    // The setup function already did the main work.
    // This test just confirms the fixture itself is working.
    ck_assert_ptr_nonnull(eng);
    ck_assert_ptr_nonnull(game_engine_get_player(eng));
    
    int count = 0;
    game_engine_get_room_count(eng, &count);
    ck_assert_int_gt(count, 0); 
}
END_TEST

START_TEST(test_engine_create_bad_config) {
    // Teardown the fixture's engine first since we are testing create itself.
    teardown_engine(); 
    
    Status st = game_engine_create("assets/non_existent_file.ini", &eng);
    ck_assert_int_eq(st, WL_ERR_CONFIG);
    ck_assert_ptr_null(eng);
}
END_TEST

// 2. Test Getters
START_TEST(test_engine_get_room_dimensions_valid) {
    int w = 0, h = 0;
    Status st = game_engine_get_room_dimensions(eng, &w, &h);
    ck_assert_int_eq(st, OK);
    ck_assert_int_gt(w, 0); // Dimensions should be positive
    ck_assert_int_gt(h, 0);
}
END_TEST

// 3. Test Movement Logic
START_TEST(test_engine_move_player_floor) {
    const Player *p = game_engine_get_player(eng);
    int start_x, start_y;
    player_get_position(p, &start_x, &start_y);

    // Assuming the starting position isn't against a wall, one of these should work.
    // This is a basic test. A better test would load a known room layout.
    if (game_engine_move_player(eng, DIR_SOUTH) == OK) {
        int new_x, new_y;
        player_get_position(p, &new_x, &new_y);
        ck_assert_int_eq(new_x, start_x);
        ck_assert_int_eq(new_y, start_y + 1);
    }
}
END_TEST

START_TEST(test_engine_move_player_wall) {
    const Player *p = game_engine_get_player(eng);
    int start_x, start_y;
    player_get_position(p, &start_x, &start_y);

    // Try to move into a known wall (assuming implicit walls at x=0)
    Status st = game_engine_move_player(eng, DIR_WEST);
    while (st == OK) { // Move player to the left edge
        st = game_engine_move_player(eng, DIR_WEST);
    }

    // Now, we should be at a wall.
    int final_x, final_y;
    player_get_position(p, &final_x, &final_y);
    st = game_engine_move_player(eng, DIR_WEST); // Try to move again
    ck_assert_int_eq(st, ROOM_IMPASSABLE);

    int after_fail_x, after_fail_y;
    player_get_position(p, &after_fail_x, &after_fail_y);
    ck_assert_int_eq(final_x, after_fail_x); // Position should not change
}
END_TEST


// 4. Test Rendering
START_TEST(test_engine_render_current_room_contains_player) {
    char *rendered_string = NULL;
    Status st = game_engine_render_current_room(eng, &rendered_string);
    ck_assert_int_eq(st, OK);
    ck_assert_ptr_nonnull(rendered_string);
    
    // Check if the player character '@' is in the string.
    ck_assert_ptr_nonnull(strchr(rendered_string, '@'));

    free(rendered_string);
}
END_TEST

// 5. Test Reset
START_TEST(test_engine_reset) {
    const Player *p = game_engine_get_player(eng);
    int start_x, start_y;
    player_get_position(p, &start_x, &start_y);
    int start_room = player_get_room(p);

    // Move the player
    game_engine_move_player(eng, DIR_SOUTH);
    
    // Reset the engine
    Status st = game_engine_reset(eng);
    ck_assert_int_eq(st, OK);

    int final_x, final_y;
    player_get_position(p, &final_x, &final_y);
    int final_room = player_get_room(p);

    // Check if player is back at the start
    ck_assert_int_eq(final_x, start_x);
    ck_assert_int_eq(final_y, start_y);
    ck_assert_int_eq(final_room, start_room);
}
END_TEST

// 6. Test Error Cases for Coverage
START_TEST(test_engine_move_invalid_params) {
    // Test NULL engine
    ck_assert_int_eq(game_engine_move_player(NULL, DIR_NORTH), INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_engine_get_room_ids_invalid) {
    int *ids = NULL;
    int count = 0;
    // Test NULL params
    ck_assert_int_eq(game_engine_get_room_ids(NULL, &ids, &count), INVALID_ARGUMENT);
    ck_assert_int_eq(game_engine_get_room_ids(eng, NULL, &count), NULL_POINTER);
    ck_assert_int_eq(game_engine_get_room_ids(eng, &ids, NULL), NULL_POINTER);
}
END_TEST

START_TEST(test_engine_render_invalid_id) {
    char *str = NULL;
    // Test a room ID that definitely doesn't exist
    ck_assert_int_eq(game_engine_render_room(eng, 9999, &str), GE_NO_SUCH_ROOM);
}
END_TEST

START_TEST(test_engine_move_all_directions) {
    // Exercise the switch statement branches for coverage
    game_engine_move_player(eng, DIR_NORTH);
    game_engine_move_player(eng, DIR_SOUTH);
    game_engine_move_player(eng, DIR_EAST);
    game_engine_move_player(eng, DIR_WEST);
    // We don't necessarily care if they succeed, just that the code paths are run
}
END_TEST

// 7. NULL Pointer Validation Tests (no fixture needed)
START_TEST(test_move_player_null_engine) {
    Status result = game_engine_move_player(NULL, DIR_NORTH);
    ck_assert_int_eq(result, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_render_null_engine) {
    char *str_out = NULL;
    Status result = game_engine_render_current_room(NULL, &str_out);
    ck_assert_int_eq(result, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_render_null_output) {
    // Create a local engine for this test since we need a valid engine
    // but this test shouldn't rely on the fixture
    GameEngine *local_eng = NULL;
    Status create_st = game_engine_create("../assets/starter.ini", &local_eng);
    ck_assert_int_eq(create_st, OK);
    
    Status result = game_engine_render_current_room(local_eng, NULL);
    ck_assert_int_eq(result, NULL_POINTER);
    
    game_engine_destroy(local_eng);
}
END_TEST

/* Test 8: Call game_engine_move_player with an invalid direction (e.g., -1 or 999).
 * Assert it returns INVALID_ARGUMENT. */
START_TEST(test_engine_move_player_invalid_direction) {
    // Create a local engine for this test
    GameEngine *local_eng = NULL;
    Status create_st = game_engine_create("../assets/starter.ini", &local_eng);
    ck_assert_int_eq(create_st, OK);

    /* Test with invalid direction -1 */
    Status result = game_engine_move_player(local_eng, (Direction)-1);
    ck_assert_int_eq(result, INVALID_ARGUMENT);

    /* Test with invalid direction 999 */
    result = game_engine_move_player(local_eng, (Direction)999);
    ck_assert_int_eq(result, INVALID_ARGUMENT);

    game_engine_destroy(local_eng);
}
END_TEST


// --- Suite Definition ---
Suite *game_engine_suite(void) {
    Suite *s = suite_create("game_engine");
    TCase *tc_core = tcase_create("core");

    // Add fixture to run setup_engine/teardown_engine around each test
    tcase_add_checked_fixture(tc_core, setup_engine, teardown_engine);

    // Add tests to the test case
    tcase_add_test(tc_core, test_engine_create_success);
    tcase_add_test(tc_core, test_engine_create_bad_config);
    tcase_add_test(tc_core, test_engine_get_room_dimensions_valid);
    tcase_add_test(tc_core, test_engine_move_player_floor);
    tcase_add_test(tc_core, test_engine_move_player_wall);
    tcase_add_test(tc_core, test_engine_render_current_room_contains_player);
    tcase_add_test(tc_core, test_engine_reset);
    tcase_add_test(tc_core, test_engine_move_invalid_params);
    tcase_add_test(tc_core, test_engine_get_room_ids_invalid);
    tcase_add_test(tc_core, test_engine_render_invalid_id);
    tcase_add_test(tc_core, test_engine_move_all_directions);

    suite_add_tcase(s, tc_core);

    // Add NULL pointer validation tests (no fixture needed)
    TCase *tc_null = tcase_create("null_validation");
    tcase_add_test(tc_null, test_move_player_null_engine);
    tcase_add_test(tc_null, test_render_null_engine);
    tcase_add_test(tc_null, test_render_null_output);
    tcase_add_test(tc_null, test_engine_move_player_invalid_direction);
    suite_add_tcase(s, tc_null);

    return s;
}
