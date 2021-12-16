/*
 Copyright (c) 2016, Lawrence Livermore National Security, LLC. 
 Produced at the Lawrence Livermore National Laboratory. Written 
 by Kevin Durrenberger: durrenberger1@llnl.gov. CODE-OCEC-16-056. 
 All rights reserved.

 This file is part of Mili. For details, see <URL describing code 
 and how to download source>.

 Please also read this link-- Our Notice and GNU Lesser General 
 Public License.

 This program is free software; you can redistribute it and/or modify 
 it under the terms of the GNU General Public License (as published by 
 the Free Software Foundation) version 2.1 dated February 1999.

 This program is distributed in the hope that it will be useful, but 
 WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF 
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms 
 and conditions of the GNU General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License 
 along with this program; if not, write to the Free Software Foundation, 
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 
 * Mesh I/O Library source code.
 *
 ************************************************************************
 * Modifications:
 *  I. R. Corey - Augist 18, 2006: Added flag for determing if we are to
 *  branch to the Silo file version of Mili.
 *
 *  I. R. Corey - August 16, 2007: Added field for TI vars to note if
 *                nodal or element result.
 *
 *  I. R. Corey - March 20, 2009: Added support for WIN32 for use with
 *                with Visit.
 *                See SCR #587.
 *
 *  I. R. Corey - February 19, 2010: Changed default for max states per
 *                file - was too low for TH runs. Fixed problem with
 *                not initializing fam->bytes_per_file_current when
 *                state limit reached.
 *                SCR #663
 ************************************************************************
 */

#include <sys/types.h>

/*
 * Sysinfo may not be present on every platform
 */

#define SYSINFO_PRESENT 1


#if defined(_WIN32) || defined(WIN32)
#undef SYSINFO_PRESENT
#endif

#ifdef SYSINFO_PRESENT
#include <sys/utsname.h>
#endif

#if defined(_WIN32) || defined(WIN32)
#define LW_KERNEL
#define USERNAME "USERNAME"
#else
#include <pwd.h>
#define USERNAME "USER"
#endif
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include "mili_internal.h"
#include "mili_endian.h"
#include "gahl.h"

/*
#define LW_KERNEL
*/


#if defined(_MSC_VER)
// MS Visual C compiler additional typedefs/defines

typedef unsigned int mode_t;
typedef unsigned int pid_t;

#ifndef S_IRUSR
#define S_IRUSR 0400
#define S_IWUSR 0200
#endif
#endif


Bool_type filelock_enable = FALSE;

/* Various TI state flags */
extern Bool_type ti_enable;
extern Bool_type ti_only;       /* If true,then only TI files are read and written */
extern Bool_type ti_data_found;


#define COMMIT_NS( f ) ( f->access_mode != 'r' \
                         && ( f->directory[f->file_count - 1].commit_count == 0\
                              || f->non_state_ready ) )

#define COMMIT_TI( f ) ( f->access_mode != 'r' \
                         && ( f->ti_directory[f->ti_file_count - 1].commit_count == 0\
                              || f->non_state_ready ) )

static void set_path( char *in_path, char **out_path, int *out_path_len );
static void map_old_header( char header[CHAR_HEADER_SIZE] );
static Return_value create_family( Mili_family * );
static Return_value init_header( Mili_family * );
static Return_value create_write_lock( Mili_family * );
static Return_value delete_family( char *, char * );
static Return_value write_header( Mili_family *fam );
static Return_value load_descriptors( Mili_family *fam );
static Return_value commit_non_state( Mili_family *fam );
static Return_value ti_read_header( Mili_family *fam );
static Return_value set_defaults( char *root_name, char *path,
                                  Mili_family* fam, char *control_string,
                                  Bool_type *create_db, Famid *fam_id );
static Return_value test_open_next( Mili_family *fam, Bool_type *open_next );

static char *name_states_per_file = "states per file";
static char *name_file_size_limit = "max size per file";

/*****************************************************************
 * TAG( fam_list )
 *
 * Dynamically allocated array of pointers to all currently open
 * MILI families.
 */
Mili_family **fam_list;

/*****************************************************************
 * TAG( fam_qty )
 *
 * The actual number of all currently open MILI families.
 */
int fam_qty;

/*****************************************************************
 * TAG( fam_array_length )
 *
 * The length of fam_list.
 */
int fam_array_length;

/*****************************************************************
 * TAG( internal_sizes )
 *
 * Array of sizes of native datatypes.
 */
int internal_sizes[QTY_PD_ENTRY_TYPES + 1] =
{
   0,
   sizeof( char ),
   sizeof( float ),
   sizeof( float ),
   sizeof( double ),
   sizeof( int ),
   sizeof( int ),
   sizeof( LONGLONG )
};


/*****************************************************************
 * TAG( mili_verbose )
 *
 * Boolean variable which controls output of diagnostics.
 */
int mili_verbose = FALSE;

/*****************************************************************
 * TAG( header_version )
 *
 * The current version of the header format.
 */
static unsigned char header_version = 2;


/*****************************************************************
 * TAG( directory_version )
 *
 * The current version of the directory format.
 * Version 2 was created to allow for the storing of state map 
 * info in the Metadata file.
 */
static unsigned char directory_version = 3;

/*****************************************************************
 * TAG( milisilo )
 *
 * If true that each mili function will branch to the Silo
 * implementation.
 */
Bool_type milisilo = FALSE;


#ifdef MODE_TEST
/*****************************************************************
 * TAG( mode_test_data )
 *
 * Array to hold pertinent internal state variables for eval by
 * testing applications.
 */
int mode_test_data[3];
#endif


/*****************************************************************
 * TAG( mc_get_next_fid ) PRIVATE
 *
 * Returns the next available file id number.
 */
int mc_get_next_fid( void )
{
   return ( fam_qty );
}


/*****************************************************************
 * TAG( set_defaults ) LOCAL
 *
 * Open a Mesh I/O library file family for access, returning a
 * family identifier to caller.
 */
static Return_value
set_defaults( char *root_name, char *path, Mili_family* fam,
              char *control_string, Bool_type *create_db, Famid *fam_id )
{
   int root_len, path_len, pos;
   char *lpath, *ctl_str;
   Return_value rval;
   
   root_len = (int) strlen( root_name );

   set_path( path, &lpath, &path_len );

   /*  Need to let the world know that this is a Mili data base */
   fam->db_type = MILI_DB_TYPE;

   /* Build the family root string. */
   fam->root_len = path_len + root_len + 1;
   fam->root = (char *) calloc( fam->root_len + 1, sizeof( char ) );
   if (fam->root_len + 1 > 0 && fam->root == NULL)
   {
      return ALLOC_FAILED;
   }
   sprintf( fam->root, "%s/%s", lpath, root_name );

   /* Save the separate components. */
   str_dup( &fam->path, lpath );
   str_dup( &fam->file_root, root_name );
   if (fam->path == NULL || fam->file_root == NULL)
   {
      return ALLOC_FAILED;
   }
   //The plus 3 is for the path separator, 'A' and null terminator
   fam->aFile = malloc(strlen(fam->root)+strlen(fam->path)+3);
   if(fam->aFile == NULL)
   {
       return ALLOC_FAILED;
   }
   
   fam->aFile[0] ='\0';
   strcat(fam->aFile,fam->root);
   strcat(fam->aFile,"A");
   /* If control string is old format, map into new format. */
   if ( !match_old_control_string_format( control_string, &ctl_str ) )
   {
      /* Didn't match as old format, so parse control string as is. */
      ctl_str = control_string;
   }


   /* Initialize file indices. */
   fam->char_header = NULL;
   fam->cur_index       = -1;
   fam->cur_st_index    = -1;
   fam->cur_file        = NULL;
   fam->directory       = NULL;

   /* Initialize TI variables */
   fam->ti_cur_index          = -1;
   fam->ti_cur_st_index       = -1;
   fam->ti_meshid             = 0;
   fam->ti_matid              = 0;
   fam->ti_state              = 0;
   fam->ti_superclass_name[0] = '\0';
   fam->ti_short_name[0]      = '\0';
   fam->ti_long_name[0]       = '\0';
   fam->ti_meshvar            = FALSE;
   fam->ti_nodal              = FALSE;
   fam->num_label_classes     = 0;
   fam->ti_search_by_metadata = FALSE;
   fam->ti_meshid             = 0;
   fam->ti_saved_matid              = 0;
   fam->ti_saved_state              = 0;
   fam->ti_saved_superclass_name[0] = '\0';
   fam->ti_saved_short_name[0]      = '\0';
   fam->ti_saved_long_name[0]       = '\0';
   fam->ti_saved_meshvar            = FALSE;
   fam->ti_saved_nodal              = FALSE;
   fam->num_label_classes     = 0;
   fam->state_closed = 0;
   fam->hide_states = FALSE;
   fam->state_dirty = 0;
   fam->visit_file_on = 0;
   /* State data file pointer. */
   fam->cur_st_file = NULL;

   /* Parse the control string and initialize the header. */
   rval = parse_control_string( ctl_str, fam, create_db );
   if ( rval != OK )
   {
      return rval;
   }
   
   pos= fam_qty;
   if(fam_qty <fam_array_length)
   {
      /* search through to see if the a NULL position*/
      for(pos=0;pos <fam_array_length;pos++)
      {
         if(fam_list[pos] == NULL)
         {
           break;
         }
      }
   }
   if(pos < fam_array_length){
      fam_list[pos] = fam;
      *fam_id = pos;
          
   } else 
   {
      /* Need this stuff if open_family() is called. */
      fam_list = RENEW_N( Mili_family *, fam_list, pos, 1,
                         "Big Kahuna pointers" );
      if (fam_list == NULL)
      {
         return ALLOC_FAILED;
      }
      fam_list[fam_qty] = fam;
      *fam_id = fam_qty;     
      fam_array_length++; 
   }


   fam_qty++;
   fam->my_id = *fam_id;

   fam->lock_file_descriptor = 0;
   
   fam->ti_enable      = ti_enable;
   fam->ti_only        = ti_only;       /* If true,then only TI files are read and written */
   fam->ti_data_found  = ti_data_found;

   return OK;
}

void delete_fam(Mili_family *fam)
{
   cleanse( fam );
   free( fam );
}


/*****************************************************************
 * TAG( mc_quick_open ) PUBLIC
 *
 * Open a Mesh I/O library file family for quick access, 
 * returning a family identifier to caller.
 */
Return_value
mc_quick_open(char *root_name, char *path,
              char *control_string, Famid *p_fam_id)
{
#if TIMER
   clock_t start,stop;
   double cumalative=0.0;
   start = clock();
#endif
   Mili_family *fam;
   Return_value rval;
   Bool_type create_db;
   int fam_id=0;

#ifdef SILOENABLED
   if (milisilo)
   {
      mc_silo_open( root_name, path, control_string, p_fam_id );
      return;
   }
#endif

   fam = NEW( Mili_family, "Mili family" );
   if (fam == NULL)
   {
      return ALLOC_FAILED;
   }

   /* Before we do anything, check environment for verbose setting. */
   verbosity( getenv( "MILI_VERBOSE" ) != NULL );

   rval = set_defaults(root_name, path, fam, control_string,
                       &create_db, &fam_id);
   
   if(create_db || rval != OK) {
      cleanse( fam );
      free( fam );
      return FAMILY_NOT_FOUND;
   }
   if(fam->char_header[DIR_VERSION_IDX] ==1)
   {   
      fam->hide_states = 1;
   }
   
   rval = open_family( fam_id );
   
#if TIMER
   stop = clock();
   cumalative =((double)(stop - start))/CLOCKS_PER_SEC;
   printf("Function mc_quick_open() Time is: %f\n",cumalative);
#endif
   /* Finish up. */
   if ( rval != OK )
   {
      cleanse( fam );
      free( fam_list[fam_qty] );
      fam_qty--;
      fam_array_length--;
      *p_fam_id = ID_FAIL;
      return rval;
   }
   else
   {
#if defined(_MSC_VER)
      fam->pid = (long) 0;
#else
      fam->pid = (long) getpid();
#endif
      *p_fam_id = fam_id;

      return OK;
   }
}


/*****************************************************************
 * TAG( mc_open ) PUBLIC
 *
 * Open a Mesh I/O library file family for access, returning a
 * family identifier to caller.
 */
Return_value
mc_open( char *root_name, char *path, char *control_string, Famid *p_fam_id )
{
#if TIMER
   clock_t start,stop;
   double cumalative=0.0;
   start = clock();
#endif
   Mili_family *fam;
   Return_value rval;
   Bool_type create_db;
   int fam_id;

#ifdef SILOENABLED
   if (milisilo)
   {
      mc_silo_open( root_name, path, control_string, p_fam_id );
      return;
   }
#endif

   fam = NEW( Mili_family, "Mili family" );
   if (fam == NULL)
   {
      return ALLOC_FAILED;
   }
   /* Before we do anything, check environment for verbose setting. */
   verbosity( getenv( "MILI_VERBOSE" ) != NULL );

   rval = set_defaults(root_name, path, fam, control_string,
                       &create_db, &fam_id);
   if (rval != OK)
   {
      return rval;
   }
   else if ( rval != OK && !create_db)
   {
      cleanse( fam );
      free( fam );
      *p_fam_id = ID_FAIL;
      return OK;
   }

#ifdef MODE_TEST
   /* Record pertinent data for test app. */
   mode_test_data[0] = fam->swap_bytes;
   mode_test_data[1] = fam->precision_limit;
   mode_test_data[2] = (int) fam->access_mode;
#endif


   /* Open or create the family. */
   if ( create_db )
   {
      rval = create_family( fam );
   }
   else
   {
      rval = open_family( fam_id );
   }
#if TIMER
   stop = clock();
   cumalative =((double)(stop - start))/CLOCKS_PER_SEC;
   printf("Function mc_open() Time is: %f\n",cumalative);
#endif
   /* Finish up. */
   if ( rval != OK )
   {
      cleanse( fam );
      fam_qty--;
      fam_array_length--;
      free( fam_list[fam_qty] );           
      *p_fam_id = ID_FAIL;
      return rval;
   }
   else
   {
#if defined(_MSC_VER)
      fam->pid = (long) 0;
#else
      fam->pid = (long) getpid();
#endif
      *p_fam_id = fam_id;
      
      return OK;
      
   }
}


