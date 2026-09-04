/**
 * @file auth_update.c
 * @brief Logica di aggiornamento delle schermate di autenticazione.
 * @ingroup auth_update
 *
 * Modulo separato che contiene la funzione AggiornaAuth.
 * Gestisce l'input utente e la logica di transizione tra gli
 * stati di autenticazione: login, registrazione, recupero password,
 * modifica profilo e pannello admin.
 *
 * Fattorizzato da auth_screen.c (769 righe) per ridurre la complessita'.
 *
 * NOTA: Il database utenti e' ora DINAMICO (MAX_UTENTI rimosso,
 * allocazione tramite DB_Alloca/realloc). La registrazione di nuovi
 * utenti non ha piu' limite fisso superiore.
 */
#include <string.h>
#include "raylib.h"

#include "../../../lib/screens/auth/auth_shared.h"
#include "../../../lib/screens/auth/auth_update.h"
#include "../../../lib/screens/auth/input_field.h"
#include "../../../lib/auth/auth_validation.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/socket/lan/lan_sync.h"
#include "../../../lib/screens/menu/string_utils.h"
#include "../../../lib/auth/auth.h"
#include "../../../lib/data_structures/bst.h"
#include "../../../lib/screens/ui.h"

/* Variabili di stato definite in auth_screen.c */
extern char recPassword[30];
extern char recUsername[30];
extern int recupero_completato;
extern int errore_recupero;
extern int utente_modifica_admin;
extern int mostra_popup_uscita_auth;

// Variabile `currentRole` definita in network.h (tipo NetworkRole)
extern NetworkRole currentRole;
// Variabile per resettare lo scroll del pannello admin (definita in admin_draw.c)
extern int admin_scroll_reset;



/**
 * @brief Gestisce l'input e la logica delle schermate di autenticazione.
 *
 * Viene chiamata nel loop principale quando la fase corrente
 * richiede gestione auth (login, registrazione, ecc.).
 *
 * @param fase Puntatore alla fase applicazione corrente.
 */
