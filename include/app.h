#ifndef APP_H
#define APP_H

#include <stddef.h>
#include <stdint.h>
#include <wayland-client.h>

#include "xdg-shell-client-protocol.h"

struct app
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
	size_t buffer_size;

	int32_t width;
	int32_t height;
	int32_t stride;

	int running;
};

int wayland_init(struct app *app);
int window_create(struct app *app);
int create_buffer(struct app *app);
void app_destroy(struct app *app);


#endif
