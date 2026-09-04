/**
 * @file end_screen.c
 * @brief Implementazione schermata fine partita.
 * @ingroup end_screen
 *
 * Gestisce classifica, statistiche, esportazione report txt,
 * transizioni host/client per "Gioca Ancora".
 *
 * Gestisce la visualizzazione dei risultati finali:
 *   - Classifica con posizioni 1°, 2°, 3° (realistica)
 *   - Statistiche del vincitore
 *   - Pulsanti per tornare al menu o salvare
 *   - Visualizzazione punteggi e durata partita
 *
 * La classifica viene calcolata da CalcolaClassifica() in game_logic.c.
 *
 * BUG FIX COMPLETO (Fase Fine Partita):
 *   - RICHIESTA 1: Client "Gioca Ancora" → riportato subito in Pre-Lobby.
 *     L'Host vede notifica "Client A e' pronto in lobby" in basso a destra.
 *   - RICHIESTA 2: Client "Esci" → torna al menu, slot liberato e non riassegnato.
 *   - RICHIESTA 3: Host "Gioca Ancora" → torna in Pre-Lobby, slot vuoti.
 *   - RICHIESTA 4: Client "Gioca Ancora" DOPO che l'Host e' gia' tornato
 *     → si unisce direttamente alla Pre-Lobby con l'Host.
 *   - RICHIESTA 5: Statistiche sempre salvate, mai modificate.
 *
 * BUG 4 FIX (end_screen.c):
 *   - Cliente ora usa host_in_prelobby per decidere se andare subito in lobby
 *     o attendere. Pattern identico a gameplay_draw.c per consistenza.
 *   - Host ora pulisce gli slot degli altri giocatori (NON host) quando preme
 *     "Gioca Ancora", così la lobby mostra solo l'host come giocatore. Gli slot
 *     si riempiono man mano che i client premono "Gioca Ancora".
 *   - Rimossa transizione immediata a FASE_CLIENT_LOBBY: ora il client va in
 *     attesa (client_play_again_attesa=1) se host_in_prelobby e' 0.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"

#include "../../../lib/screens/game/end_screen.h"
#include "../../../lib/auth/auth.h"
#include "../../../lib/screens/ui.h"
#include "../../../lib/game/game_logic.h"
#include "../../../lib/data_structures/list.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/socket/lan/lan_sync.h"

#define LARGHEZZA 1280
#define ALTEZZA 720

/* ============================================================
 *  VARIABILE GLOBALE CONDIVISA
 * ============================================================
 *  BUG 3 FIX: Gestisce l'attesa del client quando preme "Gioca Ancora"
 *  ma l'host non e' ancora tornato in pre-lobby.
 *
 *  Definita QUI (end_screen.c) come NON-static, dichiarata extern in
 *  gameplay_draw.c e gameplay_update.c per permettere la comunicazione
 *  cross-file. Quando host_in_prelobby diventa 1 (tramite
 *  PACKET_HOST_RETURN_LOBBY), il client transita automaticamente da
 *  FASE_FINE_PARTITA a FASE_CLIENT_LOBBY senza bisogno di re-cliccare.
 */
int client_play_again_attesa = 0;

/* ============================================================
 *  FUNZIONI DI CALCOLO CLASSIFICA
 * ============================================================ */

/**
 * @brief Calcola la classifica finale della partita.
 *
 * Ordina i giocatori per numero di carte rimanenti (vincitore = 0 carte).
 * Utilizza bubble sort per ordinare in modo crescente.
 * La posizione 1 corrisponde al vincitore (0 carte).
 *
 * @param gioco Puntatore allo stato di gioco corrente.
 * @param classifica Array di ClassificaRecord[4] da popolare.
 * @param num_in_classifica Puntatore a int: numero di giocatori in classifica.
 */
