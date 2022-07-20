#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define OPTIONS              "t:l:"
#define DEFAULT_THREAD_COUNT 4

#include "list.h"
#include "queue.h"
#include "request.h"
#include "util.h"

//
// Multithreaded HTTP Server Main File
// Thread Pool and handling request.
//

pthread_cond_t variable = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t joinlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t varbound = PTHREAD_COND_INITIALIZER;

static FILE *logfile;
Queue *q;
bool quite = false;
pthread_t *threadsize;
int threadnum;

//
// Converts a string to an 16 bits unsigned integer.
// Returns 0 if the string is malformed or out of the range.
//
static size_t strtouint16(char number[]) {
    char *last;
    long num = strtol(number, &last, 10);
    if (num <= 0 || num > UINT16_MAX || *last != '\0') {
        return 0;
    }
    return num;
}

//
// Creates a socket for listening for connections.
// Closes the program and prints an error message on error.
//
static int create_listen_socket(uint16_t port) {
    struct sockaddr_in addr;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        fclose(logfile);
        err(EXIT_FAILURE, "socket error");
    }
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
        fclose(logfile);
        err(EXIT_FAILURE, "bind error");
    }
    if (listen(listenfd, 128) < 0) {
        fclose(logfile);
        err(EXIT_FAILURE, "listen error");
    }
    return listenfd;
}

//
// Process request from the user
//
static void handle_connection(int connfd) {
    char method[8] = { 0 }, URI[16] = { 0 }, str[2048] = { 0 }, buf[2048] = { 0 };
    char *body, *value, *tok = NULL;
    int h1 = 0, h2 = 0, total = 0, readcall;
    List S = newList();
    readcall = read_bytes(connfd, buf, sizeof(buf)); // read the status line

    memcpy(str, buf, 2048); // copy buf to str so it can be strtok_r
    value = strstr(str, "\r\n\r\n");

    if (value == NULL) {
        char *err = "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n";
        send(connfd, err, strlen(err), 0);
        return;
    }

    read_carriage(str, readcall, &total);

    sscanf(str,
        "%"
        "[a-zA-Z] /%"
        "[a-zA-Z0-9_.] HTTP/%1d.%1d",
        method, URI, &h1, &h2);

    if (h1 != 1 && h2 != 1) { // version check for HTTP
        char *err = "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n";
        send(connfd, err, strlen(err), 0);
        return;
    }

    tok = strtok_r(str, "\r\n", &body); // seperate the header field
    while (tok != NULL) {
        tok = strtok_r(NULL, "\r\n", &body);
        if (tok == NULL) {
            break;
        }
        append(S, tok);
    }

    int cl = 0, rq = 0;
    char *val = ":\t";
    for (moveFront(S); cursor(S) >= 0; moveNext(S)) { // find the contentlength
        tok = strtok_r(get(S), val, &body);
        if ((strcasecmp("PUT", method) == 0) || (strcasecmp("APPEND", method) == 0)) {
            if (strcmp(tok, "Content-Length") == 0) {
                cl = atoi(body);

            } else if (strcmp(tok, "Request-Id") == 0) {
                rq = atoi(body);
            }
        } else if (strcasecmp("GET", method) == 0) {
            if (strcmp(tok, "Request-Id") == 0) {
                rq = atoi(body);
            }
        }
    }

    freeList(&S);
    read_request(logfile, connfd, method, URI, buf, cl, rq, total, readcall);
    //function handles request for GET, PUT, APPEND
    return;
}

//
// Thread Pool for multiple request
//
static void *threadrun(void *args) {
    (void) args;
    for (;;) {
        bool check = false;
        pthread_mutex_lock(&mutexlock);
        if (quite) {
            pthread_mutex_unlock(&mutexlock);
            break;
        }
        while (queue_empty(q)) {
            pthread_cond_wait(&variable, &mutexlock);
        }

        int queuede = 0; // grab front element before dequeue
        dequeue(q, &queuede);
        check = true;
        pthread_cond_signal(&varbound);
        pthread_mutex_unlock(&mutexlock);

        if (check) {
            handle_connection(queuede); // run the function
            close(queuede); // close the port
        }
    }
    return NULL;
}

//
// Create Thread Pool with PTHREAD API
//
static void create_thread_pool(void) {
    for (int i = 0; i < threadnum; i++) {
        if (pthread_create(&threadsize[i], NULL, &threadrun, NULL) != 0) {
            err(1, "pthread_create() failed");
        }
    }
    return;
}

//
// Signal handler
//
static void sigterm_handler(int sig) {
    if (sig == SIGTERM) {
        free(threadsize);
        fclose(logfile);
        freeQueue(&q);
        exit(EXIT_SUCCESS);
    } else if (sig == SIGINT) {
        free(threadsize);
        fclose(logfile);
        freeQueue(&q);
        exit(EXIT_SUCCESS);
    }
}

//
// Error Message Handling
//
static void usage(char *exec) {
    fprintf(stderr, "usage: %s [-t threads] [-l logfile] <port>\n", exec);
}

int main(int argc, char *argv[]) {
    int opt = 0;
    int threads = DEFAULT_THREAD_COUNT;
    logfile = stderr;
    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        case 't':
            threads = strtol(optarg, NULL, 10);
            if (threads <= 0) {
                errx(EXIT_FAILURE, "bad number of threads");
            }
            break;
        case 'l':
            logfile = fopen(optarg, "w");
            if (!logfile) {
                errx(EXIT_FAILURE, "bad logfile");
            }
            break;
        default: usage(argv[0]); return EXIT_FAILURE;
        }
    }
    if (optind >= argc) {
        warnx("wrong number of arguments");
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    uint16_t port = strtouint16(argv[optind]);
    if (port == 0) {
        fclose(logfile);
        errx(EXIT_FAILURE, "bad port number: %s", argv[optind]);
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    int listenfd = create_listen_socket(port);
    q = queue_create();
    threadnum = threads;
    threadsize = calloc(threads, sizeof(threadsize));
    create_thread_pool();
    for (;;) {
        int connfd = accept(listenfd, NULL, NULL);
        if (connfd < 0) {
            warn("accept error");
            continue;
        }
        pthread_mutex_lock(&mutexlock);
        while (queue_size(q) + 1 >= 2048) {
            pthread_cond_wait(&varbound, &mutexlock);
        }
        enqueue(q, connfd);
        pthread_cond_signal(&variable);
        pthread_mutex_unlock(&mutexlock);
    }
    return EXIT_SUCCESS;
}
