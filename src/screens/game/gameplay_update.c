/**
 * @file gameplay_update.c
 * @brief Logica di aggiornamento del gameplay UNO.
 * @ingroup gameplay_update
 *
 * Modulo separato che contiene la funzione AggiornaGameplay.
 * Gestisce l'input utente, le animazioni, i turni bot,
 * il drag-and-drop delle carte, la chat e la scelta colore.
 *
 * Fattorizzato da gameplay_screen.c per ridurre la complessita'
 * del file originale (659 righe -> 3 moduli).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "raylib.h"

#include "../../../lib/screens/game/gameplay_shared.h"
#include "../../../lib/screens/game/gameplay_update.h"
#include "../../../lib/screens/game/end_screen.h"
#include "../../../lib/screens/menu/string_utils.h"
#include "../../../lib/game_logic.h"
#include "../../../lib/ui.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/socket/lan/lan_sync.h"
#include "../../../lib/auth.h"

#define LARGHEZZA 1280
#define ALTEZZA 720

/* Variabili di stato definite in gameplay_screen.c */
extern float botTimer;
extern Vector2 offsetMouse;
extern Rectangle areaTavoloCentro;
extern int pending_move_index;
extern char inputChat[128];
extern int isTyping;
extern int popup_uscita_attivo;
extern int esc_consumato;

/**
 * @brief Calcola la posizione schermo di un giocatore in base al suo indice.
 *
 * Posizioni:
 *   - Giocatore umano (index == mio_id): in basso al centro
 *   - 2 giocatori: avversario in alto al centro
 *   - 3 giocatori: avversari a sinistra e destra
 *   - 4 giocatori: avversari a sinistra, in alto e a destra
 *
 * @param index_giocatore Indice del giocatore nella lista giocatori[].
 * @param mio_id Indice del giocatore umano.
 * @param num_giocatori Numero totale di giocatori in partita.
 * @return Vector2 con coordinate schermo (centro della mano del giocatore).
 */
Vector2 OttieniPosizioneGiocatore(int index_giocatore, int mio_id, int num_giocatori) {
    if (index_giocatore == mio_id) {
        return (Vector2){ LARGHEZZA/2.0f, ALTEZZA - 100 };
    }
    int rel_id = (index_giocatore - mio_id + num_giocatori) % num_giocatori;
    int posX = 0, posY = 0;
    if (num_giocatori == 2) {
        posX = LARGHEZZA/2 - 90; posY = 30;
    } else if (num_giocatori == 3) {
        if (rel_id == 1) { posX = 40; posY = ALTEZZA/2 - 35; }
        else { posX = LARGHEZZA - 220; posY = ALTEZZA/2 - 35; }
    } else {
        if (rel_id == 1) { posX = 40; posY = ALTEZZA/2 - 35; }
        else if (rel_id == 2) { posX = LARGHEZZA/2 - 90; posY = 30; }
        else { posX = LARGHEZZA - 220; posY = ALTEZZA/2 - 35; }
    }
    return (Vector2){ posX + 90, posY + 35 };
}

/**
 * @brief Aggiorna lo stato del gioco durante il gameplay.
 *
 * Gestisce:
 *   - Animazioni (pesca, scarto carte) con progressione temporale
 *   - Logica bot (mosse automatiche con timer)
 *   - Input giocatore umano: drag-and-drop carte, click pila pesca
 *   - Chat multiplayer (toggle TAB, input testo)
 *   - Scelta colore dopo jolly
 *   - Chiamata UNO con timer penalita
 *   - Gestione fine partita e aggiornamento statistiche
 *
 * @param gioco Puntatore allo stato di gioco corrente.
 * @param fase Puntatore alla fase applicazione (puo' cambiare a FASE_FINE_PARTITA).
 * @param mousePos Posizione corrente del mouse.
 * @param dt Delta time dall'ultimo frame (per animazioni e timer).
 */
