Co robi narzędzie `strace`?

`strace` służy do śledzenia wywołań systemowych wykonywnaych przez program oraz sygnałów, które otrzymuje.

Opcja `-e trace=` pozwala wybrać zbiór syscalli, które nas interesują.

Użycie:

strace -e trace=read,write ./so26_lista_0/2_cat

read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0p\236\2\0\0\0\0\0"..., 832) = 832
read(0, witam
"witam\n", 4096)                = 6
write(1, "witam\n", 6witam
)                  = 6
read(0, serdecznie
"serdecznie\n", 4096)           = 11
write(1, "serdecznie\n", 11serdecznie
)            = 11
read(0, "", 4096)                       = 0
+++ exited with 0 +++

Widać tutaj read(0, ...) oraz write(1, ...), co wporst pokazuję, że program oczekuje na odczyt z deskryptora 0 (stdin) oraz pisze do dyskryptora 1 (stdout). 

Wciskając na końcu `ctrl+d` (puste wejście, EOF) widzimy, że read zwrócił 0 (zerowa długość) oraz cały program kończy się z kodem 0. 

(pierwszy read to odczyt ELF)
---

Aby zmienić ten program tak, by czytał z pliku podane w lini poleceń wystraczy dodać arguemnty w `main`, ustawianie file discriptora, oraz otwrzenie pliku (O_RDONLY - open read only).

Co się stanie, jeśli przekaże ścieżkę do katalogu zamist do pliku zwykłego?

./so26_lista_0/3_cat ./so26_lista_0
read error

Dostajemy bład `read`.

openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
close(3)                                = 0
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3                                               read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0p\236\2\0\0\0\0\0"..., 832) = 832
close(3)                                = 0
openat(AT_FDCWD, "./so26_lista_0/", O_RDONLY) = 3            read(3, 0x7fff8d4fa3e0, 4096)           = -1 EISDIR (Is a directory)                                                      write(2, "read error", 10read error)              = 10
write(2, "\n", 1                                             )                       = 1
+++ exited with 1 +++

Widzemy, że program dostał wolny deskryptor `3`, ale jako że podaliśmy directory zamist pliku to:
read(3, 0x7fff8d4fa3e0, 4096)           = -1 EISDIR (Is a directory)

openat() sie udało, dopiero read() zwróciło -1 z błede EISDIR.
