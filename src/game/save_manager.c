/**
 * @file save_manager.c
 * @brief Gestione salvataggi partite su file binario.
 * @ingroup game_logic
 *
 * Modulo per la gestione completa dei salvataggi partita su file.
 * Salvataggio, caricamento, compattazione cronologica, eliminazione
 * e integrazione con broadcast LAN.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../lib/game_logic.h"
#include "../../lib/auth.h"
#include "../../lib/socket/lan/lan_sync.h"
#include "../../lib/socket/network/network.h"
#include "../../lib/fs/fs.h"

/* ==================================================================
 *  HELPERS PER PERCORSO FILE SALVATAGGIO (Stack ADT-like pattern)
 *  Formato: data/games/save_user<ID>_slot_<N>.dat
 *  Centralizzato per evitare inconsistenze di path.
 * ================================================================== */

/** @brief Struttura temporanea per ordinamento salvataggi (usata in CompattaSalvataggiUtente). */
typedef struct { char percorso[70]; char data[30]; } TempSave;

/**
 * @brief Costruisce il path completo del file di salvataggio per l'utente
 *        correntemente loggato e lo slot specificato.
 *
 * Utilizza snprintf per sicurezza contro buffer overflow.
 * Il formato include l'ID utente per separare i salvataggi
 * tra diversi profili sulla stessa macchina.
 *
 * @param buffer Buffer di destinazione.
 * @param bufsize Dimensione del buffer.
 * @param slot Indice dello slot (1-100).
 */
static void GetSlotPath(char* buffer, size_t bufsize, int slot) {
    snprintf(buffer, bufsize, "data/games/save_user%d_slot_%d.dat",
             id_utente_corrente, slot);
}

/**
 * @brief Confronta due stringhe di data/ora per ordinamento cronologico.
 *
 * Formato atteso: "DD/MM/YYYY HH:MM:SS" (o variante senza secondi).
 * Se una data e' vuota, viene considerata PIU' VECCHIA.
 *
 * @param a Prima data (stringa).
 * @param b Seconda data (stringa).
 * @return -1 se a < b, 0 se uguali, 1 se a > b.
 */
static int ConfrontaDate(const char* a, const char* b) {
    if (!a || !b || a[0] == '\0' || b[0] == '\0') return 0;
    int g1, m1, y1, h1, min1, s1, g2, m2, y2, h2, min2, s2;
    int r1 = sscanf(a, "%d/%d/%d %d:%d:%d", &g1, &m1, &y1, &h1, &min1, &s1);
    int r2 = sscanf(b, "%d/%d/%d %d:%d:%d", &g2, &m2, &y2, &h2, &min2, &s2);
    if (r1 < 5 || r2 < 5) return 0;
    if (y1 != y2) return y1 < y2 ? -1 : 1;
    if (m1 != m2) return m1 < m2 ? -1 : 1;
    if (g1 != g2) return g1 < g2 ? -1 : 1;
    if (h1 != h2) return h1 < h2 ? -1 : 1;
    if (min1 != min2) return min1 < min2 ? -1 : 1;
    if (r1 >= 6 && r2 >= 6 && s1 != s2) return s1 < s2 ? -1 : 1;
    return 0;
}

/**
 * @brief Conta quanti salvataggi appartengono all'utente corrente.
 * @return Numero di file .dat con proprietario = utente corrente.
 */
static int ContaSlotUtente(void) {
    if (id_utente_corrente < 0 || id_utente_corrente >= dbUtenti.num_utenti) return 0;
    const char* currentUser = dbUtenti.lista[id_utente_corrente].username;
    int count = 0;
    for (int i = 1; i <= 100; i++) {
        char percorso[70];
        GetSlotPath(percorso, sizeof(percorso), i);
        FILE* f = fopen(percorso, "rb");
        if (!f) continue;
        StatoGioco temp;
        size_t r = fread(&temp, sizeof(StatoGioco), 1, f);
        fclose(f);
        if (r != 1 || temp.giocatori[0].nome[0] == '\0') continue;
        if (strcmp(temp.giocatori[0].nome, currentUser) == 0) count++;
    }
    return count;
}

