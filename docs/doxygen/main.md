\mainpage UNO - Gioco Multiplayer di Carte

## Introduzione

**UNO** è un caso di studio che riporta il famoso gioco di carte integrando funzionalità multiplayer. Il caso di studio, sviluppato in C, implementa il classico gioco da tavolo UNO di Mattel combinando algoritmi di logica di gioco, strutture dati avanzate e comunicazione in rete per offrire un'esperienza completa sia in modalità locale che online.

### Caratteristiche Principali

- **Modalità di gioco**: Singolo giocatore vs Bot, Multiplayer locale (1-4 giocatori), Multiplayer online via socket TCP
- **Interfaccia grafica**: Rendering 2D con Raylib, animazioni fluide per carte e trascinamenti
- **Sistema di autenticazione**: Registrazione/Login utenti con database dinamico in memoria, BST per ricerca veloce
- **Persistenza dati**: Salvataggio partite e statistiche su file binario, sync LAN per database condiviso
- **Intelligenza Artificiale**: Bot con strategie di gioco configurabili (easy/medium/hard) basate su albero decisionale
- **Admin Panel**: Gestione utenti remote via LAN sync con lazy deletion
- **Chat Multiplayer**: Sistema di messaggistica real-time integrato durante le partite
- **Broadcast LAN**: Scoperta automatica server sulla rete locale

## Struttura del Caso di Studio

Il caso di studio è organizzato in moduli funzionali separati con chiara separazione delle responsabilità:

