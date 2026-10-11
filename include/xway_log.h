#ifndef XWAY_LOG_H
#define XWAY_LOG_H

#if defined(XWAY_DEBUG) && XWAY_DEBUG

#include <stdio.h>

#define XWAY_LOG_DEBUG(...)                                                            \
	do                                                                                 \
	{                                                                                  \
		fprintf(stderr, "[xway][debug] ");                                             \
		fprintf(stderr, __VA_ARGS__);                                                  \
		fputc('\n', stderr);                                                           \
	} while (0)

#else

#define XWAY_LOG_DEBUG(...)                                                            \
	do                                                                                 \
	{                                                                                  \
	} while (0)

#endif

#endif
