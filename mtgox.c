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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <json/json.h>
#include "mtgox.h"
#include "btrade.h"
#include "websocket.h"
/** ********** /INCLUDES ********* */


/** ********** KONFIGURATION ********** */
static char HOST[] = "websocket.mtgox.com";
static int PORT = 80;
static char MONITOR_RES[] = "mtgox";
static char BLOCK_BEGIN[] = { '"', 'c', 'h', 'a', 'n', 'n', 'e', 'l', '"' };
static char BLOCK_END[] = { 0xFF, 0x00 };
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
	//int sock = websocket_open(HOST, res, PORT);
	int sock = 4;

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
	// Verbindung als Stream interpretieren
	
	//FILE *stream = fdopen(fd, "r");
	FILE *stream = fopen("./Tests/mtgox.big.json", "r");
	if(stream==NULL) Fatal
	(
		strerror(errno),
		RET_NETWORK_ERROR
	);

	// Die Daten kommen in Blöcke, der Anfang eines Blocks
	// beginnt immer mit '{"channel"', das Ende ist binär mit
	// 0xFF00 definiert. Zum testen werden hier ersteinmal nur
	// vollständige Blöcke Zeilenweile ausgegeben.
	int c = 0; // Einzelnes Zeichen aus dem Stream
	char *buffer = NULL;
	size_t buffer_size = 0;
	while((c=fgetc(stream))!=EOF)
	{
		buffer = realloc(buffer, ++buffer_size); // Der Puffer wächst dynamsich
		(*(buffer+(buffer_size-1))) = c; // Puffer befüllen

		// Nach Blockende Ausschau halten (0xFF00)
		if(find_binary(buffer,BLOCK_END,buffer_size,sizeof(BLOCK_END))>=0)
		{
			// Anfangsoffset vom Block finden ("channel")
			sl_int begin = find_binary(buffer, BLOCK_BEGIN, buffer_size, sizeof(BLOCK_BEGIN));
			if(begin>=0)
			{
				// ##### VOLLSTÄNDIGEN BLOCK AUF DEM STACK ERZEUGEN UND SPEICHER FREI GEBEN #####
				char block[strlen(&buffer[begin])];
				{
					size_t len = sizeof(block)-sizeof(BLOCK_END);
					memset(&block, 0x00, sizeof(block));
					block[0] = '{'; // Block gültig beginnen
					memcpy(&block[1], &buffer[begin], len);
					block[len-1] = '}'; // Block gültig beenden
					// Resets
					memset(buffer, 0x00, buffer_size); // Sicherstellen das der alte Speicher sauber ist
					free(buffer); // Puffer für Neubefüllung frei geben
					buffer = NULL; // Zeiger auf Pufferanfang zurücksetzen
					buffer_size = 0; // Puffer beginnt mit neuer Größe
				}

				// Block zum verarbeiten weiter reichen
				void *data = NULL; // Zeiger auf Datenstruktur
				mtg_read_block(block, data); // Block verarbeiten und Datenstruktur füllen
				free(data); // Datenstruktur im Speicher wieder frei geben
			}
		}
	}

	// Stream schließen, Heap frei geben & Rückkehr
	fclose(stream);
	free(buffer);
	return;
}

/** Blocktype ermitteln
 *
 * @param[in] block Zeiger auf Blockanfang
 * @return Typidentifizierung für den Block oder -1 bei Fehler
 */
mtg_t mtg_get_block_type(char *block)
{
	mtg_t type = -1; // Von Fehler ausgehen
	json_object *jo = json_tokener_parse(block);

	// Ungültiges JSON bedeutet ungültiger Block
	if(jo==NULL) return -1;

	// JSON Iteration
	json_object_object_foreach(jo, key, val)
	{
		// Nach Channel-Key suchen
		if(!strncmp(key,"channel",7))
		{
			// In eigenen Speicher kopieren
			size_t len = strlen(json_object_to_json_string(val))+1;
			char *real_val = (char*)calloc(1, len);
			memcpy(real_val, json_object_to_json_string(val), len-1);

			// Erstes und letztes " ignorieren
			(*(real_val+(len-2))) = 0x00;

			// Channel auswerten und entsprechenden Typ zurück geben,
			// real_val+1 damit er erste " übersprungen wird.
			if(!strncmp(real_val+1,CH_TICKER,strlen(CH_TICKER))) // Ticker
			{
				json_object_put(jo);
				free(real_val);
				return MTG_TYPE_TICKER;
			}
			else if(!strncmp(real_val+1,CH_TRADE,strlen(CH_TRADE))) // Trade
			{
				json_object_put(jo);
				free(real_val);
				return MTG_TYPE_TRADE;
			}
			else if(!strncmp(real_val+1,CH_DEPTH,strlen(CH_DEPTH))) // Depth
			{
				json_object_put(jo);
				free(real_val);
				return MTG_TYPE_DEPTH;
			}
			else // Unbekannt
			{
				json_object_put(jo);
				free(real_val);
				return -1;
			}
		}
	}

	// JSON frei geben & Rückgabe (eigentlich sollten wir hier nie ankommen)
	json_object_put(jo);
	return type;
}

/**
 * Block lesen, Struktur ermitteln, Daten
 * speichern und Typ zurückgeben.
 *
 * @param[in] block Blockstring (JSON)
 * @param[in] Zeiger auf neue Datenstruktur
 * @return Mt.Gox Typ des Blocks
 */
mtg_t mtg_read_block(char *block, void *data)
{
	mtg_t type = mtg_get_block_type(block); // Blocktype holen
	void (*read_func)(char*, void*) = NULL;

	// Lesefunktion anhand des Typs zuweisen
	switch(type)
	{
		case MTG_TYPE_TRADE:	read_func = &mtg_read_block_trade; break; // Handelsblock
		case MTG_TYPE_TICKER:	read_func = &mtg_read_block_ticker; break; // Tickerblock
		case MTG_TYPE_DEPTH:	read_func = &mtg_read_block_depth; break; // Depthblock

		// Unbekannt
		default: read_func=NULL; break;
	}

	// Block lesen
	if(read_func!=NULL) read_func(block, data);

	// Typ zurückgeben
	return type;
}

/**
 * Handelsblock lesen
 *
 * @param[in] block JSON Blockstring
 * @param[in] data Zeiger für Datenstruktur
 */
void mtg_read_block_trade(char *block, void *data)
{
	printf("Handelsblock: %s\n\n", block);
}

/**
 * Tickerblock lesen
 *
 * @param[in] block JSON Blockstring
 * @param[in] data Zeiger für Datenstruktur
 */
void mtg_read_block_ticker(char *block, void *data)
{
	printf("Tickerblock: %s\n\n", block);
}

/**
 * Depthblock lesen
 *
 * @param[in] block JSON Blockstring
 * @param[in] data Zeiger für Datenstruktur
 */
void mtg_read_block_depth(char *block, void *data)
{
	printf("Depthblock: %s\n\n", block);
}
/** ********** /FUNKTIONEN ********* */

