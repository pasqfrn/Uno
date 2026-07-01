/**
 * @file game_logic.c
 * @brief Implementazione logica di gioco UNO: regole, effetti carte, validazione mosse e gestione ADT.
 * @ingroup game_logic
 *
 * Contiene:
 *   - MossaValida, ApplicaEffetto (validazione/applicazione effetti carte)
 *   - MostraNotifica
 *   - Variabili ADT globali (mazzo_pila, scarti_pila, coda_turni)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "../lib/game_logic.h"
#include "../lib/auth.h"
#include "../lib/list.h"
#include "../lib/data_structures.h"
#include "../lib/data_structures/stack.h"
#include "../lib/data_structures/queue.h"
#include "../lib/socket/network/network.h"
#include "../lib/socket/lan/lan_sync.h"

/** @brief Pila globale del mazzo di pesca. */
Pila* mazzo_pila = NULL;
/** @brief Pila globale degli scarti. */
Pila* scarti_pila = NULL;
/** @brief Coda globale dei turni. */
CodaTurni* coda_turni = NULL;

/* ============================================================
 *  VALIDAZIONE E APPLICAZIONE MOSSE
 * ============================================================ */

/**
 * @brief Verifica se una carta e' legalmente giocabile.
 *
 * Regole UNO:
 *   - Jolly (cambiocolore/PescaQuattro): sempre validi
 *   - Altrimenti: match colore attivo OPPURE match tipo/valore
 *
 * @pre gioco != NULL; gioco->num_carte_scarti > 0.
 * @post Restituisce 1 se corrispondenza colore/tipo o carta nera.
 * @param gioco Stato di gioco corrente.
 * @param c Carta da giocare.
 * @return 1 se mossa valida, 0 altrimenti.
 */
int MossaValida(StatoGioco *gioco, Carta c) {
    if (c.colore == NERO) return 1;
    Carta in_cima = gioco->scarti[gioco->num_carte_scarti - 1];
    return (c.colore == gioco->colore_attivo || c.tipo == in_cima.tipo);
}

/**
 * @brief Applica l'effetto di una carta giocata allo stato di gioco.
 *
 * Gestisce: cambio colore, cambio giro, salta turno, pesca carte,
 * aggiornamento colore attivo e rotazione coda turni.
 *
 * @pre gioco != NULL; carta valida.
 * @post Stato aggiornato con effetto applicato; notifica mostrata.
 * @param gioco Stato di gioco.
 * @param c Carta giocata.
 * @param colore_scelto Colore scelto dal giocatore (per jolly).
 */
void ApplicaEffetto(StatoGioco *gioco, Carta c, Colore colore_scelto) {
    gioco->colore_attivo = (c.colore == NERO) ? colore_scelto : c.colore;
    int prossimo = (gioco->turno_corrente + gioco->direzione + gioco->num_giocatori) % gioco->num_giocatori;
    if (coda_turni) {
        if (c.tipo == CAMBIA_GIRO) {
            Coda_InvertiVerso(coda_turni);
        }
    }
    if (c.tipo == SALTA) {
        gioco->turno_corrente = (prossimo + gioco->direzione + gioco->num_giocatori) % gioco->num_giocatori;
        MostraNotifica(gioco, TextFormat("%s salta il turno!", Giocatore_Nome(&gioco->giocatori[prossimo])));
    }
    else if (c.tipo == CAMBIA_GIRO) {
        gioco->direzione *= -1;
        gioco->turno_corrente = (gioco->turno_corrente + gioco->direzione + gioco->num_giocatori) % gioco->num_giocatori;
        MostraNotifica(gioco, "Giro invertito!");
    }
    else if (c.tipo == PESCA_DUE) {
        Pesca(gioco, prossimo, 2);
        gioco->turno_corrente = (prossimo + gioco->direzione + gioco->num_giocatori) % gioco->num_giocatori;
        MostraNotifica(gioco, TextFormat("%s subisce +2!", Giocatore_Nome(&gioco->giocatori[prossimo])));
    }
    else if (c.tipo == PESCA_QUATTRO) {
        Pesca(gioco, prossimo, 4);
        gioco->turno_corrente = (prossimo + gioco->direzione + gioco->num_giocatori) % gioco->num_giocatori;
        MostraNotifica(gioco, TextFormat("%s subisce +4!", Giocatore_Nome(&gioco->giocatori[prossimo])));
    }
    else if (c.tipo == CAMBIO_COLORE) {
        MostraNotifica(gioco, "Cambio colore!");
        gioco->turno_corrente = prossimo;
    }
    else {
        gioco->turno_corrente = prossimo;
    }
}

/**
 * @brief Mostra una notifica temporanea a schermo.
 *
 * Sostituisce eventuale notifica precedente. Si auto-cancella dopo
 * un certo tempo gestito dal loop di gioco.
 *
 * @pre gioco != NULL; msg != NULL.
 * @post Notifica visibile per 3 secondi.
 * @param gioco Stato di gioco.
 * @param msg Testo della notifica.
 */
void MostraNotifica(StatoGioco *gioco, const char* msg) {
    strcpy(gioco->notifica, msg);
    gioco->timer_notifica = 3.0f;
}
