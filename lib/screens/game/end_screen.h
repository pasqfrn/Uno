/**
 * @file end_screen.h
 * @brief Schermata di fine partita con classifica e statistiche.
 * @defgroup end_screen End Screen
 * @brief Calcola la classifica finale, esporta statistiche su file .txt
 * e disegna la schermata di fine partita.
 */
#ifndef END_SCREEN_H
#define END_SCREEN_H

#include "../../data_structures/data_structures.h"
#include "../../game/player.h"
#include "../../game/game_state.h"
#include "raylib.h"

/**
 * @brief Calcola e ordina la classifica finale dei giocatori.
 * @pre gioco != NULL; classifica array valido.
 * @post classifica popolata e ordinata per punteggio; num_in_classifica aggiornato.
 * @param gioco Stato del gioco.
 * @param classifica Array di ClassificaRecord da riempire.
 * @param num_in_classifica Puntatore al numero di giocatori in classifica.
 */
void CalcolaClassifica(StatoGioco* gioco, ClassificaRecord classifica[4], int* num_in_classifica);

/**
 * @brief Esporta le statistiche di fine partita su file .txt.
 * @pre gioco != NULL; esito != NULL.
 * @post File .txt creato con statistiche della partita.
 * @param gioco Stato del gioco.
 * @param esito Stringa esito ("Vittoria" o "Sconfitta").
 */
void EsportaStatisticheTxt(StatoGioco* gioco, const char* esito);

/**
 * @brief Disegna la schermata di fine partita con classifica.
 * @pre gioco != NULL; fase != NULL.
 * @post Schermata fine partita renderizzata.
 * @param gioco Stato del gioco.
 * @param fase Fase corrente.
 * @param mousePos Posizione del mouse.
 */
void DisegnaFinePartita(StatoGioco* gioco, FaseApplicazione* fase, Vector2 mousePos);

#endif
