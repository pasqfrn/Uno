/**
 * @file test_adt.c
 * @brief Test unitari per gli ADT e la logica di dominio UNO.
 * @defgroup tests Test Unitari
 * @brief Modulo di testing con ASSERT (assert.h) per verificare il corretto
 * funzionamento degli ADT (Lista, Pila, Coda, BST, Chat) e delle funzionalita'
 * di dominio (mazzo di pesca, gestione mano, avanzamento turni, validazione mosse).
 *
 * PRINCIPI:
 *   - I test interagiscono con gli ADT SOLO tramite l'API pubblica (information
 *     hiding): mai accesso diretto alla rappresentazione interna dei tipi opachi.
 *   - Ogni test usa assert(): in caso di fallimento il programma si interrompe
 *     segnalando la riga esatta (programmazione difensiva e testing rigorosi).
 *   - I test di dominio verificano funzionalita' reali del gioco (mazzo/pesca,
 *     mano, turni tramite CodaTurni, validazione mosse).
 *
 * Compilazione ed esecuzione:
 *   gcc -std=c99 -Wall -Wextra -Ilib -Isrc -Itests -Itests/stubs tests/test_adt.c \
 *       src/data_structures/stack.c src/data_structures/queue.c \
 *       src/data_structures/list.c src/data_structures/bst.c src/data_structures/chat.c \
 *       src/game/game_logic.c src/game/deck.c -o tests/test_adt.exe -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Raylib stub per i test (solo definizioni minime: Rectangle, Vector2, TextFormat) */
#include "stubs/raylib.h"

/* ============================================================
 *  Include dei moduli da testare
 * ============================================================ */
#include "../lib/data_structures/data_structures.h"
#include "../lib/data_structures/list.h"
#include "../lib/data_structures/stack.h"
#include "../lib/data_structures/queue.h"
#include "../lib/data_structures/bst.h"
#include "../lib/data_structures/chat.h"
#include "../lib/data_structures/list.h"
#include "../lib/game/player.h"
#include "../lib/game/game_state.h"
#include "../lib/game/game_logic.h"

/* ============================================================
 *  MACRO DI TEST CON ASSERT
 *  Ogni test registra esito e, in caso di fallimento, esegue
 *  assert() per fermare l'esecuzione con il numero di riga.
 * ============================================================ */
static int test_totali  = 0;
static int test_passati = 0;
/* Definizione mock per soddisfare i moduli UI collegati nei test */
int esc_consumato = 0;

