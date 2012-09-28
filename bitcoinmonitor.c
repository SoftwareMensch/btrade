/**
 * *******************************************************************************************
 * *******************************************************************************************
 * *** bitcoinmonitor.c - (C) 2012 by Romano Kleinwächter <romano.kleinwaechter@gmail.com> ***
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

/** ********** INCLUDES ********** */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <json/json.h>
#include "btrade.h"
#include "bitcoinmonitor.h"
/** ********** /INCLUDES ********* */

/** ********** KONFIGURATION ********** */
static const size_t DEFAULT_DATALEN = 10;
static const char BM_DATA_URL[] = "http://www.bitcoinmonitor.com/data/-1";
/** ********** /KONFIGURATION ********* */

/** ********** FUNKTIONEN ********** */
/**
 * Empfangene JSON Daten von bitcoinmonitor.com parsen.
 *
 * @param[in] len Länge der empfangenen Daten
 * @return Zeiger auf eine Liste von struct trade* (Handelsmatrix)
 */
struct trade** parse_data(size_t *len)
{
	// Parameter testen
	if(len==NULL) PARAM_FATAL(__FILE__, "parse_data()");

	struct trade **ret = (struct trade**)calloc(DEFAULT_DATALEN, sizeof(struct trade)); // Speicher vorbereiten
	char *jstr = fetch_data(); // JSON Daten holen
	json_object *jobj = json_tokener_parse(jstr); // JSON parsen
	free(jstr); jstr = NULL; // Den String brauchen wir nicht mehr
	size_t count = 0; // Anzahl der Daten

	// Auf JSON Fehler prüfen
	if(jobj==NULL)
	{
		fprintf(stderr, "[!!] Fatal, JSON Daten konnten nicht geparsed werden.");
		exit(RET_JSON_ERROR);
	}

	// Über JSON Daten iterieren
	json_object_object_foreach(jobj, key, val)
	{
		// Uns interessieren nur die Plotdaten
		if(!strncmp("plot_data",key,9))
		{
			// Iteration über Plotdaten
			for(int i=0; i<json_object_array_length(val); ++i)
			{
				json_object *pdata = json_object_array_get_idx(val, i);
				// Das was uns interessiert ist wiederum ein Array auf welches pdata zeigt.
				// Da wir die Indezies kennen greifen wir direkt auf die Daten zu. Wir definieren
				// im Vorfeld unsere Stackvariabeln und befüllen diese dann mit den Daten.
				// Da uns nur der Typ "trade" interessiert filtern wir im Vorfeld danach.
				json_object *jtime = json_object_array_get_idx(pdata, 0);
				json_object *jtype = json_object_array_get_idx(pdata, 1);
				json_object *jamount = json_object_array_get_idx(pdata, 2);
				json_object *jmsg = json_object_array_get_idx(pdata, 3);

				// Typ übernehmen
				char type[8]; memset(&type, 0x00, 8);
				snprintf(type, 8, "%s", json_object_to_json_string(jtype));

				// Auf "trade" prüfen
				if(!strncmp(type,"\"trade\"",7))
				{
					// Platzhalter vorbereiten
					char msg[strlen(json_object_to_json_string(jmsg))+1];
					memset(&msg, 0x00, sizeof(msg)); // Message Speicher resetten
					time_t ts = 0;
					curr_t amount = 0.0;
					curr_t rate = 0.0;

					// msg befüllen (führendes und letztes " entfernen)
					char tmp[sizeof(msg)];
					memset(&tmp, 0x00, sizeof(tmp));
					memcpy(&tmp, json_object_to_json_string(jmsg), sizeof(msg));
					memcpy(&msg, &tmp[1], sizeof(tmp)-1);
					msg[strlen(msg)-1] = 0x00;

					// time befüllen (Auf 10 Stellen kürzen für time_t)
					char _ts[11];
					memset(&_ts, 0x00, 11);
					memcpy(_ts, json_object_to_json_string(jtime), 10);
					ts = atol(_ts);

					// Anzahl befüllen
					amount = atof(json_object_to_json_string(jamount));

					// Wechselkurs extrahieren
					char *p = strstr(msg, "@");
					char _rate[sizeof(msg)];
					memset(&_rate, 0x00, sizeof(_rate));
					// Fehler
					if(p==NULL)
					{
						fprintf(stderr, "[!!] Fatal, es wurde kein Wechselkurs gefunden.");
						exit(RET_JSON_ERROR);
					}
					p += 2; // Position korrigieren
					// String ohne @ + 1 kopieren
					memcpy(_rate, p, strlen(p)-4);
					rate = atof(_rate); // Rate als echten double übernehmen

					// ISO Währungscode extrahieren
					char *curr_iso = strrchr(p, ' ');
					// Fehler
					if(curr_iso==NULL)
					{
						fprintf(stderr, "[!!] Fatal, es wurde kein ISO Code für die Währung gefunden.");
						exit(RET_JSON_ERROR);
					}
					curr_iso += 1; // Position korrigieren

					// Exchanger ermitteln
					char exch[EXCH_LEN]; memset(&exch, 0x00, EXCH_LEN);
					char *exch_begin = strchr(msg, '(');
					if(exch_begin==NULL) sprintf(exch, "%s", "unknown");
					else
					{
						++exch_begin; // Führende Klammer "(" übergehen
						char *exch_end = strchr(exch_begin, ')')-3; // Letzte Klammer und Währung ignorieren
						if(exch_end==NULL) sprintf(exch, "%s", "unknown");
						else
						{
							*exch_end = 0x00; // String abschließen
							snprintf(exch, EXCH_LEN-1, "%s", exch_begin);
						}
					}

					// Daten in eigene Struktur schreiben & ggf. den Speicher dafür reallokieren
					if(count>=(DEFAULT_DATALEN-1)) ret = (struct trade**)realloc(ret, ((count+1)*sizeof(struct trade)));
					ret[count] = new_trade(ts, amount, rate, curr_iso, exch);
					++count; // Datensatz mitzählen
				}

				// Zeiger zurücksetzen
				jmsg = NULL; jamount = NULL; jtype = NULL; jtime = NULL; pdata = NULL;
			}
		}
	}

