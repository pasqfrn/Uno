/**
 * @file network_utils.h
 * @brief Funzioni di utilità per la gestione della rete (IP, codici stanza, disconnessioni).
 * @defgroup network_utils Network Utils
 * @brief Helper per il modulo di rete: conversione IP, gestione disconnessioni,
 * riallocazione socket e connessione tramite codice stanza.
 */
#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include "../../data_structures.h"
#ifdef _WIN32
    #define NOGDI
    #define NOUSER
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

/**
 * @brief Gestisce una disconnessione inaspettata durante il gioco.
 * @pre gioco != NULL.
 * @post Stato di gioco aggiornato per disconnessione; messaggi mostrati.
 * @param gioco Puntatore allo stato di gioco.
 */
void GestisciDisconnessione(StatoGioco* gioco);

/**
 * @brief Ottiene l'indirizzo IP locale del computer.
 * @pre outbuf != NULL; buflen > 0.
 * @post Stringa IP scritta in outbuf.
 * @param outbuf Buffer di output.
 * @param buflen Dimensione del buffer.
 * @return 1 se successo, 0 altrimenti.
 */
int GetLocalIPAddress(char* outbuf, int buflen);

/**
 * @brief Converte un indirizzo IP in codice stanza leggibile.
 * @pre ip != NULL; code != NULL.
 * @post Codice stanza scritto in code.
 * @param ip Indirizzo IP.
 * @param code Buffer di output.
 */
void IpToCode(const char* ip, char* code);

/**
 * @brief Converte un codice stanza in indirizzo IP.
 * @pre code != NULL; ip != NULL.
 * @post Indirizzo IP scritto in ip.
 * @param code Codice stanza.
 * @param ip Buffer di output.
 * @return 1 se conversione valida, 0 altrimenti.
 */
int CodeToIp(const char* code, char* ip);

/**
 * @brief Si connette a una partita tramite codice stanza e porta base.
 * @pre codice stanza valido.
 * @post Socket connesso all'host; stato multiplayer aggiornato.
 * @param codice Codice stanza.
 * @param portaBase Porta base del server.
 * @return 1 se connesso, 0 altrimenti.
 */
int ConnettiTramiteCodiceStanza(const char* codice, int portaBase);

/**
 * @brief Riassegna gli indici dei socket dopo rimozione di un giocatore.
 * @pre removed_index nel range valido; num_players corretto.
 * @post Socket riallocati; indici aggiornati.
 * @param removed_index Indice del socket rimosso.
 * @param num_players Nuovo numero di giocatori.
 */
void ShiftNetworkSockets(int removed_index, int num_players);

#endif
