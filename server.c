#include "mongoose.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif

static const char *s_listen_on = "http://0.0.0.0:8888";

struct user_info {
    struct mg_connection *conn;
    char name[40];
    char room[20];
    struct user_info *next;
};

struct user_info *users = NULL;

// Conta quanti utenti sono rimasti in una specifica stanza
int conta_utenti_stanza(const char *room) {
    int count = 0;
    for (struct user_info *u = users; u != NULL; u = u->next) {
        if (strcmp(u->room, room) == 0) count++;
    }
    return count;
}

void print_server_info() {
    char buffer[100] = "Impossibile rilevare";
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock >= 0) {
        struct sockaddr_in serv;
        memset(&serv, 0, sizeof(serv));
        serv.sin_family = AF_INET;
        serv.sin_addr.s_addr = inet_addr("8.8.8.8");
        serv.sin_port = htons(53);
        if (connect(sock, (const struct sockaddr*)&serv, sizeof(serv)) == 0) {
            struct sockaddr_in name;
            socklen_t namelen = sizeof(name);
            getsockname(sock, (struct sockaddr*)&name, &namelen);
            inet_ntop(AF_INET, &name.sin_addr, buffer, 100);
        }
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }
    printf("\n==========================================\n");
    printf("      SERVER CHAT ELITE - ATTIVO\n");
    printf("==========================================\n");
    printf("[LOCAL]    http://localhost:8888\n");
    printf("[NETWORK]  http://%s:8888\n", buffer);
    printf("==========================================\n\n");
}

void add_user(struct mg_connection *c) {
    struct user_info *u = calloc(1, sizeof(*u));
    u->conn = c;
    u->next = users;
    users = u;
}

struct user_info *get_user(struct mg_connection *c) {
    for (struct user_info *u = users; u != NULL; u = u->next) {
        if (u->conn == c) return u;
    }
    return NULL;
}

void remove_user(struct mg_connection *c) {
    struct user_info **up = &users;
    while (*up) {
        if ((*up)->conn == c) {
            struct user_info *tmp = *up;
            *up = tmp->next;
            free(tmp);
            return;
        }
        up = &((*up)->next);
    }
}

void broadcast_to_room(struct mg_mgr *mgr, const char *room, const char *msg) {
    for (struct user_info *u = users; u != NULL; u = u->next) {
        if (strcmp(u->room, room) == 0) {
            mg_ws_send(u->conn, msg, strlen(msg), WEBSOCKET_OP_TEXT);
        }
    }
}

void broadcast_user_list(struct mg_mgr *mgr, const char *room) {
    char list_msg[1024] = "LIST:";
    for (struct user_info *u = users; u != NULL; u = u->next) {
        if (u->name[0] != '\0' && strcmp(u->room, room) == 0) {
            strcat(list_msg, u->name);
            strcat(list_msg, ",");
        }
    }
    broadcast_to_room(mgr, room, list_msg);
}

void invia_cronologia(struct mg_connection *c, const char *room) {
    char filename[64];
    snprintf(filename, sizeof(filename), "storia_%s.txt", room);
    FILE *f = fopen(filename, "r");
    if (f) {
        char riga[2048];
        while (fgets(riga, sizeof(riga), f)) {
            riga[strcspn(riga, "\n")] = 0;
            mg_ws_send(c, riga, strlen(riga), WEBSOCKET_OP_TEXT);
        }
        fclose(f);
    }
}

static void fn(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_OPEN) {
        add_user(c);
    } else if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        if (mg_match(hm->uri, mg_str("/ws"), NULL)) {
            mg_ws_upgrade(c, hm, NULL);
        } else {
            struct mg_http_serve_opts opts = {.root_dir = "."};
            mg_http_serve_dir(c, hm, &opts);
        }
    } else if (ev == MG_EV_WS_MSG) {
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        struct user_info *ui = get_user(c);
        if (!ui) return;

        if (strncmp(wm->data.buf, "JOIN:", 5) == 0) {
            char tmp[128];
            snprintf(tmp, sizeof(tmp), "%.*s", (int)wm->data.len - 5, wm->data.buf + 5);
            char *room = strtok(tmp, ":");
            char *name = strtok(NULL, ":");
            if (room && name) {
                strncpy(ui->room, room, sizeof(ui->room) - 1);
                strncpy(ui->name, name, sizeof(ui->name) - 1);
                
                // Messaggio Admin Join
                char admin_msg[128];
                snprintf(admin_msg, sizeof(admin_msg), "!%s è entrato nella stanza.", ui->name);                
                printf("[ROOM %s] %s si è collegato.\n", ui->room, ui->name);
                invia_cronologia(c, ui->room);
                broadcast_to_room(c->mgr, ui->room, admin_msg);
                broadcast_user_list(c->mgr, ui->room);
            }
            return;
        }

        if (ui->room[0] == '\0') return;

        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[15];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

        char formatted_msg[2048];
        int msg_len = snprintf(formatted_msg, sizeof(formatted_msg), "[%s] %.*s", 
                               time_str, (int)wm->data.len, wm->data.buf);

        printf("[%s] %s\n", ui->room, formatted_msg);
        
        char filename[64];
        snprintf(filename, sizeof(filename), "storia_%s.txt", ui->room);
        FILE *f = fopen(filename, "a");
        if (f) { fprintf(f, "%s\n", formatted_msg); fclose(f); }
        
        broadcast_to_room(c->mgr, ui->room, formatted_msg);

    } else if (ev == MG_EV_CLOSE) {
        struct user_info *ui = get_user(c);
        if (ui && ui->name[0] != '\0') {
            char room_temp[20];
            char name_temp[40];
            strcpy(room_temp, ui->room);
            strcpy(name_temp, ui->name);

            printf("[INFO] %s disconnesso.\n", name_temp);
            remove_user(c);

            // Messaggio Admin Exit
            char admin_msg[128];
            snprintf(admin_msg, sizeof(admin_msg), "!%s è uscito dalla stanza.", name_temp);
            broadcast_to_room(c->mgr, room_temp, admin_msg);
            broadcast_user_list(c->mgr, room_temp);

            // Se la stanza è vuota, cancella il file storia
            if (conta_utenti_stanza(room_temp) == 0) {
                char filename[64];
                snprintf(filename, sizeof(filename), "storia_%s.txt", room_temp);
                remove(filename); 
                printf("[CLEANUP] Stanza %s vuota, cronologia eliminata.\n", room_temp);
            }
        } else {
            remove_user(c);
        }
    }
}

int main() {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_log_set(MG_LL_NONE);
    print_server_info();
    mg_http_listen(&mgr, s_listen_on, fn, NULL);
    for (;;) mg_mgr_poll(&mgr, 1000);
    mg_mgr_free(&mgr);
    return 0;
}