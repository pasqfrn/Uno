#include <stdio.h>
#include <string.h>
#include "../../lib/game/player.h"
#include "../../lib/game/private/player_private.h"

const char* Giocatore_Nome(const Giocatore* g) {
    static char fallback[] = "Sconosciuto";
    if (!g) return fallback;
    if (g->nome[0] != '\0') return g->nome;
    if (g->info.nome[0] != '\0') return g->info.nome;
    return fallback;
}

const char* Giocatore_GetNome(const Giocatore* g) {
    return Giocatore_Nome(g);
}

void Giocatore_SetNome(Giocatore* g, const char* nome) {
    if (!g || !nome) return;
    strncpy(g->nome, nome, sizeof(g->nome) - 1);
    g->nome[sizeof(g->nome) - 1] = '\0';
}

const char* Giocatore_NomeBot(const Giocatore* g) {
    static char buf[64];
    if (!g) { buf[0] = '\0'; return buf; }
    const char* n = Giocatore_Nome(g);
    if (g->is_bot) {
        snprintf(buf, sizeof(buf), "%s (Bot)", n);
    } else {
        strncpy(buf, n, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
    }
    return buf;
}

const char* Giocatore_GetNomeBot(const Giocatore* g) {
    return Giocatore_NomeBot(g);
}

int Giocatore_IsBot(const Giocatore* g) {
    return g ? g->is_bot : 0;
}

void Giocatore_SetPronto(Giocatore* g, int pronto) {
    if (g) g->stato_pronto = pronto ? 1 : 0;
}

int Giocatore_IsPronto(const Giocatore* g) {
    return g ? g->stato_pronto : 0;
}

ListaCarte* Giocatore_GetMano(const Giocatore* g) {
    return g ? g->mano : NULL;
}

void Giocatore_SetMano(Giocatore* g, ListaCarte* mano) {
    if (g) g->mano = mano;
}

int Giocatore_GetNumCarte(const Giocatore* g) {
    return g ? g->num_carte_mano : 0;
}

void Giocatore_SetNumCarte(Giocatore* g, int num) {
    if (g) g->num_carte_mano = num;
}

int Giocatore_GetSocketId(const Giocatore* g) {
    return g ? g->socket_id : -1;
}

void Giocatore_SetSocketId(Giocatore* g, int socket_id) {
    if (g) g->socket_id = socket_id;
}

int Giocatore_EraInPrelobby(const Giocatore* g) {
    return g ? g->era_in_prelobby : 0;
}

void Giocatore_SetEraInPrelobby(Giocatore* g, int era) {
    if (g) g->era_in_prelobby = era ? 1 : 0;
}

int Giocatore_FaiBackupMano(Giocatore* g) {
    if (!g || !g->mano) return 0;
    int n = Lista_CopiaCarte(g->mano, g->mano_backup, 108);
    g->num_carte_backup = n;
    return n;
}

int Giocatore_GetBackupMano(const Giocatore* g, Carta* out, int max) {
    if (!g || !out || max <= 0) return 0;
    int n = 0;
    for (int i = 0; i < g->num_carte_backup && i < max; i++) {
        out[i] = g->mano_backup[i];
        n++;
    }
    return n;
}

int Giocatore_GetNumCarteBackup(const Giocatore* g) {
    return g ? g->num_carte_backup : 0;
}
