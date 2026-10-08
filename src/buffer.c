#define _GNU_SOURCE

#include <stddef.h>
#include <strings.h>
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

static void on_buffer_slot_release(void *data, struct wl_buffer *wayland_buffer)
{
	t_xway_buffer *buffer;

	(void)wayland_buffer;

	buffer = data;
	if (!buffer)
		return;

	buffer->busy = 0;
}

static const struct wl_buffer_listener g_buffer_slot_listener = {
	.release = on_buffer_slot_release,
};

static void xway_buffer_slot_cleanup(t_xway_buffer *buffer)
{
	if (!buffer)
		return;
	if (buffer->wayland_buffer)
		wl_buffer_destroy(buffer->wayland_buffer);
	if (buffer->pixels)
		munmap(buffer->pixels, buffer->size_bytes);

	buffer->wayland_buffer = NULL;
	buffer->pixels = NULL;
	buffer->size_bytes = 0;
	buffer->width = 0;
	buffer->height = 0;
	buffer->stride_bytes = 0;
	buffer->busy = 0;
}

static int xway_buffer_slot_create(
	t_xway_app *app,
	t_xway_buffer *buffer,
	int32_t width,
	int32_t height)
{
	struct wl_shm_pool *pool;
	int32_t stride_bytes;
	int32_t size_bytes;
	int fd;

	if (!app || !buffer || !app->shm)
		return (-1);
	if (buffer->wayland_buffer || buffer->pixels)
		return (-1);
	if (width <= 0 || height <= 0)
		return (-1);
	if (width > INT32_MAX / 4)
		return (-1);

	stride_bytes = width * 4;
	if (height > INT32_MAX / stride_bytes)
		return (-1);

	size_bytes = stride_bytes * height;
	fd = create_shm_file((size_t)size_bytes);
	if (fd == -1)
		return (-1);

	buffer->pixels =
		mmap(NULL, (size_t)size_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (buffer->pixels == MAP_FAILED)
	{
		buffer->pixels = NULL;
		close(fd);
		return (-1);
	}

	pool = wl_shm_create_pool(app->shm, fd, size_bytes);
	if (!pool)
	{
		munmap(buffer->pixels, (size_t)size_bytes);
		buffer->pixels = NULL;
		close(fd);
		return (-1);
	}

	buffer->wayland_buffer = wl_shm_pool_create_buffer(
		pool,
		0,
		width,
		height,
		stride_bytes,
		WL_SHM_FORMAT_XRGB8888);

	wl_shm_pool_destroy(pool);
	close(fd);

	if (!buffer->wayland_buffer)
	{
		munmap(buffer->pixels, (size_t)size_bytes);
		buffer->pixels = NULL;
		return (-1);
	}

	buffer->size_bytes = (size_t)size_bytes;
	buffer->width = width;
	buffer->height = height;
	buffer->stride_bytes = stride_bytes;
	buffer->busy = 0;

	if (wl_buffer_add_listener(buffer->wayland_buffer, &g_buffer_slot_listener, buffer)
		== -1)
	{
		xway_buffer_slot_cleanup(buffer);
		return (-1);
	}
	return (0);
}

static void xway_double_buffer_cleanup(t_xway_app *app)
{
	int i;

	if (!app)
		return;

	i = 0;
	while (i < XWAY_BUFFER_COUNT)
	{
		xway_buffer_slot_cleanup(&app->buffers[i]);
		i++;
	}
	app->acquired_buffer_index = -1;
}

static int xway_double_buffer_create(t_xway_app *app, int32_t width, int32_t height)
{
	int i;

	if (!app)
		return (-1);

	app->acquired_buffer_index = -1;

	i = 0;
	while (i < XWAY_BUFFER_COUNT)
	{
		if (xway_buffer_slot_create(app, &app->buffers[i], width, height) == -1)
		{
			while (i > 0)
			{
				i--;
				xway_buffer_slot_cleanup(&app->buffers[i]);
			}
			return (-1);
		}
		i++;
	}
	return (0);
}

int xway_buffer_create(t_xway_app *app)
{
	if (!app)
		return (-1);

	return (xway_double_buffer_create(app, app->width, app->height));
}

int xway_buffer_acquire(t_xway_app *app)
{
	int i;

	if (!app)
		return (-1);

	if (app->acquired_buffer_index >= 0
		&& app->acquired_buffer_index < XWAY_BUFFER_COUNT)
		return (0);

	i = 0;
	while (i < XWAY_BUFFER_COUNT)
	{
		if (!app->buffers[i].busy && app->buffers[i].wayland_buffer
			&& app->buffers[i].pixels && app->buffers[i].width == app->width
			&& app->buffers[i].height == app->height)
		{
			app->acquired_buffer_index = i;
			//			fprintf(stderr,"xway-lib: acquired buffer %d\n",i);
			return (0);
		}
		i++;
	}
	return (1);
}

void xway_buffer_cleanup(t_xway_app *app)
{
	if (!app)
		return;

	xway_double_buffer_cleanup(app);
}

static int xway_double_buffer_in_use(const t_xway_app *app)
{
	int i;

	if (!app)
		return (1);

	if (app->acquired_buffer_index >= 0)
		return (1);

	i = 0;
	while (i < XWAY_BUFFER_COUNT)
	{
		if (app->buffers[i].busy)
			return (1);
		i++;
	}
	return (0);
}

int xway_apply_resize(t_xway_app *app)
{
	int32_t old_width;
	int32_t old_height;

	if (!app)
		return (-1);
	if (!app->resize_pending)
		return (0);
	if (xway_double_buffer_in_use(app))
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
