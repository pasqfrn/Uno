/**
 * @file multiplayer_screen.c
 * @brief Schermata di selezione multiplayer (host/client).
 * @ingroup multiplayer_screen
 *
 * Gestisce la creazione di stanze, l'unione tramite codice,
 * e la risoluzione IP/codice.
 *
 * NOTA: Il codice stanza e' di 8 caratteri esadecimali (IP in hex).
 * Ogni dispositivo sulla LAN ha il proprio IP LAN unico, quindi
 * il codice generato e' gia' sufficientemente unico.
 *
 * La sincronizzazione del database utenti tra dispositivi LAN
 * e' gestita separatamente dal modulo LAN Sync (UDP broadcast).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "raylib.h"
#include "../../../lib/screens/multiplayer/multiplayer_screen.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/ui.h"
#include "../../../lib/auth.h"
#include "../../../lib/game_logic.h"
#include "../../../lib/list.h"
#include "../../../lib/screens/multiplayer/multiplayer_lobby.h"

#define LARGHEZZA 1280
#define ALTEZZA 720

/* ============================================================
 *  VARIABILI DI STATO INTERNE (multiplayer_screen.c)
 * ============================================================ */

/** @brief Buffer per il codice stanza (8 caratteri hex + terminatore). */
static char inputCode[9] = "";
/** @brief Contatore caratteri inseriti nel codice stanza. */
static int codeLetterCount = 0;
/** @brief Buffer per l'IP locale del dispositivo (es. "192.168.1.100"). */
static char localIP[32] = "127.0.0.1";
/** @brief Codice stanza generato dall'IP locale (8 char hex). */
static char roomCode[9] = "ERRORE";
/** @brief Porta TCP per le connessioni multiplayer. */
static int gamePort = 27015;
/** @brief Flag errore caricamento:
 *        0 = nessun errore,
 *        1 = errore connessione,
 *        2 = codice stanza non valido. */
static int mostraErroreCaricamento = 0;
/** @brief Flag: 1 se l'IP locale e' stato gia' inizializzato. */
static int ip_inizializzato = 0;

/* ============================================================
 *  FUNZIONI HELPER
 * ============================================================ */

/**
 * @brief Resetta l'errore di caricamento multiplayer.
 * @ingroup multiplayer_screen
 */
void ResetMultiplayerError(void) { mostraErroreCaricamento = 0; }

/* ============================================================
 *  RENDERING SCHERMATA MULTIPLAYER
 * ============================================================ */

/**
 * @brief Disegna la schermata di selezione multiplayer.
 * @ingroup multiplayer_screen
 *
 * Gestisce tre fasi:
 *   - FASE_MULTIPLAYER_SCELTA: bottoni "Crea Stanza" e "Unisciti"
 *   - FASE_CLIENT_JOIN: inserimento codice stanza
 *   - FASE_HOST_LOBBY / FASE_CLIENT_LOBBY: waiting room (delegata)
 *
 * @param gioco Puntatore allo stato di gioco.
 * @param fase Puntatore alla fase applicazione.
 * @param network_countdown Puntatore al timer countdown inizio partita.
 * @param network_countdown_active Puntatore a flag countdown attivo.
 */
