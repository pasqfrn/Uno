/**
 * @file packet_handler.c
 * @brief Dispatcher principale dei pacchetti di rete.
 * @ingroup network
 *
 * Questo file contiene la funzione HandleReceivedPacket che delega
 * la gestione dei pacchetti ai moduli specializzati:
 *   - packet_handler_game.c  : pacchetti di gioco
 *   - packet_handler_admin.c : pacchetti admin/sessione
 *
 * Fattorizzato da packet_handler.c (461 righe) in:
 *   - packet_handler_game.h/.c  : gestione pacchetti di gioco
 *   - packet_handler_admin.h/.c : gestione pacchetti admin/sessione
 *   - packet_handler.c          : dispatcher (questo file)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "raylib.h"
#include "../../../lib/socket/network/packet_handler.h"
#include "../../../lib/socket/network/packet_handler_game.h"
#include "../../../lib/socket/network/packet_handler_admin.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/socket/network/state_sync.h"
#include "../../../lib/socket/server/server_manager.h"
#include "../../../lib/game/game_logic.h"
#include "../../../lib/screens/ui.h"
#include "../../../lib/data_structures/list.h"
#include "../../../lib/auth/auth.h"
#include "../../../lib/screens/game/gameplay_screen.h"
#include "../../../lib/screens/menu/string_utils.h"

/**
 * HandleReceivedPacket - Dispatcher principale dei pacchetti.
 *
 * Classifica il tipo di pacchetto e delega la gestione
 * al modulo specializzato appropriato.
 */
void HandleReceivedPacket(NetPacket* packet, StatoGioco* gioco) {
    if (!packet || !gioco) return;

    switch (packet->type) {
        /* --- Pacchetti di gioco --- */
        case PACKET_START_GAME:
        case PACKET_JOIN_GAME:
        case PACKET_GAME_STATE:
        case PACKET_PLAYER_HAND:
        case PACKET_CHAT:
        case PACKET_DISCONNECT:
        case PACKET_COLOR_CHOICE:
        case PACKET_MOVE:
        case PACKET_DRAW:
            HandleGamePacket(packet, gioco);
            break;

        /* --- Pacchetti admin/sessione --- */
        case PACKET_CREATE_SESSION:
        case PACKET_JOIN_SESSION:
        case PACKET_PLAYER_READY:
        case PACKET_GAME_FINISHED:
        case PACKET_HEARTBEAT:
        case PACKET_ADMIN_USER_SYNC:
        case PACKET_ADMIN_DB_SYNC:
        case PACKET_STATS_SYNC:
        case PACKET_PLAY_AGAIN:
        case PACKET_HOST_RETURN_LOBBY:
            HandleAdminPacket(packet, gioco);
            break;

        /* --- Pacchetti di rifiuto connessione --- */
        case PACKET_JOIN_REJECTED:
            HandleGamePacket(packet, gioco);
            break;

        default: break;
    }
}