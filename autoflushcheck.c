/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2020, Intel Corporation. All rights reserved.
 *
 * autoflushcheck.c -- verify autoflushwrite test passed
 *
 * usage:
 *	autoflushcheck filename
 *
 *	Where filename is a DAX file created by autoflushwrite.
 */

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
        int fd;
        char *pmaddr;
	struct stat stbuf;

        if (argc != 2) {
                fprintf(stderr, "Usage: %s filename\n", argv[0]);
                exit(1);
        }

        if ((fd = open(argv[1], O_RDONLY)) < 0)
                err(1, "open: %s", argv[1]);

	if (fstat(fd, &stbuf) < 0)
		err(1, "fstat: %s", argv[1]);

        if ((pmaddr = (char *)mmap(NULL, stbuf.st_size, PROT_READ,
                                MAP_SHARED,
                                fd, 0)) == MAP_FAILED)
                err(1, "mmap: %s", argv[1]);

        close(fd);

	unsigned iteration = *(unsigned *)pmaddr;
	int nstore = (stbuf.st_size - 0x1000) / 64;

	printf("iteration from file header: 0x%x\n", iteration);
	printf("           stores to check: %d\n", nstore);
	printf("           starting offset: 0x%x\n", 0x1000);
	printf("             ending offset: 0x%x\n",
			0x1000 + 64 * (nstore - 1));
	int found = 0;	/* true if we've found where the last loop ended */

	for (int i = 0; i < nstore; i++) {
		unsigned offset = 0x1000 + 64 * i;
		unsigned val = *(unsigned *)(pmaddr + offset);
		if (val < iteration - 1)
			errx(1, "FAIL: offset 0x%x too low (0x%x)",
					offset, val);
		if (val > iteration)
			errx(1, "FAIL: offset 0x%x too high (0x%x)",
					offset, val);
		if (found) {
			/*
			 * all stores should be equal to iteration - 1
			 * from here on
			 */
			if (val != iteration - 1)
				errx(1, "FAIL: offset 0x%x found 0x%x, "
						"expected 0x%x",
						offset, val, iteration);
		} else if (val == iteration - 1) {
			printf("          end of iteration: "
				"offset 0x%x (store %d)\n", offset, i);
			found = 1;
		} else if (val != iteration) {
			errx(1, "FAIL: offset 0x%x found 0x%x, "
						"expected 0x%x",
						offset, val, iteration - 1);
		}
	}

	printf("PASS\n");
	exit(0);
}
