# Zadanie 4 - `VersionedConcurrentSet<T>` i migawki

## Cel zadania

`VersionedConcurrentSet<T>` ma pozwalać wielu wątkom pracować równolegle na
własnych kopiach zbioru. Zwykłe `insert`, `erase` i `find` nie powinny blokować
innych wątków. Dopiero rzadkie `merge()` synchronizuje zmiany z obiektem
wspólnym.

Najtrudniejszy warunek brzmi: **ostatni zapis wygrywa**. Nie wolno po prostu
zsumować zawartości wszystkich migawek, ponieważ starsza migawka mogłaby
przywrócić element, który nowsza migawka usunęła.

## Architektura rozwiązania

Obiekt główny zawiera współdzielony `State`, trzymany przez `std::shared_ptr`:

```cpp
struct Update {
    bool present;
    std::uint64_t version;
};

struct State {
    Compare compare_;
    std::mutex mutex_;
    std::map<T, Update, Compare> values_;
    std::atomic<std::uint64_t> next_version_{0};
};
```

`shared_ptr` sprawia, że stan wspólny istnieje tak długo, jak istnieje choć
jedna migawka. To eliminuje problem wiszącego wskaźnika, gdy użytkownik zachowa
`Snapshot` dłużej niż obiekt, przez który ją utworzył.

`values_` nie jest zwykłym `set`. Jest mapą od wartości do jej ostatniego
zapisu:

- `present == true` oznacza, że wartość należy do zbioru;
- `present == false` jest **tombstone**, czyli znacznikiem usunięcia;
- `version` mówi, kiedy powstał ostatni zapis tej wartości.

Tombstone jest konieczny. Usunięcie wartości z mapy całkowicie utraciłoby
informację, że nowsza operacja `erase` już wygrała.

## Co przechowuje migawka

`Snapshot` jest celowo własnością jednego wątku:

```cpp
std::shared_ptr<State> state_;
std::set<T, Compare> local_values_;
std::map<T, Update, Compare> pending_updates_;
```

- `local_values_` to lokalny widok zbioru - na nim działają `find`, `insert`
  oraz `erase`.
- `pending_updates_` to dziennik ostatnich lokalnych zapisów dla każdej
  wartości.
- `state_` służy do pobrania numeru wersji i późniejszego `merge`.

Nie wolno równocześnie używać **tej samej** migawki w dwóch wątkach, ponieważ
jej lokalne `set` i `map` nie mają muteksu. To nie jest wada modelu: wymaganiem
jest praca wątków na własnych kopiach.

## Tworzenie migawki

```cpp
[[nodiscard]] Snapshot snapshot() const {
    std::set<T, Compare> local_values(state_->compare_);
    std::scoped_lock lock(state_->mutex_);

    for (const auto& [value, update] : state_->values_) {
        if (update.present) {
            local_values.insert(value);
        }
    }
    return Snapshot(state_, std::move(local_values));
}
```

Jedyny moment blokady przy tworzeniu migawki to skopiowanie spójnego obrazu
stanu wspólnego. Potem migawka pracuje wyłącznie na prywatnych kontenerach.
Tombstones nie są kopiowane do `local_values_`, ponieważ lokalny widok ma
zawierać tylko wartości faktycznie obecne.

## Lokalne operacje

```cpp
bool insert(const T& value) {
    const bool inserted = local_values_.insert(value).second;
    record_update(value, true);
    return inserted;
}

bool erase(const T& value) {
    const bool erased = local_values_.erase(value) != 0;
    record_update(value, false);
    return erased;
}
```

`insert` i `erase` nie blokują `State::mutex_`. Wykonują operację na lokalnym
zbiorze, a potem rejestrują zapis:

```cpp
const std::uint64_t version =
    state_->next_version_.fetch_add(1, std::memory_order_relaxed) + 1;
pending_updates_.insert_or_assign(value, Update{present, version});
```

`insert_or_assign` jest istotne: jeżeli jedna migawka zrobi `insert(7)`, a
potem `erase(7)`, dziennik przechowuje wyłącznie nowszy zapis `erase`. Nie
potrzebujemy przenosić do `merge` całej historii tej samej wartości.

Nawet `erase` wartości nieobecnej lokalnie zapisuje tombstone. To świadoma
semantyka „ostatni zapis wygrywa”: późniejsze logiczne usunięcie ma pokonać
starsze wstawienie z innej migawki.

## `merge` - najważniejsza część

```cpp
void merge() {
    std::scoped_lock lock(state_->mutex_);

    for (const auto& [value, update] : pending_updates_) {
        const auto current = state_->values_.find(value);
        if (current == state_->values_.end() ||
            current->second.version < update.version) {
            state_->values_.insert_or_assign(value, update);
        }
    }
    pending_updates_.clear();
}
```

