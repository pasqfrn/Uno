/**
 * @file list.h
 * @brief ADT Lista Doppiamente Concatenata per gestione mano giocatori.
 * @defgroup list Lista (Lista Concatenata)
 * @brief Definisce NodoCarta e ListaCarte, le strutture per la lista
 * doppiamente concatenata usata per la mano di ciascun giocatore, e le
 * relative operazioni.
 *
 * Ogni nodo contiene una carta e puntatori al nodo successivo e precedente,
 * permettendo inserimenti/eliminazioni in O(1) in qualsiasi posizione.
 *
 * Riferimento: Documentazione CdS - "NodoMano", "Lista (Lista Concatenata)"
 */
#ifndef DATA_STRUCTURES_LIST_H
#define DATA_STRUCTURES_LIST_H

#include "../data_structures.h"

/* ============================================================
 *  NODO CARTA - Lista DOPPIAMENTE concatenata
 *  Riferimento: Documentazione CdS - "NodoMano"
 * ============================================================ */

/**
 * @brief Nodo per la lista doppiamente concatenata della mano del giocatore.
 *
 * Ogni nodo contiene una carta e due puntatori: uno al nodo successivo
 * (forward link) e uno al nodo precedente (backward link).
 * Questo permette inserimenti ed eliminazioni efficienti in qualsiasi posizione.
 */
typedef struct NodoCarta {
    Carta carta;                    /* La carta contenuta nel nodo */
    struct NodoCarta* prossimo;     /* Puntatore al nodo successivo (next) */
    struct NodoCarta* prev;         /* Puntatore al nodo precedente (prev) */
} NodoCarta;

/** @brief Alias per NodoCarta (usato nella documentazione CdS come NodoMano). */
typedef NodoCarta NodoMano;

/**
 * @brief Struttura container per la lista di carte (mano del giocatore).
 *
 * Mantiene i puntatori a testa e coda per operazioni O(1) e la lunghezza
 * corrente per contare rapidamente le carte in mano.
 */
typedef struct {
    NodoCarta* testa;       /* Primo nodo della lista */
    NodoCarta* coda;        /* Ultimo nodo della lista */
    int lunghezza;          /* Numero totale di carte nella lista */
} ListaCarte;

/**
 * @brief Inserisce una carta dopo un nodo specifico (o in testa se nodo==NULL).
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
 * @param lista Lista (non usato).
 * @param nodo Nodo di riferimento.
 * @return Nodo precedente, o NULL.
 */
NodoCarta* Lista_Precedente(ListaCarte* lista, NodoCarta* nodo);

/**
 * @brief Restituisce l'ultimo nodo della lista (coda).
 * @pre lista != NULL.
 * @return Puntatore alla coda, o NULL.
 * @param lista Lista.
 */
NodoCarta* Lista_Ultimo(ListaCarte* lista);

/**
 * @brief Rimuove un nodo dalla lista gestendo correttamente prev/next.
 * @pre lista != NULL; nodo appartiene alla lista.
 * @post Nodo rimosso; testa/coda aggiornati; lunghezza decrementata.
 * @param lista Lista.
 * @param nodo Nodo da rimuovere.
 */
void Lista_RimuoviNodoBidirezionale(ListaCarte* lista, NodoCarta* nodo);

/**
 * @brief Verifica l'integrita' dei puntatori prev/next (debug).
 * @param lista Lista.
 * @return 1 se valida, 0 se inconsistente.
 */
int Lista_VerificaBidirezionale(ListaCarte* lista);

#endif // DATA_STRUCTURES_LIST_H