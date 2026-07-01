/**
 * @file state_sync.h
 * @brief Sincronizzazione stato di gioco da pacchetti di rete.
 * @defgroup state_sync State Sync
 * @brief Funzione per sincronizzare lo stato locale con lo stato ricevuto via rete.
 */
#ifndef STATE_SYNC_H
#define STATE_SYNC_H

#include "network.h"
#include "../../game_logic.h"

/**
 * @brief Sincronizza lo stato di gioco locale con quello remoto.
 * @pre local != NULL; remote != NULL.
 * @post Stato locale aggiornato con i dati remoti.
 * @param local Stato di gioco locale (da aggiornare).
 * @param remote Stato di gioco remoto (fonte).
 */
void SyncGameStateFromNetwork(StatoGioco* local, const StatoGioco* remote);

#endif
