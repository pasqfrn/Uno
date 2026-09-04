/**
 * @file gameplay_draw.c
 * @brief Rendering del gameplay UNO.
 * @ingroup gameplay_draw
 *
 * Contiene la funzione DisegnaGameplay che gestisce tutto il rendering
 * della schermata di gioco:
 *   - Tavolo verde e area centrale scarti
 *   - Mani dei giocatori (carte con texture realistica)
 *   - Info turno e colore attivo
 *   - Timer partita
 *   - Animazioni carte (trascinamento, pesca, scarto)
 *   - Scelta colore dopo jolly
 *   - Pulsante UNO con countdown
 *   - Chat multiplayer
 *   - Popup uscita
 *   - Gestione fine partita e "Gioca Ancora"
 *
 * Funzioni delegate:
 *   - DisegnaPannelloChat: rendering chat
 *   - DisegnaPopupUscita: popup conferma uscita
 *
 * Fattorizzato da gameplay_screen.c per ridurre la complessita'.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "raylib.h"

#include "../../../lib/screens/game/gameplay_shared.h"
#include "../../../lib/screens/game/gameplay_draw.h"
#include "../../../lib/screens/menu/string_utils.h"
#include "../../../lib/game/game_logic.h"
#include "../../../lib/screens/ui.h"
#include "../../../lib/data_structures/list.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/screens/game/end_screen.h"
#include "../../../lib/socket/lan/lan_sync.h"
#include "../../../lib/auth/auth.h"

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
 * @brief Disegna l'intera schermata di gameplay.
 *
 * Rendering ordinato (dallo sfondo al primo piano):
 *   1. Timer partita (angolo superiore destro)
 *   2. Info turno e colore attivo (bordo inferiore)
 *   3. Info giocatori avversari (mano, nome, numero carte)
 *   4. Tavolo centrale con carta scarto
 *   5. Pila pesca (centro-sinistra)
 *   6. Mano del giocatore umano (con hover effect)
 *   7. Carta in trascinamento (effetto ombra)
 *   8. Scelta colore (overlay per jolly)
 *   9. Pulsante UNO (se il giocatore deve chiamare)
 *   10. Notifiche temporanee
 *   11. Chat (se aperta)
 *   12. Popup uscita (sempre in primo piano)
 *
 * @param gioco Puntatore allo stato di gioco corrente.
 * @param fase Puntatore alla fase applicazione (puo' cambiare a FASE_FINE_PARTITA).
 * @param mousePos Posizione corrente del mouse.
 */
