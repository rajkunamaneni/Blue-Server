#pragma once

#include <stdbool.h>
#include <stdio.h>

void read_request(FILE *out, int socket, char *method, char *filename, char *value, int cl, int rq,
    int total, int readv);
