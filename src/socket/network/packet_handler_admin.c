/**
 * @file packet_handler_admin.c
 * @brief Gestione pacchetti admin e sessione di rete.
 * @ingroup network
 *
 * @details Modulo separato per gestire i pacchetti relativi
 *          all'amministrazione e alla gestione sessioni di gioco.
 *          Responsabile di:
 *          - Creazione/join sessioni multiplayer
 *          - Sincronizzazione database utenti via TCP (admin panel)
 *          - Gestione "Gioca Ancora" e ritorno in lobby
 *          - Sincronizzazione statistiche post-partita
 *          - Tracking connessioni client TCP
 *
 * @note Il database utenti è DINAMICO (MAX_UTENTI rimosso).
 *       Le operazioni di sincronizzazione admin usano DB_Alloca() per
 *       garantire spazio sufficiente prima di aggiungere nuovi utenti.
 *       I contatori (partite_giocate, vittorie) sono READ-ONLY per l'admin
 *       e vengono gestiti SOLO da PACKET_STATS_SYNC dopo ogni partita.
 *
 * File correlati:
 *   - packet_handler.c  : dispatcher principale
 *   - network_send.c    : BroadcastAdminDB, SendStatsSync
 *   - lan_sync.h        : LAN_BroadcastStatsSync, LAN_BroadcastNow
 *   - game_logic.h      : CalcolaClassifica
 *   - end_screen.h      : gestione fine partita
 *
 * @author Pasquale Franco
 * @version 9.1.3
 * @date 2026-07-01
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "raylib.h"

#include "../../../lib/socket/network/packet_handler_admin.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/socket/lan/lan_sync.h"
#include "../../../lib/socket/network/state_sync.h"
#include "../../../lib/socket/server/server_manager.h"
#include "../../../lib/game_logic.h"
#include "../../../lib/ui.h"
#include "../../../lib/list.h"
#include "../../../lib/auth.h"
#include "../../../lib/data_structures/bst.h"
#include "../../../lib/screens/game/gameplay_screen.h"
#include "../../../lib/screens/menu/string_utils.h"

/**
 * @brief Gestisce i pacchetti admin e sessione.
 *
 * Gestisce: CREATE_SESSION, JOIN_SESSION, PLAYER_READY,
 * GAME_FINISHED, HEARTBEAT, ADMIN_USER_SYNC, ADMIN_DB_SYNC.
 *
 * @param packet Pacchetto ricevuto.
 * @param gioco Stato del gioco.
 */
