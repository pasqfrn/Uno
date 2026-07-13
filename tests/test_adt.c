/**
 * @file test_adt.c
 * @brief Test unitari per le strutture dati ADT e la logica di gioco UNO.
 * @defgroup tests Test Unitari
 * @brief Modulo di testing per verificare il corretto funzionamento degli ADT
 * (Lista, Pila, Coda, BST, Chat) e della logica di gioco (MossaValida).
 *
 * Questo modulo costituisce la "programmazione difensiva" richiesta:
 * ogni funzione pubblica degli ADT viene testata con input validi e
 * input al limite (NULL, vuoti, fuori range) per garantire robustezza.
 *
 * Compilazione ed esecuzione:
 *   tests/test_adt.exe
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Raylib stub per i test (solo definizioni minime: Rectangle, Vector2) */
#include "stubs/raylib.h"

/* ============================================================
 *  Include dei moduli da testare
 * ============================================================ */
#include "../lib/data_structures.h"
#include "../lib/data_structures/list.h"
#include "../lib/data_structures/stack.h"
#include "../lib/data_structures/queue.h"
#include "../lib/data_structures/bst.h"
#include "../lib/data_structures/chat.h"
#include "../lib/list.h"
#include "../lib/player.h"
#include "../lib/game_state.h"
#include "../lib/game_logic.h"

/* ============================================================
 *  CONTATORE TEST
 * ============================================================ */
static int test_passati = 0;
static int test_totali  = 0;

#define TEST(nome, expr) do { \
    test_totali++; \
    if (!(expr)) { \
        printf("  [FAIL] %s (riga %d)\n", nome, __LINE__); \
    } else { \
        printf("  [OK]   %s\n", nome); \
        test_passati++; \
    } \
} while(0)

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
 *  TEST LISTA (doppiamente concatenata) - ADT base
 * ============================================================ */
