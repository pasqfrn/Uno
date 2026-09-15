#ifndef PLAYER_H
#define PLAYER_H

#include "../data_structures/data_structures.h"
#include "../data_structures/list.h"

/* ============================================================
 *  GIOCATORE - Tipo opaco (information hiding)
 * ============================================================ */

/** @brief Rappresenta un giocatore nella partita (umano o bot) - tipo opaco. */
typedef struct Giocatore Giocatore;

/* ============================================================
 *  API PUBBLICA - Funzioni di accesso al giocatore
 * ============================================================ */

/** @brief Restituisce il nome del giocatore (o "Sconosciuto" se NULL/empty). */
const char* Giocatore_Nome(const Giocatore* g);

/** @brief Restituisce il nome del giocatore (alias per compatibilità). */
const char* Giocatore_GetNome(const Giocatore* g);

/** @brief Imposta il nome del giocatore. */
void Giocatore_SetNome(Giocatore* g, const char* nome);

/** @brief Restituisce il nome con suffisso "(Bot)" se è un bot; altrimenti il nome normale. */
const char* Giocatore_NomeBot(const Giocatore* g);

/** @brief Restituisce il nome con suffisso "(Bot)" (alias per compatibilità). */
const char* Giocatore_GetNomeBot(const Giocatore* g);

/** @brief Restituisce se il giocatore è un bot. */
int Giocatore_IsBot(const Giocatore* g);

/** @brief Imposta lo stato di pronto nella lobby. */
void Giocatore_SetPronto(Giocatore* g, int pronto);

/** @brief Restituisce lo stato di pronto. */
int Giocatore_IsPronto(const Giocatore* g);

/** @brief Restituisce la mano del giocatore (ADT ListaCarte). */
ListaCarte* Giocatore_GetMano(const Giocatore* g);

/** @brief Imposta la mano del giocatore. */
void Giocatore_SetMano(Giocatore* g, ListaCarte* mano);

/** @brief Restituisce il numero di carte in mano. */
int Giocatore_GetNumCarte(const Giocatore* g);

/** @brief Imposta il numero di carte in mano. */
void Giocatore_SetNumCarte(Giocatore* g, int num);

/** @brief Restituisce l'ID socket del giocatore. */
int Giocatore_GetSocketId(const Giocatore* g);

/** @brief Imposta l'ID socket del giocatore. */
void Giocatore_SetSocketId(Giocatore* g, int socket_id);

/** @brief Restituisce se il giocatore era in prelobby. */
int Giocatore_EraInPrelobby(const Giocatore* g);

/** @brief Imposta lo stato di prelobby. */
void Giocatore_SetEraInPrelobby(Giocatore* g, int era);

/** @brief Esegue la copia di sicurezza (backup) della mano per serializzazione. */
int Giocatore_FaiBackupMano(Giocatore* g);

/** @brief Restituisce il backup della mano (array di carte). */
int Giocatore_GetBackupMano(const Giocatore* g, Carta* out, int max);

/** @brief Restituisce il numero di carte nel backup. */
int Giocatore_GetNumCarteBackup(const Giocatore* g);

#endif /* PLAYER_H */
