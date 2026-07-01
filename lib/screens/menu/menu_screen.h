/**
 * @file menu_screen.h
 * @brief Schermata del menu principale e statistiche.
 * @defgroup menu_screen Menu Screen
 * @brief Disegna il menu principale e le schermate annesse (statistiche).
 */
#ifndef MENU_SCREEN_H
#define MENU_SCREEN_H

#include "../../data_structures.h"

/**
 * @brief Disegna il menu principale e gestisce la navigazione.
 * @pre gioco != NULL; fase != NULL.
 * @post Menu renderizzato; fase aggiornata su interazione.
 * @param gioco Puntatore allo stato di gioco.
 * @param fase Puntatore alla fase corrente.
 */
void DisegnaMenu(StatoGioco* gioco, FaseApplicazione* fase);

#endif
