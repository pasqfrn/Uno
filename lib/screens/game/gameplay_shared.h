/**
 * @file gameplay_shared.h
 * @brief Stato condiviso tra i moduli della schermata di gioco.
 * @defgroup gameplay_shared Gameplay Shared
 * @brief Variabili globali condivise e funzioni helper tra gameplay_update.c,
 * gameplay_draw.c, chat_render.c e bot_ai.c.
 */
#ifndef GAMEPLAY_SHARED_H
#define GAMEPLAY_SHARED_H

#include "../../data_structures.h"
#include "../../player.h"
#include "../../game_state.h"
#include "raylib.h"

/* Timer per il ritardo delle mosse dei bot */
extern float botTimer;

/* Offset del mouse per il trascinamento carte */
extern Vector2 offsetMouse;

/* Area del tavolo centrale per il drop delle carte */
extern Rectangle areaTavoloCentro;

/* Indice della mossa in sospeso (per scelta colore) */
extern int pending_move_index;

/* Buffer chat e stato di digitazione */
extern char inputChat[128];
extern int isTyping;
extern int popup_uscita_attivo;

/**
 * Confronto case-insensitive di un prefisso.
 * Restituisce 1 se inizia con il prefisso specificato.
 */
int StartsWithIgnoreCase(const char* str, const char* prefix);

/**
 * Disegna un messaggio chat evidenziando i tag @NomeGiocatore.
 * Tag diretto a mio_id: giallo (YELLOW)
 * Tag diretto ad altro giocatore: azzurro (CYAN chiaro)
 * Resto del testo: colore di default (LIGHTGRAY)
 */
void DrawChatMsgSegmentato(const char* msg, int x, int y, int fontSize,
                            StatoGioco* gioco, int mio_id);

#endif