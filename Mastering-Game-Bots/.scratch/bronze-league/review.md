# Bronze baseline review

Fixed point: `5eb23882d8e170291a169233df2cc38b6f16ef13`, the pre-change bot.
Reviewed command: `git diff 5eb23882d8e170291a169233df2cc38b6f16ef13...HEAD`.
Implementation commit: `e89a3f7` — Implement simple Bronze territory and combat baseline.
The two axes were reviewed independently by parallel agents.

Specification: [parent spec](spec.md) and all five [approved tickets](ticket-plan.md).
Standards: parent AGENTS.md, course lecture guidance, domain documentation,
protocol workflow, README, glossary, and the skill's Fowler smell baseline.
Unrelated user changes and existing untracked materials were excluded from review.

## Standards

No actionable findings. The change preserves the single-file C++20 submission,
course layout, explicit agent IDs, stderr diagnostics, and flushing between turns.
New Bronze tests use the required open-stdin process boundary; historical
tutorial checks remain available and their scope is documented. The functions
and small structs express distinct rules clearly. No material Fowler smell
warrants additional abstraction for this bounded baseline.

The stale root README and protocol summaries identified during implementation
were updated to describe the regular-match policy.

## Spec

No concrete acceptance or rule failures found. No missing implementation
requirements or scope creep were identified. Path-distance fallback stays
separate from Manhattan territory scoring. Protecting both old and planned
friendly positions is a reasonable conservative implementation.

Territory uses wetness-adjusted Manhattan distance with neutral ties; shooting
respects cooldown, range, cover, and integer rounding; defense retains enemy
exposure despite anticipated lethal attacks; bombs use planned destinations and
coordinated damage; commands refresh survivors and flush. Current enemy positions
and the damage ledger are estimates, consistent with the requested greedy policy,
and do not establish guaranteed kills or platform competitiveness.

Ticket bookkeeping and the platform handoff have now been completed locally.
Platform execution and results remain pending the student.

Findings: Standards 0 (no worst issue); Spec 0 (no worst issue).

## Validation

Full CTest suite passed: cover_geometry (12 cases) and codingame_protocol
(17 Bronze executable tests, including 120 generated limit/timing states).
The warning-enabled Debug C++20 build and `git diff --check` passed.
Local timing and limitations are recorded in the [implementation notes](README.md).
