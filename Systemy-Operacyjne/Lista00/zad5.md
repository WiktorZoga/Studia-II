 ELF (Executable and Linkable Format) to format pliku wykonywalnego. 
 Wewnątrz pliku znajduje się kod, dane programu oraz instrukcje dla SO mówiące, jak załadować program do pamięci.

1. ELF Header: podstawowe informacje m.in entry point, czyli adres, od którego nalezy ropocząć wykonywanie programu

2. Sekcje: opisują logiczną zawartośc programu:
    [] .text - kod
    [] .data - zainicjalizowane dane globalne i statyczne
    [] .bss - niezainicjalizowane dane glbalne i statyczne
    [] .rodata - dane tylko do odczytu,
    [] .symtab - tablica symboli
    [] .strtab - tablica napisów używana przez tablice symboli
    [] ...


3. Segmenty: opisują jak fragment ma wyglądać w pamięci procesu np:
    [] segment może zawierać kod programu i mieć prawa tylko do odczytu i execute
    [] segment może zaiwerać dane i móc robić odczyty i zapisy


Różnica pomiędzy sekcjami a segmentami:
    sekcje to logiczny podział zawartości plikum używany przez linker
    segmenty opisują sposób ładowania programu do pamięci i są istotne dla SO

4. Program Header Table: nagłówki opisujące poszczególne segmenty. Każdy nagłówek jest opisywany przez m.in:
    [] p_offset - gdzie segment zaczyna sie w pliku,
    [] p_vaddr - pod jakim adresem wirtualnym segment ma być w pamięci,
    [] p_filesz - ile bajtów segment zajmuję w pliku,
    [] p_memsz - ile bajtów segment zajmuję w pamięci,
    [] p_flags - prawa dostępu np: read, write, execute,
    [] ...

    To instrukacja jak SO ma posługiwać się segmentami.

Skąd SO wie, pod jakm adresem umieścić segment? 
    [] Odczytuje to z Program Header Table

Skąd system wie, gdzie znajduje sie pierwsza instrukcja programu?
    [] Odczytuje entry point w ELF Header

Polecenie readelf:
    
    Flagi:
        -h ELF header
        -S sekcje
        -l segmenty

    
