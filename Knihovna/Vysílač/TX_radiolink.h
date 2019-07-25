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
#define RADIO_DAT  PB2

// Nastavení časové prodlevy 'T'
// Zde je využit časovač TIMER0
// Časová prodleva je zde nastavena na 425 us.
// Výpočet:
// T[us] = 1000000/(Fosc/(255-RADIO_TCNT0)/Předdělička) = 
// = 1000000/(4800000/(255-0)/8) = 425us

#define RADIO_TCNT0 0x00  //TCNT=0
#define RADIO_TCCR0B 0x02 //Předdělička clkIO/8

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
 * *arrayData - Ukazatel na místo v paměti, kde je uložena sére bajtů
 * lenght - Délka série bajtů.
 * -----------------------------------------------------------------------------
 * Zašle sérii bajtů, uloženou v paměti.
 *******************************************************************************/
void RADIO_sendData(uint8_t *arrayData, uint8_t lenght);
