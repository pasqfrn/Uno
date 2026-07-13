/**
 * @file raylib.h
 * @brief Stub dei tipi Raylib per i test unitari.
 * 
 * Fornisce le definizioni minime dei tipi Raylib (Rectangle, Vector2, Texture2D, Color)
 * necessarie per compilare i moduli di data_structures.h senza dipendere da Raylib.
 * Questo file viene trovato dal compilatore quando si usa -Itests.
 */

#ifndef RAYLIB_H
#define RAYLIB_H

typedef struct { float x, y, width, height; } Rectangle;
typedef struct { float x, y; } Vector2;
typedef int Texture2D;
typedef struct { unsigned char r, g, b, a; } Color;

/* Stub per funzioni usate nel progetto */
static inline const char* TextFormat(const char* text, ...) {
    (void)text;
    return "";
}

#endif /* RAYLIB_H */