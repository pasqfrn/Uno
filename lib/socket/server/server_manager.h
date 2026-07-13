/**
 * @file server_manager.h
 * @brief Gestione delle sessioni di gioco lato server/host.
 * @defgroup server_manager Server Manager
 * @brief Implementa il server manager per gestire multiple sessioni
 * di gioco, con validazione lato server delle mosse.
 */
#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

#include "../../data_structures.h"
#include "../../player.h"
#include "../../game_state.h"
#include "../../data_structures/list.h"

/** @brief Numero massimo di sessioni di gioco contemporanee. */
#define MAX_GAME_SESSIONS 10
/** @brief Numero massimo di giocatori per sessione. */
#define MAX_PLAYERS_PER_SESSION 4
/** @brief Numero massimo di connessioni totali. */
#define MAX_CONNECTIONS 40

/**
 * @addtogroup server_manager_stati Stati Sessione
 * @brief Enumerazione degli stati possibili di una sessione.
 * @{
 */
/** @brief Sessione in attesa di giocatori. */
typedef enum { SESSION_WAITING, SESSION_STARTING, SESSION_IN_PROGRESS, SESSION_FINISHED, SESSION_CLOSED } SessionState;
/** @} */

/**
 * @addtogroup server_manager_giocatore Giocatore Sessione
 * @brief Informazioni di un giocatore in una sessione.
 * @{
 */
/**
 * @brief Informazioni giocatore nella sessione.
 */
typedef struct {
    int player_id;
    char nome[30];
    int is_connected;
    int is_bot;
    ListaCarte* mano;
    float lastHeartbeat;
} SessionPlayer;
/** @} */

/**
 * @addtogroup server_manager_sessione Sessione di Gioco
 * @brief Struttura di una sessione di gioco.
 * @{
 */
/**
 * @brief Sessione di gioco multiplayer.
 */
typedef struct {
    char session_id[16];
    int num_players;
    SessionPlayer players[MAX_PLAYERS_PER_SESSION];
    StatoGioco game_state;
    SessionState state;
    float startCountdown;
    float idleTime;
    int host_id;
} GameSession;
/** @} */

/**
 * @addtogroup server_manager_struttura Struttura Server Manager
 * @brief Container per tutte le sessioni attive.
 * @{
 */
/**
 * @brief Gestore di tutte le sessioni di gioco attive.
 */
typedef struct {
    GameSession sessions[MAX_GAME_SESSIONS];
    int num_active_sessions;
    float frameTime;
} ServerManager;
/** @} */

/** @brief Istanza globale del server manager (lato host). */
extern ServerManager serverManager;

/**
 * @addtogroup server_manager_api API Server Manager
 * @brief Funzioni per la gestione del server manager e delle sessioni.
 * @{
 */
/**
 * @brief Inizializza il server manager.
 * @post Tutte le sessioni resettate; num_active_sessions=0.
 */
void InitServerManager(void);

/**
 * @brief Aggiorna lo stato di tutte le sessioni.
 * @pre serverManager inizializzato.
 * @param dt Delta time.
 */
void UpdateServerManager(float dt);

/**
 * @brief Chiude il server manager e libera risorse.
 * @pre serverManager inizializzato.
 * @post Tutte le sessioni chiuse; memoria liberata.
 */
void ShutdownServerManager(void);

/**
 * @brief Crea una nuova sessione di gioco.
 * @pre host_name != NULL.
 * @post Sessione aggiunta a serverManager.
 * @param host_name Nome del host.
 * @param host_id ID del giocatore host.
 * @return Puntatore alla sessione creata, o NULL se errore.
 */
GameSession* CreateSession(const char* host_name, int host_id);

/**
 * @brief Cerca una sessione per ID.
 * @pre sess_id != NULL.
 * @return Puntatore a GameSession, o NULL se non trovata.
 * @param sess_id ID della sessione.
 */
GameSession* GetSession(const char* sess_id);

/**
 * @brief Cerca una sessione contenente un giocatore specifico.
 * @return Puntatore a GameSession, o NULL se non trovata.
 * @param player_id ID del giocatore.
 */
GameSession* GetSessionByPlayerId(int player_id);

/**
 * @brief Aggiunge un giocatore a una sessione.
 * @pre session valida; numero giocatori < MAX_PLAYERS_PER_SESSION.
 * @post Giocatore aggiunto; num_players incrementato.
 * @param session Sessione target.
 * @param player_id ID del giocatore.
 * @param player_name Nome del giocatore.
 * @param is_bot 1 se bot, 0 se umano.
 * @return 1 se aggiunto, 0 altrimenti.
 */
int AddPlayerToSession(GameSession* session, int player_id, const char* player_name, int is_bot);

/**
 * @brief Rimuove un giocatore da una sessione.
 * @pre session valida; player_id presente.
 * @post Giocatore rimosso; num_players decrementato.
 * @param session Sessione.
 * @param player_id ID del giocatore.
 */
void RemovePlayerFromSession(GameSession* session, int player_id);

/**
 * @brief Avvia il conteggio per l'inizio della partita.
 * @pre session valida e in SESSION_WAITING con abbastanza giocatori.
 * @post state = SESSION_STARTING; startCountdown impostato.
 * @param session Sessione da avviare.
 */
void StartSession(GameSession* session);

/**
 * @brief Aggiorna la logica di una sessione (bot AI, timer).
 * @pre session valida.
 * @param session Sessione.
 * @param dt Delta time.
 */
void UpdateSessionLogic(GameSession* session, float dt);

/**
 * @brief Chiude una sessione.
 * @pre session valida.
 * @post state = SESSION_CLOSED; risorse liberate.
 * @param session Sessione da chiudere.
 */
void CloseSession(GameSession* session);

/**
 * @brief Verifica se una mossa e' valida (lato server).
 * @pre session valida; player_id nella sessione.
 * @return 1 se mossa valida, 0 altrimenti.
 * @param session Sessione di gioco.
 * @param player_id ID del giocatore.
 * @param card_index Indice della carta da giocare.
 */
int IsValidMove(GameSession* session, int player_id, int card_index);

/**
 * @brief Processa una mossa (lato server).
 * @pre session valida; mossa valida.
 * @post Stato gioco aggiornato.
 * @param session Sessione.
 * @param player_id ID del giocatore.
 * @param card_index Indice carta.
 * @param color_choice Colore scelto (per jolly).
 * @return 1 se successo, 0 altrimenti.
 */
int ProcessMove(GameSession* session, int player_id, int card_index, Colore color_choice);

/**
 * @brief Processa una pescata (lato server).
 * @pre session valida; player_id nella sessione.
 * @post Carte pescate aggiunte alla mano.
 * @param session Sessione.
 * @param player_id ID del giocatore.
 * @param draw_count Numero di carte da pescare.
 * @return 1 se successo, 0 altrimenti.
 */
int ProcessDraw(GameSession* session, int player_id, int draw_count);

/**
 * @brief Processa una scelta colore (lato server).
 * @pre session valida; player_id nella sessione.
 * @post colore_attivo aggiornato.
 * @param session Sessione.
 * @param player_id ID del giocatore.
 * @param color Colore scelto.
 * @return 1 se successo, 0 altrimenti.
 */
int ProcessColorChoice(GameSession* session, int player_id, Colore color);

/**
 * @brief Genera un ID di sessione univoco (stringa casuale).
 * @return Puntatore a stringa allocata (da liberare con free).
 */
char* GenerateSessionId(void);
/** @} */

#endif
