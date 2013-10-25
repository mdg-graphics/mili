
/* $Id$ */

/*
 * Timer routines.
 *
 * These routines are structured so that there are only the following
 * entry points:
 *
 *	void	t_start()	start timer
 *	void	t_stop()	stop timer
 *	double	t_getrtime()	return real (elapsed) time (seconds)
 *	double	t_getutime()	return user time (seconds)
 *	double	t_getstime()	return system time (seconds)
 *
 * Purposely, there are no structures passed back and forth between
 * the caller and these routines, and there are no include files
 * required by the caller.
 *
 * This work was produced at the University of California, Lawrence
 * Livermore National Laboratory (UC LLNL) under contract no.
 * W-7405-ENG-48 (Contract 48) between the U.S. Department of Energy
 * (DOE) and The Regents of the University of California (University)
 * for the operation of UC LLNL. Copyright is reserved to the University
 * for purposes of controlled dissemination, commercialization through
 * formal licensing, or other disposition under terms of Contract 48;
 * DOE policies, regulations and orders; and U.S. statutes. The rights
 * of the Federal Government are reserved under Contract 48 subject to
 * the restrictions agreed upon by the DOE and University as allowed
 * under DOE Acquisition Letter 97-1.
 *
 *
 * DISCLAIMER
 *
 * This work was prepared as an account of work sponsored by an agency
 * of the United States Government. Neither the United States Government
 * nor the University of California nor any of their employees, makes
 * any warranty, express or implied, or assumes any liability or
 * responsibility for the accuracy, completeness, or usefulness of any
 * information, apparatus, product, or process disclosed, or represents
 * that its use would not infringe privately-owned rights.  Reference
 * herein to any specific commercial products, process, or service by
 * trade name, trademark, manufacturer or otherwise does not necessarily
 * constitute or imply its endorsement, recommendation, or favoring by
 * the United States Government or the University of California. The
 * views and opinions of authors expressed herein do not necessarily
 * state or reflect those of the United States Government or the
 * University of California, and shall not be used for advertising or
 * product endorsement purposes.
 *
 */

#include	<stdio.h>
#include	"systype.h"

#include	<sys/time.h>
#include	<sys/resource.h>

#ifdef SYS5
#include	<sys/types.h>
#include	<sys/times.h>
#include	<sys/param.h>	/* need the definition of HZ */
#define		TICKS	HZ	/* see times(2); usually 60 or 100 */
#endif

static	struct timeval		time_start, time_stop; /* for real time */
static	struct rusage		ru_start, ru_stop;     /* for user & sys time */

#ifdef SYS5
static	long			time_start, time_stop;
static	struct tms		tms_start, tms_stop;
long				times();
#endif

static	double			start, stop, seconds, msecs;

/*
 * Start the timer.
 * the start timer desn't return anything to the caller, it just stores some
 * information for the stop timer routine to access.
 */

void
t_start()
{

    if (gettimeofday(&time_start, (struct timezone *) 0) < 0)
        err_sys("t_start: gettimeofday() error");
    if (getrusage(RUSAGE_SELF, &ru_start) < 0)
        err_sys("t_start: getrusage() error");

}

/*
 * Stop the timer and save the appropriate information such as the stopping
 * time at the moment it was requested.
 */

void
t_stop()
{

    if (getrusage(RUSAGE_SELF, &ru_stop) < 0)
        err_sys("t_stop: getrusage() error");
    if (gettimeofday(&time_stop, (struct timezone *) 0) < 0)
        err_sys("t_stop: gettimeofday() error");
}

/*
 * Return the user time in seconds.
 */

double
t_getutime()
{

    start = ((double) ru_start.ru_utime.tv_sec) * 1000000.0
            + ru_start.ru_utime.tv_usec;
    stop = ((double) ru_stop.ru_utime.tv_sec) * 1000000.0
           + ru_stop.ru_utime.tv_usec;
    seconds = (stop - start) / 1000000.0;

    return(seconds);
}

/*
 * Return the system time in seconds.
 */

double
t_getstime()
{

    start = ((double) ru_start.ru_stime.tv_sec) * 1000000.0
            + ru_start.ru_stime.tv_usec;
    stop = ((double) ru_stop.ru_stime.tv_sec) * 1000000.0
           + ru_stop.ru_stime.tv_usec;

    seconds = (stop - start) / 1000000.0;

    return(seconds);
}

/*
 * Return the real (elapsed) time in seconds.
 */

double
t_getrtime()
{

    start = ((double) time_start.tv_sec) * 1000000.0 + time_start.tv_usec;
    stop = ((double) time_stop.tv_sec) * 1000000.0 + time_stop.tv_usec;

    seconds = (stop - start) / 1000000.0;

    return(seconds);
}

/*
 * Return the real (elapsed) time in micro-seconds.
 */

double
t_getmrtime()
{

    start = ((double) time_start.tv_sec) * 1000000.0 + time_start.tv_usec;
    stop = ((double) time_stop.tv_sec) * 1000000.0 + time_stop.tv_usec;

    /* seconds = (stop - start) / 1000000.0; */
    msecs = (stop - start);

    return(msecs);
}
