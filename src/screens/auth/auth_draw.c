/**
 * @file auth_draw.c
 * @brief Rendering delle schermate di autenticazione.
 * @ingroup auth_draw
 *
 * Modulo che contiene DisegnaAuth per il rendering di tutte le
 * schermate auth: login, registrazione, recupero password,
 * visualizzazione/modifica profilo, pannello admin.
 *
 * Utilizza Raylib e le texture globali caricate da ui.c.
 * Fattorizzato da auth_screen.c per ridurre la complessita'.
 */
#include <string.h>
#include "raylib.h"

#include "../../../lib/screens/auth/auth_shared.h"
#include "../../../lib/screens/auth/auth_draw.h"
#include "../../../lib/screens/auth/input_field.h"
#include "../../../lib/screens/menu/string_utils.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/socket/lan/lan_sync.h"
#include "../../../lib/auth.h"
#include "../../../lib/ui.h"
#include "../../../lib/screens/auth/admin_draw.h"

#define LARGHEZZA 1280
#define ALTEZZA 720

/* Variabili di stato definite in auth_screen.c */
extern char recPassword[30];
extern char recUsername[30];
extern int recupero_completato;
extern int errore_recupero;
extern int utente_modifica_admin;
extern int mostra_popup_uscita_auth;

extern int esc_consumato;

/**
 * @brief Disegna la schermata di autenticazione corrente.
 * @ingroup auth_draw
 * @pre fase != NULL.
 * @post Scena auth renderizzata.
 * @return 1 se confermata uscita, 0 altrimenti.
 * @param fase Fase corrente.
 */
