/**
 * @file lan_broadcast.c
 * @brief Gestione broadcast LAN: inizializzazione, chiusura e smistamento pacchetti.
 * @ingroup lan
 *
 * Modulo principale della sincronizzazione LAN. Gestisce:
 *   - Socket UDP broadcast (creazione, bind, chiusura)
 *   - Invio periodico del database utenti e dei salvataggi
 *   - Ricezione e smistamento pacchetti per tipo (DB, save, delete)
 *   - Filtraggio pacchetti propri (stesso IP)
 *   - Gestione deletedUsers cache e flag anti-ricorsione
 *   - Gestione pending_deletions per eliminazioni ritardate
 *
 * Variabili globali definite qui (non-static per altri moduli):
 *   - lanSocket, broadcastAddr (usati da lan_save_sync.c)
 *   - dentroSalvaDB, numDeletedUsers, deletedUsers
 *   - numPendingDeletions, pendingDeletions
 *
 * Protocollo pacchetti gestiti:
 *   - LAN_PACKET_DB: merge database utenti (-> LAN_ProcessDBPacket)
 *   - LAN_PACKET_SAVE: chunk di salvataggio (-> LAN_ProcessSaveChunk)
 *   - LAN_PACKET_DEL_USER: eliminazione utente (marcato come pending)
 *   - LAN_PACKET_DEL_SAVE: eliminazione salvataggio
 *
 * Gestione eliminazione salvataggi: usa il nuovo formato
 * save_user{ID}_slot_{N}.dat, con fallback al vecchio save_slot_N.dat
 * per compatibilita' con salvataggi esistenti.
 */
#include "../../../lib/socket/network/socket_compat.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../../lib/socket/lan/lan_sync.h"
#include "../../../lib/socket/lan/lan_protocol.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/auth/auth.h"
#include "../../../lib/data_structures/data_structures.h"

/* ============================================================
 *  VARIABILI GLOBALI MODULO LAN
 * ============================================================ */

/** @brief Socket UDP per broadcast LAN. */
SOCKET lanSocket = INVALID_SOCKET;
/** @brief Timer per broadcast periodico database (LAN_SYNC_INTERVAL). */
static float lanTimer = 0.0f;
/** @brief Timer per broadcast periodico salvataggi (LAN_SYNC_INTERVAL). */
static float lanSaveTimer = 0.0f;
/** @brief Buffer di ricezione pacchetti LAN (max 65536 bytes). */
static char lanRecvBuf[65536];
/** @brief Indirizzo broadcast per la subnet locale. */
struct sockaddr_in broadcastAddr;
/** @brief Flag: 1 se il modulo LAN e' stato inizializzato. */
static int lanInizializzato = 0;
/** @brief Hostname del dispositivo locale. */
static char localHostname[64] = "";

/** @brief Flag: 1 se siamo all'interno di una chiamata a SalvaDB() (per evitare ricorsione LAN). */
int dentroSalvaDB = 0;
/** @brief Numero di utenti nella cache delle eliminazioni. */
int numDeletedUsers = 0;
/** @brief Cache degli utenti eliminati (per non riaggiungerli da remoto). */
char deletedUsers[512][30] = {{0}};

/** @brief Numero massimo di eliminazioni pendenti. */

/** @brief Numero di eliminazioni pendenti (attesa conferma client). */
static int numPendingDeletions = 0;
/** @brief Lista utenti in attesa di conferma eliminazione. */
static char pendingDeletions[MAX_PENDING_DELETIONS][30] = {{0}};

/**
 * @brief Aggiunge un utente alla lista delle pending deletions.
 * @param username Username dell'utente eliminato da admin.
 */
static void AggiungiPendingDeletion(const char* username) {
    if (!username || !username[0]) return;
    if (numPendingDeletions < MAX_PENDING_DELETIONS) {
        strncpy(pendingDeletions[numPendingDeletions], username, 29);
        pendingDeletions[numPendingDeletions][29] = '\0';
        numPendingDeletions++;
    }
}

/**
 * @brief Verifica se un utente e' nelle pending deletions.
 * @param username Username da verificare.
 * @return 1 se e' pending, 0 altrimenti.
 */
static int IsPendingDeletion(const char* username) {
    if (!username || !username[0]) return 0;
    for (int i = 0; i < numPendingDeletions; i++) {
        if (strcmp(pendingDeletions[i], username) == 0) return 1;
    }
    return 0;
}