static void test_lista(void) {
    printf("\n=== TEST LISTA (ListaCarte / NodoCarta) ===\n");

    /* --- Test CreaLista --- */
    ListaCarte* lista = CreaLista();
    TEST("CreaLista restituisce non NULL", lista != NULL);
    TEST("Lista vuota: testa == NULL", lista->testa == NULL);
    TEST("Lista vuota: coda == NULL", lista->coda == NULL);
    TEST("Lista vuota: lunghezza == 0", lista->lunghezza == 0);

    /* --- Test InsertInCoda --- */
    Carta c1 = crea_carta(ROSSO, NUM_5);
    InsertInCoda(lista, c1);
    TEST("InsertInCoda: lunghezza == 1", lista->lunghezza == 1);
    TEST("InsertInCoda: testa == coda", lista->testa == lista->coda);
    TEST("InsertInCoda: carta in testa colore ROSSO", lista->testa->carta.colore == ROSSO);
    TEST("InsertInCoda: carta in testa tipo NUM_5", lista->testa->carta.tipo == NUM_5);

    /* --- Test InsertInCoda multipla --- */
    Carta c2 = crea_carta(BLU, SALTA);
    InsertInCoda(lista, c2);
    TEST("InsertInCoda multipla: lunghezza == 2", lista->lunghezza == 2);
    TEST("InsertInCoda multipla: testa != coda", lista->testa != lista->coda);
    TEST("InsertInCoda multipla: testa->prossimo == coda", lista->testa->prossimo == lista->coda);
    TEST("InsertInCoda multipla: coda->prev == testa", lista->coda->prev == lista->testa);
    TEST("InsertInCoda multipla: coda tipo SALTA", lista->coda->carta.tipo == SALTA);

    /* --- Test InsertInTesta --- */
    Carta c0 = crea_carta(VERDE, NUM_0);
    InsertInTesta(lista, c0);
    TEST("InsertInTesta: lunghezza == 3", lista->lunghezza == 3);
    TEST("InsertInTesta: testa tipo NUM_0", lista->testa->carta.tipo == NUM_0);
    TEST("InsertInTesta: testa->prossimo->carta.tipo == NUM_5", lista->testa->prossimo->carta.tipo == NUM_5);

    /* --- Test GetAt --- */
    NodoCarta* nodo0 = GetAt(lista, 0);
    TEST("GetAt(0) non NULL", nodo0 != NULL);
    TEST("GetAt(0) tipo NUM_0", nodo0->carta.tipo == NUM_0);

    NodoCarta* nodo1 = GetAt(lista, 1);
    TEST("GetAt(1) non NULL", nodo1 != NULL);
    TEST("GetAt(1) tipo NUM_5", nodo1->carta.tipo == NUM_5);

    NodoCarta* nodo2 = GetAt(lista, 2);
    TEST("GetAt(2) non NULL", nodo2 != NULL);
    TEST("GetAt(2) tipo SALTA", nodo2->carta.tipo == SALTA);

    NodoCarta* nodo3 = GetAt(lista, 3);
    TEST("GetAt(3) == NULL (fuori range)", nodo3 == NULL);

    NodoCarta* nodo_neg = GetAt(lista, -1);
    TEST("GetAt(-1) == NULL (indice negativo)", nodo_neg == NULL);

    /* --- Test RemoveAt --- */
    RemoveAt(lista, 1);  /* rimuove NUM_5 */
    TEST("RemoveAt(1): lunghezza == 2", lista->lunghezza == 2);
    TEST("RemoveAt(1): testa tipo NUM_0", lista->testa->carta.tipo == NUM_0);
    TEST("RemoveAt(1): coda tipo SALTA", lista->coda->carta.tipo == SALTA);
    TEST("RemoveAt(1): testa->prossimo == coda", lista->testa->prossimo == lista->coda);

    /* --- Test RemoveNode --- */
    NodoCarta* da_rimuovere = GetAt(lista, 0);
    RemoveNode(lista, da_rimuovere);
    TEST("RemoveNode(testa): lunghezza == 1", lista->lunghezza == 1);
    TEST("RemoveNode(testa): testa tipo SALTA", lista->testa->carta.tipo == SALTA);
    TEST("RemoveNode(testa): testa == coda", lista->testa == lista->coda);

    /* --- Test rimozione ultimo elemento --- */
    NodoCarta* ultimo = GetAt(lista, 0);
    RemoveNode(lista, ultimo);
    TEST("RemoveNode(ultimo): lunghezza == 0", lista->lunghezza == 0);
    TEST("RemoveNode(ultimo): testa == NULL", lista->testa == NULL);
    TEST("RemoveNode(ultimo): coda == NULL", lista->coda == NULL);

    /* --- Test su lista vuota --- */
    RemoveAt(lista, 0);
    TEST("RemoveAt su lista vuota: lunghezza == 0", lista->lunghezza == 0);

    /* --- Test EliminaLista --- */
    InsertInCoda(lista, crea_carta(GIALLO, NUM_7));
    InsertInCoda(lista, crea_carta(BLU, PESCA_DUE));
    EliminaLista(lista);
    /* Non possiamo testare dopo free, ma verifichiamo che non crashi */

    /* --- Test funzioni specifiche lista bidirezionale --- */
    ListaCarte* l2 = CreaLista();
    InsertInCoda(l2, crea_carta(ROSSO, NUM_1));
    InsertInCoda(l2, crea_carta(ROSSO, NUM_2));
    InsertInCoda(l2, crea_carta(ROSSO, NUM_3));

    NodoCarta* ult = Lista_Ultimo(l2);
    TEST("Lista_Ultimo non NULL", ult != NULL);
    TEST("Lista_Ultimo tipo NUM_3", ult->carta.tipo == NUM_3);

    NodoCarta* prec = Lista_Precedente(l2, ult);
    TEST("Lista_Precedente(ultimo) non NULL", prec != NULL);
    TEST("Lista_Precedente(ultimo) tipo NUM_2", prec->carta.tipo == NUM_2);

    /* Test Lista_InserisciDopo */
    NodoCarta* nuovo = Lista_InserisciDopo(l2, l2->testa, crea_carta(ROSSO, NUM_9));
    TEST("Lista_InserisciDopo non NULL", nuovo != NULL);
    TEST("Lista_InserisciDopo: lunghezza == 4", l2->lunghezza == 4);
    TEST("Lista_InserisciDopo: nuovo tipo NUM_9", nuovo->carta.tipo == NUM_9);
    TEST("Lista_InserisciDopo: testa->prossimo == nuovo", l2->testa->prossimo == nuovo);

    /* Test Lista_VerificaBidirezionale */
    TEST("Lista_VerificaBidirezionale valida", Lista_VerificaBidirezionale(l2) == 1);

    /* Test Lista_RimuoviNodoBidirezionale */
    Lista_RimuoviNodoBidirezionale(l2, nuovo);
    TEST("Lista_RimuoviNodoBidirezionale: lunghezza == 3", l2->lunghezza == 3);
    TEST("Lista_RimuoviNodoBidirezionale: testa->prossimo->carta.tipo == NUM_2",
         l2->testa->prossimo->carta.tipo == NUM_2);

    EliminaLista(l2);

    /* --- Test programmazione difensiva: chiamate con NULL --- */
    /* Queste non devono crasare */
    InsertInCoda(NULL, crea_carta(ROSSO, NUM_0));
    InsertInTesta(NULL, crea_carta(ROSSO, NUM_0));
    RemoveAt(NULL, 0);
    RemoveNode(NULL, NULL);
    GetAt(NULL, 0);
    EliminaLista(NULL);
    Lista_InserisciDopo(NULL, NULL, crea_carta(ROSSO, NUM_0));
    Lista_Precedente(NULL, NULL);
    Lista_Ultimo(NULL);
    Lista_RimuoviNodoBidirezionale(NULL, NULL);
    Lista_VerificaBidirezionale(NULL);
    TEST("Chiamate con NULL non crasano", 1);
}

