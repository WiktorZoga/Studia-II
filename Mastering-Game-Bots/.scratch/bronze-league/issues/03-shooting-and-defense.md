# 03: Balance shooting opportunities and survival

Status: ready-for-agent
Claim: resolved
Resolution: completed
Approved: user confirmed the five-ticket breakdown
Category: enhancement

**What to build:** Agents combine territory-oriented movement with useful shots
and defensive choices. Their selected position and combat action reflect damage,
cover, cooldown, and exposure rather than simply targeting the wettest enemy.

**Blocked by:** 02: Move to improve territory control.

## Acceptance criteria

- [x] Compare all relevant live enemies using each shooter's range and power;
  do not restrict the regular-match policy to the nearest two opponents.
- [x] Estimate shooting damage from the planned next position: full power within
  optimal range, half up to twice optimal range, then directional cover reduction
  and the confirmed integer rounding. Never SHOOT during positive cooldown.
- [x] Integrate attack opportunity and exposure with ticket 02's position value;
  demonstrate a worked scenario where a tactically useful position is chosen.
- [x] Consider HUNKER_DOWN when defense is useful or shooting is unavailable.
  Its 25-percentage-point protection stacks with low/high cover as specified.
- [x] Opponent hunkering and movement are uncertain future choices, not fields
  secretly available in input. Do not assume killing an enemy cancels its combat
  action earlier in the same turn's resolution.
- [x] Retain one move and at most one combat action, current surviving IDs,
  useful movement without ammunition, and safe friendly destinations.
- [x] Executable examples cover optimal/twice-optimal/out-of-range boundaries,
  cooldown, a better damage target than the wettest target, cover and defense.

## Scope boundary

A small greedy movement/combat comparison is sufficient. No opponent search,
full engine, or promise of optimal tactics is needed. Splash bombs are added next.

## Answer

Position choices combine territory with useful shot damage and shooting exposure. All live foes are considered; range, cooldown, directional cover, integer rounding, and 25-point hunker stacking are checked. Enemy future actions are uncertain, and anticipated kills never suppress incoming exposure.

Validation and review: [implementation notes](../README.md) and [review](../review.md).