```
Uno/
├── src/                        # File sorgente C
│   ├── main.c                 # Entry point e loop principale, gestione stati macchina a stati
│   ├── auth.c                 # Gestione autenticazione: registrazione, login, recupero PIN
│   ├── auth_validation.c      # Validazione email, password, PIN, username
│   ├── game_logic.c           # Logica di gioco UNO: mazzo 108 carte, effetti speciali, turni
│   ├── ui.c                   # Interfaccia utente Raylib: rendering, gestione eventi
│   ├── list.c                 # Implementazione lista doppiamente concatenata (wrapper)
│   ├── data_structures/       # Strutture dati avanzate
│   │   ├── bst.c              # Albero binario di ricerca (DAB) per indicizzazione utenti
│   │   ├── chat.c             # Chat: buffer circolare per messaggi
│   │   ├── list.c             # Lista doppiamente concatenata (ADT generico)
│   │   ├── queue.c            # Coda (FIFO) per gestione turni multiplayer
│   │   └── stack.c            # Pila (LIFO) per mazzo di pesca e scarti
│   ├── game/                  # Modulo di gioco
│   │   ├── deck.c             # Mazzo: creazione 108 carte, mescolamento Fisher-Yates
│   │   ├── game_flow.c        # Flusso di gioco: fasi, transizioni, gestione round
│   │   └── save_manager.c     # Salvataggio/caricamento partite su file binario
│   ├── screens/               # Schermate UI organizzate per dominio
│   │   ├── auth/              # Schermate autenticazione (login, registrazione, admin)
│   │   │   ├── auth_screen.c      # Schermate login/registrazione
│   │   │   ├── auth_update.c      # Aggiornamento logico schermate auth
│   │   │   ├── auth_draw.c        # Rendering schermate auth
│   │   │   ├── admin_draw.c       # Rendering pannello admin
│   │   │   └── input_field.c      # Gestione campi input testo
│   │   ├── game/              # Schermate gameplay
│   │   │   ├── gameplay_screen.c    # Schermata di gioco principale
│   │   │   ├── gameplay_draw.c     # Rendering gameplay (carte, tavolo, UI)
│   │   │   ├── gameplay_update.c   # Aggiornamento logico stato gameplay
│   │   │   ├── bot_ai.c            # Intelligenza Artificiale dei Bot
│   │   │   ├── chat_render.c       # Rendering overlay chat in-game
│   │   │   └── end_screen.c        # Schermata fine partita e classifica
│   │   ├── menu/              # Schermate menu
│   │   │   ├── menu_screen.c       # Menu principale e navigazione
│   │   │   ├── stats_screen.c      # Schermata statistiche utente
│   │   │   └── string_utils.c      # Utility stringhe per menu
│   │   └── multiplayer/       # Schermate multiplayer
│   │       ├── multiplayer_screen.c  # Creazione/join partite multiplayer
│   │       └── multiplayer_lobby.c   # Lobby multiplayer (attesa giocatori, ready)
│   └── socket/                # Networking e comunicazione
│       ├── network/
│       │   ├── network.c             # Gestione connessioni TCP, socket API
│       │   ├── network_send.c        # Invio pacchetti strutturati
│       │   ├── packet_handler.c      # Dispatcher pacchetti ricevuti
│       │   ├── packet_handler_admin.c  # Gestione pacchetti admin (sync utenti)
│       │   ├── packet_handler_game.c   # Gestione pacchetti di gioco
│       │   └── state_sync.c          # Sincronizzazione stato tra host e client
│       ├── lan/
│       │   ├── lan_broadcast.c        # Broadcast UDP per scoperta server LAN
│       │   ├── lan_db_sync.c          # Sincronizzazione database utenti via LAN
│       │   ├── lan_save_helpers.c     # Helper per salvataggio sync LAN
│       │   └── lan_save_sync.c        # Sync salvataggi tra host e client
│       ├── server/
│       │   └── server_manager.c       # Gestione server: lobby, sessioni, giocatori
│       └── utils/
│           └── network_utils.c        # Utility di rete (timeout, heartbeat, etc.)
├── lib/                        # Header files pubblici
│   ├── data_structures.h      # Definizioni tipi principali (Carta, Giocatore, StatoGioco, etc.)
│   ├── auth.h                 # API autenticazione e BST
│   ├── auth_validation.h      # API validazione input
│   ├── game_logic.h           # API logica gioco
│   ├── ui.h                   # API interfaccia Raylib
│   ├── list.h                 # API lista doppiamente concatenata (wrapper)
│   ├── data_structures/       # Header strutture dati
│   │   ├── bst.h              # API albero binario di ricerca
│   │   ├── chat.h             # API chat
│   │   ├── list.h             # API lista ADT
│   │   ├── queue.h            # API coda
│   │   └── stack.h            # API pila
│   ├── fs/
│   │   └── fs.h               # API file system (salvataggio/caricamento)
│   ├── screens/               # Header schermate
│   │   ├── string_utils.h     # Utility stringhe
│   │   ├── auth/
│   │   │   ├── admin_draw.h      # API rendering admin
│   │   │   ├── auth_draw.h       # API rendering auth
│   │   │   ├── auth_screen.h     # API schermate auth
│   │   │   ├── auth_shared.h     # Costanti condivise auth
│   │   │   ├── auth_update.h     # API aggiornamento auth
│   │   │   └── input_field.h     # API campi input
│   │   ├── game/
│   │   │   ├── bot_ai.h          # API IA Bot
│   │   │   ├── end_screen.h      # API schermata fine partita
│   │   │   ├── gameplay_animation.h  # API animazioni
│   │   │   ├── gameplay_chat.h       # API chat gameplay
│   │   │   ├── gameplay_draw.h       # API rendering gameplay
│   │   │   ├── gameplay_screen.h     # API schermata gameplay
│   │   │   ├── gameplay_shared.h     # Costanti condivise gameplay
│   │   │   └── gameplay_update.h     # API aggiornamento gameplay
│   │   ├── menu/
│   │   │   ├── menu_screen.h    # API menu principale
│   │   │   └── stats_screen.h   # API statistiche
│   │   └── multiplayer/
│   │       ├── multiplayer_lobby.h    # API lobby multiplayer
│   │       └── multiplayer_screen.h   # API schermata multiplayer
│   ├── socket/                # Header rete
│   │   ├── network.h          # API rete
│   │   ├── lan/
│   │   │   ├── lan_protocol.h # Protocollo LAN (header pacchetti)
│   │   │   └── lan_sync.h     # API sync LAN
│   │   ├── server/
│   │   │   └── server_manager.h  # API gestione server
│   │   └── utils/
│   │       └── network_utils.h   # API utility rete
│   └── raylib/                # Libreria grafica Raylib (include/lib)
├── assets/                     # Risorse grafiche
│   ├── card/                  # Immagini carte UNO (fronte/retro)
│   ├── textures/              # Texture UI (bottoni, sfondi, icone)
│   └── altro/                 # Font, suoni e risorse varie
├── data/                       # Dati persistenti
│   ├── users/                 # Database utenti (formato binario)
│   ├── games/                 # Salvataggi partite (file .bin)
│   └── saves/                 # Backup e file temporanei
├── docs/                       # Documentazione
│   ├── Documentazione CdS.pdf
│   └── doxygen/
│       ├── Doxyfile           # Configurazione Doxygen
│       ├── main.md            # Questa pagina principale
│       ├── html/              # Documentazione HTML generata
│       └── latex/             # Documentazione LaTeX generata
└── build/                      # Build system
    └── Makefile               # Makefile cross-piattaforma (MinGW, GCC, MSVC)
```

