/*******************************************************************************
*	PROJEKT:	Bezdr�tov� radiokomunikace			                           *
********************************************************************************
*
*
*	AUTOR:		Kr�sl Martin
*
*	SOUBOR:		main.c
*   PROCESOR:	ATTINY13
*   OSCIL�TOR:	4.8MHz (Intern� RC)
*	KOMPIL�TOR: AVR-GCC
*   K�DOV�N�:   ANSI
*   VERZE:		2.0
*	DATUM:		30.3.2017
*
*###############################################################################
*
*	POPIS PROGRAMU:
*  Jednoduch� uk�zka, kter� pos�l� sekvenci znak� zakon�enou hodnotou ��ta�e.
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
 * Provede po��te�n� nastaven� mikropo��ta�e
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
 * Hlavn� funkce.
 *******************************************************************************/
int main(void)
{	
	// Prove� init mikropo��ta�e
	init();
	
	// Vynuluj ��ta�
	uint8_t counter = 0;
	// P�iprav data, kter� budou zasl�na
	uint8_t data[6] = {'A','h','o','j','!',0x00};
	
	// Hlavn� programov� smy�ka
	while(1)
	{		
		// Spolu s uk�zkou pos�lej i hodnotu ��ta�e
		data[5] = counter;
		
		// Po�li v�echny p�ipraven� data
		RADIO_sendData(data, 6);
		
		// Inkreentuj ��ta�
		++counter;
		
		// �ekej
		_delay_ms(5000);
	}
}