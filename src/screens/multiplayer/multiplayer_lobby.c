/**
 * @file multiplayer_lobby.c
 * @brief Schermata della lobby multiplayer (host e client).
 * @ingroup multiplayer_lobby
 *
 * Gestisce il rendering e la logica della sala d'attesa multiplayer,
 * inclusi l'aggiunta/rimozione di bot, l'avvio della partita (host)
 * e l'attesa dell'avvio (client).
 *
 * NOTA: L'ordine di avvio partita e' ora corretto:
 * 1. InizializzaMazzo + distribuisci carte
 * 2. BroadcastGameState + BroadcastPlayerHands (invia stato ai client)
 * 3. SendStartGame (segnala ai client di iniziare la partita)
 * Questo garantisce che i client ricevano lo stato di gioco PRIMA
 * del segnale di avvio.
 */
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "../../../lib/screens/multiplayer/multiplayer_lobby.h"
#include "../../../lib/screens/multiplayer/multiplayer_screen.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/socket/server/server_manager.h"
#include "../../../lib/ui.h"
#include "../../../lib/game_logic.h"
#include "../../../lib/list.h"
#include "../../../lib/auth.h"
#include "../../../lib/screens/menu/string_utils.h"

#define LARGHEZZA 1280
#define ALTEZZA 720

void DisegnaMultiplayerLobby(GameSession* session, FaseApplicazione* fase) {
    if (!session) return;
    DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.8f));
    DrawText(TextFormat("LOBBY - %s", session->session_id), LARGHEZZA/2 - MeasureText(TextFormat("LOBBY - %s", session->session_id), 40)/2, 30, 40, GOLD);
    DrawText(TextFormat("Giocatori: %d/4", session->num_players), LARGHEZZA/2 - MeasureText("Giocatori: X/4", 20)/2, 80, 20, LIGHTGRAY);
    int playerY = 130;
    for (int i = 0; i < 4; i++) {
        Rectangle playerBox = { LARGHEZZA/2 - 150, playerY, 300, 50 };
        if (i < session->num_players && session->players[i].is_connected) {
            DrawRectangleRounded(playerBox, 0.1f, 5, GREEN);
            DrawText(TextFormat("P%d: %s", i+1, session->players[i].nome), playerBox.x + 10, playerBox.y + 15, 18, WHITE);
        } else {
            DrawRectangleRounded(playerBox, 0.1f, 5, Fade(GRAY, 0.6f));
            DrawText("In attesa di nuovi giocatori...", playerBox.x + 10, playerBox.y + 12, 20, LIGHTGRAY);
        }
        playerY += 70;
    }
    DrawText("Codice Sessione:", LARGHEZZA/2 - MeasureText("Codice Sessione:", 20)/2, 500, 20, LIGHTGRAY);
    DrawText(session->session_id, LARGHEZZA/2 - MeasureText(session->session_id, 40)/2, 530, 40, YELLOW);
    if (session->host_id == player_id) {
        if (session->num_players >= 2) {
            if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 150, 600, 300, 60 }, "Avvia Partita")) {
                StartSession(session); *fase = FASE_GIOCO;
            }
        } else {
            DrawRectangle(LARGHEZZA/2 - 150, 600, 300, 60, Fade(GRAY, 0.5f));
            DrawText("Aspetta almeno 2 giocatori", LARGHEZZA/2 - MeasureText("Aspetta almeno 2 giocatori", 18)/2, 615, 18, DARKGRAY);
        }
    } else {
        DrawText("In attesa che l'host avvii...", LARGHEZZA/2 - MeasureText("In attesa che l'host avvii...", 20)/2, 615, 20, LIGHTGRAY);
    }
}


static float timerErroreAvvio = 0.0f;

void ResetTimerErroreAvvio(void) {
    timerErroreAvvio = 0.0f;
}

/**
 * @brief Pulisce i bot-sostituti dalla lobby (ex-client disconnessi).
 * @ingroup multiplayer_lobby
 * @pre gioco != NULL; currentRole == NET_HOST.
 * @post Bot-sostituti rimossi; giocatori compattati.
 * @param gioco Stato di gioco.
 * 
 * TF-06: Quando si torna nella lobby dopo una partita finita, rimuove i bot
 * che hanno sostituito client disconnessi per permettere ai giocatori reali
 * di unirsi. Vengono mantenuti:
 *   - Bot originali (Bot_1, Bot_2, ...) scelti dall'host
 *   - Bot-sostituti che hanno mantenuto il nome del client originale
 *     (per permettere la riconnessione diretta senza passare dalla lobby)
 * Vengono rimossi solo i bot-sostituti senza nome (nome[0] == '\0').
 */
