#define _GNU_SOURCE
#include <wayland-client-protocol.h>

#include "xway.h"

#include "app.h"

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static void on_wm_base_ping(void *data, struct xdg_wm_base *wm_base, uint32_t serial)
{
	(void)data;

	xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
	.ping = on_wm_base_ping,
};

static void seat_cleanup(t_xway_app *app)
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

static void on_registry_global_remove(
	void *data,
	struct wl_registry *registry,
	uint32_t global_id)
{
	t_xway_app *app;

	(void)registry;

	app = data;
	if(app->seat && app->seat_global_id == global_id)
		seat_cleanup(app);
}

static void on_registry_global(
	void *data,
	struct wl_registry *registry,
	uint32_t global_id,
	const char *interface,
	uint32_t server_version)
{
	t_xway_app *app;
	uint32_t bind_version;

	app = data;

	if (!app->compositor && strcmp(interface, wl_compositor_interface.name) == 0)
	{
		bind_version = server_version;

		if (bind_version > (uint32_t)wl_compositor_interface.version)
			bind_version = (uint32_t)wl_compositor_interface.version;

		app->compositor = wl_registry_bind(
			registry,
			global_id,
			&wl_compositor_interface,
			bind_version);
	}

	if (!app->shm && strcmp(interface, wl_shm_interface.name) == 0)
	{
		bind_version = server_version;

		if (bind_version > (uint32_t)wl_shm_interface.version)
			bind_version = (uint32_t)wl_shm_interface.version;

		app->shm =
			wl_registry_bind(registry, global_id, &wl_shm_interface, bind_version);
	}

	if (!app->wm_base && strcmp(interface, xdg_wm_base_interface.name) == 0)
	{
		bind_version = server_version;

		if (bind_version > (uint32_t)xdg_wm_base_interface.version)
			bind_version = (uint32_t)xdg_wm_base_interface.version;

		app->wm_base =
			wl_registry_bind(registry, global_id, &xdg_wm_base_interface, bind_version);

		if (app->wm_base)
		{
			if (xdg_wm_base_add_listener(app->wm_base, &wm_base_listener, app) == -1)
			{
				xdg_wm_base_destroy(app->wm_base);
				app->wm_base = NULL;
			}
		}
	}
	if(!app->seat && strcmp(interface, wl_seat_interface.name) == 0)
	{
		bind_version = server_version;

		if(bind_version > 5)
			bind_version = 5;
		app->seat = wl_registry_bind(
				registry,
				global_id,
				&wl_seat_interface,
				bind_version);
		if(!app->seat)
		{
			fprintf(stderr,"xway-lib: failed to bind wl_seat\n");
			return ;
		}
		app->seat_global_id = global_id;

		if(wl_seat_add_listener(app->seat, &seat_listener,app) == -1)
		{
			fprintf(stderr, "xway-lib: failed to add seat listener\n");
			seat_cleanup(app);
		}
	}	
}

static const struct wl_registry_listener registry_listener = {
	.global = on_registry_global,
	.global_remove = on_registry_global_remove,
};

static void on_xdg_surface_configure(
	void *data,
	struct xdg_surface *xdg_surface,
	uint32_t serial)
{
	(void)data;

	xdg_surface_ack_configure(xdg_surface, serial);
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = on_xdg_surface_configure,
};

static void on_toplevel_configure(
	void *data,
	struct xdg_toplevel *toplevel,
	int32_t width,
	int32_t height,
	struct wl_array *states)
{
	(void)data;
	(void)toplevel;
	(void)width;
	(void)height;
	(void)states;
}

static void on_toplevel_close(void *data, struct xdg_toplevel *toplevel)
{
	t_xway_app *app;

	(void)toplevel;

	app = data;
	app->running = 0;
}

static void on_toplevel_configure_bounds(
	void *data,
	struct xdg_toplevel *toplevel,
	int32_t width,
	int32_t height)
{
	(void)data;
	(void)toplevel;
	(void)width;
	(void)height;
}

static void on_toplevel_capabilities(
	void *data,
	struct xdg_toplevel *toplevel,
	struct wl_array *capabilities)
{
	(void)data;
	(void)toplevel;
	(void)capabilities;
}
static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = on_toplevel_configure,
	.close = on_toplevel_close,
	.configure_bounds = on_toplevel_configure_bounds,
	.wm_capabilities = on_toplevel_capabilities,
};

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

	return (0);
}

int xway_app_init(t_xway_app *app)
{
	app->display = wl_display_connect(NULL);
	if (!app->display)
		return (-1);

	app->registry = wl_display_get_registry(app->display);
	if (!app->registry)
		return (-1);

	if (wl_registry_add_listener(app->registry, &registry_listener, app) == -1)
		return (-1);

	if (wl_display_roundtrip(app->display) == -1)
		return (-1);

	if (!app->compositor || !app->wm_base || !app->shm)
		return (-1);

	return (0);
}

int xway_window_create(t_xway_app *app)
{
	app->surface = wl_compositor_create_surface(app->compositor);
	if (!app->surface)
		return (-1);

	app->xdg_surface = xdg_wm_base_get_xdg_surface(app->wm_base, app->surface);
	if (!app->xdg_surface)
		return (-1);

	if (xdg_surface_add_listener(app->xdg_surface, &xdg_surface_listener, app) == -1)
		return (-1);

	app->toplevel = xdg_surface_get_toplevel(app->xdg_surface);
	if (!app->toplevel)
		return (-1);

	if (xdg_toplevel_add_listener(app->toplevel, &xdg_toplevel_listener, app) == -1)
		return (-1);

	xdg_toplevel_set_title(app->toplevel, "Xway-lib");
	xdg_toplevel_set_app_id(app->toplevel, "xway-lib");

	app->running = 1;

	wl_surface_commit(app->surface);

	if (wl_display_roundtrip(app->display) == -1)
		return (-1);

	return (0);
}

void xway_app_cleanup(t_xway_app *app)
{
	if (!app)
		return;
	seat_cleanup(app);
	
	if (app->buffer)
		wl_buffer_destroy(app->buffer);
	if (app->pixels)
		munmap(app->pixels, app->buffer_size_bytes);

	if (app->toplevel)
		xdg_toplevel_destroy(app->toplevel);
	if (app->xdg_surface)
		xdg_surface_destroy(app->xdg_surface);
	if (app->surface)
		wl_surface_destroy(app->surface);

	if (app->wm_base)
		xdg_wm_base_destroy(app->wm_base);
	if (app->shm)
		wl_shm_destroy(app->shm);
	if (app->compositor)
		wl_compositor_destroy(app->compositor);

	if (app->registry)
		wl_registry_destroy(app->registry);
	if (app->display)
		wl_display_disconnect(app->display);
}

int xway_get_frame(t_xway_app *app, t_xway_frame *frame)
{
	if( !app || !frame)
		return (-1);
	if(!app->pixels)
		return (-1);

	frame->pixels = app->pixels;
	frame->width = app->width;
	frame->height = app->height;
	frame->stride_bytes = app->stride_bytes;
	return (0);
}
