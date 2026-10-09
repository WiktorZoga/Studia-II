# Bronze: regular-match baseline

Category: enhancement
Status: ready-for-agent

## Problem Statement

The student reached Bronze and reports that the bot waits without moving.
The existing policy was designed for a fixed bomb tutorial. Bronze uses regular
territory and combat objectives, so the assumptions that made the tutorial
solution sufficient now suppress useful actions.

## Solution

Plan a simple regular-match policy for every surviving owned agent. Choose
movement for territory, combat opportunities, and survival; select shooting,
throwing, or hunkering according to current resources and position. Reuse the
working protocol parser and single-file C++20 delivery. This request produces
the specification only, with no implementation or claim of a stronger rank.

## User Stories

1. As the student, I want the differences from the bomb tutorial explained.
2. As a player, I want agents to act on shared battlefields with allies and enemies.
3. As a player, I want useful movement when no attack is immediately available.
4. As a player, I want to gain and retain more territory than the opponent.
5. As a player, I want wetness penalties included in territory evaluation.
6. As a player, I want each agent's own range, power, cooldown, and bombs respected.
7. As a player, I want ready agents to shoot viable enemy targets.
8. As a player, I want useful partial bomb attacks without requiring an entire group.
9. As a player, I want neither bombs nor movement plans to harm my team unnecessarily.
10. As a player, I want defensive actions when shooting is unavailable or unsafe.
11. As a player, I want movement even after bomb inventory is exhausted.
12. As a player, I want decisions to adapt to casualties and changed positions each turn.
13. As a player, I want simultaneous enemy actions considered as uncertainty.
14. As the student, I want small implementation steps and independently checked results.

## Implementation Decisions

- Target Bronze explicitly. Bomb possession is a resource, not a league identifier.
  Replace the tutorial-only active policy; preserve earlier tutorials as separate
  historical requirements rather than keeping their constraints in Bronze.
- Keep the parsed setup/turn fields. The new PDF now includes the expanded
  protocol and confirms the existing field layout. Support all observed agents,
  either player ID, arbitrary agent IDs, and changing surviving team sizes.
- Remove whole-floor-region enemy exclusion, the three-bunker objective, and
  mandatory full-group bomb coverage from regular-match decisions. Teammates
  and enemies can share connected terrain. Evaluate useful local attacks instead.
- Calculate tile control using the nearest surviving agent's effective Manhattan
  distance: normal distance below 50 wetness, double distance at or above 50.
  Cross-team ties are neutral. Do not substitute path length for scoring distance.
  Score advantage accrues by the excess number of controlled tiles each turn.
- Use a small deterministic greedy baseline. Compare legal positions by territory
  contribution, viable attacks, cover, and exposure. Coordinate friendly
  destinations; allow purposeful holding, but avoid waiting caused solely by
  unavailable bombs, absent tutorial groups, or nonexistent cover improvements.
- Consider all relevant enemies, not a hard limit of the closest two. For SHOOT,
  require zero current cooldown and post-move range at most twice optimal range.
  Include power, range attenuation, cover, and possible hunkering in evaluation.
- Evaluate THROW only with remaining bombs, Manhattan range at most four, and
  a useful 3-by-3 splash. Whole-group elimination is not required. Conservatively
  avoid friendly blast damage in the first baseline and coordinate expenditures.
- HUNKER_DOWN is a viable combat choice, including during shooting cooldown.
  Its 25-percentage-point shooting protection adds to cover, but bombs ignore both.
  Movement and combat are separate choices; zero ammunition must not stop movement.
- Respect movement then hunkering then combat then casualty removal. Evaluate
  attacks from the planned next position. Enemy movement/hunkering is not supplied
  before decisions; observed positions are a baseline estimate, not a guarantee.
- Preserve one explicit-ID line per owned surviving agent, at most one move and
  one combat action per line, optional message, semicolon separation, and flushing.
  Stay within 50 ms per regular turn and 1000 ms for the first turn.

## Testing Decisions

Reuse the already approved executable stdin/stdout interface, including open
stdin while awaiting command lines. Test externally visible decisions against
worked rule examples. Retain existing map/cover tests; previous tutorial target
choices belong to their stage and must not constrain Bronze policy.

Acceptance criteria for later implementation:

- A shared, connected map with bombs does not trigger the bunker tutorial policy.
- Where a safe useful advance exists and no attack is in range, the bot moves.
- Agents with zero bombs or positive shooting cooldown still make legal movement
  and/or defensive decisions; no stale target IDs after casualties.
- Cooldown blocks SHOOT. Range tests include optimal, twice-optimal, and just beyond.
- Cover/hunker stacking and bomb immunity to those reductions match the rules.
- Safe partial splash opportunities can be considered without killing every enemy.
- Territory examples cover wetness 49 versus 50, cross-team ties, and both player IDs.
- Team commands avoid obvious destination collisions and friendly splash damage.
- Command counts and syntax remain valid across changing inventories and team sizes.
- Normal-match fixtures fit the captured map/agent limits and time budget.
- Platform runs establish actual competitive behavior; local tests alone do not
  establish victories or promotion.

## Out of Scope

Implementation or test changes during this request; ticket creation; guaranteed
promotion; advanced search, neural networks, a full engine, or broad refactoring;
publishing or submitting code. A functional greedy baseline comes before tuning.

## Further Notes

Sources: [Bronze statement](../../soak-overflow/protocols/05-bronze/statement-v01.pdf),
[bomb statement](../../soak-overflow/protocols/04-bombs/statement-v01.pdf), and
[the rule/code comparison](../../soak-overflow/protocols/05-bronze/notes.md).

The capture includes the complete input/output section. It supplies no score or
opponent-action fields; current territory can be calculated, while cumulative
score estimates need correct turn accounting. No real Bronze input/replay or
new starter was supplied, so the reported idle turn was explained from the code
and rules rather than reproduced against that exact match.

The PDF says a 600-point lead wins; the official referee checks a lead strictly
greater than 600. Damage rounding and cross-team territory ties were checked
against the referee. Exact win thresholds, score ties, and opponent-action
predictions should not be silently assumed when later modeling match outcomes.

The first functional slice should replace tutorial gating and restore legal
per-agent choices. Territory movement, combat ranking, and coordinated bombs
can follow as small separately verifiable steps. Weights and a win-rate/rank
target are not established by this specification.
