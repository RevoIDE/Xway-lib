#include "app.h"
#include "pointer-constraints-unstable-v1-client-protocol.h"
#include "relative-pointer-unstable-v1-client-protocol.h"
#include "xway.h"

#include <linux/input-event-codes.h>
#include <math.h>
#include <stdlib.h>
#include <wayland-cursor.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
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
static int xway_cursor_create(t_xway_app *app)
{
	if (!app || !app->compositor || !app->shm)
		return (-1);

	if (app->cursor_surface && app->cursor_theme && app->default_cursor)
		return (0);

	app->cursor_surface = wl_compositor_create_surface(app->compositor);
	if (!app->cursor_surface)
		return (-1);

	app->cursor_theme = wl_cursor_theme_load(NULL,24,app->shm);
	if(!app->cursor_theme)
	{
		wl_surface_destroy(app->cursor_surface);
		app->cursor_surface = NULL;
		return (-1);
	}
	app->default_cursor = wl_cursor_theme_get_cursor(app->cursor_theme,"left_ptr");
	if (!app->default_cursor)
	{
		app->default_cursor = wl_cursor_theme_get_cursor(app->cursor_theme,"default");
	}
	if(!app->default_cursor || app->default_cursor->image_count == 0)
	{
		wl_cursor_theme_destroy(app->cursor_theme);
		wl_surface_destroy(app->cursor_surface);
		app->cursor_theme = NULL;
		app->cursor_surface = NULL;
		app->default_cursor = NULL;
		return (-1);
	}

	app->cursor_hidden = 0;
	return (0);
}

static int		xway_cursor_set_hidden(t_xway_app *app,int hidden)
{
	struct wl_cursor_image		*image;
	struct wl_buffer			*buffer;

	if(!app || !app->pointer || !app->pointer_focused || app->pointer_enter_serial == 0)
		return (-1);

	if(hidden)
	{
		wl_pointer_set_cursor(
				app->pointer,
				app->pointer_enter_serial,
				NULL,
				0,
				0);
		app->cursor_hidden = 1;
		return (0);
	}
	if (!app->cursor_surface || !app->default_cursor || app->default_cursor->image_count == 0)
		return (-1);

	image = app->default_cursor->images[0];
	buffer = wl_cursor_image_get_buffer(image);
	if(!buffer)
		return (-1);

	wl_pointer_set_cursor(
			app->pointer,
			app->pointer_enter_serial,
			app->cursor_surface,
			image->hotspot_x,
			image->hotspot_y);
	wl_surface_attach(app->cursor_surface, buffer,0,0);
	wl_surface_damage(
			app->cursor_surface,
			0,
			0,
			image->width,
			image->height);
	wl_surface_commit(app->cursor_surface);

	app->cursor_hidden = 0;
	return (0);
}

void	xway_mouse_cleanup(t_xway_app *app)
{
	if(!app)
		return;

	app->pointer_focused = 0;
	app->pointer_enter_serial = 0;
	app->pointer_locked = 0;
	app->mouse_delta_x = 0.0;
	app->mouse_delta_y = 0.0;
	app->mouse_scroll_x = 0.0;
	app->mouse_scroll_y = 0.0;

	memset(app->mouse_buttons_down,0,sizeof(app->mouse_buttons_down));

	if(app->locked_pointer)
	{
		zwp_locked_pointer_v1_destroy(app->locked_pointer);
		app->locked_pointer = NULL;
	}

	if(app->relative_pointer)
	{
		zwp_relative_pointer_v1_destroy(app->relative_pointer);
		app->relative_pointer = NULL;
	}
	if (app->cursor_theme)
	{
		wl_cursor_theme_destroy(app->cursor_theme);
		app->cursor_theme = NULL;
		app->default_cursor = NULL;
	}

	if(app->cursor_surface)
	{
		wl_surface_destroy(app->cursor_surface);
		app->cursor_surface = NULL;
	}

	app->cursor_hidden = 0;

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

	app = data;
	if	(surface != app->surface)
		return;

	app->pointer_focused = 1;
	app->pointer_enter_serial = serial;
	app->mouse_x = wl_fixed_to_double(surface_x);
	app->mouse_y = wl_fixed_to_double(surface_y);
	xway_cursor_set_hidden(app,app->pointer_locked);
}
static void on_pointer_leave(
		void *data,
		struct wl_pointer *pointer,
		uint32_t serial,
		struct wl_surface *surface)
{
	t_xway_app *app;
	uint8_t		buttons_down[XWAY_MOUSE_BUTTON_COUNT];
	t_xway_mouse_button button;

	(void)pointer;
	(void)serial;

	app = data;
	if(surface != app->surface)
		return;
	
	memcpy(buttons_down,app->mouse_buttons_down,sizeof(buttons_down));
	app->pointer_focused = 0;
	app->pointer_enter_serial = 0;
	app->cursor_hidden = 0;
	app->mouse_scroll_x = 0.0;
	app->mouse_scroll_y = 0.0;
	memset(app->mouse_buttons_down,0,sizeof(app->mouse_buttons_down));

	button = XWAY_MOUSE_BUTTON_UNKNOWN + 1;
	while(button < XWAY_MOUSE_BUTTON_COUNT)
	{
		if( buttons_down[button] && app->mouse_button_callback)
			app->mouse_button_callback(app,button,XWAY_MOUSE_RELEASED,app->mouse_button_user_data);

		button++;
	}
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
	t_xway_mouse_action		action;

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
	{
		app->mouse_buttons_down[xway_button] = 1;
		action = XWAY_MOUSE_PRESSED;
	}
	else if (state == WL_POINTER_BUTTON_STATE_RELEASED)
	{
		 app->mouse_buttons_down[xway_button] = 0;
		 action = XWAY_MOUSE_RELEASED;
	}
	else
		return;

