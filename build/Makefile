# ============================================================================
# @file Makefile
# @brief Build system per il gioco UNO (multipiattaforma).
# @ingroup build
#
# Compila il progetto su macOS, Linux e Windows (MinGW).
# Supporta Raylib in-tree (lib/raylib/<os>/) o di sistema.
#
# Utilizzo:
#   make              - Compila l'eseguibile 'uno'
#   make test         - Compila ed esegue i test unitari
#   make clean        - Rimuove file oggetto ed eseguibile
#   make setup-raylib - Mostra istruzioni per Raylib bundle
#
# Architettura:
#   - Rilevamento automatico OS via $(shell uname -s)
#   - Preferisce Raylib bundle in-tree se presente
#   - Fallback a Raylib di sistema (installata globalmente)
# ============================================================================

# ============================================================================
# @brief Compilatore e flags
# @param CC  Compilatore C (gcc)
# @param CFLAGS  Flags di compilazione: -Wall -Wextra -std=c99
#                Include path: lib/, src/, lib/raylib/include/
#                Definisce _POSIX_C_SOURCE=200809L per POSIX
# @param LDFLAGS Flags di linking: -lm (math)
# ============================================================================
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Ilib -Isrc -D_POSIX_C_SOURCE=200809L -Ilib/raylib/include
LDFLAGS = -lm

# ============================================================================
# @brief Rilevamento OS
# @details $(shell uname -s) restituisce "Linux", "Darwin" (macOS) o "Windows"
#          (se usi MSYS2/MinGW). Usato per selezionare librerie Raylib e
#          dipendenze OS-specifiche.
# ============================================================================
UNAME := $(shell uname -s)

# ============================================================================
# @brief Linking Raylib
# @details Se il bundle in-tree esiste (lib/raylib/<os>/libraylib.a), lo usa.
#          Altrimenti cerca Raylib installata globalmente (-lraylib).
#
# @details macOS: richiede -framework CoreFoundation -framework IOKit
# @details Linux: richiede -lGL -lpthread -ldl -lrt
# ============================================================================
ifeq ($(UNAME), Linux)
    ifneq ($(wildcard lib/raylib/Linux/libraylib.a),)
        LDFLAGS += lib/raylib/Linux/libraylib.a -lGL -lpthread -ldl -lrt
    else
        LDFLAGS += -lraylib -lGL -lpthread -ldl -lrt
    endif
else ifeq ($(UNAME), Darwin)
    ifneq ($(wildcard lib/raylib/Darwin/libraylib.a),)
        LDFLAGS += lib/raylib/Darwin/libraylib.a -framework CoreFoundation -framework IOKit
    else
        LDFLAGS += -lraylib -framework CoreFoundation -framework IOKit
    endif
endif

# ============================================================================
# @brief File sorgente raggruppati per modulo
# @details Ogni variabile elenca i file .c di un sottosistema:
#   - SRCS_CORE: main, auth, game_logic, list, ui
#   - SRCS_DATA: strutture dati (BST, chat, list, queue, stack)
#   - SRCS_GAME: logica di gioco (deck, game_flow, save_manager)
#   - SRCS_SCREENS_AUTH: schermate autenticazione
#   - SRCS_SCREENS_GAME: schermate di gioco
#   - SRCS_SCREENS_MENU: menu e statistiche
#   - SRCS_SCREENS_MULTI: multiplayer
#   - SRCS_SOCKET_LAN: sincronizzazione LAN
#   - SRCS_SOCKET_NET: rete TCP/UDP
#   - SRCS_SOCKET_SERVER: gestione sessioni
#   - SRCS_SOCKET_UTILS: utility di rete
#   - SRCS_MENU: utility stringhe
# ============================================================================
SRCS_CORE = src/main.c src/auth.c src/auth_validation.c src/game_logic.c src/list.c src/ui.c
SRCS_DATA = src/data_structures/bst.c src/data_structures/chat.c src/data_structures/list.c src/data_structures/queue.c src/data_structures/stack.c
SRCS_GAME = src/game/deck.c src/game/game_flow.c src/game/save_manager.c
SRCS_SCREENS_AUTH = src/screens/auth/input_field.c src/screens/auth/auth_screen.c src/screens/auth/auth_update.c src/screens/auth/auth_draw.c src/screens/auth/admin_draw.c
SRCS_SCREENS_GAME = src/screens/game/bot_ai.c src/screens/game/chat_render.c src/screens/game/end_screen.c src/screens/game/gameplay_screen.c src/screens/game/gameplay_update.c src/screens/game/gameplay_draw.c
SRCS_SCREENS_MENU = src/screens/menu/menu_screen.c src/screens/menu/stats_screen.c
SRCS_SCREENS_MULTI = src/screens/multiplayer/multiplayer_screen.c src/screens/multiplayer/multiplayer_lobby.c
SRCS_SCREENS = $(SRCS_SCREENS_AUTH) $(SRCS_SCREENS_GAME) $(SRCS_SCREENS_MENU) $(SRCS_SCREENS_MULTI)
SRCS_SOCKET_LAN = src/socket/lan/lan_broadcast.c src/socket/lan/lan_db_sync.c src/socket/lan/lan_save_sync.c src/socket/lan/lan_save_helpers.c
SRCS_SOCKET_NET = src/socket/network/network.c src/socket/network/network_send.c src/socket/network/packet_handler.c src/socket/network/packet_handler_game.c src/socket/network/packet_handler_admin.c src/socket/network/state_sync.c
SRCS_SOCKET_SERVER = src/socket/server/server_manager.c
SRCS_SOCKET_UTILS = src/socket/utils/network_utils.c
SRCS_SOCKET = $(SRCS_SOCKET_LAN) $(SRCS_SOCKET_NET) $(SRCS_SOCKET_SERVER) $(SRCS_SOCKET_UTILS)
SRCS_MENU = src/screens/menu/string_utils.c

