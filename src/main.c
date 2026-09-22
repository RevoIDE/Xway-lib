#define _GNU_SOURCE

#include "xway.h"
#include "app.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client-core.h>

static void draw_frame(t_xway_app *app)
{
	int32_t x;
	int32_t y;
	uint8_t *row_start;
	uint32_t *row;

	y = 0;
	while (y < app->height)
	{
		row_start = (uint8_t *)app->pixels 
			+ (size_t)y * (size_t)app->stride_bytes;
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
	struct timespec pause;

	pause.tv_sec = 0;
	pause.tv_nsec = 1000000;

	draw_frame(app);
	if(xway_present(app) == -1)
		return (EXIT_FAILURE);

	while	(xway_is_running(app))
	{
		if(xway_poll_events(app) == -1)
			return (EXIT_FAILURE);
		if(!xway_is_running(app))
			break;
		if(xway_frame_ready(app))
		{
			if(xway_present(app) == -1)
				return (EXIT_FAILURE);
		}
		nanosleep(&pause, NULL);
	}
	return (EXIT_SUCCESS);
}

static void on_key(
		t_xway_app *app,
		t_xway_key key,
		t_xway_key_action action,
		void *user_data)
{
	(void)app;
	(void)user_data;

	if(action == XWAY_KEY_PRESSED)
		fprintf(stderr, "callback: %s pressed\n",xway_key_name(key));
	else if (action == XWAY_KEY_RELEASED)
		fprintf(stderr, "callback %s released\n",xway_key_name(key));
}

int main(void)
{
	t_xway_app	*app;
	int			exit_status;

	app = xway_create(800, 600, "Ma fenêtre");
	if (!app)
		return (EXIT_FAILURE);

	xway_set_key_callback(app,on_key,NULL);

	exit_status = run_app(app);
	
	xway_destroy(app);

	return (exit_status);
}
