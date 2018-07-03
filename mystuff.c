#include <stdio.h>   /* fprintf */
#include <stdlib.h>  /* exit */
#include <string.h>  /* strlen */
#include <time.h>    /* strftime */

#include <nng/nng.h>

void nng_call(const char *funcname, int retcode)
{
	if (retcode == 0)
		return;
	fprintf(stderr, "%s: %s\n", funcname, nng_strerror(retcode));
	exit(1);
}

const char *now(const char *fmt)
{
	static char buf[64];
	time_t t = time(NULL);
	size_t n = strftime(buf, sizeof(buf), fmt ? fmt : "%Y-%m-%d %H:%M:%S", localtime(&t));
	return n ? buf : "BADTIME";
}

/* END */
