# Soak Overflow bots

Each `.cpp` file is complete and can be pasted into the CodinGame C++17 editor. No headers, downloaded libraries, random search, or external files are needed at runtime.

| Engine league | CodinGame UI league | Source | Objective |
| --- | --- | --- | --- |
| 1 | Wood 4 | `training/wood1.cpp` | Move the two agents to (6,1) and (6,3). |
| 2 | Wood 3 | `training/wood2.cpp` | Both agents shoot the wettest enemy. |
| 3 | Wood 2 | `training/wood3.cpp` | Move one tile into the best cover and shoot the less protected of the two nearby enemies. |
| 4 | Wood 1 | `training/wood4.cpp` | Bomb the three enemy bunkers, preserving the friendly bunker. |
| 5 | Bronze and full game | `bot.cpp` | Contest territory and eliminate enemies. |

`bot_defensive.cpp` is a comparison baseline that gives incoming damage a higher penalty. It uses the same protocol and combat rules. Training file names use **engine league numbers**, which run in the opposite direction to the Wood labels displayed by the website.

## Build and test

From this directory:

```sh
clang++ -std=c++17 -O2 -Wall -Wextra -pedantic bot.cpp -o /tmp/soak-bot
clang++ -std=c++17 -O2 -Wall -Wextra -pedantic bot_defensive.cpp -o /tmp/soak-defensive
clang++ -std=c++17 -O2 -Wall -Wextra -pedantic training/wood4.cpp -o /tmp/soak-wood4
python3 -m unittest discover -s tests -v
```

Tests compile all six programs into a temporary directory and run real subprocesses. Fixtures check noncontiguous agent IDs, ownership from initialization, whitespace token parsing, vanished agents, command counts, early and partial EOF, cooldowns, shot range after movement, blocked movement, safe splash bombs, and tutorial targets. All supplied board coordinates and expected tutorial decisions are hand checked.

## Full game policy

The bot considers staying put and each free orthogonal neighbor. It scores projected shooting/bomb value, exposure to ready or nearly ready enemies, reachable distance to enemies, and distance to the board centre. It accounts for shooter range, low/high cover, wetness, cooldown and scarce bombs. It chooses a shot, splash bomb, or hunker action and may add a move. Ties are deterministic.

Splash bombs have Manhattan range 4 and affect the 3x3 neighborhood, including allies. Friendly original positions and planned destinations are protected, and subsequent allies cannot enter a previously selected blast area. Movement reserves all current occupied cells and previously selected destinations, avoiding swaps and expected collisions conservatively. Attack damage uses the official engine's directional cover rule, not line of sight blocking: covers are not opaque walls for shooting.

The parser reads whitespace-delimited integers, not fixed line boundaries. Ownership comes from the static agent table. Each fully read turn produces exactly one explicit-ID command line for every surviving friendly agent, sorted by ID. A truncated turn produces no partial output. Output is flushed once per turn.

## Limitations

The policy evaluates one turn and assumes enemy positions remain at their observed coordinates for outgoing attacks and bombs. It considers adjacent enemy positions for exposure, but cannot guarantee an enemy will stay in shot range or inside a splash footprint. It does not search joint team actions, simulate enemy decisions, or prove optimal territory control. The centre/distance term is a coarse territory proxy. Blocking all currently occupied cells sacrifices possible coordinated movement for predictable safety. No performance or rank claim follows from the fixture tests; the referee arena is needed to assess results across seeds and both player sides.

The Wood 2 tutorial target is chosen from the horizontal barricade on the approach side. Optional vertical cover between enemies can make actual projected damage tie after movement; the tutorial objective still requires the foe with the lower horizontal barricade. Regression fixtures cover both observed edge cases.

The Wood 1 solver explicitly recognizes the official tutorial's barricade rings and uses its fixed bunker layout. It is not a full game strategy.

Rules checked against the [official statement](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/config/statement_en.html.tpl), [combat engine](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/src/main/java/com/codingame/game/Game.java), and [tutorial objectives](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/src/main/java/com/codingame/game/TutorialManager.java). The local referee harness pins the tested engine revision separately.
