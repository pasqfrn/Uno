/**
 * @file network_send.h
 * @brief Funzioni di invio pacchetti multiplayer.
 * @defgroup network_send Network Send
 * @brief Wrapper per l'invio di pacchetti di rete: game state, chat, mosse,
 * admin sync, statistiche e gestione sessioni.
 */
#ifndef NETWORK_SEND_H
#define NETWORK_SEND_H

#include "network.h"
#include "../../game_state.h"

/**
 * @brief Invia pacchetto di inizio partita a tutti i client.
 * @post PACKET_START_GAME broadcastato.
 */
void SendStartGame(void);

/**
 * @brief Invia lo stato di gioco completo all'host.
 * @pre gioco != NULL.
 * @param gioco Stato di gioco.
 */
void SendGameState(StatoGioco* gioco);

/**
 * @brief Invia un messaggio chat in broadcast.
 * @pre message != NULL.
 * @param message Testo del messaggio.
 */
void SendChat(const char* message);

/**
 * @brief Invia una mossa (carta giocata + colore).
 * @pre card_index valido; colore valido.
 * @param card_index Indice della carta.
 * @param colore Colore scelto (per jolly).
 */
void SendMove(int card_index, Colore colore);

/**
 * @brief Invia una richiesta di pescata.
 * @pre count > 0.
 * @param count Numero di carte da pescare.
 */
void SendDraw(int count);

/**
 * @brief Invia una scelta colore.
 * @param colore Colore scelto.
 */
void SendColorChoice(Colore colore);

/**
 * @brief Invia richiesta di join game.
 * @pre nome != NULL.
 * @param nome Nome del giocatore.
 */
void SendJoinGame(const char* nome);

/**
 * @brief Invia la mano di un giocatore a un socket specifico.
 * @pre gioco != NULL.
 * @param target_socket Socket destinatario.
 * @param data_player_id ID del giocatore.
 * @param gioco Stato di gioco.
 */
void SendPlayerHand(int target_socket, int data_player_id, StatoGioco* gioco);

/**
 * @brief Broadcasta lo stato di gioco a tutti i client.
 * @pre gioco != NULL.
 * @param gioco Stato di gioco.
 */
void BroadcastGameState(StatoGioco* gioco);

/**
 * @brief Broadcasta la mano di tutti i giocatori.
 * @pre gioco != NULL.
 * @param gioco Stato di gioco.
 */
void BroadcastPlayerHands(StatoGioco* gioco);

/**
 * @brief Broadcasta un messaggio chat a tutti.
 * @pre message != NULL.
 * @param message Testo del messaggio.
 */
void BroadcastChat(const char* message);

/**
 * @brief Invia richiesta di creazione sessione.
 * @pre host_name != NULL.
 * @param host_name Nome dell'host.
 */
void SendCreateSession(const char* host_name);

/**
 * @brief Invia richiesta di join a una sessione.
 * @pre sess_id != NULL; player_name != NULL.
 * @param sess_id ID della sessione.
 * @param player_name Nome del giocatore.
 */
void SendJoinSession(const char* sess_id, const char* player_name);

/**
 * @brief Invia stato ready/play again.
 * @param ready 1 se ready, 0 altrimenti.
 */
void SendPlayerReady(int ready);

/**
 * @brief Invia heartbeat (keep-alive).
 */
void SendHeartbeat(void);

/**
 * @brief Invia richiesta di giocare ancora (play again).
 */
void SendPlayAgain(void);

/**
 * @brief Broadcasta che l'host e' tornato in pre-lobby.
 */
void BroadcastHostReturnLobby(void);

/**
 * @addtogroup network_send_admin Admin Sync
 * @brief Funzioni per la sincronizzazione dell'admin panel.
 * @{
 */
/**
 * @brief Invia i dati utente del client all'host (al collegamento).
 * @pre userData != NULL.
 * @param userData Dati dell'utente.
 */
void SendAdminUserSync(const AdminUserSyncData* userData);

/**
 * @brief Invia un'azione admin all'host TCP.
 * @pre action != NULL.
 * @param action Azione da eseguire.
 */
void SendAdminUserAction(const AdminUserAction* action);

/**
 * @brief Invia l'azione admin (modifica/elimina/crea) a tutti i client.
 * @pre action != NULL.
 * @param action Azione da eseguire.
 */
void BroadcastAdminUserAction(const AdminUserAction* action);

/**
 * @brief Invia il DatabaseUtenti aggiornato a tutti i client.
 * @pre DB valido.
 */
void BroadcastAdminDB(void);
/** @} */

/**
 * @brief Broadcasta sincronizzazione live statistiche.
 * @pre stats != NULL.
 * @param stats Statistiche sincronizzate.
 */
void BroadcastStatsSync(const StatsSyncPacket* stats);

/**
 * @brief Invia sincronizzazione live statistiche a un client specifico.
 * @pre stats != NULL.
 * @param stats Statistiche sincronizzate.
 */
void SendStatsSync(const StatsSyncPacket* stats);

#endif
