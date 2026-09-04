/**
 * @file packet_handler_game.c
 * @brief Gestore pacchetti di gioco per multiplayer online.
 * @ingroup network
 *
 * @details Questo modulo gestisce TUTTI i pacchetti relativi alla logica di gioco
 *          scambiati tra host e client durante una partita multiplayer online.
 *          Implementa l'architettura client-server dove:
 *          - HOST: Riceve comandi dai client, valida le mosse, applica effetti
 *            e sincronizza lo stato completo tramite broadcast
 *          - CLIENT: Riceve lo stato sincronizzato dall'host e invia comandi
 *
 * @note Include gestione riconnessione diretta in partita:
 *       i client disconnessi possono rientrare senza passare per la lobby.
 *
 * Pacchetti gestiti:
 *   - PACKET_JOIN_GAME: nuovo giocatore o riconnessione client
 *   - PACKET_GAME_STATE: sincronizzazione stato completo (host -> client)
 *   - PACKET_PLAYER_HAND: sincronizzazione mano di un giocatore
 *   - PACKET_CHAT: messaggi di chat con supporto tag @utente
 *   - PACKET_DISCONNECT/HOST_DISCONNECT: gestione uscite e sostituzione bot
 *   - PACKET_COLOR_CHOICE: scelta colore dopo carta Jolly
 *   - PACKET_MOVE: mossa carta (solo host valida e applica)
 *   - PACKET_DRAW: pescata carte (solo host applica e cambia turno)
 *   - PACKET_JOIN_REJECTED: rifiuto connessione (client)
 *   - PACKET_START_GAME: segnalazione inizio partita (client)
 *
 * File correlati:
 *   - packet_handler.c  : gestore principale (smista a questo modulo)
 *   - state_sync.c      : SyncGameStateFromNetwork (merge stato remoto)
 *   - network_send.c    : funzioni di invio (BroadcastGameState, SendMove, ecc.)
 *   - game_logic.h      : funzioni di logica (ApplicaEffetto, Pesca, MossaValida)
 *   - network_utils.c   : GestisciDisconnessione, RiassegnaSocket
 *
 * @author Pasquale Franco
 * @version 9.1.3
 * @date 2026-07-01
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "raylib.h"
#include "../../../lib/socket/network/packet_handler_game.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/socket/network/state_sync.h"
#include "../../../lib/socket/server/server_manager.h"
#include "../../../lib/game/game_logic.h"
#include "../../../lib/screens/ui.h"
#include "../../../lib/data_structures/list.h"
#include "../../../lib/auth/auth.h"
#include "../../../lib/screens/game/gameplay_screen.h"
#include "../../../lib/screens/game/end_screen.h"
#include "../../../lib/screens/menu/string_utils.h"
#include "../../../lib/socket/lan/lan_sync.h"

/**
 * @brief Salva le statistiche dell'host quando la partita e' finita
 *        e il vincitore e' un client remoto (non l'host).
 *
 * Calcola la posizione dell'host nella classifica finale e aggiorna
 * il suo storico partite con esito "Sconfitta" o "Vittoria" (se
 * l'host ha comunque vinto nonostante il client remoto).
 *
 * @param gioco Puntatore allo stato di gioco.
 * @param vincitore_id ID del giocatore vincitore (client remoto).
 */
static void SalvaStatisticheHostVincitaClient(StatoGioco* gioco, int vincitore_id) {
    (void)vincitore_id;  /* parametro informativo/documentale: la vittoria e' gia' in gioco */
    if (!gioco || gioco->statistiche_gia_salvate) return;
    gioco->statistiche_gia_salvate = 1;
    Statistiche* utente = &dbUtenti.lista[id_utente_corrente];
    if (!utente || utente->partite_giocate >= MAX_PARTITE_STORICO) return;
    PartitaRegistrata* p_rec = &utente->storico_partite[utente->partite_giocate];
    strcpy(p_rec->modalita, "Online");
    ClassificaRecord cr[4]; int nc = 0;
    CalcolaClassifica(gioco, cr, &nc);
    int mio_id = 0;
    int mia_pos = nc, ho_vinto = 0;
    for (int k = 0; k < nc; k++) {
        if (Giocatore_Nome(&gioco->giocatori[mio_id])[0] &&
            strcmp(cr[k].nome, Giocatore_Nome(&gioco->giocatori[mio_id])) == 0) {
            mia_pos = cr[k].posizione;
            if (mia_pos == 1) ho_vinto = 1;
            break;
        }
    }
    strcpy(p_rec->esito, ho_vinto ? "Vittoria" : "Sconfitta");
    p_rec->durata_secondi = (int)gioco->durata_partita;
    p_rec->posizione = mia_pos;
    utente->partite_giocate++;
    if (ho_vinto) utente->vittorie_giocatore++;
    else utente->vittorie_bot++;
    SalvaDB();
}