## Analisi del Sistema

### Diagramma ER (Entity-Relationship)

Il sistema è strutturato attorno a **3 macro-aree**:

1. **Persistenza** (Utenti, statistiche, partite salvate)
2. **Sessione di gioco attiva** (StatoGioco, giocatori, carte, chat)
3. **Strutture dati di supporto** (liste, stack, BST, buffer chat)

\code
╔══════════════════════════════════════════════════════════════╗
║                   MACRO-AREA PERSISTENZA                    ║
╠══════════════════════════════════════════════════════════════╣
║                                                              ║
║  ┌──────────────────────┐     1     ┌──────────────────────┐ ║
║  │       UTENTE         │───────────│     STATISTICHE      │ ║
║  │──────────────────────│   possiede│──────────────────────│ ║
║  │ email (PK)           │           │ email (PK, FK) -------│ ║
║  │ nome                 │           │ username             │ ║
║  │ cognome              │           │ partite_giocate      │ ║
║  │ username (UQ)        │           │ vittorie (gioc/bot)  │ ║
║  │ password (XOR)       │           │ partite_online       │ ║
║  │ pin_recupero         │           │ vittorie_online      │ ║
║  │ data_creazione       │           │ punteggio_max        │ ║
║  │ flag_eliminato (lazy)│           │ storico_partite[]    │ ║
║  └──────────┬───────────┘           └──────────────────────┘ ║
║             │                         1                      ║
║             │                          │                      ║
║             │1                         N                      ║
║             │                          v                      ║
║             │              ┌──────────────────────────┐      ║
║             │              │   PARTITA_REGISTRATA      │      ║
║             │              │──────────────────────────│      ║
║             │              │ email (PK, FK)            │      ║
║             │              │ indice (PK)               │      ║
║             │              │ modalità, esito, data     │      ║
║             │              │ durata_secondi, posizione │      ║
║             │              └──────────────────────────┘      ║
║  ┌──────────────────────┐                                    ║
║  │   DATABASE_UTENTI    │  (container globale)               ║
║  │──────────────────────│                                    ║
║  │ utenti[] (array din.)│──1──▶│ BST (DAB)  │(ricerca O(logn))║
║  │ capacity, size       │      └────────────┘               ║
║  │ deleted_list[]       │                                    ║
║  └──────────────────────┘                                    ║
╚══════════════════════════════════════════════════════════════╝

