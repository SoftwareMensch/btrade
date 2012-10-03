/**
 * ***********************************************************************************
 * ***********************************************************************************
 * *** btrade.c - (C) 2012 by Romano Kleinwächter <romano.kleinwaechter@gmail.com> ***
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

/** ********** INCLUDES ********** */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include "btrade.h"
#include "mtgox.h"
#include "bitcoinmonitor.h"
/** ********** /INCLUDES ********* */

/**
 * main, der Anfang vom Ende
 *
 * @param[in] argc Anzahl der Argumente
 * @param[in] argv Argumentliste
 * @return Exitcode
 */
int main(int argc, char **argv)
{
	// Optionen definieren
	const char short_opts[] = "vb:m:";
	const struct option long_opts[] =
	{
		{"version",	0, NULL, 'v'},
		{"btm",		0, NULL, 'b'},
		{"mtgox",	0, NULL, 'm'},
		{NULL,		0, NULL, 0}
	};

	// Durch Kommandozeile laufen
	int next = 0;
	do
	{
		// Auf Optionen reagieren
		switch((next=getopt_long(argc, argv, short_opts, long_opts, NULL)))
		{
			case 'v':	printf("Version: %s\n", VERSION); return RET_OK; break;	// Version ausgeben
			case 'b':	return btm_main(optarg); break;				// Daten von bitcoinmonitor.com abrufen
			case 'm':	return mtg_main(optarg); break;				// Ausgabe des Mt.Gox Livestreams
			default:	usage(); return RET_USAGE; break;			// Hilfe ausgeben
		}
	}
	while(next != -1);

	// normales Ende
	return RET_OK;
}

/** ********** FUNKTIONEN ********** */
/**
 * Hilfeausgabe
 *
 * @return void
 */
void usage()
{
	printf
	(
		"\n"
		"*******************************************************************************\n"
		"** btrade - (C) 2012 by Romano Kleinwächter <romano.kleinwaechter@gmail.com> **\n"
		"**                                                                           **\n"
		"** Um meine Arbeit für das Bitcoinnetzwerk zu unterstützen bitte ich um      **\n"
		"** Spenden an diese Adresse: 1M2kiTW1Bc2NDrpkRk7TZqU5C8cLx2wjvR              **\n"
		"*******************************************************************************\n"
		"\n"
		"\n"
		"Benutzung:\n"
		"\n"
		"\t%-30s %s\n" // Flags 1
		"\t%-30s %s\n" // Flag 2
		"\t%-30s %s\n" // Flag 3
		"\n",

		// Beschreibungen
		"-v | --version", "Ausgabe der Programmversion", // Version
		"-b | --btm <EUR, USD, etc>", "Ausgewertete Daten von http://www.bitcoinmonitor.com anzeigen", // bitcoinmonitor.com
		"-m | --mtgox <EUR, USD, etc>", "Ausgabe des Mt.Gox Livestreams" // Mt.Gox
	);

	// Rücksprung
	return; // Rückkehr
}

/**
 * Gibt Daten als Base64 kodierten String zurück
 *
 * @param[in] data Zeiger auf Daten
 * @param[in] len Länge der Daten
 * @return Zeiger auf Base64 String
 */
