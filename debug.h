/*
 * debug.h
 * 	Defines one debugging output function.
 * 	Behavior depends on whether the preprocessor constant DEBUG is defined
 * 	at compile-time.
 * 	If it is defined,     it is set to 1, and debug() will print to stderr.
 * 	If it is not defined, it is set to 0, and debug() will do nothing.
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <stdarg.h>

#ifdef DEBUG
  #define DEBUG 1
#else
  #define DEBUG 0
#endif

void debug(char *fmt, ...);

#endif
