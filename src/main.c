#include <time.h>
#include <unistd.h>
#define _GNU_SOURCE

#include "xway.h"
#include "app.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wayland-client-core.h>

// static void check_keyboard(t_xway_app *app)
// {
// 	static int previous_state[XWAY_KEY_COUNT];
// 	t_xway_key key;
// 	int current_state;
// 
// 	key = XWAY_KEY_UNKNOWN + 1;
// 	while (key < XWAY_KEY_COUNT) 
// 	{
// 		current_state = xway_key_down(app, key);
// 		if(current_state  != previous_state[key])
// 		{
// 			if(current_state)
// 				fprintf(stderr, "xway-lib: key %s pressed\n", xway_key_name(key));
// 			else
// 				fprintf(stderr, "xway-lib: key %s released\n", xway_key_name(key));
// 			previous_state[key] = current_state;
// 		}
// 		key++;
// 	}
// }
	
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
	unsigned int updates;
	int result;
	int blocking;

	blocking = 0;
	updates = 0;
	pause.tv_sec = 0;
	pause.tv_nsec = 100000000;

	draw_frame(app);
	if(xway_present(app) ==  -1)
		return (EXIT_FAILURE);

	while	(xway_is_running(app))
	{
		if(blocking)
			result = xway_wait_events(app);
		else
			result = xway_poll_events(app);
		if(result == -1)
			return (EXIT_FAILURE);
		if(!xway_is_running(app))
			break;
		updates++;
		fprintf(stderr,"updaes moteur : %u\n", updates);
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
