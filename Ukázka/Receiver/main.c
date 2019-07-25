/*******************************************************************************
*	PROJEKT:	Bezdr�tov� radiokomunikace			                           *
********************************************************************************
*
*
*	AUTOR:		Kr�sl Martin
*
*	SOUBOR:		main.c
*   PROCESOR:	ATMEGA328P
*   OSCIL�TOR:	20MHz (Krystalov�)
*	KOMPIL�TOR: AVR-GCC
*   K�DOV�N�:   ANSI
*   VERZE:		2.0
*	DATUM:		30.3.2017
*
*###############################################################################
*
*	POPIS PROGRAMU:
*  Jednoduch� uk�zka, kter� p�ij�m� sekvenci znak� zakon�enou hodnotou ��ta�e.
*
*#############################################################################*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "uart.h"
#include "RX_radiolink.h"


// P�eru�en�
uint8_t portbHistory = 0xFF;
uint8_t dataInBuffer = 0;

// Buffery
char my_data[10]; uint8_t my_lenght = 0;
char str[50];

/*******************************************************************************
 * ISR(PCINT0_vect)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * P�eru�en� (Handler) - tato procedura se vykon� jakmile nastane p�eru�en�
 * na n�kter�m ze vstup� PCI0...PCI7
 * Je zapot�eb� pomoc� v�po�tu zjistit, na kter�m konkr�tn�m vstupu do�lo k
 * p�eru�en�.
 *******************************************************************************/
ISR(PCINT0_vect)
{
	uint8_t changebits;
	
	changebits = PINB ^ portbHistory;
	portbHistory = PINB;
	
	/*
	if(changebits & (1 << V�VOD))
	{
		// Jin� extern� zdroj p�eru�en�
	}
	*/
	
	if(changebits & (1 << PINB0))
	{
		//p�eru�en� r�dio
		RADIO_ISR();
	}
}

/*******************************************************************************
 * void ISR_RADIO_dataReceived(uint8_t *arrayData, uint8_t lenght)
 * -----------------------------------------------------------------------------
 * *arrayData - Ukazatel na m�sto v pam�ti, kde se nach�z� pole p�ijat�ch dat.
 * lenght - D�lka p�ijat�ch dat (pole).
 * -----------------------------------------------------------------------------
 * P�eru�en� (Handler) - tato procedura se vykon� jakmile softwarov� dekod�r
 * radiop�ij�ma�e dokon�n� p��jem dat se spr�vn�m automatick�m kontroln�m sou�tem.
  *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * JELIKO� JE TATO PROCEDURA SOU��ST� P�ERU�EN�, JE NEZBYTN� �LOHY ZDE KONAN�
 * PROV�ST CO NEJRYCHLEJI!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *******************************************************************************/
void ISR_RADIO_dataReceived(uint8_t *arrayData, uint8_t lenght)
{
	// Do�lo v hlavn� programov� sym�ce k dokon�en� p��jmu dat?
	if(dataInBuffer == 0)
	{
		// Ano
		my_lenght = lenght;
		
		// Vy�ti v�echna p�ijat� data
		for(int i=0; i<lenght; ++i)
		{
			my_data[i] = arrayData[i];
		}
		
		// Nastav p��znak p�ijet� dat
		dataInBuffer = 1;
	}
}

/*******************************************************************************
 * void MCU_init(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Provede po��te�n� nastaven� mikropo��ta�e
 *******************************************************************************/
void MCU_init(void)
{
	//p�eru�en� r�dio
	DDRB &= ~((1<<PB0)); //nastavit jako vstupy (p�ij�ma�)
	PORTB |= ((1<<PB2) | (1<<PB3)); //p�ipojit pullup rezistory (tla��tka)
	PCICR |= (1<<PCIE0); //povolit mo�nost p�eru�en� na vstupech PCI0...PCI7
	PCMSK0 |= (1<<PCINT0); //povol p�eru�en� konkr�tn� na vstupu PB0/PCI0
	
	RADIO_InitAndStart();
	sei(); //povolit p�eru�en�
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
	MCU_init();
	initUART();
	COMM_START();
	TX_START();
	writeString("Jsem pripraven na prijem dat.\r\r");
	
	while(1)
	{
		// P�i�li nov� data? tj. Je p��znak p�ijet� nastaven?
		if(dataInBuffer != 0)
		{
			// Ano - vypi� informuj�c� text
			sprintf(str, "Byla prijata nova data o delce %i :\r", my_lenght);
			writeString(str);
			
			// Vypi� prvn�ch 5 p�ijat�ch znak�
			writeString("DATA: ");
			for(int i=0; i<5; ++i)
			{
				putByte(my_data[i]);
			}
			
			// Vypi� ��ta�
			putByte('\r');
			sprintf(str, "CITAC: %i\r\r", my_data[5]);
			writeString(str);
			
			// Vyma� p��znak p�ijet� dat. Data jsem u� zpracoval.
			dataInBuffer = 0;		
		}
		
		/*
		
		//.. <N�jak� k�d>
		
		*/
		_delay_ms(500);
	}
	
	return 0;
}