#include "xway.h"
#include "app.h"
#include "xdg-shell-client-protocol.h"
#include <stdlib.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>


void xway_destroy(t_xway_app *app)
{
	if(!app)
		return ;

	xway_app_cleanup(app);

	free(app);
}

t_xway_app *xway_create(int width, int height, const char *title)
{
	t_xway_app *app;
	
	if(width <= 0 || height <= 0 || !title)
		return (NULL);
	
	app = calloc(1,sizeof(*app));

	if(!app)
		return (NULL);
	
	app->width =	width;
	app->height =	height;

	if(xway_app_init(app) == -1)
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
int xway_present(t_xway_app *app)
{
	if (!app)
		return (-1);
	if (!app->surface || !app->buffer)
		return (-1);

	wl_surface_attach(app->surface, app->buffer, 0,0);
	wl_surface_damage(app->surface, 0, 0, app->width, app->height);
	wl_surface_commit(app->surface);

	return (0);
}
int xway_is_running(const t_xway_app *app)
{
	if(!app)
		return (0);
	return app->running != 0;
}
int xway_wait_events(t_xway_app *app)
{
	if (!app || app->display)
		return (-1);
	if (wl_display_dispatch(app->display) == -1)
	{
		app->running = 0;
		return (-1);
	}
	return (0);
}
