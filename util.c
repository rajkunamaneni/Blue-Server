#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

//
// Functions used throughout the program that are needed
// in other files.
//

//
// Loop low level sys call (read).
//
int read_bytes(int infile, char *buf, int nbytes) {
  int file_r = 0; // this is to check if the file is done reading
  int total = 0, checkbytes = 0;
  char *value;

  do {
    file_r = recv(infile, buf + checkbytes, nbytes - checkbytes, 0);
    checkbytes += file_r; // increment the value
    if (read_carriage(buf, file_r, &total)) {
      break;
    } else if ((value = strstr(buf, "\r\n\r\n")) != NULL) {
      break;
    }

  } while (file_r > 0 && checkbytes < nbytes);

  return checkbytes;
}

//
// Loop low level sys call (write).
//
int write_bytes(int outfile, char *buf, int nbytes) {
  int file_w = 0;
  int checkbytes = 0;

  do {
    file_w = write(outfile, buf + checkbytes, nbytes - checkbytes);
    checkbytes += file_w; // increment the value write was done
  } while (file_w > 0 && checkbytes < nbytes); // check if unsuccessful write

  return file_w;
}

//
// Helper function to count the number of bytes until double
// carriage is found
//
bool read_carriage(char *buf, int nbytes, int *total) {
  for (int i = 0; i < nbytes; i++) {
    if (buf[i] == '\r') {
      if (buf[i + 1] == '\n') {
        if (buf[i + 2] == '\r') {
          if (buf[i + 3] == '\n') {
            *total = i + 4;
            return true;
          }
        }
      }
    }
  }
  return false;
}
