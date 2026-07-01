/**
 * @file list.c
 * @brief Implementazione della lista concatenata per le carte UNO.
 * @ingroup list
 *
 * ADT: Lista concatenata per la mano dei giocatori.
 * Operazioni supportate:
 *   - CreaLista: inizializza lista vuota
 *   - InsertInCoda: inserisce carta in coda (per pesca)
 *   - InsertInTesta: inserisce carta in testa (per scarti)
 *   - GetAt: ottiene carta per indice
 *   - RemoveAt: rimuove carta per indice
 *   - RemoveNode: rimuove nodo specifico
 *   - EliminaLista: distrugge tutta la lista
 *
 * Caratteristiche: lista DOPPIAMENTE concatenata (prev + prossimo).
 */
#include <stdio.h>
#include <stdlib.h>
#include "../lib/data_structures.h"
#include "../lib/list.h"

/* ============================================================
 *  IMPLEMENTAZIONE LISTA CONCATENATA BIDIREZIONALE
 * ============================================================ */

/**
 * @brief Crea una nuova lista di carte vuota.
 * @ingroup list
 * @post Lista allocata e inizializzata (testa=coda=NULL, lunghezza=0).
 * @return Puntatore alla lista, o NULL se out-of-memory.
 */
ListaCarte* CreaLista(void) {
    ListaCarte* lista = (ListaCarte*)malloc(sizeof(ListaCarte));
    if (lista) {
        lista->testa = NULL;
        lista->coda = NULL;
        lista->lunghezza = 0;
    }
    return lista;
}

/**
 * @brief Inserisce una carta in coda alla lista.
 * @ingroup list
 * @pre lista != NULL.
 * @post Carta aggiunta in coda; lunghezza incrementata.
 * @param lista Puntatore alla lista.
 * @param carta La carta da inserire.
 */
void InsertInCoda(ListaCarte* lista, Carta carta) {
    if (!lista) return;

    NodoCarta* nuovo = (NodoCarta*)malloc(sizeof(NodoCarta));
    if (!nuovo) return;

    nuovo->carta = carta;
    nuovo->prossimo = NULL;
    nuovo->prev = lista->coda;   // AGGIORNATO: link al precedente

    if (lista->testa == NULL) {
        lista->testa = nuovo;
    } else {
        lista->coda->prossimo = nuovo;
    }
    lista->coda = nuovo;
    lista->lunghezza++;
}

/**
 * @brief Inserisce una carta in testa alla lista.
 * @ingroup list
 * @pre lista != NULL.
 * @post Carta aggiunta in testa; lunghezza incrementata.
 * @param lista Puntatore alla lista.
 * @param carta La carta da inserire.
 */
void InsertInTesta(ListaCarte* lista, Carta carta) {
    if (!lista) return;

    NodoCarta* nuovo = (NodoCarta*)malloc(sizeof(NodoCarta));
    if (!nuovo) return;

    nuovo->carta = carta;
    nuovo->prossimo = lista->testa;
    nuovo->prev = NULL;          // AGGIORNATO: è la nuova testa

    if (lista->testa) {
        lista->testa->prev = nuovo;  // AGGIORNATO: la vecchia testa punta a noi
    } else {
        lista->coda = nuovo;          // lista era vuota
    }
    lista->testa = nuovo;
    lista->lunghezza++;
}

/**
 * @brief Ottiene il nodo all'indice specificato (0-based).
 * @ingroup list
 * @pre lista != NULL; 0 <= indice < lista->lunghezza.
 * @return Puntatore al NodoCarta, o NULL se fuori range.
 * @param lista Puntatore alla lista.
 * @param indice Indice (0-based).
 */
NodoCarta* GetAt(ListaCarte* lista, int indice) {
    if (!lista || indice < 0 || indice >= lista->lunghezza) {
        return NULL;
    }

    NodoCarta* corrente = lista->testa;
    for (int i = 0; i < indice; i++) {
        if (corrente == NULL) return NULL;
        corrente = corrente->prossimo;
    }
    return corrente;
}

