# Approved Bronze tickets

Source: [regular-match specification](spec.md).

These changes warrant tickets: the active policy, movement objective, combat
evaluation, and bomb decisions all change. Each draft delivers visible bot
behavior through the existing executable input/output test interface.

| Ticket | Blocked by | Observable result |
| --- | --- | --- |
| [01: regular-match baseline](issues/01-regular-match-baseline.md) | None | Agents act on shared maps despite bomb inventory or shooting cooldown |
| [02: territory movement](issues/02-territory-movement.md) | 01 | Position choices improve controlled-tile advantage |
| [03: shooting and defense](issues/03-shooting-and-defense.md) | 02 | Territory, attack opportunity, and exposure jointly affect commands |
| [04: safe splash attacks](issues/04-safe-splash-attacks.md) | 03 | Useful partial bombs compete with shots and defense without friendly hits |
| [05: match consistency](issues/05-match-consistency.md) | 04 | Decisions stay valid across changing turns and the submission is reviewed |

The chain is intentional: ticket 03 combines the movement value supplied by 02,
04 compares bombs with the combat choices supplied by 03, and 05 validates that
complete policy across turns. Transitive dependencies are not repeated.

Minimal policy separation, if needed, is included in 01. No standalone wide
refactor is proposed. Historical tutorial fixtures should remain recognizable
without forcing tutorial strategy into the active Bronze checks.

## Approval and publication

The user approved this five-ticket breakdown. Each ticket is now published under
the local tracker's issues directory with Status: ready-for-agent and Category:
enhancement. The original reviewed drafts remain in the drafts directory.

Only ticket 01 is currently unblocked. Work the tickets in order, closing each
with Resolution: completed after its acceptance criteria and checks pass.
Publication has not started implementation; the parent spec, bot, and tests
remain unchanged. Context may now be compacted before implementing ticket 01.

## Implementation status

All five tickets were implemented in dependency order and resolved after local
checks and independent standards/spec reviews. See the [handoff](README.md)
and [review](review.md). CodinGame results remain pending.
