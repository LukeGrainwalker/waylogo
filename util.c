#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include "util.h"

int dolog = 1;
/**
 * report a message to the console
 * TODO use better log systems
 */
void report(char* msg){
	if (dolog) {
		printf("%s\n", msg);
		fflush(stdout);
	}
}

int opt;
struct option options[] = {
	// for backwards compatibility ( currently does nothing ...)
	{"render", 0, &opt, CONFIG_RENDER},
	{"sharp", 0, &opt, CONFIG_SHARP},
	{"shape", 0, &opt, CONFIG_SHAPE},
	// waylogo's new arguments
	{"quiet", 0, &opt, CONFIG_QUIET},
};

struct waylogo_config *waylogo_configure(int argc, char** argv) {
	struct waylogo_config *conf = malloc(sizeof(struct waylogo_config));
	while (getopt_long_only(argc, argv, "", options, NULL)){
		conf->flags |= opt;
		if (opt == CONFIG_QUIET) dolog = 0;
	}
	return conf;
}
