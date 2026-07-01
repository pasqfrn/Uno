/**
 * @file admin_draw.h
 * @brief Rendering del pannello admin.
 * @defgroup admin_draw Admin Draw
 * @brief Funzioni di rendering per il pannello admin: visualizzazione
 * e modifica utenti.
 */
#ifndef ADMIN_DRAW_H
#define ADMIN_DRAW_H

#include "../../data_structures.h"

/**
 * @brief Disegna il pannello admin con la lista utenti.
 * @pre fase != NULL.
 * @post Lista utenti renderizzata; azioni admin gestite.
 * @param fase Puntatore alla fase corrente.
 */
void DisegnaAdminPanel(FaseApplicazione* fase);

/**
 * @brief Disegna la schermata di modifica utente admin.
 * @pre fase != NULL.
 * @post Schermata modifica renderizzata.
 * @param fase Puntatore alla fase corrente.
 */
void DisegnaAdminModificaUtente(FaseApplicazione* fase);

#endif /* ADMIN_DRAW_H */
