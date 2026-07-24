#include <stdio.h>
#include "util.h"

/**
 * report a message to the console
 * TODO use better log systems
 */
void report(char* msg){
	printf("%s\n", msg);
	fflush(stdout);
}
