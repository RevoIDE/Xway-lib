#ifndef APP_H
#define APP_H

#include <stddef.h>
#include <stdint.h>
#include <wayland-client.h>

#include "xdg-shell-client-protocol.h"

struct xway_app
{
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_shm *shm;

	struct wl_surface *surface;
	struct xdg_wm_base *wm_base;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *toplevel;

	struct wl_buffer *buffer;
	uint32_t *pixels;
	size_t buffer_size_bytes;

	int32_t width;
	int32_t height;
	int32_t stride_bytes;

	int running;
};

int xway_app_init(struct xway_app *app);
int xway_window_create(struct xway_app *app);
int xway_buffer_create(struct xway_app *app);
void xway_app_cleanup(struct xway_app *app);

#endif
