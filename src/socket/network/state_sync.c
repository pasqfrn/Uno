/**
 * @file state_sync.c
 * @brief Sincronizzazione dello stato di gioco da remoto a locale.
 * @ingroup network
 *
 * @details Gestisce la ricezione dello StatoGioco dal server/host e lo applica
 *          al client locale. Responsabile di:
 *          - Merge stato principale (giocatori, carte, turni, scarti)
 *          - Calcolo classifica finale e salvataggio statistiche
 *          - Adattamento animazioni alla prospettiva del client
 *          - Preservazione stato UI transiente (drag, chat, notifiche)
 *          - Protezione contro "partite fantasma" da lobby precedenti
 *          - Blocco sync durante attesa "Gioca Ancora" in lobby
 *
 * @note Per il client, calcola local_player_id cercando il proprio username
 *       nell'array giocatori[] dello stato remoto ricevuto.
 *
 * Protezioni implementate:
 *   - Anticopia: blocca sincronizzazione se client in lobby (host_in_prelobby==1)
 *     e stato remoto e' finito (gioco_finito==1, partita precedente). Permette
 *     invece la sincronizzazione della lobby (num_carte_scarti==0, gioco_finito==0)
 *     per permettere al client di vedere l'host e gli altri client che si uniscono
 *   - Antiduplicati: non salva statistiche se client in attesa in lobby
 *     (host_in_prelobby==1) per evitare salvataggi multipli
 *
 * ADT utilizzati:
 *   - StatoGioco (struct principale)
 *   - Statistiche (DB utenti per salvataggio piazzamento)
 *
 * Flusso sincronizzazione:
 *   1. Verifica protezione anticopia (host_in_prelobby)
 *   2. Riceve PACKET_GAME_STATE (host -> client)
 *   3. Identifica local_player_id per adattare animazioni
 *   4. Se gioco_finito==1 e non in lobby: calcola classifica e salva statistiche
 *   5. Copia campi stato: giocatori, carte, turno, direzione, scarti
 *   6. Adatta posizioni animazioni in base a perspectiva client
 *   7. Copia chat e notifiche (ecceto stato trascinamento carte)
 *
 * File correlati:
 *   - network.h        : strutture dati (StatoGioco, NetPacket)
 *   - packet_handler_game.c : ricezione PACKET_GAME_STATE
 *   - game_logic.h     : CalcolaClassifica per piazzamento finale
 *   - auth.h           : accesso dbUtenti per salvataggio statistiche
 *   - end_screen.h     : classifica fine partita
 *   - gameplay_screen.h: gestione trascinamento carte
 *   - lan_sync.h       : broadcast statistiche via LAN
 *   - multiplayer_lobby.c: gestione riconnessione diretta
 *
 * @author Pasquale Franco
 * @version 9.1.3
 * @date 2026-07-01
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../../lib/socket/network/state_sync.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/game/game_logic.h"
#include "../../../lib/auth/auth.h"
#include "../../../lib/screens/game/gameplay_screen.h"
#include "../../../lib/screens/game/end_screen.h"
#include "../../../lib/socket/lan/lan_sync.h"

/**
 * @brief Applica lo stato ricevuto da remoto (host/server) al client locale.
 *
 * Sincronizza: stato principale, animazioni, notifiche, chat, giocatori,
 * scarti. Per il client, calcola anche la classifica finale e salva le
 * statistiche nel DB locale con la posizione reale.
 *
 * Protezioni:
 *   - Anticopia: se client in lobby (host_in_prelobby==1) e stato remoto
 *     e' finito (gioco_finito==1, partita precedente), NON sovrascrive
 *     lo stato locale per evitare "partite fantasma". Permette invece
 *     la sincronizzazione della lobby per vedere host e client in attesa
 *   - Antiduplicati: non salva statistiche se client in attesa in lobby
 *     (host_in_prelobby==1) per evitare salvataggi multipli
 *
 * @param local  Puntatore allo stato di gioco locale da aggiornare.
 * @param remote Puntatore allo stato di gioco ricevuto dalla rete.
 */
