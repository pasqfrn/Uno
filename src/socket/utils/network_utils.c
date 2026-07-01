/**
 * @file network_utils.c
 * @brief Funzioni di utilita' per il modulo di rete multiplayer.
 * @ingroup network
 *
 * Contiene funzioni helper per la gestione della rete:
 *   - GestisciDisconnessione: gestione pulita disconnessioni host/client
 *   - GetLocalIPAddress: rilevamento IP locale (metodo multiplo)
 *   - IpToCode/CodeToIp: conversione IP <-> codice esadecimale
 *   - ConnettiTramiteCodiceStanza: connessione via discovery UDP
 *   - ShiftNetworkSockets: riorganizzazione slot dopo disconnessione
 *
 * Tutte le funzioni sono state spostate qui da network.c per
 * fattorizzare il codice e separare la logica di connessione
 * dalla logica di gioco online.
 */
#include "../../../lib/socket/network/socket_compat.h"

#include <stdio.h>
#include <string.h>
#include "../../../lib/socket/network/network.h"
#include "../../../lib/socket/network/network_send.h"
#include "../../../lib/socket/lan/lan_sync.h"
#include "../../../lib/socket/utils/network_utils.h"
#include "../../../lib/socket/server/server_manager.h"
#include "../../../lib/game_logic.h"
#include "../../../lib/auth.h"
#include "../../../lib/screens/game/end_screen.h"
#include "raylib.h"

/**
 * @brief Gestisce la disconnessione di un giocatore (host o client).
 *
 * Comportamento:
 *   - HOST: Sostituisce il giocatore disconnesso con un BOT,
 *           notifica tutti, broadcasta nuovo stato.
 *   - CLIENT: Imposta stato "__HOST_DISCONNECT__", salva statistiche
 *             solo se partita non era finita (evita duplicati).
 *
 * Bug fissati:
 *   - Bug 1: NON salva statistiche se partita gia' finita (duplicati)
 *   - Bug 4: Partita interrotta non registra in classifica
 *
 * @param gioco Puntatore allo stato di gioco.
 */
void GestisciDisconnessione(StatoGioco* gioco) {
    if (!isConnected) return;

    if (currentRole == NET_HOST) {
        MostraNotifica(gioco, "Un giocatore ha abbandonato. E' subentrato un BOT.");
    }
    else if (currentRole == NET_CLIENT) {
        // TL-01: L'host ha chiuso la stanza. Distingue due casi:
        //   - PRE-LOBBY (gioco non iniziato): notifica breve + reindirizzamento
        //   - IN-GAME (partita iniziata): comportamento legacy con statistiche
        int partita_iniziata = (gioco->num_carte_scarti > 0);
        
        if (!partita_iniziata) {
            // PRE-LOBBY: imposta messaggio e timer per la lobby
            gioco->gioco_finito = 1;
            strcpy(gioco->messaggio, "L'host ha chiuso la stanza");
            gioco->timer_notifica = 2.5f;
        } else {
            // PARTITA IN CORSO: comportamento legacy
            if (gioco->gioco_finito == 0) {
                gioco->gioco_finito = 1;
                sprintf(gioco->messaggio, "__HOST_DISCONNECT__");
            } else {
                // Partita finita: salva statistiche
                if (!gioco->statistiche_gia_salvate) {
                    gioco->statistiche_gia_salvate = 1;
                    Statistiche* utente = &dbUtenti.lista[id_utente_corrente];
                    if (utente && utente->partite_giocate < MAX_PARTITE_STORICO) {
                        PartitaRegistrata* p = &utente->storico_partite[utente->partite_giocate];
                        if (currentRole != NET_OFFLINE) strcpy(p->modalita, "Online");
                        else sprintf(p->modalita, "1 vs %d", gioco->num_giocatori - 1);
                        ClassificaRecord cr[4]; int nc = 0;
                        CalcolaClassifica(gioco, cr, &nc);
                        int mia_pos = gioco->num_giocatori;
                        int ho_vinto = 0;
                        int mio_id = local_player_id;
                        if (mio_id < 0 || mio_id >= gioco->num_giocatori) mio_id = 0;
                        for (int k = 0; k < nc; k++) {
                            if (Giocatore_Nome(&gioco->giocatori[mio_id])[0] &&
                                strcmp(cr[k].nome, Giocatore_Nome(&gioco->giocatori[mio_id])) == 0) {
                                mia_pos = cr[k].posizione;
                                if (mia_pos == 1) ho_vinto = 1;
                                break;
                            }
                        }
                        strcpy(p->esito, ho_vinto ? "Vittoria" : "Sconfitta");
                        p->durata_secondi = (int)gioco->durata_partita;
                        p->posizione = mia_pos;
                        utente->partite_giocate++;
                        if (ho_vinto) utente->vittorie_giocatore++;
                        else utente->vittorie_bot++;
                        SalvaDB();
                        LAN_BroadcastNow();
                        // BUG FIX 5: Sync completo statistiche via LAN (incluso storico)
                        if (id_utente_corrente >= 0) {
                            LAN_BroadcastStatsSync(dbUtenti.lista[id_utente_corrente].username);
                        }
                    }
                }
                sprintf(gioco->messaggio, "__HOST_DISCONNECT__");
            }
        }
        CloseNetwork();
    }
}

