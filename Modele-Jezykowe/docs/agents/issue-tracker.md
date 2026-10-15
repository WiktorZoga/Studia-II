# Issue tracker: lokalny Markdown

Wszystkie ścieżki są względem katalogu `Modele-Jezykowe/`.
Zgłoszenia i specyfikacje przechowujemy w `.scratch/`.

## Konwencje

- Jeden temat na katalog: `.scratch/<temat>/`.
- Specyfikacja: `.scratch/<temat>/spec.md`.
- Jedno zgłoszenie na plik: `.scratch/<temat>/issues/<NN>-<nazwa>.md`.
  Numeracja zaczyna się od `01`; nie łącz zgłoszeń w jeden plik.
- Status zapisuj jako `Status:` na początku pliku. Role opisuje
  [triage-labels.md](triage-labels.md).
- Komentarze dopisuj na końcu pod nagłówkiem `## Comments`.

Polecenie „publish to the issue tracker” oznacza utworzenie lokalnego pliku.
Polecenie „fetch the relevant ticket” oznacza odczyt wskazanego pliku.

## Wayfinding

- Mapa: `.scratch/<temat>/map.md`, z notatkami, decyzjami i otwartymi pytaniami.
- Zgłoszenia: `.scratch/<temat>/issues/NN-<nazwa>.md`.
- Typ: `Type: research`, `prototype`, `grilling` albo `task`.
- Zależności: `Blocked by: NN, NN`; zgłoszenie odblokowuje się po rozwiązaniu
  wszystkich wymienionych zależności.
- Podejmij pierwsze dostępne zgłoszenie według numeru i zapisz `Status: claimed`.
- Po rozwiązaniu dopisz wynik pod `## Answer`, ustaw `Status: resolved`
  i dodaj do mapy krótką informację z odnośnikiem.

Statusy `claimed` i `resolved` dotyczą pracy wayfinder; role triage są osobną
konwencją używaną podczas triage.
