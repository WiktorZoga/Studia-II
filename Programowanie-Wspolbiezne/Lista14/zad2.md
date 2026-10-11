# Exercise 2 — ConcurrentTaskEngine

The goal is a thread pool accepting tasks with different result types.
A mutex protects a queue of `packaged_task<void()>` wrappers; a condition variable
wakes workers. Tasks execute after releasing the queue lock. Generic `submit`
returns a future carrying the result or exception, and supports move-only work.
Arguments are stored by value; explicit reference wrappers require valid lifetimes.

Shutdown stops submissions, drains accepted tasks, joins workers, and is
idempotent. Call shutdown and destroy the engine outside its own workers: a
worker cannot join itself. Avoid waiting for child futures from workers when all
workers could be occupied by waiting parents.
The test covers results, move-only tasks, exceptions, and graceful shutdown.

From the repository root:

```sh
cmake --build --preset debug --target lista14_zad2
ctest --preset debug -R '^sheet14\.zad2_'
```
