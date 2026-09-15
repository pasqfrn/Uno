# UNO - Il Gioco di Carte

Trasposizione digitale del celebre gioco di carte UNO, sviluppata in linguaggio C con interfaccia grafica Raylib. Il progetto supporta modalità single-player contro intelligenza artificiale e multigiocatore in rete.

## Descrizione

Questo progetto implementa una versione completa del gioco UNO, rispettando rigorosamente i requisiti tecnico-implementativi didattici. L'architettura modulare garantisce scalabilità e manutenibilità, con una chiara separazione tra logica di gioco, gestione degli utenti e interfaccia grafica.

### Caratteristiche principali

- **Modalità di gioco**: singolo giocatore (1v1, 1v2, 1v3 contro bot) e multigiocatore tramite socket
- **Sistema di autenticazione**: registrazione e login con validazione completa
- **Gestione utenti**: profili con statistiche dettagliate
- **Interfaccia grafica**: rendering 2D con animazioni fluide tramite Raylib
- **Intelligenza artificiale**: avversari bot con comportamento strategico
- **Chat di gioco**: comunicazione tra giocatori durante le partite
- **Salvataggi**: salvataggio/caricamento partite con slot multipli
- **Salvataggi coerenti**: ogni partita occupa **un solo slot** — quando una partita già salvata (anche dopo essere stata ricaricata) viene salvata di nuovo, il suo file viene **sovrascritto** con lo stato attuale invece di crearne uno nuovo; alla conclusione della partita il salvataggio viene rimosso dalla bacheca
- **Classifica**: piazzamento e tempi delle partite registrati

## Tecnologie

- **Linguaggio**: C (standard C11)
- **IDE**: Code::Blocks o Visual Studio Code
- **GUI e Input**: Raylib
- **Networking**: Socket TCP/IP
- **Persistenza**: File ad accesso diretto (.dat) e file sequenziali
- **Testing**: 149 test unitari con `assert()` per ADT e logica di dominio
- **Documentazione**: Doxygen

## Struttura del progetto

```
Uno/
├── src/                       # Implementazioni (*.c) — un modulo = una cartella
│   ├── core/                  # Entry point dell'applicazione
│   │   └── main.c             # Entry point e ciclo di vita dell'applicazione
│   ├── auth/                  # Modulo autenticazione
│   │   ├── auth.c             # Gestione registrazione, login e BST utenti
│   │   └── auth_validation.c  # Validazione email, password, username
│   ├── game/                  # Modulo gioco
│   │   ├── game_logic.c       # Regole di UNO, validazione mosse, effetti carte
│   │   ├── player.c           # Implementazione Giocatore (API + campi)
│   │   ├── deck.c             # Mazzo, pesca, scarti
│   │   ├── game_flow.c        # Flusso di partita
│   │   └── save_manager.c     # Salvataggio/caricamento partite
│   ├── data_structures/       # Implementazione ADT (solo file *.c)
│   ├── screens/               # Schermate (auth, game, menu, multiplayer) + ui.c
│   └── socket/                # Implementazione rete (network, lan, server, utils)
├── lib/                       # Interfacce pubbliche (*.h) — un modulo = una cartella
│   ├── auth/                  # Header modulo autenticazione (auth.h, auth_validation.h)
│   ├── game/                  # Header gioco (game_logic.h, game_state.h, player.h)
│   │   └── private/           # Rappresentazioni interne (game_state_private.h, player_private.h)
│   ├── data_structures/       # Header pubblici ADT (solo tipi opachi + API)
│   │   └── private/           # Rappresentazioni interne degli ADT (*_private.h)
│   ├── screens/               # Header schermate (+ ui.h, string_utils)
│   ├── socket/                # Header rete (network, lan, server, utils)
│   ├── fs/                    # Header utility filesystem
│   └── raylib/                # Libreria grafica (venduta inclusa)
├── tests/                     # Test unitari
│   ├── test_adt.c             # Suite di test (149 test, solo API pubblica + assert)
│   └── stubs/                 # Stub Raylib per compilazione test senza dipendenze
├── scripts/                   # Script di automazione
│   └── compile_and_test.bat   # Compila gioco (uno.exe) + test, genera Doxygen, esegue test
├── docs/
│   ├── Documentazione CdS.pdf  # Documentazione progettuale completa
│   └── doxygen/html/           # Configurazione e documentazione Doxygen
├── data/
│   ├── users/               # Database utenti
│   └── games/               # Salvataggi partite
├── assets/
│   ├── card/                # Texture carte
│   └── img/                 # Risorse grafiche varie (sfondo, ecc.)
├── uno.exe                    # Eseguibile del gioco (compilato in radice)
├── bin/                       # Output di Code::Blocks (Debug/uno.exe, Release/uno.exe)
└── obj/
```

