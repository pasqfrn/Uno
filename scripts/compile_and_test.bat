@echo off
title UNO - Compilazione, Documentazione e Test
color 0A

cd /d "%~dp0.."

echo ========================================
echo  COMPILAZIONE GIOCO UNO
echo ========================================
echo.
C:\msys64\ucrt64\bin\gcc.exe -g src/core/main.c src/auth/auth.c src/auth/auth_validation.c src/game/game_logic.c src/screens/ui.c src/data_structures/bst.c src/data_structures/chat.c src/data_structures/list.c src/data_structures/queue.c src/data_structures/stack.c src/screens/auth/auth_screen.c src/screens/auth/auth_update.c src/screens/auth/auth_draw.c src/screens/auth/input_field.c src/screens/game/bot_ai.c src/screens/game/end_screen.c src/screens/game/gameplay_screen.c src/screens/game/gameplay_update.c src/screens/game/gameplay_draw.c src/screens/menu/menu_screen.c src/screens/menu/stats_screen.c src/screens/menu/string_utils.c src/screens/multiplayer/multiplayer_lobby.c src/screens/multiplayer/multiplayer_screen.c src/socket/network/network.c src/socket/network/network_send.c src/socket/network/packet_handler.c src/socket/network/packet_handler_game.c src/socket/network/packet_handler_admin.c src/socket/network/state_sync.c src/socket/server/server_manager.c src/socket/utils/network_utils.c src/game/player.c src/game/deck.c src/game/save_manager.c src/game/game_flow.c src/screens/game/chat_render.c src/screens/auth/admin_draw.c src/socket/lan/lan_db_sync.c src/socket/lan/lan_save_sync.c src/socket/lan/lan_broadcast.c -o uno.exe -Ilib -Ilib/raylib/include -Llib/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32
if %errorlevel% neq 0 (
    color 0C
    echo.
    echo  ERRORE nella compilazione del gioco!
    echo  (Probabilmente Raylib non e' installato)
    echo.
    goto :COMPILA_TEST
)
echo.
echo  Gioco compilato con successo!
echo.

:COMPILA_TEST
echo ========================================
echo  COMPILAZIONE TEST ADT
echo ========================================
echo.
C:\msys64\ucrt64\bin\gcc.exe -std=c99 -Wall -Wextra -Ilib -Isrc -Itests -Itests/stubs tests/test_adt.c src/data_structures/stack.c src/data_structures/queue.c src/data_structures/list.c src/data_structures/bst.c src/data_structures/chat.c src/game/game_logic.c src/game/deck.c src/game/player.c -o tests/test_adt.exe -lm
if %errorlevel% neq 0 (
    color 0C
    echo  ERRORE nella compilazione dei test!
    echo.
    pause
    exit /b %errorlevel%
)
echo  Test compilati con successo!
echo.

echo ========================================
echo  GENERAZIONE DOCUMENTAZIONE DOXYGEN
echo ========================================
echo.
where doxygen >nul 2>nul
if %errorlevel% equ 0 (
    doxygen docs\doxygen\html\Doxyfile >nul 2>&1
    if %errorlevel% equ 0 (
        echo  Documentazione generata in docs/doxygen/html/
    ) else (
        echo  WARNING: Doxygen ha generato degli errori, ma la documentazione
        echo  potrebbe essere comunque disponibile in docs/doxygen/html/
    )
) else (
    echo  Doxygen non trovato. La documentazione non e' stata rigenerata.
    echo  Installalo con: winget install Doxygen.Doxygen
)
echo.

echo ========================================
echo  ESECUZIONE TEST ADT
echo ========================================
echo.
tests\test_adt.exe
echo.
echo ========================================
echo  COMPLETATO!
echo  - Gioco: uno.exe
echo  - Test: tests/test_adt.exe  (149 test)
echo  - Documentazione: docs/doxygen/html/index.html
echo ========================================
echo.
pause