`merge` bierze jeden muteks, więc dwa scalenia nie modyfikują jednocześnie
mapy wspólnej. Dla każdej wartości porównuje wersję wspólną z wersją lokalną.
Mniejsza wersja przegrywa; większa zostaje zapisana. Po udanym scaleniu lokalny
log jest czyszczony, więc powtórne `merge()` bez nowych operacji nie robi nic.

## Przykład „stara migawka scala się później”

Załóżmy, że obie migawki zaczynają od pustego zbioru:

```text
Migawka A: insert(20)  -> wersja 1
Migawka B: erase(20)   -> wersja 2
```

Następnie kolejność scalań jest pozornie niekorzystna:

```text
B.merge(): values_[20] = { present = false, version = 2 }
A.merge(): wersja A = 1, wersja wspólna = 2, więc A jest ignorowana
```

Końcowy zbiór nie zawiera `20`, ponieważ późniejszy **zapis**, a nie późniejsze
**wywołanie merge**, ma wygrać. Test `zad4.cpp` sprawdza dokładnie ten przypadek.

Dlaczego proste sumowanie migawek jest błędne?

```text
A widzi {20} i zachowuje je w starej kopii.
B później usuwa 20.
Suma A i B zawierałaby 20, czyli cofnęłaby nowsze erase B.
```

Przenosimy więc tylko zapisane zmiany z numerami wersji, nie pełne lokalne
zbiory.

## Synchronizacja i porządki pamięci

Wspólny kontener `values_` jest zawsze używany pod `mutex_`. Oznacza to, że
nie potrzebuje własnych atomików: `unlock()` jednego `merge` synchronizuje się
z późniejszym `lock()` kolejnego `merge` lub `snapshot()`.

`next_version_` jest atomikiem, ponieważ wiele migawek może równolegle prosić
o numer. Używa `memory_order_relaxed`, bo potrzebujemy wyłącznie jednej,
rosnącej i unikalnej kolejności wersji. Numer wersji nie publikuje danych
`local_values_`; ich przeniesienie do stanu wspólnego chroni muteks w `merge`.

To dobry przykład, że `relaxed` jest poprawne, gdy atomik pełni rolę licznika,
a relacje widoczności zapewnia inny mechanizm.

## Testy

`zad4.cpp` sprawdza:

1. Lokalną pracę migawki: wartość jest widoczna lokalnie po `insert`, a globalnie
   dopiero po `merge`.
2. Konflikt dwóch migawek: starsze `insert(20)` nie pokonuje nowszego
   `erase(20)`, nawet gdy scala się później.
3. Osiem wątków tworzy własne migawki, wstawia różne wartości i scala je. Wynik
   końcowy zawiera wszystkie osiem wartości.

## Złożoność i ograniczenia

Tworzenie migawki kosztuje O(n), bo kopiuje aktualne obecne elementy. Lokalne
`find`, `insert` i `erase` kosztują O(log n) lokalnego zbioru i nie czekają na
inne migawki. `merge` kosztuje O(k log m), gdzie `k` to liczba różnych wartości
w lokalnym dzienniku, a `m` to liczba wartości w mapie wspólnej.

Tombstones nie są obecnymi elementami, ale zajmują pamięć. W wersji produkcyjnej
można rozważyć bezpieczne czyszczenie starych tombstones po ustaleniu, że żadna
aktywna migawka nie potrzebuje ich wersji. To nie jest potrzebne w tym zadaniu
i celowo nie komplikuje podstawowego rozwiązania.

## Pytania na prezentację

**Dlaczego `merge` nie scala całych `local_values_`?** Bo lokalny zbiór może
zawierać stare dane, które zostały później usunięte przez inną migawkę.

**Dlaczego usunięcie nieobecnej wartości jest logowane?** Jest to późniejszy
zapis „tej wartości nie ma”; musi on wygrać ze starszym wstawieniem.

**Dlaczego migawka ma własne kontenery bez muteksu?** Ma jednego właściciela
- jeden wątek. Dzięki temu zwykłe operacje są równoległe.

**Dlaczego numer wersji może być `relaxed`?** Potrzebujemy tylko atomowo
rosnącego numeru. Widoczność danych stanu wspólnego zapewnia muteks `merge`.

**Po co `shared_ptr<State>`?** Migawka nie trzyma wiszącego wskaźnika, jeżeli
oryginalny obiekt `VersionedConcurrentSet` zostanie wcześniej zniszczony.
