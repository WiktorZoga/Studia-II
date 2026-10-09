# Covers implementation review

Fixed point: `fc2e9c5abfc64f647ad7e6ad90af5f177ee565cb`.
Reviewed implementation commit: `227a0eb`.
Command: `git diff fc2e9c5abfc64f647ad7e6ad90af5f177ee565cb...227a0eb`.
Specification: [spec.md](spec.md).

## Standards

Zero hard documented violations. Two low-priority duplication findings:
enemy ownership/survival filtering was repeated between policies, and several
tests repeated the same layout and turn fixture. Cleanup introduced a shared
surviving-enemy helper and a named test fixture. The scoped code still uses
one C++20 submission file and preserves the earlier floor-only behavior.

## Spec

Zero findings. The implementation chooses protected, legal neighboring tiles,
selects nearby targets from the destination, checks range, and uses the specified
tie-break for decorative cover. It emits both actions in one explicit-ID line
per owned agent. No future-league strategy or simulator was added.

## Verification

Both CTest suites pass after cleanup: cover geometry and executable protocol.
The latter includes 64 cover-layout combinations, ID/order variations,
occupied/blocked destinations, range after moving, and earlier regressions.
Platform result and promotion remain pending.

Standards: 2 minor findings corrected; worst was duplicated enemy filtering.
Spec: 0 findings; no issue identified.
