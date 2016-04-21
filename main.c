/*****************
Lexander
Kwiecieñ 2016
*****************/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <stdlib.h>

uint8_t liczba;
#define ilosc_uczniow 35
uint8_t czy_spac=0;


char cyfra[11] =
{
	0b01000000, //zero
	0b01111001, //jeden
	0b00100100, //dwa
	0b00110000, //trzy
	0b00011001, //cztery
	0b00010010, //piec
	0b00000010, //szesc
	0b01011000, //siedem
	0b00000000, //osiem
	0b00010000, //dziewiec
	0b01111111, //pusto
};


uint8_t losuj() //rzeczywiste losowanie liczby jako seed do generatora
{
	volatile uint8_t liczba_RND;
	ADMUX = (1 << MUX2) | (0 << MUX1) | (1 << MUX0); //ADC: wybor kanalu 5
	ADCSRA = (1 << ADEN) | (0 << ADPS0) | (0 << ADPS1) | (0 << ADPS2); // ustawienie preskalera na 2

	for (int i = 0; i < 8; i++) //losowanie 8 bitow liczba_RND
	{
		liczba_RND = (liczba_RND << 1); //przesuniêcie bitu w lewo

		ADCSRA |= (1 << ADSC); //ADSC: uruchomienie pojedynczej konwersji
		while (ADCSRA & (1 << ADSC)) {} //czekamy na jej zakonczenie
		_delay_us(10); //delay dla stabilizacji odczytu
		liczba_RND |= ADC & 0x01; //zapisanie wyniku odczytu do ostatniego bitu liczby losowej
	}
	return liczba_RND;
}


void zeruj()
{
	int i;

	for (i = 0; i < ilosc_uczniow; i++) //sprawdzanie, czy istnieje chocia¿ jedna niewylosowana wartoœc
	{
		uint8_t odp = eeprom_read_byte((uint8_t *) i);
		if (odp == 0) return;
	}

	for (i = 0; i < ilosc_uczniow; i++) //jeœli taka nie istnieje, trzeba wszystkie liczby  oznaczyc jako niewylosowane
	{
		eeprom_update_byte((uint8_t *) i, 0);
		eeprom_busy_wait();
	}
}


int8_t czyWylosowana(int8_t liczba) //funkcja do sprawdzania czy dana liczba zosta³a ju¿ wylosowana
{
	uint8_t odp = eeprom_read_byte((uint8_t *) liczba);

	if (odp != 0) return 1;
	else return 0;
}


void pokaz (uint8_t a)
{
	liczba = a;
	TCCR0 |= (1<<CS01); // w³¹czenie timera0
}


void zgas()
{
	PORTD &= ~(1<<PD1 | 1<<PD0); //gaszenie ekranu
	TCCR0 &= (0<<CS01); //wy³¹czenie timera0
}


ISR(TIMER0_OVF_vect) //zmiana wyœwietlania cyfry na tê drug¹
{
	if (liczba<10) //gdy liczba jest jednocyfrowa
	{
		PORTB = cyfra[liczba%10];
		PORTD |= (1<<PD1);
		PORTD &= ~(1<<PD0);
	}
	else if (liczba/10 == liczba%10) //gdy obie cyfry s¹ identyczne
	{
		PORTB = cyfra[liczba%10];
		PORTD |= (1<<PD1 | 1<<PD0);
	}
	else if (PORTB == cyfra[liczba/10])
	{
		PORTB = cyfra[liczba%10]; //wyswietlanie prawej strony liczby
		PORTD |= (1<<PD1);
		PORTD &= ~(1<<PD0);
	}
	else
	{
		PORTB = cyfra[liczba/10]; //wyswietlanie lewej strony liczby
		PORTD |= (1<<PD0);
		PORTD &= ~(1<<PD1);
	}
}


void generuj_i_wyswietl()
{
	zeruj(); //sprawdza czy nale¿y wyzerowac eeprom. Jeœli tak, zeruje eeprom

	do
	{
		liczba = rand() % (ilosc_uczniow);
	} while (czyWylosowana(liczba) == 1); //losowanie bez powtórzeñ

	eeprom_update_byte((uint8_t *) liczba, 1); //zapisywanie wylosowanej liczby do pamiêci EEPROM
	eeprom_busy_wait(); //oczekiwanie na zakoñczenie procesu zapisywania
	pokaz(liczba + 1); //wyœwietlanie wylosowanej liczby na ekran.
	TCNT1=0;
}



ISR(TIMER1_COMPA_vect) //gdy minie okreœlony czas wyœwietlania liczby na ekranie
{
	zgas();
	MCUCR &= ~((1 << ISC01) | (1 << ISC00)); //INT0 aktywowane sygna³em 0
    sleep_enable();
	czy_spac = 1;
}


ISR(INT0_vect) //przerwanie na INT0
{
    sleep_disable();
    zgas();
	MCUCR |= (1<<ISC01 | 1<<ISC00); //INT0 aktywowane zboczem rosn¹cym
	if (!czy_spac)generuj_i_wyswietl();
	czy_spac = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
    DDRB = 0b01111111; //PORTB w trybie wyjœcia
    DDRD = 0b011; //Pierwsze dwa bity PORTD w trybie wyjœcia
    PORTD |= (1<<PD2); //Pull-up resistor na INT0

    ADMUX = (0 << REFS1) | (1 << REFS0); //wybor napiecia referencyjnego do ADC (Vcc)

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    TIMSK |= (1<<TOIE0); //Przerwanie overflow (przepe³nienie timera0)

    OCR1A = 22000; // 3906,25 = 1s dla 1 MHz
    TCCR1B |= (1 << WGM12);
    TIMSK |= (1 << OCIE1A);

    MCUCR |= (1<<ISC01 | 1<<ISC00); //INT0 aktywowane zboczem rosn¹cym
    GICR |= (1<<INT0); //INT0 jako przerwanie

    sei();

    srand(losuj()); //bedzie naszym seedem do randomizera
    TCCR1B |= (1 << CS12); //odpalanie timera 1
    generuj_i_wyswietl();


    while(1)
    {
    	if (czy_spac) sleep_cpu();
    }
}
