/**
 * @file network.c
 * @brief Modulo di rete multiplayer TCP/UDP - host e client.
 * @ingroup network
 *
 * @details Gestisce la connessione multiplayer online per le partite UNO.
 *          Implementa architettura client-server con:
 *          - HOST: crea stanza TCP, accetta client, gestisce disconnessioni
 *                  e sostituzione con bot, invia broadcast stato di gioco
 *          - CLIENT: si connette a stanza, riceve pacchetti, invia comandi
 *          - Entrambi: invio/ricezione pacchetti NetPacket via TCP
 *
 * @note Utilizza socket non-blocking con WSAEWOULDBLOCK per gestire
 *       multiple connessioni in modo asincrono. Ogni client ha un
 *       buffer separato (recvBuffer[4]) per evitare corruzione pacchetti.
 *
 * Protocollo:
 *   - TCP porta 27015 per il gioco (connessioni stabili)
 *   - UDP porta 27016 per discovery codici stanza
 *   - NetPacket Binario 512 byte per tutti i messaggi
 *
 * Architettura:
 *   - MAX_SERVER_CLIENTS = 4 slot (0 riservato host, 1-3 per client)
 *   - Buffer separati recvBuffer[4] per ogni client
 *   - clientRecvBuffer per ricezione client singolo
 *   - Gestione riconnessione con riassegnazione socket (RiassegnaSocket)
 *
 * File correlati:
 *   - network.h        : API pubblica e definizioni
 *   - network_send.c   : funzioni di invio (BroadcastGameState, SendMove, ecc.)
 *   - packet_handler.c : dispatcher pacchetti ricevuti
 *   - state_sync.c     : SyncGameStateFromNetwork per client
 *   - network_utils.c  : utility (GestisciDisconnessione, ShiftNetworkSockets)
 *
 * @author Pasquale Franco
 * @version 9.1.3
 * @date 2026-07-01
 */
#include "../../../lib/socket/network/socket_compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/socket/network/packet_handler.h"
#include "../../../lib/socket/server/server_manager.h"
#include "../../../lib/game_logic.h"
#include "../../../lib/auth.h"
#include "../../../lib/list.h"

#include "raylib.h"

/* ============================================================
 *  VARIABILI GLOBALI - Socket e stato di connessione
 * ============================================================ */

/** @brief Socket TCP principale (host: listening, client: connesso). */
SOCKET localSocket = INVALID_SOCKET;
/** @brief Socket remoto (solo client: connessione all'host). */
SOCKET remoteSocket = INVALID_SOCKET;

/** @brief Username dei client TCP connessi all'host (per admin panel).
 *  NOTA: Indice 0 corrisponde allo slot 1 del client. */
char connectedClientUsernames[4][30] = {{0}};

/** @brief Socket UDP per discovery codici stanza (host+client). */
SOCKET udpDiscoverySocket = INVALID_SOCKET;

/* Codice stanza per la modalita' multiplayer diretta (senza session manager).
 * Usato per la risoluzione UDP REQ_CODE. Il codice e' generato dall'IP LAN
 * dell'host (formato esadecimale), quindi e' gia' univoco per ogni dispositivo. */
static char direct_multiplayer_code[16] = "";

// Mappiamo i socket direttamente agli ID giocatore (1, 2, 3). L'indice 0 non si usa (e' l'Host).
// MAX_SERVER_CLIENTS e' ora in network.h
SOCKET clientSockets[MAX_SERVER_CLIENTS] = { INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET };
int numConnectedClients = 0;

NetworkRole currentRole = NET_OFFLINE;
int isConnected = 0;
int network_pending = 0;
int network_game_initialized = 0;
int network_client_joined = 0;
int network_start_game_flag = 0;
int join_rejected = 0;
char reject_message[100] = "";
int player_id = -1;                 // ID globale del giocatore
int host_in_prelobby = 0;           // 1 se l'host e' gia' tornato in pre-lobby
char current_session_id[16] = "";   // ID della sessione corrente
char session_id[16] = "";           // ID della sessione (alias per compatibilita')
int local_player_id = -1;           // ID del giocatore nella sessione

