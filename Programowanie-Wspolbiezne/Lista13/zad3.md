# Zadanie 3 - wielowątkowy `HistorySet<T>` z `undo`

## Cel zadania

`HistorySet<T>` ma działać jak zbiór, ale dodatkowo przechowywać historię zmian.
Udostępnia cztery operacje:

```cpp
bool insert(const T& value);
bool erase(const T& value);
bool contains(const T& value) const;
bool undo();
```

Każda modyfikacja musi być bezpieczna przy równoległym użyciu, a `undo()` ma
cofać kolejne operacje w odwrotnej kolejności. Implementacja wykorzystuje
wyłącznie standardowe kontenery i jeden muteks, zgodnie z treścią zadania.

## Wybór struktur danych

```cpp
mutable std::mutex mutex_;
std::set<T, Compare> values_;
std::deque<Operation> history_;
```

- `std::set<T, Compare>` jest właściwym zbiorem: nie ma duplikatów i daje
  uporządkowane wyszukiwanie.
- `std::deque<Operation>` jest stosem historii. Używamy `back()` i `pop_back()`,
  więc ostatnia operacja jest dostępna w O(1). `deque` nie przenosi wszystkich
  wcześniejszych elementów przy typowym dopisywaniu na końcu, co upraszcza
  myślenie o wyjątkach.
- Jeden `std::mutex` chroni **jednocześnie** zbiór i historię. To zachowuje
  niezmiennik: stan zbioru i lista operacji zawsze opisują ten sam moment.

Szablon ma też parametr `Compare = std::less<T>`. W praktyce `T` musi dać się
porównywać przez ten komparator i kopiować do historii.

## Semantyka historii

Każdy wpis ma postać:

```cpp
enum class OperationKind { insert, erase };

struct Operation {
    OperationKind kind;
    T value;
    bool changed;
};
```

Do historii trafia każde wywołanie `insert` i `erase`, również nieskuteczne:

```text
insert(10)  -> changed = true
insert(10)  -> changed = false
erase(10)   -> changed = true
erase(10)   -> changed = false
```

`contains` nie jest zapisywane. To świadoma decyzja: jest czystym zapytaniem,
nie zmienia zawartości zbioru i nie ma sensownego efektu do odwrócenia. W tym
projekcie „historia operacji podlegających undo” oznacza operacje modyfikujące
`insert` oraz `erase`.

`undo()` usuwa ostatni wpis z historii. Jeżeli wpis ma `changed == true`,
odwraca zmianę; jeśli `false`, usuwa tylko wpis, bo stan zbioru i tak się nie
zmienił. Zwraca `false` wyłącznie dla pustej historii.

## `insert` krok po kroku

```cpp
std::scoped_lock lock(mutex_);
const auto [position, inserted] = values_.insert(value);
try {
    history_.emplace_back(OperationKind::insert, value, inserted);
} catch (...) {
    if (inserted) {
        values_.erase(position);
    }
    throw;
}
return inserted;
```

1. `std::scoped_lock` blokuje oba zasoby na cały czas operacji.
2. `std::set::insert` zwraca iterator oraz flagę `inserted`.
3. Zapisywana jest historia, również gdy element już był obecny.
4. Jeżeli alokacja lub kopiowanie wartości do historii rzuci wyjątek, a element
   przed chwilą został dodany, usuwamy go przez znany iterator.
5. Obserwator albo widzi stan sprzed `insert`, albo pełny wpis i pełną zmianę.

To jest praktyczna silna gwarancja wyjątków dla tej operacji: wyjątek nie
zostawia elementu w zbiorze bez odpowiadającego mu wpisu historii.

## `erase` krok po kroku

```cpp
const auto position = values_.find(value);
if (position == values_.end()) {
    history_.emplace_back(OperationKind::erase, value, false);
    return false;
}

history_.emplace_back(OperationKind::erase, *position, true);
values_.erase(position);
```

Najpierw powstaje wpis historii, dopiero potem fizycznie znika element ze
zbioru. To ważne, ponieważ konstrukcja `Operation` kopiuje `T` i może rzucić
wyjątek. W takiej sytuacji `erase` elementu jeszcze się nie wydarzyło.