	// JSON Objekt frei geben
	json_object_put(jobj); jobj = NULL;

	// Korrekte Länge setzen und Zeiger auf Tradematrix zurückgeben.
	*len = count;
	return ret;
}

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
struct trade* new_trade(time_t ts, curr_t amount, curr_t rate, char *curr, char *exch)
{
	// Parameter testen
	if(ts==0 || amount<0 || rate<0 || curr==NULL) PARAM_FATAL(__FILE__, "new_trade()");

	// Speicher allokieren
	struct trade *t = (struct trade*)malloc(sizeof(struct trade));
	memset(t, 0x00, sizeof(struct trade));

	// Daten übernehmen
	t->ts = ts;
	t->amount = amount;
	t->rate = rate;
	snprintf(t->currency, 4, "%s", curr);

	// Exchanger speichern und auf Buffergrenzen achten
	if(strlen(exch)>EXCH_LEN) exch[EXCH_LEN-1] = 0x00; // Sicheres Stringende by Overflow
	snprintf(t->exchanger, strlen(exch)+1, "%s", exch); // Daten übernehmen

	// Zeiger auf Struktur zurückgeben
	return t;
}

/**
 * Holt die zu verarbeitenden JSON Daten via libcurl vom bitcoinmonitor Server
 * und gibt einen Zeiger auf den Anfang dieser Daten zurück. Da die Daten JSON
 * sind gehen wir davon aus das wir das Ende später leicht mit strlen() feststellen
 * können.
 *
 * @return Zeiger auf den Anfang der JSON Daten
 */
char* fetch_data()
{
	CURL *curl = NULL;
	CURLcode res;
	char *data = NULL;
	struct data curl_data;

	// Speicher allokieren
	curl_data.buffer = (char*)malloc(1);
	memset(curl_data.buffer, 0x00, 1);
	curl_data.size = 0;

	// Curl initialisieren & konfigurieren
	curl = curl_easy_init();
	res = curl_easy_setopt(curl, CURLOPT_URL, BM_DATA_URL);
	if(curl==NULL || res!=CURLE_OK)
	{
		fprintf(stderr, "[!!] Fatal, curl konnte nicht initialisiert werden.\n");
		exit(RET_CURL_ERROR);
	}
	// Wenn das geklappt hat geht auch das.
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_data);

	// Request abschicken & Curl aufräumen
	res = curl_easy_perform(curl);
	if(res != CURLE_OK)
	{
		fprintf(stderr, "[!!] Fatal, curl HTTP Request konnte nicht ausgefürht werden.\n");
		exit(RET_CURL_ERROR);
	}
	// Curl aufräumen
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	curl = NULL;

	// Platz auf dem Heap allokieren
	data = (char*)malloc(curl_data.size);
	memset(data, 0x00, curl_data.size);
	memcpy(data, curl_data.buffer, curl_data.size);

	// frei geben
	free(curl_data.buffer);
	curl_data.buffer = NULL;

	// Rückgabe
	return data;
}

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
size_t write_data(char *ptr, size_t size, size_t nmemb, struct data *stream)
{
	// Parameter testen
	if(ptr==NULL || size<1 || nmemb<1 || stream==NULL) PARAM_FATAL(__FILE__, "write_data()");

	// Speicher dynamisch allokieren & merken
	stream->size += (size*nmemb)+1;
	stream->buffer = (char*)realloc(stream->buffer,stream->size);
	strncat(stream->buffer, (char*)ptr, size*nmemb);

	// Rückgabe der Größe
	return size*nmemb;
}

