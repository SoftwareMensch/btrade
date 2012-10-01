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
#include "mtgox.h"
#include "btrade.h"
#include "websocket.h"
/** ********** /INCLUDES ********* */


/** ********** KONFIGURATION ********** */
static char HOST[] = "websocket.mtgox.com";
static int PORT = 80;
static char MONITOR_RES[] = "mtgox";
/** ********** /KONFIGURATION ********* */

/** ********** FUNKTIONEN ********** */
/**
 * Mainfunktion für den Mt.Gox Livestream Einsprung
 *
 * @param[in] currency Währung die Benutzt werden soll
 * @return Exitcode
 */
int mtgox_main(char *currency)
{
	printf("\n!! IN ARBEIT !!\n\n");

	// Ressource mit Währung ergänzen
	char res[strlen(MONITOR_RES)+strlen(currency)+10+1]; // 10="?Currency="
	memset(&res, 0x00, sizeof(res));
	snprintf(res, sizeof(res), "%s?Currency=%s", MONITOR_RES, currency);

	// Verbindung zur Mt.Gox Websocket API aufbauen und
	// Socketfilediscriptor zurückgeben
	int sock = websocket_open(HOST, res, PORT);

	// Stream puffern bis ein gültiges JSON enstanden ist
	// und dieses verarbeiten.
	char *json=NULL; // Späterer Platz für JSON Daten
	ssize_t cur_bytes=0, all_bytes=0;
	while(1)
	{
		// Puffer (re)initialisieren
		char buff[256];
		memset(&buff, 0x00, 256);

		// Von Stream lesen und Daten merken
		cur_bytes = read(sock, &buff, sizeof(buff));
		if(cur_bytes>0)
		{
			// Ausstieg
			if(all_bytes>=1024) break;

			// Speicher dynamsich halten, kopieren & Bytes zählen
			json = (char*)realloc(json, all_bytes+cur_bytes);
			memcpy(json+all_bytes, buff, cur_bytes);
			all_bytes += cur_bytes;
		}
	}

	for(size_t i=0; i<all_bytes; ++i) printf("%c", json[i]);

	free(json);
	websocket_close(sock);

	// alles gut
	return RET_OK;
}
/** ********** /FUNKTIONEN ********* */

