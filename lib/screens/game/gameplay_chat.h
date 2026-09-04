/**
 * @file gameplay_chat.h
 * @brief Gestione della chat in-game.
 * @defgroup gameplay_chat Gameplay Chat
 * @brief Funzioni di aggiornamento e rendering della chat durante la partita.
 */
#ifndef GAMEPLAY_CHAT_H
#define GAMEPLAY_CHAT_H

#include "../../data_structures/data_structures.h"
#include "../../game/player.h"
#include "../../game/game_state.h"

/**
 * @brief Aggiorna lo stato della chat (input, ricezione messaggi).
 * @pre gioco != NULL.
 * @post inputChat aggiornato; messaggi ricevuti processati.
 * @param gioco Stato del gioco.
 */
void AggiornaChat(StatoGioco* gioco);

/**
 * @brief Disegna la finestra chat sullo schermo.
 * @pre gioco != NULL.
 * @post Chat renderizzata.
 * @param gioco Stato del gioco.
 */
void DisegnaChat(StatoGioco* gioco);

/** @brief Buffer per input chat. */
extern char inputChat[256];
/** @brief Flag stato digitazione (1 se l'utente sta scrivendo). */
extern int isTyping;

#endif
