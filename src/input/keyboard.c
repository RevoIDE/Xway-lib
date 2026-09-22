#include "app.h"
#include "types.h"
#include "xway.h"

#include <linux/input-event-codes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>


static const t_xway_key g_linux_to_xway[XWAY_RAW_KEY_COUNT] = 
{
	[KEY_A] = XWAY_KEY_A,
	[KEY_B] = XWAY_KEY_B,
	[KEY_C] = XWAY_KEY_C,
	[KEY_D] = XWAY_KEY_D,
	[KEY_E] = XWAY_KEY_E,
	[KEY_F] = XWAY_KEY_F,
	[KEY_G] = XWAY_KEY_G,
	[KEY_H] = XWAY_KEY_H,
	[KEY_I] = XWAY_KEY_I,
	[KEY_J] = XWAY_KEY_J,
	[KEY_K] = XWAY_KEY_K,
	[KEY_L] = XWAY_KEY_L,
	[KEY_M] = XWAY_KEY_M,
	[KEY_N] = XWAY_KEY_N,
	[KEY_O] = XWAY_KEY_O,
	[KEY_P] = XWAY_KEY_P,
	[KEY_Q] = XWAY_KEY_Q,
	[KEY_R] = XWAY_KEY_R,
	[KEY_S] = XWAY_KEY_S,
	[KEY_T] = XWAY_KEY_T,
	[KEY_U] = XWAY_KEY_U,
	[KEY_V] = XWAY_KEY_V,
	[KEY_W] = XWAY_KEY_W,
	[KEY_X] = XWAY_KEY_X,
	[KEY_Y] = XWAY_KEY_Y,
	[KEY_Z] = XWAY_KEY_Z,

	[KEY_0] = XWAY_KEY_0,
	[KEY_1] = XWAY_KEY_1,
	[KEY_2] = XWAY_KEY_2,
	[KEY_3] = XWAY_KEY_3,
	[KEY_4] = XWAY_KEY_4,
	[KEY_5] = XWAY_KEY_5,
	[KEY_6] = XWAY_KEY_6,
	[KEY_7] = XWAY_KEY_7,
	[KEY_8] = XWAY_KEY_8,
	[KEY_9] = XWAY_KEY_9,

	[KEY_GRAVE] = XWAY_KEY_GRAVE,
	[KEY_MINUS] = XWAY_KEY_MINUS,
	[KEY_EQUAL] = XWAY_KEY_EQUAL,
	[KEY_LEFTBRACE] = XWAY_KEY_LEFT_BRACKET,
	[KEY_RIGHTBRACE] = XWAY_KEY_RIGHT_BRACKET,
	[KEY_BACKSLASH] = XWAY_KEY_BACKSLASH,
	[KEY_SEMICOLON] = XWAY_KEY_SEMICOLON,
	[KEY_APOSTROPHE] = XWAY_KEY_APOSTROPHE,
	[KEY_COMMA] = XWAY_KEY_COMMA,
	[KEY_DOT] = XWAY_KEY_PERIOD,
	[KEY_SLASH] = XWAY_KEY_SLASH,

	[KEY_ESC] = XWAY_KEY_ESCAPE,
	[KEY_ENTER] = XWAY_KEY_ENTER,
	[KEY_TAB] = XWAY_KEY_TAB,
	[KEY_BACKSPACE] = XWAY_KEY_BACKSPACE,
	[KEY_SPACE] = XWAY_KEY_SPACE,
	[KEY_CAPSLOCK] = XWAY_KEY_CAPS_LOCK,

	[KEY_LEFTSHIFT] = XWAY_KEY_LEFT_SHIFT,
	[KEY_RIGHTSHIFT] = XWAY_KEY_RIGHT_SHIFT,
	[KEY_LEFTCTRL] = XWAY_KEY_LEFT_CTRL,
	[KEY_RIGHTCTRL] = XWAY_KEY_RIGHT_CTRL,
	[KEY_LEFTALT] = XWAY_KEY_LEFT_ALT,
	[KEY_RIGHTALT] = XWAY_KEY_RIGHT_ALT,
	[KEY_LEFTMETA] = XWAY_KEY_LEFT_SUPER,
	[KEY_RIGHTMETA] = XWAY_KEY_RIGHT_SUPER,
	[KEY_COMPOSE] = XWAY_KEY_MENU,
	
	[KEY_INSERT] = XWAY_KEY_INSERT,
	[KEY_DELETE] = XWAY_KEY_DELETE,
	[KEY_HOME] = XWAY_KEY_HOME,
	[KEY_END] = XWAY_KEY_END,
	[KEY_PAGEUP] = XWAY_KEY_PAGE_UP,
	[KEY_PAGEDOWN] = XWAY_KEY_PAGE_DOWN,
	[KEY_LEFT] = XWAY_KEY_LEFT,
	[KEY_RIGHT] = XWAY_KEY_RIGHT,
	[KEY_UP] = XWAY_KEY_UP,
	[KEY_DOWN] = XWAY_KEY_DOWN,

	[KEY_SYSRQ] = XWAY_KEY_PRINT_SCREEN,
	[KEY_SCROLLLOCK] = XWAY_KEY_SCROLL_LOCK,
	[KEY_PAUSE] = XWAY_KEY_PAUSE,
	[KEY_NUMLOCK] = XWAY_KEY_NUM_LOCK,

	[KEY_F1] = XWAY_KEY_F1,
	[KEY_F2] = XWAY_KEY_F2,
	[KEY_F3] = XWAY_KEY_F3,
	[KEY_F4] = XWAY_KEY_F4,
	[KEY_F5] = XWAY_KEY_F5,
	[KEY_F6] = XWAY_KEY_F6,
	[KEY_F7] = XWAY_KEY_F7,
	[KEY_F8] = XWAY_KEY_F8,
	[KEY_F9] = XWAY_KEY_F9,
	[KEY_F10] = XWAY_KEY_F10,
	[KEY_F11] = XWAY_KEY_F11,
	[KEY_F12] = XWAY_KEY_F12,
	[KEY_F13] = XWAY_KEY_F13,
	[KEY_F14] = XWAY_KEY_F14,
	[KEY_F15] = XWAY_KEY_F15,
	[KEY_F16] = XWAY_KEY_F16,
	[KEY_F17] = XWAY_KEY_F17,
	[KEY_F18] = XWAY_KEY_F18,
	[KEY_F19] = XWAY_KEY_F19,
	[KEY_F20] = XWAY_KEY_F20,
	[KEY_F21] = XWAY_KEY_F21,
	[KEY_F22] = XWAY_KEY_F22,
	[KEY_F23] = XWAY_KEY_F23,
	[KEY_F24] = XWAY_KEY_F24,

	[KEY_KP0] = XWAY_KEY_KP_0,
	[KEY_KP1] = XWAY_KEY_KP_1,
	[KEY_KP2] = XWAY_KEY_KP_2,
	[KEY_KP3] = XWAY_KEY_KP_3,
	[KEY_KP4] = XWAY_KEY_KP_4,
	[KEY_KP5] = XWAY_KEY_KP_5,
	[KEY_KP6] = XWAY_KEY_KP_6,
	[KEY_KP7] = XWAY_KEY_KP_7,
	[KEY_KP8] = XWAY_KEY_KP_8,
	[KEY_KP9] = XWAY_KEY_KP_9,
	[KEY_KPDOT] = XWAY_KEY_KP_DECIMAL,
	[KEY_KPSLASH] = XWAY_KEY_KP_DIVIDE,
	[KEY_KPASTERISK] = XWAY_KEY_KP_MULTIPLY,
	[KEY_KPMINUS] = XWAY_KEY_KP_SUBTRACT,
	[KEY_KPPLUS] = XWAY_KEY_KP_ADD,
	[KEY_KPENTER] = XWAY_KEY_KP_ENTER,
	[KEY_KPEQUAL] = XWAY_KEY_KP_EQUAL,
	[KEY_KPCOMMA] = XWAY_KEY_KP_COMMA,

	[KEY_MUTE] = XWAY_KEY_VOLUME_MUTE,
	[KEY_VOLUMEDOWN] = XWAY_KEY_VOLUME_DOWN,
	[KEY_VOLUMEUP] = XWAY_KEY_VOLUME_UP,
	[KEY_PLAYCD] = XWAY_KEY_MEDIA_PLAY,
	[KEY_PAUSECD] = XWAY_KEY_MEDIA_PAUSE,
	[KEY_PLAYPAUSE] = XWAY_KEY_MEDIA_PLAY_PAUSE,
	[KEY_STOPCD] = XWAY_KEY_MEDIA_STOP,
	[KEY_PREVIOUSSONG] = XWAY_KEY_MEDIA_PREVIOUS,
	[KEY_NEXTSONG] = XWAY_KEY_MEDIA_NEXT,
	[KEY_REWIND] = XWAY_KEY_MEDIA_REWIND,
	[KEY_FASTFORWARD] = XWAY_KEY_MEDIA_FAST_FORWARD,

	[KEY_POWER] = XWAY_KEY_POWER,
	[KEY_SLEEP] = XWAY_KEY_SLEEP,
	[KEY_WAKEUP] = XWAY_KEY_WAKE_UP,

	[KEY_BRIGHTNESSDOWN] = XWAY_KEY_BRIGHTNESS_DOWN,
	[KEY_BRIGHTNESSUP] = XWAY_KEY_BRIGHTNESS_UP,

	[KEY_CALC] = XWAY_KEY_CALCULATOR,
	[KEY_MAIL] = XWAY_KEY_MAIL,
	[KEY_HOMEPAGE] = XWAY_KEY_BROWSER_HOME,
	[KEY_BACK] = XWAY_KEY_BROWSER_BACK,
	[KEY_FORWARD] = XWAY_KEY_BROWSER_FORWARD,
	[KEY_REFRESH] = XWAY_KEY_BROWSER_REFRESH,
	[KEY_SEARCH] = XWAY_KEY_BROWSER_SEARCH,
	[KEY_COMPUTER] = XWAY_KEY_COMPUTER,

};

