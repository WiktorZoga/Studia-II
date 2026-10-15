# Stage 02: shooting

## Source and coverage

- Captured: October 9, 2026, according to the PDF metadata.
- Original upload name: `gp_wood3.pdf`; the retained snapshot is below.
- Named snapshot: [statement-v01.pdf](statement-v01.pdf).
- League name: Wood 3, confirmed by the student; not visible in the PDF.
- Objective label in the statement: Objective 2, SHOOT.
- Stage-specific starter: not supplied.
- Coverage: two image-based pages. The export ends at Initialization Input;
  detailed input/output fields and constraints are not captured.

## Changes from movement

Enemy agents are introduced. The objective is for both owned agents to shoot
the enemy with the greatest wetness on every turn. Failure occurs when an owned
agent does not shoot the wettest enemy, or a command is invalid or late.

SHOOT takes a target agent ID. Shooting raises wetness; agents with wetness of
at least 100 are removed. Damage is based on the shooter's soaking power,
halved beyond optimal Manhattan range, and absent beyond twice that range.
The statement permits movement as well as shooting.

## Protocol review

The earlier starter already includes ownership, wetness, range, power, and
cooldown fields. Their presence does not prove the entire protocol is unchanged.
Compare the detailed stage-specific protocol or new starter before changing
parsing. The [official tutorial referee](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/src/main/java/com/codingame/game/TutorialManager.java)
selects its wettest enemy using a sequential maximum over the original enemy
list. The bot preserves that initial order when wetness is equal. Numerical
rounding and other details absent from this capture remain unresolved until
relevant to an implementation step.

## Development status

The student confirmed Wood 3. Shooting commands and explicit per-turn output
are implemented, and seven executable-level regression cases pass locally.
The tests cover output arriving before the next input turn, changing survivors,
target choice, ownership, and ID ordering. A platform run is still needed to
confirm success and any subsequent promotion. The student subsequently
confirmed advancement to stage 03, taking cover.
