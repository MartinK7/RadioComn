/*******************************************************************************
*	PROJEKT:	Bezdrátová radiokomunikace			                           *
********************************************************************************
*
*
*	AUTOR:		Krásl Martin
*
*	SOUBOR:		radiolink.h
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

// Nastavení pøipojení
#define RADIO_PORT PORTB
#define RADIO_DDR  DDRB
#define RADIO_PIN  PINB
#define RADIO_DAT  PB2

// Nastavení èasové prodlevy 'T'
// Zde je využit èasovaè TIMER0
// Èasová prodleva je zde nastavena na 425 us.
// Výpoèet:
// T[us] = 1000000/(Fosc/(255-RADIO_TCNT0)/Pøeddìlièka) = 
// = 1000000/(4800000/(255-0)/8) = 425us

#define RADIO_TCNT0 0x00  //TCNT=0
#define RADIO_TCCR0B 0x02 //Pøeddìlièka clkIO/8

// Globální:
/*******************************************************************************
 * void RADIO_init(void)
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * Uvede rádiovou komunikaci do výchozího stavu.
 *******************************************************************************/
void RADIO_init(void);

/*******************************************************************************
 * void RADIO_sendData(uint8_t *arrayData, uint8_t lenght)
 * -----------------------------------------------------------------------------
 * *arrayData - Ukazatel na místo v pamìti, kde je uložena sére bajtù
 * lenght - Délka série bajtù.
 * -----------------------------------------------------------------------------
 * Zašle sérii bajtù, uloženou v pamìti.
 *******************************************************************************/
void RADIO_sendData(uint8_t *arrayData, uint8_t lenght);