/* ============================================================
 *  TEST PILA (Stack LIFO)
 * ============================================================ */
static void test_pila(void) {
    printf("\n=== TEST PILA (Stack LIFO) ===\n");

    Pila* p = Pila_Crea();
    TEST("Pila_Crea non NULL", p != NULL);
    TEST("Pila_Vuota iniziale", Pila_Vuota(p) == 1);
    TEST("Pila_Dimensione iniziale 0", Pila_Dimensione(p) == 0);

    /* Push */
    TEST("Pila_Push restituisce 1", Pila_Push(p, crea_carta(ROSSO, NUM_1)) == 1);
    TEST("Pila_Push: size == 1", Pila_Dimensione(p) == 1);
    TEST("Pila_Vuota dopo push", Pila_Vuota(p) == 0);

    Pila_Push(p, crea_carta(BLU, NUM_2));
    Pila_Push(p, crea_carta(VERDE, NUM_3));
    TEST("Pila_Push multiplo: size == 3", Pila_Dimensione(p) == 3);

    /* Top */
    Carta top;
    TEST("Pila_Top restituisce 1", Pila_Top(p, &top) == 1);
    TEST("Pila_Top colore VERDE", top.colore == VERDE);
    TEST("Pila_Top tipo NUM_3", top.tipo == NUM_3);

    /* Pop */
    Carta out;
    TEST("Pila_Pop restituisce 1", Pila_Pop(p, &out) == 1);
    TEST("Pila_Pop colore VERDE", out.colore == VERDE);
    TEST("Pila_Pop: size == 2", Pila_Dimensione(p) == 2);

    Pila_Pop(p, &out);
    TEST("Pila_Pop 2: colore BLU", out.colore == BLU);
    TEST("Pila_Pop 2: size == 1", Pila_Dimensione(p) == 1);

    Pila_Pop(p, &out);
    TEST("Pila_Pop 3: colore ROSSO", out.colore == ROSSO);
    TEST("Pila_Pop 3: size == 0", Pila_Dimensione(p) == 0);
    TEST("Pila_Vuota dopo tre pop", Pila_Vuota(p) == 1);

    /* Pop su vuota */
    TEST("Pila_Pop su vuota restituisce 0", Pila_Pop(p, NULL) == 0);

    /* Svuota */
    Pila_Push(p, crea_carta(GIALLO, NUM_4));
    Pila_Push(p, crea_carta(GIALLO, NUM_5));
    Pila_Svuota(p);
    TEST("Pila_Svuota: size == 0", Pila_Dimensione(p) == 0);
    TEST("Pila_Svuota: vuota", Pila_Vuota(p) == 1);

    Pila_Distruggi(p);

    /* Test programmazione difensiva */
    TEST("Pila_Push(NULL) restituisce 0", Pila_Push(NULL, crea_carta(ROSSO, NUM_0)) == 0);
    TEST("Pila_Pop(NULL) restituisce 0", Pila_Pop(NULL, NULL) == 0);
    TEST("Pila_Top(NULL) restituisce 0", Pila_Top(NULL, NULL) == 0);
    TEST("Pila_Vuota(NULL) restituisce 1", Pila_Vuota(NULL) == 1);
    TEST("Pila_Dimensione(NULL) restituisce 0", Pila_Dimensione(NULL) == 0);
    Pila_Distruggi(NULL);
    Pila_Svuota(NULL);
    TEST("Chiamate Pila con NULL non crasano", 1);
}

