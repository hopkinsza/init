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
LDFLAGS = -lrt


.PHONY: all clean

$(PROG):
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(OBJS) -o $@

all: $(PROG)

$(PROG): $(OBJS)

$(OBJS): debug.h

clean:
	@rm -f $(PROG) *.o
	@echo cleaned