char* base64_encode(void *data, size_t len)
{
	// Parameter testen
	if(data==NULL || len==0) PARAM_FATAL(__FILE__, "base64_encode()");

	// Vorbereitungen
	BIO *raw = NULL; // Zeiger auf binäre Daten
	BIO *b64 = NULL; // Zeiger auf Base64 Daten
	BUF_MEM *praw = NULL; // Zeiger für Rohdaten im Puffer
	char *b64_str = NULL; // Zeiger für Base64 String

	// Speicher allokieren
	b64 = BIO_new(BIO_f_base64()); // Base64 Daten
	raw = BIO_new(BIO_s_mem()); // Rohe binäre Daten im Speicher
	
	// Auf Fehler prüfen
	if(b64==NULL || raw==NULL) Fatal
	(
		"base64_encode(): Es konnte kein Speicher allokiert werden.",
		RET_ALOC_ERROR
	);

	b64 = BIO_push(b64, raw); // Beide Speicherbereiche hintereinander legen und Ende zurückgeben
	if(BIO_write(b64,data,len)<=0) Fatal // Originale Daten nach b64 schreiben
	(
		"base64_encode(): Eingabedaten konnten nicht kopiert werden.",
		RET_OPENSSL_ERROR
	);
	if(BIO_flush(b64)<=0) Fatal // Sicherstellen das alle Daten geschrieben werden
	(
		"base64_encode(): Es konnten nicht alle Eingabedaten geschrieben werden.",
		RET_OPENSSL_ERROR
	);
	BIO_get_mem_ptr(b64, &praw); // BUF_MEM Struktur von b64 in praw abbilden
	if(praw==NULL) Fatal // Fehler
	(
		"base64_encode(): Speicherstruktur konnte nicht abgebildet werden.",
		RET_OPENSSL_ERROR
	);

	// Speicher für Base64 String allokieren, resetten und befüllen
	b64_str = (char*)malloc(praw->length);
	memset(b64_str, 0x00, praw->length);
	memcpy(b64_str, praw->data, praw->length-1);
	b64_str[praw->length-1] = 0x00; // Abschließendes Nullbyte gewährleisten

	// Verwendeten und nicht mehr benötigten Speicher frei geben
	BIO_free_all(b64);

	// Rückgabe der Stringadresse
	return b64_str;
}

/**
 * Dekodiert einen Base64 String
 *
 * @param[in] data Zeiger auf Anfang des Strings
 * @param[out] len Länge der Originaldaten
 * @return Startadresse der Originaldaten
 */
void* base64_decode(char *data, size_t *len)
{
	// Parameter testen
	if(data==NULL || len==NULL) PARAM_FATAL(__FILE__, "base64_decode()");

	// Vorbereitungen
	BIO *b64 = NULL; // Zeiger auf Base64 Daten
	BIO *raw = NULL; // Zeiger auf Originaldaten
	BUF_MEM *praw = NULL; // Zeiger für Rohdaten im Puffer
	void *dec_data = NULL; // Zeiger auf den Anfang der dekodierten Daten

	// Speicher allokieren & vorbereiten
	b64 = BIO_new(BIO_f_base64()); // Base64 Daten
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // Sicherstellen das OpenSSL nicht auf Newlines wartet
	raw = BIO_new_mem_buf(data, strlen(data)); // Platz für Originaldaten
	if(b64==NULL || raw==NULL) Fatal // Fehler
	(
		"base64_decode(): Speichstrukturen für Dekodierung konnten nicht erzeugt werden.",
		RET_OPENSSL_ERROR
	);
	raw = BIO_push(b64, raw); // Speicher zusammenlegen
	BIO_get_mem_ptr(raw, &praw); // BUF_MEM Struktur von b64 in praw abbilden
	if(praw==NULL) Fatal // Fehler
	(
		"base64_decode(): Speicherstruktur konnte nicht abgebildet werden.",
		RET_OPENSSL_ERROR
	);

	// Speicher für dekodierte Daten allokieren & vorbereiten
	dec_data = malloc(praw->length);
	memset(dec_data, 0x00, praw->length);
	int ret = BIO_read(raw, dec_data, praw->length); // Daten dekodiert in den Speicher lesen
	if(ret <= 0) Fatal // Fehler
	(
		"base64_decode(): String konnte nicht dekodiert werden. Es konnten keine Daten gelesen werden",
		RET_OPENSSL_ERROR
	);
	*len = (size_t)ret; // Länge merken
	BIO_free_all(raw); // Nich mehr benötigten Speicher frei geben

	// Zeiger auf dekodierte Daten zurückgeben
	return dec_data;
}

/**
 * Ermittelt den kleinesten Währungswert in einem Array
 *
 * @param[in] list Zeiger auf Array
 * @param[in] len Länge des Feldes
 * @return Kleinste Zahl im Array
 */
curr_t find_min(curr_t *list, size_t len)
{
	curr_t val = 0.0; // Rückgabewert
	for(size_t i=0; i<len; ++i) if(list[i]<val || val==0.0) val = list[i]; // Sortieren
	return val; // Rückgabe
}

