/**
 * @file admin_draw.c
 * @brief Rendering pannello admin e modifica utenti.
 * @ingroup admin_draw
 *
 * Modulo specializzato per il rendering delle schermate di amministrazione:
 *   - FASE_ADMIN_PANEL: lista utenti scrollabile
 *   - FASE_ADMIN_MODIFICA_UTENTE: form creazione/modifica utente
 *
 * Include bug fix: sincronizzazione immediata admin panel,
 * eliminazione utente loggato differita, nuovi account senza storico.
 */
#include <string.h>
#include "raylib.h"

#include "../../../lib/screens/auth/auth_shared.h"
#include "../../../lib/screens/auth/auth_draw.h"
#include "../../../lib/screens/auth/input_field.h"
#include "../../../lib/screens/menu/string_utils.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/lan/lan_sync.h"
#include "../../../lib/auth.h"
#include "../../../lib/ui.h"
#include "../../../lib/game_logic.h"
#include "../../../lib/data_structures/bst.h"

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

/* Variabili deletedUsers condivise (definite in lan_broadcast.c) */
extern char deletedUsers[512][30];
extern int numDeletedUsers;

/* Variabili rete (definite in network.c) per tracking client TCP connessi */
extern char connectedClientUsernames[4][30];
extern SOCKET clientSockets[4];

/** @brief Flag per resettare lo scroll del pannello admin. */
int admin_scroll_reset = 0;


/**
 * @brief Verifica se un utente e' in deletedUsers[] (pending deletion).
 * @ingroup admin_draw
 * @pre username != NULL.
 * @return 1 se eliminato, 0 altrimenti.
 * @param username Username.
 */
static int UtenteEliminato(const char* username) {
    if (!username || !username[0]) return 0;
    for (int i = 0; i < numDeletedUsers; i++) {
        if (strcmp(deletedUsers[i], username) == 0) return 1;
    }
    return 0;
}

/**
 * @brief Verifica se un utente e' loggato (locale o client TCP connesso).
 * @ingroup admin_draw
 * @pre username != NULL.
 * @return 1 se loggato/connesso, 0 altrimenti.
 * @param username Username.
 */
static int UtenteLoggato(const char* username) {
    if (!username || !username[0]) return 0;
    
    /* Controlla l'utente locale (loggato su questo dispositivo) */
    if (id_utente_corrente >= 0 && id_utente_corrente < dbUtenti.num_utenti) {
        if (strcmp(dbUtenti.lista[id_utente_corrente].username, username) == 0) return 1;
    }
    
    /* BUG ADMIN FIX: Controlla i client TCP connessi via multiplayer */
    for (int i = 1; i < 4; i++) {
        if (connectedClientUsernames[i][0] && strcmp(connectedClientUsernames[i], username) == 0) {
            return 1;
        }
    }
    
    return 0;
}

/**
 * @brief Disegna il pannello admin con lista utenti scrollabile.
 * @ingroup admin_draw
 * @pre fase != NULL.
 * @post Lista utenti renderizzata; azioni modifica/elimina gestite.
 * @param fase Fase corrente.
 */
