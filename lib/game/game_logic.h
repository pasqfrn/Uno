/**
 * @file game_logic.h
 * @brief Modulo di logica di gioco UNO.
 * @defgroup game_logic Logica di Gioco
 * @brief Gestisce le regole del gioco UNO, validazione mosse, effetti carte,
 * salvataggio/caricamento partite e manipolazione ADT (mazzo, scarti, mani).
 */

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "../data_structures/data_structures.h"
#include "player.h"
#include "private/player_private.h"      /* definizione completa di Giocatore (per accesso ai campi nei .c) */
#include "game_state.h"
#include "private/game_state_private.h"  /* definizione completa di StatoGioco (per accesso ai campi nei .c) */
#include "../data_structures/stack.h"
#include "../data_structures/queue.h"
#include "../data_structures/list.h"

/** @brief ADT globali del gioco: pila mazzo pesca, pila scarti, coda turni. */
extern Pila* mazzo_pila;
extern Pila* scarti_pila;
extern CodaTurni* coda_turni;

/* ============================================================
 *  GESTIONE PARTITE (Salvataggio / Caricamento)
 * ============================================================ */

/**
 * @brief Carica una partita da un file di salvataggio.
 * @pre gioco != NULL; slot nel range valido.
 * @post Stato partita ricaricato; ADT ricostruiti se necessario.
 * @param gioco Puntatore allo stato di gioco da popolare.
 * @param slot Indice dello slot di salvataggio (1-5).
 * @return 1 se il caricamento e' riuscito, 0 altrimenti.
 */
int CaricaPartitaDaSlot(StatoGioco *gioco, int slot);

/**
 * @brief Ricostruisce le strutture ADT statiche (mazzo_pila, scarti_pila,
 *        coda_turni) e le mani dei giocatori dopo un caricamento da file.
 *
 * Deve essere chiamata SUBITO DOPO CaricaPartitaDaSlot() per rendere
 * consistente lo stato interno con quello caricato dal file.
 * @pre gioco caricato con dati consistenti.
 * @post ADT ricostruiti e mani ricreate.
 * @param gioco Puntatore allo stato di gioco appena caricato.
 */
void RicostruisciStatoADT(StatoGioco *gioco);

/**
 * @brief Elimina un file di salvataggio esistente.
 * @pre slot nel range valido.
 * @post File di salvataggio rimosso.
 * @param slot Indice dello slot di salvataggio da eliminare.
 */
void EliminaSalvataggio(int slot);

/**
 * @brief Salva lo stato corrente della partita su file.
 * @pre gioco != NULL; partita inizializzata.
 * @post File di salvataggio aggiornato.
 * @param gioco Puntatore allo stato di gioco da salvare.
 */
void SalvaPartita(StatoGioco *gioco);
void CompattaSalvataggiUtente(void);
void EliminaTuttiSalvataggiUtente(const char* username);
void GetSaveFilePath(int user_index, int slot, char* buffer, size_t bufsize);

/**
 * @brief Registra lo slot di salvataggio da cui e' stata caricata la partita in corso.
 * @ingroup game_logic
 *
 * Tracciamento in memoria interna al modulo (variabile statica).
 * Non modifica il formato dei file: serve a sapere, alla fine della
 * partita, SE e QUale cancellare dalla bacheca.
 * -1 = partita nuova/online (nessun salvataggio di origine).
 *
 * @param slot Indice dello slot caricato (1-100), oppure -1.
 */
void RegistraSlotCaricato(int slot);

/**
 * @brief Restituisce lo slot di salvataggio da cui e' stata caricata la partita.
 * @ingroup game_logic
 * @return Slot registrato con RegistraSlotCaricato(), oppure -1 se la partita
 *         non proviene da un caricamento (nuova partita / online).
 */
int OttieniSlotCaricato(void);



/* ============================================================
 *  GESTIONE MAZZO E PESCA
 * ============================================================ */

