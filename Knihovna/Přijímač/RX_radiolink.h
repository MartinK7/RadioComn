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


// Nastavení připojení
#define RADIO_PORT PORTB
#define RADIO_DDR  DDRB
#define RADIO_PIN  PINB
#define RADIO_DAT  PB0 //PCI0

// Nastavení časové prodlevy 'T'
// Zde je využit časovač TIMER0
// Časová prodleva je zde nastavena na 425 us.
// Výpočet:
// T[us] = 1000000/Fosc*RADIO_TIME = 
// = 1000000/20000000*8500 = 425us
//
// Todchychlka[us] = 1000000/Fosc*RADIO_TIME = 
// = 1000000/20000000*2000 = 100us
#define	RADIO_TIME		8500 //T = 425us
#define	RADIO_TIMEERR	2000 //Povolená odchylka měření(vysílání) = 100us

// Maximální možná délka přijaté zprávy v bajtech
#define RADIO_MSGMAXLEN	10

// Handlery:

/*******************************************************************************
 * void ISR_RADIO_dataReceived(uint8_t *arrayData, uint8_t lenght)
 * -----------------------------------------------------------------------------
 * *arrayData - Ukazatel na místo v paměti, kde se nachází pole přijatých dat.
 * lenght - Délka přijatých dat (pole).
 * -----------------------------------------------------------------------------
 * Přerušení (Handler) - tato procedura se vykoná jakmile softwarový dekodér
 * radiopřijímače dokonční příjem dat se správným automatickým kontrolním součtem.
  *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * JELIKOŽ JE TATO PROCEDURA SOUČÁSTÍ PŘERUŠENÍ, JE NEZBYTNÉ ÚLOHY ZDE KONANÉ
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
 * Uvede dekódování radiopřijímače do výchozího stavu.
 *******************************************************************************/
void RADIO_InitAndStart(void);

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
void RADIO_ISR(void);