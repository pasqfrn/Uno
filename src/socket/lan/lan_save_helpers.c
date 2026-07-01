/**
 * @file lan_save_helpers.c
 * @brief Helper per la sincronizzazione salvataggi partite via LAN.
 * @ingroup lan
 *
 * Modulo separato da lan_save_sync.c (527 righe) per ridurre la complessita'.
 * Contiene:
 *   - GetSlotPath: costruisce path file nel formato save_user{ID}_slot_{N}.dat
 *   - GetSlotPathForUser: path per un dato ID utente
 *   - GetSlotPathForUsername: path per dato username (scansiona ID)
 *   - TrovaRecv: cerca o alloca un ricevitore di chunk
 *
 * ADT utilizzato: SaveReceiver[MAX_RECV] per accumulo chunk.
 *
 * BUG FIX 4: Tutti i riferimenti al vecchio "save_slot_N.dat" sono stati migrati
 * al nuovo formato "save_user{ID}_slot_N.dat" per coerenza con save_manager.c.
 *
 * Fattorizzato da lan_save_sync.c per modularizzazione (file >450 righe).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../../lib/auth.h"
#include "../../../lib/socket/lan/lan_protocol.h"

/* ==================================================================
 *  HELPERS PER PERCORSO FILE SALVATAGGIO
 *  Formato: data/games/save_user<ID>_slot_<N>.dat
 *  Centralizzato per evitare inconsistenze di path tra i moduli.
 * ================================================================== */

/**
 * @brief Costruisce il path del file di salvataggio nel nuovo formato
 *        "save_user{ID}_slot_{N}.dat" (coerente con save_manager.c).
 * @param buffer Buffer di destinazione.
 * @param bufsize Dimensione del buffer.
 * @param slot Indice dello slot (1-100).
 */
void GetSlotPath(char* buffer, size_t bufsize, int slot) {
    snprintf(buffer, bufsize, "data/games/save_user%d_slot_%d.dat",
             id_utente_corrente, slot);
}

/**
 * @brief Costruisce il path del file di salvataggio per un utente specifico
 *        con un dato ID utente (usato per compatibilita' con salvataggi di
 *        altri utenti ricevuti via LAN).
 * @param buffer Buffer di destinazione.
 * @param bufsize Dimensione del buffer.
 * @param uid ID dell'utente nel database.
 * @param slot Indice dello slot (1-100).
 */
void GetSlotPathForUser(char* buffer, size_t bufsize, int uid, int slot) {
    snprintf(buffer, bufsize, "data/games/save_user%d_slot_%d.dat", uid, slot);
}

/**
 * @brief Costruisce il path del file di salvataggio per username
 *        (scansiona tutti gli ID utente per trovare quello corrispondente).
 * @param buffer Buffer di destinazione.
 * @param bufsize Dimensione del buffer.
 * @param username Username del proprietario del salvataggio.
 * @param slot Indice dello slot (1-100).
 * @return 1 se trovato, 0 altrimenti.
 */
int GetSlotPathForUsername(char* buffer, size_t bufsize, const char* username, int slot) {
    int uid = -1;
    for (int i = 0; i < dbUtenti.num_utenti; i++) {
        if (strcmp(dbUtenti.lista[i].username, username) == 0) {
            uid = i;
            break;
        }
    }
    if (uid < 0) {
        // Username non trovato nel DB: usa formato generico con username come id
        snprintf(buffer, bufsize, "data/games/save_user_%s_slot_%d.dat", username, slot);
        return 0;
    }
    GetSlotPathForUser(buffer, bufsize, uid, slot);
    return 1;
}

/* ==================================================================
 *  GESTIONE RICEVITORI DI CHUNK (SaveReceiver array)
 *  ADT: array lineare di ricevitori, ricerca per chiave owner|data.
 * ================================================================== */

#define MAX_RECV 10

/** @brief Array dei ricevitori di chunk salvataggio (definito in lan_save_sync.c). */
extern SaveReceiver recv[MAX_RECV];

/**
 * @brief Cerca o alloca un ricevitore per owner/data.
 * @return Indice nell'array recv, -1 se pieno.
 */
int TrovaRecv(const char* owner, const char* data) {
    char key[70];
    snprintf(key, sizeof(key), "%s|%s", owner, data);
    for (int i = 0; i < MAX_RECV; i++)
        if (recv[i].attivo && strcmp(recv[i].key, key) == 0) return i;
    for (int i = 0; i < MAX_RECV; i++) {
        if (!recv[i].attivo) {
            memset(&recv[i], 0, sizeof(SaveReceiver));
            strncpy(recv[i].key, key, sizeof(recv[i].key) - 1);
            recv[i].attivo = 1;
            return i;
        }
    }
    return -1;
}
