/* $Id: mc_silo_file_mgr.c,v 1.7 2011/05/26 21:43:13 durrenberger1 Exp $ */

/*
 * Mesh I/O Library source code - Silo API
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

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  20-Jun-2008 IRC  Initial version.                           */
/*                                                              */
/****************************************************************/

/*  Include Files  */
#include "silo.h"
#include "SiloLib.h"

#include <sys/types.h>
#include <sys/types.h>


#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include "mili_internal.h"

#include "MiliSilo_Internal.h"
#include "SiloLib.h"

void set_path( char *in_path, char **out_path, int *out_path_len );

static Return_value
mc_silo_parse_control_string( char *ctl_str, int famid, Bool_type *p_create );

static int milisilo_init_flag=FALSE;

/*****************************************************************
 * TAG( fam_list )
 *
 * Dynamically allocated array of pointers to all currently open
 * MILI families.
 */
extern Mili_family **fam_list;

/*****************************************************************
 * TAG( mc_silo_open ) PUBLIC
 *
 * Open a Mesh I/O library file family in SILO format for access,
 * returning a family identifier to caller.
 */
int
mc_silo_open( char *root_name, char *path, char *control_string, Famid *p_famid )
{
   int root_len, path_len;
   char *lpath, *ctl_str;
   int famid;
   int run_mode=0;
   int status;

   int cycle=-1, processor=-1, seqnum=-1;

   /* Initialize if first open */
   if ( !milisilo_init_flag )
   {
      status = mc_silo_init( );
      milisilo_init_flag=TRUE;
   }

   /* Before we do anything, check environment for verbose setting. */
   verbosity( getenv( "MILI_VERBOSE" ) != NULL );

   root_len = (int) strlen( root_name );

   set_path( path, &lpath, &path_len );

   /* Build the family root string. */
   family[family_qty].root_len = path_len + root_len + 1;
   family[family_qty].root     = (char *) calloc( family[family_qty].root_len + 1, sizeof( char ) );

   sprintf( family[family_qty].root, "%s/%s", lpath, root_name );

   /* Save the separate components. */
   family[family_qty].path  = strdup( lpath );
   family[family_qty].root  = strdup( root_name );

   /* If control string is old format, map into new format. */
   if ( !match_old_control_string_format( control_string, &ctl_str ) )
      /* Didn't match as old format, so parse control string as is. */
      ctl_str = control_string;

   /* Parse the control string and initialize the header. */
   status = mc_silo_parse_control_string( ctl_str, famid, &run_mode );
   if ( status != OK )
   {
      mc_silo_free( family_qty );
      *p_famid = ID_FAIL;
      return status;
   }

   status = SiloLib_Open_Restart_File(famid, root_name, path,
                                      cycle, processor, seqnum,
                                      SILO_ROOTFILE, run_mode);

   status = SiloLib_Open_Restart_File(famid, root_name, path,
                                      cycle, processor, seqnum,
                                      SILO_PLOTFILE, run_mode);

   /* Initialize file indices. */
   family[family_qty].cur_st_index    = -1;

   famid = family_qty;

   if ( status != OK )
   {
      family_qty--;
      *p_famid = ID_FAIL;
      return status;
   }
   else
      *p_famid = famid;

   family[family_qty].fid = famid;

   return OK;
}


/*****************************************************************
 * TAG( mc_silo_close ) PUBLIC
 *
 * Close the indicated file family.
 */
int
mc_silo_close( Famid famid )
{
   Return_value status;
   char         fname[128];
   DBfile      *db;

   status = validate_fam_id( famid );
   if ( status != OK )
      return status;

   /* Close the Rootfile */
   status = SiloLib_Close_Restart_File( famid, SILO_ROOTFILE );

   /* Close the Plotfile */

   mc_silo_free( famid, SILO_PLOTFILE );
   fam_list[famid] = NULL;

   reset_class_data( NULL, 0, TRUE );

   family_qty--;
   return OK;
}

/*****************************************************************
 * TAG( mc_silo_flush ) PUBLIC
 *
 * No concept of flush with Silo files - just close and
 * do a return.
 */
int
mc_silo_flush( Famid famid, int data_type )
{
   int status=OK;

   status =  mc_silo_close( famid );

   return status;
}

/*****************************************************************
 * TAG( mc_silo_suffix_width ) PUBLIC
 *
 * Set the initial field width of the numeric suffix in state-data
 * file names.
 */
int
mc_silo_suffix_width( Famid famid, int suffix_width )
{
   int status;

   /* May not set suffix width after any state data file has been opened. */
   if ( family[famid].current_state_file_id > 0 )
      return TOO_LATE;

   /* Store new suffix width. to the ROOT file */
   family[famid].st_suffix_width = suffix_width;
   status = SiloLIB_cddir(ROOT, SILO_PLOTFILE, "/") ;
   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, NULL, "/Metadata") ;
   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, NULL, "/Family") ;

   status = SiloLib_Write_Var (ROOT, SILO_PLOTFILE, "st_suffix_width", 0., 1, 1, 1, DB_INT, &suffix_width);

   return OK;
}


