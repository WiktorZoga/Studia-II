"""Exercise the bot through the same stdin/stdout boundary as CodinGame."""

from pathlib import Path
import os
import select
import subprocess
import sys
import tempfile
import time
import unittest


BOT = str(Path(sys.argv.pop(1)).resolve())


def setup_input(definitions, width=8, height=5):
    rows = ["0", str(len(definitions))]
    rows.extend(f"{agent_id} {player} 0 5 20 0" for agent_id, player in definitions)
    rows.append(f"{width} {height}")
    rows.extend(f"{x} {y} 0" for y in range(height) for x in range(width))
    return "\n".join(rows) + "\n"


def turn_input(states, owned_count):
    rows = [str(len(states))]
    rows.extend(" ".join(map(str, state)) for state in states)
    rows.append(str(owned_count))
    return "\n".join(rows) + "\n"


class ProtocolTest(unittest.TestCase):
    def start_bot(self, definitions, width=8, height=5):
        self.diagnostics = tempfile.TemporaryFile()
        self.process = subprocess.Popen(
            [BOT], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=self.diagnostics,
        )
        self.pending_output = b""
        self.addCleanup(self.stop_bot)
        self.send(setup_input(definitions, width, height))

    def stop_bot(self):
        if self.process.poll() is None:
            self.process.terminate()
        try:
            self.process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            self.process.kill()
            self.process.wait()
        self.process.stdin.close()
        self.process.stdout.close()
        self.diagnostics.close()

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


if __name__ == "__main__":
    unittest.main()
