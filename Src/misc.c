/* $Id$ */
/*
 * misc.c - Miscellaneous routines.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Oct 24 1991
 *
 ************************************************************************
 */
#include "griz_config.h"
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>

#ifdef __hpux
#define NO_GETRUSAGE
#endif
#ifdef cray
#define NO_GETRUSAGE
#endif

#ifndef NO_GETRUSAGE
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/timeb.h>
#endif

#include "viewer.h"
#include "draw.h"
#include "misc.h"
#include "mdg.h"
#include "io_wrap.h"
//#include "alphanum_comp.c"

extern Bool_type include_util_panel, include_mtl_panel, include_utilmtl_panel;

extern void get_window_attributes( void );

#define GET_INDEXED_SREC( pr, idx, drvd ) \
        ( ( drvd ) \
          ? ((Derived_result *) (pr)->original_result)->srec_map + (idx) \
          : ((Primal_result *) (pr)->original_result)->srec_map + (idx) )

#define GET_INDEXED_SUBREC( pmap, idx, drvd ) \
        ( ( drvd ) \
          ? ((Subrecord_result *) (pmap)->list)[(idx)].subrec_id \
          : ((int *) (pmap)->list)[(idx)] )


static int _mem_total = 0;
static int autoimg_frame_count = 1;

static int temp_mem_ptr = 0;

/*****************************************************************
 * TAG( qty_connects )
 *
 * Quantity of connectivity nodes per superclass.
 */
int qty_connects[QTY_SCLASS] =
{
    0, 1, 2, 3, 3, 4, 4, 5, 6, 8, 0, 0, 4, 1
};

/*****************************************************************
 * TAG( griz_my_calloc )
 *
 * Calloc instrumented to keep track of allocations.
 */
char *
griz_my_calloc( int cnt, int size, char *descr )
{
    char *arr;

    _mem_total += cnt*size;

#ifdef DEBUG_MEM
    wrt_text( "Allocating memory for %s: %d bytes, total %d K.\n",
              descr, cnt*size, _mem_total/1024 );
#endif

    arr = (char *)calloc( cnt, size );
    if ( arr == NULL )
        popup_fatal( "Can't allocate any more memory." );

    return arr;
}


/*****************************************************************
 * TAG( verbose_realloc )
 *
 * Realloc if possible, else calloc and copy.  Warning - call
 * syntax enables caller to have a dangling reference if return
 * value is not assigned to argument passed into "ptr".
 */
void *
verbose_realloc( void *ptr, int size, int add, char *descr )
{
    unsigned char *new;
    int new_size;

    new_size = size + add;

    new = (unsigned char *) realloc( ptr, new_size );

    if ( new == NULL )
        popup_fatal( "Can't reallocate additional memory." );

#ifdef DEBUG_MEM
    _mem_total += add;

    fprintf( stderr, "Reallocating memory for %s: %d bytes, total %d K.\n",
             descr, add, _mem_total / 1024 );
#endif

    return (void *) new;
}


/*****************************************************************
 * TAG( verbose_recalloc )
 *
 * Realloc if possible with clear of new memory.  Warning - call
 * syntax enables caller to have a dangling reference if return
 * value is not assigned to argument passed into "ptr".
 */
void *
verbose_recalloc( void *ptr, size_t size, size_t add, char *descr )
{
    unsigned char *new;
    size_t new_size;

    new_size = size + add;

    new = (unsigned char *) realloc( ptr, new_size );

    if ( new == NULL )
        popup_fatal( "Can't reallocate additional memory." );

    memset( new + size, 0, add );

#ifdef DEBUG_MEM
    _mem_total += add;

    fprintf( stderr, "Reallocating memory for %s: %d bytes, total %d K.\n",
             descr, add, _mem_total / 1024 );
#endif

    return (void *) new;
}


/************************************************************
 * TAG( timing_vals )
 *
 * A shell that calls get_rusage to get timing information.
 * This is a helper function for get_timing().
 */
void
timing_vals( float *ut, float *st, long *maxrs, long *rs, long *minpf,
             long *majpf, long *ns, long *in, long *out )
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
check_timing( int end_flag )
{
    static float ut1, st1, ut2, st2;
    static long maxrs1, maxrs2, rs1, rs2, minpf1, minpf2, majpf1, majpf2;
    static long ns1, ns2, in1, in2, out1, out2;
    time_t mt, st;

#ifdef NO_GETRUSAGE
    return;

#else
    static struct timeb wc1, wc2;
    if ( end_flag == 0 )
    {
        ftime( &wc1 );
        timing_vals( &ut1, &st1, &maxrs1, &rs1, &minpf1, &majpf1,
                     &ns1, &in1, &out1 );
    }
    else
    {
        timing_vals( &ut2, &st2, &maxrs2, &rs2, &minpf2, &majpf2,
                     &ns2, &in2, &out2 );
        ftime( &wc2 );

        mt = wc2.millitm - wc1.millitm;
        if ( mt < 0 )
        {
            mt += 1000;
            st = wc2.time - wc1.time - 1;
        }
        else
            st = wc2.time - wc1.time;

        wrt_text( "Wall clock time [seconds]               : %d.%03d\n",
                  (int) st, (int) mt );
        wrt_text( "User CPU Time [seconds]                 : %e\n",
                  (float) (ut2-ut1) );
        wrt_text( "System CPU Time [seconds]               : %e\n",
                  (float) (st2-st1) );
        wrt_text( "Maximum Resident Set Size [pages]       : %d, %d\n",
                  (int) maxrs1, (int) maxrs2 );
        wrt_text( "Memory Residency Integral [page-seconds]: %d\n",
                  (int) (rs2-rs1) );
        wrt_text( "Minor Page Faults [no I/O required]     : %d\n",
                  (int) (minpf2-minpf1) );
        wrt_text( "Major Page Faults [   I/O required]     : %d\n",
                  (int) (majpf2-majpf1) );
        wrt_text( "Physical Read Requests Honored          : %d\n",
                  (int) (in2-in1) );
        wrt_text( "Physical Write Requests Honored         : %d\n",
                  (int) (out2-out1) );
        wrt_text( "\n" );
    }
#endif
}


/*****************************************************************
 * TAG( manage_timer )
 *
 * Activate/print program timing data for the user.  Up to
 * ten timers may be active simultaneously.  Parameter "timer"
 * is an integer on [0,9] which identifies which timer to
 * use.  Parameter "end_flag" is "0" to start the timing
 * interval and "1" to end the interval and print out timing
 * information.
 */
void
manage_timer( int timer, int end_flag )
{
#ifndef NO_GETRUSAGE
    typedef struct timer_data
    {
        Bool_type active;
        struct timeb wall_begin;
        struct timeb wall_end;
        struct rusage r_begin;
        struct rusage r_end;
    } TimerData;

    static TimerData timers[10];
#endif
    time_t mt, st;

    char tracker_str[1000], walltime_str[20], version[64],
         problem_str[512];

#ifdef NO_GETRUSAGE
    return;
#endif

#ifndef NO_GETRUSAGE
    if ( timer < 0 || timer > 9 )
    {
        popup_dialog( WARNING_POPUP, "Invalid timer number." );
        return;
    }

    if ( end_flag == 0 )
    {
        if ( timers[timer].active )
            popup_dialog( WARNING_POPUP, "Resetting active timer." );
        else
            timers[timer].active = TRUE;

        ftime( &timers[timer].wall_begin );
        getrusage( RUSAGE_SELF, &timers[timer].r_begin );
    }
    else
    {
        struct timeb *p_tb, *p_te;
        struct rusage *p_rb, *p_re;
        TimerData *p_td;

        p_td = timers + timer;

        if ( !p_td->active )
        {
            popup_dialog( WARNING_POPUP, "Cannot terminate inactive timer." );
            return;
        }

        p_tb = &p_td->wall_begin;
        p_te = &p_td->wall_end;
        p_rb = &p_td->r_begin;
        p_re = &p_td->r_end;

        getrusage( RUSAGE_SELF, p_re );
        ftime( p_te );

        mt = p_te->millitm - p_tb->millitm;
        if ( mt < 0 )
        {
            mt += 1000;
            st = p_te->time - p_tb->time - 1;
        }
        else
            st = p_te->time - p_tb->time;

        wrt_text( "Wall clock time [seconds]               : %d.%03d\n",
                  (int) st, (int) mt );
        wrt_text( "User CPU Time [seconds]                 : %e\n",
                  (p_re->ru_utime.tv_sec + p_re->ru_utime.tv_usec * 1.0e-6)
                  - (p_rb->ru_utime.tv_sec + p_rb->ru_utime.tv_usec * 1.0e-6) );
        wrt_text( "System CPU Time [seconds]               : %e\n",
                  (p_re->ru_stime.tv_sec + p_re->ru_stime.tv_usec * 1.0e-6)
                  - (p_rb->ru_stime.tv_sec + p_re->ru_stime.tv_usec * 1.0e-6) );
        wrt_text( "Maximum Resident Set Size [pages]       : %d, %d\n",
                  (int) p_rb->ru_maxrss, (int) p_re->ru_maxrss );
        wrt_text( "Memory Residency Integral [page-seconds]: %d\n",
                  (int) (p_re->ru_idrss - p_rb->ru_idrss) );
        wrt_text( "Minor Page Faults [no I/O required]     : %d\n",
                  (int) (p_re->ru_minflt - p_rb->ru_minflt) );
        wrt_text( "Major Page Faults [   I/O required]     : %d\n",
                  (int) (p_re->ru_majflt - p_rb->ru_majflt) );
        wrt_text( "Physical Read Requests Honored          : %d\n",
                  (int) (p_re->ru_inblock - p_rb->ru_inblock) );
        wrt_text( "Physical Write Requests Honored         : %d\n",
                  (int) (p_re->ru_oublock - p_rb->ru_oublock) );
        wrt_text( "\n" );

        p_td->active = FALSE;



#ifdef TIME_TRACKER
        /*************************************************************************************
         ** Tracker Usage
        Usage: tracker -n PROG_NAME -v VERSION [options]

            -h  --help              print this help screen
            -b  --background        work in the background
            -d  --delete            unlink FILENAME when done (must precede -f flag)
            -n  --name PROG_NAME    name of the code that was run (32-char limit; required)
            -P  --procs INT         total number of processes used (default = 1)
            -T  --threads INT       number of threads used per process (default = 1)
            -W  --walltime FLOAT    wall clock time used in seconds (default = 1.0)
            -I  --problemid ID      problem ID for this run (default = 'No_ID'; 32 char limit)
            -f  --file FILENAME     file containing KEY VALUE pairs (optional)
            -r  --runs INT          # runs <= 100; no accumulate (default = 1)
            -q  --quiet             reduce amount of output printed
            -u  --uniqueid ID       unique ID (accumulates run's walltime; 255-char limit)
            -v  --version VERSION   version number of code that was run (16-char limit; required)
            -V  --verbose           print tracker's status information

        Examples:

        tracker -n myProgramName -v 1.0
        tracker -n myProgramName -v 1.0 -P 4 -T 1 -W 10.0

        Notes:

        The -f/--file argument takes a text file that has KEY VALUE pairs.
        Each KEY and VALUE *must* be separated by a space character ' '
        There may only be *one* KEY VALUE per line (pairs are separated by newlines).
        KEY may be any string (32-char limit) that does *not* contain quotes or semicolons.
        VALUE may be an int (32 bit), float (32 bit), string (32 chars), or boolean
        boolean VALUEs are lowercase strings: true, false

        File example:

        time(diffusion) 10.0
        time(hydro) 30.0
        constant -120.046e-12
        hydro true
        sn false
        count 3
        mystr "some string"

        Usage: tracker-code-report [options]

            -h  --help                print this help screen
            -c  --codeName CODE_NAME  name of code to report or all (required)
            -C  --includeCodeVersion  include code version in report (default is not)
            -m  --month n             month (numerical index) (default is current)
            -d  --day n               day
            -y  --year n              year
            -b  --beginReport DATE    begin date for report (e.g. 2008-06-08)
            -e  --endReport DATE      end date for report (e.g. 20008-06-12)
            -H  --hostName HOST_NAME  name of host for report (default is all)
            -u  --user USER_NAME      name of user for report (default is all)
            -l  --longReport          include detailed run report
            -B  --database DB_NAME    name of tracker database (default is tracker)
            -p  --property PROP_NAME  get values of property name
            -q  --quiet               reduce amount of output printed
            -V  --verbose             print tracker's status information
            -T  --timeout             set timeout in (integer) seconds; default = 120

        Examples:

        tracker-code-report -c mycode         | tee report
        tracker-code-report -c mycode -H yana | tee report
        tracker-code-report -c mycode -m 12   | tee report

        Defaults:
        - When no time specifier is used, give monthly report.
        - When a day is specified, give day report.
        - When only a year specified, give yearly report.

        ************************************************************************************************/

        sprintf(walltime_str," -W %d.%03d ", (int) st, (int) mt );
        strcpy( version, " -v " );
        strcat( version, PACKAGE_VERSION );
        strcat( version, " " );

        strcpy( problem_str, " -I ");
        strcat( problem_str, env.curr_analy->root_name );
        strcat( version, " " );

        /* Construct the tracker execute line */
        strcpy( tracker_str, "/usr/apps/tracker/bin/tracker -q -n Griz " );
        strcat( tracker_str, version );
        strcat( tracker_str, walltime_str );
        strcat( tracker_str, problem_str );
        system( tracker_str );
#endif
    }
#endif
}