/**
 * @brief Salva lo stato corrente della partita su file binario.
 *
 * Procedura:
 *   1. Compatta i salvataggi esistenti dell'utente
 *   2. Genera il timestamp corrente
 *   3. Trova il primo slot libero
 *   4. Serializza le mani su array di backup
 *   5. Scrive lo StatoGioco su file .dat
 *   6. Ricostruisce gli ADT
 *   7. Broadcast LAN dei nuovi salvataggi
 *
 * @param gioco Puntatore allo stato di gioco da salvare.
 */
void SalvaPartita(StatoGioco *gioco) {
    /* BUG FIX: NON bloccare il salvataggio in multiplayer.
     * Se siamo host (NET_HOST), possiamo salvare la partita.
     * Se siamo client (NET_CLIENT), il salvataggio viene gestito dall'host.
     * Se siamo offline (NET_OFFLINE), salviamo normalmente. */
    if (currentRole == NET_CLIENT) return;

    /* Compatta prima di salvare: migra i vecchi salvataggi generici
     * nel nuovo schema con indice utente, così abbiamo una visione
     * coerente degli slot realmente occupati. */
    CompattaSalvataggiUtente();

    /* BUG 1 FIX: Genera il timestamp SEMPRE prima di qualsiasi altra operazione
     * che potrebbe usare data_salvataggio. Questo timestamp viene usato come
     * chiave primaria per la deduplicazione dei salvataggi durante la
     * sincronizzazione LAN (LAN_BroadcastSavesNow -> InviaSalvataggioChunked).
     * Se il timestamp e' vuoto o inconsistente, la sincronizzazione LAN fallisce
     * e i salvataggi non vengono propagati correttamente tra i dispositivi. */
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    gioco->data_salvataggio[0] = '\0'; /* Pulisce prima di scrivere per evitare residui */
    sprintf(gioco->data_salvataggio, "%02d/%02d/%d %02d:%02d:%02d",
            tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
            tm.tm_hour, tm.tm_min, tm.tm_sec);
    gioco->data_salvataggio[sizeof(gioco->data_salvataggio) - 1] = '\0'; /* Terminatore sicuro */

    {
        int sovrascrivi = -1;
        int slot_trovato = 0;
        for (int i = 1; i <= 100; i++) {
            char p[70];
            GetSlotPath(p, sizeof(p), i);
            FILE* f = fopen(p, "rb");
            if (!f) { slot_trovato = i; break; }
            StatoGioco es; size_t r = fread(&es, sizeof(StatoGioco), 1, f); fclose(f);
            if (r != 1 || es.giocatori[0].nome[0] == '\0') { slot_trovato = i; break; }
            if (strcmp(es.giocatori[0].nome, gioco->giocatori[0].nome) == 0 &&
                strcmp(es.data_salvataggio, gioco->data_salvataggio) == 0) {
                sovrascrivi = i; break;
            }
        }
        if (sovrascrivi != -1) gioco->slot_salvataggio = sovrascrivi;
        else if (slot_trovato) gioco->slot_salvataggio = slot_trovato;
        else {
            /* Tutti gli slot sono pieni: sovrascrivi il salvataggio più vecchio */
            int slot_piu_vecchio = -1;
            char data_piu_vecchia[30] = "";
            for (int s = 1; s <= 100; s++) {
                char check[70];
                GetSlotPath(check, sizeof(check), s);
                FILE* fc = fopen(check, "rb");
                if (!fc) continue;
                StatoGioco es;
                size_t r = fread(&es, sizeof(StatoGioco), 1, fc);
                fclose(fc);
                if (r != 1 || es.giocatori[0].nome[0] == '\0') continue;
                if (strcmp(es.giocatori[0].nome, gioco->giocatori[0].nome) != 0) continue;
                if (slot_piu_vecchio == -1 || ConfrontaDate(es.data_salvataggio, data_piu_vecchia) < 0) {
                    slot_piu_vecchio = s;
                    strncpy(data_piu_vecchia, es.data_salvataggio, sizeof(data_piu_vecchia) - 1);
                }
            }
            if (slot_piu_vecchio != -1) gioco->slot_salvataggio = slot_piu_vecchio;
            else return; /* Nessun salvataggio dell'utente: impossibile */
        }
    }

    for (int i = 0; i < gioco->num_giocatori; i++) {
        Giocatore *g = &gioco->giocatori[i];
        g->num_carte_backup = 0;
        if (g->mano) {
            NodoCarta *n = g->mano->testa;
            int idx = 0;
            while (n && idx < 108) {
                g->mano_backup[idx++] = n->carta;
                n = n->prossimo;
            }
            g->num_carte_backup = idx;
        }
    }
    gioco->nodo_carta_trascinata = NULL;
    gioco->id_carta_in_trascinamento = -1;
    for (int i = 0; i < gioco->num_giocatori; i++) {
        gioco->giocatori[i].mano = NULL;
    }

    fs_mkdirs();

    // BUG 3 FIX: Salva la modalita' di gioco NEL file di salvataggio
    // per ripristinarla correttamente al caricamento (es. "1 vs 3", "Online").
    if (gioco->modalita[0] == '\0') {
        if (currentRole != NET_OFFLINE) strcpy(gioco->modalita, "Online");
        else sprintf(gioco->modalita, "1 vs %d", gioco->num_giocatori - 1);
    }

    char percorso[70];
    GetSlotPath(percorso, sizeof(percorso), gioco->slot_salvataggio);
    FILE *f = fopen(percorso, "wb");
    if (f) { fwrite(gioco, sizeof(StatoGioco), 1, f); fclose(f); }

    RicostruisciStatoADT(gioco);
    LAN_BroadcastSavesNow();
}