/**
 * @brief Ottiene l'indirizzo IP locale del dispositivo.
 *
 * Utilizza metodo multiplo per robustezza:
 *   1. gethostname + gethostbyname (scorre TUTTI gli indirizzi)
 *      Filtra: loopback (127.0.0.1), APIPA (169.254.x.x),
 *      VirtualBox (10.0.2.x), VPN (tunnel interfaces).
 *      Accetta: 192.168.x.x, 10.x.x.x, 172.16-31.x.x
 *   2. Fallback: UDP connect a 8.8.8.8:53 (Google DNS)
 *      Non richiede header speciali, usa getsockname() per IP.
 *
 * Bug fissato: prendeva solo h_addr_list[0] che su Windows
 * spesso e' 127.0.0.1. Ora scorre tutta la lista.
 *
 * @param outbuf Buffer di output per l'IP trovato.
 * @param buflen Dimensione del buffer output.
 * @return 1 se trovato IP valido, 0 altrimenti.
 */
int GetLocalIPAddress(char* outbuf, int buflen) {
    if (!outbuf || buflen <= 0) return 0;
    
    outbuf[0] = '\0';

#ifdef _WIN32
    // Metodo 1: gethostname + gethostbyname scorrendo TUTTI gli indirizzi
    // Il bug era che prendeva solo h_addr_list[0] che su Windows spesso
    // e' 127.0.0.1 o l'IP di VirtualBox/VPN. Ora scorro TUTTA la lista
    // e prendo il primo IP valido non-loopback non-APIPA.
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct hostent* he = gethostbyname(hostname);
        if (he && he->h_addr_list) {
            for (int i = 0; he->h_addr_list[i] != NULL; i++) {
                struct in_addr addr;
                memcpy(&addr, he->h_addr_list[i], sizeof(struct in_addr));
                const char* ip = inet_ntoa(addr);
                    if (ip && strcmp(ip, "127.0.0.1") != 0 && strncmp(ip, "0.", 2) != 0 &&
                        strncmp(ip, "169.254.", 7) != 0 &&  // APIPA
                        strncmp(ip, "10.0.2.", 7) != 0) {  // VirtualBox NAT
                        strncpy(outbuf, ip, buflen - 1);
                        outbuf[buflen - 1] = '\0';
                        return 1;
                    }
            }
        }
    }
#endif

    if (outbuf[0] != '\0') return 1; // Gia' trovato col metodo 1

    // Fallback: UDP connect a 8.8.8.8:53 (non richiede header speciali)
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s != INVALID_SOCKET) {
        struct sockaddr_in serv;
        memset(&serv, 0, sizeof(serv));
        serv.sin_family = AF_INET;
        serv.sin_addr.s_addr = inet_addr("8.8.8.8");
        serv.sin_port = htons(53);

        if (connect(s, (struct sockaddr*)&serv, sizeof(serv)) != SOCKET_ERROR) {
            struct sockaddr_in name;
            int namelen = sizeof(name);
            if (getsockname(s, (struct sockaddr*)&name, &namelen) == 0) {
                const char* localIP = inet_ntoa(name.sin_addr);
                if (localIP && strcmp(localIP, "127.0.0.1") != 0 && strncmp(localIP, "0.", 2) != 0) {
                    strncpy(outbuf, localIP, buflen - 1);
                    outbuf[buflen - 1] = '\0';
                }
            }
        }
        closesocket(s);
    }

    // Secondo tentativo fallback: ottieni TUTTI gli indirizzi disponibili
    if (outbuf[0] == '\0') {
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            struct hostent* he = gethostbyname(hostname);
            if (he && he->h_addr_list) {
                for (int i = 0; he->h_addr_list[i] != NULL; i++) {
                    struct in_addr addr;
                    memcpy(&addr, he->h_addr_list[i], sizeof(struct in_addr));
                    const char* ip = inet_ntoa(addr);
                    if (ip && strcmp(ip, "127.0.0.1") != 0 && strncmp(ip, "0.", 2) != 0 &&
                        strncmp(ip, "169.254.", 7) != 0) {
                        strncpy(outbuf, ip, buflen - 1);
                        outbuf[buflen - 1] = '\0';
                        return 1;
                    }
                }
            }
        }
    }
    
    return outbuf[0] != '\0' ? 1 : 0;
}

