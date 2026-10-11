# Exercise 3 — Parallel merge sort

The goal is parallel sorting using the exercise 2 engine. One auxiliary buffer
and levels of independent merges divide the work. Jobs write disjoint ranges;
the caller waits between levels. Workers never wait on child tasks.

The range requires random-access iterators and writable values. The auxiliary
vector requires default construction; merges require copy assignment, and final
copy-back can require move assignment. The comparator must be copyable and
implement a strict weak ordering. Each task owns a comparator copy; any state
shared by those copies must support concurrent access. Call from outside the
engine's workers.
Keep the range, comparator, and engine alive until sorting finishes. All scheduled
jobs must finish before local storage is destroyed, including exceptional paths.
An exception can leave input partially modified. Space is O(n); work is O(n log n).
Tests compare against `std::sort` for empty, duplicate, custom, and random data.
Boolean ranges use a sequential stable-sort fallback because `vector<bool>`
packs multiple values into shared storage words; writing disjoint bits in
parallel would still race. Tests cover both packed boolean vectors and arrays.
The engine parameter is generic, defaulting to `ConcurrentTaskEngine`, so the
tests can inject rejection during submission. Futures storage is reserved before
submission. Both task failures and submission failures drain all already accepted
jobs before rethrowing, preserving the lifetime of the range and auxiliary buffer.

From the repository root:

```sh
cmake --build --preset debug --target lista14_zad3
ctest --preset debug -R '^sheet14\.zad3_'
```