static t_xway_key key_from_linux(uint32_t key)
{
	if(key >= XWAY_RAW_KEY_COUNT)
		return (XWAY_KEY_UNKNOWN);
	return (g_linux_to_xway[key]);
}

void xway_keyboard_cleanup(t_xway_app *app)
{
	if(!app)
		return;

	app->keyboard_focused = 0;
	memset(app->keys_down,0,sizeof(app->keys_down));

	if(app->keyboard)
	{
		if(wl_keyboard_get_version(app->keyboard)
				>= WL_KEYBOARD_RELEASE_SINCE_VERSION)
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

static void on_keyboard_keymap(
		void *data,
		struct wl_keyboard *keyboard,
		uint32_t format,
		int32_t fd,
		uint32_t size)
{
	t_xway_app	*app;
	char		*map;

	(void)keyboard;
	app = data;

	xkb_state_unref(app->xkb_state);
	app->xkb_state = NULL;
	xkb_keymap_unref(app->xkb_keymap);
	app->xkb_keymap = NULL;

	if	(app->xkb_state)
	{
		xkb_state_unref(app->xkb_state);
		app->xkb_state = NULL;
	}
	if	(app->xkb_keymap)
	{
		xkb_keymap_unref(app->xkb_keymap);
		app->xkb_keymap = NULL;
	}
	if(format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 
			|| size == 0
			|| !app->xkb_context)
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

static void on_keyboard_enter(

	void *data,
	struct wl_keyboard *keyboard,
	uint32_t serial,
	struct wl_surface *surface,
	struct wl_array *keys)
{
	t_xway_app *app;
	uint32_t *key;
	t_xway_key xway_key;

	(void)keyboard;
	(void)serial;

	app = data;

	if(surface != app->surface)
		return ;

	memset(app->keys_down, 0,sizeof(app->keys_down));
	app->keyboard_focused = 1;

	wl_array_for_each(key, keys)
	{
		xway_key = key_from_linux(*key);
		if(xway_key != XWAY_KEY_UNKNOWN)
			app->keys_down[xway_key] = 1;
	}

	fprintf(stderr, "xway-lib: keyboard focus entered\n");
}

static void on_keyboard_leave(
		void *data,
		struct wl_keyboard *keyboard,
		uint32_t  serial,
		struct wl_surface *surface)
{
	t_xway_app *app;
	uint8_t keys_down[XWAY_KEY_COUNT];
	t_xway_key key;

	(void)keyboard;
	(void)serial;

	app = data;

	if (surface != app->surface)
		return ;

	memcpy(keys_down, app->keys_down, sizeof(keys_down));
	app->keyboard_focused = 0;
	memset(app->keys_down,0, sizeof(app->keys_down));

	key = XWAY_KEY_UNKNOWN + 1;
	while	(key < XWAY_KEY_COUNT)
	{
		if(keys_down[key] && app->key_callback)
			app->key_callback(app, key,	XWAY_KEY_RELEASED,app->key_user_data);
		key++;
	}

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
	t_xway_key xway_key;
	xkb_keycode_t keycode;
	xkb_keysym_t keysym;
	char name[128];

	(void)keyboard;
	(void)serial;
	(void)time;

	app = data;

	if(!app->keyboard_focused)
		return ;

	xway_key = key_from_linux(key);
	if(xway_key == XWAY_KEY_UNKNOWN)
		return;

	if(state ==  WL_KEYBOARD_KEY_STATE_PRESSED)
		app->keys_down[xway_key] = 1;
	else if(state == WL_KEYBOARD_KEY_STATE_RELEASED)
			app->keys_down[xway_key] = 0;
	else
		return;

	if(app->key_callback)
	{
		if(state == WL_KEYBOARD_KEY_STATE_PRESSED)
			app->key_callback(app,xway_key,XWAY_KEY_PRESSED,app->key_user_data);
		else
			app->key_callback(app,xway_key,XWAY_KEY_RELEASED,app->key_user_data);
	}
	if(!app->xkb_state)
		return;

	keycode = key + 8;
	keysym = xkb_state_key_get_one_sym(app->xkb_state,keycode);

	if(xkb_keysym_get_name(keysym, name, sizeof(name)) <= 0)
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

	if(wl_keyboard_add_listener(app->keyboard,
				&keyboard_listener, app) == -1)
	{
		fprintf(stderr,"xway-lib: failed to add keyboard listener\n");
		xway_keyboard_cleanup(app);
		return (-1);
	}
	
	return (0);
}
