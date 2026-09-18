#include "app.h"
#include "types.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>
#include <sys/mman.h>


void xway_keyboard_cleanup(t_xway_app *app)
{
	if(!app)
		return;
	
	if(app->keyboard)
	{
		if(wl_keyboard_get_version(app->keyboard) >= WL_KEYBOARD_RELEASE_SINCE_VERSION)
			wl_keyboard_release(app->keyboard);
		else
			wl_keyboard_destroy(app->keyboard);
		app->keyboard = NULL;
	}
	if(app->xkb_state)
	{
		xkb_state_unref(app->xkb_state);
		app->xkb_state = NULL;
	}
	if(app->xkb_keymap)
	{
		xkb_keymap_unref(app->xkb_keymap);
		app->xkb_keymap = NULL;
	}
	if(app->xkb_context)
	{
		xkb_context_unref(app->xkb_context);
		app->xkb_context = NULL;
	}
}

static void on_keyboard_keymap(void *data,struct wl_keyboard *keyboard, uint32_t format,int32_t fd, uint32_t size)
{
	t_xway_app *app;
	char *map;

	(void)keyboard;

	app = data;

	xkb_state_unref(app->xkb_state);
	app->xkb_state = NULL;
	xkb_keymap_unref(app->xkb_keymap);
	app->xkb_keymap = NULL;

	if(format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 || size == 0 || !app->xkb_context)
	{
		close(fd);
		return;
	}
	map = mmap(NULL, size, PROT_READ, MAP_PRIVATE,fd, 0);
	close(fd);
	if(map == MAP_FAILED)
	{
		fprintf(stderr, "xway-lib:failed to map keymap\n");
		return;
	}
	if(map[size - 1] != '\0')
	{
		fprintf(stderr, "xway-lib: invalid keymap string\n");
		munmap(map, size);
		return;
	}
	app->xkb_keymap = xkb_keymap_new_from_string(
			app->xkb_context,
			map,
			XKB_KEYMAP_FORMAT_TEXT_V1,
			XKB_KEYMAP_COMPILE_NO_FLAGS);
	munmap(map,size);

	if(!app->xkb_keymap)
	{
		fprintf(stderr,"xway-lib:failed to create XKB keymap\n");
		return;
	}
	app->xkb_state = xkb_state_new(app->xkb_keymap);
	if(!app->xkb_state)
	{
		fprintf(stderr, "xway-lib: failed to create XKB state\n");
		xkb_keymap_unref(app->xkb_keymap);
		app->xkb_keymap = NULL;
		return;
	}
	fprintf(stderr, "xway-lib: XKB keymap loaded\n");
}

static void on_keyboard_enter(void *data,struct wl_keyboard *keyboard,uint32_t serial,struct wl_surface *surface,struct wl_array *keys)

{
	(void)data;
	(void)keyboard;
	(void)serial;
	(void)surface;
	(void)keys;

	fprintf(stderr, "xway-lib: keyboard focuse entered\n");
}
static void on_keyboard_leave(void *data,struct wl_keyboard *keyboard,uint32_t  serial,struct wl_surface *surface)
{
	(void)data;
	(void)keyboard;
	(void)serial;
	(void)surface;

	fprintf(stderr, "xway-lib: keyboard focus left\n");
}

static void on_keyboard_key(
		void *data,
		struct wl_keyboard *keyboard,
		uint32_t serial,
		uint32_t time,
		uint32_t key,
		uint32_t state)
{
	t_xway_app *app;
	xkb_keycode_t keycode;
	xkb_keysym_t keysym;
	char name[128];

	(void)keyboard;
	(void)serial;
	(void)time;

	app = data;

	if(!app->xkb_state)
		return;
	keycode = key + 8;

	keysym = xkb_state_key_get_one_sym(app->xkb_state,keycode);

	if(xkb_keysym_get_name(keysym, name, keycode) <= 0)
		snprintf(name, sizeof(name), "unknown");
	if(state == WL_KEYBOARD_KEY_STATE_PRESSED)
		fprintf(stderr, "xway-lib: key=%u symbol=%s pressed\n",(unsigned int)key, name);
	else if(state == WL_KEYBOARD_KEY_STATE_RELEASED)
		fprintf(stderr, "xway-lib: key=%u symbol=%s released\n",(unsigned int)key,name);

}
static void on_keyboard_modifiers(
		void *data,
		struct wl_keyboard *keyboard,
		uint32_t serial,
		uint32_t mods_depressed,
		uint32_t mods_latched,
		uint32_t modsa_locked,
		uint32_t group)
{
	t_xway_app *app;

	(void)keyboard;
	(void)serial;

	app = data;

	if(!app->xkb_state)
		return ;

	xkb_state_update_mask(
			app->xkb_state,
			mods_depressed,
			mods_latched,
			modsa_locked,
			0,
			0,
			group);
}
static void on_keyboard_repeat_info(
		void *data,
		struct wl_keyboard *keyboard,
		int32_t rate,
		int32_t delay)

{
	(void)data;
	(void)keyboard;
	(void)rate;
	(void)delay;
}
static const struct wl_keyboard_listener keyboard_listener = {
	.keymap = on_keyboard_keymap,
	.enter = on_keyboard_enter,
	.leave = on_keyboard_leave,
	.key = on_keyboard_key,
	.modifiers = on_keyboard_modifiers,
	.repeat_info = on_keyboard_repeat_info,
};

int xway_keyboard_create(t_xway_app *app)
{
	if(!app || !app->seat)
		return (-1);
	if(app->keyboard)
		return (0);

	app->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if(!app->xkb_context)
	{
		fprintf(stderr, "xway-lib:failed to create XKB context\n");
		return (-1);
	}
	app->keyboard = wl_seat_get_keyboard(app->seat);
	if(!app->keyboard)
	{
		fprintf(stderr, "xway-lib: failed to get keyboard\n");
		xway_keyboard_cleanup(app);
		return (-1);
	}
	if(wl_keyboard_add_listener(app->keyboard, &keyboard_listener, app) == -1)
	{
		fprintf(stderr,"xway-lib: failed to add keyboard listener\n");
		xway_keyboard_cleanup(app);
		return (-1);
	}
	return (0);
}
