#
# SPDX-License-Identifier: 0BSD
#

PROG = init
SRCS = debug.c init.c
OBJS = $(SRCS:.c=.o)

CFLAGS = -Wall
CPPFLAGS =
# Uncomment for debugging output:
#CPPFLAGS += -DDEBUG
LDLIBS = -lrt -lpthread


$(PROG): $(OBJS)

all: $(PROG)

$(OBJS): debug.h

clean:
	@rm -f $(PROG) *.o
	@echo cleaned

.PHONY: all clean
