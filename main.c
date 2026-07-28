#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <plutosvg.h>
#include <config.h>

#include "util.h"
#include "wayland.h"

/**
 * load a svg called name in path if it exists.
 */
plutosvg_document_t* get_svg(char* path, char* name){
	char* file_path = malloc(strlen(path) + strlen(name));
	strcpy(file_path, path);
	strcat(file_path, name);
	if (access(file_path, R_OK) == 0){
		// load svg
		plutosvg_document_t* svg = plutosvg_document_load_from_file(file_path, -1, -1);
		if (svg == NULL){
			report("Unable to load svg: %s", file_path);
		}
		return svg;
	}else{
		report("File %s does not exist or is not readable", file_path);
		free(file_path);
		return NULL;
	}
}

/**
 * search all possible paths for the logo, which are:
 *  - the build path (BUILD_PATH)
 *  - the installed data path (DATA_PATH)
 *  - the current working directory
 */
plutosvg_document_t *get_logo(){
	plutosvg_document_t *path = NULL;
	char *logo = "/wayland.svg";

	path = get_svg(DATA_PATH, logo);
	if (path == NULL) {
		path = get_svg(BUILD_PATH, logo);
		if (path == NULL) {
			path = get_svg(".", logo);
		}
	}
	return path;
}

int main(int argc, char** argv){
	struct window_state state = {0};
	window_state_init(&state);
	state.conf = waylogo_configure(argc, argv);
	state.svg = get_logo();
	//this should never return..
	way_launch(&state);
	return 0;
}
