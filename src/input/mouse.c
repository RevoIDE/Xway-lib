#include "app.h"
#include "xway.h"

#include <linux/input-event-codes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

static t_xway_mouse_button mouse_button_from_linux(uint32_t button)
{
	if(button == BTN_LEFT)
		return	(XWAY_MOUSE_BUTTON_LEFT);
	if(button == BTN_RIGHT)
		return (XWAY_MOUSE_BUTTON_RIGHT);
	if(button == BTN_MIDDLE)
		return (XWAY_MOUSE_BUTTON_MIDDLE);
	return (XWAY_MOUSE_BUTTON_UNKNOWN);
}

void	xway_mouse_cleanup(t_xway_app *app)
{
	if(!app)
		return;

	app->pointer_focused = 0;
	memset(app->mouse_buttons_down,0,sizeof(app->mouse_buttons_down));

	if(app->pointer)
	{
		if(wl_pointer_get_version(app->pointer)
			>= WL_POINTER_RELEASE_SINCE_VERSION)
				wl_pointer_release(app->pointer);
		else
			wl_pointer_destroy(app->pointer);
		app->pointer = NULL;
	}
}

static void on_pointer_enter(
		void *data,
		struct wl_pointer *pointer,
		uint32_t serial,
		struct wl_surface *surface,
		wl_fixed_t	surface_x,
		wl_fixed_t	surface_y)
{
	t_xway_app *app;

	(void)pointer;
	(void)serial;

	app = data;
	if	(surface != app->surface)
		return;

	app->pointer_focused = 1;
	app->mouse_x = wl_fixed_to_double(surface_x);
	app->mouse_y = wl_fixed_to_double(surface_y);
}
static void on_pointer_leave(
		void *data,
		struct wl_pointer *pointer,
		uint32_t serial,
		struct wl_surface *surface)
{
	t_xway_app *app;

	(void)pointer;
	(void)serial;

	app = data;
	if(surface != app->surface)
		return;

	app->pointer_focused = 0;
	memset(app->mouse_buttons_down, 0, sizeof(app->mouse_buttons_down));
}

static void on_pointer_motion(
		void *data,
		struct wl_pointer *pointer,
		uint32_t time,
		wl_fixed_t surface_x,
		wl_fixed_t surface_y)
{
	t_xway_app *app;

	(void)pointer;
	(void)time;

	app = data;
	if(!app->pointer_focused)
		return;

	app->mouse_x = wl_fixed_to_double(surface_x);
	app->mouse_y = wl_fixed_to_double(surface_y);
}
static void		on_pointer_button(
		void *data,
		struct wl_pointer	*pointer,
		uint32_t serial,
		uint32_t time,
		uint32_t button,
		uint32_t state)
{
	t_xway_app		*app;
	t_xway_mouse_button		xway_button;

	(void)pointer;
	(void)serial;
	(void)time;

	app = data;
	if(!app->pointer_focused)
		return;

	xway_button = mouse_button_from_linux(button);
	if(xway_button == XWAY_MOUSE_BUTTON_UNKNOWN)
		return;

	if(state == WL_POINTER_BUTTON_STATE_PRESSED)
		app->mouse_buttons_down[xway_button] = 1;
	else if (state == WL_POINTER_BUTTON_STATE_RELEASED)
		 app->mouse_buttons_down[xway_button] = 0;
}

static void  on_pointer_axis(
		void *data,
		struct	wl_pointer	*pointer,
		uint32_t time,
		uint32_t axis,
		wl_fixed_t value)
{
	(void)data;
	(void)pointer;
	(void)time;
	(void)axis;
	(void)value;
}
static void on_pointer_frame(
		void *data,
		struct wl_pointer *pointer)
{
	(void)data;
	(void)pointer;
}
static void		on_pointer_axis_source(
		void *data,
		struct wl_pointer *pointer,
		uint32_t axis_source)
{
	(void)data;
	(void)pointer;
	(void)axis_source;
}

static void on_pointer_axis_stop(
		void *data,
		struct wl_pointer *pointer,
		uint32_t time,
		uint32_t axis)
{
	(void)data;
	(void)pointer;
	(void)time;
	(void)axis;
}

static	void	on_pointer_axis_discrete(
		void *data,
		struct	wl_pointer	*pointer,
		uint32_t	axis,
		int32_t discrete)
{
	(void)data;
	(void)pointer;
	(void)axis;
	(void)discrete;
}
static	const	struct	wl_pointer_listener		g_pointer_listener = 
{
	.enter = on_pointer_enter,
	.leave = on_pointer_leave,
	.motion =  on_pointer_motion,
	.button = on_pointer_button,
	.axis = on_pointer_axis,
	.frame = on_pointer_frame,
	.axis_source = on_pointer_axis_source,
	.axis_stop = on_pointer_axis_stop,
	.axis_discrete = on_pointer_axis_discrete
};

int	xway_mouse_create(t_xway_app *app)
{
		if(!app || !app->seat)
			return (-1);
		if(app->pointer)
			return (0);

		app->pointer = wl_seat_get_pointer(app->seat);
		if(!app->pointer)
		{
			fprintf(stderr, "xway-lib: failed to get pointer\n");
			return (-1);
		}
		if(wl_pointer_add_listener(app->pointer,&g_pointer_listener,app) == -1)
		{
			fprintf(stderr,"xway-lib: failed to add pointer listener\n");
			xway_mouse_cleanup(app);
			return (-1);
		}
		return (0);
}
