/**
 * @file list.h
 * @brief ADT Lista Concatenata Bidirezionale per le carte.
 * @defgroup list Lista (Lista Concatenata)
 * @brief Implementa la lista doppiamente concatenata per gestire la mano
 * di ciascun giocatore. Ogni nodo contiene una carta e puntatori
 * al nodo successivo e precedente.
 *
 * Riferimento: Documentazione CdS - "Lista (Lista Concatenata)"
 * "Implementata per gestire la mano di ciascun giocatore. Poiche'
 * il numero di carte in mano varia continuamente (pescando o scartando),
 * una lista doppiamente concatenata permette inserimenti ed eliminazioni
 * efficienti senza sprecare memoria."
 */

#ifndef LIST_H
#define LIST_H

#include "data_structures.h"
#include "data_structures/list.h"  /* NodoCarta, ListaCarte */

/* ============================================================
 *  FUNZIONI DI BASE PER LA LISTA
 * ============================================================ */

/**
 * @brief Crea una nuova lista di carte vuota.
 * @post Lista allocata e inizializzata (testa=coda=NULL, lunghezza=0).
 * @return Puntatore alla lista appena creata (allocata con malloc), o NULL se out-of-memory.
 */
ListaCarte* CreaLista(void);

/**
 * @brief Inserisce una carta in coda alla lista.
 * @pre lista != NULL.
 * @post Carta aggiunta in coda; lunghezza incrementata.
 * @param lista Puntatore alla lista.
 * @param carta La carta da inserire.
 */
void InsertInCoda(ListaCarte* lista, Carta carta);

/**
 * @brief Inserisce una carta in testa alla lista.
 * @pre lista != NULL.
 * @post Carta aggiunta in testa; lunghezza incrementata.
 * @param lista Puntatore alla lista.
 * @param carta La carta da inserire.
 */
void InsertInTesta(ListaCarte* lista, Carta carta);

/**
 * @brief Rimuove una carta dalla lista per indice.
 * @pre lista != NULL; 0 <= indice < lista->lunghezza.
 * @post Carta rimossa; nodo deallocato; lunghezza decrementata.
 * @param lista Puntatore alla lista.
 * @param indice Indice della carta da rimuovere (0-based).
 */
void RemoveAt(ListaCarte* lista, int indice);

/**
 * @brief Rimuove un nodo specifico dalla lista.
 * @pre lista != NULL; nodo appartiene alla lista.
 * @post Nodo rimosso; lunghezza decrementata.
 * @param lista Puntatore alla lista.
 * @param nodo Puntatore al nodo da rimuovere.
 */
void RemoveNode(ListaCarte* lista, NodoCarta* nodo);

/**
 * @brief Restituisce il puntatore al nodo all'indice specificato.
 * @pre lista != NULL; 0 <= indice < lista->lunghezza.
 * @return Puntatore al NodoCarta, o NULL se fuori range.
 * @param lista Puntatore alla lista.
 * @param indice Indice del nodo da cercare (0-based).
 */
NodoCarta* GetAt(ListaCarte* lista, int indice);

/**
 * @brief Elimina tutta la lista e libera la memoria.
 * @pre lista != NULL.
 * @post Tutti i nodi e la struttura lista liberati.
 * @param lista Puntatore alla lista da deallocare.
 */
void EliminaLista(ListaCarte* lista);

/**
 * @brief Stampa il contenuto della lista su stdout (debug).
 * @pre lista != NULL.
 * @post Contenuto della lista stampato su console.
 * @param lista Puntatore alla lista da stampare.
 */
void StampaLista(ListaCarte* lista);

#endif /* LIST_H */