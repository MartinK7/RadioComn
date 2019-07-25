/*******************************************************************************
*	PROJEKT:	Bezdrátová radiokomunikace			                           *
********************************************************************************
*
*
*	AUTOR:		Krásl Martin
*
*	SOUBOR:		radiolink.c
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
*  Knihovna pøijímaèe zajištující zprostøedkování radiokomunikace.
*
*	PRINCIP:
*  Komunikace probíhá za pomocí série logických úrovní, které nabývají hodnot
*  LOG.1 nebo LOG.0 a trvají rùzných násobkù èasové prodlevy 'T'. Nejprve je
*  vygenerován krátký obdélníkový signál zahrnující nejménì 50 period. Následuje
*  STARTPULZ (LOG.0 o délce 3T následována LOG. 1 o délce 3T). Následují samotná 
*  uživatelská data, všechny bajty jsou zasílány v sérii a to vždy od LSB do MSB.
*  Uživatelská data JSOU AUTOMATICKY ZAKONÈENA 16 BITOVÝM KONTROLNÍM SOUÈTEM!
*  + NAVÍC JAKO KONTROLA JE JEŠTÌ ZASÍLÁN NEGOVANÝ 16 BITOVÝ KONTROLNÍ SOUÈET.
*  Po dokonèení zaslání všech dat, je paket zakonèen STOPPULZEM (LOG.0 o délce 1T
*  následována LOG. 1 o délce 4T).
*
*   PØIJÍMAÈ:
*  Pøijímaè nejprve v šumu hledá synchronizaèní rámec. Jednoduše mìøí délky
*  pulzù a pokud pøíjde alespoò 20 pulzù hledá poté už jenom STARTPULZ.
*  Následovaná data jsou prùbìžnì dekódována a ukládána. Po pøijetí STOPPULZu
*  automaticky zkontroluje kontrolní souèet, pokud souhlasí vyvolá externí
*  funkci 'ISR_RADIO_dataReceived', kde do pole pøedem uloží pøijatá data bez
*  automatického kontrolního souètu.
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
#include "TX_radiolink.h"

#define bitRead(_register_, _bit_)				(_register_ & (1<<_bit_))
#define bitSet(_register_, _bit_)				(_register_ |= (1<<_bit_))
#define bitClear(_register_, _bit_)				(_register_ &= ~(1<<_bit_))
#define bitToggle(_register_, _bit_)			(_register_ ^= (1<<_bit_))
#define bitWrite(_register_, _bit_, _state_)	if(_state_){_register_|=(1<<_bit_);}else{_register_&=~(1<<_bit_);}

// Kontrolní souèet
uint16_t checkSum;

/*******************************************************************************
 * void RADIO_timerInit(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Uvede èasovaè do výchozího stavu.
 *******************************************************************************/
void RADIO_timerInit(void){
	TIMSK0 = 0;
	TCCR0B = RADIO_TCCR0B;
	TCNT0 = RADIO_TCNT0;
	bitSet(TIFR0, TOV0);
}

/*******************************************************************************
 * void RADIO_timerDelay(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Èeká o délce 'T', za pomocí èasovaèe TIMER0.
 *******************************************************************************/
void RADIO_timerDelay(void){
	while(!bitRead(TIFR0, TOV0));
	TCNT0 = RADIO_TCNT0;
	bitSet(TIFR0, TOV0);
}

/*******************************************************************************
 * void RADIO_init(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Uvede rádiovou komunikaci do výchozího stavu.
 *******************************************************************************/
void RADIO_init(void){
	bitSet(RADIO_DDR, RADIO_DAT);
}

/*******************************************************************************
 * void RADIO_synchronize(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Vygeneruje sérii impulzù. (Obdélníkový signál)
 *******************************************************************************/
void RADIO_synchronize(void){	
	for(uint8_t i=0; i<50; ++i){ //50x beep
		bitSet(RADIO_PORT, RADIO_DAT);
		RADIO_timerDelay();
		bitClear(RADIO_PORT, RADIO_DAT);
		RADIO_timerDelay();
	}
}

/*******************************************************************************
 * void RADIO_sendByte(uint8_t data)
 * -----------------------------------------------------------------------------
 * data - Bajt který má být zaslán.
 * -----------------------------------------------------------------------------
 * Zašle bajt.
 *******************************************************************************/
void RADIO_sendByte(uint8_t data){
	//data
	for(uint8_t i=0; i<8; ++i){
		//synchronizaèní pulz
		bitClear(RADIO_PORT, RADIO_DAT);
		RADIO_timerDelay();
		bitSet(RADIO_PORT, RADIO_DAT);
		RADIO_timerDelay();
		//extra delay - urèuje log.1
		if(bitRead(data, i) != 0)RADIO_timerDelay();
	}
}

/*******************************************************************************
 * void RADIO_sendData(uint8_t *arrayData, uint8_t lenght)
 * -----------------------------------------------------------------------------
 * *arrayData - Ukazatel na místo v pamìti, kde je uložena sére bajtù
 * lenght - Délka série bajtù.
 * -----------------------------------------------------------------------------
 * Zašle sérii bajtù, uloženou v pamìti.
 *******************************************************************************/
void RADIO_sendData(uint8_t *arrayData, uint8_t lenght) {
	RADIO_timerInit();
	bitSet(RADIO_DDR, RADIO_DAT); //output

	// Synchronizuj
	RADIO_synchronize();
	
	// STARTPULZ
	bitSet(RADIO_PORT, RADIO_DAT);
	RADIO_timerDelay();
	RADIO_timerDelay();
	RADIO_timerDelay();
	
	// Pošli všchny bajty
	checkSum = 0x0000;
	for(uint8_t i=0; i<lenght; ++i){
		RADIO_sendByte(arrayData[i]);
		checkSum += arrayData[i];
	}
	
	// Pošli kontrolní souèet
	RADIO_sendByte((checkSum&0xFF00)>>8);	
	RADIO_sendByte(checkSum&0x00FF);
	
	// Pošli negovaný kontrolní souèet
	checkSum = ~(checkSum);
	RADIO_sendByte((checkSum&0xFF00)>>8);	
	RADIO_sendByte(checkSum&0x00FF);
	
	// STOPPULZ
	bitClear(RADIO_PORT, RADIO_DAT);
	RADIO_timerDelay();
	bitSet(RADIO_PORT, RADIO_DAT);
	RADIO_timerDelay();
	RADIO_timerDelay();
	RADIO_timerDelay();
	RADIO_timerDelay();
	bitClear(RADIO_PORT, RADIO_DAT);
}