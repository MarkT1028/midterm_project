#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "utils.h"

#ifndef SERVER_PORT
#define SERVER_PORT 12345
#endif

int main(int argc, char **argv) {
    log_level_t lvl = LOG_INFO;
    for (int i=1;i<argc;i++){
        if (strcmp(argv[i], "-v")==0 || strcmp(argv[i],"--debug")==0) lvl = LOG_DEBUG;
    }
    char *env = getenv("LOG_LEVEL");
    if (env) {
        if (strcmp(env,"DEBUG")==0) lvl = LOG_DEBUG;
        else if (strcmp(env,"INFO")==0) lvl = LOG_INFO;
    }
    log_init(lvl);
#ifdef COMPILE_DEBUG
    log_msg(LOG_DEBUG, "client compiled with COMPILE_DEBUG");
#endif

    const char *server_ip = "127.0.0.1";
    int port = SERVER_PORT;
    if (argc >= 2) server_ip = argv[1];
    if (argc >= 3) port = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }
    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &srv.sin_addr);
    srv.sin_port = htons(port);

    if (connect(sock, (struct sockaddr*)&srv, sizeof(srv)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    log_msg(LOG_INFO, "Connected to %s:%d", server_ip, port);

    char cmd[256];
    while (1) {
        printf("Enter command (SYSINFO | ECHO <msg> | QUIT): ");
        if (!fgets(cmd, sizeof(cmd), stdin)) break;
        size_t len = strlen(cmd);
        if (len==0) continue;
        if (cmd[len-1]=='\n') cmd[len-1] = '\0';
        if (send_msg(sock, cmd, (uint32_t)strlen(cmd)) != 0) {
            log_msg(LOG_ERROR, "send_msg failed");
            break;
        }
        if (strncmp(cmd, "QUIT", 4) == 0) break;
        void *resp = NULL;
        uint32_t rlen = 0;
        int rc = recv_msg(sock, &resp, &rlen);
        if (rc == 1) { log_msg(LOG_WARN, "server closed connection"); break; }
        if (rc < 0) {
            log_msg(LOG_ERROR, "recv_msg error");
            break;
        }
        printf("Response (%u bytes):\n%s\n", rlen, (char*)resp);
        free(resp);
    }

    close(sock);
    return 0;
}