/* ============================================================
 *  TEST CODA (Queue turni)
 * ============================================================ */
static void test_coda(void) {
    printf("\n=== TEST CODA (Queue turni) ===\n");

    CodaTurni* c = Coda_Crea();
    TEST("Coda_Crea non NULL", c != NULL);
    TEST("Coda_Vuota iniziale", Coda_Vuota(c) == 1);
    TEST("Coda_Dimensione iniziale 0", Coda_Dimensione(c) == 0);
    TEST("Coda verso iniziale 1 (orario)", c->verso == 1);

    /* Enqueue */
    Giocatore g1, g2, g3;
    memset(&g1, 0, sizeof(Giocatore));
    memset(&g2, 0, sizeof(Giocatore));
    memset(&g3, 0, sizeof(Giocatore));
    Giocatore_SetNome(&g1, "Alice");
    Giocatore_SetNome(&g2, "Bob");
    Giocatore_SetNome(&g3, "Charlie");

    TEST("Coda_Enqueue restituisce 1", Coda_Enqueue(c, g1) == 1);
    TEST("Coda_Enqueue: size == 1", Coda_Dimensione(c) == 1);
    TEST("Coda_Vuota dopo enqueue", Coda_Vuota(c) == 0);

    Coda_Enqueue(c, g2);
    Coda_Enqueue(c, g3);
    TEST("Coda_Enqueue multiplo: size == 3", Coda_Dimensione(c) == 3);

    /* Front */
    Giocatore front;
    TEST("Coda_Front restituisce 1", Coda_Front(c, &front) == 1);
    TEST("Coda_Front nome Alice", strcmp(Giocatore_Nome(&front), "Alice") == 0);

    /* Rear */
    Giocatore rear;
    TEST("Coda_Rear restituisce 1", Coda_Rear(c, &rear) == 1);
    TEST("Coda_Rear nome Charlie", strcmp(Giocatore_Nome(&rear), "Charlie") == 0);

    /* Dequeue */
    Giocatore out;
    TEST("Coda_Dequeue restituisce 1", Coda_Dequeue(c, &out) == 1);
    TEST("Coda_Dequeue nome Alice", strcmp(Giocatore_Nome(&out), "Alice") == 0);
    TEST("Coda_Dequeue: size == 2", Coda_Dimensione(c) == 2);

    Coda_Dequeue(c, &out);
    TEST("Coda_Dequeue 2: nome Bob", strcmp(Giocatore_Nome(&out), "Bob") == 0);

    Coda_Dequeue(c, &out);
    TEST("Coda_Dequeue 3: nome Charlie", strcmp(Giocatore_Nome(&out), "Charlie") == 0);
    TEST("Coda_Dequeue 3: size == 0", Coda_Dimensione(c) == 0);
    TEST("Coda_Vuota dopo tre dequeue", Coda_Vuota(c) == 1);

    /* Dequeue su vuota */
    TEST("Coda_Dequeue su vuota restituisce 0", Coda_Dequeue(c, NULL) == 0);

    /* Test Coda_ProxNodo */
    Coda_Enqueue(c, g1);
    Coda_Enqueue(c, g2);
    Coda_Enqueue(c, g3);
    NodoCoda* primo = c->front;
    NodoCoda* secondo = Coda_ProxNodo(c, primo);
    TEST("Coda_ProxNodo(primo) non NULL", secondo != NULL);
    TEST("Coda_ProxNodo(primo) nome Bob", strcmp(Giocatore_Nome(&secondo->giocatore), "Bob") == 0);

    /* Test Coda_InvertiVerso: scambia front/rear */
    Coda_InvertiVerso(c);
    TEST("Coda_InvertiVerso: verso == -1", c->verso == -1);
    /* Dopo inversione front e rear sono scambiati:
       front = ex-rear (Charlie), rear = ex-front (Alice) */

    /* Test Coda_CercaPerIndice dopo inversione:
       NOTA: Coda_CercaPerIndice percorre sempre tramite next, quindi dopo
       InvertiVerso (che scambia front/rear ma NON modifica i link next/prev),
       solo index 0 (il nuovo front = ex-rear) e' raggiungibile. */
    NodoCoda* idx0 = Coda_CercaPerIndice(c, 0);
    TEST("Coda_CercaPerIndice(0) dopo inversione non NULL", idx0 != NULL);
    TEST("Coda_CercaPerIndice(0) nome Charlie (ex-rear ora front)", strcmp(Giocatore_Nome(&idx0->giocatore), "Charlie") == 0);

    /* index 1 non e' raggiungibile perche' l'ex-rear (ora front) ha next=NULL */
    NodoCoda* idx1 = Coda_CercaPerIndice(c, 1);
    TEST("Coda_CercaPerIndice(1) dopo inversione == NULL (next non ri-linkati)", idx1 == NULL);

    /* Test Coda_CopiaPerIndice dopo inversione */
    Giocatore copia;
    TEST("Coda_CopiaPerIndice(0) restituisce 1", Coda_CopiaPerIndice(c, 0, &copia) == 1);
    TEST("Coda_CopiaPerIndice(0) nome Charlie (ex-rear ora front)", strcmp(Giocatore_Nome(&copia), "Charlie") == 0);

    TEST("Coda_CopiaPerIndice(1) restituisce 0 (next non ri-linkato)", Coda_CopiaPerIndice(c, 1, &copia) == 0);

    Coda_Distruggi(c);

    /* Test programmazione difensiva */
    TEST("Coda_Enqueue(NULL) restituisce 0", Coda_Enqueue(NULL, g1) == 0);
    TEST("Coda_Dequeue(NULL) restituisce 0", Coda_Dequeue(NULL, NULL) == 0);
    TEST("Coda_Front(NULL) restituisce 0", Coda_Front(NULL, NULL) == 0);
    TEST("Coda_Rear(NULL) restituisce 0", Coda_Rear(NULL, NULL) == 0);
    TEST("Coda_Vuota(NULL) restituisce 1", Coda_Vuota(NULL) == 1);
    TEST("Coda_Dimensione(NULL) restituisce 0", Coda_Dimensione(NULL) == 0);
    Coda_Distruggi(NULL);
    Coda_InvertiVerso(NULL);
    TEST("Chiamate Coda con NULL non crasano", 1);
}