## Architettura

### Strutture dati principali

- **Carta**: colore, valore numerico (0-9) e tipo (normale, CambioGiro, Salta, +2, Jolly, Jolly+4)
- **Utente**: dati anagrafici, credenziali e statistiche associate
- **Giocatore**: informazioni utente, flag bot, stato lobby, numero carte in mano
- **StatoGioco**: stato completo della partita (mazzi, giocatori, turno corrente, notifiche)

### Strutture dati dinamiche (ADT opachi)

Tutti gli ADT sono realizzati con **information hiding completo** (tipo opaco):
l'header pubblico dichiara solo `typedef struct X X;`, mentre la rappresentazione
interna (`NodoCarta`, `NodoCoda`, `NodoPila`, `NodoAlbero`, `ChatStorico`, ecc.)
è definita esclusivamente nell'header privato `*_private.h`, incluso solo dai
moduli di implementazione in `src/`. Nessun utilizzatore può accedere ai campi
interni: l'accesso avviene solo tramite le funzioni dell'API pubblica.

| ADT | Header pubblico | Header privato (rappresentazione) | Uso nel dominio |
|-----|-----------------|-----------------------------------|-----------------|
| **ListaCarte** (lista doppiamente concatenata) | `lib/data_structures/list.h` | `lib/data_structures/private/list_private.h` | Mano dei giocatori (gestita da `game_logic`/`deck`) |
| **Pila** (stack LIFO) | `lib/data_structures/stack.h` | `lib/data_structures/private/stack_private.h` | Mazzo di pesca e pila degli scarti |
| **CodaTurni** (coda FIFO) | `lib/data_structures/queue.h` | `lib/data_structures/private/queue_private.h` | Avanzamento turni di gioco (`game_logic`) |
| **Albero BST** | `lib/data_structures/bst.h` | `lib/data_structures/private/bst_private.h` | Autenticazione: utenti in RAM, ricerca per email |
| **ChatStorico** (buffer circolare FIFO) | `lib/data_structures/chat.h` | `lib/data_structures/private/chat_private.h` | Cronologia messaggi chat |

Nota: `StatoGioco` (in `lib/game_state.h`) referenzia le carte in trascinamento
tramite **indice** (`id_carta_in_trascinamento`), mai tramite puntatori a nodi
interni degli ADT: la rappresentazione resta così nascosta a tutti gli utilizzatori.

### Moduli funzionali

| Modulo | Responsabilità |
|--------|----------------|
| `auth` | Registrazione, login, cancellazione utenti, ricostruzione BST |
| `auth_validation` | Validazione formale email, password, username, PIN |
| `game_logic` | Validazione mosse, applicazione effetti, mazzo, pescate, salvataggi |
| `list` | Operazioni su lista mani (inserimenti, rimozioni, accesso) |
| `ui` | Disegno carte realistiche, bottoni animati, gestione textures |
| `socket` | Connessioni host/client, sincronizzazione stato multigiocatore |

## Test unitari

Il progetto include una suite completa di **149 test unitari**, scritti con
`assert()` della libreria standard (`<assert.h>`) e basati **solo sull'API
pubblica** degli ADT (nessun accesso alla rappresentazione interna), che
verificano il corretto funzionamento degli ADT e delle funzionalità di dominio.

### Esecuzione dei test

