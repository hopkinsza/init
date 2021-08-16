#include "debug.h"

#if DEBUG == 1
	#include <stdio.h>

	void
	debug(char *fmt, ...)
	{
		va_list ap;

		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
#else
	void
	debug(char *fmt, ...)
	{
		return;
	}
#endif
