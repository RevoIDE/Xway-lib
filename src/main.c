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

static void draw_frame(t_xway_app *app)
{
	int32_t x;
	int32_t y;
	uint8_t *row_start;
	uint32_t *row;

	y = 0;
	while (y < app->height)
	{
		row_start = (uint8_t *)app->pixels + (size_t)y * (size_t)app->stride_bytes;
		row = (uint32_t *)row_start;

		x = 0;
		while (x < app->width)
		{
			row[x] = 0x00FFFFFF;
			x++;
		}

		y++;
	}
}

static int run_app(t_xway_app *app)
{
	draw_frame(app);

	if (xway_present(app) == -1)
		return (EXIT_FAILURE);

	while (app->running != 0)
	{
		if (wl_display_dispatch(app->display) == -1)
			return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}

int main(void)
{
	t_xway_app *app;
	int exit_status;

	app = xway_create(800, 600, "Ma fenêtre");
	if (!app)
		return (EXIT_FAILURE);

	exit_status = run_app(app);
	
	xway_destroy(app);

	return (exit_status);
}
