/**
 * @file lan_save_sync.c
 * @brief Sincronizzazione salvataggi partite via LAN (chunk UDP).
 * @ingroup lan
 *
 * Gestisce l'invio e la ricezione di file di salvataggio partita
 * in chunk UDP da 1024 byte, con ricostruzione, deduplicazione
 * e compattazione automatica.
 *
 * Funzioni principali:
 *   - GetSlotPath/GetSlotPathForUser/GetSlotPathForUsername: path helper
 *   - TrovaRecv: cerca o alloca ricevitore di chunk
 *   - InviaSalvataggioChunked: invia file .dat in chunk multipli
 *   - RiceviChunk: accumula chunk e ricostruisce file completo
 *   - SerializzaESpedisciSalvataggi: scandisce slot e invia tutti
 *   - CompattaSavesUtenteCorrente: ordina cronologicamente salvataggi
 *   - LAN_ProcessSaveChunk: entry point per packet handler
 *   - LAN_BroadcastSavesNow: forza broadcast salvataggi
 *   - LAN_UpdateSaves: timeout e cleanup ricevitori
 *
 * ADT utilizzato: SaveReceiver[MAX_RECV] per accumulo chunk. Inoltre ogni SaveReceiver
 * gestisce il proprio timer (timeout 30s, reset ad ogni chunk).
 */
#include "../../../lib/socket/network/socket_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../lib/socket/lan/lan_sync.h"
#include "../../../lib/socket/lan/lan_protocol.h"
#include "../../../lib/auth/auth.h"
#include "../../../lib/game/game_logic.h"
#include "../../../lib/fs/fs.h"

#define MAX_RECV 10
#define LAN_SAVE_PATH_BUF 70

/* Ricevitore per accumulo chunk salvataggi */
static SaveReceiver recvs[MAX_RECV];

/* ================================================================== */

/**
 * @brief Costruisce il path del file di salvataggio nel formato
 *        "data/games/save_user{ID}_slot_{N}.dat" (coerente con save_manager.c).
 * @param buffer Buffer di destinazione.
 * @param bufsize Dimensione del buffer.
 * @param slot Indice dello slot (1-100).
 */
static void GetSlotPath(char* buffer, size_t bufsize, int slot) {
    snprintf(buffer, bufsize, "data/games/save_user%d_slot_%d.dat",
             id_utente_corrente, slot);
}

/**
 * @brief Cerca o alloca un ricevitore per owner/data.
 * @return Indice nell'array recvs, -1 se pieno.
 */
static int TrovaRecv(const char* owner, const char* data) {
    char key[70];
    snprintf(key, sizeof(key), "%s|%s", owner, data);
    for (int i = 0; i < MAX_RECV; i++)
        if (recvs[i].attivo && strcmp(recvs[i].key, key) == 0) return i;
    for (int i = 0; i < MAX_RECV; i++) {
        if (!recvs[i].attivo) {
            memset(&recvs[i], 0, sizeof(SaveReceiver));
            strncpy(recvs[i].key, key, sizeof(recvs[i].key) - 1);
            recvs[i].attivo = 1;
            return i;
        }
    }
    return -1;
}

/**
 * @brief Invia un file di salvataggio in chunk UDP broadcast.
 * @param filepath Percorso del file .dat da inviare.
 * @param owner Username del proprietario.
 * @param data Data del salvataggio.
 * @param slot Numero slot.
 * @param num_g Numero giocatori nel salvataggio.
 */