/**
 * Formartierte und ausgewerte Ausgabe der Daten für die Konsole.
 *
 * @param[in] tr Zeiger auf Handelsmatrix
 * @param[in] len Anzahl der Zeilen der Matrix
 * @param[in[ iso ISO Währungscode nach dem gefiltert werden soll
 * @return void
 */
void print_data(struct trade **tr, size_t len, char *iso)
{
	// Parameter testen
	if(tr==NULL || *tr==NULL || len<1 || iso==NULL) PARAM_FATAL(__FILE__, "print_data()");

	// Vorab prüfen ob es die verlangte Währung gibt
	us_int is_iso=0;
	for(size_t i=0; i<len; ++i)
	{
		if(!strncmp(tr[i]->currency,iso,3))
		{
			is_iso = 1;
			break;
		}
	}
	if(is_iso==0)
	{
		fprintf(stderr, "Ein Handel in der Währung \"%s\" konnte in den aktuellen Daten nicht festgestellt werden.\n", iso);
		exit(RET_PRINT_ERROR);
	}

	// Daten für die Zusammenfassung
	size_t count = 0;
	curr_t sum_rate = 0.0;
	curr_t sum_amount = 0.0;

	// Zeiger für Tradings
	struct trade *start_trade = NULL;
	struct trade *end_trade = NULL;

	// Überschrift
	printf
	(
		"\n%-11s %-20s %-18s %22s %22s\n%s\n",
		"Position", "Exchanger", "Datum/Uhrzeit", "Bitcoins", "Währung",
		"-------------------------------------------------------------------------------------------------------------------"
	);

	// Daten iterieren und formatiert ausgeben.
	for(size_t i=0; i<len; ++i)
	{
		// Zeiger auf aktuellen Datensatz
		struct trade *t = tr[i];

		// Währungsfilter
		if(!strncmp(t->currency,iso,3))
		{
			// Uhrzeit holen
			struct tm *local_time = localtime(&t->ts);
			// Formatierte Ausgabe der aktuellen Zeile
			printf
			(
				"%-11zu %-20s %02u.%02u.%04u %02u:%02u:%02u %17f BTC %17f %s (%f %s)\n",
				++count, t->exchanger, local_time->tm_mday, (local_time->tm_mon+1), ((local_time->tm_year)+1900),
				local_time->tm_hour, local_time->tm_min, local_time->tm_sec,
				t->amount, t->rate, t->currency, (t->amount*t->rate), t->currency
			);
			// Summieren (für Zusammenfassung)
			sum_rate += t->rate;
			sum_amount += t->amount;
			// Start- & Endtrades merken (zum eingrenzen des Zeitfensters)
			if(count==1) start_trade = tr[i]; // Anfang
			end_trade = tr[i]; // Ende
		}
	}

	// Zeitfenster für die ausgegebenen Trades ermitteln
	time_t diff_sec = (end_trade->ts-start_trade->ts);
	struct tm *diff_time = localtime(&diff_sec); // In tm Struktur abbilden

	// Zusammenfassung formatiert ausgeben
	printf
	(
		"\nETA %02u:%02u:%02u\t%f BTC\t%f %s (%f %s)\n",
		diff_time->tm_hour-1, diff_time->tm_min, diff_time->tm_sec,
		sum_amount, (sum_rate/count), iso, (sum_amount*(sum_rate/count)), iso
	);

	// Rückkehr
	return;
}

/**
 * Deallokiert alle Daten in unserer Handelsmatrix
 *
 * @param[in] t Zeiger auf Handelsmatrix
 * @param[in] len Anzahl der Zeilen in der Matrix
 * @return void
 */
void free_matrix_data(struct trade **t, size_t len)
{
	// Parameter testen
	if(t==NULL || *t==NULL || len<1) PARAM_FATAL(__FILE__, "free_matrix_data()");

	size_t i = 0;
	for(i=0; i<len; ++i) free(t[i]);
	free(t);

	// Rücksprung
	return;
}
/** ********** /FUNKTIONEN ********* */