void CalcolaClassifica(StatoGioco* gioco, ClassificaRecord classifica[4], int* num_in_classifica) {
    if (!gioco || !classifica || !num_in_classifica) return;
    *num_in_classifica = gioco->num_giocatori;

    for (int i = 0; i < gioco->num_giocatori; i++) {
        strncpy(classifica[i].nome, Giocatore_Nome(&gioco->giocatori[i]), sizeof(classifica[i].nome) - 1);
        classifica[i].num_carte = GetLunghezzaMano(&gioco->giocatori[i]);
        classifica[i].posizione = i + 1;
        classifica[i].is_bot = gioco->giocatori[i].is_bot;
    }

    // Bubble sort: chi ha 0 carte (vincitore) va in prima posizione,
    // poi in ordine crescente di carte rimanenti
    for (int i = 0; i < *num_in_classifica - 1; i++) {
        for (int j = i + 1; j < *num_in_classifica; j++) {
            int swap = 0;
            if (classifica[j].num_carte == 0 && classifica[i].num_carte > 0) swap = 1;
            else if (classifica[j].num_carte < classifica[i].num_carte) swap = 1;
            if (swap) {
                ClassificaRecord tmp = classifica[i];
                classifica[i] = classifica[j];
                classifica[j] = tmp;
            }
        }
    }

    for (int i = 0; i < *num_in_classifica; i++) {
        classifica[i].posizione = i + 1;
    }
}

/**
 * @brief Esporta il report della partita in un file di log testuale.
 *
 * Crea un file in data/logs/game_log_YYYYMMDD_HHMMSS.txt con:
 *   - Data e ora della partita
 *   - Durata in minuti:secondi
 *   - Esito (Vittoria/Sconfitta)
 *   - Classifica finale con carte rimaste
 *
 * @param gioco Puntatore allo stato di gioco.
 * @param esito Stringa con l'esito della partita ("Vittoria" o "Sconfitta").
 */
