# Programowanie współbieżne w C++

Rozwiązania ćwiczeń w C++20/C++23, pogrupowane w listy 01–14.
Kod, rozwiązania opisowe oraz materiały są rozdzielone według ich funkcji.

## Struktura

```text
ListaNN/
  zadN.cpp          rozwiązanie lub program testowy
  zadN.md           rozwiązanie opisowe albo uzasadnienie
  progN.cpp         przykład, jeśli występuje
  *.h               komponenty używane przez zadania
  CMakeLists.txt    konfiguracja listy
  materialy/        PDF-y i listingi prowadzących
  drafts/           zachowane wcześniejsze próby, poza budowaniem
cmake/              wspólna konfiguracja kompilacji
```

Markdowny `zad*.md` są częścią rozwiązań. Niektóre zadania mają wyłącznie
rozwiązanie tekstowe — nie odpowiada im osobny plik C++.

## Budowanie i testy

Wymagania: CMake 3.20+, Ninja i kompilator obsługujący C++20 oraz C++23.
Polecenia uruchamiaj z katalogu `Programowanie-Wspolbiezne`:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Jedno zadanie:

```sh
cmake --build build/debug --target lista14_zad2
./build/debug/Lista14/lista14_zad2
```

## Listy

| Listy | Tematy |
| --- | --- |
| [01](Lista01/)–[05](Lista05/) | Wątki, mutexy, semafory, future i atomiki |
| [06](Lista06/)–[08](Lista08/) | Model pamięci, synchronizacja, kolejki i bariery |
| [09](Lista09/)–[12](Lista12/) | Struktury współbieżne i odzyskiwanie pamięci |
| 13 — tylko lokalnie | Stosy EBR/IBR, historia zbioru i migawki; zadania 1–4 |
| 14 — tylko lokalnie | Zarządzanie dostępem, silniki zadań, sortowanie i kolejka; zadania 1–5 |

Listy 13 i 14 są tymczasowo wyłączone z publikacji przez `.gitignore`.
Lokalnie testy CTest obejmują cztery zadania listy 13 i pięć zadań listy 14.
Zadania 14.6–14.9 nie są zaimplementowane. Pozostałe listy zawierają
również przykłady i eksperymenty uruchamiane ręcznie. Kompilacja i przejście
testów nie stanowią dowodu poprawności algorytmów współbieżnych.

## Ograniczenia

Silniki zadań należy zamykać spoza ich własnych workerów. Kolejka dwóch stosów
wymaga niezgłaszającego wyjątków konstruktora przenoszącego. Sortowanie używa
iteratorów losowego dostępu i domyślnie konstruowalnych, kopiowalnych elementów;
dla wartości logicznych ma wariant sekwencyjny.

Sanitizery są opcjonalne (`cmake --preset asan`). W lokalnym środowisku macOS
ASan zatrzymywał się podczas inicjalizacji przed `main`; nie jest to pozytywny
wynik kontroli pamięci.

Materiały prowadzących mogą być dostępne tylko lokalnie. Nowych materiałów
nie należy publikować bez potwierdzenia praw do ich udostępnienia.
