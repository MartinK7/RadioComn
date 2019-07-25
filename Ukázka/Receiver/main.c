/*******************************************************************************
*	PROJEKT:	Bezdrátová radiokomunikace			                           *
********************************************************************************
*
*
*	AUTOR:		Krásl Martin
*
*	SOUBOR:		main.c
*   PROCESOR:	ATMEGA328P
*   OSCILÁTOR:	20MHz (Krystalový)
*	KOMPILÁTOR: AVR-GCC
*   KÓDOVÁNÍ:   ANSI
*   VERZE:		2.0
*	DATUM:		30.3.2017
*
*###############################################################################
*
*	POPIS PROGRAMU:
*  Jednoduchá ukázka, která pøijímá sekvenci znakù zakonèenou hodnotou èítaèe.
*
*#############################################################################*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "uart.h"
#include "RX_radiolink.h"


// Pøerušení
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
 * Pøerušení (Handler) - tato procedura se vykoná jakmile nastane pøerušení
 * na nìkterém ze vstupù PCI0...PCI7
 * Je zapotøebí pomocí výpoètu zjistit, na kterém konkrétním vstupu došlo k
 * pøerušení.
 *******************************************************************************/
ISR(PCINT0_vect)
{
	uint8_t changebits;
	
	changebits = PINB ^ portbHistory;
	portbHistory = PINB;
	
	/*
	if(changebits & (1 << VÝVOD))
	{
		// Jiný externí zdroj pøerušení
	}
	*/
	
	if(changebits & (1 << PINB0))
	{
		//pøerušení rádio
		RADIO_ISR();
	}
}

/*******************************************************************************
 * void ISR_RADIO_dataReceived(uint8_t *arrayData, uint8_t lenght)
 * -----------------------------------------------------------------------------
 * *arrayData - Ukazatel na místo v pamìti, kde se nachází pole pøijatých dat.
 * lenght - Délka pøijatých dat (pole).
 * -----------------------------------------------------------------------------
 * Pøerušení (Handler) - tato procedura se vykoná jakmile softwarový dekodér
 * radiopøijímaèe dokonèní pøíjem dat se správným automatickým kontrolním souètem.
  *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * JELIKOŽ JE TATO PROCEDURA SOUÈÁSTÍ PØERUŠENÍ, JE NEZBYTNÉ ÚLOHY ZDE KONANÉ
 * PROVÉST CO NEJRYCHLEJI!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *******************************************************************************/
void ISR_RADIO_dataReceived(uint8_t *arrayData, uint8_t lenght)
{
	// Došlo v hlavní programové symèce k dokonèení pøíjmu dat?
	if(dataInBuffer == 0)
	{
		// Ano
		my_lenght = lenght;
		
		// Vyèti všechna pøijatá data
		for(int i=0; i<lenght; ++i)
		{
			my_data[i] = arrayData[i];
		}
		
		// Nastav pøíznak pøijetí dat
		dataInBuffer = 1;
	}
}

/*******************************************************************************
 * void MCU_init(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Provede poèáteèní nastavení mikropoèítaèe
 *******************************************************************************/
void MCU_init(void)
{
	//pøerušení rádio
	DDRB &= ~((1<<PB0)); //nastavit jako vstupy (pøijímaè)
	PORTB |= ((1<<PB2) | (1<<PB3)); //pøipojit pullup rezistory (tlaèítka)
	PCICR |= (1<<PCIE0); //povolit možnost pøerušení na vstupech PCI0...PCI7
	PCMSK0 |= (1<<PCINT0); //povol pøerušení konkrétnì na vstupu PB0/PCI0
	
	RADIO_InitAndStart();
	sei(); //povolit pøerušení
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
	MCU_init();
	initUART();
	COMM_START();
	TX_START();
	writeString("Jsem pripraven na prijem dat.\r\r");
	
	while(1)
	{
		// Pøišli nová data? tj. Je pøíznak pøijetí nastaven?
		if(dataInBuffer != 0)
		{
			// Ano - vypiš informující text
			sprintf(str, "Byla prijata nova data o delce %i :\r", my_lenght);
			writeString(str);
			
			// Vypiš prvních 5 pøijatých znakù
			writeString("DATA: ");
			for(int i=0; i<5; ++i)
			{
				putByte(my_data[i]);
			}
			
			// Vypiš èítaè
			putByte('\r');
			sprintf(str, "CITAC: %i\r\r", my_data[5]);
			writeString(str);
			
			// Vymaž pøíznak pøijetí dat. Data jsem už zpracoval.
			dataInBuffer = 0;		
		}
		
		/*
		
		//.. <Nìjaký kód>
		
		*/
		_delay_ms(500);
	}
	
	return 0;
}