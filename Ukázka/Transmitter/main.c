/*******************************************************************************
*	PROJEKT:	Bezdrátová radiokomunikace			                           *
********************************************************************************
*
*
*	AUTOR:		Krásl Martin
*
*	SOUBOR:		main.c
*   PROCESOR:	ATTINY13
*   OSCILÁTOR:	4.8MHz (Interní RC)
*	KOMPILÁTOR: AVR-GCC
*   KÓDOVÁNÍ:   ANSI
*   VERZE:		2.0
*	DATUM:		30.3.2017
*
*###############################################################################
*
*	POPIS PROGRAMU:
*  Jednoduchá ukázka, která posílá sekvenci znakù zakonèenou hodnotou èítaèe.
*
*#############################################################################*/

#include <avr/io.h>
#include <util/delay.h>

#include "TX_radiolink.h"
#include "utils.h"


/*******************************************************************************
 * void init(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Provede poèáteèní nastavení mikropoèítaèe
 *******************************************************************************/
void init(void)
{
	RADIO_init();
}


/*******************************************************************************
 * int main(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Hlavní funkce.
 *******************************************************************************/
int main(void)
{	
	// Proveï init mikropoèítaèe
	init();
	
	// Vynuluj èítaè
	uint8_t counter = 0;
	// Pøiprav data, která budou zaslána
	uint8_t data[6] = {'A','h','o','j','!',0x00};
	
	// Hlavní programová smyèka
	while(1)
	{		
		// Spolu s ukázkou posílej i hodnotu èítaèe
		data[5] = counter;
		
		// Pošli všechny pøipravená data
		RADIO_sendData(data, 6);
		
		// Inkreentuj èítaè
		++counter;
		
		// Èekej
		_delay_ms(5000);
	}
}