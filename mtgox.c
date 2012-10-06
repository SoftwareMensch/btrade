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
#include <pthread.h>
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


/** ********** THREAD VARIABELN ********** */
static void *TH_DATA;
static us_int TH_FIN;
static mtg_t TH_DATA_TYPE;
static pthread_cond_t TH_COND = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t TH_MUTEX = PTHREAD_MUTEX_INITIALIZER;
/** ********** /THREAD VARIABELN ********* */


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

	// Thread Variabeln vorbereiten
	pthread_t stream_th; // Thread um Mt.Gox Streamdaten zu lesen
	TH_FIN = 0; // Thread soll laufen, bei 1 wird er beendet
	TH_DATA = NULL; // Datenzeiger auf 0x0
	TH_DATA_TYPE = -1; // Datentyp zunächst unbekannt

	// Thread erzeugen, die Daten werden jetzt im Hintergrund gestreamt.
	pthread_create(&stream_th, NULL, mtg_parse_data_stream_th, &sock);
	//pthread_join(stream_th, NULL);

	// Solange kein Abbruch erfolgt werden die gestreamten Daten
	// formatiert ausgegeben.
	while(!TH_FIN)
	{
		// Daten anhand ihres Typs ausgeben
		switch(TH_DATA_TYPE)
		{
			case MTG_TYPE_TRADE:	mtg_print_trade((struct mtg_type_trade*)TH_DATA); break;	// Handelsdaten ausgeben
			case MTG_TYPE_TICKER:	mtg_print_ticker((struct mtg_type_ticker*)TH_DATA); break;	// Livetickerdaten ausgeben
			case MTG_TYPE_DEPTH:	mtg_print_depth((struct mtg_type_depth*)TH_DATA); break;	// Depthdaten ausgeben
			default:		break;								// unbekannte Daten (nichts tun)
		}
		// Datentyp zurücksetzen
		TH_DATA_TYPE = -1;

		// Den Thread mitteilen das er weiter machen darf
		pthread_cond_signal(&TH_COND);
	}

	// Socket schließen
	websocket_close(sock);

	// alles gut
	return RET_OK;
}

/**
 * Daten von Stream lesen und parsen. Diese Funktion
 * muss in einem eigenen Thread laufen.
 *
 * @param[in] arg Zeiger auf Argument
 * @return void Zeiger
 */
void *mtg_parse_data_stream_th(void *arg)
{
	// Verbindung als Stream interpretieren
	int fd = *((int*)arg); // Zeigerdaten zu Integer
	FILE *stream = fdopen(fd, "r");
	//FILE *stream = fopen("./Tests/mtgox.big.json", "r");
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
		// Schleife über Threadflag verlassen
		if(TH_FIN)
		{
			pthread_cond_signal(&TH_COND); // ggf. Warten beenden
			break;
		}

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
				char block[strlen(&buffer[begin])+1];
				{
					size_t len = sizeof(block)-sizeof(BLOCK_END);
					memset(&block, 0x00, sizeof(block));
					block[0] = '{'; // Block gültig beginnen
					memcpy(&block[1], &buffer[begin], len);
					block[len+1] = '}'; // Block gültig beenden
					block[len+2] = 0x00; // sauber abschließen
					// Resets
					memset(buffer, 0x00, buffer_size); // Sicherstellen das der alte Speicher sauber ist
					free(buffer); // Puffer für Neubefüllung frei geben
					buffer = NULL; // Zeiger auf Pufferanfang zurücksetzen
					buffer_size = 0; // Puffer beginnt mit neuer Größe
				}

				// Block zum verarbeiten weiter reichen
				void *data = NULL; // Zeiger auf Datenstruktur
				TH_DATA_TYPE = mtg_read_block(block, data); // Block verarbeiten Datenstruktur füllen und Typ übergeben
				TH_DATA = data; // Auf Daten zeigen
				pthread_cond_wait(&TH_COND, &TH_MUTEX); // Warten bis wir weiter machen dürfen
				free(data); // Datenstruktur im Speicher wieder frei geben
			}
		}
	}
	// Datenzeiger resetten
	TH_DATA = NULL;

	// Stream schließen, Heap frei geben & Rückkehr
	fclose(stream);
	free(buffer);
	return NULL;
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
	//printf("Handelsblock: %s\n\n", block);
}

/**
 * Tickerblock lesen
 *
 * @param[in] block JSON Blockstring
 * @param[in] data Zeiger für Datenstruktur
 */
void mtg_read_block_ticker(char *block, void *data)
{
	//printf("Tickerblock: %s\n\n", block);
}

/**
 * Depthblock lesen
 *
 * @param[in] block JSON Blockstring
 * @param[in] data Zeiger für Datenstruktur
 */
void mtg_read_block_depth(char *block, void *data)
{
	//printf("Depthblock: %s\n\n", block);
}

/**
 * Handeldaten formatiert ausgeben
 *
 * @param[in] tr Zeiger auf Handelstruktur
 */
void mtg_print_trade(struct mtg_type_trade *tr)
{
	printf("Tradingdaten\n");

	// TODO: FREE ALL!!!
	// Rückkehr
	return;
}

/**
 * Liveticker formatiert ausgeben
 *
 * @param[in] tk Zeiger auf Tickerstruktur
 */
void mtg_print_ticker(struct mtg_type_ticker *tk)
{
	printf("Tickerdaten\n");

	// TODO: FREE ALL!!!
	// Rückkehr
	return;
}

/**
 * Depthdaten formatiert ausgeben
 *
 * @param[in] dp Zeiger auf Depthstruktur
 */
void mtg_print_depth(struct mtg_type_depth *dp)
{
	printf("Depthdaten\n");

	// TODO: FREE ALL!!!
	// Rückkehr
	return;
}
/** ********** /FUNKTIONEN ********* */

