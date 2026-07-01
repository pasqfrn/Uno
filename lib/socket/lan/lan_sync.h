/**
 * @file lan_sync.h
 * @brief Sincronizzazione LAN del database utenti via UDP broadcast.
 * @defgroup lan_sync Sincronizzazione LAN
 * @brief Permette a tutti i dispositivi sulla stessa LAN di sincronizzare
 * i database utenti in tempo reale via UDP broadcast (porta 27017).
 *
 * Funzionalita':
 * - Admin panel su ogni dispositivo vede tutti gli utenti
 * - Controllo unicita' username/email in tutta la LAN
 * - Notifica immediata di nuovi utenti ai dispositivi connessi
 *
 * ADT: BST per merge O(n log n).
 */

#ifndef LAN_SYNC_H
#define LAN_SYNC_H

#include "../../data_structures.h"
#include "../../auth.h"
#include "lan_protocol.h"

/** @brief Flag per evitare broadcast ricorsivi durante SalvaDB. */
extern int dentroSalvaDB;
/** @brief Contatore utenti eliminati. */
extern int numDeletedUsers;
/** @brief Buffer di username eliminati (per compatibilita' screens). */
extern char deletedUsers[512][30];

/**
 * @brief Serializza il database utenti in un buffer JSON.
 * @pre dbUtenti.num_utenti > 0.
 * @post Buffer riempito con stringa JSON; restituisce bytes scritti.
 * @param buf Buffer di output.
 * @param buf_size Dimensione massima del buffer.
 * @return Bytes scritti, o -1 se errore.
 */
int SerializzaDB(char* buf, int buf_size);

/** @brief Porta UDP dedicata alla sincronizzazione LAN (diversa da game 27015). */
#define LAN_SYNC_PORT 27017

/** @brief Intervallo in secondi tra broadcast LAN successivi. */
#define LAN_SYNC_INTERVAL 1.0f

/**
 * @brief Inizializza il modulo LAN (socket UDP, thread di ascolto).
 * @pre Nessuna.
 * @post Socket UDP aperto; thread di ascolto avviato.
 * @return 1 se inizializzato con successo, 0 altrimenti.
 */
int LAN_Init(void);
/**
 * @brief Aggiorna lo stato LAN (gestione ricezione pacchetti, timeout).
 * @pre LAN inizializzata.
 * @post Pacchetti ricevuti processati; broadcast inviati se scaduto intervallo.
 * @param dt Delta time dal frame precedente.
 */
void LAN_Update(float dt);

/**
 * @brief Chiude il modulo LAN e libera risorse.
 * @pre LAN inizializzata.
 * @post Socket chiuso; thread terminato; risorse liberate.
 */
void LAN_Close(void);

/**
 * @brief Verifica se uno username esiste nel database LAN-synced.
 * @pre dbUtenti inizializzato.
 * @return 1 se esiste, 0 altrimenti.
 * @param username Username da verificare.
 * @param skip_index Indice da ignorare (-1 per controllare tutti).
 */
int LAN_UsernameEsiste(const char* username, int skip_index);

/**
 * @brief Verifica se un'email esiste nel database LAN-synced.
 * @pre dbUtenti inizializzato.
 * @return 1 se esiste, 0 altrimenti.
 * @param email Email da verificare.
 */
int LAN_EmailEsiste(const char* email);

/**
 * @brief Forza un broadcast immediato del database LAN.
 * @pre LAN inizializzata e dbUtenti valido.
 * @post Pacchetto UDP broadcast inviato.
 */
void LAN_BroadcastNow(void);

/**
 * @brief Restituisce il numero di utenti nel database LAN.
 * @return Numero di utenti.
 */
int LAN_GetUtentiCount(void);

/**
 * @brief Forza un broadcast immediato dei salvataggi.
 * @pre LAN inizializzata.
 * @post Salvataggi broadcastati.
 */
void LAN_BroadcastSavesNow(void);

/**
 * @brief Broadcasta una richiesta di eliminazione di un salvataggio.
 * @pre LAN inizializzata.
 * @param owner_username Proprietario del salvataggio.
 * @param data_salvataggio Data del salvataggio da eliminare.
 */
void LAN_BroadcastDeleteSave(const char* owner_username, const char* data_salvataggio);

/**
 * @brief Processa un pacchetto DB ricevuto (merge utenti).
 * @pre buf contiene un JSON valido.
 * @param buf Buffer con i dati ricevuti.
 * @param bytes Numero di bytes nel pacchetto.
 * @param offset Offset nel buffer (per logging).
 */
void LAN_ProcessDBPacket(const char* buf, int bytes, int offset);

/**
 * @brief Processa un chunk di salvataggio ricevuto via LAN.
 * @pre chunk != NULL.
 * @param chunk Pacchetto chunk di salvataggio.
 */
void LAN_ProcessSaveChunk(const LanSaveChunkPacket* chunk);

/**
 * @brief Aggiorna lo stato dei salvataggi (scadenza, pulizia).
 * @pre LAN inizializzata.
 * @param dt Delta time.
 */
void LAN_UpdateSaves(float dt);

/**
 * @brief Compatta i salvataggi dell'utente corrente (rimuove duplicati).
 * @pre Utente loggato.
 * @post Salvati solo gli slot non vuoti e piu' recenti.
 */
void CompattaSavesUtenteCorrente(void);

/**
 * @brief Broadcasta una richiesta di eliminazione utente su LAN.
 * @pre LAN inizializzata; username != NULL.
 * @param username Username dell'utente eliminato.
 */
void LAN_BroadcastDeleteUser(const char* username);

/**
 * @brief Broadcasta un pacchetto di sincronizzazione completa delle statistiche.
 *
 * Invia a tutti i dispositivi LAN il pacchetto StatsSyncPacket contenente
 * contatori e storico partite recenti.
 *
 * BUG FIX 5: Garantisce che i profili utente siano sincronizzati tra dispositivi
 * dopo ogni partita, preservando lo storico partite (non solo i contatori).
 *
 * @pre LAN inizializzata; username != NULL.
 * @param username Username del giocatore di cui sincronizzare i dati.
 */
void LAN_BroadcastStatsSync(const char* username);

#endif // LAN_SYNC_H
