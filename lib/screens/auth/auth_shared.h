/**
 * @file auth_shared.h
 * @brief Stato condiviso tra i moduli delle schermate di autenticazione.
 * @defgroup auth_shared Auth Shared
 * @brief Dichiara la struttura CampoInput e le variabili di stato
 * condivise tra auth_update.c, auth_draw.c e admin_draw.c.
 *
 * auth_screen.c definisce queste variabili, mentre i moduli separati
 * le includono come extern.
 */
#ifndef AUTH_SHARED_H
#define AUTH_SHARED_H

#include "raylib.h"

/** @brief Struttura per la gestione dei campi di input (area + ID). */
typedef struct {
    Rectangle area;
    int campo_id;
} CampoInput;

/** @brief Buffer password di recupero. */
extern char recPassword[30];
/** @brief Buffer username di recupero. */
extern char recUsername[30];
/** @brief Flag recupero password completato. */
extern int recupero_completato;
/** @brief Flag errore recupero password. */
extern int errore_recupero;

/** @brief Flag modifica admin in corso. */
extern int utente_modifica_admin;

/** @brief Flag mostra popup uscita nelle schermate auth. */
extern int mostra_popup_uscita_auth;

/** @brief Flag nuovo utente (submit). */
extern int nuovo_utente;

#endif