/**
 * @brief Carica una partita da uno slot di salvataggio specifico.
 *
 * Cerca prima nel formato save_user{ID}_slot_{N}.dat.
 * Fallback: se non trova, controlla nel vecchio formato save_slot_N.dat
 * per retrocompatibilità con salvataggi preesistenti.
 *
 * @param gioco Puntatore allo stato di gioco da popolare.
 * @param slot Indice dello slot (1-100).
 * @return 1 se caricamento riuscito, 0 se slot vuoto o errore.
 */
int CaricaPartitaDaSlot(StatoGioco *gioco, int slot) {
    char percorso[70];
    GetSlotPath(percorso, sizeof(percorso), slot);
    FILE *f = fopen(percorso, "rb");
    if (!f) {
        /* Fallback al vecchio schema save_slot_N.dat per retrocompatibilità */
        sprintf(percorso, "data/games/save_slot_%d.dat", slot);
        f = fopen(percorso, "rb");
    }
    if (f) {
        fread(gioco, sizeof(StatoGioco), 1, f);
        fclose(f);
        gioco->nodo_carta_trascinata = NULL;
        gioco->id_carta_in_trascinamento = -1;
        for (int i = 0; i < gioco->num_giocatori; i++) {
            gioco->giocatori[i].mano = NULL;
            if (gioco->giocatori[i].num_carte_backup < 0 ||
                gioco->giocatori[i].num_carte_backup > 108) {
                gioco->giocatori[i].num_carte_backup = 0;
            }
        }
        return 1;
    }
    return 0;
}

/**
 * @brief Compatta i salvataggi dell'utente loggato in ordine cronologico.
 *
 * Raccoglie tutti i salvataggi dell'utente (cerca sia formato nuovo
 * save_user{ID}_slot_{N}.dat che vecchio save_slot_N.dat),
 * li ordina per data crescente (bubble sort) e li riscrive
 * nei primi slot liberi con il nuovo formato.
 */
