/**
 * @file packet_handler_game.h
 * @brief Gestione pacchetti di gioco (mosse, pesche, chat, disconnessione).
 * @defgroup packet_handler_game Packet Handler Game
 * @brief Funzioni che gestiscono i pacchetti relativi al gioco:
 * JOIN_GAME, GAME_STATE, PLAYER_HAND, CHAT, DISCONNECT,
 * COLOR_CHOICE, MOVE, DRAW, START_GAME.
 */
#ifndef PACKET_HANDLER_GAME_H
#define PACKET_HANDLER_GAME_H

#include "../../data_structures.h"
#include "../../player.h"
#include "../../game_state.h"
#include "../../socket/network/network.h"

/**
 * @brief Gestisce i pacchetti di gioco ricevuti.
 * @pre packet != NULL; gioco != NULL.
 * @post Pacchetto elaborato; stato di gioco aggiornato.
 * @param packet Pacchetto ricevuto.
 * @param gioco Stato del gioco.
 */
void HandleGamePacket(NetPacket* packet, StatoGioco* gioco);

#endif
