# Zasady pracy w repozytorium Studia-II

## Ochrona materiałów

- Przed zmianą sprawdź strukturę, lokalne instrukcje `AGENTS.md` i stan Git.
- Traktuj istniejące materiały, zmiany niezacommitowane, pliki nieśledzone oraz
  zagnieżdżone repozytoria jako należące do użytkownika.
- Nie usuwaj, nie przenoś ani nie nadpisuj prac użytkownika. Przed zmianą
  istniejącej ścieżki znajdź jej odwołania i wybierz bezpieczne rozwiązanie.
- Zachowuj dotychczasowy układ przedmiotów, chyba że użytkownik poprosi
  o konkretną zmianę. Nie grupuj ich automatycznie jako archiwum ani legacy.
- Nie publikuj materiałów kursowych ani nie wysyłaj zmian do zdalnego repozytorium
  bez wyraźnego polecenia.

## Wspólna nauka

- Nie rozwiązuj całej listy ani nie wpisuj za użytkownika odpowiedzi końcowych.
- Zacznij od ustalenia, które zadanie użytkownik robi i co już rozumie lub
  próbował. Jeśli nie pokazał próby, pomóż mu zacząć od interpretacji polecenia.
- Wyjaśniaj potrzebne pojęcia, dziel problem na małe kroki i podawaj jedną
  użyteczną podpowiedź naraz. Daj użytkownikowi przestrzeń na samodzielny krok.
- Gdy użytkownik pokaże rozwiązanie, sprawdź jego rozumowanie, wskaż konkretny
  błąd lub potwierdź poprawny krok i wyjaśnij dlaczego.
- Pisz kod przykładowy tylko w zakresie potrzebnym do objaśnienia. Nie wklejaj
  kompletnego rozwiązania zadania, chyba że użytkownik wyraźnie o nie poprosi.
- Nie dopowiadaj treści zadania na podstawie samej nazwy pliku lub kodu. Poproś
  o brakujący fragment polecenia, kiedy ma znaczenie.

## Dobór modeli i agentów

Celem jest dobra jakość przy możliwie małym zużyciu wspólnego limitu Codex.

- Dla krótkich, jasno określonych zmian dokumentacji, indeksowania i prostych
  ćwiczeń wybieraj `GPT-6 Luna` z niskim poziomem rozumowania, jeśli jest dostępny.
- Dla zmian obejmujących kilka plików, decyzji wymagających oceny, trudniejszego
  debugowania lub dokładniejszego przeglądu wybieraj `GPT-6.1 Sol`, zwykle
  z niskim albo średnim poziomem rozumowania.
- `GPT-6 Astra`, wysokie poziomy rozumowania, Max i Ultra zostawiaj dla
  wyjątkowo trudnych, niejednoznacznych lub wysokiego ryzyka zadań.
- Zaczynaj od najlżejszego ustawienia, które pasuje do pracy. Podnoś model lub
  poziom rozumowania dopiero, gdy widać konkretną trudność albo brak jakości.
- Nie uruchamiaj dodatkowych agentów przy małych, sekwencyjnych zadaniach.
  Deleguj tylko niezależne części pracy, gdy równoległość realnie skróci
  wykonanie lub umożliwi niezależny przegląd.
- Dostępność modeli i wpływ zadań na limity mogą zależeć od planu i zmieniać się
  w czasie. Nie obiecuj konkretnego procentowego kosztu pojedynczego zadania.

## Struktura i jakość zmian

- Każdy przedmiot pozostaje w osobnym katalogu najwyższego poziomu.
- Nowe materiały umieszczaj zgodnie z `README.md` danego przedmiotu oraz jego
  lokalnym `AGENTS.md`, jeśli istnieje.
- Nie przenoś plików list bez sprawdzenia zależności, linków i poleceń
  uruchomieniowych.
- Ogranicz zmiany do prośby użytkownika. Nie poprawiaj przy okazji niezwiązanych
  błędów w starych notatkach ani kodzie.
- Po zmianie dokumentacji sprawdź linki względne oraz `git diff --check`.
  Uruchamiaj testy i kompilacje tylko wtedy, gdy zmieniony kod tego wymaga albo
  użytkownik poprosi o weryfikację.
