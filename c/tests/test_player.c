#include <check.h>
#include "player.h"

/* Optional: shared test fixture */
static Player *p = NULL;

static void setup_player(void) {
    /* Create a default player for most tests; handle errors as failures */
    Status status = player_create(0, 1, 2, &p);
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(p);
}

static void teardown_player(void) {
    player_destroy(p);
    p = NULL;
}

/* ---------- Test cases ---------- */
/* Core behavior tests */

START_TEST(test_player_create) {
    Player *p = NULL;

    Status status = player_create(5, 10, 20, &p);
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(p);

    /* Room ID is set correctly */
    ck_assert_int_eq(player_get_room(p), 5);

    /* Position is set correctly */
    int x = -1;
    int y = -1;
    status = player_get_position(p, &x, &y);
    ck_assert_int_eq(status, OK);
    ck_assert_int_eq(x, 10);
    ck_assert_int_eq(y, 20);

    player_destroy(p);
}
END_TEST


START_TEST(test_player_destroy) {
    Player *p = NULL;

    /* Create a player to destroy */
    Status status = player_create(1, 2, 3, &p);
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(p);

    /* Should not crash or leak in the normal destroy path */
    player_destroy(p);
    p = NULL; /* defensive: avoid accidental reuse */
}
END_TEST


START_TEST(test_player_get_room) {
    Player *p = NULL;

    Status status = player_create(42, 0, 0, &p);
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(p);

    int room_id = player_get_room(p);
    ck_assert_int_eq(room_id, 42);

    player_destroy(p);
}
END_TEST


START_TEST(test_player_get_position) {
    Player *p = NULL;

    /* Create with known coordinates */
    Status status = player_create(0, 3, 7, &p);
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(p);

    int x = -1;
    int y = -1;

    status = player_get_position(p, &x, &y);
    ck_assert_int_eq(status, OK);
    ck_assert_int_eq(x, 3);
    ck_assert_int_eq(y, 7);

    player_destroy(p);
}
END_TEST


START_TEST(test_player_set_position) {
    Player *p = NULL;

    Status status = player_create(0, 0, 0, &p);
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(p);

    status = player_set_position(p, 9, 11);
    ck_assert_int_eq(status, OK);

    int x = -1;
    int y = -1;
    status = player_get_position(p, &x, &y);
    ck_assert_int_eq(status, OK);
    ck_assert_int_eq(x, 9);
    ck_assert_int_eq(y, 11);

    player_destroy(p);
}
END_TEST


START_TEST(test_player_move_to_room) {
    Player *p = NULL;

    Status status = player_create(1, 4, 4, &p);
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(p);

    status = player_move_to_room(p, 7);
    ck_assert_int_eq(status, OK);

    /* Room ID updated */
    ck_assert_int_eq(player_get_room(p), 7);

    /* Position unchanged */
    int x = -1;
    int y = -1;
    status = player_get_position(p, &x, &y);
    ck_assert_int_eq(status, OK);
    ck_assert_int_eq(x, 4);
    ck_assert_int_eq(y, 4);

    player_destroy(p);
}
END_TEST


START_TEST(test_player_reset_to_start) {
    Player *p = NULL;

    /* Start from a non-default state */
    Status status = player_create(5, 9, 9, &p);
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(p);

    status = player_reset_to_start(p, 2, 1, 1);
    ck_assert_int_eq(status, OK);

    /* Room ID reset */
    ck_assert_int_eq(player_get_room(p), 2);

    /* Position reset */
    int x = -1;
    int y = -1;
    status = player_get_position(p, &x, &y);
    ck_assert_int_eq(status, OK);
    ck_assert_int_eq(x, 1);
    ck_assert_int_eq(y, 1);

    player_destroy(p);
}
END_TEST






// /* Error / edge case tests */

START_TEST(test_player_create_null_out) {
    Status status = player_create(0, 0, 0, NULL);
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_player_destroy_null) {
    /* Must be safe to call with NULL (no crash) */
    player_destroy(NULL);
}
END_TEST


