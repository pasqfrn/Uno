/**
 * @file bot_ai.c
 * @brief Implementazione intelligenza artificiale per i bot UNO.
 * @ingroup bot_ai
 *
 * Gestisce il comportamento dei bot durante una partita:
 * selezione strategica carta, gestione speciali, scelta colore,
 * delay simulato. Basato su albero decisionale.
 */
#include <stdlib.h>
#include <string.h>
#include "../../../lib/screens/game/bot_ai.h"
#include "../../../lib/game/game_logic.h"
#include "../../../lib/data_structures/list.h"

// ============================================================
//  ALBERO DECISIONALE PER IA BOT - Implementazione
//  Riferimento: Documentazione CdS, sezione "Strutture dati
//  dinamiche" - "Albero decisionale per strutturare la logica
//  dell'IA dei bot"
// ============================================================

/**
 * @brief Costruisce l'albero decisionale dell'IA bot (radice).
 * @ingroup bot_ai
 * @post Radice allocata con priorita' definita.
 * @return Puntatore alla radice, o NULL.
 */
NodoDecisione* BotAI_CostruisciAlbero(void) {
    NodoDecisione* radice = (NodoDecisione*)malloc(sizeof(NodoDecisione));
    if (!radice) return NULL;

    radice->priorita = 10;
    radice->azione = AZIONE_GIOCA_JOLLY_PIU4;
    radice->si = NULL;
    radice->no = NULL;

    return radice;
}

/**
 * @brief Distrugge ricorsivamente l'albero decisionale.
 * @ingroup bot_ai
 * @pre radice valida o NULL.
 * @post Memoria liberata.
 * @param radice Radice.
 */
void BotAI_DistruggiAlbero(NodoDecisione* radice) {
    if (!radice) return;
    BotAI_DistruggiAlbero(radice->si);
    BotAI_DistruggiAlbero(radice->no);
    free(radice);
}

/**
 * @brief Decide la mossa del bot in base alla strategia.
 * @ingroup bot_ai
 * @pre radice e gioco validi; id_giocatore valido.
 * @return DecisioneBot presa.
 * @param radice Radice albero.
 * @param gioco Stato di gioco.
 * @param id_giocatore Indice del bot.
 */
DecisioneBot BotAI_Decidi(NodoDecisione* radice, StatoGioco* gioco, int id_giocatore) {
    if (!radice || !gioco) return AZIONE_PESCA;

    Giocatore* g = &gioco->giocatori[id_giocatore];
    int num_carte = GetLunghezzaMano(g);

    // 1. Se ho 1 carta e posso giocare Jolly+4 -> priorità massima
    if (num_carte <= 2) {
        for (int i = 0; i < num_carte; i++) {
            Carta c = GetCartaGiocatore(g, i);
            if (c.tipo == PESCA_QUATTRO && MossaValida(gioco, c))
                return AZIONE_GIOCA_JOLLY_PIU4;
        }
    }

    // 2. Se posso giocare +2 -> giocalo (strategico)
    for (int i = 0; i < num_carte; i++) {
        Carta c = GetCartaGiocatore(g, i);
        if (c.tipo == PESCA_DUE && MossaValida(gioco, c))
            return AZIONE_GIOCA_PIU2;
    }

    // 3. Se posso giocare Salta -> giocalo
    for (int i = 0; i < num_carte; i++) {
        Carta c = GetCartaGiocatore(g, i);
        if (c.tipo == SALTA && MossaValida(gioco, c))
            return AZIONE_GIOCA_SALTA;
    }

    // 4. Se posso giocare Cambio Giro -> giocalo se ho poche carte
    if (num_carte <= 3) {
        for (int i = 0; i < num_carte; i++) {
            Carta c = GetCartaGiocatore(g, i);
            if (c.tipo == CAMBIA_GIRO && MossaValida(gioco, c))
                return AZIONE_GIOCA_CAMBIO_GIRO;
        }
    }

    // 5. Carta numerata più alta possibile
    int miglior_indice = -1;
    int miglior_valore = -1;
    for (int i = 0; i < num_carte; i++) {
        Carta c = GetCartaGiocatore(g, i);
        if (MossaValida(gioco, c) && c.colore != NERO) {
            int val = (c.tipo >= NUM_0 && c.tipo <= NUM_9) ? (int)c.tipo : 10;
            if (val > miglior_valore) {
                miglior_valore = val;
                miglior_indice = i;
            }
        }
    }
    if (miglior_indice != -1) {
        Carta c = GetCartaGiocatore(g, miglior_indice);
        if (c.tipo >= NUM_0 && c.tipo <= NUM_9) {
            return (miglior_valore >= 7) ? AZIONE_GIOCA_NUMERO_ALTO : AZIONE_GIOCA_NUMERO_BASSO;
        }
        return AZIONE_GIOCA_NUMERO_ALTO;
    }

    // 6. Cambio Colore come riserva
    for (int i = 0; i < num_carte; i++) {
        Carta c = GetCartaGiocatore(g, i);
        if (c.tipo == CAMBIO_COLORE && MossaValida(gioco, c))
            return AZIONE_GIOCA_CAMBIO_COLORE;
    }

    // 7. Jolly+4 come ultima risorsa
    for (int i = 0; i < num_carte; i++) {
        Carta c = GetCartaGiocatore(g, i);
        if (c.tipo == PESCA_QUATTRO && MossaValida(gioco, c))
            return AZIONE_GIOCA_JOLLY_PIU4;
    }

    return AZIONE_PESCA;
}

