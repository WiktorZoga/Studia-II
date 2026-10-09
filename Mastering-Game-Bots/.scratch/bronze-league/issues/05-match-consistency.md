# 05: Keep the Bronze baseline correct across changing turns

Status: ready-for-agent
Claim: resolved
Resolution: completed
Approved: user confirmed the five-ticket breakdown
Category: enhancement

**What to build:** The integrated baseline adapts its team commands as agents
move, cooldowns expire, bombs are consumed, and agents are eliminated. Deliver
a reviewed, locally checked submission ready for the student's platform run.

**Blocked by:** 04: Use useful splash bombs without harming the team.

## Acceptance criteria

- [x] Multi-turn executable replays refresh positions, cooldowns, inventory,
  wetness, and surviving agents without stale targets or destination reservations.
- [x] Every turn emits exactly one valid explicit-ID line per owned survivor,
  even when input order changes or agents disappear; stdout is flushed promptly.
- [x] Scenarios across the captured map and agent limits retain legal movement,
  territory-aware choices, valid combat, and conservative friendly safety.
- [x] Measure representative local turn handling against the 50 ms regular-turn
  and 1000 ms initial-turn budget, recording local results rather than assuming
  local and CodinGame environments have identical performance.
- [x] Run the complete applicable local checks, review standards and specification,
  and save only the authorized work in a local commit.
- [x] Explain the baseline choices and provide the single-file C++20 submission
  for a CodinGame test run; record platform results and promotion as pending until
  the student reports them.

## Scope boundary

No guaranteed promotion, broad simulator, benchmark infrastructure, advanced
search, publication, or automatic account submission. Fix only problems exposed
by the integrated baseline's checks.

## Answer

Completed refreshed-state and casualty checks, exact-ID command validation, 120 generated limit/timing turns, the full local suite, and independent standards/spec reviews. The single-file C++20 code is committed locally; no code was published. Platform execution and promotion remain pending user observation.

Validation and review: [implementation notes](../README.md) and [review](../review.md).