	if (app->mouse_button_callback)
		app->mouse_button_callback(
				app,
				xway_button,
				action,
				app->mouse_button_user_data);
}

static void  on_pointer_axis(
		void *data,
		struct	wl_pointer	*pointer,
		uint32_t time,
		uint32_t axis,
		wl_fixed_t value)
{
	t_xway_app *app;
	double		scroll;

	(void)pointer;
	(void)time;

	app = data;
	if(!app || !app->pointer_focused)
		return;

	scroll = wl_fixed_to_double(value);

	if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
		app->mouse_scroll_x += scroll;
	else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
		app->mouse_scroll_y += scroll;
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
static void on_relative_motion(
		void *data,
		struct zwp_relative_pointer_v1 *relative_pointer,
		uint32_t	utime_hi,
		uint32_t	utime_lo,
		wl_fixed_t	dx,
		wl_fixed_t	dy,
		wl_fixed_t	dx_unaccelerated,
		wl_fixed_t	dy_unaccelerated)
{
	t_xway_app *app;

	(void)relative_pointer;
	(void)utime_hi;
	(void)utime_lo;
	(void)dx;
	(void)dy;

	app = data;
	if(!app->pointer_locked)
		return;

	app->mouse_delta_x += wl_fixed_to_double(dx_unaccelerated);
	app->mouse_delta_y += wl_fixed_to_double(dy_unaccelerated);

}
static const struct zwp_relative_pointer_v1_listener g_relative_pointer_listener =
{
	.relative_motion = on_relative_motion
};


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

static void on_pointer_locked(
		void *data,
		struct zwp_locked_pointer_v1 *locked_pointer)

{
	t_xway_app *app;

	(void)locked_pointer;

	app = data;
	app->pointer_locked = 1;
	xway_cursor_set_hidden(app, 1);
	app->mouse_delta_x = 0.0;
	app->mouse_delta_y = 0.0;
}
static void on_pointer_unlocked(
		void *data,
		struct zwp_locked_pointer_v1 *locked_pointer)
{
	t_xway_app *app;

	(void)locked_pointer;

	app = data;
	app->pointer_locked = 0;
	app->mouse_delta_x = 0.0;
	app->mouse_delta_y = 0.0;
	xway_cursor_set_hidden(app,0);
}
static const struct zwp_locked_pointer_v1_listener
	g_locked_pointer_listener =
{
	.locked = on_pointer_locked,
	.unlocked = on_pointer_unlocked
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
		if (xway_cursor_create(app) == -1)
			fprintf(stderr, "xway-lib: failed to create mouse cursor\n");
		if(!app->relative_pointer_manager)
			return (0);
		app->relative_pointer = zwp_relative_pointer_manager_v1_get_relative_pointer(app->relative_pointer_manager,  app->pointer);

		if(!app->relative_pointer)
		{
			fprintf(stderr,"xway-lib: relative pointer unavailable\n");
			return (0);
		}
		if(zwp_relative_pointer_v1_add_listener(app->relative_pointer,&g_relative_pointer_listener,app) == -1)
		{
			fprintf(stderr,"xway-lib: failed to add relative pointer listener\n");
			zwp_relative_pointer_v1_destroy(app->relative_pointer);
			app->relative_pointer = NULL;
		}
		return (0);
}
int xway_mouse_capture(t_xway_app *app, int enabled)
{
	if(!app)
		return (-1);

	if(!enabled)
	{
		if(app->locked_pointer)
		{
			zwp_locked_pointer_v1_destroy(app->locked_pointer);
			app->locked_pointer = NULL;
		}
		app->pointer_locked = 0;
		app->mouse_delta_x = 0.0;
		app->mouse_delta_y = 0.0;
		xway_cursor_set_hidden(app,0);
		return (0);
	}
	if(app->locked_pointer)
		return (0);

	if(!app->pointer
			|| !app->surface
			|| !app->pointer_constraints
			|| !app->relative_pointer)
	{
		return (-1);
	}
	app->locked_pointer = zwp_pointer_constraints_v1_lock_pointer(
					app->pointer_constraints,
					app->surface,
					app->pointer,
					NULL,
					ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
	if(!app->locked_pointer)
		return (-1);

	if(zwp_locked_pointer_v1_add_listener(app->locked_pointer,&g_locked_pointer_listener,app) == -1)
	{
		zwp_locked_pointer_v1_destroy(app->locked_pointer);
		app->locked_pointer = NULL;
		return (-1);
	}
	app->mouse_delta_x = 0.0;
	app->mouse_delta_y = 0.0;

	return (0);
}
int		xway_mouse_captured(const t_xway_app *app)
{
	if(!app)
		return (0);
	return (app->pointer_locked != 0);
}
int xway_mouse_delta(t_xway_app *app, double *delta_x, double *delta_y)
{
	if(!app || !delta_x || !delta_y)
		return (-1);

	*delta_x = app->mouse_delta_x;
	*delta_y = app->mouse_delta_y;

	app->mouse_delta_x = 0.0;
	app->mouse_delta_y = 0.0;

	return (0);
}
int	xway_mouse_scroll(t_xway_app *app, double *scroll_x, double *scroll_y)
{
	if(!app || !scroll_x || !scroll_y)
		return (-1);

	*scroll_x = app->mouse_scroll_x;
	*scroll_y = app->mouse_scroll_y;

	app->mouse_scroll_x = 0.0;
	app->mouse_scroll_y = 0.0;

	return (0);
}
