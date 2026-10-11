```cpp
void put(T val) {
    // P1 rezerwacja ticketu 
    size_t ticket = pos_put.fetch_add(1, std::memory_order_relaxed);
    Node& node = buffer[ticket & Mask];
    while (true) {
        // P2 odczyt stanu ze stosu 
        size_t seq = node.sequence.load(std::memory_order_acquire);
        if (seq == ticket) {
            // W1 zapis danych 
            node.data = std::move(val);
            // S1 akutalizacja ticketów 
            node.sequence.store(ticket + 1, std::memory_order_release);
            return;
        }
        std::this_thread::yield();
    }
}

T get() {
    // rezerwacja biletu
    size_t ticket = pos_get.fetch_add(1, std::memory_order_relaxed);
    Node& node = buffer[ticket & Mask];
    while(true) {
        // G2 sprawdzenie czy dane są gotowe 
        size_t seq = node.sequence.load(std::memory_order_acquire);
        if (seq == ticket + 1) {
            // R1 odczyt danych 
            T val = std::move(node.data);
            // S2 akutalizaj ticketów 
            node.sequence.store(ticket + Size, std::memory_order_release);
            return val;
        }
        std::this_thread::yield();
    }
}
```

P1 sb P2 sb W1 sb S1

G1 sb G2 sb R1 sb S2

Cel: W1 hb R1

Mamy, że:
    S1 sw G2
    S2 sw P2
czyli W1 hb S1 hb G2 hb R1

teraz w następnej turze mamy do pokazania, że

Cel: R1 hb W1

Z tego co mamy:
    R1 hb S2 hb P2 hb W1
