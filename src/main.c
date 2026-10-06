#include <math.h>
#define _GNU_SOURCE

#include "xway.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static void draw_frame(t_xway_frame *frame)
{
	int32_t x;
	int32_t y;
	uint8_t *row_start;
	uint32_t *row;

	y = 0;
	while (y < frame->height)
	{
		row_start = (uint8_t *)frame->pixels + (size_t)y * (size_t)frame->stride_bytes;
		row = (uint32_t *)row_start;

		x = 0;
		while (x < frame->width)
		{
			row[x] = 0x00FFFFFF;
			x++;
		}
		y++;
	}
}

static void on_mouse_button(
	t_xway_app *app,
	t_xway_mouse_button button,
	t_xway_mouse_action action,
	void *user_data)
{
	static const char *names[XWAY_MOUSE_BUTTON_COUNT] = {
		[XWAY_MOUSE_BUTTON_UNKNOWN] = "unknown",
		[XWAY_MOUSE_BUTTON_LEFT] = "left",
		[XWAY_MOUSE_BUTTON_RIGHT] = "right",
		[XWAY_MOUSE_BUTTON_MIDDLE] = "middle"};

	(void)user_data;
	fprintf(
		stderr,
		"callback: mouse %s %s (down=%d)\n",
		names[button],
		action == XWAY_MOUSE_PRESSED ? "pressed" : " released",
		xway_mouse_button_down(app, button));
}

static void check_mouse_position(t_xway_app *app)
{
	static double previous_x = -1.0;
	static double previous_y = -1.0;
	double x;
	double y;

	if (xway_mouse_position(app, &x, &y) == -1)
		return;
	if (x == previous_x && y == previous_y)
		return;

	fprintf(stderr, "xway_lib: mouse x=%.2f y=%.2f\n", x, y);
	previous_x = x;
	previous_y = y;
}

static void check_relative_mouse(t_xway_app *app)
{
	static int previous_capture_state = 0;
	int captured;
	double delta_x;
	double delta_y;

	captured = xway_mouse_captured(app);
	if (captured != previous_capture_state)
	{
		if (captured)
			fprintf(stderr, "xway_lib: pointer captured\n");
		else
			fprintf(stderr, "xway_lib: pointer released\n");

		previous_capture_state = captured;
	}
	if (xway_mouse_delta(app, &delta_x, &delta_y) == -1)
		return;

	if (delta_x == 0.0 && delta_y == 0.0)
		return;

	fprintf(stderr, "xway_lib: relative dx=%.2f dy=%.2f\n", delta_x, delta_y);
}

static void check_mouse_scroll(t_xway_app *app)
{
	double scroll_x;
	double scroll_y;

	if (xway_mouse_scroll(app, &scroll_x, &scroll_y) == -1)
		return;
	fprintf(stderr, "xway_lib: scroll x=%.2f y=%.2f\n", scroll_x, scroll_y);
}

static int run_app(t_xway_app *app)
{
	struct timespec pause;
	t_xway_frame frame;

	int frame_result;
	int present_result;

	pause.tv_sec = 0;
	pause.tv_nsec = 1000000;

	frame_result = xway_get_frame(app, &frame);
	if (frame_result != 0)
		return (EXIT_FAILURE);
	draw_frame(&frame);

	present_result = xway_present(app);
	if (present_result == -1)
		return (EXIT_FAILURE);

	while (xway_is_running(app))
	{
		if (xway_poll_events(app) == -1)
			return (EXIT_FAILURE);
		if (!xway_is_running(app))
			break;

		// check_mouse_position(app);
		check_relative_mouse(app);
		check_mouse_scroll(app);

		if (xway_frame_ready(app))
		{
			frame_result = xway_get_frame(app, &frame);
			if (frame_result == -1)
				return (EXIT_FAILURE);
			if (frame_result == 0)
			{
				// fprintf(stderr, "frame: %d x %d\n",frame.width , frame.height);
				draw_frame(&frame);

				present_result = xway_present(app);
				if (present_result == -1)
					return (EXIT_FAILURE);
			}
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
	(void)user_data;

	if (action == XWAY_KEY_PRESSED)
	{
		fprintf(stderr, "callback: %s pressed\n", xway_key_name(key));
		if (key == XWAY_KEY_C)
		{
			if (xway_mouse_capture(app, 1) == -1)
				fprintf(stderr, "xway_lib: pointer capture unavailable\n");
			else
				fprintf(stderr, "xway_lib pointer capture requested\n");
		}
		else if (key == XWAY_KEY_ESCAPE)
			xway_mouse_capture(app, 0);
	}
	else if (action == XWAY_KEY_RELEASED)
		fprintf(stderr, "callback %s released\n", xway_key_name(key));
}

int main(void)
{
	t_xway_app *app;
	int exit_status;

	app = xway_create(800, 600, "Ma fenêtre");
	if (!app)
		return (EXIT_FAILURE);

	xway_set_key_callback(app, on_key, NULL);
	xway_set_mouse_button_callback(app, on_mouse_button, NULL);

	exit_status = run_app(app);

	xway_destroy(app);

	return (exit_status);
}
