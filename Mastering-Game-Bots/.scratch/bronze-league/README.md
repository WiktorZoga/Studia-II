# Bronze implementation

The five [approved tickets](ticket-plan.md) implement the
[specification](spec.md) in [main.cpp](../../soak-overflow/main.cpp), a single
C++20 file for the CodinGame editor. Local checks pass; a platform run and
competitive result are pending.

The small development steps were:

1. Replace bomb-tutorial gating with regular-match movement, shooting, and defense.
2. Compare positions by controlled-tile advantage, using Manhattan distance,
   neutral ties, all map tiles, and the wetness-50 distance penalty.
3. Add range, power, cover, rounded shot damage, and shooting exposure to position
   choices. Hunkering adds 25 percentage points of protection.
4. Fix the team's destinations, then compare safe partial splash attacks with
   shots and defense. Protect both original and planned friendly positions so
   cancelled moves cannot expose teammates to our bombs.
5. Check changing resources, positions, survivors, sparse/shuffled IDs, both
   player IDs, board/team limits, and local turn handling.

Position value is tile advantage plus twice the best attack/defense benefit,
minus twice incoming shooting damage. Attack benefit caps useful damage at
remaining wetness and adds 20 for an estimated elimination. These are simple
heuristics, not game scoring rules or tuned weights. Equal-value moves use
a path-based approach as a tie-break; territory itself never uses path distance.

Agents plan in increasing ID order. Each turn rebuilds movement reservations
and an estimated enemy-damage ledger to reduce duplicate expenditure. Actual
opponent movement and hunkering are unknown: current positions are estimates,
and a planned lethal attack does not remove incoming same-turn exposure.
The bot can hold or hunker for a tactical reason; inventory alone never idles it.

## Local validation

The [test instructions](../../soak-overflow/tests/README.md) explain the active
checks and retained historical tutorial fixtures. Red tests demonstrated the
old shared-terrain inactivity, wetness-threshold movement error, covered-target
choice, exposure-blind advance, and missing partial splash before each change.

The complete CTest run passed both targets: 12 map/cover cases and 17 Bronze
executable tests. Generated input states cover 120 turns across the captured
board/team limits; they are not a full game engine. Debug-build send/read/check
wall time on this Mac was 1.326 ms median and 5.241 ms maximum for 114 regular
samples; six first-turn samples peaked at 5.906 ms. This includes Python-driver
overhead and does not guarantee identical CodinGame timing.

## Handoff

Copy [main.cpp](../../soak-overflow/main.cpp) into CodinGame's C++20 editor and
run a regular Bronze match. Local rules checks do not establish a win or promotion.
Review results and completion evidence are recorded in [review.md](review.md).
