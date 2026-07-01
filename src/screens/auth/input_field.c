/**
 * @file input_field.c
 * @brief Implementazione del campo di input interattivo.
 * @ingroup input_field
 *
 * ADT: InputField per login, registrazione, recupero password.
 * Gestione focus, inserimento caratteri, backspace, password mode.
 *
 * BUG 3 FIX: Il testo che supera la larghezza della cella viene ora
 * troncato con "..." finale, invece di uscire fuori dalla cella.
 * Viene calcolata la larghezza massima disponibile e il testo viene
 * scrollato automaticamente per mostrare sempre gli ultimi caratteri
 * inseriti (utile per campi password lunghi).
 */
#include <string.h>
#include "raylib.h"
#include "../../../lib/screens/auth/input_field.h"
#include "../../../lib/auth.h"
#include "../../../lib/ui.h"

/** @brief Flag globale per richiedere il submit del form. */
int submit_richiesto = 0;

/**
 * @brief Aggiorna lo stato di un campo input.
 * @ingroup input_field
 * @pre buffer != NULL; count != NULL; maxLength > 0.
 * @post buffer e count aggiornati.
 * @param buffer Buffer campo.
 * @param count Contatore caratteri.
 * @param maxLength Lunghezza massima.
 */
void AggiornaInputCampo(char* buffer, int* count, int maxLength) {
    int key = GetCharPressed();

    if (key > 0 || IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        errore_registrazione_email = 0;
        errore_registrazione_username = 0;
        errore_registrazione_pass = 0;
    }

    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (*count < maxLength - 1)) {
            buffer[*count] = (char)key;
            buffer[*count + 1] = '\0';
            (*count)++;
        }
        key = GetCharPressed();
    }
    if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && *count > 0) {
        (*count)--;
        buffer[*count] = '\0';
    }
}

/**
 * @brief Disegna un campo input su schermo con gestione overflow.
 * @ingroup input_field
 *
 * BUG 3 FIX: Il testo viene ora troncato se supera la larghezza
 * disponibile della cella. Viene calcolato un offset di scroll
 * per mostrare sempre gli ultimi caratteri inseriti (utile per
 * password lunghe). Se il testo troncato, vengono aggiunti ".."
 * alla fine per indicare che c'e' altro contenuto.
 *
 * @pre text != NULL; label != NULL.
 * @post Campo renderizzato.
 * @param area Rettangolo.
 * @param label Etichetta.
 * @param text Testo.
 * @param count Lunghezza testo.
 * @param isFocused Focus.
 * @param isPassword Password mode.
 * @param labelColor Colore label.
 */
void DisegnaInputField(Rectangle area, const char* label, const char* text, int count, int isFocused, int isPassword, Color labelColor) {
    Vector2 mousePos = GetMousePosition();
    int isHovered = CheckCollisionPointRec(mousePos, area);
    int isRevealed = isHovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    DrawText(label, area.x, area.y - 28, 20, labelColor);
    
    Color borderColor = isFocused ? coloriRaylib[1] : (isHovered ? coloriRaylib[3] : GRAY); 
    Color bgColor = isFocused ? Fade(WHITE, 0.98f) : (isHovered ? Fade(RAYWHITE, 0.9f) : Fade(RAYWHITE, 0.8f));
    
    DrawRectangleRounded(area, 0.15f, 10, bgColor);
    DrawRectangleRoundedLines(area, 0.15f, 10, borderColor);
    
    char displayText[100] = {0};
    if (isPassword > 0 && !isRevealed) {
        for(int i = 0; i < count; i++) displayText[i] = '*';
        displayText[count] = '\0';
    } else {
        strcpy(displayText, text);
    }
    
    /* ============================================================
     * BUG 3 FIX: Gestione overflow del testo nella cella.
     *
     * Calcola la larghezza massima disponibile per il testo
     * (larghezza cella - padding laterale di 30px).
     * Se il testo e' piu' largo, calcola un offset di scroll
     * per mostrare gli ULTIMI caratteri inseriti (utile per
     * password e campi lunghi).
     *
     * Se il testo e' troncato a sinistra, mostra ".." all'inizio
     * per indicare che c'e' altro contenuto non visibile.
     * ============================================================ */
    int maxTextWidth = (int)(area.width - 30);  // Larghezza disponibile per il testo
    int textWidth = MeasureText(displayText, 24);
    
    if (textWidth > maxTextWidth) {
        /* Testo troppo lungo: calcola quanti caratteri mostrare */
        int visibleChars = 0;
        int currentWidth = 0;
        
        /* BUG 3 FIX: Mostra gli ULTIMI caratteri (scroll automatico
         * verso destra) per permettere all'utente di vedere cosa sta
         * scrivendo mentre digita. */
        for (int i = count - 1; i >= 0; i--) {
            char testStr[2] = {displayText[i], '\0'};
            currentWidth += MeasureText(testStr, 24);
            if (currentWidth > maxTextWidth - 20) break; // -20 per ".."
            visibleChars++;
        }
        
        if (visibleChars < count) {
            /* Mostra ".." + ultimi caratteri visibili */
            int startIdx = count - visibleChars;
            char truncated[100] = "..";
            int truncLen = 2;
            for (int i = startIdx; i < count && truncLen < 99; i++) {
                truncated[truncLen++] = displayText[i];
            }
            truncated[truncLen] = '\0';
            DrawText(truncated, area.x + 15, area.y + (area.height/2) - 12, 24, BLACK);
        } else {
            DrawText(displayText, area.x + 15, area.y + (area.height/2) - 12, 24, BLACK);
        }
    } else {
        /* Testo entra perfettamente nella cella */
        DrawText(displayText, area.x + 15, area.y + (area.height/2) - 12, 24, BLACK);
    }
    
    /* Cursore lampeggiante (solo se focus) */
    if (isFocused && (int)(GetTime() * 2) % 2 == 0) {
        /* BUG 3 FIX: Posiziona il cursore DOPO l'ultimo carattere visibile,
         * non dopo la fine del testo completo. Se il testo e' troncato,
         * il cursore viene posizionato alla fine del testo visibile. */
        int visibleTextWidth = textWidth;
        if (textWidth > maxTextWidth) {
            visibleTextWidth = maxTextWidth;
        }
        DrawText("_", area.x + 15 + (visibleTextWidth < maxTextWidth ? textWidth : maxTextWidth), 
                 area.y + (area.height/2) - 12, 24, RED);
    }
}