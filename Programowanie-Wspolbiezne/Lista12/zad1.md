## A) Dlaczego węzłów w tej strukturze nie można zwalniać bezpośrednio w funkcji `pop()`, bez dodatkowych zabiegów?

Nie możemy zwalniać węzłów bezpośrednio w `pop()`, ponieważ potencjalnie inny wątek może współbieżnie operować na tym obiekcie, co doprowadziłoby do błędu typu *use-after-free*.

W stosie bez blokad sprawa była prostsza: wątek wchodził do `pop()`, czytał `head`, a jedynym zagrożeniem był inny wątek wywołujący `pop()`, który ten sam `head` zdjął i usunął przy użyciu `delete`.

W przypadku kolejki topologia jest bardziej skomplikowana, ponieważ posiadamy dwa niezależne wskaźniki końcowe: `head` i `tail`. 

Rozważmy sytuację, gdy kolejka jest pusta: wskaźniki `head` oraz `tail` wskazują na ten sam element współdzielony (`dummy node`). 
Jeśli w tym samym momencie:
* Wątek B wywoła operację `push()`,
* Wątek A wywoła operację `pop()`,

wówczas Wątek A wejdzie do `pop()` i pomyślnie wykona instrukcję `head.compare_exchange_strong`, przesuwając głowę do przodu i odpinając stary `dummy node`. W tym samym czasie Wątek B wewnątrz `push()` posługuje się pobraną wcześniej kopią wskaźnika `tail`, która nadal pokazuje na ten sam `dummy node`. Gdyby Wątek A bezwarunkowo wykonał `delete` na starym węźle, Wątek B natychmiast wywołałby błąd *use-after-free* podczas próby zapisu danych lub aktualizacji pola `next`.

## B) Do czego służą zmienne internal_count, external_count oraz external_counters? Jakie są początkowe wartości tych zmiennych i jak mogą się one zmieniać podczas wykonania programu?

W badanej strukturze zarządzanie cyklem życia pamięci opiera się na rozbiciu liczników referencji na część zewnętrzną (powiązaną ze wskaźnikami globalnymi `head`/`tail`) oraz wewnętrzną (powiązaną bezpośrednio z ciałem węzła).

1. `external_count` (wewnątrz struktury `counted_node_ptr`)   
   * **Rola:** Jest to licznik referencji krótkoterminowych (tymczasowych). Odzwierciedla liczbę wątków, które w danej mikrosekundzie odczytały globalny wskaźnik `head` lub `tail` i zamierzają wejść do wnętrza węzła. Zabezpiecza on obiekt przed usunięciem w oknie czasowym pomiędzy odczytaniem adresu a zarejestrowaniem obecności w samym węźle.
   * **Początkowa wartość:** * Dla węzła `dummy` w konstruktorze kolejki: `1` (ponieważ `head` i `tail` dzielą ten węzeł na starcie, a stan początkowy wpisywany jest przez `head`).
     * Dla każdego nowego węzła alokowanego w `push()`: `1` (wątek tworzący posiada do niego wyłączną referencję).
   * **Jak się zmienia:** Jest zwiększany atomowo o `1` za pomocą pętli CAS wewnątrz `increase_external_count()` przez każdy wątek próbujący podjąć operację na `head` lub `tail`. Wartość ta jest "ściągana" w całości wraz z adresem węzła przez wątek, który wygra wyścig CAS modyfikujący strukturę.

2. `external_counters` (wewnątrz struktury `node_counter` w klasie `node`)   
   * **Rola:** Jest to licznik referencji strukturalnych (topologicznych). Wskazuje, ile stabilnych, długoterminowych wskaźników wewnątrz samej kolejki odwołuje się do tego obiektu. Węzeł znajdujący się wewnątrz kolejki jest powiązany z dwóch stron: przez wskaźnik swojego poprzednika (`next.ptr`) oraz przez jeden ze wskaźników granicznych (`head` lub `tail`).
   * **Początkowa wartość:** `2` (nadawana w konstruktorze `node()`). Jest przydzielana "awansem", aby chronić obiekt przed przedwczesnym zniszczeniem w trakcie procesu wpinania do struktury kolejki.
   * **Jak się zmienia:** Zmniejsza się atomowo o `1` wewnątrz funkcji `free_external_counter()` w dwóch momentach:
     * Gdy węzeł zostaje fizycznie odpięty z początku kolejki przez udany CAS na wskaźniku `head`.
     * Gdy wskaźnik `tail` zostaje przesunięty na nowy element, przez co stary węzeł przestaje być oficjalnym ogonem kolejki.
   * Spadek tej wartości do `0` oznacza, że węzeł został całkowicie wyizolowany z topologii kontenera.

