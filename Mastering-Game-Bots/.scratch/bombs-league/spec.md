# Splash-bomb tutorial

Category: enhancement
Status: ready-for-agent
Resolution: completed

## Problem Statement

The student advanced from covers to Objective 4. Shooting is disabled; the bot
must eliminate three enemy-only groups with limited bombs, avoid hitting either
owned agent, and finish before the 40-turn limit. Its current policy still shoots.

## Solution

Add safe bomb targeting, ammunition planning, and movement into throwing range.
Exclude unsafe blasts around the fourth, mixed group containing an owned agent.
Keep a single C++20 submission and verify the tutorial on CodinGame afterward.

## User Stories

1. As the student, I want small implementation steps with clear local checks.
2. As a player, I want to clear the three enemy-only groups to finish the tutorial.
3. As a player, I want both owned agents kept outside every planned blast.
4. As a player, I want throws restricted to legal range and available ammunition.
5. As a player, I want legal movement toward useful throwing positions.
6. As a player, I want both agents' bomb use coordinated without wasted throws.
7. As a player, I want valid commands every turn without shooting.
8. As the student, I want local results distinguished from platform promotion.

## Implementation Decisions

- Reuse map coordinates, ownership, surviving agents, wetness, and current bomb
  counts, which are already stored. Verify the new input format before assuming
  it is unchanged.
- Replace the active shooting policy for this tutorial. Throwing is a combat
  action and may follow movement in the same turn.
- Model 30 wetness on the landing tile and all eight neighbors, independent of
  cover. Reject any blast containing either owned agent, including the thrower.
- Check safety against both agents' planned final positions. Require positive
  current ammunition and a target within four tiles of the post-movement
  position, using the referee's confirmed Manhattan-distance rule.
- Identify enemy-only groups from the observed board, not hardcoded coordinates,
  IDs, or inventories. Allocate limited bombs using remaining enemy wetness;
  avoid redundant throws and treat the mixed group as outside the victory goal.
- Navigate through empty, unoccupied cells. Respect impassable cover and verify
  the actual next movement step before checking a combined throw.
- Preserve one explicit-ID output line per owned agent, per-turn flushing, and
  stderr diagnostics. Use the confirmed non-combat MESSAGE command when no
  throw or movement is appropriate.

## Testing Decisions and Acceptance Criteria

Reuse the already approved executable stdin/stdout interface. Keep stdin open
while awaiting commands; compare behavior against independently worked examples.
The existing protocol checks provide the test structure. No new test interface
requires approval.

Later implementation must demonstrate:

- Correct throwing syntax, IDs, line count, and timely output; no shooting.
- Inventory refreshed each turn; no throw with zero bombs.
- Correct center/orthogonal/diagonal splash hits and no damage beyond the blast;
  cover does not reduce bomb damage.
- No blast affects either owned agent, including after either agent moves.
- Confirmed range boundaries and legal movement around cover and occupied cells.
- Coordinated ammunition sufficient for three enemy-only groups in representative
  layouts; the mixed group's enemies are not a fourth required objective.
- Existing map/cover and output regressions remain useful; old shooting-policy
  expectations are historical stage behavior, not the new tutorial's requirement.
- CodinGame completion is verified before claiming success or promotion.

## Out of Scope

Later leagues, leaderboard strategy, advanced search, a full simulator,
publishing, or submitting code. The original specification-only request was
subsequently followed by explicit authorization to implement this tutorial.

## Further Notes and Missing Information

Sources: [new statement](../../soak-overflow/protocols/04-bombs/statement-v01.pdf)
and [previous covers statement](../../soak-overflow/protocols/03-covers/statement-v01.pdf).
The new PDF contains rules and victory/failure conditions but ends at the
Initialization Input heading.

The capture still lacks the displayed league name, expanded protocol, and a real
input sample. Before implementation, the [official-source checks](rules.md)
confirmed serialization, throw distance, target legality, idle commands, time
limits, initial inventories, and the tutorial's group layout. The parser is
unchanged; it matches the official serializer.

The baseline now plans whole-group throws and legal one-step routes, excludes
floor regions containing owned agents, and coordinates the two agents' actions.
It deliberately targets this tutorial; future league behavior is outside scope.
