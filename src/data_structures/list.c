/**
 * @file list.c
 * @brief Implementazione lista doppiamente concatenata (funzioni specifiche).
 * @ingroup list
 *
 * ADT: ListaDoppia (doubly-linked list) per la mano dei giocatori.
 * Funzioni: Lista_InserisciDopo, Lista_Precedente, Lista_Ultimo,
 * Lista_RimuoviNodoBidirezionale, Lista_VerificaBidirezionale.
 */
#include <stdio.h>
#include <stdlib.h>
#include "../../lib/data_structures/data_structures.h"
#include "../../lib/data_structures/list.h"
#include "../../lib/data_structures/private/list_private.h"

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

/* ============================================================
 *  ACCESSOR ADT (Information Hiding)
 *  Implementazione delle operazioni pubbliche che preservano
 *  l'opacita' dei tipi NodoCarta / ListaCarte verso gli utilizzatori.
 * ============================================================ */

/**
 * @brief Restituisce il numero di carte nella lista.
 * @ingroup list
 * @pre lista != NULL.
 * @return Numero di nodi (carte) nella lista, 0 se NULL.
 * @param lista Puntatore alla lista.
 */
int Lista_Lunghezza(const ListaCarte* lista) {
    return lista ? lista->lunghezza : 0;
}

/**
 * @brief Restituisce per VALORE la carta all'indice specificato.
 * @ingroup list
 * @pre lista != NULL; 0 <= indice < Lista_Lunghezza(lista).
 * @return Copia della Carta all'indice, o Carta vuota ({0}) se fuori range.
 * @param lista Puntatore alla lista.
 * @param indice Indice della carta (0-based).
 */
Carta Lista_GetCarta(const ListaCarte* lista, int indice) {
    NodoCarta* n = (lista) ? GetAt((ListaCarte*)lista, indice) : NULL;
    return n ? n->carta : (Carta){0};
}

/**
 * @brief Sostituisce per VALORE la carta all'indice specificato.
 * @ingroup list
 * @pre lista != NULL; 0 <= indice < Lista_Lunghezza(lista).
 * @post Carta all'indice sostituita con la copia fornita.
 * @param lista Puntatore alla lista.
 * @param indice Indice della carta da aggiornare (0-based).
 * @param c Nuova carta (copia inserita al posto della precedente).
 */
void Lista_ImpostaCarta(ListaCarte* lista, int indice, Carta c) {
    if (!lista || indice < 0 || indice >= lista->lunghezza) return;
    NodoCarta* n = GetAt(lista, indice);
    if (n) n->carta = c;
}

/**
 * @brief Copia tutte le carte della lista in un array esterno.
 * @ingroup list
 * @pre lista != NULL; out != NULL; max > 0.
 * @post Le prime min(Lista_Lunghezza(lista), max) carte copiate in out.
 * @return Numero di carte effettivamente copiate.
 * @param lista Puntatore alla lista.
 * @param out Array di destinazione (Carta).
 * @param max Dimensione massima dell'array out.
 */
int Lista_CopiaCarte(const ListaCarte* lista, Carta* out, int max) {
    if (!lista || !out || max <= 0) return 0;
    int n = 0;
    NodoCarta* corrente = lista->testa;
    while (corrente && n < max) {
        out[n++] = corrente->carta;
        corrente = corrente->prossimo;
    }
    return n;
}

/**
 * @brief Sposta la prima carta della lista in fondo (rotazione O(1)).
 * @ingroup list
 * @pre lista != NULL.
 * @post La prima carta e' ora l'ultima; lunghezza invariata.
 * @param lista Puntatore alla lista.
 */
void Lista_MettiPrimaInFondo(ListaCarte* lista) {
    if (!lista || lista->lunghezza <= 1) return;
    NodoCarta* prima = lista->testa;

    /* Scollega la vecchia testa */
    lista->testa = prima->prossimo;
    lista->testa->prev = NULL;

    /* Aggancia la vecchia testa in coda */
    prima->prossimo = NULL;
    prima->prev = lista->coda;
    lista->coda->prossimo = prima;
    lista->coda = prima;
}
