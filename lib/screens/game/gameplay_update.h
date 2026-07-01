/**
 * @file gameplay_update.h
 * @brief Logica di aggiornamento del gameplay.
 * @defgroup gameplay_update Gameplay Update
 * @brief Funzioni di aggiornamento dello stato di gioco: gestione input,
 * animazioni, turni bot, drag-and-drop delle carte e scelta colore.
 */
#ifndef GAMEPLAY_UPDATE_H
#define GAMEPLAY_UPDATE_H

#include "../../data_structures.h"
#include "raylib.h"

/**
 * @brief Calcola la posizione a schermo di un giocatore in base all'indice.
 * @pre num_giocatori > 0; index_giocatore < num_giocatori.
 * @return posizione (x, y) del giocatore sullo schermo.
 * @param index_giocatore Indice del giocatore.
 * @param mio_id Indice del giocatore locale.
 * @param num_giocatori Numero totale di giocatori.
 */
Vector2 OttieniPosizioneGiocatore(int index_giocatore, int mio_id, int num_giocatori);

/**
 * @brief Aggiorna lo stato del gioco (main update del gameplay).
 * @pre gioco != NULL; fase != NULL.
 * @post Stato di gioco aggiornato per input, animazioni, bot, chat.
 * @param gioco Stato del gioco.
 * @param fase Fase corrente dell'applicazione.
 * @param mousePos Posizione del mouse.
 * @param dt Delta time.
 */
void AggiornaGameplay(StatoGioco* gioco, FaseApplicazione* fase, Vector2 mousePos, float dt);

#endif