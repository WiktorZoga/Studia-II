# Issue tracker: Local Markdown

All paths in this document are relative to `Mastering-Game-Bots/`,
the course workspace, rather than the parent Git repository.

Tickets and plans under `.scratch/` are local working records, ignored by Git.
Keep them on disk for skills; record durable conclusions in the versioned course
documentation. A fresh checkout may not contain these local records.

## Conventions

- One effort per directory: `.scratch/<feature-slug>/`.
- The spec is `.scratch/<feature-slug>/spec.md`.
- Tickets are individual files: `.scratch/<feature-slug>/issues/<NN>-<slug>.md`,
  numbered from `01`.
- Each triaged ticket has one `Category:` (`bug` or `enhancement`) and one
  `Status:` using the mapping in `triage-labels.md`.
- Append discussion under `## Comments`; preserve earlier entries.
- Use `Resolution: completed` or `Resolution: rejected` to record closure.
  When listing active tickets, omit those with a resolution.
- Work requiring the student's choices, understanding, account access, or
  submission belongs in `ready-for-human`. `ready-for-agent` is for clearly
  authorized setup, tooling, review, or bounded implementation work.

## Publish or fetch a ticket

When a skill says to publish, create a local ticket file.
When a skill says to fetch, read the referenced file or resolve its number
within the specified effort. Ask for the effort if the number is ambiguous.

## Wayfinding operations

- Map: `.scratch/<effort>/map.md`, containing Notes, Decisions-so-far, and Fog.
- Child tickets: `.scratch/<effort>/issues/NN-<slug>.md`.
- Type: `research`, `prototype`, `grilling`, or `task`.
- Blocking: `Blocked by: NN, NN`; dependencies are satisfied when resolved.
- Claiming: record `Claim: claimed` before starting; retain the triage status.
- Resolving: append `## Answer`, set `Claim: resolved` and
  `Resolution: completed`, then add a decision pointer to the map.
- Frontier: open, unblocked, unclaimed tickets, ordered by number.

These operations stay local. External publication requires an explicit
user instruction and must respect the course's individual-work rules.