/*****************************************************************
 * TAG( mc_silo_limit_filesize ) PUBLIC
 *
 * Set the quantity of states per family state-data file,
 * given a requested filesize.
 */
int
mc_silo_limit_filesize( Famid famid, int filesize )
{
   int status;

   if ( family[famid].bytes_per_file_limit == filesize )
      return OK;

   /* Store new suffix width. to the ROOT file */
   family[famid].bytes_per_file_limit = filesize;
   status = SiloLIB_cddir(ROOT, SILO_PLOTFILE, "/") ;
   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, "/Metadata") ;
   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, "/Family") ;

   status = SiloLib_Write_Var(ROOT, SILO_PLOTFILE, "filesize", 0., 1, 1, 1, DB_INT, &filesize);

   return (OK);
}


/*****************************************************************
 * TAG( mc_limit_states ) PUBLIC
 *
 * Set the quantity of states per family state-data file.
 */
int
mc_silo_limit_states( Famid famid, int states_per_file )
{
   int status;

   if ( family[famid].states_per_file == states_per_file )
      return OK;

   status = SiloLIB_cddir(ROOT, SILO_PLOTFILE, "/") ;
   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, "/Metadata") ;
   status = SiloLIB_mkcddir(ROOT, SILO_PLOTFILE, "/Family") ;

   status = SiloLib_Write_Var(ROOT, SILO_PLOTFILE, "states_per_file", 0., 1, 1, 1, DB_INT, &states_per_file);

   if ( status == OK )
      family[family_qty].states_per_file = states_per_file;

   return status;
}

/*****************************************************************
 * TAG( mc_silo_free ) PUBLIC
 *
 * This function will free up a slot in the file family
 * list.
 */
int
mc_silo_free( Famid famid )
{
   int status=OK;

   family[famid].qty_states = 0;

   return OK;
}


/*****************************************************************
 * TAG( mc_silo_parse_control_string ) PRIVATE
 *
 * Assign db control parameters based on db open argument string.
 *
 * Upon return, if accessing an existing db, its header will have
 * been read.  If the db is to be created, neither the header nor
 * the first file will have been initialized.
 */
