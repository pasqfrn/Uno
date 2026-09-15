#ifndef GAME_STATE_PRIVATE_H
#define GAME_STATE_PRIVATE_H

#include "../../data_structures/data_structures.h"
#include "player_private.h"

struct StatoGioco {
    Giocatore giocatori[4];
    int num_giocatori;
    Carta mazzo[108];
    int num_carte_mazzo;
    Carta scarti[108];
    int num_carte_scarti;
    int turno_corrente;
    int direzione;
    Colore colore_attivo;
    int id_carta_in_trascinamento;
    int gioco_finito;
    char messaggio[100];
    int anim_attiva;
    Carta anim_carta;
    Vector2 anim_start;
    Vector2 anim_end;
    float anim_t;
    int tipo_animazione;
    int in_scelta_colore;
    Carta carta_pendente;
    int deve_chiamare_uno;
    float timer_uno;
    char notifica[128];
    float timer_notifica;
    int autore_animazione;
    int anim_pesca_count;
    int anim_gia_applicata;
    char chat_buffer[256];
    float durata_partita;
    char chat_history[15][128];
    int num_chat_msg;
    int chat_aperta;
    int slot_salvataggio;
    char data_salvataggio[30];
    int statistiche_gia_salvate;
    char modalita[20];
};

#endif /* GAME_STATE_PRIVATE_H */
