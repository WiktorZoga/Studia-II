# Bomb-tutorial implementation

The [specification](spec.md) is implemented as a bounded tutorial baseline.
[Verified official rules](rules.md) resolve the gaps in the supplied PDF.

The bot identifies enclosed floor areas and excludes any area containing an
owned agent. For each enemy-only area, it finds a throw covering the whole
group and walks toward a safe firing position through empty, unoccupied cells.
Bomb inventory and enemy wetness are read afresh each turn. Planned actions
reserve groups and blast locations, preventing duplicate bombs or friendly hits.

The mode is selected from the initial owned bomb supply: earlier tutorial
setups have no bombs. Once selected it remains active even when bombs run out,
so the bot never falls back to shooting in the bomb tutorial.

Local checks pass for immediate throws, combined movement/throwing, mixed-group
exclusion, duplicate throws, empty inventory, and 64 multi-turn tutorial replays.
The replay applies only movement, inventory, and splash damage to fixtures; it
is not a complete game engine. Earlier cover and output checks still pass.

Copy only the current `soak-overflow/main.cpp` into the CodinGame C++20 editor.
Platform completion and promotion remain pending until the student's test run.

The [two-axis review](review.md) found no specification problems or hard
standards violations. Two minor test-fixture findings were corrected.
