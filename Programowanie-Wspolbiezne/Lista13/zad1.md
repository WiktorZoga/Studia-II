# Zadanie 1 - stos Treibera z Epoch-Based Reclamation

## Cel zadania

W zadaniu używamy **Epoch-Based Reclamation (EBR)** w bezblokowym stosie
Treibera (`lock_free_stack<T>`). Problemem nie jest atomowa zmiana `head`, ale
czas życia starego węzła. Samo `std::atomic<node*>` nie zabrania innemu
wątkowi wykonać `delete` węzła, którego adres został już odczytany.

```text
wątek A: head.load() -> X
wątek B: zdejmuje X i wykonuje delete X
wątek A: odczytuje X->next       // use-after-free
```

EBR oddziela **logiczne usunięcie** węzła ze stosu od jego **fizycznego
zwolnienia** przez `delete`.

## Struktura danych

W `lock_free_stack.h` węzeł ma wartość i zwykły wskaźnik `next`, a jedynym
współdzielonym wskaźnikiem stosu jest atomowy `head`:

```cpp
struct node {
    T data;
    node* next;
};

std::atomic<node*> head{nullptr};
EBRDomain ebr;
```

`EBRDomain` przechowuje bieżącą epokę, slot z epoką dla każdego wątku oraz
listę obiektów oczekujących na zwolnienie. Konstruktor stosu przyjmuje
`max_threads`; musi wystarczyć dla wszystkich różnych wątków korzystających
z tej domeny w całym jej życiu. Mapa `thread_local` przypisuje danemu wątkowi
stały slot w konkretnej domenie.

## `push` krok po kroku

```cpp
node* new_node = new node{data, head.load(std::memory_order_relaxed)};
while (!head.compare_exchange_weak(
    new_node->next, new_node,
    std::memory_order_release, std::memory_order_relaxed)) {
}
```

1. Wątek tworzy prywatny, jeszcze niewidoczny węzeł.
2. Wpisuje aktualną głowę jako kandydaturę na `next`.
3. CAS sprawdza, czy `head` nadal ma tę wartość.
4. Gdy CAS się nie powiedzie, automatycznie wpisuje świeżą wartość `head` do
   `new_node->next`; pętla ponawia próbę.
5. Udany CAS publikuje gotowy węzeł jako nową głowę.

`push` nie dereferencjuje starej głowy - używa jej wyłącznie jako liczby
adresowej w CAS. Dlatego nie potrzebuje ochrony EBR.

## `pop` i guard RAII

`pop` musi czytać `current_head->next` oraz `current_head->data`, dlatego
najpierw tworzy guard:

```cpp
auto guard = ebr.read_guard();
node* current_head = head.load(std::memory_order_acquire);

while (current_head && !head.compare_exchange_weak(
    current_head, current_head->next,
    std::memory_order_acq_rel, std::memory_order_acquire)) {
}
```

`ReadGuard` w konstruktorze publikuje epokę wątku, a w destruktorze ustawia
slot na `EBR_INACTIVE`. To jest zastosowanie RAII: opuszczenie sekcji EBR
następuje także przy `return` i wyjątku.

Kolejność działania jest kluczowa:

1. Wątek ogłasza wejście do bieżącej epoki.
2. Odczytuje `head` i może bezpiecznie dereferencjować obserwowany węzeł.
3. Udany CAS logicznie zdejmuje węzeł ze stosu.
4. Wartość trafia do `std::optional<T>`.
5. Węzeł przechodzi do `ebr.retire(current_head)`, a nie do `delete`.
6. Guard kończy ochronę przy wyjściu z funkcji.

Gdy CAS nie powiedzie się, jego argument `current_head` dostaje aktualną
wartość głowy. Porządek błędu `acquire` sprawia, że po zobaczeniu węzła
opublikowanego przez inny wątek wolno czytać jego poprawnie zainicjalizowane
pola.

## Mechanizm EBR

`EBRDomain` zawiera:

- `global_epoch` - bieżącą epokę domeny;
- `thread_epochs` - ogłoszoną epokę każdego aktywnego wątku;
- `retire_list_head` - bezblokową listę opisów wycofanych obiektów.

`retire(ptr)` zapisuje obiekt wraz z `retire_epoch` na liście retired. Następnie
`reclaim()` skanuje sloty aktywnych wątków i oblicza najstarszą obserwowaną
epokę `min_active_epoch`. Obiekt wolno zwolnić wyłącznie wtedy, gdy:

```text
retire_epoch < min_active_epoch
```

Inaczej choć jeden aktywny wątek może jeszcze korzystać ze wskaźnika uzyskanego
w epoce wycofania lub wcześniejszej. Węzły niespełniające warunku są ponownie
odkładane na listę retired. Epoka globalna zwiększa się tylko wtedy, gdy
wszystkie aktywne wątki dogoniły aktualną epokę.

## Porządki pamięci

| Miejsce | Porządek | Znaczenie |
|---|---|---|
| CAS publikujący `head` w `push` | `release` | Inicjalizacja `data` i `next` jest widoczna przed publikacją adresu. |
| `head.load` w `pop` | `acquire` | Po zobaczeniu adresu wolno czytać pola węzła. |
| Udany CAS w `pop` | `acq_rel` | Jednocześnie pobiera stary stan i publikuje nową głowę. |
| Publikacja epoki czytelnika | `release` | Reclaimer widzi, że wątek jest aktywny. |
| Odczyt epok i listy retired | `acquire` | Reclaimer pracuje na opublikowanych danych. |
| Przydzielanie numeru slotu | `relaxed` | Potrzebna jest tylko unikalność numeru, nie publikacja danych stosu. |

`relaxed` nadal oznacza operację atomową. Nie daje tylko relacji widoczności
z innymi zapisami.

## Testy

`zad1.cpp` sprawdza kolejno:

1. Stos pusty i kolejność LIFO (`20` jest zdjęte przed `10`).
2. Ośmiu producentów zapisuje rozłączne zakresy wartości; test sprawdza, że
   nie zgubiono ani nie powielono żadnej wartości.
3. Osiem producentów i osiem konsumentów działa równolegle. Tablica `seen`
   weryfikuje zasadę „dokładnie raz”.

Testy sprawdzają zachowanie programu. Nie są formalnym dowodem poprawności
algorytmu; dodatkowym narzędziem jest ASan lub TSan, jeśli środowisko je
obsługuje.

## Złożoność i ograniczenia

`push` i udany `pop` mają oczekiwaną złożoność O(1). `reclaim()` skanuje sloty,
więc jego koszt zależy od liczby wątków. Aktualizacja głowy nie używa muteksu,
ale postęp praktycznego programu może zależeć od alokatora i od tego, czy wolne
wątki opuszczają sekcje EBR. Destruktor stosu wolno wywołać dopiero po
zakończeniu wszystkich `push` i `pop`.

## Pytania na prezentację

**Dlaczego atomowy `head` nie wystarcza?** Atomowo chroni zmianę adresu, ale
nie czas życia obiektu pod tym adresem.

**Dlaczego nie ma `delete` po udanym CAS?** Inny wątek mógł wcześniej odczytać
starą głowę i dopiero przygotowuje własny CAS.

**Dlaczego guard jest tylko w `pop`?** Tylko `pop` dereferencjuje węzeł
należący wcześniej do wspólnej struktury.

**Co daje RAII?** Każda ścieżka wyjścia, także wyjątek, zwalnia rezerwację
epoki wątku.