╔══════════════════════════════════════════════════════════════╗
║                  MACRO-AREA SESSIONE DI GIOCO               ║
╠══════════════════════════════════════════════════════════════╣
║                                                              ║
║  ┌─────────────────────────────────────────────────────┐    ║
║  │                    STATO_GIOCO                       │    ║
║  │─────────────────────────────────────────────────────│    ║
║  │ mazzo_di_pesca (Stack*)    │  pila_scarti (Stack*)  │    ║
║  │ turno_corrente, direzione  │  colore_attuale        │    ║
║  │ carta_attuale, fase        │  host_socket_id        │    ║
║  │ giocatori[] (Array)        │  num_giocatori         │    ║
║  └─────┬─────────────────────────────────────┬──────────┘    ║
║        │1                                    │1              ║
║        │N                                    │N              ║
║        v                                    v                ║
║  ┌──────────────────────┐     ┌──────────────────────┐      ║
║  │      GIOCATORE       │     │   MESSAGGIO_CHAT     │      ║
║  │──────────────────────│     │──────────────────────│      ║
║  │ nome (da Utente)     │     │ giocatore_nome       │      ║
║  │ socket_id, is_bot    │     │ timestamp            │      ║
║  │ stato_pronto         │     │ testo[128]           │      ║
║  │ ha_uno, ha_pescato   │     │ colore_giocatore     │      ║
║  │ skip_prox_turno      │     └──────────────────────┘      ║
║  │ turno_attuale        │                                    ║
║  │ mani_vinte           │                                    ║
║  └──────────┬───────────┘                                    ║
║             │1 (mano)                                        ║
║             v                                                ║
║  ┌──────────────────────┐                                    ║
║  │     LISTA_CARTE      │  (lista doppiamente concatenata)   ║
║  │──────────────────────│                                    ║
║  │ testa (Nodo*)        │  coda (Nodo*)                      ║
║  │ contatore            │                                    ║
║  └──────────┬───────────┘                                    ║
║             │1                                                ║
║             N                                                ║
║             v                                                ║
║  ┌──────────────────────┐     ┌──────────────────────┐      ║
║  │     NODO_CARTA       │  N 1│       CARTA          │      ║
║  │──────────────────────│─────│──────────────────────│      ║
║  │ prev (Nodo*)         │     │ colore (4 colori)    │      ║
║  │ next (Nodo*)         │     │ tipo (0-9, Skip,     │      ║
║  │ carta (Carta)        │     │       Reverse, +2,   │      ║
║  └──────────────────────┘     │       Jolly, Jolly+4)│      ║
║                               │ valore, area (sprite)│      ║
║                               └──────────────────────┘      ║
╚══════════════════════════════════════════════════════════════╝

╔══════════════════════════════════════════════════════════════╗
║               MACRO-AREA STRUTTURE DATI DI SUPPORTO          ║
╠══════════════════════════════════════════════════════════════╣
║                                                              ║
║ ┌──────────────┐  ┌──────────────┐  ┌──────────────┐        ║
║ │ STACK (Pila) │  │ QUEUE (Coda) │  │ BST (DAB)    │        ║
║ │──────────────│  │──────────────│  │──────────────│        ║
║ │ Mazzo pesca  │  │ Turni multi- │  │ Ricerca      │        ║
║ │ Pila scarti  │  │ player       │  │ utenti       │        ║
║ │ Push/Pop O(1)│  │ FIFO         │  │ O(log n)     │        ║
║ └──────────────┘  └──────────────┘  └──────────────┘        ║
║                                                              ║
║ ┌──────────────────────┐   ┌──────────────────────┐         ║
║ │  LISTA DOPPIAMENTE   │   │  DATABASE_UTENTI     │         ║
║ │  CONCATENATA (ADT)   │   │──────────────────────│         ║
║ │──────────────────────│   │ Array dinamico + BST │         ║
║ │ Inserimento O(1)     │   │ Capacity espandibile │         ║
║ │ Rimozione O(1)       │   │ Lazy deletion        │         ║
║ └──────────────────────┘   └──────────────────────┘         ║
║                                                              ║
║ ┌──────────────────────┐                                     ║
║ │  STORICO_CHAT        │  (buffer circolare)                ║
║ │──────────────────────│                                     ║
║ │ messaggi[M] (array)  │  testa, coda, contatore            ║
║ │ Inserimento O(1)     │                                     ║
║ └──────────────────────┘                                     ║
╚══════════════════════════════════════════════════════════════╝
\endcode

### Relazioni Principali (riassunto)

| Entità A | Cardinalità | Entità B | Descrizione |
|----------|:-----------:|----------|------------|
| **UTENTE** | 1 → 1 | **STATISTICHE** | Ogni utente ha esattamente un profilo statistiche (stessa email PK/FK) |
| **UTENTE** | 1 → N | **PARTITA_REGISTRATA** | Storico partite giocate (max 100 per utente) |
| **DATABASE_UTENTI** | 1 → N | **UTENTE** | Container globale: array dinamico di utenti |
| **DATABASE_UTENTI** | 1 → 1 | **BST (DAB)** | Albero binario parallelo per ricerca veloce |
| **STATO_GIOCO** | 1 → N | **GIOCATORE** | Partita attiva con 1-4 giocatori |
| **STATO_GIOCO** | 1 → N | **MESSAGGIO_CHAT** | Cronologia chat della sessione |
| **STATO_GIOCO** | 1 → 2 | **STACK** | Mazzo di pesca + pila scarti |
| **GIOCATORE** | 1 → 1 | **LISTA_CARTE** | Mano del giocatore (lista doppiamente concatenata) |
| **LISTA_CARTE** | 1 → N | **NODO_CARTA** | Nodi collegati della lista |
| **NODO_CARTA** | N → 1 | **CARTA** | Ogni nodo contiene esattamente una carta |
| **UTENTE** | 1 → 0..N | **GIOCATORE** | Un utente può partecipare a più partite |

