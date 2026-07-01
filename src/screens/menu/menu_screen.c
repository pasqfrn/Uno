/**
 * @file menu_screen.c
 * @brief Implementazione schermata menu principale.
 * @ingroup menu_screen
 *
 * Gestisce il menu principale con:
 *   - Navigazione tra profili utente
 *   - Accesso alle funzionalita' admin
 *   - Avvio partite offline (vs bot)
 *   - Accesso alle statistiche
 *   - Gestione modalita' multiplayer
 *
 * Stati gestiti: FASE_SCELTA_ACCESSO, FASE_MENU
 */
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "../../../lib/screens/menu/menu_screen.h"
#include "../../../lib/auth.h"
#include "../../../lib/ui.h"
#include "../../../lib/game_logic.h"
#include "../../../lib/socket/network/network.h"

#define LARGHEZZA 1280
#define ALTEZZA 720

/* ============================================================
 *  VARIABILI DI STATO INTERNE (statiche alla schermata menu)
 * ============================================================ */

/** @brief Flag: 1 se mostrare errore "Nessun salvataggio trovato". */
static int mostraErroreMenu = 0;
/** @brief Flag: 1 se il sottomenu di caricamento salvataggi e' attivo. */
static int sottomenu_salvataggi = 0;
/** @brief Flag: 1 se il popup di conferma logout e' visibile. */
static int popup_logout_attivo = 0;
/** @brief Flag: 1 se la compattazione salvataggi e' gia' stata eseguita. */
static int compattazione_fatta = 0;

/* Variabile esterna dal main: indica se ESC e' stato gia' consumato. */
extern int esc_consumato;

/* ============================================================
 *  RENDERING MENU PRINCIPALE
 * ============================================================ */

/**
 * @brief Disegna la schermata del menu principale post-login.
 *
 * Gestisce tutti i bottoni e sottomenu:
 *   - Avvio partita 1v1, 1v2, 1v3
 *   - Accesso multiplayer (lobby)
 *   - Caricamento partita (sottomenu salvataggi)
 *   - Visualizzazione profilo e statistiche
 *   - Admin Panel (solo per utente "Admin")
 *   - Logout con conferma
 *
 * Gestisce anche le notifiche di sistema (es. host disconnesso).
 *
 * @param gioco Puntatore allo stato di gioco corrente.
 * @param fase Puntatore alla fase applicazione corrente.
 */
