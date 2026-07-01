/**
 * @file list.h
 * @brief Interfaccia ADT ListaDoppia (lista doppiamente concatenata).
 * @ingroup list
 *
 * Modulo specializzato per le liste bidirezionali utilizzato per
 * la gestione della mano dei giocatori.
 *
 * NodoCarta e' dotato dei campi prossimo/next e prev per supportare
 * la navigazione bidirezionale.
 *
 * Funzioni specifiche rispetto a list.h generale:
 *   - Lista_InserisciDopo: inserimento dopo nodo specifico
 *   - Lista_Precedente: accesso O(1) al nodo precedente
 *   - Lista_Ultimo: accesso O(1) all'ultimo nodo
 *   - Lista_RimuoviNodoBidirezionale: rimozione sicura con aggiornamento prev/next
 *   - Lista_VerificaBidirezionale: verifica integrita' puntatori (debug)
 */
#ifndef DATA_STRUCTURES_LIST_H
#define DATA_STRUCTURES_LIST_H

#include "../data_structures.h"

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
NodoCarta* Lista_InserisciDopo(ListaCarte* lista, NodoCarta* nodo, Carta c);

/**
 * @brief Restituisce il nodo precedente a quello dato.
 * @ingroup list
 * @param lista Lista (non usato).
 * @param nodo Nodo di riferimento.
 * @return Nodo precedente, o NULL.
 */
NodoCarta* Lista_Precedente(ListaCarte* lista, NodoCarta* nodo);

/**
 * @brief Restituisce l'ultimo nodo della lista (coda).
 * @ingroup list
 * @pre lista != NULL.
 * @return Puntatore alla coda, o NULL.
 * @param lista Lista.
 */
NodoCarta* Lista_Ultimo(ListaCarte* lista);

/**
 * @brief Rimuove un nodo dalla lista gestendo correttamente prev/next.
 * @ingroup list
 * @pre lista != NULL; nodo appartiene alla lista.
 * @post Nodo rimosso; testa/coda aggiornati; lunghezza decrementata.
 * @param lista Lista.
 * @param nodo Nodo da rimuovere.
 */
void Lista_RimuoviNodoBidirezionale(ListaCarte* lista, NodoCarta* nodo);

/**
 * @brief Verifica l'integrita' dei puntatori prev/next (debug).
 * @ingroup list
 * @param lista Lista.
 * @return 1 se valida, 0 se inconsistente.
 */
int Lista_VerificaBidirezionale(ListaCarte* lista);

#endif // DATA_STRUCTURES_LIST_H