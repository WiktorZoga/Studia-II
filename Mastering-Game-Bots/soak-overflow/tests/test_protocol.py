"""Exercise the bot through the same stdin/stdout boundary as CodinGame."""

from pathlib import Path
import itertools
import os
import random
import select
import subprocess
import sys
import tempfile
import time
import unittest


BOT = str(Path(sys.argv.pop(1)).resolve())


def setup_input(definitions, width=8, height=5, covers=None, optimal_range=5, bombs=None):
    rows = ["0", str(len(definitions))]
    bombs = bombs or {}
    rows.extend(f"{agent_id} {player} 0 {optimal_range} 20 {bombs.get(agent_id, 0)}"
                for agent_id, player in definitions)
    rows.append(f"{width} {height}")
    covers = covers or {}
    rows.extend(f"{x} {y} {covers.get((x, y), 0)}"
                for y in range(height) for x in range(width))
    return "\n".join(rows) + "\n"


def turn_input(states, owned_count):
    rows = [str(len(states))]
    rows.extend(" ".join(map(str, state)) for state in states)
    rows.append(str(owned_count))
    return "\n".join(rows) + "\n"


def basic_cover_layout():
    return {(1, 1): 2, (1, 3): 1, (11, 1): 1, (11, 3): 2,
            (3, 1): 1, (3, 3): 2, (9, 1): 2, (9, 3): 1}


