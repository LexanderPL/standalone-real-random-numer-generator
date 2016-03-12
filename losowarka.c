/****************
 *
 *  LexanderPL
 *  Losowarka v 1.04
 *  maj 2015
 *
 ****************/

// F_CPU 1000000UL - Czestotliwość działania procesora - 1 MHz
// PORTB - wyświetlacz cyfry lewej
// PORTD - wyświetlacz cyfry prawej
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <stdlib.h>

volatile uint8_t liczba_RND; //liczba losowana

#define iloscUczniow 35

/*
Funkcja realizuje rzeczywiste losowanie polegające na czytaniu wartości z niepodłączonego pinu mikrokontrolera.
Mikrokontroler zbiera szum z otoczenia (stan 0 lub 1) i przesuwa liczbę w lewo w celu sczytania kolejnej wartości.
Ośmiokrotne wykonanie tej operacji daje nam rzeczywiście losową, ośmiobitową liczbę.
*/
void losuj()
{
	ADMUX = (1 << MUX2) | (0 << MUX1) | (1 << MUX0); //ADC: wybor kanalu 5
	ADCSRA = (1 << ADEN) | (0 << ADPS0) | (0 << ADPS1) | (0 << ADPS2); // ustawienie preskalera na 2

	for (int i = 0; i < 8; i++) //losowanie 8 bitow liczba_RND
	{
		liczba_RND = (liczba_RND << 1); //przesunięcie bitu w lewo

		ADCSRA |= (1 << ADSC); //ADSC: uruchomienie pojedynczej konwersji
		while (ADCSRA & (1 << ADSC)) {} //czekamy na jej zakonczenie
		_delay_us(10); //delay dla stabilizacji odczytu
		liczba_RND |= ADC & 0x01; //zapisanie wyniku odczytu do ostatniego bitu liczby losowej
	}
}


int8_t czyWylosowana(int8_t liczba) //funkcja do sprawdzania czy dana liczba została już wylosowana
{
	uint8_t odp = eeprom_read_byte((uint8_t *) liczba);
	char wylosowana;
	
	if (odp !=0) //jezeli liczba zostala wylosowana
	{
		wylosowana = 1;
	} 
	else
	wylosowana = 0; //jesli nie zostala wylosowana

	return wylosowana;
}


/*
Sprawdzanie, czy wylosowano wszystkie liczby.
Jeśli tak, czyszczenie pamięci EEPROM
*/
void zeruj() 
{
	uint8_t wylosowana = 1; //flaga
	int i;
	
	for (i = 0; i < iloscUczniow; i++) //sprawdzanie, czy istnieje chociaż jedna niewylosowana wartość
	{
		uint8_t odp = eeprom_read_byte((uint8_t *) i);
		if (odp == 0)
		{
			wylosowana = 0;
			break;
		}
	}
	
	if (wylosowana == 1)
	{
		for (i = 0; i < iloscUczniow; i++) //trzeba je oznaczyc jako niewylosowane
		{
			eeprom_update_byte((uint8_t *) i, 0);
			eeprom_busy_wait();
		}

	}
}


int pomiar() //odczyt wartosci potencjometru
{ 
	ADCSRA = (1 << ADEN) | (1 << ADPS0) // ustawienie preskalera na 8
			| (1 << ADPS1) | (0 << ADPS2);
	ADMUX = (0 << MUX1) | (0 << MUX0); //wybór kanału 0
	ADCSRA |= (1 << ADSC); //ADSC: uruchomienie pojedynczej konwersji
	while (ADCSRA & (1 << ADSC)) {} //czekanie na zakończenie konwersji

	return ADC; //zwracamy wynik
}


int8_t cyfra[11] = 
{
    	0b00000010, //zero
		0b00011111, //jeden
		0b00100100, //dwa
		0b00001100, //trzy
		0b00011001, //cztery
		0b01001000, //piec
		0b01000000, //szesc
		0b00011010, //siedem
		0b00000000, //osiem
		0b00001000, //dziewiec
		0b11111111, //pusto [10]
};


void pusto() //zgas ekran
{
	PORTB = cyfra[10]; //ustawianie obu cyfr wyświetlacza na cyfrę "pusto"
	PORTD = cyfra[10];
}


void show(uint8_t a) //wyswietlanie liczby
{
	uint8_t lewa = a / 10;
	uint8_t prawa = a % 10;
	if (lewa == 0) lewa = 10;

	PORTB = cyfra[lewa];
	PORTD = cyfra[prawa];
}

//===========================================================================================
int main()
{
	DDRB = 0b11111111;
	DDRD = 0b01111111; //PD7 jako przycisk
	PORTD = (1 << PD7);
	DDRC = (0 << PD7);
	pusto(); //wygaszenie ekranu

	ADMUX = (0 << REFS1) | (1 << REFS0); //wybor napiecia referencyjnego do ADC (Vcc)
	MCUCR = (1 << SE) | (1 << SM1); //konfiguracja sleep mode

	int czas=pomiar();

	losuj(); //liczba prawdziwie losowa
	srand(liczba_RND); //bedzie naszym seedem do randomizera

	zeruj();

	while (1)
	{
		do
		{
			liczba_RND = rand() % (iloscUczniow); 
		} while (czyWylosowana(liczba_RND) == 1); //losowanie bez powtórzeń

		eeprom_update_byte((uint8_t *) liczba_RND, 1); //zapisywanie wylosowanej liczby do pamięci EEPROM
		eeprom_busy_wait(); //oczekiwanie na zakończenie procesu zapisywania

		show(liczba_RND + 1); //wyświetlanie wylosowanej liczby na ekran.
		
		for (int i = 0; i <czas  + 100; i++) //wyświetlanie liczby przez określony czas
			_delay_ms(10);
		
		pusto(); //wygaszenie ekranu
		sleep_cpu(); //przejście w tryb uśpienia
	}
}
