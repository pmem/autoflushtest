/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020, Intel Corporation. All rights reserved.
 *
 * autoflushwrite.c -- test flush-on-fail persistent CPU caches
 *
 * usage:
 *	autoflushwrite filename [filesize]
 *
 *	Where filename is a DAX file this program will create.  filename
 *	must not exist already.  After mapping the file and warming up
 *	the cache, the program will loop forever writing data to the cache
 *	until power is removed.  Use autoflushcheck to verify results
 *	after reboot.
 *
 *	If the file size is not specified, a default of 20M is used.
 */

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <x86intrin.h>

/* allow building with older Linux header files */
#ifndef MAP_SYNC
#define MAP_SYNC 0x80000
#endif

#ifndef MAP_SHARED_VALIDATE
#define MAP_SHARED_VALIDATE 0x03
#endif

/*
 * 20 megs fits in the cache and is still a non-trivial
 * amount of data to test flush-on-fail
 */
#define MB (1<<20)
#define DEFAULT_FILESIZE (20 * MB)

/*
 * first page is used to store iteration number with deep flush.
 * rest of file is then filled up with iteration number, written
 * once per 64-byte cache line and not flushed.
 */

int
main(int argc, char *argv[])
{
        int fd;
        char *pmaddr;
	unsigned int filesize;
	unsigned int nstore;

        if (argc < 1 || argc > 3) {
                fprintf(stderr, "Usage: %s filename [filesize]\n", argv[0]);
                exit(1);
        }

        if ((fd = open(argv[1], O_RDWR|O_CREAT|O_EXCL, 0644)) < 0)
                err(1, "open: %s", argv[1]);

	if (argv[2] != NULL)
		filesize = atoi(argv[2]) * MB;
	else
		filesize = DEFAULT_FILESIZE;

	printf("Filesize entered is %d",filesize);

        if (ftruncate(fd, (off_t)filesize) < 0)
                err(1, "ftruncate: %s", argv[1]);

        if ((pmaddr = (char *)mmap(NULL, (off_t)filesize, PROT_READ|PROT_WRITE,
                                MAP_SHARED_VALIDATE|MAP_SYNC,
                                fd, 0)) == MAP_FAILED)
                err(1, "mmap(MAP_SYNC): %s", argv[1]);

	nstore = (filesize - 0x1000) / 64;
        close(fd);

	for (unsigned iteration = 0; 1; iteration++) {
		/* store iteration number in first page and deep flush it */
		*(unsigned *)pmaddr = iteration;
		_mm_clflush(pmaddr);
		if (msync(pmaddr, 0x1000, MS_SYNC) < 0)
			err(1, "msync");

		/* store iteration number in each cache line and don't flush */
		for (int i = 0; i < nstore; i++)
			*(unsigned *)(pmaddr + 0x1000 + 64 * i) = iteration;

		if (iteration == 2) {
			printf("%s: stores running, ready for power fail...\n",
					argv[0]);
			fflush(stdout);
		}
	}
}
