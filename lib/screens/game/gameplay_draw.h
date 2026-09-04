/**
 * @file gameplay_draw.h
 * @brief Rendering della schermata di gioco.
 * @defgroup gameplay_draw Gameplay Draw
 * @brief Contiene la funzione DisegnaGameplay che disegna l'intera
 * schermata di gioco: tavolo, carte scartate, mano, info giocatori,
 * chat, popup uscita e scelta colore.
 */
#ifndef GAMEPLAY_DRAW_H
#define GAMEPLAY_DRAW_H

#include "../../data_structures/data_structures.h"
#include "../../game/player.h"
#include "../../game/game_state.h"
#include "raylib.h"

/**
 * @brief Disegna la schermata di gioco completa.
 * @pre gioco != NULL; fase != NULL.
 * @post Scena di gioco renderizzata.
 * @param gioco Stato del gioco.
 * @param fase Fase corrente.
 * @param mousePos Posizione del mouse.
 */
void DisegnaGameplay(StatoGioco* gioco, FaseApplicazione* fase, Vector2 mousePos);

#endif