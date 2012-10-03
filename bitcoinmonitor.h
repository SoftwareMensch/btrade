/**
 * *******************************************************************************************
 * *******************************************************************************************
 * *** bitcoinmonitor.h - (C) 2012 by Romano Kleinwächter <romano.kleinwaechter@gmail.com> ***
 * *******************************************************************************************
 * *******************************************************************************************
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

#ifndef _BITCOINMONITOR_H
#define _BITCOINMONITOR_H

#define EXCH_LEN 20

/** ********** INCLUDES ********** */
#include <time.h>
/** ********** /INCLUDES ********* */


/** ********** STRUKTUREN ********** */
/**
 * Trading Daten
 *
 * @param ts Zeitpunkt des Handels
 * @param amount Anzahl der gehandelten Bitcoins
 * @param rate Wechselkurs zur Währung
 * @param currency ISO Währungscode + Nullbyte
 * @param exchanger Name des Exchanger wo die Bitcoins gehandelt wurden
 */
struct btm_trade
{
	time_t ts;
	curr_t amount;
	curr_t rate;
	char currency[4];
	char exchanger[EXCH_LEN];
};

/**
 * Server Daten für Curl
 *
 * @param buffer Unser Abbild der empfangenen Daten
 * @param size Größe unserer Abbilddaten
 */
struct btm_data
{
	char *buffer;
	size_t size;
};
/** ********** /STRUKTUREN ********* */


/** ********** PROTOTYPEN ********** */
/**
 * Einstiegspunkt für den Zugriff auf bitcoinmonitor.com
 *
 * @param[in] currency Zeiger auf die Währung die Verwendet werden soll
 * @return Exitcode
 */
int btm_main(char *currency);

/**
 * Empfangene JSON Daten von bitcoinmonitor.com parsen.
 *
 * @param[in] len Länge der empfangenen Daten
 * @return Zeiger auf eine Liste von struct trade* (Handelsmatrix)
 */
struct btm_trade** btm_parse_data(size_t *);

/**
 * Holt die zu verarbeitenden JSON Daten via libcurl vom bitcoinmonitor Server
 * und gibt einen Zeiger auf den Anfang dieser Daten zurück. Da die Daten JSON
 * sind gehen wir davon aus das wir das Ende später leicht mit strlen() feststellen
 * können.
 *
 * @return Zeiger auf den Anfang der JSON Daten
 */
char* btm_fetch_data();

/**
 * Callbackfunktion für die libcurl um die empfangenen JSON Daten
 * in einem Puffer im Speicher zu schreiben.
 *
 * @param[in] ptr Zeiger auf den Anfang der nächsten Daten im Puffer
 * @param[in] size
 * @param[in[ nmemb Größe des neuen benötigten Speichers
 * @param[out] stream Zeiger auf unsere Daten die wir befüllen wollen
 * @return Länge der neu geschriebenen Daten
 */
size_t btm_write_data(char *ptr, size_t size, size_t nmemb, struct btm_data *stream);

/**
 * Allokiert Speicher für ein neue Handelsstruktur und
 * gibt einen Zeiger auf diese Struktur zurück (struct trade*).
 *
 * @param[in] ts Unixtimestamp (Zeitpunkt des Handels)
 * @param[in] amount Anzahl der gehandelten Bitcoins
 * @param[in] rate Gehandelter Wechselkurs in der jeweiligen Währung
 * @param[in] ISO-Code der Währung (z.B.: EUR, USD oder PLN)
 * @param[in] exch Name des Exchangers
 * @return Zeiger auf neue Handelstruktur (struct trade*)
 */
struct btm_trade* btm_new_trade(time_t ts, curr_t amount, curr_t rate, char *curr, char *exch);

/**
 * Formartierte und ausgewerte Ausgabe der Daten für die Konsole.
 *
 * @param[in] tr Zeiger auf Handelsmatrix
 * @param[in] len Anzahl der Zeilen der Matrix
 * @param[in[ iso ISO Währungscode nach dem gefiltert werden soll
 * @return void
 */
void btm_print_data(struct btm_trade **tr, size_t len, char *iso);

/**
 * Deallokiert alle Daten in unserer Handelsmatrix
 *
 * @param[in] t Zeiger auf Handelsmatrix
 * @param[in] len Anzahl der Zeilen in der Matrix
 * @return void
 */
void btm_free_matrix_data(struct btm_trade **t, size_t len);
/** ********** /PROTOTYPEN ********* */

#endif //_BITCOINMONITOR_H