/*****************************************************************
 * TAG( griz_str_dup )
 *
 * Duplicate a string with space allocation. Return length of
 * string duplicated.
 */
int
griz_str_dup( char **dest, char *src )
{
    int len;
    char *pd, *ps;

    for ( ps = src, len = 0; *ps; len++, ps++ );

    *dest = (char *) NEW_N( char, len + 1, "String duplicate" );

    pd = *dest;
    ps = src;
    while ( *pd++ = *ps++ );

    return len;
}


/*****************************************************************
 * TAG( is_operation_token )
 *
 * Check to see if a text string represents one of the keywords
 * used as an argument to the "plot" command to designate an
 * "operation" plot.
 */
Bool_type
is_operation_token( char token[TOKENLENGTH], Plot_operation_type *otype )
{
    int maxlen;
    Plot_operation_type op_type;

    maxlen = strlen( token );
    op_type = OP_NULL;

    if ( token[1] == '\0' )
    {
        if ( token[0] == '-' )
            op_type = OP_DIFFERENCE;
        else if ( token[0] == '+' )
            op_type = OP_SUM;
        else if ( token[0] == '*' )
            op_type = OP_PRODUCT;
        else if ( token[0] == '/' )
            op_type = OP_QUOTIENT;
    }
    else
    {
        if ( strncmp( "diff", token, maxlen ) == 0 )
            op_type = OP_DIFFERENCE;
        else if ( strncmp( "sum", token, maxlen ) == 0 )
            op_type = OP_SUM;
        else if ( strncmp( "prod", token, maxlen ) == 0 )
            op_type = OP_PRODUCT;
        else if ( strncmp( "quot", token, maxlen ) == 0 )
            op_type = OP_QUOTIENT;
    }

    if ( op_type != OP_NULL )
    {
        if ( otype != NULL )
            *otype = op_type;
        return TRUE;
    }
    else
        return FALSE;
}


/*****************************************************************
 * TAG( all_true )
 *
 * Returns TRUE if all elements of a Bool_type array are TRUE.
 */
Bool_type
all_true( Bool_type *boola, int qty )
{
    Bool_type accum;
    Bool_type *p_test, *bound;

    for ( p_test = boola, bound = boola + qty, accum = TRUE;
            p_test < bound;
            accum &= *p_test++ ); /* Bitwise! True's need a 1-bit in common. */

    return accum;
}


/*****************************************************************
 * TAG( all_true_uc )
 *
 * Returns TRUE if all elements of an unsigned char array are TRUE.
 */
Bool_type
all_true_uc( unsigned char *boola, int qty )
{
    unsigned char accum;
    unsigned char *p_test, *bound;

    for ( p_test = boola, bound = boola + qty, accum = TRUE;
            p_test < bound;
            accum &= *p_test++ ); /* Bitwise! True's need a 1-bit in common. */

    return (Bool_type) accum;
}


/*****************************************************************
 * TAG( all_false )
 *
 * Returns TRUE if all elements of a Bool_type array are FALSE.
 */
Bool_type
all_false( Bool_type *boola, int qty )
{
    Bool_type accum;
    Bool_type *p_test, *bound;

    for ( p_test = boola, bound = boola + qty, accum = FALSE;
            p_test < bound;
            accum |= *p_test++ );

    return !accum;
}


/*****************************************************************
 * TAG( all_false_uc )
 *
 * Returns TRUE if all elements of an unsigned char array are FALSE.
 */
Bool_type
all_false_uc( unsigned char *boola, int qty )
{
    unsigned char accum;
    unsigned char *p_test, *bound;

    for ( p_test = boola, bound = boola + qty, accum = FALSE;
            p_test < bound;
            accum |= *p_test++ );

    return (Bool_type) !accum;
}


/*****************************************************************
 * TAG( is_in_iarray )
 *
 * Returns TRUE if candidate integer is contained in an array
 * of integers.
 */
Bool_type
is_in_iarray( int candidate, int size, int *iarray, int *p_location )
{
    int *p_int, *bound;

    bound = iarray + size;
    for ( p_int = iarray; p_int < bound; p_int++ )
        if ( candidate == *p_int )
        {
            if ( p_location != NULL )
                *p_location = (int) (p_int - iarray);
            break;
        }

    return ( p_int != bound );
}


/*****************************************************************
 * TAG( bool_compare_array )
 *
 * Returns TRUE if elements of two arrays are identically TRUE or
 * FALSE.
 */
Bool_type
bool_compare_array( int size, int *array1, int *array2 )
{
    int i;

    for ( i = 0; i < size; i++ )
    {
        if ( array1[i] )
        {
            if ( !array2[i] )
                return FALSE;
        }
        else
        {
            if ( array2[i] )
                return FALSE;
        }
    }

    return TRUE;
}


/*****************************************************************
 * TAG( object_is_bound )
 *
 * Returns TRUE if candidate index bound to subrecord.
 */
