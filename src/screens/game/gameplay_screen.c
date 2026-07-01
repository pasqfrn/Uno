/**
 * @file gameplay_screen.c
 * @brief Definizioni variabili di stato condivise del gameplay.
 * @ingroup gameplay_screen
 *
 * Contiene le definizioni (non le dichiarazioni) delle variabili globali
 * usate da gameplay_update.c e gameplay_draw.c.
 *
 * Fattorizzato da gameplay_screen.c (659 righe) in:
 *   - gameplay_shared.h  : dichiarazioni extern
 *   - gameplay_update.c  : logica aggiornamento
 *   - gameplay_draw.c    : rendering
 *   - gameplay_screen.c  : questo file (definizioni variabili)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "raylib.h"

#include "../../../lib/screens/game/gameplay_shared.h"
#include "../../../lib/screens/game/gameplay_update.h"
#include "../../../lib/screens/game/gameplay_draw.h"
#include "../../../lib/screens/menu/string_utils.h"
#include "../../../lib/game_logic.h"
#include "../../../lib/ui.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/auth.h"

#define LARGHEZZA 1280
#define ALTEZZA 720

/* ============================================================
 *  DEFINIZIONI VARIABILI DI STATO CONDIVISE GAMEPLAY
 * ============================================================
 *  Questo file contiene le DEFINIZIONI (allocazione memoria)
 *  delle variabili globali condivise tra gameplay_update.c
 *  e gameplay_draw.c.
 *
 *  Fattorizzato da gameplay_screen.c (659 righe) in:
 *    - gameplay_shared.h  : dichiarazioni extern
 *    - gameplay_update.c  : logica aggiornamento
 *    - gameplay_draw.c    : rendering
 *    - gameplay_screen.c  : questo file (definizioni variabili)
 */

/** @brief Timer per il ritardo delle mosse dei bot (in secondi). */
float botTimer = 0.0f;

/** @brief Offset del mouse per il trascinamento delle carte (differenza tra
 *         posizione mouse e angolo superiore sinistro della carta). */
Vector2 offsetMouse = {0};

/** @brief Area del tavolo centrale per il drop delle carte (centro schermo). */
Rectangle areaTavoloCentro = { LARGHEZZA/2.0f - 110, ALTEZZA/2.0f - 110, 220, 220 };

/** @brief Indice della mossa in sospeso (per scelta colore dopo jolly).
 *         -1 = nessuna mossa in sospeso, altrimenti indice della carta. */
int pending_move_index = -1;

/** @brief Buffer per il messaggio chat in digitazione. */
char inputChat[128] = "\0";
/** @brief Flag: 1 se l'utente sta digitando nella chat. */
int isTyping = 0;
/** @brief Flag: 1 se il popup di conferma uscita e' visibile durante il gioco. */
int popup_uscita_attivo = 0;

/* Variabile esterna dal main: indica se il tasto ESC e' stato gia' gestito. */
extern int esc_consumato;

/* ============================================================
 *  NOTA ARCHITETTURALE
 * ============================================================
 *  Le funzioni AggiornaGameplay e DisegnaGameplay sono definite
 *  rispettivamente in gameplay_update.c e gameplay_draw.c.
 *  Questo file contiene ESCLUSIVAMENTE le definizioni
 *  delle variabili di stato condivise tra i moduli.
 */