Zapisywana jest wartość `*position`, nie tylko argument `value`. Przy
niestandardowym komparatorze argument może być jedynie równoważny z elementem
w zbiorze; historia powinna przechować konkretny usunięty obiekt.

## `contains` i `undo`

`contains` również blokuje `mutex_`, mimo że jest metodą `const`:

```cpp
bool contains(const T& value) const {
    std::scoped_lock lock(mutex_);
    return values_.contains(value);
}
```

Dlatego `mutex_` jest `mutable`. `const` oznacza „nie zmieniam logicznej
zawartości zbioru”; blokowanie jest szczegółem synchronizacji.

`undo()` działa pod tym samym muteksem:

- odwrócenie `insert` oznacza `erase` wartości;
- odwrócenie `erase` oznacza ponowne `insert` wartości;
- dopiero po udanym odwróceniu wywoływane jest `history_.pop_back()`.

Jeśli ponowne wstawienie lub wyszukanie rzuci wyjątek, wpis historii pozostaje,
więc można bezpiecznie spróbować `undo()` ponownie.

## Współbieżność i porządki pamięci

W tym zadaniu nie stosujemy ręcznie `memory_order`. `std::mutex` daje mocniejszą
i prostszą gwarancję:

```text
unlock() wątku A synchronizuje-z lock() wątku B
```

Zapisane przez A modyfikacje `values_` i `history_` są zatem widoczne dla B po
przejęciu tego samego muteksu. To jest relacja *happens-before*. Próba użycia
atomików zamiast muteksu dla dwóch powiązanych kontenerów byłaby trudniejsza i
niepotrzebna: trzeba byłoby atomowo utrzymać zgodność zbioru z historią.

Kosztem jest serializacja wszystkich operacji. To akceptowalny kompromis,
ponieważ zadanie wyraźnie wymaga użycia blokad.

## Testy

`zad3.cpp` zawiera trzy grupy testów:

1. Sekwencję `insert`, duplikat, `erase`, nieudane `erase` oraz cztery kolejne
   `undo`. Sprawdza, że historia jest wielopoziomowa.
2. Typ `CopyMayThrow`. Druga kopia rzuca wyjątek: pierwsza trafia do `set`,
   druga byłaby wpisem historii. Test sprawdza, że po wyjątku element nie
   zostaje w zbiorze, czyli działa rollback z `insert`.
3. Osiem wątków wstawia rozłączne zakresy po 500 liczb. Po automatycznym
   `join` test sprawdza obecność wszystkich 4000 wartości.

## Złożoność i ograniczenia

Przy `std::set` wyszukiwanie, wstawienie i usunięcie mają O(log n). `undo` ma
O(log n), bo musi ponownie odnaleźć albo wstawić element. Historia rośnie wraz
z liczbą wywołań `insert` i `erase`; nie ma automatycznego limitu pamięci.

Jedna blokada oznacza, że dwa wątki nie mogą jednocześnie wykonywać nawet
`contains`. Jest to prosty, poprawny wariant. Gdyby celem była większa
równoległość odczytów, można by rozważyć `std::shared_mutex`, ale to nie jest
potrzebne do tego zadania i komplikuje `undo`.

## Pytania na prezentację

**Dlaczego historia i zbiór są pod jednym muteksem?** Muszą zmieniać się jako
jedna logiczna operacja. Dwa niezależne muteksy mogłyby dać stan, w którym
zbiór jest już zmieniony, a historia jeszcze nie.

**Dlaczego logujemy nieudane `insert` i `erase`?** Dzięki temu `undo` odtwarza
kolejność prób modyfikacji. Cofnięcie nieudanej operacji nie zmienia zbioru,
ale zdejmuje jej wpis z historii.

**Jakie operacje `T` mogą rzucać?** Kopiowanie do `set` lub historii,
porównania wykonywane przez `Compare` oraz alokacje pamięci w kontenerach.

**Dlaczego w `erase` historia jest zapisywana przed usunięciem?** Jeśli
kopiowanie usuwanego `T` do historii rzuci wyjątek, zbiór pozostaje nietknięty.
