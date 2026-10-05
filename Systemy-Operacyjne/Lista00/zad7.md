`volatile` w C informuje kompilator, że wartość danego obiektu może zmieniać się niezależnie od widoczynych instrukcji programu.

Oznacza to, że kompilator nie może zakładać, że skoro wcześniej odczytał wartośc obiektu to przy koljenym użyciu może skorzystać z zapamiętanej wartości, musi ponowanie odczytać obiekt.

Przykłady:
    
1.  W sytuacji gdy wartość obiektu jest zmieniana przez `sprzęt`, a nie przez instrukcje programu np: obiekt opisuję status jakiegoś fizycznego miernika.

2. Inny program współdzieli z naszym jakiś asynchroniczny sygnał. Ustawia/zgasza flage, od której zależy wykonanie naszego programu. 



