# Stage 01: movement

## Source and coverage

- Captured: October 9, 2026, according to the PDF metadata.
- Original upload name: `game-protocol.pdf`; the retained snapshot is below.
- Named snapshot: [statement-v01.pdf](statement-v01.pdf).
- Starter snapshot: [starter-v01.cpp](starter-v01.cpp), copied from the original
  `cg-starting-code.cpp`. The duplicate top-level starter was removed during cleanup.
- Displayed league name: not captured.
- Objective label in the statement: Objective 1, MOVE.
- Coverage: two image-based pages. The export ends at Initialization Input;
  detailed protocol fields, output section, and constraints are not captured.

## Verified requirements

Two owned agents must reach (6,1) and (6,3). MOVE advances an agent one tile
toward the requested destination. Agents cannot occupy the same tile. Send
exactly one line per owned agent, beginning with its ID and a semicolon.
The tutorial fails after 20 turns or an invalid/late command. The statement
describes promotion by achieving the objective at least three times out of five.

## Development status

Input storage was the first implementation milestone. The student later
confirmed advancement to Wood 3; working code now targets stage 02, shooting.
This snapshot remains the movement reference. Static and turn fields are
taken from the supplied starter; the full exported protocol is still missing.
