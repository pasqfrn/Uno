/**
 * @file input_field.h
 * @brief Helper per la gestione dei campi di input nelle schermate auth.
 * @defgroup input_field Input Field
 * @brief Funzioni per aggiornare e disegnare campi di input di testo
 * (login, registrazione, recupero password).
 */
#ifndef INPUT_FIELD_H
#define INPUT_FIELD_H

#include "raylib.h"

/**
 * @brief Aggiorna lo stato di un campo input (carattere premuto, backspace).
 * @pre buffer != NULL; count != NULL; maxLength > 0.
 * @post buffer e count aggiornati in base al tasto premuto.
 * @param buffer Buffer del campo input.
 * @param count Puntatore al contatore di caratteri.
 * @param maxLength Lunghezza massima del buffer.
 */
void AggiornaInputCampo(char* buffer, int* count, int maxLength);

/**
 * @brief Disegna un campo input su schermo.
 * @pre text != NULL; label != NULL.
 * @post Campo renderizzato con label e contorno.
 * @param area Rettangolo di posizione/dimensioni.
 * @param label Etichetta del campo.
 * @param text Testo corrente inserito.
 * @param count Numero di caratteri inseriti.
 * @param isFocused 1 se il campo ha il focus.
 * @param isPassword 1 se campo password (caratteri nascosti).
 * @param labelColor Colore della label.
 */
void DisegnaInputField(Rectangle area, const char* label, const char* text, int count, int isFocused, int isPassword, Color labelColor);

/** @brief Flag globale per richiedere il submit del form. */
extern int submit_richiesto;

#endif
