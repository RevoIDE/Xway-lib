#include "xway.h"
#include "app.h"
#include "types.h"
#include "xdg-shell-client-protocol.h"

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <poll.h>

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

static void on_frame_done(void *data, struct wl_callback *callback, uint32_t callback_data)
{
	t_xway_app *app;

	(void)callback_data;

	app = data;
	wl_callback_destroy(callback);
	app->frame_callback = NULL;
	app->frame_ready = 1;
}

static const struct wl_callback_listener frame_listener = {
	.done = on_frame_done,
};

int xway_request_frame(t_xway_app *app)
{
	if (!app || !app->surface)
		return (-1);

	if (app->frame_callback)
		return (0);

	app->frame_callback = wl_surface_frame(app->surface);
	if (!app->frame_callback)
		return (-1);

	if (wl_callback_add_listener(app->frame_callback, &frame_listener, app) == -1)
	{
		wl_callback_destroy(app->frame_callback);
		app->frame_callback = NULL;
		return (-1);
	}

	app->frame_ready = 0;

	return (0);
}

int xway_frame_ready(const t_xway_app *app)
{
	if (!app)
		return (0);

	return (app->frame_ready != 0);
}

int xway_wait_frame(t_xway_app *app)
{
	if (!app || !app->display)
		return (-1);

	while (app->running != 0 && app->frame_ready == 0)
	{
		if (wl_display_dispatch(app->display) == -1)
		{
			app->running = 0;
			return (-1);
		}
	}

	return (0);
}

int	xway_present(t_xway_app *app)
{
	if (!app)
		return (-1);

	if (!app->surface || !app->buffer)
		return (-1);

	if (xway_request_frame(app) == -1)
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

int xway_poll_events(t_xway_app *app)
{
	struct pollfd fd;
	int result;
	int saved_errno;

	if(!app || !app->display)
		return (-1);

	while (wl_display_prepare_read(app->display) != 0)
	{
		if	(wl_display_dispatch_pending(app->display) == -1)
		{
			app->running = 0;
			return (-1);
		}
	}
	if(wl_display_flush(app->display) == -1 && errno != EAGAIN)
	{
		wl_display_cancel_read(app->display);
		app->running = 0;
		return (-1);
	}

	fd.fd = wl_display_get_fd(app->display);
	fd.events = POLLIN;
	fd.revents = 0;

	result = poll(&fd,  1,  0);
	if(result == -1)
	{
		saved_errno = errno;
		wl_display_cancel_read(app->display);
		if(saved_errno == EINTR)
			return (0);
		app->running = 0;
		return (-1);
	}
	if(fd.revents & (POLLERR | POLLHUP | POLLNVAL))
	{
		wl_display_cancel_read(app->display);
		app->running = 0;
		return (-1);
	}	

	if(fd.revents & POLLIN)
	{
		if(wl_display_read_events(app->display) == -1)
		{
			app->running = 0;
			return (-1);
		}
	}
	else
		wl_display_cancel_read(app->display);
	if(wl_display_dispatch_pending(app->display) == -1)
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
void xway_set_key_callback(t_xway_app *app, t_xway_key_callback callback, void *user_data)
{
	if(!app)
		return;

	app->key_callback = callback;
	app->key_user_data = user_data;
}
