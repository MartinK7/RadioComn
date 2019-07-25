/*******************************************************************************
*	PROJEKT:	Bezdr�tov� radiokomunikace			                           *
********************************************************************************
*
*
*	AUTOR:		Kr�sl Martin
*
*	SOUBOR:		radiolink.h
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
#include "RX_radiolink.h"

#define bitRead(_register_, _bit_)				(_register_ & (1<<_bit_))
#define bitSet(_register_, _bit_)				(_register_ |= (1<<_bit_))
#define bitClear(_register_, _bit_)				(_register_ &= ~(1<<_bit_))
#define bitToggle(_register_, _bit_)			(_register_ ^= (1<<_bit_))
#define bitWrite(_register_, _bit_, _state_)	if(_state_){_register_|=(1<<_bit_);}else{_register_&=~(1<<_bit_);}

// Postup v dek�dov�n�
uint8_t progress = 0x00;
// ��ta� p�ijat�ch impulz�
uint8_t counter = 0;
// Prom�nn� pro do�asn� ulo�en� meziv�sledku
uint16_t time = 0;

// Vyhrazen� m�sto v pam�ti pro p�ijat� data
uint8_t data[RADIO_MSGMAXLEN];
// Prom�nn� pro pr�ci s polem
uint8_t data_index = 0;
// Prom�nn� pro pr�ci s polem bit�
uint8_t bitIndex = 0;
// ��ta� p�ijat�ch bit�
uint8_t bitCounter = 0;

/*******************************************************************************
 * void RADIO_TimerInit(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Uvede �asova� do v�choz�ho stavu.
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
 * Resetuje �asova� a pokra�uje v ��t�n�.
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
 * Vy�te data z �asova�e.
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
 * Uvede dek�dov�n� radiop�ij�ma�e do v�choz�ho stavu.
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
 * Procedura zaji��uj�c� �kony pot�ebn� pro dokon�en� p��jmu dat.
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
 * Procedura zaji��uj�c� ulo�en� bitu do pole p�ijat�ch hodnot.
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
 * Procedura zaji��uj�c� ulo�en� bitu do pole p�ijat�ch hodnot.
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * TUTO PROCEDURU VOLEJTE POKA�D� DOJDE-LI KE ZM�N� NA VSTUPU OD RADIOP�IJ�MA�E!
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
			if(bitRead(RADIO_PIN, RADIO_DAT) == 0)//byla tam jedni�ka?
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