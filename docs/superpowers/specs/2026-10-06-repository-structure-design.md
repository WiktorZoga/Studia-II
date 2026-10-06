# Projekt uporządkowania repozytorium Studia-II

## Cel

Repozytorium ma być czytelnym, centralnym miejscem na materiały ze studiów.
Obecnie aktywne są przedmioty `Systemy-Operacyjne` i `Modele-Jezykowe`.
Pozostałe katalogi zachowują swoje nazwy, zawartość i położenie. Nie będą
oznaczane jako legacy ani przenoszone do wspólnego archiwum.

## Stan obecny

- `Systemy-Operacyjne` zawiera pierwszą listę zadań w `Lista00`.
- Starsze przedmioty znajdują się w osobnych katalogach najwyższego poziomu.
- Główny README zawiera tylko jednozdaniowy opis repozytorium.
- W głównym checkoutcie są lokalne, niezatwierdzone materiały użytkownika.
  Implementacja powstaje dlatego w osobnym worktree i nie będzie ich dotykać.

## Zakres zmian

### Główny poziom repozytorium

Główny `README.md` będzie zawierał:

- krótki opis przeznaczenia repozytorium;
- osobną sekcję aktywnych przedmiotów;
- listę pozostałych przedmiotów bez etykiety legacy;
- opis konwencji nazw i sposobu dodawania nowych materiałów;
- informację, że istniejące ścieżki pozostają stabilne.

Powstanie główny `AGENTS.md`, który zapisze zasady bezpiecznej pracy agentów AI
w całym repozytorium.

### Modele Językowe

Katalog `Modele-Jezykowe/P1` już zawiera `p1.pdf`, skrypty i pliki do pracy nad
zadaniami. Cała istniejąca zawartość i ścieżki zostają bez zmian. Zmiany dodadzą:

- `Modele-Jezykowe/README.md` jako indeks przedmiotu i przewodnik po pracy;
- `Modele-Jezykowe/P1/README.md` jako opis istniejących plików i punkt startowy
  do pracy nad listą;
- `Modele-Jezykowe/notatki/README.md` na ogólne notatki z kursu;
- `Modele-Jezykowe/projekty/README.md` na projekty większe niż pojedyncza lista.

Kolejne listy będą otrzymywać katalogi `P2`, `P3` itd., analogiczne do `P1`.
Materiały konkretnej listy pozostają razem z jej plikami. Nie tworzymy
powielającego je, centralnego katalogu `listy/` ani `materialy/`.

Praca dydaktyczna ma wspierać samodzielne rozwiązywanie: asystent objaśnia
pojęcia, pomaga rozbić problem na kroki, daje podpowiedzi stopniowo i omawia
próby użytkownika. Nie uzupełnia całej listy ani nie wpisuje gotowych odpowiedzi
bez wyraźnej prośby użytkownika.

### Systemy Operacyjne

Istniejący katalog `Lista00` pozostanie bez zmian. Nowy
`Systemy-Operacyjne/README.md` opisze bieżący układ i przyjmie `ListaNN` jako
konwencję dla kolejnych list. Nie powstaną dodatkowe poziomy katalogów, które
rozbiegałyby się z używaną już ścieżką.

Lokalne obrazy instalacyjne `*.iso` zostaną dodane do `.gitignore`. Istniejący
obraz nie zostanie usunięty z dysku.

## Zasady pracy z modelami i agentami AI

Domyślna polityka minimalizuje zużycie limitu:

1. `gpt-6-luna` z niskim poziomem rozumowania obsługuje małe, dobrze określone
   zadania: README, porządkowanie plików, drobne poprawki i proste ćwiczenia.
2. `gpt-6.1-sol` z poziomem niskim lub średnim jest używany przy pracy nad
   kilkoma plikami, implementacji wymagającej decyzji projektowych, debugowaniu
   i końcowym przeglądzie ważniejszych zmian.
3. `gpt-6-astra`, wysokie poziomy rozumowania, Max i Ultra są zarezerwowane dla
   problemów, które faktycznie wymagają długiego rozumowania albo szerokiego
   kontekstu.
4. Dodatkowi agenci są uruchamiani tylko wtedy, gdy co najmniej dwa niezależne
   zadania można wykonać równolegle. Małe zadania wykonuje jeden agent.
5. Najpierw wybierany jest najlżejszy model pasujący do zadania. Eskalacja
   następuje dopiero po napotkaniu konkretnej trudności lub ryzyka jakości.

Ta reorganizacja zostanie wykonana przez jednego agenta; równoległa praca nie
przyniosłaby korzyści proporcjonalnej do dodatkowego zużycia.

## Weryfikacja

Po implementacji zostaną sprawdzone:

- czystość głównego checkoutu użytkownika;
- lista zmienionych i nowych plików w worktree;
- poprawność względnych linków Markdown;
- brak przeniesień i usunięć istniejących materiałów;
- ignorowanie obrazów `*.iso`.

Repozytorium nie ma jednego wspólnego zestawu testów dla wszystkich
przedmiotów. Zmiana nie ingeruje w kod ćwiczeń, więc nie wymaga budowania
poszczególnych projektów.

## Poza zakresem

- przenoszenie lub zmiana nazw istniejących przedmiotów;
- poprawianie kodu i notatek ze starszych semestrów;
- modyfikowanie lokalnych, niezatwierdzonych plików użytkownika;
- publikowanie zmian lub wysyłanie ich do zdalnego repozytorium.