/*****************************************************************
 * TAG( set_path ) LOCAL
 *
 */
static void
set_path( char *in_path, char **out_path, int *out_path_len )
{
   static char *this_dir = ".";

   if ( in_path != NULL && *in_path != '\0' )
   {
      *out_path_len = (int) strlen( in_path );
      *out_path = in_path;
   }
   else
   {
      *out_path = this_dir;
      *out_path_len = 1;
   }
}


/*****************************************************************
 * TAG( match_old_control_string_format ) PRIVATE
 *
 * Detect possible old format values of an mc_open() control
 * string and pass back a pointer to an equivalent new format
 * string.
 * Changed behavior to always be double precision 
 */
Bool_type
match_old_control_string_format( char *control_string, char **ctl_str )
{
   static char *r_trans = "Ar";
   static char *a_trans = "Aa";
   static char *as_trans = "AaPd";
   static char *ad_trans = "AaPd";
   static char *w_trans = "AwPd";
   static char *ws_trans = "AwPd";
   static char *wd_trans = "AwPd";

   if ( control_string == NULL || *control_string == '\0' )
   {
      return FALSE;
   }

   if ( strcmp( control_string, "r" ) == 0 )
   {
      *ctl_str = r_trans;
   }
   else if ( strcmp( control_string, "rs" ) == 0 )
   {
      *ctl_str = r_trans;
   }
   else if ( strcmp( control_string, "rd" ) == 0 )
   {
      *ctl_str = r_trans;
   }
   else if ( strcmp( control_string, "a" ) == 0 )
   {
      *ctl_str = a_trans;
   }
   else if ( strcmp( control_string, "as" ) == 0 )
   {
      *ctl_str = as_trans;
   }
   else if ( strcmp( control_string, "ad" ) == 0 )
   {
      *ctl_str = ad_trans;
   }
   else if ( strcmp( control_string, "w" ) == 0 )
   {
      *ctl_str = w_trans;
   }
   else if ( strcmp( control_string, "ws" ) == 0 )
   {
      *ctl_str = ws_trans;
   }
   else if ( strcmp( control_string, "wd" ) == 0 )
   {
      *ctl_str = wd_trans;
   }
   else
   {
      return FALSE;
   }

   return TRUE;
}


/*****************************************************************
 * TAG( parse_control_string ) PRIVATE
 *
 * Assign db control parameters based on db open argument string.
 *
 * Upon return, if accessing an existing db, its header will have
 * been read.  If the db is to be created, neither the header nor
 * the first file will have been initialized.
 */
Return_value
parse_control_string( char *ctl_str, Mili_family *fam, Bool_type *p_create )
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
   for ( i = 0; i < QTY_CONTROL_FUNCTIONS; i++ )
   {
      func_requested[i] = FALSE;
   }

   /*
    * Parse the control string characters to extract application-specified
    * parameters.
    */
   rval = OK;
   func_specified_count = 0;
   create_family = FALSE;
   endian_request = '\0';
   precision_request = PREC_LIMIT_NULL;

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
                  fam->access_mode = *p_c;
                  create_family = TRUE;
                  break;
               case 'r':
               case 'a':
                  fam->access_mode = *p_c;
                  rval = OK;

                  if ( !fam->ti_only )
                  {
                     rval = read_header( fam );
                  }
                  else
                  {
                     rval = ti_read_header( fam );
                  }

                  if ( rval == FAMILY_NOT_FOUND && *p_c == 'a' )
                  {
                     /* Opened with "append", but must be created. */
                     create_family = TRUE;
                     rval = OK;
                  }
                  else if ( rval == OK ||
                            ( rval == MAPPED_OLD_HEADER &&
                              *p_c == 'r' ) )
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
//            if ( precision_request != '\0' )
//            {
//               break;
//            }

//            switch ( *p_c )
//            {
                  /*
                   * We don't set fam->precision_limit yet because
                   * we don't want to overwrite a setting that might
                   * come via read_header() and we can't fully consider
                   * the precision limit until the access mode has
                   * been set.
                   */
//               case 's':
//                  precision_request = PREC_LIMIT_SINGLE;
//                  break;
//               case 'd':
//                  precision_request = PREC_LIMIT_DOUBLE;
//                  break;
//               default:
//                  rval = BAD_CONTROL_STRING;
//                  break;
//            }

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
            /* Parse endianness specification. */
            p_c++;

            /* Only parse endianness specification once. */
            if ( endian_request != '\0' )
            {
               break;
            }

            switch ( *p_c )
            {
               case 'b':
               case 'l':
               case 'n':
                  endian_request = *p_c;
                  break;
               default:
                  rval = BAD_CONTROL_STRING;
                  break;
            }

            /*
             * We don't set func_requested[CTL_ENDIAN] yet because
             * this parameter must be evaluated in the context of
             * the access mode, and we can't guarantee that's set
             * until the logic block following this loop.
             */
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
   while ( (func_specified_count < QTY_CONTROL_FUNCTIONS) && (rval == OK) )
   {
      switch ( func_idx )
      {
         case CTL_ACCESS_MODE:
            /* Abort if already specified. */
            if ( func_requested[func_idx] )
            {
               break;
            }

            /* Implement default treatment of db access mode. */

            /* Does the db exist?  Try to read the header. */
            rval = read_header( fam );

            if ( rval == OK || rval == MAPPED_OLD_HEADER )
            {
               /* Database already exists - default access is "read". */
               fam->access_mode = 'r';

               /* read_header() sets fam->precision_limit. */
               func_specified_count++;
               func_requested[CTL_PRECISION_LIMIT] = TRUE;

               rval = OK;
            }
            else if ( rval == FAMILY_NOT_FOUND )
            {
               /* No database - default access is "write". */
               fam->access_mode = 'w';
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
            {
               break;
            }

            /* Force access mode to be handled first. */
            if ( !func_requested[CTL_ACCESS_MODE] )
            {
               break;
            }

            /*
             * To be here, we must be creating the db (any successful
             * call to read_header() would have set the func_requested[]
             * flag).  If a precision limit was specified in the control
             * string, use that, otherwise use the default.
             */
            if ( precision_request != PREC_LIMIT_NULL )
            {
               fam->precision_limit = precision_request;
            }
            else
            {
               /*
                * Implement default treatment of precision limit, which
                * is to change the data as little as possible.
                */
               fam->precision_limit = PREC_LIMIT_DOUBLE;
            }

            /* Been here, done this. */
            func_requested[CTL_PRECISION_LIMIT] = TRUE;
            func_specified_count++;

            break;

         case CTL_ENDIAN:
            /* Abort if already specified. */
            if ( func_requested[func_idx] )
            {
               break;
            }

            /*
             * Can't manage endianness until we know whether we're
             * reading or writing the db.  Also, since we need to be
             * ready to initialize the header if the db is being
             * created, we'll pass here until the precision limit is
             * set as well.
             */
            if ( !func_requested[CTL_ACCESS_MODE] ||
                  !func_requested[CTL_PRECISION_LIMIT] )
            {
               break;
            }

            if ( endian_request != '\0' )
            {
               /* The app specified endianness. */

               /* Set endianness for read access cases. */
               if ( fam->access_mode == 'r' )
               {
                  if ( (BIG_ENDIAN_HOST &&
                        fam->char_header[ENDIAN_IDX] == MILI_BIG_ENDIAN)
                        ||
                        (LITTLE_ENDIAN_HOST &&
                        fam->char_header[ENDIAN_IDX] == MILI_LITTLE_ENDIAN) )
                  {
                     fam->swap_bytes = FALSE;
                  }
                  else
                  {
                     fam->swap_bytes = TRUE;
                  }
               }
               else if ( fam->access_mode == 'w' )
               {
                  if ( endian_request == 'n' ||
                        (endian_request == 'b' && BIG_ENDIAN_HOST) ||
                        (endian_request == 'l' && LITTLE_ENDIAN_HOST) )
                  {
                     fam->swap_bytes = FALSE;
                  }
                  else
                  {
                     fam->swap_bytes = TRUE;
                  }
               }
               else if ( fam->access_mode == 'a' && !create_family )
               {
                  /*
                   * For append mode and extant db, we ignore any
                   * request and enforce moving data correctly between
                   * the host and the db, i.e., we don't support requests
                   * for internal data in other than the host endian
                   * format.
                   */
                  if ( (BIG_ENDIAN_HOST &&
                        fam->char_header[ENDIAN_IDX] == MILI_BIG_ENDIAN)
                        ||
                        (LITTLE_ENDIAN_HOST &&
                        fam->char_header[ENDIAN_IDX] == MILI_LITTLE_ENDIAN) )
                  {
                     fam->swap_bytes = FALSE;
                  }
                  else
                  {
                     fam->swap_bytes = TRUE;
                  }
               }
               else if ( fam->access_mode == 'a' && create_family )
               {
                  /*
                   * For append mode with no extant db, we will pay
                   * attention to the requested endianness.
                   */
                  if ( endian_request == 'n' ||
                        (endian_request == 'b' && BIG_ENDIAN_HOST) ||
                        (endian_request == 'l' && LITTLE_ENDIAN_HOST) )
                  {
                     fam->swap_bytes = FALSE;
                  }
                  else
                  {
                     fam->swap_bytes = TRUE;
                  }
               }
            }
            else
            {
               /* Implement default treatment of endianness. */
               switch ( fam->access_mode )
               {
                  case 'r':
                  case 'a':
                     /*
                      * For read access, we need internal data in native
                      * endian format.  For append access, the same is
                      * true, and we also need external data in the
                      * extant endian format.
                      */
                     if ( create_family )
                     {
                        /* Don't swap if creating the family. */
                        fam->swap_bytes = FALSE;
                     }
                     else
                     {
                        if ( (BIG_ENDIAN_HOST &&
                              fam->char_header[ENDIAN_IDX] == MILI_BIG_ENDIAN) ||
                              (LITTLE_ENDIAN_HOST &&
                              fam->char_header[ENDIAN_IDX] == MILI_LITTLE_ENDIAN) )
                        {
                           fam->swap_bytes = FALSE;
                        }
                        else
                        {
                           fam->swap_bytes = TRUE;
                        }
                     }
                     break;

                  case 'w':
                     /* For write access, default is native format. */
                     fam->swap_bytes = FALSE;
                     break;
               }
            }

            /* Been here, done this. */
            func_requested[CTL_ENDIAN] = TRUE;
            func_specified_count++;

            break;
         default:
            break;
      }

      if (func_idx == CTL_ACCESS_MODE)
      {
         func_idx = CTL_PRECISION_LIMIT;
      }
      else if (func_idx == CTL_PRECISION_LIMIT)
      {
         func_idx = CTL_ENDIAN;
      }
      else if (func_idx == CTL_ENDIAN)
      {
         func_idx = CTL_ACCESS_MODE;
      }
   }

   *p_create = create_family;

   return rval;
}


/*****************************************************************
 * TAG( open_family ) PRIVATE
 *
 * Open an extant Mili family.
 */
