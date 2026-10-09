"""Bronze rules and decisions observed through the live stdin/stdout boundary."""

import json
from pathlib import Path
import random
import statistics
import time
import unittest

from test_protocol import BotProcess, turn_input


class BronzeTest(BotProcess):
    def start_bot(self, definitions, width=12, height=6, covers=None, optimal_range=3,
                  bombs=None, player_id=0, attributes=None):
        self.definitions = dict(definitions)
        self.width, self.height = width, height
        self.covers = covers or {}
        self.player = player_id
        self.attributes = attributes or {}
        self.range = optimal_range
        super().start_bot(definitions, width, height, covers, optimal_range,
                          bombs, player_id, attributes)

    def play_turn(self, states):
        """Check the output against the input's rule constraints, not bot helpers."""
        current = {row[0]: row for row in states}
        owned = {i for i in current if self.definitions[i] == self.player}
        self.send(turn_input(states, len(owned)))
        result = {}
        for line in self.commands(len(owned)):
            fields = line.split(';')
            agent_id = int(fields.pop(0))
            self.assertIn(agent_id, owned)
            self.assertNotIn(agent_id, result)
            row = current[agent_id]
            destination = (row[1], row[2])
            if fields[0].startswith('MOVE '):
                action = fields.pop(0).split()
                self.assertEqual(len(action), 3)
                destination = tuple(map(int, action[1:]))
                x, y = destination
                self.assertTrue(0 <= x < self.width and 0 <= y < self.height)
                self.assertEqual(abs(x-row[1]) + abs(y-row[2]), 1)
                self.assertEqual(self.covers.get(destination, 0), 0)
                self.assertNotIn(destination, [(r[1], r[2]) for i, r in current.items()
                                               if i != agent_id])
            self.assertEqual(len(fields), 1)
            combat = fields[0].split()
            self.assertIn(combat[0], ('SHOOT', 'THROW', 'HUNKER_DOWN'))
            if combat[0] == 'SHOOT':
                self.assertEqual(len(combat), 2)
                target = int(combat[1])
                self.assertIn(target, current)
                self.assertNotIn(target, owned)
                self.assertEqual(row[3], 0)
                distance = abs(destination[0]-current[target][1]) + abs(destination[1]-current[target][2])
                self.assertLessEqual(distance, 2*self.attributes.get(agent_id, (0, self.range, 20))[1])
            elif combat[0] == 'THROW':
                self.assertEqual(len(combat), 3)
                x, y = map(int, combat[1:])
                self.assertTrue(0 <= x < self.width and 0 <= y < self.height)
                self.assertGreater(row[4], 0)
                self.assertLessEqual(abs(x-destination[0]) + abs(y-destination[1]), 4)
            else:
                self.assertEqual(len(combat), 1)
            result[agent_id] = (destination, combat)
        self.assertEqual(set(result), owned)
        self.assertEqual(len({p for p, _ in result.values()}), len(owned))
        for _, combat in result.values():
            if combat[0] == 'THROW':
                x, y = map(int, combat[1:])
                for (fx, fy), _ in result.values():
                    self.assertGreater(max(abs(x-fx), abs(y-fy)), 1)
                for row in current.values():
                    if row[0] in owned:
                        self.assertGreater(max(abs(x-row[1]), abs(y-row[2])), 1)
        return result

    def test_shared_terrain_and_empty_inventory_do_not_disable_agents(self):
        for player in (0, 1):
            with self.subTest(player=player):
                self.start_bot([(91, player), (42, 1-player), (7, player)],
                               width=12, height=6, optimal_range=1,
                               bombs={91: 3, 7: 2}, player_id=player)
                self.send(turn_input([(42, 10, 2, 0, 2, 0),
                                      (7, 1, 3, 2, 0, 0), (91, 1, 1, 0, 0, 0)], 2))
                lines = self.commands(2)
                self.assertEqual({int(line.split(';')[0]) for line in lines}, {7, 91})
                for line in lines:
                    self.assertIn(';MOVE ', line)
                    self.assertNotIn('SHOOT', line)
                self.stop_bot()

    def test_cooldown_allows_defense_and_moves_avoid_walls_and_teammates(self):
        self.start_bot([(1, 0), (2, 0), (3, 1)], covers={(2, 1): 2, (1, 2): 1})
        actions = self.play_turn([(1, 1, 1, 3, 0, 0), (2, 2, 2, 2, 0, 0),
                                  (3, 4, 1, 0, 0, 0)])
        self.assertTrue(all(c == ['HUNKER_DOWN'] for _, c in actions.values()))

    def test_shoots_a_live_in_range_enemy_after_inventory_is_exhausted(self):
        self.start_bot([(8, 0), (15, 1), (99, 1)], bombs={8: 3}, optimal_range=1)
        actions = self.play_turn([(8, 0, 0, 0, 0, 0), (15, 1, 0, 0, 0, 0),
                                  (99, 11, 5, 0, 0, 90)])
        self.assertEqual(actions[8][1], ['SHOOT', '15'])

    def test_territory_changes_the_move_at_wetness_fifty(self):
        # On this 7x3 board, team-control advantages for stay/right/down are
        # 12/15/13 at wetness 49, and 8/7/9 at 50. Cross-team ties are neutral.
        for player in (0, 1):
            for wetness, destination in ((49, (1, 0)), (50, (0, 1))):
                with self.subTest(player=player, wetness=wetness):
                    self.start_bot([(1, player), (2, player), (3, 1-player)],
                                   width=7, height=3, optimal_range=1, player_id=player)
                    actions = self.play_turn([(1, 0, 0, 5, 0, wetness),
                                              (2, 3, 0, 5, 0, 0), (3, 2, 0, 5, 0, 0)])
                    self.assertEqual(actions[1][0], destination)
                    self.stop_bot()

    def test_cover_tiles_still_count_as_territory(self):
        self.start_bot([(1, 0), (2, 1)], width=7, height=3,
                       covers={(0, 2): 2, (1, 1): 2, (2, 1): 2, (2, 2): 2})
        actions = self.play_turn([(1, 4, 2, 5, 0, 0), (2, 5, 2, 5, 0, 0)])
        # Stay/left/up control advantages are 9/6/8, including cover tiles.
        # Counting floor alone would instead choose up (advantages 5/2/6).
        self.assertEqual(actions[1][0], (4, 2))

    def test_cross_team_distance_ties_remain_neutral(self):
        self.start_bot([(1, 0), (2, 1)], width=7, height=3)
        actions = self.play_turn([(1, 2, 2, 5, 0, 0), (2, 0, 1, 5, 0, 0)])
        # Stay/right/left/up advantages are 11/8/4/12. Incorrectly claiming
        # equal-distance tiles for our team would prefer left instead of up.
        self.assertEqual(actions[1][0], (2, 1))

    def test_shooting_compares_all_enemies_and_actual_cover_damage(self):
        covers = {(x, y): 2 for x in range(12) for y in range(6)}
        for floor in ((0, 0), (3, 0), (4, 2), (4, 0), (5, 0)):
            covers.pop(floor)
        self.start_bot([(1, 0), (2, 1), (3, 1), (4, 1)], covers=covers,
                       optimal_range=5)
        actions = self.play_turn([(1, 0, 0, 0, 0, 0), (2, 3, 0, 2, 0, 90),
                                  (3, 4, 2, 2, 0, 80), (4, 5, 0, 2, 0, 0)])
        # The first two foes receive only 5 and 3 wetness through high cover;
        # the third receives 20. Wettest-first/nearest-two choices both miss it.
        self.assertEqual(actions[1][1], ['SHOOT', '4'])

    def test_cover_and_hunkering_can_outweigh_a_territory_advance(self):
        self.start_bot([(1, 0), (2, 1), (3, 1)], optimal_range=6,
                       covers={(1, 1): 2, (1, 3): 1},
                       attributes={2: (1, 6, 40), 3: (1, 6, 40)})
        actions = self.play_turn([(1, 0, 2, 0, 0, 0), (2, 4, 1, 0, 0, 0),
                                  (3, 4, 3, 0, 0, 0)])
        # High cover plus hunker blocks both observed 40-power shots. An
        # uncovered step forward would expose this agent to 80 wetness.
        self.assertEqual(actions[1], ((0, 1), ['HUNKER_DOWN']))

    def test_shooting_range_and_positive_damage_rounding(self):
        for distance, power, combat in ((3, 7, ['SHOOT', '2']),
                                        (6, 7, ['SHOOT', '2']),
                                        (7, 7, ['HUNKER_DOWN']),
                                        (6, 3, ['HUNKER_DOWN'])):
            with self.subTest(distance=distance, power=power):
                self.start_bot([(1, 0), (2, 1)],
                               covers={(1, 0): 2, (0, 1): 2, (distance-1, 0): 2},
                               attributes={1: (1, 3, power)})
                actions = self.play_turn([(1, 0, 0, 0, 0, 0), (2, distance, 0, 2, 0, 0)])
                # 7 * half range * quarter protection = 0.875 -> 1, but
                # 3 * half * quarter = 0.375 -> 0, so do not waste that shot.
                self.assertEqual(actions[1][1], combat)
                self.stop_bot()

    def test_safe_partial_splash_includes_diagonal_targets(self):
        self.start_bot([(1, 0), (2, 1), (3, 1), (4, 1)],
                       covers={(1, 0): 2, (0, 1): 2}, bombs={1: 1})
        actions = self.play_turn([(1, 0, 0, 2, 1, 0), (2, 3, 0, 2, 0, 0),
                                  (3, 4, 1, 2, 0, 0), (4, 11, 5, 2, 0, 0)])
        self.assertEqual(actions[1][1], ['THROW', '3', '0'])

    def test_throw_range_and_inventory_boundaries(self):
        for enemy_x, bombs, combat in ((5, 1, ['THROW', '4', '0']),
                                      (6, 1, ['HUNKER_DOWN']),
                                      (5, 0, ['HUNKER_DOWN'])):
            with self.subTest(enemy_x=enemy_x, bombs=bombs):
                self.start_bot([(1, 0), (2, 1)], bombs={1: 1},
                               covers={(1, 0): 2, (0, 1): 2})
                actions = self.play_turn([(1, 0, 0, 2, bombs, 0), (2, enemy_x, 0, 2, 0, 0)])
                self.assertEqual(actions[1][1], combat)
                self.stop_bot()

    def test_movement_can_put_a_safe_throw_in_range(self):
        self.start_bot([(1, 0), (2, 1)], bombs={1: 1})
        actions = self.play_turn([(1, 1, 0, 2, 1, 0), (2, 7, 0, 2, 0, 0)])
        self.assertEqual(actions[1], ((2, 0), ['THROW', '6', '0']))

    def test_surrounded_enemy_is_not_bombed_through_friendly_agents(self):
        positions = {1: (0, 1), 2: (2, 1), 3: (4, 1), 4: (3, 0), 5: (3, 2), 6: (3, 1)}
        covers = {(x, y): 2 for x in range(12) for y in range(6)
                  if (x, y) not in positions.values()}
        self.start_bot([(i, 0 if i < 6 else 1) for i in positions],
                       covers=covers, bombs={1: 3})
        actions = self.play_turn([(i, x, y, 0 if i == 1 else 2, 3 if i == 1 else 0, 0)
                                  for i, (x, y) in positions.items()])
        self.assertEqual(actions[1][1], ['SHOOT', '6'])
        self.assertFalse(any(c[0] == 'THROW' for _, c in actions.values()))

    def test_team_does_not_spend_two_bombs_on_an_already_soaked_target(self):
        self.start_bot([(1, 0), (2, 0), (3, 1)], bombs={1: 1, 2: 1},
                       covers={(1, 0): 2, (0, 1): 2, (1, 2): 2, (0, 3): 2})
        actions = self.play_turn([(1, 0, 0, 2, 1, 0), (2, 0, 2, 2, 1, 0),
                                  (3, 3, 1, 2, 0, 70)])
        self.assertEqual(sum(c[0] == 'THROW' for _, c in actions.values()), 1)

    def test_a_sufficient_shot_preserves_the_bomb(self):
        self.start_bot([(1, 0), (2, 1)], bombs={1: 1},
                       covers={(1, 0): 2, (0, 1): 2}, attributes={1: (1, 3, 40)})
        actions = self.play_turn([(1, 0, 0, 0, 1, 0), (2, 3, 0, 2, 0, 70)])
        self.assertEqual(actions[1][1], ['SHOOT', '2'])

    def test_turns_refresh_resources_positions_ids_and_casualties(self):
        self.start_bot([(91, 1), (15, 0), (7, 1), (120, 0), (42, 1), (88, 0)],
                       player_id=1, bombs={91: 3, 7: 2, 42: 1})
        turns = [
            [(15, 8, 0, 0, 2, 49), (42, 1, 5, 2, 1, 50), (7, 1, 2, 0, 2, 0),
             (120, 9, 5, 2, 0, 0), (91, 1, 0, 0, 3, 49), (88, 8, 2, 0, 2, 70)],
            [(88, 4, 2, 3, 0, 80), (7, 1, 2, 0, 0, 30),
             (42, 1, 4, 1, 0, 50), (120, 10, 4, 3, 0, 49)],
            [(120, 10, 4, 3, 0, 49), (7, 1, 1, 2, 0, 30)],
            [(7, 2, 1, 0, 0, 30), (120, 4, 1, 3, 0, 50)],
            [(7, 2, 1, 0, 0, 30)],
        ]
        for index, states in enumerate(turns):
            with self.subTest(turn=index):
                actions = self.play_turn(states)
                if index == 2:
                    self.assertNotEqual(actions[7][0], (1, 1))
                    self.assertEqual(actions[7][1], ['HUNKER_DOWN'])
                if index == 3:
                    self.assertEqual(actions[7][1], ['SHOOT', '120'])
                if index == 4:
                    self.assertEqual(actions[7][1], ['HUNKER_DOWN'])

    def test_map_agent_limits_and_local_turn_timing(self):
        # Generated legal input states, not a complete game engine or a win test.
        initial, regular = [], []
        for player in (0, 1):
            for width, height, team_size in ((12, 6, 3), (16, 8, 4), (20, 10, 5)):
                rng = random.Random(100*player + width)
                ids = [11 + i*7 for i in range(2*team_size)]
                definitions = [(i, player if n < team_size else 1-player)
                               for n, i in enumerate(ids)]
                covers = {(x, y): rng.choice((1, 2))
                          for x in range(width) for y in range(height) if rng.random() < .15}
                floor = [(x, y) for x in range(width) for y in range(height) if (x, y) not in covers]
                attributes = {i: (1 + n % 3, 3 + n % 3, 10 + 10*(n % 3))
                              for n, i in enumerate(ids)}
                self.start_bot(definitions, width, height, covers, bombs=dict.fromkeys(ids, 2),
                               player_id=player, attributes=attributes)
                for turn in range(20):
                    surviving = ids if turn < 10 else ids[1:-1]
                    positions = rng.sample(floor, len(surviving))
                    states = [(i, x, y, rng.choice((0, 1, 2)), rng.choice((0, 1, 2)),
                               rng.choice((0, 49, 50, 70, 95)))
                              for i, (x, y) in zip(surviving, positions)]
                    rng.shuffle(states)
                    started = time.perf_counter()
                    self.play_turn(states)
                    elapsed = (time.perf_counter() - started) * 1000
                    (initial if turn == 0 else regular).append(elapsed)
                self.stop_bot()
        timings = {'measurement': 'Python send/read/validation wall time; includes harness overhead',
                   'initial_max_ms': round(max(initial), 3),
                   'regular_median_ms': round(statistics.median(regular), 3),
                   'regular_max_ms': round(max(regular), 3),
                   'initial_samples': len(initial), 'regular_samples': len(regular)}
        # A local observation is not a CodinGame performance guarantee.
        from test_protocol import BOT
        Path(BOT).with_name('bronze-timing.json').write_text(json.dumps(timings, indent=2) + '\n')
        self.assertLess(max(initial), 1000)
        self.assertLess(max(regular), 50)


if __name__ == '__main__':
    unittest.main()
