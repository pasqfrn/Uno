/**
 * @file stats_screen.h
 * @brief Schermata delle statistiche utente.
 * @defgroup stats_screen Stats Screen
 * @brief Schermata avanzata per la visualizzazione delle statistiche
 * del giocatore (partite giocate, vittorie, storico partite).
 */
#ifndef STATS_SCREEN_H
#define STATS_SCREEN_H

#include "../../data_structures/data_structures.h"

/**
 * @brief Disegna la schermata delle statistiche.
 * @pre fase != NULL.
 * @post Statistiche renderizzate; navigazione gestita.
 * @param fase Puntatore alla fase corrente.
 */
void DisegnaStatistiche(FaseApplicazione* fase);

#endif
