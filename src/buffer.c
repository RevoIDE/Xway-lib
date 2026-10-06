#define _GNU_SOURCE
#include <stdint.h>
#include <wayland-client-protocol.h>

#include "app.h"

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

static int create_shm_file(size_t size_bytes)
{
	int fd;

	fd = memfd_create("xway-buffer", MFD_CLOEXEC);
	if (fd == -1)
	{
		perror("xway-lib: memfd_create");
		return (-1);
	}

	if (ftruncate(fd, (off_t)size_bytes) == -1)
	{
		perror("xway-lib: ftruncate");
		close(fd);
		return (-1);
	}

	return (fd);
}

static void on_buffer_release(void *data, struct wl_buffer *buffer)
{
	t_xway_app *app;

	(void)buffer;

	app = data;
	app->buffer_busy = 0;
}
static const struct wl_buffer_listener buffer_listener = {
	.release = on_buffer_release,
};

int xway_buffer_create(t_xway_app *app)
{
	struct wl_shm_pool *pool;
	int fd;
	int32_t size_bytes;

	if (app->width <= 0 || app->height <= 0)
	{
		fprintf(stderr, "xway-lib: invalid buffer dimensions\n");
		return (-1);
	}

	if (app->width > INT32_MAX / 4)
	{
		fprintf(stderr, "xway-lib: buffer stride is too large\n");
		return (-1);
	}

	app->stride_bytes = app->width * 4;

	if (app->height > INT32_MAX / app->stride_bytes)
	{
		fprintf(stderr, "xway-lib: buffer size is too large\n");
		return (-1);
	}
	size_bytes = app->stride_bytes * app->height;
	app->buffer_size_bytes = (size_t)size_bytes;

	fd = create_shm_file(app->buffer_size_bytes);
	if (fd == -1)
		return (-1);

	app->pixels =
		mmap(NULL, app->buffer_size_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (app->pixels == MAP_FAILED)
	{
		perror("xway-lib: mmap");
		app->pixels = NULL;
		close(fd);
		return (-1);
	}

	pool = wl_shm_create_pool(app->shm, fd, size_bytes);
	if (!pool)
	{
		fprintf(stderr, "xway-lib: failed to create wl_shm_pool\n");
		munmap(app->pixels, app->buffer_size_bytes);
		app->pixels = NULL;
		close(fd);
		return (-1);
	}

	app->buffer = wl_shm_pool_create_buffer(
		pool,
		0,
		app->width,
		app->height,
		app->stride_bytes,
		WL_SHM_FORMAT_XRGB8888);

	wl_shm_pool_destroy(pool);
	close(fd);

	if (!app->buffer)
	{
		fprintf(stderr, "xway-lib: failed to create wl_buffer\n");
		munmap(app->pixels, app->buffer_size_bytes);
		app->pixels = NULL;
		return (-1);
	}
	if (wl_buffer_add_listener(app->buffer, &buffer_listener, app) == -1)
	{
		fprintf(stderr, "xway-lib: failed to add buffer listener\n");
		xway_buffer_cleanup(app);
		return (-1);
	}
	app->buffer_busy = 0;

	return (0);
}

void xway_buffer_cleanup(t_xway_app *app)
{
	if (!app)
		return;
	if (app->buffer)
		wl_buffer_destroy(app->buffer);
	if (app->pixels)
		munmap(app->pixels, app->buffer_size_bytes);
	app->buffer = NULL;
	app->pixels = NULL;
	app->buffer_size_bytes = 0;
	app->stride_bytes = 0;
	app->buffer_busy = 0;
}

int xway_apply_resize(t_xway_app *app)
{
	int32_t old_width;
	int32_t old_height;

	if (!app)
		return (-1);
	if (!app->resize_pending)
		return (0);
	if (app->buffer_busy)
		return (1);
	if (app->pending_width <= 0 || app->pending_height <= 0)
	{
		app->resize_pending = 0;
		return (-1);
	}
	old_width = app->width;
	old_height = app->height;

	xway_buffer_cleanup(app);

	app->width = app->pending_width;
	app->height = app->pending_height;

	if (xway_buffer_create(app) == -1)
	{
		app->width = old_width;
		app->height = old_height;

		xway_buffer_cleanup(app);
		if (xway_buffer_create(app) == -1)
			app->running = 0;

		return (-1);
	}
	app->resize_pending = 0;
	return (0);
}
