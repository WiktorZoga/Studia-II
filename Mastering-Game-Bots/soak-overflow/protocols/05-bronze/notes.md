# Stage 05: Bronze regular matches

## Source and coverage

- [statement-v01.pdf](statement-v01.pdf), five image-based pages, captured
  October 9, 2026 according to PDF metadata. All five pages were visually read.
- The student identifies this as Bronze and reports idle behavior.
- The export includes rules, diagrams, scoring, action order, victory conditions,
  initialization, turn input, output syntax, and constraints. Earlier exports
  lacked the expanded Game Protocol; this one includes it.
- No current starter or real match input/replay has been supplied.

## What changed from the bomb tutorial

| Aspect | Bomb tutorial | Bronze |
| --- | --- | --- |
| Goal | Clear three prescribed enemy-only groups | Win by territory points or eliminating the opposing team |
| Match length | 40-turn tutorial limit | Most points after 100 turns, or an earlier win condition |
| Combat | Bomb-focused objective; shooting disabled | SHOOT, THROW, and HUNKER_DOWN are available |
| Board assumption | Four enclosed groups, including one trapped friendly | The policy must handle shared terrain and moving opponents |
| Ammunition | Fixed tutorial supply sufficient for the three groups | Varies by agent; ammunition does not define the game mode |
| Friendly damage | Tutorial failure condition | Still physically dangerous; the baseline should avoid it |
| Enemy behavior | Tutorial-specific boss behavior | Both sides choose actions simultaneously |
| Scoring | Complete the tutorial objective | Territory advantage accumulates points each turn |

## Bronze rules that affect decisions

- Wetness of at least 100 eliminates an agent. Attributes differ by agent:
  optimal range, soaking power, shot cooldown, and initial/current bombs.
- Distances use Manhattan distance. Shooting deals full power within optimal
  range, half power up to twice that range, and fails beyond it. Shooting must
  respect the current cooldown; other actions remain available during cooldown.
- THROW reaches four tiles and deals 30 wetness in the landing tile and its eight
  neighbors. It damages friendly agents too and ignores cover and hunkering.
- Low cover reduces shooting damage by 50%; high cover by 75%, using the existing
  directional/adjacency rules. Only the strongest applicable cover counts.
- HUNKER_DOWN adds 25 percentage points of shooting protection for that turn:
  low cover plus hunker gives 75%; high cover plus hunker gives 100%. It does not
  make an agent immune to bombs.
- Cover tiles are impassable. A distant MOVE requests automatic pathing toward
  its coordinate; blocked or colliding movement can be cancelled.
- Resolve moves, then hunkering, then shots/throws, then removal of soaked agents.
  All agents participate in the same turn; do not assume a lethal shot prevents
  the enemy's combat action earlier in that same resolution.

## Territory and victory

For each map tile, compare the closest friendly and closest enemy using effective
Manhattan distance. At wetness 50 or more, the distance to that agent doubles.
Strictly closer friendly influence controls the tile; cross-team ties are neutral.
Pathfinding distance and impassable cover do not replace this scoring metric.

If one side controls more tiles, it gains the difference in counts as points
that turn. This makes positioning and preserving healthy agents relevant even
when no immediate kill is available. The statement lists winning by a 600-point
lead, eliminating all opposing agents, or leading after 100 turns.

Precision checked against the official [game logic](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/src/main/java/com/codingame/game/Game.java)
and [grid logic](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/src/main/java/com/codingame/game/grid/Grid.java):
cross-team nearest-distance ties are neutral, territory scoring follows casualty
resolution, positive shot damage uses rounded integer values, and the immediate
score-win check is strictly greater than a 600-point lead. This last boundary
differs from the statement's wording and must be kept explicit.

## Protocol comparison

The field layout matches the current parser:

- Setup: player ID; all agent definitions with ID, owner, cooldown attribute,
  range, power, and starting bombs; map dimensions and coordinate/type triples.
- Turn: surviving-agent count; ID, position, current cooldown, bombs, wetness;
  owned-agent count. There is no player-score or opponent-action field.
- Output: one line per owned agent; explicit ID; optional MOVE; one combat action;
  optional MESSAGE; semicolon separators. Keep IDs explicit rather than relying
  on the implicit ordering described in the statement.
- Limits: 50 ms regular turns, 1000 ms first turn; width 12-20, height 6-10,
  initial total agent count 6-10.

Numeric extraction handles whitespace, so no parser change is needed solely
because the PDF describes map triples as separate lines while serialization
can combine them into rows. A new field or input example would need comparison.

## Why the current bot can wait

Code references are relative to the course workspace:

1. `soak-overflow/main.cpp`, the `bomb_tutorial` branch, selects bomb mode whenever
   any owned definition has a positive starting bomb count. Bronze also supplies
   bombs, so this does not identify a tutorial.
2. `plan_bomb_actions` protects every floor region containing an owned agent,
   then excludes enemies in those regions. A shared connected battlefield can
   therefore leave no enemy groups to plan against.
3. It requires one blast to cover the entire group and enough bombs to finish
   it. Scattered, healthier regular-match enemies need not satisfy these conditions.
4. An agent with zero current bombs is skipped entirely by that planner. Its
   movement and shooting opportunities are never considered.
5. With no plan, the default destination stays unchanged and output is MESSAGE
   waiting. Bomb mode never falls back to the regular combat options.

Even the older non-bomb fallback is not a complete Bronze policy: it ignores
shooting cooldown in action selection, can choose a global wettest target outside
range, looks at only two enemies for cover choices, and does not evaluate territory.

This explains the reported behavior without claiming to have replayed the exact
idle match. It is a tutorial-policy mismatch; input parsing still matches the
captured protocol. The trapped-agent explanation from stage 04 does not explain
whole-team inactivity in regular matches.

## Specification and next scope

The local-only `.scratch/bronze-league/spec.md` under the course root describes a small
regular-match baseline and acceptance criteria. At specification time, executable
input/output checks were the agreed test interface. That documentation step
changed no code or tests and ran no build or game tests.

## Implementation follow-up

The original analysis above describes the bot before the Bronze implementation.
The student subsequently approved all five tickets and requested their
implementation. The active bot now uses regular-match territory, shooting,
hunkering, and safe partial splash decisions; initial bombs no longer select
a tutorial. The historical implementation and check records remain local in
`.scratch/bronze-league/README.md` under the course root.
The parser and statement snapshot remain unchanged. Results on CodinGame are
pending the student's test run.

The local test suite was later retired; see the [protocol workflow](../README.md)
for the current checks and history pointer.
