import ctypes
from typing import Tuple, List
from treasure_runner.bindings import Direction, GameEngine as CEnginePtr
from treasure_runner.models.player import Player, get_lib
from treasure_runner.models.exceptions import status_to_status_exception

class GameEngine:
    def __init__(self, config_path: str):
        self._lib = get_lib()      # load the backend C library
        self._eng = CEnginePtr()   # allocate pointer to the C GameEngine struct

        b_config = config_path.encode('utf-8')  # C functions expect a UTF-8 byte string
        status = self._lib.game_engine_create(b_config, ctypes.byref(self._eng))
        status_to_status_exception(status, f"Failed to create engine with config: {config_path}")

        # retrieve the Player pointer from the engine and wrap it in the Python Player class
        c_player = self._lib.game_engine_get_player(self._eng)
        if not c_player:
            raise RuntimeError("Engine created but Player is NULL")

        self._player = Player(c_player)

    def destroy(self) -> None:
        # explicitly destroy the underlying C engine instance
        if self._eng:
            self._lib.game_engine_destroy(self._eng)
            self._eng = None

    @property
    def player(self) -> Player:
        return self._player  # expose the wrapped Player object

    def move_player(self, direction: Direction) -> None:
        # call C engine logic to move the player
        status = self._lib.game_engine_move_player(self._eng, direction)
        status_to_status_exception(status, "Failed to move player")

    def render_current_room(self) -> str:
        str_out = ctypes.c_char_p()

        # C function allocates and returns a string describing the current room
        status = self._lib.game_engine_render_current_room(self._eng, ctypes.byref(str_out))
        status_to_status_exception(status, "Failed to render room")

        if not str_out:
            return ""

        py_str = str_out.value.decode('utf-8')  # convert C string to Python string
        self._lib.game_engine_free_string(str_out)  # free memory allocated by the C backend
        return py_str

    def get_room_count(self) -> int:
        count = ctypes.c_int()
        status = self._lib.game_engine_get_room_count(self._eng, ctypes.byref(count))
        status_to_status_exception(status, "Failed to get room count")
        return count.value

    def get_room_dimensions(self) -> Tuple[int, int]:
        """Returns (width, height) of the current room."""
        width = ctypes.c_int()
        height = ctypes.c_int()

        # C backend writes room dimensions into the provided variables
        status = self._lib.game_engine_get_room_dimensions(self._eng, ctypes.byref(width), ctypes.byref(height))
        status_to_status_exception(status, "Failed to get room dimensions")

        return (width.value, height.value)

    def get_room_ids(self) -> List[int]:
        ids_ptr = ctypes.POINTER(ctypes.c_int)()
        count = ctypes.c_int()

        # returns pointer to array of room IDs allocated by the C backend
        status = self._lib.game_engine_get_room_ids(self._eng, ctypes.byref(ids_ptr), ctypes.byref(count))
        status_to_status_exception(status, "Failed to get room IDs")

        try:
            result = [ids_ptr[i] for i in range(count.value)]  # copy IDs into Python list
        finally:
            self._lib.game_engine_free_string(ids_ptr)  # free backend allocation

        return result

    def reset(self) -> None:
        # restore the engine to its initial state
        status = self._lib.game_engine_reset(self._eng)
        status_to_status_exception(status, "Failed to reset game")

    def __del__(self):
        # ensure C engine memory is freed if Python object is garbage collected
        self.destroy()