void SyncGameStateFromNetwork(StatoGioco* local, const StatoGioco* remote) {
    if (!local || !remote) return;

    /* PROTEZIONE ANTICOPIA (Gioca Ancora -> lobby): Se il client e' in lobby
     * (host_in_prelobby == 1, client in attesa dopo aver premuto "Gioca Ancora"),
     * NON sovrascrivere lo stato di gioco locale con quello remoto se:
     *   - remoto ha gioco_finito == 1 (partita finita precedente, vecchio stato)
     * Questo previene la "partita fantasma" in cui il client riceve
     * uno stato di gioco non valido dalla lobby precedente.
     * NOTA: Il controllo su num_carte_scarti == 0 e' stato rimosso perche'
     * in lobby la sincronizzazione e' necessaria per vedere l'host e gli
     * altri client che si uniscono. */
    if (currentRole == NET_CLIENT && host_in_prelobby && remote->gioco_finito) {
        /* Se siamo in lobby (host_in_prelobby) e il remoto e' una partita
         * finita precedente, blocca la sincronizzazione. Lascia passare
         * la lobby (num_carte_scarti==0, gioco_finito==0) e le nuove
         * partite (num_carte_scarti>0, gioco_finito==0). */
        return;
    }

    /* ================================================================
     *  IDENTIFICA local_player_id (solo client)
     *  Deve essere fatto PRIMA di salvare le statistiche, altrimenti
     *  il calcolo della posizione risulta sbagliato.
     *  Usa remote->giocatori[] perche' local non e' ancora sincronizzato.
     * ================================================================ */
    if (currentRole == NET_CLIENT && id_utente_corrente >= 0) {
        for (int i = 0; i < remote->num_giocatori; i++) {
            if (strcmp(remote->giocatori[i].nome, dbUtenti.lista[id_utente_corrente].username) == 0) {
                local_player_id = i;
                break;
            }
        }
    }

    /* ================================================================
     *  STATISTICHE FINE PARTITA (host e client)
     *  Calcola la classifica reale basata su num_carte_mano e salva
     *  la posizione effettiva (non solo 1 o ultimo) nel DB locale.
     *
     *  - Per CLIENT: quando riceve PACKET_GAME_STATE con gioco_finito=1
     *  - Per HOST: quando BroadcastGameState setta gioco_finito=1 (da
     *    PACKET_MOVE ricevuto da un altro giocatore che fa vincere un client)
     *
     *  Protezione: statistiche_gia_salvate evita duplicati.
     *  BUG 4 FIX: NON salvare se partita interrotta (__HOST_DISCONNECT__).
     *  BUG FIX (Gioca Ancora): NON salvare se il client sta aspettando
     *  in lobby (host_in_prelobby) per evitare duplicati dalla vecchia
     *  partita che arriva dopo aver premuto "Gioca Ancora".
     * ================================================================ */
    // Salva le statistiche se il gioco e' finito E non sono gia' state salvate
    // NOTA: Non controlliamo piu' "__HOST_DISCONNECT__" perche' se gioco_finito == 1
    // significa che la partita e' terminata regolarmente e le statistiche VANNO salvate.
    // "__HOST_DISCONNECT__" viene impostato DOPO che l'host ha inviato lo stato finale.
    if (local->gioco_finito == 0 && remote->gioco_finito == 1 && id_utente_corrente >= 0 && !host_in_prelobby) {
        if (!local->statistiche_gia_salvate) {
            local->statistiche_gia_salvate = 1;
            Statistiche* utente = &dbUtenti.lista[id_utente_corrente];
            if (utente && utente->partite_giocate < MAX_PARTITE_STORICO) {
                PartitaRegistrata* p = &utente->storico_partite[utente->partite_giocate];
                if (currentRole != NET_OFFLINE) strcpy(p->modalita, "Online");
                else sprintf(p->modalita, "1 vs %d", remote->num_giocatori - 1);
                p->durata_secondi = (int)remote->durata_partita;

                /* Calcola classifica reale basata su num_carte_mano (gia' sincronizzato).
                 * Ordina i giocatori per numero di carte crescente (chi ha 0 carte = vincitore). */
                int carte[4], indici[4], nc = remote->num_giocatori;
                for (int i = 0; i < nc; i++) {
                    carte[i] = remote->giocatori[i].num_carte_mano;
                    indici[i] = i;
                }
                /* Bubble sort: ordine crescente di carte rimanenti */
                for (int i = 0; i < nc - 1; i++)
                    for (int j = i + 1; j < nc; j++)
                        if (carte[j] < carte[i]) {
                            int tmp = carte[i]; carte[i] = carte[j]; carte[j] = tmp;
                            tmp = indici[i]; indici[i] = indici[j]; indici[j] = tmp;
                        }
                /* Trova posizione del giocatore locale nella classifica ordinata */
                int mio_id = (currentRole == NET_CLIENT) ? local_player_id : 0;
                int mia_pos = nc, hoVinto = 0;
                for (int i = 0; i < nc; i++) {
                    if (indici[i] == mio_id) {
                        mia_pos = i + 1;
                        if (mia_pos == 1) hoVinto = 1;
                        break;
                    }
                }
                strcpy(p->esito, hoVinto ? "Vittoria" : "Sconfitta");
                p->posizione = mia_pos;
                utente->partite_giocate++;
                if (hoVinto) utente->vittorie_giocatore++;
                else utente->vittorie_bot++;
                SalvaDB();
                /* Broadcast immediato via LAN per sincronizzare statistiche su altri PC */
                LAN_BroadcastNow();
                /* BUG FIX 5: Sync completo statistiche (incluso storico partite) via LAN */
                if (id_utente_corrente >= 0) {
                    LAN_BroadcastStatsSync(dbUtenti.lista[id_utente_corrente].username);
                }
                
                // Invia le statistiche aggiornate all'host (se sono un client)
                if (currentRole == NET_CLIENT && local_player_id >= 0 && local_player_id < remote->num_giocatori) {
                    StatsSyncPacket sync = {0};
                    strncpy(sync.username, dbUtenti.lista[id_utente_corrente].username, sizeof(sync.username) - 1);
                    sync.partite_giocate = dbUtenti.lista[id_utente_corrente].partite_giocate;
                    sync.vittorie_giocatore = dbUtenti.lista[id_utente_corrente].vittorie_giocatore;
                    sync.vittorie_bot = dbUtenti.lista[id_utente_corrente].vittorie_bot;
                    sync.durata_ultima_partita = remote->durata_partita;
                    strncpy(sync.modalita_ultima, p->modalita, sizeof(sync.modalita_ultima) - 1);
                    strncpy(sync.esito_ultima, p->esito, sizeof(sync.esito_ultima) - 1);
                    sync.posizione_ultima = mia_pos;
                    SendStatsSync(&sync);
                }
            }
        }
    }

    /* ================================================================
     *  STATO GIOCO PRINCIPALE
     *  Copia i campi fondamentali dello stato di gioco dal remoto.
     * ================================================================ */
    local->num_giocatori = remote->num_giocatori;
    local->turno_corrente = remote->turno_corrente;
    local->direzione = remote->direzione;
    local->colore_attivo = remote->colore_attivo;
    local->gioco_finito = remote->gioco_finito;
    local->statistiche_gia_salvate = remote->statistiche_gia_salvate;
    local->durata_partita = remote->durata_partita;
    local->in_scelta_colore = remote->in_scelta_colore;
    local->deve_chiamare_uno = remote->deve_chiamare_uno;
    local->timer_uno = remote->timer_uno;
    local->num_carte_mazzo = remote->num_carte_mazzo;
    local->num_carte_scarti = remote->num_carte_scarti;
    local->carta_pendente = remote->carta_pendente;
    local->autore_animazione = remote->autore_animazione;

    /* ================================================================
     *  STATO ANIMAZIONI
     *  Il client ricalcola start/end in base alla sua prospettiva.
     * ================================================================ */
    local->anim_attiva = remote->anim_attiva;
    local->anim_carta = remote->anim_carta;
    local->anim_t = remote->anim_t;
    local->tipo_animazione = remote->tipo_animazione;
    local->anim_pesca_count = remote->anim_pesca_count;
    local->anim_gia_applicata = remote->anim_gia_applicata;

    if (remote->anim_attiva && currentRole == NET_CLIENT) {
        Vector2 posAutore = OttieniPosizioneGiocatore(remote->autore_animazione, local_player_id, remote->num_giocatori);
        if (remote->tipo_animazione == 1) {
            local->anim_start = (Vector2){ 1280/2.0f - 130, 720/2.0f - 80 };
            local->anim_end = posAutore;
        } else {
            local->anim_start = posAutore;
            local->anim_end = (Vector2){ 1280/2.0f + 25, 720/2.0f - 80 };
        }
    } else {
        local->anim_start = remote->anim_start;
        local->anim_end = remote->anim_end;
    }

    /* ================================================================
     *  NOTIFICHE E MESSAGGI
     * ================================================================ */
    strncpy(local->messaggio, remote->messaggio, sizeof(local->messaggio) - 1);
    strncpy(local->notifica, remote->notifica, sizeof(local->notifica) - 1);
    local->timer_notifica = remote->timer_notifica;

    /* ================================================================
     *  STATO CHAT
     * ================================================================ */
    strncpy(local->chat_buffer, remote->chat_buffer, sizeof(local->chat_buffer) - 1);
    for (int i = 0; i < 15 && i < remote->num_chat_msg; i++) {
        strncpy(local->chat_history[i], remote->chat_history[i], sizeof(local->chat_history[i]) - 1);
    }
    local->num_chat_msg = remote->num_chat_msg;
    local->chat_aperta = remote->chat_aperta;

    /* ================================================================
     *  DATI GIOCATORI
     *  Copia nome, info, stato bot, carte in mano, socket.
     *  Per il client, individua anche il local_player_id.
     * ================================================================ */
    for (int i = 0; i < remote->num_giocatori; i++) {
        strncpy(local->giocatori[i].nome, remote->giocatori[i].nome, sizeof(local->giocatori[i].nome) - 1);
        local->giocatori[i].info = remote->giocatori[i].info;
        local->giocatori[i].is_bot = remote->giocatori[i].is_bot;
        local->giocatori[i].stato_pronto = remote->giocatori[i].stato_pronto;
        local->giocatori[i].num_carte_mano = remote->giocatori[i].num_carte_mano;
        local->giocatori[i].socket_id = remote->giocatori[i].socket_id;
    }

    /* ================================================================
     *  PILE SCARTI
     * ================================================================ */
    for (int i = 0; i < remote->num_carte_scarti; i++) {
        local->scarti[i] = remote->scarti[i];
    }

    /* ================================================================
     *  PULIZIA STATO TRANSIENTE UI (solo client)
     *  Reset dello stato di trascinamento per evitare dangling pointer.
     * ================================================================ */
    if (currentRole == NET_CLIENT) {
        local->id_carta_in_trascinamento = -1;
    }
}