3. `internal_count` (wewnątrz struktury `node_counter` w klasie `node`)
   * **Rola:** Jest to centralny rejestrator ostatecznego rozliczenia referencji. Gromadzi on sumaryczną informację o wszystkich zakończonych operacjach wątków, które przewinęły się przez dany węzeł.
   * **Początkowa wartość:** `0` (w konstruktorze `node()`).
   * **Jak się zmienia:** W trakcie aktywnego życia węzła przyjmuje wartości ujemne. Każdy wątek, który wycofuje się z operacji (ponieważ przegrał wyścig CAS na wskaźnikach globalnych lub zastał kolejkę pustą), wywołuje metodę `release_ref()`, która dekrementuje ten licznik o `1`. Z kolei wątek, który wygrywa wyścig CAS i odpina węzeł, wywołuje `free_external_counter()`, gdzie atomowo dodaje skumulowaną wartość `external_count` (pomniejszoną o `2`) do `internal_count`. Kiedy oba liczniki w obiekcie (`internal_count` oraz `external_counters`) osiągną jednocześnie wartość `0`, następuje bezpieczne wywołanie `delete this`.

## C) Jaka jest rola funkcji increase_external_count(), free_external_counter() oraz release_ref()?

1. `increase_external_count(std::atomic<counted_node_ptr>& counter, counted_node_ptr& old_counter)`
   Służy do bezpiecznego, atomowego zwiększenia licznika `external_count` w globalnym wskaźniku `head` lub `tail`. Wykorzystuje pętlę `CAS`, która gwarantuje, że wątek inkrementuje licznik dokładnie dla tego węzła, którego adres pobrał do pamięci podręcznej. Jeśli inny wątek zmodyfikował licznik w międzyczasie, funkcja ponawia próbę. Jeśli wskaźnik uległ całkowitej zmianie (węzeł odpięto), funkcja aktualizuje lokalny stan i zaczyna podbijać licznik na nowym, aktualnym wierzchołku, chroniąc wątek przed wejściem do usuniętej pamięci.

2. `free_external_counter(counted_node_ptr& old_node_ptr)`
   Jest to funkcja wywoływana wyłącznie przez "zwycięzcę" – wątek, któremu udało się pomyślnie zmodyfikować strukturę i odpiąć węzeł (poprzez przesunięcie `head` lub `tail`). Jej rolą jest trwałe rozliczenie odpiętego węzła: zmniejsza ona licznik referencji strukturalnych `external_counters` o `1` (ponieważ dany wskaźnik globalny już na niego nie wskazuje) oraz przelewa skumulowane z zewnątrz referencje tymczasowe z `external_count` do `internal_count`. Jeśli z kalkulacji wyniknie, że nikt więcej nie korzysta z obiektu, funkcja dokonuje ostatecznej destrukcji poprzez `delete`.

3. `node::release_ref()`
   Jest to funkcja wywoływana przez wątki, które muszą się "wycofać" z operowania na danym węźle (ponieważ przegrały wyścig `CAS` w pętli `pop()` / `push()` lub zastały kolejkę pustą). Jej zadaniem jest atomowe zmniejszenie licznika `internal_count` o `1` bezpośrednio wewnątrz obiektu `node`. Podobnie jak funkcja powyżej, po wykonaniu dekrementacji sprawdza stan liczników i jeśli oba osiągnęły `0`, bezpiecznie zwalnia pamięć węzła.

## D) Dlaczego release_ref() używa compare_exchange_strong() zamiast fetch_sub() przy zmniejszaniu licznika?

1. Liczniki wewnątrz węzła (`internal_count` oraz `external_counters`) są upakowane w jedną wspólną strukturę pól bitowych `node_counter`. Funkcja `fetch_sub` potrafi operować wyłącznie na prostych typach. Pętla CAS umożliwia precyzyjną modyfikację wyłącznie pola `internal_count` w lokalnej kopii struktury i bezpieczną podmianę całości.
2. Użycie wersji `strong` zamiast `weak` gwarantuje, że pętla `do-while` ponowi próbę wykonania operacji tylko wtedy, gdy inny wątek faktycznie zmodyfikował stan liczników w pamięci globalnej. Chroni to algorytm przed sprzętowymi, fałszywymi porażkami.

## E) W jakim przypadku może dojść do podwójnego delete, jeśli zignorujemy external_counters?

Do podwójnego wywołania instrukcji `delete` dojdzie w sytuacji, gdy wątki, które przegrały wyścig CAS i wycofały się z operacji (`release_ref()`), doprowadzą stan licznika `internal_count` do wartości `0`, podczas gdy węzeł **wciąż jest połączony ze strukturą kolejki** (jego pole `external_counters` wynosi `1`, ponieważ wciąż wskazuje na niego wskaźnik `tail` lub pole `next` poprzednika).

