/**
 * @file bst.c
 * @brief Implementazione Albero Binario di Ricerca (BST) per ricerca utenti.
 * @ingroup bst
 *
 * ADT: BST con chiave = email utente (case-insensitive).
 * Operazioni: inserzione, ricerca, rimozione O(log n) medio.
 * Usato in auth.c come indice parallelo a dbUtenti per ricerca O(log n).
 */
#include <stdlib.h>
#include <string.h>
#include "../../lib/data_structures.h"
#include "../../lib/data_structures/bst.h"

/* ============================================================
 *  ALBERO BINARIO DI RICERCA - Implementazione
 * ============================================================ */

/** @brief Confronto case-insensitive: a < b. */
static int email_lt(const char* a, const char* b) {
    return strcasecmp(a, b) < 0;
}

/** @brief Confronto case-insensitive: a == b. */
static int email_eq(const char* a, const char* b) {
    return strcasecmp(a, b) == 0;
}

/**
 * @brief Crea un nuovo nodo BST.
 * @ingroup bst
 * @pre u.email valida.
 * @post Nodo allocato e inizializzato.
 * @param u Utente da inserire.
 * @return Puntatore al nodo, o NULL.
 */
NodoAlbero* BST_CreaNodo(Utente u) {
    NodoAlbero* n = (NodoAlbero*)malloc(sizeof(NodoAlbero));
    if (!n) return NULL;
    n->utente = u;
    n->left = NULL;
    n->right = NULL;
    return n;
}

/**
 * @brief Inserisce un utente nell'albero.
 * @ingroup bst
 * @pre radice valida; u.email non duplicata.
 * @post Nuovo nodo inserito se non duplicato.
 * @param radice Radice corrente.
 * @param u Utente da inserire.
 * @param duplicato Flag duplicato (1 se email gia' presente).
 * @return Nuova radice.
 */
NodoAlbero* BST_Inserisci(NodoAlbero* radice, Utente u, int* duplicato) {
    if (duplicato) *duplicato = 0;
    if (!radice) return BST_CreaNodo(u);

    if (email_lt(u.email, radice->utente.email)) {
        radice->left = BST_Inserisci(radice->left, u, duplicato);
    } else if (email_lt(radice->utente.email, u.email)) {
        radice->right = BST_Inserisci(radice->right, u, duplicato);
    } else {
        if (duplicato) *duplicato = 1;   // email già presente
    }
    return radice;
}

/**
 * @brief Ricerca un utente per email.
 * @ingroup bst
 * @pre radice valida; email != NULL.
 * @return Nodo trovato o NULL.
 * @param radice Radice.
 * @param email Email da cercare.
 */
NodoAlbero* BST_RicercaEmail(NodoAlbero* radice, const char* email) {
    if (!radice || !email) return NULL;
    if (email_lt(email, radice->utente.email))
        return BST_RicercaEmail(radice->left, email);
    if (email_lt(radice->utente.email, email))
        return BST_RicercaEmail(radice->right, email);
    if (email_eq(email, radice->utente.email)) return radice;
    return NULL;
}

/** @brief Trova il nodo con email minima (piu' a sinistra). */
static NodoAlbero* BST_Minimo(NodoAlbero* n) {
    while (n && n->left) n = n->left;
    return n;
}

/**
 * @brief Rimuove il nodo con l'email specificata.
 * @ingroup bst
 * @pre radice valida; email esistente.
 * @post Nodo rimosso; albero aggiornato.
 * @param radice Radice.
 * @param email Email da rimuovere.
 * @return Nuova radice.
 */
NodoAlbero* BST_RimuoviEmail(NodoAlbero* radice, const char* email) {
    if (!radice || !email) return radice;
    if (email_lt(email, radice->utente.email)) {
        radice->left = BST_RimuoviEmail(radice->left, email);
    } else if (email_lt(radice->utente.email, email)) {
        radice->right = BST_RimuoviEmail(radice->right, email);
    } else {
        // Trovato
        if (!radice->left) {
            NodoAlbero* tmp = radice->right;
            free(radice);
            return tmp;
        } else if (!radice->right) {
            NodoAlbero* tmp = radice->left;
            free(radice);
            return tmp;
        } else {
            NodoAlbero* succ = BST_Minimo(radice->right);
            radice->utente = succ->utente;
            radice->right = BST_RimuoviEmail(radice->right, succ->utente.email);
        }
    }
    return radice;
}

/**
 * @brief Distrugge ricorsivamente l'albero.
 * @ingroup bst
 * @pre radice valida o NULL.
 * @post Memoria liberata.
 * @param radice Radice.
 */
void BST_Distruggi(NodoAlbero* radice) {
    if (!radice) return;
    BST_Distruggi(radice->left);
    BST_Distruggi(radice->right);
    free(radice);
}

/** @brief Visita in-order ricorsiva (helper). */
static void BST_InordinaRec(NodoAlbero* n, Utente* out, int max, int* idx) {
    if (!n || *idx >= max) return;
    BST_InordinaRec(n->left, out, max, idx);
    if (*idx < max) {
        out[(*idx)++] = n->utente;
    }
    BST_InordinaRec(n->right, out, max, idx);
}

/**
 * @brief Visita in-order: copia utenti in array ordinato.
 * @ingroup bst
 * @pre radice valida; out != NULL; max > 0.
 * @return Numero di elementi scritti.
 * @param radice Radice.
 * @param out Array output.
 * @param max Dimensione massima.
 */
int BST_Inordina(NodoAlbero* radice, Utente* out, int max) {
    int idx = 0;
    BST_InordinaRec(radice, out, max, &idx);
    return idx;
}

/**
 * @brief Restituisce l'altezza dell'albero.
 * @ingroup bst
 * @return Altezza (0 se vuoto).
 * @param radice Radice.
 */
int BST_Altezza(NodoAlbero* radice) {
    if (!radice) return 0;
    int hl = BST_Altezza(radice->left);
    int hr = BST_Altezza(radice->right);
    return 1 + (hl > hr ? hl : hr);
}

/**
 * @brief Restituisce il numero di nodi.
 * @ingroup bst
 * @return Numero di nodi.
 * @param radice Radice.
 */
int BST_ContaNodi(NodoAlbero* radice) {
    if (!radice) return 0;
    return 1 + BST_ContaNodi(radice->left) + BST_ContaNodi(radice->right);
}
