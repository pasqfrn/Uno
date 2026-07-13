/**
 * @file data_structures.h
 * @brief Header centrale del progetto UNO - Tipi di dato condivisi.
 * @defgroup data_structures Tipi di Dato Condivisi
 * @brief Definizione dei tipi di dato fondamentali e condivisi del progetto UNO.
 *
 * Contiene solo i tipi base (Carta, Colore, TipoCarta, Utente, ecc.) che
 * sono utilizzati da MOLTI moduli. Le strutture dati specifiche degli ADT
 * (NodoCarta, ListaCarte, Giocatore, StatoGioco) sono state spostate nei
 * rispettivi header dei moduli per migliorare l'incapsulamento.
 *
 * Riferimento: Documentazione CdS - "Strutture dati e tipi (dati strutturati)"
 *
 * NOTA: Il database utenti e' ora allocato DINAMICAMENTE (malloc/realloc)
 * per garantire scalabilita'. La capacita' iniziale e' DB_CAPACITA_INIZIALE.
 */

#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include "raylib.h"
#include <stddef.h>  /* per size_t */

/** @brief Capacita' iniziale del database utenti (allocazione dinamica). */
#define DB_CAPACITA_INIZIALE 64

/** @brief Numero massimo di partite nello storico di ogni utente. */
#define MAX_PARTITE_STORICO 100

/* ============================================================
 *  COLORI DELLE CARTE (Enum)
 *  Riferimento: Documentazione CdS - "Gestione carta"
 * ============================================================ */

/** @brief Colori disponibili per le carte UNO. NERO = jolly/cambio colore. */
typedef enum { ROSSO, GIALLO, VERDE, BLU, NERO } Colore;

/* ============================================================
 *  TIPI DI CARTA (Enum)
 *  Riferimento: Documentazione CdS - "Gestione carta"
 * ============================================================ */

/** @brief Tipi di carta UNO. Da NUM_0 a NUM_9 = carte normali, poi speciali. */
typedef enum {
    NUM_0, NUM_1, NUM_2, NUM_3, NUM_4, NUM_5, NUM_6, NUM_7, NUM_8, NUM_9,
    SALTA,           /* Salta il prossimo giocatore */
    CAMBIA_GIRO,     /* Inverte la direzione del turno */
    PESCA_DUE,       /* Il prossimo giocatore pesca 2 carte */
    CAMBIO_COLORE,   /* Jolly: scegli un colore */
    PESCA_QUATTRO    /* Jolly +4: il prossimo pesca 4 carte e scegli un colore */
} TipoCarta;

/* ============================================================
 *  CARTA - Entita' di dominio fondamentale
 *  Riferimento: Documentazione CdS - "Gestione carta"
 * ============================================================ */

/**
 * @brief Rappresenta una singola carta UNO.
 *
 * Contiene il colore, il tipo, l'area di rendering (Rectangle),
 * la posizione originale (per animazioni di trascinamento) e
 * il flag di trascinamento.
 */
typedef struct {
    Colore colore;              /* Colore della carta (ROSSO, GIALLO, VERDE, BLU, NERO) */
    TipoCarta tipo;             /* Tipo della carta (NUM_0-NUM_9, SALTA, CAMBIA_GIRO, ecc.) */
    Rectangle area;             /* Rettangolo di rendering per Raylib */
    Vector2 posOriginale;       /* Posizione originale prima del trascinamento */
    int isTrascinata;           /* 1 se la carta e' in fase di trascinamento, 0 altrimenti */
} Carta;

/* ============================================================
 *  UTENTE - Entita' di dominio
 *  Riferimento: Documentazione CdS - "Gestione utente"
 * ============================================================ */

/**
 * @brief Rappresenta un utente registrato nel sistema.
 *
 * Contiene i dati anagrafici e le credenziali di accesso.
 * L'email funge da chiave primaria per il database.
 */
typedef struct {
    char nome[50];          /* Nome dell'utente */
    char cognome[50];       /* Cognome dell'utente */
    char username[30];      /* Username univoco per il login */
    char email[100];        /* Email (chiave primaria per il database) */
    char password[50];      /* Password (eventualmente cifrata con XOR) */
} Utente;

/* ============================================================
 *  PARTITA REGISTRATA - Storico di una singola partita
 * ============================================================ */

/**
 * @brief Record di una singola partita nello storico di un utente.
 *
 * Viene salvato nell'array storico_partite della struttura Statistiche.
 */
typedef struct {
    char modalita[20];      /* Modalita' di gioco (es. "1 vs 2", "Online") */
    char esito[15];         /* Esito della partita (es. "Vittoria", "Sconfitta") */
    int durata_secondi;     /* Durata della partita in secondi */
    int posizione;          /* Piazzamento finale (1 = vincitore, 2+ = perdenti) */
} PartitaRegistrata;

