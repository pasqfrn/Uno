/**
 * @file lan_save_helpers.h
 * @brief Header per gli helper di sincronizzazione salvataggi LAN.
 *
 * Dichiara le funzioni helper fattorizzate da lan_save_sync.c:
 *   - GetSlotPath: costruisce path file nel formato save_user{ID}_slot_{N}.dat
 *   - GetSlotPathForUser: path per un dato ID utente
 *   - GetSlotPathForUsername: path per dato username (scansiona ID)
 *   - TrovaRecv: cerca o alloca un ricevitore di chunk
 *
 * Fattorizzato per ridurre la complessita' di lan_save_sync.c (>450 righe).
 */
#ifndef LAN_SAVE_HELPERS_H
#define LAN_SAVE_HELPERS_H

#include <stddef.h>

/**
 * @brief Costruisce il path del file di salvataggio nel nuovo formato
 *        "save_user{ID}_slot_{N}.dat" (coerente con save_manager.c).
 * @param buffer Buffer di destinazione.
 * @param bufsize Dimensione del buffer.
 * @param slot Indice dello slot (1-100).
 */
void GetSlotPath(char* buffer, size_t bufsize, int slot);

/**
 * @brief Costruisce il path del file di salvataggio per un utente specifico
 *        con un dato ID utente.
 * @param buffer Buffer di destinazione.
 * @param bufsize Dimensione del buffer.
 * @param uid ID dell'utente nel database.
 * @param slot Indice dello slot (1-100).
 */
void GetSlotPathForUser(char* buffer, size_t bufsize, int uid, int slot);

/**
 * @brief Costruisce il path del file di salvataggio per username.
 * @param buffer Buffer di destinazione.
 * @param bufsize Dimensione del buffer.
 * @param username Username del proprietario del salvataggio.
 * @param slot Indice dello slot (1-100).
 * @return 1 se trovato, 0 altrimenti.
 */
int GetSlotPathForUsername(char* buffer, size_t bufsize, const char* username, int slot);

/**
 * @brief Cerca o alloca un ricevitore per owner/data.
 * @return Indice nell'array recvs, -1 se pieno.
 */
int TrovaRecv(const char* owner, const char* data);

#endif /* LAN_SAVE_HELPERS_H */