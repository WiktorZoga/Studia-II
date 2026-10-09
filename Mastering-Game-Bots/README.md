# Mastering Game Bots

Course workspace for 2026/2027. Selected game: **Soak Overflow**.
Implementation language: **C++**.

- [Assignments](Assignments.pdf)
- [Lecture 1](Mastering%20Game%20Bots%20-%20Lecture%201.pdf)
- [Lecture 1 notes](docs/course/lecture-1.md)
- [Assignment 1 checklist](P1/README.md)
- [League statements and update workflow](soak-overflow/protocols/README.md)
- [Local issue workflow](docs/agents/issue-tracker.md)

## Layout

`P1/` holds Assignment 1 notes and checklists.
`soak-overflow/` holds the C++ project used throughout the course.
`soak-overflow/protocols/` holds named, versioned league statement snapshots,
reference starters, and notes about changes between stages.
`docs/course/` holds lecture summaries.
`.scratch/<effort>/issues/` holds local tickets.

## Local build

    cmake -S soak-overflow -B soak-overflow/build -DCMAKE_BUILD_TYPE=Debug
    cmake --build soak-overflow/build
    ctest --test-dir soak-overflow/build --output-on-failure

The executable targets Bronze regular matches. It compares legal positions by
territory and shooting exposure, then chooses a useful shot, safe splash, or
hunkering. Bomb inventory is a resource rather than a tutorial selector.
Commands include explicit agent IDs, and stdout is flushed before reading the
next turn. Diagnostics go to stderr. See the
[Bronze implementation notes](.scratch/bronze-league/README.md) and
[local checks](soak-overflow/tests/README.md).
Copy only `soak-overflow/main.cpp` into the CodinGame C++20 editor. Local
protocol tests pass; platform results still need confirmation from a game run.
The original starter and named statement snapshots remain references.

The project uses C++20, confirmed in CodinGame by __cplusplus=202002.
The captured Bronze statement specifies 50 ms per turn and 1000 ms initially.
Local macOS Clang is available;
the lecture recommends Linux and GCC for the eventual workflow.
