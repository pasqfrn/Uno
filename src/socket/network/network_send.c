/**
 * @file network_send.c
 * @brief Funzioni di invio pacchetti per il multiplayer online.
 * @ingroup network
 *
 * Contiene le funzioni per l'invio di pacchetti TCP/UDP:
 *   - SendGameState: invia stato completo all'host (client)
 *   - BroadcastGameState: invia stato a tutti i client (host)
 *   - SendMove/SendDraw: invia mossa/pescata all'host (client)
 *   - SendChat/SendColorChoice: invia chat/scelta colore
 *   - SendCreateSession/SendJoinSession: gestione sessioni
 *   - SendAdminUserSync/BroadcastAdminDB: sincronizzazione admin
 *   - SendPlayAgain/BroadcastHostReturnLobby: gestione "Gioca Ancora"
 *
 * Architettura:
 *   - Host usa Broadcast*() per inviare a tutti i client
 *   - Client usa Send*() per inviare all'host
 */
#include "../../../lib/socket/network/socket_compat.h"

#include <stdio.h>
#include <string.h>

#include "raylib.h"

#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/game/game_logic.h"
#include "../../../lib/auth/auth.h"

extern SOCKET remoteSocket;
extern SOCKET clientSockets[4];

void SendStartGame(void) {
    NetPacket packet = {0};
    packet.type = PACKET_START_GAME;
    
    if (currentRole == NET_HOST) {
        for (int i = 1; i < 4; i++) {
            if (clientSockets[i] != INVALID_SOCKET) {
                send(clientSockets[i], (char*)&packet, sizeof(NetPacket), 0);
            }
        }
    } else if (currentRole == NET_CLIENT && remoteSocket != INVALID_SOCKET) {
        send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
    }
}

void SendGameState(StatoGioco* gioco) {
    if (!isConnected || remoteSocket == INVALID_SOCKET) return;
    NetPacket packet = {0};
    packet.type = PACKET_GAME_STATE;
    packet.data.game_state.stato_gioco = *gioco;
    send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
}

void SendChat(const char* message) {
    if (!isConnected || remoteSocket == INVALID_SOCKET) return;
    NetPacket packet = {0};
    packet.type = PACKET_CHAT;
    packet.data.chat.player_id = local_player_id;
    strncpy(packet.data.chat.message, message, sizeof(packet.data.chat.message) - 1);
    send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
}

void SendMove(int card_index, Colore colore) {
    if (!isConnected || remoteSocket == INVALID_SOCKET) return;
    NetPacket packet = {0};
    packet.type = PACKET_MOVE;
    packet.data.move.player_id = local_player_id;
    packet.data.move.card_index = card_index;
    packet.data.move.colore_scelto = colore;
    send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
}

void SendDraw(int count) {
    if (!isConnected || remoteSocket == INVALID_SOCKET) return;
    NetPacket packet = {0};
    packet.type = PACKET_DRAW;
    packet.data.draw.player_id = local_player_id;
    packet.data.draw.count = count;
    send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
}

void SendColorChoice(Colore colore) {
    if (!isConnected || remoteSocket == INVALID_SOCKET) return;
    NetPacket packet = {0};
    packet.type = PACKET_COLOR_CHOICE;
    packet.data.color_choice.player_id = local_player_id;
    packet.data.color_choice.colore = colore;
    send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
}

void SendJoinGame(const char* nome) {
    if (!isConnected || remoteSocket == INVALID_SOCKET) return;
    NetPacket packet = {0};
    packet.type = PACKET_JOIN_GAME;
    strncpy(packet.data.join_game.nome, nome, sizeof(packet.data.join_game.nome) - 1);
    send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
}

