# 03: Poprawne porównanie etykiet opinii

**What to build:** Obie proste metody klasyfikacji można uruchomić z katalogu
projektu, a metoda warunkowa ocenia wszystkie tokeny etykiety.

**Blocked by:** None (can start immediately).

**Status:** resolved

- [x] Zachowane są dwie konstrukcje napisów bez few-shot.
- [x] Metoda warunkowa obejmuje całą etykietę, również wielotokenową.
- [x] Kontekst jest zamaskowany przy obliczaniu straty etykiety.
- [x] Dane opinii są odnajdywane względem skryptu.
- [x] Sprawdzono tokenizację i jeden krótki przebieg każdej metody.
- [x] Bez dodatkowych klas, frameworków i infrastruktury testowej.

## Wynik

Potwierdzono etykiety o 1 i 2 tokenach, skończone straty obu pełnych etykiet, pojedyncze wywołania obu metod oraz ścieżkę do danych.

Końcowy przegląd Standards i Spec: brak nowych ustaleń wymagających poprawy.