/**
 * @brief Inizializza il mazzo di pesca con 108 carte UNO standard.
 *
 * Crea 4 copie di ogni carta (ROSSO, VERDE, BLU, GIALLO) con valori
 * da 0 a 9, piu' le carte speciali (CambioGiro, Salta, Più2) e
 * le 4 Jolly + 4 JollyPiù4 nere. Il mazzo viene poi mescolato
 * con l'algoritmo di Fisher-Yates.
 *
 * @pre gioco != NULL.
 * @post mazzo_pila inizializzata con 108 carte mescolate.
 * @param gioco Puntatore allo stato di gioco contenente il mazzo.
 */
void InizializzaMazzo(StatoGioco *gioco);

/**
 * @brief Ricarica il mazzo di pesca dagli scarti.
 *
 * Quando il mazzo di pesca e' vuoto, prende tutte le carte dalla
 * pila degli scarti (tranne l'ultima in cima) e le rimette nel
 * mazzo di pesca dopo averle rimescolate con Fisher-Yates.
 *
 * @pre gioco != NULL; scarti_pila non vuota.
 * @post mazzo_pila ricaricata e rimescolata.
 * @param gioco Puntatore allo stato di gioco.
 */
void RicarcaMazzo(StatoGioco *gioco);

/* ============================================================
 *  VALIDAZIONE E APPLICAZIONE MOSSE
 * ============================================================ */

/**
 * @brief Verifica se una carta puo' essere giocata legalmente.
 *
 * Controlla se il colore o il valore della carta corrisponde
 * alla carta in cima alla pila degli scarti, oppure se e' un Jolly.
 *
 * @pre gioco != NULL; gioco->num_carte_scarti > 0.
 * @post Restituisce 1 se mossa valida, 0 altrimenti.
 * @param gioco Puntatore allo stato di gioco corrente.
 * @param c La carta che si vuole giocare.
 * @return 1 se la mossa e' valida, 0 altrimenti.
 */
int MossaValida(StatoGioco *gioco, Carta c);

/**
 * @brief Applica l'effetto speciale di una carta giocata.
 *
 * Gestisce gli effetti di tutte le carte speciali:
 * - CAMBIA_GIRO: Inverte la direzione del turno.
 * - SALTA: Salta il giocatore successivo.
 * - PIU_2: Il giocatore successivo pesca 2 carte.
 * - JOLLY: Il giocatore sceglie un colore.
 * - JOLLY_PIU_4: Il giocatore successivo pesca 4 carte e si sceglie un colore.
 *
 * @pre gioco != NULL; carta valida e giocabile.
 * @post Stato gioco aggiornato con effetto applicato.
 * @param gioco Puntatore allo stato di gioco.
 * @param c La carta giocata con effetto speciale.
 * @param colore_scelto Il colore scelto dal giocatore (per Jolly/JollyPiù4).
 */
void ApplicaEffetto(StatoGioco *gioco, Carta c, Colore colore_scelto);

/* ============================================================
 *  INIZIALIZZAZIONE E FINE PARTITA
 * ============================================================ */

/**
 * @brief Inizializza una nuova partita con il numero di avversari specificato.
 *
 * Crea il mazzo, distribuisce 7 carte a ogni giocatore, pesca la prima
 * carta dagli scarti e gestisce le eventuali carte speciali iniziali.
 *
 * @pre gioco != NULL; num_avversari in range 1-3.
 * @post Stato partita inizializzato e pronto per il gioco.
 * @param gioco Puntatore allo stato di gioco da inizializzare.
 * @param num_avversari Numero di avversari (bot).
 */
void NuovaPartita(StatoGioco *gioco, int num_avversari);

/**
 * @brief Libera tutte le risorse allocate per una partita.
 *
 * Distrugge la mano di ogni giocatore (liste collegate),
 * svuota le pile di pesca e scarti, libera l'albero BST.
 *
 * @pre gioco != NULL.
 * @post Tutte le risorse liberate; ADT resettate.
 * @param gioco Puntatore allo stato di gioco da deallocare.
 */
void EliminaPartita(StatoGioco *gioco);