static Return_value
mc_silo_parse_control_string( char *ctl_str, int famid, Bool_type *p_create )
{
   Bool_type func_requested[QTY_CONTROL_FUNCTIONS];
   int i;
   int func_specified_count;
   char *p_c;
   char endian_request;
   Precision_limit_type precision_request;
   Return_value rval;
   Control_function_type func_idx;
   Bool_type create_family;

   /* Init logical flags to indicate no functions have been assigned values. */
   for ( i = 0;
         i < QTY_CONTROL_FUNCTIONS;
         i++ )
      func_requested[i] = FALSE;

   /*
    * Parse the control string characters to extract application-specified
    * parameters.
    */
   rval = OK;
   func_specified_count = 0;
   create_family        = FALSE;
   endian_request       = '\0';
   precision_request    = PREC_LIMIT_NULL;

   for ( p_c = ctl_str;
         p_c != NULL && *p_c != '\0' && rval == OK;
         p_c++ )
   {
      switch ( *p_c )
      {
         case 'A':
            /* Parse access mode specification. */
            p_c++;

            /* Only process mode specification once. */
            if ( func_requested[CTL_ACCESS_MODE] )
            {
               i=0;
               break;
            }
            switch ( *p_c )
            {
               case 'w':
                  family[famid].access_mode = *p_c;
                  create_family = TRUE;
                  break;
               case 'r':
               case 'a':
                  family[famid].access_mode = *p_c;
                  rval = OK;

                  rval = mc_silo_read_header( &family[famid] );

                  if ( rval == FAMILY_NOT_FOUND && *p_c == 'a' )
                  {
                     /* Opened with "append", but must be created. */
                     create_family = TRUE;
                     rval = OK;
                  }
                  else if ( rval == OK
                            || ( rval == MAPPED_OLD_HEADER
                                 && *p_c == 'r' ) )
                  {
                     /* Permit read access on old format db's. */

                     /*
                      * read_header() sets fam->precision_limit, but
                      * make sure we don't increment the function
                      * count a second time if we already parsed
                      * a precision limit from the control string.
                      */
                     if ( !func_requested[CTL_PRECISION_LIMIT] )
                     {
                        func_specified_count++;
                        func_requested[CTL_PRECISION_LIMIT] = TRUE;
                     }

                     rval = OK;
                  }
                  break;
               default:
                  rval = BAD_CONTROL_STRING;
                  break;
            }
            if ( rval == OK )
            {
               func_requested[CTL_ACCESS_MODE] = TRUE;
               func_specified_count++;
            }
            break;

         case 'P':
            /* Parse precision limit specification. */
            p_c++;

            /* Only parse precision limit specification once. */
            if ( precision_request != '\0' )
               break;

            switch ( *p_c )
            {
                  /*
                   * We don't set fam->precision_limit yet because
                   * we don't want to overwrite a setting that might
                   * come via read_header() and we can't fully consider
                   * the precision limit until the access mode has
                   * been set.
                   */
               case 's':
                  precision_request = PREC_LIMIT_SINGLE;
                  break;
               case 'd':
                  precision_request = PREC_LIMIT_DOUBLE;
                  break;
               default:
                  rval = BAD_CONTROL_STRING;
                  break;
            }

            /*
             * Don't set the func_requested[] flag yet so as to force
             * precision_request to be evaluated in the defaults loop
             * below in cases where read_header() is not called.  If
             * read_header() is successfully called, meaning the db
             * exists, the precision limit for the db already exists
             * and it's correct to ignore any specification from the
             * control string.
             */

            break;

         case 'E':
            /* Parse unused specification. */
            p_c++;
            break;

         default:
            rval = BAD_CONTROL_STRING;
            break;
      }
   }

   if ( rval != OK )
   {
      *p_create = create_family;
      return rval;
   }

   /*
    * Fill-in unspecified functions with defaults and finish processing.
    *
    * We loop until all control functions have been accounted for,
    * permitting individual functions to "pass" until any prerequisite
    * functions have been addressed.
    */
   func_idx = CTL_ACCESS_MODE;
   while ( func_specified_count < QTY_CONTROL_FUNCTIONS
           && rval == OK )
   {
      switch ( func_idx )
      {
         case CTL_ENDIAN:
            func_specified_count++;
            break;

         case CTL_ACCESS_MODE:
            /* Abort if already specified. */
            if ( func_requested[func_idx] )
               break;

            /* Implement default treatment of db access mode. */

            /* Does the db exist?  Try to read the header. */
            rval = mc_silo_read_header( family[famid] );

            if ( rval == OK
                  || rval == MAPPED_OLD_HEADER )
            {
               /* Database already exists - default access is "read". */
               family[famid].access_mode = 'r';

               /* read_header() sets fam->precision_limit. */
               func_specified_count++;
               func_requested[CTL_PRECISION_LIMIT] = TRUE;

               rval = OK;
            }
            else if ( rval == FAMILY_NOT_FOUND )
            {
               /* No database - default access is "write". */
               family[famid].access_mode = 'w';
               create_family = TRUE;
               rval = OK;
            }

            /* Been here, done this. */
            func_requested[CTL_ACCESS_MODE] = TRUE;
            func_specified_count++;

            break;

         case CTL_PRECISION_LIMIT:
            /* Abort if already specified. */
            if ( func_requested[func_idx] )
               break;

            /* Force access mode to be handled first. */
            if ( !func_requested[CTL_ACCESS_MODE] )
               break;

            /*
             * To be here, we must be creating the db (any successful
             * call to read_header() would have set the func_requested[]
             * flag).  If a precision limit was specified in the control
             * string, use that, otherwise use the default.
             */
            if ( precision_request != PREC_LIMIT_NULL )
               family[famid].precision_limit = precision_request;
            else
               /*
                * Implement default treatment of precision limit, which
                * is to change the data as little as possible.
                */
               family[famid].precision_limit = PREC_LIMIT_DOUBLE;

            /* Been here, done this. */
            func_requested[CTL_PRECISION_LIMIT] = TRUE;
            func_specified_count++;

            break;
      }

      func_idx = (func_idx + 1) % QTY_CONTROL_FUNCTIONS;
   }

   *p_create = create_family;

   return rval;
}


/*****************************************************************
 * TAG( mc_silo_read_header ) PRIVATE
 *
 * Read the family header and initialize the fam struct.
 */
int
mc_silo_read_header( MiliSilo_family *fam )
{
   char *filename;
   size_t nitems;
   int i;
   int run_mode=0;
   int status;

   struct stat sbuf;
   extern int errno;

   int cycle=-1, processor=-1, seqnum=-1;

   DBfile *dbPlot;
   double var;

   filename = SiloLib_Create_Filename(fam->root, SILO_ROOTFILE,
                                      cycle, processor, seqnum);

   status = SiloLib_Open_Restart_File(fam->fid, filename, NULL,
                                      cycle, processor, seqnum,
                                      SILO_PLOTFILE, run_mode);

   if ( status != OK )
      return OPEN_FAILED;

   dbPlot = SiloLIB_getDBid(fam->fid, SILO_PLOTFILE);

   status = SiloLIB_cddir(dbPlot, "/Metadata") ;

   status = Silo_Read_Var (dbPlot, "precision_limit", &var);
   fam->precision_limit = var;

   status = Silo_Read_Var (dbPlot, "st_suffix_width", &var);
   fam->st_suffix_width = (int)var;

   mc_silo_close( fam->fid );

   return OK;
}


/*****************************************************************
 * TAG( mc_silo_init ) PRIVATE
 *
 * Initialize MiliSilo I/O.
 */
int
mc_silo_init( )
{
   int status=OK;

   family_qty=0;
   return OK;
}

/* End of mc_silo_file_mgr.c */
