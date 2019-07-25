/*******************************************************************************
*	PROJEKT:	Bezdr�tov� radiokomunikace			                           *
********************************************************************************
*
*
*	AUTOR:		Kr�sl Martin
*
*	SOUBOR:		radiolink.h
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

// Nastaven� p�ipojen�
#define RADIO_PORT PORTB
#define RADIO_DDR  DDRB
#define RADIO_PIN  PINB
#define RADIO_DAT  PB2

// Nastaven� �asov� prodlevy 'T'
// Zde je vyu�it �asova� TIMER0
// �asov� prodleva je zde nastavena na 425 us.
// V�po�et:
// T[us] = 1000000/(Fosc/(255-RADIO_TCNT0)/P�edd�li�ka) = 
// = 1000000/(4800000/(255-0)/8) = 425us

#define RADIO_TCNT0 0x00  //TCNT=0
#define RADIO_TCCR0B 0x02 //P�edd�li�ka clkIO/8

// Glob�ln�:
/*******************************************************************************
 * void RADIO_init(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Uvede r�diovou komunikaci do v�choz�ho stavu.
 *******************************************************************************/
void RADIO_init(void);

/*******************************************************************************
 * void RADIO_sendData(uint8_t *arrayData, uint8_t lenght)
 * -----------------------------------------------------------------------------
 * *arrayData - Ukazatel na m�sto v pam�ti, kde je ulo�ena s�re bajt�
 * lenght - D�lka s�rie bajt�.
 * -----------------------------------------------------------------------------
 * Za�le s�rii bajt�, ulo�enou v pam�ti.
 *******************************************************************************/
void RADIO_sendData(uint8_t *arrayData, uint8_t lenght);