void CompattaSalvataggiUtente(void) {
    if (id_utente_corrente < 0 || id_utente_corrente >= dbUtenti.num_utenti) return;
    const char* currentUser = dbUtenti.lista[id_utente_corrente].username;

    /* BUG 14 FIX: Usa array STATIC invece che stack allocation.
     * TempSave (100 bytes) x 100 = 10KB. Su Windows, lo stack di ogni thread
     * e' SOLO 1MB di default. Se questa funzione viene chiamata ricorsivamente
     * o da funzioni con grandi frame, si rischia overflow dello stack.
     * Con static, i dati finiscono nel segmento DATA/relocated section. */
    static TempSave temps[100];
    static int tempCounterGlobal = 0;
    int count = 0;
    tempCounterGlobal = 0;

    /* Fase 1: raccogli tutti i salvataggi dell'utente da entrambi i formati */
    for (int fmt = 0; fmt < 2; fmt++) {
        for (int i = 1; i <= 100; i++) {
            char percorso[70];
            if (fmt == 0) {
                GetSlotPath(percorso, sizeof(percorso), i);
            } else {
                sprintf(percorso, "data/games/save_slot_%d.dat", i);
            }
            FILE* f = fopen(percorso, "rb");
            if (!f) continue;
            StatoGioco temp;
            size_t r = fread(&temp, sizeof(StatoGioco), 1, f);
            fclose(f);
            if (r != 1 || temp.giocatori[0].nome[0] == '\0') continue;
            if (strcmp(temp.giocatori[0].nome, currentUser) != 0) continue;

            char temp_percorso[50];
            sprintf(temp_percorso, "data/games/temp_save_%d.dat", tempCounterGlobal++);
            rename(percorso, temp_percorso);
            strncpy(temps[count].percorso, temp_percorso, sizeof(temps[count].percorso) - 1);
            strncpy(temps[count].data, temp.data_salvataggio, sizeof(temps[count].data) - 1);
            temps[count].percorso[sizeof(temps[count].percorso) - 1] = '\0';
            temps[count].data[sizeof(temps[count].data) - 1] = '\0';
            count++;
        }
    }

    if (count == 0) return;

    /* Fase 2: bubble sort cronologico */
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (ConfrontaDate(temps[j].data, temps[i].data) < 0) {
                TempSave tmp = temps[i]; temps[i] = temps[j]; temps[j] = tmp;
            }
        }
    }

    /* Fase 3: riscrivi nel nuovo formato ordinato */
    for (int t = 0; t < count; t++) {
        int slot_dest = -1;
        for (int s = 1; s <= 100 && slot_dest == -1; s++) {
            char check[70];
            GetSlotPath(check, sizeof(check), s);
            FILE* fc = fopen(check, "rb");
            if (!fc) { slot_dest = s; }
            else fclose(fc);
        }
        /* Se non ci sono slot liberi, il salvataggio viene perso: NON sovrascrivere */
        if (slot_dest == -1) { continue; }

        FILE* ft = fopen(temps[t].percorso, "rb");
        if (!ft) continue;
        StatoGioco save;
        size_t r = fread(&save, sizeof(StatoGioco), 1, ft);
        fclose(ft);
        remove(temps[t].percorso);
        if (r != 1) continue;
        save.slot_salvataggio = slot_dest;
        char nuovo_percorso[70];
        GetSlotPath(nuovo_percorso, sizeof(nuovo_percorso), slot_dest);
        FILE* fn = fopen(nuovo_percorso, "wb");
        if (fn) {
            fwrite(&save, sizeof(StatoGioco), 1, fn);
            fclose(fn);
        }
    }
}

