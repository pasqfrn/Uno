/**
 * @file chat_render.c
 * @brief Rendering della chat e componenti UI correlati.
 *
 * Modulo specializzato per il rendering della chat multiplayer,
 * suggerimenti @mentions, invio messaggi e popup di uscita.
 *
 * Funzioni:
 *   - DrawChatMsgSegmentato: disegna messaggio chat con tag evidenziati
 *   - DisegnaPannelloChat: pannello completo chat con cronologia e input
 *   - DisegnaPopupUscita: popup di conferma uscita partita
 *
 * Estratto da gameplay_draw.c per ridurre la complessita' del file.
 * Utilizza le variabili di stato definite in gameplay_screen.c.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "raylib.h"

#include "../../../lib/screens/game/gameplay_shared.h"
#include "../../../lib/screens/game/gameplay_draw.h"
#include "../../../lib/screens/menu/string_utils.h"
#include "../../../lib/game/game_logic.h"
#include "../../../lib/screens/ui.h"
#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/auth/auth.h"

#define LARGHEZZA 1280
#define ALTEZZA 720

/* Variabili esterne definite in gameplay_screen.c */
extern char inputChat[128];
extern int isTyping;
extern int popup_uscita_attivo;

/**
 * @file chat_render.c
 * @brief Rendering della chat in-game.
 * @ingroup gameplay_chat
 *
 * Gestisce il disegno della finestra chat e dei messaggi.
 */
int StartsWithIgnoreCase(const char* str, const char* prefix) {
    if (!str || !prefix) return 0;
    while (*prefix) {
        if (!*str) return 0;
        char c1 = *str;
        char c2 = *prefix;
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return 0;
        str++; prefix++;
    }
    return 1;
}

/**
 * @brief Disegna un messaggio chat con tag @ evidenziati.
 *
 * I tag diretti a mio_id vengono evidenziati in giallo (YELLOW),
 * i tag ad altri giocatori in azzurro (CYAN).
 * Il resto del testo viene disegnato in LIGHTGRAY.
 *
 * @param msg Messaggio da disegnare.
 * @param x Coordinata X di partenza.
 * @param y Coordinata Y di partenza.
 * @param fontSize Dimensione del font.
 * @param gioco Stato di gioco (per accedere ai nomi dei giocatori).
 * @param mio_id ID del giocatore locale.
 */
void DrawChatMsgSegmentato(const char* msg, int x, int y, int fontSize,
                            StatoGioco* gioco, int mio_id) {
    Color defaultColor = LIGHTGRAY;
    Color tagMeColor = YELLOW;
    Color tagOtherColor = (Color){100, 200, 255, 255};

    int cursorX = x;
    int msgLen = strlen(msg);
    int i = 0;

    while (i < msgLen) {
        if (msg[i] == '@') {
            int matched = 0;
            for (int p = 0; p < gioco->num_giocatori; p++) {
                const char* name = Giocatore_Nome(&gioco->giocatori[p]);
                int nameLen = strlen(name);
                if (nameLen > 0 && i + 1 + nameLen <= msgLen &&
                    StartsWithIgnoreCase(msg + i + 1, name)) {
                    int afterIdx = i + 1 + nameLen;
                    if (afterIdx >= msgLen || msg[afterIdx] == ' ' || msg[afterIdx] == ',' ||
                        msg[afterIdx] == '!' || msg[afterIdx] == '?' || msg[afterIdx] == '.' ||
                        msg[afterIdx] == ':') {
                        char tagStr[64];
                        snprintf(tagStr, sizeof(tagStr), "@%s", name);
                        Color tc = (p == mio_id) ? tagMeColor : tagOtherColor;
                        DrawText(tagStr, cursorX, y, fontSize, tc);
                        cursorX += MeasureText(tagStr, fontSize);
                        i += 1 + nameLen;
                        matched = 1;
                        break;
                    }
                }
            }
            if (!matched) {
                DrawText("@", cursorX, y, fontSize, defaultColor);
                cursorX += MeasureText("@", fontSize);
                i++;
            }
        } else {
            int nextAt = msgLen;
            for (int j = i + 1; j < msgLen; j++) {
                if (msg[j] == '@') { nextAt = j; break; }
            }
            int len = nextAt - i;
            if (len > 0) {
                char buf[256];
                if (len >= (int)sizeof(buf)) len = (int)sizeof(buf) - 1;
                memcpy(buf, msg + i, len);
                buf[len] = '\0';
                DrawText(buf, cursorX, y, fontSize, defaultColor);
                cursorX += MeasureText(buf, fontSize);
            }
            i = nextAt;
        }
    }
}

