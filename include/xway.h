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

/**
 * @brief Create an application and its Wayland window.
 *
 * Connect to the compositor,	creates the window and allocates the initial framebuffer.
 * @param width Initial window in pixels.Must be greater than zero.
 * @param height Initial window height in pixels.Must be greater than zero.
 * @param title Window title.Must not be NULL
 *
 * @return A pointer to the created application, or NULL on failure
 */
t_xway_app *xway_create(int width,int height,const char *title);

/**
 * @brief Destroys an Xway application and releases its resources.
 * @param app Application to destroy. May be NULL.
 */
void xway_destroy(t_xway_app *app);
/**
 * @brief Presents the current framebuffer to the Wayland compositor.
 *
 * Requests the next frame callback, attaches the current buffer to
 * the surface and commits its contents.
 *
 * @param app application whose framebuffer will be presente.
 *
 * @return 0 on success,on -1 on failure.
 */
int xway_present(t_xway_app *app);
/**
 * @brief Dispatches Wayland events and waits for new events if necessary.
 *
 * This function is blocking and should generally not be used in a continuously updating engine loop.
 *
 * @param app Application whose events will be dispatched. Must not be NULL.
 *
 * @return The result returned by wl_display_dispatch(), or -1 on failure.
 */
int	xway_dispatch(t_xway_app *app);
/**
 * @brief Checks wheter the application is still running.
 *
 * @param app Application to query.
 *
 * @return Non-zero while the application is running,otherwise zero.
 */
int xway_is_running(const t_xway_app *app);
/**
 * @brief Waits for and processes Wayland events.
 *
 * This function blocks until at least one event is received. If the Wayland connection fails, the application is marked as stopped.
 *
 * @param app Application whose events will be processed.
 *
 * @return 0 on success, or -1 on failure.
 */
int xway_wait_events(t_xway_app *app);

/**
 * @brief Processes available Wayland events without blocking.
 *
 * This functions reads and dispatchesa events already available from
 * Wayland connection and returned immediately when no event is pending.It is intended to be called ONCE during
 * each engine update.
 *
 * @param app Application whose events will be processed.
 *
 * @return 0 on success, or -1 on failure.
 */
int xway_poll_events(t_xway_app *app);
/**
 * @brief Requests notification when the compositor is ready for a new frame.
 *
 * If a frame callback is already pending, this function does nothing and reports success.
 *
 * @param app Application requesting the frame notification.
 *
 * @return 0 on success, or -1 on failure.
 */
int xway_request_frame(t_xway_app *app);
/**
 * @brief Checks wheter the compositor is ready for a new frame.
 *
 * @param app Application to query.
 *
 * @return Non-zero if a new frame may be presented, otherwise zero.
 */
int xway_frame_ready(const t_xway_app *app);

/**
 * @brief Waits until the compositor is ready for a new frame.
 *
 * Wayland events are dispatched while waiting. This function is blocking.
 *
 * @param app Application waiting for a frame notification.
 *
 * @return 0 on success, or -1 if event processing fails.
 */
int xway_wait_frame(t_xway_app *app);

/**
 * @brief Retrieves the current software framebuffer.
 *
 * The returned pixel memory is owned by xway and must NOT be freed by caller.Its contents may be modified by the calller before ths frame is presented.
 *
 * @param app Application containning the framebuffer
 * @param frame Output structure receiving	 the framebuffer information.
 *
 * @return 0 on success, or -1 arguments or framebuffer are invalid.
 */
int xway_get_frame(t_xway_app *app, t_xway_frame *frame);

/**
 * @brief Copies an external pixel array into the current framebuffer.
 * The source must contain at least width multiplied by height pixels
 * in 32-bit XRGB8888 format.
 *
 * @param app Application containning the destination framebuffer.
 * @param pixels pixels Source pixels array.
*/

void	xway_blit(t_xway_app *app, uint32_t *pixels);

/**
 *
 * @brief Checks wheter a keyboard key is currently held down.
 *
 * The function returns zero when the application has no keyboard focus.
 *
 * @param app Application whose keyboard state will be queried
 * @param key Xway phyisical key identifier.
 *
 * @return Non-zero if the key is held down,otherwise zero.
 */

int	xway_key_down(const t_xway_app *app, t_xway_key key);

/**
 * @brief Returns the readable name of an Xway key.
 *
 * The returned string is owned by Xway and must not be freed or modified.
 *
 * @param key Xway key identifier.
 *
 * @return A constant key name, or "XWAY_KEY_UNKNOWN" for an invalid key.
 */

const char *xway_key_name(t_xway_key key);

/**
 * @brief Registers the keyboard event callback
 *
 * The callback is invoked for key presses, key releases and synthetic
 * releases generated when the window loses keyboard focus.
 *
 * Passing NULL as the callback disables keyboard notifications.THe user data pointer is stored without taking ownership of it.
 *
 * @param app Application receiving keyboard events.
 * @param callback function called for each keyboard event, or NULL.
 * @param user_data User-defined pointer passed to the callback.
 */

void xway_set_key_callback(t_xway_app *app,t_xway_key_callback callback,void *user_data);
#endif
