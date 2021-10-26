/* $Id: io_funcs.c,v 1.20 2012/12/13 17:04:05 durrenberger1 Exp $ */
/*
 * io_wrappers.c - Wrappers for Mili and Taurus plotfile I/O functions
 *                 which implement a standard Griz I/O interface.
 *
 *      Douglas E. Speck
 *      Methods Development Group
 *      Lawrence Livermore National Laboratory
 *      14 Feb 1997
 *
 * Copyright (c) 1997 Regents of the University of California
 */
#include <stdlib.h>
#include "driver.h"

/************************************************************
 * TAG( mili_db_open )
 *
 * Open a Mili database family.
 *
 * This would be a lot cleaner if mc_open() just took a
 * consolidated path/name string instead of separate arguments.
 */
extern int
mili_db_open( char *path_root, int *p_dbid )
{
   char *p_c;
   char *p_root_start, *p_root_end;
   char *p_src, *p_dest;
   char *path;
   char root[128];
   char path_text[256];
   int rval;
   Famid dbid;

   /* Scan forward to end of name string. */
   for ( p_c = path_root; *p_c != '\0'; p_c++ ) {
      ;
   }

   /* Scan backward to last non-slash character. */
   for ( p_c--; *p_c == '/' && p_c != path_root; p_c-- ) {
      ;
   }
   p_root_end = p_c;

   /* Scan backward to last slash character preceding last non-slash char. */
   for ( ; *p_c != '/' && p_c != path_root; p_c-- ) {
      ;
   }

   p_root_start = ( *p_c == '/' ) ? p_c + 1 : p_c;

   /* Generate the path argument to mc_open(). */
   if ( p_root_start == path_root )
      /* No path preceding root name. */
   {
      path = NULL;
   } else {
      /* Copy path (less last slash). */

      path = path_text;

      for ( p_src = path_root, p_dest = path_text;
            p_src < p_root_start - 1;
            *p_dest++ = *p_src++ ) {
         ;
      }

      if ( p_src == path_root )
         /* Path must be just "/".  If that's what the app wants... */
      {
         *p_dest++ = *path_root;
      }

      *p_dest = '\0';
   }

   /* Generate root name argument to mc_open(). */
   for ( p_src = p_root_start, p_dest = root;
         p_src <= p_root_end;
         *p_dest++ = *p_src++ ) {
      ;
   }
   *p_dest = '\0';

   rval = mc_open( root, path, "r", &dbid );

   if ( rval != OK ) {
      mc_print_error( "mili_db_open() call mc_open()", rval );
      return XMILICS_FAIL;
   } else {
      *p_dbid = dbid;
   }

   return OK;
}


/************************************************************
 * TAG( taurus_db_open )
 *
 * Open a Taurus plotfile database family.
 */
extern int
taurus_db_open( char *path_root, int *p_dbid )
{
   char *p_c;
   char *p_root_start, *p_root_end;
   char *p_src, *p_dest;
   char *path;
   char root[128];
   char path_text[256];
   int rval;
   Famid dbid;

   /* Scan forward to end of name string. */
   for ( p_c = path_root; *p_c != '\0'; p_c++ ) {
      ;
   }

   /* Scan backward to last non-slash character. */
   for ( p_c--; *p_c == '/' && p_c != path_root; p_c-- ) {
      ;
   }
   p_root_end = p_c;

   /* Scan backward to last slash character preceding last non-slash char. */
   for ( ; *p_c != '/' && p_c != path_root; p_c-- ) {
      ;
   }

   p_root_start = ( *p_c == '/' ) ? p_c + 1 : p_c;

   /* Generate the path argument to taurus_open(). */
   if ( p_root_start == path_root )
      /* No path preceding root name. */
   {
      path = NULL;
   } else {
      /* Copy path (less last slash). */

      path = path_text;

      for ( p_src = path_root, p_dest = path_text;
            p_src < p_root_start - 1;
            *p_dest++ = *p_src++ ) {
         ;
      }

      if ( p_src == path_root )
         /* Path must be just "/".  If that's what the app wants... */
      {
         *p_dest++ = *path_root;
      }

      *p_dest = '\0';
   }

   /* Generate root name argument to taurus_open(). */
   for ( p_src = p_root_start, p_dest = root;
         p_src <= p_root_end;
         *p_dest++ = *p_src++ ) {
      ;
   }
   *p_dest = '\0';

   rval = taurus_open( root, path, "r", &dbid );

   if ( rval == 0 ) {
      *p_dbid = dbid;
   }

   return rval;
}
/************************************************************
 * TAG( open_input_dbase )
 *
 * Open a plotfile and initialize an database.
 */
