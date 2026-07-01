/**
 * @file gameplay_animation.h
 * @brief Gestione delle animazioni di gioco.
 * @defgroup gameplay_animation Gameplay Animation
 * @brief Funzioni per l'aggiornamento e il rendering delle animazioni
 * durante il gameplay (spostamento carte, effetti visivi, etc.).
 */
#ifndef GAMEPLAY_ANIMATION_H
#define GAMEPLAY_ANIMATION_H

#include "../../data_structures.h"

/**
 * @brief Aggiorna lo stato delle animazioni in corso.
 * @pre gioco != NULL.
 * @post Animazioni avanzate nel tempo.
 * @param gioco Stato del gioco.
 * @param dt Delta time.
 */
void AggiornaAnimazioni(StatoGioco* gioco, float dt);

/**
 * @brief Disegna le animazioni attive sullo schermo.
 * @pre gioco != NULL.
 * @post Animazioni renderizzate.
 * @param gioco Stato del gioco.
 */
void DisegnaAnimazioni(StatoGioco* gioco);

#endif