/**
 * @brief Trova l'indice della carta corrispondente all'azione.
 * @ingroup bot_ai
 * @pre gioco valido; azione valida.
 * @return Indice carta, -1 se non trovata.
 * @param gioco Stato di gioco.
 * @param id_giocatore Indice del bot.
 * @param azione DecisioneBot.
 */
int BotAI_TrovaCarta(StatoGioco* gioco, int id_giocatore, DecisioneBot azione) {
    if (!gioco) return -1;
    Giocatore* g = &gioco->giocatori[id_giocatore];
    int num_carte = GetLunghezzaMano(g);

    if (azione == AZIONE_PESCA) return -1;

    // Cerca la carta appropriata
    for (int i = 0; i < num_carte; i++) {
        Carta c = GetCartaGiocatore(g, i);
        if (!MossaValida(gioco, c)) continue;

        switch (azione) {
            case AZIONE_GIOCA_JOLLY_PIU4:
                if (c.tipo == PESCA_QUATTRO) return i;
                break;
            case AZIONE_GIOCA_PIU2:
                if (c.tipo == PESCA_DUE) return i;
                break;
            case AZIONE_GIOCA_SALTA:
                if (c.tipo == SALTA) return i;
                break;
            case AZIONE_GIOCA_CAMBIO_GIRO:
                if (c.tipo == CAMBIA_GIRO) return i;
                break;
            case AZIONE_GIOCA_CAMBIO_COLORE:
                if (c.tipo == CAMBIO_COLORE) return i;
                break;
            case AZIONE_GIOCA_NUMERO_ALTO:
                if (c.tipo >= NUM_7 && c.tipo <= NUM_9 && c.colore != NERO) return i;
                if (c.tipo == SALTA || c.tipo == CAMBIA_GIRO || c.tipo == PESCA_DUE) return i;
                break;
            case AZIONE_GIOCA_NUMERO_BASSO:
                if (c.tipo >= NUM_0 && c.tipo <= NUM_6 && c.colore != NERO) return i;
                break;
            default:
                break;
        }
    }

    // Fallback: prendi qualsiasi carta valida
    for (int i = 0; i < num_carte; i++) {
        Carta c = GetCartaGiocatore(g, i);
        if (MossaValida(gioco, c)) return i;
    }

    return -1;
}

/**
 * @brief Sceglie il colore piu' frequente nella mano del bot (dopo Jolly).
 * @ingroup bot_ai
 * @pre gioco valido; id_giocatore valido.
 * @return Colore con piu' carte.
 * @param gioco Stato di gioco.
 * @param id_giocatore Indice del bot.
 */
Colore BotAI_ScegliColore(StatoGioco* gioco, int id_giocatore) {
    if (!gioco) return ROSSO;

    Giocatore* g = &gioco->giocatori[id_giocatore];
    int num_carte = GetLunghezzaMano(g);

    // Conta carte per colore
    int conteggio[4] = {0, 0, 0, 0};
    for (int i = 0; i < num_carte; i++) {
        Carta c = GetCartaGiocatore(g, i);
        if (c.colore >= ROSSO && c.colore <= BLU)
            conteggio[c.colore]++;
    }

    // Scegli il colore più frequente
    int max_col = ROSSO;
    int max_cnt = conteggio[ROSSO];
    for (int i = 1; i < 4; i++) {
        if (conteggio[i] > max_cnt) {
            max_cnt = conteggio[i];
            max_col = i;
        }
    }

    return (Colore)max_col;
}