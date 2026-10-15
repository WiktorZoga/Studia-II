# Pracownia 1

Polecenia: [p1.pdf](p1.pdf). Poniższe komendy uruchamiaj z katalogu
`Modele-Jezykowe/`, używając środowiska zarządzanego przez `uv`.
W rozwiązaniach używamy modelu `sdadas/polish-gpt2-large`.

## Zadanie 1 — chatbot

```bash
uv run python P1/zad1.py
```

Bot generuje pięć propozycji i wybiera odpowiedź o liczbie słów najbliższej 10.
W promptcie zachowuje pięć ostatnich wymian. Pusty wpis kończy rozmowę.
Skrypt korzysta z urządzenia `mps` (Apple Silicon).

## Zadanie 2 — kolejność słów

```bash
uv run python P1/zad2.py
```

Program przegląda unikalne permutacje słów i pokazuje pięć najlepiej ocenionych
przez model. Mniejsza średnia strata oznacza wyższą pozycję w rankingu.
Przykłady zdań można zmieniać na końcu skryptu.

## Zadanie 3 — wydźwięk opinii

```bash
uv run python P1/zad3.py
```

Program ocenia opinie z [reviews_for_task3.txt](reviews_for_task3.txt)
dwiema metodami: porównuje średnią stratę opinii z dopisaną oceną oraz
prawdopodobieństwo całej etykiety po opinii. Nie stosuje few-shot.
Na końcu wypisuje skuteczność każdej metody.

## Zadanie 4 — pytania o fakty

```bash
uv run python P1/zad4/analyze_task4_prefixes.py
uv run python P1/zad4/zad4.py
```

Pierwsza komenda pokazuje częste prefiksy znalezione przez trie i liczebność
wybranych grup. Nie ładuje modelu.

Druga komenda odpowiada na pytania:

- `Czy…` — większościowa odpowiedź tak/nie z demonstracji;
- `W którym wieku…` — porównanie strat kandydatów, również p.n.e.;
- pozostałe grupy — po trzy demonstracje few-shot;
- pytania zawierające alternatywy ze słowem „czy” — ogólny few-shot.

Demonstracje to przykładowe pary pytanie–odpowiedź umieszczane w promptcie
lub używane przez heurystykę. Ich pytania pomijamy przy obliczaniu wyniku.
Model nie jest dostrajany. Odpowiedzi wzorcowe pochodzą z dostarczonego
[task4_answers.txt](zad4/task4_answers.txt).

Skrypt zadania 4 wymaga modelu pobranego wcześniej do lokalnego cache.
Jeśli go jeszcze nie masz, wcześniejsze uruchomienie zadania 2 lub 3 pobierze
ten sam model. Na końcu zadanie 4 wypisuje szacowaną skuteczność według
prostego dopasowania pełnych fragmentów odpowiedzi po normalizacji.

## Wyniki do uzupełnienia po własnym uruchomieniu

| Pomiar | Poprawne / wszystkie | Skuteczność |
| --- | --- | --- |
| Zadanie 3: całkowity napis | — | — |
| Zadanie 3: etykieta po opinii | — | — |
| Zadanie 4 | — | — |

Wpisz rzeczywiste wyniki z konsoli oraz datę uruchomienia: **do uzupełnienia**.
