/**
 * @file list.h
 * @brief ADT Lista Doppiamente Concatenata per gestione mano giocatori.
 * @defgroup list Lista (Lista Concatenata)
 * @brief Definisce l'ADT per la lista doppiamente concatenata usata per la
 * mano di ciascun giocatore e le relative operazioni.
 *
 * PRINCIPIO DI INFORMATION HIDING:
 * I tipi `NodoCarta` e `ListaCarte` sono dichiarati come tipi OPACHI:
 * la loro rappresentazione interna (&eacute; definita esclusivamente nei
 * moduli di implementazione (src/data_structures/list_private.h).
 * Gli utilizzatori interagiscono solo tramite le funzioni pubbliche.
 *
 * Ogni nodo contiene una carta e puntatori al nodo successivo e precedente,
 * permettendo inserimenti/eliminazioni in O(1) in qualsiasi posizione.
 *
 * Riferimento: Documentazione CdS - "NodoMano", "Lista (Lista Concatenata)"
 */
#ifndef DATA_STRUCTURES_LIST_H
#define DATA_STRUCTURES_LIST_H

#include "data_structures.h"

/* ============================================================
 *  TIPI OPACHI - Lista DOPPIAMENTE concatenata
 *  Riferimento: Documentazione CdS - "NodoMano"
 *  La rappresentazione interna NON e' visibile agli utilizzatori:
 *  i dettagli implementativi sono in src/data_structures/list_private.h.
 * ============================================================ */

/** @brief Nodo della lista (tipo opaco: rappresentazione interna nascosta). */
typedef struct NodoCarta NodoCarta;

/** @brief Alias per NodoCarta (usato nella documentazione CdS come NodoMano). */
typedef NodoCarta NodoMano;

/**
 * @brief Lista di carte (tipo opaco: rappresentazione interna nascosta).
 *
 * La lista doppiamente concatenata mantiene puntatori a testa e coda per
 * operazioni O(1) e la lunghezza corrente per contare rapidamente le carte
 * in mano. Gli utilizzatori accedono ai dati solo tramite l'API pubblica.
 */
typedef struct ListaCarte ListaCarte;

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

/* ============================================================
 *  ACCESSOR ADT (Information Hiding)
 *  Operazioni che permettono agli utilizzatori di lavorare sulla
 *  lista SENZA accedere alla rappresentazione interna (tipi opachi).
 * ============================================================ */

/**
 * @brief Restituisce il numero di carte nella lista.
 * @pre lista != NULL.
 * @return Numero di nodi (carte) nella lista, 0 se NULL.
 * @param lista Puntatore alla lista.
 */
int Lista_Lunghezza(const ListaCarte* lista);

/**
 * @brief Restituisce per VALORE la carta all'indice specificato.
 *
 * Il valore viene letto senza esporre il puntatore al nodo interno,
 * preservando l'information hiding dell'ADT.
 *
 * @pre lista != NULL; 0 <= indice < Lista_Lunghezza(lista).
 * @return Copia della Carta all'indice, o Carta vuota ({0}) se fuori range.
 * @param lista Puntatore alla lista.
 * @param indice Indice della carta (0-based).
 */
Carta Lista_GetCarta(const ListaCarte* lista, int indice);

/**
 * @brief Sostituisce per VALORE la carta all'indice specificato.
 *
 * Permette di aggiornare i campi di una carta (area, posizione, ecc.)
 * senza esporre il puntatore al nodo interno.
 *
 * @pre lista != NULL; 0 <= indice < Lista_Lunghezza(lista).
 * @post Carta all'indice sostituita con la copia fornita.
 * @param lista Puntatore alla lista.
 * @param indice Indice della carta da aggiornare (0-based).
 * @param c Nuova carta (copia inserita al posto della precedente).
 */
void Lista_ImpostaCarta(ListaCarte* lista, int indice, Carta c);

/**
 * @brief Copia tutte le carte della lista in un array esterno.
 *
 * Utile per letture collettive (rendering, serializzazione) senza
 * esporre i nodi interni.
 *
 * @pre lista != NULL; out != NULL; max > 0.
 * @post Le prime min(Lista_Lunghezza(lista), max) carte copiate in out.
 * @return Numero di carte effettivamente copiate.
 * @param lista Puntatore alla lista.
 * @param out Array di destinazione (Carta).
 * @param max Dimensione massima dell'array out.
 */
int Lista_CopiaCarte(const ListaCarte* lista, Carta* out, int max);

/**
 * @brief Sposta la prima carta della lista in fondo (rotazione).
 *
 * Equivale visivamente a "metti in fondo alla mano la carta pescata".
 * Operazione O(1) sui puntatori interni (nessun spostamento dei dati).
 *
 * @pre lista != NULL.
 * @post La prima carta e' ora l'ultima; lunghezza invariata.
 * @param lista Puntatore alla lista.
 */
void Lista_MettiPrimaInFondo(ListaCarte* lista);

#endif // DATA_STRUCTURES_LIST_H