### Dettaglio Composizione Carte UNO

\code
MAZZO COMPLETO (108 carte):
┌─────────────────────────────────────────────────────────────┐
│   COLORE    │  0  │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │ 8 │ 9 │Spec│
├─────────────┼─────┼───┼───┼───┼───┼───┼───┼───┼───┼───┼────┤
│ Rosso       │  1  │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2  │
│ Blu         │  1  │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2  │
│ Verde       │  1  │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2  │
│ Giallo      │  1  │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2 │ 2  │
├─────────────┴─────┴───┴───┴───┴───┴───┴───┴───┴───┴───┴────┤
│ Speciali per colore: Skip (2), Reverse (2), +2 (2)           │
│ Jolly (Wild): 4   |   Jolly+4 (Wild Draw Four): 4           │
└─────────────────────────────────────────────────────────────┘
\endcode

### Strutture Dati Implementate

| Struttura | Tipo | File | Complessità |
|-----------|------|------|-------------|
| **Lista doppiamente concatenata** | ADT generico | `data_structures/list.c` | Inserimento/rimozione O(1) |
| **BST (Binary Search Tree)** | DAB | `data_structures/bst.c` | Ricerca O(log n) medio |
| **Stack (Pila)** | LIFO | `data_structures/stack.c` | Push/pop O(1) |
| **Queue (Coda)** | FIFO | `data_structures/queue.c` | Enqueue/dequeue O(1) |
| **Chat buffer** | Circolare | `data_structures/chat.c` | Inserimento O(1) |
| **DatabaseUtenti** | Array dinamico + BST | `auth.c` | Ricerca O(log n) |

### Mapping Strutture Dati a Carte

\code
Mano del Giocatore:  ListaCarte (lista doppiamente concatenata)
  testa --> NodoCarta <--> NodoCarta <--> NodoCarta <-- coda
               |              |              |
             Carta          Carta          Carta

Mazzo di Pesca:     Stack (pila)
  cima  --> Carta -> Carta -> Carta ... -> fondo

Pila Scarti:        Stack (pila)
  cima  --> Carta -> Carta -> Carta ... -> fondo
  (ultima carta giocata in cima)

Turni Multiplayer:  Coda + array circolare
  front --> [P1] -> [P3] -> [P2] -> [P4] <-- back
  (direzione +1 avanti, -1 indietro)
\endcode

## Flusso Operativo

### Macchina a Stati Principale

\code
+------------------+
|     AVVIO        |
|  Init Raylib     |
|  Carica DB       |
|  Init Assets     |
+------------------+
         |
         v
+------------------+
|  FASE_LOGIN/REG  |<------+
|  Autenticazione  |       |
|  Validazione     |       |
+------------------+       |
         |                 |
    [Login OK]             |
         |                 |
         v                 |
+------------------+       |
|   FASE_MENU      |-------+
|  Scelta modalità |
+------------------+
    |    |     |
    v    v     v
  [SINGOLO] [MULTI] [STATS]

SINGOLO:
  +-> Inizializza Mazzo (deck.c)
  |     Fisher-Yates shuffle
  |-> Inizializza Giocatori + Bot (bot_ai.c)
  |-> Distribuisci 7 carte
  |-> Posiziona prima carta
  |
  v
+------------------+
|  FASE_GAMEPLAY   |
|  Loop Gioco      |
+------------------+
    |
    v
[Turno Giocatore]
    |
    +-> [UMANO]
    |     Mostra mano (lista carte)
    |     Input: click carta / trascinamento
    |     Valida: colore/tipo combacia? (game_logic.c)
    |     Effetto: applica azione speciale
    |     Chiamata UNO se 1 carta
    |     Controlla fine partita
    |
    +-> [BOT]
    |     Valuta mosse possibili (bot_ai.c)
    |     Seleziona strategia:
    |        - easy: carta casuale giocabile
    |        - medium: priorità colore + carte speciali
    |        - hard: albero decisionale (minimax semplificato)
    |     Esegui mossa ottimale
    |
    v