Bool_type
open_input_dbase( char *fname, Mili_analysis *analy )
{
   int stat;

   /*
    * Don't fail before db is actually open.
    */

   /* Initialize I/O functions by database type. */
   if ( !is_known_db( fname, &db_type ) ) {
      fprintf(stderr, "\n\t WARNING: Unable to identify database type. %s",fname );
      return FALSE;
   }

   /* Open the database. */
   if( db_type == MILI_DB_TYPE ) {
      stat = mili_db_open( fname, &analy->db_ident );
      if ( stat != OK ) {
         fprintf(stderr, "\n\t WARNING: Unable to open Mili database." );
         return FALSE;
      }
   } else if( db_type == TAURUS_DB_TYPE ) {
      stat = taurus_db_open( fname, &analy->db_ident );
      if ( stat != OK ) {
         fprintf(stderr, "\n\t WARNING: Unable to open Taurus database." );
         return FALSE;
      }
   }
   if(analy->root_name == NULL) {
      analy->root_name = NEW_N( char, strlen(fname)+1, "Mili_analysis root_name" );
      analy->root_name[strlen(fname)] = '\0';

      strcpy( analy->root_name, fname );
   }
   
   return TRUE;
}


/************************************************************
 * TAG( open_output_dbase )
 *
 * Open a plotfile and initialize an database.
 */
Bool_type
open_output_dbase( char *path_root, Mili_analysis *dbase )
{
   Famid dbid;
   char *p_c;
   char *p_root_start, *p_root_end;
   char *p_src, *p_dest;
   char *path;
   char root[128];
   char path_text[256];
   int rval;
   Mili_mesh_data *mesh_array;

   /* Scan forward to end of name string. */
   for ( p_c = path_root; *p_c != '\0'; p_c++ ) {
      ;
   }

   /* Scan backward to last non-slash character. */
   for ( p_c--; *p_c == '/' && p_c != path_root; p_c-- ) {
      ;
   }
   p_root_end = p_c;

   /* Scan backward to last slash character preceding last non-slash char. */
   for ( ; *p_c != '/' && p_c != path_root; p_c-- ) {
      ;
   }

   p_root_start = ( *p_c == '/' ) ? p_c + 1 : p_c;

   /* Generate the path argument to taurus_open(). */
   if ( p_root_start == path_root )
      /* No path preceding root name. */
   {
      path = NULL;
   } else {
      /* Copy path (less last slash). */

      path = path_text;

      for ( p_src = path_root, p_dest = path_text;
            p_src < p_root_start - 1;
            *p_dest++ = *p_src++ ) {
         ;
      }

      if ( p_src == path_root )
         /* Path must be just "/".  If that's what the app wants... */
      {
         *p_dest++ = *path_root;
      }

      *p_dest = '\0';
   }

   /* Generate root name argument to taurus_open(). */
   for ( p_src = p_root_start, p_dest = root;
         p_src <= p_root_end;
         *p_dest++ = *p_src++ ) {
      ;
   }
   *p_dest = '\0';

   /* Open or create the family. */
   rval = mc_open( root, path, "wd", &dbid );

   if ( rval != 0 ) {
      fprintf(stderr, "WARNING: Couldn't open output database \"%s\".", path_root );
      return FALSE;
   }
   dbase->root_name = NEW_N( char, strlen(path_root)+1, "Dbase root_name" );
   dbase->root_name[strlen(path_root)] = '\0';

   strcpy( dbase->root_name, path_root );
   dbase->db_ident = dbid;

   return TRUE;
}


/************************************************************
 * TAG( close_dbase )
 * Free up all the memory absorbed by a database.
 */
void
close_dbase(Mili_analysis *dbase, int full )
{
   if(full){
      if ( dbase->result != NULL ) {
         free( dbase->result );
      }
      if ( dbase->root_name != NULL ) {
         free( dbase->root_name );
      }
      if ( dbase->state_times != NULL ) {
         free( dbase->state_times );
      }
   }
   mc_close( dbase->db_ident );
}


