Mechanizm obsługi przewań bazujący na wektorze przerwań polega na tym, że SO posiada specjalną tablice, której wpisy wskazują na procedury obsługi poszczególnych przerwań. Każde przerwanie ma przypisany swój odpowiedni numer/indentyfikator. Gdy procesor otrzyma przerwanie to wykorzystują jego numer wybiera adres właściwego interrupt handlera.

Przed rozpoczęciem wykonywania pierwszej instrukcji procedury obsługi przerwania procesor musi zachować stan pdtrzebny do późniejszego wznowienie przerwanego programu. Musi zachować adres wykonywania (program counter), stan procesora (rejestry, flagi). Jeśli przerwanie nastąpiło w trybie użytkownika, to procesor też musi przełączyć się w tryb jądra. 

Po zakończenieu procedury obsługi wykowyana jest specjlanai instrukcja poworotu z przerwania. Procesor odtwarza wtedy wcześniej zapisany, abyt wrócić do wcześniejszej pracy.

Procedura obsługi przerwań powinna dziłać w trybie jądra, ponieważ może potrzebować w wykonywać operacje uprzywilejowane, np zarządzać pamięcią. Stos jądra powinien być odrębny. Stos użytkownia kontroluje uzytkownik, może on być przepełniony, uszkodzony. Gdy te sotsy są rodzielone to jądro może bezpiecznie przechowywać swój stan podczas przerwania. Zyskujemy więcej niezależnosci, bezpieczeństwa. 
