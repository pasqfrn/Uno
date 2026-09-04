/**
 * @file lan_protocol.h
 * @brief Protocollo UDP per sincronizzazione LAN.
 * @defgroup lan_protocol Protocollo LAN
 * @brief Definizione dei magic number, strutture pacchetti e costanti
 * per il protocollo UDP di sincronizzazione LAN.
 */
#ifndef LAN_PROTOCOL_H
#define LAN_PROTOCOL_H

#include <stdint.h>
#include "../../data_structures/data_structures.h"

/** @brief Magic number per pacchetti database utenti. */
#define LAN_PACKET_DB        0x4C414E01
/** @brief Magic number per pacchetti di salvataggio partita. */
#define LAN_PACKET_SAVE      0x4C414E02
/** @brief Magic number per segnalazione eliminazione salvataggio. */
#define LAN_PACKET_DEL_SAVE  0x4C414E03
/** @brief Magic number per segnalazione eliminazione utente da admin. */
#define LAN_PACKET_DEL_USER  0x4C414E04
/** @brief Magic number per sincronizzazione completa statistiche (incluso storico partite). */
#define LAN_PACKET_STATS_SYNC 0x4C414E05

/** @brief Dimensione di ogni chunk di salvataggio in byte. */
#define SAVE_CHUNK_SIZE 1024

/** @brief Numero massimo di ricevitori contemporanei. */
#define MAX_RECV 10

/**
 * @brief Numero di partite storiche trasmesse via sync LAN (ultime N per compattezza).
 * Definito PRIMA di LanCompactUser perche' usato nel campo storico_partite[].
 */
#define STORICO_SYNC_COUNT 10

/**
 * @brief Versione compatta di Statistiche per trasmissione via UDP.
 * @defgroup lan_protocol_user Dati Utente LAN
 * @brief Struttura compatta per sincronizzazione utenti via UDP.
 *
 * BUG 2 FIX: Aggiunto campo storico_partite (ultime STORICO_SYNC_COUNT partite)
 * per garantire che la sincronizzazione LAN preservi lo storico partite completo,
 * non solo i contatori numerici.
 */
typedef struct {
    char email[100];
    char nome[30];
    char cognome[30];
    char username[30];
    char password[30];
    char pin_recupero[7];
    int partite_giocate;
    int vittorie_giocatore;
    int vittorie_bot;
    /** @brief Storico partite per sincronizzazione completa. */
    PartitaRegistrata storico_partite[STORICO_SYNC_COUNT];
    /** @brief Numero effettivo di partite storiche valide. */
    int storico_count;
} LanCompactUser;

/**
 * @brief Pacchetto per sincronizzazione COMPLETA delle statistiche (incluso storico partite).
 * @defgroup lan_protocol_stats Stats Sync Packet
 * @brief Inviato dopo ogni partita o quando un utente si connette come client.
 */
typedef struct {
    /** @brief magic = LAN_PACKET_STATS_SYNC */
    int magic;
    /** @brief Username del giocatore */
    char username[30];
    /** @brief Totale partite giocate */
    int partite_giocate;
    /** @brief Totale vittorie */
    int vittorie_giocatore;
    /** @brief Totale sconfitte */
    int vittorie_bot;
    /** @brief Numero di partite storiche valide (0-STORICO_SYNC_COUNT) */
    int storico_count;
    /** @brief Ultime N partite */
    PartitaRegistrata storico_partite[STORICO_SYNC_COUNT];
} LanStatsSyncPacket;

/**
 * @brief Header comune a tutti i pacchetti database LAN.
 */
typedef struct {
    /** @brief Magic number per identificare il tipo pacchetto. */
    int magic;
    /** @brief Lunghezza totale del pacchetto. */
    int length;
} LanPacketHeader;

/**
 * @brief Pacchetto per invio salvataggio in chunk multipli.
 */
typedef struct __attribute__((packed)) {
    /** @brief Magic number. */
    int magic;
    /** @brief Username del proprietario. */
    char owner_username[30];
    /** @brief Data del salvataggio. */
    char data_salvataggio[30];
    /** @brief Slot di salvataggio. */
    int slot;
    /** @brief Numero di giocatori nella partita. */
    int num_giocatori;
    /** @brief Numero totale di chunk. */
    int total_chunks;
    /** @brief Indice di questo chunk. */
    int chunk_index;
    /** @brief Dimensione totale del file. */
    int file_size;
    /** @brief Dati del chunk (SAVE_CHUNK_SIZE byte). */
    unsigned char data[SAVE_CHUNK_SIZE];
} LanSaveChunkPacket;

/**
 * @brief Pacchetto per segnalazione eliminazione salvataggio.
 */
typedef struct {
    /** @brief Magic number. */
    int magic;
    /** @brief Username del proprietario. */
    char owner_username[30];
    /** @brief Data del salvataggio. */
    char data_salvataggio[30];
} LanDeleteSavePacket;

/**
 * @brief Struttura per ricezione chunk di salvataggio.
 */
typedef struct {
    /** @brief Flag di attività (1 se ricezione in corso). */
    int attivo;
    /** @brief Chiave identificativa (username + data). */
    char key[70];
    /** @brief Numero totale di chunk attesi. */
    int total;
    /** @brief Numero di chunk ricevuti finora. */
    int ricevuti;
    /** @brief Dimensione totale del file. */
    int file_size;
    /** @brief Buffer per i dati ricevuti (allocato dinamicamente). */
    unsigned char* filedata;
    /** @brief Timer per timeout ricezione. */
    float timer;
} SaveReceiver;

#endif /* LAN_PROTOCOL_H */