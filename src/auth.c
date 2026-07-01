/**
 * @file auth.c
 * @brief Implementazione modulo autenticazione e database utenti (allocazione dinamica).
 * @ingroup auth
 *
 * Fornisce le funzioni per la gestione del database utenti in memoria,
 * con allocazione DINAMICA tramite malloc/realloc. Il database viene
 * caricato da file all'avvio e salvato ad ogni modifica.
 *
 * Supporta anche un BST (Binary Search Tree) parallelo per ricerca
 * logaritmica O(log n) dell'email.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../lib/auth.h"
#include "../lib/auth_validation.h"
#include "../lib/data_structures.h"
#include "../lib/data_structures/bst.h"
#include "../lib/game_logic.h"
#include "../lib/socket/lan/lan_sync.h"
#include "../lib/screens/menu/string_utils.h"
#include "../lib/fs/fs.h"

/**
 * @addtogroup auth_impl Implementazioni Auth
 * @brief Implementazioni delle funzionalità di autenticazione e database.
 * @{
 */

/** @brief Database utenti globale in memoria (allocazione dinamica). */
DatabaseUtenti dbUtenti = {0};
/** @brief Indice dell'utente correntemente loggato (-1 se nessuno). */
int id_utente_corrente = -1;

char inputEmail[100] = "\0";
int emailCount = 0;
char inputNome[30] = "\0";
int nomeCount = 0;
char inputCognome[30] = "\0";
int cognomeCount = 0;
char inputUsername[30] = "\0";
int usernameCount = 0;
char inputPassword[30] = "\0";
int passCount = 0;
char inputPin[7] = "\0";
int pinCount = 0;
int id_temp_login = -1;
int errore_password = 0;
int errore_registrazione_pass = 0;
int errore_registrazione_email = 0;
int errore_registrazione_username = 0;
int campo_focusato = 0;

/** @brief BST parallelo per ricerca O(log n) per email. */
static NodoAlbero* utenti_bst = NULL;

/**
 * @addtogroup lazy_deletion_impl Lazy Deletion Implementazione
 * @brief Strutture e funzioni per eliminazione differita utenti.
 * @{
 */
/** @brief Numero di eliminazioni pendenti correnti. */
static int num_pending_deletions_auth = 0;
/** @brief Array di email marcate per eliminazione differita. */
static char pending_deletions_auth[MAX_PENDING_DELETIONS][100] = {{0}};

int Auth_MarkForDeletion(const char* email) {
    if (!email || !email[0]) return 0;
    if (num_pending_deletions_auth >= MAX_PENDING_DELETIONS) return 0;
    for (int i = 0; i < num_pending_deletions_auth; i++) {
        if (strcmp(pending_deletions_auth[i], email) == 0) return 0;
    }
    strncpy(pending_deletions_auth[num_pending_deletions_auth], email, 99);
    pending_deletions_auth[num_pending_deletions_auth][99] = '\0';
    num_pending_deletions_auth++;
    return 1;
}

int Auth_IsMarkedForDeletion(const char* email) {
    if (!email || !email[0]) return 0;
    for (int i = 0; i < num_pending_deletions_auth; i++) {
        if (strcmp(pending_deletions_auth[i], email) == 0) return 1;
    }
    return 0;
}

void Auth_ProcessPendingDeletions(const char* email) {
    if (!email || !email[0]) return;
    if (!Auth_IsMarkedForDeletion(email)) return;
    int idx = -1;
    for (int i = 0; i < dbUtenti.num_utenti; i++) {
        if (strcmp(dbUtenti.lista[i].email, email) == 0) { idx = i; break; }
    }
    if (idx < 0) return;
    EliminaTuttiSalvataggiUtente(dbUtenti.lista[idx].username);
    extern int numDeletedUsers;
    extern char deletedUsers[512][30];
    if (numDeletedUsers < 512) {
        strncpy(deletedUsers[numDeletedUsers], dbUtenti.lista[idx].username, 29);
        deletedUsers[numDeletedUsers][29] = '\0';
        numDeletedUsers++;
    }
    utenti_bst = BST_RimuoviEmail(utenti_bst, email);
    for (int i = idx; i < dbUtenti.num_utenti - 1; i++)
        dbUtenti.lista[i] = dbUtenti.lista[i + 1];
    dbUtenti.num_utenti--;
    SalvaDB();
    LAN_BroadcastNow();
    for (int i = 0; i < num_pending_deletions_auth; i++) {
        if (strcmp(pending_deletions_auth[i], email) == 0) {
            for (int j = i; j < num_pending_deletions_auth - 1; j++)
                strcpy(pending_deletions_auth[j], pending_deletions_auth[j + 1]);
            num_pending_deletions_auth--;
            break;
        }
    }
}

