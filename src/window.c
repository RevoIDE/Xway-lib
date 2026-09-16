#include "app.h"
#include "xdg-shell-client-protocol.h"
#include <wayland-client-protocol.h>

static void on_xdg_surface_configure(
	void *data,
	struct xdg_surface *xdg_surface,
	uint32_t serial)
{
	(void)data;

	xdg_surface_ack_configure(xdg_surface, serial);
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = on_xdg_surface_configure,
};

static void on_toplevel_configure(
	void *data,
	struct xdg_toplevel *toplevel,
	int32_t width,
	int32_t height,
	struct wl_array *states)
{
	(void)data;
	(void)toplevel;
	(void)width;
	(void)height;
	(void)states;
}

static void on_toplevel_close(void *data, struct xdg_toplevel *toplevel)
{
	t_xway_app *app;

	(void)toplevel;

	app = data;
	app->running = 0;
}

static void on_toplevel_configure_bounds(
	void *data,
	struct xdg_toplevel *toplevel,
	int32_t width,
	int32_t height)
{
	(void)data;
	(void)toplevel;
	(void)width;
	(void)height;
}

static void on_toplevel_capabilities(
	void *data,
	struct xdg_toplevel *toplevel,
	struct wl_array *capabilities)
{
	(void)data;
	(void)toplevel;
	(void)capabilities;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = on_toplevel_configure,
	.close = on_toplevel_close,
	.configure_bounds = on_toplevel_configure_bounds,
	.wm_capabilities = on_toplevel_capabilities,
};

int xway_window_create(t_xway_app *app)
{
	app->surface = wl_compositor_create_surface(app->compositor);
	if (!app->surface)
		return (-1);

	app->xdg_surface = xdg_wm_base_get_xdg_surface(app->wm_base, app->surface);
	if (!app->xdg_surface)
		return (-1);

	if (xdg_surface_add_listener(app->xdg_surface, &xdg_surface_listener, app) == -1)
		return (-1);

	app->toplevel = xdg_surface_get_toplevel(app->xdg_surface);
	if (!app->toplevel)
		return (-1);

	if (xdg_toplevel_add_listener(app->toplevel, &xdg_toplevel_listener, app) == -1)
		return (-1);

	xdg_toplevel_set_title(app->toplevel, "Xway-lib");
	xdg_toplevel_set_app_id(app->toplevel, "xway-lib");

	app->running = 1;

	wl_surface_commit(app->surface);

	if (wl_display_roundtrip(app->display) == -1)
		return (-1);

	return (0);
}

void xway_window_cleanup(t_xway_app *app)
{
	if(!app)
		return ;
	if(app->toplevel)
		xdg_toplevel_destroy(app->toplevel);
	if(app->xdg_surface)
		xdg_surface_destroy(app->xdg_surface);
	if(app->surface)
		wl_surface_destroy(app->surface);
	app->toplevel = NULL;
	app->xdg_surface = NULL;
	app->surface = NULL;
}
