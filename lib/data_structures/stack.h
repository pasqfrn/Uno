/**
 * @file stack.h
 * @brief ADT Pila (Stack LIFO) per gestione mazzo di pesca e scarti.
 * @defgroup stack Stack (Pila)
 * @brief Implementa una pila LIFO (Last In, First Out) basata su nodi
 * collegati, utilizzata per il mazzo di pesca e gli scarti UNO.
 *
 * PRINCIPIO DI INFORMATION HIDING:
 * I tipi `NodoPila` e `Pila` sono dichiarati come tipi OPACHI: la loro
 * rappresentazione interna e' definita esclusivamente nel modulo di
 * implementazione (src/data_structures/stack.c). Gli utilizzatori
 * interagiscono solo tramite l'API pubblica di questo header.
 */
#ifndef DATA_STRUCTURES_STACK_H
#define DATA_STRUCTURES_STACK_H

#include "data_structures.h"

/**
 * @addtogroup stack_nodi Nodi Pila
 * @brief Tipo opaco del nodo della pila LIFO.
 * @{
 */
/** @brief Nodo della pila (tipo opaco: rappresentazione interna nascosta). */
typedef struct NodoPila NodoPila;
/** @} */

/**
 * @addtogroup stack_struttura Struttura Pila
 * @brief Container della pila LIFO.
 * @{
 */
/**
 * @brief Pila LIFO (tipo opaco: rappresentazione interna nascosta).
 *
 * Gestisce le carte del mazzo di pesca e della pila scarti.
 * Gli utilizzatori interagiscono solo tramite l'API pubblica.
 */
typedef struct Pila Pila;
/** @} */

/**
 * @addtogroup stack_api API Stack
 * @brief Funzioni per la gestione della pila (push, pop, top, distruzione).
 * @{
 */
/**
 * @brief Crea una pila vuota.
 * @post Pila allocata e inizializzata (size=0).
 * @return Puntatore a Pila, o NULL se out-of-memory.
 */
Pila* Pila_Crea(void);

/**
 * @brief Distruttore: libera tutti i nodi e la struttura Pila.
 * @pre p != NULL.
 * @post Tutta la memoria della pila liberata.
 * @param p Puntatore a Pila.
 */
void Pila_Distruggi(Pila* p);

/**
 * @brief Inserisce una carta in cima alla pila.
 * @pre p != NULL.
 * @post top aggiornato; size incrementato.
 * @param p Puntatore a Pila.
 * @param c Carta da inserire.
 * @return 1 se successo, 0 altrimenti.
 */
int Pila_Push(Pila* p, Carta c);

/**
 * @brief Rimuove la carta in cima alla pila.
 * @pre p != NULL.
 * @post top aggiornato; size decrementato; carta rimossa scritta in out (se non NULL).
 * @param p Puntatore a Pila.
 * @param out Puntatore a Carta per ricevere l'elemento rimosso (può essere NULL).
 * @return 1 se successo, 0 se pila vuota.
 */
int Pila_Pop(Pila* p, Carta* out);

/**
 * @brief Restituisce la carta in cima SENZA rimuoverla.
 * @pre p != NULL.
 * @post Carta in top scritta in out.
 * @param p Puntatore a Pila.
 * @param out Puntatore a Carta per ricevere l'elemento in cima.
 * @return 1 se successo, 0 se pila vuota.
 */
int Pila_Top(const Pila* p, Carta* out);

/**
 * @brief Verifica se la pila è vuota.
 * @pre p != NULL.
 * @return 1 se vuota, 0 altrimenti.
 * @param p Puntatore a Pila.
 */
int Pila_Vuota(const Pila* p);

/**
 * @brief Restituisce la dimensione corrente della pila.
 * @pre p != NULL.
 * @return Numero di elementi nella pila.
 * @param p Puntatore a Pila.
 */
int Pila_Dimensione(const Pila* p);

/**
 * @brief Svuota la pila lasciando la struttura allocata.
 * @pre p != NULL.
 * @post top=NULL, size=0, tutti i nodi liberati.
 * @param p Puntatore a Pila.
 */
void Pila_Svuota(Pila* p);
/** @} */

#endif // DATA_STRUCTURES_STACK_H
