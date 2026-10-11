# Zadanie 2 - stos Treibera z Interval-Based Reclamation

## Cel zadania

Zadanie 2 rozwiązuje ten sam problem co zadanie 1: węzła zdjętego z
bezblokowego stosu nie wolno zwolnić, dopóki inny wątek może go jeszcze
odczytywać. Tym razem używany jest **Interval-Based Reclamation (IBR)**.

EBR opisuje aktywność czytelnika jedną epoką: „wszedłem w epoce `e`”. IBR
opisuje ją przedziałem `[lower, upper]`: „w trakcie tej operacji mogłem
obserwować obiekty z tego przedziału epok”. Jest to dokładniejszy opis dla
długich przejść po strukturze wskaźnikowej.

## Dane IBR

Opis obiektu oczekującego na zwolnienie (`IBRNode`) zawiera:

```cpp
void* pointer;
std::uint64_t birth_epoch;
std::uint64_t retire_epoch;
void (*deleter)(void*);
```

Zatem czas życia obiektu to przedział:

```text
[birth_epoch, retire_epoch]
```

Każdy wątek ma własną rezerwację:

```cpp
struct alignas(64) ThreadReservation {
    std::atomic<std::uint64_t> lower_epoch{IBR_INACTIVE};
    std::atomic<std::uint64_t> upper_epoch{IBR_INACTIVE};
};
```

`alignas(64)` jest praktycznym dodatkiem. Dąży do tego, aby sloty często
modyfikowane przez różne wątki znajdowały się w osobnych liniach cache. Bez
takiego rozdzielenia występuje false sharing: rdzenie wzajemnie unieważniają
cache mimo pracy na logicznie różnych zmiennych.

## Protokół czytelnika

`ReadGuard` odpowiada za wejście i wyjście z sekcji IBR:

```cpp
auto guard = ibr_.read_guard();

while (true) {
    guard.update();
    node* current = head_.load(std::memory_order_acquire);
    // próba bezpiecznego użycia current
}
```

1. `read_lock()` pobiera globalną epokę i wpisuje ją jako `lower` oraz `upper`.
2. `update()` pobiera nowszą epokę i przesuwa wyłącznie `upper`.
3. `read_unlock()` ustawia granice na `IBR_INACTIVE`.

W `pop()` aktualizacja górnej granicy następuje przed kolejną próbą odczytu
`head_`. Dzięki temu rezerwacja może rozszerzać się w miarę obserwowania
nowszych stanów podczas pętli CAS.

## Stos IBR

Węzeł stosu ma dodatkowe pole:

```cpp
struct node {
    T data;
    node* next;
    std::uint64_t birth_epoch;
};
```

Podczas `push` zapamiętywana jest bieżąca epoka:

```cpp
node* new_node = new node{
    data,
    head_.load(std::memory_order_relaxed),
    ibr_.current_epoch(),
};
```

Później CAS z `release` publikuje nowy węzeł. `pop` używa guardu, atomowo
zdejmuje głowę i zamiast `delete` wykonuje:

```cpp
ibr_.retire(current, current->birth_epoch);
```

W `retire` zegar domeny jest zwiększany przez `fetch_add(acq_rel)`. Powstaje
wtedy `retire_epoch`, a opis obiektu trafia na listę retired.

## Reguła przecinania przedziałów

Dla węzła `[birth, retire]` i czytelnika `[lower, upper]` brak przecięcia
zachodzi, jeżeli:

```text
retire < lower   albo   birth > upper
```

W kodzie odpowiada temu:

```cpp
const bool is_before_reader = node->retire_epoch < interval.lower;
const bool is_after_reader = node->birth_epoch > interval.upper;
if (!is_before_reader && !is_after_reader) {
    overlaps_active_reader = true;
}
```

Węzeł można zwolnić tylko wtedy, gdy **nie przecina się z żadnym aktywnym
przedziałem**. Jedno przecięcie wystarcza, by zachować go na liście retired.

```text
życie węzła:          [ 4 -------- 9 ]
czytelnik A:             [ 5 -- 7 ]       -> przecięcie, zatrzymujemy
czytelnik B:                           [10 -- 12] -> brak przecięcia
```

To jest sedno IBR. Nie pytamy jedynie „czy czytelnik jest w starej epoce?”, ale
„czy ten konkretny czytelnik mógł widzieć ten konkretny obiekt?”.

## Porządki pamięci

| Operacja | Porządek | Uzasadnienie |
|---|---|---|
| Publikacja `head_` przez `push` | `release` | Pola nowego węzła są gotowe przed udostępnieniem jego adresu. |
| Odczyt `head_` w `pop` | `acquire` | Po odczytaniu adresu można bezpiecznie czytać jego pola. |
| Udany CAS w `pop` | `acq_rel` | Łączy pobranie starej głowy z publikacją nowej. |
| `lower_epoch` i `upper_epoch` | zapis `release` | Reclaimer musi zobaczyć aktywną rezerwację czytelnika. |
| Skanowanie rezerwacji | odczyt `acquire` | Reclaimer widzi poprawnie opublikowane granice. |
| Zwiększenie czasu logicznego | `fetch_add(acq_rel)` | Porządkuje wycofanie z innymi obserwacjami domeny. |
| Numer slotu | `relaxed` | Liczy wyłącznie unikalne sloty, bez publikacji danych węzła. |

Nie należy uzasadniać `relaxed` stwierdzeniem „bo to mniej ważne”. Jest poprawne
wyłącznie tam, gdzie wymagana jest atomowość i unikalność, ale nie relacja
`happens-before` z innymi danymi.

## EBR kontra IBR

| Cecha | EBR | IBR |
|---|---|---|
| Stan czytelnika | Jedna epoka | Przedział `[lower, upper]` |
| Dane przy obiekcie | Epoka wycofania | Epoka narodzin i wycofania |
| Warunek zwolnienia | Wszyscy aktywni są dalej | Brak przecięcia z każdym czytelnikiem |
| Złożoność idei | Mniejsza | Większa, ale bardziej precyzyjna |
| Typowe użycie | Krótkie sekcje odczytu | Długie przejścia po wskaźnikach |

IBR nie jest automatycznie szybsze. Dokładniejsza informacja kosztuje dodatkowe
atomiki, pamięć i skanowanie przedziałów.

## Testy i ograniczenia

`zad2.cpp` sprawdza najpierw semantykę LIFO. Następnie czterech producentów i
czterech konsumentów pracuje równolegle. Każdy producent tworzy rozłączny zakres
500 liczb, a końcowy test sprawdza, że pobrano wszystkie i żadnej dwa razy.

Podobnie jak w EBR, konstruktor musi dostać wystarczającą liczbę slotów, a
niszczenie stosu wymaga stanu spoczynku wszystkich wątków. Test wykrywa błędy
funkcjonalne, ale nie zastępuje analizy protokołu ani sanitizera wyścigów.

## Pytania na prezentację

**Po co `birth_epoch`?** Dzięki niej znamy całe życie obiektu, a nie wyłącznie
moment jego wycofania.

**Czym jest `upper_epoch`?** To najnowsza epoka, z której obiekty czytelnik
mógł zacząć obserwować w trakcie nadal aktywnej operacji.

**Dlaczego nie ma natychmiastowego `delete`?** Przedział życia węzła może
przecinać się z rezerwacją czytelnika, który ma już jego adres.

**Dlaczego IBR jest trudniejsze od EBR?** Trzeba śledzić i porównywać dwa
przedziały zamiast jednej liczby dla każdego czytelnika.
