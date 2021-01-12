/* $Id$ */
/*
 * misc.h - Miscellaneous definitions.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Oct 24 1991
 *
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - May 5, 2008: Added support for code usage tracking using
 *                the AX tracker tool.
 *                See SRC#533.
 *
 ************************************************************************
 */

#ifndef MISC_H
#define MISC_H

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include "griz_config.h"

#ifndef GRIZ_VERSION
#define GRIZ_VERSION PACKAGE_VERSION
#endif


#ifndef MILI_VERSION
#define MILI_VERSION "V19_01"
#endif


#define TIME_TRACKER 1
#define TIME_GRIZ    0

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
#define NEW_N_MALLOC(type,cnt,descr) ( (type *)malloc( (cnt)*sizeof( type ) ) )
#define RENEW_N(type,old,cnt,add,descr)                                       \
    ( (type *)realloc( (void *) (old), ((cnt) + (add)) * sizeof( type ) ) )
#define RENEWC_N(type,old,cnt,add,descr)                                       \
    ( (type *)verbose_recalloc( (void *) (old),                                \
                                (cnt) * sizeof( type ),                        \
                                (add) * sizeof( type ),                        \
                                descr ) )
#endif

#ifdef DEBUG_MEM
#define NEW(type,descr) ( (type *)griz_my_calloc( 1, sizeof( type ), descr ) )
#define NEW_N(type,cnt,descr) ( (type *)griz_my_calloc( (cnt), sizeof( type ), \
                                descr ) )

#define RENEW_N(type,old,cnt,add,descr)                                       \
    ( (type *)verbose_realloc( (void *) (old),                                \
                               (cnt) * sizeof( type ),                        \
                               (add) * sizeof( type ),                        \
                               descr ) )

#define RENEWC_N(type,old,cnt,add,descr)                                       \
    ( (type *)verbose_recalloc( (void *) (old),                                \
                                (cnt) * sizeof( type ),                        \
                                (add) * sizeof( type ),                        \
                                descr ) )
#endif

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
 * TAG( ON OFF )
 *
 * The boolean values of on and off.
 */
#ifndef ON
#define ON  1
#define OFF 0
#endif

/*****************************************************************
 * TAG( BIT ARRAYS )
 */

#define BITMASK(b)     (1 << ((b) % CHAR_BIT))
#define BITSLOT(b)     ((b) / CHAR_BIT)
#define BITSET(a, b)   ((a)[BITSLOT(b)] |=  BITMASK(b))
#define BITCLEAR(a, b) ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b)  ((a)[BITSLOT(b)] &   BITMASK(b))
#define BITNSLOTS(nb)  ((nb + CHAR_BIT - 1) / CHAR_BIT)


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
#ifndef AIX
#endif
#define BADINT          0x80000000              /* Invalid return. */
#define EPS             1e-6
#define EPS2            1e-10

#ifndef PI
#ifdef M_PI
#define PI    M_PI
#else
#define PI    3.14159265358979323846
#endif
#endif

/*****************************************************************
 * TAG( DEG_TO_RAD RAD_TO_DEG )
 *
 * DEG_TO_RAD   Convert degrees to radians.
 * RAD_TO_DEG   Convert radians to degrees.
 */
#define DEG_TO_RAD(x) ( (x) * PI / 180.0 )
#define RAD_TO_DEG(x) ( (x) * 180.0 / PI )

/*****************************************************************
 * TAG( SIGN ISIGN SQR CUBE )
 *
 * SIGN         Return the sign of a float.
 * ISIGN        Return the sign of an integer.
 * SQR          Square of a number.
 * CUBE         Cube of a number.
 */
#define SIGN(x) ((x) == 0.0 ? 0.0 : (x) / fabs( (double)(x) ) )
#define ISIGN(x) ((x) == 0 ? 0 : (x) / abs( (x) ) )

#define SQR(a)  ((a) * (a))
#define CUBE(a) ((a) * (a) * (a))

/*****************************************************************
 * TAG( MAX MIN )
 *
 * MAX          Return the largest of the two.
 * MIN          Return the smallest of the two.
 */
#undef MAX
#define MAX(i,j) ((i) > (j) ? (i) : (j))

#undef MIN
#define MIN(i,j) ((i) < (j) ? (i) : (j))


/*****************************************************************
 * TAG( BOUND SWAP )
 *
 * BOUND        Return X if X is contained in the range (LO, HI),
 *              otherwise return LO if X <= LO or HI if X >= HI.
 * SWAP         Swap X and Y.
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


/*****************************************************************
 * TAG( qty_connects )
 *
 * Array of connectivity quantities by mesh object superclass.
 */
extern int qty_connects[];


/************************************************************
 * TAG( check_timing )
 *
 * Print program timing data for the user.  Called with a "0"
 * to start the timing interval and with a "1" to end the
 * the interval and print out timing information.
 */
extern void
check_timing( int end_flag );


/************************************************************
 * TAG( manage_timer )
 *
 * Print program timing data for the user.  Called with a "0"
 * to start the timing interval and with a "1" to end the
 * the interval and print out timing information.
 */
extern void
manage_timer( int timer, int end_flag );


/* Other routines. */
extern char *griz_my_calloc( int, int, char * );
extern void *verbose_realloc( void *, int, int, char * );
extern void *verbose_recalloc( void *, size_t, size_t, char * );
extern int griz_str_dup( char **, char * );
extern Bool_type all_true( Bool_type *, int );
extern Bool_type all_false( Bool_type *, int );
extern Bool_type all_true_uc( unsigned char *, int );
extern Bool_type all_false_uc( unsigned char *, int );
extern Bool_type is_in_iarray( int, int, int *, int * );
extern char *find_text( FILE *, char * );
extern int calc_fracsz( float, float, int );
extern void st_var_delete( void * );
extern void delete_primal_result( void * );
extern void delete_derived_result( void * );
extern void delete_mat_name( void * );
extern void delete_mo_class_data( void * );
extern void do_nothing_stub( char * );
extern void blocks_to_list( int qty_blocks, int *mo_blocks, int *list,
                            Bool_type decrement_indices );

extern int my_comparator(void const *item1, void const *item2);

#endif
