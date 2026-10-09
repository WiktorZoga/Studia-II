# Small-step development plan

Work in milestone order rather than assigning calendar dates: the student's
weekly time budget is not established. The immediate goal remains Bronze
League, with an explanation and verification after each small change.

## How local execution works

The C++ compiler builds the bot as an ordinary command-line executable.
It reads the game's text input from stdin and writes command lines to stdout.
Our Python checks supply small synthetic turns and inspect the actual commands.
These checks need a compiler and a script, not a running CodinGame match.

For cover geometry, a small C++ test executable calls the same public helpers
that the bot will use. It supplies a small map and shooter/target positions,
then compares the result with known examples from the statement and referee.
The current environment has Clang 17, CMake, and Python, and already runs the
seven turn-output checks.

No complete local game engine is installed. These tests verify selected rules,
parsing, and output; CodinGame still verifies the tutorial and actual promotion.
Later, a Java referee can support local matches and benchmarking if needed.

## Milestones

| Order | Deliverable | Verification | State |
| --- | --- | --- | --- |
| 1 | Lookup map tiles and evaluate target cover against a shot | Public helper tests plus existing turn-output checks | Implemented; local checks pass |
| 2 | Select a reachable protected position and a less-protected target | Worked cover scenarios and combined-command checks | Proposed; depends on 1 |
| 3 | Produce MOVE and SHOOT in one line per owned agent, then verify covers on the platform | Local output checks, then student's CodinGame run | Proposed; depends on 2 |
| 4 | Capture the next statement and repeat the comparison/implementation cycle toward Bronze | Versioned statement, stage notes, local tests, platform result | Proposed; depends on promotion |

Post-mortem reading and instructor discussion remain Assignment 1 tasks in
parallel with development. A full simulator, search algorithms, and performance
benchmarking are deferred until the tutorial milestones are understood.

## Skills

- `grill-with-docs`: settle a new design decision and record shared terminology.
- `to-spec`: turn agreed requirements into a written specification.
- `to-tickets`: propose small, independently verifiable tasks with dependencies;
  publish separate local tickets after their breakdown is agreed.
- `implement`: complete one approved task, using TDD where appropriate.
- `diagnosing-bugs`: reproduce a platform failure with a local check and fix it.
- `code-review`: check the resulting changes against standards and the scope.

The recommended workflow is one approved task at a time. The remaining rows
are a proposed schedule, not authorization to complete the entire tutorial.
