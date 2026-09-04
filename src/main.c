#include <stdio.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-util.h>
int	main(void)
{
	struct wl_display *display;
	struct wl_registry *registry;

	display = wl_display_connect(NULL);
	if(display == NULL)
		return (1);
	registry = wl_display_get_registry(display);

	wl_registry_destroy(registry);
	wl_display_disconnect(display);
	return (0) ;
}
