# 02: Move to improve territory control

Status: ready-for-agent
Claim: resolved
Resolution: completed
Approved: user confirmed the five-ticket breakdown
Category: enhancement

**What to build:** Agents compare legal positions by their team's territory
advantage, so movement can improve the score even without an immediate attack.
This extends the working regular-match baseline rather than creating a detached
scoring helper that never affects commands.

**Blocked by:** 01: Restore useful actions in regular matches.

## Acceptance criteria

- [x] Tile control compares the nearest surviving agent on each team using
  Manhattan distance; at wetness 50 or above, distance to that agent is doubled.
- [x] Cross-team ties are neutral. Evaluate map tiles independently of walkable
  floor regions; do not use pathfinding distance as the territory metric.
- [x] Candidate owned moves affect the territory comparison and the emitted MOVE
  command in independently worked examples with a clear better position.
- [x] Use controlled-tile advantage as a movement value; do not assume scores are
  directly provided in the input or claim an unverified cumulative score.
- [x] Legal movement and friendly destination reservations from ticket 01 remain
  in force. A flat territory comparison can retain the baseline's useful approach.
- [x] Executable examples cover wetness 49/50, neutral ties, healthy versus wet
  agents, casualties, both player IDs, and movement while out of ammunition.

## Scope boundary

Use a small deterministic greedy comparison. Combat-aware trade-offs extend
this position assessment in ticket 03; search and weight tuning are not required.

## Answer

Movement evaluates all tiles using effective Manhattan distance and neutral ties. Worked executable examples distinguish wetness 49/50 and counting cover tiles. Current survivors and prior team destinations feed the comparison; no supplied or cumulative score is assumed.

Validation and review: [implementation notes](../README.md) and [review](../review.md).
