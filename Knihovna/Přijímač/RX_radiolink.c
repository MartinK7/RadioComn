/*******************************************************************************
*	PROJEKT:	Bezdrátová radiokomunikace			                           *
********************************************************************************
*
*
*	AUTOR:		Krásl Martin
*
*	SOUBOR:		radiolink.h
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
*  Knihovna přijímače zajištující zprostředkování radiokomunikace.
*
*	PRINCIP:
*  Komunikace probíhá za pomocí série logických úrovní, které nabývají hodnot
*  LOG.1 nebo LOG.0 a trvají různých násobků časové prodlevy 'T'. Nejprve je
*  vygenerován krátký obdélníkový signál zahrnující nejméně 50 period. Následuje
*  STARTPULZ (LOG.0 o délce 3T následována LOG. 1 o délce 3T). Následují samotná 
*  uživatelská data, všechny bajty jsou zasílány v sérii a to vždy od LSB do MSB.
*  Uživatelská data JSOU AUTOMATICKY ZAKONČENA 16 BITOVÝM KONTROLNÍM SOUČTEM!
*  + NAVÍC JAKO KONTROLA JE JEŠTĚ ZASÍLÁN NEGOVANÝ 16 BITOVÝ KONTROLNÍ SOUČET.
*  Po dokončení zaslání všech dat, je paket zakončen STOPPULZEM (LOG.0 o délce 1T
*  následována LOG. 1 o délce 4T).
*
*   PŘIJÍMAČ:
*  Přijímač nejprve v šumu hledá synchronizační rámec. Jednoduše měří délky
*  pulzů a pokud příjde alespoň 20 pulzů hledá poté už jenom STARTPULZ.
*  Následovaná data jsou průběžně dekódována a ukládána. Po přijetí STOPPULZu
*  automaticky zkontroluje kontrolní součet, pokud souhlasí vyvolá externí
*  funkci 'ISR_RADIO_dataReceived', kde do pole předem uloží přijatá data bez
*  automatického kontrolního součtu.
*#############################################################################*/
/*
                  SARTPULZ     log.0   log.1       log.0
šum --- ---     -- ------- --- --- --- ------ ------ --- ---     - --------- šum
šum |1|1|1|      |1| 3T  |1|1|1|1|1|1|1| 2T |1| 2T |1|1|1|1|     |1|  4T   | šum
šum |T|T|T|      |T|     |T|T|T|T|T|T|T|    |T|    |T|T|T|T|     |T|       | šum
šum | | | |      | |     | | | | | | | |    | |    | | | | |     | |       | šum
šum-- --- -- ... ---     --- --- --- ---    ---    --- --- - ... ---       --šum

šum SYNCHRONIZACE        log.0   log.0      log.1      log.0     STOPPULZ

*/


#include <avr/io.h>
#include <util/delay.h>
#include "RX_radiolink.h"

#define bitRead(_register_, _bit_)				(_register_ & (1<<_bit_))
#define bitSet(_register_, _bit_)				(_register_ |= (1<<_bit_))
#define bitClear(_register_, _bit_)				(_register_ &= ~(1<<_bit_))
#define bitToggle(_register_, _bit_)			(_register_ ^= (1<<_bit_))
#define bitWrite(_register_, _bit_, _state_)	if(_state_){_register_|=(1<<_bit_);}else{_register_&=~(1<<_bit_);}

// Postup v dekódování
uint8_t progress = 0x00;
// Čítač přijatých impulzů
uint8_t counter = 0;
// Proměnná pro dočasné uložení mezivýsledku
uint16_t time = 0;

// Vyhrazené místo v paměti pro přijatá data
uint8_t data[RADIO_MSGMAXLEN];
// Proměnná pro práci s polem
uint8_t data_index = 0;
// Proměnná pro práci s polem bitů
uint8_t bitIndex = 0;
// Čítač přijatých bitů
uint8_t bitCounter = 0;

/*******************************************************************************
 * void RADIO_TimerInit(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Uvede časovač do výchozího stavu.
 *******************************************************************************/
