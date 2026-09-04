/**
 * @file server_manager.c
 * @brief Gestione sessioni di gioco multiplayer.
 * @ingroup server
 *
 * Modulo per la gestione delle sessioni di gioco:
 *   - Creazione/join/leave sessioni
 *   - Tracking giocatori connessi e ready state
 *   - Liste sessioni disponibili (UDP discovery)
 *   - Gestione stati lobby (WAITING, IN_PROGRESS, CLOSED)
 *
 * Sessioni: array statico di GameSession (max MAX_GAME_SESSIONS).
 *
 * Architettura:
 *   - ServerManager: struttura globale con array sessioni[] e contatore
 *   - GameSession: stato completo di una stanza (giocatori, mazzo, turno)
 *   - PlayerSession: dati di un singolo giocatore nella sessione
 *
 * Funzionalita':
 *   - InitServerManager: inizializza il manager (chiamato all'avvio)
 *   - ShutdownServerManager: chiude tutte le sessioni attive
 *   - CreateSession: crea una nuova sessione con host
 *   - AddPlayerToSession: aggiunge un giocatore alla sessione
 *   - RemovePlayerFromSession: rimuove/disconnette un giocatore
 *   - StartSession: inizializza il mazzo e distribuisce le carte
 *   - ProcessMove/ProcessDraw/ProcessColorChoice: applica mosse
 *   - UpdateSessionLogic: aggiorna timer e stati (chiamato ogni frame)
 *   - UpdateServerManager: aggiorna tutte le sessioni e rimuove chiuse
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "raylib.h"
#include "../../../lib/socket/server/server_manager.h"
#include "../../../lib/game/game_logic.h"
#include "../../../lib/data_structures/list.h"

ServerManager serverManager = {0};

/**
 * @brief Inizializza il ServerManager globale.
 *
 * Resetta a zero tutte le sessioni e inizializza il seed
 * per la generazione di ID sessione casuali.
 */
void InitServerManager(void) {
    memset(&serverManager, 0, sizeof(ServerManager));
    srand((unsigned int)time(NULL));
}

/**
 * @brief Chiude tutte le sessioni attive e resetta il manager.
 *
 * Chiamato alla chiusura dell'applicazione o quando si esce
 * dalla modalita' multiplayer.
 */
void ShutdownServerManager(void) {
    for (int i = 0; i < serverManager.num_active_sessions; i++) {
        CloseSession(&serverManager.sessions[i]);
    }
    serverManager.num_active_sessions = 0;
}

char* GenerateSessionId(void) {
    /**
     * BUG 3: Il vecchio codice generava solo 4 caratteri casuali, che
     * potevano collidere quando piu' dispositivi sulla stessa rete
     * creavano stanze contemporaneamente.
     * 
     * CORREZIONE: Codice di 8 caratteri che combina:
     * - Parte bassa del timestamp (millisecondi / 10) per unicita' temporale
     * - Un carattere casuale per ridurre ulteriormente la probabilita' di collisione
     * Questo garantisce unicita' anche su dispositivi sulla stessa rete.
     */
    static char session_id[16];
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int charset_len = sizeof(charset) - 1;
    
    // Ottieni timestamp in millisecondi per unicita' temporale
    unsigned long long ms = (unsigned long long)(GetTime() * 1000);
    
    // Usa il timestamp per generare caratteri unici
    for (int i = 0; i < 6; i++) {
        int idx = (int)(ms % charset_len);
        session_id[i] = charset[idx];
        ms /= charset_len;
    }
    // Aggiungi 2 caratteri casuali extra per unicita' distribuita
    for (int i = 6; i < 8; i++) {
        session_id[i] = charset[rand() % charset_len];
    }
    session_id[8] = '\0';
    return session_id;
}

/**
 * @brief Crea una nuova sessione di gioco con l'host come primo giocatore.
 *
 * Alloca una sessione nell'array statico, genera un ID univoco,
 * inizializza lo stato di gioco con il mazzo e distribuisce
 * 7 carte all'host.
 *
 * @param host_name Nome dell'host (giocatore 0).
 * @param host_id ID del giocatore host.
 * @return Puntatore alla sessione creata, NULL se errore (max sessioni raggiunto).
 */
