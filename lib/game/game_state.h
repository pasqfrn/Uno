/**  
 * @file game_state.h  
 * @brief Modulo StatoGioco - ADT Stato di gioco (tipo opaco).  
 * @defgroup game_state Stato Gioco  
 * @brief Definisce l'ADT per lo stato completo di una partita UNO in corso.  
 *  
 * PRINCIPIO DI INFORMATION HIDING:  
 * Il tipo `StatoGioco` è dichiarato come tipo OPACO: la sua rappresentazione  
 * interna è definita esclusivamente in `lib/game/game_state_private.h`.  
 * Gli utilizzatori accedono ai dati solo tramite le funzioni di dominio  
 * in `lib/game/game_logic.h` e le funzioni di serializzazione.  
 *  
 * FUNZIONALITÀ DI DOMINIO (non meno di 2):  
 *   1. Gestione mazzo di pesca e pila scarti (tramite ADT Pila)  
 *   2. Gestione mano giocatori (tramite ADT ListaCarte)  
 *   3. Avanzamento turni (tramite ADT CodaTurni)  
 *   4. Validazione mosse e applicazione effetti carte  
 *  
 * Riferimento: Documentazione CdS - "Gestione stato partita"  
 */  

#ifndef GAME_STATE_H  
#define GAME_STATE_H  

#include "../data_structures/data_structures.h"  
#include "player.h"  

/* ============================================================  
 *  STATO GIOCO - Tipo opaco (information hiding)  
 * ============================================================ */  

/**  
 * @brief Stato completo di una partita in corso (tipo opaco).  
 *  
 * La rappresentazione interna è definita esclusivamente in  
 * `lib/game/game_state_private.h`. Gli utilizzatori NON devono  
 * mai accedere direttamente ai campi della struct.  
 *  
 * Per manipolare lo stato si usano le funzioni di dominio in  
 * `lib/game/game_logic.h`.  
 */  
typedef struct StatoGioco StatoGioco;  

#endif /* GAME_STATE_H */  
