import os
from ctypes import CDLL
from .bindings import Direction, Status, Treasure, setup_bindings, GameEngine, Player, Room

GameEngineStatus = Status

def _load_lib():
    paths = [
        "dist/libbackend.so",
        "../dist/libbackend.so",
        os.path.join(os.environ.get("TREASURE_RUNNER_DIST", "."), "libbackend.so")
    ]
    for path_str in paths:
        if os.path.exists(path_str):
            try:
                loaded_lib = CDLL(path_str)
                setup_bindings(loaded_lib)
                return loaded_lib
            except OSError:
                continue
    return None

lib = _load_lib()

__all__ = ["Direction", "Status", "GameEngineStatus", "Treasure", "setup_bindings", "lib", "GameEngine", "Player", "Room"]