START_TEST(test_player_get_room_null) {
    int room_id = player_get_room(NULL);
    ck_assert_int_eq(room_id, -1);
}
END_TEST


START_TEST(test_player_get_position_null_arg) {
    Player *p = NULL;
    Status status = player_create(0, 0, 0, &p);
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(p);

    int x = 0;
    int y = 0;

    /* p is NULL */
    status = player_get_position(NULL, &x, &y);
    ck_assert_int_eq(status, INVALID_ARGUMENT);

    /* x_out is NULL */
    status = player_get_position(p, NULL, &y);
    ck_assert_int_eq(status, INVALID_ARGUMENT);

    /* y_out is NULL */
    status = player_get_position(p, &x, NULL);
    ck_assert_int_eq(status, INVALID_ARGUMENT);

    player_destroy(p);
}
END_TEST


START_TEST(test_player_set_position_null) {
    Status status = player_set_position(NULL, 1, 2);
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_player_move_to_room_null) {
    Status status = player_move_to_room(NULL, 7);
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


START_TEST(test_player_reset_to_start_null) {
    Status status = player_reset_to_start(NULL, 0, 0, 0);
    ck_assert_int_eq(status, INVALID_ARGUMENT);
}
END_TEST


/* ---------- player_try_collect NULL pointer tests (autograder tests 1-3) ---------- */

START_TEST(test_player_try_collect_null_player) {
    /* Test 1: Call player_collect with a NULL player. Assert it returns NULL_POINTER. */
    Treasure t = {1, "gold", 0, 1, 1, 1, 1, false};
    Status status = player_try_collect(NULL, &t);
    ck_assert_int_eq(status, NULL_POINTER);
}
END_TEST


START_TEST(test_player_try_collect_null_treasure) {
    /* Test 2: Call player_collect with a NULL treasure. Assert it returns NULL_POINTER. */
    Player *p = NULL;
    Status status = player_create(0, 1, 2, &p);
    ck_assert_int_eq(status, OK);
    ck_assert_ptr_nonnull(p);

    status = player_try_collect(p, NULL);
    ck_assert_int_eq(status, NULL_POINTER);

    player_destroy(p);
}
END_TEST


START_TEST(test_player_try_collect_both_null) {
    /* Test 3: Call player_collect with BOTH NULL. Assert it returns NULL_POINTER. */
    Status status = player_try_collect(NULL, NULL);
    ck_assert_int_eq(status, NULL_POINTER);
}
END_TEST


/* ---------- Suite builder ---------- */

Suite *player_suite(void) {
    Suite *s = suite_create("player");
    TCase *tc_core = tcase_create("core");

    tcase_add_checked_fixture(tc_core, setup_player, teardown_player);

    /* Core behavior tests */
    tcase_add_test(tc_core, test_player_create);
    tcase_add_test(tc_core, test_player_destroy);
    tcase_add_test(tc_core, test_player_get_room);
    tcase_add_test(tc_core, test_player_get_position);
    tcase_add_test(tc_core, test_player_set_position);
    tcase_add_test(tc_core, test_player_move_to_room);
    tcase_add_test(tc_core, test_player_reset_to_start);

    /* Error / edge case tests */
    tcase_add_test(tc_core, test_player_create_null_out);
    tcase_add_test(tc_core, test_player_destroy_null);
    tcase_add_test(tc_core, test_player_get_room_null);
    tcase_add_test(tc_core, test_player_get_position_null_arg);
    tcase_add_test(tc_core, test_player_set_position_null);
    tcase_add_test(tc_core, test_player_move_to_room_null);
    tcase_add_test(tc_core, test_player_reset_to_start_null);

    /* player_try_collect NULL pointer tests (autograder tests 1-3) */
    tcase_add_test(tc_core, test_player_try_collect_null_player);
    tcase_add_test(tc_core, test_player_try_collect_null_treasure);
    tcase_add_test(tc_core, test_player_try_collect_both_null);

    suite_add_tcase(s, tc_core);
    return s;
}
