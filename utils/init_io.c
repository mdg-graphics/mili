/* $Id: init_io.c,v 1.8 2012/11/15 20:38:28 durrenberger1 Exp $ */
/*
 * init_io.c - Database access initialization routines.
 *             Routines in this module hard-code knowledge of database
 *             formats in order to ascertain the type (I/O library of
 *             origin) without actually calling the originating library.
 *
 *      Douglas E. Speck
 *      Methods Development Group
 *      Lawrence Livermore National Laboratory
 *      14 Feb 1997
 *
 * Copyright (c) 1997 Regents of the University of California
 */

#include "driver.h"


#define CTL_WORDS 40

/************************************************************
 * TAG( is_mili_db )
 *
 * Verify a Mili database family by looking for an "A" file
 * and reading the Mili "magic" byte sequence at the beginning
 * of the file.
 */
Bool_type
is_mili_db( char *fname )
{
   char mili_fname[M_MAX_NAME_LEN];
   char *cbuf;
   int mb_len;
   FILE *p_f;
   Bool_type rval;

   /* MILI LIBRARY-DEPENDENT! */
   char *magic_bytes = "mili";

   /* Build the "A" file name. */
   sprintf( mili_fname, "%sA", fname );

   mb_len = strlen( magic_bytes );
   cbuf = NEW_N( char, mb_len + 1, "Mili magic bytes" );

   rval = FALSE;

   p_f = fopen( mili_fname, "r" );
   if ( p_f != NULL ) {
      fread( cbuf, 1, mb_len, p_f );
      if ( strcmp( magic_bytes, cbuf ) == 0 ) {
         rval = TRUE;
      }

      fclose( p_f );
   }

   free( cbuf );

   return rval;
}


/************************************************************
 * TAG( is_taurus_plot_db )
 *
 * Attempt to verify a Taurus plotfile database family by
 * empirical checking of selected control header values.
 */
Bool_type
is_taurus_plot_db( char *fname )
{
   FILE *p_f;
   Bool_type rval;
   int ctl[CTL_WORDS];
   size_t count;
   int title_bytes;
   int prtchar_cnt;
#ifdef KEEP_TITLE_CHECK
   int i;
   char *p_title;
#endif

   rval = TRUE;

   p_f = fopen( fname, "r" );
   if ( p_f != NULL ) {
      /* Read in the control section. */
      count = fread( ctl, sizeof( int ), CTL_WORDS, p_f );
      if ( count != CTL_WORDS ) {
         return FALSE;
      }

      /* First 10 words should contain only printable characters. */
      title_bytes = 10 * sizeof( int );
#ifndef KEEP_TITLE_CHECK
      prtchar_cnt = title_bytes;
#else
      prtchar_cnt = 0;
      for ( i = 0, p_title = (char *) ctl; i < title_bytes; p_title++, i++ )
         if ( isprint( (int) *p_title ) ) {
            prtchar_cnt++;
         }
#endif

      /* Perform checks. */
      if ( ( prtchar_cnt < title_bytes - 1 ) || ( prtchar_cnt > title_bytes ) ) {
         rval = FALSE;
      } else if ( ctl[15] != 2      /* 2D database */
                  && ctl[15] != 3     /* 3D packed database */
                  && ctl[15] != 4 ) { /* 3D unpacked database */
         rval = FALSE;
      } else if ( ctl[17] != 1      /* Topaz database */
                  && ctl[17] != 2     /* old Dyna/Nike database */
                  && ctl[17] != 6     /* new Dyna/Nike database */
                  && ctl[17] != 200   /* Hydra database */
                  && ctl[17] != 300 ) { /* Ping database */
         rval = FALSE;
      } else if ( ctl[19] != 0 && ctl[19] != 1 ) { /* node temperature flag */
         rval = FALSE;
      } else if ( ctl[20] != 0 && ctl[20] != 1 ) { /* node displacements flag */
         rval = FALSE;
      } else if ( ctl[21] != 0 && ctl[21] != 1 ) { /* node velocities flag */
         rval = FALSE;
      } else if ( ctl[22] != 0 && ctl[22] != 1 ) { /* node accelerations flag */
         rval = FALSE;
      }

      fclose( p_f );
   } else {
      rval = FALSE;
   }

   return rval;
}


/************************************************************
 * TAG( is_byte_swapped_taurus_plot_db )
 *
 * Attempt to verify a Taurus plotfile database family by
 * empirical checking of selected control header values,
 * with values appropriately modified to match a db written
 * on a platform with opposite endianness of current host.
 */
Bool_type
is_byte_swapped_taurus_plot_db( char *fname )
{
   FILE *p_f;
   Bool_type rval;
   unsigned int ctl[CTL_WORDS];
   size_t count;
   int title_bytes;
   int prtchar_cnt;
#ifdef KEEP_TITLE_CHECK
   char *p_title;
   int i;
#endif

   rval = TRUE;

   p_f = fopen( fname, "r" );
   if ( p_f != NULL ) {
      /* Read in the control section. */
      count = fread( ctl, sizeof( int ), CTL_WORDS, p_f );
      if ( count != CTL_WORDS ) {
         return FALSE;
      }

      /* First 10 words should contain only printable characters. */
      title_bytes = 10 * sizeof( int );
#ifndef KEEP_TITLE_CHECK
      prtchar_cnt = title_bytes;
#else
      prtchar_cnt = 0;
      for ( i = 0, p_title = (char *) ctl; i < title_bytes; p_title++, i++ )
         if ( isprint( (int) *p_title ) ) {
            prtchar_cnt++;
         }
#endif

      /* Perform checks. */
      if ( prtchar_cnt != title_bytes ) {
         rval = FALSE;
      } else if ( ctl[15] != 33554432      /* 2D database */
                  && ctl[15] != 50331648     /* 3D packed database */
                  && ctl[15] != 67108864 ) { /* 3D unpacked database */
         rval = FALSE;
      } else if ( ctl[17] != 16777216      /* Topaz database */
                  && ctl[17] != 33554432     /* old Dyna/Nike database */
                  && ctl[17] != 100663296    /* new Dyna/Nike database */
                  && ctl[17] != 3355443200   /* Hydra database */
                  && ctl[17] != 738263040 ) { /* Ping database */
         rval = FALSE;
      } else if ( ctl[19] != 0 && ctl[19] != 16777216 ) { /* node temperature flag */
         rval = FALSE;
      } else if ( ctl[20] != 0 && ctl[20] != 16777216 ) { /* node displacements flag */
         rval = FALSE;
      } else if ( ctl[21] != 0 && ctl[21] != 16777216 ) { /* node velocities flag */
         rval = FALSE;
      } else if ( ctl[22] != 0 && ctl[22] != 16777216 ) { /* node accelerations flag */
         rval = FALSE;
      }

      fclose( p_f );
   } else {
      rval = FALSE;
   }

   return rval;
}

/************************************************************
 * TAG( is_known_db )
 *
 * Verify that a handle represents a database of a known
 * format.
 */
Bool_type
is_known_db( char *fname, Database_type *p_db_type )
{
   if ( is_mili_db( fname ) ) {
      *p_db_type = MILI_DB_TYPE;
   } else if ( is_taurus_plot_db( fname ) ) {
      *p_db_type = TAURUS_DB_TYPE;
   } else if ( is_byte_swapped_taurus_plot_db( fname ) ) {
      *p_db_type = TAURUS_DB_TYPE;
   } else {
      return FALSE;
   }

   return TRUE;
}

