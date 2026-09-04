/**
 * @file player.h
 * @brief Modulo Giocatore - Entità di flusso/stato per la partita.
 * @defgroup player Giocatore
 * @brief Definisce la struttura Giocatore e le relative funzioni di accesso,
 * incapsulando i dettagli implementativi rispetto a data_structures.h.
 *
 * Riferimento: Documentazione CdS - "Gestione giocatore"
 */

#ifndef PLAYER_H
#define PLAYER_H

#include "../data_structures/data_structures.h"
#include "../data_structures/list.h"
#include <string.h>  /* per strncpy */
#include <stdio.h>   /* per snprintf */

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

#endif /* PLAYER_H */