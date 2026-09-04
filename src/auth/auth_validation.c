/**
 * @file auth_validation.c
 * @brief Funzioni di validazione per autenticazione e gestione utenti.
 * @ingroup auth
 *
 * Contiene le funzioni di validazione per:
 *   - Email: controllo formato base (presenza di @)
 *   - Password: controllo lunghezza minima (8 char) e presenza maiuscolo
 *   - PIN: controllo lunghezza minima (6 char)
 *   - Username: controllo non vuoto e unicita' nel database
 *   - Credenziali: verifica login (username + password)
 *   - Recupero password: verifica email + PIN
 *
 * Tutte le funzioni lavorano sul database utenti DINAMICO (dbUtenti).
 */
#include <string.h>
#include "../../lib/auth/auth.h"
#include "../../lib/data_structures/data_structures.h"
#include "../../lib/screens/menu/string_utils.h"
#include "../../lib/auth/auth_validation.h"
#include "../../lib/screens/string_utils.h"

/**
 * @brief Verifica se un'email e' formalmente valida.
 *
 * Controlla che la stringa contenga almeno un carattere '@'.
 *
 * @pre email != NULL.
 * @post Restituisce 1 se contiene '@', 0 altrimenti.
 * @param email Stringa email da validare.
 * @return 1 se valida, 0 altrimenti.
 */
int Auth_EmailFormalmenteValida(const char* email) {
    if (!email) return 0;
    int has_at = 0;
    for (int i = 0; email[i]; i++) {
        if (email[i] == '@') has_at = 1;
    }
    return has_at;
}

/**
 * @brief Verifica se una password soddisfa i requisiti minimi.
 *
 * Controlla lunghezza >= 8 caratteri e presenza di almeno una lettera maiuscola.
 *
 * @pre password != NULL.
 * @post Restituisce 1 se lunghezza >= 8 e contiene almeno una maiuscola.
 * @param password Stringa password da validare.
 * @param len Lunghezza della password.
 * @return 1 se valida, 0 altrimenti.
 */
int Auth_PasswordValida(const char* password, int len) {
    if (!password || len < 8) return 0;
    int has_upper = 0;
    for (int i = 0; i < len; i++) {
        if (password[i] >= 'A' && password[i] <= 'Z') has_upper = 1;
    }
    return has_upper;
}

/**
 * @brief Verifica se un PIN di recupero e' valido.
 *
 * @pre pin != NULL.
 * @post Restituisce 1 se len >= 6, 0 altrimenti.
 * @param pin Stringa PIN da validare.
 * @param len Lunghezza del PIN.
 * @return 1 se valido, 0 altrimenti.
 */
int Auth_PINValido(const char* pin, int len) {
    return (pin && len >= 6);
}

/**
 * @brief Verifica se uno username non e' vuoto.
 *
 * @pre username != NULL.
 * @post Restituisce 1 se len > 0, 0 altrimenti.
 * @param username Stringa username da verificare.
 * @param len Lunghezza dello username.
 * @return 1 se non vuoto, 0 altrimenti.
 */
int Auth_UsernameNonVuoto(const char* username, int len) {
    return (username && len > 0);
}

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
int Auth_UsernameGiaUsato(const char* username) {
    for (int i = 0; i < dbUtenti.num_utenti; i++) {
        if (CompareIgnoreCase(dbUtenti.lista[i].username, username)) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Cerca un utente nel database per username e password.
 *
 * Scansiona dbUtenti.lista[] alla ricerca di una corrispondenza.
 * La password viene confrontata in chiaro (eventuale decifratura
 * XOR gestita esternamente).
 *
 * @pre dbUtenti inizializzato.
 * @post Restituisce indice, -1 o -2 in base al risultato.
 * @param username Username inserito dall'utente.
 * @param password Password inserita dall'utente.
 * @return Indice dell'utente trovato, -1 se non trovato, -2 se password errata.
 */
int Auth_VerificaCredenziali(const char* username, const char* password) {
    for (int i = 0; i < dbUtenti.num_utenti; i++) {
        if (strcmp(dbUtenti.lista[i].username, username) == 0) {
            if (strcmp(dbUtenti.lista[i].password, password) == 0) {
                return i;  // trovato, password corretta
            }
            return -2;     // trovato, password errata (codice speciale)
        }
    }
    return -1;             // non trovato
}

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
int Auth_EmailGiaUsata(const char* email, int ignora_indice) {
    for (int i = 0; i < dbUtenti.num_utenti; i++) {
        if (i == ignora_indice) continue;
        if (CompareIgnoreCase(dbUtenti.lista[i].email, email)) return 1;
    }
    return 0;
}

/**
 * @brief Cerca un utente nel database per email e PIN.
 *
 * Usato per il recupero password. Confronta email (case-insensitive) e PIN.
 *
 * @pre dbUtenti inizializzato.
 * @post Restituisce indice se trovato, -1 altrimenti.
 * @param email Email inserita dall'utente.
 * @param pin PIN di recupero inserito.
 * @return Indice dell'utente trovato, o -1 se non trovato.
 */
int Auth_VerificaEmailPIN(const char* email, const char* pin) {
    for (int i = 0; i < dbUtenti.num_utenti; i++) {
        if (CompareIgnoreCase(dbUtenti.lista[i].email, email) &&
            strcmp(dbUtenti.lista[i].pin_recupero, pin) == 0) {
            return i;
        }
    }
    return -1;
}
