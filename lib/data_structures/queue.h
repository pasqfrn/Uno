/**
 * @file queue.h
 * @brief ADT Coda (Queue) bidirezionale per la gestione dei turni di gioco.
 * @defgroup queue Queue (Coda)
 * @brief Implementa una coda doppiamente concatenata (deque) per gestire
 * i turni di gioco con supporto al Cambio Giro (inversione verso O(1)).
 *
 * PRINCIPIO DI INFORMATION HIDING:
 * I tipi `NodoCoda` e `CodaTurni` sono dichiarati come tipi OPACHI: la
 * loro rappresentazione interna e' definita esclusivamente nel modulo di
 * implementazione (src/data_structures/queue.c). Gli utilizzatori
 * interagiscono solo tramite l'API pubblica di questo header.
 */
#ifndef DATA_STRUCTURES_QUEUE_H
#define DATA_STRUCTURES_QUEUE_H

#include "data_structures.h"
#include "../game/player.h"

/**
 * @addtogroup queue_nodi Nodi Coda
 * @brief Tipo opaco del nodo della coda dei turni.
 * @{
 */
/** @brief Nodo della coda (tipo opaco: rappresentazione interna nascosta). */
typedef struct NodoCoda NodoCoda;
/** @} */

/**
 * @addtogroup queue_struttura Struttura Coda
 * @brief Container della coda dei turni di gioco.
 * @{
 */
/**
 * @brief Coda dei turni (tipo opaco: rappresentazione interna nascosta).
 *
 * Rappresenta l'ordine dei turni dei giocatori. I nodi interni (NodoCoda)
 * sono allocati/gestiti esclusivamente dalle funzioni di questo ADT.
 * Gli utilizzatori interagiscono solo tramite l'API pubblica.
 */
typedef struct CodaTurni CodaTurni;
/** @} */

/**
 * @addtogroup queue_api API Queue
 * @brief Funzioni per la gestione della coda dei turni.
 * @{
 */
/**
 * @brief Crea una coda vuota.
 * @post Coda allocata e inizializzata (size=0, verso=1).
 * @return Puntatore a CodaTurni, o NULL se out-of-memory.
 */
CodaTurni* Coda_Crea(void);

/**
 * @brief Distruttore: libera tutti i nodi e la struttura CodaTurni.
 * @pre c != NULL.
 * @post Memoria della coda completamente liberata.
 * @param c Puntatore a CodaTurni.
 */
void Coda_Distruggi(CodaTurni* c);

/**
 * @brief Inserisce un giocatore in coda (rear).
 * @pre c != NULL.
 * @post Giocatore aggiunto in coda; size incrementato.
 * @param c Puntatore a CodaTurni.
 * @param g Giocatore da inserire (copiato per valore).
 * @return 1 se successo, 0 altrimenti.
 */
int Coda_Enqueue(CodaTurni* c, Giocatore g);

/**
 * @brief Rimuove il giocatore in testa (front).
 * @pre c != NULL.
 * @post Giocatore rimosso; size decrementato; dati scritti in out (se non NULL).
 * @param c Puntatore a CodaTurni.
 * @param out Puntatore a Giocatore per ricevere l'elemento rimosso (può essere NULL).
 * @return 1 se successo, 0 se coda vuota.
 */
int Coda_Dequeue(CodaTurni* c, Giocatore* out);

/**
 * @brief Restituisce il giocatore in testa SENZA rimuoverlo.
 * @pre c != NULL e coda non vuota.
 * @post Giocatore in front scritto in out.
 * @param c Puntatore a CodaTurni.
 * @param out Puntatore a Giocatore per ricevere l'elemento in testa.
 * @return 1 se successo, 0 se coda vuota.
 */
int Coda_Front(const CodaTurni* c, Giocatore* out);

/**
 * @brief Restituisce il giocatore in fondo (rear) SENZA rimuoverlo.
 * @pre c != NULL e coda non vuota.
 * @post Giocatore in rear scritto in out.
 * @param c Puntatore a CodaTurni.
 * @param out Puntatore a Giocatore.
 * @return 1 se successo, 0 se coda vuota.
 */
int Coda_Rear(const CodaTurni* c, Giocatore* out);

/**
 * @brief Restituisce il nodo che segue `n` nella direzione corrente.
 *
 * Utilizza `verso` e i puntatori prev/next per determinare il prossimo turno in O(1).
 *
 * @pre c != NULL; n != NULL.
 * @return Puntatore al NodoCoda successivo, o NULL se non trovato.
 * @param c Puntatore a CodaTurni.
 * @param n Nodo corrente.
 */
NodoCoda* Coda_ProxNodo(const CodaTurni* c, NodoCoda* n);

/**
 * @brief Verifica che la coda sia consistente (debug/difensiva).
 *
 * Controlla che tutti i puntatori next/prev siano coerenti e che
 * size corrisponda al numero di nodi raggiungibili da front.
 *
 * @pre c != NULL.
 * @post Restituisce 1 se valida, 0 se inconsistente.
 * @param c Puntatore a CodaTurni.
 * @return 1 se valida, 0 altrimenti.
 */
int Coda_VerificaConsistenza(const CodaTurni* c);

/**
 * @brief Verifica se la coda è vuota.
 * @pre c != NULL.
 * @return 1 se vuota, 0 altrimenti.
 * @param c Puntatore a CodaTurni.
 */
int Coda_Vuota(const CodaTurni* c);

/**
 * @brief Restituisce il numero di giocatori in coda.
 * @pre c != NULL.
 * @return Numero di elementi.
 * @param c Puntatore a CodaTurni.
 */
int Coda_Dimensione(const CodaTurni* c);

/**
 * @brief Inverte il verso di scorrimento (carta Cambio Giro).
 *
 * Operazione O(1): inverte il segno di `verso`.
 *
 * @pre c != NULL.
 * @post verso *= -1.
 * @param c Puntatore a CodaTurni.
 */
void Coda_InvertiVerso(CodaTurni* c);

/**
 * @brief Ruota la coda di una posizione nella direzione corrente.
 *
 * Equivale alle operazioni Dequeue + Enqueue ma in O(1): il giocatore
 * in testa viene spostato in coda (verso orario) oppure il giocatore
 * in coda viene portato in testa (verso antiorario). Usata dal dominio
 * per far avanzare il turno senza ricostruire la coda.
 *
 * @pre c != NULL; coda non vuota.
 * @post La coda risulta ruotata di una posizione nel verso corrente.
 * @param c Puntatore a CodaTurni.
 */
void Coda_Ruota(CodaTurni* c);

/**
 * @brief Cerca il nodo all'indice specificato (0 = front).
 * @pre c != NULL; indice nel range [0, size-1].
 * @post Nessuna modifica allo stato.
 * @return Puntatore a NodoCoda, o NULL se fuori range.
 * @param c Puntatore a CodaTurni.
 * @param indice Indice del nodo (0-based).
 */
NodoCoda* Coda_CercaPerIndice(const CodaTurni* c, int indice);

/**
 * @brief Copia l'i-esimo giocatore in out (0 = front).
 * @pre c != NULL; indice nel range [0, size-1]; out != NULL.
 * @post Giocatore copiato in out.
 * @param c Puntatore a CodaTurni.
 * @param indice Indice del giocatore (0-based).
 * @param out Puntatore a Giocatore per il risultato.
 * @return 1 se successo, 0 altrimenti.
 */
int Coda_CopiaPerIndice(const CodaTurni* c, int indice, Giocatore* out);
/** @} */

#endif // DATA_STRUCTURES_QUEUE_H