Con Visual Studio Code:
- **Ctrl+Shift+B**: compila gioco + test, esegue suite completa
- **Ctrl+Shift+P** → `Tasks: Run Task` → `▶ Esegui solo i TEST (senza ricompilare)`

Da linea di comando:

```bash
# Compilazione ed esecuzione completa
scripts\compile_and_test.bat

# Solo compilazione test
gcc -std=c99 -Wall -Wextra -Ilib -Isrc -Itests -Itests/stubs tests/test_adt.c src/data_structures/*.c src/game/game_logic.c src/game/deck.c src/game/player.c -o tests/test_adt.exe -lm

# Solo esecuzione (se già compilato)
tests\test_adt.exe
```

### Risultati

| Modulo | Test | Note |
|--------|------|------|
| **Lista doppiamente concatenata (ADT)** | 28 | Inserimenti, rimozioni, accesso per indice, chiamate NULL |
| **Pila (ADT)** | 14 | Push, Pop, Top, svuota, chiamate NULL |
| **Coda (ADT)** | 26 | Enqueue, dequeue, front, rear, rotazione turni, copia per indice |
| **BST (ADT)** | 16 | Inserimento, ricerca per email, duplicati, rimozione, inordine |
| **Chat (ADT)** | 9 | Buffer circolare, overflow, formattazione, chiamate NULL |
| **Dominio: mazzo e pesca** | 8 | InizializzaMazzo (108 carte), Pesca, ricostruzione mano |
| **Dominio: gestione mano** | 21 | Aggiunta/rimozione carte, salvataggio/ricostruzione mano (ADT ListaCarte) |
| **Dominio: avanzamento turni** | 12 | Sincronizzazione CodaTurni con StatoGioco, direzione, effetti Salta |
| **Dominio: regole di gioco** | 10 | MossaValida (colore/tipo/jolly), CambioGiro, +2 |
| **Giocatore (difensivo)** | 5 | Nome vuoto, SetNome, NomeBot, chiamate NULL |

**Totale**: 🟢 149/149 test passati

## Requisiti di sistema

- **Sistema operativo**: Windows 10/11
- **Compilatore**: GCC (testato con MinGW / MSYS2)
- **IDE**: Code::Blocks o Visual Studio Code con C/C++ Extension Pack
- **Librerie**: Raylib (inclusa in `lib/raylib/`)

## Compilazione

### Prerequisiti

Assicurarsi di avere installato:
- Code::Blocks con supporto C oppure Visual Studio Code
- MinGW GCC con supporto C11

### Procedura

1. Aprire il file `Uno.cbp` in Code::Blocks.
2. Selezionare la configurazione di build desiderata (**Debug**, **Release** o **Test**).
3. Premere **F9** o selezionare *Build > Build*.

#### Target di Code::Blocks

| Target | Output | Contenuto | Linker |
|--------|--------|-----------|--------|
| **Debug** | `bin/Debug/uno.exe` | tutti i moduli in `src/` | `-lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32` |
| **Release** | `bin/Release/uno.exe` | tutti i moduli in `src/` | come Debug, con `-O2 -s` |
| **Test** | `tests/test_adt.exe` | `tests/test_adt.c` + ADT + moduli di dominio | `-lm` (usa gli stub in `tests/stubs`) |

Il target **Test** include solo le unità dei moduli dati (`data_structures/*.c`) e di
dominio (`game_logic.c`, `deck.c`, `player.c`), usa gli stub Raylib di `tests/stubs`
per non dipendere dalla libreria grafica e stampa il report dei 149 test.

In alternativa, da linea di comando (nella cartella del progetto):

Il metodo consigliato è usare lo script di build:

```bash
scripts/compile_and_test.bat
```

Questo script compila il gioco (`uno.exe`), i test, genera la documentazione Doxygen e li esegue automaticamente.

Oppure, per compilare solo il gioco:

