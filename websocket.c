/**
 * **************************************************************************************
 * **************************************************************************************
 * *** websocket.c - (C) 2012 by Romano Kleinwächter <romano.kleinwaechter@gmail.com> ***
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

/** ********** INCLUDES ********** */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "websocket.h"
#include "btrade.h"
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
int websocket_open(char *server, char *res, int port)
{
	// Variabeln
	int fd = 0; // Socketdateidiscriptor
	int con = 0; // Rückabe für connect()
	struct hostent *server_host; // Hostinformationen
	struct sockaddr_in server_addr; // Adressinformationen

	// Socket erzeugen
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd<0) Fatal
	(
		strerror(errno),
		RET_NETWORK_ERROR
	);

	// Serverhost auflösen
	server_host = gethostbyname(server);
	if(server_host==NULL) Fatal
	(
		strerror(h_errno),
		RET_NETWORK_ERROR
	);

	// Adressstruktur resetten & Verbindung konfigurieren
	memset(&server_addr, 0x00, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET; // IPv4 Streamsocket
	server_addr.sin_addr.s_addr = *((ul_int*)server_host->h_addr_list[0]); // IP Adresse übernehmen
	server_addr.sin_port = htons(port);
	
	// Verbindung zum Server aufnehmen
	con = connect(fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in));
	if(con<0) Fatal
	(
		strerror(errno),
		RET_NETWORK_ERROR
	);

	// ##### WEBSOCKET-HANDSHAKE NACH RFC-6455 #####
	{
		// GET-Zeile
		char get[5+strlen(res)+11+1]; // Get Request (5="GET ", 11=" HTTP/1.1\r\n", 1=Nullbyte
		memset(&get, 0x00, sizeof(get));
		snprintf(get, sizeof(get), "GET /%s HTTP/1.1\r\n", res);
		// Host-Zeile
		char host[6+strlen(server)+2+1]; // Host Zeile (6="Host: ", 2="\r\n", 1=Nullbyte
		memset(&host, 0x00, sizeof(host));
		snprintf(host, sizeof(host), "Host: %s\r\n", server);
		// Rest
		char upgrade[] = "Upgrade: websocket\r\n"; // Upgrade-Zeile
		char connection[] = "Connection: Upgrade\r\n"; // Connection-Zeile
		char fin[] = "\r\n"; // Abschluss

		// Eine Anfrage aus allem machen
		char request[sizeof(get)+sizeof(host)+sizeof(upgrade)+sizeof(connection)+sizeof(fin)];
		memset(&request, 0x00, sizeof(request));
		snprintf(request, sizeof(request), "%s%s%s%s%s", get, host, upgrade, connection, fin);

		// Request zum Server schicken
		ssize_t written_bytes = write(fd, request, strlen(request));
		if(written_bytes<1) Fatal
		(
			strerror(errno),
			RET_NETWORK_ERROR
		);

		// Header prüfen ob der Server mit dem Handshake einverstanden ist.
		char line[] = "HTTP/1.1 101 Web Socket Protocol Handshake";
		char buff[43]; memset(&buff, 0x00, sizeof(buff));
		ssize_t rb = 0;
		
		// Zu suchende Zeile lesen
		rb = read(fd, &buff, sizeof(buff));
		buff[42] = 0x00;

		// Wenn das nicht stimmt ist was falsch
		if(strncmp(buff,line,sizeof(line)) || rb<1) Fatal
		(
			"Websocket konnte nicht aufgebaut werden",
			RET_NETWORK_ERROR
		);
	}

	// Rückgabe
	return fd;
}

/**
 * Eine Websocketverbindung schließen
 *
 * @param[in] fd Dateidiscriptor
 * @return -1 bei Fehler, sonst alles ok
 */
int websocket_close(int fd)
{
	return close(fd);
}

/** ********** /PROTOTYPEN ********* */