int DisegnaAuth(FaseApplicazione* fase) {
    if (textureSfondo.id != 0) {
        DrawTexturePro(textureSfondo, 
                       (Rectangle){ 0.0f, 0.0f, (float)textureSfondo.width, (float)textureSfondo.height }, 
                       (Rectangle){ 0.0f, 0.0f, (float)LARGHEZZA, (float)ALTEZZA }, 
                       (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    }

    // ============================================================
    //  FASE_SCELTA_ACCESSO - Schermata iniziale con bottoni
    // ============================================================
    if (*fase == FASE_SCELTA_ACCESSO) {
        DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.5f)); 
        
        DrawText("UNO", LARGHEZZA/2 - MeasureText("UNO", 100)/2 + 4, 74, 100, coloriRaylib[1]);
        DrawText("UNO", LARGHEZZA/2 - MeasureText("UNO", 100)/2, 70, 100, coloriRaylib[0]);
        
        DrawText("SECURE MULTIPLAYER EDITION", LARGHEZZA/2 - MeasureText("SECURE MULTIPLAYER EDITION", 28)/2, 180, 28, LIGHTGRAY);
        DrawText("Chi sta giocando oggi?", LARGHEZZA/2 - MeasureText("Chi sta giocando oggi?", 24)/2, 256, 24, WHITE);
        
        if (!esc_consumato && IsKeyPressed(KEY_ESCAPE)) {
            mostra_popup_uscita_auth = !mostra_popup_uscita_auth;
        }
        
        if (!mostra_popup_uscita_auth && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 180, 320, 360, 80 }, "ACCEDI A PROFILO")) {
            *fase = FASE_LOGIN;
            usernameCount = 0; passCount = 0;
            memset(inputUsername, 0, 30); memset(inputPassword, 0, 30);
            errore_registrazione_username = 0; errore_password = 0;
            campo_focusato = 0;
        }
        if (!mostra_popup_uscita_auth && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 180, 440, 360, 80 }, "CREA NUOVO PROFILO")) {
            *fase = FASE_REGISTRAZIONE;
            emailCount = 0; nomeCount = 0; cognomeCount = 0; usernameCount = 0; passCount = 0;
            memset(inputEmail, 0, 100); memset(inputNome, 0, 30); memset(inputCognome, 0, 30); memset(inputUsername, 0, 30); memset(inputPassword, 0, 30);
            errore_registrazione_email = 0; errore_registrazione_username = 0; errore_registrazione_pass = 0;
            campo_focusato = 1;
        }
        if (!mostra_popup_uscita_auth && DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 180, 560, 360, 80 }, "ESCI")) {
            mostra_popup_uscita_auth = 1;
        }

        int credX = LARGHEZZA - MeasureText("Decarolis Alessio", 16) - 20;
        DrawText("Realizzato da:", credX, ALTEZZA - 80, 16, Fade(LIGHTGRAY, 0.7f));
        DrawText("Decarolis Alessio", credX, ALTEZZA - 60, 16, Fade(LIGHTGRAY, 0.7f));
        DrawText("Franco Pasquale", credX, ALTEZZA - 40, 16, Fade(LIGHTGRAY, 0.7f));
        
        if (mostra_popup_uscita_auth) {
            DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.85f));
            DrawRectangleRounded((Rectangle){ LARGHEZZA/2 - 250, ALTEZZA/2 - 120, 500, 240 }, 0.1f, 10, Fade(DARKGRAY, 0.95f));
            DrawRectangleRoundedLines((Rectangle){ LARGHEZZA/2 - 250, ALTEZZA/2 - 120, 500, 240 }, 0.1f, 10, GOLD);

            DrawText("VUOI DAVVERO USCIRE?", LARGHEZZA/2 - MeasureText("VUOI DAVVERO USCIRE?", 32)/2, ALTEZZA/2 - 60, 32, WHITE);

            if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 180, ALTEZZA/2 + 20, 150, 60 }, "SI")) {
                return 1;
            }
            if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 30, ALTEZZA/2 + 20, 150, 60 }, "NO")) {
                mostra_popup_uscita_auth = 0; 
            }
        }
    }
    // ============================================================
    //  FASE_LOGIN - Schermata di login
    // ============================================================
    else if (*fase == FASE_LOGIN) {
        DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.4f));
        
        DrawText("ACCEDI AL TUO PROFILO", LARGHEZZA/2 - MeasureText("ACCEDI AL TUO PROFILO", 48)/2 + 2, 52, 48, BLACK); 
        DrawText("ACCEDI AL TUO PROFILO", LARGHEZZA/2 - MeasureText("ACCEDI AL TUO PROFILO", 48)/2, 50, 48, coloriRaylib[1]); 
        
        DrawRectangleRounded((Rectangle){280, 210, 720, 300}, 0.1f, 10, Fade(coloriRaylib[4], 0.9f));
        DrawRectangleRoundedLines((Rectangle){280, 210, 720, 300}, 0.1f, 10, coloriRaylib[0]);
        
        DisegnaInputField((Rectangle){350, 260, 580, 60}, "Username", inputUsername, usernameCount, campo_focusato == 0, 0, coloriRaylib[1]);
        DisegnaInputField((Rectangle){350, 380, 580, 60}, "Password", inputPassword, passCount, campo_focusato == 1, 1, coloriRaylib[1]);

        if (errore_registrazione_username) DrawText("Usern non trovato!", LARGHEZZA/2 - MeasureText("Usern non trovato!", 20)/2, 475, 20, RED);
        if (errore_password) DrawText("Password errata!", LARGHEZZA/2 - MeasureText("Password errata!", 20)/2, 475, 20, RED);

        DrawText("TAB o Click: Seleziona campo  |  INVIO: Accedi", LARGHEZZA/2 - MeasureText("TAB o Click: Seleziona campo  |  INVIO: Accedi", 18)/2, 515, 18, LIGHTGRAY);
        
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 140, 560, 130, 40 }, "Indietro")) {
            *fase = FASE_SCELTA_ACCESSO;
            usernameCount = 0; passCount = 0;
            memset(inputUsername, 0, 30); memset(inputPassword, 0, 30);
            errore_registrazione_username = 0; errore_password = 0;
        }
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 10, 560, 130, 40 }, "Accedi")) {
            submit_richiesto = 1;
        }
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 140, 620, 280, 40 }, "Recupera Credenziali")) {
            *fase = FASE_RECUPERO_PASSWORD;
            emailCount = 0; nomeCount = 0; cognomeCount = 0;
            memset(inputEmail, 0, 100); memset(inputNome, 0, 30); memset(inputCognome, 0, 30);
            campo_focusato = 0; recupero_completato = 0; errore_recupero = 0;
        }
    }
    // ============================================================
    //  FASE_REGISTRAZIONE - Schermata di registrazione
    // ============================================================
    else if (*fase == FASE_REGISTRAZIONE) {
        DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.4f));
        
        DrawText("CREA UN NUOVO PROFILO", LARGHEZZA/2 - MeasureText("CREA UN NUOVO PROFILO", 44)/2 + 2, 42, 44, BLACK);
        DrawText("CREA UN NUOVO PROFILO", LARGHEZZA/2 - MeasureText("CREA UN NUOVO PROFILO", 44)/2, 40, 44, coloriRaylib[1]);
        
        DrawRectangleRounded((Rectangle){130, 110, 1020, 450}, 0.1f, 10, Fade(coloriRaylib[4], 0.9f));
        DrawRectangleRoundedLines((Rectangle){130, 110, 1020, 450}, 0.1f, 10, coloriRaylib[1]);
        
        DrawText("Completa tutti i campi richiesti per iniziare a giocare", LARGHEZZA/2 - MeasureText("Completa tutti i campi richiesti per iniziare a giocare", 20)/2, 130, 20, LIGHTGRAY);
        
        DrawText("DATI PERSONALI", 190, 170, 22, coloriRaylib[1]);
        DisegnaInputField((Rectangle){190, 230, 420, 50}, "Nome", inputNome, nomeCount, campo_focusato == 1, 0, LIGHTGRAY);
        DisegnaInputField((Rectangle){190, 320, 420, 50}, "Cognome", inputCognome, cognomeCount, campo_focusato == 2, 0, LIGHTGRAY);

        DrawText("CREDENZIALI", 670, 170, 22, coloriRaylib[1]);
        DisegnaInputField((Rectangle){670, 230, 420, 50}, "Email", inputEmail, emailCount, campo_focusato == 0, 0, LIGHTGRAY);
        DisegnaInputField((Rectangle){670, 320, 420, 50}, "Username", inputUsername, usernameCount, campo_focusato == 3, 0, LIGHTGRAY);

        DrawText("SICUREZZA", 190, 405, 22, coloriRaylib[1]);
        DisegnaInputField((Rectangle){190, 455, 420, 50}, "Password", inputPassword, passCount, campo_focusato == 4, 1, LIGHTGRAY);
        DisegnaInputField((Rectangle){670, 455, 420, 50}, "PIN Recupero (6 cifre)", inputPin, pinCount, campo_focusato == 5, 1, LIGHTGRAY);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            if (CheckCollisionPointRec(mousePos, (Rectangle){670, 230, 420, 50})) campo_focusato = 0;
            else if (CheckCollisionPointRec(mousePos, (Rectangle){190, 230, 420, 50})) campo_focusato = 1;
            else if (CheckCollisionPointRec(mousePos, (Rectangle){190, 320, 420, 50})) campo_focusato = 2;
            else if (CheckCollisionPointRec(mousePos, (Rectangle){670, 320, 420, 50})) campo_focusato = 3;
            else if (CheckCollisionPointRec(mousePos, (Rectangle){190, 455, 420, 50})) campo_focusato = 4;
            else if (CheckCollisionPointRec(mousePos, (Rectangle){670, 455, 420, 50})) campo_focusato = 5;
        }

        int errorY = 515;
        if (errore_registrazione_email == 4) DrawText("Email obbligatoria!", LARGHEZZA/2 - MeasureText("Email obbligatoria!", 18)/2, errorY, 18, RED);
        else if (errore_registrazione_email == 1) DrawText("Email non valida! Deve contenere '@'", LARGHEZZA/2 - MeasureText("Email non valida! Deve contenere '@'", 18)/2, errorY, 18, RED);
        else if (errore_registrazione_email == 2) DrawText("Nome obbligatorio!", LARGHEZZA/2 - MeasureText("Nome obbligatorio!", 18)/2, errorY, 18, RED);
        else if (errore_registrazione_email == 3) DrawText("Cognome obbligatorio!", LARGHEZZA/2 - MeasureText("Cognome obbligatorio!", 18)/2, errorY, 18, RED);
        else if (errore_registrazione_email == 5) DrawText("Email già utilizzata!", LARGHEZZA/2 - MeasureText("Email già utilizzata!", 18)/2, errorY, 18, RED);

        if (errore_registrazione_username == 1) DrawText("Username già utilizzato!", LARGHEZZA/2 - MeasureText("Username già utilizzato!", 18)/2, errorY, 18, RED);
        else if (errore_registrazione_username == 2) DrawText("Username obbligatorio!", LARGHEZZA/2 - MeasureText("Username obbligatorio!", 18)/2, errorY, 18, RED);

        if (errore_registrazione_pass == 1) DrawText("La Password deve avere almeno 8 caratteri", LARGHEZZA/2 - MeasureText("La Password deve avere almeno 8 caratteri", 18)/2, errorY + 25, 18, RED);
        else if (errore_registrazione_pass == 2) DrawText("La Password deve contenere almeno 1 lettera maiuscola", LARGHEZZA/2 - MeasureText("La Password deve contenere almeno 1 lettera maiuscola", 18)/2, errorY + 25, 18, RED);
        else if (errore_registrazione_pass == 3) DrawText("Il PIN deve essere di 6 cifre", LARGHEZZA/2 - MeasureText("Il PIN deve essere di 6 cifre", 18)/2, errorY + 25, 18, RED);

        DrawText("TAB o Click: Seleziona campo  |  INVIO: Registrati", LARGHEZZA/2 - MeasureText("TAB o Click: Seleziona campo  |  INVIO: Registrati", 18)/2, 580, 18, LIGHTGRAY);
        
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 170, 615, 160, 40 }, "Indietro")) {
            *fase = FASE_SCELTA_ACCESSO;
            emailCount = 0; nomeCount = 0; cognomeCount = 0; usernameCount = 0; passCount = 0;
            memset(inputEmail, 0, 100); memset(inputNome, 0, 30); memset(inputCognome, 0, 30); memset(inputUsername, 0, 30); memset(inputPassword, 0, 30);
            errore_registrazione_email = 0; errore_registrazione_username = 0; errore_registrazione_pass = 0;
        }
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 10, 615, 160, 40 }, "Registrati")) {
            submit_richiesto = 1;
        }
    }
    // ============================================================
    //  FASE_RECUPERO_PASSWORD - Schermata recupero credenziali
    // ============================================================
    else if (*fase == FASE_RECUPERO_PASSWORD) {
        DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.4f));
        DrawText("RECUPERA CREDENZIALI", LARGHEZZA/2 - MeasureText("RECUPERA CREDENZIALI", 44)/2 + 2, 42, 44, BLACK);
        DrawText("RECUPERA CREDENZIALI", LARGHEZZA/2 - MeasureText("RECUPERA CREDENZIALI", 44)/2, 40, 44, coloriRaylib[1]);
        
        int boxX = 340, boxW = 600;
        int boxHeight = (recupero_completato == 3) ? 200 : 420;
        int boxY = (recupero_completato == 3) ? 200 : 130;
        DrawRectangleRounded((Rectangle){boxX, boxY, boxW, boxHeight}, 0.1f, 10, Fade(coloriRaylib[4], 0.9f));
        DrawRectangleRoundedLines((Rectangle){boxX, boxY, boxW, boxHeight}, 0.1f, 10, coloriRaylib[0]);
        
        if (recupero_completato == 0 || recupero_completato == 1) {
            DrawText("Inserisci Email e PIN per resettare la password:", LARGHEZZA/2 - MeasureText("Inserisci Email e PIN per resettare la password:", 18)/2, 155, 18, LIGHTGRAY);
            
            DisegnaInputField((Rectangle){440, 225, 400, 55}, "Email", inputEmail, emailCount, campo_focusato == 0, 0, coloriRaylib[1]);
            DisegnaInputField((Rectangle){440, 345, 400, 55}, "PIN di recupero (6 cifre)", inputPin, pinCount, campo_focusato == 1, 1, coloriRaylib[1]);

            if (errore_recupero == 1) DrawText("Email obbligatoria!", LARGHEZZA/2 - MeasureText("Email obbligatoria!", 20)/2, 425, 20, RED);
            else if (errore_recupero == 2) DrawText("PIN obbligatorio!", LARGHEZZA/2 - MeasureText("PIN obbligatorio!", 20)/2, 425, 20, RED);
            else if (errore_recupero == 3) DrawText("Email non valida! Deve contenere '@'", LARGHEZZA/2 - MeasureText("Email non valida! Deve contenere '@'", 20)/2, 425, 20, RED);
            else if (errore_recupero == 4) DrawText("Email o PIN errati!", LARGHEZZA/2 - MeasureText("Email o PIN errati!", 20)/2, 425, 20, RED);
        } else if (recupero_completato == 2) {
            const char* titoloNuovaPass = "NUOVA PASSWORD";
            DrawText(titoloNuovaPass, LARGHEZZA/2 - MeasureText(titoloNuovaPass, 32)/2, 195, 32, coloriRaylib[1]);
            DrawText("Inserisci la nuova password:", LARGHEZZA/2 - MeasureText("Inserisci la nuova password:", 18)/2, 240, 18, LIGHTGRAY);
            
            DisegnaInputField((Rectangle){440, 320, 400, 55}, "", inputPassword, passCount, campo_focusato == 0, 1, coloriRaylib[1]);

            if (errore_registrazione_pass == 1) DrawText("La Password deve avere almeno 8 caratteri", LARGHEZZA/2 - MeasureText("La Password deve avere almeno 8 caratteri", 18)/2, 380, 18, RED);
            else if (errore_registrazione_pass == 2) DrawText("La Password deve contenere almeno 1 lettera maiuscola", LARGHEZZA/2 - MeasureText("La Password deve contenere almeno 1 lettera maiuscola", 18)/2, 380, 18, RED);
        } else if (recupero_completato == 3) {
            DrawText("Password reimpostata con successo!", LARGHEZZA/2 - MeasureText("Password reimpostata con successo!", 22)/2, 260, 22, GREEN);
        }

        int buttonY = (recupero_completato == 3) ? 320 : 485;
        int buttonX = (recupero_completato == 3) ? (LARGHEZZA/2 - 70) : (LARGHEZZA/2 - 160);
        if (DisegnaBottoneMenuAnimato((Rectangle){ buttonX, buttonY, 140, 40 }, recupero_completato == 3 ? "Vai al Login" : "Annulla")) {
            *fase = FASE_LOGIN;
            usernameCount = 0; passCount = 0; emailCount = 0; pinCount = 0;
            memset(inputUsername, 0, 30); memset(inputPassword, 0, 30); memset(inputEmail, 0, 100); memset(inputPin, 0, 7);
            errore_registrazione_username = 0; errore_password = 0;
            errore_recupero = 0; recupero_completato = 0; errore_registrazione_pass = 0;
        }
        
        if (recupero_completato != 3) {
            if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 20, buttonY, 140, 40 }, recupero_completato == 2 ? "Salva" : "Avanti")) {
                submit_richiesto = 1;
            }
        }
    }
    // ============================================================
    //  FASE_PROFILO_VISUALIZZA - Visualizzazione profilo
    // ============================================================
    else if (*fase == FASE_PROFILO_VISUALIZZA) {
        DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.4f));
        DrawText("IL MIO PROFILO", LARGHEZZA/2 - MeasureText("IL MIO PROFILO", 44)/2 + 2, 52, 44, BLACK);
        DrawText("IL MIO PROFILO", LARGHEZZA/2 - MeasureText("IL MIO PROFILO", 44)/2, 50, 44, YELLOW);
        
        DrawRectangleRounded((Rectangle){240, 150, 800, 380}, 0.1f, 10, (Color){100, 70, 40, 200});
        DrawRectangleRoundedLines((Rectangle){240, 150, 800, 380}, 0.1f, 10, GOLD);
        
        DrawLine(640, 170, 640, 510, GOLD);
        
        DrawText("DATI PERSONALI", 300, 180, 24, YELLOW);
        DrawText(TextFormat("Nome: %s", dbUtenti.lista[id_utente_corrente].nome), 300, 240, 20, WHITE);
        DrawText(TextFormat("Cognome: %s", dbUtenti.lista[id_utente_corrente].cognome), 300, 290, 20, WHITE);
        DrawText(TextFormat("Email: %s", dbUtenti.lista[id_utente_corrente].email), 300, 340, 20, WHITE);
        DrawText(TextFormat("Username: %s", dbUtenti.lista[id_utente_corrente].username), 300, 390, 20, WHITE);

        DrawText("STATISTICHE", 700, 180, 24, YELLOW);
        DrawText(TextFormat("Partite Giocate: %d", dbUtenti.lista[id_utente_corrente].partite_giocate), 700, 240, 20, WHITE);
        DrawText(TextFormat("Vittorie: %d", dbUtenti.lista[id_utente_corrente].vittorie_giocatore), 700, 290, 20, (Color){0, 255, 0, 255});
        DrawText(TextFormat("Sconfitte: %d", dbUtenti.lista[id_utente_corrente].vittorie_bot), 700, 340, 20, RED);

        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 200, 560, 170, 50 }, "Indietro")) *fase = FASE_MENU;
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 30, 560, 170, 50 }, "Modifica")) {
            *fase = FASE_PROFILO_MODIFICA;
            strcpy(inputEmail, dbUtenti.lista[id_utente_corrente].email);
            strcpy(inputNome, dbUtenti.lista[id_utente_corrente].nome);
            strcpy(inputCognome, dbUtenti.lista[id_utente_corrente].cognome);
            strcpy(inputPassword, dbUtenti.lista[id_utente_corrente].password);
            strcpy(inputUsername, dbUtenti.lista[id_utente_corrente].username);
            strcpy(inputPin, dbUtenti.lista[id_utente_corrente].pin_recupero);
            emailCount = strlen(inputEmail);
            nomeCount = strlen(inputNome);
            cognomeCount = strlen(inputCognome);
            passCount = strlen(inputPassword);
            usernameCount = strlen(inputUsername);
            pinCount = strlen(inputPin);
            campo_focusato = 0;
        }
    }
    // ============================================================
    //  FASE_PROFILO_MODIFICA - Modifica profilo utente
    // ============================================================
    else if (*fase == FASE_PROFILO_MODIFICA) {
        DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.4f));
        DrawText("IL MIO PROFILO", LARGHEZZA/2 - MeasureText("IL MIO PROFILO", 44)/2 + 2, 52, 44, BLACK);
        DrawText("IL MIO PROFILO", LARGHEZZA/2 - MeasureText("IL MIO PROFILO", 44)/2, 50, 44, YELLOW);
        
        DrawRectangleRounded((Rectangle){280, 110, 720, 470}, 0.1f, 10, Fade(coloriRaylib[4], 0.9f));
        DrawRectangleRoundedLines((Rectangle){280, 110, 720, 470}, 0.1f, 10, coloriRaylib[0]);
        
        DrawText("Modifica Dati Personali", LARGHEZZA/2 - MeasureText("Modifica Dati Personali", 24)/2, 125, 24, YELLOW);

        DrawText("CREDENZIALI", 690, 160, 20, coloriRaylib[1]);
        DisegnaInputField((Rectangle){690, 215, 280, 50}, "Username", inputUsername, usernameCount, campo_focusato == 0, 0, LIGHTGRAY);
        
        DrawText("DATI PERSONALI", 310, 160, 20, coloriRaylib[1]);
        DisegnaInputField((Rectangle){310, 215, 280, 50}, "Email", inputEmail, emailCount, campo_focusato == 1, 0, LIGHTGRAY);
        
        DisegnaInputField((Rectangle){310, 310, 280, 50}, "Nome", inputNome, nomeCount, campo_focusato == 2, 0, LIGHTGRAY);
        DisegnaInputField((Rectangle){690, 310, 280, 50}, "Cognome", inputCognome, cognomeCount, campo_focusato == 3, 0, LIGHTGRAY);
        
        DrawText("SICUREZZA", 310, 395, 20, coloriRaylib[1]);
        DisegnaInputField((Rectangle){310, 445, 280, 50}, "Password", inputPassword, passCount, campo_focusato == 4, 1, LIGHTGRAY);
        DisegnaInputField((Rectangle){690, 445, 280, 50}, "PIN Recupero (6 cifre)", inputPin, pinCount, campo_focusato == 5, 2, LIGHTGRAY);

        int errY = 510;
        if (errore_registrazione_username == 2) DrawText("Username obbligatorio!", LARGHEZZA/2 - MeasureText("Username obbligatorio!", 18)/2, errY, 18, RED);
        else if (errore_registrazione_username == 1) DrawText("Username già utilizzato!", LARGHEZZA/2 - MeasureText("Username già utilizzato!", 18)/2, errY, 18, RED);
        else if (errore_registrazione_email == 4) DrawText("Email obbligatoria!", LARGHEZZA/2 - MeasureText("Email obbligatoria!", 18)/2, errY, 18, RED);
        else if (errore_registrazione_email == 1) DrawText("Email non valida! Deve contenere '@'", LARGHEZZA/2 - MeasureText("Email non valida! Deve contenere '@'", 18)/2, errY, 18, RED);
        else if (errore_registrazione_email == 2) DrawText("Nome obbligatorio!", LARGHEZZA/2 - MeasureText("Nome obbligatorio!", 18)/2, errY, 18, RED);
        else if (errore_registrazione_email == 3) DrawText("Cognome obbligatorio!", LARGHEZZA/2 - MeasureText("Cognome obbligatorio!", 18)/2, errY, 18, RED);
        else if (errore_registrazione_pass == 1) DrawText("La Password deve avere almeno 8 caratteri", LARGHEZZA/2 - MeasureText("La Password deve avere almeno 8 caratteri", 18)/2, errY, 18, RED);
        else if (errore_registrazione_pass == 2) DrawText("La Password deve contenere almeno 1 lettera maiuscola", LARGHEZZA/2 - MeasureText("La Password deve contenere almeno 1 lettera maiuscola", 18)/2, errY, 18, RED);
        else if (errore_registrazione_pass == 3) DrawText("Il PIN deve essere di 6 cifre", LARGHEZZA/2 - MeasureText("Il PIN deve essere di 6 cifre", 18)/2, errY, 18, RED);

        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 170, 615, 160, 40 }, "Indietro")) *fase = FASE_PROFILO_VISUALIZZA;
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 10, 615, 160, 40 }, "Salva")) submit_richiesto = 1;
    }
    // ============================================================
    //  FASE_ADMIN_PANEL - Pannello di controllo admin (delegato)
    // ============================================================
    else if (*fase == FASE_ADMIN_PANEL) {
        DisegnaAdminPanel(fase);
    }
    // ============================================================
    //  FASE_ADMIN_MODIFICA_UTENTE - Modifica/crea utente admin (delegato)
    // ============================================================
    else if (*fase == FASE_ADMIN_MODIFICA_UTENTE) {
        DisegnaAdminModificaUtente(fase);
    }
    
    return 0;
}