/* Converte Statistiche (record DB) in Utente (entita' di dominio). */
static Utente Statistiche_a_Utente(const Statistiche* s) {
    Utente u;
    memset(&u, 0, sizeof(Utente));
    strncpy(u.nome,      s->nome,      sizeof(u.nome) - 1);
    strncpy(u.cognome,   s->cognome,   sizeof(u.cognome) - 1);
    strncpy(u.username,  s->username,  sizeof(u.username) - 1);
    strncpy(u.email,     s->email,     sizeof(u.email) - 1);
    strncpy(u.password,  s->password,  sizeof(s->password) - 1);
    return u;
}

// Ricostruisce il BST a partire dall'array dbUtenti.
void AuthBST_Ricostruisci(void) {
    if (utenti_bst) {
        BST_Distruggi(utenti_bst);
        utenti_bst = NULL;
    }
    for (int i = 0; i < dbUtenti.num_utenti; i++) {
        Utente u = Statistiche_a_Utente(&dbUtenti.lista[i]);
        int duplicato = 0;
        utenti_bst = BST_Inserisci(utenti_bst, u, &duplicato);
    }
}

/**
 * @brief Alloca o rialloca dinamicamente il database utenti.
 *
 * Se la capacita' corrente e' insufficiente, raddoppia l'array
 * con realloc finche' non raggiunge `min_slot`.
 *
 * @param min_slot Numero minimo di slot richiesti.
 * @return 1 se allocazione OK, 0 se out-of-memory.
 */
int DB_Alloca(int min_slot) {
    if (min_slot <= dbUtenti.capacita) return 1;
    int nuova_cap = dbUtenti.capacita ? dbUtenti.capacita * 2 : DB_CAPACITA_INIZIALE;
    while (nuova_cap < min_slot) nuova_cap *= 2;
    Statistiche* nuova_lista = (Statistiche*)realloc(dbUtenti.lista, nuova_cap * sizeof(Statistiche));
    if (!nuova_lista) return 0;
    dbUtenti.lista = nuova_lista;
    dbUtenti.capacita = nuova_cap;
    return 1;
}

// Inizializza il database (alloca la memoria iniziale se non gia' allocata).
static void DB_Inizializza(void) {
    if (!dbUtenti.lista) {
        dbUtenti.lista = (Statistiche*)malloc(DB_CAPACITA_INIZIALE * sizeof(Statistiche));
        dbUtenti.capacita = DB_CAPACITA_INIZIALE;
        dbUtenti.num_utenti = 0;
    }
}

/**
 * @brief Cifra/decifra password e PIN in memoria (XOR 0xAA).
 */
void CifraDecifraDB(void) {
    for (int i = 0; i < dbUtenti.num_utenti; i++) {
        for (int j = 0; j < 30; j++) {
            if (dbUtenti.lista[i].password[j] == '\0') break;
            dbUtenti.lista[i].password[j] ^= 0xAA;
        }
        for (int j = 0; j < 7; j++) {
            if (dbUtenti.lista[i].pin_recupero[j] == '\0') break;
            dbUtenti.lista[i].pin_recupero[j] ^= 0xAA;
        }
    }
}

/**
 * @brief Salva il database su file binario (data/users/users.dat).
 */
void SalvaDB(void) {
    DB_Inizializza();
    CifraDecifraDB();
    /* BUG 7 FIX: Crea la directory data/users/ se non esiste.
     * Senza questo, il salvataggio fallisce silenziosamente se la cartella
     * e' stata cancellata o non esiste ancora. */
    fs_mkdirs();
    FILE* f = fopen("data/users/users.dat", "wb");
    if (f) {
        fwrite(&dbUtenti.num_utenti, sizeof(int), 1, f);
        fwrite(dbUtenti.lista, sizeof(Statistiche), dbUtenti.num_utenti, f);
        fclose(f);
    }
    CifraDecifraDB();
    extern int dentroSalvaDB;
    /* ============================================================
     * BUG 1 FIX: Broadcast LAN sempre dopo SalvaDB, indipendentemente
     * dal numero di utenti. Il vecchio codice controllava
     * LAN_GetUtentiCount() > 0, ma questo impediva il broadcast
     * quando il database veniva aggiornato da remoto (es. nuovo
     * utente registrato su un altro PC). La condizione dentroSalvaDB
     * e' sufficiente per evitare loop infiniti.
     * ============================================================ */
    if (!dentroSalvaDB) {
        LAN_BroadcastNow();
    }
}

