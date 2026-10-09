# Bomb tutorial review

Fixed point: `76bb74387befa54bbba657a67c03fa2a2a10be3f`.
Reviewed implementation commit: `ece6928`.
Diff: `git diff 76bb74387befa54bbba657a67c03fa2a2a10be3f...ece6928`.
Specification: [spec.md](spec.md).

## Standards

Zero hard documented violations. Two minor findings in test code: repeated
single-bunker fixtures and opaque numbered replay-state fields. Cleanup adds
a shared bunker fixture and a named ReplayAgent record. No new submission
dependencies or future-league strategy were introduced.

## Spec

Zero findings. The planner excludes friendly regions, selects full-group bomb
coverage, respects ammunition and post-move range, and reserves groups and
planned blast cells. Bomb-mode commands use MOVE, THROW, or MESSAGE, without
shooting, and are flushed every turn.

## Verification

The C++20 build and both CTest suites pass after the review cleanup. Tests cover
immediate throws, movement plus throwing, mixed-group safety, duplicate targets,
zero inventory, earlier tutorial regressions, and 64 scripted multi-turn bomb
layouts. Each replay clears three enemy-only groups before turn 40 while
preserving both owned agents. These are local rule checks, not a full game engine.

Standards: 2 minor findings corrected; worst was unreadable positional test state.
Spec: 0 findings; none identified. CodinGame completion and promotion are pending.
