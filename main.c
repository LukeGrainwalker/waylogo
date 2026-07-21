#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/mman.h>

#include <wayland-client.h>
#include "xdg-shell-protocol.h"

//struct wl_compositor *compositor;
//struct wl_shm *shm;
//struct xdg_wm_base *shell;

void report(char* msg){
	printf("%s\n", msg);
	fflush(stdin);
}

struct window_state {
	/*Globals*/
	struct wl_display *disp;
	struct wl_registry *reg;
	struct wl_compositor *comp;
	struct wl_shm *shm;
	struct xdg_wm_base *shell;
	/*Objects*/
	struct wl_surface *wlsurf;
	struct xdg_surface *xdgsurf;
	struct xdg_toplevel *toplevel;
	/*Settings*/
	int32_t width;
	int32_t height;
};


//buffer listener
void buffer_release(void *data, struct wl_buffer *wl_buffer){
	wl_buffer_destroy(wl_buffer);
	report("buffer release");
}

static const struct wl_buffer_listener buffer_listener = {
	.release = buffer_release,
};

void configure_surface(struct window_state *state, int32_t width, int32_t height){
	//
	if (width <= 0 || height <= 0) {
		width = 200;
		height = 200;
	}

	int stride = width * 4;
	int size = stride*height;

	// open an anonymous file (that only we and the compositor will know about) and write some zero bytes to it
	int fd = memfd_create("buffer", 0);
	ftruncate(fd, size);

	// map the file into memory
	//unsigned char *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	// turn it into a shared memory pool (tell the compositor, that it should create a pool in fd)
	struct wl_shm_pool *pool = wl_shm_create_pool(state->shm, fd, size);

	//allocate the buffer in that pool (tell the compositor to memory share and how to interpret the data)
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);

	wl_shm_pool_destroy(pool);
	close(fd);

	wl_buffer_add_listener(buffer, &buffer_listener, NULL);

	//attach the buffer to the surface
	wl_surface_attach(state->wlsurf, buffer, 0, 0);

	// tell the compositor to render (or just take for now) the buffer contents
	wl_surface_commit(state->wlsurf);
	xdg_surface_set_window_geometry(state->xdgsurf, 0, 0, width, height);
}

// registry global listener
void registry_global_handler(void *data , struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version){
	struct window_state *state = data;
	if (strcmp(interface, "wl_compositor") == 0){
		state->comp = wl_registry_bind(registry, name, &wl_compositor_interface, 3);
	} else if (strcmp(interface, "wl_shm") == 0){
		state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, "xdg_wm_base") == 0){
		state->shell = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
	}
}

void registry_global_remove_handler(void *data, struct wl_registry *registry, uint32_t name) {}

const struct wl_registry_listener registry_listener = {
	.global = registry_global_handler,
	.global_remove = registry_global_remove_handler,
};

// xdg configure listener
void config_handler(void* data, struct xdg_surface *xdg_surf, uint32_t serial){
	xdg_surface_ack_configure(xdg_surf, serial);
	struct window_state *state = data;
	configure_surface(state, state->width, state->height);
	//wl_surface_commit(((struct window_state *)data)->wlsurf);
	report("config xdg surface");
}

struct xdg_surface_listener config_listener = {
	.configure = config_handler,
};

// xdg wm base listener (respond to ping)

void ping_handler(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial){
	xdg_wm_base_pong(xdg_wm_base, serial);
	report("ping");
}

struct xdg_wm_base_listener ping_listener = {
	.ping = ping_handler,
};

// xdg toplevel listener (configuration and window events...)
void toplevel_conf(void* data, struct xdg_toplevel *xgd_toplevel, int32_t width, int32_t height, struct wl_array *states){
	//configure_surface((struct window_state *)data, width, height);
	struct window_state* state = data;
	state->width = width;
	state->height = height;
	report("toplevel conf");
}

void toplevel_bounds(void* data, struct xdg_toplevel *xgd_toplevel, int32_t width, int32_t height){
	//configure_surface((struct window_state *)data, width, height);
	struct window_state* state = data;
	if (state->width > width) {
		state->width = width;
	}
	if (state->height > height) {
		state->height = height;
	}
	report("toplevel bounds");
}

void toplevel_close(void* data, struct xdg_toplevel *xdg_toplevel) {
	// TODO: make a better exit!!!
	report("close");
	exit(0);
}

void toplevel_capabil(void* data, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities){
	report("capabilities");
}

struct xdg_toplevel_listener event_listener = {
	.configure = toplevel_conf,
	.configure_bounds = toplevel_bounds,
	.close = toplevel_close,
	.wm_capabilities = toplevel_capabil,
};

int main(int argc, char** argv){
	struct window_state state = {0};
	// init connection, get the display
	state.disp = wl_display_connect(NULL);
	// retrive the registry
	state.reg = wl_display_get_registry(state.disp);
	// add the registry listener
	wl_registry_add_listener(state.reg, &registry_listener, &state);

	// wait for the "initial" set of globals appear ( wait for the events ..)
	wl_display_roundtrip(state.disp);

	// add the ping listener
	xdg_wm_base_add_listener(state.shell, &ping_listener, NULL);

	//create a surface and assign the toplevel role
	state.wlsurf = wl_compositor_create_surface(state.comp);
	state.xdgsurf = xdg_wm_base_get_xdg_surface(state.shell, state.wlsurf);
	xdg_surface_add_listener(state.xdgsurf, &config_listener, &state);
	state.toplevel = xdg_surface_get_toplevel(state.xdgsurf);
	xdg_toplevel_add_listener(state.toplevel, &event_listener, &state);

	// xdg surface configure sequens
	wl_surface_commit(state.wlsurf);
	wl_display_roundtrip(state.disp);
	


	//subject to change (dynamic?)
	//int width = 200;
	//int height = 200;
	//int stride = width*4;
	//int size = stride * height; // bytes
	
	// open an anonymous file (that only we and the compositor will know about) and write some zero bytes to it
	//int fd = memfd_create("buffer", 0);
	//ftruncate(fd, size);

	// map the file into memory
	//unsigned char *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	// turn it into a shared memory pool (tell the compositor, that it should create a pool in fd)
	//struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);

	//allocate the buffer in that pool (tell the compositor to memory share and how to interpret the data)
	//struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);

	//attach the buffer to the surface
	//wl_surface_attach(surface, buffer, 0, 0);

	// tell the compositor to render (or just take for now) the buffer contents
	//wl_surface_commit(surface);

	while (1) {
		wl_display_dispatch(state.disp);
	}

	return 0;
}
