
#include <stdio.h>
#include <stdlib.h>

/*
 * hallocate
 *
 *	Allocate some memory, barfing if malloc returns NULL.
 */
char *
hallocate( unsigned int size )
{
	char	*p;

	if ((p = (char *)malloc(size)) == (char *)NULL) {
		fprintf(stderr,"hallocate: request for %d bytes returned NULL", size);
		exit(1);
	}

	return (p);
}
