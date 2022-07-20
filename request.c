#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "auditlog.h"
#include "request.h"
#include "util.h"

#define BLOCK 4096

//
// Function used for processing GET, PUT, and APPEND. This function
// will output the result to the socket. This function also deals with
// error handling.
//
void read_request(FILE *out, int socket, char *method, char *filename, char *value, int cl, int rq,
    int total, int readv) {
    char buf[BLOCK] = { 0 };
    int readcall, errorcode = 0;
    struct stat statbuf;

    if (strcasecmp("GET", method) == 0) { // GET REQUEST
        int infile = open(filename, O_WRONLY); // error handling only
        flock(infile, LOCK_SH);
        if ((infile == -1) && (errno == EISDIR)) {
            char *str = "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n";
            send(socket, str, strlen(str), 0);
            return;
        } else if ((infile == -1) && (errno == ENOENT)) { // check for invalid open
            char *str = "HTTP/1.1 404 File Not Found\r\nContent-Length: "
                        "15\r\n\r\nFile Not Found\n";
            errorcode = 404;
            if (errno == EISDIR) {
                str = "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n";
            }
            auditlog(out, method, filename, errorcode, rq);
            send(socket, str, strlen(str), 0);
            return;
        } else if ((infile == -1) && (errno == EACCES)) { // check if file is allowed for R_OK
            char *str = "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n";
            send(socket, str, strlen(str), 0);
            close(infile);
            return;
        } else { // GET the file and valid OK message
            infile = open(filename, O_RDONLY);
            char str[400];
            fstat(infile, &statbuf);
            sprintf(str, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", (statbuf.st_size));
            write(socket, str, strlen(str));
            errorcode = 200;
            auditlog(out, method, filename, errorcode, rq);
            while ((readcall = read(infile, buf, BLOCK)) > 0) {
                write_bytes(socket, buf, readcall);
            }
            flock(infile, LOCK_UN);
            close(infile);
            return;
        }
    }

    else if (strcasecmp("PUT", method) == 0) { // PUT REQUEST
        char *str;
        int infile, conlen = 0;

        infile = open(filename, O_WRONLY | O_TRUNC);
        flock(infile, LOCK_EX);
        str = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n";
        errorcode = 200;
        if ((infile == -1) && (errno == EISDIR || errno == EACCES)) {
            str = "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n";
            send(socket, str, strlen(str), 0);
            flock(infile, LOCK_UN);
            return;
        } else if (infile == -1) {
            infile = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
            str = "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n";
            errorcode = 201;
        }

        int writeout = 0;
        if (cl == 0) {
            close(infile);
            auditlog(out, method, filename, errorcode, rq);
            send(socket, str, strlen(str), 0);
            flock(infile, LOCK_UN);
            return;
        } else if (total > 0) {
            for (int c = 0; c < (readv - total); c++) {
                conlen++;
                if (conlen == cl) {
                    if ((readv - total) > conlen) {
                        writeout = conlen;
                    } else {
                        writeout = (readv - total);
                    }
                    write_bytes(infile, value + total, writeout);
                    close(infile);
                    auditlog(out, method, filename, errorcode, rq);
                    send(socket, str, strlen(str), 0);
                    flock(infile, LOCK_UN);
                    return;
                }
            }
            write_bytes(infile, value + total, readv - total);
        }

        if (conlen == cl) {
            send(socket, str, strlen(str), 0);
            close(infile);
            auditlog(out, method, filename, errorcode, rq);
            flock(infile, LOCK_UN);
            return;
        }

        while (((readcall = recv(socket, buf, BLOCK, 0)) > 0)) {
            for (int i = 0; i < readcall; i++) {
                conlen++;
                if (conlen == cl) {
                    write_bytes(infile, buf, readcall);
                    close(infile);
                    send(socket, str, strlen(str), 0);
                    auditlog(out, method, filename, errorcode, rq);
                    flock(infile, LOCK_UN);
                    return;
                }
            }
            if (cl > 0) {
                write_bytes(infile, buf, readcall);
            }
        }

        close(infile);
        send(socket, str, strlen(str), 0);
        auditlog(out, method, filename, errorcode, rq);
        flock(infile, LOCK_UN);
        return;

    } else if (strcasecmp("APPEND", method) == 0) { // APPEND REQUEST
        char *str;
        int conlen = 0;
        int checkf = access(filename, F_OK);
        int infile = open(filename, O_WRONLY | O_APPEND);
        flock(infile, LOCK_EX);
        if (infile == -1) {
            str = "HTTP/1.1 404 File Not Found\r\nContent-Length: 15\r\n\r\nFile Not "
                  "Found\n";
            errorcode = 404;
            if (errno == EISDIR) {
                str = "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n";
            }
            auditlog(out, method, filename, errorcode, rq);
            send(socket, str, strlen(str), 0);
            flock(infile, LOCK_UN);
            return;
        }
        str = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n";
        errorcode = 200;
        if ((checkf == 0) && (errno == EACCES)) {
            str = "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n";
            send(socket, str, strlen(str), 0);
            close(infile);
            flock(infile, LOCK_UN);
            return;
        }

        int writeout = 0;
        if (cl == 0) {
            close(infile);
            auditlog(out, method, filename, errorcode, rq);
            send(socket, str, strlen(str), 0);
            flock(infile, LOCK_UN);
            return;

        } else if (total > 0) {
            for (int c = 0; c < (readv - total); c++) {
                conlen++;
                if (conlen == cl) {
                    if ((readv - total) > conlen) {
                        writeout = conlen;
                    } else {
                        writeout = (readv - total);
                    }
                    write_bytes(infile, value + total, writeout);
                    close(infile);
                    auditlog(out, method, filename, errorcode, rq);
                    send(socket, str, strlen(str), 0);
                    flock(infile, LOCK_UN);
                    return;
                }
            }
            write_bytes(infile, value + total, readv - total);
        }

        if (conlen == cl) {
            send(socket, str, strlen(str), 0);
            close(infile);
            auditlog(out, method, filename, errorcode, rq);
            flock(infile, LOCK_UN);
            return;
        }

        while (((readcall = recv(socket, buf, BLOCK, 0)) > 0)) {
            for (int i = 0; i < readcall; i++) {
                conlen++;
                if (conlen == cl) {
                    write_bytes(infile, buf, readcall);
                    close(infile);
                    send(socket, str, strlen(str), 0);
                    auditlog(out, method, filename, errorcode, rq);
                    flock(infile, LOCK_UN);
                    return;
                }
            }
            if (cl > 0) {
                write_bytes(infile, buf, readcall);
            }
        }
        close(infile);
        send(socket, str, strlen(str), 0);
        auditlog(out, method, filename, errorcode, rq);
        flock(infile, LOCK_UN);
        return;

    } else { // INVALID REQUEST
        char *str = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot "
                    "Implemented\n";
        errorcode = 501;
        auditlog(out, method, filename, errorcode, rq);
        send(socket, str, strlen(str), 0);
        return;
    }
    return;
}
