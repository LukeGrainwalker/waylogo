#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <syscall.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <linux/input-event-codes.h>

#include "draw.h"
#include "util.h"
#include "wayland.h"

#include <config.h>

/**
 * Handels the shared memory and calls the render handler.
 */
void render(struct window_state *state, int fd){
	int stride = state->width * 4;
	int size = stride * state->height;

	// map the file into memory
	uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		ereport("could not map the file into memory: %m");
		close_win(state);
	}
	if ((state->conf)->flags & CONFIG_SHAPE && state->floating){
		memset(data, 0, size);
	}
	// render the svg logo if it exists
	if (state->svg) {
		image(data, state->svg, state->width, state->height);
	} else {
		checker(data, state->width, state->height);
	}
	// unmap the file
	munmap(data, size);
}


//buffer listener
void buffer_release(void *data, struct wl_buffer *wl_buffer){
	wl_buffer_destroy(wl_buffer);
	//wreport("buffer release");
}

static const struct wl_buffer_listener buffer_listener = {
	.release = buffer_release,
};

void configure_surface(struct window_state *state){
	if (state->width <= 0 || state->height <= 0) {
		state->width = 200;
		state->height = 200;
		state->floating = 1;
	}

	int stride = state->width * 4;
	int size = stride * state->height;

	// open an anonymous file (that only we and the compositor will know about) and write some zero bytes to it
	//int fd = memfd_create("buffer", 0);
	// function memfd_create is not in any header file... (get's rid of warnings)
	int fd = syscall(SYS_memfd_create, "buffer", 0);
	if (fd == -1){
		ereport("can't open a anonymous file: %m");
		close_win(state);
	}
	ftruncate(fd, size);

	render(state, fd);
	
	// turn it into a shared memory pool (tell the compositor, that it should create a pool in fd)
	struct wl_shm_pool *pool = wl_shm_create_pool(state->shm, fd, size);
	if (pool == NULL){
		ereport("can't get a pool frome a file descriptor: %m");
		close_win(state);
	}

	// check for transparency
	int format = WL_SHM_FORMAT_XRGB8888;
	if ((state->conf)->flags & CONFIG_SHAPE && state->floating){
		format = WL_SHM_FORMAT_ARGB8888;
	}

	//allocate the buffer in that pool (tell the compositor to memory share and how to interpret the data)
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, state->width, state->height, stride, format);
	if (buffer == NULL){
		ereport("can't get a buffer from the shm pool: %m");
		close_win(state);
	}

	wl_shm_pool_destroy(pool);
	close(fd);

	wl_buffer_add_listener(buffer, &buffer_listener, NULL);

	//attach the buffer to the surface
	wl_surface_attach(state->wlsurf, buffer, 0, 0);

	// tell the compositor to render (or just take for now) the buffer contents
	wl_surface_commit(state->wlsurf);
	xdg_surface_set_window_geometry(state->xdgsurf, 0, 0, state->width, state->height);
	ireport("configuring the surface");
}

/** registry global listener
 * This listens for global signals on the registry
 * aka it listens for the global resources, that the compositor provides
 * and get's a reference (bind) to them.
 */
void registry_global_handler(void *data , struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version){
	struct window_state *state = data;
	// we don't care for errors here ... 
	if (strcmp(interface, "wl_compositor") == 0){
		state->comp = wl_registry_bind(registry, name, &wl_compositor_interface, 3);
	} else if (strcmp(interface, "wl_shm") == 0){
		state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, "xdg_wm_base") == 0){
		state->shell = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
	} else if (strcmp(interface, "wl_seat") == 0){
		state->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
	}
}

void registry_global_remove_handler(void *data, struct wl_registry *registry, uint32_t name) {}

const struct wl_registry_listener registry_listener = {
	.global = registry_global_handler,
	.global_remove = registry_global_remove_handler,
};


/**
 * This is the listener for the xdg surface configure signal
 * in theory to be protocol complitant it only needs to send xdg_surface_ack_configure
 * requests, but it is also expected to aply the accumulated changes, due to the toplevel
 * configuration signals
 */
void config_handler(void* data, struct xdg_surface *xdg_surf, uint32_t serial){
	xdg_surface_ack_configure(xdg_surf, serial);
	struct window_state *state = data;
	if (state->changed) configure_surface(state);
	state->changed = 0;
	if (state->tlstate == XDG_TOPLEVEL_STATE_MAXIMIZED) {
		xdg_toplevel_set_maximized(state->toplevel);
		state->current = state->tlstate;
		state->tlstate = 0;
	} else if (state->tlstate == XDG_TOPLEVEL_STATE_FULLSCREEN) {
		xdg_toplevel_set_fullscreen(state->toplevel, NULL);
		state->current = state->tlstate;
		state->tlstate = 0;
	}
	wl_surface_commit(state->wlsurf);
	wreport("xdg_surface::configure");
}