Jeśli zignorujemy `external_counters`, wątek dekrementujący licznik wewnętrzny błędnie uzna, że jest ostatnim użytkownikiem węzła i wywoła `delete ptr`. Pamięć zostanie zwolniona. W późniejszym czasie, gdy kolejny wątek fizycznie odepnie ostatni wskaźnik łączący ten węzeł z resztą struktury, licznik `external_counters` spadnie do zera. Wówczas ten kolejny wątek ponownie zweryfikuje warunek i po raz drugi spróbuje wywołać `delete` na tym samym adresie pamięci.

## F) Co się stanie, jeśli zapomnimy wywołać `free_external_counter()` po udanym compare_exchange na head lub tail?

Jeśli po udanej modyfikacji wskaźnika globalnego (`head` lub `tail`) zapomnimy wywołać funkcję `free_external_counter()`, doprowadzimy do **memory leak** odpiętego węzła. 

1. Licznik referencji strukturalnych `external_counters` wewnątrz węzła nigdy nie zostanie zdekrementowany, przez co na stałe pozostanie zawyżony (nie osiągnie wymaganej do usunięcia wartości `0`).
2. Skumulowane w odpiętym wskaźniku referencje tymczasowe (`external_count`) nie zostaną przelane do licznika `internal_count`, co uniemożliwi poprawne zrównoważenie bilansu wejść i wyjść wątków z tego węzła.
3. Wątki, które przegrały wyścig `CAS`, będą wciąż poprawnie odejmować `1` od `internal_count`, jednak z powodu braku przelewu i zawieszenia licznika strukturalnego, ostateczny warunek usunięcia nigdy nie zostanie spełniony. Węzeł na zawsze pozostanie nienaruszony w stercie.

## G) Dlaczego compare_exchange_strong() w free_external_counter() używa memory_order_acquire / memory_order_relaxed?

1. **Sukces (`std::memory_order_acquire`):**
   Gwarantuje, że późniejsze odczyty nie zostaną reorderowane przez procesor lub kompilator powyżej punktu CAS. Jest to absolutnie kluczowe dla poprawnego działania instrukcji warunkowej sprawdzającej, czy oba liczniki osiągnęły wartość `0`. Wątek podejmujący decyzję o wykonaniu `delete ptr` musi operować na zsynchronizowanych i najświeższych stanach obu liczników bitowych.

2. **Porażka (`std::memory_order_relaxed`):**
   Gdy operacja CAS kończy się niepowodzeniem, oznacza to, że wartość w pamięci globalnej uległa zmianie i żadne dane nie zostały przez nas zapisane (sprzęt jedynie zaktualizował naszą lokalną zmienną `old_counter`). W tym scenariuszu nie potrzebujemy żadnych barier synchronizacyjnych, ponieważ pętla `do-while` natychmiast ponowi próbę wykonania operacji. Jeśli kolejna próba zakończy się sukcesem, to wymagana bariera `acquire` zostanie poprawnie zaaplikowana. 

## H) Dlaczego potrzebujemy memory_order_acquire w increase_external_count()?

Funkcja `increase_external_count()` służy do powiadomienia innych wątków, że bieżący wątek rozpoczyna pracę na węźle i nie wolno go usunąć. Natychmiast po wyjściu z tej funkcji, wątki w metodach `pop()` oraz `push()` zaczynają odczytywać i modyfikować wewnętrzne pola węzła (takie jak `ptr->next` czy `ptr->data`).

Gdybyśmy zastosowali w tym miejscu porządek `relaxed`, procesor mógłby zoptymalizować wykonanie i pobrać dane z wnętrza węzła *z wyprzedzeniem*, zanim licznik `external_count` zostałby faktycznie podbity i opublikowany w pamięci globalnej. W tym krótkim oknie czasowym inny wątek mógłby odpiąć węzeł i wywołać na nim `delete`. W efekcie bieżący wątek operowałby na nieaktualnych lub uszkodzonych danych ze zwolnionej pamięci.

## I) Dlaczego pop() ładuje head z memory_order_relaxed, ale potem wykonuje compare_exchange_strong()?

Zastosowanie porządku `std::memory_order_relaxed` przy wstępnym ładowaniu wskaźnika `head` oraz w głównym wyścigu CAS jest celową optymalizacją wydajnościową, wynikającą z faktu, że pełna synchronizacja pamięci jest gwarantowana przez inne mechanizmy w tym kodzie:

