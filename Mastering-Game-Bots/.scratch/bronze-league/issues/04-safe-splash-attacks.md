# 04: Use useful splash bombs without harming the team

Status: ready-for-agent
Claim: resolved
Resolution: completed
Approved: user confirmed the five-ticket breakdown
Category: enhancement

**What to build:** Agents can choose a worthwhile safe bomb over their shooting
or defensive option, including partial attacks on scattered enemies. Decisions
use current inventory and both agents' planned positions, without bunker rules.

**Blocked by:** 03: Balance shooting opportunities and survival.

## Acceptance criteria

- [x] THROW requires positive current inventory and Manhattan distance at most
  four from the planned post-movement position.
- [x] Evaluate 30 wetness on the landing tile and its eight neighbors; cover and
  hunkering do not reduce splash damage.
- [x] A useful partial hit can be selected without covering an entire floor
  region, clearing a whole group, or owning enough bombs to kill every opponent.
- [x] Compare expected bomb benefit with ticket 03's shooting/defense options;
  avoid unnecessary expenditure where the simpler option is sufficient.
- [x] Conservatively reject any blast affecting an owned agent at its planned
  position, including the thrower, and prevent later friendly moves into it.
- [x] Coordinate damage/expenditure to avoid redundant bombs on targets already
  sufficiently covered by planned team attacks; recompute reservations each turn.
- [x] Losing the last bomb never stops movement, shooting, or defense.
- [x] Executable examples cover partial clusters, diagonal splash, range four/five,
  zero inventory, friendly positions after movement, and duplicate expenditure.

## Scope boundary

Use the existing blast and movement rules with a simple greedy choice.
No new stage inference based on bomb supply, multi-turn bomb search, or tutorial
group identification belongs in the active Bronze policy.

## Answer

Combat choices support useful partial 3x3 splashes at range four, with positive current inventory. All team destinations are fixed before blasts; both old and planned friendly cells are protected. A per-turn estimated damage ledger avoids duplicate expenditure, and a sufficient shot wins a tie to preserve bombs.

Validation and review: [implementation notes](../README.md) and [review](../review.md).