static void PulisciBotSostituti(StatoGioco* gioco) {
    if (!gioco || currentRole != NET_HOST) return;
    
    int scritti = 0;
    for (int i = 0; i < gioco->num_giocatori; i++) {
        // Rimuovi solo i bot-sostituti senza nome: is_bot==1, socket_id==-1, era_in_prelobby==1, nome vuoto
        // I bot-sostituti con nome di giocatore reale vengono preservati per permettere la riconnessione
        if (gioco->giocatori[i].is_bot && 
            gioco->giocatori[i].socket_id == -1 && 
            gioco->giocatori[i].era_in_prelobby == 1 &&
            gioco->giocatori[i].nome[0] == '\0') {
            // Elimina la mano se esiste
            if (gioco->giocatori[i].mano) {
                EliminaLista(gioco->giocatori[i].mano);
                gioco->giocatori[i].mano = NULL;
            }
            memset(&gioco->giocatori[i], 0, sizeof(Giocatore));
            gioco->giocatori[i].socket_id = -1;
            gioco->giocatori[i].is_bot = 0;
            gioco->giocatori[i].stato_pronto = 0;
            gioco->giocatori[i].era_in_prelobby = 0;
        } else {
            // Mantieni questo giocatore, compatta se necessario
            scritti++;
            if (i != scritti - 1) {
                gioco->giocatori[scritti - 1] = gioco->giocatori[i];
                memset(&gioco->giocatori[i], 0, sizeof(Giocatore));
                gioco->giocatori[i].socket_id = -1;
                gioco->giocatori[i].is_bot = 0;
            }
        }
    }
    gioco->num_giocatori = scritti;
    
    // Rinumera i bot originali rimasti
    int b_idx = 1;
    for (int k = 0; k < gioco->num_giocatori; k++) {
        const char* nome = Giocatore_Nome(&gioco->giocatori[k]);
        if (gioco->giocatori[k].is_bot && (strncmp(nome, "Bot_", 4) == 0 || nome[0] == '\0')) {
            Giocatore_SetNome(&gioco->giocatori[k], TextFormat("Bot_%d", b_idx++));
        }
    }
}

/**
 * @brief Disegna la waiting room multiplayer.
 * @ingroup multiplayer_lobby
 * @pre gioco != NULL.
 * @post Lobby renderizzata; logica avvio/attesa gestita.
 * @param session Sessione di gioco.
 * @param fase Fase corrente.
 * @param gioco Stato di gioco.
 */
