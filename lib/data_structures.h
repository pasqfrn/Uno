/**
 * @file data_structures.h
 * @brief Header principale del progetto UNO - Definizione di tutti i tipi di dato.
 * @defgroup data_structures Tipi di Dato e Strutture
 * @brief Definizione centralizzata di tutti i tipi di dato, enumerazioni, strutture e typedef del progetto UNO.
 *
 * Questo header costituisce il dominio applicativo: contiene entità come Carta, Utente, Giocatore,
 * StatoGioco e le strutture dati per liste, pile, code e database dinamico.
 * Viene incluso dalla maggior parte dei moduli.
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
#include <string.h>  /* per strncpy usato nelle inline functions */
#include <stdio.h>   /* per snprintf in Giocatore_NomeBot */

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
 *  NODO CARTA - Lista DOPPIAMENTE concatenata - ADT 1/4
 *  Riferimento: Documentazione CdS - "NodoMano"
 *  "Gestione nodo per la mano del giocatore (lista doppiamente concatenata)"
 * ============================================================ */

/**
 * @brief Nodo per la lista doppiamente concatenata della mano del giocatore.
 *
 * Ogni nodo contiene una carta e due puntatori: uno al nodo successivo
 * (forward link) e uno al nodo precedente (backward link).
 * Questo permette inserimenti ed eliminazioni efficienti in qualsiasi posizione.
 */
typedef struct NodoCarta {
    Carta carta;                    /* La carta contenuta nel nodo */
    struct NodoCarta* prossimo;     /* Puntatore al nodo successivo (next) */
    struct NodoCarta* prev;         /* Puntatore al nodo precedente (prev) */
} NodoCarta;

/** @brief Alias per NodoCarta (usato nella documentazione CdS come NodoMano). */
typedef NodoCarta NodoMano;

/**
 * @brief Struttura container per la lista di carte (mano del giocatore).
 *
 * Mantiene i puntatori a testa e coda per operazioni O(1) e la lunghezza
 * corrente per contare rapidamente le carte in mano.
 */
typedef struct {
    NodoCarta* testa;       /* Primo nodo della lista */
    NodoCarta* coda;        /* Ultimo nodo della lista */
    int lunghezza;          /* Numero totale di carte nella lista */
} ListaCarte;

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
 *  GIOCATORE - Entita' di flusso/stato
 *  Riferimento: Documentazione CdS - "Gestione giocatore"
 * ============================================================ */

/**
 * @brief Rappresenta un giocatore nella partita (umano o bot).
 *
 * Contiene i dati anagrafici dell'utente (o nome del bot),
 * la mano di carte (lista concatenata bidirezionale), e lo stato
 * nella lobby (pronto/in attesa).
 *
 * mano_backup[] e num_carte_backup servono per serializzare su file:
 * la lista linkata (mano) non e' serializzabile via fwrite, quindi
 * prima di salvare si copiano le carte in mano_backup e si setta
 * mano = NULL. Al caricamento si ricostruisce la lista da mano_backup.
 */
typedef struct {
    Utente info;                /* Dati anagrafici completi dell'utente */
    char nome[30];              /* Nome breve (alias per retrocompatibilita') */
    ListaCarte* mano;           /* Puntatore alla lista delle carte in mano (ADT Lista) */
    int is_bot;                 /* 1 se controllato dall'IA, 0 se umano */
    int stato_pronto;           /* 1 = Pronto nella lobby, 0 = In attesa */
    int num_carte_mano;         /* Numero di carte (usato a fine partita per classifica) */
    int socket_id;              /* ID socket del giocatore (-1 se bot o locale) */
    int era_in_prelobby;        /* 1 se il giocatore era presente in prelobby prima di iniziare (per riconnessione) */
    /* Campi per serializzazione: backup flat delle carte mano */
    Carta mano_backup[108];     /* Backup flat delle carte mano per salvataggio su file */
    int num_carte_backup;       /* Numero di carte valide in mano_backup */
} Giocatore;

/* ============================================================
 *  HELPER INLINE PER GIOCATORE
 * ============================================================ */

/**
 * @brief Restituisce il nome del giocatore (compatta con campo nome[30]).
 * @param g Puntatore al giocatore.
 * @return Nome del giocatore, o "Sconosciuto" se vuoto.
 */
static inline const char* Giocatore_Nome(const Giocatore* g) {
    return g->nome[0] ? g->nome : "Sconosciuto";
}

/** @brief Dimensione del buffer per Giocatore_NomeBot. */
#define GIOCATORE_NOME_BUF 60

/**
 * @brief Restituisce il nome del giocatore con suffisso "(Bot)" se e' un bot.
 * @param g Puntatore al giocatore.
 * @return Nome formattato (buffer statico, thread-local).
 */
