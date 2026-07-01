/**
 * @file auth.h
 * @brief Modulo di autenticazione e gestione database utenti.
 * @defgroup auth Autenticazione e Database Utenti
 * @brief Gestisce registrazione, login, recupero password, eliminazione utenti
 * e sincronizzazione Lan dell'Admin Panel.
 *
 * Caratteristiche principali:
 * - Database utenti con allocazione dinamica (malloc/realloc)
 * - Eliminazione lazy: la rimozione fisica avviene solo al logout/disconnessione
 * - Admin Panel real-time via LAN + TCP broadcast
 * - BST parallelo per ricerca O(log n) dell'email
 */

#ifndef AUTH_H
#define AUTH_H
#include "data_structures.h"

extern DatabaseUtenti dbUtenti;
extern int id_utente_corrente, nuovo_utente;
extern char inputEmail[100], inputNome[30], inputCognome[30];
extern char inputUsername[30], inputPassword[30], inputPin[7];
extern int emailCount, nomeCount, cognomeCount;
extern int usernameCount, passCount, pinCount, id_temp_login;
extern int errore_password, errore_registrazione_pass;
extern int errore_registrazione_email, errore_registrazione_username;
extern int campo_focusato;

/**
 * @addtogroup lazy_deletion Eliminazione Diffusa (Lazy Deletion)
 * @brief API per la rimozione differita degli utenti.
 * @{
 */
/** @brief Numero massimo di eliminazioni pendenti. */
#define MAX_PENDING_DELETIONS 64
/** @brief Marca un utente per eliminazione differita. @param email Email dell'utente. @return 1 se marcato, 0 altrimenti. */
int Auth_MarkForDeletion(const char* email);
/** @brief Verifica se un utente e' marcato per eliminazione. @param email Email dell'utente. @return 1 se marcato, 0 altrimenti. */
int Auth_IsMarkedForDeletion(const char* email);
/** @brief Processa le eliminazioni pendenti per un utente. @param email Email dell'utente. */
void Auth_ProcessPendingDeletions(const char* email);
/** @} */

/**
 * @addtogroup database_api Database API
 * @brief Funzioni per la gestione del database utenti in memoria e su file.
 * @{
 */
/** @brief Alloca o rialloca il database per garantire almeno min_slot. @param min_slot Numero minimo di slot. @return 1 se OK, 0 se out-of-memory. */
int DB_Alloca(int min_slot);
/**
 * @brief Salva il database su file binario con cifratura XOR.
 * @pre DB inizializzato e utenti presenti.
 * @post File data/users/users.dat aggiornato; eventuale broadcast LAN eseguito.
 */
void SalvaDB(void);
/** @brief Ricostruisce il BST parallelo a partire dall'array dbUtenti. */
void AuthBST_Ricostruisci(void);
/**
 * @brief Carica il database da file binario.
 * @pre File data/users/users.dat esiste.
 * @post dbUtenti popolato; BST ricostruito.
 */
void CaricaDB(void);
/**
 * @brief Registra un nuovo utente dopo aver generato l'ID.
 * @pre email, username univoci; password conforme ai requisiti; PIN valido.
 * @post Utente aggiunto a dbUtenti e BST; ID incrementato.
 * @param email Email univoca.
 * @param nome Nome utente.
 * @param cognome Cognome utente.
 * @param username Username univoco.
 * @param password Password in chiaro.
 * @param pin PIN di recupero.
 * @return 1 se registrato, 0 altrimenti.
 */
int Auth_RegistraUtente(const char*,const char*,const char*,
                        const char*,const char*,const char*);
/**
 * @brief Rimuove un utente da database e BST.
 * @pre email esiste nel database.
 * @post Utente rimosso da dbUtenti e BST.
 * @param email Email dell'utente da rimuovere.
 */
void Auth_RimuoviUtente(const char* email);
/** @brief Ricerca l'indice di un utente per email tramite BST. @param email Email da cercare. @return Indice in dbUtenti, -1 se non trovato. */
int AuthBST_CercaIndicePerEmail(const char*);
/** @brief Restituisce l'altezza del BST (per debug). @return Altezza dell'albero. */
int AuthBST_Altezza(void);
/** @brief Restituisce il numero di nodi nel BST. @return Numero di nodi. */
int AuthBST_ContaNodi(void);
/** @} */
#endif

