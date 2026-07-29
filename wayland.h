#include <stdint.h>
#include "xdg-shell-protocol.h"

struct pointer_state {
	struct wl_cursor_image *image;
};

struct window_state {
	/*Globals*/
	struct wl_display *disp;
	struct wl_registry *reg;
	struct wl_compositor *comp;
	struct wl_shm *shm;
	struct wl_seat *seat;
	struct xdg_wm_base *shell;
	/*Objects*/
	struct wl_surface *wlsurf;
	struct wl_pointer *pointer;
	struct wl_surface *pointer_surf;
	struct xdg_surface *xdgsurf;
	struct xdg_toplevel *toplevel;
	/*Settings*/
	int32_t width;
	int32_t height;
	enum xdg_toplevel_state tlstate;
	enum xdg_toplevel_state current;
	int changed; //is set when there was a change made to the dimensions since the last xdg_surface config
	struct pointer_state *ptr_state;
	/*Misc*/
	plutosvg_document_t *svg;
	struct waylogo_config *conf;
};

void render(struct window_state *state, int fd);


void buffer_release(void *data, struct wl_buffer *wl_buffer);

void configure_surface(struct window_state *state);


void registry_global_handler(void *data , struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
void registry_global_remove_handler(void *data, struct wl_registry *registry, uint32_t name);

void config_handler(void* data, struct xdg_surface *xdg_surf, uint32_t serial);

void ping_handler(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);

void toplevel_conf(void* data, struct xdg_toplevel *xgd_toplevel, int32_t width, int32_t height, struct wl_array *states);
void toplevel_bounds(void* data, struct xdg_toplevel *xgd_toplevel, int32_t width, int32_t height);
void toplevel_close(void* data, struct xdg_toplevel *xdg_toplevel);
void toplevel_capabil(void* data, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities);

void pointer_enter_handler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y);

void pointer_leave_handler(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface);

void pointer_motion_handler(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);


void pointer_button_handler(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);

void pointer_frame_handler(void *data, struct wl_pointer *pointer);

void  seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities);

void seat_name(void *data, struct wl_seat *wl_seat, const char *name);

struct window_state *window_state_init();

void way_launch(struct window_state *state);