Return_value
open_family( Famid fam_id )
{
#if TIMER
   clock_t start,stop;
   double cumalative=0.0;
   start = clock();
#endif
   Return_value status, lkstat;
   Mili_family *fam;
   char p_fname[M_MAX_NAME_LEN];
   char fname[M_MAX_NAME_LEN];
   FILE *p_f;
   int fnum;
   Return_value rval;
   int write_ct;
   int limit;  // limit is used in calculating the length of the state 
               // file name string

   fam = fam_list[fam_id];

   fam->my_id = fam_id;
   
   fam->state_closed = 1;

   /**/
   /* Need to initialize all of fam */
   /*
   * Adding a check for a really old mili file format 
   */
   make_fnam( TI_DATA, fam, 0, fname );
   if((p_f = fopen( fname, "rb" )) == NULL && fam->char_header[DIR_VERSION_IDX]==1)
   {
      fam->ti_enable = FALSE;
   }
   /* Set appropriate I/O routines for family. */
   status = set_default_io_routines( fam );
   if ( status != OK )
   {
      return status;
   }

   /* Check family for current write activity by any process. */
   test_write_lock( fam );
   if ( fam->access_mode == 'a' )
   {
      if ( fam->active_family )
      {
         /* Only one process can have write access on a family. */
         return LOCK_EXISTS;
      }
      else
      {
         status = create_write_lock( fam );
         if ( status != OK )
         {
            return status;
         }
      }

      /*
       * Append access not permitted if dir version of family is newer than
       * dir version of library.
       */
      if ( fam->char_header[DIR_VERSION_IDX] > directory_version )
      {
         return DIRECTORY_WRITE_CONFLICT;
      }

      /*
       * Append access not permitted if header version of family is newer
       * than header version of library.
       */
      if ( fam->char_header[HDR_VERSION_IDX] > header_version )
      {
         return HEADER_WRITE_CONFLICT;
      }
   }

   /* Traverse non-state data files and load directories. */
   if (!fam->ti_only)
   {
      status = load_directories( fam );
      if ( status != OK || fam->file_count == 0 )
      {
         return status;
      }
   }
#if TIMER
   clock_t start2,stop2;
   double cumalative2=0.0;
   start2 = clock();
#endif
   /* Load TI directories if they exist */
   if ( fam->char_header[DIR_VERSION_IDX] == 1 && fam->ti_enable )
   {
      rval = load_ti_directories( fam );
      if ( fam->ti_file_count == 0 || rval != OK)
      {
         fam->ti_enable = FALSE; /* TI data not found */
      }
   }
#if TIMER
   stop2 = clock();
   cumalative2 =((double)(stop2 - start2))/CLOCKS_PER_SEC;
   printf("Function load_ti_directories()\nTime is: %f\n",cumalative2);
#endif

   /*
    * For append access, need to update write lock file with initial
    * db contents.  Do non-state data now, state data after state map is built.
    */
   if ( fam->access_mode == 'a' && fam->file_count > 0 )
   {
      /*
       * Lock the length bytes to be written to notify any reader
       * that an update is in progress.
       */
      lkstat = get_name_lock( fam, NON_STATE_DATA );
      if ( lkstat != OK )
      {
         /* Shouldn't ever get here - notify user. */
         if ( mili_verbose )
         {
            fprintf( stderr, "Mili - Unable to place name lock for "
                     "non-state init.\n" );
         }
         return lkstat;
      }
      else
      {
         memset( p_fname, (int) '\0', M_MAX_NAME_LEN );
         make_fnam( NON_STATE_DATA, fam, fam->file_count - 1, p_fname );

         /* Ignore if file locking disabled */
         if ( filelock_enable )
         {
            write_ct = write( fam->lock_file_descriptor, (void *) p_fname,
                              M_MAX_NAME_LEN );
            if (write_ct != M_MAX_NAME_LEN)
            {
               return SHORT_WRITE;
            }
         }

         status = free_name_lock( fam, NON_STATE_DATA );
         if (status != OK)
         {
            return status;
         }
      }
   }

#if TIMER
   start2 = clock();
#endif
   /* Traverse non-state data files and load descriptors. */

   if (!fam->ti_only)
   {
      status = load_descriptors( fam );
      if ( status != OK )
      {
         return status;
      }
   }
#if TIMER
   stop2 = clock();
   cumalative2 =((double)(stop2 - start2))/CLOCKS_PER_SEC;
   printf("Function load_descriptors()\nTime is: %f\n",cumalative2);
#endif
   status = mc_read_scalar( fam_id, name_file_size_limit,
                            &fam->bytes_per_file_limit);
   if(status != OK){
      fam->bytes_per_file_limit=0;
   }
   
   status = OK;
   status = mc_read_scalar( fam_id, "post_modified",
                            &fam->post_modified);
   if(status != OK){
      fam->post_modified=0;
   }
   status = OK;
   /* With directory (and param table) loaded, get fam->states_per_file. */
   if(!fam->hide_states)
   {
      status = mc_read_scalar( fam_id, name_states_per_file,
                               &fam->states_per_file );
      if (status != OK)
      {
         return status;
      }
   }

   /* Traverse state-data files and build (initial) state map. */
   if(!fam->hide_states)
   {
      status = build_state_map( fam, TRUE );
      if (status != OK)
      {
         return status;
      }
   }
   
   status = mc_read_scalar( fam_id, "nproc", &fam->num_procs );
   if (status != OK)
   {
      fam->num_procs = find_proc_count(fam_id);
      if(fam->num_procs <1)
      {
        return status;
      }else
      {
         status = OK;
      }
   }
   
   if ( fam->access_mode == 'a' && fam->st_file_count > 0 )
   {
      /*
       * Lock the length bytes to be written to notify any reader
       * that an update is in progress.
       */
      lkstat = get_name_lock( fam, STATE_DATA );
      if ( lkstat != OK )
      {
         /* Shouldn't ever get here - notify user. */
         if ( mili_verbose )
         {
            fprintf( stderr,
                     "Mili - Unable to place name lock for state "
                     "data init.\n" );
         }
         return lkstat;
      }
      else
      {
         memset( p_fname, (int) '\0', M_MAX_NAME_LEN );
         fnum = ST_FILE_SUFFIX( fam, fam->st_file_count - 1 );
         
         limit = M_MAX_NAME_LEN - 1 - strlen(fam->root);
         if (fnum >0 && log10(fnum) > limit)
         {
            return INVALID_FILE_STATE_INDEX;
         }
         make_fnam( STATE_DATA, fam, fnum, p_fname );

         /* Ignore if file locking disabled */
         if ( filelock_enable )
         {
            write_ct = write( fam->lock_file_descriptor, (void *) p_fname,
                              M_MAX_NAME_LEN );
            if (write_ct != M_MAX_NAME_LEN)
            {
               return SHORT_WRITE;
            }
         }

         status = free_name_lock( fam, STATE_DATA );
         if (status != OK)
         {
            return status;
         }
      }
   }
#if TIMER
   stop = clock();
   cumalative =((double)(stop - start))/CLOCKS_PER_SEC;
   printf("Function open_family()\nOpening time is: %f\n",cumalative);
#endif
   return status;
}


/*****************************************************************
 * TAG( load_descriptors ) LOCAL
 *
 * Read non-state files and load available descriptive information
 * for state variables, meshes, and state record formats.
 *
 * Should only be called at start-up, but after load_directories().
 */
static Return_value
load_descriptors( Mili_family *fam )
{
#if TIMER
   clock_t start,stop,start1,stop1;
   double cumalative=0.0;
   double cumalative1=0.0;
   start = clock();
#endif
   Return_value stat;

#if TIMER
   start1 = clock();
#endif
   stat = load_svars( fam );
#if TIMER
   stop1 = clock();
   cumalative1 =((double)(stop1 - start1))/CLOCKS_PER_SEC;
   printf("Function load_svars() Opening time is: %f\n",cumalative1);
#endif
   if ( stat != OK )
   {
      return stat;
   }
#if TIMER
   start1 = clock();
#endif
   stat = init_meshes( fam );
#if TIMER
   stop1 = clock();
   cumalative1 =((double)(stop1 - start1))/CLOCKS_PER_SEC;
   printf("Function init_meshes() Opening time is: %f\n",cumalative1);
#endif
   if ( stat != OK )
   {
      return stat;
   }
#if TIMER
   start1 = clock();
#endif
   stat = load_srec_formats( fam );
#if TIMER
   stop1 = clock();
   cumalative1 =((double)(stop1 - start1))/CLOCKS_PER_SEC;
   printf("Function load_srec_formats() Opening time is: %f\n",cumalative1);
   stop = clock();
   cumalative =((double)(stop - start))/CLOCKS_PER_SEC;
   printf("Function load_descriptors()\nOpening time is: %f\n",cumalative);
#endif
   return stat;
}


/*****************************************************************
 * TAG( read_header ) PRIVATE
 *
 * Read the family header and initialize the fam struct.
 */
Return_value
read_header( Mili_family *fam )
{
   char fname[M_MAX_NAME_LEN];
   char header[CHAR_HEADER_SIZE];
   FILE *p_f;
   LONGLONG nitems;
   int i;
   int status;
   struct stat sbuf;
   Return_value rval;

   if ( fam->char_header != NULL )
   {
      return MULTIPLE_HEADER_READ;
   }

   if ( fam->ti_only )
   {
      make_fnam( TI_DATA, fam, 0, fname );
   }
   else
   {
      make_fnam( NON_STATE_DATA, fam, 0, fname );
   }

   /*
    * Treat syscall "stat" failure with ENOENT as sufficient and necessary
    * condition for determining that the file does not exist (which opens
    * the door for application of write access in default case).
    */
   status = stat( fname, &sbuf );
   if ( status == -1 )
   {
      if ( errno == ENOENT )
      {
         return FAMILY_NOT_FOUND;
      }
      else
      {
         return OPEN_FAILED;
      }
   }

   /* Found the file; open it. */
   p_f = fopen( fname, "rb" );
   if ( p_f == NULL )
   {
      return OPEN_FAILED;
   }

   /* Read in the header to verify compatibility and init family. */
   nitems = fread( (void *) header, 1, CHAR_HEADER_SIZE, p_f );

   fclose( p_f );

   if ( nitems != CHAR_HEADER_SIZE )
   {
      return SHORT_READ;
   }

   /* Check for old style header - bytes 5-9 will be ASCII upper case. */
   rval = OK;
   for ( i = 4; i < 9; i++ )
   {
      if ( header[i] < 65 )
      {
         break;
      }
   }
   if ( i == 9 )
   {
      /* Map old header into new format. */
      map_old_header( header );

      /* OK to continue, but set status to show re-mapped header. */
      rval = MAPPED_OLD_HEADER;
   }

   if ( header[HDR_VERSION_IDX] > header_version )
   {
      if ( mili_verbose )
      {
         fprintf( stderr, "Mili warning - family header version newer than "
                  "library header version.\n" );
      }
   }

   if ( header[DIR_VERSION_IDX] > directory_version )
   {
      if ( mili_verbose )
      {
         fprintf( stderr, "Mili warning - family directory version newer "
                  "than library directory version.\n" );
      }
   }

   fam->precision_limit = (Precision_limit_type) header[PRECISION_LIMIT_IDX];
   fam->st_suffix_width = (int) header[ST_FILE_SUFFIX_WIDTH_IDX];
   fam->partition_scheme = (File_partition_scheme) header[PARTITION_SCHEME_IDX];

   /* Allocate space for the character header data. */
   fam->char_header = NEW_N( char, CHAR_HEADER_SIZE, "Family char header" );

   /* Copy the header. */
   memcpy( fam->char_header, header, CHAR_HEADER_SIZE );

   return rval;
}


/*****************************************************************
 * TAG( map_old_header ) LOCAL
 *
 * Map an old-format byte header into the new format.
 */
#define PRINTABLE_OFFSET ((int) 'A')
static void
map_old_header( char header[CHAR_HEADER_SIZE] )
{
   Precision_limit_type pl_type;
   Endianness_type endian_type;
   int suffix_width, i;
   File_partition_scheme fp_scheme;
   enum
   {
      OLD_MILI_MAGIC_NUMBER_IDX = 0,
      OLD_OS_TYPE_IDX = 4,
      OLD_FAMILY_PRECISION_IDX,
      OLD_DIR_VERSION_IDX,
      OLD_ST_FILE_SUFFIX_WIDTH_IDX,
      OLD_PARTITION_SCHEME_IDX,
      OLD_ADDL_FIELDS_IDX = CHAR_HEADER_SIZE - 1
   };
   enum
   {
      OLD_BEGIN_OS = 64,

      OLD_AIX_OS,
      OLD_HPUX_OS,
      OLD_IRIX_OS,
      OLD_IRIX64_OS,
      OLD_OSF1_OS,
      OLD_SUNOS_OS,
      OLD_UNICOS_OS,
      OLD_UNICOS_T3D_OS,
      OLD_VMS_OS,

      OLD_END_OS
   };
   enum
   {
      OLD_MILI_SINGLE = 65,
      OLD_MILI_DOUBLE,
      OLD_NATIVE
   };

   /* Infer endianness from OS type. */
   if ( header[OLD_OS_TYPE_IDX] == OLD_OSF1_OS )   /* Don't bother with */
   {
      endian_type = MILI_LITTLE_ENDIAN;           /*  OLD_VMS_OS       */
   }
   else
   {
      endian_type = MILI_BIG_ENDIAN;
   }

   /* Define precision limit from old family precision. */
   if ( header[OLD_FAMILY_PRECISION_IDX] == OLD_MILI_SINGLE )
   {
      pl_type = PREC_LIMIT_SINGLE;
   }
   else
   {
      pl_type = PREC_LIMIT_DOUBLE;
   }

   /* Compute suffix width from "printable" value stored in old header. */
   suffix_width = header[OLD_ST_FILE_SUFFIX_WIDTH_IDX] - PRINTABLE_OFFSET;

   /*
    * Compute File_partition_scheme from "printable" definition used in
    * old header.
    */
   fp_scheme = (File_partition_scheme) ((int)header[OLD_PARTITION_SCHEME_IDX] - 64);

   /*
    * Now modify the header.
    */

   /* Bytes 1-4 stay the same. */

   /* Overwrite the active entries. */
   header[HDR_VERSION_IDX] = 1;
   header[DIR_VERSION_IDX] = 1;
   header[ENDIAN_IDX] = endian_type;
   header[PRECISION_LIMIT_IDX] = pl_type;
   header[ST_FILE_SUFFIX_WIDTH_IDX] = suffix_width;
   header[PARTITION_SCHEME_IDX] = fp_scheme;

   /* Zero out the rest. */
   for ( i = PARTITION_SCHEME_IDX + 1; i < CHAR_HEADER_SIZE; i++ )
   {
      header[i] = '\0';
   }
}


/*****************************************************************
 * TAG( test_write_lock ) PRIVATE
 *
 * Check to see if another process has write access on a family.
 */
