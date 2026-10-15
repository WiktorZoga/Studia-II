# Assignment 2: Evaluation and local testing

**Due November 4, 2026, 14:00. Maximum: 2 points + 1 extra point.**
Authoritative requirements: [Assignments, page 2](../Assignments.pdf#page=2).
Explanation and unresolved wording: [Lecture 2 notes](../docs/course/lecture-2.md).

- [ ] Prepare local game testing with the supplied benchmarker or another tool.
- [ ] Implement an evaluation function or heuristic action selection.
- [ ] Use local matches to test evaluation ingredients and tune weights.
- [ ] Demonstrate at least three nontrivial versions, each confidently winning
  at least 60% against the previous best.
- [ ] Reach Silver League.
- [ ] Optional bonus: reach Golden League.

## Starting point

The C++20 bot already has heuristic action selection: territory, expected shooting
damage, defense, and safe bombs. Its weights are unmeasured. This is a useful
starting baseline; it is not yet evidence that Assignment 2 is complete.

The supplied [benchmarker](../benchmarker-generic-2026.10.13/) has been inspected
but not integrated or run. A full local game needs the official Java referee,
which advances the game and exchanges input/output with the C++ executables.
The benchmarker schedules those games and compares results. We do not need to
write our own game engine to run these matches.

## Next work

The investigation and proposed plan are kept locally in
`.scratch/assignment-2/plan.md` relative to the course root. These skill records
are ignored by Git and may be absent from a fresh checkout.
Begin with a repeatable baseline-versus-baseline match, then try one strategy
change at a time. Conservatively plan for a baseline plus three successful
improvements; keep unsuccessful experiments in the record too.

Open questions for the instructor concern confidence, draws, version counting,
and submission format. Their answers affect what evidence is sufficient.
Silver must be confirmed on CodinGame; local wins alone do not prove promotion.

Status: investigation and planning only. No Assignment 2 matches or improvements
have been claimed, and no submission has been made.