[Aggiorna turno]
  direzione (+1/-1)
  salta se PescaDue/Salta
  cambia direzione se CambioGiro
    |
    v
[Ripeti fino a fine partita]
    |
    v
+------------------+
|  FASE_FINE_PARTITA|
|  Calcola classifica|
|  Aggiorna stats   |
|  Mostra risultati |
|  Offri rivincita  |
+------------------+
\endcode

### Flusso Multiplayer Online

\code
[HOST]                              [CLIENT]
   |                                    |
   v                                    |
+------------+                          |
| Init Server|                          |
| TCP Listen |                          |
+------------+                          |
   |                                    |
   v                                    |
+----------------+                      |
| FASE_HOST_LOBBY|                      |
| Attendi giocat.|                      |
+----------------+                      |
   |                    [Connetti]       |
   |<-----------------------------------|
   v                                    |
+----------------+                      |
| FASE_CLIENT_    |                     |
| LOBBY (Ready)   |                    |
+----------------+                     |
   |                    [Ready OK]       |
   |<-----------------------------------|
   v                                    |
[Inizio Partita]                       |
   |                                    |
   v                                    |
+--------------------+                  |
| Sincronizza Stato  |                  |
| Invia Mazzo/Gioc.  |                  |
+--------------------+                  |
   |                    [Stato Ricevuto] |
   |----------------------------------->|
   |                                    |
   |   +----- Loop Gameplay ----+       |
   |   |                         |       |
   |   v                         |       |
   | [Turno Host]               |       |
   |   |                         |       |
   |   v                         |       |
   | [Invia Pacchetto]          |       |
   |   |                         |       |
   |   v                         |       |
   | [Attendi Pacchetto]        |       |
   |   |                         |       |
   |   v                         |       |
   | [Turno Client]             |       |
   |   |                         |       |
   |   |                         |       |
   +---+-------------------------+       |
       |                                 |
       v                                 |
   [Fine Partita]                       |
       |                                 |
       v                                 |
   +----------------+                    |
   | Sincronizza    |                    |
   | Classifica     |                    |
   +----------------+                    |
       |                                 |
       v                                 |
   [FASE_END_SCREEN] <--------------------+

Protocollo di rete:
  - Heartbeat ogni N secondi (network_utils.c)
  - Packet types: MOVE, DRAW, CHAT, STATE_SYNC, HEARTBEAT, GAME_FINISHED
  - Broadcast LAN per discovery (lan_broadcast.c)
  - DB Sync per admin panel (lan_db_sync.c)
\endcode

### Architettura della Macchina a Stati

\code
STATI PRINCIPALI:
+----------------------+
| FASE_SCELTA_ACCESSO  |  Login / Registrazione / Esci
+----------------------+
         |
         v
+----------------------+
|   FASE_LOGIN         |  Inserimento credenziali
+----------------------+
         |
         v
+----------------------+
| FASE_REGISTRAZIONE   |  Creazione nuovo account
+----------------------+
         |
         v
+----------------------+--------------------------------------+
|     FASE_MENU         |  Nuova Partita | Multiplayer | Stats |
+-----------------------+----------------+-------------+-------+
         |                              |             |
         v                              v             v
+------------------+     +----------------------+  +----------------+
| FASE_GAMEPLAY    |     | FASE_MULTIPLAYER_    |  | FASE_STATS     |
| (vs Bot)         |     | SCELTA (Host/Client) |  | Statistiche    |
+------------------+     +----------------------+  +----------------+
         |                        |
         v                        v
+------------------+     +----------------------+
| FASE_END_SCREEN  |     | FASE_HOST_LOBBY /    |
| Fine Partita     |     | FASE_CLIENT_LOBBY    |
+------------------+     +----------------------+
         |                        |
         v                        v
   [Ritorno Menu]         +----------------------+
                          | FASE_GAMEPLAY (ONLINE)|
                          +----------------------+
                                   |
                                   v
                          +----------------------+
                          | FASE_END_SCREEN      |
                          | (Online)            |
                          +----------------------+

