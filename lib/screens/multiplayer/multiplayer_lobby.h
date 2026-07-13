/**
 * @file multiplayer_lobby.h
 * @brief Schermate della lobby multiplayer (host e waiting room).
 * @defgroup multiplayer_lobby Multiplayer Lobby
 * @brief Gestisce la lobby host e la waiting room per il multiplayer
 * con fino a 4 giocatori.
 */
#ifndef MULTIPLAYER_LOBBY_H
#define MULTIPLAYER_LOBBY_H

#include "../../data_structures.h"
#include "../../player.h"
#include "../../game_state.h"
#include "../../socket/server/server_manager.h"
#include "raylib.h"

/**
 * @brief Disegna la lobby multiplayer (schermata host).
 * @pre session != NULL; fase != NULL.
 * @post Lobby host renderizzata con giocatori in attesa.
 * @param session Sessione di gioco.
 * @param fase Fase corrente.
 */
void DisegnaMultiplayerLobby(GameSession* session, FaseApplicazione* fase);

/**
 * @brief Disegna la waiting room (schermata client in attesa).
 * @pre session != NULL; fase != NULL.
 * @post Waiting room renderizzata.
 * @param session Sessione di gioco.
 * @param fase Fase corrente.
 * @param gioco Stato del gioco.
 */
void DisegnaWaitingRoom(GameSession* session, FaseApplicazione* fase, StatoGioco* gioco);

/**
 * @brief Resetta il timer di errore avvio partita.
 * @post Timer di errore resettato.
 */
void ResetTimerErroreAvvio(void);

#endif
