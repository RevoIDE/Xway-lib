#include "app.h"

#include <stdint.h>
#include <stdio.h>
#include <wayland-client-protocol.h>

void xway_seat_cleanup(t_xway_app *app)
{
	if(app->seat)
	{
		if(wl_seat_get_version(app->seat) >= WL_SEAT_RELEASE_SINCE_VERSION)
			wl_seat_release(app->seat);
		else
			wl_seat_destroy(app->seat);
	}
	app->seat = NULL;
	app->seat_global_id = 0;
	app->has_keyboard = 0;
	app->has_pointer = 0;
}
static void on_seat_capabilities(
		void *data,
		struct wl_seat *seat,
		uint32_t capabilities
		)
{
	t_xway_app *app;

	(void)seat;

	app = data;

	app->has_keyboard = 
		(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0;
	app->has_pointer =
		(capabilities & WL_SEAT_CAPABILITY_POINTER) != 0;
	fprintf(stderr, "xway-lib:  keyboard=%d pointer=%d\n", app->has_keyboard, app->has_pointer );

}
static void on_seat_name(void *data,struct wl_seat *seat,const char *name)
{
	(void)data;
	(void)seat;

	fprintf(stderr, "xway-lib: seat=%s\n", name);
}

static const struct wl_seat_listener seat_listener = {
	.capabilities = on_seat_capabilities,
	.name = on_seat_name,
};

int xway_seat_bind(
		t_xway_app *app,
		uint32_t global_id,
		uint32_t server_version)
{
	uint32_t bind_version;

	bind_version = server_version;

	if(bind_version > 5)
		bind_version = 5;

	app->seat = wl_registry_bind(app->registry, global_id, &wl_seat_interface, bind_version);

	if(!app->seat)
	{
		fprintf(stderr, "xway-lib: failed to bind wl_seat\n");
		return (-1);
	}
	app->seat_global_id = global_id;
	if(wl_seat_add_listener(app->seat,&seat_listener,app) == -1 )
	{
		fprintf(stderr, "xway-lib: failed to add seat listener\n");
		xway_seat_cleanup(app);
		return (-1);
	}
	return (0);
}
