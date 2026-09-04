#ifndef ERR_H
# define ERR_H

# include <stdlib.h>
# include <stdio.h>

/* ANSI colors */
# define RESET   "\033[0m"
# define RED     "\033[31m"
# define YELLOW  "\033[33m"
# define GREEN   "\033[32m"
# define CYAN    "\033[36m"

/* Messages */
# define ERROR(msg) \
    do { \
        fprintf(stderr, RED "Error: " RESET "%s\n", msg); \
        exit(EXIT_FAILURE); \
    } while (0)

# define WARN(msg) \
    do { \
        fprintf(stderr, YELLOW "Warning: " RESET "%s\n", msg); \
    } while (0)

# define INFO(msg) \
    do { \
        fprintf(stdout, CYAN "Info: " RESET "%s\n", msg); \
    } while (0)

#endif