/**
 * @brief Mostra una notifica temporanea a schermo.
 *
 * Imposta il testo della notifica e un timer di visualizzazione.
 * La notifica viene disegnata in DisegnaGameplay.
 *
 * @pre gioco != NULL; msg != NULL.
 * @post Notifica attiva per 3 secondi.
 * @param gioco Puntatore allo stato di gioco.
 * @param msg Testo della notifica da mostrare.
 */
void MostraNotifica(StatoGioco *gioco, const char* msg);

/* ============================================================
 *  FUNZIONI HELPER PER ACCESSO ALLE MANI (Liste collegate)
 * ============================================================ */

/**
 * @brief Restituisce il numero di carte nella mano di un giocatore.
 * @pre giocatore != NULL.
 * @param giocatore Puntatore al giocatore.
 * @return Numero di nodi (carte) nella lista della mano.
 */
int GetLunghezzaMano(Giocatore *giocatore);

/**
 * @brief Restituisce una carta dalla mano di un giocatore per indice.
 * @pre giocatore != NULL; indice < lunghezza mano.
 * @param giocatore Puntatore al giocatore.
 * @param indice Indice della carta nella lista (0-based).
 * @return La carta all'indice specificato.
 */
Carta GetCartaGiocatore(Giocatore *giocatore, int indice);

/**
 * @brief Rimuove una carta dalla mano di un giocatore per indice.
 * @pre giocatore != NULL; indice < lunghezza mano.
 * @post Carta rimossa dalla mano; nodo deallocato.
 * @param giocatore Puntatore al giocatore.
 * @param indice Indice della carta da rimuovere (0-based).
 */
void RimuoviCartaGiocatore(Giocatore *giocatore, int indice);

/**
 * @brief Rimescola le carte dalla pila degli scarti nel mazzo di pesca.
 *
 * Prende tutte le carte dalla pila degli scarti (tranne l'ultima in cima
 * che rimane come carta attiva) e le rimette nel mazzo di pesca
 * applicando l'algoritmo di Fisher-Yates per la mescolatura.
 *
 * @pre gioco != NULL; scarti_pila ha almeno 2 carte.
 * @post mazzo_pila ricaricata e rimescolata; scarti ridotte a 1.
 * @param gioco Puntatore allo stato di gioco.
 */
void RimescolaScartiNelMazzo(StatoGioco *gioco);

/**
 * @brief Fa pescare un certo numero di carte a un giocatore.
 *
 * Pesca le carte dalla pila di pesca e le aggiunge alla mano
 * del giocatore. Se il mazzo di pesca finisce, ricarica dagli scarti.
 *
 * @pre gioco != NULL; id_giocatore valido; quantita > 0.
 * @post quantita carte aggiunte alla mano del giocatore.
 * @param gioco Puntatore allo stato di gioco.
 * @param id_giocatore Indice del giocatore che pesca.
 * @param quantita Numero di carte da pescare.
 */
void Pesca(StatoGioco *gioco, int id_giocatore, int quantita);

/* ============================================================
 *  FUNZIONI DI DOMINIO BASATE SUGLI ADT
 *  Uso ESPLICITO degli ADT (ListaCarte, CodaTurni, Pila) nelle
 *  funzionalita' di dominio del gioco. Queste funzioni incapsulano
 *  l'uso degli ADT: gli utilizzatori non accedono mai alla
 *  rappresentazione interna (information hiding).
 * ============================================================ */

/**
 * @brief (Ri)costruisce la coda dei turni ADT a partire dall'array dei giocatori.
 *
 * Funzionalita' di dominio: sincronizza l'ADT CodaTurni con lo StatoGioco.
 * Usata alla creazione di una nuova partita e dopo il caricamento da file.
 *
 * @pre gioco != NULL.
 * @post coda_turni ricreata con i giocatori di gioco->giocatori[].
 * @param gioco Puntatore allo stato di gioco.
 * @return 1 se la coda e' stata ricreata con successo, 0 altrimenti.
 */
int GameLogic_SincronizzaCodaTurni(StatoGioco *gioco);

