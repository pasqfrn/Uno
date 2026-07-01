/**
 * @file packet_handler.h
 * @brief Header principale per la gestione pacchetti di rete.
 * @defgroup packet_handler Packet Handler
 * @brief Include tutti i sotto-moduli di gestione pacchetti per retrocompatibilità.
 */
#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H

#include "network.h"
#include "../../game_logic.h"
#include "packet_handler_game.h"
#include "packet_handler_admin.h"

/**
 * @brief Gestisce un pacchetto di rete ricevuto.
 * @pre packet != NULL; gioco != NULL.
 * @post Pacchetto dispatchato al gestore appropriato; stato aggiornato.
 * @param packet Pacchetto ricevuto.
 * @param gioco Stato di gioco.
 */
void HandleReceivedPacket(NetPacket* packet, StatoGioco* gioco);

#endif
