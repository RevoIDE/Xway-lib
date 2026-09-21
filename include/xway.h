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

t_xway_app *xway_create(int width,int height,const char *title);

void xway_destroy(t_xway_app *app);

int xway_present(t_xway_app *app);
int	xway_dispatch(t_xway_app *app);

int xway_is_running(const t_xway_app *app);
int xway_wait_events(t_xway_app *app);
int xway_get_frame(t_xway_app *app, t_xway_frame *frame);

void	xway_blit(t_xway_app *app, uint32_t *pixels);

int	xway_key_down(const t_xway_app *app, t_xway_key key);

const char *xway_key_name(t_xway_key key);
#endif
