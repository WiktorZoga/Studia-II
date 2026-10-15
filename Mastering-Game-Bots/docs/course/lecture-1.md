# Lecture 1: course context

Source: [Lecture 1, updated copy](Lecture%201.pdf), supplied October 15, 2026.
The [earlier copy](../../Mastering%20Game%20Bots%20-%20Lecture%201.pdf) is retained.
Assignment source: [Assignments](../../Assignments.pdf).
This file summarizes course guidance; the original slides remain authoritative.

## Project and assessment

Choose one bot programming game and improve the bot through successive
assignments. The laboratory project deadline is the last lab before the
session, listed as February 3. A follow-up project course is planned for
summer semester 2027.

The slides list these thresholds: 100% gives 5.0 and exam exemption with
5.0; 90% gives 5.0 and exemption with 4.5; 80% gives 4.5 and exemption with
4.0; 70% gives 4.0; 60% gives 3.5; 50% gives 3.0.
The exam is approximately one hour with open and closed written questions.

## Development and AI collaboration

C++ and common search algorithms are expected knowledge. Java can help
with referees. Recommended tools are Linux, GCC, IntelliJ for Java,
and CLion or VS Code for the bot; many CPU cores help benchmarking.

LLMs are allowed, but the student must understand their code. The lecture
recommends reviews, bug finding, ideas, well-defined functions, AVX code,
and auxiliary scripts. It discourages generating the whole bot, long code
blocks, and performance-critical algorithms, data structures, or engines.
Follow the parent AGENTS.md's incremental learning workflow.

Bots must be developed individually; the lecture prohibits sharing or
publishing bot code and using multiple accounts. Work remains local.

## Platform constraints presented in the lecture

Typical limits are 50 ms per move and one second for the initial move;
these are not yet verified Soak Overflow limits. The slides list a
768 MiB memory limit, a 100,000 UTF-16-character source limit, and prohibit encoded executable
binaries. Manual plays and submissions are rate-limited; examples in the
slides are 80 plays/hour and 600/day. Verify current game-specific rules
before depending on any of these numbers.

## Techniques and planned lectures

- October 14: local benchmarking, game state, and evaluation functions;
  see [Lecture 2 notes](lecture-2.md).
- October 21: greedy strategies, opponent prediction, confidence, sequential
  testing, versioning, and weight tuning.
- October 28, 12:15: Daniel Górski (Zylo), automatic bot development;
  followed by an open lecture on orchestrating AI at 14:15, room 119.
- November 18: beam search, MCTS, and hill climbing.
- November 25: accurate engines, debugging, timing, performance, bitboards, and AVX.
- December 2: Cultist Wars example.

The schedule is flexible. Classify games by information visibility,
alternating or simultaneous moves, randomness, and board generation before
choosing algorithms. Post-mortems are a starting point for strategy research.

## Assignment 1

Due October 21, 2026 at 14:00, for at most one point. Choose a game
(past contests and unfamiliar games are recommended), read top-player
post-mortems, reach Bronze League, discuss ideas with the instructor,
and submit the profile and game links at SKOS. The lecture also says to
enroll in SKOS. Set the school to University of Wroclaw on CodinGame.

The assignment lists a large bonus for beating the instructor's bot in
one of the named past contests. Soak Overflow (Summer Challenge 2025)
is on that list.
