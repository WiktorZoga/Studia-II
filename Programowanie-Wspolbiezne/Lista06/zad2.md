# Relacje w modelu pamięci
1. Sequenced-before:
    * jest to relacja w obrebie jednego wątku, lokalna
    * jeśli w kodzie operacja A występuje przed operacją B, to A $sb$ B
    * intuicyjnie jest to koljeność wywoływania intrukcji przez programiste
    * jest przechodnia

2. Synchronizes-with:
    * ta relacja zachodzi pomiedzy operacjami w różnych wątkach
    *  Relacja $sw$ zachodzi między operacją zapisu $W$ (write) do zmiennej atomowej $M$, a operacją odczytu $R$ (read) z tej samej zmiennej $M$, pod warunkiem, że:
        a. Użyto odpowiednich flag (np. seq_cst lub release dla zapisu i acquire dla odczytu).
        b. Operacja odczytu $R$ faktycznie odczytała wartość, którą zapisał $W$ (lub wartość zapisaną później w tym samym wątku przez tę samą zmienną).
    * inticyjnie jesst to moment, gdy dane "przeskasują pomiędzy wątkami"

3. Happens-before:
    * najważniejsza relacja, bo realnie mówi o tym co się wydarzy
    * A hb B jeśli
        * A sb B (czyli w obrebie jednego wątku), lub
        * A sw B (czyli pomiędzy dwoma wątkami), lub
        * Jeśli A hb X oraz X hb B
    * daję gwarancję, że wszystkie zmiany w pamięci dokonane w A muszą być widoczne w B
    * jeśli udowodnimy, że (zapis do kolejki) hb (odczyt danych z kolejki) to znaczy, że nie ma tam data race

    Happens-before nie łaczy "lini kodu", ale konkretne instancje (wykonania) tych linii w czasie. 

# Typy Memory Order (Od najsłabszego do najsilniejszego)
1. Relaxed
    * tylko atomowość
    * brak barier
    * procesor może mieszać jak chce
    * SW nie zachodzi
    * gwarantuje jedynie, że operacje na tej konkretnej zmiennej jest atomowa, nie narzuca żadnej kolejności

2. Acquire/Release 
    * "tworzą most"
    * SW będzie zachodzić
    * zapis (release) sprawia, że wszystkie wcześniejsze operacje w tym wątku (nawet te na zwykłych zmiennych!) stają się widoczne dla wątku, który wykona odczyt acquire.

3. Seq_cst
    * pełna, globalna kolejność
    * SW będzie zachodzić
    * Wszystkie operacje oznaczone jako seq_cst w całym programie (wszystkie wątki) układają się w jedną, spójną linię czasu. Nie ma możliwości, żeby Wątek A widział, że najpierw stało się $X$, a potem $Y$, podczas gdy Wątek B widzi odwrotnie.

# Data Race występuje, gdy:
    1. Dwa wątki uzyskują dostęp do tej samej komórki pamięci.
    2. Przynajmniej jeden z nich to zapis.
    3. Dostępy te nie są połączone relacją happens-before.


```cpp
bool push(const T& val) {
    // READ1 pobranie head, by sprawdzić miejsce w kolejce 
    size_t current_head = head.load(std::memory_order_acquire); 
    // READ2 lokalny odczyt tail  
    size_t current_tail = tail.load(std::memory_order_relaxed); 
    size_t new_tail = (current_tail + 1) & (Size - 1);

    if (new_tail == current_head){
        return false;
    }
    // WRITE1 zapis danych do bufora 
    buff[current_tail] = val;
    // STORE1 "publikacja" danych 
    tail.store(new_tail, std::memory_order_release);    
    return true;
}

std::optional<T> pop() {
    // READ3 lokalny odczyt head 
    size_t current_head = head.load(std::memory_order_relaxed); 
    // READ4 pobranie tail 
    size_t current_tail = tail.load(std::memory_order_acquire); 

    if (current_head == current_tail) {
        return std::nullopt;
    }
    // READ5 odczyt danych z bufora 
    std::optional<T> opt{buff[current_head]};
    size_t new_head = (current_head + 1) & (Size - 1);
    // STORE2 "zwolnienie" miejsca, info że slot jest pusty 
    head.store(new_head, std::memory_order_release);    
    return opt;
}
```

CEL: [WRITE1] hb [READ5], aby konsument miał co czytać

push:
[READ1] sb [READ2] sb [WRITE1] sb [STORE1]

pop:
[READ3] sb [REAED4] sb [READ5] sb [STORE2]

[STORE1] sw [READ4]

[STORE2] sw [READ1]

możemy z tego wyciągnąć, że:

[WRITE1] hb (bo sb) [STORE1] hb (bo sw oraz odpowiednie flagi) [READ4] hb (bo sb) [READ5]

CEL: W następnej turze!!! [READ5] hb [WRITE1], aby producent nie nadpisał danych 

[READ5] hb (bo sb) [STORE2] hb (bo sw oraz odpowiednie flagi) [READ1] hb (bo sb sb) [WRITE]

memory_order_seq_cst jest mocniejszą relacją, więc tym bardziej wszystko będzie również zachodzić