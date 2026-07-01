/**
 * @file stats_screen.c
 * @brief Implementazione schermata statistiche utente.
 * @ingroup menu_screen
 *
 * Mostra le statistiche dell'utente loggato:
 *   - Partite giocate totali, vittorie, sconfitte
 *   - Storico partite recenti con modalita', esito, posizione, durata
 *   - Scrollbar dragabile per navigare lo storico
 *
 * STATISTICHE READ-ONLY: Questa schermata e' SOLO in lettura.
 * Non e' possibile modificare le statistiche da qui.
 * Le modifiche avvengono solo tramite PACKET_STATS_SYNC dopo ogni partita.
 *
 * Stati gestiti: FASE_STATISTICHE
 */
#include <string.h>
#include <stdlib.h>
#include "raylib.h"
#include "../../../lib/screens/menu/stats_screen.h"
#include "../../../lib/auth.h"
#include "../../../lib/ui.h"

#define LARGHEZZA 1280
#define ALTEZZA 720

/**
 * @brief Disegna la schermata statistiche dell'utente loggato.
 * @ingroup menu_screen
 * @pre fase != NULL.
 * @post Statistiche renderizzate; navigazione gestita.
 * @param fase Fase corrente.
 */
void DisegnaStatistiche(FaseApplicazione* fase) {
    if (textureSfondo.id != 0) {
        DrawTexturePro(textureSfondo,
                       (Rectangle){ 0.0f, 0.0f, (float)textureSfondo.width, (float)textureSfondo.height },
                       (Rectangle){ 0.0f, 0.0f, (float)LARGHEZZA, (float)ALTEZZA },
                       (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    }
    DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.6f));

    Statistiche* utente = &dbUtenti.lista[id_utente_corrente];

    DrawText(TextFormat("STATISTICHE DI %s", utente->username), LARGHEZZA/2 - MeasureText(TextFormat("STATISTICHE DI %s", utente->username), 44)/2 + 2, 42, 44, BLACK);
    DrawText(TextFormat("STATISTICHE DI %s", utente->username), LARGHEZZA/2 - MeasureText(TextFormat("STATISTICHE DI %s", utente->username), 44)/2, 40, 44, coloriRaylib[1]);

    // Contenitore scrollabile
    Rectangle areaLista = { 140, 110, 1000, 480 };
    DrawRectangleRounded(areaLista, 0.1f, 10, Fade(coloriRaylib[4], 0.95f));
    DrawRectangleRoundedLines(areaLista, 0.1f, 10, coloriRaylib[1]);

    // Intestazioni
    int y_start = 140;
    int col_num = 160;
    int col_mod = 370;
    int col_pos = 620;
    int col_esi = 750;
    int col_dur = 960;

    DrawText("NUMERO", col_num, y_start, 20, coloriRaylib[1]);
    DrawText("MODALITA", col_mod, y_start, 20, coloriRaylib[1]);
    DrawText("POS", col_pos, y_start, 20, coloriRaylib[1]);
    DrawText("ESITO", col_esi, y_start, 20, coloriRaylib[1]);
    DrawText("DURATA", col_dur, y_start, 20, coloriRaylib[1]);

    DrawLine(160, y_start + 35, 1120, y_start + 35, LIGHTGRAY);

    // Scroll
    static float scroll_offset = 0;
    int partite_totali = utente->partite_giocate;
    int riga_h = 35;
    int righe_visibili = 10;
    int area_lista_y = y_start + 50;
    int area_lista_h = areaLista.y + areaLista.height - area_lista_y - 10;

    /* BUG 8 FIX: Calcola il numero di righe VALIDE (solo partite con esito/durata)
     * invece di usare partite_totali che include buchi/vuoti. */
    int righe_totali = 0;
    for (int v = 0; v < utente->partite_giocate && v < MAX_PARTITE_STORICO; v++) {
        if (utente->storico_partite[v].esito[0] != '\0' && utente->storico_partite[v].durata_secondi != 0) {
            righe_totali++;
        }
    }
    float max_scroll = (righe_totali > righe_visibili) ? (righe_totali - righe_visibili) * riga_h : 0;

    // Rotellina
    scroll_offset -= GetMouseWheelMove() * 20.0f;
    if (scroll_offset < 0) scroll_offset = 0;
    if (scroll_offset > max_scroll) scroll_offset = max_scroll;

    // Barra laterale
    int barra_x = 1120;
    int barra_y = area_lista_y;
    int barra_h = area_lista_h;
    int barra_larghezza = 12;
    DrawRectangleRounded((Rectangle){ (float)barra_x, (float)barra_y, (float)barra_larghezza, (float)barra_h }, 0.3f, 5, Fade(DARKGRAY, 0.5f));
    if (max_scroll > 0) {
        float barra_thumb_h = barra_h * (righe_visibili / (float)righe_totali);
        float barra_thumb_y = barra_y + (scroll_offset / max_scroll) * (barra_h - barra_thumb_h);
        DrawRectangleRounded((Rectangle){ (float)barra_x, barra_thumb_y, (float)barra_larghezza, barra_thumb_h }, 0.3f, 5, GRAY);

        // Click e drag sulla barra
        Vector2 mouse = GetMousePosition();
        static int dragging = 0;
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(mouse, (Rectangle){ (float)barra_x, barra_thumb_y, (float)barra_larghezza, barra_thumb_h })) {
            dragging = 1;
        }
        if (dragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            float mouse_rel = (mouse.y - barra_y) / barra_h;
            scroll_offset = mouse_rel * max_scroll;
            if (scroll_offset < 0) scroll_offset = 0;
            if (scroll_offset > max_scroll) scroll_offset = max_scroll;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) dragging = 0;
    }

    // Contenuto scrollato (clip)
    BeginScissorMode((int)areaLista.x, (int)area_lista_y, (int)areaLista.width, (int)area_lista_h);

    int row_y = area_lista_y - (int)scroll_offset;

    if (partite_totali == 0) {
        DrawText("Nessuna partita giocata ancora.", LARGHEZZA/2 - MeasureText("Nessuna partita giocata ancora.", 20)/2, area_lista_y + 50, 20, GRAY);
    } else {
        int partita_counter = 0;
        for (int i = 0; i < partite_totali; i++) {
            PartitaRegistrata p = utente->storico_partite[i];
            if (p.esito[0] == '\0' || p.durata_secondi == 0) continue; // Salta partite corrotte
            partita_counter++;
            int minuti = p.durata_secondi / 60;
            int secondi = p.durata_secondi % 60;
            Color esito_color = (strcmp(p.esito, "Vittoria") == 0) ? GREEN : RED;
            DrawText(TextFormat("Partita %d", partita_counter), col_num, row_y, 20, LIGHTGRAY);
            DrawText(p.modalita, col_mod, row_y, 20, WHITE);
            DrawText(TextFormat("%do", p.posizione), col_pos, row_y, 20, (p.posizione == 1) ? GOLD : WHITE);
            DrawText(p.esito, col_esi, row_y, 20, esito_color);
            DrawText(TextFormat("%02d:%02d", minuti, secondi), col_dur, row_y, 20, WHITE);
            row_y += riga_h;
        }
    }

    EndScissorMode();

    if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 100, 620, 200, 50 }, "Indietro")) {
        *fase = FASE_MENU;
    }
}