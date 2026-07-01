/**
 * @file string_utils.h
 * @brief Utilità per la manipolazione di stringhe.
 * @defgroup string_utils String Utils
 * @brief Funzioni helper per confronto e manipolazione di stringhe.
 */
#ifndef STRING_UTILS_H
#define STRING_UTILS_H

/**
 * @brief Confronta due stringhe ignorando le differenze tra maiuscole/minuscole.
 * @pre s1 != NULL; s2 != NULL.
 * @return 0 se uguali, <0 se s1 < s2, >0 se s1 > s2.
 * @param s1 Prima stringa.
 * @param s2 Seconda stringa.
 */
int CompareIgnoreCase(const char* s1, const char* s2);

#endif
