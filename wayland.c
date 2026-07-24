#include "wayland.h"

void way_launch(struct window_state *win){
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
	xdg_toplevel_set_title(state.toplevel, NAME_STR);

	// xdg surface configure sequens
	wl_surface_commit(state.wlsurf);
	wl_display_roundtrip(state.disp);
	
	while (1) {
		wl_display_dispatch(state.disp);
	}

}
