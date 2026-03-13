import ctypes
from enum import IntEnum

# --- Enums ---
# Python representations of status codes returned by the C backend
class Status(IntEnum):
    OK = 0
    INVALID_ARGUMENT = 1
    NULL_POINTER = 2
    NO_MEMORY = 3
    BOUNDS_EXCEEDED = 4
    INTERNAL_ERROR = 5
    ROOM_IMPASSABLE = 6
    ROOM_NO_PORTAL = 7
    ROOM_NOT_FOUND = 8
    GE_NO_SUCH_ROOM = 9
    WL_ERR_CONFIG = 10
    WL_ERR_DATAGEN = 11


# Directions used when calling the C movement functions
class Direction(IntEnum):
    NORTH = 0
    SOUTH = 1
    EAST = 2
    WEST = 3


# --- Structs ---
# Python mapping of the C Treasure struct using ctypes
class Treasure(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_int),
        ("name", ctypes.c_char_p),
        ("starting_room_id", ctypes.c_int),
        ("initial_x", ctypes.c_int),
        ("initial_y", ctypes.c_int),
        ("x", ctypes.c_int),
        ("y", ctypes.c_int),
        ("collected", ctypes.c_bool),
    ]


# Opaque pointers representing C structs whose internal layout we don't access in Python
GameEngine = ctypes.c_void_p
Player = ctypes.c_void_p
Room = ctypes.c_void_p


def setup_bindings(lib):
    # Configure ctypes argument and return types for all backend functions

    # Game Engine Lifecycle
    lib.game_engine_create.argtypes = [ctypes.c_char_p, ctypes.POINTER(GameEngine)]
    lib.game_engine_create.restype = ctypes.c_int  # returns Status

    lib.game_engine_destroy.argtypes = [GameEngine]
    lib.game_engine_destroy.restype = None

    # Game Engine Operations
    lib.game_engine_get_player.argtypes = [GameEngine]
    lib.game_engine_get_player.restype = Player

    lib.game_engine_move_player.argtypes = [GameEngine, ctypes.c_int]
    lib.game_engine_move_player.restype = ctypes.c_int

    lib.game_engine_render_current_room.argtypes = [GameEngine, ctypes.POINTER(ctypes.c_char_p)]
    lib.game_engine_render_current_room.restype = ctypes.c_int

    lib.game_engine_get_room_count.argtypes = [GameEngine, ctypes.POINTER(ctypes.c_int)]
    lib.game_engine_get_room_count.restype = ctypes.c_int

    lib.game_engine_get_room_dimensions.argtypes = [GameEngine, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
    lib.game_engine_get_room_dimensions.restype = ctypes.c_int

    lib.game_engine_get_room_ids.argtypes = [GameEngine, ctypes.POINTER(ctypes.POINTER(ctypes.c_int)), ctypes.POINTER(ctypes.c_int)]
    lib.game_engine_get_room_ids.restype = ctypes.c_int

    lib.game_engine_reset.argtypes = [GameEngine]
    lib.game_engine_reset.restype = ctypes.c_int

    # Player Operations
    lib.player_get_room.argtypes = [Player]
    lib.player_get_room.restype = ctypes.c_int

    lib.player_get_position.argtypes = [Player, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
    lib.player_get_position.restype = ctypes.c_int

    lib.player_get_collected_count.argtypes = [Player]
    lib.player_get_collected_count.restype = ctypes.c_int

    lib.player_has_collected_treasure.argtypes = [Player, ctypes.c_int]
    lib.player_has_collected_treasure.restype = ctypes.c_bool

    #returns a pointer to an array of pointers to Treasure structs
    lib.player_get_collected_treasures.argtypes = [Player, ctypes.POINTER(ctypes.c_int)]
    lib.player_get_collected_treasures.restype = ctypes.POINTER(ctypes.POINTER(Treasure))

    lib.player_reset_to_start.argtypes = [Player, ctypes.c_int, ctypes.c_int, ctypes.c_int]
    lib.player_reset_to_start.restype = ctypes.c_int

    # Memory Helpers (used when C allocates memory returned to Python)
    lib.game_engine_free_string.argtypes = [ctypes.c_void_p]
    lib.game_engine_free_string.restype = None

    lib.destroy_treasure.argtypes = [ctypes.POINTER(Treasure)]
    lib.destroy_treasure.restype = None