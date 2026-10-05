Program operuje na adresach wirtualncyh, które nastepnie są tłumaczone na adresy fizyczne w RAM.

virtual address -> page tables -> physical address

Strony pamięci:
    
    Pamięc wirtualna jest dzielona na strony (pages), a pamięć fizyczna na odpowiadające im ramki stron (page frames).

    W x86-64 typowy romziar strony to 4KiB = 2^12 B

    Adres wirtualny można traktowac jako:

    numer strony wirtualnej + offset wewnątrz strony

    Ponieważ strona ma 2^12 bajtów to ostatnie 12 bitów adresu służy nam do zapisania offsetu. Reszta jest do obsługi stron.

    Tablica stron tłumaczy więc virtual page na physical frame.

Wielopoziomowa tablica stron:
    
    Jedna duża tablica stron zajmowałaby bardzo dużo pamięci, dlatego x86-64 używa wielopoziomych tablic stron. 

    W klasycznym przypadku mamy 4 poziomy, każdy z nich ma 2^9 wpisów. Dostajemy w ten sposób 9 * 4 + 12 = 48 bitów wirtualnej przestrzeni adresowej.

    W ten sposób mamy też 512 * 64 bity spiów dalją 4KiB czyli tyle co jedna strona.

    Procesor przechodzi przez wszystkie poziomy aby dobrać sie do fizycznego adresu.

    Gybyśmy używali zwykłej tablicy to mielibyśmy w niej 2^48 / 2^16 = 2^36 stron, jeśli jeden wpiss to te 8 bajtów to dostajmy 512 GiB.

    Drzewiastra strukutra pozwalna nam nie tworzyc nieużywanych fragmentów tablicy. Liniowe podejście tutaj nie zadziała, dlatego że program nie musi efketywnie korzystać z zdanej mu struktury, powstałyby dziury i alokowalibyśmy znacznie więcej niż realnie używamy.


Uprawnienia dostępu:
    
    Wpisy w tablicy stron oprócz adresu następnej tablicy lub fizycznej ramki mają też informacje o uprawnieniach dostępu:
    [] present - czy wpis / strona jest obecna w pamięci
    [] read/write - czy dozwolny zapis
    [] user - czy user ma dostęp
    [] no execute - czy można wykonywać kod znajdujący się na tej stornie

    Jeśli podczas tłumacznie okaże się, że wpisu nie ma albo dany dostęp jest niedostępny to zgłaszany jest page fault.

TLB:
    
    Przejście pełnej tablicy stron jest kosztowe, dlatego CPU używa Translation Lookaside Buffer.

    Jest to mała, szybka pamięć cache przechowująca ostatnio używane tłumaczenia:
    virtual page -> physical frame oraz związne z nimi uprawnienia. 

    Jeżeli tłumacznie jest już w TLB to [TLB hit]. Procesor nie musis przechodzić przez wszystkie poziomy tablicy stron.

    Jeśli go nie ma [TLB miss] to CPU wykonuje normalny spacer po tablicach stron, a uzyskane tłumaczenie może zapisać w TLB.
