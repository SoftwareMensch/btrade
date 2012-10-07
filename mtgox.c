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
			default:				break;														// unbekannte Daten (nichts tun)
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
 * Einen neuen Mt.Gox Valuetype erzeugen
 *
 * @param[in] currency Währungsart
 * @param[in] display Anzeige für den Wert (lang)
 * @param[in] display_short Anzeige für den Wert (kurz(
 * @param[in] value Wert als String
 * @param[in] value_int Wert als Integer
 * @param[in] type Typ des Wertes
 * @return Zeiger auf Mt.Gox Value
 */
struct mtg_type_value* mtg_new_value(char *currency, char *display, char *display_short, char *value, ul_int value_int, mtg_value_t type)
{
	// Neue Struktur auf dem Heap erzeugen
	struct mtg_type_value *val = (struct mtg_type_value*)calloc(1, sizeof(struct mtg_type_value));
	// Speicher für die drei Strings allokieren
	val->currency = (char*)calloc(1, strlen(currency)+1);
	val->display = (char*)calloc(1, strlen(display)+1);
	val->display_short = (char*)calloc(1, strlen(display_short)+1);
	val->value = (char*)calloc(1, strlen(value)+1);

	// Werte zuweisen
	memcpy(val->currency, currency, strlen(currency));
	memcpy(val->display, display, strlen(display));
	memcpy(val->display_short, display_short, strlen(display_short));
	memcpy(val->value, value, strlen(value));
	val->value_int = value_int;
	val->type = type;

	// Zeiger auf Struktur zurück geben
	return val;
}

/**
 * Speicherbereich eines Mt.Gox Values frei geben
 *
 * @param[in] val Zeiger auf Datenstruktur
 */
void mtg_free_value(struct mtg_type_value *val)
{
	if(val!=NULL)
	{
		// Zuerst die Strings frei geben
		free(val->currency);		val->currency = NULL;
		free(val->display);			val->display = NULL;
		free(val->display_short);	val->display_short = NULL;
		free(val->value);			val->value = NULL;
		// Jetzt die eigentliche Struktur frei geben
		free(val); val = NULL;
	}

	// Rückkehr
	return;
}

/**
 * Speicherbereich einer Mt.Gox Tickerstruktur frei geben
 *
 * @param[in] tk Zeiger auf Struktur
 */
void mtg_free_ticker(struct mtg_type_ticker *tk)
{
	if(tk!=NULL)
	{
		// Werttypen frei geben
		mtg_free_value(tk->avg);			tk->avg = NULL;
		mtg_free_value(tk->buy);			tk->buy = NULL;
		mtg_free_value(tk->high);			tk->high = NULL;
		mtg_free_value(tk->last);			tk->last = NULL;
		mtg_free_value(tk->last_all);		tk->last_all = NULL;
		mtg_free_value(tk->last_local);		tk->last_local = NULL;
		mtg_free_value(tk->last_orig);		tk->last_orig = NULL;
		mtg_free_value(tk->low);			tk->low = NULL;
		mtg_free_value(tk->sell);			tk->sell = NULL;
		mtg_free_value(tk->vol);			tk->vol = NULL;
		mtg_free_value(tk->vwap);			tk->vwap = NULL;
		// Eigentliche Struktur frei geben
		free(tk); tk = NULL;
	}

	// Rückkehr
	return;
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
	void *data = NULL; // Zeiger auf Datenstruktur
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
				TH_DATA_TYPE = mtg_read_block(block, &data); // Block verarbeiten Datenstruktur füllen und Typ übergeben
				TH_DATA = data; // Auf Daten zeigen
				pthread_cond_wait(&TH_COND, &TH_MUTEX); // Warten bis wir weiter machen dürfen
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
mtg_t mtg_read_block(char *block, void **data)
{
	mtg_t type = mtg_get_block_type(block); // Blocktype holen
	void (*read_func)(char*, void**) = NULL;

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
void mtg_read_block_trade(char *block, void **data)
{
	//printf("Handelsblock: %s\n\n", block);
}

/**
 * Tickerblock lesen
 *
 * @param[in] block JSON Blockstring
 * @param[in] data Zeiger für Datenstruktur
 */
void mtg_read_block_ticker(char *block, void **data)
{
	json_object *jo = json_tokener_parse(block); // Block als JSON parsen
	if(jo==NULL) return; // Bei einem Fehler springen wir einfach zurück
	struct mtg_type_ticker *ticker = (struct mtg_type_ticker*)calloc(1, sizeof(struct mtg_type_ticker)); // Struktur im Heap anlegen

	// Iteration des JSON Blocks
	json_object_object_foreach(jo, key, val)
	{
		// Interessant ist für uns nur der "ticker" Schlüssel
		if(!strncmp(key,"ticker",6))
		{
			// Iteration der Tickerobjekte
			json_object_object_foreach(val, tkey, tval)
			{
				// Generische Zeiger auf Daten erzeugen
				// 0 = currency
				// 1 = display
				// 2 = display_short
				// 3 = value
				// 4 = value_int
				size_t sub_len = 0;
				char *sub_key[5], *sub_val[5];
				json_object_object_foreach(tval, _tkey, _tval)
				{
					// Speicher allokieren
					sub_key[sub_len] = (char*)calloc(1, strlen(_tkey)+2);
					sub_val[sub_len] = (char*)calloc(1, strlen(json_object_to_json_string(_tval))+2);
					// Daten als String kopieren
					snprintf(sub_key[sub_len], strlen(_tkey)+1, "%s", _tkey);
					snprintf(sub_val[sub_len], strlen(json_object_to_json_string(_tval))+1, "%s", json_object_to_json_string(_tval)+1); // +1 um führendes " zu ignorieren
					sub_val[sub_len][strlen(sub_val[sub_len])-1] = 0x00; // Das letzte " entfernen

					// Mitzählen
					++sub_len;
				}

				// ###### WERTTYP ERMITTELN UND DATEN AN KORREKTE POSITION SCHREIBEN #####
				{
					if			(!strncmp(tkey,"avg",3))			ticker->avg			= mtg_new_value(sub_val[0], sub_val[1], sub_val[2], sub_val[3], atol(sub_val[4]), MTG_VALUE_TYPE_AVG);
					else if		(!strncmp(tkey,"buy",3))			ticker->buy			= mtg_new_value(sub_val[0], sub_val[1], sub_val[2], sub_val[3], atol(sub_val[4]), MTG_VALUE_TYPE_BUY);
					else if		(!strncmp(tkey,"high",4))			ticker->high		= mtg_new_value(sub_val[0], sub_val[1], sub_val[2], sub_val[3], atol(sub_val[4]), MTG_VALUE_TYPE_HIGH);
					else if		(!strncmp(tkey,"last",4))			ticker->last		= mtg_new_value(sub_val[0], sub_val[1], sub_val[2], sub_val[3], atol(sub_val[4]), MTG_VALUE_TYPE_LAST);
					else if		(!strncmp(tkey,"last_all",8))		ticker->last_all	= mtg_new_value(sub_val[0], sub_val[1], sub_val[2], sub_val[3], atol(sub_val[4]), MTG_VALUE_TYPE_LASTALL);
					else if		(!strncmp(tkey,"last_local",10))	ticker->last_local	= mtg_new_value(sub_val[0], sub_val[1], sub_val[2], sub_val[3], atol(sub_val[4]), MTG_VALUE_TYPE_LASTLOCAL);
					else if		(!strncmp(tkey,"last_orig",9))		ticker->last_orig	= mtg_new_value(sub_val[0], sub_val[1], sub_val[2], sub_val[3], atol(sub_val[4]), MTG_VALUE_TYPE_LASTORIG);
					else if		(!strncmp(tkey,"low",3))			ticker->low			= mtg_new_value(sub_val[0], sub_val[1], sub_val[2], sub_val[3], atol(sub_val[4]), MTG_VALUE_TYPE_LOW);
					else if		(!strncmp(tkey,"sell",4))			ticker->sell		= mtg_new_value(sub_val[0], sub_val[1], sub_val[2], sub_val[3], atol(sub_val[4]), MTG_VALUE_TYPE_SELL);
					else if		(!strncmp(tkey,"vol",3))			ticker->vol			= mtg_new_value(sub_val[0], sub_val[1], sub_val[2], sub_val[3], atol(sub_val[4]), MTG_VALUE_TYPE_VOL);
					else if		(!strncmp(tkey,"vwap",4))			ticker->vwap		= mtg_new_value(sub_val[0], sub_val[1], sub_val[2], sub_val[3], atol(sub_val[4]), MTG_VALUE_TYPE_VWAP);
				}

				// Heap von Werten befreien
				for(size_t i=0; i<sub_len; ++i)
				{
					free(sub_key[i]);
					free(sub_val[i]);
				}
			}
		}
	}
	// Auf Struktur im Heap zeigen
	*data = ticker;

	// Object löschen & Rückkehr
	json_object_put(jo);
	return;
}

/**
 * Depthblock lesen
 *
 * @param[in] block JSON Blockstring
 * @param[in] data Zeiger für Datenstruktur
 */
void mtg_read_block_depth(char *block, void **data)
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
	//printf("Tradingdaten: %p\n", tr);

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
	// Last price:$11.90159 High:$12.74797 Low:$11.70200 Volume:61872 BTC Weighted Avg:$12.18968
	printf
	(
		"Letzter Preis: %s, Höchstpreis %s, Tiefstpreis %s, Durchschnittspreis %s, Umlauf %s\n",
		tk->last->display, tk->high->display, tk->low->display, tk->avg->display, tk->vol->display
	);

	// Strutkur frei geben & Rückkehr
	mtg_free_ticker(tk);
	return;
}

/**
 * Depthdaten formatiert ausgeben
 *
 * @param[in] dp Zeiger auf Depthstruktur
 */
void mtg_print_depth(struct mtg_type_depth *dp)
{
	//printf("Depthdaten\n");

	// TODO: FREE ALL!!!
	// Rückkehr
	return;
}
/** ********** /FUNKTIONEN ********* */

