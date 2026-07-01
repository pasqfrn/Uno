/**
 * @file list.c
 * @brief Implementazione lista doppiamente concatenata (funzioni specifiche).
 * @ingroup list
 *
 * ADT: ListaDoppia (doubly-linked list) per la mano dei giocatori.
 * Funzioni: Lista_InserisciDopo, Lista_Precedente, Lista_Ultimo,
 * Lista_RimuoviNodoBidirezionale, Lista_VerificaBidirezionale.
 */
#include <stdlib.h>
#include "../../lib/data_structures.h"
#include "../../lib/data_structures/list.h"

/* ============================================================
 *  FUNZIONI SPECIFICHE LISTA BIDIREZIONALE
 * ============================================================ */

/**
 * @brief Inserisce una carta dopo un nodo specifico (o in testa se nodo==NULL).
 * @ingroup list
 * @pre lista != NULL.
 * @post Carta inserita; lunghezza incrementata.
 * @param lista Lista.
 * @param nodo Nodo dopo cui inserire (NULL per inserimento in testa).
 * @param c Carta.
 * @return Puntatore al nuovo nodo, o NULL.
 */
NodoCarta* Lista_InserisciDopo(ListaCarte* lista, NodoCarta* nodo, Carta c) {
    if (!lista) return NULL;
    NodoCarta* nuovo = (NodoCarta*)malloc(sizeof(NodoCarta));
    if (!nuovo) return NULL;
    nuovo->carta = c;
    nuovo->prossimo = NULL;
    nuovo->prev = NULL;

    if (nodo == NULL) {
        // Inserimento in testa
        nuovo->prossimo = lista->testa;
        if (lista->testa) {
            lista->testa->prev = nuovo;
        }
        lista->testa = nuovo;
        if (lista->coda == NULL) lista->coda = nuovo;
    } else {
        // Inserimento dopo `nodo`
        NodoCarta* succ = nodo->prossimo;
        nuovo->prossimo = succ;
        nuovo->prev = nodo;
        if (succ) {
            succ->prev = nuovo;
        }
        nodo->prossimo = nuovo;
        if (lista->coda == nodo) lista->coda = nuovo;
    }
    lista->lunghezza++;
    return nuovo;
}

/**
 * @brief Restituisce il nodo precedente a quello dato.
 * @ingroup list
 * @param lista Lista (non usato).
 * @param nodo Nodo di riferimento.
 * @return Nodo precedente, o NULL.
 */
NodoCarta* Lista_Precedente(ListaCarte* lista, NodoCarta* nodo) {
    (void)lista;
    if (!nodo) return NULL;
    return nodo->prev;
}

/**
 * @brief Restituisce l'ultimo nodo della lista (coda).
 * @ingroup list
 * @pre lista != NULL.
 * @return Puntatore alla coda, o NULL.
 * @param lista Lista.
 */
NodoCarta* Lista_Ultimo(ListaCarte* lista) {
    if (!lista) return NULL;
    return lista->coda;
}

/**
 * @brief Rimuove un nodo dalla lista gestendo correttamente prev/next.
 * @ingroup list
 * @pre lista != NULL; nodo appartiene alla lista.
 * @post Nodo rimosso; testa/coda aggiornati; lunghezza decrementata.
 * @param lista Lista.
 * @param nodo Nodo da rimuovere.
 */
void Lista_RimuoviNodoBidirezionale(ListaCarte* lista, NodoCarta* nodo) {
    if (!lista || !nodo) return;

    NodoCarta* p = nodo->prev;
    NodoCarta* s = nodo->prossimo;

    if (p) p->prossimo = s;
    else   lista->testa = s;

    if (s) s->prev = p;
    else   lista->coda = p;

    free(nodo);
    lista->lunghezza--;
}

/**
 * @brief Verifica l'integrita' dei puntatori prev/next (debug).
 * @ingroup list
 * @param lista Lista.
 * @return 1 se valida, 0 se inconsistente.
 */
int Lista_VerificaBidirezionale(ListaCarte* lista) {
    if (!lista) return 1;
    NodoCarta* prev = NULL;
    NodoCarta* cur = lista->testa;
    while (cur) {
        if (cur->prev != prev) return 0;
        prev = cur;
        cur = cur->prossimo;
    }
    if (prev != lista->coda) return 0;
    return 1;
}