STATI SECONDARI:
- FASE_ADMIN_PANEL: Gestione utenti (da menu con flag admin)
- FASE_PASSWORD_DIMENTICATA: Recupero password via PIN
- FASE_HOST_GAME: Host in attesa durante partita online
- FASE_CLIENT_GAME: Client in attesa durante partita online
- FASE_CARICA_PARTITA: Caricamento partita salvata da file
\endcode

### Gestione Eventi e Input

\code
Ogni frame nel loop principale:
  [Poll Events (Raylib)]
         |
         v
  [Update State Machine]
         |
         +-> [FASE_LOGIN/REG] --> gestisci input campi, pulsanti
         +-> [FASE_MENU]      --> navigazione menu, selezione
         +-> [FASE_GAMEPLAY]  --> eventi click carte, drag, chat
         +-> [FASE_LOBBY]     --> ready, start, chat lobby
         +-> [FASE_END]       --> mostra risultati, scegli rivincita
         |
         v
  [Draw Frame]
         |
         v
  [Swap buffers / vsync]
\endcode

## Componenti del Sistema

### Modulo Autenticazione (`auth.c`, `auth_validation.c`)

- **Registrazione utenti** con validazione email (regex), username (alfanumerico), password (min 8 char, cifratura XOR)
- **Login** con ricerca O(log n) tramite BST (DAB) per username/email
- **Recupero password** tramite PIN di sicurezza a 6 cifre
- **Database dinamico** con allocazione malloc/realloc e capacity espandibile
- **Lazy Deletion**: rimozione logica immediata (`flag_eliminato`), fisica al logout
- **Admin Panel** per gestione utenti via LAN sync (sync DB tra host e client)
- **Validazione input**: email, password, PIN, username con pattern specifici
- **Cifratura password**: XOR con chiave fissa per offuscamento

### Modulo Logica di Gioco (`game_logic.c`, `game/deck.c`)

- **Mazzo UNO completo**: 108 carte (4 colori × speciali + numeriche)
- **Mescolamento**: algoritmo Fisher-Yates con inizializzazione basata su tempo
- **Validazione mosse**: controllo colore/tipo, gestione Jolly con scelta colore
- **Effetti carte**:
  - **Salta (Skip)**: salta turno prossimo giocatore
  - **CambioGiro (Reverse)**: inverte direzione (+1/-1)
  - **PescaDue (+2)**: prossimo pesca 2 carte, perde turno
  - **Jolly (Wild)**: cambia colore a scelta
  - **Jolly+4 (Wild Draw Four)**: cambia colore + prossimo pesca 4
- **Gestione turni**: array circolare con direzione ±1, skip condizionale
- **Pesca automatica**: se mazzo esaurito, rimescola scarti (tranne ultima)
- **Chiamata UNO**: rilevamento quando giocatore ha 1 carta
- **Salvataggio/Caricamento**: serializzazione binaria struttura `StatoGioco` -> file

### Strutture Dati (`data_structures/`)

| Struttura | Utilizzo | Complessità |
|-----------|----------|-------------|
| **Lista Doppiamente Concatenata** | Mano del giocatore (ListaCarte) | Inserimento/rimozione O(1) noti |
| **BST (DAB)** | Ricerca utenti per username/email | O(log n) medio |
| **Stack (Pila)** | Mazzo di pesca e pila scarti | Push/pop O(1) |
| **Queue (Coda)** | Gestione turni multiplayer ordinata | Enqueue/dequeue O(1) |
| **Chat Buffer** | Buffer circolare messaggi chat | Inserimento O(1) |
| **DatabaseUtenti** | Array ridimensionabile + BST per indice | Ricerca O(log n) |

### Interfaccia Grafica (`ui.c`, `screens/game/gameplay_draw.c`, `screens/`)

- **Raylib**: Libreria grafica 2D cross-piattaforma
- **Animazioni**: Trascinamento carte con interpolazione, pesca animata, scarto con effetto
- **UI Adattiva**: Menu navigabili, pulsanti, pannelli notifiche, timer partita
- **Chat Multiplayer**: Overlay chat in-game con messaggi colorati per giocatore, cronologia
- **Feedback visivi**: Notifiche temporanee (toast), indici turno, evidenziazione carte giocabili
- **Admin Panel**: Schermata di gestione utenti remota con sync LAN
- **Campi Input**: Gestione testo con cursore, backspace, limite caratteri

### Networking Multiplayer (`socket/`)

