#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include <config.h>
#include "util.h"

int quiet = 0;
/**
 * report a message to the console
 */
void vreport_prio(int priority, char* fmt, va_list va){
	va_list va2;

	if (!quiet){
		va_copy(va2, va);
		vfprintf(stderr, fmt, va2);
		fprintf(stderr, "\n");
		va_end(va2);
	}
	vsyslog(priority, fmt, va);
}
void report(char* fmt, ...){
	va_list va;
	va_start(va);
	vreport_prio(LOG_NOTICE, fmt, va);
	va_end(va);
}

void ireport(char* fmt, ...){
	va_list va;
	va_start(va);
	vreport_prio(LOG_INFO, fmt, va);
	va_end(va);
}

void wreport(char* msg){
	report_prio(LOG_DEBUG, "wayland signal: %s", msg);
}

void report_prio(int priority, char* fmt, ...){
	va_list va;
	va_start(va);
	vreport_prio(priority, fmt, va);
	va_end(va);
}


void report_start() {
	int opt = LOG_ODELAY;
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
		if (opt == CONFIG_QUIET) quiet = 1;
		if (opt == CONFIG_HELP) phelp(argc, argv);
	} while (opt >= 0);
	if (optind < argc || opt == '?') {
		phelp(argc, argv);
	}
	return conf;
}