void AggiornaAuth(FaseApplicazione* fase) {
    Vector2 mousePos = GetMousePosition();

    // ============================================================
    //  FASE_REGISTRAZIONE - Gestione input e validazione
    // ============================================================
    if (*fase == FASE_REGISTRAZIONE) {
        CampoInput campi[6] = {
            { {670, 230, 420, 50}, 0 },   // Email
            { {190, 230, 420, 50}, 1 },   // Nome
            { {190, 320, 420, 50}, 2 },   // Cognome
            { {670, 320, 420, 50}, 3 },   // Username
            { {190, 470, 420, 50}, 4 },   // Password
            { {670, 470, 420, 50}, 5 }    // Pin recupero
        };

        for (int i = 0; i < 6; i++) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, campi[i].area)) {
                campo_focusato = campi[i].campo_id;
            }
        }

        if (IsKeyPressed(KEY_TAB)) {
            if (campo_focusato == 1) campo_focusato = 0;
            else if (campo_focusato == 0) campo_focusato = 2;
            else if (campo_focusato == 2) campo_focusato = 3;
            else if (campo_focusato == 3) campo_focusato = 4;
            else if (campo_focusato == 4) campo_focusato = 5;
            else if (campo_focusato == 5) campo_focusato = 1;
        }

        if (campo_focusato == 0) AggiornaInputCampo(inputEmail, &emailCount, 100);
        else if (campo_focusato == 1) AggiornaInputCampo(inputNome, &nomeCount, 30);
        else if (campo_focusato == 2) AggiornaInputCampo(inputCognome, &cognomeCount, 30);
        else if (campo_focusato == 3) AggiornaInputCampo(inputUsername, &usernameCount, 30);
        else if (campo_focusato == 4) AggiornaInputCampo(inputPassword, &passCount, 30);
        else if (campo_focusato == 5) AggiornaInputCampo(inputPin, &pinCount, 7);

        if (IsKeyPressed(KEY_ENTER) || submit_richiesto) {
            submit_richiesto = 0;
            errore_registrazione_email = 0;
            errore_registrazione_username = 0;
            errore_registrazione_pass = 0;

            int has_at = 0;
            for (int i = 0; i < emailCount; i++) {
                if (inputEmail[i] == '@') has_at = 1;
            }
            if (emailCount == 0) {
                errore_registrazione_email = 4;
            } else if (!has_at) {
                errore_registrazione_email = 1;
            } else if (nomeCount == 0) {
                errore_registrazione_email = 2;
            } else if (cognomeCount == 0) {
                errore_registrazione_email = 3;
            } else if (usernameCount == 0) {
                errore_registrazione_username = 2;
            } else {
                // Verifica username UNICO su TUTTA la LAN (locale + remoto)
                // LAN_UsernameEsiste controlla nel database locale mergiato
                int username_exists = LAN_UsernameEsiste(inputUsername, -1);
                if (username_exists) {
                    errore_registrazione_username = 1;
                } else {
                    int has_upper = 0;
                    for (int i = 0; i < passCount; i++) {
                        if (inputPassword[i] >= 'A' && inputPassword[i] <= 'Z') has_upper = 1;
                    }
                    if (passCount < 8) {
                        errore_registrazione_pass = 1;
                    } else if (!has_upper) {
                        errore_registrazione_pass = 2;
                    } else if (pinCount < 6) {
                        errore_registrazione_pass = 3;
                    } else if (Auth_EmailGiaUsata(inputEmail, -1)) {
                        errore_registrazione_email = 5;
                    } else if (LAN_EmailEsiste(inputEmail)) {
                        // BUG 1 FIX: Verifica anche nel DB mergiato LAN per evitare
                        // email duplicate su altri PC della stessa rete.
                        errore_registrazione_email = 5;
                    } else {
                        // Alloca DINAMICAMENTE spazio per il nuovo utente
                        DB_Alloca(dbUtenti.num_utenti + 1);
                        
                        /* ============================================================
                         * BUG 3 FIX: Azzera COMPLETAMENTE la struttura Statistiche
                         * prima di riempirla. Senza memset, il campo storico_partite[]
                         * potrebbe contenere DATI SPURI dalla memoria allocata da
                         * realloc(), facendo apparire partite salvate preesistenti
                         * per i nuovi account.
                         * ============================================================ */
                        memset(&dbUtenti.lista[dbUtenti.num_utenti], 0, sizeof(Statistiche));

                        strcpy(dbUtenti.lista[dbUtenti.num_utenti].email, inputEmail);
                        strcpy(dbUtenti.lista[dbUtenti.num_utenti].nome, inputNome);
                        strcpy(dbUtenti.lista[dbUtenti.num_utenti].cognome, inputCognome);
                        strcpy(dbUtenti.lista[dbUtenti.num_utenti].username, inputUsername);
                        strcpy(dbUtenti.lista[dbUtenti.num_utenti].password, inputPassword);
                        strcpy(dbUtenti.lista[dbUtenti.num_utenti].pin_recupero, inputPin);
                        /* BUG 3 FIX: partite_giocate, vittorie e storico sono gia' 0
                         * grazie al memset sopra. Li impostiamo esplicitamente per
                         * chiarezza e robustezza. */
                        dbUtenti.lista[dbUtenti.num_utenti].partite_giocate = 0;
                        dbUtenti.lista[dbUtenti.num_utenti].vittorie_giocatore = 0;
                        dbUtenti.lista[dbUtenti.num_utenti].vittorie_bot = 0;

                        id_utente_corrente = dbUtenti.num_utenti;
                        dbUtenti.num_utenti++;
                        AuthBST_Ricostruisci();
                        SalvaDB();
                        /* BUG 12 FIX: Rimosso LAN_BroadcastNow() duplicato.
                         * SalvaDB() chiama gia' LAN_BroadcastNow() internamente.
                         * Chiamarlo due volte spreca banda e causa broadcast
                         * inutili sulla rete. Teniamo solo BroadcastAdminDB()
                         * per gli host. */
                        nuovo_utente = 1;

                        // NOTA: Se siamo connessi come host (NET_HOST), broadcast SUBITO
                        // il DB aggiornato a tutti i client connessi, in modo
                        // che il pannello admin si aggiorni IMMEDIATAMENTE.
                        if (currentRole == NET_HOST) {
                            BroadcastAdminDB();
                        }

                        // BUG ADMIN FIX: Resetta scroll admin panel quando viene creato
                        // un nuovo utente tramite registrazione. Se l'admin apre il pannello
                        // dopo la registrazione, il nuovo utente sara' visibile in cima alla lista.
                        admin_scroll_reset = 1;

                        *fase = FASE_MENU;
                    }
                }
            }
        }
    }
    // ============================================================
    //  FASE_LOGIN - Gestione input e autenticazione
    // ============================================================
    else if (*fase == FASE_LOGIN) {
        CampoInput campi[2] = {
            { {350, 260, 580, 60}, 0 },
            { {350, 380, 580, 60}, 1 }
        };

        for (int i = 0; i < 2; i++) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, campi[i].area)) {
                campo_focusato = campi[i].campo_id;
            }
        }

        if (IsKeyPressed(KEY_TAB)) {
            campo_focusato = 1 - campo_focusato;
        }

        if (campo_focusato == 0) AggiornaInputCampo(inputUsername, &usernameCount, 30);
        else if (campo_focusato == 1) AggiornaInputCampo(inputPassword, &passCount, 30);

        if (IsKeyPressed(KEY_ENTER) || submit_richiesto) {
            submit_richiesto = 0;
            if (usernameCount > 0 && passCount > 0) {
                errore_registrazione_username = 0;
                errore_password = 0;

                int found = 0;
                for (int i = 0; i < dbUtenti.num_utenti; i++) {
                    if (strcmp(dbUtenti.lista[i].username, inputUsername) == 0) {
                        if (strcmp(dbUtenti.lista[i].password, inputPassword) == 0) {
                            id_utente_corrente = i;
                            errore_password = 0;
                            *fase = FASE_MENU;
                            nuovo_utente = 0;
                            found = 1;
                            break;
                        } else {
                            errore_password = 1;
                            memset(inputPassword, 0, 30);
                            passCount = 0;
                            found = 1;
                            break;
                        }
                    }
                }
                if (!found) {
                    errore_registrazione_username = 1;
                }
            }
        }
    }
    // ============================================================
    //  FASE_RECUPERO_PASSWORD - Gestione recupero credenziali
    // ============================================================
    else if (*fase == FASE_RECUPERO_PASSWORD) {
        if (recupero_completato == 0 || recupero_completato == 1) {
            CampoInput campi[2] = {
                { {440, 225, 400, 55}, 0 },
                { {440, 345, 400, 55}, 1 }
            };
            for (int i = 0; i < 2; i++) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, campi[i].area)) {
                    campo_focusato = campi[i].campo_id;
                }
            }
            if (IsKeyPressed(KEY_TAB)) campo_focusato = (campo_focusato + 1) % 2;

            if (campo_focusato == 0) AggiornaInputCampo(inputEmail, &emailCount, 100);
            else if (campo_focusato == 1) AggiornaInputCampo(inputPin, &pinCount, 7);

            if (IsKeyPressed(KEY_ENTER) || submit_richiesto) {
            submit_richiesto = 0;
            errore_recupero = 0;
            int found = -1;

            while(emailCount > 0 && inputEmail[emailCount-1] == ' ') { inputEmail[emailCount-1] = '\0'; emailCount--; }
                inputEmail[emailCount] = '\0';

                int has_at = 0;
                for (int i = 0; i < emailCount; i++) if (inputEmail[i] == '@') has_at = 1;

                if (emailCount == 0) {
                    errore_recupero = 1;
                } else if (pinCount == 0) {
                    errore_recupero = 2;
                } else if (!has_at) {
                    errore_recupero = 3;
                } else {
                    found = Auth_VerificaEmailPIN(inputEmail, inputPin);
                    if (found == -1) {
                        errore_recupero = 4;
                    } else {
                        errore_recupero = 0;
                        recupero_completato = 2;
                        utente_modifica_admin = found;
                        passCount = 0;
                        memset(inputPassword, 0, 30);
                        campo_focusato = 0;
                    }
                }
            }
        } else if (recupero_completato == 2) {
            CampoInput campi[1] = {
                { {440, 320, 400, 55}, 0 }
            };
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, campi[0].area)) {
                campo_focusato = 0;
            }
            if (campo_focusato == 0) AggiornaInputCampo(inputPassword, &passCount, 30);

            if (IsKeyPressed(KEY_ENTER) || submit_richiesto) {
                submit_richiesto = 0;
                errore_registrazione_pass = 0;

                int has_upper = 0;
                for (int i = 0; i < passCount; i++) if (inputPassword[i] >= 'A' && inputPassword[i] <= 'Z') has_upper = 1;

                if (passCount < 8) {
                    errore_registrazione_pass = 1;
                } else if (!has_upper) {
                    errore_registrazione_pass = 2;
                } else {
                    strcpy(dbUtenti.lista[utente_modifica_admin].password, inputPassword);
                    SalvaDB();
                    recupero_completato = 3;
                }
            }
        }
    }
    // ============================================================
    //  FASE_PROFILO_MODIFICA - Gestione modifica profilo utente
    // ============================================================
    else if (*fase == FASE_PROFILO_MODIFICA) {
        CampoInput campi[6] = {
            { {670, 210, 280, 50}, 0 },
            { {330, 210, 280, 50}, 1 },
            { {330, 320, 280, 50}, 2 },
            { {670, 320, 280, 50}, 3 },
            { {330, 440, 280, 50}, 4 },
            { {670, 440, 280, 50}, 5 }
        };
        for (int i = 0; i < 6; i++) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, campi[i].area)) {
                campo_focusato = campi[i].campo_id;
            }
        }
        if (IsKeyPressed(KEY_TAB)) campo_focusato = (campo_focusato + 1) % 6;

        if (campo_focusato == 0) AggiornaInputCampo(inputUsername, &usernameCount, 30);
        else if (campo_focusato == 1) AggiornaInputCampo(inputEmail, &emailCount, 100);
        else if (campo_focusato == 2) AggiornaInputCampo(inputNome, &nomeCount, 30);
        else if (campo_focusato == 3) AggiornaInputCampo(inputCognome, &cognomeCount, 30);
        else if (campo_focusato == 4) AggiornaInputCampo(inputPassword, &passCount, 30);
        else if (campo_focusato == 5) AggiornaInputCampo(inputPin, &pinCount, 7);

        if (IsKeyPressed(KEY_ENTER) || submit_richiesto) {
            submit_richiesto = 0;

            int has_at = 0;
            for (int i = 0; i < emailCount; i++) if (inputEmail[i] == '@') has_at = 1;
            int has_upper = 0;
            for (int i = 0; i < passCount; i++) if (inputPassword[i] >= 'A' && inputPassword[i] <= 'Z') has_upper = 1;

            int username_exists = 0;
            for (int i = 0; i < dbUtenti.num_utenti; i++) {
                if (i != id_utente_corrente && CompareIgnoreCase(dbUtenti.lista[i].username, inputUsername)) {
                    username_exists = 1;
                    break;
                }
            }

            if (usernameCount == 0) errore_registrazione_username = 2;
            else if (username_exists) errore_registrazione_username = 1;
            else if (emailCount == 0) errore_registrazione_email = 4;
            else if (!has_at) errore_registrazione_email = 1;
            else if (nomeCount == 0) errore_registrazione_email = 2;
            else if (cognomeCount == 0) errore_registrazione_email = 3;
            else if (passCount < 8) errore_registrazione_pass = 1;
            else if (!has_upper) errore_registrazione_pass = 2;
            else if (pinCount < 6) errore_registrazione_pass = 3;
            else if (Auth_EmailGiaUsata(inputEmail, id_utente_corrente)) {
                errore_registrazione_email = 5;
            }
            else {
                strcpy(dbUtenti.lista[id_utente_corrente].username, inputUsername);
                strcpy(dbUtenti.lista[id_utente_corrente].email, inputEmail);
                strcpy(dbUtenti.lista[id_utente_corrente].nome, inputNome);
                strcpy(dbUtenti.lista[id_utente_corrente].cognome, inputCognome);
                strcpy(dbUtenti.lista[id_utente_corrente].password, inputPassword);
                strcpy(dbUtenti.lista[id_utente_corrente].pin_recupero, inputPin);
                SalvaDB();
                *fase = FASE_PROFILO_VISUALIZZA;
            }
        }
    }
    // ============================================================
    //  FASE_ADMIN_MODIFICA_UTENTE - Gestione modifica/crea utente admin
    // ============================================================
    else if (*fase == FASE_ADMIN_MODIFICA_UTENTE) {
        CampoInput campi[6] = {
            { {690, 215, 280, 50}, 0 },
            { {310, 215, 280, 50}, 1 },
            { {310, 310, 280, 50}, 2 },
            { {690, 310, 280, 50}, 3 },
            { {310, 445, 280, 50}, 4 },
            { {690, 445, 280, 50}, 5 }
        };
        for (int i = 0; i < 6; i++) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, campi[i].area)) {
                campo_focusato = campi[i].campo_id;
            }
        }
        if (IsKeyPressed(KEY_TAB)) campo_focusato = (campo_focusato + 1) % 6;

        if (campo_focusato == 0) AggiornaInputCampo(inputUsername, &usernameCount, 30);
        else if (campo_focusato == 1) AggiornaInputCampo(inputEmail, &emailCount, 100);
        else if (campo_focusato == 2) AggiornaInputCampo(inputNome, &nomeCount, 30);
        else if (campo_focusato == 3) AggiornaInputCampo(inputCognome, &cognomeCount, 30);
        else if (campo_focusato == 4) AggiornaInputCampo(inputPassword, &passCount, 30);
        else if (campo_focusato == 5) AggiornaInputCampo(inputPin, &pinCount, 7);

        if (IsKeyPressed(KEY_ENTER) || submit_richiesto) {
            submit_richiesto = 0;

            int has_at = 0;
            for (int i = 0; i < emailCount; i++) if (inputEmail[i] == '@') has_at = 1;
            int has_upper = 0;
            for (int i = 0; i < passCount; i++) if (inputPassword[i] >= 'A' && inputPassword[i] <= 'Z') has_upper = 1;

            int username_exists = 0;
            for (int i = 0; i < dbUtenti.num_utenti; i++) {
                if (i != utente_modifica_admin && CompareIgnoreCase(dbUtenti.lista[i].username, inputUsername)) {
                    username_exists = 1;
                    break;
                }
            }

            if (usernameCount == 0) errore_registrazione_username = 2;
            else if (username_exists) errore_registrazione_username = 1;
            else if (emailCount == 0) errore_registrazione_email = 4;
            else if (!has_at) errore_registrazione_email = 1;
            else if (nomeCount == 0) errore_registrazione_email = 2;
            else if (cognomeCount == 0) errore_registrazione_email = 3;
            else if (passCount < 8) errore_registrazione_pass = 1;
            else if (!has_upper) errore_registrazione_pass = 2;
            else if (pinCount < 6) errore_registrazione_pass = 3;
            else if (Auth_EmailGiaUsata(inputEmail, utente_modifica_admin)) {
                errore_registrazione_email = 5;
            }
            else if (LAN_EmailEsiste(inputEmail)) {
                // BUG 1 FIX: Verifica anche nel DB mergiato LAN per evitare
                // che l'admin possa assegnare un'email gia' in uso su
                // un altro PC della stessa rete.
                // BUG EMAIL FIX: Se stiamo modificando un utente esistente,
                // escludi la sua email corrente dal controllo per evitare
                // falsi positivi ("email gia' utilizzata" quando si modifica
                // nome/cognome/username senza cambiare email).
                if (!(utente_modifica_admin >= 0 && utente_modifica_admin < dbUtenti.num_utenti &&
                      strcmp(dbUtenti.lista[utente_modifica_admin].email, inputEmail) == 0)) {
                    errore_registrazione_email = 5;
                }
            }
            /* ============================================================
             * BUG 4 FIX: Controlla TUTTI i flag di errore prima di salvare,
             * non solo errore_registrazione_email. Il vecchio codice
             * controllava SOLO errore_registrazione_email, permettendo
             * all'admin di creare/modificare utenti con password o PIN
             * non validi (es. password < 8 caratteri, PIN < 6 cifre).
             * 
             * Se UNO qualsiasi dei flag e' impostato, il salvataggio viene
             * bloccato e l'admin resta nella schermata di modifica/creazione.
             * ============================================================ */
            if (errore_registrazione_email || errore_registrazione_username || errore_registrazione_pass) {
                // Resta nella schermata di modifica/creazione senza procedere
            } else {
                int is_new_user = (utente_modifica_admin >= dbUtenti.num_utenti);

                // BUG FIX: Se stiamo creando un NUOVO utente (utente_modifica_admin == dbUtenti.num_utenti)
                // alloca lo spazio e AZZERA TUTTO con memset per evitare dati spazzatura
                // in storico_partite[] (partite_giocate e' gia' 0 ma l'array storico potrebbe
                // contenere residui di memoria).
                if (utente_modifica_admin >= dbUtenti.num_utenti) {
                    DB_Alloca(utente_modifica_admin + 1);
                    // Azzera COMPLETAMENTE la struttura Statistiche per sicurezza:
                    // - partite_giocate = 0
                    // - storico_partite[] tutta a zero (nessuna partita caricata)
                    memset(&dbUtenti.lista[utente_modifica_admin], 0, sizeof(Statistiche));
                }

                strcpy(dbUtenti.lista[utente_modifica_admin].username, inputUsername);
                strcpy(dbUtenti.lista[utente_modifica_admin].nome, inputNome);
                strcpy(dbUtenti.lista[utente_modifica_admin].cognome, inputCognome);
                strcpy(dbUtenti.lista[utente_modifica_admin].email, inputEmail);
                strcpy(dbUtenti.lista[utente_modifica_admin].password, inputPassword);
                strcpy(dbUtenti.lista[utente_modifica_admin].pin_recupero, inputPin);

                if (utente_modifica_admin == dbUtenti.num_utenti) {
                    // Nuovo utente: partite_giocate e' gia' 0 per il memset sopra,
                    // ma lo impostiamo esplicitamente per chiarezza.
                    dbUtenti.lista[utente_modifica_admin].partite_giocate = 0;
                    dbUtenti.lista[utente_modifica_admin].vittorie_giocatore = 0;
                    dbUtenti.lista[utente_modifica_admin].vittorie_bot = 0;
                    dbUtenti.num_utenti++;
                    AuthBST_Ricostruisci();
                    SalvaDB();
                    /* BUG 12 FIX: Rimosso LAN_BroadcastNow() duplicato.
                     * SalvaDB() chiama gia' LAN_BroadcastNow() internamente.
                     * Chiamarlo due volte spreca banda. */
                }

                AuthBST_Ricostruisci();
                SalvaDB();
                /* BUG 12 FIX: Rimosso LAN_BroadcastNow() duplicato.
                 * SalvaDB() chiama gia' LAN_BroadcastNow() internamente.
                 * Il broadcast ai client connessi (BroadcastAdminDB) e' separato. */
                // Broadcast il database aggiornato a tutti i client connessi
                if (currentRole == NET_HOST) {
                    BroadcastAdminDB();
                } else if (currentRole == NET_CLIENT) {
                    AdminUserAction action = {0};
                    action.action = is_new_user ? 3 : 1;
                    action.target_id = utente_modifica_admin;
                    strncpy(action.user.email, dbUtenti.lista[utente_modifica_admin].email, sizeof(action.user.email) - 1);
                    strncpy(action.user.nome, dbUtenti.lista[utente_modifica_admin].nome, sizeof(action.user.nome) - 1);
                    strncpy(action.user.cognome, dbUtenti.lista[utente_modifica_admin].cognome, sizeof(action.user.cognome) - 1);
                    strncpy(action.user.username, dbUtenti.lista[utente_modifica_admin].username, sizeof(action.user.username) - 1);
                    strncpy(action.user.password, dbUtenti.lista[utente_modifica_admin].password, sizeof(action.user.password) - 1);
                    strncpy(action.user.pin_recupero, dbUtenti.lista[utente_modifica_admin].pin_recupero, sizeof(action.user.pin_recupero) - 1);
                    action.user.partite_giocate = dbUtenti.lista[utente_modifica_admin].partite_giocate;
                    action.user.vittorie_giocatore = dbUtenti.lista[utente_modifica_admin].vittorie_giocatore;
                    action.user.vittorie_bot = dbUtenti.lista[utente_modifica_admin].vittorie_bot;
                    SendAdminUserAction(&action);
                }
                // BUG FIX: Reset scroll admin per mostrare subito il nuovo utente
                admin_scroll_reset = 1;

                *fase = FASE_ADMIN_PANEL;
                utente_modifica_admin = -1;
            }
        }
    }
}