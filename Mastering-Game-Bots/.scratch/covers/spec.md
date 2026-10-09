# Taking cover: map lookup and cover evaluation

## Requested step

Implement only map lookup and evaluation of the cover protecting a target
against a shot from a supplied shooter position. Keep the single-file C++20
submission. Movement selection and replacement of the shooting policy are
later steps; this milestone alone does not complete the covers tutorial.

## Proposed public interface

- A position is a pair of integer grid coordinates, x and y.
- Map lookup returns the tile at a requested coordinate or no tile outside
  the map. Read coordinates from the map rather than relying on input ordering.
- Cover evaluation returns target damage reduction as an integer percentage:
  0 for no cover, 50 for low cover, or 75 for high cover. Its arguments are an
  ordered shooter/target pair of valid map positions; the target is protected.

The student approved direct tests of these two interfaces.
Existing executable stdin/stdout tests protect turn output during this step.

## Rules to cover

- Empty tiles provide no protection.
- Low cover reduces incoming shot damage by 50%; high cover by 75%.
- Only cover adjacent to the target on a relevant incoming axis can protect it.
- Shots from the unprotected side do not gain the reduction.
- A shooter adjacent to that same cover does not grant the target protection.
- If two covers apply, choose the strongest rather than adding reductions.
- Map edges must be handled safely.

The [official referee](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/src/main/java/com/codingame/game/Game.java)
checks candidate target-adjacent cover along the horizontal and vertical
incoming directions when separation on that axis exceeds one tile. It ignores
that cover if the shooter is within Chebyshev distance one of it, including
diagonal adjacency. Tests should use literal results from the supplied diagrams
and these explicit referee conditions, rather than duplicating the algorithm.

## Verification and review

Work one failing example and its implementation at a time. Build after each
slice; run the existing full CTest suite when the helpers are complete.
For the final two-axis review, the proposed fixed point is Git HEAD
`c492f356eeadd4b70dacd0b1de31e4045658c3da`, with scope restricted to the files
in this step. The current bot sources are untracked, so a first scoped commit
also records their existing contents. The student accepted the recommended
review and scoped commit workflow. Earlier unrelated files remain unstaged.
