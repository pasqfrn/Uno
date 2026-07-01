/**
 * @file string_utils.c
 * @brief Funzioni di utilita' per la gestione di stringhe.
 * @ingroup string_utils
 *
 * Contiene funzioni helper per confronti e manipolazioni di stringhe:
 *   - CompareIgnoreCase: confronto case-insensitive tra due stringhe
 *   - Altre utility per manipolazione testo
 *
 * Usato in tutti i moduli che necessitano di confronti case-insensitive
 * (auth, multiplayer, chat, ecc.).
 */
#include <string.h>
#include "../../../lib/screens/menu/string_utils.h"

int CompareIgnoreCase(const char* s1, const char* s2) {
    if (!s1 || !s2) return 0;
    while (*s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return 0;
        s1++; s2++;
    }
    return (*s1 == *s2);
}