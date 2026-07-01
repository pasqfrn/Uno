/**
 * @file chat.h
 * @brief ADT per la gestione della cronologia chat multiplayer.
 * @defgroup chat Chat
 * @brief Fornisce l'ADT ChatStorico (cronologia circolare FIFO) per
 * memorizzare e formattare i messaggi della chat di gioco.
 *
 * La struttura MessaggioChat è definita in `data_structures.h`.
 */
#ifndef DATA_STRUCTURES_CHAT_H
#define DATA_STRUCTURES_CHAT_H

#include "../data_structures.h"

/** @brief Numero massimo di messaggi nella cronologia chat. */
#define CHAT_STORICO_MAX 15

/**
 * @addtogroup chat_storico Storico Chat
 * @brief Struttura per la cronologia circolare dei messaggi chat.
 * @{
 */
/**
 * @brief Cronologia circolare (FIFO) di messaggi.
 *
 * Quando si supera la capacità, il messaggio più vecchio viene sovrascritto.
 */
typedef struct {
    /** @brief Buffer circolare di messaggi. */
    MessaggioChat buffer[CHAT_STORICO_MAX];
    /** @brief Numero di messaggi attualmente memorizzati (<= CHAT_STORICO_MAX). */
    int count;
    /** @brief Indice del messaggio più vecchio nella FIFO. */
    int head;
} ChatStorico;
/** @} */

/**
 * @addtogroup chat_api API Chat
 * @brief Funzioni per la gestione della chat (creazione, inserimento, accesso, formattazione).
 * @{
 */
/**
 * @brief Crea uno storico chat vuoto.
 * @pre c != NULL.
 * @post Storico inizializzato (count=0, head=0).
 * @param c Puntatore a ChatStorico da inizializzare.
 */
void Chat_Crea(ChatStorico* c);

/**
 * @brief Aggiunge un messaggio in coda (FIFO).
 *
 * Se la cronologia è piena, sovrascrive il messaggio più vecchio.
 *
 * @pre c != NULL; mittente != NULL; testo != NULL.
 * @post count aggiornato; head avanzato se pieno.
 * @param c Puntatore a ChatStorico.
 * @param mittente Username del mittente.
 * @param testo Testo del messaggio.
 */
void Chat_Aggiungi(ChatStorico* c, const char* mittente, const char* testo);

/**
 * @brief Genera un timestamp "HH:MM" nel buffer fornito.
 * @pre out != NULL; out_size >= 6.
 * @post Stringa "HH:MM" scritta in out.
 * @param out Buffer di output (almeno 6 byte).
 * @param out_size Dimensione del buffer.
 */
void Chat_OraCorrente(char* out, size_t out_size);

/**
 * @brief Restituisce il messaggio in posizione i della cronologia.
 *
 * Ordine: 0 = più vecchio, count-1 = più recente.
 *
 * @pre c != NULL; 0 <= i < c->count.
 * @return Puntatore a MessaggioChat, o NULL se indice non valido.
 * @param c Puntatore a ChatStorico.
 * @param i Indice del messaggio (0-based).
 */
const MessaggioChat* Chat_Ottieni(const ChatStorico* c, int i);

/**
 * @brief Svuota la cronologia chat.
 * @pre c != NULL.
 * @post count=0, head=0.
 * @param c Puntatore a ChatStorico.
 */
void Chat_Svuota(ChatStorico* c);

/**
 * @brief Formatta un MessaggioChat in una riga "HH:MM @Mittente: testo".
 *
 * Helper per il rendering delle schermate.
 *
 * @pre m != NULL; out != NULL; out_size >= 320.
 * @post Riga formattata scritta in out.
 * @param m Puntatore a MessaggioChat.
 * @param out Buffer di output (almeno 320 byte).
 * @param out_size Dimensione del buffer.
 */
void Chat_FormattaRiga(const MessaggioChat* m, char* out, size_t out_size);
/** @} */

#endif // DATA_STRUCTURES_CHAT_H
