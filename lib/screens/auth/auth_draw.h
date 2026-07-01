/**
 * @file auth_draw.h
 * @brief Rendering delle schermate di autenticazione.
 * @defgroup auth_draw Auth Draw
 * @brief Disegna tutte le schermate di autenticazione: login, registrazione,
 * recupero password, profilo, modifica profilo, pannello admin e modifica utente.
 */
#ifndef AUTH_DRAW_H
#define AUTH_DRAW_H

#include "../../data_structures.h"

/**
 * @brief Disegna la schermata di autenticazione corrente.
 * @pre fase != NULL.
 * @post Scena renderizzata; 1 se richiesta uscita, 0 altrimenti.
 * @param fase Puntatore alla fase corrente.
 * @return 1 se l'utente ha confermato l'uscita, 0 altrimenti.
 */
int DisegnaAuth(FaseApplicazione* fase);

#endif