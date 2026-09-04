/**
 * @file chat.c
 * @brief Implementazione ADT ChatStorico per la chat di gioco.
 * @ingroup chat
 *
 * ADT: Buffer circolare FIFO per la storia dei messaggi chat.
 * Operazioni: creazione, aggiunta, lettura, formattazione, svuotamento.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../lib/data_structures/data_structures.h"
#include "../../lib/data_structures/chat.h"
#include "../../lib/data_structures/private/chat_private.h"

/* ============================================================
 *  IMPLEMENTAZIONE ADT CHAT
 * ============================================================ */

/**
 * @brief Crea uno storico chat vuoto (allocazione dinamica).
 * @ingroup chat
 * @post Storico allocato e inizializzato (count=0, head=0), o NULL se out-of-memory.
 * @return Puntatore a ChatStorico, o NULL.
 */
ChatStorico* Chat_Crea(void) {
    ChatStorico* c = (ChatStorico*)malloc(sizeof(ChatStorico));
    if (!c) return NULL;
    c->count = 0;
    c->head  = 0;
    memset(c->buffer, 0, sizeof(c->buffer));
    return c;
}

/**
 * @brief Distrugge uno storico chat liberandone la memoria.
 * @ingroup chat
 * @pre c != NULL.
 * @post Memoria dello storico liberata.
 * @param c Puntatore a ChatStorico.
 */
void Chat_Distruggi(ChatStorico* c) {
    if (!c) return;
    free(c);
}

/**
 * @brief Aggiunge un messaggio in coda (FIFO).
 * @ingroup chat
 * @pre c != NULL; mittente != NULL; testo != NULL.
 * @post Messaggio aggiunto; count aggiornato; head ruotato se pieno.
 * @param c ChatStorico.
 * @param mittente Mittente.
 * @param testo Testo.
 */
void Chat_Aggiungi(ChatStorico* c, const char* mittente, const char* testo) {
    if (!c || !mittente || !testo) return;

    int idx;
    if (c->count < CHAT_STORICO_MAX) {
        idx = (c->head + c->count) % CHAT_STORICO_MAX;
        c->count++;
    } else {
        // Sovrascrivi il più vecchio (head) e ruota
        idx = c->head;
        c->head = (c->head + 1) % CHAT_STORICO_MAX;
    }

    MessaggioChat* m = &c->buffer[idx];
    strncpy(m->mittente, mittente, sizeof(m->mittente) - 1);
    m->mittente[sizeof(m->mittente) - 1] = '\0';
    strncpy(m->testo, testo, sizeof(m->testo) - 1);
    m->testo[sizeof(m->testo) - 1] = '\0';
    Chat_OraCorrente(m->orario, sizeof(m->orario));
}

/**
 * @brief Genera timestamp "HH:MM".
 * @ingroup chat
 * @pre out != NULL; out_size >= 6.
 * @post Stringa "HH:MM" in out.
 * @param out Buffer output.
 * @param out_size Dimensione buffer.
 */
void Chat_OraCorrente(char* out, size_t out_size) {
    if (!out || out_size < 6) return;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(out, out_size, "%02d:%02d", tm.tm_hour, tm.tm_min);
}

/**
 * @brief Ottiene messaggio per indice (0 = piu' vecchio).
 * @ingroup chat
 * @pre c != NULL; 0 <= i < c->count.
 * @return Puntatore a MessaggioChat, o NULL.
 * @param c ChatStorico.
 * @param i Indice.
 */
const MessaggioChat* Chat_Ottieni(const ChatStorico* c, int i) {
    if (!c || i < 0 || i >= c->count) return NULL;
    int idx = (c->head + i) % CHAT_STORICO_MAX;
    return &c->buffer[idx];
}

/**
 * @brief Svuota la cronologia chat.
 * @ingroup chat
 * @pre c != NULL.
 * @post count=0, head=0.
 * @param c ChatStorico.
 */
void Chat_Svuota(ChatStorico* c) {
    if (!c) return;
    c->count = 0;
    c->head = 0;
    memset(c->buffer, 0, sizeof(c->buffer));
}

/**
 * @brief Formatta messaggio in riga "HH:MM @Mittente: testo".
 * @ingroup chat
 * @pre m != NULL; out != NULL; out_size >= 320.
 * @post Riga formattata in out.
 * @param m MessaggioChat.
 * @param out Buffer output.
 * @param out_size Dimensione buffer.
 */
void Chat_FormattaRiga(const MessaggioChat* m, char* out, size_t out_size) {
    if (!m || !out || out_size == 0) return;
    snprintf(out, out_size, "%s @%s: %s",
             m->orario[0] ? m->orario : "--:--",
             m->mittente[0] ? m->mittente : "?",
             m->testo);
}