/**
 * @brief Gestisce un pacchetto di gioco (smistamento principale).
 *
 * Switch sul tipo di pacchetto e delega alle funzioni specifiche:
 *   - PACKET_START_GAME: inizio partita (client)
 *   - PACKET_JOIN_GAME: nuovo giocatore (host)
 *   - PACKET_GAME_STATE: sync stato completo (client)
 *   - PACKET_PLAYER_HAND: sync mano giocatore (client)
 *   - PACKET_CHAT: messaggio chat
 *   - PACKET_DISCONNECT/HOST_DISCONNECT: gestione uscite
 *   - PACKET_COLOR_CHOICE: scelta colore dopo jolly
 *   - PACKET_MOVE: mossa carta (host valida e applica)
 *   - PACKET_DRAW: pescata carte (host applica)
 *   - PACKET_JOIN_REJECTED: rifiuto connessione (client)
 *
 * @param packet Pacchetto ricevuto (gia' deserializzato).
 * @param gioco Puntatore allo stato di gioco locale.
 */
void HandleGamePacket(NetPacket* packet, StatoGioco* gioco) {
    if (!packet || !gioco) return;

    switch (packet->type) {
        case PACKET_START_GAME:
            if (currentRole == NET_CLIENT) {
                network_game_initialized = 1;
                host_in_prelobby = 0;  // BUG FIX 3: Reset quando la partita inizia
                // TG-07: Riconnessione. PACKET_START_GAME solo segnala che l'host
                // ha accettato la connessione. Il reindirizzamento a FASE_GIOCO
                // avviene quando arrivano PACKET_GAME_STATE + PACKET_PLAYER_HAND
                // per garantire che il client abbia il mazzo sincronizzato.
            }
            break;
        /**
         * @brief PACKET_JOIN_GAME: Un client richiede di entrare nella stanza.
         *
         * Riconnessione diretta in partita:
         * Quando un client si disconnette durante una partita, il server lo
         * sostituisce con un Bot. Se il client si riconnette:
         * 1. Il server riconosce l'ID del client tramite il nome del giocatore
         *    (campo join_game.nome) o tramite il socket_id (sender_id).
         * 2. Trova lo slot del giocatore nell'array giocatori[].
         * 3. Rimuove il Bot che lo stava sostituendo (is_bot = 0).
         * 4. Riassegna i comandi al client umano.
         * 5. Il client NON passa dalla lobby: entra direttamente in partita
         *    senza attendere il suo turno. Lo stato di gioco e la mano vengono
         *    broadcastati immediatamente.
         *
         * Se la partita NON e' iniziata (prelobby), il client si unisce
         * normalmente alla lobby.
         *
         * Se la partita e' gia' iniziata e il client non e' riconosciuto
         * (nuovo tentativo di connessione), viene RIFIUTATO.
         */
        case PACKET_JOIN_GAME:
            if (currentRole == NET_HOST) {
                /* ============================================================
                 * BUG 4 FIX: Verifica se l'utente e' stato eliminato dall'admin.
                 * Se l'admin ha eliminato un utente (marcato in deletedUsers[]),
                 * il tentativo di riconnessione deve essere RIFIUTATO.
                 * L'utente eliminato non puo' piu' rientrare in partita.
                 * ============================================================ */
                {
                    int is_deleted = 0;
                    for (int d = 0; d < numDeletedUsers; d++) {
                        if (strcmp(deletedUsers[d], packet->data.join_game.nome) == 0) {
                            is_deleted = 1;
                            break;
                        }
                    }
                    if (is_deleted) {
                        MostraNotifica(gioco, TextFormat("%s rifiutato: utente eliminato!", packet->data.join_game.nome));
                        NetPacket rejectPkt = {0};
                        rejectPkt.type = PACKET_JOIN_REJECTED;
                        rejectPkt.sender_id = packet->sender_id;
                        strncpy(rejectPkt.data.join_rejected.motivo, "Utente eliminato dall'admin", sizeof(rejectPkt.data.join_rejected.motivo) - 1);
                        rejectPkt.data.join_rejected.motivo[sizeof(rejectPkt.data.join_rejected.motivo) - 1] = '\0';
                        if (packet->sender_id >= 1 && packet->sender_id < 4 && clientSockets[packet->sender_id] != INVALID_SOCKET) {
                            send(clientSockets[packet->sender_id], (char*)&rejectPkt, sizeof(NetPacket), 0);
                        }
                        break;
                    }
                }
                
                // Determina se la partita e' gia' iniziata.
                // La partita e' iniziata SOLO se ci sono carte negli scarti
                // (carta iniziale pescata da StartSession o carta giocata).
                int partita_iniziata = (gioco->num_carte_scarti > 0);
                
                int slot = -1;
                int is_reconnect = 0;
                
                // Cerca il giocatore per nome nell'array giocatori[].
                // Se il nome esiste gia', e' una RICONNESSIONE.
                for (int j = 1; j < gioco->num_giocatori; j++) {
                    if (strcmp(gioco->giocatori[j].nome, packet->data.join_game.nome) == 0) {
                        slot = j;
                        is_reconnect = 1;
                        break;
                    }
                }
                
                // Se non trovato per nome, prova per socket_id
                // (riconnessione dopo che l'host ha resettato i nomi).
                if (!is_reconnect) {
                    for (int j = 1; j < gioco->num_giocatori; j++) {
                        if (gioco->giocatori[j].socket_id == packet->sender_id) {
                            slot = j;
                            is_reconnect = 1;
                            break;
                        }
                    }
                }
                
                // IMPOSTA socket_id per permettere il riconoscimento alla prossima disconnessione
                if (slot >= 0 && slot < 4 && is_reconnect) {
                    gioco->giocatori[slot].socket_id = packet->sender_id;
                }
                
                if (is_reconnect) {
                    /* ============================================================
                     * Riconnessione diretta in partita.
                     * Il server riconosce l'ID del client, rimuove il Bot che
                     * lo stava sostituendo e riassegna i comandi al client umano.
                     * Il client NON passa dalla lobby: entra direttamente in partita.
                     * ============================================================ */
                    
                    // Verifica che il giocatore avesse era_in_prelobby = 1
                    // (era presente prima dell'inizio partita). Se no, rifiuta.
                    if (slot >= 0 && slot < gioco->num_giocatori && !gioco->giocatori[slot].era_in_prelobby) {
                        MostraNotifica(gioco, TextFormat("%s rifiutato: non autorizzato!", packet->data.join_game.nome));
                        break;
                    }
                    
                    if (slot >= 0 && slot < gioco->num_giocatori) {
                        /* Rimuovi il Bot che stava sostituendo il client */
                        gioco->giocatori[slot].is_bot = 0;
                        gioco->giocatori[slot].socket_id = packet->sender_id;
                        MostraNotifica(gioco, TextFormat("%s e' rientrato in partita!", Giocatore_Nome(&gioco->giocatori[slot])));
                        
                        if (partita_iniziata) {
                            /* Riassegna il socket e broadcasta lo stato.
                             * Il client riceve PACKET_START_GAME + PACKET_GAME_STATE +
                             * PACKET_PLAYER_HAND direttamente, saltando la lobby. */
                            RiassegnaSocket(packet->sender_id, slot, 1, 1);
                            BroadcastGameState(gioco);
                            BroadcastPlayerHands(gioco);
                        }
                    }
                } else if (!partita_iniziata) {
                    // PRELOBBY (partita NON iniziata): client NUOVO (non in riconnessione).
                    // Puo' entrare solo se c'e' un bot originale da sostituire o uno slot vuoto.
                    int bot_slot = -1;
                    for (int j = 1; j < gioco->num_giocatori; j++) {
                        if (gioco->giocatori[j].is_bot) {
                            const char* nome_bot = Giocatore_Nome(&gioco->giocatori[j]);
                            // Solo bot ORIGINALI (Bot_1, Bot_2, ...) sono sostituibili.
                            // I bot con nome di giocatore umano (disconnesso) NON sono sostituibili.
                            if (strncmp(nome_bot, "Bot_", 4) == 0 || nome_bot[0] == '\0') {
                                bot_slot = j;
                                break;
                            }
                        }
                    }
                    
                    if (bot_slot != -1) {
                        slot = bot_slot;
                        gioco->giocatori[slot].socket_id = packet->sender_id;
                        strncpy(gioco->giocatori[slot].nome, packet->data.join_game.nome, sizeof(gioco->giocatori[slot].nome) - 1);
                        gioco->giocatori[slot].nome[sizeof(gioco->giocatori[slot].nome) - 1] = '\0';
                        gioco->giocatori[slot].is_bot = 0;
                        gioco->giocatori[slot].era_in_prelobby = 1; // Ora e' in prelobby
                        if (!gioco->giocatori[slot].mano) gioco->giocatori[slot].mano = CreaLista();
                        
                        GameSession* sessionNet = GetSessionByPlayerId(packet->sender_id);
                        if (sessionNet) {
                            strcpy(sessionNet->players[slot].nome, packet->data.join_game.nome);
                            sessionNet->players[slot].is_connected = 1;
                            strcpy(sessionNet->game_state.giocatori[slot].nome, packet->data.join_game.nome);
                            sessionNet->game_state.giocatori[slot].is_bot = 0;
                        }
                        // Notifica rimossa: l'ingresso è già visibile nella lobby
                    } else {
                        // Nessun bot sostituibile: cerca uno slot vuoto
                        for (int j = 1; j < 4; j++) {
                            if (gioco->giocatori[j].nome[0] == '\0') {
                                slot = j;
                                break;
                            }
                        }
                        if (slot == -1) {
                            slot = gioco->num_giocatori;
                        }
                        if (slot >= 0 && slot < 4) {
                            if (slot >= gioco->num_giocatori) {
                                gioco->num_giocatori = slot + 1;
                            }
                            gioco->giocatori[slot].socket_id = packet->sender_id;
                            strncpy(gioco->giocatori[slot].nome, packet->data.join_game.nome, sizeof(gioco->giocatori[slot].nome) - 1);
                            gioco->giocatori[slot].nome[sizeof(gioco->giocatori[slot].nome) - 1] = '\0';
                            gioco->giocatori[slot].is_bot = 0;
                            gioco->giocatori[slot].era_in_prelobby = 1; // Ora e' in prelobby
                            if (!gioco->giocatori[slot].mano) gioco->giocatori[slot].mano = CreaLista();
                            // Notifica rimossa: l'ingresso è già visibile nella lobby
                        } else {
                            break; // Stanza piena
                        }
                    }
                    RiassegnaSocket(packet->sender_id, slot, 0, 0);
                } else {
                    // PARTITA GIA' INIZIATA e client NON riconosciuto:
                    // RIFIUTA la connessione! (TG-06)
                    MostraNotifica(gioco, TextFormat("%s rifiutato: partita iniziata!", packet->data.join_game.nome));
                    // Invia pacchetto di rifiuto SOLO al client che ha inviato la richiesta
                    NetPacket rejectPkt = {0};
                    rejectPkt.type = PACKET_JOIN_REJECTED;
                    rejectPkt.sender_id = packet->sender_id;
                    strncpy(rejectPkt.data.join_rejected.motivo, "Partita gia' iniziata", sizeof(rejectPkt.data.join_rejected.motivo) - 1);
                    rejectPkt.data.join_rejected.motivo[sizeof(rejectPkt.data.join_rejected.motivo) - 1] = '\0';
                    if (packet->sender_id >= 1 && packet->sender_id < 4 && clientSockets[packet->sender_id] != INVALID_SOCKET) {
                        send(clientSockets[packet->sender_id], (char*)&rejectPkt, sizeof(NetPacket), 0);
                    }
                    break;
                }
                
                BroadcastGameState(gioco);
                if (gioco->num_carte_scarti > 0) {
                    BroadcastPlayerHands(gioco);
                }
            }
            break;
        case PACKET_GAME_STATE:
            SyncGameStateFromNetwork(gioco, &packet->data.game_state.stato_gioco);
            network_pending = 0;
            network_game_initialized = 1;
            
            if (currentRole == NET_CLIENT) {
                for (int i = 0; i < gioco->num_giocatori; i++) {
                    if (strcmp(gioco->giocatori[i].nome, dbUtenti.lista[id_utente_corrente].username) == 0) {
                        local_player_id = i;
                        break;
                    }
                }
                // TG-07: Riconnessione. Se c'è già una partita in corso,
                // il client salterà la lobby quando riceverà anche la sua mano.
                if (gioco->num_carte_scarti > 0) {
                    // Se la mano del giocatore locale è già arrivata prima del
                    // game state, abilita comunque il passaggio diretto in partita.
                    if (local_player_id >= 0 && local_player_id < gioco->num_giocatori &&
                        GameLogic_GetLunghezzaMano(&gioco->giocatori[local_player_id]) > 0) {
                        network_start_game_flag = 1;
                    }
                }
            }
            break;
        case PACKET_JOIN_REJECTED:
            if (currentRole == NET_CLIENT) {
                join_rejected = 1;
                strncpy(reject_message, packet->data.join_rejected.motivo, sizeof(reject_message) - 1);
                reject_message[sizeof(reject_message) - 1] = '\0';
                CloseNetwork();
                currentRole = NET_OFFLINE;
            }
            break;
        case PACKET_PLAYER_HAND:
            if (currentRole == NET_CLIENT) {
                int pid = packet->data.player_hand.player_id;
                
                // Protezione: valida player_id per evitare out-of-bounds
                if (pid < 0 || pid >= gioco->num_giocatori) {
                    // Ignora pacchetto non valido
                    break;
                }
                
                /* ============================================================
                 * BUG 5 FIX: Riconnessione diretta in partita.
                 *
                 * Quando un client si riconnette durante una partita, l'host
                 * invia PACKET_START_GAME, PACKET_GAME_STATE e PACKET_PLAYER_HAND
                 * in rapida successione. A causa del buffering di rete, l'ordine
                 * di arrivo NON E' GARANTITO.
                 *
                 * Se PACKET_PLAYER_HAND arriva PRIMA di PACKET_GAME_STATE,
                 * local_player_id e' ancora -1 (non ancora impostato da
                 * SyncGameStateFromNetwork). In questo caso, il controllo
                 * "pid == local_player_id" fallisce e network_start_game_flag
                 * NON viene mai impostato.
                 *
                 * Il client rimane bloccato nella lobby (waiting room) perche'
                 * network_start_game_flag == 0 e la mano non viene riconosciuta
                 * come propria. Il giocatore vede la lobby ma non puo' interagire
                 * fino a quando non arriva il suo turno (e anche in quel caso
                 * non vede le carte).
                 *
                 * FIX: Se local_player_id e' -1, cerca il giocatore per username
                 * nell'array giocatori[] (come fa SyncGameStateFromNetwork).
                 * Se trovato, imposta local_player_id e network_start_game_flag.
                 * ============================================================ */
                if (pid == local_player_id) {
                    network_start_game_flag = 1;
                } else if (local_player_id == -1 && id_utente_corrente >= 0) {
                    /* BUG 5 FIX: local_player_id non ancora impostato.
                     * Cerca il giocatore per username nell'array giocatori[]. */
                    for (int i = 0; i < gioco->num_giocatori; i++) {
                        if (strcmp(gioco->giocatori[i].nome, dbUtenti.lista[id_utente_corrente].username) == 0) {
                            local_player_id = i;
                            if (pid == i) {
                                network_start_game_flag = 1;
                            }
                            break;
                        }
                    }
                }
                
                /* Ricostruisce la mano del giocatore usando l'API dell'ADT ListaCarte
                 * (nessun accesso alla rappresentazione interna della lista). */
                if (gioco->giocatori[pid].mano) {
                    EliminaLista(gioco->giocatori[pid].mano);
                    gioco->giocatori[pid].mano = NULL;
                }
                gioco->giocatori[pid].mano = CreaLista();
                for(int c=0; c < packet->data.player_hand.num_carte; c++) {
                    InsertInCoda(gioco->giocatori[pid].mano, packet->data.player_hand.carte[c]);
                }
                gioco->giocatori[pid].num_carte_mano = Lista_Lunghezza(gioco->giocatori[pid].mano);
            }
            break;
        case PACKET_CHAT:
            // Il comando "/GIOCA_ANCORA" e' stato rimosso.
            // Ora il client usa PACKET_PLAY_AGAIN gestito da packet_handler_admin.c
            // per segnalare all'host di voler giocare ancora.
            
            if (currentRole == NET_HOST && strncmp(packet->data.chat.message, "/UNO", 4) == 0) {
                if (gioco->deve_chiamare_uno == packet->data.chat.player_id + 1) {
                    gioco->deve_chiamare_uno = 0;
                    MostraNotifica(gioco, TextFormat("%s ha gridato UNO!", gioco->giocatori[packet->data.chat.player_id].nome));
                    BroadcastGameState(gioco);
                }
                break;
            }
            
            if (gioco->num_chat_msg < 15) {
                strcpy(gioco->chat_history[gioco->num_chat_msg], packet->data.chat.message);
                gioco->num_chat_msg++;
            } else {
                for(int i=1; i<15; i++) strcpy(gioco->chat_history[i-1], gioco->chat_history[i]);
                strcpy(gioco->chat_history[14], packet->data.chat.message);
            }
            
            {
                const char* msg_chat = packet->data.chat.message;
                const char* at_ptr = strchr(msg_chat, '@');
                if (at_ptr) {
                    char nomeTag[30] = "";
                    sscanf(at_ptr + 1, "%s", nomeTag);
                    int tagger_id = packet->data.chat.player_id;
                    for (int t = 0; t < gioco->num_giocatori; t++) {
                        if (CompareIgnoreCase(gioco->giocatori[t].nome, nomeTag)) {
                            // Non mostrare notifica se il taggato è un BOT (i bot non ricevono notifiche)
                            if (t != tagger_id && !gioco->giocatori[t].is_bot) {
                                MostraNotifica(gioco, TextFormat("Sei stato taggato da %s!", Giocatore_Nome(&gioco->giocatori[tagger_id])));
                            }
                            break;
                        }
                    }
                } else {
                    MostraNotifica(gioco, msg_chat);
                }
            }
            
            if (currentRole == NET_HOST) {
                BroadcastChat(packet->data.chat.message);
            }
            break;
        case PACKET_DISCONNECT:
            GestisciDisconnessione(gioco);
            break;
        case PACKET_HOST_DISCONNECT:
            // TL-01: L'host ha chiuso la stanza.
            // Il client riceve il pacchetto, mostra una notifica breve
            // e viene reindirizzato automaticamente al menu principale.
            if (currentRole == NET_CLIENT) {
                gioco->gioco_finito = 1;
                strcpy(gioco->messaggio, "L'host ha chiuso la stanza");
                gioco->timer_notifica = 2.5f;  // Notifica visibile 2.5 secondi
                CloseNetwork();
                currentRole = NET_OFFLINE;
            }
            break;
        case PACKET_COLOR_CHOICE: {
            int pid = packet->data.color_choice.player_id;
            if (currentRole == NET_HOST && gioco->in_scelta_colore && gioco->autore_animazione == pid) {
                Colore coloreS = packet->data.color_choice.colore;
                gioco->in_scelta_colore = 0;
                ApplicaEffetto(gioco, gioco->carta_pendente, coloreS);
                gioco->scarti[gioco->num_carte_scarti++] = gioco->carta_pendente;
                
                if (GetLunghezzaMano(&gioco->giocatori[pid]) == 0) {
                    gioco->gioco_finito = 1;
                    sprintf(gioco->messaggio, "HA VINTO %s!", Giocatore_Nome(&gioco->giocatori[pid]));
                    SalvaStatisticheHostVincitaClient(gioco, pid);
                } else if (GetLunghezzaMano(&gioco->giocatori[pid]) == 1) {
                    gioco->deve_chiamare_uno = pid + 1;
                    gioco->timer_uno = 3.5f;
                }
                
                BroadcastGameState(gioco);
                BroadcastPlayerHands(gioco);
            }
            break;
        }
        case PACKET_MOVE: {
            int pid = packet->data.move.player_id;
            int idx = packet->data.move.card_index;
            if (currentRole == NET_HOST) {
                if (gioco->turno_corrente != pid || idx < 0 || idx >= GetLunghezzaMano(&gioco->giocatori[pid])) {
                    MostraNotifica(gioco, "Mossa non valida (turno/indice)!");
                    break;
                }
                
                Giocatore *p = &gioco->giocatori[pid];
                Carta c = GetCartaGiocatore(p, idx);
                
                if (!MossaValida(gioco, c)) {
                    MostraNotifica(gioco, "Mossa non valida (carta non giocabile)!");
                    break;
                }
                
                RimuoviCartaGiocatore(p, idx);
                gioco->scarti[gioco->num_carte_scarti++] = c;
                if (c.colore == NERO) {
                    gioco->colore_attivo = packet->data.move.colore_scelto;
                } else {
                    gioco->colore_attivo = c.colore;
                }
                ApplicaEffetto(gioco, c, gioco->colore_attivo);
                
                /* Controlla UNO/Vittoria con classifica reale */
                if (GetLunghezzaMano(p) == 0) {
                    gioco->gioco_finito = 1;
                    sprintf(gioco->messaggio, "HA VINTO %s!", Giocatore_Nome(p));
                    SalvaStatisticheHostVincitaClient(gioco, pid);
                } else if (GetLunghezzaMano(p) == 1) {
                    if (!p->is_bot) {
                        gioco->deve_chiamare_uno = pid + 1;
                        gioco->timer_uno = 3.5f;
                    }
                }
                
                gioco->anim_attiva = 1;
                gioco->anim_gia_applicata = 1;
                gioco->anim_carta = c;
                gioco->tipo_animazione = 0;
                gioco->anim_start = OttieniPosizioneGiocatore(pid, local_player_id, gioco->num_giocatori); 
                gioco->anim_end = (Vector2){ 1280/2.0f + 25, 720/2.0f - 80 };
                gioco->anim_t = 0.0f;
                gioco->autore_animazione = pid;
                
                BroadcastGameState(gioco);
                BroadcastPlayerHands(gioco);
            }
            break;
        }
        case PACKET_DRAW: {
            int pid = packet->data.draw.player_id;
            if (currentRole == NET_HOST) {
                if (gioco->turno_corrente != pid) {
                    MostraNotifica(gioco, "Non e' il tuo turno!");
                    break;
                }
                
                /* Esegui la pescata */
                Pesca(gioco, pid, packet->data.draw.count);
                Giocatore* p = &gioco->giocatori[pid];
                /* Sposta le carte pescate in fondo alla mano (ordine visivo)
                 * tramite la funzione di dominio basata sull'ADT ListaCarte. */
                for (int k = 0; k < packet->data.draw.count; k++) {
                    GameLogic_MettiPrimaInFondo(p);
                }
                /* Cambia turno: avanzamento gestito dalla funzione di dominio
                 * che mantiene sincronizzata la CodaTurni (ADT). */
                GameLogic_AvanzamentoTurno(gioco, 1);
                
                /* Avvia animazione di pescata */
                gioco->anim_attiva = 1;
                gioco->anim_gia_applicata = 1;
                gioco->tipo_animazione = 1;
                gioco->anim_carta = (Carta){0};
                gioco->anim_start = (Vector2){ 1280/2.0f - 130, 720/2.0f - 80 };
                gioco->anim_end = OttieniPosizioneGiocatore(pid, local_player_id, gioco->num_giocatori);
                gioco->anim_t = 0.0f;
                gioco->autore_animazione = pid;
                gioco->anim_pesca_count = 0;
                
                MostraNotifica(gioco, TextFormat("%s ha pescato.", gioco->giocatori[pid].nome));
                
                BroadcastGameState(gioco);
                BroadcastPlayerHands(gioco);
            }
            break; 
        }
        default: break;
    }
}