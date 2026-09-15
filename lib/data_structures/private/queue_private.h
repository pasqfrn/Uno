/**
 * @file queue_private.h
 * @brief Rappresentazione INTERNA dell'ADT CodaTurni (uso esclusivo dei moduli di implementazione).
 *
 * Questo header NON fa parte dell'interfaccia pubblica e NON deve essere
 * incluso dagli utilizzatori dell'ADT. Contiene la definizione dei tipi
 * `struct NodoCoda` e `struct CodaTurni`, la cui parte pubblica e'
 * esposta come tipo OPACO in `lib/data_structures/queue.h`.
 */

#ifndef DATA_STRUCTURES_QUEUE_PRIVATE_H
#define DATA_STRUCTURES_QUEUE_PRIVATE_H

#include "../data_structures.h"
#include "../../game/private/player_private.h"

/**
 * @brief Nodo della coda dei turni (rappresentazione interna).
 *
 * Contiene un Giocatore e i puntatori next/prev per la doppia concatenazione.
 */
struct NodoCoda {
    Giocatore giocatore;
    struct NodoCoda* next;
    struct NodoCoda* prev;
};

/**
 * @brief Coda dei turni (rappresentazione interna).
 *
 * Coda doppiamente concatenata con direzione di scorrimento per il
 * Cambio Giro. `verso` vale +1 (orario) o -1 (antiorario); front/rear
 * vengono scambiati da Coda_InvertiVerso per mantenere la circolarita'.
 */
struct CodaTurni {
    struct NodoCoda* front;     /* Primo giocatore della coda */
    struct NodoCoda* rear;      /* Ultimo giocatore della coda */
    int size;                   /* Numero di giocatori in coda */
    int verso;                  /* +1 = orario, -1 = antiorario */
};

#endif /* DATA_STRUCTURES_QUEUE_PRIVATE_H */