struct sockaddr_in serverAddr;

// Assegniamo un buffer separato per OGNI client per evitare che i pacchetti si corrompano mischiandosi!
static char recvBuffer[4][sizeof(NetPacket) * 2 + 4096]; // Buffer per ogni client (almeno 2 pacchetti + margine)
static int recvOffset[4] = {0, 0, 0, 0};
static char clientRecvBuffer[sizeof(NetPacket) * 2 + 4096]; // Buffer isolato per quando si fa da Client (almeno 2 pacchetti + margine)
static int clientRecvOffset = 0;

int InitNetwork(void) {
    WSADATA wsaData;
    return (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
}

void CloseNetwork(void) {
    // Chiusura chirurgica: shutdown prima di closesocket per permettere
    // l'invio dei dati pendenti (es. PACKET_HOST_DISCONNECT) prima della
    // chiusura effettiva del socket.
    if (localSocket != INVALID_SOCKET) { shutdown(localSocket, SD_BOTH); closesocket(localSocket); localSocket = INVALID_SOCKET; }
    if (remoteSocket != INVALID_SOCKET) { shutdown(remoteSocket, SD_BOTH); closesocket(remoteSocket); remoteSocket = INVALID_SOCKET; }
    if (udpDiscoverySocket != INVALID_SOCKET) { shutdown(udpDiscoverySocket, SD_BOTH); closesocket(udpDiscoverySocket); udpDiscoverySocket = INVALID_SOCKET; }
    
    for (int i = 0; i < MAX_SERVER_CLIENTS; i++) {
        if (clientSockets[i] != INVALID_SOCKET) {
            shutdown(clientSockets[i], SD_BOTH);
            closesocket(clientSockets[i]);
            clientSockets[i] = INVALID_SOCKET;
        }
    }
    
    currentRole = NET_OFFLINE;
    isConnected = 0;
    numConnectedClients = 0;
    network_client_joined = 0;
    network_game_initialized = 0;
    network_start_game_flag = 0;
    host_in_prelobby = 0;
}

void SetNonBlocking(SOCKET sock) {
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
}

int HostGame(int port) {
    // Se siamo gia' Host, non ri-creiamo il socket forzando la porta!
    if (currentRole == NET_HOST && localSocket != INVALID_SOCKET) return 1;
    
    CloseNetwork(); // Assicuriamoci che tutto sia pulito prima di aprire
    
    // 1. Configurazione TCP per le stanze multiple
    localSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (localSocket == INVALID_SOCKET) return 0;
    
    int opt = 1;
    setsockopt(localSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if (bind(localSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) { 
        closesocket(localSocket); 
        return 0; 
    }
    listen(localSocket, 10); // Permette piu' connessioni in coda
    SetNonBlocking(localSocket);

    // 2. Configurazione UDP per ascoltare i client che cercano i codici delle stanze
    udpDiscoverySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpDiscoverySocket != INVALID_SOCKET) {
        setsockopt(udpDiscoverySocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
        struct sockaddr_in udpAddr;
        udpAddr.sin_family = AF_INET;
        udpAddr.sin_addr.s_addr = INADDR_ANY;
        udpAddr.sin_port = htons(port + 1); // Porta UDP affiancata a quella TCP
        bind(udpDiscoverySocket, (struct sockaddr*)&udpAddr, sizeof(udpAddr));
        SetNonBlocking(udpDiscoverySocket);
    }

    currentRole = NET_HOST;
    local_player_id = 0; // L'Host e' SEMPRE il giocatore 0
    for(int i=0; i<MAX_SERVER_CLIENTS; i++) clientSockets[i] = INVALID_SOCKET;
    
    // FEEDBACK FIX: Usa codice hex da IP (es. C0A8010A) invece di GenerateSessionId()
    // che genera codici non-esadecimali (es. ILGDAADR) impossibili da inserire per i client.
    // Ogni dispositivo ha IP unico sulla LAN, quindi il codice e' gia' univoco.
    char localIP[32] = "";
    GetLocalIPAddress(localIP, sizeof(localIP));
    if (localIP[0] != '\0') {
        IpToCode(localIP, direct_multiplayer_code);
    } else {
        // Fallback nel caso l'IP non sia disponibile
        strcpy(direct_multiplayer_code, "UNKNOWN");
    }
    strcpy(session_id, direct_multiplayer_code);
    
    return 1;
}

int JoinGame(const char* ip_address, int port) {
    localSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (localSocket == INVALID_SOCKET) return 0;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip_address);
    serverAddr.sin_port = htons(port);
    if (connect(localSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) { closesocket(localSocket); return 0; }
    SetNonBlocking(localSocket);
    remoteSocket = localSocket;
    currentRole = NET_CLIENT;
    isConnected = 1;
    return 1;
}

void UpdateNetwork(StatoGioco* gioco) {
    if (currentRole == NET_OFFLINE) return;

    if (currentRole == NET_HOST) {
        // --- 1. RISOLUZIONE DEI CODICI VIA UDP ---
        if (udpDiscoverySocket != INVALID_SOCKET) {
            char udpBuf[64];
            struct sockaddr_in clientUdpAddr;
            int addrLen = sizeof(clientUdpAddr);
            int bytes = recvfrom(udpDiscoverySocket, udpBuf, sizeof(udpBuf) - 1, 0, (struct sockaddr*)&clientUdpAddr, &addrLen);
            if (bytes > 0) {
                udpBuf[bytes] = '\0';
                // Il client invia il codice che sta cercando, es: "REQ_CODE:X7R2"
                if (strncmp(udpBuf, "REQ_CODE:", 9) == 0) {
                    char targetCode[16];
                    strcpy(targetCode, udpBuf + 9);
                    
                    // Controlla sia nel server manager che nella modalita' multiplayer diretta
                    if (GetSession(targetCode) != NULL || strcmp(targetCode, direct_multiplayer_code) == 0) {
                        // Risponde al client confermando che la stanza esiste su questo server
                        char reply[] = "CODE_FOUND";
                        sendto(udpDiscoverySocket, reply, strlen(reply), 0, (struct sockaddr*)&clientUdpAddr, addrLen);
                    }
                }
            }
        }

        // --- 2. ACCETTAZIONE DI NUOVI CLIENT TCP ---
        struct sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET incoming = accept(localSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (incoming != INVALID_SOCKET) {
            SetNonBlocking(incoming);
            int assigned = 0;
            int assigned_slot = -1;
            // Assegniamo i client agli slot 1, 2 o 3
            for (int i = 1; i < 4; i++) {
                if (clientSockets[i] == INVALID_SOCKET) {
                    clientSockets[i] = incoming;
                    numConnectedClients++;
                    isConnected = 1;
                    assigned = 1;
                    assigned_slot = i;
                    break;
                }
            }
            if (!assigned) {
                closesocket(incoming); // Stanza piena
            } else {
                // Invia subito lo stato completo al nuovo client
                // cosi' vede tutti i giocatori (host, client, bot) immediatamente.
                // IMPORTANTE: Azzera la notifica per evitare lampeggiamenti/flash
                // di notifiche dell'host sul client al momento della connessione.
                if (gioco && gioco->num_giocatori > 0) {
                    NetPacket statePacket = {0};
                    statePacket.type = PACKET_GAME_STATE;
                    statePacket.data.game_state.stato_gioco = *gioco;
                    // Pulisce la notifica per evitare flash di notifiche dell'host sul nuovo client
                    statePacket.data.game_state.stato_gioco.timer_notifica = 0.0f;
                    statePacket.data.game_state.stato_gioco.notifica[0] = '\0';
                    send(clientSockets[assigned_slot], (char*)&statePacket, sizeof(NetPacket), 0);
                }
            }
        }

        // --- 3. RICEZIONE DATI DA TUTTI I CLIENT CONNESSI ---
        for (int i = 1; i < 4; i++) { 
            if (clientSockets[i] != INVALID_SOCKET) {
                // Usiamo il buffer specifico per questo socket (recvBuffer[i])
                int bytesReceived = recv(clientSockets[i], recvBuffer[i] + recvOffset[i], sizeof(recvBuffer[i]) - recvOffset[i], 0);
                if (bytesReceived > 0) {
                    recvOffset[i] += bytesReceived;
                    while (recvOffset[i] >= (int)sizeof(NetPacket)) {
                        NetPacket packet;
                        memcpy(&packet, recvBuffer[i], sizeof(NetPacket));
                        packet.sender_id = i; 
                        HandleReceivedPacket(&packet, gioco);
                        int leftover = recvOffset[i] - sizeof(NetPacket);
                        if (leftover > 0) memmove(recvBuffer[i], recvBuffer[i] + sizeof(NetPacket), leftover);
                        recvOffset[i] = leftover;
                    }
                } else if (bytesReceived == 0 || (bytesReceived == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)) {
                    // Disconnessione pulita - pulisce anche il tracking admin
                    connectedClientUsernames[i][0] = '\0';
                    closesocket(clientSockets[i]);
                    clientSockets[i] = INVALID_SOCKET;
                    numConnectedClients--;

                    if (gioco && gioco->num_carte_mazzo > 0 && !gioco->gioco_finito) {
                        // PARTITA IN CORSO: sostituisci il giocatore con un BOT
                        int trovato = 0;
                        for (int p = 0; p < gioco->num_giocatori; p++) {
                            if (gioco->giocatori[p].socket_id == i) {
                                gioco->giocatori[p].is_bot = 1;
                                gioco->giocatori[p].socket_id = -1;
                                // Mantieni il nome originale per permettere il rientro
                                // Messaggio corto per non coprire le carte nel riquadro notifica
                                MostraNotifica(gioco, TextFormat("%s: sostituito da BOT", Giocatore_Nome(&gioco->giocatori[p])));
                                trovato = 1;
                                break;
                            }
                        }
                        // Se non trovato per socket_id, cerca per nome usando i socket collegati
                        if (!trovato) {
                            for (int p = 0; p < gioco->num_giocatori; p++) {
                                if (!gioco->giocatori[p].is_bot && gioco->giocatori[p].socket_id == -1) {
                                    // Questo potrebbe essere un giocatore che non ha ancora ricevuto socket_id
                                    // Controlla se il suo socket e' ancora aperto ma non corrisponde
                                    continue;
                                }
                            }
                        }
                    } else if (gioco && !gioco->gioco_finito) {
                        // PRE-LOBBY: rimuovi il giocatore dalla lista
                        // NOTA: A partita FINITA (gioco_finito == 1) NON rimuoviamo il giocatore
                        // dalla classifica finale: tutti i partecipanti devono rimanere visibili.
                        for (int p = 0; p < gioco->num_giocatori; p++) {
                            if (gioco->giocatori[p].socket_id == i) {
                                for (int k = p; k < gioco->num_giocatori - 1; k++) {
                                    gioco->giocatori[k] = gioco->giocatori[k+1];
                                }
                                gioco->num_giocatori--;
                                memset(&gioco->giocatori[gioco->num_giocatori], 0, sizeof(Giocatore));
                                gioco->giocatori[gioco->num_giocatori].socket_id = -1;
                                gioco->giocatori[gioco->num_giocatori].is_bot = 0;
                                gioco->giocatori[gioco->num_giocatori].stato_pronto = 0;
                                break;
                            }
                        }
                    } else if (gioco && gioco->gioco_finito) {
                        // PARTITA FINITA - Il client ha premuto "Esci".
                        // NON rimuovere il giocatore dall'array giocatori[] per preservare
                        // la classifica finale (tutti i partecipanti devono rimanere visibili).
                        // Imposta solo come BOT con socket_id = -1 e mostra notifica.
                        for (int p = 0; p < gioco->num_giocatori; p++) {
                            if (gioco->giocatori[p].socket_id == i) {
                                const char* nome_player = Giocatore_Nome(&gioco->giocatori[p]);
                                gioco->giocatori[p].is_bot = 1;       // Segna come bot (nessun rientro)
                                gioco->giocatori[p].socket_id = -1;   // Libera socket
                                gioco->giocatori[p].stato_pronto = 0; // Non piu' pronto
                                // Notifica in stile gioco "il client A ha lasciato la stanza"
                                MostraNotifica(gioco, TextFormat("%s ha lasciato la stanza", nome_player));
                                break;
                            }
                        }
                        // Salva le statistiche se non sono gia' state salvate
                        if (!gioco->statistiche_gia_salvate) {
                            GestisciDisconnessione(gioco);
                        }
                    }
                    
                    BroadcastGameState(gioco);
                    if (numConnectedClients == 0) isConnected = 0;
                }
            }
        }
    } 
    else if (currentRole == NET_CLIENT) {
        // --- LOGICA CLIENT: RICEZIONE DATI DAL SERVER ---
        if (isConnected && remoteSocket != INVALID_SOCKET) {
            int bytesReceived = recv(remoteSocket, clientRecvBuffer + clientRecvOffset, sizeof(clientRecvBuffer) - clientRecvOffset, 0);
            if (bytesReceived > 0) {
                clientRecvOffset += bytesReceived;
                while (clientRecvOffset >= (int)sizeof(NetPacket)) {
                    NetPacket packet;
                    memcpy(&packet, clientRecvBuffer, sizeof(NetPacket));
                    HandleReceivedPacket(&packet, gioco);
                    int leftover = clientRecvOffset - sizeof(NetPacket);
                    if (leftover > 0) memmove(clientRecvBuffer, clientRecvBuffer + sizeof(NetPacket), leftover);
                    clientRecvOffset = leftover;
                }
            } else if (bytesReceived == 0 || (bytesReceived == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)) {
                GestisciDisconnessione(gioco);
            }
        }
    }
}

// Le funzioni GestisciDisconnessione, GetLocalIPAddress, IpToCode,
// CodeToIp, ConnettiTramiteCodiceStanza, ShiftNetworkSockets sono
// state spostate in network_utils.c per la fattorizzazione del file.

int IsPlayerConnected(int player_id) {
    if (currentRole == NET_HOST && player_id >= 1 && player_id <= 3) {
        return (clientSockets[player_id] != INVALID_SOCKET);
    }
    return 0;
}

void RiassegnaSocket(int old_id, int new_id, int is_reconnect, int partita_iniziata) {
    if (currentRole != NET_HOST) return;
    
    // CASO 1: old_id == new_id -> nessuno spostamento fisico, ma il socket e' gia' allocato
    //         correttamente. Serve solo inviare PACKET_START_GAME se partita iniziata.
    // CASO 2: old_id != new_id -> sposta il socket dallo slot temporaneo a quello del giocatore
    if (old_id >= 1 && old_id <= 3 && new_id >= 1 && new_id <= 3) {
        if (old_id != new_id) {
            // Sposta il socket dallo slot temporaneo (assegnato da accept) allo slot del giocatore
            clientSockets[new_id] = clientSockets[old_id];
            clientSockets[old_id] = INVALID_SOCKET; // Pulisce SEMPRE il vecchio slot temporaneo
        }
    } else if (old_id >= 1 && old_id <= 3 && new_id == -1) {
        // CASO SPECIALE: nessun riassegnamento, solo pulizia slot temporaneo inutilizzato
        clientSockets[old_id] = INVALID_SOCKET;
    }
    
    if (is_reconnect) {
        // Invia PACKET_START_GAME al client riconnesso se la partita e' gia' iniziata
        if (partita_iniziata && new_id >= 0 && new_id <= 3 && clientSockets[new_id] != INVALID_SOCKET) {
            NetPacket startPkt = {0};
            startPkt.type = PACKET_START_GAME;
            send(clientSockets[new_id], (char*)&startPkt, sizeof(NetPacket), 0);
        }
    }
}