#ifndef PLAYER_PRIVATE_H
#define PLAYER_PRIVATE_H

#include "../../data_structures/data_structures.h"
#include "../../data_structures/list.h"

struct Giocatore {
    Utente info;
    char nome[30];
    ListaCarte* mano;
    int is_bot;
    int stato_pronto;
    int num_carte_mano;
    int socket_id;
    int era_in_prelobby;
    Carta mano_backup[108];
    int num_carte_backup;
};

#endif /* PLAYER_PRIVATE_H */
