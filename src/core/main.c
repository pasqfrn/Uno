/**
 * @file main.c
 * @brief Punto di ingresso dell'applicazione UNO Multiplayer.
 * @defgroup main Entry Point
 * @brief Inizializza tutti i moduli, gestisce il loop principale e coordina
 *        la sincronizzazione LAN e il multiplayer.
 *
 * @details Questo modulo rappresenta il cuore dell'applicazione. Responsabile di:
 *          - Inizializzazione di Raylib (finestra 1280x720, textures, assets)
 *          - Caricamento del database utenti locale (data/users/users.dat)
 *          - Inizializzazione rete multiplayer (TCP socket + UDP discovery)
 *          - Inizializzazione sincronizzazione LAN database (UDP broadcast 27017)
 *          - Macchina a stati principale (FaseApplicazione enum)
 *          - Loop di gioco a 60 FPS con gestione input, update e rendering
 *          - Countdown multiplayer con avvio automatico partita
 *          - Gestione navigazione globale (ESC per tornare indietro)
 *          - Chiusura ordinata risorse (salvataggio DB, chiusura socket, cleanup)
 *
 * @note La sincronizzazione LAN permette a tutti i dispositivi sulla stessa rete
 *       di condividere automaticamente il database utenti, garantendo che
 *       l'admin panel su ogni dispositivo vedi tutti gli utenti e prevenendo
 *       la creazione di username/email duplicati.
 *
 * Moduli coordinati:
 * - Raylib (window, textures, input)
 * - Database utenti (caricamento/salvataggio binario)
 * - Rete multiplayer (TCP 27015 per gioco, UDP 27016 per discovery)
 * - Sincronizzazione LAN (UDP broadcast 27017 per DB utenti)
 *
 * @author Pasquale Franco
 * @version 9.1.3
 * @date 2026-07-01
 *
 * @pre Nessun prerequisito.
 * @post Inizializzazione completata; loop principale avviato a 60 FPS.
 * @return Codice di uscita (0 = successo, diverso da 0 = errore).
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"

// Inclusione Librerie Modulari
#include "../../lib/data_structures/data_structures.h"
#include "../../lib/auth/auth.h"
#include "../../lib/game/game_logic.h"
#include "../../lib/screens/ui.h"
#include "../../lib/socket/network/network.h"
#include "../../lib/socket/lan/lan_sync.h"

// Inclusione Schermate (struttura a sottocartelle)
#include "../../lib/screens/auth/auth_screen.h"
#include "../../lib/screens/menu/menu_screen.h"
#include "../../lib/screens/game/gameplay_screen.h"
#include "../../lib/screens/game/end_screen.h"
#include "../../lib/screens/multiplayer/multiplayer_screen.h"
#include "../../lib/screens/menu/stats_screen.h"
#include "../../lib/screens/multiplayer/multiplayer_lobby.h"

// Dichiarazione esterna variabile esc_consumato (usata in auth_screen.c e gameplay_screen.c)
int esc_consumato = 0;

/**
 * @brief Punto di ingresso dell'applicazione.
 *
 * Inizializza Raylib, carica texture e database, avvia rete e
 * sincronizzazione LAN, poi entra nel loop principale.
 *
 * @pre Nessuna.
 * @post Inizializzazione completata; loop principale avviato.
 * @return Codice di uscita (0 = successo).
 */
