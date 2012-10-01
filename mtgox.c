/**
 * **********************************************************************************
 * **********************************************************************************
 * *** mtgox.c - (C) 2012 by Romano Kleinwächter <romano.kleinwaechter@gmail.com> ***
 * **********************************************************************************
 * **********************************************************************************
 *
 * Dieses Tool basiert auf den Daten von http://www.bitcoinmonitor.com welcher von
 * Jan Vornberger entwickelt wurde. Dies ist eine erste frühe Version die sicher
 * noch mit sehr sehr vielen Features für die Datenauswertung gefüllt werden kann.
 *
 * Lizensiert unter der GPLv2 (http://www.gnu.de/documents/gpl-2.0.de.html)
 *
 *
 * /------------------------------------------------------------------------------]
 * | Um meine Arbeit für das Bitcoinnetzwerk zu unterstützen bitte ich um Spenden
 * | zu der unten aufgeführten Bitcoinadresse.
 * |
 * | To support my work for the bitcoinnetwork I would like to receive donations
 * | to the bitcoinaddress below.
 * |
 * | DONATION ADDRESS: 1M2kiTW1Bc2NDrpkRk7TZqU5C8cLx2wjvR
 * \------------------------------------------------------------------------------]
 */

/** ********** INCLUDES ********** */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mtgox.h"
#include "btrade.h"
#include "websocket.h"
/** ********** /INCLUDES ********* */


/** ********** FUNKTIONEN ********** */
/**
 * Mainfunktion für den Mt.Gox Livestream Einsprung
 *
 * @param[in] currency Währung die Benutzt werden soll
 * @return Exitcode
 */
int mtgox_main(char *currency)
{
	printf("Derzeit nicht implementiert.\n");

	// alles gut
	return RET_OK;
}
/** ********** /FUNKTIONEN ********* */

