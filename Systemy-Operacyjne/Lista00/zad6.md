1. W jaki sposób jądro musi przygotować przestrzeń adresową procesu?

Kiedy program jest uruchamiany (np. przez exexv()), kernel beirze plik ELF i na podstawie Program Headers Table buduje nową przestrzeń adresową procesu.

Jądro interesują segmenty programu ich offsety, rozmiar w pamięci, uprawnienia. 

Jądro towrzy odpowiednie mapowanie pamięci wirtualnej, odpwoiednią tablicę stron. 

Oprócz segmentów OS tworzy też stos procesu.

Segmenty nie musza od razu wcałosci znjdować się w RAM, przy odpowiednim mapowaniu zostaną one ściągnięte na żądanie.

2. Co musi znajdować się na stosie w momencie wywolania procedury `_start`?

Według AMD64 przy wejściu do `_start` `%rsp` wskazuje na `argc`, następnie znajduję się `argv`, `NULL`, wskaźnik `envp`,`NULL`, a następnie `auxv` auxiliary vector.

    [] `argc` to liczba argumentów programu,
    [] `argv` to tablica tych wskaźników na te arguemnty,
    [] `envp` to tablica wskaźników na zmienne środowiskowe,    [] `auxv` auxiliary vector to dodatkowe infromacje dla jądra do uruchomienia procesu.

3. Do czego służy auxilary vector?

Jest to lista par z dodatkowymi informacjami dla procesu np.rozmiar strony (AT_PAGESZ), entry point (AT_ENTRY), adres Program Header Table (AT_PHDR).


4. W jaki sposób wywolać funkcje jądra?

Program działający w user mode nie może bezpośrednio wywołaćfunkcji jądra. Do tego używa `system call`.

W x86-64:
    [] ro rejestru %rax wpisujemy numer syscalla,
    [] argumenty umieszczamy w odpowiednich rejestrach,
    [] wykonujemy `syscall`

Numery to np:
    0 - read
    1 - write
    2 - open
    3 - close
    ...
    57 - fork
    59 - execve
    ...
    62 - kill
    ...

5. W ktorych rejestrach należy umieścic argumenty?

Kolejno w %rdi, %rsi, %rdx, %r10, %r8, %r9.
ABI x86-64 przewiduje maksymalnie 6 argumentów do syscalli.


6. Gdzie można spodziewać się wyników i jak jądro sygnalizuj niepowedzenie wywołania systemowgo?

Wynik powinien być w %rax. Jeżeli wystąpi błąd, jądro zwraca w %rax `ujemny` kod błędu.

