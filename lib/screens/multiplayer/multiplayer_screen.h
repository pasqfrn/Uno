/**
 * @file multiplayer_screen.h
 * @brief Schermate multiplayer (host, join, lobby).
 * @defgroup multiplayer_screen Multiplayer Screen
 * @brief Gestisce la logica e il disegno delle schermate multiplayer:
 * creazione/join sessione, lobby host, lobby client.
 */
#ifndef MULTIPLAYER_SCREEN_H
#define MULTIPLAYER_SCREEN_H

#include "../../data_structures.h"
#include "../../player.h"
#include "../../game_state.h"
#include "raylib.h"

/**
 * @brief Disegna e gestisce le schermate multiplayer.
 * @pre gioco != NULL; fase != NULL.
 * @post Schermata multiplayer aggiornata e renderizzata.
 * @param gioco Stato del gioco.
 * @param fase Fase corrente.
 * @param network_countdown Puntatore al timer countdown.
 * @param network_countdown_active Puntatore al flag countdown attivo.
 */
void DisegnaMultiplayer(StatoGioco* gioco, FaseApplicazione* fase, float* network_countdown, int* network_countdown_active);

/**
 * @brief Resetta gli errori della schermata multiplayer.
 * @post Flag errori resettati.
 */
void ResetMultiplayerError(void);

#endif