/**
 * @brief Disegna il pannello chat completo.
 *
 * Include:
 *   - Cronologia messaggi (ultimi 7 visibili)
 *   - Evidenziazione messaggi con tag verso il giocatore locale
 *   - Campo di input se isTyping
 *   - Suggerimento giocatori con @
 *   - Pulsante INVIA
 *
 * @param gioco Stato di gioco.
 * @param mio_id ID del giocatore locale.
 * @param mousePos Posizione del mouse per rilevamento click.
 */
void DisegnaPannelloChat(StatoGioco* gioco, int mio_id, Vector2 mousePos) {
    if (!gioco->chat_aperta || currentRole == NET_OFFLINE) return;

    int chatW = 340, chatH = 250;
    int chatX = LARGHEZZA - chatW - 20;
    int chatY = ALTEZZA - chatH - (isTyping ? 200 : 160);
    DrawRectangle(chatX, chatY, chatW, chatH, Fade(BLACK, 0.90f));
    DrawRectangleLines(chatX, chatY, chatW, chatH, LIGHTGRAY);
    DrawText("CHAT", chatX + 10, chatY + 10, 16, GOLD);

    int msgY = chatY + 40;
    char tagMio[64] = "";
    snprintf(tagMio, sizeof(tagMio), "@%s", Giocatore_Nome(&gioco->giocatori[mio_id]));

    int maxMsgVisibili = 7;
    int startMsg = (gioco->num_chat_msg > maxMsgVisibili) ? (gioco->num_chat_msg - maxMsgVisibili) : 0;
    for (int i = startMsg; i < gioco->num_chat_msg; i++) {
        if (strstr(gioco->chat_history[i], tagMio) != NULL) {
            int msgW = MeasureText(gioco->chat_history[i], 16);
            DrawRectangle(chatX + 5, msgY - 2, msgW + 10, 22, Fade(YELLOW, 0.15f));
        }
        DrawChatMsgSegmentato(gioco->chat_history[i], chatX + 10, msgY, 16, gioco, mio_id);
        msgY += 30;
    }

    if (isTyping) {
        int typeX = chatX, typeY = chatY + chatH;
        DrawRectangle(typeX, typeY, chatW, 40, Fade(BLACK, 0.95f));
        DrawRectangleLines(typeX, typeY, chatW, 40, GOLD);
        DrawText(TextFormat("Scrivi: %s_", inputChat), typeX + 10, typeY + 12, 16, WHITE);

        int lenInput = strlen(inputChat);
        if (lenInput > 0 && inputChat[lenInput - 1] == '@') {
            int suggY = typeY - 35 * (gioco->num_giocatori - 1);
            DrawRectangle(typeX, suggY, 160, 35 * (gioco->num_giocatori - 1), Fade(BLACK, 0.95f));
            DrawRectangleLines(typeX, suggY, 160, 35 * (gioco->num_giocatori - 1), GOLD);
            int sY = suggY + 6;
            for (int s = 0; s < gioco->num_giocatori; s++) {
                if (s == mio_id) continue;
                Color corSug = (gioco->giocatori[s].is_bot) ? ORANGE : LIME;
                if (CheckCollisionPointRec(mousePos, (Rectangle){typeX, sY - 3, 160, 32})) {
                    corSug = WHITE;
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        strcat(inputChat, Giocatore_Nome(&gioco->giocatori[s]));
                        strcat(inputChat, " ");
                    }
                }
                DrawText(Giocatore_Nome(&gioco->giocatori[s]), typeX + 5, sY, 18, corSug);
                sY += 35;
            }
        }

        if (DisegnaBottoneMenuAnimato((Rectangle){ typeX + chatW - 90, typeY, 90, 40 }, "INVIA") ||
            (IsKeyPressed(KEY_ENTER) && strlen(inputChat) > 0)) {
            if (strlen(inputChat) > 0) {
                char tagNotifica[256] = "";
                if (inputChat[0] == '@') {
                    char nomeTag[30] = "";
                    sscanf(inputChat + 1, "%s", nomeTag);
                    for (int t = 0; t < gioco->num_giocatori; t++) {
                        if (CompareIgnoreCase(gioco->giocatori[t].nome, nomeTag)) {
                            // BUG FIX: Non mostrare notifica se il taggato e' un BOT (i bot non ricevono notifiche)
                            if (!gioco->giocatori[t].is_bot) {
                                snprintf(tagNotifica, sizeof(tagNotifica),
                                         "Sei stato taggato da %s che scrive: %s",
                                         Giocatore_Nome(&gioco->giocatori[mio_id]), inputChat);
                                MostraNotifica(gioco, tagNotifica);
                            }
                            break;
                        }
                    }
                }
                char fullMsg[128];
                /* Program. difensiva: limita la lunghezza del messaggio a quanto
                 * realmente disponibile nel buffer, evitando troncamenti a meta'
                 * carattere e il falso positivo -Wformat-truncation di GCC. */
                int spazio_restante = (int)sizeof(fullMsg) - 2 - (int)strlen(Giocatore_Nome(&gioco->giocatori[mio_id]));
                if (spazio_restante < 0) spazio_restante = 0;
                snprintf(fullMsg, sizeof(fullMsg), "%s: %.*s",
                         Giocatore_Nome(&gioco->giocatori[mio_id]), spazio_restante, inputChat);
                if (currentRole == NET_CLIENT) { SendChat(fullMsg); }
                else {
                    if (currentRole == NET_HOST) { BroadcastChat(fullMsg); }
                    if (gioco->num_chat_msg < 15) {
                        strcpy(gioco->chat_history[gioco->num_chat_msg], fullMsg);
                        gioco->num_chat_msg++;
                    } else {
                        for(int i=1; i<15; i++)
                            strcpy(gioco->chat_history[i-1], gioco->chat_history[i]);
                        strcpy(gioco->chat_history[14], fullMsg);
                    }
                }
            }
            inputChat[0] = '\0';
            isTyping = 0;
        }
    }
}

