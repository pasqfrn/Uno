/**
 * @file deck.c
 * @brief Gestione mazzo di pesca e operazioni sulle carte.
 * @ingroup game_logic
 *
 * Modulo specializzato per la creazione, mescolamento, rimescolamento
 * e pesca delle carte UNO. Utility per accesso alle mani dei giocatori.
 *
 * ADT utilizzati: Pila (mazzo_pila, scarti_pila), ListaCarte (mani).
 */
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../lib/game_logic.h"
#include "../../lib/data_structures.h"
#include "../../lib/data_structures/stack.h"
#include "../../lib/list.h"

/* ============================================================
 *  FUNZIONI HELPER PRIVATE - Conversione Pila <-> Array
 * ============================================================ */

/**
 * @brief Svuota una pila e ne copia il contenuto nell'array mazzo[].
 * @ingroup game_logic
 * @pre gioco != NULL; pila != NULL.
 * @post Array mazzo[] popolato; pila ricostruita.
 * @param gioco Stato di gioco.
 * @param pila Pila da convertire.
 */
static void PilaToMazzoArray(StatoGioco* gioco, Pila* pila) {
    Carta tmp[108];
    int n = 0;
    Carta c;
    while (Pila_Pop(pila, &c) && n < 108) {
        tmp[n++] = c;
    }
    gioco->num_carte_mazzo = 0;
    for (int i = n - 1; i >= 0; i--) {
        gioco->mazzo[gioco->num_carte_mazzo++] = tmp[i];
        Pila_Push(pila, tmp[i]);
    }
}

/**
 * @brief Versione alternativa di PilaToMazzoArray (compatibilita' legacy).
 * @ingroup game_logic
 * @deprecated Usare PilaToMazzoArray().
 */
static void MettiMazzoInArray(StatoGioco* gioco, Pila* pila) {
    Carta tmp[108];
    int n = 0;
    Carta c;
    while (Pila_Pop(pila, &c) && n < 108) tmp[n++] = c;
    gioco->num_carte_mazzo = 0;
    for (int i = n - 1; i >= 0; i--) {
        gioco->mazzo[gioco->num_carte_mazzo++] = tmp[i];
        Pila_Push(pila, tmp[i]);
    }
}

/* ============================================================
 *  INIZIALIZZAZIONE MAZZO
 * ============================================================ */

/**
 * @brief Inizializza il mazzo di 108 carte UNO standard e le mescola (Fisher-Yates).
 * @ingroup game_logic
 * @pre gioco != NULL.
 * @post Mazzo creato, mescolato, caricato in mazzo_pila.
 * @param gioco Stato di gioco.
 */
void InizializzaMazzo(StatoGioco *gioco) {
    int indice = 0;
    if (mazzo_pila) { Pila_Distruggi(mazzo_pila); mazzo_pila = NULL; }
    if (scarti_pila) { Pila_Distruggi(scarti_pila); scarti_pila = NULL; }
    mazzo_pila = Pila_Crea();
    scarti_pila = Pila_Crea();
    for (int c = ROSSO; c <= BLU; c++) {
        gioco->mazzo[indice++] = (Carta){c, NUM_0, {0}, {0}, 0};
        for (int t = NUM_1; t <= PESCA_DUE; t++) {
            gioco->mazzo[indice++] = (Carta){c, t, {0}, {0}, 0};
            gioco->mazzo[indice++] = (Carta){c, t, {0}, {0}, 0};
        }
    }
    for (int i = 0; i < 4; i++) {
        gioco->mazzo[indice++] = (Carta){NERO, CAMBIO_COLORE, {0}, {0}, 0};
        gioco->mazzo[indice++] = (Carta){NERO, PESCA_QUATTRO, {0}, {0}, 0};
    }
    gioco->num_carte_mazzo = indice;
    gioco->num_carte_scarti = 0;
    for (int i = gioco->num_carte_mazzo - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Carta temp = gioco->mazzo[i];
        gioco->mazzo[i] = gioco->mazzo[j];
        gioco->mazzo[j] = temp;
    }
    for (int i = 0; i < gioco->num_carte_mazzo; i++) {
        Pila_Push(mazzo_pila, gioco->mazzo[i]);
    }
}

/* ============================================================
 *  RIMESCOLAMENTO SCARTI NEL MAZZO
 * ============================================================ */

/**
 * @brief Ricarica il mazzo dalla pila scarti (mazzo esaurito).
 * @ingroup game_logic
 * @pre gioco != NULL; num_carte_scarti > 1.
 * @post Mazzo ricaricato e rimescolato; notifica mostrata.
 * @param gioco Stato di gioco.
 */
