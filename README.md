# Xway-Lib

Xway-Lib is a small C11 library for creating a native Wayland window and drawing into a software framebuffer.

The project currently supports:

- Wayland window creation and resizing
- XRGB8888 software framebuffer rendering
- Keyboard state and callbacks
- Mouse position and button state
- Relative move movement and pointer capture
- Blocking and non-blocking event processing
- Compositor frame callbacks

> Xway-Lib is under development and may still change.

## Requirements

- A Linux Wayland session
- A C11 compiler
- CMake 3.20 or newer
- `pkg-config`
- `wayland-client`
- `wayland-protocols`
- `wayland-scanner`
- `xkbcommon`

## Build

```sh
cmake -S . -B build
cmake --build build --parallel
```
The build produces:

- `build/libxway.a`: the static library
- `build/xway-main`: the example application

Run the example with:

```sh
./build/xway-main
```

Run the example with:

## Minimal example

```c
#include <stdlib.h>

#include "xway.h"

int main(void)
{
	t_xway_app	*app;

	app = xway_create(800, 600, "Xway example");
	if (!app)
		return (EXIT_FAILURE);
	if (xway_present(app) == -1)
	{
		xway_destroy(app);
		return (EXIT_FAILURE);
	}
	while (xway_is_running(app))
	{
		if (xway_wait_events(app) == -1)
			break;
	}
	xway_destroy(app);
	return (EXIT_SUCCESS);
}
```

## Limitations

- Linux and Wayland only
- A single software framebuffer is used
- No installation target or stable package is provided yet
- Pointer capture requires compositor support for the relative-pointer and pointer-constraints protocols

## License

This project is licensed under the MIT License. see [LICENSE](LICENSE).