void DisegnaMultiplayer(StatoGioco* gioco, FaseApplicazione* fase, float* network_countdown, int* network_countdown_active) {
    if (!ip_inizializzato) {
        if (GetLocalIPAddress(localIP, sizeof(localIP))) {
            IpToCode(localIP, roomCode);
            if ((currentRole == NET_OFFLINE || currentRole == NET_HOST) && session_id[0] == '\0') {
                strcpy(session_id, roomCode);
            }
        }
        ip_inizializzato = 1;
    }

    if (*fase == FASE_MULTIPLAYER_SCELTA) {
        DrawRectangle(0,0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.8f));
        DrawText("MODALITA MULTIPLAYER", LARGHEZZA/2 - MeasureText("MODALITA MULTIPLAYER", 40)/2, 100, 40, GOLD);

        /* Bottoni principali: crea stanza o unisciti */
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 150, 250, 300, 60 }, "Crea Stanza (Host)")) {
            if (HostGame(gamePort)) {
                ResetTimerErroreAvvio();
                *fase = FASE_HOST_LOBBY;
                memset(gioco, 0, sizeof(StatoGioco));
                gioco->num_giocatori = 1;
                gioco->direzione = 1;
                gioco->gioco_finito = 0;
                gioco->id_carta_in_trascinamento = -1;
                gioco->slot_salvataggio = 0;
                strcpy(gioco->giocatori[0].nome, dbUtenti.lista[id_utente_corrente].username);
                gioco->giocatori[0].is_bot = 0;
                gioco->giocatori[0].stato_pronto = 1;
                gioco->giocatori[0].mano = CreaLista();
            } else { mostraErroreCaricamento = 1; }
        }
        /* Passa alla schermata di inserimento codice */
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 150, 330, 300, 60 }, "Unisciti tramite Codice")) {
            *fase = FASE_CLIENT_JOIN;
            mostraErroreCaricamento = 0;
            memset(inputCode, 0, sizeof(inputCode));
            codeLetterCount = 0;
            gioco->num_chat_msg = 0;
            for(int i=0; i<15; i++) gioco->chat_history[i][0] = '\0';
        }
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 100, 500, 200, 60 }, "Indietro")) *fase = FASE_MENU;
        if (mostraErroreCaricamento) DrawText("Errore: Impossibile creare la stanza", LARGHEZZA/2 - MeasureText("Errore: Impossibile creare la stanza", 20)/2, 600, 20, RED);

        // TL-01: Notifica host disconnesso (riquadro nero in basso a destra, DOPO lo sfondo)
        if (gioco->timer_notifica > 0.0f && gioco->notifica[0] != '\0') {
            int notifW = MeasureText(gioco->notifica, 18) + 40;
            DrawRectangleRounded((Rectangle){ LARGHEZZA - notifW - 20, ALTEZZA - 80, (float)notifW, 60 }, 0.3f, 10, Fade(BLACK, 0.85f));
            DrawText(gioco->notifica, LARGHEZZA - notifW, ALTEZZA - 60, 18, WHITE);
        }
    }
    else if (*fase == FASE_CLIENT_JOIN) {
        DrawRectangle(0,0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.8f));
        DrawText("UNISCITI A UNA STANZA", LARGHEZZA/2 - MeasureText("UNISCITI A UNA STANZA", 40)/2, 100, 40, GOLD);
        DrawText("Inserisci il Codice Stanza:", LARGHEZZA/2 - MeasureText("Inserisci il Codice Stanza:", 20)/2, 200, 20, LIGHTGRAY);

        int key = GetCharPressed();
        while (key > 0) {
            if (((key >= '0' && key <= '9') || (key >= 'A' && key <= 'F') || (key >= 'a' && key <= 'f')) && (codeLetterCount < 8)) {
                if (key >= 'a' && key <= 'f') key -= 32;
                inputCode[codeLetterCount] = (char)key;
                inputCode[codeLetterCount+1] = '\0';
                codeLetterCount++;
            }
            key = GetCharPressed();
        }
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && codeLetterCount > 0) {
            codeLetterCount--;
            inputCode[codeLetterCount] = '\0';
        }

        Rectangle box = { LARGHEZZA/2 - 150, 250, 300, 50 };
        DrawRectangleRounded(box, 0.2f, 10, Fade(WHITE, 0.9f));
        DrawText(inputCode, box.x + 15, box.y + 15, 20, BLACK);
        if ((int)(GetTime() * 2) % 2 == 0) DrawText("_", box.x + 15 + MeasureText(inputCode, 20), box.y + 15, 20, MAROON);

        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 150, 350, 300, 60 }, "Connetti")) {
            char targetIP[32];
            if (CodeToIp(inputCode, targetIP)) {
                if (JoinGame(targetIP, gamePort)) {
                    *fase = FASE_CLIENT_LOBBY;
                    strcpy(session_id, inputCode);
                    
                    memset(gioco, 0, sizeof(StatoGioco));
                    gioco->num_giocatori = 1;
                    gioco->direzione = 1;
                    gioco->gioco_finito = 0;
                    gioco->id_carta_in_trascinamento = -1;
                    gioco->slot_salvataggio = 0;
                    strcpy(gioco->giocatori[0].nome, dbUtenti.lista[id_utente_corrente].username);
                    gioco->giocatori[0].is_bot = 0;
                    gioco->giocatori[0].stato_pronto = 1;
                    gioco->giocatori[0].mano = CreaLista();
                    gioco->giocatori[0].socket_id = -1;
                    for (int i = 1; i < 4; i++) {
                        gioco->giocatori[i].mano = NULL;
                        gioco->giocatori[i].is_bot = 0;
                        gioco->giocatori[i].stato_pronto = 0;
                        gioco->giocatori[i].socket_id = -1;
                    }
                    
                    SendJoinGame(dbUtenti.lista[id_utente_corrente].username);
                    {
                        AdminUserSyncData userData = {0};
                        Statistiche* s = &dbUtenti.lista[id_utente_corrente];
                        strncpy(userData.email, s->email, sizeof(userData.email) - 1);
                        strncpy(userData.nome, s->nome, sizeof(userData.nome) - 1);
                        strncpy(userData.cognome, s->cognome, sizeof(userData.cognome) - 1);
                        strncpy(userData.username, s->username, sizeof(userData.username) - 1);
                        strncpy(userData.password, s->password, sizeof(userData.password) - 1);
                        strncpy(userData.pin_recupero, s->pin_recupero, sizeof(userData.pin_recupero) - 1);
                        userData.partite_giocate = s->partite_giocate;
                        userData.vittorie_giocatore = s->vittorie_giocatore;
                        userData.vittorie_bot = s->vittorie_bot;
                        SendAdminUserSync(&userData);
                    }
                } else mostraErroreCaricamento = 1;
            } else mostraErroreCaricamento = 2;
        }
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 100, 500, 200, 60 }, "Indietro")) {
            *fase = FASE_MULTIPLAYER_SCELTA;
            ResetMultiplayerError();
        }
        if (mostraErroreCaricamento == 1) DrawText("Errore di connessione", LARGHEZZA/2 - MeasureText("Errore di connessione", 20)/2, 600, 20, RED);
        if (mostraErroreCaricamento == 2) DrawText("Codice Stanza non valido!", LARGHEZZA/2 - MeasureText("Codice Stanza non valido!", 20)/2, 600, 20, RED);
    }
    else if (*fase == FASE_HOST_LOBBY || *fase == FASE_CLIENT_LOBBY) {
        // Controlla se il client è stato rifiutato
        if (join_rejected) {
            // Mostra notifica e torna automaticamente al menu multiplayer
            // BUG FIX: Copia anche in notifica per la visualizzazione nel menu multiplayer
            strcpy(gioco->messaggio, reject_message);
            strcpy(gioco->notifica, reject_message);
            gioco->timer_notifica = 3.0f;
            *fase = FASE_MULTIPLAYER_SCELTA;
            join_rejected = 0;
        } else {
            DisegnaWaitingRoom(NULL, fase, gioco);
        }
    }
}