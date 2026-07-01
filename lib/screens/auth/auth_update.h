/**
 * @file auth_update.h
 * @brief Logica di aggiornamento delle schermate di autenticazione.
 * @defgroup auth_update Auth Update
 * @brief Gestisce l'input utente e le transizioni tra le fasi di
 * login, registrazione, recupero password, modifica profilo e admin panel.
 */
#ifndef AUTH_UPDATE_H
#define AUTH_UPDATE_H

#include "../../data_structures.h"

/**
 * @brief Gestisce l'input e la logica delle schermate auth.
 * @pre fase != NULL.
 * @post Fase aggiornata in base all'input; eventi di autenticazione processati.
 * @param fase Puntatore alla fase corrente dell'applicazione.
 */
void AggiornaAuth(FaseApplicazione* fase);

#endif