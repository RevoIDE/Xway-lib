#define _GNU_SOURCE
#include <wayland-client-protocol.h>

#include "xway.h"

#include "app.h"

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static void on_wm_base_ping(void *data, struct xdg_wm_base *wm_base, uint32_t serial)
{
	(void)data;

	xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
	.ping = on_wm_base_ping,
};

static void on_registry_global_remove(
	void *data,
	struct wl_registry *registry,
	uint32_t global_id)
{
	t_xway_app *app;

	(void)registry;

	app = data;
	if(app->seat && app->seat_global_id == global_id)
		xway_seat_cleanup(app);
}

static void on_registry_global(
	void *data,
	struct wl_registry *registry,
	uint32_t global_id,
	const char *interface,
	uint32_t server_version)
{
	t_xway_app *app;
	uint32_t bind_version;

	app = data;

	if (!app->compositor && strcmp(interface, wl_compositor_interface.name) == 0)
	{
		bind_version = server_version;

		if (bind_version > (uint32_t)wl_compositor_interface.version)
			bind_version = (uint32_t)wl_compositor_interface.version;

		app->compositor = wl_registry_bind(
			registry,
			global_id,
			&wl_compositor_interface,
			bind_version);
	}

	if (!app->shm && strcmp(interface, wl_shm_interface.name) == 0)
	{
		bind_version = server_version;

		if (bind_version > (uint32_t)wl_shm_interface.version)
			bind_version = (uint32_t)wl_shm_interface.version;

		app->shm =
			wl_registry_bind(registry, global_id, &wl_shm_interface, bind_version);
	}

	if (!app->wm_base && strcmp(interface, xdg_wm_base_interface.name) == 0)
	{
		bind_version = server_version;

		if (bind_version > (uint32_t)xdg_wm_base_interface.version)
			bind_version = (uint32_t)xdg_wm_base_interface.version;

		app->wm_base =
			wl_registry_bind(registry, global_id, &xdg_wm_base_interface, bind_version);

		if (app->wm_base)
		{
			if (xdg_wm_base_add_listener(app->wm_base, &wm_base_listener, app) == -1)
			{
				xdg_wm_base_destroy(app->wm_base);
				app->wm_base = NULL;
			}
		}
	}
	if(!app->seat && strcmp(interface, wl_seat_interface.name) == 0)
	{
		if(xway_seat_bind(app,global_id,server_version) == -1)	
			return;
	}
}

static const struct wl_registry_listener registry_listener = {
	.global = on_registry_global,
	.global_remove = on_registry_global_remove,
};

int xway_app_init(t_xway_app *app)
{
	app->display = wl_display_connect(NULL);
	if (!app->display)
		return (-1);

	app->registry = wl_display_get_registry(app->display);
	if (!app->registry)
		return (-1);

	if (wl_registry_add_listener(app->registry, &registry_listener, app) == -1)
		return (-1);

	if (wl_display_roundtrip(app->display) == -1)
		return (-1);

	if (!app->compositor || !app->wm_base || !app->shm)
		return (-1);

	return (0);
}

void xway_app_cleanup(t_xway_app *app)
{
	if (!app)
		return;
	xway_seat_cleanup(app);
	xway_buffer_cleanup(app);
	xway_window_cleanup(app);

	if (app->wm_base)
		xdg_wm_base_destroy(app->wm_base);
	if (app->shm)
		wl_shm_destroy(app->shm);
	if (app->compositor)
		wl_compositor_destroy(app->compositor);

	if (app->registry)
		wl_registry_destroy(app->registry);
	if (app->display)
		wl_display_disconnect(app->display);
}

int xway_get_frame(t_xway_app *app, t_xway_frame *frame)
{
	if( !app || !frame)
		return (-1);
	if(!app->pixels)
		return (-1);

	frame->pixels = app->pixels;
	frame->width = app->width;
	frame->height = app->height;
	frame->stride_bytes = app->stride_bytes;
	return (0);
}

