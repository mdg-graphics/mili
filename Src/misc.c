/* $Id$ */
/*
 * misc.c - Miscellaneous routines.
 *
 *  	Donald J. Dovey
 * 	Lawrence Livermore National Laboratory
 *     	Oct 24 1991
 *
 * Copyright (c) 1991 Regents of the University of California
 */

#ifdef __hpux
#define NO_GETRUSAGE
#endif
#ifdef cray
#define NO_GETRUSAGE
#endif

#ifndef NO_GETRUSAGE
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include "misc.h"

static int _mem_total = 0;


/*****************************************************************
 * TAG( my_calloc )
 * 
 * Calloc instrumented to keep track of allocations.
 */
char *
my_calloc( cnt, size, descr )
int cnt;
int size;
char *descr;
{
    char *arr;

    _mem_total += cnt*size;

#ifdef DEBUG_MEM
    wrt_text( "Allocating memory for %s: %d bytes, total %d K.\n",
              descr, cnt*size, _mem_total/1024 ); 
#endif DEBUG_MEM

    arr = (char *)calloc( cnt, size );
    if ( arr == NULL )
        popup_fatal( "Can't allocate any more memory.\n" );

    return arr;
}


/************************************************************
 * TAG( timing_vals )
 *
 * A shell that calls get_rusage to get timing information.
 * This is a helper function for get_timing().
 */
void
timing_vals( ut, st, maxrs, rs, minpf, majpf, ns, in, out )
float *ut, *st;
int *maxrs, *rs, *minpf, *majpf, *ns, *in, *out;
{
#ifndef NO_GETRUSAGE
    int err;
    struct rusage usage;
 
    err = getrusage( RUSAGE_SELF, &usage );
 
    if( err != -1 )                         
    {
        *ut = usage.ru_utime.tv_sec + 
              usage.ru_utime.tv_usec*1.0e-6;   
        *st = usage.ru_stime.tv_sec +
              usage.ru_stime.tv_usec*1.0e-6;   
        *maxrs = usage.ru_maxrss;
        *rs    = usage.ru_idrss;
        *minpf = usage.ru_minflt;
        *majpf = usage.ru_majflt;
        *ns    = usage.ru_nswap;
        *in    = usage.ru_inblock;
        *out   = usage.ru_oublock;
    }
    else
        popup_dialog( WARNING_POPUP, "Error encountered in getrusage() call." );
#else
    *ut = 0.0;
    *st = 0.0;
    *maxrs = 0.0;
    *rs    = 0.0;
    *minpf = 0.0;
    *majpf = 0.0;
    *ns    = 0.0;
    *in    = 0.0;
    *out   = 0.0;
#endif
}          


/************************************************************
 * TAG( check_timing )
 *
 * Print program timing data for the user.  Called with a "0"
 * to start the timing interval and with a "1" to end the
 * the interval and print out timing information.
 */
void
check_timing( end_flag )
int end_flag;
{
    int i, err;
    static float ut1, st1, ut2, st2;
    static int maxrs1, maxrs2, rs1, rs2, minpf1, minpf2, majpf1, majpf2;
    static int ns1, ns2, in1, in2, out1, out2;
    
#ifdef NO_GETRUSAGE
    return;
#endif
 
    if ( end_flag == 0 )
        timing_vals( &ut1, &st1, &maxrs1, &rs1, &minpf1, &majpf1,
                     &ns1, &in1, &out1 );
    else
    {   
        timing_vals( &ut2, &st2, &maxrs2, &rs2, &minpf2, &majpf2,
                     &ns2, &in2, &out2 );
        wrt_text( "User CPU Time [seconds]                 : %e\n", ut2-ut1 );
        wrt_text( "System CPU Time [seconds]               : %e\n", st2-st1 );
        wrt_text( "Maximum Resident Set Size [pages]       : %d, %d\n",
                  maxrs1, maxrs2 );
        wrt_text( "Memory Residency Integral [page-seconds]: %d\n", rs2-rs1 );
        wrt_text( "Minor Page Faults [no I/O required]     : %d\n",
                  minpf2-minpf1 );
        wrt_text( "Major Page Faults [   I/O required]     : %d\n",
                  majpf2-majpf1 );
        wrt_text( "Physical Read Requests Honored          : %d\n", in2-in1 );
        wrt_text( "Physical Write Requests Honored         : %d\n", out2-out1 );
    }
}


/*****************************************************************
 * TAG( str_dup )
 *
 * Duplicate a string with space allocation. Return length of
 * string duplicated.
 */
int
str_dup( char **dest, char *src )
{
    int len;
    char *tmp, *pd, *ps;

    for ( ps = src, len = 0; *ps; len++, ps++ );
    
    *dest = (char *) NEW_N( char, len + 1, "String duplicate" );

    pd = *dest;
    ps = src;
    while ( *pd++ = *ps++ );

    return len;
}


/*****************************************************************
 * TAG( all_true )
 *
 * Returns TRUE if all elements of a Bool_type array are TRUE.
 */
Bool_type
all_true( boola, qty )
Bool_type boola[];
int qty;
{
    Bool_type accum;
    Bool_type *p_test, *bound;
    
    for ( p_test = boola, bound = boola + qty, accum = TRUE; 
          p_test < bound; 
	  accum &= *p_test++ ); /* Bitwise! True's need a 1-bit in common. */
    
    return accum;
}


/*****************************************************************
 * TAG( all_false )
 *
 * Returns TRUE if all elements of a Bool_type array are FALSE.
 */
Bool_type
all_false( boola, qty )
Bool_type boola[];
int qty;
{
    Bool_type accum;
    Bool_type *p_test, *bound;
    
    for ( p_test = boola, bound = boola + qty, accum = FALSE; 
          p_test < bound; 
	  accum |= *p_test++ );
    
    return !accum;
}


/*****************************************************************
 * TAG( is_in_iarray )
 *
 * Returns TRUE if candidate integer is contained in an array
 * of integers.
 */
Bool_type
is_in_iarray( candidate, size, iarray )
int candidate;
int size;
int *iarray;
{
    int *p_int, *bound;
    
    bound = iarray + size;
    for ( p_int = iarray; p_int < bound; p_int++ )
	if ( candidate == *p_int )
	    break;
    
    return ( p_int != bound );
}


/*****************************************************************
 * TAG( find_text )
 *
 * Search an open text file on a line-by-line basis to find a
 * line containing the passed text string.  Return a pointer to
 * the first character of the string in the line of text.
 *
 * The line of text is only guaranteed to exist until the next
 * call to find_text().
 */
 
#define TXT_BUF_SIZE (256)

char *
find_text( ifile, p_text )
FILE *ifile;
char *p_text;
{
    static char txtline[TXT_BUF_SIZE];
    char *p_str, *p_line;
    
    do
	p_line = fgets( txtline, TXT_BUF_SIZE, ifile );
    while ( p_line != NULL
            && ( p_str = strstr( txtline, p_text ) ) == NULL );

    return ( p_line != NULL ) ? p_str : NULL;
}