void DisegnaWaitingRoom(GameSession* session, FaseApplicazione* fase, StatoGioco* gioco) {
    if (!gioco) return;
    
    // TL-01: Controlla PRIMA se l'host ha chiuso la stanza (CloseNetwork setta NET_OFFLINE,
    // quindi il controllo su currentRole di seguito non funzionerebbe).
    if (gioco->timer_notifica > 0.0f && gioco->gioco_finito &&
        (strcmp(gioco->messaggio, "L'host ha chiuso la stanza") == 0 ||
         strcmp(gioco->messaggio, "__HOST_DISCONNECT__") == 0)) {
        // Copia il messaggio nella notifica per il menu multiplayer
        strncpy(gioco->notifica, gioco->messaggio, sizeof(gioco->notifica) - 1);
        gioco->notifica[sizeof(gioco->notifica) - 1] = '\0';
        if (strcmp(gioco->notifica, "__HOST_DISCONNECT__") == 0) {
            strcpy(gioco->notifica, "L'host ha chiuso la stanza");
        }
        *fase = FASE_MULTIPLAYER_SCELTA;
        return;
    }
    
    /* BUG FIX (Riconnessione diretta): Se la partita e' gia' iniziata
     * (num_carte_scarti > 0) e abbiamo ricevuto il game state, vai
     * direttamente in FASE_GIOCO saltando la lobby, ma SOLO se la mano
     * del giocatore e' gia' stata popolata (PACKET_PLAYER_HAND arrivato).
     *
     * BUG ADMIN FIX 1 (Riconnessione senza mano): Il vecchio codice
     * controllava solo network_game_initialized + num_carte_scarti,
     * ma PACKET_GAME_STATE arriva PRIMA di PACKET_PLAYER_HAND,
     * quindi il client entrava in partita con mano VUOTA.
     * 
     * BUG ADMIN FIX 2 (Riconnessione funzionante): Tuttavia dobbiamo
     * ANCORA supportare reconnection senza network_start_game_flag,
     * per esempio se PACKET_START_GAME e' saltato. Quindi usiamo
     * AMBEDUE i percorsi: network_start_game_flag (veloce) OPPURE
     * game_state + mano popolata (fallback sicuro).
     *
     * BUG FIX 3 (Reset host_in_prelobby): Quando il client entra in
     * partita (riconnessione o inizio nuova partita), resetta
     * host_in_prelobby = 0 per permettere la visualizzazione della
     * classifica finale quando la partita finisce. */
    if (network_start_game_flag) { 
        network_start_game_flag = 0; 
        host_in_prelobby = 0;  // Reset: la partita è iniziata, non siamo più in lobby
        *fase = FASE_GIOCO; 
        return; 
    }
    if (network_game_initialized && gioco->num_carte_scarti > 0 && currentRole == NET_CLIENT) {
        /* BUG ADMIN FIX: Verifica che la mano del giocatore locale sia
         * effettivamente popolata (PACKET_PLAYER_HAND gia' arrivato).
         * Se e' ancora NULL o vuota, NON entrare in partita.
         * Questo evita la "partita fantasma" (mano vuota) ma permette
         * la riconnessione quando la mano e' gia' stata ricevuta. */
        if (local_player_id >= 0 && local_player_id < gioco->num_giocatori) {
            if (gioco->giocatori[local_player_id].mano && gioco->giocatori[local_player_id].mano->lunghezza > 0) {
                host_in_prelobby = 0;  // Reset: riconnessione diretta in partita
                *fase = FASE_GIOCO;
                return;
            }
        }
    }

    // BUG FIX 4 (HOST): Quando l'host arriva qui dalla schermata finale (fase == FASE_HOST_LOBBY)
    // con gioco_finito == 1 o gioco_finito == 0 ma host_in_prelobby appena attivato,
    // deve: pulire bot-sostituti, broadcastare host_return_lobby ai client, 
    // resettare gioco_finito e broadcastare stato aggiornato. Questo gestisce il caso
    // in cui l'host arriva da end_screen.c (Gioca Ancora) o da gameplay_draw.c.
    // TF-06: Pulisci i bot-sostituti quando si entra nella lobby dopo una partita
    static int gia_pulito = 0;
    // BUG FIX 4: MostraNotificaReset: resetta gia_pulito quando gioco_finito == 1
    // (la partita e' finita e stiamo per tornare in lobby). Questo garantisce che
    // la pulizia dei bot-sostituti venga eseguita ogni volta che si torna in lobby
    // dopo una partita, anche dopo la prima volta.
    if (currentRole == NET_HOST && gioco->gioco_finito) {
        gia_pulito = 0;
    }
    if (currentRole == NET_HOST && !gia_pulito && (gioco->gioco_finito || host_in_prelobby)) {
        PulisciBotSostituti(gioco);
        gia_pulito = 1;
        // TF-02 EXT: Quando l'host torna in pre-lobby (Gioca Ancora), 
        // segnala a tutti i client che l'host e' tornato in lobby,
        // cosi' i client cliccando "Gioca Ancora" possono riunirsi.
        BroadcastHostReturnLobby();
        host_in_prelobby = 1;
        gioco->gioco_finito = 0;
        gioco->num_carte_scarti = 0;
        BroadcastGameState(gioco);
    }

    DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.8f));
    DrawText("STANZA MULTIPLAYER", LARGHEZZA/2 - MeasureText("STANZA MULTIPLAYER", 40)/2, 50, 40, GOLD);
    DrawText(TextFormat("Codice Invito: %s", session_id), LARGHEZZA/2 - MeasureText("Codice Invito: XXXXXXXX", 24)/2, 120, 24, YELLOW);

    int boxWidth = 600, boxHeight = 80, spacing = 15;
    int startX = LARGHEZZA/2 - boxWidth/2, startY = 170;
    int azione_effettuata = 0;

    /* Reset avviso quando ci sono abbastanza giocatori */
    if (gioco->num_giocatori >= 2) timerErroreAvvio = 0.0f;

    for (int i = 0; i < gioco->num_giocatori; i++) {
        Rectangle playerBox = { startX, startY + i * (boxHeight + spacing), (float)boxWidth, (float)boxHeight };
        if (gioco->giocatori[i].is_bot) {
            DrawRectangleRounded(playerBox, 0.2f, 8, ORANGE);
            DrawText(Giocatore_Nome(&gioco->giocatori[i]), playerBox.x + 30, playerBox.y + 25, 30, WHITE);
            if (currentRole == NET_HOST && !azione_effettuata && DisegnaBottoneMenuAnimato((Rectangle){ playerBox.x + boxWidth - 160, playerBox.y + 20, 140, 40 }, "Rimuovi")) {
                azione_effettuata = 1;
                if (gioco->giocatori[i].mano) EliminaLista(gioco->giocatori[i].mano);
                ShiftNetworkSockets(i, gioco->num_giocatori);
                for (int k = i; k < gioco->num_giocatori - 1; k++) gioco->giocatori[k] = gioco->giocatori[k+1];
                gioco->giocatori[gioco->num_giocatori - 1].mano = NULL;
                memset(&gioco->giocatori[gioco->num_giocatori - 1].info, 0, sizeof(Utente));
                gioco->giocatori[gioco->num_giocatori - 1].is_bot = 0;
                gioco->giocatori[gioco->num_giocatori - 1].stato_pronto = 0;
                gioco->giocatori[gioco->num_giocatori - 1].num_carte_mano = 0;
                gioco->giocatori[gioco->num_giocatori - 1].socket_id = -1;
                gioco->num_giocatori--;
                int b_idx = 1;
                for (int k = 0; k < gioco->num_giocatori; k++) {
                    // NON rinominare i bot che hanno un nome utente reale (client che si è disconnesso)
                    // Rinomina solo i bot_originali che già iniziano con "Bot_"
                    const char* nome = Giocatore_Nome(&gioco->giocatori[k]);
                    if (gioco->giocatori[k].is_bot && (strncmp(nome, "Bot_", 4) == 0 || nome[0] == '\0')) {
                        Giocatore_SetNome(&gioco->giocatori[k], TextFormat("Bot_%d", b_idx++));
                    }
                }
                BroadcastGameState(gioco);
                break;
            }
        } else {
            DrawRectangleRounded(playerBox, 0.2f, 8, GREEN);
            DrawText(TextFormat("P%d", i+1), playerBox.x + 30, playerBox.y + 25, 30, WHITE);
            DrawText(Giocatore_Nome(&gioco->giocatori[i]), playerBox.x + 100, playerBox.y + 32, 20, WHITE);
        }
        if (i == local_player_id) DrawText("(Tu)", playerBox.x + boxWidth - 80, playerBox.y + 30, 20, LIGHTGRAY);
        else if (i == 0) DrawText("(Host)", playerBox.x + boxWidth - 100, playerBox.y + 30, 20, LIGHTGRAY);
        else if (!gioco->giocatori[i].is_bot) DrawText("(Client)", playerBox.x + boxWidth - 110, playerBox.y + 30, 20, LIGHTGRAY);
    }

    if (gioco->num_giocatori < 4) {
        int i = gioco->num_giocatori;
        Rectangle extraBox = { startX, startY + i * (boxHeight + spacing), (float)boxWidth, (float)boxHeight };
        DrawRectangleRounded(extraBox, 0.2f, 8, Fade(GRAY, 0.5f));
        if (currentRole == NET_HOST) {
            DrawText("IN ATTESA DI GIOCATORI...", extraBox.x + 30, extraBox.y + 25, 22, LIGHTGRAY);
                if (!azione_effettuata && DisegnaBottoneMenuAnimato((Rectangle){ extraBox.x + boxWidth - 180, extraBox.y + 15, 160, 50 }, "+ BOT")) {
                    azione_effettuata = 1;
                    Giocatore_SetNome(&gioco->giocatori[i], "Bot");
                    gioco->giocatori[i].is_bot = 1;
                    gioco->giocatori[i].stato_pronto = 1;
                    gioco->giocatori[i].socket_id = -1;
                    gioco->giocatori[i].num_carte_mano = 0;
                    gioco->giocatori[i].era_in_prelobby = 0; // I bot originali non sono riconnettibili
                    if (!gioco->giocatori[i].mano) gioco->giocatori[i].mano = CreaLista();
                    gioco->num_giocatori++;
                    int b_idx = 1;
                    for (int k = 0; k < gioco->num_giocatori; k++) {
                        if (gioco->giocatori[k].is_bot) Giocatore_SetNome(&gioco->giocatori[k], TextFormat("Bot_%d", b_idx++));
                    }
                    BroadcastGameState(gioco);
                }
        } else {
            DrawText("IN ATTESA DI NUOVI GIOCATORI...", extraBox.x + 30, extraBox.y + 25, 22, LIGHTGRAY);
        }
    }

    int buttonY = 620;
    if (currentRole == NET_HOST) {
        if (timerErroreAvvio > 0.0f) timerErroreAvvio -= GetFrameTime();
        // TL-01: Quando l'host preme "Esci", avvisa tutti i client prima di chiudere
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 220, buttonY, 200, 50 }, "ESCI")) { 
            BroadcastHostDisconnect();
            CloseNetwork(); 
            *fase = FASE_MENU; 
        }
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 20, buttonY, 200, 50 }, "AVVIA")) {
            if (gioco->num_giocatori >= 2) {
                // RICHIESTA 3: Segna tutti i giocatori presenti in prelobby come riconnettibili
                for (int p = 0; p < gioco->num_giocatori; p++) {
                    gioco->giocatori[p].era_in_prelobby = 1;
                }
                gioco->turno_corrente = 0; gioco->direzione = 1; gioco->gioco_finito = 0;
                gioco->deve_chiamare_uno = 0; gioco->id_carta_in_trascinamento = -1; gioco->in_scelta_colore = 0;
                InizializzaMazzo(gioco);
                for (int p = 0; p < gioco->num_giocatori; p++) {
                    if (gioco->giocatori[p].mano) EliminaLista(gioco->giocatori[p].mano);
                    gioco->giocatori[p].mano = CreaLista();
                    Pesca(gioco, p, 7);
                }
                do { gioco->scarti[0] = gioco->mazzo[--gioco->num_carte_mazzo]; }
                while (gioco->scarti[0].colore == NERO || gioco->scarti[0].tipo > NUM_9);
                gioco->num_carte_scarti = 1;
                gioco->colore_attivo = gioco->scarti[0].colore;

                // ORDINE CORRETTO:
                // 1. Broadcast stato di gioco (invia mazzo, scarti, turni, ecc.)
                BroadcastGameState(gioco);
                // 2. Broadcast mani dei giocatori (invia le carte a ciascun client)
                BroadcastPlayerHands(gioco);
                // 3. Segnala ai client di iniziare la partita (DOPO stato e mani)
                SendStartGame();

                *fase = FASE_GIOCO;
            } else { timerErroreAvvio = 3.0f; }
        }
        if (timerErroreAvvio > 0.0f) {
            DrawText("Servono almeno 2 giocatori!", LARGHEZZA/2 - MeasureText("Servono almeno 2 giocatori!", 18)/2, buttonY - 45, 18, RED);
        }
    } else if (currentRole == NET_CLIENT) {
        // TL-01: Rileva host disconnessione. Reindirizza al menu multiplayer.
        if (gioco->timer_notifica > 0.0f && (
            strcmp(gioco->messaggio, "L'host ha chiuso la stanza") == 0 ||
            strcmp(gioco->messaggio, "__HOST_DISCONNECT__") == 0)) {
            *fase = FASE_MULTIPLAYER_SCELTA;
            return;
        }
        // Client normale: attesa
        DrawText("In attesa che l'Host avvii...", LARGHEZZA/2 - MeasureText("In attesa che l'Host avvii...", 20)/2, buttonY - 30, 20, LIGHTGRAY);
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 140, buttonY, 280, 50 }, "ESCI (Torna al menu)")) { CloseNetwork(); *fase = FASE_MENU; }
    }
}