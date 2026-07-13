/**
 * @file packet_handler_admin.h
 * @brief Gestione pacchetti admin e sessione multiplayer.
 * @defgroup packet_handler_admin Packet Handler Admin
 * @brief Funzioni che gestiscono i pacchetti di amministrazione e sessioni:
 * CREATE_SESSION, JOIN_SESSION, PLAYER_READY, GAME_FINISHED,
 * HEARTBEAT, ADMIN_USER_SYNC, ADMIN_DB_SYNC.
 */
#ifndef PACKET_HANDLER_ADMIN_H
#define PACKET_HANDLER_ADMIN_H

#include "../../data_structures.h"
#include "../../player.h"
#include "../../game_state.h"
#include "../../socket/network/network.h"

/**
 * @brief Gestisce i pacchetti admin e di sessione ricevuti.
 * @pre packet != NULL; gioco != NULL.
 * @post Pacchetto elaborato; sessione/admin aggiornato.
 * @param packet Pacchetto ricevuto.
 * @param gioco Stato del gioco.
 */
void HandleAdminPacket(NetPacket* packet, StatoGioco* gioco);

#endif
