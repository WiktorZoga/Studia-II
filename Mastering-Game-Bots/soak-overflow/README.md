# Soak Overflow bot

A small C++20 bot for [Soak Overflow on CodinGame](https://www.codingame.com/multiplayer/bot-programming/soak-overflow).
It currently targets Bronze matches. The aim is to follow the rules, make useful
moves, and keep the code easy to understand.

## What is here

- **[main.cpp](main.cpp)** — the complete bot. This is the file to paste into CodinGame.
- **[RESEARCH.md](RESEARCH.md)** — player post-mortems and ideas worth exploring.
- **CMakeLists.txt** — optional local build configuration.
- **protocols/** — saved game statements and reference notes from earlier leagues.

## How it plays

The bot tries to gain territory while avoiding unnecessary damage. For every
agent, it compares staying still with the available neighboring tiles. It then
chooses between shooting, throwing a bomb, and hunkering down.

There are two passes each turn:

1. **Choose movement for the whole team.** Prefer positions with better territory,
   useful shooting opportunities, and less exposure to enemy fire. Avoid cover
   tiles, occupied cells, and destinations another teammate has already chosen.
2. **Choose combat from those positions.** Compare the damage from a shot or bomb
   with the damage avoided by hunkering. Keep track of planned enemy damage so
   teammates do not waste attacks on the same target.

Agents are considered in increasing ID order. Plans are rebuilt from fresh input
on every turn, including current cooldowns, bombs, wetness, and surviving agents.
Having no bombs does not prevent movement, shooting, or defense.

### Territory

Every map tile is controlled by the team with the closest agent. Distance is
Manhattan distance: horizontal steps plus vertical steps. At wetness 50 or more,
that agent's distance counts double. A tie between teams is neutral.

The bot prefers a larger difference between friendly and enemy controlled tiles.
For equally valuable positions, a floor-path distance helps it approach enemies
around walls. This path distance is separate from the game's territory distance.

### Shooting and defense

Shots require zero cooldown. They deal full power within optimal range, half
power up to twice that range, and nothing beyond it. Directional cover reduces
damage by 50% or 75%; hunkering adds another 25 percentage points of protection.

The bot compares every surviving enemy using estimated damage. It caps useful
damage at the target's remaining capacity before wetness 100 and adds a small
bonus for an expected elimination. It also estimates incoming shots and can
choose hunkering when avoiding damage is more valuable than attacking.

### Bombs

Bombs reach four Manhattan steps and add 30 wetness in a 3×3 area, ignoring cover.
A useful partial hit is enough; the bot does not need to clear an entire group.
It rejects blasts touching a teammate's original or planned position, including
the thrower. If a shot and bomb are equally useful, it saves the bomb.

## The scoring formula

Movement uses this simple score:

```text
territory advantage + 2 × (best shot or defense benefit − incoming shot damage)
```

The multiplier `2` and elimination bonus `20` are chosen heuristics, not game
rules. They have not been tuned against match results. Bombs are evaluated after
movement, rather than being included in this movement score.

## Where to start reading the code

Start with `main` to see input and output, then read `plan_actions` for the two
planning passes. The other main pieces are:

- `legal_positions` and `territory_advantage` for movement.
- `shot_damage`, `best_shot`, and `shooting_exposure` for shooting and defense.
- `best_splash` for bombs and friendly safety.

`GameSetup` holds fixed map and agent attributes. `TurnState` holds current
positions and resources. Agent IDs connect the two.

## Running it

On CodinGame, select C++20 and paste [main.cpp](main.cpp). The bot prints one
command per friendly survivor and flushes output before waiting for the next
turn. Diagnostics go to stderr.

For an optional local build, run these commands from this folder:

```sh
cmake -S . -B build
cmake --build build
```

The executable is `build/bot`. It reads CodinGame-format input from stdin;
building it locally does not launch a game. Generated build files are ignored.

## Current limitations

Enemy movement and hunkering are unknown, so attacks use their observed positions.
The bot does not yet evaluate enemy bomb threats, predict future turns, or search
all team actions together. Its damage ledger contains estimates, not guaranteed
hits. It can deliberately hold or hunker when that scores better than advancing.

See [the research notes](RESEARCH.md) for possible next improvements. Match
results are needed to judge whether a change makes the strategy stronger.