| Componente | Descrizione |
|-----------|-------------|
| **TCP Sockets** | `network.c` - Connessioni stabili e ordinate |
| **Broadcast LAN** | `lan_broadcast.c` - Scoperta automatica server sulla rete locale (UDP) |
| **Protocollo Binario** | `lan_protocol.h` - Formato compatto per ridurre latenza |
| **Packet Handler** | `packet_handler.c` - Dispatcher basato su tipo pacchetto |
| **State Sync** | `state_sync.c` - Sincronizzazione stato gioco host/client |
| **DB Sync** | `lan_db_sync.c` - Sincronizzazione database utenti (admin) |
| **Save Sync** | `lan_save_sync.c` - Sincronizzazione salvataggi tra host e client |
| **Server Manager** | `server_manager.c` - Gestione lobby, sessioni, giocatori |
| **Heartbeat** | `network_utils.c` - Keep-alive e rilevamento disconnessioni |

### Intelligenza Artificiale (`screens/game/bot_ai.c`)

- **Strategia Easy**: Seleziona carta casuale tra quelle giocabili
- **Strategia Medium**: Priorità colore dominante + carte speciali difensive
- **Strategia Hard**: Algoritmo decisionale con valutazione euristica:
  - Punteggio carte per colore e tipo
  - Previsione turni futuri
  - Gestione ottimale carte speciali (Jolly, +4 ritardati)
  - Chiamata UNO automatica

## Architettura Tecnica

### Pattern Architetturali

- **State Pattern**: Macchina a stati finiti con enum `FaseApplicazione` e array di function pointer per update/draw per ogni stato
- **Separation of Concerns**: Suddivisione in `_screen.c` (logica stato), `_update.c` (aggiornamento), `_draw.c` (rendering)
- **Strategy Pattern**: IA Bot con diverse strategie (easy/medium/hard) selezionabili
- **Repository Pattern**: Accesso astratto al database utenti con caricamento/salvataggio su file binario
- **Observer implicito**: Aggiornamento stato giocatori tramite loop centrale con polling
- **Singleton**: DatabaseUtenti e StatoGioco come strutture globali accessibili

### Scelte Progettuali

1. **Lazy Deletion**: Rimozione fisica utenti solo al logout per evitare riallocazioni costose
2. **Denormalizzazione**: Statistiche duplicate in `Statistiche` per accesso O(1)
3. **Serializzazione Flat**: Array backup `mano_backup[]` per salvataggio liste su file
4. **Comunicazione Binary**: Protocollo socket con struct packate compatte (nessuna serializzazione testo)
5. **Rendering 2D**: Raylib per portabilità cross-piattaforma (Windows, Linux, macOS)
6. **StatoGioco Monolitico**: Singola struttura serializzabile per semplicità save/load
7. **DAB + Lista**: Database array lineare + BST parallelo per indicizzazione veloce
8. **Buffer Circolare**: Chat implementata come buffer circolare per memoria pre-allocata O(1)
9. **Configurazione a Compile-time**: Costanti per limiti (max giocatori, max carte, ecc.) definite in header

### Performance e Ottimizzazioni

- **Ricerca utenti O(log n)** con BST (DAB) per username/email
- **Accesso statistiche O(1)** per denormalizzazione in array
- **Inserimenti/rimozioni O(1)** su liste doppiamente concatenate
- **Rendering ottimizzato**: Texture caching, draw call minimization, update selettivo
- **Salvataggio incrementale**: Solo delta modifiche su file binario
- **Gestione memoria**: Pulizia automatica risorse Raylib, free su riallocazioni, no memory leak
- **Broadcast LAN**: UDP multicast per discovery senza polling

## Sviluppi Futuri

- [ ] Modalità torneo con bracket eliminatorie
- [ ] Personalizzazione carte e temi
- [ ] Classifiche globali online
- [ ] Replay partite con log eventi
- [ ] IA avanzata con machine learning
- [ ] Supporto Unicode completo per nickname
- [ ] Achievements e sistema di progressione
- [ ] Modalità spettatore
- [ ] Tornei automatici con scheduling

---

| Autore | Matricola |
|--------|-----------|
| Alessio Decarolis | 852865 |
| Pasquale Franco | 852866 |

**Versione**: 9.1.3  
**Data**: 1/07/2026
