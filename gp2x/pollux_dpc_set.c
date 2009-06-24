/*
 * quick tool to set LCD timings for Wiz
 * (c) notaz, 2009
 * code dedicated to public domain.
 *
 * HTOTAL:    X VTOTAL:  341
 * HSWIDTH:   1 VSWIDTH:   0
 * HASTART:  37 VASTART:  17
 * HAEND:   277 VAEND:   337
 *
 * 120Hz
 * pcd  8, 447: + 594us
 * pcd  9, 397: +  36us
 * pcd 10, 357: - 523us
 * pcd 11, 325: +1153us
 *
 * 'lcd_timings=397,1,37,277,341,0,17,337;clkdiv0=9'
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pollux_dpc_set.h"

/*
 * set LCD timings based on preformated string (see usage() below for params).
 * returns 0 on success.
 */
int pollux_dpc_set(volatile unsigned short *memregs, const char *str)
{
	int timings[8], have_timings = 0;
	int pcd = 0, have_pcd = 0;
	const char *p;
	int i, ret;

	if (str == NULL)
		return -1;

	p = str;
	while (1)
	{
		if (strncmp(p, "lcd_timings=", 12) == 0)
		{
			int c;
			p += 12;
			ret = sscanf(p, "%d,%d,%d,%d,%d,%d,%d,%d",
				&timings[0], &timings[1], &timings[2], &timings[3],
				&timings[4], &timings[5], &timings[6], &timings[7]);
			if (ret != 8)
				break;
			/* skip seven commas */
			for (c = 0; c < 7 && *p != 0; p++)
				if (*p == ',')
						c++;
			if (c != 7)
				break;
			/* skip last number */
			while ('0' <= *p && *p <= '9')
				p++;
			have_timings = 1;
		}
		else if (strncmp(p, "clkdiv0=", 8) == 0)
		{
			char *ep;
			p += 8;
			pcd = strtoul(p, &ep, 10);
			if (p == ep)
				break;
			p = ep;
			have_pcd = 1;
		}
		else
			break;

		while (*p == ';' || *p == ' ')
			p++;
		if (*p == 0)
			goto parse_done;
	}

	fprintf(stderr, "dpc_set parser: error at '%s'\n", p);
	return -1;

parse_done:
	/* some validation */
	if (have_timings)
	{
		for (i = 0; i < 8; i++)
			if (timings[i] & ~0xffff) {
				fprintf(stderr, "dpc_set: invalid timing %d: %d\n", i, timings[i]);
				return -1;
			}
	}

	if (have_pcd)
	{
		if ((pcd - 1) & ~0x3f) {
			fprintf(stderr, "dpc_set: invalid clkdiv0: %d\n", pcd);
			return -1;
		}
	}

	/* write */
	if (have_timings)
	{
		for (i = 0; i < 8; i++)
			memregs[(0x307c>>1) + i] = timings[i];
	}

	if (have_pcd)
	{
		int tmp;
		pcd = (pcd - 1) & 0x3f;
		tmp = memregs[0x31c4>>1];
		memregs[0x31c4>>1] = (tmp & ~0x3f0) | (pcd << 4);
	}

	return 0;
}

#ifdef BINARY
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static void usage(const char *binary)
{
	printf("usage:\n%s <set_str[;set_str[;...]]>\n"
		"set_str:\n"
		"  lcd_timings=<htotal,hswidth,hastart,haend,vtotal,vswidth,vastart,vaend>\n"
		"  clkdiv0=<divider>\n", binary);
}

int main(int argc, char *argv[])
{
	volatile unsigned short *memregs;
	int ret, memdev;

	if (argc != 2) {
		usage(argv[0]);
		return 1;
	}

	memdev = open("/dev/mem", O_RDWR);
	if (memdev == -1)
	{
		perror("open(/dev/mem) failed");
		return 1;
	}

	memregs	= mmap(0, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED, memdev, 0xc0000000);
	if (memregs == MAP_FAILED)
	{
		perror("mmap(memregs) failed");
		close(memdev);
		return 1;
	}

	ret = pollux_dpc_set(memregs, argv[1]);

	munmap((void *)memregs, 0x10000);
	close(memdev);

	return ret;
}
#endif