/**
 * @brief Conferma tutte le pending deletions e le promuove a deletedUsers.
 * Chiamato quando un utente conferma la disconnessione.
 */
static void ConfermaPendingDeletions(void) {
    for (int i = 0; i < numPendingDeletions; i++) {
        if (numDeletedUsers < 512) {
            strncpy(deletedUsers[numDeletedUsers], pendingDeletions[i], 29);
            deletedUsers[numDeletedUsers][29] = '\0';
            numDeletedUsers++;
        }
    }
    numPendingDeletions = 0;
}

/**
 * @brief Calcola l'indirizzo broadcast della subnet locale.
 *
 * Ottiene l'IP locale, estrae i primi 3 ottetti e costruisce
 * l'indirizzo broadcast (es. 192.168.1.255).
 * Fallback: 255.255.255.255.
 */
static void CalcolaSubnetBroadcast(void) {
    char localIP[32] = "";
    GetLocalIPAddress(localIP, sizeof(localIP));
    if (localIP[0] != '\0') {
        int a, b, c, d;
        if (sscanf(localIP, "%d.%d.%d.%d", &a, &b, &c, &d) == 4) {
            char subnet[32];
            sprintf(subnet, "%d.%d.%d.255", a, b, c);
            broadcastAddr.sin_addr.s_addr = inet_addr(subnet);
            return;
        }
    }
    broadcastAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
}

