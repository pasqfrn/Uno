/**
 * @file list_private.h
 * @brief Rappresentazione INTERNA dell'ADT ListaCarte (uso esclusivo dei moduli di implementazione).
 *
 * Questo header NON fa parte dell'interfaccia pubblica e NON deve essere
 * incluso dagli utilizzatori dell'ADT. Contiene la definizione dei tipi
 * `struct NodoCarta` e `struct ListaCarte`, la cui parte pubblica e'
 * esposta come tipo OPACO in `lib/data_structures/list.h` (information hiding).
 *
 * I moduli che possono includerlo sono esclusivamente:
 *   - src/data_structures/list.c     (unica implementazione dell'ADT ListaCarte)
 */

#ifndef DATA_STRUCTURES_LIST_PRIVATE_H
#define DATA_STRUCTURES_LIST_PRIVATE_H

#include "../data_structures.h"

/* ============================================================
 *  NODO CARTA - Lista DOPPIAMENTE concatenata
 * ============================================================ */

/**
 * @brief Nodo della lista doppiamente concatenata (rappresentazione interna).
 *
 * Ogni nodo contiene una carta e due puntatori: uno al nodo successivo
 * (prossimo) e uno al nodo precedente (prev). Questo permette
 * inserimenti/eliminazioni O(1) in qualsiasi posizione.
 */
struct NodoCarta {
    Carta carta;                    /* La carta contenuta nel nodo */
    struct NodoCarta* prossimo;     /* Puntatore al nodo successivo (next) */
    struct NodoCarta* prev;         /* Puntatore al nodo precedente (prev) */
};

/* ============================================================
 *  LISTA CARTE - Container
 * ============================================================ */

/**
 * @brief Container della lista di carte (rappresentazione interna).
 *
 * Mantiene i puntatori a testa e coda per operazioni O(1) e la lunghezza
 * corrente per contare rapidamente le carte in mano.
 */
struct ListaCarte {
    struct NodoCarta* testa;        /* Primo nodo della lista */
    struct NodoCarta* coda;         /* Ultimo nodo della lista */
    int lunghezza;                  /* Numero totale di carte nella lista */
};

#endif /* DATA_STRUCTURES_LIST_PRIVATE_H */