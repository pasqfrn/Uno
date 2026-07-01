/**
 * @file game_flow.c
 * @brief Gestione flusso di gioco: inizializzazione, fine partita, ricostruzione ADT.
 * @ingroup game_logic
 *
 * Modulo specializzato per il ciclo di vita di una partita:
 * NuovaPartita, EliminaPartita, RicostruisciStatoADT.
 * Estratto da game_logic.c per modularizzazione.
 */
#include <stdlib.h>
#include <string.h>

#include "../../lib/game_logic.h"
#include "../../lib/data_structures.h"
#include "../../lib/data_structures/stack.h"
#include "../../lib/data_structures/queue.h"
#include "../../lib/list.h"
#include "../../lib/auth.h"

/**
 * @brief Ricostruisce TUTTE le strutture ADT statiche dopo il caricamento da file.
 *
 * Dopo CaricaPartitaDaSlot(), le strutture ADT dinamiche (pile, code, liste)
 * sono NULL. Questa funzione ricrea tutto partendo dai dati serializzati:
 *   - Ricrea mazzo_pila da gioco->mazzo[]
 *   - Ricrea scarti_pila da gioco->scarti[]
 *   - Ricrea coda_turni da gioco->giocatori[]
 *   - Ricrea la mano di ogni giocatore da gioco->mano_backup[]
 *
 * @param gioco Puntatore allo stato di gioco appena caricato.
 */
void RicostruisciStatoADT(StatoGioco *gioco) {
    if (mazzo_pila) { Pila_Distruggi(mazzo_pila); mazzo_pila = NULL; }
    if (scarti_pila) { Pila_Distruggi(scarti_pila); scarti_pila = NULL; }
    if (coda_turni) { Coda_Distruggi(coda_turni); coda_turni = NULL; }
    mazzo_pila = Pila_Crea();
    scarti_pila = Pila_Crea();
    for (int i = 0; i < gioco->num_carte_mazzo; i++) {
        Pila_Push(mazzo_pila, gioco->mazzo[i]);
    }
    for (int i = 0; i < gioco->num_carte_scarti; i++) {
        Pila_Push(scarti_pila, gioco->scarti[i]);
    }
    coda_turni = Coda_Crea();
    for (int i = 0; i < gioco->num_giocatori; i++) {
        Coda_Enqueue(coda_turni, gioco->giocatori[i]);
    }
    for (int i = 0; i < gioco->num_giocatori; i++) {
        Giocatore *g = &gioco->giocatori[i];
        g->mano = CreaLista();
        for (int j = 0; j < g->num_carte_backup; j++) {
            InsertInCoda(g->mano, g->mano_backup[j]);
        }
        g->num_carte_mano = g->mano->lunghezza;
    }
    gioco->anim_attiva = 0;
    gioco->anim_t = 0;
    gioco->in_scelta_colore = 0;
    gioco->id_carta_in_trascinamento = -1;
    gioco->nodo_carta_trascinata = NULL;
    gioco->deve_chiamare_uno = 0;
    gioco->timer_uno = 0;
}

/**
 * @brief Inizializza una nuova partita offline contro bot.
 *
 * Procedura:
 *   1. Azzera lo StatoGioco
 *   2. Imposta il giocatore umano (slot 0) e i bot (1..N)
 *   3. Crea il mazzo standard UNO (108 carte)
 *   4. Distribuisce 7 carte per giocatore
 *   5. Pesca la prima carta scarto (evitando jolly)
 *   6. Inizializza la coda turni
 *   7. Trova il primo slot di salvataggio libero
 *
 * @param gioco Puntatore allo stato di gioco da inizializzare.
 * @param num_avversari Numero di avversari bot (1..3).
 */
void NuovaPartita(StatoGioco *gioco, int num_avversari) {
    memset(gioco, 0, sizeof(StatoGioco));
    gioco->num_giocatori = num_avversari + 1;
    gioco->direzione = 1;
    gioco->id_carta_in_trascinamento = -1;
    gioco->durata_partita = 0.0f;
    strcpy(gioco->giocatori[0].nome, dbUtenti.lista[id_utente_corrente].username);
    for (int i = 1; i <= num_avversari; i++) {
        sprintf(gioco->giocatori[i].nome, "Bot_%d", i);
        gioco->giocatori[i].is_bot = 1;
        gioco->giocatori[i].stato_pronto = 1;
        gioco->giocatori[i].socket_id = -1;
        gioco->giocatori[i].mano = CreaLista();
    }
    gioco->giocatori[0].mano = CreaLista();
    gioco->giocatori[0].stato_pronto = 1;
    gioco->giocatori[0].socket_id = -1;
    InizializzaMazzo(gioco);
    for (int i = 0; i < gioco->num_giocatori; i++) Pesca(gioco, i, 7);
    do {
        gioco->scarti[0] = gioco->mazzo[--gioco->num_carte_mazzo];
    } while (gioco->scarti[0].colore == NERO || gioco->scarti[0].tipo > NUM_9);
    gioco->num_carte_scarti = 1;
    gioco->colore_attivo = gioco->scarti[0].colore;
    if (scarti_pila) Pila_Svuota(scarti_pila);
    else scarti_pila = Pila_Crea();
    Pila_Push(scarti_pila, gioco->scarti[0]);
    if (coda_turni) Coda_Distruggi(coda_turni);
    coda_turni = Coda_Crea();
    for (int i = 0; i < gioco->num_giocatori; i++) {
        Coda_Enqueue(coda_turni, gioco->giocatori[i]);
    }
    gioco->slot_salvataggio = 1;
    while (1) {
        char percorso[70];
        snprintf(percorso, sizeof(percorso), "data/games/save_user%d_slot_%d.dat",
                 id_utente_corrente, gioco->slot_salvataggio);
        FILE *f = fopen(percorso, "rb");
        if (!f) break;
        fclose(f);
        gioco->slot_salvataggio++;
    }
}

/**
 * @brief Libera tutte le risorse di una partita terminata.
 *
 * Distrugge: mani giocatori (liste), mazzo_pila, scarti_pila, coda_turni.
 * Resetta num_giocatori, num_carte_mazzo, num_carte_scarti a 0.
 *
 * @param gioco Puntatore allo stato di gioco da deallocare.
 */
void EliminaPartita(StatoGioco *gioco) {
    for (int i = 0; i < gioco->num_giocatori; i++) {
        if (gioco->giocatori[i].mano) {
            EliminaLista(gioco->giocatori[i].mano);
            gioco->giocatori[i].mano = NULL;
        }
    }
    if (mazzo_pila) { Pila_Distruggi(mazzo_pila); mazzo_pila = NULL; }
    if (scarti_pila) { Pila_Distruggi(scarti_pila); scarti_pila = NULL; }
    if (coda_turni) { Coda_Distruggi(coda_turni); coda_turni = NULL; }
    gioco->num_giocatori = 0;
    gioco->num_carte_mazzo = 0;
    gioco->num_carte_scarti = 0;
}