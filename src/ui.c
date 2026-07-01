/**
 * @file ui.c
 * @brief Implementazione modulo di rendering dell'interfaccia utente.
 * @ingroup ui
 *
 * Contiene le funzioni per il disegno di elementi UI comuni:
 *   - CaricaTextureReali: carica tutte le texture delle carte e sfondo
 *   - ScaricaTextureReali: libera le texture RAM
 *   - DisegnaCartaRealistica: disegna una carta con texture vera
 *   - DisegnaBottoneMenuAnimato: bottone con effetto hover/press
 *
 * Tutte le funzioni usano Raylib per il rendering.
 * Le texture sono globali (array textureCarte[5][15]).
 */
#include "../lib/ui.h"
#include <stdio.h>

/**
 * @addtogroup ui_texture Texture e Risorse Grafiche
 * @brief Texture globali per il rendering delle carte e sfondo.
 * @{
 */
/** @brief Texture delle carte UNO: textureCarte[colore][tipo]. */
Texture2D textureCarte[5][15];
/** @brief Texture del dorso delle carte (per carte coperte). */
Texture2D textureDorso;
/** @brief Texture dello sfondo del gioco. */
Texture2D textureSfondo;

/** @brief Colori Raylib associati ai colori UNO (ROSSO, GIALLO, VERDE, BLU, NERO). */
const Color coloriRaylib[] = { 
    (Color){ 218, 41, 28, 255 },
    (Color){ 255, 199, 44, 255 },
    (Color){ 0, 154, 68, 255 },
    (Color){ 0, 130, 202, 255 },
    (Color){ 30, 30, 30, 255 }
};
/** @brief Nomi descrittivi dei colori UNO (per debug/UI). */
const char* nomiColori[] = { "Rosso", "Giallo", "Verde", "Blu", "Jolly" };
/** @} */

/* ============================================================
 *  GESTIONE TEXTURE
 * ============================================================ */

/**
 * @brief Carica tutte le texture delle carte e lo sfondo da file PNG.
 *
 * Carica textureCarte per tutti i colori e tipi, textureDorso e textureSfondo.
 * Applica filtro bilineare per scaling di qualita'.
 *
 * @pre Cartelle assets/card/cards_assets/ e assets/altro/ esistono con i PNG richiesti.
 * @post Texture globali caricate in RAM; filtri bilineari applicati.
 */
void CaricaTextureReali(void) {
    const char* prefissi[] = { "rosso", "giallo", "verde", "blu" };
    char percorso[150]; // Aumentato a 150 per evitare overflow con percorsi lunghi

    // Presumo che anche il dorso sia insieme alle altre carte
    textureDorso = LoadTexture("assets/card/cards_assets/dorso.png");
    SetTextureFilter(textureDorso, TEXTURE_FILTER_BILINEAR);
    
    // Se lo sfondo si trova nella cartella principale "assets", lascialo così. 
    // Se lo hai spostato insieme alle carte, cambia il percorso in "assets/card/cards_assets/sfondo.png"
    textureSfondo = LoadTexture("assets/altro/sfondo.png");
    SetTextureFilter(textureSfondo, TEXTURE_FILTER_BILINEAR);

    for (int c = 0; c < 4; c++) {
        for (int t = 0; t <= PESCA_QUATTRO; t++) {
            // Aggiornati tutti i percorsi inserendo la sottocartella corretta
            if (t <= NUM_9) sprintf(percorso, "assets/card/cards_assets/%s_%d.png", prefissi[c], t);
            else if (t == SALTA) sprintf(percorso, "assets/card/cards_assets/%s_salta.png", prefissi[c]);
            else if (t == CAMBIA_GIRO) sprintf(percorso, "assets/card/cards_assets/%s_inverti.png", prefissi[c]);
            else if (t == PESCA_DUE) sprintf(percorso, "assets/card/cards_assets/%s_piu2.png", prefissi[c]);
            else continue;
            
            textureCarte[c][t] = LoadTexture(percorso);
            SetTextureFilter(textureCarte[c][t], TEXTURE_FILTER_BILINEAR);
        }
    }
    // Aggiornati anche i percorsi per le carte Jolly e Jolly+4
    textureCarte[NERO][CAMBIO_COLORE] = LoadTexture("assets/card/cards_assets/jolly.png");
    SetTextureFilter(textureCarte[NERO][CAMBIO_COLORE], TEXTURE_FILTER_BILINEAR);
    textureCarte[NERO][PESCA_QUATTRO] = LoadTexture("assets/card/cards_assets/jolly_piu4.png");
    SetTextureFilter(textureCarte[NERO][PESCA_QUATTRO], TEXTURE_FILTER_BILINEAR);
}