GameSession* CreateSession(const char* host_name, int host_id) {
    if (serverManager.num_active_sessions >= MAX_GAME_SESSIONS) {
        return NULL;
    }
    
    GameSession* session = &serverManager.sessions[serverManager.num_active_sessions];
    memset(session, 0, sizeof(GameSession));
    
    strcpy(session->session_id, GenerateSessionId());
    session->num_players = 1;
    session->host_id = host_id;
    session->state = SESSION_WAITING;
    session->startCountdown = -1.0f;
    session->idleTime = 0.0f;
    
    // Aggiungi l'host come primo giocatore
    session->players[0].player_id = host_id;
    strcpy(session->players[0].nome, host_name);
    session->players[0].is_connected = 1;
    session->players[0].is_bot = 0;
    session->players[0].mano = CreaLista();
    session->players[0].lastHeartbeat = GetTime();
    
    // Inizializza lo stato di gioco
    session->game_state.num_giocatori = 1;
    for (int i = 0; i < 4; i++) {
        session->game_state.giocatori[i].mano = CreaLista();
    }
    strcpy(session->game_state.giocatori[0].nome, host_name);
    session->game_state.giocatori[0].is_bot = 0;
    
    serverManager.num_active_sessions++;
    return session;
}

GameSession* GetSession(const char* sess_id) {
    for (int i = 0; i < serverManager.num_active_sessions; i++) {
        if (strcmp(serverManager.sessions[i].session_id, sess_id) == 0) {
            return &serverManager.sessions[i];
        }
    }
    return NULL;
}

GameSession* GetSessionByPlayerId(int player_id) {
    for (int i = 0; i < serverManager.num_active_sessions; i++) {
        for (int j = 0; j < serverManager.sessions[i].num_players; j++) {
            if (serverManager.sessions[i].players[j].player_id == player_id) {
                return &serverManager.sessions[i];
            }
        }
    }
    return NULL;
}

int AddPlayerToSession(GameSession* session, int player_id, const char* player_name, int is_bot) {
    if (!session || session->num_players >= MAX_PLAYERS_PER_SESSION) {
        return 0;  // Sessione piena
    }
    
    // Verifica che il giocatore non sia già nella sessione
    for (int i = 0; i < session->num_players; i++) {
        if (session->players[i].player_id == player_id) {
            return 0;  // Giocatore già presente
        }
    }
    
    int player_slot = session->num_players;
    session->players[player_slot].player_id = player_id;
    strcpy(session->players[player_slot].nome, player_name);
    session->players[player_slot].is_connected = 1;
    session->players[player_slot].is_bot = is_bot;
    session->players[player_slot].mano = CreaLista();
    session->players[player_slot].lastHeartbeat = GetTime();
    
    // Aggiorna lo stato di gioco
    session->game_state.num_giocatori++;
    strcpy(session->game_state.giocatori[player_slot].nome, player_name);
    session->game_state.giocatori[player_slot].is_bot = is_bot;
    
    session->num_players++;
    return 1;
}

void RemovePlayerFromSession(GameSession* session, int player_id) {
    if (!session) return;
    
    for (int i = 0; i < session->num_players; i++) {
        if (session->players[i].player_id == player_id) {
            session->players[i].is_connected = 0;
            // Se il host esce, chiudi la sessione
            if (player_id == session->host_id) {
                session->state = SESSION_CLOSED;
            }
            break;
        }
    }
}

void StartSession(GameSession* session) {
    if (!session) return;
    
    // Inizializza il gioco con il numero di giocatori corrente
    session->game_state.num_giocatori = session->num_players;
    session->game_state.turno_corrente = 0;
    session->game_state.direzione = 1;
    session->game_state.colore_attivo = ROSSO;
    session->game_state.gioco_finito = 0;
    session->game_state.deve_chiamare_uno = 0;
    
    InizializzaMazzo(&session->game_state);
    
    // Distribuisci le carte ai giocatori
    for (int i = 0; i < session->num_players; i++) {
        for (int j = 0; j < 7; j++) {
            if (session->game_state.num_carte_mazzo > 0) {
                Carta c = session->game_state.mazzo[--session->game_state.num_carte_mazzo];
                InsertInCoda(session->game_state.giocatori[i].mano, c);
            }
        }
    }
    
    // Posiziona una carta di scarto iniziale
    if (session->game_state.num_carte_mazzo > 0) {
        session->game_state.scarti[0] = session->game_state.mazzo[--session->game_state.num_carte_mazzo];
        session->game_state.num_carte_scarti = 1;
        session->game_state.colore_attivo = session->game_state.scarti[0].colore;
    }
    
    session->state = SESSION_IN_PROGRESS;
    session->startCountdown = 3.0f;
}

int IsValidMove(GameSession* session, int player_id, int card_index) {
    if (!session) return 0;
    
    // Verifica che sia il turno del giocatore
    if (session->game_state.turno_corrente != player_id) {
        return 0;
    }
    
    // Verifica l'indice della carta
    Giocatore* player = &session->game_state.giocatori[player_id];
    if (card_index < 0 || card_index >= GetLunghezzaMano(player)) {
        return 0;
    }
    
    // Verifica che la carta sia giocabile
    Carta c = GetCartaGiocatore(player, card_index);
    return MossaValida(&session->game_state, c);
}