void SendPlayerHand(int target_socket, int data_player_id, StatoGioco* gioco) {
    if (!gioco || data_player_id < 0 || data_player_id >= gioco->num_giocatori) return;
    NetPacket packet = {0};
    packet.type = PACKET_PLAYER_HAND;
    packet.data.player_hand.player_id = data_player_id;
    /* Copia la mano tramite l'API pubblica dell'ADT ListaCarte (information hiding) */
    int count = GameLogic_CopiaMano(&gioco->giocatori[data_player_id],
                                    packet.data.player_hand.carte, 108);
    packet.data.player_hand.num_carte = count;

    if (currentRole == NET_CLIENT && remoteSocket != INVALID_SOCKET) {
        send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
    } else if (currentRole == NET_HOST && clientSockets[target_socket] != INVALID_SOCKET) {
        send(clientSockets[target_socket], (char*)&packet, sizeof(NetPacket), 0);
    }
}

void BroadcastGameState(StatoGioco* gioco) {
    if (currentRole != NET_HOST) return;
    NetPacket packet = {0};
    packet.type = PACKET_GAME_STATE;
    packet.data.game_state.stato_gioco = *gioco;
    
    for (int i = 1; i < 4; i++) {
        if (clientSockets[i] != INVALID_SOCKET) {
            send(clientSockets[i], (char*)&packet, sizeof(NetPacket), 0);
        }
    }
}

void BroadcastPlayerHands(StatoGioco* gioco) {
    if (currentRole != NET_HOST) return;
    for (int target = 1; target < 4; target++) {
        if (clientSockets[target] != INVALID_SOCKET) {
            for (int p = 0; p < gioco->num_giocatori; p++) {
                SendPlayerHand(target, p, gioco);
            }
        }
    }
}

void BroadcastChat(const char* message) {
    if (currentRole != NET_HOST) return;
    NetPacket packet = {0};
    packet.type = PACKET_CHAT;
    strncpy(packet.data.chat.message, message, sizeof(packet.data.chat.message) - 1);
    for (int i = 1; i < 4; i++) {
        if (clientSockets[i] != INVALID_SOCKET) send(clientSockets[i], (char*)&packet, sizeof(NetPacket), 0);
    }
}

void SendCreateSession(const char* host_name) {
    if (!isConnected || remoteSocket == INVALID_SOCKET) return;
    NetPacket packet = {0};
    packet.type = PACKET_CREATE_SESSION;
    packet.sender_id = player_id;
    strncpy(packet.data.create_session.host_name, host_name, sizeof(packet.data.create_session.host_name) - 1);
    send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
}

void SendJoinSession(const char* sess_id, const char* player_name) {
    if (!isConnected || remoteSocket == INVALID_SOCKET) return;
    NetPacket packet = {0};
    packet.type = PACKET_JOIN_SESSION;
    packet.sender_id = player_id;
    strncpy(packet.data.join_session.session_id, sess_id, sizeof(packet.data.join_session.session_id) - 1);
    strncpy(packet.data.join_session.player_name, player_name, sizeof(packet.data.join_session.player_name) - 1);
    send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
}

void SendPlayerReady(int ready) {
    if (!isConnected || remoteSocket == INVALID_SOCKET) return;
    NetPacket packet = {0};
    packet.type = PACKET_PLAYER_READY;
    packet.sender_id = player_id;
    packet.data.player_ready.player_id = local_player_id;
    packet.data.player_ready.is_ready = ready;
    send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
}

void SendHeartbeat(void) {
    if (!isConnected || remoteSocket == INVALID_SOCKET) return;
    NetPacket packet = {0};
    packet.type = PACKET_HEARTBEAT;
    packet.sender_id = player_id;
    packet.data.heartbeat.player_id = local_player_id;
    packet.data.heartbeat.timestamp = GetTime();
    send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
}

/**
 * @brief Invia un pacchetto PACKET_PLAY_AGAIN all'host.
 * Il client segnala all'host che vuole giocare ancora (tornare in pre-lobby).
 * Invia anche il nome del giocatore nel campo player_name[] per permettere
 * all'host di ricreare lo slot correttamente dopo aver pulito la lobby.
 */
