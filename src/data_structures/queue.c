/**
 * @file queue.c
 * @brief Implementazione Coda (Queue) bidirezionale per i turni di gioco.
 * @ingroup queue
 *
 * ADT: CodaTurni (FIFO con InvertiVerso per carta Cambio Giro).
 * Operazioni: crea, enqueue, dequeue, front, rear, inversione verso,
 * ricerca per indice, copia per indice.
 */
#include <stdlib.h>
#include "../../lib/data_structures.h"
#include "../../lib/data_structures/queue.h"

/* ============================================================
 *  IMPLEMENTAZIONE CODA TURNI
 * ============================================================ */

/**
 * @brief Crea una coda vuota.
 * @ingroup queue
 * @post Coda allocata; verso=1 (orario); size=0.
 * @return Puntatore a CodaTurni, o NULL.
 */
CodaTurni* Coda_Crea(void) {
    CodaTurni* c = (CodaTurni*)malloc(sizeof(CodaTurni));
    if (!c) return NULL;
    c->front = NULL;
    c->rear = NULL;
    c->size = 0;
    c->verso = 1;   // di default orario
    return c;
}

/**
 * @brief Distrugge la coda e libera la memoria.
 * @ingroup queue
 * @pre c != NULL.
 * @post Memoria liberata.
 * @param c CodaTurni.
 */
void Coda_Distruggi(CodaTurni* c) {
    if (!c) return;
    NodoCoda* cur = c->front;
    while (cur) {
        NodoCoda* tmp = cur;
        cur = cur->next;
        free(tmp);
    }
    free(c);
}

/**
 * @brief Inserisce un giocatore in coda.
 * @ingroup queue
 * @pre c != NULL.
 * @post Giocatore aggiunto; size incrementato.
 * @param c CodaTurni.
 * @param g Giocatore.
 * @return 1 se successo, 0 altrimenti.
 */
int Coda_Enqueue(CodaTurni* c, Giocatore g) {
    if (!c) return 0;
    NodoCoda* n = (NodoCoda*)malloc(sizeof(NodoCoda));
    if (!n) return 0;
    n->giocatore = g;
    n->next = NULL;
    n->prev = c->rear;
    if (c->rear) c->rear->next = n;
    else c->front = n;
    c->rear = n;
    c->size++;
    return 1;
}

/**
 * @brief Rimuove il giocatore in testa.
 * @ingroup queue
 * @pre c != NULL.
 * @post Giocatore rimosso; size decrementato.
 * @param c CodaTurni.
 * @param out Puntatore output (puo' essere NULL).
 * @return 1 se successo, 0 se vuota.
 */
int Coda_Dequeue(CodaTurni* c, Giocatore* out) {
    if (!c || !c->front) return 0;
    NodoCoda* n = c->front;
    if (out) *out = n->giocatore;
    c->front = n->next;
    if (c->front) c->front->prev = NULL;
    else c->rear = NULL;
    free(n);
    c->size--;
    return 1;
}

/**
 * @brief Restituisce il giocatore in testa (senza rimuovere).
 * @ingroup queue
 * @pre c != NULL.
 * @return 1 se successo, 0 se vuota.
 * @param c CodaTurni.
 * @param out Puntatore output.
 */
int Coda_Front(const CodaTurni* c, Giocatore* out) {
    if (!c || !c->front) return 0;
    if (out) *out = c->front->giocatore;
    return 1;
}

/**
 * @brief Restituisce il giocatore in fondo.
 * @ingroup queue
 * @pre c != NULL.
 * @return 1 se successo, 0 se vuota.
 * @param c CodaTurni.
 * @param out Puntatore output.
 */
int Coda_Rear(const CodaTurni* c, Giocatore* out) {
    if (!c || !c->rear) return 0;
    if (out) *out = c->rear->giocatore;
    return 1;
}

/**
 * @brief Restituisce il prossimo nodo nella direzione corrente.
 * @ingroup queue
 * @pre c != NULL; n != NULL.
 * @return Prossimo nodo (con wrap-around), o NULL.
 * @param c CodaTurni.
 * @param n Nodo corrente.
 */
NodoCoda* Coda_ProxNodo(const CodaTurni* c, NodoCoda* n) {
    if (!c || !n) return NULL;
    if (c->verso == 1) {
        return n->next ? n->next : c->front;  // wrap-around orario
    } else {
        return n->prev ? n->prev : c->rear;    // wrap-around antiorario
    }
}

/**
 * @brief Verifica se la coda e' vuota.
 * @ingroup queue
 * @return 1 se vuota, 0 altrimenti.
 * @param c CodaTurni.
 */
int Coda_Vuota(const CodaTurni* c) {
    return (!c || c->size == 0);
}

/**
 * @brief Restituisce il numero di giocatori.
 * @ingroup queue
 * @return Dimensione, 0 se NULL.
 * @param c CodaTurni.
 */
int Coda_Dimensione(const CodaTurni* c) {
    return c ? c->size : 0;
}

/**
 * @brief Inverte il verso di scorrimento (carta Cambio Giro).
 * @ingroup queue
 * @pre c != NULL.
 * @post verso invertito; front/rear scambiati.
 * @param c CodaTurni.
 */
void Coda_InvertiVerso(CodaTurni* c) {
    if (!c) return;
    c->verso = -c->verso;
    // Per mantenere la circolarità corretta scambiamo front/rear
    NodoCoda* tmp = c->front;
    c->front = c->rear;
    c->rear = tmp;
}

/**
 * @brief Cerca un nodo per indice (0 = front).
 * @ingroup queue
 * @pre c != NULL; indice valido.
 * @return NodoCoda, o NULL.
 * @param c CodaTurni.
 * @param indice Indice (0-based).
 */
NodoCoda* Coda_CercaPerIndice(const CodaTurni* c, int indice) {
    if (!c || indice < 0 || indice >= c->size) return NULL;
    NodoCoda* cur = c->front;
    for (int i = 0; i < indice && cur; i++) cur = cur->next;
    return cur;
}

/**
 * @brief Copia il giocatore all'indice in out.
 * @ingroup queue
 * @pre c != NULL; out != NULL; indice valido.
 * @post Giocatore copiato.
 * @return 1 se successo, 0 altrimenti.
 * @param c CodaTurni.
 * @param indice Indice (0-based).
 * @param out Puntatore output.
 */
int Coda_CopiaPerIndice(const CodaTurni* c, int indice, Giocatore* out) {
    NodoCoda* n = Coda_CercaPerIndice(c, indice);
    if (!n || !out) return 0;
    *out = n->giocatore;
    return 1;
}