void DisegnaMenu(StatoGioco* gioco, FaseApplicazione* fase) {
    // TL-01: Copia il messaggio nella notifica se è host disconnect
    if (gioco->gioco_finito && gioco->timer_notifica > 0.0f &&
        (strcmp(gioco->messaggio, "L'host ha chiuso la stanza") == 0 ||
         strcmp(gioco->messaggio, "__HOST_DISCONNECT__") == 0)) {
        strncpy(gioco->notifica, gioco->messaggio, sizeof(gioco->notifica) - 1);
        gioco->notifica[sizeof(gioco->notifica) - 1] = '\0';
        // Se è "__HOST_DISCONNECT__", sostituisci con testo leggibile
        if (strcmp(gioco->notifica, "__HOST_DISCONNECT__") == 0) {
            strcpy(gioco->notifica, "L'host ha chiuso la stanza");
        }
        gioco->gioco_finito = 0;
    }

    if (*fase == FASE_MENU) {
        DrawRectangle(0,0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.6f));
        DrawText("UNO", LARGHEZZA/2 - MeasureText("UNO", 100)/2, 80, 100, GOLD);

        if (!esc_consumato && IsKeyPressed(KEY_ESCAPE)) {
            if (popup_logout_attivo) { popup_logout_attivo = 0; }
            else if (sottomenu_salvataggi == 1) { sottomenu_salvataggi = 0; }
            else { popup_logout_attivo = 1; }
        }

        if (sottomenu_salvataggi == 0) {
            const char* msg = nuovo_utente ? "Benvenuto" : "Bentornato";
            DrawText(TextFormat("%s, %s!", msg, dbUtenti.lista[id_utente_corrente].username),
                     LARGHEZZA/2 - MeasureText(TextFormat("%s, %s!", msg, dbUtenti.lista[id_utente_corrente].username), 24)/2, 190, 24, LIGHTGRAY);

            DrawText("GIOCATORE SINGOLO", LARGHEZZA/2 - MeasureText("GIOCATORE SINGOLO", 20)/2, 250, 20, WHITE);
            if (!popup_logout_attivo && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 200, 280, 120, 50 }, "1 vs 1")) { NuovaPartita(gioco, 1); *fase = FASE_GIOCO; mostraErroreMenu = 0; }
            if (!popup_logout_attivo && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 60, 280, 120, 50 }, "1 vs 2")) { NuovaPartita(gioco, 2); *fase = FASE_GIOCO; mostraErroreMenu = 0; }
            if (!popup_logout_attivo && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 80, 280, 120, 50 }, "1 vs 3")) { NuovaPartita(gioco, 3); *fase = FASE_GIOCO; mostraErroreMenu = 0; }

            if (!popup_logout_attivo && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 150, 360, 300, 60 }, "Multiplayer (Lobby)")) {
                *fase = FASE_MULTIPLAYER_SCELTA;
                mostraErroreMenu = 0;
            }
            if (!popup_logout_attivo && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 150, 440, 300, 60 }, "Carica Partita")) {
                if (currentRole != NET_OFFLINE) { mostraErroreMenu = 2; }
                else { sottomenu_salvataggi = 1; mostraErroreMenu = 0; }
            }
            if (!popup_logout_attivo && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 150, 520, 140, 60 }, "Profilo")) {
                strcpy(inputEmail, dbUtenti.lista[id_utente_corrente].email);
                emailCount = strlen(inputEmail);
                strcpy(inputNome, dbUtenti.lista[id_utente_corrente].nome);
                nomeCount = strlen(inputNome);
                strcpy(inputCognome, dbUtenti.lista[id_utente_corrente].cognome);
                cognomeCount = strlen(inputCognome);
                memset(inputPassword, 0, 30);
                passCount = 0;
                campo_focusato = 0;
                *fase = FASE_PROFILO_VISUALIZZA;
            }
            if (!popup_logout_attivo && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 10, 520, 140, 60 }, "Statistiche")) *fase = FASE_STATISTICHE;

            if (strcmp(dbUtenti.lista[id_utente_corrente].username, "Admin") == 0) {
                if (!popup_logout_attivo && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 150, 590, 300, 50 }, "Admin Panel")) *fase = FASE_ADMIN_PANEL;
                if (!popup_logout_attivo && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 150, 650, 300, 50 }, "Disconnetti")) { popup_logout_attivo = 1; }
            } else {
                if (!popup_logout_attivo && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 150, 600, 300, 60 }, "Disconnetti")) { popup_logout_attivo = 1; }
            }

            if (mostraErroreMenu == 1) DrawText("Nessun salvataggio trovato!", LARGHEZZA/2 - MeasureText("Nessun salvataggio trovato!", 20)/2, 680, 20, RED);
            if (mostraErroreMenu == 2) DrawText("Impossibile caricare in Multiplayer!", LARGHEZZA/2 - MeasureText("Impossibile caricare in Multiplayer!", 20)/2, 680, 20, RED);

            if (popup_logout_attivo) {
                DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.8f));
                DrawRectangleRounded((Rectangle){ LARGHEZZA/2 - 270, ALTEZZA/2 - 120, 540, 240 }, 0.1f, 10, Fade(DARKGRAY, 0.95f));
                DrawRectangleRoundedLines((Rectangle){ LARGHEZZA/2 - 270, ALTEZZA/2 - 120, 540, 240 }, 0.1f, 10, GOLD);
                DrawText("SEI SICURO DI VOLERTI", LARGHEZZA/2 - MeasureText("SEI SICURO DI VOLERTI", 24)/2, ALTEZZA/2 - 70, 24, WHITE);
                DrawText("DISCONNETTERE?", LARGHEZZA/2 - MeasureText("DISCONNETTERE?", 24)/2, ALTEZZA/2 - 40, 24, WHITE);
                if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 160, ALTEZZA/2 + 30, 140, 50 }, "SI")) {
                    popup_logout_attivo = 0;
                    *fase = FASE_SCELTA_ACCESSO;
                    id_utente_corrente = -1;
                    memset(inputPin, 0, 7); pinCount = 0;
                    memset(inputPassword, 0, 30); passCount = 0;
                    memset(inputUsername, 0, 30); usernameCount = 0;
                    memset(inputEmail, 0, 100); emailCount = 0;
                    memset(inputNome, 0, 30); nomeCount = 0;
                    memset(inputCognome, 0, 30); cognomeCount = 0;
                }
                if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 20, ALTEZZA/2 + 30, 140, 50 }, "NO")) { popup_logout_attivo = 0; }
            }
        } else if (sottomenu_salvataggi == 1) {
            if (!compattazione_fatta) {
                CompattaSalvataggiUtente();
                compattazione_fatta = 1;
            }
            static float scroll_offset = 0;
            scroll_offset += GetMouseWheelMove() * 35.0f;
            if (scroll_offset > 0) scroll_offset = 0;

            DrawRectangle(LARGHEZZA/2 - 380, 180, 760, 450, Fade(BLACK, 0.85f));
            DrawRectangleLines(LARGHEZZA/2 - 380, 180, 760, 450, GOLD);
            DrawText("SELEZIONA SALVATAGGIO", LARGHEZZA/2 - MeasureText("SELEZIONA SALVATAGGIO", 24)/2, 200, 24, GOLD);

            BeginScissorMode(LARGHEZZA/2 - 380, 240, 760, 390);
            int startY = 250 + (int)scroll_offset;
            int count = 0;

            for (int i = 1; i <= 100; i++) {
                char percorso[70];
                /* Cerca prima nel formato save_user{ID}_slot_{N}.dat */
                snprintf(percorso, sizeof(percorso), "data/games/save_user%d_slot_%d.dat",
                         id_utente_corrente, i);
                FILE *f = fopen(percorso, "rb");
                if (!f) {
                    /* Fallback: controlla il vecchio schema generico */
                    sprintf(percorso, "data/games/save_slot_%d.dat", i);
                    f = fopen(percorso, "rb");
                }
                if (f) {
                    StatoGioco tempGioco;
                    fread(&tempGioco, sizeof(StatoGioco), 1, f);
                    fclose(f);

                    // FILTRO: mostra solo salvataggi dell'utente loggato
                    if (tempGioco.giocatori[0].nome[0] != '\0' &&
                        strcmp(tempGioco.giocatori[0].nome, dbUtenti.lista[id_utente_corrente].username) != 0) {
                        continue;
                    }

                    count++;  /* contatore progressivo SOLO per questo utente */
                    DrawRectangleRounded((Rectangle){ LARGHEZZA/2 - 350, startY, 700, 55 }, 0.2f, 8, Fade(GRAY, 0.3f));
                    // BUG 3 FIX: Usa la modalita' salvata nel file, se presente.
                    // Altrimenti ricalcola da num_giocatori per retrocompatibilita'.
                    char tipo[20];
                    if (tempGioco.modalita[0] != '\0') {
                        strncpy(tipo, tempGioco.modalita, sizeof(tipo) - 1);
                    } else if (tempGioco.num_giocatori > 1) {
                        sprintf(tipo, "1 vs %d", tempGioco.num_giocatori - 1);
                    } else {
                        strcpy(tipo, "Sconosciuta");
                    }
                    DrawText(TextFormat("Partita %d", count), LARGHEZZA/2 - 330, startY + 8, 18, GOLD);
                    DrawText(tipo, LARGHEZZA/2 - 330, startY + 30, 14, GRAY);
                    DrawText(TextFormat("Salvato il: %s", tempGioco.data_salvataggio), LARGHEZZA/2 - 200, startY + 20, 14, LIGHTGRAY);
                    if (!popup_logout_attivo && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 70, startY + 7, 120, 40 }, "CARICA")) {
                        if (CaricaPartitaDaSlot(gioco, i)) {
                            gioco->slot_salvataggio = i;
                            RicostruisciStatoADT(gioco);
                            *fase = FASE_GIOCO;
                            sottomenu_salvataggi = 0;
                            scroll_offset = 0;
                        }
                    }
                    if (!popup_logout_attivo && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 210, startY + 7, 120, 40 }, "ELIMINA")) {
                        EliminaSalvataggio(i);
                        scroll_offset = 0;
                    }
                    startY += 65;
                }
            }
            if (count == 0) DrawText("Nessun salvataggio presente.", LARGHEZZA/2 - MeasureText("Nessun salvataggio presente.", 20)/2, 350, 20, DARKGRAY);
            int min_scroll = -((count * 65) - 370);
            if (min_scroll > 0) min_scroll = 0;
            if (scroll_offset < min_scroll) scroll_offset = min_scroll;
            EndScissorMode();

            if (!popup_logout_attivo && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 100, 645, 200, 40 }, "Indietro")) {
                sottomenu_salvataggi = 0;
                scroll_offset = 0;
            }
        }

        // TL-01: Notifica host disconnessione in riquadro nero (basso a destra)
        // Stile: come le notifiche di gameplay (gameplay_draw.c)
        if (gioco->timer_notifica > 0.0f && gioco->notifica[0] != '\0') {
            int notifW = MeasureText(gioco->notifica, 18) + 40;
            DrawRectangleRounded((Rectangle){ LARGHEZZA - notifW - 20, ALTEZZA - 80, (float)notifW, 60 }, 0.3f, 10, Fade(BLACK, 0.85f));
            DrawText(gioco->notifica, LARGHEZZA - notifW, ALTEZZA - 60, 18, WHITE);
        }
    }
}