void SendPlayAgain(void) {
    if (!isConnected || remoteSocket == INVALID_SOCKET) return;
    NetPacket packet = {0};
    packet.type = PACKET_PLAY_AGAIN;
    packet.data.play_again.player_id = local_player_id;
    packet.data.play_again.play_again = 1;
    // Invia il nome del giocatore perche' l'host potrebbe averlo perso
    if (id_utente_corrente >= 0 && id_utente_corrente < dbUtenti.num_utenti) {
        strncpy(packet.data.play_again.player_name, 
                dbUtenti.lista[id_utente_corrente].username, 
                sizeof(packet.data.play_again.player_name) - 1);
    }
    send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
}

/**
 * @brief Broadcasta PACKET_HOST_RETURN_LOBBY a tutti i client.
 * L'host segnala che e' tornato in pre-lobby dopo aver premuto "Gioca Ancora".
 * I client possono cosi' decidere se unirsi o andarsene.
 */
void BroadcastHostReturnLobby(void) {
    if (currentRole != NET_HOST) return;
    NetPacket packet = {0};
    packet.type = PACKET_HOST_RETURN_LOBBY;
    packet.data.host_return_lobby.host_ready = 1;
    for (int i = 1; i < 4; i++) {
        if (clientSockets[i] != INVALID_SOCKET)
            send(clientSockets[i], (char*)&packet, sizeof(NetPacket), 0);
    }
}

// ============================================================
//  ADMIN SYNC - Funzioni di invio per la sincronizzazione admin
// ============================================================

/**
 * SendAdminUserSync - Invia i dati utente del client all'host.
 * Viene chiamato quando un client si collega alla stanza, per
 * far conoscere i propri dati anagrafici all'host (admin).
 */
void SendAdminUserSync(const AdminUserSyncData* userData) {
    if (!isConnected || remoteSocket == INVALID_SOCKET || !userData) return;
    NetPacket packet = {0};
    packet.type = PACKET_ADMIN_USER_SYNC;
    packet.sender_id = player_id;
    memcpy(&packet.data.admin_action.user, userData, sizeof(AdminUserSyncData));
    packet.data.admin_action.action = 0; // sync iniziale
    send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
}

void SendAdminUserAction(const AdminUserAction* action) {
    if (!isConnected || remoteSocket == INVALID_SOCKET || !action) return;
    NetPacket packet = {0};
    packet.type = PACKET_ADMIN_USER_SYNC;
    packet.sender_id = player_id;
    memcpy(&packet.data.admin_action, action, sizeof(AdminUserAction));
    send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
}

/**
 * BroadcastAdminUserAction - Invia un'azione admin (modifica/elimina/crea)
 * a tutti i client connessi. Usato quando l'admin modifica un utente
 * nel pannello di controllo.
 */
void BroadcastAdminUserAction(const AdminUserAction* action) {
    if (currentRole != NET_HOST || !action) return;
    NetPacket packet = {0};
    packet.type = PACKET_ADMIN_USER_SYNC;
    packet.sender_id = 0; // L'host e' sempre l'admin
    memcpy(&packet.data.admin_action, action, sizeof(AdminUserAction));
    for (int i = 1; i < 4; i++) {
        if (clientSockets[i] != INVALID_SOCKET) {
            send(clientSockets[i], (char*)&packet, sizeof(NetPacket), 0);
        }
    }
}

/**
 * @brief Invia un pacchetto di notifica disconnessione host a tutti i client.
 * Usato quando l'host chiude la stanza (lobby o fine partita).
 * I client ricevono l'avviso e vengono reindirizzati al menu principale.
 */
void BroadcastHostDisconnect(void) {
    if (currentRole != NET_HOST) return;
    NetPacket packet = {0};
    packet.type = PACKET_HOST_DISCONNECT;
    packet.sender_id = 0; // L'host e' sempre 0
    for (int i = 1; i < 4; i++) {
        if (clientSockets[i] != INVALID_SOCKET) {
            send(clientSockets[i], (char*)&packet, sizeof(NetPacket), 0);
        }
    }
}

