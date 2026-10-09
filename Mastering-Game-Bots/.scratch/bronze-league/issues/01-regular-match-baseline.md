# 01: Restore useful actions in regular matches

Status: ready-for-agent
Claim: resolved
Resolution: completed
Approved: user confirmed the five-ticket breakdown
Category: enhancement

**What to build:** A working Bronze baseline that chooses legal actions for
every owned surviving agent on a shared battlefield. Possessing bombs no longer
selects the bunker tutorial. The bot can advance toward opponents, shoot a legal
target, or defend, even when bombs are absent or exhausted.

**Blocked by:** None (can start immediately).

## Acceptance criteria

- [x] Bronze uses a regular-match policy with both positive and zero bomb inventory.
- [x] Enemies remain eligible when they share connected floor terrain with allies.
- [x] With no target in range and a legal useful approach available, an agent moves;
  purposeful holding remains allowed when movement is blocked or unnecessary.
- [x] Basic shooting requires an enemy still alive, zero current cooldown, and
  distance within twice that agent's optimal range from its planned next position.
- [x] Shooting cooldown does not prevent movement or HUNKER_DOWN.
- [x] Moves avoid cover, occupied cells, and obvious friendly destination conflicts.
- [x] Commands cover exactly the owned surviving IDs, flush each turn, and preserve
  the single-file C++20 submission and existing numeric input format.
- [x] Open-stdin executable examples demonstrate shared terrain, exhausted bombs,
  cooldown, variable team size, shuffled IDs, and both player IDs.
- [x] Keep earlier tutorial fixtures as historical stage expectations; reuse their
  general formatting and map checks without requiring tutorial strategy in Bronze.

## Scope boundary

Use simple deterministic target and approach choices. Territory optimization,
damage-based target ranking, and bomb-versus-shot selection arrive in later tickets.
Any small policy separation needed for this behavior belongs in this ticket;
do not introduce a separate broad refactor or simulator.

## Answer

Replaced tutorial gating with a regular-match policy. Shared terrain, empty inventory, cooldown, arbitrary IDs, both players, legal moves, and live targets are covered by the executable checks. Previous tutorial fixtures remain as historical expectations.

Validation and review: [implementation notes](../README.md) and [review](../review.md).
