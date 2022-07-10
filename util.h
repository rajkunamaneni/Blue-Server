#pragma once

#include <stdbool.h>
#include <stdio.h>

int read_bytes(int infile, char buf[], int nbytes);

int write_bytes(int outfile, char buf[], int nbytes);

bool read_carriage(char *buf, int nbytes, int *total);