/**
 * @brief Rimuove la carta all'indice specificato.
 * @ingroup list
 * @pre lista != NULL; 0 <= indice < lista->lunghezza.
 * @post Carta rimossa; nodo deallocato; lunghezza decrementata.
 * @param lista Puntatore alla lista.
 * @param indice Indice (0-based).
 */
void RemoveAt(ListaCarte* lista, int indice) {
    if (!lista || indice < 0 || indice >= lista->lunghezza) {
        return;
    }

    NodoCarta* nodo_da_rimuovere;

    if (indice == 0) {
        nodo_da_rimuovere = lista->testa;
        lista->testa = lista->testa->prossimo;
        if (lista->testa) {
            lista->testa->prev = NULL;  // AGGIORNATO
        } else {
            lista->coda = NULL;          // lista ora vuota
        }
    } else {
        NodoCarta* precedente = GetAt(lista, indice - 1);
        if (!precedente || !precedente->prossimo) return;

        nodo_da_rimuovere = precedente->prossimo;
        precedente->prossimo = nodo_da_rimuovere->prossimo;

        // AGGIORNATO: aggiorna prev del successore (se esiste)
        if (nodo_da_rimuovere->prossimo) {
            nodo_da_rimuovere->prossimo->prev = precedente;
        }

        if (indice == lista->lunghezza - 1) {
            lista->coda = precedente;
        }
    }

    free(nodo_da_rimuovere);
    lista->lunghezza--;
}

/**
 * @brief Rimuove un nodo specifico dalla lista.
 * @ingroup list
 * @pre lista != NULL; nodo appartiene alla lista.
 * @post Nodo rimosso; lunghezza decrementata.
 * @param lista Puntatore alla lista.
 * @param nodo Puntatore al nodo da rimuovere.
 */
void RemoveNode(ListaCarte* lista, NodoCarta* nodo) {
    if (!lista || !nodo) return;

    if (lista->testa == nodo) {
        lista->testa = nodo->prossimo;
        if (lista->testa) {
            lista->testa->prev = NULL;   // AGGIORNATO
        } else {
            lista->coda = NULL;
        }
    } else {
        NodoCarta* corrente = lista->testa;
        while (corrente && corrente->prossimo != nodo) {
            corrente = corrente->prossimo;
        }

        if (corrente) {
            corrente->prossimo = nodo->prossimo;
            // AGGIORNATO: aggiorna prev del successore
            if (nodo->prossimo) {
                nodo->prossimo->prev = corrente;
            }
            if (nodo == lista->coda) {
                lista->coda = corrente;
            }
        }
    }

    free(nodo);
    lista->lunghezza--;
}

/**
 * @brief Elimina completamente la lista e libera la memoria.
 * @ingroup list
 * @pre lista != NULL.
 * @post Tutti i nodi e la struttura deallocati.
 * @param lista Puntatore alla lista.
 */
void EliminaLista(ListaCarte* lista) {
    if (!lista) return;

    NodoCarta* corrente = lista->testa;
    while (corrente) {
        NodoCarta* temp = corrente;
        corrente = corrente->prossimo;
        free(temp);
    }

    free(lista);
}

/**
 * @brief Stampa il contenuto della lista su stdout (debug).
 * @ingroup list
 * @pre lista != NULL o lista puo' essere NULL.
 * @post Contenuto stampato su console.
 * @param lista Puntatore alla lista.
 */
void StampaLista(ListaCarte* lista) {
    if (!lista) {
        printf("Lista NULL\n");
        return;
    }

    printf("Lista (%d carte): ", lista->lunghezza);
    NodoCarta* corrente = lista->testa;
    while (corrente) {
        printf("[%d] -> ", corrente->carta.tipo);
        corrente = corrente->prossimo;
    }
    printf("NULL\n");
}