void
test_write_lock( Mili_family *fam )
{
   char fname[M_MAX_NAME_LEN];
   int fd;
   int stat = 0;

   /* Exit if file locking disabled */
   if ( !filelock_enable )
   {
      return;
   }

   make_fnam( WRITE_LOCK, fam, 0, fname );

   /* First see if file exists. */
   fd = open( fname, O_RDWR );
   if ( fd == -1 )
   {
      fam->active_family = FALSE;
      return;
   }

   /* File exists.  Check for lock to verify writing process exists. */
#if defined(_WIN32) || defined(WIN32)
   stat = 0;
#else
   stat = lockf( fd, F_TEST, 1L );
#endif
   if ( stat == -1 )
   {
      /* Another process has a lock in place - family is active. */
      fam->active_family = TRUE;

      fam->lock_file_descriptor = fd;
   }
   else
   {
      /*
       * Locking file exists but no process has established write
       * access. Presumably the creating process terminated
       * prematurely.  Act as though file did not exist.
       */
      fam->active_family = FALSE;

      close( fd );
   }
   return;
}


/*****************************************************************
 * TAG( create_family ) LOCAL
 *
 * Create a new db family by opening the first file and
 * initializing the table of contents and metadata dictionary.
 */
static Return_value
create_family( Mili_family *fam )
{
   Return_value rval;

   /* Establish write-lock on family. */
   if ( (rval = create_write_lock( fam )) != OK )
   {
      return rval;
   }

   /* Remove extant family of same name. */
   if ( !fam->ti_only )
   {
#ifndef _WIN32
      rval = delete_family( fam->file_root, fam->path );
#endif
   }
   else
   {
      rval = OK;
   }

   if ( rval != OK && rval != FAMILY_NOT_FOUND )
   {
      return rval;
   }

   fam->st_suffix_width  = DEFAULT_SUFFIX_WIDTH;
   fam->partition_scheme = DEFAULT_PARTITION_SCHEME;
   fam->states_per_file  = 0;
   fam->bytes_per_file_limit   = 0;
   fam->post_modified = 0;

   fam->st_file_count = 0;
   fam->file_count = 0;
   fam->cur_index = -1;
   fam->commit_max = -1;

   fam->ti_cur_index     = -1;
   fam->ti_file_count    = 0;

   if ( (rval = set_default_io_routines( fam )) != OK )
   {
      return rval;
   }

   /* Create first file and its directory. */
   rval = non_state_file_open( fam, fam->file_count, fam->access_mode );
   if (rval != OK)
   {
      return rval;
   }
   fam->param_table = htable_create( DEFAULT_HASH_TABLE_SIZE );

   if ( fam->param_table != NULL )
   {
      fam->file_count++;

      rval = init_header( fam );
      if (rval != OK)
      {
         return rval;
      }
      rval = mc_init_metadata( fam->my_id );
   }
   else
   {
      rval = ALLOC_FAILED;
   }
   
   return rval;
}


/*****************************************************************
 * TAG( create_write_lock ) LOCAL
 *
 * Create the family write-lock indicator file and put a UNIX
 * lock on it.
 *
 * The lock file contains three separately locked fields:
 *
 * OFFSET   FIELD LENGTH   USE
 * -------------------------------------------------------------
 *      0              1   When locked, indicates a process has
 *                           write access on database.
 *      1             255  Contains name of last complete non-
 *                           state data file written to database.
 *                           May be NULL-filled if no complete
 *                           non-state files exist.
 *                           Will be locked during update by
 *                           writing process.
 *    256            256   Contains name of last complete state
 *                           data file written to database.
 *                           May be NULL-filled if no complete
 *                           state files exist.
 *                           Will be locked during update by
 *                           writing process.
 */
static Return_value
create_write_lock( Mili_family *fam )
{
   int fd, rval = 0;
   char fname[M_MAX_NAME_LEN];
   char *null_buffer;
   mode_t mode;
   Return_value status = OK;
   int write_ct;

   /* Exit if file locking disabled */
   if ( !filelock_enable )
   {
      return OK;
   }

   /* Build lock file name. */
   make_fnam( WRITE_LOCK, fam, 0, fname );

   /* Set mode to 644 to allow advisory locks. */
#if defined(_WIN32) || defined(WIN32)
   mode = S_IRUSR | S_IWUSR;
#else
   mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
#endif

   /* Open the lock file. */
   fd = open( fname, O_CREAT | O_RDWR, mode );
   if ( fd == -1 )
   {
      return LOCK_OPEN_FAILED;
   }

   /* Establish the lock over the first byte. */
#if defined(_WIN32) || defined(WIN32)
   rval = 0;
#else
   rval = lockf( fd, F_TLOCK, 1 );
#endif
   if ( rval == -1 )
   {
      return LOCK_EXISTS;
   }

   /* Write (overwrite) file to be empty of meaningful data. */
   null_buffer = NEW_N( char, LOCK_FILE_SIZE, "Empty lock file buffer" );
   if (null_buffer == NULL)
   {
      return ALLOC_FAILED;
   }

   write_ct = write( fd, (void *) null_buffer, LOCK_FILE_SIZE );
   if (write_ct != LOCK_FILE_SIZE)
   {
      status = SHORT_WRITE;
   }

   fam->lock_file_descriptor = fd;

   free( null_buffer );
   return status;
}


/*****************************************************************
 * TAG( mc_delete_family ) PUBLIC
 *
 * Delete all files in a Mili family.
 */
Return_value
mc_delete_family( char *root, char *path )
{
   char *lpath;
   char fname[M_MAX_NAME_LEN];
   int fd;
   int path_len;
   Return_value rval;
   int stat;
   int i;
   pid_t pid;

#if defined(_MSC_VER)
   pid = 0;
#else
   pid = getpid();
#endif

   set_path( path, &lpath, &path_len );

   /* If this process has the db open, close it. */
   for ( i = 0; i < fam_array_length; i++ )
   {
      if ( fam_list[i] != NULL )
      {
         if ( strcmp( fam_list[i]->path, lpath ) == 0 &&
               strcmp( fam_list[i]->file_root, root ) == 0 &&
               pid == (pid_t) fam_list[i]->pid )
         {
            /* Specified family is open by this process -> close it. */
            rval = mc_close( i );
            if ( rval != OK )
            {
               return rval;
            }

            break;
         }
      }
   }

   /* Now does any process have write access on the specified db? */
   /**/
   /* This logic duplicates parts of make_fnam() and test_write_lock()
    * and may break if changes dictate revision of the logic in either
    * of those functions but similar changes are not made here.
    * Those should be re-written so as not to require an open family,
    * then they could be called here and their logic re-used.
    */
   /* First see if a write lock file exists. */
   sprintf( fname, "%s/%swlock", lpath, root );
   fd = open( fname, O_RDWR );

   if ( fd != -1 )
   {
      /* File exists.  Check for lock to verify writing process exists. */

      stat = 0;
#if defined(_WIN32) || defined(WIN32)
      stat = 0;
#else
      stat = lockf( fd, F_TEST, 1L );
#endif
      if ( stat == -1 )
      {
         /* Another process has a write lock in place - don't delete. */
         close( fd );
         return BAD_ACCESS_TYPE;
      }
   }
   /* We've done all we can do...blow it away. */
   rval = delete_family( root, lpath );

   return rval;
}


/*****************************************************************
 * TAG( delete_family ) LOCAL
 *
 * Delete extant family member files.
 *
 * Note - Even though the logic here permits identification of the
 * condition where no files were found to delete, the return status
 * for such cases is OK, not FAMILY_NOT_FOUND.  When/if Mili
 * return values are made public in mili.h, permitting an application
 * to discriminate between "acceptable" and non-acceptable (true)
 * failures, these OK return values could be changed to
 * FAMILY_NOT_FOUND.
 */
static Return_value
delete_family( char *root, char *path )
{
   int qty, rootlen;
   char fname[M_MAX_NAME_LEN];
   int i;
   StringArray sarr;
   Return_value rval;
   char *p_fname;
   char *tmp_fname;

   SANEW( sarr );
   if (sarr == NULL)
   {
      return ALLOC_FAILED;
   }

   rval = mili_scandir( path, root, &sarr );
   if ( rval != OK )
   {
      free( sarr );
      return rval;
   }
   else
   {
      qty = SAQTY( sarr );
   }

   if ( qty == 0 )
   {
      free( sarr );
      return OK;
   }

   /*
    * Delete all directory files which consist of the root and an entirely
    * numeric suffix or the root and an entirely upper-case letter suffix.
    * These are all files which _could_ be created by Mili, but mixed
    * alphanumeric or lower case suffixes could not be created by Mili
    * so leave them alone.
    */
   rootlen = strlen( root );
   for ( i = 0; i < qty; i++ )
   {
      p_fname = SASTRING( sarr, i );

      if ( is_numeric( p_fname + rootlen ) ||
            is_all_upper( p_fname + rootlen ) )
      {
         sprintf( fname, "%s/%s", path, p_fname );
         if ( unlink( fname ) != 0 )
         {
            if( i > 0 )
            {
               tmp_fname = SASTRING( sarr, i-1 );
               if ( strcmp( p_fname, tmp_fname ) != 0 )
               {
                  free( sarr );
                  return MEMBER_DELETE_FAILED;
               }
            }
         }
      }
   }

   free( sarr );

   return OK;
}


/*****************************************************************
 * TAG( init_header ) LOCAL
 *
 * Initialize the file family header.
 */
static Return_value
init_header( Mili_family *fam )
{
   int write_ct;

   /* Allocate space for the character header data. */
   fam->char_header = NEW_N( char, CHAR_HEADER_SIZE, "Family char header" );
   if (fam->char_header == NULL)
   {
      return ALLOC_FAILED;
   }

   /* Fill character header fields. */
   strncpy( &fam->char_header[MILI_MAGIC_NUMBER_IDX], "mili", 4 );
   fam->char_header[HDR_VERSION_IDX] = header_version;
   fam->char_header[DIR_VERSION_IDX] = directory_version;
   if ( fam->swap_bytes )
   {
      if ( BIG_ENDIAN_HOST )
      {
         fam->char_header[ENDIAN_IDX] = (char) MILI_LITTLE_ENDIAN;
      }
      else
      {
         fam->char_header[ENDIAN_IDX] = (char) MILI_BIG_ENDIAN;
      }
   }
   else
   {
      if ( BIG_ENDIAN_HOST )
      {
         fam->char_header[ENDIAN_IDX] = (char) MILI_BIG_ENDIAN;
      }
      else
      {
         fam->char_header[ENDIAN_IDX] = (char) MILI_LITTLE_ENDIAN;
      }
   }
   fam->char_header[PRECISION_LIMIT_IDX] = (char) fam->precision_limit;

   fam->char_header[ST_FILE_SUFFIX_WIDTH_IDX] = (char) fam->st_suffix_width;

   fam->char_header[PARTITION_SCHEME_IDX] = (char) fam->partition_scheme;

   /* Write it out (even if incomplete) to allocate space in the file. */
   write_ct = fam->write_funcs[M_STRING]( fam->cur_file, fam->char_header,
                                          CHAR_HEADER_SIZE );
   if (write_ct != CHAR_HEADER_SIZE)
   {
      return SHORT_WRITE;
   }

   /* Init the next free location in the current file. */
   fam->next_free_byte = CHAR_HEADER_SIZE;

   return OK;
}


/*****************************************************************
 * TAG( mc_init_metadata ) PUBLIC
 *
 * Write currently known metadata entries to family.
 */
Return_value
mc_init_metadata( Famid fam_id )
{
   Return_value rval;
   time_t t;
   char char_t[25];
   char nambuf[64];
   char mili_version[M_MAX_NAME_LEN];

   Mili_family *fam;
   int alloc_size;

#ifdef SYSINFO_PRESENT
   char* archbuf;
   long sys_stat;
   struct utsname sysdata;
#endif

   char* user_info;

   fam = fam_list[fam_id];

   /* Library version. */

   get_mili_version( mili_version );

   rval = write_string( fam, "lib version", (char *) mili_version,
                        MILI_PARAM );
   
   if (rval != OK)
   {
      return rval;
   }
   alloc_size =0;
   mc_wrt_scalar(fam_id,M_INT,"state_count",(void*)&alloc_size);
   if (rval != OK)
   {
      return rval;
   }
   
   mc_wrt_scalar(fam_id,M_INT,"post_modified",(void*)&(fam->post_modified));
   if (rval != OK)
   {
      return rval;
   }
   /* Host name and OS type. */
#if defined(_WIN32) || defined(WIN32)
   sprintf( nambuf, "windows_machine");
#else
   if (gethostname( nambuf, 64 ) != 0)
   {
      return BAD_HOST_NAME;
   }
#endif
   rval = write_string( fam, "host name", nambuf, MILI_PARAM );
   if (rval != OK)
   {
      return rval;
   }

#ifdef SYSINFO_PRESENT
   sys_stat = uname( &sysdata );
   if (sys_stat != 0)
   {
      return BAD_SYSTEM_INFO;
   }
   alloc_size = strlen(sysdata.sysname) + 3 + strlen(sysdata.release) +
                strlen(sysdata.nodename) + 1;
   archbuf = NEW_N(char, alloc_size, "Architecture string buffer.");
   if (alloc_size > 0 && archbuf == NULL)
   {
      return ALLOC_FAILED;
   }
   strncpy(archbuf,sysdata.sysname, strlen(sysdata.sysname)+1);
   strncat(archbuf, " + ", 3);
   strncat(archbuf, sysdata.release, strlen(sysdata.release));
   strncat(archbuf, sysdata.nodename, strlen(sysdata.nodename));
   rval = write_string( fam, "arch name", archbuf,  MILI_PARAM );
   free(archbuf);
   if (rval != OK)
   {
      return rval;
   }
#endif

   /* File creation date and time. */
   t = time( NULL );
   memcpy( char_t, ctime( &t ), sizeof( char_t ) );
   char_t[24] = '\0';
   rval = write_string( fam, "date", char_t, MILI_PARAM );
   if (rval != OK)
   {
      return rval;
   }

   /* Default (initial) quantity of states-per-file for partitioning. */
   rval = write_scalar( fam, M_INT, name_states_per_file,
                        &fam->states_per_file, MILI_PARAM );
   if (rval != OK)
   {
      return rval;
   }

   /* User info */
   /* Does not exist on Lightweight kernel - Red Storm */
   user_info = getenv(USERNAME);
   if (user_info != NULL)
   {
      rval = write_string( fam, "username", user_info, MILI_PARAM );
   }else{
      rval = write_string( fam, "username", "undefined", MILI_PARAM );
   }
   return rval;
}