void DisegnaAdminPanel(FaseApplicazione* fase) {
    DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.8f));
    DrawText("PANNELLO ADMIN - GESTIONE UTENTI", LARGHEZZA/2 - MeasureText("PANNELLO ADMIN - GESTIONE UTENTI", 40)/2 + 2, 42, 40, BLACK);
    DrawText("PANNELLO ADMIN - GESTIONE UTENTI", LARGHEZZA/2 - MeasureText("PANNELLO ADMIN - GESTIONE UTENTI", 40)/2, 40, 40, GOLD);

    static float scroll_admin = 0;

    /* BUG FIX: Reset scroll quando richiesto (dopo creazione nuovo utente admin) */
    if (admin_scroll_reset) {
        scroll_admin = 0;
        admin_scroll_reset = 0;
    }


    scroll_admin += GetMouseWheelMove() * 35.0f;
    if (scroll_admin > 0) scroll_admin = 0;

    DrawRectangle(150, 120, 1000, 480, Fade(BLACK, 0.85f));
    DrawRectangleLines(150, 120, 1000, 480, GOLD);

    BeginScissorMode(150, 120, 1000, 480);
    int startY = 140 + (int)scroll_admin;
    int utenti_visibili = 0;

    for (int i = 0; i < dbUtenti.num_utenti; i++) {
        /* BUG 2 FIX: Salta utenti in deletedUsers (ancora in gioco) */
        if (UtenteEliminato(dbUtenti.lista[i].username)) continue;

        DrawRectangleRounded((Rectangle){ 170, startY, 960, 60 }, 0.2f, 8, Fade(GRAY, 0.5f));
        DrawText(TextFormat("ID: %d | %s %s | %s", i, dbUtenti.lista[i].nome, dbUtenti.lista[i].cognome, dbUtenti.lista[i].email),
                 190, startY + 10, 20, LIGHTGRAY);
        DrawText(TextFormat("Partite: %d | Vittorie: %d | Sconfitte: %d", dbUtenti.lista[i].partite_giocate, dbUtenti.lista[i].vittorie_giocatore, dbUtenti.lista[i].vittorie_bot),
                 190, startY + 35, 16, Fade(LIGHTGRAY, 0.7f));

        if (strcmp(dbUtenti.lista[i].username, "Admin") != 0) {
            if (DisegnaBottoneMenuAnimato((Rectangle){ 890, startY + 10, 100, 40 }, "Modifica")) {
                *fase = FASE_ADMIN_MODIFICA_UTENTE;
                utente_modifica_admin = i;
                strcpy(inputUsername, dbUtenti.lista[i].username);
                strcpy(inputNome, dbUtenti.lista[i].nome);
                strcpy(inputCognome, dbUtenti.lista[i].cognome);
                strcpy(inputEmail, dbUtenti.lista[i].email);
                strcpy(inputPassword, dbUtenti.lista[i].password);
                strcpy(inputPin, dbUtenti.lista[i].pin_recupero);
                usernameCount = strlen(inputUsername);
                nomeCount = strlen(inputNome);
                cognomeCount = strlen(inputCognome);
                emailCount = strlen(inputEmail);
                passCount = strlen(inputPassword);
                pinCount = strlen(inputPin);
                campo_focusato = 0;
            }
            if (DisegnaBottoneMenuAnimato((Rectangle){ 1010, startY + 10, 100, 40 }, "Elimina")) {
                char del_username[30];
                strncpy(del_username, dbUtenti.lista[i].username, sizeof(del_username)-1);
                del_username[sizeof(del_username)-1] = '\0';

                /* ============================================================
                 * BUG 2 FIX: Eliminazione utente differita alla disconnessione.
                 *
                 * 1. Se l'utente NON e' loggato: rimozione IMMEDIATA dal DB.
                 * 2. Se l'utente E' loggato: lo aggiunge a deletedUsers[]
                 *    per NASCONDERLO dalla vista dell'admin, ma NON lo
                 *    rimuove dal database. L'utente continua a giocare
                 *    senza alcuna modifica. Solo quando si disconnettera'
                 *    (LAN_Close), il record verra' fisicamente rimosso.
                 *
                 * In entrambi i casi: elimina i salvataggi su disco,
                 * broadcast LAN e BroadcastAdminDB per sincronizzare
                 * istantaneamente tutti i peer connessi.
                 * ============================================================ */
                EliminaTuttiSalvataggiUtente(del_username);

                if (currentRole == NET_HOST || currentRole == NET_OFFLINE) {
                    if (UtenteLoggato(del_username)) {
                        /* BUG 2 FIX: Utente loggato - nascondi dalla vista admin
                         * ma preserva il record nel DB per il gioco in corso */
                        if (numDeletedUsers < 512) {
                            strncpy(deletedUsers[numDeletedUsers], del_username, 29);
                            deletedUsers[numDeletedUsers][29] = '\0';
                            numDeletedUsers++;
                        }
                        
                        /* BUG ADMIN FIX: Se l'utente e' connesso via TCP, disconettilo
                         * immediatamente chiudendo il suo socket. Il tentativo di
                         * riconnessione sara' rifiutato dall'host perche' l'utente
                         * e' in deletedUsers[] e non verra' piu' sincronizzato.
                         * NOTA: network.h include gia' winsock2.h su Windows. */
                        for (int s = 1; s < 4; s++) {
                            if (connectedClientUsernames[s][0] && 
                                strcmp(connectedClientUsernames[s], del_username) == 0) {
                                if (clientSockets[s] != INVALID_SOCKET) {
                                    shutdown(clientSockets[s], SD_BOTH);
                                    closesocket(clientSockets[s]);
                                    clientSockets[s] = INVALID_SOCKET;
                                }
                                connectedClientUsernames[s][0] = '\0';
                                break;
                            }
                        }
                    } else {
                        /* Utente non loggato: rimozione immediata dal DB */
                        for (int j = i; j < dbUtenti.num_utenti - 1; j++) {
                            dbUtenti.lista[j] = dbUtenti.lista[j+1];
                        }
                        dbUtenti.num_utenti--;
                        AuthBST_Ricostruisci();
                        SalvaDB();
                        i--;
                    }

                    /* BUG 1 FIX: Broadcast immediato a tutti i peer connessi */
                    LAN_BroadcastDeleteUser(del_username);
                    BroadcastAdminDB();
                    /* BUG 10 FIX: Invia anche via LAN UDP broadcast per aggiornare
                     * i peer che non sono connessi via TCP multiplayer. */
                    LAN_BroadcastNow();
                } else if (currentRole == NET_CLIENT) {
                    AdminUserAction action = {0};
                    action.action = 2;
                    action.target_id = -1;
                    strncpy(action.user.username, del_username, sizeof(action.user.username) - 1);
                    SendAdminUserAction(&action);
                }
            }
        } else {
            DrawText("Admin", 890, startY + 20, 20, GOLD);
        }
        startY += 70;
        utenti_visibili++;
    }
    EndScissorMode();

    if (utenti_visibili == 0) {
        BeginScissorMode(150, 120, 1000, 480);
        DrawText("Nessun utente disponibile.", 400, 300, 20, GRAY);
        EndScissorMode();
    }

    /* Calcola scroll minimo basato sugli utenti VISIBILI */
    int min_scroll = -((utenti_visibili * 70) - 460);
    if (min_scroll > 0) min_scroll = 0;
    if (scroll_admin < min_scroll) scroll_admin = min_scroll;

    if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 220, 630, 200, 50 }, "Indietro")) *fase = FASE_MENU;
    if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 20, 630, 200, 50 }, "Crea Utente")) {
        *fase = FASE_ADMIN_MODIFICA_UTENTE;
        utente_modifica_admin = dbUtenti.num_utenti;
        memset(inputNome, 0, 30); memset(inputCognome, 0, 30); memset(inputEmail, 0, 100);
        memset(inputPassword, 0, 30); memset(inputUsername, 0, 30); memset(inputPin, 0, 7);
        nomeCount = 0; cognomeCount = 0; emailCount = 0; passCount = 0; usernameCount = 0; pinCount = 0;
        campo_focusato = 0;
    }
}

