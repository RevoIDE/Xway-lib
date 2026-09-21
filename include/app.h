#ifndef APP_H
#define APP_H

#include "xway.h"

#include <stddef.h>
#include <stdint.h>
#include <wayland-client.h>

#include "xdg-shell-client-protocol.h"

#include <xkbcommon/xkbcommon.h>

#include <linux/input-event-codes.h>

#define XWAY_RAW_KEY_COUNT KEY_CNT

typedef struct s_xway_app
{
	struct wl_display		*display;
	struct wl_registry		*registry;
	struct wl_compositor	*compositor;
	struct wl_shm			*shm;

	struct wl_surface	*surface;
	struct xdg_wm_base 	*wm_base;
	struct xdg_surface 	*xdg_surface;
	struct xdg_toplevel	*toplevel;

	struct wl_buffer	*buffer;
	uint32_t			*pixels;
	size_t 				buffer_size_bytes;

	int32_t	width;
	int32_t	height;
	int32_t	stride_bytes;

	struct wl_seat *seat;
	struct wl_keyboard *keyboard;
	uint8_t keys_down[XWAY_KEY_COUNT];
	int keyboard_focused;

	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;

	uint32_t seat_global_id;
	int has_keyboard;
	int has_pointer;

	int	running;
}	t_xway_app;


int	xway_app_init		(t_xway_app *app);
int	xway_window_create	(t_xway_app *app);
int	xway_buffer_create	(t_xway_app *app);

void	xway_app_cleanup(t_xway_app *app);

int xway_seat_bind(t_xway_app *app,uint32_t global_id,uint32_t server_version);

void xway_seat_cleanup(t_xway_app *app);

void xway_buffer_cleanup(t_xway_app *app);

void xway_window_cleanup(t_xway_app *app);

int xway_keyboard_create(t_xway_app *app);

void xway_keyboard_cleanup(t_xway_app *app);


#endif
