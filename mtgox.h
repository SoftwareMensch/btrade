/**
 * **********************************************************************************
 * **********************************************************************************
 * *** mtgox.h - (C) 2012 by Romano Kleinwächter <romano.kleinwaechter@gmail.com> ***
 * **********************************************************************************
 * **********************************************************************************
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

#ifndef _MTGOX_H
#define _MTGOX_H

/** ********** INCLUDES ********** */
#include "btrade.h"
/** ********** /INCLUDES ********* */


/** ********** DEFINITIONEN ********** */
typedef s_int mtg_t;
#define MTG_TYPE_VALUE	0x00
#define MTG_TYPE_TRADE	0x01
#define MTG_TYPE_TICKER	0x02
#define MTG_TYPE_DEPTH	0x03
/** ********** /DEFINITIONEN ********* */


/** ********** STRUKTUREN ********** */
/**
 * Abstrakte Channel Struktur, für Details bitte
 * in der Mt.Gox API Dokumentation nachschauen.
 *
 * @param uuid Eindeutige UUID für den Channel
 * @param op Operation
 * @param origin ???
 * @param private In der Regel der Channelname
 * @param type Zeiger auf die eingebettete Struktur welche eigentliche Daten enthält.
 */
struct mtg_channel
{
	char *uuid;
	char *op;
	char *origin;
	char *private;
	void *type;
};

/**
 * Mt.Gox Datentyp für die Ausgabe von "Werten"
 *
 * @param currency Währungssymbol
 * @param display Fertige Ausgabe als Text
 * @param value Wert als String
 * @param value_int Wert als reiner Integer
 */
struct mtg_type_value
{
	char *currency;
	char *display;
	char *value;
	ul_int value_int;
};

/**
 * Mt.Gox Datentyp für Trades
 *
 * @param amount Anzahl der gehandelten Elemente
 * @param item Währungszeichen der gehandelten Elemente
 * @param price Wert für 1 gehandeltes Element
 * @param price_currency Währung für gehandelten Preis
 * @param properties Diverse Eigenschaften
 * @param tid Trade ID
 * @param trade_type Typ des Handels
 * @param type Fast nutzlos, ist meist "trade"
 * @param amount_int Wert der Elemente als reiner Integer
 * @param price_int Preis für 1 Element als reiner Integer
 * @param date Timestamp des Handels
 * @param primary ???
 */
struct mtg_type_trade
{
	char *amount;
	char *item;
	char *price;
	char *price_currency;
	char *properties;
	char *tid;
	char *trade_type;
	char *type;
	ul_int amount_int;
	ul_int price_int;
	time_t date;
	char primary;
};

/**
 * Mt.Gox Daten für den Liveticker
 *
 * @param avg Durchschnittswert
 * @param buy Kaufwert
 * @param high Höchstwert
 * @param last Letzter Wert
 * @param last_local Letzter lokaler Wert
 * @param last_orig Letzer originaler Wert
 * @param low Tiefstwert
 * @param sell Verkaufswert
 * @param vol Gesamtvolumen
 * @param vwap ???
 */
struct mtg_type_ticker
{
	struct mtg_type_value *avg;
	struct mtg_type_value *buy;
        struct mtg_type_value *high;
        struct mtg_type_value *last;
        struct mtg_type_value *last_local;
        struct mtg_type_value *last_orig;
        struct mtg_type_value *low;
        struct mtg_type_value *sell;
        struct mtg_type_value *vol;
        struct mtg_type_value *vwap;
};

/**
 * Mt.Gox Datentyp um die Marktiefe zu reflektieren
 *
 * @param currency Währung
 * @param item Gehandeltes Element
 * @param now ???
 * @param price Preis als String
 * @param type_str Typ als String (ask, bid)
 * @param volume Volumen als String
 * @param price_int Preis als Integer
 * @param volume_int Volumen als Integer
 * @param total_volume_int Gesamtvolumen als Integer
 * @param Typ als Integer
 */
struct mtg_type_depth
{
	char *currency;
	char *item;
	char *now;
	char *price;
	char *type_str;
	char *volume;
	ul_int price_int;
	ul_int volume_int;
	ul_int total_volume_int;
	us_int type;
};
/** ********** /STRUKTUREN ********* */


/** ********** PROTOTYPEN ********** */
/**
 * Mainfunktion für den Mt.Gox Livestream Einsprung
 *
 * @param[in] currency Währung die Benutzt werden soll
 * @return Exitcode
 */
int mtg_main(char *currency);

/**
 * Daten von Stream lesen und parsen. Diese Funktion
 * muss in einem eigenen Thread laufen.
 *
 * @param[in] arg Zeiger auf Argument
 * @return void Zeiger
 */
void *mtg_parse_data_stream_th(void *arg);

/**
 * Blocktype ermitteln
 *
 * @param[in] block Zeiger auf Blockanfang
 * @return Typidentifizierung für den Block oder -1 bei Fehler
 */
mtg_t mtg_get_block_type(char *block);

/**
 * Block lesen, Struktur ermitteln, Daten
 * speichern und Typ zurückgeben.
 *
 * @param[in] block Blockstring (JSON)
 * @param[in] Zeiger auf neue Datenstruktur
 * @return Mt.Gox Typ des Blocks
 */
mtg_t mtg_read_block(char *block, void *data);

/**
 * Handelsblock lesen
 *
 * @param[in] block JSON Blockstring
 * @param[in] data Zeiger für Datenstruktur
 */
void mtg_read_block_trade(char *block, void *data);

/**
 * Tickerblock lesen
 *
 * @param[in] block JSON Blockstring
 * @param[in] data Zeiger für Datenstruktur
 */
void mtg_read_block_ticker(char *block, void *data);

/**
 * Depthblock lesen
 *
 * @param[in] block JSON Blockstring
 * @param[in] data Zeiger für Datenstruktur
 */
void mtg_read_block_depth(char *block, void *data);

/**
 * Handeldaten formatiert ausgeben
 *
 * @param[in] tr Zeiger auf Handelstruktur
 */
void mtg_print_trade(struct mtg_type_trade *tr);

/**
 * Liveticker formatiert ausgeben
 *
 * @param[in] tk Zeiger auf Tickerstruktur
 */
void mtg_print_ticker(struct mtg_type_ticker *tk);

/**
 * Depthdaten formatiert ausgeben
 *
 * @param[in] dp Zeiger auf Depthstruktur
 */
void mtg_print_depth(struct mtg_type_depth *dp);
/** ********** /PROTOTYPEN ********* */

#endif //_MTGOX_H

