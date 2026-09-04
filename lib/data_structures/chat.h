/**
 * @file chat.h
 * @brief ADT per la gestione della cronologia chat multiplayer.
 * @defgroup chat Chat
 * @brief Fornisce l'ADT ChatStorico (cronologia circolare FIFO) per
 * memorizzare e formattare i messaggi della chat di gioco.
 *
 * PRINCIPIO DI INFORMATION HIDING:
 * Il tipo `ChatStorico` e' dichiarato come tipo OPACO: la sua rappresentazione
 * interna e' definita esclusivamente nel modulo di implementazione
 * (src/data_structures/chat.c). Gli utilizzatori interagiscono solo tramite
 * l'API pubblica di questo header.
 *
 * La struttura MessaggioChat è definita in `data_structures.h`.
 */
#ifndef DATA_STRUCTURES_CHAT_H
#define DATA_STRUCTURES_CHAT_H

#include "data_structures.h"

/** @brief Numero massimo di messaggi nella cronologia chat. */
#define CHAT_STORICO_MAX 15

/**
 * @addtogroup chat_storico Storico Chat
 * @brief Tipo opaco della cronologia circolare dei messaggi chat.
 * @{
 */
/**
 * @brief Cronologia circolare (FIFO) di messaggi (tipo opaco).
 *
 * La rappresentazione interna e' nascosta agli utilizzatori; l'accesso
 * avviene tramite le funzioni Chat_Crea/Chat_Aggiungi/Chat_Ottieni.
 */
typedef struct ChatStorico ChatStorico;
/** @} */

/**
 * @addtogroup chat_api API Chat
 * @brief Funzioni per la gestione della chat (creazione, inserimento, accesso, formattazione).
 * @{
 */
/**
 * @brief Crea uno storico chat vuoto (allocazione dinamica).
 * @post Storico allocato e inizializzato (count=0, head=0), o NULL se out-of-memory.
 * @return Puntatore a ChatStorico, o NULL.
 */
ChatStorico* Chat_Crea(void);

/**
 * @brief Distrugge uno storico chat liberandone la memoria.
 * @pre c != NULL.
 * @post Memoria dello storico liberata.
 * @param c Puntatore a ChatStorico.
 */
void Chat_Distruggi(ChatStorico* c);

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