```bash
gcc -g src/core/main.c src/auth/auth.c src/auth/auth_validation.c src/game/game_logic.c src/game/player.c src/screens/ui.c src/data_structures/bst.c src/data_structures/chat.c src/data_structures/list.c src/data_structures/queue.c src/data_structures/stack.c src/screens/auth/auth_screen.c src/screens/auth/auth_update.c src/screens/auth/auth_draw.c src/screens/auth/input_field.c src/screens/game/bot_ai.c src/screens/game/end_screen.c src/screens/game/gameplay_screen.c src/screens/game/gameplay_update.c src/screens/game/gameplay_draw.c src/screens/menu/menu_screen.c src/screens/menu/stats_screen.c src/screens/menu/string_utils.c src/screens/multiplayer/multiplayer_lobby.c src/screens/multiplayer/multiplayer_screen.c src/socket/network/network.c src/socket/network/network_send.c src/socket/network/packet_handler.c src/socket/network/packet_handler_game.c src/socket/network/packet_handler_admin.c src/socket/network/state_sync.c src/socket/server/server_manager.c src/socket/utils/network_utils.c src/game/deck.c src/game/save_manager.c src/game/game_flow.c src/screens/game/chat_render.c src/screens/auth/admin_draw.c src/socket/lan/lan_db_sync.c src/socket/lan/lan_save_sync.c src/socket/lan/lan_broadcast.c -o uno.exe -Ilib -Ilib/raylib/include -Llib/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32
```

## Esecuzione

Dopo la compilazione, eseguire:

```bash
uno.exe
```

## Documentazione

La documentazione Doxygen completa è disponibile in:

- `docs/doxygen/html/index.html` (aprila nel browser)

Per rigenerarla:

```bash
doxygen docs/Doxyfile
```

## Utilizzo

### Flusso dell'applicazione

1. **Autenticazione**: schermata iniziale con sign-up o login.
2. **Dashboard**: visualizzazione profilo, statistiche e scelta modalità.
3. **Lobby**: attesa giocatori (reali o bot).
4. **Gameplay**: gioco vero e proprio con gestione carte, turni e chat.
5. **Fine partita**: classifica e salvataggio statistiche.

### Controlli durante il gioco

- **Mouse**: selezionare e giocare le carte, navigare nei menu
- **Tasti rapidi**: implementazione di tasti funzione per azioni comuni

### Funzionalità avanzate sviluppate

- Tasto "UNO!" con check per penalità
- Cifratura avanzata delle password
- Gestione robusta disconnessioni in rete
- Auto-ricostruzione del mazzo quando finiscono le carte

## Progettazione e design

Il progetto è stato progettato per rispettare i vincoli didattici:

- **Incapsulamento**: ogni ADT è implementato in moduli separati (.h/.c)
- **Information hiding**: le strutture interne non sono esposte oltre i moduli di competenza
- **Gestione memoria**: allocazioni/deallocazioni controllate
- **Robustezza**: controlli di validità su ogni input
- **Persistenza**: accesso diretto a file per aggiornamenti efficienti

### Scelte progettuali

- **Lista doppiamente concatenata** per le mani: inserimenti/rimozioni O(1)
- **Pila LIFO** per mazzo pesca e scarti: gestione naturale della cima
- **Coda circolare** per i turni: rotazione efficiente dei giocatori
- **BST** per gli utenti: ricerca O(log n) per email
- **File ad accesso diretto** per il database utenti: aggiornamenti senza riscrittura totale

## Autori

- **Alessio Decarolis** - Matricola 852865 - https://github.com/decarolisalessio24-dotcom
- **Pasquale Franco** - Matricola 852866 - https://github.com/pasqfrn

## Licenza

Caso di studio - Eventuale licenza da definire.

## Note

- La documentazione progettuale completa è disponibile in  `docs/doxygen/html/index.html`
- Nel branch corrente sono presenti funzionalità di gestione avanzata del database (marcature per eliminazione, ricostruzione BST, cifratura)
- Le texture delle carte sono caricate dinamicamente dalla cartella `assets/card/`
- La suite di test è automatizzabile tramite `scripts/compile_and_test.bat` per compilazione, test e generazione documentazione Doxygen