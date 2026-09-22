#ifndef XWAY_H
#define XWAY_H

#include <stdint.h>
typedef struct s_xway_app t_xway_app;


typedef struct s_xway_frame
{
	uint32_t	*pixels;
	int32_t		width;
	int32_t		height;
	int32_t		stride_bytes;
}	t_xway_frame;

#define X(name) name,

typedef enum e_xway_key
{
	#include "xway_keys.def"
	XWAY_KEY_COUNT
}	t_xway_key;

#undef X

typedef enum e_xway_key_action
{
	XWAY_KEY_RELEASED,
	XWAY_KEY_PRESSED
}	t_xway_key_action;

typedef void	(*t_xway_key_callback)(
		t_xway_app *app,
		t_xway_key key,
		t_xway_key_action action,
		void *user_data);

t_xway_app *xway_create(int width,int height,const char *title);

void xway_destroy(t_xway_app *app);

int xway_present(t_xway_app *app);
int	xway_dispatch(t_xway_app *app);

int xway_is_running(const t_xway_app *app);
int xway_wait_events(t_xway_app *app);
int xway_poll_events(t_xway_app *app);

int xway_request_frame(t_xway_app *app);
int xway_frame_ready(const t_xway_app *app);
int xway_wait_frame(t_xway_app *app);

int xway_get_frame(t_xway_app *app, t_xway_frame *frame);

void	xway_blit(t_xway_app *app, uint32_t *pixels);

int	xway_key_down(const t_xway_app *app, t_xway_key key);

const char *xway_key_name(t_xway_key key);

void xway_set_key_callback(t_xway_app *app,t_xway_key_callback callback,void *user_data);
#endif
