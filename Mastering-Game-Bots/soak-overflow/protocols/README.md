# League statements and protocols

Each league introduces its own objective, rules, and potentially input/output
changes. Read the relevant snapshot before changing the bot. A later statement
does not prove that the student has completed an earlier league.

## Available snapshots

| Stage | Statement | Starter | Coverage |
| --- | --- | --- | --- |
| 01: movement | [statement-v01.pdf](01-movement/statement-v01.pdf) | [starter-v01.cpp](01-movement/starter-v01.cpp) | Rules captured; detailed input/output missing |
| 02: shooting | [statement-v01.pdf](02-shooting/statement-v01.pdf) | Not supplied for this stage | Rules captured; detailed input/output missing |
| 03: taking cover | [statement-v01.pdf](03-covers/statement-v01.pdf) | Not supplied for this stage | Rules and diagrams captured; detailed input/output missing |
| 04: splash bombs | [statement-v01.pdf](04-bombs/statement-v01.pdf) | Not supplied for this stage | Objective and bomb rules captured; detailed input/output missing |
| 05: Bronze | [statement-v01.pdf](05-bronze/statement-v01.pdf) | Not supplied for this stage | Full rules and expanded input/output captured |

The latest available statement is stage 05. The reported Bronze inactivity led
to replacing the tutorial policy with a regular-match baseline. The
[Bronze comparison](05-bronze/notes.md) records the rules and original mismatch;
the [bot overview](../README.md) describes the current policy. Historical local
implementation and check records are kept in the course's ignored
`.scratch/bronze-league/README.md`. A platform run of this version is still pending.

Folders use the verified objective order. The shooting source filename says
Wood 3; the exact displayed league for movement was not supplied. Record actual
league names when visible rather than inventing them from the objective order.

## Snapshot naming

Use `NN-objective/statement-vNN.pdf` and, when supplied,
`NN-objective/starter-vNN.cpp`. Examples: `01-movement/statement-v01.pdf`,
`02-shooting/statement-v02.pdf`. Numbers in folders order the observed stages;
version numbers distinguish captures of the same stage.

Keep snapshots immutable. A fuller export, corrected export, or changed starter
gets the next unused version number. Record the source filename, capture date,
displayed league, and coverage in that stage's `notes.md`. A future league gets
a folder only after its actual statement is available. Keep working code in
`../main.cpp`; starter snapshots are references.

## Workflow after promotion or a new upload

1. Inspect the supplied PDF and starter before choosing a stage or filename.
   For image-based PDFs, render and read all relevant pages. Identify the
   objective and displayed league, and check whether the export is complete.
2. Save a clearly named snapshot without overwriting existing captures. Preserve
   original uploads and any user code. Link the snapshot here and record its
   provenance and missing sections in `notes.md`.
3. Compare with the previous stage: objective, victory/failure conditions,
   map/agent assumptions, initialization, turn input, output syntax, enabled
   actions, and time limits. Explicitly distinguish changed rules from changed
   protocol fields. Mark unknown details instead of assuming continuity.
4. Check the existing parser against the actual fields and the supplied starter.
   If a missing section affects the next change, ask for that section. The
   platform export should include expanded Game Protocol, initialization input,
   per-turn input, output, constraints, and examples when provided.
5. Create a local ticket under `.scratch/<effort>/issues/`, using the course's
   triage statuses. Use `needs-info` for a blocking missing protocol section;
   use `ready-for-agent` for an authorized bounded code change. Keep personal
   account actions and promotion verification with the student.
6. Explain the new mechanic and implement one small agreed change. Build the
   single-file bot and run it on CodinGame. Record observed results; mark
   promotion only when confirmed. The former local test suite was removed at
   the student's request and remains available in Git history at `e89a3f7`.
7. Keep earlier snapshots. Adapt the working bot to
   the target league; defer large strategy or architecture changes until needed.

## Source provenance

- Movement statement was uploaded as `game-protocol.pdf`; its retained source
  is [the stage 01 snapshot](01-movement/statement-v01.pdf).
- Movement starter: [starter-v01.cpp](01-movement/starter-v01.cpp).
- Shooting statement was uploaded as `gp_wood3.pdf`; its retained source
  is [the stage 02 snapshot](02-shooting/statement-v01.pdf).
- Taking-cover statement was supplied directly at its stage 03 snapshot path.
- Splash-bomb statement was supplied directly at its stage 04 snapshot path.
- Bronze statement was supplied directly at its stage 05 snapshot path.

The first two snapshots were verified as byte-for-byte copies when created.
Their original upload paths are no longer present; use the named snapshots.