static void InviaSalvataggioChunked(const char* filepath, const char* owner,
                                     const char* data, int slot, int num_g) {
    extern SOCKET lanSocket;
    extern struct sockaddr_in broadcastAddr;

    FILE* f = fopen(filepath, "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    int fs = (int)ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* fd = (unsigned char*)malloc(fs);
    if (!fd) { fclose(f); return; }
    size_t letto = fread(fd, 1, fs, f);
    fclose(f);
    if ((int)letto != fs) { free(fd); return; }

    int total = 1 + (fs + SAVE_CHUNK_SIZE - 1) / SAVE_CHUNK_SIZE;
    LanSaveChunkPacket ch;
    memset(&ch, 0, sizeof(ch));
    ch.magic = LAN_PACKET_SAVE;
    strncpy(ch.owner_username, owner, 29);
    strncpy(ch.data_salvataggio, data, 29);
    ch.slot = slot;
    ch.num_giocatori = num_g;
    ch.total_chunks = total;
    ch.chunk_index = 0;
    ch.file_size = fs;
    sendto(lanSocket, (char*)&ch, sizeof(ch), 0,
           (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));

    int offset = 0;
    int ci = 1;
    while (offset < fs) {
        int cds = fs - offset;
        if (cds > SAVE_CHUNK_SIZE) cds = SAVE_CHUNK_SIZE;
        memset(&ch, 0, sizeof(ch));
        ch.magic = LAN_PACKET_SAVE;
        strncpy(ch.owner_username, owner, 29);
        strncpy(ch.data_salvataggio, data, 29);
        ch.slot = slot;
        ch.num_giocatori = num_g;
        ch.total_chunks = total;
        ch.chunk_index = ci;
        ch.file_size = fs;
        memcpy(ch.data, fd + offset, cds);
        sendto(lanSocket, (char*)&ch, sizeof(ch), 0,
               (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
        offset += cds;
        ci++;
    }
    free(fd);
}

/**
 * @brief Accumula un chunk di salvataggio. All'ultimo chunk, deduplica,
 *        scrive su disco nel nuovo formato e compatta.
 *
 * Strategia:
 *   - Trova un SaveReceiver tramite TrovaRecv (chiave owner|data)
 *   - Se chunk_index == 0: inizializza ricevitore (header)
 *   - Altrimenti: accumola dati in filedata
 *   - Quando tutti i chunk sono arrivati: verifica se esiste gia',
 *     trova slot libero, scrive il file, compatta. Timeout 30s.
 */
static void RiceviChunk(const LanSaveChunkPacket* chunk) {
    if (!chunk || chunk->magic != LAN_PACKET_SAVE) return;
    int ridx = TrovaRecv(chunk->owner_username, chunk->data_salvataggio);
    if (ridx == -1) return;
    SaveReceiver* r = &recvs[ridx];

    if (chunk->chunk_index == 0) {
        r->total = chunk->total_chunks;
        r->ricevuti = 0;
        r->file_size = chunk->file_size;
        if (r->filedata) free(r->filedata);
        r->filedata = NULL;
        r->timer = 0;
        return;
    }

    if (r->total <= 1) return;
    if (!r->filedata && r->file_size > 0) {
        r->filedata = (unsigned char*)malloc(r->file_size);
        if (!r->filedata) { r->attivo = 0; return; }
        memset(r->filedata, 0, r->file_size);
    }

    int offset = (chunk->chunk_index - 1) * SAVE_CHUNK_SIZE;
    int ds = r->file_size - offset;
    if (ds > SAVE_CHUNK_SIZE) ds = SAVE_CHUNK_SIZE;
    if (ds > 0 && r->filedata)
        memcpy(r->filedata + offset, chunk->data, ds);

    r->ricevuti++;
    r->timer = 0;

    if (r->ricevuti >= r->total - 1) {
        if (id_utente_corrente >= 0 && id_utente_corrente < dbUtenti.num_utenti) {
            char owner[30], data_buf[30];
            sscanf(r->key, "%[^|]|%[^|]", owner, data_buf);
            if (strcmp(owner, dbUtenti.lista[id_utente_corrente].username) == 0) {
                int gia_esiste = 0;
                for (int s = 1; s <= 100 && !gia_esiste; s++) {
                    char sp[LAN_SAVE_PATH_BUF];
                    GetSlotPath(sp, sizeof(sp), s);
                    FILE* ff = fopen(sp, "rb");
                    if (!ff) continue;
                    StatoGioco ex;
                    size_t rr = fread(&ex, sizeof(StatoGioco), 1, ff);
                    fclose(ff);
                    if (rr == 1 && ex.giocatori[0].nome[0] != '\0' &&
                        strcmp(ex.data_salvataggio, data_buf) == 0) {
                        gia_esiste = 1;
                    }
                }
                if (gia_esiste) {
                    if (r->filedata) free(r->filedata);
                    r->attivo = 0;
                    r->filedata = NULL;
                    return;
                }

                int slot_trovato = -1;
                for (int s = 1; s <= 100 && slot_trovato == -1; s++) {
                    char sp[LAN_SAVE_PATH_BUF];
                    GetSlotPath(sp, sizeof(sp), s);
                    FILE* ff = fopen(sp, "rb");
                    if (!ff) { slot_trovato = s; break; }
                    StatoGioco ex;
                    size_t rr = fread(&ex, sizeof(StatoGioco), 1, ff);
                    fclose(ff);
                    if (rr != 1 || ex.giocatori[0].nome[0] == 0) {
                        slot_trovato = s;
                        break;
                    }
                }
                if (slot_trovato == -1) slot_trovato = 100;

    fs_mkdirs();
                char p[LAN_SAVE_PATH_BUF];
                GetSlotPath(p, sizeof(p), slot_trovato);
                FILE* fw = fopen(p, "wb");
                if (fw && r->filedata) {
                    fwrite(r->filedata, 1, r->file_size, fw);
                    fclose(fw);
                }
                CompattaSavesUtenteCorrente();
            }
        }
        if (r->filedata) free(r->filedata);
        r->attivo = 0;
        r->filedata = NULL;
    }
}

/**
 * @brief Scandisce gli slot 1-100 e invia i salvataggi dell'utente corrente.
 *
 * Per ogni slot:
 *   - Legge il file in formato nuovo (save_user{ID}_slot_N.dat)
 *   - Verifica che il proprietario corrisponda all'utente loggato
 *   - Invia il file in chunk via LAN broadcast
 *
 * Fallback retrocompatibilita': cerca anche nel vecchio formato
 * save_slot_N.dat per migrare i salvataggi esistenti.
 */
static void SerializzaESpedisciSalvataggi(void) {
    if (id_utente_corrente < 0 || id_utente_corrente >= dbUtenti.num_utenti) return;
    const char* cu = dbUtenti.lista[id_utente_corrente].username;

    for (int slot = 1; slot <= 100; slot++) {
        char p[LAN_SAVE_PATH_BUF];
        GetSlotPath(p, sizeof(p), slot);
        FILE* f = fopen(p, "rb");
        if (f) {
            StatoGioco t;
            size_t r = fread(&t, sizeof(StatoGioco), 1, f);
            fclose(f);
            if (r == 1 && t.giocatori[0].nome[0] != 0 &&
                strcmp(t.giocatori[0].nome, cu) == 0) {
                InviaSalvataggioChunked(p, cu, t.data_salvataggio, slot, t.num_giocatori);
            }
        }
    }

    for (int slot = 1; slot <= 100; slot++) {
        char oldPath[50];
        sprintf(oldPath, "data/games/save_slot_%d.dat", slot);
        FILE* f = fopen(oldPath, "rb");
        if (!f) continue;
        StatoGioco t;
        size_t r = fread(&t, sizeof(StatoGioco), 1, f);
        fclose(f);
        if (r != 1 || t.giocatori[0].nome[0] == 0 ||
            strcmp(t.giocatori[0].nome, cu) != 0) continue;

        int gia_migrato = 0;
        for (int ns = 1; ns <= 100 && !gia_migrato; ns++) {
            char newPath[LAN_SAVE_PATH_BUF];
            GetSlotPath(newPath, sizeof(newPath), ns);
            FILE* nf = fopen(newPath, "rb");
            if (!nf) continue;
            StatoGioco nt;
            size_t nr = fread(&nt, sizeof(StatoGioco), 1, nf);
            fclose(nf);
            if (nr == 1 && nt.giocatori[0].nome[0] != 0 &&
                strcmp(nt.data_salvataggio, t.data_salvataggio) == 0) {
                gia_migrato = 1;
            }
        }

        if (!gia_migrato) {
            int freeSlot = -1;
            for (int ns = 1; ns <= 100 && freeSlot == -1; ns++) {
                char newPath[LAN_SAVE_PATH_BUF];
                GetSlotPath(newPath, sizeof(newPath), ns);
                FILE* nf = fopen(newPath, "rb");
                if (!nf) { freeSlot = ns; break; }
                fclose(nf);
            }
            if (freeSlot == -1) freeSlot = 100;

            FILE* of = fopen(oldPath, "rb");
            if (of) {
                StatoGioco save;
                size_t sr = fread(&save, sizeof(StatoGioco), 1, of);
                fclose(of);
                if (sr == 1) {
                    char newPath[LAN_SAVE_PATH_BUF];
                    GetSlotPath(newPath, sizeof(newPath), freeSlot);
                    FILE* nf = fopen(newPath, "wb");
                    if (nf) {
                        fwrite(&save, sizeof(StatoGioco), 1, nf);
                        fclose(nf);
                        remove(oldPath);
                        InviaSalvataggioChunked(newPath, cu, save.data_salvataggio, freeSlot, save.num_giocatori);
                    }
                }
            }
        }
    }
}

/**
 * @brief Aggiorna i timer dei ricevitori di chunk salvataggi.
 *
 * Chiamata ogni frame da LAN_Update(). Gestisce:
 *   - Timeout 30s: se un ricevitore non completa il file entro 30s, viene rilasciato
 *   - Cleanup automatico: se tutti i chunk sono arrivati, rilascia il buffer
 *
 * @param dt Delta time dall'ultimo frame (per aggiornamento timer).
 */
void LAN_UpdateSaves(float dt) {
    for (int i = 0; i < MAX_RECV; i++) {
        if (!recvs[i].attivo) continue;
        if (recvs[i].ricevuti >= recvs[i].total - 1) {
            if (recvs[i].filedata) free(recvs[i].filedata);
            recvs[i].attivo = 0;
            recvs[i].filedata = NULL;
        } else {
            recvs[i].timer += dt;
            if (recvs[i].timer > 30.0f) {
                if (recvs[i].filedata) free(recvs[i].filedata);
                recvs[i].attivo = 0;
                recvs[i].filedata = NULL;
            }
        }
    }
}

/**
 * @brief Forza broadcast immediato di tutti i salvataggi dell'utente corrente.
 *
 * Chiama SerializzaESpedisciSalvataggi() per inviare tutti gli slot
 * dell'utente loggato ai dispositivi sulla LAN.
 */
void LAN_BroadcastSavesNow(void) {
    SerializzaESpedisciSalvataggi();
}

/**
 * @brief Entry point pubblico per processare un chunk di salvataggio ricevuto.
 *
 * Chiamato da LAN_Update() in lan_broadcast.c quando arriva un
 * pacchetto LAN_PACKET_SAVE.
 *
 * @param chunk Puntatore al pacchetto chunk ricevuto.
 */
void LAN_ProcessSaveChunk(const LanSaveChunkPacket* chunk) {
    RiceviChunk(chunk);
}

/**
 * @brief Compatta i salvataggi dell'utente corrente in ordine cronologico.
 *
 * Procedura:
 *   1. Raccoglie tutti i salvataggi dell'utente (formato nuovo e vecchio)
 *   2. Ordina per data crescente (bubble sort)
 *   3. Rinnomina i file in formato temporaneo
 *   4. Riscrive negli slot 1-100 in ordine cronologico
 *   5. Rimuove i file temporanei
 *
 * Usa il nuovo formato save_user{ID}_slot_{N}.dat con fallback
 * al vecchio save_slot_N.dat per retrocompatibilita'.
 */
void CompattaSavesUtenteCorrente(void) {
    if (id_utente_corrente < 0 || id_utente_corrente >= dbUtenti.num_utenti) return;
    const char* currentUser = dbUtenti.lista[id_utente_corrente].username;

    typedef struct { char path[60]; char data[30]; int slot; } SaveInfo;
    SaveInfo saves[100];
    int count = 0;

    for (int s = 1; s <= 100; s++) {
        char sp[LAN_SAVE_PATH_BUF];
        GetSlotPath(sp, sizeof(sp), s);
        FILE* ff = fopen(sp, "rb");
        if (!ff) continue;
        StatoGioco ex;
        size_t rr = fread(&ex, sizeof(StatoGioco), 1, ff);
        fclose(ff);
        if (rr != 1 || ex.giocatori[0].nome[0] == 0) continue;
        if (strcmp(ex.giocatori[0].nome, currentUser) != 0) continue;
        strncpy(saves[count].data, ex.data_salvataggio, sizeof(saves[count].data) - 1);
        saves[count].data[sizeof(saves[count].data) - 1] = '\0';
        sprintf(saves[count].path, "data/games/save_user%d_slot_%d.dat", id_utente_corrente, s);
        saves[count].slot = s;
        count++;
    }

    for (int s = 1; s <= 100; s++) {
        char sp[50];
        sprintf(sp, "data/games/save_slot_%d.dat", s);
        FILE* ff = fopen(sp, "rb");
        if (!ff) continue;
        StatoGioco ex;
        size_t rr = fread(&ex, sizeof(StatoGioco), 1, ff);
        fclose(ff);
        if (rr != 1 || ex.giocatori[0].nome[0] == 0) continue;
        if (strcmp(ex.giocatori[0].nome, currentUser) != 0) continue;

        int gia_presente = 0;
        for (int c = 0; c < count; c++) {
            if (strcmp(saves[c].data, ex.data_salvataggio) == 0) {
                gia_presente = 1;
                break;
            }
        }
        if (!gia_presente) {
            strncpy(saves[count].data, ex.data_salvataggio, sizeof(saves[count].data) - 1);
            saves[count].data[sizeof(saves[count].data) - 1] = '\0';
            sprintf(saves[count].path, "data/games/save_slot_%d.dat", s);
            saves[count].slot = s;
            count++;
        }
    }

    if (count <= 1) return;

    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (strcmp(saves[j].data, saves[j + 1].data) > 0) {
                SaveInfo tmp = saves[j];
                saves[j] = saves[j + 1];
                saves[j + 1] = tmp;
            }
        }
    }

    char tempPath[60];
    for (int i = 0; i < count; i++) {
        sprintf(tempPath, "data/games/temp_lan_%d.dat", i);
        rename(saves[i].path, tempPath);
    }
    for (int i = 0; i < count; i++) {
        sprintf(tempPath, "data/games/temp_lan_%d.dat", i);
        int slot_dest = -1;
        for (int s = 1; s <= 100 && slot_dest == -1; s++) {
            char check[LAN_SAVE_PATH_BUF];
            GetSlotPath(check, sizeof(check), s);
            FILE* fc = fopen(check, "rb");
            if (!fc) { slot_dest = s; }
            else fclose(fc);
        }
        if (slot_dest == -1) { remove(tempPath); continue; }
        char newPath[LAN_SAVE_PATH_BUF];
        GetSlotPath(newPath, sizeof(newPath), slot_dest);
        FILE* ft = fopen(tempPath, "rb");
        if (ft) {
            StatoGioco save;
            size_t r = fread(&save, sizeof(StatoGioco), 1, ft);
            fclose(ft);
            if (r == 1) {
                save.slot_salvataggio = slot_dest;
                FILE* fn = fopen(newPath, "wb");
                if (fn) {
                    fwrite(&save, sizeof(StatoGioco), 1, fn);
                    fclose(fn);
                }
                remove(tempPath);
                continue;
            }
        }
        rename(tempPath, newPath);
    }
}