static inline const char* Giocatore_NomeBot(const Giocatore* g) {
    static char buf[GIOCATORE_NOME_BUF];
    if (g->is_bot) {
        snprintf(buf, GIOCATORE_NOME_BUF, "%s (Bot)", Giocatore_Nome(g));
        return buf;
    }
    return Giocatore_Nome(g);
}

/**
 * @brief Imposta il nome del giocatore in tutti i campi rilevanti.
 *
 * Copia il nome in: nome[30], info.nome e info.username.
 *
 * @param g Puntatore al giocatore.
 * @param nome Nome da impostare.
 */
static inline void Giocatore_SetNome(Giocatore* g, const char* nome) {
    if (g && nome) {
        strncpy(g->nome, nome, sizeof(g->nome) - 1);
        strncpy(g->info.nome, nome, sizeof(g->info.nome) - 1);
        strncpy(g->info.username, nome, sizeof(g->info.username) - 1);
    }
}

/* ============================================================
 *  STATO GIOCO - Stato completo della partita in corso
 * ============================================================ */

/**
 * @brief Stato completo di una partita in corso.
 *
 * Contiene tutti i dati necessari per gestire il gameplay:
 * giocatori, mazzo, scarti, turno, animazioni, chat, UNO, ecc.
 * Questa struttura viene serializzata per il salvataggio su file
 * e per la sincronizzazione multiplayer via socket.
 */
typedef struct {
    Giocatore giocatori[4];         /* Array dei giocatori (max 4) */
    int num_giocatori;              /* Numero effettivo di giocatori nella partita */
    Carta mazzo[108];               /* Mazzo di pesca (max 108 carte UNO standard) */
    int num_carte_mazzo;            /* Carte rimanenti nel mazzo di pesca */
    Carta scarti[108];              /* Pila degli scarti (max 108) */
    int num_carte_scarti;           /* Carte nella pila degli scarti */
    int turno_corrente;             /* Indice del giocatore di turno */
    int direzione;                  /* 1 = orario, -1 = antiorario (CambioGiro) */
    Colore colore_attivo;           /* Colore attualmente valido per giocare */
    int id_carta_in_trascinamento;  /* Indice della carta trascinata (-1 = nessuna) */
    NodoCarta* nodo_carta_trascinata; /* Puntatore al nodo della carta trascinata */
    int gioco_finito;               /* 1 se la partita e' terminata */
    char messaggio[100];            /* Messaggio da mostrare (es. "HA VINTO X!") */

    /* --- Stato animazioni --- */
    int anim_attiva;                /* 1 se un'animazione e' in corso */
    Carta anim_carta;               /* Carta coinvolta nell'animazione */
    Vector2 anim_start;             /* Punto di partenza dell'animazione */
    Vector2 anim_end;               /* Punto di arrivo dell'animazione */
    float anim_t;                   /* Progresso animazione (0.0 - 1.0) */
    int tipo_animazione;            /* 0 = normale, 1 = animazione speciale */

    /* --- Stato scelta colore e UNO --- */
    int in_scelta_colore;           /* 1 se il giocatore deve scegliere un colore */
    Carta carta_pendente;           /* Carta giocata in attesa della scelta del colore */
    int deve_chiamare_uno;          /* ID+1 del giocatore che deve premere UNO */
    float timer_uno;                /* Timer countdown per il pulsante UNO */
    char notifica[128];             /* Testo della notifica temporanea (aumentato da 50 a 128 per evitare overflow con nomi lunghi) */
    float timer_notifica;           /* Timer di visualizzazione della notifica */
    int autore_animazione;          /* ID del giocatore che ha scatenato l'animazione */
    int anim_pesca_count;           /* Quante carte pescare quando animazione DRAW termina */
    int anim_gia_applicata;         /* 1 = stato gia' applicato dal packet_handler */

    /* --- Stato chat --- */
    char chat_buffer[256];          /* Buffer per il messaggio chat in scrittura */

    /* --- Informazioni partita --- */
    float durata_partita;           /* Durata in secondi della partita in corso */
    char chat_history[15][128];     /* Cronologia ultimi 15 messaggi chat */
    int num_chat_msg;               /* Numero di messaggi nella cronologia */
    int chat_aperta;                /* 1 se il pannello chat e' visibile */
    int slot_salvataggio;           /* Slot di salvataggio corrente */
    char data_salvataggio[30];      /* Data/ora del salvataggio */
    int statistiche_gia_salvate;    /* 1 se le statistiche fine-partita sono gia' state salvate */
    char modalita[20];              /* BUG 3 FIX: Modalita' di gioco salvata (es. "1 vs 3", "Online")
                                       per ripristino corretto al caricamento della partita. */
} StatoGioco;

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