void HandleAdminPacket(NetPacket* packet, StatoGioco* gioco) {
    if (!packet || !gioco) return;

    switch (packet->type) {
        /* ============================================================
         *  SESSION MANAGEMENT
         * ============================================================ */
        case PACKET_CREATE_SESSION: {
            if (currentRole == NET_HOST) {
                GameSession* session = CreateSession(packet->data.create_session.host_name, packet->sender_id);
                if (session) {
                    strcpy(current_session_id, session->session_id);
                    local_player_id = 0;
                    MostraNotifica(gioco, TextFormat("Sessione creata: %s", session->session_id));
                }
            }
            break;
        }
        case PACKET_JOIN_SESSION: {
            if (currentRole == NET_HOST) {
                GameSession* session = GetSession(packet->data.join_session.session_id);
                if (session) {
                    int bot_slot = -1;
                    for (int j = 1; j < gioco->num_giocatori; j++) {
                        if (gioco->giocatori[j].is_bot) {
                            bot_slot = j;
                            break;
                        }
                    }

                    if (bot_slot != -1) {
                        int slot = bot_slot;
                        strcpy(gioco->giocatori[slot].nome, packet->data.join_session.player_name);
                        gioco->giocatori[slot].is_bot = 0;
                        strcpy(session->players[slot].nome, packet->data.join_session.player_name);
                        session->players[slot].player_id = packet->sender_id;
                        session->players[slot].is_connected = 1;
                        MostraNotifica(gioco, TextFormat("%s e' entrato in gioco al posto di un Bot!", packet->data.join_session.player_name));
                    } else if (session->num_players < 4) {
                        int slot = session->num_players;
                        if (AddPlayerToSession(session, packet->sender_id, packet->data.join_session.player_name, 0)) {
                            gioco->num_giocatori = session->num_players;
                            strcpy(gioco->giocatori[slot].nome, packet->data.join_session.player_name);
                            gioco->giocatori[slot].is_bot = 0;
                            MostraNotifica(gioco, TextFormat("%s e' entrato in gioco!", packet->data.join_session.player_name));
                        }
                    }
                }
            }
            break;
        }
        case PACKET_PLAYER_READY: {
            GameSession* session = GetSessionByPlayerId(packet->sender_id);
            if (session && currentRole == NET_HOST) {
                for (int i = 0; i < session->num_players; i++) {
                    if (session->players[i].player_id == packet->sender_id) {
                        MostraNotifica(gioco, TextFormat("%s e' pronto!", session->players[i].nome));
                        break;
                    }
                }
                BroadcastGameState(gioco);
            }
            break;
        }
        /**
         * @brief PACKET_PLAY_AGAIN: Un client ha premuto "Gioca Ancora"
         *        nella schermata finale.
         *
         * Comportamento:
         * 1. Mostra una notifica in basso a destra stile notifiche di gioco
         *    ("Client A e' pronto in lobby").
         * 2. Se l'host e' gia' in pre-lobby (host_in_prelobby == 1), segna il
         *    client come pronto e broadcasta lo stato aggiornato.
         * 3. Se l'host e' ancora nella schermata finale, la notifica informa
         *    l'host che il client e' in attesa.
         *
         * Meccanismo:
         * Quando l'host ha premuto "Gioca Ancora", pulisce gli slot
         * degli ALTRI giocatori e setta num_giocatori = 1 (solo host). Quando
         * un client manda PACKET_PLAY_AGAIN, il suo player_id (pid) potrebbe
         * essere >= num_giocatori. In tal caso, non accedere a giocatori[pid]
         * ma AGGIUNGI il client come nuovo giocatore in lobby.
         *
         * Lo slot del client viene trovato tramite socket_id (packet->sender_id):
         * se il client si riconnette dal vecchio slot 1-3, lo riattiviamo.
         * Se num_giocatori < 4, lo aggiungiamo come nuovo slot.
         */
        /**
         * @brief PACKET_PLAY_AGAIN: Un client ha premuto "Gioca Ancora"
         *        nella schermata finale.
         *
         * Meccanismo completo:
         * 1. Quando il client preme "Gioca Ancora", invia PACKET_PLAY_AGAIN
         *    all'host con il proprio nome (player_name) e il socket_id (sender_id).
         * 2. Se l'host e' gia' in pre-lobby (host_in_prelobby == 1):
         *    - Cerca lo slot del client per socket_id (1-3)
         *    - Se trovato: lo segna come pronto e notifica "Client X e' pronto in lobby"
         *    - Se NON trovato: lo aggiunge come nuovo giocatore in lobby
         * 3. Se l'host e' ANCORA nella schermata finale (host_in_prelobby == 0):
         *    - Notifica in stile gioco: "Client X e' in attesa di rigiocare..."
         *    - Questa notifica appare in basso a destra nell'end screen dell'host
         *    - Il nome del client viene estratto dal campo player_name[] del pacchetto
         *
         * Riconnessione: Il sender_id del pacchetto (1-3) viene usato
         * come socket_id per identificare univocamente il client in fase di
         * re-join, permettendo la corrispondenza anche dopo la rimozione del bot.
         */
        case PACKET_PLAY_AGAIN: {
            if (currentRole == NET_HOST) {
                int pid = packet->data.play_again.player_id;
                int sid = packet->sender_id; // Socket ID (1-3)
                
                if (host_in_prelobby) {
                    /* ============================================================
                     * L'host e' gia' in pre-lobby.
                     * num_giocatori e' stato resettato a 1 (solo host allo slot 0).
                     * Il client che vuole rigiocare deve essere AGGIUNTO come
                     * nuovo giocatore nella lobby, non aggiornato a un indice
                     * che potrebbe essere out-of-bounds.
                     * ============================================================ */
                    
                    int trovato_slot = -1;
                    
                    /* Cerca se questo client ha gia' uno slot nella lobby
                     * (per socket_id o per nome dal pacchetto) */
                    for (int i = 1; i < gioco->num_giocatori; i++) {
                        if (gioco->giocatori[i].socket_id == sid) {
                            trovato_slot = i;
                            break;
                        }
                    }
                    
                    /* Se non trovato per socket_id, cerca se il nome
                     * del client e' gia' presente (caso di riconnessione dopo
                     * che l'host ha resettato la lobby) */
                    const char* player_name_from_packet = packet->data.play_again.player_name;
                    if (trovato_slot == -1 && player_name_from_packet && player_name_from_packet[0]) {
                        for (int i = 1; i < gioco->num_giocatori; i++) {
                            if (strcmp(gioco->giocatori[i].nome, player_name_from_packet) == 0) {
                                trovato_slot = i;
                                break;
                            }
                        }
                    }
                    
                    if (trovato_slot == -1 && gioco->num_giocatori < 4) {
                        /* Nuovo slot per il client */
                        trovato_slot = gioco->num_giocatori;
                        
                        char nome_player[30] = "";
                        if (player_name_from_packet && player_name_from_packet[0]) {
                            strncpy(nome_player, player_name_from_packet, sizeof(nome_player) - 1);
                        } else {
                            /* Fallback: usa i dati residui della partita precedente */
                            for (int i = 1; i < 4; i++) {
                                if (gioco->giocatori[i].socket_id == sid && gioco->giocatori[i].nome[0]) {
                                    strncpy(nome_player, gioco->giocatori[i].nome, sizeof(nome_player) - 1);
                                    break;
                                }
                            }
                            if (nome_player[0] == '\0') {
                                snprintf(nome_player, sizeof(nome_player), "Giocatore %d", trovato_slot + 1);
                            }
                        }
                        
                        /* Crea il nuovo slot per il client */
                        memset(&gioco->giocatori[trovato_slot], 0, sizeof(Giocatore));
                        strncpy(gioco->giocatori[trovato_slot].nome, nome_player, sizeof(gioco->giocatori[trovato_slot].nome) - 1);
                        gioco->giocatori[trovato_slot].mano = CreaLista();
                        gioco->giocatori[trovato_slot].is_bot = 0;
                        gioco->giocatori[trovato_slot].stato_pronto = 1; /* Pronto per giocare */
                        gioco->giocatori[trovato_slot].socket_id = sid;
                        gioco->num_giocatori = trovato_slot + 1;
                        
                        MostraNotifica(gioco, TextFormat("%s e' pronto in lobby", nome_player));
                    } else if (trovato_slot != -1) {
                        /* Client gia' presente in lobby - segna come pronto */
                        gioco->giocatori[trovato_slot].stato_pronto = 1;
                        MostraNotifica(gioco, TextFormat("%s e' pronto in lobby", Giocatore_Nome(&gioco->giocatori[trovato_slot])));
                    }
                    
                    BroadcastGameState(gioco);
                } else {
                    /* ============================================================
                     * L'host e' ANCORA nella schermata finale.
                     * Mostra notifica in stile gioco in basso a destra dell'end screen.
                     * Il nome del client viene estratto dal campo player_name[]
                     * del pacchetto, o dai dati residui della partita precedente.
                     * ============================================================ */
                    const char* nome_cliente = "Giocatore";
                    
                    /* Cerca il nome prima dal campo player_name del pacchetto */
                    if (packet->data.play_again.player_name[0]) {
                        nome_cliente = packet->data.play_again.player_name;
                    }
                    /* Poi dagli slot residui della partita precedente */
                    else if (pid >= 0 && pid < 4 && gioco->giocatori[pid].nome[0]) {
                        nome_cliente = Giocatore_Nome(&gioco->giocatori[pid]);
                    } else if (sid >= 1 && sid <= 3 && sid < 4 && gioco->giocatori[sid].nome[0]) {
                        nome_cliente = Giocatore_Nome(&gioco->giocatori[sid]);
                    }
                    
                    MostraNotifica(gioco, TextFormat("%s e' in attesa di rigiocare...", nome_cliente));
                }
            }
            break;
        }
        /**
         * @brief PACKET_HOST_RETURN_LOBBY: L'host e' tornato in pre-lobby
         *        (ha premuto "Gioca Ancora").
         *
         * Comportamento:
         * 1. Imposta host_in_prelobby = 1 per permettere la transizione
         *    dalla schermata finale alla lobby.
         * 2. Resetta gioco_finito e num_carte_scarti per prepararsi
         *    a una nuova partita.
         * 3. NON resetta statistiche_gia_salvate per non duplicare il salvataggio.
         */
        case PACKET_HOST_RETURN_LOBBY: {
            if (currentRole == NET_CLIENT) {
                host_in_prelobby = 1;
                gioco->gioco_finito = 0;
                gioco->num_carte_scarti = 0;
                // NON resettare statistiche_gia_salvate: sono gia' state
                // salvate quando la partita e' finita e non devono essere duplicati.
                
                // Mostra notifica in stile gioco in basso a destra
                // per informare il client che l'host e' tornato in lobby.
                // Il client puo' cosi' decidere se premere "Gioca Ancora" per riunirsi
                // o "Esci" per andarsene. La notifica usa lo stesso stile di gioco
                // (MostraNotifica) per coerenza visiva.
                MostraNotifica(gioco, "L'host e' pronto in lobby!");
            }
            break;
        }
        case PACKET_GAME_FINISHED: {
            if (packet->data.game_finished.winning_player_id == local_player_id) {
                MostraNotifica(gioco, TextFormat("HAI VINTO! %s!", packet->data.game_finished.winner_name));
            } else {
                MostraNotifica(gioco, TextFormat("%s HA VINTO!", packet->data.game_finished.winner_name));
            }
            gioco->gioco_finito = 1;
            break;
        }
        case PACKET_HEARTBEAT: {
            GameSession* session = GetSessionByPlayerId(packet->sender_id);
            if (session) {
                for (int i = 0; i < session->num_players; i++) {
                    if (session->players[i].player_id == packet->sender_id) {
                        session->players[i].lastHeartbeat = GetTime();
                        break;
                    }
                }
            }
            break;
        }

        /* ============================================================
         *  ADMIN SYNC - Sincronizzazione database utenti (DINAMICO)
         * ============================================================ */
        case PACKET_ADMIN_USER_SYNC: {
            /**
             * @brief L'host riceve i dati utente da un client collegato.
             *
             * Dopo aver aggiunto il nuovo utente al database locale,
             * l'host chiama BroadcastAdminDB() per propagare l'aggiornamento
             * a TUTTI i client TCP connessi (non solo quello che ha inviato
             * i dati). Senza questo, il pannello admin degli altri client
             * non vede il nuovo utente finche' non arriva il prossimo broadcast
             * LAN periodico (ogni LAN_SYNC_INTERVAL secondi).
             *
             * BroadcastAdminDB() invia un pacchetto PACKET_ADMIN_DB_SYNC per
             * ogni utente nel database, seguito da un pacchetto con
             * user_index=-1 per segnalare il completamento della sincronizzazione.
             *
             * Tracking connessioni TCP:
             * Memorizza l'username del client collegato in connectedClientUsernames[].
             * Questo permette all'admin panel di sapere quali utenti sono connessi
             * via TCP e non eliminarli immediatamente dal DB.
             */
            if (currentRole == NET_HOST) {
                const AdminUserAction* act = &packet->data.admin_action;
                const AdminUserSyncData* u = &act->user;

                /* Memorizza l'username del client TCP connesso (sender_id = slot 1-3) */
                int slot = packet->sender_id;
                if (slot >= 1 && slot <= 3 && u->username[0]) {
                    strncpy(connectedClientUsernames[slot], u->username, 29);
                    connectedClientUsernames[slot][29] = '\0';
                }

                if (act->action == 0) {
                    int found = -1;
                    for (int i = 0; i < dbUtenti.num_utenti; i++) {
                        if (strcmp(dbUtenti.lista[i].username, u->username) == 0) {
                            found = i;
                            break;
                        }
                    }

                    if (found == -1) {
                        DB_Alloca(dbUtenti.num_utenti + 1);
                        if (dbUtenti.num_utenti < dbUtenti.capacita) {
                            Statistiche* s = &dbUtenti.lista[dbUtenti.num_utenti];
                            memset(s, 0, sizeof(Statistiche));
                            strncpy(s->email, u->email, sizeof(s->email) - 1);
                            strncpy(s->nome, u->nome, sizeof(s->nome) - 1);
                            strncpy(s->cognome, u->cognome, sizeof(s->cognome) - 1);
                            strncpy(s->username, u->username, sizeof(s->username) - 1);
                            strncpy(s->password, u->password, sizeof(s->password) - 1);
                            strncpy(s->pin_recupero, u->pin_recupero, sizeof(s->pin_recupero) - 1);
                            s->partite_giocate = u->partite_giocate;
                            s->vittorie_giocatore = u->vittorie_giocatore;
                            s->vittorie_bot = u->vittorie_bot;
                            dbUtenti.num_utenti++;
                            SalvaDB();
                            AuthBST_Ricostruisci();
                            BroadcastAdminDB();
                        }
                    }
                } else {
                    int target = -1;
                    if (u->username[0]) {
                        for (int i = 0; i < dbUtenti.num_utenti; i++) {
                            if (strcmp(dbUtenti.lista[i].username, u->username) == 0) {
                                target = i;
                                break;
                            }
                        }
                    }
                    if (target == -1 && act->target_id >= 0 && act->target_id < dbUtenti.num_utenti) {
                        target = act->target_id;
                    }

                    if (act->action == 1 && target >= 0 && target < dbUtenti.num_utenti) {
                        Statistiche* s = &dbUtenti.lista[target];
                        char old_username[30];
                        strncpy(old_username, s->username, sizeof(old_username) - 1);
                        old_username[sizeof(old_username) - 1] = '\0';
                        int old_username_changed = (strcmp(old_username, u->username) != 0);
                        strncpy(s->email, u->email, sizeof(s->email) - 1);
                        strncpy(s->nome, u->nome, sizeof(s->nome) - 1);
                        strncpy(s->cognome, u->cognome, sizeof(s->cognome) - 1);
                        strncpy(s->username, u->username, sizeof(s->username) - 1);
                        strncpy(s->password, u->password, sizeof(s->password) - 1);
                        strncpy(s->pin_recupero, u->pin_recupero, sizeof(s->pin_recupero) - 1);
                        SalvaDB();
                        AuthBST_Ricostruisci();
                        if (old_username_changed) {
                            for (int i = 1; i < 4; i++) {
                                if (connectedClientUsernames[i][0] && strcmp(connectedClientUsernames[i], old_username) == 0) {
                                    strncpy(connectedClientUsernames[i], u->username, sizeof(connectedClientUsernames[i]) - 1);
                                }
                            }
                        }
                        BroadcastAdminDB();
                    } else if (act->action == 2 && target >= 0 && target < dbUtenti.num_utenti) {
                        char del_username[30] = "";
                        strncpy(del_username, dbUtenti.lista[target].username, sizeof(del_username) - 1);
                        for (int j = target; j < dbUtenti.num_utenti - 1; j++) {
                            dbUtenti.lista[j] = dbUtenti.lista[j+1];
                        }
                        dbUtenti.num_utenti--;
                        AuthBST_Ricostruisci();
                        SalvaDB();
                        LAN_BroadcastDeleteUser(del_username);
                        /* Se l'utente era connesso via TCP, disconnettilo subito. */
                        for (int i = 1; i < 4; i++) {
                            if (connectedClientUsernames[i][0] && strcmp(connectedClientUsernames[i], del_username) == 0) {
                                if (clientSockets[i] != INVALID_SOCKET) {
                                    shutdown(clientSockets[i], SD_BOTH);
                                    closesocket(clientSockets[i]);
                                    clientSockets[i] = INVALID_SOCKET;
                                }
                                connectedClientUsernames[i][0] = '\0';
                                break;
                            }
                        }
                        BroadcastAdminDB();
                    } else if (act->action == 3) {
                        int found = -1;
                        for (int i = 0; i < dbUtenti.num_utenti; i++) {
                            if (strcmp(dbUtenti.lista[i].username, u->username) == 0) {
                                found = i;
                                break;
                            }
                        }
                        if (found == -1) {
                            DB_Alloca(dbUtenti.num_utenti + 1);
                            if (dbUtenti.num_utenti < dbUtenti.capacita) {
                                Statistiche* s = &dbUtenti.lista[dbUtenti.num_utenti];
                                memset(s, 0, sizeof(Statistiche));
                                strncpy(s->email, u->email, sizeof(s->email) - 1);
                                strncpy(s->nome, u->nome, sizeof(s->nome) - 1);
                                strncpy(s->cognome, u->cognome, sizeof(s->cognome) - 1);
                                strncpy(s->username, u->username, sizeof(s->username) - 1);
                                strncpy(s->password, u->password, sizeof(s->password) - 1);
                                strncpy(s->pin_recupero, u->pin_recupero, sizeof(s->pin_recupero) - 1);
                                s->partite_giocate = u->partite_giocate;
                                s->vittorie_giocatore = u->vittorie_giocatore;
                                s->vittorie_bot = u->vittorie_bot;
                                dbUtenti.num_utenti++;
                                SalvaDB();
                                AuthBST_Ricostruisci();
                                BroadcastAdminDB();
                            }
                        }
                    }
                }
            }
            break;
        }

        case PACKET_STATS_SYNC: {
            /**
             * @brief Sincronizzazione live delle statistiche dopo una partita.
             * STATISTICHE READ-ONLY: Questo e' l'UNICO canale che modifica
             * partite_giocate/vittore. L'admin panel NON puo' toccare
             * questi campi. Solo il gioco stesso tramite PACKET_STATS_SYNC
             * puo' aggiornare le statistiche.
             *
             * Ora merge anche l'intero array storico_partite[]
             * trasmesso nel pacchetto StatsSyncPacket esteso. Senza questo,
             * i client ricevono solo i contatori numerici ma non i dettagli
             * delle partite storiche, causando campi "??????" nella schermata
             * statistiche.
             *
             * La logica di merge e' identica a quella in lan_broadcast.c
             * per LAN_PACKET_STATS_SYNC, per garantire consistenza tra
             * sincronizzazione TCP e UDP.
             */
            const StatsSyncPacket* sp = &packet->data.stats_sync;
            if (sp->username[0] == '\0') break;
            
            int modified = 0;
            
            // Cerca l'utente nel DB locale per username
            for (int i = 0; i < dbUtenti.num_utenti; i++) {
                if (strcmp(dbUtenti.lista[i].username, sp->username) == 0) {
                    Statistiche* s = &dbUtenti.lista[i];
                    
                    // Aggiorna contatori se il remoto ne ha di piu' (cresce solo)
                    if (sp->partite_giocate > s->partite_giocate) {
                        s->partite_giocate = sp->partite_giocate;
                        s->vittorie_giocatore = sp->vittorie_giocatore;
                        s->vittorie_bot = sp->vittorie_bot;
                        modified = 1;
                    }
                    
                    // Merge dello storico partite COMPLETO dal pacchetto StatsSync
                    // Usa la stessa logica di deduplica di lan_db_sync.c e lan_broadcast.c:
                    // (durata, esito, modalita, posizione) come chiave composta.
                    for (int h = 0; h < sp->storico_count && h < MAX_PARTITE_STORICO; h++) {
                        const PartitaRegistrata* remota = &sp->storico_partite[h];
                        int gia_presente = 0;
                        int max_local = (s->partite_giocate < MAX_PARTITE_STORICO) ? s->partite_giocate : MAX_PARTITE_STORICO;
                        for (int lh = 0; lh < max_local; lh++) {
                            PartitaRegistrata* locale = &s->storico_partite[lh];
                            if (locale->durata_secondi == remota->durata_secondi &&
                                strcmp(locale->esito, remota->esito) == 0 &&
                                strcmp(locale->modalita, remota->modalita) == 0 &&
                                locale->posizione == remota->posizione) {
                                gia_presente = 1;
                                break;
                            }
                        }
                        if (!gia_presente && s->partite_giocate < MAX_PARTITE_STORICO) {
                            s->storico_partite[s->partite_giocate] = *remota;
                            s->partite_giocate++;
                            if (strcmp(remota->esito, "Vittoria") == 0) {
                                s->vittorie_giocatore++;
                            } else {
                                s->vittorie_bot++;
                            }
                            modified = 1;
                        }
                    }
                    break;
                }
            }
            
            // Se sono io il giocatore, aggiorna anche l'utente loggato (gia' fatto sopra se i==id_utente_corrente)
            if (id_utente_corrente >= 0 && id_utente_corrente < dbUtenti.num_utenti &&
                strcmp(dbUtenti.lista[id_utente_corrente].username, sp->username) == 0) {
                MostraNotifica(gioco, TextFormat("Statistiche aggiornate: %s - Vittorie: %d", sp->username, dbUtenti.lista[id_utente_corrente].vittorie_giocatore));
            }
            
            // Persisti subito su file se ci sono modifiche
            if (modified) {
                SalvaDB();
            }
            break;
        }
        case PACKET_ADMIN_DB_SYNC: {
            /* Un client riceve un singolo utente o il segnale di fine sync dall'host */
            if (currentRole == NET_CLIENT) {
                const AdminDBSyncPacket* dbp = &packet->data.admin_db_packet;
                
                // Salva l'EMAIL dell'utente loggato PRIMA di sovrascrivere il DB
                // per ri-trovarlo dopo il sync (l'indice potrebbe cambiare).
                char savedUserEmail[100] = "";
                if (id_utente_corrente >= 0 && id_utente_corrente < dbUtenti.num_utenti) {
                    strncpy(savedUserEmail, dbUtenti.lista[id_utente_corrente].email, sizeof(savedUserEmail) - 1);
                }
                
                if (dbp->user_index == -1) {
                    // Segnale di sync completato: riassegna id_utente_corrente
                    if (savedUserEmail[0] != '\0') {
                        int found = -1;
                        for (int i = 0; i < dbUtenti.num_utenti; i++) {
                            if (strcmp(dbUtenti.lista[i].email, savedUserEmail) == 0) {
                                found = i;
                                break;
                            }
                        }
                        if (found != -1) {
                            id_utente_corrente = found;
                        }
                    }
                    
                    SalvaDB();
                    AuthBST_Ricostruisci();
                    break;
                }
                
                // Riceviamo un singolo utente: alloca spazio se necessario e aggiungilo
                int idx = dbp->user_index;
                if (idx >= 0 && idx < 256) {
                    DB_Alloca(idx + 1);
                    // Se questo indice e' oltre il nostro count corrente, espandi
                    if (idx >= dbUtenti.num_utenti) {
                        // Nuovo utente - usa memset SOLO per la prima allocazione
                        // e imposta tutti i campi da zero, incluso storico_partite[] (vuoto).
                        memset(&dbUtenti.lista[idx], 0, sizeof(Statistiche));
                        dbUtenti.num_utenti = idx + 1;
                    } else {
                        // Utente ESISTENTE (gia' nel DB locale) - NON usare
                        // memset perche' azzererebbe storico_partite[] che contiene dati
                        // delle partite gia' giocate. Copia SOLO i campi trasmessi via
                        // AdminDBSyncPacket, preservando lo storico_partite[] esistente.
                        // Questo garantisce che le statistiche non vengano MAI perse
                        // durante la sincronizzazione TCP con l'host.
                        Statistiche* s = &dbUtenti.lista[idx];
                        // NON sincronizzare contatori via PACKET_ADMIN_DB_SYNC.
                        // I contatori (partite_giocate, vittorie) sono READ-ONLY per l'admin
                        // e vengono gestiti SOLO da PACKET_STATS_SYNC dopo ogni partita.
                        // Sovrascriverli qui con il "max" causava report di statistiche
                        // errate (es. 9 invece di 2) quando l'host aveva contatori sbagliati.
                        // Aggiorna SOLO i campi anagrafici, preserva sempre i contatori locali.
                        strncpy(s->email, dbp->user.email, sizeof(s->email) - 1);
                        strncpy(s->nome, dbp->user.nome, sizeof(s->nome) - 1);
                        strncpy(s->cognome, dbp->user.cognome, sizeof(s->cognome) - 1);
                        strncpy(s->username, dbp->user.username, sizeof(s->username) - 1);
                        strncpy(s->password, dbp->user.password, sizeof(s->password) - 1);
                        strncpy(s->pin_recupero, dbp->user.pin_recupero, sizeof(s->pin_recupero) - 1);
                        // storico_partite[] NON viene toccato - preserva i dati
                        // esistenti delle partite giocate. I nuovi dati arrivano tramite
                        // PACKET_STATS_SYNC (ora con storico_partite) o LAN sync.
                        break; // Salta la copia sotto per evitare di sovrascrivere
                    }
                    
                    // Solo per NUOVI utenti (idx appena creati): copia tutti i campi
                    if (idx >= dbUtenti.num_utenti - 1 && idx < 256) {
                        Statistiche* s = &dbUtenti.lista[idx];
                        strncpy(s->email, dbp->user.email, sizeof(s->email) - 1);
                        strncpy(s->nome, dbp->user.nome, sizeof(s->nome) - 1);
                        strncpy(s->cognome, dbp->user.cognome, sizeof(s->cognome) - 1);
                        strncpy(s->username, dbp->user.username, sizeof(s->username) - 1);
                        strncpy(s->password, dbp->user.password, sizeof(s->password) - 1);
                        strncpy(s->pin_recupero, dbp->user.pin_recupero, sizeof(s->pin_recupero) - 1);
                        s->partite_giocate = dbp->user.partite_giocate;
                        s->vittorie_giocatore = dbp->user.vittorie_giocatore;
                        s->vittorie_bot = dbp->user.vittorie_bot;
                        // storico_partite[] rimane vuoto (memset a 0 fatto sopra)
                    }
                    
                    /* Dopo aver aggiornato il DB via TCP, se siamo NET_HOST
                     * invia anche via LAN UDP broadcast per aggiornare i peer LAN
                     * che non sono connessi via TCP multiplayer. */
                    if (currentRole == NET_HOST) {
                        LAN_BroadcastNow();
                    }
                }
            }
            break;
        }

        default: break;
    }
}