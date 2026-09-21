#include "xway.h"
#include "app.h"
#include "types.h"
#include "xdg-shell-client-protocol.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#define X(name) [name] = #name,

static const char *g_xway_key_names[XWAY_KEY_COUNT] = 
{
	#include "xway_keys.def"
};

#undef X

const char	*xway_key_name(t_xway_key key)
{
	if(key < XWAY_KEY_UNKNOWN || key >= XWAY_KEY_COUNT)
		return ("XWAY_KEY_UNKNOWN");
	return (g_xway_key_names[key]);
}

void	xway_destroy(t_xway_app *app)
{
	if(!app)
		return;

	xway_app_cleanup(app);
	free(app);
}

int xway_key_down(const t_xway_app  *app, t_xway_key key)
{
	if(!app)
		return (0);

	if(!app->keyboard_focused)
		return (0);

	if(key <= XWAY_KEY_UNKNOWN || key >= XWAY_KEY_COUNT)
		return (0);
	return (app->keys_down[key] != 0);
}
t_xway_app	*xway_create(int width, int height, const char *title)
{
	t_xway_app *app;

	if(width <= 0 || height <= 0 || !title)
		return (NULL);

	app = calloc(1, sizeof(*app));
	if (!app)
		return (NULL);

	app->width	= width;
	app->height = height;

	if (xway_app_init(app) == -1)
	{
		xway_destroy(app);
		return (NULL);
	}

	if (xway_window_create(app) == -1)
	{
		xway_destroy(app);
		return (NULL);
	}

	xdg_toplevel_set_title(app->toplevel, title);

	if (xway_buffer_create(app) == -1)
	{
		xway_destroy(app);
		return (NULL);
	}

	return app;
}

int	xway_present(t_xway_app *app)
{
	if (!app)
		return (-1);

	if (!app->surface || !app->buffer)
		return (-1);

	wl_surface_attach(app->surface, app->buffer, 0,0);
	wl_surface_damage(app->surface, 0, 0,app->width, app->height);
	wl_surface_commit(app->surface);

	return (0);
}

int	xway_is_running(const t_xway_app *app)
{
	if (!app)
		return (0);

	return app->running != 0;
}

int	xway_wait_events(t_xway_app *app)
{
	if (!app || !app->display)
		return (-1);

	if (wl_display_dispatch(app->display) == -1)
	{
		app->running = 0;
		return (-1);
	}

	return (0);
}

int	xway_dispatch(t_xway_app *app)
{
	return (wl_display_dispatch(app->display));
}

void	xway_blit(t_xway_app *app, uint32_t *pixels)
{
	uint8_t *dst_row;
	uint8_t *src_row;

	int32_t y;

	if (!app || !app->pixels || !pixels)
		return;

	y = 0;
	while (y < app->height)
	{
		dst_row = (uint8_t *) app->pixels	
				+ (size_t) y * (size_t) app->stride_bytes;
		src_row = (uint8_t *) pixels
				+ (size_t) y * (size_t)app->width * sizeof(uint32_t);

		memcpy(dst_row, src_row, (size_t)app->width * sizeof(uint32_t));

		y++;
	}
}