/*****************************************************************
 * TAG( mc_get_metadata ) PUBLIC
 *
 * Read currently known metadata entries from a Mili file.
 */
Return_value
mc_get_metadata( Famid fam_id, char *mili_version, char *host,
                 char *arch,   char *timestamp, char *xmilics_version )
{
   mili_version[0]    = '\0';
   host[0]            = '\0';
   arch[0]            = '\0';
   timestamp[0]       = '\0';
   xmilics_version[0] = '\0';

   /* Return value not checked as these are all optional */
   mc_read_string( fam_id, "lib version", mili_version );
   mc_read_string( fam_id, "host name", host );
   mc_read_string( fam_id, "arch name", arch );
   mc_read_string( fam_id, "date", timestamp );
   mc_read_string( fam_id, "xmilics version", xmilics_version );

   return OK;
}


/*****************************************************************
 * TAG( mc_close ) PUBLIC
 *
 * Close the indicated file family.
 */
Return_value
mc_close( Famid fam_id )
{
   Mili_family *fam;
   Return_value rval;
   char fname[M_MAX_NAME_LEN];
   int state_qty=0;
   State_descriptor *p_sd;
#ifdef SILOENABLED
   if (milisilo)
   {
      mc_silo_close( fam_id );
      return;
   }
#endif

   rval = validate_fam_id( fam_id );
   if ( rval != OK )
   {
      return rval;
   }

   fam = fam_list[fam_id];
   /*
    * Reset the non-state file count to 1 for taurus databases.
    * There may be more than one physical file for all the non-state
    * data, but there is only one 'logical' non-state file for taurus.
    */
   if( fam->db_type == TAURUS_DB_TYPE )
   {
      fam->file_count = 1;
   }

   json_value_free(fam->root_value);
   
   if ( fam->cur_file != NULL )
   {
      
      rval = mc_flush( fam_id, NON_STATE_DATA );
      if (rval != OK && rval != BAD_ACCESS_TYPE)
      {
         return rval;
      }
      rval = non_state_file_close( fam );
      if (rval != OK)
      {
         return rval;
      }
   }
   /* Added to flush out the last write of the state map to the A file */
   if(fam->char_header[DIR_VERSION_IDX]>1 && fam->state_dirty){
      state_qty = fam->state_qty;
      if(!(fam->state_closed) && state_qty >0){
         p_sd = fam->state_map + (state_qty-1);
         update_static_map(fam_id,p_sd);
      }
   }
   
   if ( fam->ti_cur_file != NULL )
   {
      rval = mc_flush( fam_id, TI_DATA );
      if (rval != OK && rval != BAD_ACCESS_TYPE)
      {
         return rval;
      }
      rval = ti_file_close( fam_id );
      if (rval != OK)
      {
         return rval;
      }
   }

   if ( fam->cur_st_file != NULL )
   {
      rval = mc_flush( fam_id, STATE_DATA );
      if (rval != OK && rval != BAD_ACCESS_TYPE)
      {
         return rval;
      }
      rval = state_file_close( fam );
      if (rval != OK)
      {
         return rval;
      }
   }
   /* If access allowed writing, remove the write lock indicator. */
   if ( filelock_enable &&
         (fam->access_mode == 'w' || fam->access_mode == 'a' ) )
   {
#if defined(_WIN32) || defined(WIN32)
#else
      lockf( fam->lock_file_descriptor, F_ULOCK, 0 );
#endif
      make_fnam( WRITE_LOCK, fam, 0, fname );
      unlink( fname );
   }
   cleanse( fam );
   free( fam );
   fam_list[fam_id] = NULL;

   rval = reset_class_data( NULL, 0, TRUE );

   /* Added September 30, 2006: IRC */
   fam_qty--;
   return rval;
}


/*****************************************************************
 * TAG( mc_suffix_width ) PUBLIC
 *
 * Set the initial field width of the numeric suffix in state-data
 * file names.
 */
Return_value
mc_suffix_width( Famid fam_id, int suffix_width )
{
   Mili_family *fam;
   Return_value rval;

#ifdef SILOENABLED
   if (milisilo)
   {
      rval = mc_silo_suffix_width( fam_id, suffix_width ) ;
      return OK ;
   }
#endif

   /* Suffix width must be small enough numerically to fit into one byte. */
   if ( suffix_width < 1 || suffix_width > 255 )
   {
      return INVALID_SUFFIX_WIDTH;
   }

   fam = fam_list[fam_id];

   /* May not set suffix width after any state data file has been opened. */
   if ( fam->st_file_count > 0 || fam->cur_st_file != NULL )
   {
      return TOO_LATE;
   }

   /* Store new suffix width. */
   fam->st_suffix_width = suffix_width;
   fam->char_header[ST_FILE_SUFFIX_WIDTH_IDX] = (char) suffix_width;

   /* Re-write the character header to disk. */
   rval = write_header( fam );
   return rval;
}


/*****************************************************************
 * TAG( mc_limit_states ) PUBLIC
 *
 * Set the quantity of states per family state-data file.
 */
Return_value
mc_limit_states( Famid fam_id, int states_per_file )
{
   Mili_family *fam;
   Return_value rval;

#ifdef SILOENABLED
   if (milisilo)
   {
      /* rval = mc_silo_limit_states( fam_id, states_per_file ) ; */
      return rval;
   }
#endif

   fam = fam_list[fam_id];

   if ( fam->partition_scheme == STATE_COUNT )
   {
      if ( fam->states_per_file == states_per_file )
      {
         return OK;
      }

      rval = mc_wrt_scalar( fam_id, M_INT, name_states_per_file,
                            (void *) &states_per_file );

      if ( rval == OK )
      {
         fam->states_per_file = states_per_file;
      }

      return rval;
   }
   else
   {
      return NOT_APPLICABLE;
   }
}


/*****************************************************************
 * TAG( mc_limit_filesize ) PUBLIC
 *
 * Set the quantity of states per family state-data file,
 * given a requested filesize.
 */
Return_value
mc_limit_filesize( Famid fam_id, LONGLONG filesize )
{
   Mili_family *fam;
   Return_value rval = OK;
   if( filesize > ABSOLUTE_MAX_FILE_SIZE)
   {
      filesize = ABSOLUTE_MAX_FILE_SIZE;
      printf("Adjusted file size to Mili internal maximum of %d.\n",
             ABSOLUTE_MAX_FILE_SIZE);
   }

#ifdef SILOENABLED
   if (milisilo)
   {
      rval = mc_silo_limit_filesize( fam_id, filesize ) ;
      return rval;
   }
#endif

   fam = fam_list[fam_id];

   fam->bytes_per_file_limit = filesize;
   rval = mc_wrt_scalar( fam_id, M_INT,name_file_size_limit,
                            (void *) &filesize );

   return rval;
}


/*****************************************************************
 * TAG( mc_flush ) PUBLIC
 *
 * Write in-memory data to the current non-state file and close
 * the file,  or flush the current state file.
 */
Return_value
mc_flush( Famid fam_id, int data_type )
{
   Mili_family *fam;
   Return_value rval;

#ifdef SILOENABLED
   if (milisilo)
   {
      mc_silo_flush( fam_id, data_type );
      return;
   }
#endif

   fam = fam_list[fam_id];

   rval = OK;

   CHECK_WRITE_ACCESS( fam )

   if ( data_type == STATE_DATA )
   {
      if ( fam->cur_st_file != NULL )
      {
#ifndef AIX
         if (fflush( fam->cur_st_file ) != 0)
         {
            return UNABLE_TO_FLUSH_FILE;
         }
#else
         /* UNTESTED!!! (currently don't do buffered state i/o) */
         char mode[8], fname[M_MAX_NAME_LEN];
         fpos64_t offset;
         FILE *p_f;

         fgetpos64( fam->cur_st_file, &offset );

         fclose( fam->cur_st_file );

         set_file_access( 'a', TRUE, mode );
         make_fnam( STATE_DATA, fam,
                    ST_FILE_SUFFIX( fam, fam->cur_st_index ), fname );

         p_f = fopen( fname, mode );

         fsetpos64( p_f, &offset );

         fam->cur_st_file = (long) p_f;
#endif
      }
   }
   else if ( data_type == NON_STATE_DATA && COMMIT_NS( fam ) )
   {

      rval = commit_non_state( fam );
      if ( rval != OK )
      {
         return rval;
      }
      rval = non_state_file_close( fam );
      if ( rval != OK )
      {
         return rval;
      }

      /* Flush the TI Data if present 
      if ( fam->ti_enable )
      {
         if (COMMIT_TI( fam ) )
         {
            rval = ti_commit( fam_id );
            if ( rval != OK )
            {
               return rval;
            }
            rval = ti_file_close( fam_id );
         }
      }
      */
   }

   return rval;
}


/*****************************************************************
 * TAG( commit_non_state ) LOCAL
 *
 *
 */
static Return_value
commit_non_state( Mili_family *fam )
{
   char *p_fname;
   int fd;
   Return_value rval;
   int write_ct;

   rval = prep_for_new_data( fam, NON_STATE_DATA );
   if ( rval != OK )
   {
      return rval;
   }

   /* Ensure correct file pointer.  Could be off if access is "a". */
   if (fseek( fam->cur_file, 0L, SEEK_END ) != 0)
   {
      return SEEK_FAILED;
   }

   if ( fam->svar_table != NULL )
   {
      rval = commit_svars( fam );
      if ( rval != OK )
      {
         return rval;
      }
   }

   if ( fam->qty_srecs != 0 )
   {
      rval = commit_srecs( fam );
      if ( rval != OK )
      {
         return rval;
      }
   }

   rval = commit_dir( fam );
   if ( rval != OK )
   {
      return rval;
   }

   /* fflush( fam->cur_file ); */

   /*
    * Update contents of the write lock file to indicate last
    * complete non-state file.
    */
   fd = fam->lock_file_descriptor;

   /* Prepare the text. */
   p_fname = NEW_N( char, M_MAX_NAME_LEN, "Lock file NS name buffer" );
   if (p_fname == NULL)
   {
      return ALLOC_FAILED;
   }
   make_fnam( NON_STATE_DATA, fam, fam->file_count - 1, p_fname );

   /*
    * Lock the length bytes to be written to notify any reader
    * that an update is in progress.
    */
   rval = get_name_lock( fam, NON_STATE_DATA );
   if ( rval != OK )
   {
      /* Shouldn't ever get here - notify user. */
      if ( mili_verbose )
      {
         fprintf( stderr, "Mili - Flush failed to place name lock.\n" );
      }
   }

   /*
    * Write the length, then the name (get_name_lock() seeks
    * correct file position).
    */
   if ( filelock_enable )
   {
      write_ct = write( fd, (void *) p_fname, M_MAX_NAME_LEN );
      if (write_ct != M_MAX_NAME_LEN)
      {
         return SHORT_WRITE;
      }
   }

   /* Remove the advisory lock. */
   if ( rval == OK )
   {
      rval = free_name_lock( fam, NON_STATE_DATA );
   }

   fam->commit_count++;
   fam->non_state_ready = FALSE;

   free( p_fname );

   return rval;
}


/*****************************************************************
 * TAG( get_name_lock ) PRIVATE
 *
 *
 */
Return_value
get_name_lock( Mili_family *fam, int ftype )
{
   int fd;
   int tries;
   int offset;
   long len;

   /* Exit if file locking disabled */
   if ( !filelock_enable )
   {
      return OK;
   }

   if ( ftype == STATE_DATA )
   {
      offset = 256;
      len    = 256;
   }
   else
   {
      offset = 1;
      len    = 255;
   }

   tries = 0;
   fd = fam->lock_file_descriptor;
   if (lseek( fd, offset, SEEK_SET ) == -1)
   {
      return SEEK_FAILED;
   }

   /* Busy loop to get lock */

#if defined(_WIN32) || defined(WIN32)
#else
   while ( tries < MAX_LOCK_TRIES )
   {
      if ( lockf( fd, F_TLOCK, len ) != -1 )
      {
         break;
      }
      tries++;
   }
#endif
   if ( tries == MAX_LOCK_TRIES )
   {
      return LOCK_NAME_FAILED;
   }
   else
   {
      return OK;
   }
}


