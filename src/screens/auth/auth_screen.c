/**
 * @file auth_screen.c
 * @brief Definizioni delle variabili di stato condivise per le schermate auth.
 * @ingroup auth_screen
 *
 * Contiene le definizioni (non le dichiarazioni) delle variabili globali
 * usate da auth_update.c e auth_draw.c.
 *
 * Fattorizzato da auth_screen.c (769 righe) in:
 *   - auth_shared.h  : dichiarazioni extern
 *   - auth_update.c  : logica di aggiornamento (AggiornaAuth)
 *   - auth_draw.c    : rendering (DisegnaAuth)
 *   - admin_draw.c   : rendering pannello admin
 *   - auth_screen.c  : questo file (definizioni variabili)
 */
#include <string.h>
#include "raylib.h"

#include "../../../lib/screens/auth/auth_shared.h"
#include "../../../lib/screens/auth/auth_update.h"
#include "../../../lib/screens/auth/auth_draw.h"
#include "../../../lib/screens/auth/input_field.h"
#include "../../../lib/screens/menu/string_utils.h"
#include "../../../lib/auth.h"
#include "../../../lib/ui.h"

#define LARGHEZZA 1280
#define ALTEZZA 720

/* ============================================================
 *  DEFINIZIONI VARIABILI DI STATO CONDIVISE
 * ============================================================
 *  Questo file contiene le DEFINIZIONI (allocazione memoria)
 *  delle variabili globali condivise tra i moduli auth.
 *  Le dichiarazioni extern sono in auth_shared.h.
 */

/** @brief Flag: 1 se un nuovo utente e' stato appena registrato (mostra "Benvenuto"). */
int nuovo_utente = 0;

/** @brief Buffer per la password di recupero (usato nel flusso recupero credenziali). */
char recPassword[30] = "";
/** @brief Buffer per lo username di recupero (usato nel flusso recupero credenziali). */
char recUsername[30] = "";
/** @brief Stato recupero password:
 *        0 = in attesa di inserimento email/PIN,
 *        2 = PIN verificato, in attesa nuova password,
 *        3 = password reimpostata con successo. */
int recupero_completato = 0;
/** @brief Flag errore recupero password:
 *        0 = nessun errore,
 *        1 = email obbligatoria,
 *        2 = PIN obbligatorio,
 *        3 = email non valida (manca @),
 *        4 = email o PIN errati. */
int errore_recupero = 0;

/** @brief Indice dell'utente in modifica nel pannello admin (-1 = nessuno).
 *  Se == dbUtenti.num_utenti, indica che si sta CREANDO un nuovo utente
 *  (valore sentinella). */
int utente_modifica_admin = -1;

/** @brief Flag: 1 se il popup di conferma uscita e' visibile nella schermata auth. */
int mostra_popup_uscita_auth = 0;

/* Variabile esterna dal main: indica se il tasto ESC e' stato gia' gestito. */
extern int esc_consumato;

/* ============================================================
 *  NOTA ARCHITETTURALE
 * ============================================================
 *  Le funzioni AggiornaAuth e DisegnaAuth sono definite
 *  rispettivamente in auth_update.c e auth_draw.c.
 *  Questo file contiene ESCLUSIVAMENTE le definizioni
 *  delle variabili di stato condivise tra i moduli.
 */