int LAN_Init(void) {
    gethostname(localHostname, sizeof(localHostname) - 1);
    lanSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (lanSocket == INVALID_SOCKET) return 0;
    int broadcastOpt = 1;
    setsockopt(lanSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcastOpt, sizeof(broadcastOpt));
    int opt = 1;
    setsockopt(lanSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    struct sockaddr_in localAddr;
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(LAN_SYNC_PORT);
    if (bind(lanSocket, (struct sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        closesocket(lanSocket);
        lanSocket = INVALID_SOCKET;
        return 0;
    }
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(LAN_SYNC_PORT);
    CalcolaSubnetBroadcast();
    u_long mode = 1;
    ioctlsocket(lanSocket, FIONBIO, &mode);
    lanInizializzato = 1;
    lanTimer = 0.0f;
    lanSaveTimer = 0.0f;
    return 1;
}

void LAN_Close(void) {
    if (id_utente_corrente >= 0 && id_utente_corrente < dbUtenti.num_utenti) {
        const char* current_username = dbUtenti.lista[id_utente_corrente].username;
        int is_deleted = 0;
        for (int i = 0; i < numDeletedUsers; i++) {
            if (strcmp(deletedUsers[i], current_username) == 0) {
                is_deleted = 1;
                break;
            }
        }
        if (is_deleted) {
            for (int j = id_utente_corrente; j < dbUtenti.num_utenti - 1; j++)
                dbUtenti.lista[j] = dbUtenti.lista[j+1];
            dbUtenti.num_utenti--;
            id_utente_corrente = -1;
            AuthBST_Ricostruisci();
            SalvaDB();
        }
    }
    
    if (lanSocket != INVALID_SOCKET) {
        closesocket(lanSocket);
        lanSocket = INVALID_SOCKET;
    }
    lanInizializzato = 0;
}

void LAN_BroadcastNow(void) {
    if (!lanInizializzato || lanSocket == INVALID_SOCKET) return;
    char buf[65536];
    int b = SerializzaDB(buf, sizeof(buf));
    if (b > 0) {
        sendto(lanSocket, buf, b, 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
    }
}

void LAN_BroadcastDeleteUser(const char* username) {
    if (!lanInizializzato || lanSocket == INVALID_SOCKET || !username) return;
    AggiungiPendingDeletion(username);
    struct { int magic; char username[30]; } pkt;
    pkt.magic = LAN_PACKET_DEL_USER;
    strncpy(pkt.username, username, sizeof(pkt.username) - 1);
    sendto(lanSocket, (char*)&pkt, sizeof(pkt), 0,
           (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
    LAN_BroadcastNow();
}

void LAN_ConfirmDeleteUser(const char* username) {
    if (!username || !username[0]) return;
    if (IsPendingDeletion(username)) {
        ConfermaPendingDeletions();
    }
}

void LAN_BroadcastStatsSync(const char* username) {
    if (!lanInizializzato || lanSocket == INVALID_SOCKET || !username || !username[0]) return;
    int idx = -1;
    for (int i = 0; i < dbUtenti.num_utenti; i++) {
        if (strcmp(dbUtenti.lista[i].username, username) == 0) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return;
    Statistiche* s = &dbUtenti.lista[idx];
    LanStatsSyncPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.magic = LAN_PACKET_STATS_SYNC;
    strncpy(pkt.username, s->username, sizeof(pkt.username) - 1);
    pkt.partite_giocate = s->partite_giocate;
    pkt.vittorie_giocatore = s->vittorie_giocatore;
    pkt.vittorie_bot = s->vittorie_bot;
    int start = (s->partite_giocate > STORICO_SYNC_COUNT) ? s->partite_giocate - STORICO_SYNC_COUNT : 0;
    pkt.storico_count = s->partite_giocate - start;
    if (pkt.storico_count > STORICO_SYNC_COUNT) pkt.storico_count = STORICO_SYNC_COUNT;
    for (int i = 0; i < pkt.storico_count; i++) {
        pkt.storico_partite[i] = s->storico_partite[start + i];
    }
    sendto(lanSocket, (char*)&pkt, sizeof(pkt), 0,
           (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
}

void LAN_BroadcastDeleteSave(const char* owner_username, const char* data_salvataggio) {
    if (!lanInizializzato || lanSocket == INVALID_SOCKET ||
        !owner_username || !data_salvataggio) return;
    LanDeleteSavePacket pkt;
    pkt.magic = LAN_PACKET_DEL_SAVE;
    strncpy(pkt.owner_username, owner_username, sizeof(pkt.owner_username) - 1);
    strncpy(pkt.data_salvataggio, data_salvataggio, sizeof(pkt.data_salvataggio) - 1);
    sendto(lanSocket, (char*)&pkt, sizeof(pkt), 0,
           (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
}

void LAN_Update(float dt) {
    if (!lanInizializzato || lanSocket == INVALID_SOCKET) return;
    struct sockaddr_in sa;
    int al = sizeof(sa);
    int bytes;
    while (1) {
        bytes = recvfrom(lanSocket, lanRecvBuf, sizeof(lanRecvBuf) - 1, 0,
                         (struct sockaddr*)&sa, &al);
        if (bytes <= 0) break;
        char lip[32] = "";
        GetLocalIPAddress(lip, sizeof(lip));
        const char* sip = inet_ntoa(sa.sin_addr);
        if (sip && lip[0] && strcmp(sip, lip) == 0) continue;
        if (bytes < (int)sizeof(int)) continue;
        int magic;
        memcpy(&magic, lanRecvBuf, sizeof(int));
        if (magic == LAN_PACKET_DB && bytes >= (int)sizeof(LanPacketHeader) + (int)sizeof(int)) {
            LanPacketHeader h;
            memcpy(&h, lanRecvBuf, sizeof(LanPacketHeader));
            if (bytes >= (int)(sizeof(LanPacketHeader) + h.length)) {
                extern void LAN_ProcessDBPacket(const char* buf, int bytes, int offset);
                LAN_ProcessDBPacket(lanRecvBuf, bytes, sizeof(LanPacketHeader));
            }
        } else if (magic == LAN_PACKET_SAVE &&
                   bytes >= (int)sizeof(LanSaveChunkPacket) - SAVE_CHUNK_SIZE + 1) {
            extern void LAN_ProcessSaveChunk(const LanSaveChunkPacket* chunk);
            LAN_ProcessSaveChunk((LanSaveChunkPacket*)lanRecvBuf);
        } else if (magic == LAN_PACKET_DEL_USER && bytes >= (int)(sizeof(int) + 1)) {
            char* del_username = lanRecvBuf + sizeof(int);
            del_username[sizeof(lanRecvBuf) - sizeof(int) - 1] = '\0';
            int modified = 0;
            int deleted_idx = -1;
            if (id_utente_corrente >= 0 && id_utente_corrente < dbUtenti.num_utenti &&
                strcmp(del_username, dbUtenti.lista[id_utente_corrente].username) != 0 &&
                !IsPendingDeletion(del_username)) {
                for (int i = dbUtenti.num_utenti - 1; i >= 0; i--) {
                    if (strcmp(dbUtenti.lista[i].username, del_username) == 0) {
                        deleted_idx = i;
                        for (int j = i; j < dbUtenti.num_utenti - 1; j++)
                            dbUtenti.lista[j] = dbUtenti.lista[j + 1];
                        memset(&dbUtenti.lista[dbUtenti.num_utenti - 1], 0, sizeof(Statistiche));
                        dbUtenti.num_utenti--;
                        modified = 1;
                        break;
                    }
                }
            }
            if (modified) {
                /* BUG 9 FIX: Se l'utente eliminato era prima di id_utente_corrente,
                 * aggiorna id_utente_corrente perche' l'array e' stato compattato */
                if (deleted_idx >= 0 && deleted_idx < id_utente_corrente) {
                    id_utente_corrente--;
                }
                AuthBST_Ricostruisci();
                /* BUG 6 FIX: Dopo aver rimosso l'utente dal DB, salva su disco. */
                SalvaDB();
            }
            if (numDeletedUsers < 512) {
                strncpy(deletedUsers[numDeletedUsers], del_username, 29);
                deletedUsers[numDeletedUsers][29] = '\0';
                numDeletedUsers++;
            }
            /**
             * @brief Sincronizza le statistiche di un utente ricevute da un dispositivo
             * remoto. La sincronizzazione avviene in modo incrementale:
             * - I contatori (partite_giocate, vittorie_giocatore, vittorie_bot)
             *   vengono aggiornati solo se il valore remoto e' maggiore di quello
             *   locale (crescita monotona).
             * - Lo storico partite viene sempre mergiato, indipendentemente dai
             *   contatori. Le partite remote vengono aggiunte solo se non gia'
             *   presenti (deduplica per durata+esito+modalita+posizione).
             *
             * Questo approccio garantisce che entrambi i PC abbiano la stessa
             * vista dello storico partite, anche se i contatori numerici sono
             * identici. Garantisce inoltre che le statistiche non vengano
             * mai sovrascritte ma solo estese con nuove partite.
             */
        } else if (magic == LAN_PACKET_STATS_SYNC && bytes >= (int)sizeof(LanStatsSyncPacket)) {
            /**
             * @brief Ricezione pacchetto sincronizzazione statistiche (LAN_PACKET_STATS_SYNC).
             *
             * BUG FIX 1 (Double Counting): La logica di merge e' stata corretta per evitare
             * che le partite storiche vengano contate due volte.
             *
             * Strategia:
             * 1. Se il remoto ha contatori PIU' ALTI del locale (crescita monotona),
             *    COPIA i contatori e NON mergiare lo storico (le partite sono gia' in quei numeri).
             * 2. Se i contatori sono UGUALI ma ci sono partite storiche NON PRESENTI in locale
             *    (deduplica per durata+esito+modalita+posizione), aggiungile a storico_partite[]
             *    SENZA incrementare partite_giocate (la partita e' gia' contata).
             * 3. Se i contatori sono UGUALI e lo storico e' gia' presente, non fare nulla.
             */
            LanStatsSyncPacket* sp = (LanStatsSyncPacket*)lanRecvBuf;
            if (sp->username[0]) {
                int found = -1;
                for (int i = 0; i < dbUtenti.num_utenti; i++) {
                    if (strcmp(dbUtenti.lista[i].username, sp->username) == 0) {
                        found = i;
                        break;
                    }
                }
                if (found >= 0) {
                    Statistiche* s = &dbUtenti.lista[found];
                    int modified = 0;
                    
                    /* Salva il conteggio vecchio prima di modificare */
                    int vecchie_partite = s->partite_giocate;
                    
                    /* Passo 1: Aggiorna contatori SOLO se il remoto ne ha di piu' (crescita monotona) */
                    if (sp->partite_giocate > s->partite_giocate) {
                        s->partite_giocate = sp->partite_giocate;
                        s->vittorie_giocatore = sp->vittorie_giocatore;
                        s->vittorie_bot = sp->vittorie_bot;
                        modified = 1;
                        /* BUG FIX 1: I contatori sono stati aggiornati.
                         * NON mergiare lo storico perche' le partite sono gia'
                         * incluse nel conteggio. Il double counting si verificava
                         * quando partite_giocate veniva incrementato SIA dalla
                         * copia dei contatori SIA dall'aggiunta dello storico. */
                    } else if (vecchie_partite >= sp->partite_giocate) {
                        /* Passo 2: Contatori UGUALI (o locali piu' alti).
                         * Merge SOLO dello storico partite per avere i dettagli,
                         * SENZA incrementare partite_giocate. */
                        for (int h = 0; h < sp->storico_count && h < STORICO_SYNC_COUNT; h++) {
                            PartitaRegistrata* remota = &sp->storico_partite[h];
                            int gia_presente = 0;
                            int max_local = (vecchie_partite < MAX_PARTITE_STORICO) ? vecchie_partite : MAX_PARTITE_STORICO;
                            for (int lh = 0; lh < max_local; lh++) {
                                PartitaRegistrata* locale = &s->storico_partite[lh];
                                if (locale->durata_secondi == remota->durata_secondi &&
                                    strcmp(locale->esito, remota->esito) == 0 &&
                                    strcmp(locale->modalita, remota->modalita) == 0 &&
                                    locale->posizione == remota->posizione) {
                                    gia_presente = 1;
                                    break;
                                }
                            }
                            /* Aggiungi solo se non presente, SENZA incrementare partite_giocate */
                            if (!gia_presente && vecchie_partite < MAX_PARTITE_STORICO) {
                                s->storico_partite[vecchie_partite] = *remota;
                                vecchie_partite++;
                                modified = 1;
                            }
                        }
                    }
                    
                    if (modified) {
                        dentroSalvaDB = 1;
                        SalvaDB();
                        AuthBST_Ricostruisci();
                        dentroSalvaDB = 0;
                    }
                }
            }
        } else if (magic == LAN_PACKET_DEL_SAVE && bytes >= (int)sizeof(LanDeleteSavePacket)) {
            /**
             * @brief Ricezione notifica di eliminazione salvataggio.
             * Cerca il file nel nuovo formato save_user{ID}_slot_{N}.dat,
             * con fallback al vecchio formato save_slot_N.dat per retrocompatibilita'.
             */
            LanDeleteSavePacket* dp = (LanDeleteSavePacket*)lanRecvBuf;
            if (id_utente_corrente >= 0 && id_utente_corrente < dbUtenti.num_utenti &&
                strcmp(dp->owner_username, dbUtenti.lista[id_utente_corrente].username) == 0) {
                int trovato = 0;
                for (int s = 1; s <= 100 && !trovato; s++) {
                    char sp[70];
                    snprintf(sp, sizeof(sp), "data/games/save_user%d_slot_%d.dat", id_utente_corrente, s);
                    FILE* ff = fopen(sp, "rb");
                    if (!ff) continue;
                    StatoGioco ex;
                    size_t rr = fread(&ex, sizeof(StatoGioco), 1, ff);
                    fclose(ff);
                    if (rr == 1 && ex.giocatori[0].nome[0] != '\0' &&
                        strcmp(ex.giocatori[0].nome, dp->owner_username) == 0 &&
                        strcmp(ex.data_salvataggio, dp->data_salvataggio) == 0) {
                        remove(sp);
                        trovato = 1;
                        break;
                    }
                }
                if (!trovato) {
                    for (int s = 1; s <= 100; s++) {
                        char sp[50];
                        sprintf(sp, "data/games/save_slot_%d.dat", s);
                        FILE* ff = fopen(sp, "rb");
                        if (!ff) continue;
                        StatoGioco ex;
                        size_t rr = fread(&ex, sizeof(StatoGioco), 1, ff);
                        fclose(ff);
                        if (rr == 1 && ex.giocatori[0].nome[0] != '\0' &&
                            strcmp(ex.giocatori[0].nome, dp->owner_username) == 0 &&
                            strcmp(ex.data_salvataggio, dp->data_salvataggio) == 0) {
                            remove(sp);
                            break;
                        }
                    }
                }
                extern void CompattaSavesUtenteCorrente(void);
                CompattaSavesUtenteCorrente();
            }
        }
    }
    lanTimer += dt;
    if (lanTimer >= LAN_SYNC_INTERVAL) { lanTimer = 0; LAN_BroadcastNow(); }
    lanSaveTimer += dt;
    if (lanSaveTimer >= LAN_SYNC_INTERVAL) { lanSaveTimer = 0; LAN_BroadcastSavesNow(); }

    extern void LAN_UpdateSaves(float dt);
    LAN_UpdateSaves(dt);
}

int LAN_GetUtentiCount(void) { return dbUtenti.num_utenti; }