/*****************************************************************
 * TAG( free_name_lock ) PRIVATE
 *
 *
 */
Return_value
free_name_lock( Mili_family *fam, int ftype )
{
   int fd;
   int offset;
   long len;

   /* Exit if file locking disabled */
   if ( !filelock_enable )
   {
      return OK;
   }

   if ( ftype == STATE_DATA )
   {
      offset = 256;
      len    = 256;
   }
   else
   {
      offset = 1;
      len = 255;
   }
   fd = fam->lock_file_descriptor;

   if (lseek( fd, offset, SEEK_SET ) == -1)
   {
      return SEEK_FAILED;
   }

#if defined(_WIN32) || defined(WIN32)
#else
   if (lockf( fd, F_ULOCK, len ) == -1)
   {
      return LOCK_NAME_FAILED;
   }
#endif
   return OK;
}


/*****************************************************************
 * TAG( update_active_family ) PRIVATE
 *
 * Update internal structures for read-access of a database
 * for which another process currently has write or append access.
 */
Return_value
update_active_family( Mili_family *fam )
{
   struct stat statbuf;
   Return_value rval;

   /* Update state record map. */
   rval = build_state_map( fam, FALSE );
   if (rval != OK)
   {
      return rval;
   }

   /* Update non-state data here. */

   /*** Not implemented for now (ever?). ***/


   /*** UNTESTED ***/
   /* If lock file has been unlinked, database is no longer active. */
   if (fstat( fam->lock_file_descriptor, &statbuf ) == 0)
   {
      if ( statbuf.st_nlink == (short) 0 )
      {
         if (close( fam->lock_file_descriptor ) == 0)
         {
            fam->active_family = FALSE;
         }
         else
         {
            rval = UNABLE_TO_CLOSE_FILE;
         }
      }
   }
   else
   {
      rval = UNABLE_TO_STAT_FILE;
   }

   return rval;
}


/*****************************************************************
 * TAG( validate_fam_id ) PRIVATE
 *
 * Verify that a family identifier is valid by traversing the list
 * of family structs.
 */
Return_value
validate_fam_id( Famid fam_id )
{
   if ( fam_id < 0 ||
         fam_id >= fam_array_length ||
         fam_list[fam_id] == NULL )
   {
      return BAD_FAMILY;
   }

   return OK;
}


/*****************************************************************
 * TAG( verbosity ) PRIVATE
 *
 * Turn verbose diagnostics on or off.
 */
void
verbosity( int turn_on )
{
   mili_verbose = turn_on ? TRUE : FALSE;
}


/*****************************************************************
 * TAG( test_open_next ) LOCAL
 *
 * For STATE_COUNT partition scheme, evaluate state branch status
 * and determine if opening a new file is necessary to receive new
 * data.
 */
static Return_value
test_open_next( Mili_family *fam, Bool_type *open_next )
{
   Return_value rval = OK;

   *open_next = FALSE;
   
   if(fam->cur_st_file == NULL)
   {
      *open_next = TRUE;
   }
   else if(fam->bytes_per_file_limit >0 && fam->states_per_file >0)
   {
      if(fam->cur_st_file_size >= fam->bytes_per_file_limit ||
            fam->file_st_qty >= fam->states_per_file)
      {
         *open_next = TRUE;
      }
   }
   else if ( fam->bytes_per_file_limit >0 &&
             fam->cur_st_file_size >= fam->bytes_per_file_limit)
   {
      *open_next = TRUE;
   }
   else if( (fam->states_per_file > 0 &&
             fam->file_st_qty >= fam->states_per_file) ||
            fam->cur_st_file_size >= ABSOLUTE_MAX_FILE_SIZE)
   {
      if(fam->cur_st_file_size >= ABSOLUTE_MAX_FILE_SIZE)
      {
         if (mc_limit_states(fam->my_id,fam->file_st_qty) != OK)
         {
            rval = CORRUPTED_FILE;
         }
      }
      *open_next = TRUE;
   }
   else if(fam->cur_st_file_size >= ABSOLUTE_MAX_FILE_SIZE)
   {
      if (mc_limit_states(fam->my_id,fam->file_st_qty) != OK)
      {
         rval = CORRUPTED_FILE;
      }
      *open_next = TRUE;
   }

   return rval;
}


/*****************************************************************
 * TAG( prep_for_new_data ) PRIVATE
 *
 * Ensures that a file is open to receive data.  To be called
 * prior to an output operation which puts new data into a family,
 * not for re-writes of existing data.
 */
Return_value
prep_for_new_data( Mili_family *fam, int ftype )
{
   char amode;
   int index;
   Return_value rval;
   Bool_type open_next;

   amode = fam->access_mode;

   if ( amode == 'r' )
   {
      return BAD_ACCESS_TYPE;
   }

   if ( ftype == NON_STATE_DATA || ftype == TI_DATA)
   {
      if ( amode == 'w' || amode == 'a' )
      {
         /*
          * If read/write mode, any file might be open and a current
          * file may/may not be committed.
          */

         /* Is a file currently open? */
         if ( fam->cur_file != NULL )
         {
            index = fam->cur_index;

            /* Is the open file the last file in the family? */
            if ( index == fam->file_count - 1 )
            {
               /* Has the last file been committed? */
               if ( fam->directory[index].commit_count == 0 )
               {
                  /* Not committed - ensure file pointer is at end. */
                  if (fseek( fam->cur_file, 0L, SEEK_END ) == 0)
                  {
                     return OK;
                  }
                  else
                  {
                     return SEEK_FAILED;
                  }
               }
            }
            else
            {
               rval = non_state_file_close( fam );
               if (rval != OK)
               {
                  return rval;
               }
            }
         }
      }

      /* Open a new file within the family. */
      rval = non_state_file_open( fam, fam->file_count, fam->access_mode );
      if (rval != OK)
      {
         return rval;
      }
      fam->next_free_byte = 0;
      fam->file_count++;
   }
   else if ( ftype == STATE_DATA )
   {
      /*
       * This section implements state-branch partitioning schemes.
       */

      if ( fam->partition_scheme == STATE_COUNT )
      {
         rval = test_open_next(fam, &open_next);
         if (rval != OK)
         {
            return rval;
         }
         if ( open_next )
         {
            fam->file_map = RENEWC_N( State_file_descriptor, fam->file_map,
                                      fam->st_file_count, 1,
                                      "New file map entry" );
            if (fam->file_map == NULL)
            {
               return ALLOC_FAILED;
            }
            /* Open a new file. */
            rval = state_file_open( fam, fam->st_file_count,
                                    fam->access_mode );
            if ( rval != OK )
            {
               return rval;
            }
            fam->file_st_qty = 0;
            fam->cur_st_offset = 0;
            fam->st_file_count++;
            
         }
         else
         {
            /*
             * Note - state rec size represents only size of application-
             * defined data, not state "header" (time and srec_id) written
             * by Mili.
             */
            fam->cur_st_offset = fam->state_map[fam->file_st_qty-1].offset + fam->srecs[fam->cur_srec_id]->size;

            /* Position file pointer (for mc_wrt_st_stream()). */
            rval = seek_state_file( fam->cur_st_file, fam->cur_st_offset );
            if ( rval != OK )
            {
               return rval;
            }
         }
      }
      else
      {
         return UNKNOWN_PARTITION_SCHEME;
      }
   }
   else
   {
      return UNKNOWN_FAMILY_FILE_TYPE;
   }

   return OK;
}


/*****************************************************************
 * TAG( gen_next_state_loc ) PRIVATE
 *
 * Given a valid file sequence number and state index, determine
 * those parameters for the next state considering the family's
 * partition scheme and state branch status.
 */
Return_value
gen_next_state_loc( Mili_family *fam, int in_file, int in_state,
                    int *p_out_file, int *p_out_state )
{
   int fidx, sidx;
   Bool_type open_next;
   Return_value rval = OK;

   fidx = in_file - fam->st_file_index_offset;
   sidx = in_state;

   if ( fidx < 0 || fidx >= fam->st_file_count )
   {
      return INVALID_FILE_NAME_INDEX;
   }

   if ( sidx < 0 || sidx >= fam->file_map[fidx].state_qty )
   {
      return INVALID_FILE_STATE_INDEX;
   }

   switch( fam->partition_scheme )
   {
      case STATE_COUNT:
         if ( fidx == fam->st_file_count - 1 )
         {
            if ( sidx == fam->file_map[fidx].state_qty - 1 )
            {
            
               /*
                * Input is last extant state in family - must determine
                * if a file partition is imminent.  If so, increment
                * the file and set the state to zero.  Otherwise just
                * increment the state since another one will fit in the
                * current file.
                */
               rval = test_open_next( fam, &open_next );
               if (rval != OK)
               {
                  return rval;
               }
               if ( open_next )
               {
                  *p_out_file = ST_FILE_SUFFIX( fam, fam->st_file_count );
                  *p_out_state = 0;
               }
               else
               {
                  *p_out_file = in_file;
                  *p_out_state = sidx + 1;
               }
            }
            else
            {
               /* Last extant file, not last state in file. */
               /* Next state already exists in this file. */
               *p_out_file = in_file;
               *p_out_state = sidx + 1;
            }
         }
         else if ( sidx == fam->file_map[fidx].state_qty - 1 )
         {
            /* Not last file, but last state in file. */
            /* Next state already exists in next file. */
            *p_out_file = in_file + 1;
            *p_out_state = 0;
         }
         else
         {
            /* Not last file, not last state in file. */
            /* Next state already exists in this file. */
            *p_out_file = in_file;
            *p_out_state = sidx + 1;
         }

         break;

      default:
         rval = UNKNOWN_PARTITION_SCHEME;
         break;
   }

   return rval;
}


/*****************************************************************
 * TAG( cleanse ) PRIVATE
 *
 * Free all memory associated with a Mili_family struct.
 */
void
cleanse( Mili_family *fam )
{
   
   int i;
   free( fam->root );
   free( fam->char_header );
   free( fam->path );
   free( fam->file_root );
   free( fam->aFile);

   if ( fam->file_map != NULL )
   {
      free( fam->file_map );
   }

   if ( fam->state_map != NULL )
   {
      free( fam->state_map );
   }

   if ( fam->directory != NULL )
   {
      delete_dir( fam );
   }

   if ( fam->ti_directory != NULL )
   {
      delete_ti_dir( fam );
   }

   if ( fam->meshes != NULL )
   {
      delete_meshes( fam );
   }

   if ( fam->srecs != NULL )
   {
      delete_srecs( fam );
   }

   if ( fam->param_table != NULL )
   {
      htable_delete( fam->param_table, NULL, TRUE );
   }

   if ( fam->ti_param_table != NULL )
   {
      htable_delete( fam->ti_param_table, NULL, TRUE );
   }

   if ( fam->svar_table != NULL )
   {
      htable_delete( fam->svar_table, delete_svar, TRUE );
      ios_destroy( fam->svar_c_ios );
      ios_destroy( fam->svar_i_ios );
      fam->svar_hdr = NULL;
   }

   /* Delete other TI related vars */
   for ( i=0;
      i<fam->num_label_classes;
      i++ )
      free (fam->label_class_list[fam->num_label_classes].mclass); 
}


/*****************************************************************
 * TAG( write_header ) LOCAL
 *
 * Write the character header to the "a-file".
 */
static Return_value
write_header( Mili_family *fam )
{
   Return_value rval;
   int write_ct;

   /* Open file with read/write access to avoid truncating the A-file. */
   rval = non_state_file_open( fam, 0, 'a' );
   if (rval == OK)
   {

      write_ct = fwrite( (void *) fam->char_header, 1,
                         CHAR_HEADER_SIZE, fam->cur_file );
      if (write_ct != CHAR_HEADER_SIZE)
      {
         rval = SHORT_WRITE;
      }
      else
      {
         rval = non_state_file_close( fam );
      }
   }
   return rval;
}


/*****************************************************************
 * TAG( state_file_open ) PRIVATE
 *
 * Open a family state data file.
 */
Return_value
state_file_open( Mili_family *fam, int index, char mode )
{
   char fname[M_MAX_NAME_LEN];
   char access[4];
   Return_value rval;

   make_fnam( STATE_DATA, fam, ST_FILE_SUFFIX( fam, index ), fname );

   rval = OK;

   /* Don't need to open if it already is and has requested mode. */
   if ( fam->cur_st_index != index || fam->cur_st_file_mode != mode )
   {
      if ( fam->cur_st_index != -1 )
      {
         rval = state_file_close( fam );
         if (rval != OK)
         {
            return rval;
         }
      }

      set_file_access( mode, fam->st_file_count > index, access );
      /*fam->st_file_count > index, (void *) access );*/

      rval = open_buffered( fname, access,
                            &fam->cur_st_file,
                            &fam->cur_st_file_size );

      /* Set current file index. */
      if ( rval == OK )
      {
         fam->cur_st_index = index;
         fam->cur_st_file_mode = mode;
         if ( fam->file_map != NULL )
         {
            fam->file_st_qty = fam->file_map[index].state_qty;
         }
      }
   }

   return rval;
}


/*****************************************************************
 * TAG( non_state_file_open ) PRIVATE
 *
 * Open a family file.
 */
