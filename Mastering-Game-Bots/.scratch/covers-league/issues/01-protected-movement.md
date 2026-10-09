# 01: Move behind better cover

Category: enhancement
Status: ready-for-agent
Resolution: completed
Blocked by: None

## What to build

Each agent sends a one-tile move to the better protected neighboring position,
while preserving its shooting command and the one-line-per-agent contract.

- [x] Empty cardinal neighbors only; exclude occupied positions and map edges.
- [x] Compare protection against the two nearby enemies independently per agent.
- [x] Prefer improved protection and preserve shooting-only behavior without it.
- [x] Demonstrate movement for both sides with executable input/output checks.
