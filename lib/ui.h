/**
 * @file ui.h
 * @brief Modulo di rendering dell'interfaccia utente.
 * @defgroup ui Rendering e Interfaccia Utente
 * @brief Gestisce il caricamento delle texture, il disegno delle carte
 * e dei componenti UI comuni (bottoni animati, menu).
 *
 * Utilizza Raylib per tutte le operazioni di rendering.
 */
#ifndef UI_H
#define UI_H

#include "data_structures.h"

/** @brief Cache globale delle texture delle carte UNO: textureCarte[colore][tipo]. */
extern Texture2D textureCarte[5][15];
/** @brief Texture del dorso delle carte (per carte coperte). */
extern Texture2D textureDorso;
/** @brief Texture dello sfondo del gioco. */
extern Texture2D textureSfondo;

/** @brief Colori Raylib associati ai colori UNO. */
extern const Color coloriRaylib[];
/** @brief Nomi descrittivi dei colori UNO. */
extern const char* nomiColori[];

/* ============================================================
 *  FUNZIONI DI RENDERING
 * ============================================================ */

/**
 * @brief Carica tutte le texture delle carte e lo sfondo da file PNG.
 * @pre Asset cartelle esistenti con PNG corretti.
 * @post Texture caricate in RAM e filtri bilineari applicati.
 */
void CaricaTextureReali(void);
/**
 * @brief Libera dalla RAM tutte le texture caricate.
 * @pre Texture caricate in precedenza.
 * @post Risorse grafiche rilasciate.
 */
void ScaricaTextureReali(void);
/**
 * @brief Disegna una carta UNO con texture realistica e ombra.
 * @pre c != NULL; r dimensioni valide.
 * @post Carta disegnata a schermo.
 * @param c Puntatore alla carta da disegnare.
 * @param r Rettangolo di destinazione sullo schermo.
 * @param coperta 1 per disegnare il dorso, 0 per la carta scoperta.
 */
void DisegnaCartaRealistica(Carta *c, Rectangle r, int coperta);
/**
 * @brief Disegna un bottone animato per il menu con effetto hover/press.
 * @pre rettangolo dimensioni valide; testo != NULL.
 * @post Bottone disegnato; 1 se cliccato, 0 altrimenti.
 * @param rect Rettangolo di posizione/dimensioni del bottone.
 * @param testo Testo da visualizzare.
 * @return 1 se cliccato, 0 altrimenti.
 */
int DisegnaBottoneMenuAnimato(Rectangle rect, const char* testo);

#endif
