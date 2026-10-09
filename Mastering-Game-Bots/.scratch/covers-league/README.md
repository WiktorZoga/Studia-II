# Covers league work

The student requested a working covers submission through very small local
tickets. The [spec](spec.md) defines the scope; the tickets are implemented
in dependency order.

| Ticket | Result |
| --- | --- |
| [01: protected movement](issues/01-protected-movement.md) | Both agents move to stronger cover |
| [02: cover-aware target](issues/02-cover-aware-target.md) | Each agent selects a nearby enemy from its new position |
| [03: layout verification](issues/03-layout-verification.md) | Local tests pass; final review in progress |

The bot sends combined movement/shooting commands. Local checks cover 64 base
layout/decorative-cover combinations, shuffled IDs, occupied or impassable
destinations, range after moving, and previous output/cover behavior.

Paste the current `soak-overflow/main.cpp` into CodinGame's C++20 editor.
Local tests exercise the real executable; they do not run a complete match.
The covers platform result and promotion remain pending until reported.
