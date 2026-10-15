# Stage 04: throwing splash bombs

## Source and coverage

- Snapshot: [statement-v01.pdf](statement-v01.pdf), supplied in this folder.
- Captured: October 9, 2026, according to the PDF metadata; newest available capture.
- The student reported advancement to a new league after the covers tutorial.
- Objective label: Objective 4, Throwing splash bombs. Displayed league name absent.
- Two image-based pages contain the goal, rules, victory/failure conditions, and
  a throw diagram. Detailed Game Protocol is missing: export ends at Initialization Input.
- No stage-specific starter or representative turn input has been supplied.

## Changes from covers

The one-turn movement/shooting objective becomes a 40-turn bomb objective.
Shooting is disabled. Four barricaded groups are present; one contains an owned
agent. Clear the three enemy-only groups using the limited bomb supply.

THROW targets a map coordinate within the stated distance of four tiles. It
adds 30 wetness to the landing tile and all eight adjacent tiles, regardless of
cover. Movement can precede throwing. Hitting any owned agent causes defeat.
Current bomb supply is provided per agent each turn.

## Comparison with the bot

The parser already stores initial and current bomb counts and current wetness.
The new decision policy uses floor regions to find enemy-only barricades,
plans full-group bomb coverage, checks both owned agents' safety, and walks
through empty unoccupied cells into Manhattan throwing range. It uses MESSAGE
to idle, emits no shooting in bomb mode, and reads current inventory each turn.

## Specification status

The local-only `.scratch/bombs-league/README.md` under the course root records
the spec, verified primary-source rules, and historical test results. The executable
stdin/stdout test interface was reused. At implementation time, sixty-four scripted
tutorial replays cleared all three enemy-only groups safely before the deadline. Platform
verification and promotion remain pending.

The local test suite was later retired; see the [protocol workflow](../README.md)
for the current checks and history pointer.