def bomb_tutorial_fixture(seed):
    """Construct board variations from the official tutorial's documented setup."""
    rng = random.Random(seed)
    width = rng.choice((15, 17))
    mobile, trapped = rng.choice(((1, 2), (2, 1)))
    definitions = [(mobile, 0), (trapped, 0)]
    states = {mobile: [0, width // 2, 6, 3, 70]}
    covers, goals, mixed = {}, set(), set()
    counts = [rng.randint(5, 7), rng.randint(4, 6), 7, rng.randint(7, 10)]
    rng.shuffle(counts)
    trapped_group = min(range(4), key=lambda i: (counts[i], -i))
    next_id = 3
    for group, count in enumerate(counts):
        center_x, center_y = 2 + (width - 5) * (group // 2), 2 + 7 * (group % 2)
        xs, ys = list(range(-2, 3)), list(range(-2, 3))
        rng.shuffle(xs)
        rng.shuffle(ys)
        trapped_placed = False
        for dx in xs:
            for dy in ys:
                x, y = center_x + dx, center_y + dy
                if abs(dx) == 2 or abs(dy) == 2:
                    covers[(x, y)] = 2
                elif count > 0:
                    if group == trapped_group and not trapped_placed:
                        states[trapped] = [0, x, y, 1, 70]
                        trapped_placed = True
                    else:
                        states[next_id] = [1, x, y, 0, 70]
                        definitions.append((next_id, 1))
                        (mixed if group == trapped_group else goals).add(next_id)
                        next_id += 1
                        count -= 1
    return width, covers, definitions, states, goals, mixed


class ProtocolTest(unittest.TestCase):
    def start_bot(self, definitions, width=8, height=5, covers=None, optimal_range=5, bombs=None):
        self.diagnostics = tempfile.TemporaryFile()
        self.process = subprocess.Popen(
            [BOT], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=self.diagnostics,
        )
        self.pending_output = b""
        self.addCleanup(self.stop_bot, self.process, self.diagnostics)
        self.send(setup_input(definitions, width, height, covers, optimal_range, bombs))

    def stop_bot(self, process=None, diagnostics=None):
        process = process or self.process
        diagnostics = diagnostics or self.diagnostics
        if process.poll() is None:
            process.terminate()
        try:
            process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        process.stdin.close()
        process.stdout.close()
        diagnostics.close()

    def send(self, text):
        self.process.stdin.write(text.encode())
        self.process.stdin.flush()

    def commands(self, count):
        # Keep stdin open: waiting for EOF would hide the platform deadlock.
        deadline = time.monotonic() + 1.0
        lines = []
        while len(lines) < count:
            if b"\n" in self.pending_output:
                line, self.pending_output = self.pending_output.split(b"\n", 1)
                lines.append(line.decode())
                continue
            remaining = deadline - time.monotonic()
            if remaining <= 0 or not select.select(
                [self.process.stdout], [], [], max(0, remaining)
            )[0]:
                self.fail(f"Bot has not provided {count} lines in time; got {lines}")
            chunk = os.read(self.process.stdout.fileno(), 4096)
            if not chunk:
                self.fail(f"Bot exited before providing {count} commands")
            self.pending_output += chunk
        self.assertEqual(self.pending_output, b"", "Unexpected extra stdout")
        return lines

    def test_outputs_two_shoot_commands_without_waiting_for_eof(self):
        self.start_bot([(1, 0), (2, 0), (3, 1), (4, 1)])
        self.send(turn_input([
            (1, 3, 2, 0, 0, 0), (2, 4, 2, 0, 0, 0),
            (3, 2, 2, 0, 0, 20), (4, 5, 2, 0, 0, 70),
        ], 2))
        self.assertEqual(self.commands(2), ["1;SHOOT 4", "2;SHOOT 4"])

    def test_minimal_one_agent_turn(self):
        self.start_bot([(1, 0), (3, 1)], width=2, height=1)
        self.send(turn_input([(1, 0, 0, 0, 0, 0), (3, 1, 0, 0, 0, 10)], 1))
        self.assertEqual(self.commands(1), ["1;SHOOT 3"])

    def test_sparse_ids_and_wet_ally_are_not_enemy_targets(self):
        self.start_bot([(91, 0), (42, 1), (7, 0), (120, 1)])
        self.send(turn_input([
            (120, 5, 2, 0, 0, 15), (7, 4, 2, 0, 0, 90),
            (42, 2, 2, 0, 0, 60), (91, 3, 2, 0, 0, 0),
        ], 2))
        self.assertEqual(self.commands(2), ["7;SHOOT 42", "91;SHOOT 42"])

    def test_target_and_survivors_are_updated_each_turn(self):
        self.start_bot([(1, 0), (2, 0), (3, 1), (4, 1)])
        self.send(turn_input([
            (1, 3, 2, 0, 0, 0), (2, 4, 2, 0, 0, 0),
            (3, 2, 2, 0, 0, 20), (4, 5, 2, 0, 0, 70),
        ], 2))
        self.assertEqual(self.commands(2), ["1;SHOOT 4", "2;SHOOT 4"])
        self.send(turn_input([
            (2, 4, 2, 0, 0, 0), (3, 2, 2, 0, 0, 80),
        ], 1))
        self.assertEqual(self.commands(1), ["2;SHOOT 3"])

    def test_zero_wetness_enemy_is_still_selected(self):
        self.start_bot([(1, 0), (3, 1)])
        self.send(turn_input([(1, 0, 0, 0, 0, 0), (3, 1, 0, 0, 0, 0)], 1))
        self.assertEqual(self.commands(1), ["1;SHOOT 3"])

    def test_equal_wetness_uses_initial_enemy_order(self):
        self.start_bot([(1, 0), (3, 1), (4, 1)])
        self.send(turn_input([
            (4, 2, 0, 0, 0, 50), (1, 0, 0, 0, 0, 0),
            (3, 1, 0, 0, 0, 50),
        ], 1))
        self.assertEqual(self.commands(1), ["1;SHOOT 3"])

    def test_no_enemy_still_emits_an_explicit_agent_command(self):
        self.start_bot([(1, 0), (3, 1)])
        self.send(turn_input([(1, 0, 0, 0, 0, 0)], 1))
        self.assertEqual(self.commands(1), ["1;HUNKER_DOWN"])

    def test_both_agents_move_behind_high_cover(self):
        self.start_basic_cover_turn()
        lines = self.commands(2)
        self.assertTrue(lines[0].startswith("1;MOVE 0 1;SHOOT "), lines)
        self.assertTrue(lines[1].startswith("2;MOVE 12 3;SHOOT "), lines)

    def test_moving_agents_choose_different_less_protected_enemies(self):
        self.start_basic_cover_turn()
        self.assertEqual(self.commands(2),
                         ["1;MOVE 0 1;SHOOT 3", "2;MOVE 12 3;SHOOT 6"])

    def start_basic_cover_turn(self):
        self.start_bot([(1, 0), (2, 0), (3, 1), (4, 1), (5, 1), (6, 1)],
                       width=13, covers=basic_cover_layout(), optimal_range=6)
        self.send(turn_input([
            (1, 0, 2, 0, 0, 0), (2, 12, 2, 0, 0, 0),
            (3, 4, 1, 0, 0, 0), (4, 4, 3, 0, 0, 90),
            (5, 8, 1, 0, 0, 80), (6, 8, 3, 0, 0, 10),
        ], 2))

    def test_all_base_tutorial_layouts_with_and_without_decorations(self):
        # The objective rows are selected independently by the fixture.
        for choices in itertools.product((False, True), repeat=6):
            left_up, right_up, left_target_up, right_target_up, low, decorated = choices
            left_row, right_row = (1 if left_up else 3), (1 if right_up else 3)
            left_target_row = 1 if left_target_up else 3
            right_target_row = 1 if right_target_up else 3
            covers = {
                (1, left_row): 2, (1, 4 - left_row): 1,
                (11, right_row): 2, (11, 4 - right_row): 1,
                (3, left_target_row): int(low), (3, 4 - left_target_row): 2,
                (9, right_target_row): int(low), (9, 4 - right_target_row): 2,
            }
            if decorated:
                covers.update({(6, 1): 2, (6, 3): 2, (4, 2): 2, (8, 2): 1,
                               (4, 0): 1, (8, 0): 2, (4, 4): 2, (8, 4): 1})
            with self.subTest(layout=choices):
                self.start_bot([(1, 0), (2, 0), (3, 1), (4, 1), (5, 1), (6, 1)],
                               width=13, covers=covers, optimal_range=6)
                try:
                    self.send(turn_input([
                        (1, 0, 2, 0, 0, 0), (2, 12, 2, 0, 0, 0),
                        (3, 4, 1, 0, 0, 0), (4, 4, 3, 0, 0, 90),
                        (5, 8, 1, 0, 0, 80), (6, 8, 3, 0, 0, 10),
                    ], 2))
                    left_id = 3 if left_target_up else 4
                    right_id = 5 if right_target_up else 6
                    self.assertEqual(self.commands(2), [
                        f"1;MOVE 0 {left_row};SHOOT {left_id}",
                        f"2;MOVE 12 {right_row};SHOOT {right_id}",
                    ])
                finally:
                    self.stop_bot()

    def test_cover_commands_use_shuffled_ids_and_current_input_order(self):
        self.start_bot([(42, 1), (91, 0), (120, 1), (7, 0), (17, 1), (99, 1)],
                       width=13, covers=basic_cover_layout(), optimal_range=6)
        self.send(turn_input([
            (7, 12, 2, 0, 0, 0), (120, 4, 3, 0, 0, 90),
            (99, 8, 3, 0, 0, 10), (91, 0, 2, 0, 0, 0),
            (17, 8, 1, 0, 0, 80), (42, 4, 1, 0, 0, 0),
        ], 2))
        self.assertEqual(self.commands(2),
                         ["7;MOVE 12 3;SHOOT 99", "91;MOVE 0 1;SHOOT 42"])

    def test_cover_tiles_are_not_movement_destinations(self):
        covers = {(1, 1): 2, (1, 3): 1, (0, 1): 2,
                  (3, 1): 1, (3, 3): 2}
        self.start_bot([(1, 0), (3, 1), (4, 1)],
                       width=13, covers=covers, optimal_range=6)
        self.send(turn_input([(1, 0, 2, 0, 0, 0),
                              (3, 4, 1, 0, 0, 0), (4, 4, 3, 0, 0, 90)], 1))
        self.assertTrue(self.commands(1)[0].startswith("1;MOVE 0 3;SHOOT "))

    def test_occupied_tiles_are_not_movement_destinations(self):
        covers = {(1, 1): 2, (1, 3): 1, (3, 1): 1, (3, 3): 2}
        self.start_bot([(1, 0), (2, 0), (3, 1), (4, 1)],
                       width=13, covers=covers, optimal_range=6)
        self.send(turn_input([(1, 0, 2, 0, 0, 0), (2, 0, 1, 0, 0, 0),
                              (3, 4, 1, 0, 0, 0), (4, 4, 3, 0, 0, 90)], 2))
        self.assertTrue(self.commands(2)[0].startswith("1;MOVE 0 3;SHOOT "))

    def test_cover_target_must_be_in_range_after_moving(self):
        covers = {(1, 1): 2, (1, 3): 1, (3, 1): 2}
        self.start_bot([(1, 0), (3, 1), (4, 1)],
                       width=13, covers=covers, optimal_range=2)
        self.send(turn_input([(1, 0, 2, 0, 0, 0),
                              (3, 4, 1, 0, 0, 0), (4, 4, 3, 0, 0, 90)], 1))
        self.assertEqual(self.commands(1), ["1;MOVE 0 1;SHOOT 3"])

    def test_bomb_throw_clears_a_barricaded_group_without_shooting(self):
        covers = {(x, y): 2 for y in range(1, 6) for x in range(4, 9)
                  if x in (4, 8) or y in (1, 5)}
        self.start_bot([(1, 0), (3, 1), (4, 1), (5, 1), (6, 1)],
                       width=9, height=7, covers=covers, bombs={1: 3})
        self.send(turn_input([
            (1, 2, 3, 0, 3, 70), (3, 5, 2, 0, 0, 70),
            (4, 7, 2, 0, 0, 70), (5, 5, 4, 0, 0, 70), (6, 7, 4, 0, 0, 70),
        ], 1))
        self.assertEqual(self.commands(1), ["1;THROW 6 3"])

    def test_bomb_agent_moves_into_range_before_throwing(self):
        covers = {(x, y): 2 for y in range(1, 6) for x in range(4, 9)
                  if x in (4, 8) or y in (1, 5)}
        self.start_bot([(1, 0), (3, 1), (4, 1), (5, 1), (6, 1)],
                       width=9, height=7, covers=covers, bombs={1: 3})
        self.send(turn_input([
            (1, 1, 3, 0, 3, 70), (3, 5, 2, 0, 0, 70),
            (4, 7, 2, 0, 0, 70), (5, 5, 4, 0, 0, 70), (6, 7, 4, 0, 0, 70),
        ], 1))
        self.assertEqual(self.commands(1), ["1;MOVE 2 3;THROW 6 3"])

    def test_bombs_do_not_target_the_mixed_friendly_group(self):
        covers = {(x, y): 2 for y in range(1, 6) for x in range(4, 9)
                  if x in (4, 8) or y in (1, 5)}
        self.start_bot([(1, 0), (2, 0), (3, 1), (4, 1), (5, 1)],
                       width=9, height=7, covers=covers, bombs={1: 3, 2: 1})
        self.send(turn_input([
            (1, 2, 3, 0, 3, 70), (2, 7, 4, 0, 1, 70),
            (3, 5, 2, 0, 0, 70), (4, 7, 2, 0, 0, 70), (5, 5, 4, 0, 0, 70),
        ], 2))
        self.assertEqual(self.commands(2), ["1;MESSAGE waiting", "2;MESSAGE waiting"])

    def test_bombs_are_not_duplicated_and_zero_inventory_is_respected(self):
        covers = {(x, y): 2 for y in range(1, 6) for x in range(4, 9)
                  if x in (4, 8) or y in (1, 5)}
        self.start_bot([(1, 0), (2, 0), (3, 1), (4, 1), (5, 1), (6, 1)],
                       width=9, height=7, covers=covers, bombs={1: 3, 2: 1})
        states = [(1, 2, 3, 0, 3, 70), (2, 2, 2, 0, 1, 70),
                  (3, 5, 2, 0, 0, 70), (4, 7, 2, 0, 0, 70),
                  (5, 5, 4, 0, 0, 70), (6, 7, 4, 0, 0, 70)]
        self.send(turn_input(states, 2))
        self.assertEqual(self.commands(2), ["1;THROW 6 3", "2;MESSAGE waiting"])
        self.send(turn_input([(1, 2, 3, 0, 0, 70), (2, 2, 2, 0, 0, 70)] + states[2:], 2))
        self.assertEqual(self.commands(2), ["1;MESSAGE waiting", "2;MESSAGE waiting"])

    def test_bomb_tutorial_replays_clear_three_groups_safely(self):
        # This checks only the tutorial's movement, blast, and inventory rules.
        # It is a scripted replay through stdin/stdout, not a full game engine.
        for seed in range(64):
            width, covers, definitions, states, goals, mixed = bomb_tutorial_fixture(seed)
            bombs = {agent_id: state[3] for agent_id, state in states.items() if state[0] == 0}
            with self.subTest(seed=seed):
                self.start_bot(definitions, width=width, height=12,
                               covers=covers, optimal_range=1, bombs=bombs)
                try:
                    for turn in range(39):
                        owned = {i for i, state in states.items() if state[0] == 0}
                        ids = list(states)
                        random.Random(seed + turn).shuffle(ids)
                        self.send(turn_input([
                            (i, states[i][1], states[i][2], 0, states[i][3], states[i][4])
                            for i in ids
                        ], len(owned)))
                        lines = self.commands(len(owned))
                        old_positions = {i: tuple(s[1:3]) for i, s in states.items()}
                        destinations, throws, commanded = {}, [], set()
                        for line in lines:
                            tokens = line.split(";")
                            agent_id = int(tokens[0])
                            self.assertIn(agent_id, owned)
                            self.assertNotIn(agent_id, commanded)
                            commanded.add(agent_id)
                            for token in tokens[1:]:
                                parts = token.split()
                                self.assertIn(parts[0], ("MOVE", "THROW", "MESSAGE"))
                                if parts[0] == "MOVE":
                                    self.assertFalse(any(i == agent_id for i, _ in throws))
                                    x, y = map(int, parts[1:])
                                    old_x, old_y = old_positions[agent_id]
                                    self.assertEqual(abs(x - old_x) + abs(y - old_y), 1)
                                    self.assertTrue(0 <= x < width and 0 <= y < 12)
                                    self.assertEqual(covers.get((x, y), 0), 0)
                                    self.assertNotIn((x, y), [p for i, p in old_positions.items()
                                                            if i != agent_id])
                                    destinations[agent_id] = (x, y)
                                elif parts[0] == "THROW":
                                    throws.append((agent_id, tuple(map(int, parts[1:]))))
                        self.assertEqual(commanded, owned)
                        for i, destination in destinations.items():
                            states[i][1:3] = destination
                        self.assertEqual(len({tuple(states[i][1:3]) for i in owned}), len(owned))
                        for i, (x, y) in throws:
                            self.assertGreater(states[i][3], 0)
                            self.assertTrue(0 <= x < width and 0 <= y < 12)
                            self.assertLessEqual(abs(x - states[i][1]) + abs(y - states[i][2]), 4)
                            for friendly in owned:
                                self.assertGreater(max(abs(x - states[friendly][1]),
                                                       abs(y - states[friendly][2])), 1)
                            states[i][3] -= 1
                            for state in states.values():
                                if max(abs(x - state[1]), abs(y - state[2])) <= 1:
                                    state[4] += 30
                        states = {i: s for i, s in states.items() if s[4] < 100}
                        self.assertTrue(owned.issubset(states))
                        self.assertTrue(mixed.issubset(states))
                        if not goals.intersection(states):
                            break
                    else:
                        self.fail("Enemy-only groups were not cleared before the tutorial deadline")
                finally:
                    self.stop_bot()


if __name__ == "__main__":
    unittest.main()
