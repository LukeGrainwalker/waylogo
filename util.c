#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <syslog.h>
#include <config.h>
#include "util.h"

int dolog = 1;
/**
 * report a message to the console
 * TODO use syslog for loging
 */
void report(char* msg){
	report_code(WLERR_GENERIC, msg);
}

void wreport(char* msg){
	report_code(WLERR_WDEBUG, msg);
}

void report_code(enum waylogo_error err, char* msg){
	switch (err){
		case WLERR_EXIST:
			syslog(LOG_NOTICE, "file %s does not exist or is not readable", msg);
			break;
		case WLERR_LOAD:
			syslog(LOG_NOTICE, "Unable to load: %s", msg);
			break;
		case WLERR_WDEBUG:
			syslog(LOG_DEBUG, "recieved wayland protocol signal %s", msg);
			break;
		case WLERR_GENERIC: default:
			syslog(LOG_INFO, "%s\n", msg);
	}
}

void report_start() {
	int opt = LOG_ODELAY;
	if (dolog) {
		opt |= LOG_CONS;
	}
	openlog(NAME_STR, opt, LOG_USER);
}

void report_end(){
	closelog();
}

struct option options[] = {
	// for backwards compatibility ( currently does nothing ...)
	{"render", 0, NULL, CONFIG_RENDER},
	{"sharp", 0, NULL, CONFIG_SHARP},
	{"shape", 0, NULL, CONFIG_SHAPE},
	// waylogo's new arguments
	{"quiet", 0, NULL, CONFIG_QUIET},
	{"help", 0, NULL, CONFIG_HELP},
	{NULL, 0, NULL, 0}
};

/**
 * print help and exit...
 */
void phelp(int argc, char** argv){
	printf("usage: %s", argv[0]);
	for (int i = 0; options[i].name; i++){
		printf(" [-%s]", options[i].name);
	}
	printf("\n");
	exit(0);
}

/**
 * get the command line options and save them in a waylogo_config structure
 * or call phelp
 */
struct waylogo_config *waylogo_configure(int argc, char** argv) {
	int opt;
	struct waylogo_config *conf = malloc(sizeof(struct waylogo_config));
	memset(conf, 0, sizeof(struct waylogo_config));
	do{
		opt = getopt_long_only(argc, argv, "", options, NULL);
		//printf("%i, %c\n", opt, (char)opt);
		if (opt < 0 || opt == '?') break;
		conf->flags |= opt;
		if (opt == CONFIG_QUIET) dolog = 0;
		if (opt == CONFIG_HELP) phelp(argc, argv);
	} while (opt >= 0);
	if (optind < argc || opt == '?') {
		phelp(argc, argv);
	}
	return conf;
}
