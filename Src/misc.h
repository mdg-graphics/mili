/* $Id$ */
/*
 * misc.h - Miscellaneous definitions.
 *
 *  	Donald J. Dovey
 * 	Lawrence Livermore National Laboratory
 *     	Oct 24 1991
 *
 * Copyright (c) 1991 Regents of the University of California
 */

#ifndef MISC_H
#define MISC_H

#include <stdio.h>
#include <math.h>
#include <string.h>

/*
 * Uncomment next lines if debugging.
 */
/*
#define DEBUG
#define DEBUG_MEM
*/


/*****************************************************************
 * TAG( NEW NEW_N )
 * 
 * Macros for allocating an object or a number of objects.
 */
#ifndef DEBUG_MEM
#define NEW(type,descr) ( (type *)calloc( 1, sizeof( type ) ) ) 
#define NEW_N(type,cnt,descr) ( (type *)calloc( (cnt), sizeof( type ) ) ) 
#define RENEW_N(type,old,cnt,add,descr)                                       \
    ( (type *)realloc( (void *) (old), ((cnt) + (add)) * sizeof( type ) ) )
#endif DEBUG_MEM

#ifdef DEBUG_MEM
extern char *my_calloc();
#define NEW(type,descr) ( (type *)my_calloc( 1, sizeof( type ), descr ) ) 
#define NEW_N(type,cnt,descr) ( (type *)my_calloc( (cnt), sizeof( type ), \
                                descr ) )
extern void *verbose_realloc();
#define RENEW_N(type,old,cnt,add,descr)                                       \
    ( (type *)verbose_realloc( (void *) (old),                                \
                               (cnt) * sizeof( type ),                        \
			       (add) * sizeof( type ),                        \
			       descr ) )
#endif DEBUG_MEM

/*****************************************************************
 * TAG( Bool_type )
 * 
 * A boolean value.
 */
typedef int Bool_type;

/*****************************************************************
 * TAG( TRUE FALSE )
 * 
 * The boolean values of true and false.
 */
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/*****************************************************************
 * TAG( Popup_Dialog_Type )
 *
 * Different flavors of non-fatal error popup dialogs. Placed here
 * for accessibility by files which don't include viewer.h.
 */
typedef enum
{
    INFO_POPUP, 
    USAGE_POPUP, 
    WARNING_POPUP
} Popup_Dialog_Type;

/*****************************************************************
 * 
 * Numerical constants.
 */
#define INFINITY	4.2535295865117308e37	/* (2^125) */
#define BADINT  	0x80000000		/* Invalid return. */
#define EPS     	1e-6
#define EPS2     	1e-10

#ifndef PI
#ifdef M_PI
#define PI    M_PI
#else
#define PI    3.14159265
#endif
#endif

/*****************************************************************
 * TAG( DEG_TO_RAD RAD_TO_DEG )
 * 
 * DEG_TO_RAD	Convert degrees to radians.
 * RAD_TO_DEG	Convert radians to degrees.
 */
#define DEG_TO_RAD(x) ( (x) * PI / 180.0 )
#define RAD_TO_DEG(x) ( (x) * 180.0 / PI )

/*****************************************************************
 * TAG( SIGN ISIGN SQR )
 * 
 * SIGN		Return the sign of a float.
 * ISIGN	Return the sign of an integer.
 * SQR         	Square of a number.
 */
#define SIGN(x) ((x) == 0.0 ? 0.0 : (x) / fabs( (double)(x) ) )
#define ISIGN(x) ((x) == 0 ? 0 : (x) / abs( (x) ) )

#define SQR(a) ((a) * (a))


/*****************************************************************
 * TAG( MAX MIN )
 * 
 * MAX		Return the largest of the two.
 * MIN		Return the smallest of the two.
 */
#undef MAX
#define MAX(i,j) ((i) > (j) ? (i) : (j))

#undef MIN
#define MIN(i,j) ((i) < (j) ? (i) : (j))


/*****************************************************************
 * TAG( BOUND SWAP )
 * 
 * BOUND	Return X if X is contained in the range (LO, HI),
 * 		otherwise return LO if X <= LO or HI if X >= HI.
 * SWAP		Swap X and Y.
 */
#define BOUND(lo,x,hi)   MAX( lo, MIN( x, hi ) )
#define SWAP(tmp,x,y) { (tmp) = (x); (x) = (y); (y) = (tmp); }


/*****************************************************************
 * TAG( APX_EQ )
 *
 * Approximate equality.
 */
#define APX_EQ(x,y) ( fabs((double)(x)-(y)) < EPS )


/*****************************************************************
 * TAG( DEF_LT )
 *
 * Definitely less than.
 */
#define DEF_LT(x,y) ( (x) < (y) - EPS )


/*****************************************************************
 * TAG( DEF_GT )
 *
 * Definitely greater than.
 */
#define DEF_GT(x,y) ( (x) > (y) + EPS )


/************************************************************
 * TAG( check_timing )
 *
 * Print program timing data for the user.  Called with a "0"
 * to start the timing interval and with a "1" to end the
 * the interval and print out timing information.
 */
extern void
check_timing( /* end_flag */ );


/* Other routines. */
extern int str_dup();
extern Bool_type all_true();
extern Bool_type all_false();
extern Bool_type is_in_iarray();
extern char *find_text();
extern int calc_fracsz();
extern void do_nothing_stub();

#endif MISC_H