/**
 * @brief Avanza il turno di `passi` posizioni usando l'ADT CodaTurni.
 *
 * Funzionalita' di dominio: fa avanzare il turno corrente ruotando
 * l'ADT CodaTurni (Coda_Ruota) in base a `direzione` e aggiorna
 * turno_corrente nello StatoGioco.
 *
 * @pre gioco != NULL; passi > 0; num_giocatori > 0.
 * @post turno_corrente avanzato di passi; coda_turni ruotata di conseguenza.
 * @param gioco Puntatore allo stato di gioco.
 * @param passi Numero di posizioni da avanzare (1 = turno successivo).
 */
void GameLogic_AvanzamentoTurno(StatoGioco *gioco, int passi);

/**
 * @brief Copia la mano (ADT ListaCarte) di un giocatore nel backup flat.
 *
 * Funzionalita' di dominio: prepara la serializzazione su file della mano.
 * La mano viene letta tramite l'API pubblica della lista e copiata in
 * mano_backup[], senza esporre i nodi interni.
 *
 * @pre g != NULL.
 * @post mano_backup[] e num_carte_backup aggiornati con la mano corrente.
 * @param g Puntatore al giocatore.
 * @return Numero di carte copiate nel backup (0 se mano nulla).
 */
int GameLogic_PreparaManoSalvataggio(Giocatore *g);

/**
 * @brief Ricostruisce la mano (ADT ListaCarte) di un giocatore dal backup flat.
 *
 * Funzionalita' di dominio: dopo CaricaPartita ri-crea la lista collegata
 * della mano a partire da mano_backup[].
 *
 * @pre g != NULL; mano_backup valido.
 * @post g->mano allocata e popolata con mano_backup[].
 * @param g Puntatore al giocatore.
 * @return 1 se la mano e' stata ricostruita, 0 altrimenti.
 */
int GameLogic_RicostruisciMano(Giocatore *g);

/**
 * @brief Sposta la prima carta della mano in fondo (ordine visivo dopo una pescata).
 *
 * Funzionalita' di dominio basata sull'ADT ListaCarte (rotazione O(1)).
 *
 * @pre g != NULL.
 * @post La prima carta della mano e' ora l'ultima.
 * @param g Puntatore al giocatore.
 */
void GameLogic_MettiPrimaInFondo(Giocatore *g);

/**
 * @brief Sostituisce per valore la carta all'indice nella mano del giocatore.
 *
 * Funzionalita' di dominio: consente di aggiornare i campi di una carta
 * (area di rendering, posizione) durante il trascinamento senza esporre
 * i nodi interni dell'ADT ListaCarte.
 *
 * @pre g != NULL; indice valido.
 * @post Carta all'indice aggiornata con la copia fornita.
 * @param g Puntatore al giocatore.
 * @param indice Indice della carta (0-based).
 * @param c Nuova carta da impostare.
 */
void GameLogic_ImpostaCarta(Giocatore *g, int indice, Carta c);

/**
 * @brief Copia le carte della mano di un giocatore in un array esterno.
 *
 * Funzionalita' di dominio: ritorna una snapshot della mano tramite
 * l'API pubblica dell'ADT ListaCarte (nessun accesso alla rappresentazione).
 *
 * @pre g != NULL; out != NULL; max > 0.
 * @post out popolato con le carte della mano (max o meno).
 * @param g Puntatore al giocatore.
 * @param out Array di destinazione.
 * @param max Dimensione massima dell'array.
 * @return Numero di carte copiate.
 */
int GameLogic_CopiaMano(const Giocatore *g, Carta* out, int max);

/**
 * @brief Distrugge la mano (ADT ListaCarte) di un giocatore.
 *
 * Funzionalita' di dominio: dealloca la lista collegata della mano
 * e resetta i campi correlati.
 *
 * @pre g != NULL.
 * @post g->mano == NULL; num_carte_mano = 0.
 * @param g Puntatore al giocatore.
 */
void GameLogic_DistruggiMano(Giocatore *g);

/**
 * @brief Restituisce il numero di carte della mano usando l'API ADT.
 * @pre g != NULL.
 * @return Numero di carte in mano (0 se mano nulla o vuota).
 * @param g Puntatore al giocatore.
 */
int GameLogic_GetLunghezzaMano(const Giocatore *g);

#endif