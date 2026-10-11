# Exercise 4 — Work-stealing task engine

The goal is load distribution with idle workers helping busy ones.
Each worker has a queue and its own mutex. `submit` distributes tasks round-robin;
workers take local tasks and try stealing from a randomly chosen starting queue.
Separate queue locks implement the fine-grained locking bonus. Tasks execute
outside queue locks, and futures return results and exceptions.

Shutdown stops submissions, drains accepted work, and joins workers. Call shutdown
and destroy the engine outside its own workers, since a worker cannot join itself.
Avoid parent tasks occupying every worker while waiting for child futures.
Captured references must outlive their tasks. Tests cover results, exceptions,
and shutdown, including four concurrent submitters and a one-worker pool with
queued work that must complete during graceful shutdown.

From the repository root:

```sh
cmake --build --preset debug --target lista14_zad4
ctest --preset debug -R '^sheet14\.zad4_'
```
