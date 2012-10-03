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
#include <unistd.h>
#include <errno.h>
#include <json/json.h>
#include "mtgox.h"
#include "btrade.h"
#include "websocket.h"
/** ********** /INCLUDES ********* */


/** ********** KONFIGURATION ********** */
static char HOST[] = "websocket.mtgox.com";
static int PORT = 80;
static char MONITOR_RES[] = "mtgox";
/** ********** /KONFIGURATION ********* */


/** ********** MTGOX CHANNEL ********** */
const static char CH_TICKER[]	= "d5f06780-30a8-4a48-a2f8-7ed181b4a13f";
const static char CH_TRADE[] 	= "dbf1dee9-4f2e-4a08-8cb7-748919a71b21";
const static char CH_DEPTH[]	= "24e67e0d-1cad-4cc0-9e7a-f8523ef460fe";
/** ********** /MTGOX CHANNEL ********* */

/** ********** FUNKTIONEN ********** */
/**
 * Mainfunktion für den Mt.Gox Livestream Einsprung
 *
 * @param[in] currency Währung die Benutzt werden soll
 * @return Exitcode
 */
int mtg_main(char *currency)
{
	// Ressource mit Währung ergänzen
	char res[strlen(MONITOR_RES)+strlen(currency)+10+1]; // 10="?Currency="
	memset(&res, 0x00, sizeof(res));
	snprintf(res, sizeof(res), "%s?Currency=%s", MONITOR_RES, currency);

	// Verbindung zur Mt.Gox Websocket API aufbauen und
	// Socketfilediscriptor zurückgeben
	int sock = websocket_open(HOST, res, PORT);

	// Daten parsen
	mtg_parse_data(sock);

	// Socket schließen
	websocket_close(sock);

	// alles gut
	return RET_OK;
}

/**
 * Daten von Stream lesen und parsen
 *
 * @param[in] fd Socketdateidiscriptor
 * @return void
 */
void mtg_parse_data(int fd)
{
	// Verbdinung als Stream interpretieren
	FILE *stream = fdopen(fd, "r");
	if(stream==NULL) Fatal
	(
		strerror(errno),
		RET_NETWORK_ERROR
	);

	// Erstmal alles ausgeben was ankommt
	int c = 0;
	while((c=fgetc(stream))!=EOF)
	{
		printf("%c", (char)c);
	}

	// Stream schließen & Rückkehr
	fclose(stream);
	return;
}
/** ********** /FUNKTIONEN ********* */