struct xdg_surface_listener config_listener = {
	.configure = config_handler,
};

// xdg wm base listener (respond to ping)
void ping_handler(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial){
	xdg_wm_base_pong(xdg_wm_base, serial);
	wreport("xdg_wm_base::ping");
}

struct xdg_wm_base_listener ping_listener = {
	.ping = ping_handler,
};

// xdg toplevel listener (configuration and window events...)
void toplevel_conf(void* data, struct xdg_toplevel *xgd_toplevel, int32_t width, int32_t height, struct wl_array *states){
	struct window_state* state = data;
	if (width > 0 
		&& height > 0 
		&& (state->width != width 
		|| state->height != height)) {
		state->width = width;
		state->height = height;
		state->changed = 1;
		state->floating = 0;
	}
	//because for what ever reason the compositor remenbers the size...
	if (width == 200 && height == 200) {
		state->floating = 1;
	}
	enum xdg_toplevel_state *s;
	wl_array_for_each(s, states) {
		if (state->current != *s 
			&& (*s == XDG_TOPLEVEL_STATE_MAXIMIZED 
			|| *s == XDG_TOPLEVEL_STATE_FULLSCREEN)) {
			state->tlstate = *s;
			state->changed = 1;
			state->floating = 0;
		}
	}
}

void toplevel_bounds(void* data, struct xdg_toplevel *xgd_toplevel, int32_t width, int32_t height){
	struct window_state* state = data;
	if (state->width > width) {
		state->width = width;
		state->changed = 1;
	}
	if (state->height > height) {
		state->height = height;
		state->changed = 1;
	}
}

void toplevel_close(void* data, struct xdg_toplevel *xdg_toplevel) {
	// TODO: make a better exit!!!
	wreport("xdg_toplevel::close");
	struct window_state* state = data;
	close_win(state);
}

void close_win(struct window_state *state){
	if (state == NULL) return;
	if (state->seat != NULL) {
		wl_seat_release(state->seat);
	}
	if (state->pointer != NULL) {
		wl_pointer_release(state->pointer);
	}
	if (state->ptr_state != NULL){
		ptr_delete(state->ptr_state);
	}
	if (state->wlsurf) {
		wl_surface_destroy(state->wlsurf);
	}
	if (state->shell != NULL) {
		xdg_wm_base_destroy(state->shell);
	}
	if (state->shm != NULL) {
		wl_shm_destroy(state->shm);
	}
	if (state->comp != NULL) {
		wl_compositor_destroy(state->comp);
	}
	//wl_proxy_destroy(state->disp);
	//wl_event_queue_destroy(state->disp);
	if (state->disp != NULL) {
		wl_display_disconnect(state->disp);
	}
	plutosvg_document_destroy(state->svg);
	report_end();
	free(state->conf);
	free(state->ptr_state);
	free(state);
	exit(0);
}

void toplevel_capabil(void* data, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities){
}

struct xdg_toplevel_listener event_listener = {
	.configure = toplevel_conf,
	.configure_bounds = toplevel_bounds,
	.close = toplevel_close,
	.wm_capabilities = toplevel_capabil,
};
/**
 * pointer input section
 * The following section takes care of pointer and seat menagement
 */

/**
 * delete all of the data associated with the cursor/pointer
 */
void ptr_delete(struct pointer_state *pstate) {
	if (pstate->surf) {
		wl_surface_destroy(pstate->surf);
	}
	if (pstate->theme != NULL) {
		wl_cursor_theme_destroy(pstate->theme);
	}
}
/**
 * pointer listener (listen for changes of the pointer)
 * this is another accumulation event sequens, all values recived in
 * these events should be accumulated(in theory, in reality many 
 * compositors fail to comply to that due to input handling issues ...) 
 * until the next frame event is recieved...
 */

void pointer_enter_handler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y){
	// set pointer ...
	struct window_state *state = (struct window_state *)data;
	struct pointer_state *pstate = state->ptr_state;
	// set the default pointer surface
	if (pstate->surf != NULL && pstate->image != NULL){
		struct wl_cursor_image *pimage = pstate->image;
		wl_pointer_set_cursor(pointer, serial, pstate->surf, pimage->hotspot_x, pimage->hotspot_y);
	}
	wreport("wl_pointer::enter");
}

void pointer_leave_handler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface) {
	// do nothing
	wreport("wl_pointer::leave");
}

void pointer_motion_handler(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y) {}

void pointer_button_handler(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state){
	//move the window when a left button press event is revieved ...
	struct window_state *wstate = data;
	if (button == BTN_LEFT && (enum wl_pointer_button_state)state == WL_POINTER_BUTTON_STATE_PRESSED){
		xdg_toplevel_move(wstate->toplevel, wstate->seat, serial);
		ireport("pointer button (left button press)");
	}
}

