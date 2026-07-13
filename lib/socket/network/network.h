/**
 * @file network.h
 * @brief Modulo di rete multiplayer TCP/UDP per partite e sincronizzazione.
 * @defgroup network Rete Multiplayer
 * @brief Gestisce connessioni TCP, sessioni di gioco, pacchetti di rete
 * e stato multiplayer. Include anche sincronizzazione live statistiche.
 */
#ifndef NETWORK_H
#define NETWORK_H

#include "../../data_structures.h"
#include "../../player.h"
#include "../../game_state.h"
#include "../lan/lan_protocol.h" /* Per STORICO_SYNC_COUNT usato in StatsSyncPacket */

#ifdef _WIN32
    #define NOGDI
    #define NOUSER
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

/**
 * @addtogroup network_ruoli Ruoli di Rete
 * @brief Enumerazione dei ruoli multiplayer.
 * @{
 */
/** @brief Giocatore offline (multigiocatore disattivato). */
typedef enum { NET_OFFLINE, NET_HOST, NET_CLIENT } NetworkRole;
/** @} */

/**
 * @addtogroup network_packet Tipi di Pacchetto
 * @brief Enumerazione di tutti i tipi di pacchetti scambiati in rete.
 * @{
 */
typedef enum {
    PACKET_JOIN,
    PACKET_START_GAME,
    PACKET_GAME_STATE,
    PACKET_MOVE,
    PACKET_DRAW,
    PACKET_CHAT,
    PACKET_DISCONNECT,
    PACKET_JOIN_GAME,
    PACKET_PLAYER_HAND,
    PACKET_COLOR_CHOICE,
    PACKET_CREATE_SESSION,
    PACKET_JOIN_SESSION,
    PACKET_SESSION_LIST,
    PACKET_SESSION_STATE,
    PACKET_HEARTBEAT,
    PACKET_PLAYER_READY,
    PACKET_GAME_FINISHED,
    PACKET_ADMIN_USER_SYNC,
    PACKET_ADMIN_DB_SYNC,
    PACKET_ADMIN_DB_SYNC_DONE,
    PACKET_HOST_DISCONNECT,
    PACKET_STATS_SYNC,
    PACKET_JOIN_REJECTED,
    PACKET_PLAY_AGAIN,
    PACKET_HOST_RETURN_LOBBY
} PacketType;
/** @} */

/**
 * @addtogroup network_packet_strutture Strutture Pacchetti
 * @brief Strutture dati per ogni tipo di pacchetto multiplayer.
 * @{
 */
/** @brief Pacchetto mossa: giocatore, indice carta, colore scelto. */
typedef struct { int player_id; int card_index; Colore colore_scelto; } PacketMove;
/** @brief Pacchetto pesca: giocatore e quantità. */
typedef struct { int player_id; int count; } PacketDraw;
/** @brief Pacchetto stato di gioco completo. */
typedef struct { StatoGioco stato_gioco; } PacketGameState;
/** @brief Pacchetto scelta colore. */
typedef struct { int player_id; Colore colore; } PacketColorChoice;
/** @brief Pacchetto chat. */
typedef struct { int player_id; char message[128]; } PacketChat;
/** @brief Pacchetto join game. */
typedef struct { char nome[50]; } PacketJoinGame;
/** @brief Pacchetto join rifiutato. */
typedef struct { char motivo[100]; } PacketJoinRejected;
/** @brief Pacchetto mano giocatore. */
typedef struct { int player_id; Carta carte[108]; int num_carte; } PacketPlayerHand;
/** @brief Pacchetto creazione sessione. */
typedef struct { char session_id[16]; char host_name[30]; int num_players; } PacketCreateSession;
/** @brief Pacchetto join sessione. */
typedef struct { char session_id[16]; char player_name[30]; } PacketJoinSession;
/** @brief Pacchetto lista sessioni. */
typedef struct { int session_count; char sessions[10][16]; } PacketSessionList;
/**
 * @brief Pacchetto stato sessione multiplayer.
 */
typedef struct { 
    char session_id[16]; 
    int num_players;
    char player_names[4][30]; 
    int player_ready[4];
    int game_started;
} PacketSessionState;
/** @brief Pacchetto ready/play again. */
typedef struct { int player_id; int is_ready; int want_play_again; } PacketPlayerReady;
/** @brief Pacchetto fine partita. */
typedef struct { int winning_player_id; char winner_name[30]; } PacketGameFinished;
/**
 * @brief Pacchetto play again (con nome giocatore per fix bug).
 */
typedef struct { int player_id; float timestamp; int play_again; char player_name[30]; } PacketPlayAgain;
/** @brief Pacchetto heartbeat per keep-alive. */
typedef struct { int player_id; float timestamp; } PacketHeartbeat;
/** @brief Pacchetto host return lobby. */
typedef struct { int host_ready; } PacketHostReturnLobby;

// --- Strutture per la sincronizzazione Admin ---
// Contiene i dati completi di un singolo utente (Statistiche) SENZA storico partite
// per mantenere i pacchetti di rete piccoli.
typedef struct { 
    char email[100]; 
    char nome[30]; 
    char cognome[30]; 
    char username[30]; 
    char password[30]; 
    char pin_recupero[7]; 
    int partite_giocate; 
    int vittorie_giocatore; 
    int vittorie_bot; 
} AdminUserSyncData;

// Contiene l'azione admin (modifica/elimina/crea)
typedef struct { AdminUserSyncData user; int action; /* 0=sync, 1=update, 2=delete, 3=create */ int target_id; } AdminUserAction;

