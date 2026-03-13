import argparse
import sys
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.bindings.bindings import Direction
from treasure_runner.models.exceptions import ImpassableError, StatusImpassableError, GameError

# Map enum directions to readable strings for logging
DIR_MAP = {
    Direction.NORTH: "NORTH",
    Direction.SOUTH: "SOUTH",
    Direction.EAST: "EAST",
    Direction.WEST: "WEST"
}

# Write a formatted event line to both console and log file
def log(file, event, **kwargs):
    if kwargs:
        kv_pairs = [f"{k}={v}" for k, v in kwargs.items()]
        line = f"{event}|{'|'.join(kv_pairs)}"
    else:
        line = event
    print(line)
    file.write(line + "\n")

# Serialize the current engine/player state for logging
def get_state_str(engine):
    pos = engine.player.get_position()
    return f"room={engine.player.get_room()}|x={pos[0]}|y={pos[1]}|collected={engine.player.get_collected_count()}"

def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", required=True)
    parser.add_argument("--log", required=True)
    args = parser.parse_args()

    # Open log file for the run
    with open(args.log, "w") as log_file:
        try:
            # Initialize game engine from config
            engine = GameEngine(args.config)

            log(log_file, "RUN_START", config=args.config)
            log(log_file, "STATE", step=0, phase="SPAWN", state=get_state_str(engine))

            # --- Determine entry direction ---
            entry_dir = None
            initial_room = engine.player.get_room()

            # Try moves in all directions to detect which move changes player state
            for d in [Direction.SOUTH, Direction.WEST, Direction.NORTH, Direction.EAST]:
                try:
                    engine.reset()
                    before_state = get_state_str(engine)

                    engine.move_player(d)

                    after_state = get_state_str(engine)

                    # Entry direction must change state but remain in same room
                    if engine.player.get_room() == initial_room and before_state != after_state:
                        entry_dir = d
                        break
                except Exception:
                    continue

            # If no valid entry move was found, terminate run
            if entry_dir is None:
                log(log_file, "ENTRY")
                log_file.write("TERMINATED: Initial Move Error\n")
                print("TERMINATED: Initial Move Error")
                log(log_file, "RUN_END", steps=0, collected_total=0)
                return

            log(log_file, "ENTRY", direction=DIR_MAP[entry_dir])

            # Reset engine and perform the confirmed entry move
            engine.reset()

            before = get_state_str(engine)
            c_before = engine.player.get_collected_count()

            engine.move_player(entry_dir)

            after = get_state_str(engine)
            c_after = engine.player.get_collected_count()

            log(log_file, "MOVE", step=1, phase="ENTRY", dir=DIR_MAP[entry_dir], result="OK",
                before=before, after=after, delta_collected=(c_after - c_before))

            step = 2

            # Sweep phases attempt exploration in each direction
            phases = [
                ("SWEEP_SOUTH", Direction.SOUTH),
                ("SWEEP_WEST", Direction.WEST),
                ("SWEEP_NORTH", Direction.NORTH),
                ("SWEEP_EAST", Direction.EAST)
            ]

            total_collected = engine.player.get_collected_count()

            for phase_name, sweep_dir in phases:
                log(log_file, "SWEEP_START", phase=phase_name, dir=DIR_MAP[sweep_dir])

                # Track visited states to detect loops
                seen_states = set()

                while True:
                    before = get_state_str(engine)
                    c_before = engine.player.get_collected_count()

                    # Stop if we revisit a previously seen state
                    if before in seen_states:
                        log(log_file, "SWEEP_END", phase=phase_name, reason="CYCLE_DETECTED")
                        break

                    seen_states.add(before)

                    try:
                        engine.move_player(sweep_dir)

                        after = get_state_str(engine)
                        c_after = engine.player.get_collected_count()

                        # If move caused no change, direction is blocked
                        if before == after:
                            log(log_file, "SWEEP_END", phase=phase_name, reason="BLOCKED")
                            break

                        log(log_file, "MOVE", step=step, phase=phase_name, dir=DIR_MAP[sweep_dir],
                            result="OK", before=before, after=after, delta_collected=(c_after - c_before))

                        step += 1
                        total_collected = engine.player.get_collected_count()

                    # Handle movement failures
                    except (ImpassableError, StatusImpassableError):
                        log(log_file, "SWEEP_END", phase=phase_name, reason="BLOCKED")
                        break
                    except GameError:
                        log(log_file, "SWEEP_END", phase=phase_name, reason="ERROR")
                        break
                    except Exception:
                        log(log_file, "SWEEP_END", phase=phase_name, reason="ERROR")
                        break

            # Log final state and summary
            log(log_file, "STATE", phase="FINAL", state=get_state_str(engine))
            log(log_file, "RUN_END", steps=step-1, collected_total=total_collected)

        except Exception as e:
            print(f"Critical Failure: {e}")

if __name__ == "__main__":
    main()