void pointer_frame_handler(void *data, struct wl_pointer *pointer){
	// apply accumulated changes ... 
	// issue move request in the toplevel protocol...
	// do nothing ... (apparently due to input handling problems
	// compositors fail to send a frame event, so we have to apply
	// changes in the handler functions...
	// this will probably not be seen, ever...
	wreport("wl_pointer::frame");
}

struct wl_pointer_listener pointer_listener = {
	.enter = pointer_enter_handler,
	.leave = pointer_leave_handler,
	.motion = pointer_motion_handler,
	.button = pointer_button_handler,
	.frame = pointer_frame_handler
};

/**
 * steat listener ( listen for seat capabilities...)
 */
void  seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities){
	struct window_state *state = data;
	if (1 & capabilities){
		//get the pointer and add listeners:
		state->pointer = wl_seat_get_pointer(wl_seat);
		wl_pointer_add_listener(state->pointer, &pointer_listener, state);
	} else if (state->pointer) {
		wl_pointer_release(state->pointer);
	}
	wreport("wl_seat::capabilities");
}

void seat_name(void *data, struct wl_seat *wl_seat, const char *name){}

struct wl_seat_listener seat_listener = {
	.capabilities = seat_capabilities,
	.name = seat_name,
};



/**
 * initialize the window state structure
 */
struct window_state* window_state_init() {
	struct window_state *state = malloc(sizeof(struct window_state));
	memset(state, 0, sizeof(struct window_state));
	// using this the first frame goes to waste (on tilers or just wlroots)
	// becouse the compositor only tells us about the requested dimensions
	// after a reconigure...
	state->changed = 1;
	state->ptr_state = malloc(sizeof(struct pointer_state));
	memset(state->ptr_state, 0, sizeof(struct pointer_state));
	return state;
}
/**
 * lauch the waylogo window
 * this should never return...
 */
void way_launch(struct window_state *state){
	// init connection, get the display
	state->disp = wl_display_connect(NULL);
	if (state->disp == NULL) {
		ereport("could not connect to a wayland display");
		close_win(state);
	}
	// retrive the registry
	state->reg = wl_display_get_registry(state->disp);
	if (state->reg == NULL) {
		// wl_display_get_registry is using wl_proxy_marshal_flags which set's errno (that's why we use %m)
		ereport("could not get the wayland registry: %m");
		close_win(state);
	}
	// add the registry listener
	wl_registry_add_listener(state->reg, &registry_listener, state);

	// wait for the "initial" set of globals to appear ( wait for the events ..)
	wl_display_roundtrip(state->disp);

	// add the ping listener
	if (state->shell != NULL){
		xdg_wm_base_add_listener(state->shell, &ping_listener, NULL);
	}

	if (state->seat != NULL) {
		// add the seat listener (listen for the capabilities (does a pointer exist?))
		wl_seat_add_listener(state->seat, &seat_listener, state);

		// prepair the cursor surface this will stay in memory until exit
		struct pointer_state *pstate = state->ptr_state;
		pstate->theme = wl_cursor_theme_load(NULL, 24, state->shm);
		if (pstate->theme != NULL) {
			pstate->cursor = wl_cursor_theme_get_cursor(pstate->theme, "left_ptr");
			if (pstate->cursor != 0){
				pstate->image = (pstate->cursor)->images[0];
				struct wl_buffer *cursor_buffer = wl_cursor_image_get_buffer(pstate->image);
				// might not be needet, since the the documentation says, 
				// that the buffer schould not be closed by the user 
				//wl_buffer_add_listener(cursor_buffer, &buffer_listener, NULL);
				
				// create a new surface for the cursor image buffer
				pstate->surf = wl_compositor_create_surface(state->comp);
				wl_surface_attach(pstate->surf, cursor_buffer, 0, 0);
				wl_surface_commit(pstate->surf);
			} else {
				ireport("cursor 'left_ptr' could not be found: %m");
			}
		} else {
			ireport("no cursor theme could be found: %m");
		}
	}

	//create a surface and assign the toplevel role
	if (state->comp != NULL) {
		state->wlsurf = wl_compositor_create_surface(state->comp);
		if (state->shell != NULL) {
			state->xdgsurf = xdg_wm_base_get_xdg_surface(state->shell, state->wlsurf);
			xdg_surface_add_listener(state->xdgsurf, &config_listener, state);
			//get and asign the toplevel role to the window
			state->toplevel = xdg_surface_get_toplevel(state->xdgsurf);
			xdg_toplevel_add_listener(state->toplevel, &event_listener, state);
			xdg_toplevel_set_title(state->toplevel, NAME_STR);
		}

		// xdg surface configure sequens
		wl_surface_commit(state->wlsurf);
		wl_display_roundtrip(state->disp);
	} else {
		// becouse there is no compositor?
		ereport("could not get a compositor: %m");
		close_win(state);
	}
	
	while (1) {
		wl_display_dispatch(state->disp);
	}

}
