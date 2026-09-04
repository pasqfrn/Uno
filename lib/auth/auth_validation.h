/**
 * @file auth_validation.h
 * @brief Modulo di validazione per l'autenticazione.
 * @defgroup auth_validation Validazione Autenticazione
 * @brief Funzioni di validazione dei campi di input per registrazione,
 * login e verifica delle credenziali nel database.
 *
 * Modulo fattorizzato da auth.c per separare la logica di
 * validazione dalla logica di UI.
 */

#ifndef AUTH_VALIDATION_H
#define AUTH_VALIDATION_H

#include "../data_structures/data_structures.h"

/* ============================================================
 *  VALIDAZIONE CAMPI INPUT
 * ============================================================ */

/**
 * @brief Verifica se un'email e' formalmente valida.
 *
 * Contiene almeno un carattere prima della '@' e almeno uno dopo.
 *
 * @pre email != NULL.
 * @post Restituisce 1 se valida, 0 altrimenti.
 * @param email Stringa email da validare.
 * @return 1 se valida, 0 altrimenti.
 */
int Auth_EmailFormalmenteValida(const char* email);

/**
 * @brief Verifica se una password soddisfa i requisiti minimi.
 *
 * Requisiti: almeno 8 caratteri, almeno una lettera maiuscola.
 *
 * @pre password != NULL.
 * @post Restituisce 1 se valida, 0 altrimenti.
 * @param password Stringa password da validare.
 * @param len Lunghezza della password.
 * @return 1 se valida, 0 altrimenti.
 */
int Auth_PasswordValida(const char* password, int len);

/**
 * @brief Verifica se un PIN di recupero e' valido.
 *
 * Deve essere esattamente 6 cifre.
 *
 * @pre pin != NULL.
 * @post Restituisce 1 se len >= 6, 0 altrimenti.
 * @param pin Stringa PIN da validare.
 * @param len Lunghezza del PIN.
 * @return 1 se valido, 0 altrimenti.
 */
int Auth_PINValido(const char* pin, int len);

/**
 * @brief Verifica se uno username non e' vuoto.
 *
 * @pre username != NULL.
 * @post Restituisce 1 se len > 0, 0 altrimenti.
 * @param username Stringa username da verificare.
 * @param len Lunghezza dello username.
 * @return 1 se non vuoto, 0 altrimenti.
 */
int Auth_UsernameNonVuoto(const char* username, int len);

/**
 * @brief Verifica se uno username e' gia' in uso nel database.
 *
 * Scansiona dbUtenti.lista[] alla ricerca di un username duplicato.
 *
 * @pre dbUtenti inizializzato.
 * @post Restituisce 1 se trovato, 0 altrimenti.
 * @param username Stringa username da verificare.
 * @return 1 se gia' in uso, 0 se disponibile.
 */
int Auth_UsernameGiaUsato(const char* username);

/**
 * @brief Verifica se un'email e' gia' in uso nel database.
 *
 * Scansiona dbUtenti.lista[] alla ricerca di un'email duplicata (case-insensitive).
 * Il parametro ignora_indice permette di escludere un utente dal controllo
 * (utile nella modifica profilo per non bloccare il proprio indirizzo email).
 *
 * @pre dbUtenti inizializzato.
 * @post Restituisce 1 se duplicata, 0 se disponibile.
 * @param email Email da verificare.
 * @param ignora_indice Indice dell'utente da ignorare (-1 per controllare tutti).
 * @return 1 se gia' in uso, 0 se disponibile.
 */
int Auth_EmailGiaUsata(const char* email, int ignora_indice);

/* ============================================================
 *  VERIFICA CREDENZIALI
 * ============================================================ */

/**
 * @brief Cerca un utente nel database per username e password.
 *
 * Scansiona dbUtenti.lista[] alla ricerca di una corrispondenza.
 * La password viene confrontata dopo eventuale decifratura XOR.
 *
 * @pre dbUtenti inizializzato.
 * @post Restituisce indice, -1 o -2 in base al risultato.
 * @param username Username inserito dall'utente.
 * @param password Password inserita dall'utente.
 * @return Indice dell'utente trovato, o -1 se non trovato, -2 se password errata.
 */
int Auth_VerificaCredenziali(const char* username, const char* password);

/**
 * @brief Cerca un utente nel database per email e PIN.
 *
 * Usato per il recupero password. Confronta email e PIN di recupero.
 *
 * @pre dbUtenti inizializzato.
 * @post Restituisce indice se trovato, -1 altrimenti.
 * @param email Email inserita dall'utente.
 * @param pin PIN di recupero inserito.
 * @return Indice dell'utente trovato, o -1 se non trovato.
 */
int Auth_VerificaEmailPIN(const char* email, const char* pin);


#endif /* AUTH_VALIDATION_H */
