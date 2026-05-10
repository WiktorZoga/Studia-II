# Kolejka CppMem
```cpp
int main() {
    atomic_int head = 0;
    atomic_int tail = 0;
    int buff0 = 0;

    {{{
        {
            int curr_head = head.load(memory_order_acquire);
            int curr_tail = tail.load(memory_order_relaxed);
            buff0 = 42;
            tail.store(1, memory_order_release);
        }
    |||
        {
            int curr_head2 = head.load(memory_order_relaxed);
            int curr_tail2 = tail.load(memory_order_acquire);
            
            if (curr_tail2 == 1) {
                int val = buff0;
                head.store(1, memory_order_release);
            }
        }
    }}};

    return 0;
}
```

# SeqLock
```cpp
int main() {
    atomic_int seq = 0;
    atomic_int payload = 0;

    {{{
        {
            seq.store(1, memory_order_relaxed);
            atomic_thread_fence(memory_order_release);
            payload.store(42, memory_order_relaxed);
            seq.store(2, memory_order_release);
        }
    |||
        {
            int r1 = seq.load(memory_order_acquire);
            int r2 = payload.load(memory_order_relaxed);
            atomic_thread_fence(memory_order_acquire);
            int r3 = seq.load(memory_order_relaxed);
        }
    }}};

    return 0;
}
```