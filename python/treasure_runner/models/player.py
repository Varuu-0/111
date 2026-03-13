import ctypes
import os
from typing import Tuple, List, Dict, Any
from ctypes import CDLL
from treasure_runner.bindings import setup_bindings, Player as CPlayerPtr, Treasure
from treasure_runner.models.exceptions import status_to_status_exception

_CACHE = {}  # cache the loaded C library so it is only loaded once

def _load_lib():
    # possible locations of the compiled backend library
    paths = [
        "dist/libbackend.so",
        "../dist/libbackend.so",
        os.path.join(os.environ.get("TREASURE_RUNNER_DIST", "."), "libbackend.so")
    ]

    for path_str in paths:
        if os.path.exists(path_str):
            try:
                lib = CDLL(path_str)  # load shared C library
                setup_bindings(lib)   # configure ctypes function signatures
                return lib
            except OSError:
                continue

    raise RuntimeError("Could not load libbackend.so")

def get_lib():
    if "lib" not in _CACHE:
        _CACHE["lib"] = _load_lib()
    return _CACHE["lib"]

class Player:
    def __init__(self, player_ptr: CPlayerPtr):
        if not player_ptr:
            raise ValueError("Player pointer cannot be NULL")

        self._ptr = player_ptr  # store pointer to C Player struct
        self._lib = get_lib()

    def get_room(self) -> int:
        return self._lib.player_get_room(self._ptr)

    def get_position(self) -> Tuple[int, int]:
        x = ctypes.c_int()
        y = ctypes.c_int()

        # C function writes position values into x and y
        status = self._lib.player_get_position(self._ptr, ctypes.byref(x), ctypes.byref(y))
        status_to_status_exception(status, "Failed to get player position")

        return (x.value, y.value)

    def get_collected_count(self) -> int:
        return self._lib.player_get_collected_count(self._ptr)

    def has_collected_treasure(self, treasure_id: int) -> bool:
        return self._lib.player_has_collected_treasure(self._ptr, treasure_id)

    def get_collected_treasures(self) -> List[Dict[str, Any]]:
        count = ctypes.c_int()

        # returns pointer to array of Treasure pointers from the C backend
        treasures_ptr = self._lib.player_get_collected_treasures(self._ptr, ctypes.byref(count))

        result = []
        if not treasures_ptr:
            return result

        for i in range(count.value):
            treasure_ptr = treasures_ptr[i]
            if not treasure_ptr:
                continue

            treasure_val = treasure_ptr.contents  # dereference C struct

            # convert C Treasure struct into Python dictionary
            treasure_dict = {
                "id": treasure_val.id,
                "name": treasure_val.name.decode('utf-8') if treasure_val.name else "",
                "starting_room_id": treasure_val.starting_room_id,
                "initial_x": treasure_val.initial_x,
                "initial_y": treasure_val.initial_y,
                "x": treasure_val.x,
                "y": treasure_val.y,
                "collected": treasure_val.collected
            }

            result.append(treasure_dict)

        return result

    def reset_to_start(self, room_id: int, start_x: int, start_y: int) -> None:
        status = self._lib.player_reset_to_start(self._ptr, room_id, start_x, start_y)
        status_to_status_exception(status, "Failed to reset player")