/**
 * @brief Disegna il form di modifica/creazione utente (admin).
 * @ingroup admin_draw
 * @pre fase != NULL.
 * @post Form renderizzato; salvataggio gestito.
 * @param fase Fase corrente.
 */
void DisegnaAdminModificaUtente(FaseApplicazione* fase) {
    DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.4f));
    DrawText("PANNELLO ADMIN - GESTIONE UTENTI", LARGHEZZA/2 - MeasureText("PANNELLO ADMIN - GESTIONE UTENTI", 44)/2 + 2, 52, 44, BLACK);
    DrawText("PANNELLO ADMIN - GESTIONE UTENTI", LARGHEZZA/2 - MeasureText("PANNELLO ADMIN - GESTIONE UTENTI", 44)/2, 50, 44, GOLD);

    DrawRectangleRounded((Rectangle){280, 110, 720, 470}, 0.1f, 10, Fade(coloriRaylib[4], 0.9f));
    DrawRectangleRoundedLines((Rectangle){280, 110, 720, 470}, 0.1f, 10, coloriRaylib[0]);

    int is_new_user = (utente_modifica_admin == dbUtenti.num_utenti);
    if (is_new_user) {
        DrawText("CREA NUOVO UTENTE", LARGHEZZA/2 - MeasureText("CREA NUOVO UTENTE", 24)/2, 125, 24, YELLOW);
    } else {
        DrawText(TextFormat("Modifica Utente: %s", dbUtenti.lista[utente_modifica_admin].username),
                 LARGHEZZA/2 - MeasureText(TextFormat("Modifica Utente: %s", dbUtenti.lista[utente_modifica_admin].username), 24)/2, 125, 24, YELLOW);
    }

    DrawText("DATI PERSONALI", 310, 160, 20, coloriRaylib[1]);
    DisegnaInputField((Rectangle){310, 215, 280, 50}, "Email", inputEmail, emailCount, campo_focusato == 1, 0, LIGHTGRAY);
    DrawText("CREDENZIALI", 690, 160, 20, coloriRaylib[1]);
    DisegnaInputField((Rectangle){690, 215, 280, 50}, "Username", inputUsername, usernameCount, campo_focusato == 0, 0, LIGHTGRAY);
    DisegnaInputField((Rectangle){310, 310, 280, 50}, "Nome", inputNome, nomeCount, campo_focusato == 2, 0, LIGHTGRAY);
    DisegnaInputField((Rectangle){690, 310, 280, 50}, "Cognome", inputCognome, cognomeCount, campo_focusato == 3, 0, LIGHTGRAY);
    DrawText("SICUREZZA", 310, 395, 20, coloriRaylib[1]);
    DisegnaInputField((Rectangle){310, 445, 280, 50}, "Password", inputPassword, passCount, campo_focusato == 4, 1, LIGHTGRAY);
    DisegnaInputField((Rectangle){690, 445, 280, 50}, "PIN Recupero (6 cifre)", inputPin, pinCount, campo_focusato == 5, 2, LIGHTGRAY);

    int errY = 510;
    if (errore_registrazione_username == 2) DrawText("Username obbligatorio!", LARGHEZZA/2 - MeasureText("Username obbligatorio!", 18)/2, errY, 18, RED);
    else if (errore_registrazione_username == 1) DrawText("Username gia' utilizzato!", LARGHEZZA/2 - MeasureText("Username gia' utilizzato!", 18)/2, errY, 18, RED);
    else if (errore_registrazione_email == 4) DrawText("Email obbligatoria!", LARGHEZZA/2 - MeasureText("Email obbligatoria!", 18)/2, errY, 18, RED);
    else if (errore_registrazione_email == 1) DrawText("Email non valida!", LARGHEZZA/2 - MeasureText("Email non valida!", 18)/2, errY, 18, RED);
    else if (errore_registrazione_email == 2) DrawText("Nome obbligatorio!", LARGHEZZA/2 - MeasureText("Nome obbligatorio!", 18)/2, errY, 18, RED);
    else if (errore_registrazione_email == 3) DrawText("Cognome obbligatorio!", LARGHEZZA/2 - MeasureText("Cognome obbligatorio!", 18)/2, errY, 18, RED);
    else if (errore_registrazione_email == 5) DrawText("Email gia' utilizzata!", LARGHEZZA/2 - MeasureText("Email gia' utilizzata!", 18)/2, errY, 18, RED);
    else if (errore_registrazione_pass == 1) DrawText("Minimo 8 caratteri!", LARGHEZZA/2 - MeasureText("Minimo 8 caratteri!", 18)/2, errY, 18, RED);
    else if (errore_registrazione_pass == 2) DrawText("Servono lettere maiuscole!", LARGHEZZA/2 - MeasureText("Servono lettere maiuscole!", 18)/2, errY, 18, RED);
    else if (errore_registrazione_pass == 3) DrawText("Il PIN deve essere di 6 cifre!", LARGHEZZA/2 - MeasureText("Il PIN deve essere di 6 cifre!", 18)/2, errY, 18, RED);

    if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 170, 615, 160, 40 }, "Annulla")) {
        *fase = FASE_ADMIN_PANEL;
        utente_modifica_admin = -1;
    }
    if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 10, 615, 160, 40 }, "Salva")) submit_richiesto = 1;
}