Return_value
non_state_file_open( Mili_family *fam, int index, char mode )
{
   char fname[M_MAX_NAME_LEN];
   int fcnt;
   Return_value rval;
   char access[4];

   make_fnam( NON_STATE_DATA, fam, index, fname );

   if( mili_verbose )
   {
      printf("\nOpening NON_STATE_FILE %s", fname);
   }

   rval = OK;

   if ( fam->cur_index != index )
   {
      fcnt = fam->file_count;

      if ( fam->cur_index != -1 )
      {
         rval = non_state_file_close( fam );
         if (rval != OK)
         {
            return rval;
         }
      }

      set_file_access( mode, fcnt > index, access );

      rval = open_buffered( fname, access, &fam->cur_file,
                            &fam->cur_file_size );
      /**/
      /* "next_free_byte" is managed as "cur_file_size" should be, and the two
       * should probably be consolidated, but need to check libtaurus's
       * dependencies on "cur_file_size" first.
       */
      fam->next_free_byte = fam->cur_file_size;

      /* Set current file index and manage directory. */
      if ( rval == OK )
      {
         fam->cur_index = index;

         if ( index == fcnt )
         {
            fam->directory = RENEW_N( File_dir, fam->directory, fcnt, 1,
                                      "File dir head" );
            if (fam->directory == NULL)
            {
               rval = ALLOC_FAILED;
            }
         }
      }
      else
      {
         fam->cur_index = -1;
      }
   }

   return rval;
}


/*****************************************************************
 * TAG( set_file_access ) PRIVATE
 *
 * Set the appropriate access for a file depending on file type,
 * family access mode, and I/O type.
 */
void
set_file_access( char fam_access, Bool_type file_exists, char *file_access )
{
   switch( fam_access )
   {
      case 'w':
      case 'a':
         if ( file_exists )
         {
            strcpy( file_access, "rb+" );
         }
         else
         {
            strcpy( file_access, "wb+" );
         }
         break;
      case 'r':
         strcpy( file_access, "rb" );
         break;
   }
}


/*****************************************************************
 * TAG( open_buffered ) PRIVATE
 *
 * Open a file for buffered I/O access.
 */
Return_value
open_buffered( char *fname, char *mode, FILE **p_file_descr, 
               LONGLONG *p_size )
{
   FILE *p_f;
   struct stat file_stat;
   int status;
   Return_value rval = OK;

   /* Open the file. */
   p_f = fopen( fname, mode );
   if ( p_f == NULL )
   {
      *p_file_descr = NULL;
      rval = OPEN_FAILED;
   }
   else
   {
      status = stat( fname, &file_stat );
      if ( status == 0 )
      {
         *p_size = file_stat.st_size;
      }
      else
      {
         if ( mili_verbose )
         {
            fprintf( stderr, "Mili warning - unable to obtain file "
                     "size.\n" );
         }
         rval = UNABLE_TO_STAT_FILE;
      }
      *p_file_descr = p_f;
   }
   return rval;
}


/*****************************************************************
 * TAG( non_state_file_seek ) PRIVATE
 *
 * Seek a family file.
 */
Return_value
non_state_file_seek( Mili_family *fam, LONGLONG offset )
{
   int stat;

   /*    if ( offset >= fam->cur_file_size ) */
   if ( offset >= fam->next_free_byte )
   {
      return SEEK_FAILED;
   }

   stat = fseek( fam->cur_file, offset, SEEK_SET );

   return ( stat == 0 ) ? OK : SEEK_FAILED;
}


/*****************************************************************
 * TAG( seek_state_file ) PRIVATE
 *
 * Seek a family state data file via the standard C I/O library.
 */
Return_value
seek_state_file( FILE *cur_st_file, LONGLONG offset )
{
   int stat;

   stat = fseek( cur_st_file, offset, SEEK_SET );

   return stat ? SEEK_FAILED : OK;
}


/*****************************************************************
 * TAG( non_state_file_close ) PRIVATE
 *
 * Open a family file.
 */
Return_value
non_state_file_close( Mili_family *fam )
{
   Return_value rval;

   rval = OK;
   if ( fam->cur_file != NULL )
   {
      if ( COMMIT_NS( fam ) && ( fam->db_type != TAURUS_DB_TYPE ) )
      {
         rval = commit_non_state( fam );
         if (rval != OK)
         {
            return rval;
         }
      }
      if (fclose( fam->cur_file ) != 0)
      {
         rval = UNABLE_TO_CLOSE_FILE;
      }
      fam->cur_file  = NULL;
      fam->cur_index = -1;
   }

   return rval;
}


/*****************************************************************
 * TAG( state_file_close ) PRIVATE
 *
 * Close a family state file.
 */
Return_value
state_file_close( Mili_family *fam )
{
   char *p_fname;
   Return_value rval = OK;
   int fd;

   if(!fam){
      return OK;
   } 
   if( !(fam->cur_st_file)){
      return OK;
   }

   if (fclose( fam->cur_st_file ) != 0)
   {
      rval = UNABLE_TO_CLOSE_FILE;
   }
   fam->cur_st_file = 0;
   if (rval != OK)
   {
      return rval;
   }

   /* Lock file update necessary for a new file in write access database. */
   if ( (fam->access_mode == 'w' || fam->access_mode == 'a')
         && fam->cur_st_index == fam->st_file_count )
   {
      /*
       * Update contents of the write lock file to indicate last
       * complete state file.
       */
      fd = fam->lock_file_descriptor;

      /* Prepare the text. */
      p_fname = NEW_N( char, M_MAX_NAME_LEN, "Lock file NS name buffer" );
      if (p_fname == NULL)
      {
         return ALLOC_FAILED;
      }
      make_fnam( STATE_DATA, fam,
                 ST_FILE_SUFFIX( fam, fam->st_file_count - 1 ), p_fname );

      /*
       * Lock the length bytes to be written to notify any reader
       * that an update is in progress.
       */
      rval = get_name_lock( fam, STATE_DATA );
      if ( rval != OK )
      {
         /* Shouldn't ever get here - notify user. */
         if ( mili_verbose )
         {
            fprintf( stderr,
                     "Mili - State file close failed to place name "
                     "lock.\n" );
         }
      }

      /*
       * Write the length, then the name (get_name_lock() seeks
       * correct file position).
       */

      if ( filelock_enable )
      {
         write( fd, (void *) p_fname, M_MAX_NAME_LEN );
      }

      free( p_fname );

      /* Remove the advisory lock. */
      if ( rval == OK )
      {
         rval = free_name_lock( fam, STATE_DATA );
      }
   }

   fam->cur_st_index = -1;
   fam->cur_st_file_mode = '\0';

   return rval;
}


/*****************************************************************
 * TAG( mc_partition_state_data ) PUBLIC
 *
 * Create a file partition in the state data stream.
 */
Return_value
mc_partition_state_data( Famid fam_id )
{
   Mili_family *p_fam;

   p_fam = fam_list[fam_id];

   if ( p_fam->cur_st_file != NULL )
   {
      return state_file_close( p_fam );
   }
   else
   {
      return OK;
   }
}



/*****************************************************************
 * Time Independent file functions:
 * Added May 2006 by I.R. Corey
 *****************************************************************
 */


/*****************************************************************
 * TAG( ti_file_open ) PRIVATE
 *
 * Open a TI filetype.
 */
Return_value
ti_file_open( Famid fam_id, int index, char mode )
{
   Mili_family *fam;
   char fname[M_MAX_NAME_LEN];
   int fcnt;
   Return_value rval;
   char access[4];

   fam = fam_list[fam_id];

   /* Exit if TI capability not requested */
   if (!fam->ti_enable)
   {
      return OK;
   }

   make_fnam( TI_DATA, fam, index, fname );

   if( mili_verbose )
   {
      printf("\nOpening TI file %s", fname);
   }

   rval = OK;

   if ( fam->ti_cur_index != index )
   {
      fcnt = fam->ti_file_count;

      if ( fam->ti_cur_index != -1 )
      {
         rval = ti_file_close( fam_id );
         if (rval != OK)
         {
            return rval;
         }
      }

      set_file_access( mode, fcnt > index, access );

      rval = open_buffered( fname, access, &fam->ti_cur_file,
                            &fam->ti_cur_file_size );

      /* "next_free_byte" is managed as "ti_cur_file_size" should be, and the two
       * should probably be consolidated, but need to check libtaurus's
       * dependencies on "cur_file_size" first.
       */
      fam->ti_next_free_byte = fam->ti_cur_file_size;

      /* Set current file index and manage directory. */
      if ( rval == OK )
      {
         mc_ti_data_found( fam_id );
         fam->ti_cur_index = index;

         if ( index == fcnt )
         {
            fam->ti_directory = RENEW_N( File_dir, fam->ti_directory, fcnt, 1,
                                         "File dir head" );
         }
      }
      else
      {
         mc_ti_data_not_found( fam_id );
         fam->ti_cur_index = -1;
      }
   }
   return rval;
}


/*****************************************************************
 * TAG( ti_file_close ) PRIVATE
 *
 * Close a TI type file.
 */
Return_value
ti_file_close(  Famid fam_id )
{
   Mili_family *fam;
   Return_value rval = OK;

   fam = fam_list[fam_id];

   if ( fam != NULL )
   {
      if ( fam->ti_enable && fam->ti_cur_file != NULL )
      {
         if ( COMMIT_TI( fam )  )
         {
            rval = ti_commit( fam_id );
            if (rval != OK)
            {
               return rval;
            }
         }
         if (fclose( fam->ti_cur_file ) != 0)
         {
            rval = UNABLE_TO_CLOSE_FILE;
         }
         fam->ti_cur_file  = NULL;
         fam->ti_cur_index = -1;
      }
   }

   return rval;
}


/*****************************************************************
 * TAG( ti_commit ) PRIVATE
 *
 *
 */
Return_value
ti_commit( Famid fam_id )
{
   Mili_family *fam;
   Return_value rval;

   char *p_fname;
   int fd;

   rval = validate_fam_id( fam_id );
   if ( rval != OK )
   {
      return rval;
   }

   fam = fam_list[fam_id];

   /* Exit if TI capability not requested */
   if (!fam->ti_enable)
   {
      return OK;
   }

   rval = prep_for_new_data( fam, TI_DATA );
   if ( rval != OK )
   {
      return rval;
   }

   /* Ensure correct file pointer. Could be off if access is "a". */
   if (fseek( fam->ti_cur_file, 0L, SEEK_END ) != 0)
   {
      return SEEK_FAILED;
   }

   rval = commit_ti_dir( fam );
   if ( rval != OK )
   {
      return rval;
   }

   if (fflush( fam->ti_cur_file ) != 0)
   {
      return UNABLE_TO_FLUSH_FILE;
   }

   /*
    * Update contents of the write lock file to indicate last
    * complete non-state file.
    */
   /* Ignore if file locking disabled */
   if ( filelock_enable )
   {
      fd = fam->lock_file_descriptor;
   }

   /* Prepare the text. */
   p_fname = NEW_N( char, M_MAX_NAME_LEN, "Lock file NS name buffer" );
   if (p_fname == NULL)
   {
      return ALLOC_FAILED;
   }
   make_fnam( TI_DATA, fam, fam->file_count - 1, p_fname );

   /*
    * Lock the length bytes to be written to notify any reader
    * that an update is in progress.
    */
   rval = get_name_lock( fam, NON_STATE_DATA );
   if ( rval != OK )
   {
      /* Shouldn't ever get here - notify user. */
      if ( mili_verbose )
      {
         fprintf( stderr, "Mili - Flush failed to place name lock.\n" );
      }
   }

   /*
    * Write the length, then the name (get_name_lock() seeks
    * correct file position).
    */
   /* Ignore if file locking disabled */
   if ( filelock_enable )
   {
      if (write( fd, (void *) p_fname, M_MAX_NAME_LEN ) != M_MAX_NAME_LEN)
      {
         return SHORT_WRITE;
      }
   }

   /* Remove the advisory lock.*/
   if ( rval == OK )
   {
      rval = free_name_lock( fam, NON_STATE_DATA );
   }

   fam->ti_commit_count++;
   fam->non_state_ready = FALSE;

   free( p_fname );

   return rval;
}


/*****************************************************************
 * TAG( mc_ti_init_metadata ) PUBLIC
 *
 * Write currently known metadata entries to a TI file.
 */