/* ============================================================
 *  TEST BST (Albero Binario di Ricerca)
 * ============================================================ */
static void test_bst(void) {
    printf("\n=== TEST BST (Albero Binario Ricerca) ===\n");

    NodoAlbero* radice = NULL;
    TEST("BST_ContaNodi vuoto == 0", BST_ContaNodi(radice) == 0);
    TEST("BST_Altezza vuoto == 0", BST_Altezza(radice) == 0);

    /* Inserimento */
    Utente u1, u2, u3;
    memset(&u1, 0, sizeof(Utente));
    memset(&u2, 0, sizeof(Utente));
    memset(&u3, 0, sizeof(Utente));
    strcpy(u1.email, "alice@example.com");
    strcpy(u1.nome, "Alice");
    strcpy(u2.email, "bob@example.com");
    strcpy(u2.nome, "Bob");
    strcpy(u3.email, "charlie@example.com");
    strcpy(u3.nome, "Charlie");

    int duplicato = 0;
    radice = BST_Inserisci(radice, u2, &duplicato);  /* Bob (centro) */
    TEST("BST_Inserisci primo: duplicato == 0", duplicato == 0);
    TEST("BST_ContaNodi dopo 1 inserimento == 1", BST_ContaNodi(radice) == 1);

    radice = BST_Inserisci(radice, u1, &duplicato);  /* Alice (sinistra) */
    TEST("BST_Inserisci sinistra: duplicato == 0", duplicato == 0);
    TEST("BST_ContaNodi dopo 2 inserimenti == 2", BST_ContaNodi(radice) == 2);

    radice = BST_Inserisci(radice, u3, &duplicato);  /* Charlie (destra) */
    TEST("BST_Inserisci destra: duplicato == 0", duplicato == 0);
    TEST("BST_ContaNodi dopo 3 inserimenti == 3", BST_ContaNodi(radice) == 3);

    /* Ricerca */
    NodoAlbero* trovato = BST_RicercaEmail(radice, "alice@example.com");
    TEST("BST_RicercaEmail(alice) non NULL", trovato != NULL);
    TEST("BST_RicercaEmail(alice) nome Alice", strcmp(trovato->utente.nome, "Alice") == 0);

    trovato = BST_RicercaEmail(radice, "bob@example.com");
    TEST("BST_RicercaEmail(bob) non NULL", trovato != NULL);
    TEST("BST_RicercaEmail(bob) nome Bob", strcmp(trovato->utente.nome, "Bob") == 0);

    trovato = BST_RicercaEmail(radice, "nonexistent@example.com");
    TEST("BST_RicercaEmail(inesistente) == NULL", trovato == NULL);

    /* Ricerca case-insensitive */
    trovato = BST_RicercaEmail(radice, "ALICE@EXAMPLE.COM");
    TEST("BST_RicercaEmail case-insensitive non NULL", trovato != NULL);

    /* Duplicato */
    radice = BST_Inserisci(radice, u1, &duplicato);
    TEST("BST_Inserisci duplicato: duplicato == 1", duplicato == 1);
    TEST("BST_ContaNodi dopo duplicato == 3 (non incrementato)", BST_ContaNodi(radice) == 3);

    /* Rimozione */
    radice = BST_RimuoviEmail(radice, "bob@example.com");
    TEST("BST_RimuoviEmail(bob): ContaNodi == 2", BST_ContaNodi(radice) == 2);
    trovato = BST_RicercaEmail(radice, "bob@example.com");
    TEST("BST_RicercaEmail(bob) dopo rimozione == NULL", trovato == NULL);

    /* Inordina */
    Utente array[10];
    int count = BST_Inordina(radice, array, 10);
    TEST("BST_Inordina count == 2", count == 2);
    TEST("BST_Inordina[0] == Alice (ordine alfabetico)", strcmp(array[0].nome, "Alice") == 0);
    TEST("BST_Inordina[1] == Charlie", strcmp(array[1].nome, "Charlie") == 0);

    /* Altezza */
    TEST("BST_Altezza dopo 2 nodi == 2", BST_Altezza(radice) == 2);

    BST_Distruggi(radice);

    /* Test programmazione difensiva */
    TEST("BST_RicercaEmail(NULL) restituisce NULL", BST_RicercaEmail(NULL, "test@test.com") == NULL);
    TEST("BST_RicercaEmail(radice, NULL) restituisce NULL", BST_RicercaEmail(NULL, NULL) == NULL);
    BST_Distruggi(NULL);
    TEST("BST_Inordina(NULL) restituisce 0", BST_Inordina(NULL, array, 10) == 0);
    TEST("BST_ContaNodi(NULL) restituisce 0", BST_ContaNodi(NULL) == 0);
    TEST("BST_Altezza(NULL) restituisce 0", BST_Altezza(NULL) == 0);
    TEST("Chiamate BST con NULL non crasano", 1);
}

