# Exercise 5 — FIFO queue using two stacks

The goal is a bounded FIFO queue. Separate mutexes protect the input and output
stacks; spaces and items semaphores reserve capacity and available values.
`push` and `pop` block; `try_push` and `try_pop` throw overflow and underflow errors.
Reversing input into empty output preserves FIFO. A consumer holds the output
mutex continuously across checking, transfer, and removal. Transfer additionally
locks the input stack, so another consumer cannot take the last item between
transfer and removal.

The actual type requirement is `std::is_nothrow_move_constructible_v<T>`;
copyability alone is insufficient. Transfer reserves storage before moving values.
Allocation may throw; failed operations restore their semaphore reservation.
Capacity must be positive and fit `std::ptrdiff_t` and
`std::counting_semaphore<>::max()`. The queue has no close or cancellation API;
blocked users need counterparts to make progress. Destroy only after all users finish.

Tests run four producers with four consumers and verify every element arrives
exactly once. Blocking wakeups are also checked.
A failed argument copy occurs before semaphore acquisition; its test verifies
that available capacity remains intact.

From the repository root:

```sh
cmake --build --preset debug --target lista14_zad5
ctest --preset debug -R '^sheet14\.zad5_'
```