#define TEST(cond) do { \
    test_totali++; \
    if ((cond)) { \
        test_passati++; \
        printf("  [OK]   %s\n", #cond); \
    } else { \
        printf("  [FAIL] %s (riga %d)\n", #cond, __LINE__); \
        assert(cond); \
    } \
} while (0)

/* ============================================================
 *  HELPER: crea una carta con colore e tipo
 * ============================================================ */
static Carta crea_carta(Colore colore, TipoCarta tipo) {
    Carta c;
    c.colore = colore;
    c.tipo = tipo;
    c.area = (Rectangle){0,0,0,0};
    c.posOriginale = (Vector2){0,0};
    c.isTrascinata = 0;
    return c;
}

/* ============================================================
 *  ADT LISTA (ListaCarte / API pubblica)
 *  Operazioni testate: crea, inserisci, leggi, aggiorna, copia,
 *  rimuovi, ruota, elimina. Nessun accesso alla rappresentazione.
 * ============================================================ */
static void test_lista(void) {
    printf("\n=== TEST ADT LISTA (API pubblica, information hiding) ===\n");

    ListaCarte* lista = CreaLista();
    TEST(lista != NULL);
    TEST(Lista_Lunghezza(lista) == 0);
    TEST(Lista_GetCarta(lista, 0).tipo == NUM_0); /* carta vuota {0} */

    Carta c1 = crea_carta(ROSSO, NUM_5);
    InsertInCoda(lista, c1);
    TEST(Lista_Lunghezza(lista) == 1);
    TEST(Lista_GetCarta(lista, 0).colore == ROSSO);
    TEST(Lista_GetCarta(lista, 0).tipo == NUM_5);

    InsertInCoda(lista, crea_carta(BLU, SALTA));
    InsertInCoda(lista, crea_carta(GIALLO, NUM_3));
    TEST(Lista_Lunghezza(lista) == 3);

    /* Lettura per valore */
    TEST(Lista_GetCarta(lista, 1).tipo == SALTA);
    /* Aggiornamento per valore (accessor) */
    Lista_ImpostaCarta(lista, 1, crea_carta(VERDE, PESCA_DUE));
    TEST(Lista_GetCarta(lista, 1).tipo == PESCA_DUE);
    TEST(Lista_GetCarta(lista, 1).colore == VERDE);

    /* Copia collettiva in array esterno */
    Carta snapshot[8];
    int nCopia = Lista_CopiaCarte(lista, snapshot, 8);
    TEST(nCopia == 3);
    TEST(snapshot[0].tipo == NUM_5 && snapshot[2].tipo == NUM_3);

    /* Rotazione: la prima carta va in fondo */
    Lista_MettiPrimaInFondo(lista);
    TEST(Lista_Lunghezza(lista) == 3);
    TEST(Lista_GetCarta(lista, 0).tipo == PESCA_DUE);
    TEST(Lista_GetCarta(lista, 2).tipo == NUM_5);

    /* Rimozione per indice */
    RemoveAt(lista, 0);
    TEST(Lista_Lunghezza(lista) == 2);
    TEST(Lista_GetCarta(lista, 0).tipo == NUM_3);

    /* Integrita' bidirezionale dopo le operazioni */
    TEST(Lista_VerificaBidirezionale(lista) == 1);

    /* Funzioni specializzate (data_structures/list.h) */
    Lista_InserisciDopo(lista, NULL, crea_carta(NERO, CAMBIO_COLORE));
    TEST(Lista_Lunghezza(lista) == 3);
    TEST(Lista_GetCarta(lista, 0).tipo == CAMBIO_COLORE);
    TEST(Lista_Ultimo(lista) != NULL);
    TEST(Lista_Precedente(lista, GetAt(lista, 1)) != NULL);

    /* Rimozione per nodo (tipo opaco, senza dereferenziarlo) */
    RemoveNode(lista, GetAt(lista, 1));
    TEST(Lista_Lunghezza(lista) == 2);
    TEST(Lista_VerificaBidirezionale(lista) == 1);

    /* Input non validi (programmazione difensiva) */
    TEST(Lista_Lunghezza(NULL) == 0);
    TEST(Lista_GetCarta(NULL, 0).tipo == NUM_0);
    TEST(Lista_GetCarta(lista, 99).tipo == NUM_0);
    Lista_ImpostaCarta(lista, -1, crea_carta(ROSSO, NUM_1));
    TEST(Lista_Lunghezza(lista) == 2);

    EliminaLista(lista);
}
/* ============================================================
 *  ADT PILA (Stack LIFO)
 *  Operazioni: crea, push, top, pop, dimensione, svuota, distruggi.
 * ============================================================ */
static void test_pila(void) {
    printf("\n=== TEST ADT PILA (Stack LIFO) ===\n");

    Pila* p = Pila_Crea();
    TEST(p != NULL);
    TEST(Pila_Vuota(p) == 1);

    TEST(Pila_Push(p, crea_carta(ROSSO, NUM_1)) == 1);
    TEST(Pila_Push(p, crea_carta(BLU, NUM_2)) == 1);
    TEST(Pila_Push(p, crea_carta(VERDE, NUM_3)) == 1);
    TEST(Pila_Dimensione(p) == 3);

    Carta top;
    TEST(Pila_Top(p, &top) == 1 && top.tipo == NUM_3);
    TEST(Pila_Pop(p, &top) == 1 && top.tipo == NUM_3);
    TEST(Pila_Top(p, &top) == 1 && top.tipo == NUM_2);
    TEST(Pila_Dimensione(p) == 2);

    Pila_Svuota(p);
    TEST(Pila_Vuota(p) == 1);
    TEST(Pila_Pop(p, &top) == 0);  /* pop su pila vuota */

    Pila_Distruggi(p);
    TEST(Pila_Vuota(NULL) == 1);
    TEST(Pila_Dimensione(NULL) == 0);
}

/* ============================================================
 *  ADT CODA (CodaTurni per i turni di gioco)
 *  Operazioni: enqueue, dequeue, front, rear, rotazione,
 *  inversione verso, copia per indice.
 * ============================================================ */
static void test_coda(void) {
    printf("\n=== TEST ADT CODA (CodaTurni, turni di gioco) ===\n");

    Giocatore g1, g2, g3;
    memset(&g1, 0, sizeof(Giocatore)); Giocatore_SetNome(&g1, "Alice");
    memset(&g2, 0, sizeof(Giocatore)); Giocatore_SetNome(&g2, "Bob");
    memset(&g3, 0, sizeof(Giocatore)); Giocatore_SetNome(&g3, "Charlie");

    CodaTurni* c = Coda_Crea();
    TEST(c != NULL);
    TEST(Coda_Vuota(c) == 1);
    TEST(Coda_Dimensione(c) == 0);

    TEST(Coda_Enqueue(c, g1) == 1);
    TEST(Coda_Enqueue(c, g2) == 1);
    TEST(Coda_Enqueue(c, g3) == 1);
    TEST(Coda_Dimensione(c) == 3);

    Giocatore curr;
    TEST(Coda_Front(c, &curr) == 1 && strcmp(Giocatore_Nome(&curr), "Alice") == 0);
    TEST(Coda_Rear(c, &curr) == 1 && strcmp(Giocatore_Nome(&curr), "Charlie") == 0);

    /* Rotazione oraria: dopo il turno di Alice tocca a Bob */
    Coda_Ruota(c);
    TEST(Coda_Front(c, &curr) == 1 && strcmp(Giocatore_Nome(&curr), "Bob") == 0);
    TEST(Coda_Dimensione(c) == 3);

    Coda_Ruota(c);
    TEST(Coda_Front(c, &curr) == 1 && strcmp(Giocatore_Nome(&curr), "Charlie") == 0);
    Coda_Ruota(c);
    TEST(Coda_Front(c, &curr) == 1 && strcmp(Giocatore_Nome(&curr), "Alice") == 0); /* wrap-around */

    /* Inversione verso + rotazione antioraria */
    Coda_InvertiVerso(c);
    Coda_Ruota(c);
    TEST(Coda_Front(c, &curr) == 1 && strcmp(Giocatore_Nome(&curr), "Charlie") == 0);
    Coda_Ruota(c);
    TEST(Coda_Front(c, &curr) == 1 && strcmp(Giocatore_Nome(&curr), "Bob") == 0);
    Coda_Ruota(c);
    TEST(Coda_Front(c, &curr) == 1 && strcmp(Giocatore_Nome(&curr), "Alice") == 0); /* wrap-around */

    /* Copia per indice */
    Giocatore out;
    TEST(Coda_CopiaPerIndice(c, 1, &out) == 1 && strcmp(Giocatore_Nome(&out), "Bob") == 0);
    TEST(Coda_CopiaPerIndice(c, 5, &out) == 0);  /* indice fuori range */

    /* Dequeue/Enqueue (rotazione FIFO classica) */
    TEST(Coda_Dequeue(c, &curr) == 1 && strcmp(Giocatore_Nome(&curr), "Alice") == 0);
    TEST(Coda_Enqueue(c, curr) == 1 && Coda_Dimensione(c) == 3);
    TEST(Coda_Front(c, &curr) == 1 && strcmp(Giocatore_Nome(&curr), "Bob") == 0);

    /* Programmazione difensiva */
    TEST(Coda_Vuota(NULL) == 1);
    TEST(Coda_Dimensione(NULL) == 0);
    TEST(Coda_Front(NULL, &curr) == 0);
    TEST(Coda_Dequeue(NULL, &curr) == 0);
    TEST(Coda_CopiaPerIndice(NULL, 0, &curr) == 0);

    Coda_Distruggi(c);
}

/* ============================================================
 *  ADT BST (Albero Binario di Ricerca per email)
 *  Operazioni: inserisci, cerca, rimuovi, conta, accessor.
 * ============================================================ */
static void test_bst(void) {
    printf("\n=== TEST ADT BST (ricerca utenti per email) ===\n");

    Utente alice, bob, carlo;
    memset(&alice, 0, sizeof(Utente)); strcpy(alice.email, "alice@example.com"); strcpy(alice.nome, "Alice");
    memset(&bob, 0, sizeof(Utente));   strcpy(bob.email, "bob@example.com");     strcpy(bob.nome, "Bob");
    memset(&carlo, 0, sizeof(Utente)); strcpy(carlo.email, "carlo@example.com"); strcpy(carlo.nome, "Carlo");

    NodoAlbero* radice = NULL;
    int duplicato = 0;
    radice = BST_Inserisci(radice, bob, &duplicato);
    radice = BST_Inserisci(radice, alice, &duplicato);
    radice = BST_Inserisci(radice, carlo, &duplicato);
    TEST(radice != NULL);
    TEST(BST_ContaNodi(radice) == 3);
    TEST(BST_Altezza(radice) == 2);

    NodoAlbero* trovato = BST_RicercaEmail(radice, "alice@example.com");
    TEST(trovato != NULL);
    /* Accessor informativo: NON accediamo alla rappresentazione interna */
    TEST(BST_GetEmail(trovato) != NULL && strcmp(BST_GetEmail(trovato), "alice@example.com") == 0);

    TEST(BST_RicercaEmail(radice, "nonexistent@example.com") == NULL);
    TEST(BST_RicercaEmail(NULL, "x@y.z") == NULL);

    /* Inserimento duplicato */
    radice = BST_Inserisci(radice, alice, &duplicato);
    TEST(duplicato == 1);
    TEST(BST_ContaNodi(radice) == 3);

    /* Rimozione */
    radice = BST_RimuoviEmail(radice, "bob@example.com");
    TEST(BST_ContaNodi(radice) == 2);
    TEST(BST_RicercaEmail(radice, "bob@example.com") == NULL);

    /* In-order produce array ordinato */
    Utente ordinati[8];
    int n = BST_Inordina(radice, ordinati, 8);
    TEST(n == 2);
    TEST(strcmp(ordinati[0].email, "alice@example.com") == 0);
    TEST(strcmp(ordinati[1].email, "carlo@example.com") == 0);

    BST_Distruggi(radice);
    TEST(BST_ContaNodi(NULL) == 0);
    TEST(BST_Altezza(NULL) == 0);
}

/* ============================================================
 *  ADT CHAT (cronologia circolare FIFO)
 *  Operazioni: crea, aggiungi, ottieni, formatta, svuota.
 * ============================================================ */
static void test_chat(void) {
    printf("\n=== TEST ADT CHAT (cronologia circolare FIFO) ===\n");

    ChatStorico* chat = Chat_Crea();
    TEST(chat != NULL);

    Chat_Aggiungi(chat, "Alice", "Ciao!");
    Chat_Aggiungi(chat, "Bob", "UNO!");
    TEST(Chat_Ottieni(chat, 0) != NULL);
    TEST(Chat_Ottieni(chat, 1) != NULL);
    TEST(Chat_Ottieni(chat, 2) == NULL);

    /* Verifica formattazione riga */
    char riga[320];
    const MessaggioChat* m0 = Chat_Ottieni(chat, 0);
    TEST(m0 != NULL && strcmp(m0->mittente, "Alice") == 0);
    Chat_FormattaRiga(m0, riga, sizeof(riga));
    TEST(strstr(riga, "@Alice: Ciao!") != NULL);

    /* Riempimento oltre la capacita': il piu' vecchio viene sovrascritto */
    for (int i = 2; i < CHAT_STORICO_MAX + 3; i++) {
        char mitt[16], testo[16];
        snprintf(mitt, sizeof(mitt), "U%d", i);
        snprintf(testo, sizeof(testo), "msg%d", i);
        Chat_Aggiungi(chat, mitt, testo);
    }
    TEST(Chat_Ottieni(chat, CHAT_STORICO_MAX - 1) != NULL);

    Chat_Svuota(chat);
    TEST(Chat_Ottieni(chat, 0) == NULL);

    /* Programmazione difensiva */
    Chat_Aggiungi(NULL, "a", "b");
    Chat_FormattaRiga(NULL, riga, sizeof(riga));
    ChatStorico* chat2 = Chat_Crea();   /* allocazione e distruzione corretta */
    TEST(chat2 != NULL);
    Chat_Distruggi(chat2);

    Chat_Distruggi(chat);
    Chat_Distruggi(NULL);            /* nessun crash con NULL */
}

/* ============================================================
 *  FUNZIONALITA' DI DOMINIO 1: MAZZO E PESCA
 *  Verifica che InizializzaMazzo produca un mazzo standard da 108
 *  carte e che Pesca distribuisca correttamente le carte in mano.
 * ============================================================ */
static void test_dominio_mazzo(void) {
    printf("\n=== TEST DOMINIO: mazzo e pesca carte ===\n");

    StatoGioco gioco;
    memset(&gioco, 0, sizeof(StatoGioco));

    /* Il mazzo UNO standard ha 108 carte */
    InizializzaMazzo(&gioco);
    TEST(gioco.num_carte_mazzo == 108);
    TEST(gioco.num_carte_scarti == 0);
    TEST(GameLogic_GetLunghezzaMano(&gioco.giocatori[0]) == 0);

    /* Prima della pesca la mano non esiste: Pesca la crea */
    gioco.num_giocatori = 1;
    Pesca(&gioco, 0, 7);
    TEST(GameLogic_GetLunghezzaMano(&gioco.giocatori[0]) == 7);
    TEST(gioco.num_carte_mazzo == 108 - 7);

    /* Le carte pescate sono state correttamente inserite (accessor) */
    Carta prima = GetCartaGiocatore(&gioco.giocatori[0], 0);
    TEST(prima.tipo >= NUM_0 && prima.tipo <= PESCA_QUATTRO);
    TEST(Lista_VerificaBidirezionale(gioco.giocatori[0].mano) == 1);

    /* Rimozione di una carta ed aggiornamento contatore */
    RimuoviCartaGiocatore(&gioco.giocatori[0], 3);
    TEST(GameLogic_GetLunghezzaMano(&gioco.giocatori[0]) == 6);

    /* Pulizia della mano (senza EliminaPartita, che e' in game_flow.c) */
    GameLogic_DistruggiMano(&gioco.giocatori[0]);
}

/* ============================================================
 *  FUNZIONALITA' DI DOMINIO 2: MANO E SERIALIZZAZIONE
 *  Verifica le funzioni di dominio basate sull'ADT ListaCarte:
 *  backup flat per il salvataggio, ricostruzione, rotazione visiva.
 * ============================================================ */
static void test_dominio_mano(void) {
    printf("\n=== TEST DOMINIO: gestione mano (ADT ListaCarte) ===\n");

    Giocatore g;
    memset(&g, 0, sizeof(Giocatore));
    g.mano = CreaLista();
    TEST(g.mano != NULL);

    InsertInCoda(g.mano, crea_carta(ROSSO, NUM_1));
    InsertInCoda(g.mano, crea_carta(BLU, SALTA));
    InsertInCoda(g.mano, crea_carta(GIALLO, NUM_3));
    TEST(GameLogic_GetLunghezzaMano(&g) == 3);

    /* Rotazione visiva: la prima carta passa in fondo */
    GameLogic_MettiPrimaInFondo(&g);
    TEST(GameLogic_GetLunghezzaMano(&g) == 3);
    TEST(Lista_GetCarta(g.mano, 0).tipo == SALTA);
    TEST(Lista_GetCarta(g.mano, 2).tipo == NUM_1);

    /* Backup flat per la serializzazione (come fa SaveManager) */
    TEST(GameLogic_PreparaManoSalvataggio(&g) == 3);
    TEST(g.num_carte_backup == 3);

    /* Simula il salvataggio su file: la lista viene scollegata (mano = NULL)
     * ma il backup flat (mano_backup[]) rimane per la ricostruzione. */
    g.mano = NULL;
    g.num_carte_mano = 0;
    TEST(GameLogic_GetLunghezzaMano(&g) == 0);
    TEST(g.num_carte_backup == 3);

    /* Ricostruzione dal backup (come fa RicostruisciStatoADT) */
    TEST(GameLogic_RicostruisciMano(&g) == 1);
    TEST(g.mano != NULL);
    TEST(GameLogic_GetLunghezzaMano(&g) == 3);
    TEST(Lista_GetCarta(g.mano, 0).tipo == SALTA);
    TEST(Lista_VerificaBidirezionale(g.mano) == 1);

    /* Copia collettiva della mano tramite dominio */
    Carta out[8];
    TEST(GameLogic_CopiaMano(&g, out, 8) == 3);
    TEST(out[2].tipo == NUM_1);

    /* Aggiornamento per valore tramite dominio (drag & drop) */
    Carta c = Lista_GetCarta(g.mano, 1);
    c.area.x = 100.0f;
    GameLogic_ImpostaCarta(&g, 1, c);
    TEST(Lista_GetCarta(g.mano, 1).area.x == 100.0f);

    /* Programmazione difensiva */
    TEST(GameLogic_GetLunghezzaMano(NULL) == 0);
    TEST(GameLogic_PreparaManoSalvataggio(NULL) == 0);
    TEST(GameLogic_CopiaMano(&g, NULL, 8) == 0);
    TEST(GameLogic_RicostruisciMano(NULL) == 0);

    GameLogic_DistruggiMano(&g);
}

/* ============================================================
 *  FUNZIONALITA' DI DOMINIO 3: TURNI TRAMITE ADT CodaTurni
 *  Verifica GameLogic_SincronizzaCodaTurni e
 *  GameLogic_AvanzamentoTurno (rotazione dell'ADT + turno_corrente).
 * ============================================================ */
static void test_dominio_turni(void) {
    printf("\n=== TEST DOMINIO: avanzamento turni (ADT CodaTurni) ===\n");

    StatoGioco gioco;
    memset(&gioco, 0, sizeof(StatoGioco));
    gioco.num_giocatori = 4;
    gioco.direzione = 1;
    Giocatore_SetNome(&gioco.giocatori[0], "A");
    Giocatore_SetNome(&gioco.giocatori[1], "B");
    Giocatore_SetNome(&gioco.giocatori[2], "C");
    Giocatore_SetNome(&gioco.giocatori[3], "D");

    /* La coda dei turni viene sincronizzata con lo stato di gioco */
    TEST(GameLogic_SincronizzaCodaTurni(&gioco) == 1);
    TEST(Coda_Dimensione(coda_turni) == 4);
    Giocatore front;
    TEST(Coda_Front(coda_turni, &front) == 1 && strcmp(Giocatore_Nome(&front), "A") == 0);

    /* Avanzamento di 1 in senso orario */
    GameLogic_AvanzamentoTurno(&gioco, 1);
    TEST(gioco.turno_corrente == 1);
    TEST(Coda_Front(coda_turni, &front) == 1 && strcmp(Giocatore_Nome(&front), "B") == 0);

    /* Due passi (es. carta SALTA) */
    GameLogic_AvanzamentoTurno(&gioco, 2);
    TEST(gioco.turno_corrente == 3);
    TEST(Coda_Front(coda_turni, &front) == 1 && strcmp(Giocatore_Nome(&front), "D") == 0);

    /* Cambio verso (carta CAMBIA_GIRO) */
    gioco.direzione = -1;
    GameLogic_AvanzamentoTurno(&gioco, 1);
    TEST(gioco.turno_corrente == 2);
    GameLogic_AvanzamentoTurno(&gioco, 1);
    TEST(gioco.turno_corrente == 1);
    GameLogic_AvanzamentoTurno(&gioco, 4);   /* giro completo */
    TEST(gioco.turno_corrente == 1);

    /* La coda resta consistente dopo le rotazioni */
    TEST(Coda_Dimensione(coda_turni) == 4);

    /* Programmazione difensiva */
    GameLogic_AvanzamentoTurno(NULL, 1);
    TEST(GameLogic_SincronizzaCodaTurni(NULL) == 0);
}

/* ============================================================
 *  FUNZIONALITA' DI DOMINIO 4: VALIDAZIONE MOSSE ED EFFETTI
 *  Testa MossaValida e ApplicaEffetto sulle regole del gioco.
 * ============================================================ */
static void test_dominio_regole(void) {
    printf("\n=== TEST DOMINIO: regole del gioco ===\n");

    StatoGioco gioco;
    memset(&gioco, 0, sizeof(StatoGioco));

    /* --- MossaValida --- */
    gioco.scarti[0] = crea_carta(ROSSO, NUM_5);
    gioco.num_carte_scarti = 1;
    gioco.colore_attivo = ROSSO;

    TEST(MossaValida(&gioco, crea_carta(ROSSO, NUM_7)) == 1);  /* stesso colore */
    TEST(MossaValida(&gioco, crea_carta(VERDE, NUM_5)) == 1);  /* stesso tipo */
    TEST(MossaValida(&gioco, crea_carta(VERDE, NUM_7)) == 0);  /* niente in comune */
    TEST(MossaValida(&gioco, crea_carta(NERO, CAMBIO_COLORE)) == 1); /* jolly sempre valido */

    /* --- ApplicaEffetto: CAMBIA_GIRO --- */
    gioco.num_giocatori = 4;
    gioco.turno_corrente = 0;
    gioco.direzione = 1;
    GameLogic_SincronizzaCodaTurni(&gioco);
    ApplicaEffetto(&gioco, crea_carta(BLU, CAMBIA_GIRO), BLU);
    TEST(gioco.direzione == -1);
    TEST(gioco.turno_corrente == 3);

    /* --- ApplicaEffetto: SALTA --- */
    gioco.turno_corrente = 0;
    gioco.direzione = 1;
    GameLogic_SincronizzaCodaTurni(&gioco);
    ApplicaEffetto(&gioco, crea_carta(BLU, SALTA), BLU);
    TEST(gioco.turno_corrente == 2);

    /* --- ApplicaEffetto: PESCA_DUE --- */
    gioco.turno_corrente = 0;
    gioco.direzione = 1;
    gioco.num_giocatori = 4;
    InizializzaMazzo(&gioco);
    gioco.giocatori[1].mano = CreaLista();
    ApplicaEffetto(&gioco, crea_carta(BLU, PESCA_DUE), BLU);
    TEST(GameLogic_GetLunghezzaMano(&gioco.giocatori[1]) == 2);
    TEST(gioco.turno_corrente == 2);
    GameLogic_DistruggiMano(&gioco.giocatori[1]);

    /* --- ApplicaEffetto: carta normale --- */
    gioco.turno_corrente = 0;
    gioco.direzione = 1;
    GameLogic_SincronizzaCodaTurni(&gioco);
    ApplicaEffetto(&gioco, crea_carta(ROSSO, NUM_9), ROSSO);
    TEST(gioco.turno_corrente == 1);
}

/* ============================================================
 *  PROGRAMMAZIONE DIFENSIVA - Giocatore
 * ============================================================ */
static void test_giocatore(void) {
    printf("\n=== TEST GIOCATORE (programmazione difensiva) ===\n");

    Giocatore g;
    memset(&g, 0, sizeof(Giocatore));

    TEST(strcmp(Giocatore_Nome(&g), "Sconosciuto") == 0);

    Giocatore_SetNome(&g, "Mario");
    TEST(strcmp(Giocatore_Nome(&g), "Mario") == 0);

    g.is_bot = 1;
    TEST(strstr(Giocatore_NomeBot(&g), "(Bot)") != NULL);

    g.is_bot = 0;
    TEST(strcmp(Giocatore_NomeBot(&g), "Mario") == 0);

    Giocatore_SetNome(NULL, "Test");
    Giocatore_SetNome(&g, NULL);
    TEST(1);  /* nessun crash con input non validi */
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void) {
    printf("========================================\n");
    printf("  TEST UNITARI ADT E DOMINIO - UNO\n");
    printf("========================================\n");

    test_lista();
    test_pila();
    test_coda();
    test_bst();
    test_chat();
    test_dominio_mazzo();
    test_dominio_mano();
    test_dominio_turni();
    test_dominio_regole();
    test_giocatore();

    printf("\n========================================\n");
    printf("  RISULTATI: %d/%d test passati\n", test_passati, test_totali);
    printf("========================================\n");

    return (test_passati == test_totali) ? 0 : 1;
}
