# Stage 03: taking cover

## Source and coverage

- Snapshot: [statement-v01.pdf](statement-v01.pdf), supplied in this folder.
- Captured: October 9, 2026, according to the PDF metadata.
- The student confirmed advancement from shooting to this tutorial.
- Exact displayed league name: not captured; the statement says Objective 3.
- Three image-based pages contain the objective, cover rules, diagrams, and
  movement/shooting order. Detailed Game Protocol input/output is still absent:
  the capture ends at Initialization Input.
- A new starter or real turn-input fixture has not been supplied.

## Changes from shooting

The objective now lasts one turn: move both owned agents behind the best
available cover, then shoot the nearby opponent with the least cover protection.
Selecting the wettest enemy alone no longer describes the required behavior.

The map tile type has three meanings: 0 is empty, 1 is low cover, 2 is high
cover. Cover tiles are impassable; MOVE automatically paths around them.
Low cover reduces shooting damage by 50%; high cover by 75%.

Cover protection depends on the shot direction and the target being
orthogonally adjacent to cover. Both agents adjacent to the same cover do not
receive protection against each other. When multiple covers apply, only the
highest cover counts. The statement's diagrams specify the protected regions.

Movement and shooting may share one command line:
`agentId;MOVE x y;SHOOT enemyId`. Movement occurs first; evaluate the shot from
the new position. The tutorial says each owned agent is one move from the
candidate cover positions, with two nearby enemies.

## Next implementation slice

Start with map lookup and a small cover-evaluation function, using worked
examples from the diagrams. Then use that evaluation to select a reachable
protected position and a less-protected enemy, and emit a combined command.
Keep one explicit-ID line per owned agent and flush after the turn.

Preserve the previous output/ownership regression checks. Reclassify tests of
the wettest-enemy policy as shooting-stage behavior; they do not define the
new tutorial's target-selection rule. Obtain representative current-league
input before claiming the entire new protocol has been verified.

## Development status

Map lookup and cover evaluation are implemented in the single submission file.
The public helpers are `tile_at(setup, position)` and
`cover_reduction(setup, shooter, target)`, returning the target's damage
reduction percentage (0, 50, or 75). Position arguments to cover evaluation
must be valid map coordinates. Tests cover map ordering/edges, cover levels,
incoming directions, shared-cover exceptions, and strongest-cover selection.

The bot now examines empty, unoccupied neighboring tiles and chooses improved
protection against its two nearby enemies. It then selects a nearby enemy from
the destination, favoring less protection and respecting shooting range.
If decorative cover ties the post-move reductions, original-position protection
breaks the tie consistently with the tutorial's intended enemy.

Combined `MOVE;SHOOT` output passes local checks across 64 cover/layout
combinations, with ID ordering, occupancy, map blockage, and shooting range
checks. Floor-only scenarios retain the earlier shooting behavior.
The local-only `.scratch/covers-league/README.md` under the course root records
the spec, tickets, and implemented steps. Platform completion and subsequent
promotion remain pending.
