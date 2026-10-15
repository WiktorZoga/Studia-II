# Soak Overflow: ideas from player post-mortems

Read after the [bot README](README.md). These are ideas for future work, not
features already added to our bot. Sources were read on October 11, 2026;
placements refer to the July 2025 contest, not today's multiplayer ranking.

## Useful reads

**Vyn — around 100th place, Legend.** A relatively simple approach: generate
selected movement/combat combinations, reward territory and useful damage, and
avoid bomb threats. The author also explains weaknesses, including missing cover
handling and overly restrictive move selection.
[Read the author's write-up](https://evalyte.net/notes/codingame-summer-challenge-2025/).

This is a good first read because its decisions resemble ours. We already handle
cover; enemy bomb danger and comparing team moves together are useful differences.

**bowwowforeach — 1st place.** Used DUCT search with handcrafted evaluation,
accelerated territory calculation with bitboards, and tuned parameters against
older bot versions. Generally preferred bombs guaranteed to hit, with an exception
for bomber-versus-bomber encounters.
[Read the author's post](https://forum.codingame.com/t/summer-challenge-2025-post-mortem/207074/13).

The immediate lesson is to examine reliable bomb placement. The search and
optimization machinery can wait until we can simulate and compare versions.

**zasmu — 3rd place.** Optimized plans for both sides, keeping several possible
opponent replies. Evaluation included resources, distances, and agent roles;
precomputation and caching sped up simulation.
[Read the author's post](https://forum.codingame.com/t/summer-challenge-2025-post-mortem/207074/10).

For us, the useful idea is comparing a move against several enemy responses
instead of assuming enemies stay still.

**gaha — 4th place.** Used a genetic algorithm to find promising team action sets,
then beam search over future turns. Evaluated enemy replies pessimistically and
included territory, health, bombs, and distances.
[Read the author's post](https://forum.codingame.com/t/summer-challenge-2025-post-mortem/207074/19).

The useful starting point is evaluating the whole team's plan. A large search
algorithm is a later step.

**viewlagoon — 6th place.** Used one-turn simulation and hill climbing over plans
for both teams. Damage evaluation included health thresholds and agent roles;
bomb decisions considered movement, collisions, allies, and ammunition value.
[Read the author's article](https://qiita.com/viewlagoon/items/62e3e4a1ae6e7322eab6) (Japanese).

For us, this suggests investigating the wetness-50 threshold and when to save a
bomb instead of taking a small immediate gain.

The [full CodinGame discussion](https://forum.codingame.com/t/summer-challenge-2025-post-mortem/207074)
and [official contest page](https://www.codingame.com/contests/summer-challenge-2025-coderpad-play)
provide context. Some forum permalinks failed in the browsing tool; their posts
were available on the full thread page. Vyn's placement is self-reported.

## What our bot already does

It evaluates territory, range, cover, cooldown, and useful damage; reserves
friendly destinations; and avoids blasting allies. It also limits redundant
attacks with an estimated damage ledger. These are already in `main.cpp`, so
adding them again would not be a new improvement.

## Small next steps

1. **Understand one decision.** Compare two positions by hand using our movement
   formula. Explain why one wins and what would change at wetness 50.
2. **Look at bomb reliability.** Draw an enemy's current cell and its four possible
   steps. A blast centered on its current cell covers all five positions; an
   offset blast may let it escape. This is a geometric inference from the rules,
   not a guarantee that our throw remains in range if our movement is cancelled.
3. **Consider enemy bombs.** Our movement score currently estimates incoming
   shots but ignores bomb threats. Ask whether spreading out or keeping distance
   would improve a specific situation.
4. **Compare versions before adding complexity.** Use the same maps from both
   sides and several opponents. Record wins, missed bombs, friendly damage, and
   point differences. A single win does not establish an improvement.
5. **Then consider joint-turn simulation.** This would let us compare team plans
   and possible enemy replies. Deeper search and weight tuning come after a
   correct simulation and useful measurements.

These are proposals based on the sources and our code, not proven improvements
for our bot. Contest results do not show which individual feature caused success.
For exact rules, use the saved [Bronze notes](protocols/05-bronze/notes.md) and
[official referee](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/src/main/java/com/codingame/game/Game.java).
