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
- **Classifica**: piazzamento e tempi delle partite registrati

## Tecnologie

- **Linguaggio**: C (standard C11)
- **IDE**: Code::Blocks
- **GUI e Input**: Raylib
- **Networking**: Socket TCP/IP
- **Persistenza**: File ad accesso diretto (.dat) e file sequenziali

## Struttura del progetto

```
Uno/
├── src/
│   ├── main.c               # Entry point e ciclo di vita dell'applicazione
│   ├── auth.c               # Gestione registrazione, login e BST utenti
│   ├── auth_validation.c    # Validazione email, password, username
│   ├── game_logic.c         # Regole di UNO, validazione mosse, effetti carte
│   ├── list.c               # Implementazione lista doppiamente concatenata
│   ├── ui.c                 # Rendering e gestione interfaccia Raylib
│   └── data_structures/     # Strutture dati specializzate per il gioco
├── lib/
│   ├── auth.h               # Header modulo autenticazione
│   ├── auth_validation.h    # Header modulo validazione
│   ├── data_structures.h    # Tipi fondamentali (Carta, Utente, Giocatore, etc.)
│   ├── game_logic.h         # Header modulo logica di gioco
│   ├── list.h               # Header lista concatenata
│   ├── ui.h                 # Header interfaccia grafica
│   ├── socket/              # Gestione connessioni di rete
│   ├── screens/             # Schermate dell'applicazione
│   ├── game/                # Entità di gioco specializzate
│   └── raylib/              # Libreria grafica
├── data/
│   ├── users/               # Database utenti
│   └── games/               # Salvataggi partite
├── assets/
│   ├── card/                # Texture carte
│   └── altro/               # Risorse grafiche varie
├── docs/
│   └── Documentazione CdS.pdf  # Documentazione progettuale completa
├── bin/                     # Eseguibili compilati
├── obj/                     # File oggetto
└── build/                   # File di build
```

## Architettura

### Strutture dati principali

- **Carta**: colore, valore numerico (0-9) e tipo (normale, CambioGiro, Salta, +2, Jolly, Jolly+4)
- **Utente**: dati anagrafici, credenziali e statistiche associate
- **Giocatore**: informazioni utente, flag bot, stato lobby, numero carte in mano
- **StatoGioco**: stato completo della partita (mazzi, giocatori, turno corrente, notifiche)

### Strutture dati dinamiche

- **Lista doppiamente concatenata**: mano del giocatore
- **Pila (Stack)**: mazzo di pesca e mazzo degli scarti
- **Coda (Queue)**: gestione turni di gioco
- **Albero BST**: caricamento utenti in RAM per ricerca logaritmica

### Moduli funzionali

| Modulo | Responsabilità |
|--------|----------------|
| `auth` | Registrazione, login, cancellazione utenti, ricostruzione BST |
| `auth_validation` | Validazione formale email, password, username, PIN |
| `game_logic` | Validazione mosse, applicazione effetti, mazzo, pescate, salvataggi |
| `list` | Operazioni su lista mani (inserimenti, rimozioni, accesso) |
| `ui` | Disegno carte realistiche, bottoni animati, gestione textures |
| `socket` | Connessioni host/client, sincronizzazione stato multigiocatore |

## Requisiti di sistema

- **Sistema operativo**: Windows 10/11
- **Compilatore**: GCC (testato con MinGW)
- **IDE**: Code::Blocks (consigliato) o compilazione da linea di comando
- **Librerie**: Raylib (inclusa in `lib/raylib/`)

## Compilazione

### Prerequisiti

Assicurarsi di avere installato:
- Code::Blocks con supporto C
- MinGW GCC con supporto C11

### Procedura

1. Aprire il file `Uno.cbp` in Code::Blocks.
2. Selezionare la configurazione di build **Debug** o **Release**.
3. Premere **F9** o selezionare *Build > Build*.

In alternativa, da linea di comando (nella cartella del progetto):

```bash
gcc -std=c11 -Wall -Wextra -Ilib -Isrc -c src/main.c -o obj/main.o
gcc -std=c11 -Wall -Wextra -Ilib -Isrc -c src/auth.c -o obj/auth.o
gcc -std=c11 -Wall -Wextra -Ilib -Isrc -c src/auth_validation.c -o obj/auth_validation.o
gcc -std=c11 -Wall -Wextra -Ilib -Isrc -c src/game_logic.c -o obj/game_logic.o
gcc -std=c11 -Wall -Wextra -Ilib -Isrc -c src/list.c -o obj/list.o
gcc -std=c11 -Wall -Wextra -Ilib -Isrc -c src/ui.c -o obj/ui.o
gcc obj/main.o obj/auth.o obj/auth_validation.o obj/game_logic.o obj/list.o obj/ui.o -Llib/raylib -lraylib -o bin/Uno
```

## Esecuzione

Dopo la compilazione, eseguire:

```bash
bin/Uno.exe
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

### Funzionalità avanzate in sviluppo

- Tasto "UNO!" con时限 per penalità
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

- **Alessio Decarolis** - Matricola 852865 - 
- **Pasquale Franco** - Matricola 852866 - https://github.com/pasqfrn

## Licenza

Progetto didattico - Eventuale licenza da definire.

## Note

- La documentazione progettuale completa è disponibile in  `C:\Users\pasqu\OneDrive\Documenti\Uno\docs\doxygen\html\index.html` 
- Nel branch corrente sono presenti funzionalità di gestione avanzata del database (marcature per eliminazione, ricostruzione BST, cifratura)
- Le texture delle carte sono caricate dinamicamente dalla cartella `assets/card/`
