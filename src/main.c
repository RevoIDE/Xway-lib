#define _GNU_SOURCE

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <string.h>
#include "app.h"

static void draw_frame(struct app *app)
{
	int32_t x;
	int32_t y;
	uint8_t *row_start;
	uint32_t *row;

	y = 0;
	while(y < app->height)
	{
		row_start = (uint8_t *)app->pixels + (size_t)y * (size_t)app->stride;
		row = (uint32_t *)row_start;
		x = 0;
		while(x < app->width)
		{
			row[x] =  0x00202020;
			x++;
		}
		y++;
	}
}
int main(void)
{
	struct app app = {0};
	int status = EXIT_FAILURE;

	app.width = 800;
	app.height = 600;

	if(wayland_init(&app) == -1)
		goto cleanup;

	if(window_create(&app) == -1)
		goto cleanup;

	if(create_buffer(&app) == -1)
		goto cleanup;

	draw_frame(&app);

	wl_surface_attach(
		app.surface,
		app.buffer,
		0,
		0);

	wl_surface_damage(
		app.surface,
		0,
		0,
		app.width,
		app.height);

	wl_surface_commit(app.surface);

	while(app.running != 0)
	{
		if(wl_display_dispatch(app.display) == -1)
			goto cleanup;
	}
	status = EXIT_SUCCESS;
cleanup:
	app_destroy(&app);
	return (status);
}
