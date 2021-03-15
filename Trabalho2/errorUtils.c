#include "errorUtils.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>



const int MAX_ERROR_MESSAGE = 4096;



static void DisplayError(int errorn, const char *fmtStr, va_list args) {
    char msgBuffer[MAX_ERROR_MESSAGE];

    vsprintf(msgBuffer, fmtStr, args);
	sprintf(msgBuffer + strlen(msgBuffer), ": (%d) %s\n\n", errorn, strerror(errorn));

	fputs(msgBuffer, stderr);
}


	
/**
 * Erro de Sistema
 * 
 * Imprime a mensagem seguida do código de sistema e mensagem descritiva desse código.
 * A função termina a aplicação.
 */
void FatalErrorSystem(const char *fmtStr, ...) {
    va_list args;

    va_start(args, fmtStr);
    DisplayError( errno, fmtStr, args);
    va_end( args );

	//terminar();
	exit(EXIT_FAILURE);
} /* end FatalErrorSystem */
