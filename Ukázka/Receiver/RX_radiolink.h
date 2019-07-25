/*******************************************************************************
*	PROJEKT:	Bezdrátová radiokomunikace			                           *
********************************************************************************
*
*
*	AUTOR:		Krásl Martin
*
*	SOUBOR:		radiolink.h
*   PROCESOR:	ATMEGA328P
*   OSCILÁTOR:	20MHz (Krystalovı)
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
*  Komunikace probíhá za pomocí série logickıch úrovní, které nabıvají hodnot
*  LOG.1 nebo LOG.0 a trvají rùznıch násobkù èasové prodlevy 'T'. Nejprve je
*  vygenerován krátkı obdélníkovı signál zahrnující nejménì 50 period. Následuje
*  STARTPULZ (LOG.0 o délce 3T následována LOG. 1 o délce 3T). Následují samotná 
*  uivatelská data, všechny bajty jsou zasílány v sérii a to vdy od LSB do MSB.
*  Uivatelská data JSOU AUTOMATICKY ZAKONÈENA 16 BITOVİM KONTROLNÍM SOUÈTEM!
*  + NAVÍC JAKO KONTROLA JE JEŠTÌ ZASÍLÁN NEGOVANİ 16 BITOVİ KONTROLNÍ SOUÈET.
*  Po dokonèení zaslání všech dat, je paket zakonèen STOPPULZEM (LOG.0 o délce 1T
*  následována LOG. 1 o délce 4T).
*
*   PØIJÍMAÈ:
*  Pøijímaè nejprve v šumu hledá synchronizaèní rámec. Jednoduše mìøí délky
*  pulzù a pokud pøíjde alespoò 20 pulzù hledá poté u jenom STARTPULZ.
*  Následovaná data jsou prùbìnì dekódována a ukládána. Po pøijetí STOPPULZu
*  automaticky zkontroluje kontrolní souèet, pokud souhlasí vyvolá externí
*  funkci 'ISR_RADIO_dataReceived', kde do pole pøedem uloí pøijatá data bez
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


// Nastavení pøipojení
#define RADIO_PORT PORTB
#define RADIO_DDR  DDRB
#define RADIO_PIN  PINB
#define RADIO_DAT  PB0 //PCI0

// Nastavení èasové prodlevy 'T'
// Zde je vyuit èasovaè TIMER0
// Èasová prodleva je zde nastavena na 425 us.
// Vıpoèet:
// T[us] = 1000000/Fosc*RADIO_TIME = 
// = 1000000/20000000*8500 = 425us
//
// Todchychlka[us] = 1000000/Fosc*RADIO_TIME = 
// = 1000000/20000000*2000 = 100us
#define	RADIO_TIME		8500 //T = 425us
#define	RADIO_TIMEERR	2000 //Povolená odchylka mìøení(vysílání) = 100us

// Maximální moná délka pøijaté zprávy v bajtech
#define RADIO_MSGMAXLEN	10

// Handlery:

/*******************************************************************************
 * void ISR_RADIO_dataReceived(uint8_t *arrayData, uint8_t lenght)
 * -----------------------------------------------------------------------------
 * *arrayData - Ukazatel na místo v pamìti, kde se nachází pole pøijatıch dat.
 * lenght - Délka pøijatıch dat (pole).
 * -----------------------------------------------------------------------------
 * Pøerušení (Handler) - tato procedura se vykoná jakmile softwarovı dekodér
 * radiopøijímaèe dokonèní pøíjem dat se správnım automatickım kontrolním souètem.
  *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * JELIKO JE TATO PROCEDURA SOUÈÁSTÍ PØERUŠENÍ, JE NEZBYTNÉ ÚLOHY ZDE KONANÉ
 * PROVÉST CO NEJRYCHLEJI!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *******************************************************************************/
extern void ISR_RADIO_dataReceived(uint8_t *arrayData, uint8_t lenght);

// Globální:
/*******************************************************************************
 * void RADIO_InitAndStart(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Uvede dekódování radiopøijímaèe do vıchozího stavu.
 *******************************************************************************/
void RADIO_InitAndStart(void);

/*******************************************************************************
 * void RADIO_ISR(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Procedura zajišující uloení bitu do pole pøijatıch hodnot.
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * TUTO PROCEDURU VOLEJTE POKADÉ DOJDE-LI KE ZMÌNÌ NA VSTUPU OD RADIOPØIJÍMAÈE!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *******************************************************************************/
void RADIO_ISR(void);