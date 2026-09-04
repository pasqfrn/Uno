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
#include "../../lib/game/game_logic.h"
#include "../../lib/auth/auth.h"
#include "../../lib/data_structures/list.h"
#include "../../lib/data_structures/data_structures.h"
#include "../../lib/data_structures/stack.h"
#include "../../lib/data_structures/queue.h"
#include "../../lib/socket/network/network.h"
#include "../../lib/socket/lan/lan_sync.h"

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

    /* Nota: l'avanzamento del turno e' gestito dalla funzione di dominio
     * GameLogic_AvanzamentoTurno, che mantiene sincronizzato l'ADT
     * CodaTurni (rotazione O(1)) con l'indice turno_corrente. */
    if (c.tipo == CAMBIA_GIRO) {
        if (coda_turni) Coda_InvertiVerso(coda_turni);
        gioco->direzione *= -1;
        GameLogic_AvanzamentoTurno(gioco, 1);
        MostraNotifica(gioco, "Giro invertito!");
    }
    else if (c.tipo == SALTA) {
        GameLogic_AvanzamentoTurno(gioco, 2);
        MostraNotifica(gioco, TextFormat("%s salta il turno!", Giocatore_Nome(&gioco->giocatori[prossimo])));
    }
    else if (c.tipo == PESCA_DUE) {
        Pesca(gioco, prossimo, 2);
        GameLogic_AvanzamentoTurno(gioco, 2);
        MostraNotifica(gioco, TextFormat("%s subisce +2!", Giocatore_Nome(&gioco->giocatori[prossimo])));
    }
    else if (c.tipo == PESCA_QUATTRO) {
        Pesca(gioco, prossimo, 4);
        GameLogic_AvanzamentoTurno(gioco, 2);
        MostraNotifica(gioco, TextFormat("%s subisce +4!", Giocatore_Nome(&gioco->giocatori[prossimo])));
    }
    else if (c.tipo == CAMBIO_COLORE) {
        GameLogic_AvanzamentoTurno(gioco, 1);
        MostraNotifica(gioco, "Cambio colore!");
    }
    else {
        GameLogic_AvanzamentoTurno(gioco, 1);
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

/* ============================================================
 *  FUNZIONI DI DOMINIO BASATE SUGLI ADT
 * ============================================================ */

/**
 * @brief (Ri)costruisce la coda dei turni ADT a partire dall'array dei giocatori.
 * @ingroup game_logic
 * @pre gioco != NULL.
 * @post coda_turni ricreata con i giocatori di gioco->giocatori[].
 * @param gioco Puntatore allo stato di gioco.
 * @return 1 se la coda e' stata ricreata con successo, 0 altrimenti.
 */
int GameLogic_SincronizzaCodaTurni(StatoGioco *gioco) {
    if (!gioco) return 0;
    if (coda_turni) { Coda_Distruggi(coda_turni); coda_turni = NULL; }
    coda_turni = Coda_Crea();
    if (!coda_turni) return 0;
    if (gioco->direzione == 0) gioco->direzione = 1;
    for (int i = 0; i < gioco->num_giocatori; i++) {
        Coda_Enqueue(coda_turni, gioco->giocatori[i]);
    }
    return 1;
}

/**
 * @brief Avanza il turno di `passi` posizioni usando l'ADT CodaTurni.
 * @ingroup game_logic
 * @pre gioco != NULL; passi > 0; num_giocatori > 0.
 * @post turno_corrente avanzato di passi; coda_turni ruotata di conseguenza.
 * @param gioco Puntatore allo stato di gioco.
 * @param passi Numero di posizioni da avanzare (1 = turno successivo).
 */
void GameLogic_AvanzamentoTurno(StatoGioco *gioco, int passi) {
    if (!gioco || gioco->num_giocatori <= 0) return;
    if (passi <= 0) passi = 1;
    if (coda_turni) {
        for (int i = 0; i < passi; i++) Coda_Ruota(coda_turni);
    }
    int spostamento = (gioco->direzione * passi) % gioco->num_giocatori;
    gioco->turno_corrente = (gioco->turno_corrente + spostamento + gioco->num_giocatori) % gioco->num_giocatori;
}

/**
 * @brief Copia la mano (ADT ListaCarte) di un giocatore nel backup flat.
 * @ingroup game_logic
 * @pre g != NULL.
 * @post mano_backup[] e num_carte_backup aggiornati con la mano corrente.
 * @param g Puntatore al giocatore.
 * @return Numero di carte copiate nel backup (0 se mano nulla).
 */
int GameLogic_PreparaManoSalvataggio(Giocatore *g) {
    if (!g) return 0;
    g->num_carte_backup = 0;
    if (g->mano) {
        int n = Lista_CopiaCarte(g->mano, g->mano_backup, 108);
        g->num_carte_backup = n;
        g->num_carte_mano = n;
        return n;
    }
    return 0;
}

/**
 * @brief Ricostruisce la mano (ADT ListaCarte) di un giocatore dal backup flat.
 * @ingroup game_logic
 * @pre g != NULL; mano_backup valido.
 * @post g->mano allocata e popolata con mano_backup[].
 * @param g Puntatore al giocatore.
 * @return 1 se la mano e' stata ricostruita, 0 altrimenti.
 */
int GameLogic_RicostruisciMano(Giocatore *g) {
    if (!g) return 0;
    if (g->mano) { EliminaLista(g->mano); g->mano = NULL; }
    g->mano = CreaLista();
    if (!g->mano) return 0;
    for (int j = 0; j < g->num_carte_backup; j++) {
        InsertInCoda(g->mano, g->mano_backup[j]);
    }
    g->num_carte_mano = Lista_Lunghezza(g->mano);
    return 1;
}

/**
 * @brief Sposta la prima carta della mano in fondo (ordine visivo dopo una pescata).
 * @ingroup game_logic
 * @pre g != NULL.
 * @post La prima carta della mano e' ora l'ultima.
 * @param g Puntatore al giocatore.
 */
void GameLogic_MettiPrimaInFondo(Giocatore *g) {
    if (g && g->mano) {
        Lista_MettiPrimaInFondo(g->mano);
        g->num_carte_mano = Lista_Lunghezza(g->mano);
    }
}

/**
 * @brief Sostituisce per valore la carta all'indice nella mano del giocatore.
 * @ingroup game_logic
 * @pre g != NULL; indice valido.
 * @post Carta all'indice aggiornata con la copia fornita.
 * @param g Puntatore al giocatore.
 * @param indice Indice della carta (0-based).
 * @param c Nuova carta da impostare.
 */
void GameLogic_ImpostaCarta(Giocatore *g, int indice, Carta c) {
    if (g && g->mano) {
        Lista_ImpostaCarta(g->mano, indice, c);
    }
}

/**
 * @brief Copia le carte della mano di un giocatore in un array esterno.
 * @ingroup game_logic
 * @pre g != NULL; out != NULL; max > 0.
 * @post out popolato con le carte della mano (max o meno).
 * @param g Puntatore al giocatore.
 * @param out Array di destinazione.
 * @param max Dimensione massima dell'array.
 * @return Numero di carte copiate.
 */
int GameLogic_CopiaMano(const Giocatore *g, Carta* out, int max) {
    if (!g || !g->mano || !out || max <= 0) return 0;
    return Lista_CopiaCarte(g->mano, out, max);
}

/**
 * @brief Distrugge la mano (ADT ListaCarte) di un giocatore.
 * @ingroup game_logic
 * @pre g != NULL.
 * @post g->mano == NULL; num_carte_mano = 0.
 * @param g Puntatore al giocatore.
 */
void GameLogic_DistruggiMano(Giocatore *g) {
    if (!g) return;
    if (g->mano) {
        EliminaLista(g->mano);
        g->mano = NULL;
    }
    g->num_carte_mano = 0;
    g->num_carte_backup = 0;
}

/**
 * @brief Restituisce il numero di carte della mano usando l'API ADT.
 * @ingroup game_logic
 * @pre g != NULL.
 * @return Numero di carte in mano (0 se mano nulla o vuota).
 * @param g Puntatore al giocatore.
 */
int GameLogic_GetLunghezzaMano(const Giocatore *g) {
    if (!g) return 0;
    if (g->mano) return Lista_Lunghezza(g->mano);
    return g->num_carte_mano;
}