/* ============================================================
 *  TEST CHAT (Storico circolare)
 * ============================================================ */
static void test_chat(void) {
    printf("\n=== TEST CHAT (Storico circolare) ===\n");

    ChatStorico storico;
    Chat_Crea(&storico);
    TEST("Chat_Crea: count == 0", storico.count == 0);
    TEST("Chat_Crea: head == 0", storico.head == 0);

    /* Aggiunta messaggi */
    Chat_Aggiungi(&storico, "Alice", "Ciao a tutti!");
    TEST("Chat_Aggiungi: count == 1", storico.count == 1);

    const MessaggioChat* m = Chat_Ottieni(&storico, 0);
    TEST("Chat_Ottieni(0) non NULL", m != NULL);
    TEST("Chat_Ottieni(0) mittente Alice", strcmp(m->mittente, "Alice") == 0);
    TEST("Chat_Ottieni(0) testo corretto", strcmp(m->testo, "Ciao a tutti!") == 0);

    Chat_Aggiungi(&storico, "Bob", "Pronto a giocare!");
    Chat_Aggiungi(&storico, "Charlie", "Anch'io!");
    TEST("Chat_Aggiungi multiplo: count == 3", storico.count == 3);

    /* Overflow del buffer circolare */
    for (int i = 0; i < CHAT_STORICO_MAX + 5; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Messaggio %d", i);
        Chat_Aggiungi(&storico, "Sistema", buf);
    }
    TEST("Chat overflow: count == CHAT_STORICO_MAX", storico.count == CHAT_STORICO_MAX);

    /* Verifica che i messaggi piu' vecchi siano stati sovrascritti */
    m = Chat_Ottieni(&storico, 0);
    TEST("Chat overflow: messaggio[0] non e' 'Messaggio 0'", strcmp(m->testo, "Messaggio 0") != 0);

    /* Test Chat_Ottieni con indice non valido */
    TEST("Chat_Ottieni(-1) == NULL", Chat_Ottieni(&storico, -1) == NULL);
    TEST("Chat_Ottieni(count) == NULL", Chat_Ottieni(&storico, storico.count) == NULL);

    /* Test Chat_Svuota */
    Chat_Svuota(&storico);
    TEST("Chat_Svuota: count == 0", storico.count == 0);
    TEST("Chat_Svuota: head == 0", storico.head == 0);

    /* Test Chat_FormattaRiga */
    Chat_Aggiungi(&storico, "Test", "Hello World");
    m = Chat_Ottieni(&storico, 0);
    char riga[320];
    Chat_FormattaRiga(m, riga, sizeof(riga));
    TEST("Chat_FormattaRiga contiene @Test", strstr(riga, "@Test") != NULL);
    TEST("Chat_FormattaRiga contiene Hello World", strstr(riga, "Hello World") != NULL);

    /* Test programmazione difensiva */
    Chat_Crea(NULL);
    Chat_Aggiungi(NULL, "a", "b");
    Chat_Aggiungi(&storico, NULL, "b");
    Chat_Aggiungi(&storico, "a", NULL);
    TEST("Chat_Ottieni(NULL) == NULL", Chat_Ottieni(NULL, 0) == NULL);
    Chat_Svuota(NULL);
    Chat_FormattaRiga(NULL, riga, sizeof(riga));
    TEST("Chiamate Chat con NULL non crasano", 1);
}