/**
 * @brief Elimina un salvataggio e sincronizza via LAN.
 *
 * Legge i metadati (owner, data) prima di eliminare, poi:
 * rimuove il file, compatta, notifica altri dispositivi LAN.
 * Cerca nel formato save_user{ID}_slot_{N}.dat.
 *
 * @param slot Indice dello slot da eliminare.
 */
void EliminaSalvataggio(int slot) {
    char data_salvataggio[30] = "";
    char owner_username[30] = "";
    char percorso[70];
    GetSlotPath(percorso, sizeof(percorso), slot);
    FILE* f = fopen(percorso, "rb");
    if (f) {
        StatoGioco tmp;
        size_t r = fread(&tmp, sizeof(StatoGioco), 1, f);
        fclose(f);
        if (r == 1 && tmp.giocatori[0].nome[0] != '\0') {
            strncpy(owner_username, tmp.giocatori[0].nome, sizeof(owner_username)-1);
            strncpy(data_salvataggio, tmp.data_salvataggio, sizeof(data_salvataggio)-1);
        }
    }
    remove(percorso);
    CompattaSalvataggiUtente();
    if (owner_username[0] && data_salvataggio[0]) {
        LAN_BroadcastDeleteSave(owner_username, data_salvataggio);
    }
    LAN_BroadcastSavesNow();
}

/**
 * @brief Elimina TUTTI i salvataggi di un utente specifico.
 *
 * Scandisce tutti gli slot 1-100 e rimuove i file .dat
 * il cui proprietario corrisponde allo username specificato.
 * Controlla sia il formato save_user{ID}_slot_{N}.dat
 * che il vecchio save_slot_N.dat.
 *
 * @param username Username dell'utente i cui salvataggi vanno eliminati.
 */
void EliminaTuttiSalvataggiUtente(const char* username) {
    if (!username || username[0] == '\0') return;
    
    /* Cerca in tutti i file save_slot_N.dat (vecchio formato) */
    for (int i = 1; i <= 100; i++) {
        char percorso[70];
        sprintf(percorso, "data/games/save_slot_%d.dat", i);
        FILE* f = fopen(percorso, "rb");
        if (f) {
            StatoGioco temp;
            size_t r = fread(&temp, sizeof(StatoGioco), 1, f);
            fclose(f);
            if (r == 1 && temp.giocatori[0].nome[0] != '\0' &&
                strcmp(temp.giocatori[0].nome, username) == 0) {
                remove(percorso);
            }
        }
    }
    
    /* Cerca in tutti i file save_user*_slot_*.dat (nuovo formato) */
    /* Nota: non conosciamo l'ID utente, quindi usiamo pattern generico */
    for (int uid = 0; uid < 100; uid++) {
        for (int i = 1; i <= 100; i++) {
            char percorso[70];
            snprintf(percorso, sizeof(percorso), "data/games/save_user%d_slot_%d.dat", uid, i);
            FILE* f = fopen(percorso, "rb");
            if (f) {
                StatoGioco temp;
                size_t r = fread(&temp, sizeof(StatoGioco), 1, f);
                fclose(f);
                if (r == 1 && temp.giocatori[0].nome[0] != '\0' &&
                    strcmp(temp.giocatori[0].nome, username) == 0) {
                    remove(percorso);
                }
            }
        }
    }
}

/**
 * @brief Restituisce il path del file di salvataggio per un dato utente e slot.
 *
 * Implementazione della funzione dichiarata in game_logic.h per compatibilità.
 * Formato: data/games/save_user{ID}_slot_{N}.dat
 *
 * @param user_index Indice dell'utente nel db (non usato, si usa id_utente_corrente).
 * @param slot Indice dello slot (1-100).
 * @param buffer Buffer di destinazione.
 * @param bufsize Dimensione del buffer.
 */
void GetSaveFilePath(int user_index, int slot, char* buffer, size_t bufsize) {
    GetSlotPath(buffer, bufsize, slot);
}