void EsportaStatisticheTxt(StatoGioco* gioco, const char* esito) {
    if (!gioco || !esito) return;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char filename[64];
    snprintf(filename, sizeof(filename), "data/logs/game_log_%02d%02d%04d_%02d%02d%02d.txt",
             tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
             tm.tm_hour, tm.tm_min, tm.tm_sec);

    FILE* f = fopen(filename, "w");
    if (!f) return;

    fprintf(f, "========================================\n");
    fprintf(f, "  REPORT PARTITA UNO\n");
    fprintf(f, "========================================\n");
    fprintf(f, "Data: %02d/%02d/%04d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
    fprintf(f, "Ora: %02d:%02d\n", tm.tm_hour, tm.tm_min);
    fprintf(f, "Durata: %02d:%02d\n", (int)gioco->durata_partita / 60, (int)gioco->durata_partita % 60);
    fprintf(f, "Esito: %s\n", esito);
    fprintf(f, "Numero giocatori: %d\n", gioco->num_giocatori);
    fprintf(f, "\n");

    ClassificaRecord classifica[4];
    int num = 0;
    CalcolaClassifica(gioco, classifica, &num);
    fprintf(f, "CLASSIFICA FINALE:\n");
    fprintf(f, "------------------\n");
    for (int i = 0; i < num; i++) {
        fprintf(f, "  %d. %s - %d carte %s\n",
                classifica[i].posizione, classifica[i].nome, classifica[i].num_carte,
                classifica[i].posizione == 1 ? "(VINCITORE)" : "");
    }
    fprintf(f, "\nGenerato da UNO - Secure Multi-Profile Ed.\n");
    fclose(f);
}

/**
 * @brief Disegna la schermata di fine partita con classifica e opzioni.
 *
 * Gestisce il rendering di:
 *   - Titolo con messaggio di vittoria
 *   - Classifica finale (1°, 2°, 3° posti)
 *   - Tempo di gioco
 *   - Pulsanti: "Gioca Ancora", "Torna al Menu", "Esci"
 *   - Notifiche live (es. "Client X e' pronto in lobby")
 *
 * Gestisce anche la transizione automatica per i client in attesa
 * (BUG 4 FIX: client_play_again_attesa + host_in_prelobby).
 *
 * @param gioco Puntatore allo stato di gioco.
 * @param fase Puntatore alla fase applicazione (puo' cambiare a FASE_MENU, FASE_HOST_LOBBY, FASE_CLIENT_LOBBY).
 * @param mousePos Posizione corrente del mouse.
 */
void DisegnaFinePartita(StatoGioco* gioco, FaseApplicazione* fase, Vector2 mousePos) {
    (void)mousePos;  /* parametro mantenuto per uniformita' API (non usato nel rendering) */
    if (!gioco || !fase) return;

    DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.85f));

    int fontSize = 60;
    int titoloW = MeasureText(gioco->messaggio, fontSize);
    DrawText(gioco->messaggio, LARGHEZZA/2 - titoloW/2 + 2, 132, fontSize, BLACK);
    DrawText(gioco->messaggio, LARGHEZZA/2 - titoloW/2, 130, fontSize, GOLD);

    int boxW = 500, boxH = 200;
    int boxX = LARGHEZZA/2 - boxW/2;
    int boxY = 250;

    DrawRectangleRounded((Rectangle){ (float)boxX, (float)boxY, (float)boxW, (float)boxH },
                         0.1f, 10, Fade(DARKGRAY, 0.9f));
    DrawRectangleRoundedLines((Rectangle){ (float)boxX, (float)boxY, (float)boxW, (float)boxH },
                              0.1f, 10, GOLD);

    DrawText("CLASSIFICA FINALE", LARGHEZZA/2 - MeasureText("CLASSIFICA FINALE", 28)/2, boxY + 10, 28, YELLOW);

    Color coloriPos[] = { GOLD, LIGHTGRAY, (Color){205, 127, 50, 255}, WHITE };
    const char* medaglie[] = { "1", "2", "3", "4" };

    ClassificaRecord classifica[4];
    int num = 0;
    CalcolaClassifica(gioco, classifica, &num);

    int rowY = boxY + 50;
    for (int i = 0; i < num && i < 4; i++) {
        Color posColor = coloriPos[i < 4 ? i : 3];
        Rectangle row = { (float)(boxX + 20), (float)(rowY + i * 36), (float)(boxW - 40), 32 };
        DrawRectangleRounded(row, 0.15f, 8, Fade(posColor, 0.2f));

        char playerInfo[128];
        if (classifica[i].posizione == 1) {
            snprintf(playerInfo, sizeof(playerInfo), "%s  %s  (%d carte)  VINCITORE",
                     medaglie[i], classifica[i].nome, classifica[i].num_carte);
        } else {
            snprintf(playerInfo, sizeof(playerInfo), "%s  %s  (%d carte)",
                     medaglie[i], classifica[i].nome, classifica[i].num_carte);
        }
        DrawText(playerInfo, row.x + 10, row.y + 8, 18, posColor);
    }

    int tempoY = boxY + boxH + 20;
    char tempoStr[50];
    snprintf(tempoStr, sizeof(tempoStr), "Tempo: %02d:%02d",
             (int)gioco->durata_partita / 60, (int)gioco->durata_partita % 60);
    DrawText(tempoStr, LARGHEZZA/2 - MeasureText(tempoStr, 20)/2, tempoY, 20, LIGHTGRAY);

    int btnY = tempoY + 45;
    int host_attivo = (currentRole == NET_HOST);
    int client_attivo = (currentRole == NET_CLIENT && isConnected);
    int rete_ancora_attiva = host_attivo || client_attivo;

    // ================================================================
    // NOTIFICHE LIVE: Mostra notifiche in stile gioco in basso a destra.
    // L'host vede i messaggi "Client A e' pronto in lobby" quando un
    // client preme "Gioca Ancora" (RICHIESTA 1).
    // ================================================================
    if (gioco->timer_notifica > 0.0f && gioco->notifica[0] != '\0') {
        int notifW = MeasureText(gioco->notifica, 18) + 40;
        Rectangle notifRect = { LARGHEZZA - notifW - 20, ALTEZZA - 80, (float)notifW, 60 };
        DrawRectangleRounded(notifRect, 0.3f, 10, Fade(BLACK, 0.85f));
        DrawText(gioco->notifica, notifRect.x + 20, notifRect.y + 20, 18, WHITE);
    }

    // ================================================================
    // BUG 4 FIX (CLIENT ATTESA): Il client che ha premuto "Gioca Ancora"
    // ma l'host non e' ancora tornato in pre-lobby, ora transita
    // automaticamente in lobby quando host_in_prelobby diventa 1.
    // client_play_again_attesa viene impostato quando il client clicca
    // "Gioca Ancora" e host_in_prelobby == 0. host_in_prelobby viene
    // settato a 1 da PACKET_HOST_RETURN_LOBBY.
    //
    // Questo pattern e' IDENTICO a gameplay_draw.c per evitare
    // inconsistenza tra le due schermate (errore che causava la
    // "partita fantasma" descritta nel bug 4).
    // ================================================================
    if (currentRole == NET_CLIENT && (client_play_again_attesa || host_in_prelobby)) {
        // Entra in lobby se: ha premuto "Gioca Ancora" E l'host e' tornato in lobby
        if (client_play_again_attesa && host_in_prelobby) {
            client_play_again_attesa = 0;
            gioco->num_carte_scarti = 0;
            *fase = FASE_CLIENT_LOBBY;
            return;
        }
        // Se l'host e' tornato in lobby ma il client non ha premuto "Gioca Ancora"
        // rimane nella schermata finale (non entra in lobby automaticamente)
    }

    if (rete_ancora_attiva) {
        if (currentRole == NET_HOST) {
            // ================================================================
            // BUG 4 FIX (HOST): "Gioca Ancora" -> torna in Pre-Lobby.
            // La stanza rimane attiva, slot vuoti fino a connessioni.
            //
            // CORREZIONE: Quando l'host preme "Gioca Ancora", PULISCE gli slot
            // degli ALTRI giocatori (tranne se stesso/slot 0) per far si' che
            // la lobby mostri solo l'host come giocatore. Gli slot si
            // riempiono man mano che i client premono "Gioca Ancora"
            // (PACKET_PLAY_AGAIN ricevuto dall'host).
            //
            // Senza questa pulizia, la lobby mostrava ancora tutti i giocatori
            // della partita appena conclusa, causando confusione e slot
            // "occupati" anche se i client non avevano ancora deciso se
            // rigiocare o meno.
            // ================================================================
            if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 250, (float)btnY, 220, 50 }, "Gioca Ancora")) {
                // BUG 4 FIX: Pulisce gli slot degli ALTRI giocatori (indice > 0) prima
                // di tornare in lobby. Questo garantisce che la lobby mostri solo l'host
                // come giocatore iniziale. I client riempiranno i propri slot quando
                // premeranno "Gioca Ancora" tramite PACKET_PLAY_AGAIN.
                for (int i = 1; i < gioco->num_giocatori; i++) {
                    memset(&gioco->giocatori[i], 0, sizeof(Giocatore));
                    gioco->giocatori[i].mano = CreaLista();
                    gioco->giocatori[i].is_bot = 0;
                    gioco->giocatori[i].stato_pronto = 0;
                    gioco->giocatori[i].socket_id = -1;
                }
                // Imposta num_giocatori a 1 (solo l'host) finche' i client non si riuniscono
                gioco->num_giocatori = 1;
                
                // BUG 4 FIX: Mostra notifica in stile gioco in basso a destra
                // per l'host stesso, informandolo che e' tornato in lobby e puo'
                // attendere i client che vogliono rigiocare.
                MostraNotifica(gioco, "Lobby riaperta! I client possono riunirsi.");
                
                BroadcastHostReturnLobby();
                *fase = FASE_HOST_LOBBY;
                host_in_prelobby = 1;
                gioco->gioco_finito = 0;
                gioco->num_carte_scarti = 0;
                gioco->num_chat_msg = 0;
                // BUG 3 FIX: NON resettare statistiche_gia_salvate (RICHIESTA 5)
                for(int i=0; i<15; i++) gioco->chat_history[i][0] = '\0';
            }
            // "Torna al Menu" -> chiude la stanza permanentemente, avvisa client.
            if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 30, (float)btnY, 220, 50 }, "Torna al Menu")) {
                BroadcastHostDisconnect();
                CloseNetwork();
                currentRole = NET_OFFLINE;
                *fase = FASE_MENU;
            }
        } else {
            // ================================================================
            // BUG 4 FIX (CLIENT): "Gioca Ancora" o "Esci".
            //
            // CORREZIONE: Il client NON va piu' direttamente a FASE_CLIENT_LOBBY
            // se host_in_prelobby == 0. Invece, imposta client_play_again_attesa = 1
            // e rimane nella schermata finale. Quando arriva PACKET_HOST_RETURN_LOBBY
            // (che setta host_in_prelobby = 1), il loop sopra transita automaticamente
            // a FASE_CLIENT_LOBBY. Questo pattern e' identico a gameplay_draw.c.
            //
            // Preveniva il bug della "partita fantasma": il client andava in lobby
            // anche se l'host era ancora nella schermata finale, causando uno stato
            // inconsistente in cui il client era in lobby ma l'host non lo sapeva.
            // ================================================================
            if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 250, (float)btnY, 220, 50 }, "Gioca Ancora")) {
                SendPlayAgain();
                gioco->num_carte_scarti = 0;
                // BUG 4 FIX: Se host_in_prelobby e' gia' 1, vai subito in lobby
                if (host_in_prelobby) {
                    *fase = FASE_CLIENT_LOBBY;
                } else {
                    // BUG 4 FIX: L'host non e' ancora tornato in pre-lobby.
                    // Il flag client_play_again_attesa (definito in end_screen.c)
                    // viene impostato per permettere la transizione automatica
                    // quando arriva PACKET_HOST_RETURN_LOBBY dall'host.
                    client_play_again_attesa = 1;
                    gioco->timer_notifica = 5.0f;
                    strcpy(gioco->notifica, "In attesa che l'Host torni in lobby...");
                }
                // NON resettare statistiche_gia_salvate (RICHIESTA 5)
                gioco->num_chat_msg = 0;
                for(int i=0; i<15; i++) gioco->chat_history[i][0] = '\0';
            }
            // RICHIESTA 2 (CLIENT): "Esci" -> torna al menu principale
            if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 30, (float)btnY, 220, 50 }, "Esci")) {
                CloseNetwork();
                currentRole = NET_OFFLINE;
                *fase = FASE_MENU;
            }
        }
    } else {
        // Nessuna rete attiva: controlla se era multiplayer con host disconnesso
        if (strstr(gioco->messaggio, "__HOST_DISCONNECT__") != NULL) {
            if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 110, (float)btnY, 220, 50 }, "Esci")) {
                CloseNetwork();
                currentRole = NET_OFFLINE;
                *fase = FASE_MULTIPLAYER_SCELTA;
            }
        } else {
            // Single player normale
            if (currentRole == NET_OFFLINE) {
                if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 190, (float)btnY, 175, 50 }, "Torna al Menu")) {
                    *fase = FASE_MENU;
                }
                if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 15, (float)btnY, 175, 50 }, "Gioca Ancora")) {
                    int num_avv = (gioco->num_giocatori - 1 > 0) ? gioco->num_giocatori - 1 : 3;
                    EliminaPartita(gioco);
                    NuovaPartita(gioco, num_avv);
                    *fase = FASE_GIOCO;
                }
            } else {
                // Client connesso ma rete non attiva (caso anomalo): solo esci
                if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 110, (float)btnY, 220, 50 }, "Esci")) {
                    CloseNetwork();
                    currentRole = NET_OFFLINE;
                    *fase = FASE_MULTIPLAYER_SCELTA;
                }
            }
        }
    }
}