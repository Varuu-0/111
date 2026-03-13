import unittest
import os
from treasure_runner.bindings.bindings import Direction, Status
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.player import Player
from treasure_runner.models.exceptions import GameError

# Points to the assets folder used by the grader
CONFIG_PATH = "../assets/treasure_runner.ini"

class TestTreasureRunnerModels(unittest.TestCase):
    def setUp(self):
        if not os.path.exists(CONFIG_PATH):
            # Fallback path if running from root
            fallback = "assets/treasure_runner.ini"
            if os.path.exists(fallback):
                self.config_path = fallback
            else:
                self.skipTest(f"Config not found at {CONFIG_PATH} or {fallback}")
        else:
            self.config_path = CONFIG_PATH
            
        self.engine = GameEngine(self.config_path)

    def tearDown(self):
        if hasattr(self, 'engine') and self.engine is not None:
            self.engine.destroy()

    def test_engine_initialization(self):
        self.assertIsNotNone(self.engine.player)
        self.assertIsInstance(self.engine.player, Player)

    def test_get_room_count(self):
        count = self.engine.get_room_count()
        self.assertGreater(count, 0)

    def test_get_room_dimensions(self):
        width, height = self.engine.get_room_dimensions()
        self.assertGreater(width, 0)
        self.assertGreater(height, 0)

    def test_get_room_ids(self):
        ids = self.engine.get_room_ids()
        self.assertIsInstance(ids, list)
        self.assertEqual(len(ids), self.engine.get_room_count())

    def test_player_get_room(self):
        room_id = self.engine.player.get_room()
        self.assertIn(room_id, self.engine.get_room_ids())

    def test_player_position(self):
        x, y = self.engine.player.get_position()
        self.assertGreaterEqual(x, 0)
        self.assertGreaterEqual(y, 0)

    def test_engine_reset(self):
        initial_room = self.engine.player.get_room()
        init_x, init_y = self.engine.player.get_position()
        try:
            self.engine.move_player(Direction.SOUTH)
        except Exception:
            pass
        self.engine.reset()
        new_room = self.engine.player.get_room()
        new_x, new_y = self.engine.player.get_position()
        self.assertEqual(initial_room, new_room)
        self.assertEqual(init_x, new_x)
        self.assertEqual(init_y, new_y)

    def test_render_current_room(self):
        rendered = self.engine.render_current_room()
        self.assertIsInstance(rendered, str)
        self.assertTrue(len(rendered) > 0)
        self.assertIn('@', rendered)

    def test_player_treasures_initial(self):
        self.assertEqual(self.engine.player.get_collected_count(), 0)
        self.assertEqual(self.engine.player.get_collected_treasures(), [])
        self.assertFalse(self.engine.player.has_collected_treasure(999))

if __name__ == '__main__':
    unittest.main()
