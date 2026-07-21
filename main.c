#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/mman.h>

#include <wayland-client.h>
#include "xdg-shell-protocol.h"

struct wl_compositor *compositor;
struct wl_shm *shm;
struct xdg_wm_base *shell;

// registry global listener
void registry_global_handler(void *data , struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version){
	if (strcmp(interface, "wl_compositor") == 0){
		compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 3);
	} else if (strcmp(interface, "wl_shm") == 0){
		shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, "xdg_wm_base") == 0){
		shell = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
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
}

struct xdg_surface_listener config_listener = {
	.configure = config_handler,
};

int main(int argc, char** argv){
	// init connection, get the display
	struct wl_display *display = wl_display_connect(NULL);
	// retrive the registry
	struct wl_registry *registry = wl_display_get_registry(display);
	// add our listener
	wl_registry_add_listener(registry, &registry_listener, NULL);

	// wait for the "initial" set of globals appear ( wait for the events ..)
	wl_display_roundtrip(display);

	//create a surface and assign the toplevel role
	struct wl_surface *surface = wl_compositor_create_surface(compositor);
	struct xdg_surface* shell_surface = xdg_wm_base_get_xdg_surface(shell, surface);
	xdg_surface_add_listener(shell_surface, &config_listener, NULL);
	xdg_surface_get_toplevel(shell_surface);

	// xdg surface configure sequens
	wl_surface_commit(surface);
	wl_display_roundtrip(display);

	//subject to change (dynamic?)
	int width = 200;
	int height = 200;
	int stride = width*4;
	int size = stride * height; // bytes
	
	// open an anonymous file (that only we and the compositor will know about) and write some zero bytes to it
	int fd = memfd_create("buffer", 0);
	ftruncate(fd, size);

	// map the file into memory
	unsigned char *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	// turn it into a shares memory pool (tell the compositor, that it should create a pool in fd)
	struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);

	//allocate the buffer in that pool (tell the compositor to memory share and how to interpret the data)
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);

	//attach the buffer to the surface
	wl_surface_attach(surface, buffer, 0, 0);

	// tell the compositor to render (or just take for now) the buffer contents
	wl_surface_commit(surface);

	while (1) {
		wl_display_dispatch(display);
	}

	return 0;
}