void DisegnaGameplay(StatoGioco* gioco, FaseApplicazione* fase, Vector2 mousePos) {
    int mio_id = (currentRole == NET_CLIENT) ? local_player_id : 0;

    if (!popup_uscita_attivo && DisegnaBottoneMenuAnimato((Rectangle){ 20, ALTEZZA - 70, 150, 40 }, "Esci (ESC)")) { popup_uscita_attivo = 1; }

    /* Timer partita */
    int min = (int)gioco->durata_partita / 60;
    int sec = (int)gioco->durata_partita % 60;
    Rectangle rectTimer = { LARGHEZZA - 160, 20, 140, 50 };
    DrawRectangleRounded(rectTimer, 0.2f, 8, Fade(BLACK, 0.7f));
    DrawRectangleRoundedLines(rectTimer, 0.2f, 8, GOLD);
    const char* timeText = TextFormat("%02d:%02d", min, sec);
    int timeW = MeasureText(timeText, 30);
    DrawText(timeText, rectTimer.x + (rectTimer.width - timeW)/2, rectTimer.y + 10, 30, GOLD);

    /* Info turno e colore attivo */
    Rectangle infoBox = { LARGHEZZA/2.0f - 250, ALTEZZA - 240, 500, 50 };
    DrawRectangleRounded(infoBox, 0.3f, 8, Fade(BLACK, 0.8f));
    Color bordoInfo = (gioco->colore_attivo == NERO) ? LIGHTGRAY : coloriRaylib[gioco->colore_attivo];
    DrawRectangleRoundedLines(infoBox, 0.3f, 8, bordoInfo);
    const char* turnoText = TextFormat("Turno di: %s", Giocatore_NomeBot(&gioco->giocatori[gioco->turno_corrente]));
    Color coloreTestoTurno = (gioco->turno_corrente == mio_id) ? LIME : WHITE;
    DrawText(turnoText, infoBox.x + 30, infoBox.y + 15, 20, coloreTestoTurno);
    DrawText("Colore Attivo:", infoBox.x + 300, infoBox.y + 15, 20, LIGHTGRAY);
    DrawRectangle(infoBox.x + 470, infoBox.y + 15, 20, 20, bordoInfo);
    DrawRectangleLines(infoBox.x + 470, infoBox.y + 15, 20, 20, WHITE);

    /* Info giocatori avversari */
    for (int i = 0; i < gioco->num_giocatori; i++) {
        if (i == mio_id) continue;
        int rel_id = (i - mio_id + gioco->num_giocatori) % gioco->num_giocatori;
        int posX = 0, posY = 0;
        if (gioco->num_giocatori == 2) { posX = LARGHEZZA/2 - 90; posY = 30; }
        else if (gioco->num_giocatori == 3) {
            if (rel_id == 1) { posX = 40; posY = ALTEZZA/2 - 35; }
            else { posX = LARGHEZZA - 220; posY = ALTEZZA/2 - 35; }
        } else {
            if (rel_id == 1) { posX = 40; posY = ALTEZZA/2 - 35; }
            else if (rel_id == 2) { posX = LARGHEZZA/2 - 90; posY = 30; }
            else { posX = LARGHEZZA - 220; posY = ALTEZZA/2 - 35; }
        }
        int cartesLeft = GetLunghezzaMano(&gioco->giocatori[i]);
        DrawRectangleRounded((Rectangle){ posX, posY, 180, 70 }, 0.2f, 8,
                           (gioco->turno_corrente == i) ? Fade(GREEN, 0.5f) : Fade(BLACK, 0.4f));
        DrawText(TextFormat("P%d: %s", i+1, Giocatore_NomeBot(&gioco->giocatori[i])), posX + 10, posY + 10, 14, WHITE);
        DrawText(TextFormat("%d carte", cartesLeft), posX + 10, posY + 35, 12, LIGHTGRAY);
        if (gioco->turno_corrente == i) DrawRectangleRoundedLines((Rectangle){ posX-2, posY-2, 184, 74 }, 0.2f, 8, GOLD);
    }

    /* Tavolo centrale e carta scartata */
    DrawRectangleRoundedLines(areaTavoloCentro, 0.1f, 6, Fade(WHITE, 0.15f));
    if (gioco->num_carte_scarti > 0) {
        DisegnaCartaRealistica(&gioco->scarti[gioco->num_carte_scarti - 1],
                              (Rectangle){ LARGHEZZA/2.0f + 25, ALTEZZA/2.0f - 80, 105, 160 }, 0);
    }

    /* Animazione carta in movimento */
    if (gioco->anim_attiva) {
        Vector2 cP = {
            gioco->anim_start.x + (gioco->anim_end.x - gioco->anim_start.x) * gioco->anim_t,
            gioco->anim_start.y + (gioco->anim_end.y - gioco->anim_start.y) * gioco->anim_t
        };
        DisegnaCartaRealistica(&gioco->anim_carta, (Rectangle){ cP.x, cP.y, 105, 160 }, gioco->tipo_animazione == 1);
    }

    /* Pila pescate */
    DisegnaCartaRealistica(&(Carta){0}, (Rectangle){ LARGHEZZA/2.0f - 130, ALTEZZA/2.0f - 80, 105, 160 }, 1);

    /* Mano del giocatore */
    int numCarte = GetLunghezzaMano(&gioco->giocatori[mio_id]);
    int cardW = 85, cardH = 130, gap = 45;
    int startX = (LARGHEZZA - (numCarte * gap + (cardW - gap))) / 2;

    /* Legge la mano in una snapshot tramite l'API pubblica dell'ADT ListaCarte
     * (nessun accesso alla rappresentazione interna della lista). */
    Carta carte_mano[108];
    int nCarte = GameLogic_CopiaMano(&gioco->giocatori[mio_id], carte_mano, 108);

    int hovered_idx = -1;
    for (int i = 0; i < nCarte; i++) {
        if (!carte_mano[i].isTrascinata) {
            Rectangle r_check = { startX + (i * gap), ALTEZZA - 170, (float)cardW, (float)cardH };
            if (CheckCollisionPointRec(mousePos, r_check)) hovered_idx = i;
        }
    }

    for (int i = 0; i < nCarte; i++) {
        if (!carte_mano[i].isTrascinata) {
            Rectangle r = { startX + (i * gap), ALTEZZA - 170, (float)cardW, (float)cardH };
            carte_mano[i].area = r;
            if (i == hovered_idx) r.y -= 20;
            /* Aggiorna l'area nel container ADT (accessor, information hiding) */
            GameLogic_ImpostaCarta(&gioco->giocatori[mio_id], i, carte_mano[i]);
            DisegnaCartaRealistica(&carte_mano[i], r, 0);
        }
    }

    /* Carta in trascinamento */
    if (gioco->id_carta_in_trascinamento != -1) {
        Carta trascinata = GetCartaGiocatore(&gioco->giocatori[mio_id], gioco->id_carta_in_trascinamento);
        if (trascinata.area.width > 0) {
            DisegnaCartaRealistica(&trascinata, trascinata.area, 0);
        }
    }

    /* Scelta colore (carta nera) */
    if (gioco->in_scelta_colore && gioco->turno_corrente == mio_id && !gioco->gioco_finito) {
        DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.7f));
        DrawText("SCEGLI COLORE", LARGHEZZA/2 - 100, 200, 30, WHITE);
        Rectangle btn[] = {
            { LARGHEZZA/2 - 120, 260, 100, 100 },
            { LARGHEZZA/2 + 20, 260, 100, 100 },
            { LARGHEZZA/2 - 120, 380, 100, 100 },
            { LARGHEZZA/2 + 20, 380, 100, 100 }
        };
        for(int i=0; i<4; i++) {
            DrawRectangleRounded(btn[i], 0.2f, 5, coloriRaylib[i]);
            if (!popup_uscita_attivo && CheckCollisionPointRec(mousePos, btn[i]) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                gioco->in_scelta_colore = 0;
                if (currentRole == NET_CLIENT) {
                    SendMove(pending_move_index, (Colore)i);
                } else {
                    // HOST completa la mossa: usa autore_animazione per sapere chi ha scartato
                    int autore = gioco->autore_animazione;
                    Colore coloreScelto = (Colore)i;
                    gioco->colore_attivo = coloreScelto;
                    ApplicaEffetto(gioco, gioco->carta_pendente, coloreScelto);
                    gioco->scarti[gioco->num_carte_scarti++] = gioco->carta_pendente;
                    
                    // Gestione UNO dopo scelta colore
                    if (GetLunghezzaMano(&gioco->giocatori[autore]) == 1) {
                        gioco->deve_chiamare_uno = autore + 1;
                        gioco->timer_uno = 3.5f;
                    }
                    
                    // Cambio turno (solo se non ha vinto)
                    if (!gioco->gioco_finito) {
                        gioco->turno_corrente = (gioco->turno_corrente + gioco->direzione + gioco->num_giocatori) % gioco->num_giocatori;
                    }
                    
                    // Broadcast dello stato e delle mani aggiornate
                    BroadcastGameState(gioco);
                    BroadcastPlayerHands(gioco);
                }
            }
        }
    }

    /* Bottone UNO! */
    if (gioco->deve_chiamare_uno == mio_id + 1 && !gioco->in_scelta_colore) {
        Rectangle btnUno = { LARGHEZZA/2 - 100, ALTEZZA/2 - 60, 200, 100 };
        DrawRectangleRounded(btnUno, 0.3f, 10, ((int)(gioco->timer_uno * 10) % 2 == 0) ? RED : MAROON);
        DrawText("UNO!", LARGHEZZA/2 - 45, ALTEZZA/2 - 40, 40, WHITE);
        DrawText(TextFormat("%.1f", gioco->timer_uno), LARGHEZZA/2 - 20, ALTEZZA/2 + 5, 24, YELLOW);
        if (!popup_uscita_attivo && CheckCollisionPointRec(mousePos, btnUno) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (currentRole == NET_CLIENT) SendChat("/UNO");
            gioco->deve_chiamare_uno = 0;
            MostraNotifica(gioco, TextFormat("%s ha gridato UNO!", Giocatore_Nome(&gioco->giocatori[mio_id])));
            if (currentRole == NET_HOST) BroadcastGameState(gioco);
        }
    }

    /* Notifica temporanea */
    if (gioco->timer_notifica > 0) {
        int notifW = MeasureText(gioco->notifica, 18) + 40;
        DrawRectangleRounded((Rectangle){ LARGHEZZA - notifW - 20, ALTEZZA - 80, (float)notifW, 60 }, 0.3f, 10, Fade(BLACK, 0.85f));
        DrawText(gioco->notifica, LARGHEZZA - notifW, ALTEZZA - 60, 18, WHITE);
    }

    /* Schermata attesa client */
    /* Schermata attesa client (solo se il giocatore e' DAVVERO un bot,
     * non durante la riconnessione) */
    if (currentRole == NET_CLIENT && mio_id >= 0 && mio_id < gioco->num_giocatori &&
        gioco->giocatori[mio_id].is_bot &&
        /* BUG FIX: Se partita gia' iniziata (carte negli scarti),
         * il giocatore si sta riconnettendo. Non bloccare con ATTENDI... */
        gioco->num_carte_scarti == 0) {
        DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.85f));
        DrawText("ATTENDI...", LARGHEZZA/2 - MeasureText("ATTENDI...", 40)/2, ALTEZZA/2 - 20, 40, YELLOW);
    }


    if (currentRole == NET_CLIENT && gioco->giocatori[mio_id].is_bot) {
        DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.85f));
        DrawText("ATTENDI...", LARGHEZZA/2 - MeasureText("ATTENDI...", 40)/2, ALTEZZA/2 - 20, 40, YELLOW);
    }

    /* Game over */
    if (gioco->gioco_finito) {
        if (strstr(gioco->messaggio, "__HOST_DISCONNECT__") != NULL) {
            DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.85f));
            int fSize = 50;
            const char* msg = "L'host ha chiuso la stanza!";
            int txtW = MeasureText(msg, fSize);
            DrawText(msg, LARGHEZZA/2 - txtW/2, ALTEZZA/2 - 60, fSize, GOLD);
            if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 100, ALTEZZA/2 + 30, 200, 60 }, "ESCI")) {
                gioco->gioco_finito = 0;
                CloseNetwork();
                currentRole = NET_OFFLINE;
                *fase = FASE_MULTIPLAYER_SCELTA;
                return;
            }
            return;
        }
        
        // ================================================================
        // BUG 4 FIX (CLIENT "Gioca Ancora"): Pattern identico a end_screen.c.
        //
        // Il client non va piu' direttamente a FASE_CLIENT_LOBBY se
        // host_in_prelobby == 0. Invece, imposta client_play_again_attesa = 1
        // e rimane nella schermata finale. Quando arriva PACKET_HOST_RETURN_LOBBY
        // (che setta host_in_prelobby = 1), il loop principale transita
        // automaticamente a FASE_CLIENT_LOBBY.
        //
        // Questo previene la "partita fantasma": il client veniva portato in lobby
        // mentre l'host era ancora nella schermata finale, creando uno stato
        // inconsistente in cui il client era in attesa ma l'host non lo aveva
        // registrato.
        // ================================================================
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 100, ALTEZZA/2 + 100, 200, 60 }, "Gioca Ancora")) {
            if (currentRole == NET_HOST) {
                // BUG 4 FIX (HOST): Pulisce gli slot degli ALTRI giocatori prima di tornare in lobby,
                // cosi' la lobby mostra solo l'host. I client riempiranno i propri slot
                // quando premeranno "Gioca Ancora" (PACKET_PLAY_AGAIN).
                for (int i = 1; i < gioco->num_giocatori; i++) {
                    memset(&gioco->giocatori[i], 0, sizeof(Giocatore));
                    gioco->giocatori[i].mano = CreaLista();
                    gioco->giocatori[i].is_bot = 0;
                    gioco->giocatori[i].stato_pronto = 0;
                    gioco->giocatori[i].socket_id = -1;
                }
                gioco->num_giocatori = 1;
                gioco->gioco_finito = 0;
                gioco->num_carte_scarti = 0;
                BroadcastHostReturnLobby();
                BroadcastGameState(gioco);
                host_in_prelobby = 1;
                *fase = FASE_HOST_LOBBY;
                return;
            } else if (currentRole == NET_CLIENT) {
                SendPlayAgain();
                // BUG 4 FIX: Se host_in_prelobby e' gia' 1, vai subito in lobby
                if (host_in_prelobby) {
                    *fase = FASE_CLIENT_LOBBY;
                } else {
                    // BUG 4 FIX: L'host non e' ancora tornato in pre-lobby.
                    // Il flag client_play_again_attesa (definito in end_screen.c)
                    // viene impostato per permettere la transizione automatica
                    // quando arriva PACKET_HOST_RETURN_LOBBY dall'host.
                    extern int client_play_again_attesa;
                    client_play_again_attesa = 1;
                    gioco->timer_notifica = 5.0f;
                    strcpy(gioco->notifica, "In attesa che l'Host torni in lobby...");
                }
                network_start_game_flag = 0;
                return;
            }
        }
        
        network_start_game_flag = 0;
        *fase = FASE_FINE_PARTITA;
        return;
    }

    /* Bottone chat */
    if (currentRole != NET_OFFLINE) {
        if (!popup_uscita_attivo && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA - 160, ALTEZZA - 140, 140, 40 }, "Chat [TAB]")) {
            gioco->chat_aperta = !gioco->chat_aperta;
        }
    }

    /* Renderizza pannello chat (funziona in multiplayer) */
    if (gioco->chat_aperta && currentRole != NET_OFFLINE) {
        extern void DisegnaPannelloChat(StatoGioco* gioco, int mio_id, Vector2 mousePos);
        int mio_id = (currentRole == NET_CLIENT) ? local_player_id : 0;
        DisegnaPannelloChat(gioco, mio_id, mousePos);
    }

    /* Popup di uscita (SEMPRE in fondo per stare sopra tutto) */
    if (popup_uscita_attivo) {
        extern void DisegnaPopupUscita(StatoGioco* gioco, FaseApplicazione* fase);
        DisegnaPopupUscita(gioco, fase);
    }
}