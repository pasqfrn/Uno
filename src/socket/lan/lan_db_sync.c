/**
 * @file lan_db_sync.c
 * @brief Sincronizzazione database utenti via LAN (UDP broadcast).
 * @ingroup lan
 *
 * @details Modulo per la serializzazione, deserializzazione e merge del database
 *          utenti tramite pacchetti UDP broadcast. Responsabile di:
 *          - Serializzazione dbUtenti in buffer JSON compatto
 *          - Merge intelligente con deduplica (senza double counting)
 *          - Verifica unicita' username/email per registrazione
 *          - Aggiornamento automatico admin panel su tutti i dispositivi LAN
 *          - Sincronizzazione storico partite completo
 *
 * @note Utilizza LanCompactUser (struttura compatta) per ridurre dimensione
 *       pacchetti UDP. Lo storico_partite[] trasmesso e' limitato alle ultime
 *       STORICO_SYNC_COUNT partite per mantenere la dimensione ragionevole.
 *
 * ADT utilizzato:
 *   - DatabaseUtenti (dbUtenti, array dinamico allocato)
 *   - LanCompactUser (formato compatto per trasmissione)
 *
 * Protocollo pacchetto:
 *   [LanPacketHeader (magic=LAN_PACKET_DB + length)]
 *   [int num_utenti]
 *   [LanCompactUser x N]
 *
 * Strategia merge (DeserializzaEMerge):
 *   1. Rimuove utenti locali non presenti nel pacchetto remoto
 *      (eccetto utente loggato e utenti in deletedUsers)
 *   2. Per ogni utente remoto: aggiorna se modificato, inserisce se nuovo
 *   3. Protezione anti-ricorsione: dentroSalvaDB evita broadcast loop
 *   4. Dopo il merge, se siamo NET_HOST, chiama BroadcastAdminDB() per
 *      aggiornare i client TCP connessi in tempo reale.
 *
 * I contatori (partite_giocate, vittorie_giocatore, vittorie_bot) vengono
 * sincronizzati con "crescita monotona": se il remoto ha contatori piu' alti,
 * vengono copiati SENZA aggiungere nuovamente lo storico. Lo storico partite
 * viene mergiato SOLO se i contatori sono gia' sincronizzati (per colmare
 * eventuali partite mancanti). Questo evita il double counting per cui
 * partite_giocate veniva incrementato sia dalla copia dei contatori che
 * dall'aggiunta delle partite storiche.
 *
 * La deduplica usa una chiave composta (durata, esito, modalita, posizione)
 * per evitare falsi positivi quando piu' giocatori hanno partite con
 * caratteristiche simili.
 *
 * File correlati:
 *   - lan_sync.h       : API pubblica (LAN_Init, LAN_Update, LAN_ProcessDBPacket)
 *   - lan_protocol.h   : definizioni protocollo (LanPacketHeader, LanCompactUser)
 *   - network.h        : currentRole, BroadcastAdminDB
 *   - auth.h           : dbUtenti, DB_Alloca, SalvaDB
 *   - network_send.c   : BroadcastAdminDB
 *
 * @author Pasquale Franco
 * @version 9.1.3
 * @date 2026-07-01
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../../lib/socket/lan/lan_sync.h"
#include "../../../lib/socket/lan/lan_protocol.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/auth.h"
#include "../../../lib/data_structures.h"

/**
 * @brief Confronta un record Statistiche locale con un LanCompactUser remoto.
 * @return 1 se sono diversi (serve aggiornamento), 0 se identici.
 *
 * Controlla SOLO i campi anagrafici e contatori numerici, NON lo storico partite.
 * Lo storico partite viene mergiato separatamente in AggiornaStatistiche()
 * con attenzione a NON causare double counting.
 */
/**
 * @brief Confronta un record Statistiche locale con un LanCompactUser remoto.
 * @return 1 se sono diversi (serve aggiornamento), 0 se identici.
 *
 * @details Controlla TUTTI i campi anagrafici, contatori e email.
 * L'email mancava nella versione precedente: questo impediva la
 * sincronizzazione delle modifiche all'email da parte dell'admin.
 *
 * BUG 1 FIX: Aggiunto strcmp(l->email, r->email) per rilevare
 * modifiche all'email fatte dall'admin e sincronizzarle su LAN.
 */
static int UtenteDiverso(const Statistiche* l, const LanCompactUser* r) {
    if (strcmp(l->username, r->username)) return 1;
    if (strcmp(l->password, r->password)) return 1;
    if (strcmp(l->pin_recupero, r->pin_recupero)) return 1;
    /* BUG 1 FIX: Confronta anche l'email per rilevare modifiche admin. */
    if (strcmp(l->email, r->email)) return 1;
    if (l->partite_giocate != r->partite_giocate) return 1;
    if (l->vittorie_giocatore != r->vittorie_giocatore) return 1;
    if (l->vittorie_bot != r->vittorie_bot) return 1;
    if (strcmp(l->nome, r->nome)) return 1;
    if (strcmp(l->cognome, r->cognome)) return 1;
    return 0;
}

