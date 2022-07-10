#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "auditlog.h"

pthread_mutex_t auditlock = PTHREAD_MUTEX_INITIALIZER;

//
// Logs with mutex locks to create no race conditions
// when writing to file
//
void auditlog(FILE *out, char *method, char *URI, int status, int rqid) {
  pthread_mutex_lock(&auditlock);
  fprintf(out, "%s,/%s,%d,%d\n", method, URI, status, rqid);
  fflush(out);
  pthread_mutex_unlock(&auditlock);
  return;
}
