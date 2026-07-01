/**
 * @file fs.h
 * @brief Helper filesystem multipiattaforma per creazione directory.
 */
#ifndef FS_H
#define FS_H
#include <stdlib.h>
static inline void fs_mkdirs(void) {
#ifdef _WIN32
    system("if not exist data mkdir data");
    system("if not exist data\\users mkdir data\\users");
    system("if not exist data\\games mkdir data\\games");
#else
    system("mkdir -p data/users data/games");
#endif
}
#endif