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
int lmask = 0;
/**
 * report a message to the console
 */
void vreport_prio(int priority, char* fmt, va_list va){
	va_list va2;

	if (!quiet && LOG_MASK(priority) & lmask){
		va_copy(va2, va);
		vfprintf(stderr, fmt, va2);
		fprintf(stderr, "\n");
		va_end(va2);
	}
	vsyslog(priority, fmt, va);
}
void report(char* fmt, ...){
	va_list va;
	va_start(va, fmt);
	vreport_prio(LOG_NOTICE, fmt, va);
	va_end(va);
}

void ireport(char* fmt, ...){
	va_list va;
	va_start(va, fmt);
	vreport_prio(LOG_INFO, fmt, va);
	va_end(va);
}

void ereport(char* fmt, ...){
	va_list va;
	va_start(va, fmt);
	vreport_prio(LOG_ERR, fmt, va);
	va_end(va);
}

void wreport(char* msg){
	report_prio(LOG_DEBUG, "wayland signal: %s", msg);
}

void report_prio(int priority, char* fmt, ...){
	va_list va;
	va_start(va, fmt);
	vreport_prio(priority, fmt, va);
	va_end(va);
}


void report_start(int log_level) {
	int opt = LOG_ODELAY;
	openlog(NAME_STR, opt, LOG_USER);
	lmask = LOG_UPTO(log_level);
	setlogmask(lmask);
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
	{"debug", 0, NULL, CONFIG_DEBUG},
	{"version", 0, NULL, CONFIG_VERSION},
	{"floating", 0, NULL, CONFIG_FLOATING},
	{NULL, 0, NULL, 0}
};

char *descriptions[] = {
	"do nothing (xlogo option)",
	"do nothing (xlogo option)",
	"make the background transparent (xlogo option)",
	"do not print log messages to stderr",
	"print a help message and exit",
	"log debug messages",
	"print the version string and exit",
	"force the window to be floating"
};

/**
 * print help and exit...
 */
void phelp(int argc, char** argv){
	printf("Usage: %s", argv[0]);
	for (int i = 0; options[i].name; i++){
		printf(" [-%s]", options[i].name);
	}
	printf("\n\nShow the Wayland protocol logo\n\nOptions:\n");
	for (int i = 0; options[i].name; i++){
		printf("\t-%s\t%s\n", options[i].name, descriptions[i]);
	}
	printf("\n");
	exit(0);
}

void pversion(){
	printf("%s %s\n", NAME_STR, VERSION_STR);
	exit(0);
}

/**
 * get the command line options and save them in a waylogo_config structure
 * or call phelp
 */
struct waylogo_config *waylogo_configure(int argc, char** argv) {
	int opt;
	struct waylogo_config *conf = malloc(sizeof(struct waylogo_config));
	if (conf == NULL){
		ereport("malloc: %m");
		exit(1);
	}
	memset(conf, 0, sizeof(struct waylogo_config));
	//set defaults:
	conf->log_level = LOG_NOTICE;
	do{
		opt = getopt_long_only(argc, argv, "", options, NULL);
		//printf("%i, %c\n", opt, (char)opt);
		if (opt < 0 || opt == '?') break;
		conf->flags |= opt;
		switch (opt) {
			case CONFIG_QUIET:
				quiet = 1;
				break;
			case CONFIG_HELP:
				phelp(argc, argv);
			case CONFIG_VERSION:
				pversion();
			case CONFIG_DEBUG:
				conf->log_level = LOG_DEBUG;
		}
	} while (opt >= 0);
	if (optind < argc || opt == '?') {
		phelp(argc, argv);
	}
	report_start(conf->log_level);
	return conf;
}
