# Verified bomb-tutorial rules

These primary-source checks resolve the gaps in the PDF export.

- [Game](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/src/main/java/com/codingame/game/Game.java): throw range is Manhattan distance at most four; any valid map tile may be the landing point; blast damage is 30 on the landing tile and eight neighbors, including friendlies, with no cover reduction.
- [Serializer](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/src/main/java/com/codingame/game/Serializer.java): the existing initialization and turn-field layout is unchanged; current bombs and wetness are supplied each turn.
- [Command manager](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/src/main/java/com/codingame/game/CommandManager.java) and [action patterns](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/src/main/java/com/codingame/game/action/ActionType.java): explicit-ID THROW and MOVE commands are accepted; MESSAGE alone supplies a valid non-combat idle line.
- [Referee](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/src/main/java/com/codingame/game/Referee.java): first-turn allowance is 1000 ms; regular turns allow 50 ms.
- [Tutorial setup](https://github.com/CodinGame/SummerChallenge2025-SoakOverflow/blob/main/src/main/java/com/codingame/game/TutorialManager.java): the map has four enclosed groups. Agents start at 70 wetness. The mobile owned agent has three bombs and the trapped owned agent one; owned IDs may be swapped. Each enemy-only group's occupants fit one 3-by-3 blast. Completion requires eliminating those three groups; shooting or losing an owned agent fails the tutorial.

The implemented baseline identifies enclosed floor regions, excludes regions
containing an owned agent, and finds safe whole-group throws. A short BFS supplies
the next empty, unoccupied step toward a throwing position. Actions reserve their
targets and landing/blast positions so another owned agent cannot waste a bomb
on the same group or move into a planned blast.

This targets the supplied tutorial; it is not a general strategy for later leagues.
