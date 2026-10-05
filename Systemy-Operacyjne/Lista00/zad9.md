Czym jest narzędzie `ltrace`?

`ltrace` służy do śledzenia wywołań funkcji bibliotek współdzielonych wykonywanych przez program. Opcja `-S` dodaje do tego jestszcze wywołania systemowe (syscall'e)

Polecenia do wykonaia
    ltrace /so26_lista_0/1_ls so26_lista_0

Mamy tam taki fragment:

opendir("./so26_lista_0" <unfinished ...>
openat@SYS(AT_FDCWD, "./so26_lista_0", 0x90800, 00)      = 3
fstat@SYS(3, 0x7ffd3fab0e70)                             = 0
getrandom@SYS(0x7f68c4932218, 8, 1, 0)                   = 8
brk@SYS(nil)                                             = 0x55ed09957000
brk@SYS(0x55ed09978000)                                  = 0x55ed09978000
<... opendir resumed> )                                  = {3}

Mamy tam 5 syscalli
    [] openat - otwiera lub tworzy pliki
    [] fstat - pobiera metadane o stanie pliku przekzywanego deskryptorem
    [] getrandom - wypełnia podany bufor losowymi bajtami
    [] brk - zmienia rozmiar sterty procesu, ustawiajac wskaźnik końca programu (program break) na wskazany adres

Zapewnie opendir chce uruchomić pierwsze dwa: openat, fstat

Kolejny fragment:

readdir({3} <unfinished ...>
getdents64@SYS(3, 0x55ed099572d0, 0x8000, 3)             = 432
<... readdir resumed> )                                  = {38088, ".2_cat.d"}

Czyli ewidentnie readdir odpala:
    [] getdents64 - odczytuje wpisy z katalogu do wskazanego bufora pamięci

Jednak w koljnych fragmentach już tego nie ma!!! Dlaczego?

<... puts resumed> )                                     = 9
readdir({3})                                             = {51789, "1_ls"}
puts("1_ls" <unfinished ...>
write@SYS(1, "1_ls\n", 51_ls
)                                = 5
<... puts resumed> )                                     = 5
readdir({3})                                             = {44832, "1_ls.c"}
puts("1_ls.c" <unfinished ...>
write@SYS(1, "1_ls.c\n", 71_ls.c
)                              = 7
<... puts resumed> )                                     = 7
readdir({3})                                             = {46017, "Makefile"}

Kolejny:

closedir({3} <unfinished ...>
close@SYS(3)                                             = 0
<... closedir resumed>

Czyli closedir odpala:
    [] close - zamyka deskryptor plików

--- 

Jaka standardowa funkcja biblioteczna jest wywoływana przez `printf`?

Tutaj widzać, że jest to puts()

puts(".2_cat.d" <unfinished ...>
fstat@SYS(1, 0x7ffd3fab0db0)                             = 0
write@SYS(1, ".2_cat.d\n", 9.2_cat.d
)                            = 9
<... puts resumed> )                                     = 9
readdir({3})                                             = {51789, "1_ls"}
puts("1_ls" <unfinished ...>
write@SYS(1, "1_ls\n", 51_ls
)                                = 5
<... puts resumed> )                                     = 5

printf nigdzie nie ma. 

Jakie wywołanie systemowe przekazuje dane z «printf» do jądra?

`puts` odpala `write` syscalla.

Dlaczego pojawia się ono z opóźnieniem?

Nie widzę żadnego opóźnienia, pod każdym `puts` jest `write`. 
Chodzi o buforowanie std i/o. Sycall zapewne pojawia się dopiero przy opróżnianiu bufora na dane.

---

Do czego służy wywołanie systemowe `brk`?

`brk` służy do zmiany końca obszaru danych procesu - program break. W praktyce jest to zwiazna z powiększanie sterty

GDB:

Catchpoint 2 (call to syscall brk), __brk (addr=addr@entry=0x0)
    at ../sysdeps/unix/sysv/linux/brk_call.h:24
warning: 24     ../sysdeps/unix/sysv/linux/brk_call.h: No such file or directory
(gdb) bt
#0  __brk (addr=addr@entry=0x0) at ../sysdeps/unix/sysv/linux/brk_call.h:24
#1  0x00007ffff7ed4ea7 in __GI___sbrk (increment=increment@entry=135168) at ./misc/sbrk.c:59
#2  0x00007ffff7e66c42 in __glibc_morecore (increment=increment@entry=135168)
    at ./malloc/morecore.c:29
#3  0x00007ffff7e67b42 in sysmalloc (nb=nb@entry=656, av=0x7ffff7facac0 <main_arena>)
    at ./malloc/malloc.c:2711
#4  0x00007ffff7e68eeb in _int_malloc (av=av@entry=0x7ffff7facac0 <main_arena>,
    bytes=bytes@entry=640) at ./malloc/malloc.c:4539
#5  0x00007ffff7e68fc3 in tcache_init () at ./malloc/malloc.c:3317
#6  0x00007ffff7e6997a in tcache_init () at ./malloc/malloc.c:3313
#7  tcache_try_malloc (bytes=32816, memptr=<synthetic pointer>) at ./malloc/malloc.c:3361
#8  __GI___libc_malloc (bytes=bytes@entry=32816) at ./malloc/malloc.c:3395
#9  0x00007ffff7ea3705 in __alloc_dir (fd=fd@entry=3, close_fd=close_fd@entry=true,
    flags=flags@entry=0, statp=statp@entry=0x7fffffffe130)
    at ../sysdeps/unix/sysv/linux/opendir.c:115
#10 0x00007ffff7ea377f in opendir_tail (fd=3) at ../sysdeps/unix/sysv/linux/opendir.c:63
#11 0x00007ffff7ea3820 in __opendir (name=<optimized out>)
    at ../sysdeps/unix/sysv/linux/opendir.c:86
#12 0x0000555555556691 in main (argc=<optimized out>, argv=0x7fffffffe318) at 1_ls.c:11
(gdb)


Tutaj winowajcą dla brk zdaje się być sysmalloc.