/**
 * @brief Invia un pacchetto di sincronizzazione statistiche COMPLETA a tutti i client.
 * Usato dopo ogni partita per aggiornare le statistiche di tutti i giocatori.
 *
 * Garantisce che i client connessi via TCP vedano TUTTI i dettagli delle
 * partite storiche, non solo i contatori numerici. Popola l'intero array
 * storico_partite[] nel pacchetto StatsSync.
 *
 * @param stats Puntatore al pacchetto StatsSync (gia' compilato con username e contatori).
 */
void BroadcastStatsSync(const StatsSyncPacket* stats) {
    if (currentRole != NET_HOST || !stats) return;
    NetPacket packet = {0};
    packet.type = PACKET_STATS_SYNC;
    packet.sender_id = 0;
    
    // Popola i campi del pacchetto StatsSync con TUTTO lo storico
    StatsSyncPacket* sp = &packet.data.stats_sync;
    strncpy(sp->username, stats->username, sizeof(sp->username) - 1);
    sp->partite_giocate = stats->partite_giocate;
    sp->vittorie_giocatore = stats->vittorie_giocatore;
    sp->vittorie_bot = stats->vittorie_bot;
    sp->durata_ultima_partita = stats->durata_ultima_partita;
    strncpy(sp->modalita_ultima, stats->modalita_ultima, sizeof(sp->modalita_ultima) - 1);
    strncpy(sp->esito_ultima, stats->esito_ultima, sizeof(sp->esito_ultima) - 1);
    sp->posizione_ultima = stats->posizione_ultima;
    
    // Popola lo storico partite (ultime STORICO_SYNC_COUNT partite)
    // nel pacchetto StatsSync. Cerca l'utente nel DB locale per username e
    // copia le ultime STORICO_SYNC_COUNT partite dallo storico_partite[].
    sp->storico_count = 0;
    for (int u = 0; u < dbUtenti.num_utenti; u++) {
        if (strcmp(dbUtenti.lista[u].username, stats->username) == 0) {
            int start = 0;
            if (dbUtenti.lista[u].partite_giocate > STORICO_SYNC_COUNT) {
                start = dbUtenti.lista[u].partite_giocate - STORICO_SYNC_COUNT;
            }
            for (int h = start; h < dbUtenti.lista[u].partite_giocate && sp->storico_count < STORICO_SYNC_COUNT; h++) {
                sp->storico_partite[sp->storico_count] = dbUtenti.lista[u].storico_partite[h];
                sp->storico_count++;
            }
            break;
        }
    }
    
    for (int i = 1; i < 4; i++) {
        if (clientSockets[i] != INVALID_SOCKET) {
            send(clientSockets[i], (char*)&packet, sizeof(NetPacket), 0);
        }
    }
}

/**
 * @brief Invia un pacchetto di sincronizzazione statistiche COMPLETA all'host (client).
 * Usato quando il client salva le proprie statistiche dopo una partita.
 *
 * Garantisce che l'host (e tutti gli altri client tramite broadcast) ricevano
 * TUTTI i dettagli delle partite, non solo i contatori numerici.
 *
 * Popola l'intero array storico_partite[] nel pacchetto StatsSync.
 */
