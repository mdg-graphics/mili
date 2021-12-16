/* $Id: misc.c,v 1.30 2021/05/06 02:42:16 jdurren Exp $ */
/*
 * misc.c - Miscellaneous routines.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Oct 24 1991
 *
 * Copyright (c) 1991 Regents of the University of California
 */


#ifndef NO_GETRUSAGE
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/timeb.h>
#endif

#include "driver.h"

#define BUFSIZE 50
#define DFLT_BUF_COUNT  2

/*****************************************************************
 * TAG( fam_list )
 *
 * Dynamically allocated array of pointers to all currently open
 * MILI families.
 */
extern Mili_family **fam_list;


/*****************************************************************
 * TAG( qty_connects )
 *
 * Quantity of connectivity nodes per superclass.
 */
int qty_connects[M_QTY_SUPERCLASS] = {
   0, 1, 2, 3, 3, 4, 4, 5, 6, 8, 0, 0
};


//compares if the float f1 is equal with f2 and returns 1 if true and 0 if false
int is_equal(float f1, float f2)
 {
    float precision = 0.0000001;
    if (((f1 - precision) < f2) && 
        ((f1 + precision) > f2))
    {
        return 1;
    }
    else
    {
        return 0;
    }
 }

/*****************************************************************
 *
 * TAG(set_timesteps ) PUBLIC
 * For use in append functionality. It set the start and stop states. If nothing
 * to do then it sets startstate to -1.
 */
void set_timesteps(Mili_analysis *in_db, Mili_analysis *out_db ,int * start_state , int *stop_state)
{
   int j=0;
   double last_out_state;

   Mili_family *in_fam, *out_fam;
   Famid in_dbid, out_dbid;
   /* Get each Mili db */
   out_dbid = out_db->db_ident;
   out_fam = fam_list[out_dbid];
   in_dbid = in_db->db_ident;
   in_fam = fam_list[in_dbid];
   if(out_fam->state_qty == 0){
      *start_state=1;
      *stop_state= in_fam->state_qty;
      return; 
   }else if(env.wait){
      *start_state = out_fam->state_qty+1;
      return;
   }

   last_out_state = out_fam->state_map[out_fam->state_qty-1].time;

   /* Do some quick logic checking if it makes sense to even attempt the Mili_analysis
   */
   if( in_fam->state_map[0].time > last_out_state) {
      start_state[0]=0;
      stop_state[0] = in_fam->state_qty;
   } else if(in_fam->state_map[in_fam->state_qty-1].time <= last_out_state) {
      start_state[0]=-1;
      stop_state[0] = -1;
   } else {
      float time = out_fam->state_map[out_fam->state_qty-1].time;
      while((is_equal( in_fam->state_map[j].time,time ) ||
           in_fam->state_map[j].time < out_fam->state_map[out_fam->state_qty-1].time )
            &&
            j<in_fam->state_qty) {
         j++;
      }
      j++;
      if(j == in_fam->state_qty-1) {
         /*should have been caught in the exterior checks but doesn't hurt to make sure*/
         start_state[0]=-1;
         stop_state[0] = -1;
      } else if(j==0) {
         /*also should have been caught in the exterior checks but doesn't hurt to make sure*/
         start_state[0]=0;
         stop_state[0] = in_fam->state_qty;
      } else {
         start_state[0]=j;
         stop_state[0] = in_fam->state_qty;
      }
   }

}


/*****************************************************************
 * TAG( get_max_state )
 *
 * Obtain a maximum state number by querying the database and
 * applying any extant constraint.
 */
int
get_max_state( Mili_analysis *analy )
{
   int qty_states;
   int rval;

   rval = mc_query_family( analy->db_ident, QTY_STATES, NULL, NULL,
                           &qty_states );

   /*
    * Since this func returns the desired information and not status,
    * pop a diagnostic if there's an error.
    */
   if ( rval != OK ) {
      fprintf(stderr, "\n\t WARNING: get_max_state() - unable to query QTY_STATES!" );
      return 0;
   }

   return qty_states;
}
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
*************************************************************************************/
void manage_timer( int end_flag )
{
#ifndef NO_GETRUSAGE
   typedef struct timer_data {
      Bool_type active;
      struct timeb wall_begin;
      struct timeb wall_end;
      struct rusage r_begin;
      struct rusage r_end;
   } TimerData;

   static TimerData timer;
#endif

   time_t mt, st;


#ifdef NO_GETRUSAGE
   return;
#endif

#ifndef NO_GETRUSAGE
   if ( end_flag == 0 ) {
      if ( timer.active ) {
         fprintf(stderr, "\n\t WARNING: Resetting active timer." );
      } else {
         timer.active = TRUE;
      }

      ftime( &timer.wall_begin );
      getrusage( RUSAGE_SELF, &timer.r_begin );
   } else {
      struct timeb *p_tb, *p_te;
      struct rusage *p_rb, *p_re;
      TimerData *p_td;

      p_td = &timer;

      if ( !p_td->active ) {
         fprintf( stderr, "\n\t WARNING Cannot terminate inactive timer." );
         return;
      }

      p_tb = &p_td->wall_begin;
      p_te = &p_td->wall_end;
      p_rb = &p_td->r_begin;
      p_re = &p_td->r_end;

      getrusage( RUSAGE_SELF, p_re );
      ftime( p_te );

      mt = p_te->millitm - p_tb->millitm;
      if ( mt < 0 ) {
         mt += 1000;
         st = p_te->time - p_tb->time - 1;
      } else {
         st = p_te->time - p_tb->time;
      }

      p_td->active = FALSE;

#ifdef TIME_TRACKER
      char tracker_str[256],
           walltime_str[20],
           version[64],
           problem_str[128];
      sprintf(walltime_str," -W %d.%03d ", (int) st, (int) mt );
      strcpy( version, " -v " );
      strcat( version, PACKAGE_VERSION );
      strcat( version, " " );

      strcpy( problem_str, " -I ");
      strcat( problem_str, env.output_file_name );
      strcat( version, " " );

      /* Construct the tracker execute line */
      strcpy( tracker_str, "/usr/apps/tracker/bin/tracker -n xmilics " );
      strcat( tracker_str, version );
      strcat( tracker_str, walltime_str );
      strcat( tracker_str, problem_str );
      system( tracker_str );
#endif
   }
#endif
}

