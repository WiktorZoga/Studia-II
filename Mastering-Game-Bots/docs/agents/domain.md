# Domain docs

This course uses a single-context layout. Paths are relative to
`Mastering-Game-Bots/`, not the parent Git repository.

## Before exploring code

Before changing Soak Overflow parsing or actions, read
`soak-overflow/protocols/README.md` and the target stage's `notes.md` and statement.
For a newly supplied statement, follow that document's snapshot and comparison
workflow before changing the bot. Its latest snapshot is not proof of promotion.

Read `GLOSSARY.md` if it exists and relevant decisions under `docs/adr/`.
If these files do not exist, proceed silently. Create them lazily when
domain terminology or architectural decisions actually become settled.

## Vocabulary and decisions

Use glossary terminology in tickets, code reviews, hypotheses, and tests.
Surface conflicts with existing ADRs explicitly before proposing a change.
The original course PDFs are the source for assignment requirements;
course notes are summaries, not game-protocol specifications.

## Evaluation and local matches

Before Assignment 2 planning, benchmark integration, or evaluation changes,
read `P2/README.md` and `docs/course/lecture-2.md`. If available locally, also
read `.scratch/assignment-2/plan.md`. The versioned documents capture course
requirements and tooling limitations; the ignored plan records proposed experiments.

The course and standalone portfolio repositories currently contain matching bot
copies. Check the development-location decision in the local plan before editing
either copy. If the plan is absent or the decision is still open, ask the student
which copy to edit. Lecturer-provided tools and materials remain in the course workspace.
