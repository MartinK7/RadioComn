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


// Nastaven� p�ipojen�
#define RADIO_PORT PORTB
#define RADIO_DDR  DDRB
#define RADIO_PIN  PINB
#define RADIO_DAT  PB0 //PCI0

// Nastaven� �asov� prodlevy 'T'
// Zde je vyu�it �asova� TIMER0
// �asov� prodleva je zde nastavena na 425 us.
// V�po�et:
// T[us] = 1000000/Fosc*RADIO_TIME = 
// = 1000000/20000000*8500 = 425us
//
// Todchychlka[us] = 1000000/Fosc*RADIO_TIME = 
// = 1000000/20000000*2000 = 100us
#define	RADIO_TIME		8500 //T = 425us
#define	RADIO_TIMEERR	2000 //Povolen� odchylka m��en�(vys�l�n�) = 100us

// Maxim�ln� mo�n� d�lka p�ijat� zpr�vy v bajtech
#define RADIO_MSGMAXLEN	10

// Handlery:

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
extern void ISR_RADIO_dataReceived(uint8_t *arrayData, uint8_t lenght);

// Glob�ln�:
/*******************************************************************************
 * void RADIO_InitAndStart(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Uvede dek�dov�n� radiop�ij�ma�e do v�choz�ho stavu.
 *******************************************************************************/
void RADIO_InitAndStart(void);

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
void RADIO_ISR(void);