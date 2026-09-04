/**
 * @file bot_ai.h
 * @brief Albero decisionale per l'IA dei bot.
 * @defgroup bot_ai Bot AI
 * @brief Implementazione di un albero decisionale per la strategia
 * dell'IA dei bot UNO.
 *
 * Priorità IA (albero decisionale):
 * 1. Gioca Jolly+4 (se poche carte in mano)
 * 2. Gioca +2
 * 3. Gioca Cambio Colore
 * 4. Gioca Cambio Giro / Salta
 * 5. Gioca carta numerata più alta
 * 6. Se nessuna mossa: pesca
 */
#ifndef BOT_AI_H
#define BOT_AI_H

#include "../../data_structures/data_structures.h"
#include "../../game/player.h"
#include "../../game/game_state.h"

/**
 * @addtogroup bot_ai_azioni Azioni Bot
 * @brief Enumerazione delle possibili decisioni del bot.
 * @{
 */
/** @brief Gioca Jolly+4 (priorità max). */
typedef enum { AZIONE_GIOCA_JOLLY_PIU4, AZIONE_GIOCA_PIU2, AZIONE_GIOCA_CAMBIO_COLORE,
               AZIONE_GIOCA_CAMBIO_GIRO, AZIONE_GIOCA_SALTA, AZIONE_GIOCA_NUMERO_ALTO,
               AZIONE_GIOCA_NUMERO_BASSO, AZIONE_PESCA } DecisioneBot;
/** @} */

/**
 * @addtogroup bot_ai_nodo Nodo Albero Decisionale
 * @brief Nodo dell'albero decisionale binario.
 * @{
 */
/** @brief Nodo albero: azione, priorità, figli si/no. */
typedef struct NodoDecisione {
    DecisioneBot azione;
    int priorita;
    struct NodoDecisione* si;
    struct NodoDecisione* no;
} NodoDecisione;
/** @} */

/**
 * @addtogroup bot_ai_api API Bot AI
 * @brief Funzioni per costruire, valutare e distruggere l'albero decisionale.
 * @{
 */
/**
 * @brief Costruisce l'albero decisionale per l'IA del bot.
 * @post Albero allocato con priorità definite.
 * @return Radice dell'albero decisionale.
 */
NodoDecisione* BotAI_CostruisciAlbero(void);

/**
 * @brief Distrugge l'albero decisionale.
 * @pre radice != NULL.
 * @post Memoria dell'albero completamente liberata.
 * @param radice Radice dell'albero.
 */
void BotAI_DistruggiAlbero(NodoDecisione* radice);

/**
 * @brief Valuta l'albero decisionale e restituisce l'azione.
 * @pre radice e gioco validi; id_giocatore valido.
 * @return DecisioneBot presa.
 * @param radice Radice dell'albero.
 * @param gioco Stato del gioco.
 * @param id_giocatore ID del giocatore bot.
 */
DecisioneBot BotAI_Decidi(NodoDecisione* radice, StatoGioco* gioco, int id_giocatore);

/**
 * @brief Trova l'indice di una carta da giocare per la decisione presa.
 * @pre gioco valido; DecisioneBot valida.
 * @return Indice carta, o -1 se non disponibile.
 * @param gioco Stato del gioco.
 * @param id_giocatore ID del giocatore.
 * @param azione Decisione da eseguire.
 */
int BotAI_TrovaCarta(StatoGioco* gioco, int id_giocatore, DecisioneBot azione);

/**
 * @brief Sceglie il colore migliore per un Jolly in base alla mano.
 * @pre gioco valido.
 * @return Colore con più carte nella mano del bot.
 * @param gioco Stato del gioco.
 * @param id_giocatore ID del giocatore bot.
 */
Colore BotAI_ScegliColore(StatoGioco* gioco, int id_giocatore);
/** @} */

#endif // BOT_AI_H
