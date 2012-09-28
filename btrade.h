/**
 * **********************************************************************************
 * ***********************************************************************************
 * *** btrade.h - (C) 2012 by Romano Kleinwächter <romano.kleinwaechter@gmail.com> ***
 * ***********************************************************************************
 * ***********************************************************************************
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

#ifndef _BTRADE_H
#define _BTRADE_H

#define VERSION "0.3"

/** ********** RETURNCODES ********** */
#define RET_PARAM_ERROR -6
#define RET_OPENSSL_ERROR -5
#define RET_ALOC_ERROR -4
#define RET_PRINT_ERROR -3
#define RET_JSON_ERROR -2
#define RET_CURL_ERROR -1
#define RET_OK 0
#define RET_USAGE 1
/** ********** /RETURNCODES ********* */

/** ********** INCLUDES ********** */
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
/** ********** /INCLUDES ********* */

/** ********** MAKROS ********** */
/**
 * Syntaktisch korrekte Fehlerausgabe bei falschen Funktionsparametern
 */
#define PARAM_FATAL(file, func) ParamFatal(file" in "func)
/** ********** /MAKROS ********* */

/** ********** DATENTYPEN ********** */
typedef double curr_t;
typedef short int s_int;
typedef unsigned int u_int;
typedef unsigned short int us_int;
/** ********** /DATENTYPEN ********* */

/** ********** PROTOTYPEN ********** */
/**
 * Hilfeausgabe
 *
 * @return void
 */
void usage();

/**
 * Gibt Daten als Base64 kodierten String zurück
 *
 * @param[in] data Zeiger auf Daten
 * @param[in] len Länge der Daten
 * @return Zeiger auf Base64 String
 */
char* base64_encode(void *data, size_t len);

/**
 * Dekodiert einen Base64 String
 *
 * @param[in] data Zeiger auf Anfang des Strings
 * @param[out] len Länge der Originaldaten
 * @return Startadresse der Originaldaten
 */
void* base64_decode(char *data, size_t *len);

/**
 * Fatale Fehler die eine sofortige Beendigung des Programms zur Folge haben.
 *
 * @param[in] msg Nachricht die auf stderr ausgegeben wird
 * @param[in] code Rückgabecode
 */
void Fatal(char *msg, s_int code) __attribute__ ((noreturn));

/**
 * Fatale Fehler aufgrund falscher Funktionsparameter
 *
 * @param[in] place Wo der Fehler passiert ist
 */
void ParamFatal(char *place) __attribute__ ((noreturn));
/** ********** /PROTOTYPEN ********* */

#endif //_BTRADE_H