void SendStatsSync(const StatsSyncPacket* stats) {
    if (!isConnected || remoteSocket == INVALID_SOCKET || !stats) return;
    NetPacket packet = {0};
    packet.type = PACKET_STATS_SYNC;
    packet.sender_id = player_id;
    
    // Popola i campi del pacchetto con TUTTO lo storico
    StatsSyncPacket* sp = &packet.data.stats_sync;
    strncpy(sp->username, stats->username, sizeof(sp->username) - 1);
    sp->partite_giocate = stats->partite_giocate;
    sp->vittorie_giocatore = stats->vittorie_giocatore;
    sp->vittorie_bot = stats->vittorie_bot;
    sp->durata_ultima_partita = stats->durata_ultima_partita;
    strncpy(sp->modalita_ultima, stats->modalita_ultima, sizeof(sp->modalita_ultima) - 1);
    strncpy(sp->esito_ultima, stats->esito_ultima, sizeof(sp->esito_ultima) - 1);
    sp->posizione_ultima = stats->posizione_ultima;
    
    // Popola lo storico partite (ultime STORICO_SYNC_COUNT partite)
    // dal DB locale per la sincronizzazione TCP.
    sp->storico_count = 0;
    for (int u = 0; u < dbUtenti.num_utenti; u++) {
        if (strcmp(dbUtenti.lista[u].username, stats->username) == 0) {
            int start = 0;
            if (dbUtenti.lista[u].partite_giocate > STORICO_SYNC_COUNT) {
                start = dbUtenti.lista[u].partite_giocate - STORICO_SYNC_COUNT;
            }
            for (int h = start; h < dbUtenti.lista[u].partite_giocate && sp->storico_count < STORICO_SYNC_COUNT; h++) {
                sp->storico_partite[sp->storico_count] = dbUtenti.lista[u].storico_partite[h];
                sp->storico_count++;
            }
            break;
        }
    }
    
    send(remoteSocket, (char*)&packet, sizeof(NetPacket), 0);
}

void BroadcastAdminDB(void) {
    if (currentRole != NET_HOST) return;
    
    int total = dbUtenti.num_utenti;
    if (total > 256) total = 256; // Limite ragionevole
    
    // Invia ogni utente come pacchetto separato (indice progressivo)
    for (int idx = 0; idx < total; idx++) {
        NetPacket packet = {0};
        packet.type = PACKET_ADMIN_DB_SYNC;
        packet.sender_id = 0;
        
        // Copia i dati dell'utente SENZA storico partite (troppo grande)
        Statistiche* s = &dbUtenti.lista[idx];
        AdminDBSyncPacket* dbp = &packet.data.admin_db_packet;
        dbp->user_index = idx;
        dbp->total_users = total;
        strncpy(dbp->user.email, s->email, sizeof(dbp->user.email) - 1);
        strncpy(dbp->user.nome, s->nome, sizeof(dbp->user.nome) - 1);
        strncpy(dbp->user.cognome, s->cognome, sizeof(dbp->user.cognome) - 1);
        strncpy(dbp->user.username, s->username, sizeof(dbp->user.username) - 1);
        strncpy(dbp->user.password, s->password, sizeof(dbp->user.password) - 1);
        strncpy(dbp->user.pin_recupero, s->pin_recupero, sizeof(dbp->user.pin_recupero) - 1);
        dbp->user.partite_giocate = s->partite_giocate;
        dbp->user.vittorie_giocatore = s->vittorie_giocatore;
        dbp->user.vittorie_bot = s->vittorie_bot;
        
        for (int i = 1; i < 4; i++) {
            if (clientSockets[i] != INVALID_SOCKET) {
                send(clientSockets[i], (char*)&packet, sizeof(NetPacket), 0);
            }
        }
    }
    
    // Invia un pacchetto speciale "sync done" per segnalare che la sincro e' completa
    // (si usa PACKET_ADMIN_DB_SYNC con user_index = -1)
    NetPacket donePacket = {0};
    donePacket.type = PACKET_ADMIN_DB_SYNC;
    donePacket.sender_id = 0;
    donePacket.data.admin_db_packet.user_index = -1;
    donePacket.data.admin_db_packet.total_users = total;
    for (int i = 1; i < 4; i++) {
        if (clientSockets[i] != INVALID_SOCKET) {
            send(clientSockets[i], (char*)&donePacket, sizeof(NetPacket), 0);
        }
    }
}