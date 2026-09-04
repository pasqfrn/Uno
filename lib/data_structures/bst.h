/**
 * @file bst.h
 * @brief Albero Binario di Ricerca (BST) per ricerca utenti per email O(log n).
 * @defgroup bst BST (Albero Binario Ricerca)
 * @brief Implementa un BST con chiave email per ricerca, inserimento e cancellazione
 * in O(log n) medio degli utenti nel database.
 *
 * PRINCIPIO DI INFORMATION HIDING:
 * Il tipo `NodoAlbero` e' dichiarato come tipo OPACO: la sua rappresentazione
 * interna e' definita esclusivamente nel modulo di implementazione
 * (src/data_structures/bst.c). Gli utilizzatori interagiscono solo tramite
 * le funzioni pubbliche di questo header.
 */
#ifndef DATA_STRUCTURES_BST_H
#define DATA_STRUCTURES_BST_H

#include "data_structures.h"

/**
 * @addtogroup bst_nodi Nodi BST
 * @brief Tipo opaco del nodo dell'albero binario di ricerca.
 * @{
 */
/**
 * @brief Nodo dell'albero BST (tipo opaco: rappresentazione interna nascosta).
 *
 * Contiene un Utente (chiave = email) e i puntatori ai figli; la struttura
 * completa e' definita solo nel modulo di implementazione.
 */
typedef struct NodoAlbero NodoAlbero;
/** @} */

/**
 * @addtogroup bst_operazioni Operazioni BST
 * @brief Funzioni di gestione dell'albero binario di ricerca.
 * @{
 */
/**
 * @brief Crea un nuovo nodo BST a partire da un Utente.
 * @pre u.email valida.
 * @post Nodo allocato e inizializzato, o NULL se out-of-memory.
 * @param u Utente da inserire nel nodo.
 * @return Puntatore al nodo creato, o NULL.
 */
NodoAlbero* BST_CreaNodo(Utente u);

/**
 * @brief Inserisce un utente nell'albero.
 * @pre radice valida (o NULL); u.email non duplicata.
 * @post Albero aggiornato; nuovo nodo inserito se non duplicato.
 * @param radice Radice corrente dell'albero.
 * @param u Utente da inserire.
 * @param duplicato Flag impostato a 1 se email duplicata.
 * @return Nuova radice dell'albero.
 */
NodoAlbero* BST_Inserisci(NodoAlbero* radice, Utente u, int* duplicato);

/**
 * @brief Ricerca un utente per email.
 * @pre radice valida; email != NULL.
 * @post Restituisce nodo trovato o NULL.
 * @param radice Radice dell'albero.
 * @param email Email da cercare.
 * @return Puntatore al NodoAlbero trovato, o NULL.
 */
NodoAlbero* BST_RicercaEmail(NodoAlbero* radice, const char* email);

/**
 * @brief Rimuove il nodo con l'email specificata.
 * @pre radice valida; email esistente nell'albero.
 * @post Nodo rimosso; albero bilanciato.
 * @param radice Radice corrente.
 * @param email Email del nodo da rimuovere.
 * @return Nuova radice dell'albero.
 */
NodoAlbero* BST_RimuoviEmail(NodoAlbero* radice, const char* email);

/**
 * @brief Libera ricorsivamente tutta la memoria dell'albero.
 * @pre radice valida o NULL.
 * @post Tutti i nodi deallocati.
 * @param radice Radice dell'albero da distruggere.
 */
void BST_Distruggi(NodoAlbero* radice);

/**
 * @brief Visita in-order e copia i puntatori agli Utenti in un array.
 * @pre radice valida; out != NULL; max > 0.
 * @post Array out popolato con i nodi in ordine; restituisce il conteggio.
 * @param radice Radice dell'albero.
 * @param out Array di Utente da riempire.
 * @param max Dimensione massima di out.
 * @return Numero di elementi scritti.
 */
int BST_Inordina(NodoAlbero* radice, Utente* out, int max);

/**
 * @brief Restituisce l'altezza dell'albero.
 * @pre radice valida o NULL.
 * @return Altezza (0 se vuoto).
 */
int BST_Altezza(NodoAlbero* radice);

/**
 * @brief Restituisce il numero di nodi nell'albero.
 * @pre radice valida o NULL.
 * @return Numero di nodi.
 */
int BST_ContaNodi(NodoAlbero* radice);

/**
 * @brief Restituisce l'email dell'utente contenuto nel nodo (accessor).
 *
 * Accessor informativo che preserva l'information hiding: evita che gli
 * utilizzatori dereferenzino il NodoAlbero opaco.
 *
 * @pre nodo != NULL.
 * @return Puntatore alla stringa email memorizzata nel nodo, o NULL.
 * @param nodo Nodo dell'albero.
 */
const char* BST_GetEmail(const NodoAlbero* nodo);
/** @} */

#endif // DATA_STRUCTURES_BST_H
