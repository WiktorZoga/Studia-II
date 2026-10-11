# Exercise 1 — ResourceManager

The goal is concurrent searches, one insertion at a time, and exclusive removal.
Searches may overlap an insertion. A mutex, condition variable, and counters of
active and waiting operations enforce admission. A waiting removal blocks new
searches and insertions, preventing starvation from incoming work.
The test checks allowed overlap, serialized insertion, and exclusive removal.
Keep the manager and resource alive until all users finish.

From the repository root:

```sh
cmake --build --preset debug --target lista14_zad1
ctest --preset debug -R '^sheet14\.zad1_'
```