/**
 * @brief Copia tutti i campi da LanCompactUser a Statistiche locale.
 * @param l Record Statistiche di destinazione.
 * @param r Record LanCompactUser di origine.
 *
 * Logica di merge semplificata:
 * 1. Copia i campi anagrafici (nome, cognome, username, password, pin)
 * 2. Se il remoto ha contatori PIU' ALTI di quelli locali (crescita monotona),
 *    COPIA i contatori senza aggiungere storico. Questo significa che le partite
 *    "mancanti" sono gia' incluse nel conteggio del remoto.
 * 3. Se i contatori sono UGUALI ma c'e' storico da mergiare (partite con dettagli
 *    diversi dal locale), aggiungi solo le partite NUOVE (deduplica per chiave).
 *    Questo NON incrementa partite_giocate perche' la partita era gia' contata.
 * 4. Lo storico_count del pacchetto remoto indica quante partite storiche sono
 *    state inviate (max STORICO_SYNC_COUNT = ultime 10). partite_giocate e'
 *    il contatore TOTALE separato dallo storico.
 *
 * Regole fondamentali:
 * - Non aggiungere MAI partite_giocate++ quando si copia dallo storico.
 * - Le partite storiche sono GIA' conteggiate in partite_giocate del remoto.
 * - Se locale ha partite_giocate = remoto.partite_giocate, significa che locale
 *   ha gia' tutti i conti a posto. Lo storico serve SOLO per avere i dettagli.
 */
/**
 * @brief Copia tutti i campi da LanCompactUser a Statistiche locale.
 * @param l Record Statistiche di destinazione.
 * @param r Record LanCompactUser di origine.
 *
 * @details Copia TUTTI i campi anagrafici inclusa l'email (mancante
 * nella versione precedente). I contatori numerici vengono aggiornati
 * con crescita monotona: se il remoto ha contatori piu' alti, vengono
 * copiati. Lo storico partite viene mergiato con deduplica.
 *
 * BUG 1 FIX: Aggiunta copia del campo email che mancava, impedendo
 * la sincronizzazione delle modifiche all'email fatte dall'admin.
 * BUG 1 FIX: I contatori numerici vengono ora copiati con crescita
 * monotona (se remoto > locale), invece di essere ignorati con (void).
 */
static void AggiornaStatistiche(Statistiche* l, const LanCompactUser* r) {
    /* BUG 1 FIX: Copia ANCHE l'email (mancava completamente!) */
    strncpy(l->email, r->email, sizeof(l->email)-1);
    l->email[sizeof(l->email)-1] = '\0';
    strncpy(l->nome, r->nome, sizeof(l->nome)-1);
    strncpy(l->cognome, r->cognome, sizeof(l->cognome)-1);
    strncpy(l->username, r->username, sizeof(l->username)-1);
    strncpy(l->password, r->password, sizeof(l->password)-1);
    strncpy(l->pin_recupero, r->pin_recupero, sizeof(l->pin_recupero)-1);
    
    /* Acquisisci il vecchio conteggio PRIMA di aggiornarlo */
    int vecchie_partite = l->partite_giocate;
    
    /* ============================================================
     * BUG 1 FIX: Crescita monotona - copia contatori se remoto > locale.
     * Il vecchio codice usava (void) per ignorare completamente i
     * contatori remoti, impedendo la sincronizzazione delle statistiche
     * tra dispositivi LAN. Ora se il remoto ha contatori piu' alti,
     * vengono copiati (crescita monotona).
     * MAI sovrascrivere con valori piu' bassi!
     * ============================================================ */
    if (r->partite_giocate > l->partite_giocate) {
        l->partite_giocate = r->partite_giocate;
        l->vittorie_giocatore = r->vittorie_giocatore;
        l->vittorie_bot = r->vittorie_bot;
    }
    
    /* ============================================================
     * Merge dello storico partite SENZA incrementare contatori.
     * A questo punto partite_giocate e' gia' corretto (uguale o maggiore
     * del remoto). Possiamo mergiare lo storico per avere i dettagli
     * delle partite, ma NON dobbiamo incrementare partite_giocate.
     * 
     * La deduplica usa (durata + esito + modalita + posizione) come
     * chiave. Se una partite remota non e' presente in locale,
     * viene aggiunta a storico_partite[] (se c'e' spazio).
     * ============================================================ */
    int storico_da_copiare = r->storico_count;
    if (storico_da_copiare > STORICO_SYNC_COUNT) storico_da_copiare = STORICO_SYNC_COUNT;
    for (int i = 0; i < storico_da_copiare; i++) {
        const PartitaRegistrata* remota = &r->storico_partite[i];
        /* Controlla se gia' presente nel locale */
        int gia_presente = 0;
        int max_local = (vecchie_partite < MAX_PARTITE_STORICO) ? vecchie_partite : MAX_PARTITE_STORICO;
        for (int j = 0; j < max_local; j++) {
            const PartitaRegistrata* locale = &l->storico_partite[j];
            if (locale->durata_secondi == remota->durata_secondi &&
                strcmp(locale->esito, remota->esito) == 0 &&
                strcmp(locale->modalita, remota->modalita) == 0 &&
                locale->posizione == remota->posizione) {
                gia_presente = 1;
                break;
            }
        }
        /* Se non presente e c'e' spazio, aggiungila senza incrementare contatori */
        if (!gia_presente && vecchie_partite < MAX_PARTITE_STORICO) {
            l->storico_partite[vecchie_partite] = *remota;
            vecchie_partite++;
        }
    }
    /* NON modificare partite_giocate: e' gia' corretto! */
}