Bool_type
object_is_bound( int ident, MO_class_data *p_mo_class, Subrec_obj *p_subrec )
{
    Int_2tuple *p_block;
    int blk_qty;
    int i;

    if ( p_mo_class != p_subrec->p_object_class )
    {
      return FALSE;
    }

    blk_qty = p_subrec->subrec.qty_blocks;

    for ( i = 0, p_block = (Int_2tuple *) p_subrec->subrec.mo_blocks;
            i < blk_qty;
            i++ )
    {
        if ( ident >= p_block[i][0] && ident <= p_block[i][1] )
            return TRUE;
    }

    return FALSE;
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
find_text( FILE *ifile, char *p_text )
{
    static char txtline[TXT_BUF_SIZE];
    char *p_str, *p_line;

    do
        p_line = fgets( txtline, TXT_BUF_SIZE, ifile );
    while ( p_line != NULL
            && ( p_str = strstr( txtline, p_text ) ) == NULL );

    return ( p_line != NULL ) ? p_str : NULL;
}


/*****************************************************************
 * TAG( calc_fracsz )
 *
 * Calculate the quantity of digits to display after the decimal
 * point for a given range and quantity of values.  Bound the
 * minimum result to 1.
 */
int
calc_fracsz( float min, float max, int intervals )
{
    double dmin, dmax, big_log, little_log, whole_little_log, dtmp;
    int fracsz;

    /* Sanity check. */
    if ( min == max )
        return 2;

    /*
     * Calculate the fraction size as the difference between the ceiling
     * of the base-10 log of the largest number to be displayed and the
     * ceiling of the base-10 log of the interval between adjacent samples.
     * Note that the number of significant digits in the "e" format
     * representation of the scale tic labels needs to be one more than
     * this difference for the interval to show up as a visible delta
     * between adjacent tic labels, so, technically, this routine should
     * add one to the value calculated.  But, we know a priori that the
     * caller is going to treat the number returned as just the quantity
     * of digits after the decimal point and provide for one digit
     * preceding the decimal point, so we don't perform the increment
     * and let the value returned really be just the length of the fraction.
     */

    /* Big_log: log of largest displayed value. */
    dmin = fabs( (double) min );
    dmax = fabs( (double) max );
    if ( dmin > dmax )
        dmax = dmin;

    big_log = log10( dmax );

    /* Little_log: log of interval magnitude. */
    dtmp = ((double) max - min) / (double) intervals;
    dtmp = fabs( dtmp );

    little_log = log10( dtmp );
    whole_little_log = ceil( little_log );

    /*
     * If the interval is a whole power of 10 then its visual string
     * representation will require one more character than the log value
     * (i.e. 100 has a log of 2 but requires three characters to write out).
     * This has the effect of reducing the fraction size because, in terms
     * of the number of characters required to represent it, it makes the
     * interval more like the tic label numbers (i.e., the change in
     * adjacent label numbers is represented by a change in a more significant
     * digit).
     *
     * We actually permit the interval to be "real close" to a whole power
     * of ten, as experience dictates that roundoff errors can corrupt
     * what ought to be whole powers in this process.
     */
    if ( whole_little_log == little_log
            || fabs( (double) 1.0 - whole_little_log / little_log ) < EPS )
        whole_little_log += 1.0;

    fracsz = (int) (ceil( big_log ) - whole_little_log + 0.5);

    /* Bound the value. */
    if ( fracsz < 1 )
        fracsz = 1;

    return fracsz;
}


/*****************************************************************
 * TAG( st_var_delete )
 *
 * Function to clean and delete a State_variable struct.  Designed
 * as a hash table data deletion function for htable_delete().
 */
void
st_var_delete( void *p_state_variable )
{
    env.curr_analy->db_cleanse_state_var( (State_variable *) p_state_variable );
    free( p_state_variable );
}


/*****************************************************************
 * TAG( delete_primal_result )
 *
 * Function to delete a Primal_result struct.  Designed
 * as a hash table data deletion function for htable_delete().
 */
void
delete_primal_result( void *p_primal_result )
{
    Primal_result *p_pr;
    int i,j;

    p_pr = (Primal_result *) p_primal_result;
    if(p_pr->long_name == NULL)
    {
       p_primal_result = NULL;
       return;
    }
    free( p_pr->long_name );
    p_pr->long_name = NULL;
    free( p_pr->short_name );
    p_pr->short_name = NULL;

    //These are just null and freed elsewhere
    for(i=0;i<p_pr->owning_vec_count;i++)
    {
        p_pr->owning_vector_result[i] = NULL;
    }

    for ( i = 0; i < env.curr_analy->qty_srec_fmts; i++ )
    {
        if ( p_pr->srec_map[i].qty != 0 )
        {
            if(p_pr->original_names_per_subrec != NULL)
            {
                for(j=0; j<p_pr->srec_map[i].qty;j++)
                {
                    free( p_pr->original_names_per_subrec[j]);
                    p_pr->original_names_per_subrec[j] = NULL;
                }
            }
            free( p_pr->srec_map[i].list );
        }
    }

    free( p_pr->srec_map );

    if(p_pr->original_names_per_subrec != NULL)
    {
        free( p_pr->original_names_per_subrec);
        p_pr->original_names_per_subrec = NULL;
    }

    if(p_pr->subrecs != NULL)
        free( p_pr->subrecs);

    free( p_primal_result );
    p_primal_result = NULL;
    p_pr= NULL;
}


/*****************************************************************
 * TAG( delete_derived_result )
 *
 * Function to delete a Derived_result struct.  Designed
 * as a hash table data deletion function for htable_delete().
 */
void
delete_derived_result( void *p_derived_result )
{
    Derived_result *p_dr;
    int i;

    p_dr = (Derived_result *) p_derived_result;

    for ( i = 0; i < env.curr_analy->qty_srec_fmts; i++ )
        if ( p_dr->srec_map[i].qty != 0 )
            free( p_dr->srec_map[i].list );

    free( p_dr->srec_map );

    free( p_dr->srec_ids );

    free( p_dr );
}


/*****************************************************************
 * TAG( delete_mo_class_data )
 *
 * Delete MO_class_data struct and referenced memory.
 * Designed as a hash table data deletion function for htable_delete().
 */
void
delete_mo_class_data( void *p_data )
{
    MO_class_data *p_mocd;
    Elem_data *p_ed;
    Material_data *p_mtld;
    Surface_data *p_surface;
    int sclass;
    int i;

    p_mocd = (MO_class_data *) p_data;
    sclass = p_mocd->superclass;

    switch ( sclass )
    {
    case G_NODE:
        free( p_mocd->objects.nodes );
        break;

    case G_UNIT:
        /* do nothing */
        break;

    case G_MAT:
        p_mtld = p_mocd->objects.materials;
        for ( i = 0; i < p_mocd->qty; i++ )
        {
            if ( p_mtld[i].elem_qty > 0 )
                free( p_mtld[i].elem_blocks );
            if ( p_mtld[i].node_blocks != NULL )
                free( p_mtld[i].node_blocks );
        }
        free( p_mtld );
        break;

    case G_SURFACE:
        p_surface = p_mocd->objects.surfaces;
        for ( i = 0; i < p_mocd->qty; i++ )
        {
            free( p_surface[i].nodes );
        }
        free_vis_data( &p_surface->p_vis_data, sclass );

        free( p_surface );
        break;

    case G_TRUSS:
    case G_BEAM:
    case G_TRI:
    case G_QUAD:
    case G_TET:
    case G_PYRAMID:
    case G_WEDGE:
    case G_HEX:
        p_ed = p_mocd->objects.elems;
        free( p_ed->nodes );
        p_ed->nodes = NULL;
        free( p_ed->mat );
        p_ed->mat = NULL;
        free( p_ed->part );
        p_ed->part = NULL;
        free( p_ed );
        p_ed = NULL;
        if ( p_mocd->vector_pts != NULL )
            DELETE_LIST( p_mocd->vector_pts );
        break;
    }

    /* Free additional resources for fully implemented superclasses. */
    if ( sclass == G_TRI || sclass == G_QUAD )
    {
        free_elem_block( &p_mocd->p_elem_block, 2 );
        free_vis_data( &p_mocd->p_vis_data, sclass );
    }
    else if ( sclass == G_TET || sclass == G_HEX )
    {
        free_elem_block( &p_mocd->p_elem_block, 3 );
        free_vis_data( &p_mocd->p_vis_data, sclass );
    }

    free( p_mocd->short_name );
    p_mocd->short_name = NULL;
    free( p_mocd->long_name );
    free( p_mocd->data_buffer );

    if ( p_mocd->referenced_nodes != NULL )
        free( p_mocd->referenced_nodes );

    free( p_mocd );
}

/*****************************************************************
 * TAG( delete_mat_name )
 *
 * Function to delete a material name.  Designed as a
 * hash table data deletion function for htable_delete().
 *
 */
void
delete_mat_name( void *mat_name )
{
    char *matn;
    matn = (char *) mat_name;
    free( matn );
}


/*****************************************************************
 * TAG( do_nothing_stub )
 *
 * A stub that does nothing - to be used as error handler for
 * image lib.
 */
void
do_nothing_stub( char *text )
{
    /* OK, here is where we do nothing. */
}


/*****************************************************************
 * TAG( blocks_to_list )
 *
 * Expand identifiers implied by an array of first/last blocks
 * into explicit identifiers.
 */
void
blocks_to_list( int qty_blocks, int *mo_blocks, int *list,
                Bool_type decrement_indices )
{
    int i, j;
    int increment, ident;
    int *block, *list_entry;
    int start, stop;

    list_entry = list;

    for ( i = 0, block = mo_blocks; i < qty_blocks; i++, block += 2 )
    {
        if ( block[0] < block[1] )
        {
            increment = 1;
            start = block[0];
            stop = block[1];
        }
        else
        {
            increment = -1;
            start = block[1];
            stop = block[0];
        }

        ident = block[0];
        if ( decrement_indices )
            ident--;

        for ( j = start; j <= stop; j++, ident += increment )
            *list_entry++ = ident;
    }
}


/*****************************************************************
 * TAG( get_max_state )
 *
 * Obtain a maximum state number by querying the database and
 * applying any extant constraint.
 */
int
get_max_state( Analysis *analy )
{
    int qty_states;
    int rval;

    rval = analy->db_query( analy->db_ident, QRY_QTY_STATES, NULL, NULL,
                            &qty_states );

    if ( qty_states <= 0 )
        return 0;

    /*
     * Since this func returns the desired information and not status,
     * pop a diagnostic if there's an error.
     */
    if ( rval != OK )
        popup_dialog( WARNING_POPUP,
                      "get_max_state() - unable to query QRY_QTY_STATES!" );

    if ( analy->limit_max_state )
        return ( qty_states - 1 >= analy->max_state ) ? analy->max_state
               : qty_states - 1;
    else
        return qty_states - 1;
}


/*****************************************************************
 * TAG( find_named_class )
 *
 * Find a mesh object class by name among a particular mesh's classes.
 */
MO_class_data *
find_named_class( Mesh_data *p_mesh, char *test_name )
{
    int rval;
    Htable_entry *p_hte;

    rval = htable_search( p_mesh->class_table, test_name, FIND_ENTRY, &p_hte );

    if ( rval == OK )
        return (MO_class_data *) p_hte->data;
    else
        return NULL;
}


/*****************************************************************
 * TAG( find_class_by_long_name )
 *
 * Find a mesh object class by long name among a particular mesh's
 * classes.
 */
MO_class_data *
find_class_by_long_name( Mesh_data *p_mesh, char *test_lname )
{
    List_head *p_lh;
    int i, j;
    MO_class_data **p_mo_classes;

    for ( i = 0; i < QTY_SCLASS; i++ )
    {
        p_lh = p_mesh->classes_by_sclass + i;

        if ( p_lh->qty != 0 )
        {
            p_mo_classes = (MO_class_data **) p_lh->list;

            for ( j = 0; j < p_lh->qty; j++ )
            {
                if ( strcmp( test_lname, p_mo_classes[j]->long_name ) == 0 )
                    return p_mo_classes[j];
            }
        }
    }

    return NULL;
}


/*****************************************************************
 * TAG( destroy_time_series_obj )
 *
 * Release all resources associated with a Time_series_obj.
 */
void
destroy_time_series_obj( Time_series_obj **pp_tso )
{
    Time_series_obj *p_tso;

    p_tso = *pp_tso;
    if ( p_tso == NULL )
        return;

    if ( p_tso->qty_blocks != 0 )
        free( p_tso->state_blocks );

    if ( p_tso->data != NULL )
        free( p_tso->data );

    free( p_tso );

    *pp_tso = NULL;

}


/*****************************************************************
 * TAG( cleanse_tso_data )
 *
 * Remove time history data resources from a Time_series_obj struct.
 */
void
cleanse_tso_data( Time_series_obj *p_tso )
{
    if ( p_tso->qty_blocks != 0 )
    {
        free( p_tso->state_blocks );
        p_tso->state_blocks = NULL;
        p_tso->qty_blocks = 0;
    }

    if ( p_tso->data != NULL )
    {
        free( p_tso->data );
        p_tso->data = NULL;
    }

    p_tso->qty_states = 0;

    /* p_tso->modifiers = empty; */

    p_tso->min_val = MAXFLOAT;
    p_tso->max_val = -MAXFLOAT;

    p_tso->min_val_state = 0;
    p_tso->max_val_state = 0;

    p_tso->min_eval_st = 0;
    p_tso->max_eval_st = 0;
}


/*****************************************************************
 * TAG( build_oper_series_label )
 *
 * Create a result title with pertinent modifiers appended.
 */
void
build_oper_series_label( Time_series_obj *p_oper_tso, Bool_type brief,
                         char *label )
{
    /* These names are aligned with Plot_operation_type values. */
    char *titles[] =
    {
        "Null", "Difference", "Sum", "Product", "Quotient"
    };
    char *brief_titles[] =
    {
        "Null", "Diff", "Sum", "Prod", "Quot"
    };
    Bool_type shared_result;

    if ( p_oper_tso == NULL || label == NULL )
        return;

    shared_result = ( strcmp( p_oper_tso->operand1->result->name,
                              p_oper_tso->operand2->result->name ) == 0 );

    if ( brief )
    {
        if ( shared_result )
            sprintf( label, "%s(%s%d,%s%d:%s)",
                     brief_titles[p_oper_tso->op_type],
                     p_oper_tso->operand1->mo_class->short_name,
                     p_oper_tso->operand1->ident + 1,
                     p_oper_tso->operand2->mo_class->short_name,
                     p_oper_tso->operand2->ident + 1,
                     p_oper_tso->operand1->result->name );
        else
            sprintf( label, "%s(%s%d:%s,%s%d:%s)",
                     brief_titles[p_oper_tso->op_type],
                     p_oper_tso->operand1->mo_class->short_name,
                     p_oper_tso->operand1->ident + 1,
                     p_oper_tso->operand1->result->name,
                     p_oper_tso->operand2->mo_class->short_name,
                     p_oper_tso->operand2->ident + 1,
                     p_oper_tso->operand2->result->name );
    }
    else
    {
        if ( shared_result )
            sprintf( label, "%s(%s %d, %s %d : %s)",
                     titles[p_oper_tso->op_type],
                     p_oper_tso->operand1->mo_class->long_name,
                     p_oper_tso->operand1->ident + 1,
                     p_oper_tso->operand2->mo_class->long_name,
                     p_oper_tso->operand2->ident + 1,
                     p_oper_tso->operand1->result->title );
        else
            sprintf( label, "%s(%s%d:%s,%s%d:%s)",
                     titles[p_oper_tso->op_type],
                     p_oper_tso->operand1->mo_class->long_name,
                     p_oper_tso->operand1->ident + 1,
                     p_oper_tso->operand1->result->title,
                     p_oper_tso->operand2->mo_class->long_name,
                     p_oper_tso->operand2->ident + 1,
                     p_oper_tso->operand2->result->title );
    }
}


/*****************************************************************
 * TAG( build_result_label )
 *
 * Create a result title with pertinent modifiers appended.
 */
void
build_result_label( Result *p_result, Bool_type for_shells, char *label )
{
    int mods;
    char cbuf[32];

    strcpy( label, p_result->title );

    mods = 0;
    /*    if ( (int) p_result->modifiers.use_flags ) */
    if ( !all_false_uc( (unsigned char *) &p_result->modifiers.use_flags,
                        sizeof( p_result->modifiers.use_flags ) ) )
    {
        mods++;
        strcat( label, " (" );
    }

    if ( p_result->modifiers.use_flags.use_strain_variety )
    {
        mods++;
        strcat( label, strain_label[p_result->modifiers.strain_variety] );
    }

    if ( p_result->modifiers.use_flags.use_ref_surface && for_shells )
    {
        if ( mods > 1 )
            strcat( label, ", " );

        mods++;
        strcat( label, ref_surf_label[p_result->modifiers.ref_surf] );
    }

    if ( p_result->modifiers.use_flags.use_ref_frame && for_shells )
    {
        if ( mods > 1 )
            strcat( label, ", " );

        mods++;
        strcat( label, ref_frame_label[p_result->modifiers.ref_frame] );
    }

    if ( p_result->modifiers.use_flags.time_derivative )
    {
        if ( mods > 1 )
            strcat( label, ", " );

        mods++;
        strcat( label, "time deriv" );
    }

    if ( p_result->modifiers.use_flags.coord_transform )
    {
        if ( mods > 1 )
            strcat( label, ", " );

        mods++;
        strcat( label, "coord xform" );
    }

    if ( p_result->modifiers.use_flags.use_ref_state )
    {
        if ( mods > 1 )
            strcat( label, ", " );

        mods++;
        if ( p_result->modifiers.ref_state == 0 )
            sprintf( cbuf, "ref state=%s", "initial geom" );
        else
            sprintf( cbuf, "ref state=%d", p_result->modifiers.ref_state );
        strcat( label,  cbuf );
    }

    /* if ( mods )
        strcat( label, ")" ); */
    switch ( mods )
    {
    case 0:
        /* do nothing. */
        break;
    case 1:
        /* We thought we'd have modifiers, but didn't. */
        label[strlen( label ) - 2] = '\0';
        break;
    default:
        strcat( label, ")" );
    }
}


/*****************************************************************
 * TAG( svar_field_qty )
 *
 * Calculate the quantity of numeric fields in a state variable.
 * Based on Mili's svar_atom_qty().
 */
extern int
svar_field_qty( State_variable *p_sv )
{
    int qty;
    int k;

    qty = 1;

    if ( p_sv->agg_type == VECTOR )
    {
        qty = p_sv->vec_size;
    }
    else if ( p_sv->agg_type == ARRAY
              || p_sv->agg_type == VEC_ARRAY )
    {
        for ( k = 0; k < p_sv->rank; k++ )
            qty *= p_sv->dims[k];

        if ( p_sv->agg_type == VEC_ARRAY )
            qty *= p_sv->vec_size;
    }

    return qty;
}


/*****************************************************************
 * TAG( create_class_list )
 *
 * Create a list of classes of a mesh according to superclasses
 * passed in variable length argument list.
 *
 * Calling function assumes responsibility for freeing dynamically
 * allocated class list.
 */
void
create_class_list( int *p_qty, MO_class_data ***p_class_list, Mesh_data *p_mesh,
                   int qty_sclasses, ... )
{
    va_list ap;
    int i, class_qty, cur_class_qty, sclass;
    MO_class_data **mo_classes;

    /*
     * Loop over superclasses specified in arg list and extend current
     * class list to include classes from mesh for each superclass.
     */
    va_start( ap, qty_sclasses );

    mo_classes = NULL;
    class_qty = 0;
    for ( i = 0; i < qty_sclasses; i++ )
    {
        sclass = va_arg( ap, int );

        cur_class_qty = p_mesh->classes_by_sclass[sclass].qty;

        if ( cur_class_qty == 0 )
            continue;

        mo_classes = RENEW_N( MO_class_data *, mo_classes, class_qty,
                              cur_class_qty, "Extend class list" );

        memcpy( (void *) (mo_classes + class_qty),
                p_mesh->classes_by_sclass[sclass].list,
                cur_class_qty * sizeof( MO_class_data * ) );

        class_qty += cur_class_qty;
    }

    va_end( ap );

    *p_qty = class_qty;
    *p_class_list = mo_classes;
}


/*****************************************************************
 * TAG( intersect_srec_maps )
 *
 * Create an availability map which is the intersection of an
 * arbitrary number of Result availability maps.
 *
 * As implemented, intersect_srec_maps() only identifies subrecords
 * which support all the specified results.  This implies that no
 * intersection will exist when results come from different mesh
 * object classes, since those can only be written into different
 * subrecords.
 *
 * The following apply:
 *   -NULL results are treated as always available so they do not
 *    constrain the intersection, but all results may not be NULL.
 *
 * A more general approach could permit intersections that cross
 * classes by considering that different classes do in fact overlap
 * spatially in the mesh (i.e., the progression
 * "node->element->material->mesh" forms a spatial heirarchy), but
 * it could be very tedious to compute exactly the overlap of classes
 * based upon their subrecord bindings, and it's not clear that
 * there would be a useful advantage to such generality.
 */

/**/
/* Check this with indirect results */

Bool_type
intersect_srec_maps( List_head **p_srec_map, Result *results[], int result_qty,
                     Analysis *analy )
{
    int i, j, k, l;
    int first, next, qty_fmts, leading_nulls, support_qty;
    int src_subrec_id, test_subrec_id;
    Result *p_src;
    List_head *p_lh, *p_src_srec, *p_test_srec;
    Bool_type src_derived, test_derived, have_intersection;

    qty_fmts = analy->qty_srec_fmts;

    /* Clean up extant map. */
    if ( *p_srec_map != NULL )
    {
        p_lh = *p_srec_map;

        for ( i = 0; i < qty_fmts; i++ )
            if ( p_lh[i].qty != 0 )
                free( p_lh[i].list );

        free( p_lh );
        *p_srec_map = NULL;
    }

    p_lh = NEW_N( List_head, qty_fmts, "Srec map intersection heads" );

    /* Find first non-NULL result. */
    leading_nulls = 0;
    for ( first = 0; first < result_qty; first++ )
        if ( results[first] != NULL )
        {
            p_src = results[first];
            break;
        }
        else
            leading_nulls++;

    if ( leading_nulls == result_qty )
        return FALSE;

    have_intersection = FALSE;
    src_derived = (int) p_src->origin.is_derived;
    next = first + 1;

    /*
     * Using the first result as a source, search the srec maps
     * of the other results to determine if they all support their
     * respective results on the same subrecords as the source
     * result.
     */

    /* For each srec format... */
    for ( i = 0; i < qty_fmts; i++ )
    {
        p_src_srec = GET_INDEXED_SREC( p_src, i, src_derived );

        if ( p_src_srec->qty == 0 )
            /*
             * This result is not supported anywhere on the srec, so don't
             * bother checking the other results - i.e., the intersection
             * is empty for _this_ srec format.
             */
            continue;

        /* For each subrecord in the srec format... */
        for ( j = 0; j < p_src_srec->qty; j++ )
        {
            src_subrec_id = GET_INDEXED_SUBREC( p_src_srec, j, src_derived );

            support_qty = leading_nulls + 1; /* "1" accounts for the source. */

            /* For each result after the first... */
            for ( k = next; k < result_qty; k++ )
            {
                if ( results[k] == NULL )
                {
                    /* NULL results are always available. */
                    support_qty++;
                    continue;
                }

                test_derived = (int) results[k]->origin.is_derived;

                p_test_srec = GET_INDEXED_SREC( results[k], i, test_derived );

                if ( p_test_srec->qty == 0 )
                    /*
                     * The result isn't NULL, but it's not supported
                     * anywhere on the current (i'th) srec format.
                     * As above, the intersection must be empty.
                     */
                    break;

                /* Check to find a subrecord matching the "source" result's. */
                for ( l = 0; l < p_test_srec->qty; l++ )
                {
                    test_subrec_id = GET_INDEXED_SUBREC( p_test_srec, l,
                                                         test_derived );
                    if ( test_subrec_id == src_subrec_id )
                        break;
                }

                if ( l < p_test_srec->qty )
                    /* Broke the loop off; must have found a subrecord match. */
                    support_qty++;
            }

            if ( support_qty == result_qty )
            {
                /*
                 * Success - all results supported on the source's
                 * j'th subrecord.
                 */

                /* Add it to the list. */
                p_lh[i].list = (void *) RENEW_N( int, p_lh[i].list,
                                                 p_lh[i].qty, 1,
                                                 "New common subrecord" );
                ((int *) p_lh[i].list)[p_lh[i].qty] = src_subrec_id;
                p_lh[i].qty++;

                /* Note that the intersection is not empty. */
                have_intersection = TRUE;
            }
        }
    }

    if ( !have_intersection )
    {
        free( p_lh );
        return FALSE;
    }
    else
    {
        *p_srec_map = p_lh;
        return TRUE;
    }
}


/*****************************************************************
 * TAG( gen_component_results )
 *
 * Extract the components of a non-scalar primal as scalar results.
 */
Bool_type
gen_component_results( Analysis *analy, Primal_result *p_src, int *p_dest_idx,
                       int idx_limit, Result dest_array[] )
{
    State_variable *p_sv = p_src->var;
    int idx = *p_dest_idx;
    int j;
    char resnam[M_MAX_NAME_LEN];

    switch ( p_src->var->agg_type )
    {
    case VECTOR:
        for ( j = 0; j < p_sv->vec_size && idx < idx_limit; idx++, j++ )
        {
            /* Build component name. */
            sprintf( resnam, "%s[%s]", p_sv->short_name,
                     p_sv->components[j] );
            /* Prepare... */
            cleanse_result( dest_array + j );
            /* Go get it. */
            find_result( analy, PRIMAL, TRUE, dest_array + j, resnam );
        }
        break;
    case ARRAY:
        /* Only 1-D arrays are OK for this. */
        if ( p_sv->rank > 1 )
            return FALSE;

        for ( j = 0; j < p_sv->dims[0] && idx < idx_limit; idx++, j++ )
        {
            /* Build component name. */
            sprintf( resnam, "%s[%d]", p_sv->short_name, j );
            /* Prepare... */
            cleanse_result( dest_array + j );
            /* Go get it. */
            find_result( analy, PRIMAL, TRUE, dest_array + j, resnam );
        }
        break;
    default:
        /* Don't handle vector arrays... */
        return FALSE;
    }

    /* Update the current result index. */
    *p_dest_idx = idx;

    return TRUE;
}


/*****************************************************************
 * TAG( create_elem_class_node_list )
 *
 * Allocate and initialize an integer array listing all nodes
 * referenced by an element class.
 */
void
create_elem_class_node_list( Analysis *analy, MO_class_data *p_elem_class,
                             int *p_list_size, int **p_list )
{
    int conn_qty, obj_qty, node_cnt, node_tmp_size;
    int i, j;
    int superclass;
    int *connects, *node_list;
    unsigned char *node_tmp;

    /* Sanity check. */
    superclass = p_elem_class->superclass;
    if ( !IS_ELEMENT_SCLASS( superclass ) )
        return;

    conn_qty = qty_connects[superclass];
    node_tmp_size = MESH_P( analy )->node_geom->qty;
    node_tmp = NEW_N( unsigned char, node_tmp_size, "Temp node flags" );

    /* Mark all nodes referenced by elements; count marked nodes. */
    obj_qty = p_elem_class->qty;
    connects = p_elem_class->objects.elems->nodes;
    node_cnt = 0;
    for ( i = 0; i < obj_qty; i++, connects += conn_qty )
    {
        for ( j = 0; j < conn_qty; j++ )
        {
            if ( node_tmp[connects[j]] )
                continue;
            else
            {
                node_tmp[connects[j]] = 1;
                node_cnt++;
            }
        }
    }

    *p_list_size = node_cnt;

    /* Build list of marked nodes. */
    node_list = NEW_N( int, node_cnt, "Subrec node list" );

    node_cnt = 0;
    for ( i = 0; i < node_tmp_size; i++ )
    {
        if ( node_tmp[i] )
        {
            node_list[node_cnt] = i;
            node_cnt++;
        }
    }
    *p_list = node_list;

    free( node_tmp );
}


/*****************************************************************
 * TAG( reorder_float_array )
 *
 * Reorder an input array of floats or float vectors from an input
 * array to an output array according to an index array.  Returns
 * TRUE if successful.
 *
 */
Bool_type
reorder_float_array( int index_qty, int *index_list, int vec_size,
                     float *data_in, float *data_out )
{
    int i, j;
    float *vec_in, *vec_out;
    GVec2D *data2_in, *data2_out;
    GVec3D *data3_in, *data3_out;

    switch ( vec_size )
    {
    case 0:
        return FALSE;

    case 1:
        for ( i = 0; i < index_qty; i++ )
            data_out[index_list[i]] = data_in[i];
        break;

    case 2:
        data2_in = (GVec2D *) data_in;
        data2_out = (GVec2D *) data_out;
        for ( i = 0; i < index_qty; i++ )
        {
            vec_in = data2_in[i];
            vec_out = data2_out[index_list[i]];

            vec_out[0] = vec_in[0];
            vec_out[1] = vec_in[1];
        }
        break;

    case 3:
        data3_in = (GVec3D *) data_in;
        data3_out = (GVec3D *) data_out;
        for ( i = 0; i < index_qty; i++ )
        {
            vec_in = data3_in[i];
            vec_out = data3_out[index_list[i]];

            vec_out[0] = vec_in[0];
            vec_out[1] = vec_in[1];
            vec_out[2] = vec_in[2];
        }
        break;

    default:
        for ( i = 0; i < index_qty; i++ )
        {
            vec_in = data_in + i * vec_size;
            vec_out = data_out + index_list[i] * vec_size;

            for ( j = 0; j < vec_size; j++ )
                vec_out[j] = vec_in[j];
        }
    }

    return TRUE;
}


/*****************************************************************
 * TAG( reorder_double_array )
 *
 * Reorder an input array of doubles or double vectors from an input
 * array to an output array according to an index array.  Returns
 * TRUE if successful.
 *
 */
Bool_type
reorder_double_array( int index_qty, int *index_list, int vec_size,
                      double *data_in, double *data_out )
{
    int i, j;
    double *vec_in, *vec_out;
    GVec2D2P *data2_in, *data2_out;
    GVec3D2P *data3_in, *data3_out;

    switch ( vec_size )
    {
    case 0:
        return FALSE;

    case 1:
        for ( i = 0; i < index_qty; i++ )
            data_out[index_list[i]] = data_in[i];
        break;

    case 2:
        data2_in = (GVec2D2P *) data_in;
        data2_out = (GVec2D2P *) data_out;
        for ( i = 0; i < index_qty; i++ )
        {
            vec_in = data2_in[i];
            vec_out = data2_out[index_list[i]];

            vec_out[0] = vec_in[0];
            vec_out[1] = vec_in[1];
        }
        break;

    case 3:
        data3_in = (GVec3D2P *) data_in;
        data3_out = (GVec3D2P *) data_out;
        for ( i = 0; i < index_qty; i++ )
        {
            vec_in = data3_in[i];
            vec_out = data3_out[index_list[i]];

            vec_out[0] = vec_in[0];
            vec_out[1] = vec_in[1];
            vec_out[2] = vec_in[2];
        }
        break;

    default:
        for ( i = 0; i < index_qty; i++ )
        {
            vec_in = data_in + i * vec_size;
            vec_out = data_out + index_list[i] * vec_size;

            for ( j = 0; j < vec_size; j++ )
                vec_out[j] = vec_in[j];
        }
    }

    return TRUE;
}


/*****************************************************************
 * TAG( prep_object_class_buffers )
 *
 * Initialize the class data buffers referenced by the current result.
 * Note this skips the node class' buffer as that is explicitly cleared
 * in existing logic that uses it.
 */
void
prep_object_class_buffers( Analysis *analy, Result *p_r )
{
    Subrec_obj *p_so;
    MO_class_data *p_mo_class;
    int i, j;
    int limit, subrec;
    int int_args[3];
    int rval, dbid, qty_facets;
    float init_val;
    float *p_data_buffer;

    /* Get array of subrecords. */
    p_so = analy->srec_tree[p_r->srec_id].subrecs;

    /* Loop over result to get supporting subrecords of current format. */
    for ( i = 0; i < p_r->qty; i++ )
    {
        /* Get the mesh object class struct for current subrecord. */
        subrec = p_r->subrecs[i];
        p_mo_class = p_so[subrec].p_object_class;

        /* Don't bother for nodes. */
        if ( p_mo_class->superclass == G_NODE )
            continue;

        if ( p_mo_class->superclass == G_SURFACE )
        {
            dbid = analy->db_ident;
            limit = 0;
            for ( j = 0; j < p_mo_class->qty; j++ )
            {
                int_args[0] = p_mo_class->mesh_id;
                int_args[1] = j + 1;
                rval = mc_query_family( dbid, QTY_FACETS_IN_SURFACE,
                                        (void *) int_args,
                                        p_mo_class->short_name,
                                        (void *) &qty_facets );
                limit  += qty_facets;
            }
        }
        else
        {
            limit = p_mo_class->qty;
        }

        /* Init the class data buffer. */
        init_val = analy->buffer_init_val;
        p_data_buffer = p_mo_class->data_buffer;
        if(is_particle_class(analy, p_mo_class->superclass,p_mo_class->short_name ))
        {
            for ( j = 0; j < limit; j++)
            {
                if(MESH(analy).disable_particle[j])
                    p_data_buffer[j] = init_val;
            }

        }
        else
        {
            for ( j = 0; j < limit; j++ )
                p_data_buffer[j] = init_val;
        }
    }
}


/*****************************************************************
 * TAG( fr_state2 )
 *
 * State2 de-allocation.
 */
void fr_state2( State2 *p_state, Analysis *analy )
{
    int mesh_id, eclass_qty, i;
    int rval;

    if ( p_state != NULL )
    {
        if ( !p_state->position_constant )
            free( p_state->nodes.nodes3d );

        if ( p_state->sand_present )
        {
            rval = analy->db_query( analy->db_ident, QRY_SREC_MESH,
                                    (void *) &p_state->srec_id, NULL,
                                    (void *) &mesh_id );
            /* Error status not propagated up; pop a diagnostic. */
            if ( rval != OK )
                popup_dialog( WARNING_POPUP,
                              "fr_state2() - unable to query QRY_SREC_MESH!" );

            eclass_qty = analy->mesh_table[mesh_id].elem_class_qty;

            for ( i = 0; i < eclass_qty; i++ )
                free( p_state->elem_class_sand[i] );
            free( p_state->elem_class_sand );
        }

        if ( p_state->particles.particles2d != NULL )
            free( p_state->particles.particles2d );

        free( p_state );
    }
}


/*****************************************************************
 * TAG( set_ref_state )
 *
 * Set the reference state data pointer, performing I/O if necessary
 * to read in node positions.
 */
extern void
set_ref_state( Analysis *analy, int new_ref_state )
{
    int qty,
        srec_id,
        mesh_id,
        rval,
        first_state=1;
    State_rec_obj *p_sro = NULL;
    int index;
    int num_nodes=0;
    Mesh_data *p_md = NULL;
    MO_class_data *p_node_class = NULL;

    p_node_class = MESH_P( analy )->node_geom;
    num_nodes    = p_node_class->qty;

    /*
     * If new reference state indicates mesh geom data, always make the
     * assignment so this routine can do the right thing at startup.
     */
    if ( new_ref_state == 0 )
    {
        /* Use initial nodal coordinates for reference. */
        analy->cur_ref_state_data = MESH( analy ).node_geom->objects.nodes;
        analy->reference_state    = 0;

        if (MESH_P( analy )->double_precision_nodpos)
        {
            /* Init arguments needed for load_nodpos(). */

            /* Get the state rec format of the requested state. */
            rval = analy->db_query( analy->db_ident, QRY_SREC_FMT_ID,
                                    (void *) &first_state, NULL,
                                    (void *) &srec_id );
            if ( rval != 0 )
                return;

            p_sro = analy->srec_tree + srec_id;
            mesh_id = p_sro->subrecs[0].p_object_class->mesh_id;
            p_md = analy->mesh_table + mesh_id;

            if (!analy->cur_ref_state_dataDp)
            {
                analy->cur_ref_state_dataDp = NEW_N( double, num_nodes*analy->dimension,
                                 "Tmp DP node cache" );
            }

            load_nodpos( analy, p_sro, p_md, analy->dimension, 1,
                         FALSE, (void *) analy->cur_ref_state_dataDp );

        }
    }
    else if ( new_ref_state == analy->reference_state )
    {
        /* Don't do work if we don't need to. */
        return;
    }
    else
    {
        /* Not the current and not the original - have to do I/O. */

        if ( analy->cur_ref_state_data == NULL )
        {
            /* Need buffer. */
            qty = MESH( analy ).node_geom->qty * analy->dimension;
            analy->cur_ref_state_data = NEW_N( float, qty, "Ref st node buffer" );
        }

        /* Get the max state possible (may be a user set limit). */
        qty = get_max_state( analy ) + 1;
        if ( new_ref_state > qty )
        {
            popup_dialog( INFO_POPUP, "Requested reference state exceeds "
                          "maximum (%d).", qty );
            return;
        }

        /* Get the state rec format of the requested state. */
        rval = analy->db_query( analy->db_ident, QRY_SREC_FMT_ID,
                                (void *) &new_ref_state, NULL,
                                (void *) &srec_id );
        if ( rval != 0 )
            return;

        /* Init arguments needed for load_nodpos(). */
        p_sro = analy->srec_tree + srec_id;
        mesh_id = p_sro->subrecs[0].p_object_class->mesh_id;
        p_md = analy->mesh_table + mesh_id;

        /* Last QC check; there needs to be node position data at the state. */
        if ( p_sro->node_pos_subrec == -1 )
        {
            popup_dialog( INFO_POPUP, "Nodal coordinate data does not exist at"
                          "\nrequested reference state; request aborted." );
            return;
        }

        /* Go get it... */
        analy->reference_state = new_ref_state;
        load_nodpos( analy, p_sro, p_md, analy->dimension, new_ref_state,
                     TRUE, analy->cur_ref_state_data );

        /* Assign the reference pointer to the data that was just read in. */

        if (MESH_P( analy )->double_precision_nodpos)
        {
            if ( !analy->cur_ref_state_dataDp )
            {
                analy->cur_ref_state_dataDp = NEW_N( double, num_nodes*analy->dimension,
                                 "Tmp DP node cache" );
            }

            load_nodpos( analy, p_sro, p_md, analy->dimension, new_ref_state,
                         FALSE, analy->cur_ref_state_dataDp );

        }

    }
}


#ifdef NEW_TMP_RESULT_USE
typedef struct _data_buffer_ref
{
    struct _data_buffer_ref *next;
    struct _data_buffer_ref *prev;
    struct _data_buffer_ref *last;
    float *buffer;
    int index;
    int contiguous_qty;
} Data_buffer_ref;

static Data_buffer_ref *free_head = NULL;
static Data_buffer_ref *free_tail = NULL;
static Data_buffer_ref *alloc_head = NULL;
static Data_buffer_ref pre_alloc_bufs[TEMP_RESULT_ARRAY_QTY];
static int buffer_size;
static Data_buffer_ref *extra_head = NULL;


/*****************************************************************
 * TAG( init_data_buffers )
 *
 * Initialize temporary data buffer management.
 */
void
init_data_buffers( int data_buffer_size )
{
    int i;

    buffer_size = data_buffer_size;

    pre_alloc_bufs[0].buffer = NEW_N( float,
                                      TEMP_RESULT_ARRAY_QTY * buffer_size,
                                      "Tmp result cache" );

    pre_alloc_bufs[0].prev = NULL;
    pre_alloc_bufs[TEMP_RESULT_ARRAY_QTY - 1].next = NULL;

    for ( i = 0; i < TEMP_RESULT_ARRAY_QTY ; i++ )
    {
        if ( i < TEMP_RESULT_ARRAY_QTY - 1 )
        {
            pre_alloc_bufs[i].next = pre_alloc_bufs + (i + 1);
        }

        pre_alloc_bufs[i].last = &pre_alloc_bufs[i];
        pre_alloc_bufs[i].index = i;
        pre_alloc_bufs[i].contiguous_qty = TEMP_RESULT_ARRAY_QTY - i;

        if ( i > 0 )
        {
            pre_alloc_bufs[i].prev = pre_alloc_bufs + (i - 1);
            pre_alloc_bufs[i].buffer = pre_alloc_bufs[i - 1].buffer
                                       + buffer_size;
        }
    }

    free_head = &pre_alloc_bufs[0];
    free_tail = &pre_alloc_bufs[TEMP_RESULT_ARRAY_QTY - 1];
    alloc_head = NULL;
}


/*****************************************************************
 * TAG( get_data_buffer )
 *
 * Assign one or more contiguous data buffers for the caller,
 * allocating if necessary.
 */
Bool_type
get_data_buffer( int bufqty, float **p_buffer, char *caller_msg )
{
    Data_buffer_ref *p_dbr, *p_dbr2, *first, *last;
    int i;

    /* Search pre-allocated buffers to accommodate request. */
    for ( p_dbr = free_head; p_dbr != NULL; NEXT( p_dbr ) )
    {
        if ( p_dbr->contiguous_qty >= bufqty )
        {
            first = p_dbr;
            last = first + (bufqty - 1);

            /* Unlink list of free buffers. */
            if ( first->prev != NULL )
                first->prev->next = last->next;
            else
                /* First is head. */
                free_head = last->next;

            if ( last->next != NULL )
                last->next->prev = first->prev;

            first->prev = NULL;
            last->next = NULL;

            break;
        }
    }

    if ( p_dbr == NULL )
    {
        /* Couldn't accommodate request with pre-allocated buffers. */

        p_dbr = NEW( Data_buffer_ref, "Dynamic temp buffer ref" );
        if ( p_dbr == NULL )
            return FALSE;

        p_dbr->index = -1;
        p_dbr->buffer = NEW_N( float, bufqty * buffer_size,
                               "Dynamic temp buffers" );
        if ( p_dbr->buffer != NULL )
        {
            /**/
            fprintf( stderr, "%s\n\"%s\";\n%s\n",
                     "Extra result buffer allocation performed for",
                     caller_msg, "Please notify MDG." );
            INSERT( p_dbr, extra_head );
            *p_buffer = p_dbr->buffer;
            return TRUE;
        }
        else
        {
            free( p_dbr );
            return FALSE;
        }
    }

    /* Indicate how many contiguous buffers were allocated. */
    for ( i = 0, p_dbr = first; i < bufqty; i++, NEXT( p_dbr ) )
        p_dbr->contiguous_qty = bufqty - i;

    /* Quick reference last from first. */
    first->last = last;

    /* Insert found pre-allocated buffers into used-buffer list. */
    p_dbr2 = NULL;
    for ( p_dbr = alloc_head; p_dbr != NULL; NEXT( p_dbr ) )
    {
        if ( p_dbr->index > first->index )
        {
            if ( p_dbr->prev != NULL )
            {
                p_dbr->prev->next = first;
                first->prev = p_dbr->prev;
            }
            else
                alloc_head = first;

            if ( p_dbr->next != NULL )
            {
                p_dbr->next->prev = last;
                last->next = p_dbr->next;
            }

            break;
        }

        p_dbr2 = p_dbr;
    }

    if ( p_dbr == NULL )
    {
        if ( p_dbr2 == NULL )
            alloc_head = first;
        else
        {
            first->prev = p_dbr2;
            p_dbr2->next = first;
        }
    }

    *p_buffer = first->buffer;

    return TRUE;
}


/*****************************************************************
 * TAG( put_data_buffer )
 *
 * Return result data buffer(s) for reuse.
 */
Bool_type
put_data_buffer( float *buffer )
{
    Data_buffer_ref *p_dbr;
    int contig_counter;

    /* Search among used pre-allocated buffers to match. */
    p_dbr = alloc_head;
    contig_counter = 0;
    while ( p_dbr != NULL )
    {
        if ( contig_counter > 0 )
            NEXT( p_dbr );
        else if ( p_dbr->buffer != buffer )
        {
            contig_counter = p_dbr->contiguous_qty;
            NEXT( p_dbr );
        }
        else if ( p_dbr->buffer == buffer )
        {
            /* What goes here? */
        }

        contig_counter--;
    }

    return TRUE;
}
#endif


/************************************************************
 * TAG( gen_material_node_list )
 *
 * Traverse material element list to generate node list.
 */
extern void
gen_material_node_list( Mesh_data *p_mesh, int mat, Material_data *p_md )
{
    int i, j, k;
    int last;
    int block_qty, conn_qty, node_qty;
    int *connects, *elem_conns, *mat_nodes;
    Int_2tuple *p_i2t;

    node_qty = p_mesh->node_geom->qty;
    mat_nodes = NEW_N( int, node_qty, "Material nodes array" );
    connects = p_md->elem_class->objects.elems->nodes;
    conn_qty = qty_connects[p_md->elem_class->superclass];

    /* Traverse material's element list to mark all connected nodes. */
    block_qty = p_md->elem_block_qty;
    for ( i = 0; i < block_qty; i++ )
    {
        last = p_md->elem_blocks[i][1];
        j = p_md->elem_blocks[i][0];
        elem_conns = connects + j * conn_qty;

        for ( ; j <= last; j++ )
        {
            for ( k = 0; k < conn_qty; k++ )
                mat_nodes[elem_conns[k]] = 1;

            elem_conns += conn_qty;
        }
    }

    /* Re-init block_qty for use with node blocks. */
    block_qty = 0;

    /* Generate the first/last tuples of nodes connected to the material. */
    i = 0;
    p_i2t = NULL;
    while ( i < node_qty )
    {
        if ( mat_nodes[i] == 0 )
        {
            /* Find end of un-used nodes. */
            do
                i++;
            while ( i < node_qty && mat_nodes[i] == 0 );
        }

        if ( i < node_qty && mat_nodes[i] > 0 )
        {
            /* Record the block start. */
            p_i2t = RENEW_N( Int_2tuple, p_i2t, block_qty, 1,
                             "Extend mtl node blk" );
            p_i2t[block_qty][0] = i;

            /* Find end of connected nodes. */
            do
                i++;
            while ( i < node_qty && mat_nodes[i] > 0 );

            /* Record the block end. */
            p_i2t[block_qty][1] = i - 1;
            block_qty++;
        }
    }

    /* Save node blocks in Material data. */
    p_md->node_block_qty = block_qty;
    p_md->node_blocks = p_i2t;

    free( mat_nodes );
}


/*****************************************************************
 * TAG( reset_autoimg_count )
 *
 * Reset the animation frame count so any subsequent output
 * sequence will begin with suffix 1.
 */
extern void
reset_autoimg_count()
{
    autoimg_frame_count = 1;
}


/*****************************************************************
 * TAG( output_image )
 *
 * Logic to output the current image of an animation.
 */
extern void
output_image( Analysis *analy )
{
    extern int jpeg_quality;
    extern int png_compression_level;
    char filename[128];
    char *suffix;

    switch ( analy->autoimg_format )
    {
    case IMAGE_FORMAT_RGB:
        suffix = "rgb";
        break;
#ifdef JPEG_SUPPORT
    case IMAGE_FORMAT_JPEG:
        suffix = "jpg";
        break;
#endif
#ifdef PNG_SUPPORT
    case IMAGE_FORMAT_PNG:
        suffix = "png";
        break;
#endif
    default:
        popup_dialog( WARNING_POPUP,
                      "Unknown/uncompiled output image format requested; "
                      "aborting." );
        return;
    }

    sprintf( filename, "%s%04d.%s", analy->img_root, autoimg_frame_count,
             suffix );

    switch ( analy->autoimg_format )
    {
    case IMAGE_FORMAT_RGB:
        screen_to_rgb( filename, FALSE );
        break;
#ifdef JPEG_SUPPORT
    case IMAGE_FORMAT_JPEG:
        write_JPEG_file( filename, FALSE, jpeg_quality );
        break;
#endif
#ifdef PNG_SUPPORT
    case IMAGE_FORMAT_PNG:
        write_PNG_file( filename, FALSE, png_compression_level );
        break;
#endif
    }

    autoimg_frame_count++;
}


/*****************************************************************
 * TAG( rotation_angles )
 *
 * compute rotations from transformation matrix
 */
int
rotation_angles( float *theta, float *phi, float *psi, Transf_mat transformation )
{
    float
    pi
    ,two_pi;

    float
    radians_to_degrees;

    float
    phi_0
    ,phi_1
    ,psi_0
    ,psi_1
    ,psi_2
    ,psi_3
    ,theta_0
    ,theta_1
    ,theta_2
    ,theta_3;

    float
    cosine_phi_0
    ,cosine_phi_1;

    float
    cosine_psi
    ,cosine_theta
    ,sine_phi
    ,sine_psi
    ,sine_theta;

    float
    combination[8][3];

    float
    f1
    ,f2
    ,f3
    ,f4;

    float
    tolerance;

    int
    i
    ,solved;


    /* */

    /*
     * Concatenated rotation matricies:  rotation_x(theta) * rotation_y(phi) * rotation_z(psi) =
     *
     *     cosine_phi * cosine_psi                    cosine_phi * sine_psi                   -sine_phi
     *
     *     sine_theta * sine_phi * cosine_psi -       sine_theta * sine_phi * sine_psi +       sine_theta * cosine_phi
     *     cosine_theta * sine_psi                    cosine_theta * cosine_psi
     *
     *     cosine_theta * sine_phi * cosine_psi +     cosine_theta * sine_phi * sine_psi -     cosine_theta * cosine_phi
     *     sine_theta * sine_psi                      sine_theta * cosine_psi
     */


    pi     = 4.0 * atan2( 1.0, 1.0 );
    two_pi = pi * 2.0;

    radians_to_degrees = 180.0 / pi;


    /*
     * establish permutations of angles
     */

    phi_0 = asin( -(double)transformation.mat[0][2] );
    phi_1 = pi - phi_0;

    if ( fabs( (double)transformation.mat[0][2] ) < 0.99999 )
    {
        cosine_phi_0 = cos( (double)phi_0 );
        cosine_phi_1 = cos( (double)phi_1 );

        psi_0 = acos( (double)transformation.mat[0][0] / (double)cosine_phi_0 );
        psi_1 = two_pi - psi_0;
        psi_2 = acos( (double)transformation.mat[0][0] / (double)cosine_phi_1 );
        psi_3 = two_pi - psi_2;

        theta_0 = acos( (double)transformation.mat[2][2] / (double)cosine_phi_0 );
        theta_1 = two_pi - theta_0;
        theta_2 = acos( (double)transformation.mat[2][2] / (double)cosine_phi_1 );
        theta_3 = two_pi - theta_2;
    }
    else
    {
        /*
         * phi = 90 degrees
         *
         * assume theta = 0 degrees, therefore placing these values ( phi = 90; theta = 0 )
         * into transformation matrix[1][0] yields sine of psi
         */

        theta_0 = 0.0;
        theta_1 = 0.0;
        theta_2 = 0.0;
        theta_3 = 0.0;

        psi_0 = asin( -(double)transformation.mat[1][0] );
        psi_1 = pi - psi_0;
        psi_2 = psi_0;
        psi_3 = psi_1;
    }


    /*
     * assign angle permutation for theta, phi, and psi
     *
     * examination of rotation matrix reveals:
     *
     *     theta depends upon phi
     *     psi   depends upon phi
     *
     *     theta is not a result of psi
     *
     *     therefore, 8 possible combinations result:
     *
     *             phi_0              phi_1
     *
     *        theta_0  psi_0     theta_1  psi_1
     *        theta_1  psi_1     theta_2  psi_2
     */

    combination[0][0] = theta_0;
    combination[0][1] = phi_0;
    combination[0][2] = psi_0;

    combination[1][0] = theta_0;
    combination[1][1] = phi_0;
    combination[1][2] = psi_1;

    combination[2][0] = theta_1;
    combination[2][1] = phi_0;
    combination[2][2] = psi_0;

    combination[3][0] = theta_1;
    combination[3][1] = phi_0;
    combination[3][2] = psi_1;

    combination[4][0] = theta_2;
    combination[4][1] = phi_1;
    combination[4][2] = psi_2;

    combination[5][0] = theta_2;
    combination[5][1] = phi_1;
    combination[5][2] = psi_3;

    combination[6][0] = theta_3;
    combination[6][1] = phi_1;
    combination[6][2] = psi_2;

    combination[7][0] = theta_3;
    combination[7][1] = phi_1;
    combination[7][2] = psi_3;


    /*
     * test to determine correct combinationination of angles
     * by comparing concatenated rotation matrix against
     * actual values contained in transformation matrix
     */

    solved = FALSE;
    tolerance = 4.0e-07;

    while ( solved == FALSE )
    {
        /*
         * NOTE:  Code should execute outer loop ONCE if no errors are present
         */

        i = 0;

        tolerance *= 10.0;

        while ( (solved == FALSE) && (i < 8) )
        {
            sine_theta   = sin( (double)combination[i][0] );
            cosine_theta = cos( (double)combination[i][0] );

            sine_phi   = sin( (double)combination[i][1] );

            sine_psi   = sin( (double)combination[i][2] );
            cosine_psi = cos( (double)combination[i][2] );


            f1 = fabs( (double)((cosine_psi * sine_phi * sine_theta) - (sine_psi * cosine_theta) -
                                transformation.mat[1][0]) );

            if ( f1 <= tolerance )
            {
                f2 = fabs( (double)((cosine_psi * sine_phi * cosine_theta) + (sine_psi * sine_theta) -
                                    transformation.mat[2][0]) );
                f3 = fabs( (double)((sine_psi * sine_phi * sine_theta) + (cosine_psi * cosine_theta) -
                                    transformation.mat[1][1]) );
                f4 = fabs( (double)((sine_psi * sine_phi * cosine_theta) - (cosine_psi * sine_theta) -
                                    transformation.mat[2][1]) );

                if ( f1 + f2 + f3 + f4 < tolerance )
                    solved = TRUE;
            }

            i++;
        }
    }


    i--;

    if ( tolerance < 0.01 )
    {
        *theta = radians_to_degrees * combination[i][0];
        *phi   = radians_to_degrees * combination[i][1];
        *psi   = radians_to_degrees * combination[i][2];

        if ( *theta < 0.0 )
            *theta += 360.0;

        if ( *phi < 0.0 )
            *phi += 360.0;

        if ( *psi < 0.0 )
            *psi += 360.0;

        return( EXIT_SUCCESS );
    }
    else
    {
        /*
         * errors in decomposition of rotation matrix
         */

        *theta = 0.0;
        *phi   = 0.0;
        *psi   = 0.0;

        return( EXIT_FAILURE );
    }
}


/*****************************************************************
 * TAG( outview )
 *
 * option for user to save (in GRIZ command form)
 * the complete state viewing transformation (see "draw_mesh_view" )
 */
extern void
outview( FILE *fp, Analysis *analy )
{
    float theta, phi, psi;
    int status;


    /* */

    /*
     * Re-create GRIZ command sequence to establish the viewing transformation
     * at any given state as performed in procedure "draw_mesh_view".
     *
     *
     * from draw_mesh_view:
     *
     * E.  look_rot_mat( v_win->look_from, v_win->look_at, v_win->lookup, &look_rot );
     *
     *     mat_to_array( &look_rot, arr );
     *     glMultMatrix( arr );
     *
     *     glTranslatef( -v_win->look_from[0], -v_win->look_from[1], -v_win->look_from[2] );
     *
     * D.  glTranslatef( v_win->trans[0], v_win->trans[1], v_win->trans[2] );
     *
     * C.  mat_to_array( &v_win->rot_mat, arr );
     *
     *     glMultMatrix( arr );
     *
     * A.  scal = v_win->bbox_scale;
     * B.  glScalef( scal * v_win->scale[0], scal * v_win->scale[1], scal * v_win->scale[2] );
     * A.  glTranslatef( v_win->bbox_trans[0], v_win->bbox_trans[1], v_win->bbox_trans[2] );
     */

    /*
     * NOTE:  viewing transformation sequence of operations MUST be performed in "reverse"
     *        order to meet requirements of OpenGL
     */

    /*
     * A.  Translation of bounding box
     *
     *     "bbox" -->
     *                "set_view_to_bbox":  sets:  v_win->bbox_trans[i]
     *                                            v_win->bbox_scale
     */

    fprintf( fp, "bbox %f %f %f %f %f %f\n"
             ,analy->bbox[0][0]
             ,analy->bbox[0][1]
             ,analy->bbox[0][2]
             ,analy->bbox[1][0]
             ,analy->bbox[1][1]
             ,analy->bbox[1][2] );


    /*
     * A-1.  Reset view
     */

    fprintf( fp, "rview\n" );



    /*
     * B.  Scaling of window axes within bounding box
     *
     *     "scalax" -->
     *                  "set_mesh_scale":  sets:  v_win->scale[i]
     */

    fprintf( fp, "scalax %f %f %f\n"
             ,v_win->scale[0]
             ,v_win->scale[1]
             ,v_win->scale[2] );


    /*
     * C.  Transformation matrix -- rotations
     */

    status = rotation_angles( &theta, &phi, &psi, v_win->rot_mat );

    if ( 0 == status )
    {
        /*
         * IMPORTANT NOTE:  All angles are measured from normal axes position
         *                  therefore command "rview" MUST be issued!!!!!
         */

        /*
        fprintf( fp, "rview\n" );
         */
        fprintf( fp, "rx %f\nry %f\nrz %f\n", theta, phi, psi );
    }
    else
    {
        popup_dialog( WARNING_POPUP,
                      "Unable to decompose rotation angles" );
    }


    /*
     * D.  Translation of view
     *
     *     "tx" "ty" "tz" -->
     *                           perform "absolute mesh translation(s)" [by virtue of
     *                           rview command which "zeros" translation matrix]:  sets v_win->trans[i]
     */

    fprintf( fp, "tx %f\nty %f\ntz %f\n"
             ,v_win->trans[0]
             ,v_win->trans[1]
             ,v_win->trans[2] );


    /*
     * E.  Set look-from point, look-at point, and look-up vector
     *
     *     "lookfr" -->
     *                  "look_from":  sets v_win->look_from[i]
     *
     *     "lookat" -->
     *                  "look_at":    sets v_win->look_at[i]
     *
     *     "lookup" -->
     *                  "look_up":    sets v_win->look_up[i]
    */

    fprintf( fp, "lookfr %f %f %f\nlookat %f %f %f\nlookup %f %f %f\n"
             ,v_win->look_from[0]
             ,v_win->look_from[1]
             ,v_win->look_from[2]
             ,v_win->look_at[0]
             ,v_win->look_at[1]
             ,v_win->look_at[2]
             ,v_win->look_up[0]
             ,v_win->look_up[1]
             ,v_win->look_up[2] );


    return;
}


/*****************************************************************
 * TAG( string_to_upper )
 *
 * Converts an input string to uppercase.
 */
void
string_to_upper( char *input_str, char *upper_str )
{
    int i, len=0;
    char tmp_str[64];

    len = strlen( input_str );

    for ( i=0;
            i < len;
            i++)
    {
        tmp_str[i] = toupper( input_str[i] );
    }

    tmp_str[len] = '\0';
    strcpy( upper_str, tmp_str );
}


/*****************************************************************
 * TAG( write_log_message )
 *
 * This function will generate an entry in the Griz usage log file.
 */
void
write_log_message( void )
{
    FILE *fp;

    char logFileName[128];
    double elapsed_time = 0.;

    float ut, st;
    long maxrs, rs, minpf, majpf, ns, in, out;

    /* Get timing info for this session */
    timing_vals( &ut, &st, &maxrs, &rs, &minpf, &majpf, &ns, &in, &out );
    env.time_stop = time( NULL );
    elapsed_time = difftime( env.time_stop, env.time_start );

    if ( env.enable_logging)
    {
        strcpy(logFileName, env.grizlogdir);
        strcat(logFileName, "/");
        strcat(logFileName, env.grizlogfile);

        fp = fopen(logFileName, "a" );

        fprintf(fp, "\nUSER=[%s] DATE=[%s] TIME=[%s] HOST=[%s] MACHINE=[%s] OS=[%s] ELAPSED_TIME(sec)=[%7.0f] CMD=[%s] VERSION=[%s/Mili=%s]",
                env.user_name,
                env.date,
                env.time,
                env.host,
                env.arch,
                env.systype,
                elapsed_time,
                env.command_line,
                PACKAGE_VERSION, MILI_VERSION);

        fclose(fp);
    }
}



/*****************************************************************
 * TAG( get_griz_session )
 *
 * This function will save analy session vars to the session data
 * structure.
 */
void
get_griz_session( Analysis *analy,  Session *session )
{

    update_session( GET, NULL );
    get_window_attributes();
}


/*****************************************************************
 * TAG( put_griz_session )
 *
 * This function will restore analy session vars to the analysis
 * data structure.
 */
void
put_griz_session( Analysis *analy,  Session *session )
{
    update_session( PUT, NULL );
    put_window_attributes();
}


/*****************************************************************
 * TAG( write_griz_session_file )
 *
 * This function will write all Griz session variables to a file.
 */
void
write_griz_session_file( Analysis *analy, Session *session, char *sessionFileName,
                         int session_id, Bool_type global )
{
    FILE *fp;
    char fullpath[M_MAX_NAME_LEN], session_id_string[16];

    if ( session_id>0 )
    {
        sprintf( session_id_string, "%d", session_id );
    }

    get_griz_session( analy, session );

    if ( global )
    {
        /* Open the Global session file */
        strcpy(fullpath, getenv("HOME"));
        strcat(fullpath, "/");
        strcat(fullpath,sessionFileName);

        if ( session_id>0 )
        {
            strcat( fullpath, session_id_string );
        }

        fp = fopen(fullpath, "w" );

        fprintf(fp, "* Griz session file [global variables] written on: %s\n", env.date);
        fprintf(fp, "*\n");

        fprintf(fp, "Win-Render-Height\t%d\n",   (int) session->win_render_size[0]);
        fprintf(fp, "Win-Render-Length\t%d\n",   (int) session->win_render_size[1]);
        fprintf(fp, "Win-Render-PosX\t\t%d\n",   (int) session->win_render_pos[0]);
        fprintf(fp, "Win-Render-PosY\t\t%d\n\n", (int) session->win_render_pos[1]);

        fprintf(fp, "Win-Ctl-Height\t\t%d\n", (int) session->win_ctl_size[0]);
        fprintf(fp, "Win-Ctl-Length\t\t%d\n", (int) session->win_ctl_size[1]);
        fprintf(fp, "Win-Ctl-PosX\t\t%d\n",   (int) session->win_ctl_pos[0]);
        fprintf(fp, "Win-Ctl-PosY\t\t%d\n\n", (int) session->win_ctl_pos[1]);

        /* Material Panel Window */
        if ( session->win_mtl_active != 0 )
        {
            fprintf(fp, "Win-Mtl-Active\n");
            fprintf(fp, "Win-Mtl-Height\t\t%d\n", (int) session->win_mtl_size[0]);
            fprintf(fp, "Win-Mtl-Length\t\t%d\n", (int) session->win_mtl_size[1]);
            fprintf(fp, "Win-Mtl-PosX\t\t%d\n",   (int) session->win_mtl_pos[0]);
            fprintf(fp, "Win-Mtl-PosY\t\t%d\n\n", (int) session->win_mtl_pos[1]);
        }

        /* Utility Panel Window */
        if ( session->win_util_active != 0 )
        {
            fprintf(fp, "Win-Util-Active\n");
            fprintf(fp, "Win-Util-Height\t\t%d\n", (int) session->win_util_size[0]);
            fprintf(fp, "Win-Util-Length\t\t%d\n", (int) session->win_util_size[1]);
            fprintf(fp, "Win-Util-PosX\t\t%d\n",   (int) session->win_util_pos[0]);
            fprintf(fp, "Win-Util-PosY\t\t%d\n\n", (int) session->win_util_pos[1]);
        }

        /* Surface Manager Window */
        if ( session->win_surf_active != 0 )
        {
            fprintf(fp, "Win-Surf-Active\n");
            fprintf(fp, "Win-Surf-Height\t\t%d\n", (int) session->win_surf_size[0]);
            fprintf(fp, "Win-Surf-Length\t\t%d\n", (int) session->win_surf_size[1]);
            fprintf(fp, "Win-Surf-PosX\t\t%d\n",   (int) session->win_surf_pos[0]);
            fprintf(fp, "Win-Surf-PosY\t\t%d\n\n", (int) session->win_surf_pos[1]);
        }
        update_session( WRITE, fp );
    }
    else
    {
        strcpy(fullpath, getenv("HOME"));
        strcat(fullpath, "/");
        strcat(fullpath,sessionFileName);

        if ( session_id>0 )
        {
            strcat( fullpath, session_id_string );
        }

        fp = fopen(fullpath, "w" );

        fprintf(fp, "* Griz session file [plot variables] written on: %s\n", env.date);
        fprintf(fp, "*\n");

        update_session( WRITE, fp );
    }

    fclose(fp);
}


/*****************************************************************
 * TAG( init_griz_session )
 *
 * This function will initialize a session data structure.
 */
void
init_griz_session( Session *session1 )
{
    session1->win_mtl_active =  0;
    session1->win_util_active = 0;
    session1->win_surf_active = 0;
}


/*****************************************************************
 * TAG( read_griz_session_file )
 *
 * This function will read all Griz session variables from a file.
 */
int
read_griz_session_file( Session *session, char *sessionFileName,
                        int session_id,   Bool_type global )
{
    FILE *fp;
    char input[120];
    int temp_int;
    char fullpath[256], var_name[M_MAX_NAME_LEN], var_value[64],
         session_id_string[16];
    int i;

    char *session_file_buff[2000];
    int session_file_buff_cnt=0;

    if ( session_id>0 )
    {
        sprintf( session_id_string, "%d", session_id );
    }

    if ( global )
    {
        /* Open the Global session file */
        strcpy(fullpath, getenv("HOME"));
        strcat(fullpath, "/");
        strcat(fullpath,sessionFileName);

        if ( session_id>0 )
        {
            strcat( fullpath, session_id_string );
        }

        fp = fopen(fullpath, "r" );
    }
    else
    {
        strcpy(fullpath,sessionFileName);

        if ( session_id>0 )
        {
            strcat( fullpath, session_id_string );
        }

        fp = fopen(fullpath, "r" );
    }

    /* Exit if the session file is not found */
    if ( fp==NULL )
        return (!OK);

    /* Initialize the activity flags for all optional widgets */

    session->win_mtl_active =  0;
    session->win_util_active = 0;
    session->win_surf_active = 0;

    fgets(input, 120, fp);
    while ( !feof( fp ) )
    {
        fgets(input, 120, fp);

        /* Skip comment lines */
        if ( input[0]=='*' || input[0]=='#' )
        {
            continue;
        }

        sscanf(input,"%s %s", var_name, var_value);

        if ( !strcmp("Win-Render-Height", var_name) )
            session->win_render_size[0] = atoi(var_value);
        if ( !strcmp("Win-Render-Length", var_name) )
            session->win_render_size[1] = atoi(var_value);
        if ( !strcmp("Win-Render-PosX", var_name) )
            session->win_render_pos[0] = atoi(var_value);
        if ( !strcmp("Win-Render-PosY", var_name) )
            session->win_render_pos[1] = atoi(var_value);
        if ( !strcmp("Win-Ctl-Height", var_name) )
            session->win_ctl_size[0] = atoi(var_value);
        if ( !strcmp("Win-Ctl-Length", var_name) )
            session->win_ctl_size[1] = atoi(var_value);
        if ( !strcmp("Win-Ctl-PosX", var_name) )
            session->win_ctl_pos[0] = atoi(var_value);
        if ( !strcmp("Win-Ctl-PosY", var_name) )
            session->win_ctl_pos[1] = atoi(var_value);

        if ( !strcmp("Win-Mtl-Active", var_name) )
            session->win_mtl_active = 1;
        if ( !strcmp("Win-Mtl-Height", var_name) )
            session->win_mtl_size[0] = atoi(var_value);
        if ( !strcmp("Win-Mtl-Length", var_name) )
            session->win_mtl_size[1] = atoi(var_value);
        if ( !strcmp("Win-Mtl-PosX", var_name) )
            session->win_mtl_pos[0] = atoi(var_value);
        if ( !strcmp("Win-Mtl-PosY", var_name) )
            session->win_mtl_pos[1] = atoi(var_value);

        if ( !strcmp("Win-Util-Active", var_name) )
            session->win_util_active = 1;
        if ( !strcmp("Win-Util-Height", var_name) )
            session->win_util_size[0] = atoi(var_value);
        if ( !strcmp("Win-Util-Length", var_name) )
            session->win_util_size[1] = atoi(var_value);
        if ( !strcmp("Win-Util-PosX", var_name) )
            session->win_util_pos[0] = atoi(var_value);
        if ( !strcmp("Win-Util-PosY", var_name) )
            session->win_util_pos[1] = atoi(var_value);

        if ( !strcmp("Win-Surf-Active", var_name) )
        {
            session->win_surf_active = 1;
        }
        if ( !strcmp("Win-Surf-Height", var_name) )
        {
            session->win_surf_size[0] = atoi(var_value);
        }
        if ( !strcmp("Win-Surf-Length", var_name) )
        {
            session->win_surf_size[1] = atoi(var_value);
        }
        if ( !strcmp("Win-Surf-PosX", var_name) )
            session->win_surf_pos[0] = atoi(var_value);
        if ( !strcmp("Win-Surf-PosY", var_name) )
            session->win_surf_pos[1] = atoi(var_value);

        if ( strstr(input, "var_name=" ) )
            session_file_buff[session_file_buff_cnt++] = strdup(input);
    }

    update_session( READ, (void *) session_file_buff, session_file_buff_cnt );
    for ( i=0;
            i<session_file_buff_cnt;
            i++ )
        free( session_file_buff[i] );

    if (fp)
        fclose(fp);

    return OK;
}

/*****************************************************************
 * TAG( replace_string )
 *
 * This function will replace sub_str with all occurances of
 * replace_str in the input string.
 */
char *
replace_string( char *input_str, char *sub_str, char *replace_str )
{
    static char str_buffer[512];
    char tmp_str[1024];
    char *str_ptr=NULL;
    int repl_index=0, index2=0;
    Bool_type string_found=TRUE;

    /* Do a quick check to see if the string is in the input.
     * if not return original string.
     */
    str_ptr = strstr( input_str, sub_str );
    if ( !str_ptr )
        return ( input_str );

    strcpy( tmp_str, input_str );

    while (string_found)
    {
        str_ptr = strstr( tmp_str, sub_str );
        repl_index = str_ptr-tmp_str;
        strncpy( str_buffer, tmp_str, repl_index );
        strcat( str_buffer, replace_str );
        index2=repl_index+strlen(sub_str);

        strcat( str_buffer, &tmp_str[repl_index+strlen(sub_str)] );

        str_ptr = strstr( str_buffer, sub_str );
        if ( !str_ptr )
            string_found = FALSE;
        else
            strcpy( tmp_str, str_buffer );
    }
    return ( str_buffer );
}

/*****************************************************************
 * TAG( Begins_with )
 *
 * This function will check the beginnig of str for substr
 */
Bool_type
begins_with(char* str, char* substr)
{
    Bool_type result = False;
    if(strncmp(str,substr,strlen(substr)) == 0){
        result = True;
    }
    return result;
}

/*****************************************************************
 * TAG( ends_with )
 *
 * This function will check the beginnig of str for substr
 */
Bool_type
ends_with(char* str, char* substr)
{
    Bool_type result = False;
    if(strcmp(str + strlen(str) - strlen(substr),substr) == 0){
        result = True;
    }
    return result;
}

Bool_type
is_classname(char* input, Hash_table *class_table){
    Bool_type result = False;
    Htable_entry *p_hte;
    int rval;
    char token_upper[256];
    string_to_upper( input, token_upper );
    rval = htable_search( class_table, input, FIND_ENTRY, &p_hte );
    if ( rval!=OK ){
        rval = htable_search( class_table, token_upper, FIND_ENTRY, &p_hte );
    }
    if (rval != OK){
        result = False;
    }
    else{
        result = True;
    }
    return result;
}

void str_swap(const char * arr[], int * perm, int a, int b)
{
    if( perm != NULL )
    {
        int tmp = perm[b];
        perm[b] = perm[a];
        perm[a] = tmp;
    }

    const char * tmp_str = arr[b];
    arr[b] = arr[a];
    arr[a] = tmp_str;
}

void str_heapify(const char * arr[], int * perm, int mid, int ii)
{
    // Find largest among root, left child and right child
    int largest = ii;
    int left = 2 * ii + 1;
    int right = 2 * ii + 2;

    if (left < mid && strcmp(arr[left], arr[largest]) > 0 )
        largest = left;

    if (right < mid && strcmp(arr[right], arr[largest]) > 0 )
        largest = right;

    // Swap and continue heapifying if root is not largest
    if (largest != ii)
    {
        str_swap( arr, perm, ii, largest );
        str_heapify( arr, perm, mid, largest );
    }
}

void str_heapsort(const char * arr[], int * perm, int cnt)
{
    int ii = 0;
    // Build max heap
    for (ii = cnt / 2 - 1; ii >= 0; ii--)
        str_heapify(arr, perm, cnt, ii);

    // Heap sort
    for (ii = cnt - 1; ii >= 0; ii--)
    {
        str_swap(arr, perm, 0, ii);
        str_heapify(arr, perm, ii, 0);
    }
}
