/**
 * @file chat_private.h
 * @brief Rappresentazione INTERNA dell'ADT ChatStorico (uso esclusivo dei moduli di implementazione).
 *
 * Questo header NON fa parte dell'interfaccia pubblica e NON deve essere
 * incluso dagli utilizzatori dell'ADT. Contiene la definizione del tipo
 * `struct ChatStorico`, la cui parte pubblica e' esposta come tipo OPACO
 * in `lib/data_structures/chat.h`.
 */

#ifndef DATA_STRUCTURES_CHAT_PRIVATE_H
#define DATA_STRUCTURES_CHAT_PRIVATE_H

#include "../data_structures.h"
#include "../chat.h"

/**
 * @brief Cronologia circolare (FIFO) dei messaggi chat (rappresentazione interna).
 *
 * Buffer circolare: quando si supera la capacita' (CHAT_STORICO_MAX),
 * il messaggio piu' vecchio viene sovrascritto e `head` avanza.
 */
struct ChatStorico {
    MessaggioChat buffer[CHAT_STORICO_MAX];
    int count;      /* Numero di messaggi attualmente memorizzati (<= CHAT_STORICO_MAX) */
    int head;       /* Indice del messaggio piu' vecchio nella FIFO */
};

#endif /* DATA_STRUCTURES_CHAT_PRIVATE_H */