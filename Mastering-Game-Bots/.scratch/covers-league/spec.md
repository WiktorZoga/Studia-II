# Covers tutorial

Category: enhancement
Status: ready-for-agent

## Problem Statement

The bot reaches the shooting tutorial but does not yet perform the movement
and cover-aware shooting required by the next league. The student wants small
local tickets implemented in order, ending with a pasteable C++20 submission.

## Solution

For each owned agent, inspect free neighboring tiles, select the position with
the best protection against its two nearby enemies, then shoot a less-protected
nearby enemy from the destination. Send both actions on one command line.

## User Stories

1. As the student, I want each change in a small ticket with a clear result.
2. As a player, I want both agents to move to the best available cover.
3. As a player, I want cover tiles and occupied tiles excluded as destinations.
4. As a player, I want each agent to consider the enemies near its own position.
5. As a player, I want target protection evaluated from the position after moving.
6. As a player, I want each owned agent to send exactly one combined command.
7. As the student, I want local examples checked before running CodinGame.
8. As the student, I want the earlier shooting behavior retained on floor-only maps.

## Implementation Decisions

- Reuse the existing coordinate lookup and percentage cover evaluation.
- Examine cardinal, empty, unoccupied neighboring cells. Prefer a destination
  improving the minimum protection against the two closest enemies.
- Identify enemies through static ownership and current surviving IDs.
- For a move, select the least-protected of the two nearby enemies within
  shooting range. Evaluate protection from the new position. When reductions
  tie, prefer less protection from the original position, preserving the
  tutorial's distinction when decorative cover produces equal reductions.
- Keep the existing shooting policy when no better protected move exists.
- Keep the single-file C++20 submission and current input/output format.

## Testing Decisions

Use the already approved executable stdin/stdout interface for the new behavior.
Keep stdin open while checking commands. Literal expected positions and target
IDs come from the tutorial objective, not from recomputing the bot algorithm.
Work one failing example and small implementation at a time. Retain the direct
cover helper checks and previous command/ownership regression checks.

## Out of Scope

Future leagues, full-match simulation, search algorithms, benchmarking, external
publication, and claiming promotion without a CodinGame result.

## Further Notes

The supplied statement and the [official tutorial referee](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/src/main/java/com/codingame/game/TutorialManager.java)
define the covers objective. Local generated layouts are test fixtures, not a
complete game engine. The detailed exported protocol remains incomplete; the
parser's established field layout is unchanged by this work.