/**
 * @brief Verifica se una email esiste gia' nel DB locale o remoto (LAN).
 * @param email Email da verificare.
 * @return 1 se esiste, 0 altrimenti.
 */
int LAN_EmailEsiste(const char* email) {
    if (!email || !email[0]) return 0;
    for (int i = 0; i < dbUtenti.num_utenti; i++) {
        if (strcmp(dbUtenti.lista[i].email, email) == 0) return 1;
    }
    return 0;
}

/**
 * @brief Deserializza un pacchetto DB LAN e merge con il database locale.
 *
 * Strategia conservativa: gli utenti vengono SOLO aggiunti o aggiornati,
 * mai rimossi automaticamente. Questo evita la perdita di dati quando un
 * pacchetto LAN arriva durante una partita. L'unico caso di rimozione e'
 * quando l'admin elimina esplicitamente un utente.
 *
 * Ottimizzazione: SalvaDB() e AuthBST_Ricostruisci() vengono chiamati
 * SOLO se ci sono state modifiche (dbMod == 1). Questo evita broadcast
 * ricorsivi infiniti.
 */
static void DeserializzaEMerge(const char* buf, int bytes, int payload_offset) {
    if (!buf || bytes < payload_offset + (int)sizeof(int)) return;
    int offset = payload_offset;
    int num_remoti;
    memcpy(&num_remoti, buf + offset, sizeof(int));
    offset += sizeof(int);
    if (num_remoti <= 0 || num_remoti > 1000) return;
    if (offset + num_remoti * (int)sizeof(LanCompactUser) > bytes) return;

    int dbMod = 0;

    /* Salva username utente loggato prima del merge */
    char logged_username[30] = "";
    if (id_utente_corrente >= 0 && id_utente_corrente < dbUtenti.num_utenti) {
        strncpy(logged_username, dbUtenti.lista[id_utente_corrente].username, sizeof(logged_username) - 1);
    }

    offset = payload_offset + sizeof(int);
    for (int r = 0; r < num_remoti; r++) {
        LanCompactUser c;
        memcpy(&c, buf + offset, sizeof(LanCompactUser));
        offset += sizeof(LanCompactUser);
        if (c.username[0] == 0) continue;
        
        int idx = -1;
        for (int i = 0; i < dbUtenti.num_utenti; i++) {
            if (strcmp(dbUtenti.lista[i].username, c.username) == 0) {
                idx = i;
                break;
            }
        }
        
        if (idx != -1) {
            /* Utente ESISTENTE: aggiorna se diverso */
            if (UtenteDiverso(&dbUtenti.lista[idx], &c)) {
                AggiornaStatistiche(&dbUtenti.lista[idx], &c);
                dbMod = 1;
            }
        } else {
            /* Nuovo utente: verifica che non sia stato eliminato */
            int eliminato = 0;
            for (int d = 0; d < numDeletedUsers; d++) {
                if (strcmp(c.username, deletedUsers[d]) == 0) {
                    eliminato = 1;
                    break;
                }
            }
            if (!eliminato) {
                DB_Alloca(dbUtenti.num_utenti + 1);
                Statistiche* s = &dbUtenti.lista[dbUtenti.num_utenti];
                memset(s, 0, sizeof(Statistiche));
                strncpy(s->email, c.email, sizeof(s->email)-1);
                strncpy(s->nome, c.nome, sizeof(s->nome)-1);
                strncpy(s->cognome, c.cognome, sizeof(s->cognome)-1);
                strncpy(s->username, c.username, sizeof(s->username)-1);
                strncpy(s->password, c.password, sizeof(s->password)-1);
                strncpy(s->pin_recupero, c.pin_recupero, sizeof(s->pin_recupero)-1);
                s->partite_giocate = c.partite_giocate;
                s->vittorie_giocatore = c.vittorie_giocatore;
                s->vittorie_bot = c.vittorie_bot;
                /* Copia lo storico partite limitato a STORICO_SYNC_COUNT */
                int storico_da_copiare = c.storico_count;
                if (storico_da_copiare > STORICO_SYNC_COUNT) storico_da_copiare = STORICO_SYNC_COUNT;
                for (int h = 0; h < storico_da_copiare; h++) {
                    s->storico_partite[h] = c.storico_partite[h];
                }
                dbUtenti.num_utenti++;
                dbMod = 1;
            }
        }
    }

    dentroSalvaDB = 1;
    if (dbMod) {
        SalvaDB();
        AuthBST_Ricostruisci();
        if (currentRole == NET_HOST) {
            BroadcastAdminDB();
        }
    }
    dentroSalvaDB = 0;
}