/**
 * @brief Disegna il popup di conferma uscita partita.
 *
 * Mostra overlay scuro con opzioni:
 *   - Offline: "Salva ed Esci", "Esci", "Annulla"
 *   - Online: "SI" (esce + chiude rete), "NO"
 *
 * @param gioco Stato di gioco (per salvataggio opzionale).
 * @param fase Puntatore alla fase (cambia a FASE_MENU se confermato).
 */
void DisegnaPopupUscita(StatoGioco* gioco, FaseApplicazione* fase) {
    if (!popup_uscita_attivo) return;

    static int btn_clicked = 0;
    DrawRectangle(0, 0, LARGHEZZA, ALTEZZA, Fade(BLACK, 0.95f));
    int pWidth = (currentRole == NET_OFFLINE) ? 580 : 500;
    DrawRectangleRounded((Rectangle){ LARGHEZZA/2 - pWidth/2, ALTEZZA/2 - 140, pWidth, 240 }, 0.1f, 10, Fade(DARKGRAY, 0.95f));
    DrawRectangleRoundedLines((Rectangle){ LARGHEZZA/2 - pWidth/2, ALTEZZA/2 - 140, pWidth, 240 }, 0.1f, 10, GOLD);
    DrawText("SEI SICURO?", LARGHEZZA/2 - MeasureText("SEI SICURO?", 24)/2, ALTEZZA/2 - 90, 24, WHITE);

    if (currentRole == NET_OFFLINE) {
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 270, ALTEZZA/2 - 20, 260, 50 }, "Salva ed Esci")) { btn_clicked = 1; }
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 10, ALTEZZA/2 - 20, 260, 50 }, "Esci")) { btn_clicked = 2; }
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 100, ALTEZZA/2 + 50, 200, 40 }, "Annulla")) { popup_uscita_attivo = 0; btn_clicked = 0; }
    } else {
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 - 160, ALTEZZA/2, 140, 50 }, "SI")) { btn_clicked = 3; }
        if (DisegnaBottoneMenuAnimato((Rectangle){ LARGHEZZA/2 + 20, ALTEZZA/2, 140, 50 }, "NO")) { popup_uscita_attivo = 0; btn_clicked = 0; }
    }
    if (btn_clicked > 0 && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (btn_clicked == 1) { SalvaPartita(gioco); EliminaPartita(gioco); *fase = FASE_MENU; SetExitKey(0); }
        else if (btn_clicked == 2) { EliminaPartita(gioco); *fase = FASE_MENU; SetExitKey(0); }
        else if (btn_clicked == 3) { CloseNetwork(); EliminaPartita(gioco); *fase = FASE_MENU; SetExitKey(0); }
        popup_uscita_attivo = 0; btn_clicked = 0;
    }
}