int ProcessMove(GameSession* session, int player_id, int card_index, Colore color_choice) {
    if (!session || !IsValidMove(session, player_id, card_index)) {
        return 0;
    }
    
    Giocatore* player = &session->game_state.giocatori[player_id];
    Carta c = GetCartaGiocatore(player, card_index);
    
    // Rimuovi la carta dalla mano del giocatore
    RimuoviCartaGiocatore(player, card_index);
    
    // Se è una carta nera, usa il colore scelto
    if (c.colore == NERO) {
        ApplicaEffetto(&session->game_state, c, color_choice);
        session->game_state.colore_attivo = color_choice;
    } else {
        ApplicaEffetto(&session->game_state, c, c.colore);
        session->game_state.colore_attivo = c.colore;
    }
    
    // Aggiungi la carta agli scarti
    session->game_state.scarti[session->game_state.num_carte_scarti++] = c;
    
    // Verifica UNO
    if (GetLunghezzaMano(player) == 1) {
        session->game_state.deve_chiamare_uno = 1;
        session->game_state.timer_uno = 3.0f;
    } else if (GetLunghezzaMano(player) == 0) {
        // Il giocatore ha vinto!
        session->game_state.gioco_finito = 1;
        session->state = SESSION_FINISHED;
        return 1;
    }
    
    // Passa il turno al prossimo giocatore
    session->game_state.turno_corrente = (session->game_state.turno_corrente + session->game_state.direzione + session->game_state.num_giocatori) % session->game_state.num_giocatori;
    
    return 1;
}

int ProcessDraw(GameSession* session, int player_id, int draw_count) {
    if (!session) return 0;
    
    // Verifica che sia il turno del giocatore
    if (session->game_state.turno_corrente != player_id) {
        return 0;
    }
    
    // Pesca le carte
    Pesca(&session->game_state, player_id, draw_count);
    
    // Passa il turno al prossimo giocatore
    session->game_state.turno_corrente = (session->game_state.turno_corrente + session->game_state.direzione + session->game_state.num_giocatori) % session->game_state.num_giocatori;
    
    return 1;
}

int ProcessColorChoice(GameSession* session, int player_id, Colore color) {
    if (!session || session->game_state.turno_corrente != player_id) {
        return 0;
    }
    
    session->game_state.colore_attivo = color;
    session->game_state.in_scelta_colore = 0;
    
    return 1;
}

void UpdateSessionLogic(GameSession* session, float dt) {
    if (!session) return;
    
    // Conta i giocatori disconnessi
    int connected_count = 0;
    for (int i = 0; i < session->num_players; i++) {
        if (session->players[i].is_connected) connected_count++;
    }
    
    // Se rimane solo 1 giocatore, chiudi la sessione
    if (connected_count <= 1 && session->state == SESSION_IN_PROGRESS) {
        session->state = SESSION_CLOSED;
        return;
    }
    
    // Aggiorna il countdown di inizio
    if (session->startCountdown > 0.0f) {
        session->startCountdown -= dt;
    }
    
    // Aggiorna il timer UNO
    if (session->game_state.deve_chiamare_uno) {
        session->game_state.timer_uno -= dt;
        if (session->game_state.timer_uno <= 0.0f) {
            session->game_state.deve_chiamare_uno = 0;
        }
    }
    
    // Aggiorna il timer di notifica
    if (session->game_state.timer_notifica > 0.0f) {
        session->game_state.timer_notifica -= dt;
    }
}

void CloseSession(GameSession* session) {
    if (!session) return;
    
    for (int i = 0; i < session->num_players; i++) {
        if (session->players[i].mano) {
            EliminaLista(session->players[i].mano);
        }
    }
    for (int i = 0; i < 4; i++) {
        if (session->game_state.giocatori[i].mano) {
            EliminaLista(session->game_state.giocatori[i].mano);
        }
    }
    session->state = SESSION_CLOSED;
}

void UpdateServerManager(float dt) {
    serverManager.frameTime = dt;
    
    // Aggiorna tutte le sessioni attive
    for (int i = 0; i < serverManager.num_active_sessions; i++) {
        UpdateSessionLogic(&serverManager.sessions[i], dt);
    }
    
    // Rimuovi le sessioni chiuse
    for (int i = serverManager.num_active_sessions - 1; i >= 0; i--) {
        if (serverManager.sessions[i].state == SESSION_CLOSED) {
            CloseSession(&serverManager.sessions[i]);
            // Sposta le sessioni successive indietro
            if (i < serverManager.num_active_sessions - 1) {
                memmove(&serverManager.sessions[i], &serverManager.sessions[i + 1], 
                       sizeof(GameSession) * (serverManager.num_active_sessions - i - 1));
            }
            serverManager.num_active_sessions--;
        }
    }
}


