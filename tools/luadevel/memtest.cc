#include <stdio.h>
#include <stdlib.h>

#define LIBPBD_API
//#define  RAP_WITH_SEGMENT_STATS
#define  RAP_WITH_CALL_STATS
#include "../../libs/pbd/pbd/reallocpool.h"
#include "../../libs/pbd/reallocpool.cc"

int main (int argc, char **argv)
{
#if 0
	PBD::ReallocPool *m = new PBD::ReallocPool("TestPool", 64);
	x[0] = m->malloc(10);
	x[1] = m->malloc(5);
	m->free (x[0]);
	x[2] = m->malloc(5);
	x[3] = m->malloc(5);
	m->printstats();
	m->dumpsegments();
	m->free (x[1]);
	m->free (x[2]);
	m->free (x[3]);
#else

#if 1
#define MALLOC  m->malloc
#define FREE  m->free
#else
#define MALLOC  malloc
#define FREE  free
#endif

	srand (0);
	PBD::ReallocPool *m = new PBD::ReallocPool("TestPool", 64*1024);
	void *x[16];

	for (int l = 0; l < 8 * 1024 * 1024  ; ++l) {
		size_t s[16];
		for (int i = 0; i < 16; ++i) {
			s[i] = rand() % 1024;
			x[i] = MALLOC (s[i]);
		}
		for (int i = 0; i < 16; ++i) {
			if (x[i]) {
				memset (x[i], 0xff, s[i]);
			}
		}
		for (int i = 0; i < 16; ++i) {
			FREE (x[i]);
		}
	}
#endif

	delete (m);
}