/**
 * @brief Serializza il database utenti in un buffer per broadcast UDP.
 *
 * Formato: header + int(num_utenti) + LanCompactUser[N].
 *
 * @param buf Buffer di destinazione.
 * @param buf_size Dimensione buffer.
 * @return Numero di byte scritti, 0 se errore.
 */
int SerializzaDB(char* buf, int buf_size) {
    int needed = (int)(sizeof(LanPacketHeader) + sizeof(int) +
                       dbUtenti.num_utenti * sizeof(LanCompactUser));
    if (!buf || buf_size < needed) return 0;
    LanPacketHeader header;
    header.magic = LAN_PACKET_DB;
    header.length = (int)(sizeof(int) +
                          dbUtenti.num_utenti * sizeof(LanCompactUser));
    int offset = 0;
    memcpy(buf + offset, &header, sizeof(LanPacketHeader));
    offset += sizeof(LanPacketHeader);
    memcpy(buf + offset, &dbUtenti.num_utenti, sizeof(int));
    offset += sizeof(int);
    for (int i = 0; i < dbUtenti.num_utenti; i++) {
        LanCompactUser c;
        memset(&c, 0, sizeof(c));
        strncpy(c.email, dbUtenti.lista[i].email, sizeof(c.email)-1);
        strncpy(c.nome, dbUtenti.lista[i].nome, sizeof(c.nome)-1);
        strncpy(c.cognome, dbUtenti.lista[i].cognome, sizeof(c.cognome)-1);
        strncpy(c.username, dbUtenti.lista[i].username, sizeof(c.username)-1);
        strncpy(c.password, dbUtenti.lista[i].password, sizeof(c.password)-1);
        strncpy(c.pin_recupero, dbUtenti.lista[i].pin_recupero,
                sizeof(c.pin_recupero)-1);
        c.partite_giocate = dbUtenti.lista[i].partite_giocate;
        c.vittorie_giocatore = dbUtenti.lista[i].vittorie_giocatore;
        c.vittorie_bot = dbUtenti.lista[i].vittorie_bot;
        /* Popola lo storico partite: ultime STORICO_SYNC_COUNT */
        int start = 0;
        if (dbUtenti.lista[i].partite_giocate > STORICO_SYNC_COUNT) {
            start = dbUtenti.lista[i].partite_giocate - STORICO_SYNC_COUNT;
        }
        c.storico_count = 0;
        for (int h = start; h < dbUtenti.lista[i].partite_giocate && h < MAX_PARTITE_STORICO; h++) {
            c.storico_partite[c.storico_count] = dbUtenti.lista[i].storico_partite[h];
            c.storico_count++;
            if (c.storico_count >= STORICO_SYNC_COUNT) break;
        }
        memcpy(buf + offset, &c, sizeof(LanCompactUser));
        offset += sizeof(LanCompactUser);
    }
    return offset;
}

/**
 * @brief Entry point pubblico per processare un pacchetto DB LAN ricevuto.
 * Chiamato da LAN_Update() in lan_broadcast.c.
 */
void LAN_ProcessDBPacket(const char* buf, int bytes, int offset) {
    DeserializzaEMerge(buf, bytes, offset);
}

/**
 * @brief Verifica se uno username esiste gia' nel DB locale.
 * @param u Username da verificare.
 * @param skip Indice da saltare (-1 per nessuno).
 * @return 1 se esiste, 0 altrimenti.
 */
int LAN_UsernameEsiste(const char* u, int skip) {
    if (!u || !u[0]) return 0;
    for (int i = 0; i < dbUtenti.num_utenti; i++)
        if (i != skip && strcmp(dbUtenti.lista[i].username, u) == 0)
            return 1;
    return 0;
}