void AggiornaGameplay(StatoGioco* gioco, FaseApplicazione* fase, Vector2 mousePos, float dt) {
    SetExitKey(0);
    int mio_id = (currentRole == NET_CLIENT) ? local_player_id : 0;

    /* Gestione tasto ESC (apri/chiudi popup uscita, chiudi chat, esci da digitazione) */
    if (!esc_consumato && IsKeyPressed(KEY_ESCAPE)) {
        if (isTyping) { isTyping = 0; }
        else if (gioco->chat_aperta) { gioco->chat_aperta = 0; }
        else { popup_uscita_attivo = !popup_uscita_attivo; }
    }
    if (popup_uscita_attivo) return;

    /* Aggiorna durata partita (tempo in secondi) */
    if (!gioco->gioco_finito) gioco->durata_partita += dt;

    // ============================================================
    //  Animazione in corso (pesca o scarto carta)
    // ============================================================
    if (gioco->anim_attiva) {
        gioco->anim_t += dt * 3.5f;
        if (gioco->anim_t >= 1.0f) {
            gioco->anim_attiva = 0;
            if (gioco->anim_gia_applicata) {
                gioco->anim_gia_applicata = 0;
                gioco->anim_pesca_count = 0;
            } else if (currentRole != NET_CLIENT) {
                if (gioco->tipo_animazione == 1) {
                    /* Animazione pesca */
                    int drawCount = (gioco->anim_pesca_count > 0) ? gioco->anim_pesca_count : 1;
                    Pesca(gioco, gioco->autore_animazione, drawCount);
                    gioco->anim_pesca_count = 0;
                    Giocatore* p = &gioco->giocatori[gioco->autore_animazione];
                    for (int k = 0; k < drawCount; k++) {
                        if (p->mano && p->mano->lunghezza > 1) {
                            NodoCarta* prima = p->mano->testa;
                            p->mano->testa = prima->prossimo;
                            prima->prossimo = NULL;
                            p->mano->coda->prossimo = prima;
                            p->mano->coda = prima;
                        }
                    }
                    MostraNotifica(gioco, TextFormat("%s ha pescato.", Giocatore_Nome(&gioco->giocatori[gioco->autore_animazione])));
                    gioco->turno_corrente = (gioco->turno_corrente + gioco->direzione + gioco->num_giocatori) % gioco->num_giocatori;
                    botTimer = 0.0f;
                    if (currentRole == NET_HOST) BroadcastGameState(gioco);
                } else {
                    /* Animazione scarto */
                    gioco->scarti[gioco->num_carte_scarti++] = gioco->anim_carta;
                    if (gioco->anim_carta.colore == NERO && gioco->autore_animazione == mio_id) {
                        gioco->in_scelta_colore = 1;
                        gioco->carta_pendente = gioco->anim_carta;
                    } else {
                        int autore = gioco->autore_animazione;
                        ApplicaEffetto(gioco, gioco->anim_carta, gioco->colore_attivo);
                        if (GetLunghezzaMano(&gioco->giocatori[autore]) == 0) {
                            gioco->gioco_finito = 1;
                            sprintf(gioco->messaggio, "HA VINTO %s!", Giocatore_Nome(&gioco->giocatori[autore]));
                            if (!gioco->statistiche_gia_salvate && currentRole != NET_CLIENT) {
                                Statistiche* utente = &dbUtenti.lista[id_utente_corrente];
                                if (utente && utente->partite_giocate < MAX_PARTITE_STORICO) {
                                    gioco->statistiche_gia_salvate = 1;
                                    PartitaRegistrata* p = &utente->storico_partite[utente->partite_giocate];
                                    if (currentRole != NET_OFFLINE) strcpy(p->modalita, "Online");
                                    else sprintf(p->modalita, "1 vs %d", gioco->num_giocatori - 1);
                                    ClassificaRecord cr[4]; int nc = 0;
                                    CalcolaClassifica(gioco, cr, &nc);
                                    int mia_pos = gioco->num_giocatori;
                                    int ho_vinto = 0;
                                    for (int k = 0; k < nc; k++) {
                                        if (Giocatore_Nome(&gioco->giocatori[mio_id])[0] &&
                                            strcmp(cr[k].nome, Giocatore_Nome(&gioco->giocatori[mio_id])) == 0) {
                                            mia_pos = cr[k].posizione;
                                            if (mia_pos == 1) ho_vinto = 1;
                                            break;
                                        }
                                    }
                                    strcpy(p->esito, ho_vinto ? "Vittoria" : "Sconfitta");
                                    p->durata_secondi = (int)gioco->durata_partita;
                                    p->posizione = mia_pos;
                                    utente->partite_giocate++;
                                    if (ho_vinto) utente->vittorie_giocatore++;
                                    else utente->vittorie_bot++;
                                    SalvaDB();
                                    LAN_BroadcastNow();
                                    if (id_utente_corrente >= 0) {
                                        LAN_BroadcastStatsSync(dbUtenti.lista[id_utente_corrente].username);
                                    }
                                    if (currentRole == NET_HOST) {
                                        StatsSyncPacket sync = {0};
                                        strncpy(sync.username, dbUtenti.lista[id_utente_corrente].username, sizeof(sync.username) - 1);
                                        sync.partite_giocate = dbUtenti.lista[id_utente_corrente].partite_giocate;
                                        sync.vittorie_giocatore = dbUtenti.lista[id_utente_corrente].vittorie_giocatore;
                                        sync.vittorie_bot = dbUtenti.lista[id_utente_corrente].vittorie_bot;
                                        sync.durata_ultima_partita = gioco->durata_partita;
                                        strncpy(sync.modalita_ultima, p->modalita, sizeof(sync.modalita_ultima) - 1);
                                        strncpy(sync.esito_ultima, p->esito, sizeof(sync.esito_ultima) - 1);
                                        sync.posizione_ultima = mia_pos;
                                        BroadcastStatsSync(&sync);
                                    }
                                }
                            }
                        } else if (GetLunghezzaMano(&gioco->giocatori[autore]) == 1) {
                            if (gioco->giocatori[autore].is_bot) {
                                MostraNotifica(gioco, TextFormat("%s grida UNO!", Giocatore_Nome(&gioco->giocatori[autore])));
                            } else {
                                gioco->deve_chiamare_uno = autore + 1;
                                gioco->timer_uno = 3.5f;
                            }
                        }
                        botTimer = 0.0f;
                        BroadcastGameState(gioco);
                        BroadcastPlayerHands(gioco);
                    }
                }
            }
        }
    }
    // ============================================================
    //  Turno normale (nessuna animazione)
    // ============================================================
    else if (!gioco->gioco_finito && !gioco->anim_attiva && !gioco->in_scelta_colore) {
        /* Timer penalita UNO */
        if (gioco->deve_chiamare_uno) {
            if (currentRole == NET_CLIENT) {
                gioco->timer_uno -= dt;
                if (gioco->timer_uno <= 0) {
                    gioco->deve_chiamare_uno = 0;
                    gioco->timer_uno = 0;
                }
            } else {
                gioco->timer_uno -= dt;
                if (gioco->timer_uno <= 0) {
                    int player_to_penalize = gioco->deve_chiamare_uno - 1;
                    Pesca(gioco, player_to_penalize, 2);
                    Giocatore* p = &gioco->giocatori[player_to_penalize];
                    for (int k = 0; k < 2; k++) {
                        if (p->mano && p->mano->lunghezza > 1) {
                            NodoCarta* prima = p->mano->testa;
                            p->mano->testa = prima->prossimo;
                            prima->prossimo = NULL;
                            p->mano->coda->prossimo = prima;
                            p->mano->coda = prima;
                        }
                    }
                    gioco->deve_chiamare_uno = 0;
                    MostraNotifica(gioco, TextFormat("Penalita! %s non ha gridato UNO!", Giocatore_Nome(&gioco->giocatori[player_to_penalize])));
                    BroadcastGameState(gioco);
                    BroadcastPlayerHands(gioco);
                }
            }
        }

        /* Turno bot o giocatore umano */
        Giocatore *g_corrente = &gioco->giocatori[gioco->turno_corrente];
        if (g_corrente->is_bot && currentRole != NET_CLIENT) {
            /* Logica bot: mossa automatica con timer */
            if (currentRole == NET_HOST && IsPlayerConnected(gioco->turno_corrente)) {
                g_corrente->is_bot = 0;
                MostraNotifica(gioco, TextFormat("%s rientrato!", Giocatore_Nome(g_corrente)));
                BroadcastGameState(gioco);
                BroadcastPlayerHands(gioco);
            } else {
                botTimer += dt;
                if (botTimer >= 1.5f) {
                    int mossa = -1;
                    for (int i = 0; i < GetLunghezzaMano(g_corrente); i++) {
                        if (MossaValida(gioco, GetCartaGiocatore(g_corrente, i))) {
                            mossa = i;
                            if (GetCartaGiocatore(g_corrente, i).colore != NERO) break;
                        }
                    }
                    if (mossa != -1) {
                        Carta c = GetCartaGiocatore(g_corrente, mossa);
                        if (c.colore == NERO) gioco->colore_attivo = rand() % 4;
                        gioco->anim_attiva = 1;
                        gioco->anim_carta = c;
                        gioco->tipo_animazione = 0;
                        gioco->anim_start = OttieniPosizioneGiocatore(gioco->turno_corrente, mio_id, gioco->num_giocatori);
                        gioco->anim_end = (Vector2){LARGHEZZA/2.0f + 25, ALTEZZA/2.0f - 80};
                        gioco->anim_t = 0.0f;
                        gioco->autore_animazione = gioco->turno_corrente;
                        RimuoviCartaGiocatore(g_corrente, mossa);
                        if (currentRole == NET_HOST) BroadcastGameState(gioco);
                    } else {
                        /* Bot pesca */
                        gioco->anim_attiva = 1;
                        gioco->tipo_animazione = 1;
                        gioco->anim_carta = (Carta){0};
                        gioco->anim_start = (Vector2){ LARGHEZZA/2.0f - 130, ALTEZZA/2.0f - 80 };
                        gioco->anim_end = OttieniPosizioneGiocatore(gioco->turno_corrente, mio_id, gioco->num_giocatori);
                        gioco->anim_t = 0.0f;
                        gioco->autore_animazione = gioco->turno_corrente;
                    }
                }
            }
        } else {
            /* Logica giocatore umano: click, drag-and-drop */
            if (!network_pending && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && gioco->id_carta_in_trascinamento == -1 && !gioco->deve_chiamare_uno) {
                /* Click sulla pila pescate */
                if (CheckCollisionPointRec(mousePos, (Rectangle){ LARGHEZZA/2.0f - 130, ALTEZZA/2.0f - 80, 105, 160 }) && gioco->turno_corrente == mio_id) {
                    if (currentRole == NET_CLIENT) SendDraw(1);
                    else {
                        gioco->anim_attiva = 1;
                        gioco->tipo_animazione = 1;
                        gioco->anim_carta = (Carta){0};
                        gioco->anim_start = (Vector2){ LARGHEZZA/2.0f - 130, ALTEZZA/2.0f - 80 };
                        gioco->anim_end = OttieniPosizioneGiocatore(mio_id, mio_id, gioco->num_giocatori);
                        gioco->anim_t = 0.0f;
                        gioco->autore_animazione = mio_id;
                        BroadcastGameState(gioco);
                    }
                }
                /* Inizio trascinamento carta */
                int len = GetLunghezzaMano(&gioco->giocatori[mio_id]);
                for (int i = len - 1; i >= 0; i--) {
                    NodoCarta* n = GetNodoCartaGiocatore(&gioco->giocatori[mio_id], i);
                    if (n && CheckCollisionPointRec(mousePos, n->carta.area) && gioco->turno_corrente == mio_id) {
                        n->carta.isTrascinata = 1;
                        offsetMouse = (Vector2){ mousePos.x - n->carta.area.x, mousePos.y - n->carta.area.y };
                        gioco->id_carta_in_trascinamento = i; gioco->nodo_carta_trascinata = n; break;
                    }
                }
            }
            /* Trascinamento in corso */
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && gioco->id_carta_in_trascinamento != -1 && gioco->nodo_carta_trascinata) {
                gioco->nodo_carta_trascinata->carta.area.x = mousePos.x - offsetMouse.x;
                gioco->nodo_carta_trascinata->carta.area.y = mousePos.y - offsetMouse.y;
            }
            /* Rilascio carta */
            if (!network_pending && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && gioco->id_carta_in_trascinamento != -1) {
                if (gioco->nodo_carta_trascinata) {
                    gioco->nodo_carta_trascinata->carta.isTrascinata = 0;
                    if (CheckCollisionRecs(gioco->nodo_carta_trascinata->carta.area, areaTavoloCentro) && MossaValida(gioco, gioco->nodo_carta_trascinata->carta)) {
                        Carta scartata = gioco->nodo_carta_trascinata->carta;
                        if (currentRole == NET_CLIENT) {
                            if (scartata.colore == NERO) {
                                gioco->in_scelta_colore = 1;
                                gioco->carta_pendente = scartata;
                                pending_move_index = gioco->id_carta_in_trascinamento;
                            } else {
                                SendMove(gioco->id_carta_in_trascinamento, scartata.colore);
                            }
                        } else {
                            gioco->anim_attiva = 1; gioco->anim_carta = scartata;
                            gioco->tipo_animazione = 0;
                            gioco->anim_start = (Vector2){ gioco->nodo_carta_trascinata->carta.area.x, gioco->nodo_carta_trascinata->carta.area.y };
                            gioco->anim_end = (Vector2){ LARGHEZZA/2.0f + 25, ALTEZZA/2.0f - 80 };
                            gioco->anim_t = 0.0f;
                            gioco->autore_animazione = mio_id;
                            RimuoviCartaGiocatore(&gioco->giocatori[mio_id], gioco->id_carta_in_trascinamento);
                            if (scartata.colore == NERO) gioco->carta_pendente = scartata;
                            BroadcastGameState(gioco);
                        }
                    } else {
                        /* Riporta carta alla posizione originale */
                        gioco->nodo_carta_trascinata->carta.area.x = gioco->nodo_carta_trascinata->carta.posOriginale.x;
                        gioco->nodo_carta_trascinata->carta.area.y = gioco->nodo_carta_trascinata->carta.posOriginale.y;
                    }
                }
                gioco->id_carta_in_trascinamento = -1; gioco->nodo_carta_trascinata = NULL;
            }
        }
    }

    /* Toggle chat con TAB */
    if (IsKeyPressed(KEY_TAB)) {
        gioco->chat_aperta = !gioco->chat_aperta;
        if (gioco->chat_aperta && currentRole != NET_OFFLINE) {
            isTyping = 1;
            inputChat[0] = '\0';
        } else { isTyping = 0; }
    }

    /* Apri chat con ENTER */
    if (IsKeyPressed(KEY_ENTER) && !isTyping && currentRole != NET_OFFLINE) {
        gioco->chat_aperta = 1; isTyping = 1; inputChat[0] = '\0';
    }

    /* Input digitazione chat */
    if (isTyping) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 125 && strlen(inputChat) < 127) {
                int len = strlen(inputChat);
                inputChat[len] = (char)key;
                inputChat[len+1] = '\0';
            }
            key = GetCharPressed();
        }
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && strlen(inputChat) > 0) inputChat[strlen(inputChat)-1] = '\0';
        if (IsKeyPressed(KEY_ESCAPE)) isTyping = 0;
    }
}