/**
 * @brief Libera dalla RAM tutte le texture caricate.
 *
 * Deve essere chiamata alla chiusura del gioco per evitare memory leak.
 *
 * @pre Texture caricate in precedenza con CaricaTextureReali.
 * @post Risorse grafiche rilasciate dalla RAM.
 */
void ScaricaTextureReali(void) {
    UnloadTexture(textureDorso);
    UnloadTexture(textureSfondo);
    for (int c = 0; c < 5; c++) {
        for (int t = 0; t < 15; t++) {
            if (textureCarte[c][t].id != 0) UnloadTexture(textureCarte[c][t]);
        }
    }
}

/**
 * @brief Disegna una carta UNO con texture realistica e ombra.
 *
 * Se coperta=1 disegna il dorso, altrimenti la texture corrispondente
 * a colore/tipo. Applica ombra dinamica e bordo luminoso se trascinata.
 *
 * @pre c != NULL; r.width > 0 && r.height > 0.
 * @post Carta renderizzata a schermo con ombra e bordo se applicabile.
 * @param c Puntatore alla carta da disegnare.
 * @param r Rettangolo di destinazione sullo schermo.
 * @param coperta 1 per disegnare il dorso, 0 per la carta scoperta.
 */
void DisegnaCartaRealistica(Carta *c, Rectangle r, int coperta) {
    if (!c->isTrascinata) c->area = r; 
    Rectangle drawRect = r;

    float raggioOmbra = c->isTrascinata ? 12.0f : 5.0f;
    Color coloreOmbra = Fade(BLACK, c->isTrascinata ? 0.6f : 0.3f);
    DrawRectangleRounded((Rectangle){ drawRect.x + raggioOmbra, drawRect.y + raggioOmbra, drawRect.width, drawRect.height }, 0.15f, 10, coloreOmbra);

    Texture2D tex = coperta ? textureDorso : textureCarte[c->colore][c->tipo];

    if (tex.id != 0) {
        Rectangle sourceRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
        Vector2 origin = { 0.0f, 0.0f };
        DrawTexturePro(tex, sourceRec, drawRect, origin, 0.0f, WHITE);
    }
    if (c->isTrascinata) {
        DrawRectangleRoundedLines(drawRect, 0.15f, 10, Fade(LIME, 0.8f));
    }
}

/**
 * @brief Disegna un bottone animato per il menu con effetto hover/press.
 *
 * Il bottone ingrandisce leggermente al passaggio del mouse e si "preme"
 * quando cliccato. Restituisce 1 se e' stato cliccato.
 *
 * @pre testo != NULL; rect dimensioni > 0.
 * @post Bottone renderizzato; 1 se click confermato, 0 altrimenti.
 * @param rect Rettangolo di posizione/dimensioni del bottone.
 * @param testo Testo da visualizzare.
 * @return 1 se il bottone e' stato cliccato, 0 altrimenti.
 */
int DisegnaBottoneMenuAnimato(Rectangle rect, const char* testo) {
    int cliccato = 0;
    Vector2 mousePos = GetMousePosition();
    Color color = Fade(BLACK, 0.7f);
    
    if (CheckCollisionPointRec(mousePos, rect)) {
        color = Fade(DARKBLUE, 0.9f);
        rect.x -= 5; rect.y -= 5;
        rect.width += 10; rect.height += 10;
        
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            rect.x += 8; rect.y += 8;
            rect.width -= 16; rect.height -= 16;
            color = Fade(BLUE, 0.9f);
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) cliccato = 1;
    }
    
    DrawRectangleRounded(rect, 0.2f, 10, color);
    DrawRectangleRoundedLines(rect, 0.2f, 10, Fade(WHITE, 0.5f));
    int textW = MeasureText(testo, 24);
    DrawText(testo, rect.x + (rect.width - textW)/2, rect.y + rect.height/2 - 12, 24, WHITE);
    
    return cliccato;
}