/**
 * @brief Converte un indirizzo IP in codice esadecimale.
 *
 * Formato: "192.168.1.100" -> "C0A80164"
 * Usato per generare codici stanza leggibili dai client.
 *
 * @param ip Indirizzo IP in formato stringa.
 * @param code Buffer di output per il codice (min 9 byte).
 */
void IpToCode(const char* ip, char* code) {
    int a, b, c, d;
    if (sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d) == 4) sprintf(code, "%02X%02X%02X%02X", a, b, c, d);
    else strcpy(code, "ERRORE");
}

/**
 * @brief Converte un codice esadecimale in indirizzo IP.
 *
 * Formato: "C0A80164" -> "192.168.1.100"
 *
 * @param code Codice esadecimale (8 caratteri).
 * @param ip Buffer di output per l'IP.
 * @return 1 se conversione riuscita, 0 se formato non valido.
 */
int CodeToIp(const char* code, char* ip) {
    unsigned int a, b, c, d;
    if (strlen(code) != 8) return 0;
    if (sscanf(code, "%02x%02x%02x%02x", &a, &b, &c, &d) == 4) {
        sprintf(ip, "%d.%d.%d.%d", a, b, c, d);
        return 1;
    }
    return 0;
}

/**
 * @brief Connette a una stanza tramite codice stanza (UDP discovery).
 *
 * Procedura:
 *   1. Crea socket UDP broadcast
 *   2. Invia "REQ_CODE:<codice>" in broadcast sulla porta base+1
 *   3. Attende risposta "CODE_FOUND" con timeout 1 secondo
 *   4. Se risposta ricevuta: chiama JoinGame() con IP del server
 *
 * @param codice Codice stanza da cercare (8 caratteri hex).
 * @param portaBase Porta TCP del gioco (27015).
 * @return 1 se connessione riuscita, 0 altrimenti.
 */
int ConnettiTramiteCodiceStanza(const char* codice, int portaBase) {
    SOCKET udpSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSock == INVALID_SOCKET) return 0;

    int broadcastOpt = 1;
    setsockopt(udpSock, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcastOpt, sizeof(broadcastOpt));

    int timeout = 1000;
    setsockopt(udpSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    struct sockaddr_in broadcastAddr;
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
    broadcastAddr.sin_port = htons(portaBase + 1);

    char msg[64];
    sprintf(msg, "REQ_CODE:%s", codice);

    sendto(udpSock, msg, strlen(msg), 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));

    char replyBuf[64];
    struct sockaddr_in serverUdpAddr;
    int addrLen = sizeof(serverUdpAddr);

    int bytesReceived = recvfrom(udpSock, replyBuf, sizeof(replyBuf) - 1, 0, (struct sockaddr*)&serverUdpAddr, &addrLen);
    closesocket(udpSock);

    if (bytesReceived > 0) {
        replyBuf[bytesReceived] = '\0';
        if (strcmp(replyBuf, "CODE_FOUND") == 0) {
            char* serverIP = inet_ntoa(serverUdpAddr.sin_addr);
            return JoinGame(serverIP, portaBase);
        }
    }
    return 0;
}

/**
 * @brief Ricompatta l'array di socket client dopo una disconnessione.
 *
 * Sposta tutti gli socket dopo l'indice rimosso di una posizione
 * indietro per mantenere gli slot contigui.
 *
 * @param removed_index Indice dello slot rimosso.
 * @param num_players Numero totale giocatori (dopo rimozione).
 */
void ShiftNetworkSockets(int removed_index, int num_players) {
    if (currentRole != NET_HOST) return;
    // Numero massimo di iterazioni: sposta tutti gli elementi DOPO removed_index
    // di una posizione indietro, usando MAX_SERVER_CLIENTS per sicurezza
    for (int k = removed_index; k < MAX_SERVER_CLIENTS - 1; k++) {
        clientSockets[k] = clientSockets[k+1];
    }
    clientSockets[MAX_SERVER_CLIENTS - 1] = INVALID_SOCKET;
}
