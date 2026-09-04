/**
 * @file stack_private.h
 * @brief Rappresentazione INTERNA dell'ADT Pila (uso esclusivo dei moduli di implementazione).
 *
 * Questo header NON fa parte dell'interfaccia pubblica e NON deve essere
 * incluso dagli utilizzatori dell'ADT. Contiene la definizione dei tipi
 * `struct NodoPila` e `struct Pila`, la cui parte pubblica e' esposta
 * come tipo OPACO in `lib/data_structures/stack.h`.
 */

#ifndef DATA_STRUCTURES_STACK_PRIVATE_H
#define DATA_STRUCTURES_STACK_PRIVATE_H

#include "../data_structures.h"

/**
 * @brief Nodo della pila LIFO (rappresentazione interna).
 *
 * Contiene una carta e il puntatore al nodo sottostante.
 */
struct NodoPila {
    Carta carta;
    struct NodoPila* next;
};

/**
 * @brief Pila LIFO (rappresentazione interna).
 *
 * `top` punta all'ultimo elemento inserito; `size` e' la cardinalita'.
 */
struct Pila {
    struct NodoPila* top;
    int size;
};

#endif /* DATA_STRUCTURES_STACK_PRIVATE_H */