void RADIO_TimerInit(void)
{
	TCCR1B = 0b00000001; //1:1
	TCNT1 = 0x0000;
}

/*******************************************************************************
 * void RADIO_resetAndStartCount(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Resetuje časovač a pokračuje v čítání.
 *******************************************************************************/
void RADIO_resetAndStartCount(void)
{
	TCNT1 = 0x0000;
}

/*******************************************************************************
 * uint16_t RADIO_timerRead(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Vyčte data z časovače.
 *******************************************************************************/
uint16_t RADIO_timerRead(void)
{
	return TCNT1;
}

/*******************************************************************************
 * void RADIO_InitAndStart(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Uvede dekódování radiopřijímače do výchozího stavu.
 *******************************************************************************/
void RADIO_InitAndStart(void)
{
	progress = 0x00;
	counter= 0;
	data_index = 0;
	bitIndex = 0;
	bitCounter = 0;
	RADIO_TimerInit();
}

/*******************************************************************************
 * void RADIO_sequenceEnd(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Procedura zajišťující úkony potřebné pro dokončení příjmu dat.
 *******************************************************************************/
void RADIO_sequenceEnd(void)
{
	uint8_t lenght = bitCounter/8;
	
	if(lenght>4)
	{
		uint16_t KS = 0;
		for(uint8_t i=0; i<lenght-4; ++i)
		{
			KS += data[i];
		}
		
		uint16_t KS_0 = (data[lenght-4]<<8)+data[lenght-3];
		uint16_t KS_1 = (data[lenght-2]<<8)+data[lenght-1];
		
		if((KS == KS_0) && (KS_0 == (~KS_1)))
		{
			ISR_RADIO_dataReceived(data, lenght-4);
		}
	}
	
	RADIO_InitAndStart();
}

/*******************************************************************************
 * void RADIO_bitReceived(uint8_t state)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Procedura zajišťující uložení bitu do pole přijatých hodnot.
 *******************************************************************************/
void RADIO_bitReceived(uint8_t state)
{
	++bitCounter;
	if(bitIndex >= 8)
	{
		bitIndex = 0;
		++data_index;
		if(data_index>RADIO_MSGMAXLEN-1)
		{
			RADIO_sequenceEnd();
			return;
		}
	}
	if((data_index>=0) && (data_index<RADIO_MSGMAXLEN)){
	bitWrite(data[data_index], bitIndex, state);}
	++bitIndex;
}

/*******************************************************************************
 * void RADIO_ISR(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Procedura zajišťující uložení bitu do pole přijatých hodnot.
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * TUTO PROCEDURU VOLEJTE POKAŽDÉ DOJDE-LI KE ZMĚNĚ NA VSTUPU OD RADIOPŘIJÍMAČE!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *******************************************************************************/
void RADIO_ISR(void)
{
	switch(progress)
	{
		case 0x00:
			RADIO_resetAndStartCount();
			progress = 0x01;
			return;
		case 0x01:
			time = RADIO_timerRead();
			if((time > (RADIO_TIME-RADIO_TIMEERR) )&&(time < (RADIO_TIME+RADIO_TIMEERR)))
			{
				++counter;
				RADIO_resetAndStartCount();
			}else{
				if((counter > 20)&&(bitRead(RADIO_PIN, RADIO_DAT) == 0)&&(time>(2*RADIO_TIME)))
				{
					progress = 0x02;
					RADIO_resetAndStartCount();
				}else{
					RADIO_InitAndStart();
				}
			}
			return;
		case 0x02:
			if(bitRead(RADIO_PIN, RADIO_DAT) == 0)//byla tam jednička?
			{
				time = RADIO_timerRead();
				if(time > (3*RADIO_TIME))
				{
					RADIO_sequenceEnd();
					return;
				}else{
					if(time < (RADIO_TIME+RADIO_TIMEERR))
					{
						RADIO_bitReceived(0);
					}else{
						RADIO_bitReceived(1);
					}
				}
			}
			RADIO_resetAndStartCount();
			return;
	}
}