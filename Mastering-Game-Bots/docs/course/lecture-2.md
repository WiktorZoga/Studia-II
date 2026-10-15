# Lecture 2: evaluation and local testing

Sources: [Lecture 2](Lecture%202.pdf) and [Assignments, October 14, 2026](../../Assignments.pdf).
Page numbers below count PDF pages. These local notes summarize the sources;
the originals remain authoritative. The lecture prohibits sharing its slides,
nonpublic tools, and nonpublic bot details outside the University of Wrocław
([lecture, p. 2](Lecture%202.pdf#page=2)). Keep these course materials local.

## Assignment 2: exact requirements

**Due November 4, 2026 at 14:00, at the end of the lab. Maximum: 2 points
plus 1 extra point.** [Assignment 2, p. 2](../../Assignments.pdf#page=2) requires:

1. Prepare local game testing using the SKOS **Benchmarker (generic) 2026**,
   or another tool of your choice.
2. Implement an evaluation function, or heuristic action selection where applicable.
3. Locally test evaluation ingredients and tune weights. Show **at least three
   nontrivial versions**, each confidently winning **at least 60%** against the
   previous best version.
4. Reach **Silver League**. Reaching **Golden League** earns the extra point.

The earlier assignment focused on choosing a game, reading post-mortems,
reaching Bronze, discussing ideas, and submitting profile/game links. It remains
due October 21 at 14:00; the new assignment adds measured improvement and local
testing rather than merely another league promotion
([Assignment 1, p. 1](../../Assignments.pdf#page=1)).

## What the lecture adds

**Run full games locally.** Combine the official game referee with the supplied
benchmarker and add bot executables. Check whether referee static variables
need resetting between games and whether order-dependent hash collection
iteration introduces nondeterminism. The lecture demonstrates increasing local
timeouts through `GameManager` and `Referee`; those are testing settings,
not CodinGame submission limits. Alternative testing tools are allowed
([lecture, pp. 3-4](Lecture%202.pdf#page=3)).

**Separate decisions, simulation, and evaluation.** A rule-based bot can be an
initial baseline. Search requires a game engine that applies actions accurately
and efficiently, plus a function that scores resulting states. Evaluations can
compare components in priority order or combine them with weights; overlapping
components can accidentally count the same benefit twice
([lecture, p. 5](Lecture%202.pdf#page=5)).

**Keep state and actions compact.** For simultaneous games, model actions for
both players and their combined effect on state. The lecture recommends small,
fixed-size representations, attention to padding, and leaving advanced encoding
optimizations for later ([lecture, pp. 6-7](Lecture%202.pdf#page=6)).

**Test features instead of trusting intuition.** Suggested evaluation ingredients
include score, territory, resources, living agents, health, mobility, ammunition,
cooldowns, and vulnerability. Start with the most important features, watch
computation cost, try small weights, and allow individual features to be switched
off. Their usefulness can change with game phase or other improvements
([lecture, p. 9](Lecture%202.pdf#page=9)). The SnakeByte distance maps illustrate
that movement assumptions change reachability; the Cellularena maps illustrate
Voronoi territory. Those game-specific mechanics are examples, not Soak Overflow
rules ([lecture, pp. 10-16](Lecture%202.pdf#page=10)).

## Applying this to our bot: recommendations

Keep C++20 and the single-file CodinGame submission. First establish a local
referee-based match loop and preserve the current bot as a baseline. Then change
one meaningful evaluation ingredient at a time, compare complete games, and
record which versions actually improve. Heuristic action selection is explicitly
allowed, so a custom simulator or advanced search is not a prerequisite for this
assignment. Consider one later only if the measured limitations justify it.

For repeatable evidence, retain version identifiers, build settings, opponent,
seeds, player sides, wins/draws/losses, and timeout or invalid-action counts.
Use separate tuning and confirmation games. These are our proposed experiment
practices, not additional requirements stated by the assignment.

## Information still missing

- What statistical confidence and sample size the instructor expects for the
  60% requirement; how draws count.
- Whether three versions means three improvements beyond a preserved baseline.
  Plan conservatively for a baseline plus three successful improvements.
- Required submission format and evidence for Assignment 2, including league proof.
- Exact interpretation of a "nontrivial" version and whether weight changes alone
  qualify.

The PDF does not specify these details. Confirm them with the instructor before
claiming the assignment complete; do not invent a confidence threshold or count
unmeasured changes as successful versions.
