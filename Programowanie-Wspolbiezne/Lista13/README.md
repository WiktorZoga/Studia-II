# Lista 13 - notatki do obrony

## Budowanie i uruchamianie

Z katalogu `Lista13`:

```sh
cmake -S . -B build -DENABLE_SANITIZERS=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure --parallel 1
```

Domyślnie projekt włącza AddressSanitizer. Gdy środowisko go nie obsługuje,
ustaw `-DENABLE_SANITIZERS=OFF`, jak w poleceniu powyżej. Do sprawdzania
wyścigów wybierz ThreadSanitizer:
`cmake -S . -B build-tsan -DUSE_TSAN=ON`.

## Zadanie 1 - EBR

`lock_free_stack<T>` jest stosem Treibera. `push` publikuje nowy węzeł przez
CAS, a `pop` atomowo zdejmuje aktualną głowę. Węzła zdjętego przez `pop` nie
wolno od razu usunąć: inny wątek mógł właśnie odczytać jego adres.

`EBRDomain` daje każdemu uczestniczącemu wątkowi slot z aktualną epoką.
`ReadGuard` oznacza wejście do sekcji odczytu i gwarantuje jej opuszczenie
nawet przy wyjątku. Zdjęty węzeł trafia na listę retired. Jest usuwany dopiero,
gdy jego epoka jest starsza niż wszystkie aktywne epoki. Parametr konstruktora
stosu musi pomieścić wszystkie różne wątki korzystające z tej domeny.

## Zadanie 2 - IBR

`lock_free_stack_ibr<T>` używa tego samego problemu, lecz zapisuje dla węzła
epokę narodzin i wycofania. Czytelnik rezerwuje przedział `[lower, upper]` i
odświeża górną granicę przed kolejną próbą zdjęcia głowy stosu. Obiekt można
usunąć tylko wtedy, gdy przedział życia obiektu nie przecina żadnego aktywnego
przedziału czytelnika. EBR pamięta jedną epokę; IBR zachowuje więcej informacji
i dlatego lepiej opisuje długie przejścia po strukturze.

## Zadanie 3 - HistorySet<T>

`HistorySet<T>` używa `std::set`, kolejki historii i jednego muteksu. Każde
wywołanie `insert` lub `erase` jest wpisywane do historii, również gdy nic nie
zmieniło. `undo` cofa skuteczną zmianę albo tylko usuwa wpis pustej operacji.
Najpierw powstaje wpis historii, dopiero potem wykonywane jest `erase`; dzięki
temu wyjątek podczas kopiowania `T`, porównania lub alokacji nie pozostawia
częściowo zmienionego zbioru.

## Zadanie 4 - VersionedConcurrentSet<T>

`snapshot()` tworzy prywatną kopię zbioru dla jednego wątku. `insert`, `erase`
i `find` działają tylko na niej i nie biorą muteksu. Każdy zapis dostaje globalny
rosnący numer. Podczas `merge()` jeden muteks chroni zbiór główny, a dla każdej
wartości przyjmowana jest tylko zmiana o nowszym numerze. Usunięcia zostają jako
znaczniki (tombstones), więc późniejsze scalenie starej migawki nie odtwarza
usuniętego elementu. To jest zasada „ostatni zapis wygrywa”.

Nie należy niszczyć stosów EBR/IBR, dopóki korzystające z nich wątki nie
zakończą pracy. Pojedyncza migawka z zadania 4 jest przeznaczona dla jednego
wątku; współdzielony jest tylko obiekt główny i operacja `merge`.