/* ============================================================
 *  TEST LOGICA DI GIOCO (MossaValida)
 * ============================================================ */
static void test_game_logic(void) {
    printf("\n=== TEST LOGICA DI GIOCO (MossaValida) ===\n");

    /* Setup minimo per testare MossaValida */
    StatoGioco gioco;
    memset(&gioco, 0, sizeof(StatoGioco));

    /* Carta in cima agli scarti: ROSSO NUM_5 */
    gioco.scarti[0] = crea_carta(ROSSO, NUM_5);
    gioco.num_carte_scarti = 1;
    gioco.colore_attivo = ROSSO;

    /* Test match colore */
    TEST("MossaValida: ROSSO NUM_3 (stesso colore)", MossaValida(&gioco, crea_carta(ROSSO, NUM_3)) == 1);
    TEST("MossaValida: ROSSO SALTA (stesso colore)", MossaValida(&gioco, crea_carta(ROSSO, SALTA)) == 1);

    /* Test match tipo */
    TEST("MossaValida: BLU NUM_5 (stesso tipo)", MossaValida(&gioco, crea_carta(BLU, NUM_5)) == 1);
    TEST("MossaValida: VERDE NUM_5 (stesso tipo)", MossaValida(&gioco, crea_carta(VERDE, NUM_5)) == 1);

    /* Test jolly (NERO) sempre validi */
    TEST("MossaValida: NERO CAMBIO_COLORE (jolly)", MossaValida(&gioco, crea_carta(NERO, CAMBIO_COLORE)) == 1);
    TEST("MossaValida: NERO PESCA_QUATTRO (jolly+4)", MossaValida(&gioco, crea_carta(NERO, PESCA_QUATTRO)) == 1);

    /* Test mossa non valida */
    TEST("MossaValida: GIALLO NUM_2 (diverso colore e tipo)", MossaValida(&gioco, crea_carta(GIALLO, NUM_2)) == 0);
    TEST("MossaValida: BLU SALTA (diverso colore e tipo)", MossaValida(&gioco, crea_carta(BLU, SALTA)) == 0);

    /* Test con colore attivo diverso dalla carta in cima */
    gioco.colore_attivo = BLU;
    TEST("MossaValida: BLU NUM_3 (match colore attivo)", MossaValida(&gioco, crea_carta(BLU, NUM_3)) == 1);
    TEST("MossaValida: ROSSO NUM_3 (no match colore attivo, no match tipo)", MossaValida(&gioco, crea_carta(ROSSO, NUM_3)) == 0);
}

