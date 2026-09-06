#define _GNU_SOURCE

#include "app.h"

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static void wm_base_ping(void *data,struct xdg_wm_base *wm_base,uint32_t serial)
{
	(void)data;

	xdg_wm_base_pong(wm_base,serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
	.ping = wm_base_ping,
};

static void registry_global_remove(void *data,struct wl_registry *registry,uint32_t name)
{
	(void)data;
	(void)registry;
	(void)name;
}

static void registry_global(void *data,struct wl_registry *registry,uint32_t name,const char *interface,uint32_t version)
{
	struct app *app;
	uint32_t bind_version;

	app = data;
	if(app->compositor == NULL && strcmp(interface, wl_compositor_interface.name) == 0)
	{
		bind_version = version;
		if(bind_version > (uint32_t)wl_compositor_interface.version)
			bind_version = (uint32_t)wl_compositor_interface.version;

		app->compositor= wl_registry_bind(registry,name,&wl_compositor_interface,bind_version);
	}
	if(app->shm == NULL &&
	strcmp(interface, wl_shm_interface.name) == 0)
{
	bind_version = version;

	if(bind_version > (uint32_t)wl_shm_interface.version)
		bind_version = (uint32_t)wl_shm_interface.version;

	app->shm = wl_registry_bind(
		registry,
		name,
		&wl_shm_interface,
		bind_version);
}
	if(app->wm_base == NULL && strcmp(interface, xdg_wm_base_interface.name) == 0 )
	{
		bind_version = version;

		if(bind_version > (uint32_t)xdg_wm_base_interface.version)
			bind_version = (uint32_t)xdg_wm_base_interface.version;
		app->wm_base = wl_registry_bind(
	registry,
	name,
	&xdg_wm_base_interface,
	bind_version);

if(app->wm_base != NULL)
{
	if(xdg_wm_base_add_listener(
		app->wm_base,
		&wm_base_listener,
		app) == -1)
	{
		xdg_wm_base_destroy(app->wm_base);
		app->wm_base = NULL;
	}
}
	}
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove,
};

static void xdg_surface_configure(void *data,struct xdg_surface *xdg_surface,uint32_t serial)
{
	(void)data;

	xdg_surface_ack_configure(xdg_surface, serial);
}
static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};
static void xdg_toplevel_configure(
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
static void xdg_toplevel_close(
	void *data,
	struct xdg_toplevel *toplevel)
{
	struct app *app;

	(void)toplevel;

	app = data;
	app->running = 0;
}
static void xdg_toplevel_configure_bounds(
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

static void xdg_toplevel_wm_capabilities(
	void *data,
	struct xdg_toplevel *toplevel,
	struct wl_array *capabilities)
{
	(void)data;
	(void)toplevel;
	(void)capabilities;
}
static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = xdg_toplevel_configure,
	.close = xdg_toplevel_close,
	.configure_bounds = xdg_toplevel_configure_bounds,
	.wm_capabilities = xdg_toplevel_wm_capabilities,
};
static int create_shm_file(size_t size)
{
	int fd;

	fd = memfd_create("xway-buffer", MFD_CLOEXEC);
	if(fd == -1)
	{
		perror("xway-lib: memfd_create");
		return (-1);
	}

	if(ftruncate(fd, (off_t)size) == -1)
	{
		perror("xway-lib: ftruncate");
		close(fd);
		return (-1);
	}

	return (fd);
}
int create_buffer(struct app *app)
{
	struct wl_shm_pool *pool;
	int fd;
	int32_t size;

	if(app->width <= 0 || app->height <= 0)
	{
		fprintf(stderr, "xway-lib: invalid buffer dimensions\n");
		return (-1);
	}
	if(app->width > INT32_MAX / 4)
	{
		fprintf(stderr, "xway-lib: buffer stride is too large\n");
		return (-1);
	}
	app->stride = app->width * 4;
	if(app->height > INT32_MAX / app->stride)
	{
		fprintf(stderr, "xway-lib: buffer size is too large\n");
		return (-1);
	}
	size = app->stride * app->height;
	app->buffer_size = (size_t)size;

	fd = create_shm_file(app->buffer_size);
	if(fd == -1)
		return (-1);

	app->pixels = mmap(
		NULL,
		app->buffer_size,
		PROT_READ | PROT_WRITE,
		MAP_SHARED,
		fd,
		0);

	if(app->pixels == MAP_FAILED)
	{
		perror("xway-lib: mmap");
		app->pixels = NULL;
		close(fd);
		return (-1);
	}

	pool = wl_shm_create_pool(
		app->shm,
		fd,
		size);

	if(pool == NULL)
	{
		fprintf(stderr,"xway-lib: failed to create wl_shm_pool\n");
		munmap(app->pixels, app->buffer_size);
		app->pixels = NULL;
		close(fd);
		return (-1);
	}

	app->buffer = wl_shm_pool_create_buffer(
		pool,
		0,
		app->width,
		app->height,
		app->stride,
		WL_SHM_FORMAT_XRGB8888);

	wl_shm_pool_destroy(pool);
	close(fd);

	if(app->buffer == NULL)
	{
		fprintf(stderr,"xway-lib: failed to create wl_buffer\n");
		munmap(app->pixels, app->buffer_size);
		app->pixels = NULL;
		return (-1);
	}

	return (0);
}

int wayland_init(struct app *app)
{
	app->display = wl_display_connect(NULL);
	if(app->display == NULL)
		return (-1);
	app->registry = wl_display_get_registry(app->display);
	if(app->registry == NULL)
		return (-1);
	if(wl_registry_add_listener(
		app->registry,
		&registry_listener,
		app) == -1)
		return (-1);

	if(wl_display_roundtrip(app->display) == -1)
		return (-1);

	if(app->compositor == NULL
		|| app->wm_base == NULL
		|| app->shm == NULL)
		return (-1);

	return (0);
}

int window_create(struct app *app)
{
	app->surface = wl_compositor_create_surface(app->compositor);
	if(app->surface == NULL)
		return (-1);
	app->xdg_surface = xdg_wm_base_get_xdg_surface(
		app->wm_base,
		app->surface);

	if(app->xdg_surface == NULL)
		return (-1);

	if(xdg_surface_add_listener(
		app->xdg_surface,
		&xdg_surface_listener,
		app) == -1)
		return (-1);

	app->toplevel = xdg_surface_get_toplevel(app->xdg_surface);
	if(app->toplevel == NULL)
		return (-1);

	if(xdg_toplevel_add_listener(
		app->toplevel,
		&xdg_toplevel_listener,
		app) == -1)
		return (-1);

	xdg_toplevel_set_title(app->toplevel, "Xway-lib");
	xdg_toplevel_set_app_id(app->toplevel, "xway-lib");

	app->running = 1;

	wl_surface_commit(app->surface);

	if(wl_display_roundtrip(app->display) == -1)
		return (-1);

	return (0);
}

void app_destroy(struct app *app)
{
	if(app == NULL)
		return ;
	if(app->buffer != NULL)
		wl_buffer_destroy(app->buffer);
	if(app->pixels)
		munmap(app->pixels,app->buffer_size);
	if(app->toplevel != NULL)
		xdg_toplevel_destroy(app->toplevel);
	if(app->xdg_surface != NULL)
		xdg_surface_destroy(app->xdg_surface);
	if(app->surface != NULL)
		wl_surface_destroy(app->surface);
	if(app->wm_base != NULL)
		xdg_wm_base_destroy(app->wm_base);
	if(app->shm != NULL)
		wl_shm_destroy(app->shm);
	if(app->compositor != NULL)
		wl_compositor_destroy(app->compositor);
	if(app->registry != NULL)
		wl_registry_destroy(app->registry);
	if(app->display != NULL )
		wl_display_disconnect(app->display);
}
