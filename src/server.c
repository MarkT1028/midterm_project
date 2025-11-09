#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "utils.h"

#ifndef SERVER_PORT
#define SERVER_PORT 12345
#endif

static int listen_fd = -1;

void handle_sigchld(int sig) {
    (void)sig;
    // non-blocking wait to reap all children
    while (1) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0) break;
        log_msg(LOG_INFO, "Reaped child pid=%d status=%d", pid, status);
    }
}

void setup_signals(void) {
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        log_msg(LOG_WARN, "sigaction(SIGCHLD) failed: %s", strerror(errno));
    }
}

void child_process(int client_fd, struct sockaddr_in client_addr) {
    char addrstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, addrstr, sizeof(addrstr));
    log_msg(LOG_INFO, "Child %d handling client %s:%d", getpid(), addrstr, ntohs(client_addr.sin_port));

    // set recv timeout (robustness)
    struct timeval tv = {.tv_sec = 30, .tv_usec = 0};
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (1) {
        void *buf = NULL;
        uint32_t len = 0;
        int rc = recv_msg(client_fd, &buf, &len);
        if (rc == 1) { log_msg(LOG_INFO, "Client closed connection"); break; }
        if (rc < 0) {
            log_msg(LOG_WARN, "recv_msg error: %s", strerror(errno));
            break;
        }
        log_msg(LOG_DEBUG, "Received command: %s", (char*)buf);
        if (strncmp((char*)buf, "SYSINFO", 7) == 0) {
            char *info = get_sysinfo();
            send_msg(client_fd, info, (uint32_t)strlen(info));
            free(info);
        } else if (strncmp((char*)buf, "ECHO ", 5) == 0) {
            send_msg(client_fd, (char*)buf+5, len>5?(len-5):0);
        } else if (strncmp((char*)buf, "QUIT", 4) == 0) {
            free(buf);
            break;
        } else {
            const char *unk = "UNKNOWN COMMAND";
            send_msg(client_fd, unk, (uint32_t)strlen(unk));
        }
        free(buf);
    }

    close(client_fd);
    log_msg(LOG_INFO, "Child %d exiting", getpid());
    _exit(0); // ensure child exits
}

int main(int argc, char **argv) {
    // runtime debug control via arg or env
    log_level_t lvl = LOG_INFO;
    for (int i=1;i<argc;i++){
        if (strcmp(argv[i], "-v")==0 || strcmp(argv[i],"--debug")==0) lvl = LOG_DEBUG;
    }
    char *env = getenv("LOG_LEVEL");
    if (env) {
        if (strcmp(env,"DEBUG")==0) lvl = LOG_DEBUG;
        else if (strcmp(env,"INFO")==0) lvl = LOG_INFO;
        else if (strcmp(env,"WARN")==0) lvl = LOG_WARN;
    }
    log_init(lvl);
#ifdef COMPILE_DEBUG
    log_msg(LOG_DEBUG, "Compiled with COMPILE_DEBUG");
#endif

    setup_signals();

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); exit(1); }
    int opt=1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = htonl(INADDR_ANY);
    srv.sin_port = htons(SERVER_PORT);

    if (bind(listen_fd, (struct sockaddr*)&srv, sizeof(srv)) < 0) { perror("bind"); exit(1); }
    if (listen(listen_fd, 128) < 0) { perror("listen"); exit(1); }

    log_msg(LOG_INFO, "Server listening on port %d", SERVER_PORT);

    while (1) {
        struct sockaddr_in cli;
        socklen_t cli_len = sizeof(cli);
        int cfd = accept(listen_fd, (struct sockaddr*)&cli, &cli_len);
        if (cfd < 0) {
            if (errno == EINTR) continue;
            log_msg(LOG_ERROR, "accept failed: %s", strerror(errno));
            continue;
        }
        pid_t pid = fork();
        if (pid < 0) {
            log_msg(LOG_ERROR, "fork failed: %s", strerror(errno));
            close(cfd);
            continue;
        } else if (pid == 0) {
            // child
            close(listen_fd);
            child_process(cfd, cli);
            // never reach
        } else {
            // parent
            log_msg(LOG_DEBUG, "Forked child %d", pid);
            close(cfd); // parent closes the socket; child handles it
        }
    }

    return 0;
}
