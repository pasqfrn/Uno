/**
 * @file game_state.h
 * @brief Modulo StatoGioco - Stato completo della partita in corso.
 * @defgroup game_state Stato Gioco
 * @brief Definisce la struttura StatoGioco, che rappresenta lo stato
 * completo di una partita UNO in corso. Separato da data_structures.h
 * per migliorare la modularizzazione e l'incapsulamento.
 */

#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "data_structures.h"
#include "player.h"

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

#endif /* GAME_STATE_H */