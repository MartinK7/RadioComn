/*******************************************************************************
*	PROJEKT:	Bezdr�tov� radiokomunikace			                           *
********************************************************************************
*
*
*	AUTOR:		Kr�sl Martin
*
*	SOUBOR:		radiolink.c
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
*  Knihovna p�ij�ma�e zaji�tuj�c� zprost�edkov�n� radiokomunikace.
*
*	PRINCIP:
*  Komunikace prob�h� za pomoc� s�rie logick�ch �rovn�, kter� nab�vaj� hodnot
*  LOG.1 nebo LOG.0 a trvaj� r�zn�ch n�sobk� �asov� prodlevy 'T'. Nejprve je
*  vygenerov�n kr�tk� obd�ln�kov� sign�l zahrnuj�c� nejm�n� 50 period. N�sleduje
*  STARTPULZ (LOG.0 o d�lce 3T n�sledov�na LOG. 1 o d�lce 3T). N�sleduj� samotn� 
*  u�ivatelsk� data, v�echny bajty jsou zas�l�ny v s�rii a to v�dy od LSB do MSB.
*  U�ivatelsk� data JSOU AUTOMATICKY ZAKON�ENA 16 BITOV�M KONTROLN�M SOU�TEM!
*  + NAV�C JAKO KONTROLA JE JE�T� ZAS�L�N NEGOVAN� 16 BITOV� KONTROLN� SOU�ET.
*  Po dokon�en� zasl�n� v�ech dat, je paket zakon�en STOPPULZEM (LOG.0 o d�lce 1T
*  n�sledov�na LOG. 1 o d�lce 4T).
*
*   P�IJ�MA�:
*  P�ij�ma� nejprve v �umu hled� synchroniza�n� r�mec. Jednodu�e m��� d�lky
*  pulz� a pokud p��jde alespo� 20 pulz� hled� pot� u� jenom STARTPULZ.
*  N�sledovan� data jsou pr�b�n� dek�dov�na a ukl�d�na. Po p�ijet� STOPPULZu
*  automaticky zkontroluje kontroln� sou�et, pokud souhlas� vyvol� extern�
*  funkci 'ISR_RADIO_dataReceived', kde do pole p�edem ulo�� p�ijat� data bez
*  automatick�ho kontroln�ho sou�tu.
*#############################################################################*/
/*
                  SARTPULZ     log.0   log.1       log.0
�um --- ---     -- ------- --- --- --- ------ ------ --- ---     - --------- �um
�um |1|1|1|      |1| 3T  |1|1|1|1|1|1|1| 2T |1| 2T |1|1|1|1|     |1|  4T   | �um
�um |T|T|T|      |T|     |T|T|T|T|T|T|T|    |T|    |T|T|T|T|     |T|       | �um
�um | | | |      | |     | | | | | | | |    | |    | | | | |     | |       | �um
�um-- --- -- ... ---     --- --- --- ---    ---    --- --- - ... ---       --�um

�um SYNCHRONIZACE        log.0   log.0      log.1      log.0     STOPPULZ

*/

#include <avr/io.h>
#include <util/delay.h>
#include "TX_radiolink.h"

#define bitRead(_register_, _bit_)				(_register_ & (1<<_bit_))
#define bitSet(_register_, _bit_)				(_register_ |= (1<<_bit_))
#define bitClear(_register_, _bit_)				(_register_ &= ~(1<<_bit_))
#define bitToggle(_register_, _bit_)			(_register_ ^= (1<<_bit_))
#define bitWrite(_register_, _bit_, _state_)	if(_state_){_register_|=(1<<_bit_);}else{_register_&=~(1<<_bit_);}

// Kontroln� sou�et
uint16_t checkSum;

/*******************************************************************************
 * void RADIO_timerInit(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Uvede �asova� do v�choz�ho stavu.
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
 * �ek� o d�lce 'T', za pomoc� �asova�e TIMER0.
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
 * Uvede r�diovou komunikaci do v�choz�ho stavu.
 *******************************************************************************/
void RADIO_init(void){
	bitSet(RADIO_DDR, RADIO_DAT);
}

/*******************************************************************************
 * void RADIO_synchronize(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Vygeneruje s�rii impulz�. (Obd�ln�kov� sign�l)
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
 * data - Bajt kter� m� b�t zasl�n.
 * -----------------------------------------------------------------------------
 * Za�le bajt.
 *******************************************************************************/
void RADIO_sendByte(uint8_t data){
	//data
	for(uint8_t i=0; i<8; ++i){
		//synchroniza�n� pulz
		bitClear(RADIO_PORT, RADIO_DAT);
		RADIO_timerDelay();
		bitSet(RADIO_PORT, RADIO_DAT);
		RADIO_timerDelay();
		//extra delay - ur�uje log.1
		if(bitRead(data, i) != 0)RADIO_timerDelay();
	}
}

/*******************************************************************************
 * void RADIO_sendData(uint8_t *arrayData, uint8_t lenght)
 * -----------------------------------------------------------------------------
 * *arrayData - Ukazatel na m�sto v pam�ti, kde je ulo�ena s�re bajt�
 * lenght - D�lka s�rie bajt�.
 * -----------------------------------------------------------------------------
 * Za�le s�rii bajt�, ulo�enou v pam�ti.
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
	
	// Po�li v�chny bajty
	checkSum = 0x0000;
	for(uint8_t i=0; i<lenght; ++i){
		RADIO_sendByte(arrayData[i]);
		checkSum += arrayData[i];
	}
	
	// Po�li kontroln� sou�et
	RADIO_sendByte((checkSum&0xFF00)>>8);	
	RADIO_sendByte(checkSum&0x00FF);
	
	// Po�li negovan� kontroln� sou�et
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