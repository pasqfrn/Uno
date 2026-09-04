/**
 * @file bst_private.h
 * @brief Rappresentazione INTERNA dell'ADT BST (uso esclusivo dei moduli di implementazione).
 *
 * Questo header NON fa parte dell'interfaccia pubblica e NON deve essere
 * incluso dagli utilizzatori dell'ADT. Contiene la definizione del tipo
 * `struct NodoAlbero`, la cui parte pubblica e' esposta come tipo OPACO
 * in `lib/data_structures/bst.h`.
 */

#ifndef DATA_STRUCTURES_BST_PRIVATE_H
#define DATA_STRUCTURES_BST_PRIVATE_H

#include "../data_structures.h"

/**
 * @brief Nodo dell'albero BST (rappresentazione interna).
 *
 * Contiene un Utente (chiave = email) e i puntatori ai figli.
 */
struct NodoAlbero {
    Utente utente;
    struct NodoAlbero* left;
    struct NodoAlbero* right;
};

#endif /* DATA_STRUCTURES_BST_PRIVATE_H */