/* ============================================================
 *  STATISTICHE - Dati anagrafici + cronologia partite
 *  Riferimento: Documentazione CdS - "Gestione statistiche"
 * ============================================================ */

/**
 * @brief Statistiche complete di un utente registrato.
 *
 * Contiene tutti i dati dell'utente (denormalizzati per velocita'
 * di accesso) e lo storico delle partite giocate.
 * L'array storico_partite ha dimensione fissa MAX_PARTITE_STORICO.
 */
typedef struct {
    char email[100];                        /* Email dell'utente (chiave primaria) */
    char nome[30];                          /* Nome dell'utente */
    char cognome[30];                       /* Cognome dell'utente */
    char username[30];                      /* Username univoco */
    char password[30];                      /* Password (cifrata) */
    char pin_recupero[7];                   /* PIN di 6 cifre per recupero password */
    int partite_giocate;                    /* Contatore totale partite giocate */
    int vittorie_giocatore;                 /* Contatore vittorie */
    int vittorie_bot;                       /* Contatore sconfitte (perdite contro bot) */
    PartitaRegistrata storico_partite[MAX_PARTITE_STORICO]; /* Cronologia partite */
} Statistiche;

/**
 * @brief Database degli utenti in memoria (allocazione DINAMICA).
 *
 * Il campo `lista` e' un puntatore a un array allocato con malloc/realloc.
 * La capacita' corrente e' `capacita`, il numero effettivo di utenti e' `num_utenti`.
 * Quando num_utenti raggiunge capacita, l'array viene riallocato con DB_CAPACITA_INIZIALE
 * nuovi slot.
 */
typedef struct {
    Statistiche* lista;     /* Array DINAMICO degli utenti registrati (malloc/realloc) */
    int num_utenti;         /* Numero effettivo di utenti nel database */
    int capacita;           /* Capacita' corrente dell'array allocato */
} DatabaseUtenti;

/* ============================================================
 *  MESSAGGIO CHAT - Entita' di flusso multiplayer
 *  Riferimento: Documentazione CdS - "Gestione interazioni multiplayer"
 * ============================================================ */

/**
 * @brief Singolo messaggio nella chat multiplayer.
 *
 * Contiene mittente, testo e orario di invio.
 */
typedef struct {
    char mittente[30];  /* Username di chi ha inviato il messaggio */
    char testo[256];    /* Contenuto del messaggio */
    char orario[10];    /* Timestamp (es. "14:35") */
} MessaggioChat;

/* ============================================================
 *  CLASSIFICA RECORD - Record per la classifica fine partita
 * ============================================================ */

/**
 * @brief Record per la classifica visualizzata a fine partita.
 *
 * Contiene il nome del giocatore, il numero di carte rimaste
 * e la posizione finale (1 = vincitore).
 */
typedef struct {
    char nome[30];      /* Nome del giocatore */
    int num_carte;      /* Numero di carte rimaste in mano */
    int posizione;      /* Posizione finale (1 = vincitore) */
    int is_bot;         /* 1 se e' un bot, 0 se umano */
} ClassificaRecord;

/* ============================================================
 *  FASE APPLICAZIONE - Enum per lo stato macchina
 * ============================================================ */

/**
 * @brief Enum che rappresenta tutte le fasi dello stato macchina dell'applicazione.
 *
 * Ogni fase corrisponde a una schermata o modalita' dell'applicazione.
 * Le transizioni tra le fasi sono gestite dalle funzioni AggiornaAuth e AggiornaGameplay.
 */
typedef enum {
    FASE_SCELTA_ACCESSO,            /* Schermata iniziale: Login / Registrazione / Esci */
    FASE_REGISTRAZIONE,             /* Form di registrazione nuovo utente */
    FASE_LOGIN,                     /* Form di login */
    FASE_RECUPERO_PASSWORD,         /* Recupero password tramite PIN */
    FASE_PROFILO_VISUALIZZA,        /* Visualizzazione profilo utente */
    FASE_PROFILO_MODIFICA,          /* Modifica profilo utente */
    FASE_ADMIN_PANEL,               /* Pannello di controllo admin */
    FASE_ADMIN_MODIFICA_UTENTE,     /* Modifica/crea utente da admin */
    FASE_MENU,                      /* Menu principale dopo il login */
    FASE_GIOCO,                     /* Gameplay loop (tavolo verde) */
    FASE_FINE_PARTITA,              /* Schermata di fine partita con classifica */
    FASE_STATISTICHE,               /* Schermata statistiche e storico */
    FASE_MULTIPLAYER_SCELTA,        /* Scelta modalita' multiplayer (host/client) */
    FASE_HOST_LOBBY,                /* Lobby come host (in attesa di giocatori) */
    FASE_CLIENT_JOIN,               /* Join a una stanza multiplayer */
    FASE_CLIENT_LOBBY               /* Lobby come client (in attesa di avvio) */
} FaseApplicazione;

#endif