int main(void) {
    srand(time(NULL));
    const int LARGHEZZA = 1280;
    const int ALTEZZA = 720;

    InitWindow(LARGHEZZA, ALTEZZA, "UNO - Secure Multi-Profile Edition");
    SetTargetFPS(60);

    // Disabilita chiusura automatica con ESC
    SetExitKey(KEY_NULL);

    CaricaTextureReali();
    CaricaDB();  // Carica DB locale dal file

    // Inizializza rete multiplayer
    if (!InitNetwork()) {
        printf("Attenzione: Impossibile inizializzare la rete. Multiplayer non disponibile.\n");
    }

    // Inizializza sincronizzazione LAN del database utenti
    // (UDP broadcast su porta 27017 per condividere utenti tra dispositivi)
    int lanSyncOk = LAN_Init();
    if (!lanSyncOk) {
        printf("Attenzione: Impossibile inizializzare la sincronizzazione LAN.\n");
    }

    StatoGioco gioco = {0};
    FaseApplicazione fase = FASE_SCELTA_ACCESSO;

    float network_countdown = 0.0f;
    int network_countdown_active = 0;

    // Flag per uscita sicura
    int chiudi_gioco = 0;

    while (!WindowShouldClose() && !chiudi_gioco) {
        Vector2 mousePos = GetMousePosition();
        float dt = GetFrameTime();

        esc_consumato = 0;

        // ==========================================
        // 0. GESTIONE NAVIGAZIONE CON TASTO ESC
        // ==========================================
        if (IsKeyPressed(KEY_ESCAPE)) {
            switch (fase) {
                case FASE_LOGIN:
                case FASE_REGISTRAZIONE:
                    fase = FASE_SCELTA_ACCESSO;
                    esc_consumato = 1;
                    break;
                case FASE_RECUPERO_PASSWORD:
                    fase = FASE_LOGIN;
                    esc_consumato = 1;
                    break;
                case FASE_PROFILO_VISUALIZZA:
                case FASE_STATISTICHE:
                case FASE_ADMIN_PANEL:
                    fase = FASE_MENU;
                    esc_consumato = 1;
                    break;
                case FASE_PROFILO_MODIFICA:
                    fase = FASE_PROFILO_VISUALIZZA;
                    esc_consumato = 1;
                    break;
                case FASE_ADMIN_MODIFICA_UTENTE:
                    fase = FASE_ADMIN_PANEL;
                    esc_consumato = 1;
                    break;
                case FASE_MULTIPLAYER_SCELTA:
                    fase = FASE_MENU;
                    esc_consumato = 1;
                    break;
                case FASE_CLIENT_JOIN:
                case FASE_HOST_LOBBY:
                case FASE_CLIENT_LOBBY:
                    if (fase == FASE_HOST_LOBBY || fase == FASE_CLIENT_LOBBY) {
                        CloseNetwork();
                    }
                    // Protezione: resetta il countdown multiplayer quando si esce dalla lobby
                    network_countdown_active = 0;
                    network_countdown = 0.0f;
                    network_start_game_flag = 0;
                    fase = (fase == FASE_CLIENT_JOIN) ? FASE_MULTIPLAYER_SCELTA : FASE_MENU;
                    esc_consumato = 1;
                    break;
                case FASE_FINE_PARTITA:
                    fase = FASE_MENU;
                    esc_consumato = 1;
                    break;
                case FASE_SCELTA_ACCESSO:
                case FASE_MENU:
                case FASE_GIOCO:
                    // L'uscita e' gestita dalle relative schermate
                    break;
            }
        }

        // ==========================================
        // 1. GESTIONE RETE E SINCRONIZZAZIONE LAN
        // ==========================================
        if (currentRole != NET_OFFLINE) {
            UpdateNetwork(&gioco);
        }

        // Sincronizzazione LAN del database utenti (UDP broadcast)
        // Funziona SEMPRE, indipendentemente dalla modalita' multiplayer.
        // Sincronizza gli utenti tra TUTTI i dispositivi sulla stessa LAN.
        LAN_Update(dt);

        if (network_countdown_active) {
            // Gestione sicura: se la fase corrente non è più una fase multiplayer,
            // il countdown viene resettato per evitare avvii non intenzionali
            if (fase != FASE_MULTIPLAYER_SCELTA && 
                fase != FASE_CLIENT_JOIN && 
                fase != FASE_HOST_LOBBY && 
                fase != FASE_CLIENT_LOBBY) {
                network_countdown_active = 0;
                network_countdown = 0.0f;
                network_start_game_flag = 0;
            } else {
                network_countdown -= dt;
                if (network_countdown <= 0.0f) {
                    network_countdown_active = 0;
                    network_start_game_flag = 0;
                    fase = FASE_GIOCO;
                }
            }
        }

        if (gioco.timer_notifica > 0) gioco.timer_notifica -= dt;

        // ==========================================
        // 2. LOGICA DELLE SCHERMATE
        // ==========================================
        if (fase == FASE_REGISTRAZIONE || fase == FASE_LOGIN || fase == FASE_RECUPERO_PASSWORD ||
            fase == FASE_PROFILO_VISUALIZZA || fase == FASE_PROFILO_MODIFICA ||
            fase == FASE_ADMIN_MODIFICA_UTENTE || fase == FASE_ADMIN_PANEL) {
            AggiornaAuth(&fase);
        }
        else if (fase == FASE_GIOCO) {
            AggiornaGameplay(&gioco, &fase, mousePos, dt);
        }

        // ==========================================
        // 3. RENDERING DELLE SCHERMATE
        // ==========================================
        BeginDrawing();
        ClearBackground(BLACK);

        // Sfondo condiviso
        DrawTexturePro(textureSfondo,
                       (Rectangle){ 0.0f, 0.0f, (float)textureSfondo.width, (float)textureSfondo.height },
                       (Rectangle){ 0.0f, 0.0f, (float)LARGHEZZA, (float)ALTEZZA },
                       (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);

        // Smistamento rendering in base alla fase
        if (fase == FASE_SCELTA_ACCESSO || fase == FASE_REGISTRAZIONE || fase == FASE_LOGIN ||
            fase == FASE_RECUPERO_PASSWORD || fase == FASE_PROFILO_VISUALIZZA ||
            fase == FASE_PROFILO_MODIFICA || fase == FASE_ADMIN_MODIFICA_UTENTE ||
            fase == FASE_ADMIN_PANEL) {
            if (DisegnaAuth(&fase) == 1) {
                chiudi_gioco = 1;
            }
        }
        else if (fase == FASE_MENU) {
            DisegnaMenu(&gioco, &fase);
        }
        else if (fase == FASE_STATISTICHE) {
            DisegnaStatistiche(&fase);
        }
        else if (fase == FASE_MULTIPLAYER_SCELTA || fase == FASE_CLIENT_JOIN ||
                 fase == FASE_HOST_LOBBY || fase == FASE_CLIENT_LOBBY) {
            DisegnaMultiplayer(&gioco, &fase, &network_countdown, &network_countdown_active);
        }
        else if (fase == FASE_GIOCO) {
            DisegnaGameplay(&gioco, &fase, mousePos);
        }
        else if (fase == FASE_FINE_PARTITA) {
            DisegnaFinePartita(&gioco, &fase, mousePos);
        }

        // Overlay globale: countdown inizio partita
        if (network_countdown_active) {
            DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.75f));
            int seconds = (int)(network_countdown + 0.999f);
            DrawText(TextFormat("LA PARTITA INIZIERA TRA %d...", seconds),
                     LARGHEZZA/2 - MeasureText(TextFormat("LA PARTITA INIZIERA TRA %d...", seconds), 40)/2,
                     ALTEZZA/2 - 20, 40, WHITE);
        }

        EndDrawing();
    }

    // Chiusura ordinata delle risorse
    // BUG FIX: Salva il database utenti su disco prima di chiudere.
    // I dati vengono scritti in data/users/users.dat.
    if (id_utente_corrente >= 0) {
        SalvaDB();
    }
    LAN_Close();    // Chiude socket sincronizzazione LAN
    CloseNetwork(); // Chiude socket multiplayer
    ScaricaTextureReali();
    CloseWindow();
    return 0;
}