# ============================================================================
# @brief Unione di tutti i sorgenti e oggetti
# @var SRCS  Lista completa dei file .c
# @var OBJS  Lista dei file .o corrispondenti
# @var TARGET Nome dell'eseguibile prodotto
# ============================================================================
SRCS = $(SRCS_CORE) $(SRCS_DATA) $(SRCS_GAME) $(SRCS_SCREENS) $(SRCS_SOCKET) $(SRCS_MENU)
OBJS = $(SRCS:.c=.o)

TARGET = uno

# ============================================================================
# @brief Regola di compilazione oggetti
# @details Converte ogni file .c in .o usando il compilatore con CFLAGS.
# ============================================================================
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# @brief Target principali
# @details
#   - all:      Compila l'eseguibile 'uno' (target di default)
#   - $(TARGET): Linka tutti gli oggetti nell'eseguibile finale
#   - clean:    Rimuove tutti i file oggetto e l'eseguibile
#   - test:     Compila ed esegue i test unitari ADT
#   - setup-raylib: Mostra istruzioni per configurare Raylib bundle
# ============================================================================
# @brief Target non-file - non corrispondono a file effettivi
.PHONY: all clean setup-raylib test

# @brief Target di default - compila l'eseguibile
all: $(TARGET)

# @brief Linka gli oggetti per produrre l'eseguibile
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# ============================================================================
# @brief TEST UNITARI ADT
# @details Compila ed esegue i 171 test per verificare il corretto
#          funzionamento di tutti gli ADT (Lista, Pila, Coda, BST, Chat)
#          e della logica di gioco (MossaValida).
#          Usa Itests/stubs/ per lo stub di Raylib.
# @var TEST_OBJ_DIR Directory dove risiedono i file .o dei test
# @var TEST_OBJS   Lista dei file oggetto necessari
# @var TEST_TARGET Nome dell'eseguibile di test
# ============================================================================
TEST_OBJ_DIR = tests/obj
TEST_OBJS = \
	$(TEST_OBJ_DIR)/stack_test.o \
	$(TEST_OBJ_DIR)/queue_test.o \
	$(TEST_OBJ_DIR)/list_data_test.o \
	$(TEST_OBJ_DIR)/bst_test.o \
	$(TEST_OBJ_DIR)/chat_test.o \
	$(TEST_OBJ_DIR)/list_std_test.o \
	$(TEST_OBJ_DIR)/game_logic_test.o \
	$(TEST_OBJ_DIR)/deck_test.o

TEST_TARGET = tests/test_adt

# @brief Target per compilare ed eseguire i test
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# @brief Linka i file oggetto dei moduli con il main di test
$(TEST_TARGET): tests/test_adt.c $(TEST_OBJS)
	$(CC) -std=c99 -Wall -Wextra -Ilib -Isrc -Itests -Itests/stubs tests/test_adt.c $(TEST_OBJS) -o $@ -lm

# Regole per compilare ogni modulo nei test (con stub Raylib)
$(TEST_OBJ_DIR)/stack_test.o: src/data_structures/stack.c
	$(CC) -std=c99 -Wall -Wextra -Ilib -Isrc -Itests -Itests/stubs -c $< -o $@

$(TEST_OBJ_DIR)/queue_test.o: src/data_structures/queue.c
	$(CC) -std=c99 -Wall -Wextra -Ilib -Isrc -Itests -Itests/stubs -c $< -o $@

$(TEST_OBJ_DIR)/list_data_test.o: src/data_structures/list.c
	$(CC) -std=c99 -Wall -Wextra -Ilib -Isrc -Itests -Itests/stubs -c $< -o $@

$(TEST_OBJ_DIR)/bst_test.o: src/data_structures/bst.c
	$(CC) -std=c99 -Wall -Wextra -Ilib -Isrc -Itests -Itests/stubs -c $< -o $@

$(TEST_OBJ_DIR)/chat_test.o: src/data_structures/chat.c
	$(CC) -std=c99 -Wall -Wextra -Ilib -Isrc -Itests -Itests/stubs -c $< -o $@

$(TEST_OBJ_DIR)/list_std_test.o: src/list.c
	$(CC) -std=c99 -Wall -Wextra -Ilib -Isrc -Itests -Itests/stubs -c $< -o $@

$(TEST_OBJ_DIR)/game_logic_test.o: src/game_logic.c
	$(CC) -std=c99 -Wall -Wextra -Ilib -Isrc -Itests -Itests/stubs -c $< -o $@

$(TEST_OBJ_DIR)/deck_test.o: src/game/deck.c
	$(CC) -std=c99 -Wall -Wextra -Ilib -Isrc -Itests -Itests/stubs -c $< -o $@

# ============================================================================
# @brief Pulizia
# @details Rimuove tutti i .o, l'eseguibile principale e quello di test
# ============================================================================
clean:
	rm -f $(OBJS) $(TARGET) $(TEST_OBJS) $(TEST_TARGET)

# @brief Mostra le istruzioni per scaricare e configurare Raylib
setup-raylib:
	@echo "Scarica Raylib prebuilt per il tuo OS da https://github.com/raysan5/raylib/releases"
	@echo " e estrai in:"
	@echo "   macOS -> lib/raylib/Darwin/"
	@echo "   Linux -> lib/raylib/Linux/"
	@echo "   Windows -> lib/raylib/Windows/"
	@echo "Poi rilanci 'make'."