1. **Wstępne ładowanie (`head.load`)**: Pierwszy odczyt głowy kolejki służy wyłącznie jako dostarczenie (jakieś) wartości bazowej dla algorytmu. Nawet jeśli pobrana wartość będzie nieaktualna (co jest dopuszczalne przy `relaxed`), zostanie to natychmiast zweryfikowane i skorygowane w kolejnej linii kodu przez funkcję `increase_external_count()`.
2. `increase_external_count()`: Funkcja ta operuje na pętli `CAS` z barierą `std::memory_order_acquire`. Oznacza to, że w momencie wyjścia z niej wątek i tak ma absolutną sprzętową gwarancję, że posiada najświeższy stan wskaźnika, a procesor nie dokonał niedozwolonych reorderingów. Dublowanie bariery `acquire` w wywołaniu `head.load` byłoby całkowicie bezużyteczne.

3. **Główny CAS w `pop()`**: Modyfikacja wskaźnika `head` wewnątrz `pop()` również bezpiecznie używa `relaxed`, ponieważ ta konkretna linia jedynie odpina wierzchołek strukturalnie, nie publikując żadnych nowych danych dla innych wątków. Ostateczne i bezpieczne domknięcie transakcji pamięciowej (`acquire`) i tak zostanie wykonane chwilę później wewnątrz funkcji `free_external_counter()`.

## J) Czy można by zastosować memory_order_seq_cst wszędzie dla bezpieczeństwa? Dlaczego tego nie robimy?

Z punktu widzenia czystej poprawności logicznej, zastosowanie `std::memory_order_seq_cst` w każdym miejscu algorytmu jest technicznie możliwe i poprawne, ale znacnzie wolniejsze.


## K) Co może pójść nie tak, jeśli dwa wątki jednocześnie wywołają push() i oba wykonają compare_exchange_strong() na tym samym ogonie tail bez zwiększenia external_count?

Pominięcie wywołania `increase_external_count()` przy współbieżnych operacjach `push()` doprowadzi do  utraty spójności liczników referencji i natychmiastowego błędu `use-after-free` albo `double-free`.

Jeśli dwa wątki równolegle spróbują zmodyfikować ogon bez uprzedniego atomowego zabezpieczenia go za pomocą inkrementacji `external_count`:
1. Wątek, który wygra wyścig, pomyślnie dopnie nowy element, przestawi globalny wskaźnik `tail` i wywoła `free_external_counter()`. Ponieważ licznik zewnętrzny nie odnotował obecności drugiego wątku, funkcja ta wyliczy błędny bilans przesunięcia ($\text{count\_increase} = \text{external\_count} - 2$), zaniżając wartość pola `internal_count` wewnątrz węzła.
2. Wątek, który przegrał wyścig, wycofa się i wywoła metodę `release_ref()`, która bezwarunkowo odejmie `1` od `internal_count`.
3. Z powodu braku wcześniejszej inkrementacji, ta dekrementacja spowoduje przedwczesne spadnięcie obu liczników wewnętrznych w węźle do zera. Węzeł zostanie fizycznie usunięty z pamięci za pomocą instrukcji `delete ptr`, mimo że przegrany wątek wciąż posiada do niego wskaźnik i w kolejnym obiegu pętli `for(;;)` podejmie próbę ponownego odczytu jego pól.

## L) Dlaczego, mimo braku użycia muteksów i jawnego zastosowania innych blokad, implementację tę nie możemy uznać za w pełni lock-free?

Algorytm jest uznawany za *lock-free* tylko wtedy, gdy awaria lub wstrzymanie (wywłaszczenie) dowolnego wątku w systemie nie blokuje możliwości dokonywania postępu przez pozostałe wątki.

W analizowanej implementacji ta zasada zostaje złamana wewnątrz metody `push()` z powodu rozbicia procesu dodawania elementu na kilka niezależnych kroków atomowych:
1. Wpisanie danych do aktualnego węzła ogona (`old_tail.ptr->data`).
2. Podczepienie nowego węzła pod pole następnika (`old_tail.ptr->next`).
3. Przesunięcie globalnego wskaźnika końcowego (`tail`).

Jeśli wątek wykonujący `push()` pomyślnie zrealizuje krok pierwszy (zajmie pole `data`), a następnie zostanie wywłaszczony przez scheduler systemu operacyjnego przed wykonaniem kroków 2 i 3, struktura kolejki przejdzie w stan przejściowy (niespójny). W tym stanie kolejne wywołania `push()` będą bezradnie kręcić się w pętli `for(;;)` (busy-waiting), oczekując na zwolnienie i przesunięcie ogona, a wywołania `pop()` będą zwracać puste wskaźniki.

