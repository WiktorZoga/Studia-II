# 04: Wiarygodna ocena odpowiedzi na pytania

**What to build:** Prosty program odpowiada na pytania przez heurystykę,
few-shot i porównanie prawdopodobieństw, a wynik nie zalicza częściowych liczb
ani nie obejmuje pytań użytych jako demonstracje.

**Blocked by:** None (can start immediately).

**Status:** resolved

- [x] Przykłady demonstracyjne są wykluczone z oceny, bez losowego podziału
  na train/test i bez trenowania modelu.
- [x] Heurystyka większościowa korzysta tylko z odpowiedzi demonstracyjnych.
- [x] Dopasowanie odbywa się po pełnych tokenach; 10 nie pasuje do 1,
  a XVIII nie pasuje do XVI. Era p.n.e. jest rozróżniana.
- [x] Kandydaci wieku obejmują również p.n.e.
- [x] Pytania zawierające alternatywy trafiają do few-shot zamiast tak/nie.
- [x] Trie pozostaje w analizatorze, a routing nie buduje zbędnego drzewa.
- [x] Krótka kontrola przykładów i jednego wywołania modelu, bez pełnej oceny.
- [x] Proste funkcje, bez nowych klas, frameworków i infrastruktury testowej.

## Wynik

Sprawdzono pełne tokeny, rozróżnienie ery i routing alternatyw. 24 demonstracje są wykluczone z 2476 ocenianych pytań. Analizator działa; pojedyncza generacja zwróciła Warszawa.

Końcowy przegląd Standards i Spec: brak nowych ustaleń wymagających poprawy.