Return_value
mc_ti_init_metadata( Famid fam_id )
{
   time_t t;
   char char_t[25];
   char nambuf[70];
   char mili_version[64];

   Mili_family *fam;
   int alloc_size;

   Return_value rval;

#ifdef SYSINFO_PRESENT
   char* archbuf;
   long sys_stat;
   struct utsname sysdata;
#endif

   char* user_info;

   fam = fam_list[fam_id];

   /* Exit if TI capability not requested */
   if (!fam->ti_enable)
   {
      return OK;
   }

   /* Library version. */
   get_mili_version( mili_version );
   rval = mc_ti_wrt_string( fam_id, "lib version", (char *) mili_version );
   if (rval != OK)
   {
      return rval;
   }

   /* Host name and OS type. */
#if defined(_WIN32) || defined(WIN32)
   sprintf( nambuf, "windows_machine");
#else
   if (gethostname( nambuf, 64 ) != 0)
   {
      return BAD_HOST_NAME;
   }
#endif
   rval = mc_ti_wrt_string( fam_id, "host name", nambuf );
   if (rval != OK)
   {
      return rval;
   }

#ifdef SYSINFO_PRESENT
   sys_stat = uname( &sysdata );
   if (sys_stat != 0)
   {
      return BAD_SYSTEM_INFO;
   }
   alloc_size = strlen(sysdata.sysname) + 3 + strlen(sysdata.release) +
                strlen(sysdata.nodename) + 1;
   archbuf = NEW_N(char, alloc_size, "Architecture string buffer.");
   if (alloc_size > 0 && archbuf == NULL)
   {
      return ALLOC_FAILED;
   }
   strncpy(archbuf,sysdata.sysname, strlen(sysdata.sysname)+1);
   strncat(archbuf, " + ", 3);
   strncat(archbuf, sysdata.release, strlen(sysdata.release));
   strncat(archbuf, sysdata.nodename, strlen(sysdata.nodename));
   rval = mc_ti_wrt_string( fam_id, "arch name", archbuf );
   if (rval != OK)
   {
      return rval;
   }
   free(archbuf);

#endif

   /* File creation date and time. */
   t = time( 0 );
   memcpy( char_t, ctime( &t ), sizeof( char_t ) );
   char_t[24] = '\0';
   rval = mc_ti_wrt_string( fam_id, "date", char_t );
   if (rval != OK)
   {
      return rval;
   }

   /* User info */

   /* Does not exist on Lightweight kernel - Red Storm */
   user_info = getenv(USERNAME);
   if (user_info != NULL)
   {
      rval = mc_ti_wrt_string( fam_id, "username", user_info );
   }else{
      rval = mc_ti_wrt_string( fam_id, "username", "undefined" );
   }
   
   return rval;
}


/*****************************************************************
 * TAG( mc_ti_get_metadata ) PUBLIC
 *
 * Read currently known metadata entries from a TI file.
 */
Return_value
mc_ti_get_metadata( Famid fam_id, char *mili_version, char *host,
                    char *arch, char *timestamp, char *username,
                    char *xmilics_version, char *code_name )
{
   Mili_family *fam;

   fam = fam_list[fam_id];

   /* Exit if TI capability not requested */
   if (!fam->ti_enable)
   {
      return OK;
   }

   mili_version[0]    = '\0';
   host[0]            = '\0';
   arch[0]            = '\0';
   timestamp[0]       = '\0';
   username[0]        = '\0';
   xmilics_version[0] = '\0';
   code_name[0]       = '\0';

   /* Not checking return value as these are all optional */
   mc_ti_read_string(fam_id, "lib version", mili_version );
   mc_ti_read_string(fam_id, "host name", host );
   mc_ti_read_string(fam_id, "arch name", arch );
   mc_ti_read_string(fam_id, "date", timestamp );
   mc_ti_read_string(fam_id, "username", username );
   mc_ti_read_string(fam_id, "xmiliics version", xmilics_version );
   mc_ti_read_string(fam_id, "code_name", code_name );

   return OK;
}


/*****************************************************************
 * TAG( mc_ti_check_arch ) PUBLIC
 *
 * Check TI files for consistant architecture - Used to make sure
 * that TI files were generated on same arch as mili database,
 *.
 */
Return_value
mc_ti_check_arch( Famid fam_id )
{
   Mili_family *fam;

   char mili_version[32], host[64],
        arch[32],         timestamp[64],
        xmilics_version[64];

   char ti_mili_version[32], ti_host[64],
        ti_arch[32],         ti_timestamp[64],
        ti_user[32],         ti_xmilics_version[64],
        ti_code_name[64];
   Return_value rval;

   fam = fam_list[fam_id];

   /* Exit if TI capability not requested */
   if (!fam->ti_enable)
   {
      return OK;
   }

   rval = mc_get_metadata(    fam_id, mili_version,    host,    arch,
                              timestamp, xmilics_version ) ;
   if (rval != OK)
   {
      return rval;
   }
   rval = mc_ti_get_metadata( fam_id, ti_mili_version, ti_host, ti_arch,
                              ti_timestamp, ti_user,   ti_xmilics_version,
                              ti_code_name ) ;
   if (rval != OK)
   {
      return rval;
   }

   if (!strcmp(arch,ti_arch))
   {
      fprintf( stderr, "Mili Error - Mili Arch [%s] does not match TI arch [%s]\n",
               arch, ti_arch );
      return NOT_OK;
   }
   else
   {
      return OK;
   }
}


/*****************************************************************
 * TAG( ti_read_header ) LOCAL
 *
 * Read the family header from a TI file and initialize the fam struct.
 */
static Return_value
ti_read_header( Mili_family *fam )
{
   char fname[M_MAX_NAME_LEN];
   char header[CHAR_HEADER_SIZE];
   FILE *p_f;
   LONGLONG nitems;
   int i;
   int status;
   struct stat sbuf;
   extern int errno;
   Return_value rval;

   if ( fam->char_header != NULL )
   {
      return MULTIPLE_HEADER_READ;
   }

   make_fnam( TI_DATA, fam, 0, fname );

   /*
    * Treat syscall "stat" failure with ENOENT as sufficient and necessary
    * condition for determining that the file does not exist (which opens
    * the door for application of write access in default case).
    */
   status = stat( fname, &sbuf );
   if ( status == -1 )
   {
      if ( errno == ENOENT )
      {
         return FAMILY_NOT_FOUND;
      }
      else
      {
         return OPEN_FAILED;
      }
   }

   /* Found the file; open it. */
   p_f = fopen( fname, "rb" );
   if ( p_f == NULL )
   {
      return OPEN_FAILED;
   }

   /* Read in the header to verify compatibility and init family. */
   nitems = fread( (void *) header, 1, CHAR_HEADER_SIZE, p_f );

   fclose( p_f );

   if ( nitems != CHAR_HEADER_SIZE )
   {
      return SHORT_READ;
   }

   /* Check for old style header - bytes 5-9 will be ASCII upper case. */
   rval = OK;
   for ( i = 4; i < 9; i++ )
   {
      if ( header[i] < 65 )
      {
         break;
      }
   }
   if ( i == 9 )
   {
      /* Map old header into new format. */
      map_old_header( header );

      /* OK to continue, but set status to show re-mapped header. */
      rval = MAPPED_OLD_HEADER;
   }

   if ( header[HDR_VERSION_IDX] > header_version )
   {
      if ( mili_verbose )
      {
         fprintf( stderr, "Mili warning - family header version newer than "
                  "library header version.\n" );
      }
   }

   if ( header[DIR_VERSION_IDX] > directory_version )
   {
      if ( mili_verbose )
      {
         fprintf( stderr, "Mili warning - family directory version newer "
                  "than library directory version.\n" );
      }
   }

   fam->precision_limit  = (Precision_limit_type) header[PRECISION_LIMIT_IDX];
   fam->st_suffix_width  = (int) header[ST_FILE_SUFFIX_WIDTH_IDX];
   fam->partition_scheme = (File_partition_scheme) header[PARTITION_SCHEME_IDX];

   /* Allocate space for the character header data. */
   fam->char_header = NEW_N( char, CHAR_HEADER_SIZE, "Family char header" );
   if (fam->char_header == NULL)
   {
      return ALLOC_FAILED;
   }

   /* Copy the header. */
   memcpy( fam->char_header, header, CHAR_HEADER_SIZE );

   return rval;
}
/*****************************************************************
 * TAG( is_correct_param_type ) PRIVATE
 *
 * Function to determine if the parameter is a either a TI_PARAM or MILI_PARAM
 */
static int
is_correct_param_type(int fam_id,char *param_name, int type){
   
   Mili_family *fam;
   int i,name_idx,fidx,qty_entries;
   fam = fam_list[fam_id];
   name_idx=0;
    
   for( fidx=0; fidx < fam->file_count; fidx++ )
   {
      qty_entries = fam->directory[fidx].qty_entries;
 		
      for (i=0; i<qty_entries; i++)
      {
			
         if ( !strcmp(fam->directory[fidx].names[name_idx], param_name) )
         {
				
            if(fam->directory[fidx].dir_entries[i][TYPE_IDX]== type)
            {
               return 1;
            }else{
               return 0;
            }
            
         }
         name_idx += fam->directory[fidx].dir_entries[i][STRING_QTY_IDX];
      }
   }
   return 0;
          
}
/*****************************************************************
 * TAG( mc_htable_search_wildcard ) PUBLIC
 *
 * MC version of htable_search_wildcard used for searching the mili
 * param table only.
 */
int
mc_htable_search_wildcard( int fid, int list_len, Bool_type allow_duplicates,
                           char *key1, char *key2, char *key3,
                           char **return_list )
{
   int         i,count,pos;
   Mili_family *fam;
   Hash_table  *table;
   char **temp_list =NULL;
   int old_ti_file = 0;
   pos=0;
   fam   = fam_list[fid];
	if ( (!fam->db_type == TAURUS_DB_TYPE) && fam->char_header[DIR_VERSION_IDX] == 1 && fam->ti_enable )
   {
		table = fam->ti_param_table;
		old_ti_file = 1;
   }else
   {
		table = fam->param_table;
   }

   count = htable_search_wildcard( table, list_len, allow_duplicates,
                                    key1, key2, key3,
                                    temp_list );
   if(count > 0 )
   {
      temp_list = (char **) malloc(count*sizeof(char *));
   
      count = 0;
   
      count = htable_search_wildcard( table, list_len, allow_duplicates,
                                      key1, key2, key3,
                                      temp_list );
                                      
      /**
      Need to Check if these are actually TI parameters.
      */
      for(i=0;i<count;i++)
      {
         if(!old_ti_file)
			{
				if(!is_correct_param_type(fid,temp_list[i],TI_PARAM))
         	{
            	if(return_list != NULL)
            	{
               	str_dup( &return_list[pos],temp_list[i] );
               	//strcpy(return_list[pos] ,temp_list[i]);
            	}
            	pos++;
         	}
			}else
			{
				if(return_list != NULL)
            {
               str_dup( &return_list[pos],temp_list[i] );
               //strcpy(return_list[pos] ,temp_list[i]);
            }
			}
      }
      free(temp_list);
      count = pos;
   }
   return ( count );
}

/*****************************************************************
 * TAG( mc_ti_htable_search_wildcard ) PUBLIC
 *
 * MC version of htable_search_wildcard used for searching the TI
 * param table only.
 */
int
mc_ti_htable_search_wildcard( int fid, int list_len,
                              Bool_type allow_duplicates,
                              char *key1, char *key2, char *key3,
                              char **return_list )
{
   int         i,count,pos=0;
   Mili_family *fam;
   Hash_table  *table;
   char **temp_list =NULL;
	int old_ti_file = 0;

   fam   = fam_list[fid];
   if ( (!fam->db_type == TAURUS_DB_TYPE) && fam->char_header[DIR_VERSION_IDX] == 1 && fam->ti_enable )
   {
      table = fam->ti_param_table;
		old_ti_file = 1;
   }else
   {  
      table = fam->param_table;
   }
   count = htable_search_wildcard( table, list_len, allow_duplicates,
                                    key1, key2, key3,
                                    temp_list );
												
	if(count > 0 )
   {
      temp_list = (char **) malloc(count*sizeof(char *));
   
      count = 0;
   
      count = htable_search_wildcard( table, list_len, allow_duplicates,
                                      key1, key2, key3,
                                      temp_list );
      /**
      Need to Check if these are actually TI parameters.
      */
      for(i=0;i<count;i++)
      {
			if(!old_ti_file && is_correct_param_type(fid,temp_list[i],TI_PARAM))
         {
				if(return_list != NULL)
            {
                 str_dup( &return_list[pos],temp_list[i] );
                 //strcpy(return_list[pos] ,temp_list[i]);
            }
            pos++;
         }else 
			{
				if(return_list != NULL)
            {
                 str_dup( &return_list[pos],temp_list[i] );
                 //strcpy(return_list[pos] ,temp_list[i]);
            }
			}
      }
      free(temp_list);
      count = pos;
   }
   
   return ( count );
}


/*
 * New file locking enable/disable functions
 */

/*****************************************************************
 * TAG( mc_filelock_enable ) PUBLIC
 *
 * Enable all file locking
 */
void
mc_filelock_enable( void )

{
   filelock_enable = TRUE;
}


/*****************************************************************
 * TAG( mc_filelock_disable ) PUBLIC
 *
 * Disable all file locking
 */
void
mc_filelock_disable( void )

{
   filelock_enable = FALSE;
}


/*****************************************************************
 * TAG( mc_silo_enable ) PUBLIC
 *
 * Silo I/O library will be used for all low-level I/O.
 */
void
mc_silo_enable( )
{
   milisilo = TRUE;
}


/*****************************************************************
 * TAG( mc_silo_disable ) PUBLIC
 *
 * Silo I/O library will NOT be used for all low-level I/O.
 */
void
mc_silo_disable( )
{
   milisilo = FALSE;
}


/*****************************************************************
 * TAG( mc_get_next_famid ) PUBLIC
 *
 * Returns the next available family id number.
 */
int
mc_get_next_famid( void )
{
   return fam_qty;
}


/*****************************************************************
 * TAG( mc_calc_bytecount ) PUBLIC
 *
 * This function calculates and returns the number of bytes of
 * data for a give datatype.
 */
LONGLONG
mc_calc_bytecount( int datatype, LONGLONG size )
{
   LONGLONG byte_count=0;

   switch (datatype)
   {
      case (M_STRING):
      case (M_FLOAT):
      case (M_FLOAT4):
      case (M_FLOAT8):
      case (M_INT):
      case (M_INT4):
      case (M_INT8):
         byte_count = size*internal_sizes[datatype];
         break;
   }
   return ( byte_count );
}
