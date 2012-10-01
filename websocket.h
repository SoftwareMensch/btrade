/**
 * **************************************************************************************
 * **************************************************************************************
 * *** websocket.h - (C) 2012 by Romano Kleinwächter <romano.kleinwaechter@gmail.com> ***
 * **************************************************************************************
 * **************************************************************************************
 *
 * Implementierung für Websocketverbindungen
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

#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

/** ********** INCLUDES ********** */
/** ********** /INCLUDES ********* */

/** ********** PROTOTYPEN ********** */
/**
 * Einen Websocket öffnen und den Dateidiscriptor zurück geben.
 *
 * @param[in] server Websocketserver
 * @param[in] res Ressource die aufgerufen werden soll
 * @param[in] port Port für Verbindung
 * @return Socketdateidiscriptor oder -1 bei Fehler
 */
int websocket_open(char *server, char *res, int port);

/**
 * Eine Websocketverbindung schließen
 *
 * @param[in] fd Dateidiscriptor
 * @return -1 bei Fehler, sonst alles ok
 */
int websocket_close(int fd);

/** ********** /PROTOTYPEN ********* */

#endif //_WEBSOCKET_H

