1. Wyjątek procesora (exception) to zdarzenie, na które CPU reaguje przez przerwanie normalnego przepływu instukcji i przekazanie sterowania do odpwiedniego handlera.

2. Przerwanie sprzętowe (hardware interrupt) to zdarzenie podczas którego, nie wykonywane akutalnie program tylko hardwarewymuszają na procesorze przerwanie pracy i przejście w tryb uprzywilejowany jądra systemu operacyjnego. (asynchroniczne)

[] przerwanie od zegara systemowego,
[] naciśnięcie klawaisza klawaitury/myszy,
[] zakończenie operacji I/O przez dysk.

3. Wyjątek programowy (fault) to wyjątek spowodowany przez akaktualnie wykonywaną instrukcję programu przez CPU. (synchroniczne)

[] page fault - program odwowłuje się do porapwnej strony w pamięci, ale nie ma jej aktaulnie w RAM,
[] dzielenie przez zero,
[] naruszenie ochorny pamięci, czyli np. próba zapisu do strony służacje tylko do odczytu.

4. Pułapka (trap) to celowo wywołane przez program zdarzenie mające na celue przejście w tryb jądra systemu.

[] system call, program prości system operacyjny o wykonanie jakieś operacji
[] breakpoint ustawiony przez debugger,
[] single-step, wykowanie pojedyńczej operacji programu


5. W jakim scenariuszu wyjątek procesoa nie oznacza błędu wykonania programu?

Page fault. Program może odwołac się do poprawnego adresu pamięci, ale nie ma odpowiedniej strony akurat w RAM. Wtedy CPU generuję fault, jądro ładuję potrzebną stronę, a następnie program może ponowić swoją instrukcję i działa dalej normalnie.

Exception nie zawsze oznacza błąd programu - może to być tez normlany mechanizm współpracy CPU z SO.

6. Kiedy pułpaka jest generowana w wyniku prawidłwoej pracy programu?

Podczas zwykłego system calla. Program działący w trybie użytkownika wywołuje mechanizm, któ®e przkazuje sterowanie do jądra, ponieważ on nie ma uprawnień do tego działania.

