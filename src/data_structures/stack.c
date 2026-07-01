/**
 * @file stack.c
 * @brief Implementazione Pila (Stack LIFO) per mazzo di pesca e scarti.
 * @ingroup stack
 *
 * ADT: Pila LIFO per mazzo di pesca e pila scarti.
 * Operazioni: crea, push, pop, top, svuota, distruggi.
 */
#include <stdlib.h>
#include "../../lib/data_structures.h"
#include "../../lib/data_structures/stack.h"

/* ============================================================
 *  IMPLEMENTAZIONE PILA
 * ============================================================ */

/**
 * @brief Crea una pila vuota.
 * @ingroup stack
 * @post Pila allocata; top=NULL; size=0.
 * @return Puntatore a Pila, o NULL.
 */
Pila* Pila_Crea(void) {
    Pila* p = (Pila*)malloc(sizeof(Pila));
    if (!p) return NULL;
    p->top = NULL;
    p->size = 0;
    return p;
}

/**
 * @brief Distrugge la pila e libera la memoria.
 * @ingroup stack
 * @pre p != NULL.
 * @post Memoria liberata.
 * @param p Pila.
 */
void Pila_Distruggi(Pila* p) {
    if (!p) return;
    Pila_Svuota(p);
    free(p);
}

/**
 * @brief Inserisce una carta in cima alla pila.
 * @ingroup stack
 * @pre p != NULL.
 * @post Carta in cima; size incrementato.
 * @param p Pila.
 * @param c Carta.
 * @return 1 se successo, 0 altrimenti.
 */
int Pila_Push(Pila* p, Carta c) {
    if (!p) return 0;
    NodoPila* n = (NodoPila*)malloc(sizeof(NodoPila));
    if (!n) return 0;
    n->carta = c;
    n->next = p->top;
    p->top = n;
    p->size++;
    return 1;
}

/**
 * @brief Rimuove la carta in cima.
 * @ingroup stack
 * @pre p != NULL.
 * @post Carta rimossa; size decrementato.
 * @param p Pila.
 * @param out Puntatore output (puo' essere NULL).
 * @return 1 se successo, 0 se vuota.
 */
int Pila_Pop(Pila* p, Carta* out) {
    if (!p || !p->top) return 0;
    NodoPila* n = p->top;
    if (out) *out = n->carta;
    p->top = n->next;
    free(n);
    p->size--;
    return 1;
}

/**
 * @brief Restituisce la carta in cima (senza rimuovere).
 * @ingroup stack
 * @pre p != NULL.
 * @return 1 se successo, 0 se vuota.
 * @param p Pila.
 * @param out Puntatore output.
 */
int Pila_Top(const Pila* p, Carta* out) {
    if (!p || !p->top) return 0;
    if (out) *out = p->top->carta;
    return 1;
}

/**
 * @brief Verifica se la pila e' vuota.
 * @ingroup stack
 * @return 1 se vuota, 0 altrimenti.
 * @param p Pila.
 */
int Pila_Vuota(const Pila* p) {
    return (!p || p->size == 0);
}

/**
 * @brief Restituisce la dimensione della pila.
 * @ingroup stack
 * @return Dimensione, 0 se NULL.
 * @param p Pila.
 */
int Pila_Dimensione(const Pila* p) {
    return p ? p->size : 0;
}

/**
 * @brief Svuota la pila (libera nodi, mantiene struttura).
 * @ingroup stack
 * @pre p != NULL.
 * @post top=NULL; size=0.
 * @param p Pila.
 */
void Pila_Svuota(Pila* p) {
    if (!p) return;
    NodoPila* cur = p->top;
    while (cur) {
        NodoPila* tmp = cur;
        cur = cur->next;
        free(tmp);
    }
    p->top = NULL;
    p->size = 0;
}