// Struttura per la sincronizzazione DB completa: invia un singolo utente per pacchetto
// con indice progressivo e flag di completamento
typedef struct { 
    int user_index;              // Indice progressivo di questo utente nella sequenza
    int total_users;             // Numero totale di utenti da sincronizzare
    AdminUserSyncData user;      // Dati dell'utente (senza storico partite)
} AdminDBSyncPacket;

/**
 * @brief StatsSyncPacket - Sincronizzazione live statistiche dopo una partita.
 *
 * BUG 3 FIX: Il campo storico_partite[] (fino a STORICO_SYNC_COUNT partite) e
 * storico_count sono stati aggiunti per trasmettere anche i dettagli delle
 * partite recenti via TCP, non solo i contatori numerici. Il ricevente merge
 * questi dati nel proprio storico_partite[] locale.
 *
 * Il merge avviene nel packet handler PACKET_STATS_SYNC usando (durata, esito,
 * modalita, posizione) come chiave composta di deduplica, identica alla logica
 * usata in lan_db_sync.c e lan_broadcast.c per la sincronizzazione LAN.
 *
 * IMPORTANTE: Il pacchetto StatsSyncPacket e' contenuto nell'unione NetPacket
 * che include gia' StatoGioco (grande ~20KB), quindi l'aggiunta di un piccolo
 * storico non impatta la dimensione totale del pacchetto.
 */
typedef struct {
    char username[30];                // Username del giocatore a cui appartengono le statistiche
    int partite_giocate;              // Totale partite giocate
    int vittorie_giocatore;           // Totale vittorie
    int vittorie_bot;                 // Totale sconfitte
    float durata_ultima_partita;      // Durata in secondi (singola partita per retrocompatibilità)
    char modalita_ultima[20];         // "Online" o "1 vs X" (singola partita per retrocompatibilità)
    char esito_ultima[15];            // "Vittoria" o "Sconfitta" (singola partita per retrocompatibilità)
    int posizione_ultima;             // Posizione finale 1-4 (singola partita per retrocompatibilità)
    /* BUG 3 FIX: Piccolo storico partite (ultime STORICO_SYNC_COUNT partite)
     * per sincronizzazione TCP. Viene popolato in BroadcastStatsSync/SendStatsSync
     * e mergiato in PACKET_STATS_SYNC. storico_count indica quante partite valide. */
    PartitaRegistrata storico_partite[STORICO_SYNC_COUNT]; /* Ultime N partite */
    int storico_count;                /* Numero di partite valide in storico_partite[] */
} StatsSyncPacket;

// Struttura generica per i pacchetti di rete
typedef struct {
    PacketType type;
    int sender_id;
    union {
        PacketMove move;
        PacketDraw draw;
        PacketGameState game_state;
        PacketColorChoice color_choice;
        PacketChat chat;
        PacketJoinGame join_game;
        PacketJoinRejected join_rejected;
        PacketPlayerHand player_hand;
        PacketCreateSession create_session;
        PacketJoinSession join_session;
        PacketSessionList session_list;
        PacketSessionState session_state;
        PacketPlayerReady player_ready;
        PacketGameFinished game_finished;
        PacketPlayAgain play_again;
        PacketHostReturnLobby host_return_lobby;
        PacketHeartbeat heartbeat;
        AdminUserAction admin_action;
        AdminDBSyncPacket admin_db_packet;  // Sostituisce il vecchio AdminDBSyncData
        StatsSyncPacket stats_sync;         // Sincronizzazione live statistiche
        char raw[512];
    } data;
} NetPacket;

// Massimo numero di client gestibili dall'host (incluso lo slot 0 riservato)
#define MAX_SERVER_CLIENTS 4

// --- Funzioni e Variabili Globali ---
extern int network_start_game_flag;
extern NetworkRole currentRole;
extern int join_rejected;
extern char reject_message[100];
extern int isConnected;
extern int network_pending;
extern int network_game_initialized;
extern int network_client_joined;
extern int player_id;                    // ID del giocatore locale
extern char current_session_id[16];      // ID della sessione corrente
extern char session_id[16];              // ID della sessione (alias per compatibilità)
extern int local_player_id;              // ID del giocatore locale nella sessione
extern int host_in_prelobby;             // 1 se l'host e' gia' tornato in pre-lobby (per i client)

// Socket dei client connessi (visibile a network_utils.c)
extern SOCKET clientSockets[MAX_SERVER_CLIENTS];

// Username dei client TCP connessi all'host (per admin panel - visibile ad admin_draw.c)
extern char connectedClientUsernames[4][30];

int GetLocalIPAddress(char* outbuf, int buflen);
int IsPlayerConnected(int player_id);
void RiassegnaSocket(int old_id, int new_id, int is_reconnect, int partita_iniziata);

// Funzioni di invio (dichiarate in network_send.c, usate da piu' moduli)
void BroadcastHostDisconnect(void);
void SendPlayerReady(int ready);
void SendPlayAgain(void);
void BroadcastHostReturnLobby(void);

int ConnettiTramiteCodiceStanza(const char* codice, int portaBase);
int InitNetwork(void);
void ShiftNetworkSockets(int removed_index, int num_players);
void CloseNetwork(void);
int HostGame(int port);
int JoinGame(const char* ip_address, int port);
void UpdateNetwork(StatoGioco* gioco);
void HandleReceivedPacket(NetPacket* packet, StatoGioco* gioco);
void GestisciDisconnessione(StatoGioco* gioco);

void IpToCode(const char* ip, char* code);
int CodeToIp(const char* code, char* ip);

// Sincronizzazione live statistiche
void BroadcastStatsSync(const StatsSyncPacket* stats);
void SendStatsSync(const StatsSyncPacket* stats);

#endif