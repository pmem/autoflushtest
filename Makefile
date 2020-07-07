#
# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright 2020, Intel Corporation. All rights reserved.
#
# Makefile for autoflush test
#
PROGS = autoflushwrite autoflushcheck
CFLAGS = -Wall -Werror -std=gnu99

all: $(PROGS)

autoflushwrite: autoflushwrite.o
	$(CC) -o $@ $(CFLAGS) $^ $(LIBS)

autoflushcheck: autoflushcheck.o
	$(CC) -o $@ $(CFLAGS) $^ $(LIBS)

clean:
	$(RM) *.o a.out core


clobber: clean
	$(RM) $(PROGS)