void RimescolaScartiNelMazzo(StatoGioco *gioco) {
    if (gioco->num_carte_scarti <= 1) return;
    Carta ultimaCarta = gioco->scarti[gioco->num_carte_scarti - 1];
    if (scarti_pila) Pila_Svuota(scarti_pila);
    for (int i = 0; i < gioco->num_carte_scarti - 1; i++) {
        gioco->mazzo[gioco->num_carte_mazzo++] = gioco->scarti[i];
        Pila_Push(mazzo_pila, gioco->scarti[i]);
    }
    gioco->scarti[0] = ultimaCarta;
    gioco->num_carte_scarti = 1;
    for (int i = gioco->num_carte_mazzo - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Carta temp = gioco->mazzo[i];
        gioco->mazzo[i] = gioco->mazzo[j];
        gioco->mazzo[j] = temp;
    }
    Pila_Svuota(mazzo_pila);
    for (int i = gioco->num_carte_mazzo - 1; i >= 0; i--) {
        Pila_Push(mazzo_pila, gioco->mazzo[i]);
    }
    MostraNotifica(gioco, "Mazzo esaurito: scarti rimescolati!");
}

/* ============================================================
 *  PESCA CARTA
 * ============================================================ */

/**
 * @brief Fa pescare un numero di carte a un giocatore.
 * @ingroup game_logic
 * @pre gioco != NULL; id_giocatore valido; quantita > 0.
 * @post Carte pescate nella mano del giocatore.
 * @param gioco Stato di gioco.
 * @param id_giocatore Indice giocatore.
 * @param quantita Numero carte.
 */
void Pesca(StatoGioco *gioco, int id_giocatore, int quantita) {
    for (int i = 0; i < quantita; i++) {
        if (gioco->num_carte_mazzo == 0) RimescolaScartiNelMazzo(gioco);
        if (gioco->num_carte_mazzo > 0) {
            Giocatore *g = &gioco->giocatori[id_giocatore];
            if (!g->mano) g->mano = CreaLista();
            Carta pescata_da_pila;
            if (Pila_Pop(mazzo_pila, &pescata_da_pila) &&
                pescata_da_pila.tipo == gioco->mazzo[gioco->num_carte_mazzo - 1].tipo) {
                gioco->num_carte_mazzo--;
                InsertInCoda(g->mano, gioco->mazzo[gioco->num_carte_mazzo]);
            } else {
                gioco->num_carte_mazzo--;
                InsertInCoda(g->mano, gioco->mazzo[gioco->num_carte_mazzo]);
            }
            g->num_carte_mano = g->mano->lunghezza;
        }
    }
}

/* ============================================================
 *  UTILITY PUBBLICHE - Accesso alle mani dei giocatori (Liste)
 * ============================================================ */

/**
 * @brief Restituisce il numero di carte nella mano di un giocatore.
 * @ingroup game_logic
 * @pre g != NULL.
 * @return Numero di carte.
 * @param g Giocatore.
 */
int GetLunghezzaMano(Giocatore *g) {
    if (!g) return 0;
    if (!g->mano) return g->num_carte_mano;
    int len = g->mano->lunghezza;
    if (len > 0) g->num_carte_mano = len;
    return len > 0 ? len : g->num_carte_mano;
}

/**
 * @brief Restituisce il nodo carta per indice.
 * @ingroup game_logic
 * @pre g != NULL; indice valido.
 * @return NodoCarta, o NULL.
 * @param g Giocatore.
 * @param indice Indice (0-based).
 */
NodoCarta* GetNodoCartaGiocatore(Giocatore *g, int indice) {
    return (g && g->mano) ? GetAt(g->mano, indice) : NULL;
}

/**
 * @brief Restituisce una carta dalla mano per indice.
 * @ingroup game_logic
 * @pre g != NULL; indice valido.
 * @return Carta, o carta vuota.
 * @param g Giocatore.
 * @param indice Indice (0-based).
 */
Carta GetCartaGiocatore(Giocatore *g, int indice) {
    NodoCarta *n = GetNodoCartaGiocatore(g, indice);
    return n ? n->carta : (Carta){0};
}

/**
 * @brief Rimuove una carta dalla mano per indice.
 * @ingroup game_logic
 * @pre g != NULL; indice valido.
 * @post Carta rimossa; num_carte_mano aggiornato.
 * @param g Giocatore.
 * @param indice Indice (0-based).
 */
void RimuoviCartaGiocatore(Giocatore *g, int indice) {
    if (g && g->mano) {
        RemoveAt(g->mano, indice);
        g->num_carte_mano = g->mano->lunghezza;
    }
}