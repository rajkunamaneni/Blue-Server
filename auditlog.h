#pragma once

#include <stdbool.h>
#include <stdio.h>

//
// auditlog header file
//

void auditlog(FILE *out, char *method, char *URI, int status, int rqid);