/**
 * Ermittelt den durchschnittlichen Währungswert in einem Array
 *
 * @param[in] list Zeiger auf Array
 * @param[in] len Länge des Feldes
 * @return Durchschnittswert
 */
curr_t find_avg(curr_t *list, size_t len)
{
	curr_t val = 0.0; // Rückgabewert
	for(size_t i=0; i<len; ++i) val += list[i]; // Summieren
	return (val/len); // Rückgabe 
}

/**
 * Ermittelt den größten Währungswert in einem Array
 *
 * @param[in] list Zeiger auf Array
 * @param[in] len Länge des Feldes
 * @return Größte Zahl im Array
 */
curr_t find_max(curr_t *list, size_t len)
{
	curr_t val = 0.0; // Rückgabewert
	for(size_t i=0; i<len; ++i) if(list[i]>val || val==0.0) val = list[i]; // Sortieren
	return val; // Rückgabe
}

/**
 * Ermittelt den am häufigsten, kleinsten gehandelten Kurs
 *
 * @param[in] list Zeiger auf Array
 * @param[in] len Länge des Feldes
 * @param[out] proz Prozentualer Anteil
 * @return Den am häufigsten gehandelten Kurs
 */
curr_t find_most_min_rate(curr_t *list, size_t len, float *proz)
{
	curr_t val = 0.0; // Rückgabewert
	size_t max_pos = 0; // Position mit höchstem Wert
	size_t count[len]; // Feld zum zählen der Zahlen

	// Zähler initialisieren
	for(size_t i=0; i<len; ++i) count[i] = 0;

	// Zahlen durchlaufen und gucken wie oft sie auftauchen
	for(size_t i=0; i<len; ++i)
	{
		// Auf aktuelle Zahl zeigen
		curr_t *p = &list[i];
		// Vergleichen
		for(size_t j=0; j<len; ++j) if(list[j]==*p) ++count[i];
	}

	// Den größten Zähler und damit den häufigsten Kurs feststellen
	for(size_t i=0, max=0; i<len; ++i)
	{
		if(count[i]>max || max==0)
		{
			max = count[i];
			max_pos = i;
		}
	}
	// Den vorerst häufigsten Wert ünbernehmen
	val = list[max_pos];

	// Es kann sein das auch andere Werte die gleich Anzahl haben,
	// danach suchen wir nun, finden wir so einen Wert wird er mit
	// dem letzten Wert verglichen, ist er kleiner wird er ersetzt.
	int new_max_pos = -1;
	for(size_t i=0; i<len; ++i)
	{
		// Treffer, es wurde ein weiterer Wert gefunden der genauso oft
		// vor kommt wie der den wir schon hatten.
		if((count[i]==count[max_pos]) && (i!=max_pos))
		{
			// Größen vergleichen, der kleinere gewinnt.
			if(list[i]<val)
			{
				val = list[i];
				new_max_pos = i;
			}
		}
	}
	// Ggf. neue max_pos übernehmen
	if(new_max_pos != -1) max_pos = (size_t)new_max_pos;

	// Häufigsten Wert und dessen Anteil endgültig übernehmen
	val = list[max_pos];
	*proz = ((count[max_pos]*100)/len);

	// Rückgabe
	return val;
}

/**
 * Fatale Fehler die eine sofortige Beendigung des Programms zur Folge haben.
 *
 * @param[in] msg Nachricht die auf stderr ausgegeben wird
 * @param[in] code Rückgabecode
 */
void Fatal(char *msg, s_int code)
{
	fprintf(stderr, "[!!] FATAL: %s\n", msg);
	exit(code);
}

/**
 * Fatale Fehler aufgrund falscher Funktionsparameter
 *
 * @param[in] place Wo der Fehler passiert ist
 */
void ParamFatal(char *place)
{
	const char fatal[] = ": Funktionsparameter ungültig";
	char msg[strlen(place)+strlen(fatal)+1];
	memset(&msg, 0x00, sizeof(msg));
	snprintf(msg, sizeof(msg), "%s%s", place, fatal);
	Fatal(msg, RET_PARAM_ERROR);
}
/** ********** /FUNKTIONEN ********* */