/**
 * @brief Carica il database da file binario (data/users/users.dat).
 */
void CaricaDB(void) {
    DB_Inizializza();
    FILE* f = fopen("data/users/users.dat", "rb");
    if (f) {
        int num_utenti_file = 0;
        fread(&num_utenti_file, sizeof(int), 1, f);
        if (num_utenti_file > 0 && num_utenti_file < 10000) {
            DB_Alloca(num_utenti_file);
            fread(dbUtenti.lista, sizeof(Statistiche), num_utenti_file, f);
            dbUtenti.num_utenti = num_utenti_file;
        }
        fclose(f);
        CifraDecifraDB();
    }
    AuthBST_Ricostruisci();
}

// ============================================================
//  API registrazione (email univoca come primary key)
// ============================================================

/**
 * @brief Registra un nuovo utente nel sistema.
 *
 * @param email Email dell'utente.
 * @param nome Nome.
 * @param cognome Cognome.
 * @param username Username univoco.
 * @param password Password.
 * @param pin PIN di recupero.
 * @return 1 se registrazione OK, 0 se errore allocazione o PIN duplicato.
 */
int Auth_RegistraUtente(const char* email, const char* nome, const char* cognome,
                        const char* username, const char* password, const char* pin) {
    DB_Inizializza();
    if (!DB_Alloca(dbUtenti.num_utenti + 1)) return 0;
    if (Auth_EmailGiaUsata(email, -1)) return 0;

    Statistiche* s = &dbUtenti.lista[dbUtenti.num_utenti];
    memset(s, 0, sizeof(Statistiche));
    strncpy(s->email,    email,    sizeof(s->email) - 1);
    strncpy(s->nome,     nome,     sizeof(s->nome) - 1);
    strncpy(s->cognome,  cognome,  sizeof(s->cognome) - 1);
    strncpy(s->username, username, sizeof(s->username) - 1);
    strncpy(s->password, password, sizeof(s->password) - 1);
    strncpy(s->pin_recupero, pin, sizeof(s->pin_recupero) - 1);
    s->partite_giocate = 0;
    s->vittorie_giocatore = 0;
    s->vittorie_bot = 0;
    dbUtenti.num_utenti++;

    Utente u = Statistiche_a_Utente(s);
    int duplicato = 0;
    utenti_bst = BST_Inserisci(utenti_bst, u, &duplicato);
    return 1;
}

/**
 * @brief Rimuove un utente da database e BST.
 */
void Auth_RimuoviUtente(const char* email) {
    utenti_bst = BST_RimuoviEmail(utenti_bst, email);
    for (int i = 0; i < dbUtenti.num_utenti; i++) {
        if (strcmp(dbUtenti.lista[i].email, email) == 0) {
            for (int j = i; j < dbUtenti.num_utenti - 1; j++)
                dbUtenti.lista[j] = dbUtenti.lista[j+1];
            dbUtenti.num_utenti--;
            break;
        }
    }
}

/**
 * @brief Ricerca O(log n) di un utente per email tramite BST.
 * @return Indice nel dbUtenti, -1 se non trovato.
 */
int AuthBST_CercaIndicePerEmail(const char* email) {
    NodoAlbero* n = BST_RicercaEmail(utenti_bst, email);
    if (!n) return -1;
    for (int i = 0; i < dbUtenti.num_utenti; i++) {
        if (strcmp(dbUtenti.lista[i].email, n->utente.email) == 0) return i;
    }
    return -1;
}

/**
 * @brief Restituisce l'altezza del BST (utile per debug/test).
 * @return Altezza dell'albero.
 */
int AuthBST_Altezza(void) { return BST_Altezza(utenti_bst); }

/**
 * @brief Restituisce il numero di nodi nel BST.
 * @return Numero di nodi.
 */
int AuthBST_ContaNodi(void) { return BST_ContaNodi(utenti_bst); }