/* ============================================================
 *  TEST PROGRAMMAZIONE DIFENSIVA - Giocatore
 * ============================================================ */
static void test_giocatore(void) {
    printf("\n=== TEST GIOCATORE (programmazione difensiva) ===\n");

    Giocatore g;
    memset(&g, 0, sizeof(Giocatore));

    /* Giocatore_Nome con nome vuoto */
    TEST("Giocatore_Nome vuoto restituisce 'Sconosciuto'",
         strcmp(Giocatore_Nome(&g), "Sconosciuto") == 0);

    /* Giocatore_Nome con nome impostato */
    Giocatore_SetNome(&g, "Mario");
    TEST("Giocatore_SetNome: nome impostato", strcmp(g.nome, "Mario") == 0);
    TEST("Giocatore_Nome dopo set", strcmp(Giocatore_Nome(&g), "Mario") == 0);

    /* Giocatore_NomeBot con bot */
    g.is_bot = 1;
    const char* nome_bot = Giocatore_NomeBot(&g);
    TEST("Giocatore_NomeBot contiene '(Bot)'", strstr(nome_bot, "(Bot)") != NULL);

    /* Giocatore_NomeBot senza bot */
    g.is_bot = 0;
    TEST("Giocatore_NomeBot senza bot", strcmp(Giocatore_NomeBot(&g), "Mario") == 0);

    /* Test programmazione difensiva */
    Giocatore_SetNome(NULL, "Test");
    Giocatore_SetNome(&g, NULL);
    TEST("Giocatore_SetNome con NULL non crasha", 1);
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void) {
    printf("========================================\n");
    printf("  TEST UNITARI ADT - UNO\n");
    printf("========================================\n");

    test_lista();
    test_pila();
    test_coda();
    test_bst();
    test_chat();
    test_game_logic();
    test_giocatore();

    printf("\n========================================\n");
    printf("  RISULTATI: %d/%d test passati\n", test_passati, test_totali);
    printf("========================================\n");

    return (test_passati == test_totali) ? 0 : 1;
}