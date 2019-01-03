/* Architecture dependent code.
 *
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
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include "mili_internal.h"

/* Specific low level IO functions. */
static size_t rd_byte( FILE *file, void *data, size_t qty );
static size_t wrt_byte( FILE *file, void *data, size_t qty );
static size_t rd_4byte( FILE *file, void *data, size_t qty );
static size_t wrt_4byte( FILE *file, void *data, size_t qty );
static size_t rd_4byte_swap( FILE *file, void *data, size_t qty );
static size_t wrt_4byte_swap( FILE *file, void *data, size_t qty );
static size_t rd_8byte( FILE *file, void *data, size_t qty );
static size_t wrt_8byte( FILE *file, void *data, size_t qty );
static size_t rd_8byte_swap( FILE *file, void *data, size_t qty );
static size_t wrt_8byte_swap( FILE *file, void *data, size_t qty );
static size_t rd_bytes_swap( FILE *file, void *data,
                             size_t qty, size_t chunk_size);
static size_t wrt_bytes_swap( FILE *file, void *data,
                              size_t qty, size_t chunk_size);


/*****************************************************************
 * TAG( set_default_io_routines ) PRIVATE
 *
 * Initialize pointers to the various i/o routines in the family
 * struct on the basis of target file precision and host architecture.
 */
Return_value
set_default_io_routines( Mili_family *fam )
{
#if TIMER
   clock_t start,stop;
   double cumalative=0.0;
   start = clock();
#endif
   Return_value rval;

   rval = OK;

   switch ( fam->precision_limit )
   {
      /*
       * Single precision - change formats as required.
       */
      case PREC_LIMIT_SINGLE:
         /* Initialize external sizes and types of internal data types. */
         fam->external_size[M_STRING] = 1;
         fam->external_size[M_INT] = 4;
         fam->external_size[M_FLOAT] = 4;
         fam->external_size[M_INT4] = 4;
         fam->external_size[M_INT8] = 8;
         fam->external_size[M_FLOAT4] = 4;
         fam->external_size[M_FLOAT8] = 8;

         fam->external_type[M_STRING] = M_STRING;
         fam->external_type[M_INT] = M_INT;
         fam->external_type[M_FLOAT] = M_FLOAT;
         fam->external_type[M_INT4] = M_INT;
         fam->external_type[M_INT8] = M_INT8;
         fam->external_type[M_FLOAT4] = M_FLOAT;
         fam->external_type[M_FLOAT8] = M_FLOAT8;

         /* Writing to an already existing single precision database from
          * double precision hardware is problematic as there is no guarantee
          * that the double precision data that we want to write can be
          * represented as single precision values.  Since we will never
          * write M_INT8 or M_FLOAT8 data to a single precision database
          * those write function pointers will never be accessed for a
          * single precision database and will be set to NULL below.
          *
         if ((internal_sizes[M_FLOAT8] > fam->external_size[M_FLOAT8]) &&
             (fam->access_mode == 'a'))
         {
            return DATA_PRECISION_MISMATCH;
         }
         */

         /* Initialize write and read routines. */    
         if ( fam->swap_bytes )
         {
            fam->write_funcs[M_STRING] = wrt_byte;
            fam->write_funcs[M_INT4] = wrt_4byte_swap;
            fam->write_funcs[M_INT8] = wrt_8byte_swap;
            fam->write_funcs[M_FLOAT4] = wrt_4byte_swap;
            fam->write_funcs[M_FLOAT8] = wrt_8byte_swap;
            fam->write_funcs[M_INT] = wrt_4byte_swap;
            fam->write_funcs[M_FLOAT] = wrt_4byte_swap;

            fam->read_funcs[M_STRING] = rd_byte;
            fam->read_funcs[M_INT4] = rd_4byte_swap;
            fam->read_funcs[M_INT8] = rd_8byte_swap;
            fam->read_funcs[M_FLOAT4] = rd_4byte_swap;
            fam->read_funcs[M_FLOAT8] = rd_8byte_swap;
            fam->read_funcs[M_INT] = rd_4byte_swap;
            fam->read_funcs[M_FLOAT] = rd_4byte_swap;
         }
         else
         {
            fam->write_funcs[M_STRING] = wrt_byte;
            fam->write_funcs[M_INT4] = wrt_4byte;
            fam->write_funcs[M_INT8] = wrt_8byte;
            fam->write_funcs[M_FLOAT4] = wrt_4byte;
            fam->write_funcs[M_FLOAT8] = wrt_8byte;
            fam->write_funcs[M_INT] = wrt_4byte;
            fam->write_funcs[M_FLOAT] = wrt_4byte;

            fam->read_funcs[M_STRING] = rd_byte;
            fam->read_funcs[M_INT4] = rd_4byte;
            fam->read_funcs[M_INT8] = rd_8byte;
            fam->read_funcs[M_FLOAT4] = rd_4byte;
            fam->read_funcs[M_FLOAT8] = rd_8byte;
            fam->read_funcs[M_INT] = rd_4byte;
            fam->read_funcs[M_FLOAT] = rd_4byte;
         }
         break;

      /* Double precision - any size OK; format changes may be required. */
      case PREC_LIMIT_DOUBLE:
         /* Initialize external sizes and types of internal data types. */
         fam->external_size[M_STRING] = 1;
         fam->external_size[M_INT] = 4;
         fam->external_size[M_FLOAT] = 4;
         fam->external_size[M_INT4] = 4;
         fam->external_size[M_INT8] = 8;
         fam->external_size[M_FLOAT4] = 4;
         fam->external_size[M_FLOAT8] = 8;

         fam->external_type[M_STRING] = M_STRING;
         fam->external_type[M_INT] = M_INT;
         fam->external_type[M_FLOAT] = M_FLOAT;
         fam->external_type[M_INT4] = M_INT;
         fam->external_type[M_INT8] = M_INT8;
         fam->external_type[M_FLOAT4] = M_FLOAT;
         fam->external_type[M_FLOAT8] = M_FLOAT8;

         /* Reading an already existing double precision database from
          * single precision hardware is problematic as there is no gaurantee
          * that the double precision data in the database can be represented
          * internally as single precision values.
          *
         * 
         This check should occur in the functions to read and write. Not 
         here as although it is probably not a good idea to open an 8 byte
         mili database on a 4 byte machine, all the data stored internally 
         may be only 4 byte data.
         if ((internal_sizes[M_FLOAT8] < fam->external_size[M_FLOAT8]) &&
             (fam->access_mode == 'r'))
         {
            return DATA_PRECISION_MISMATCH;
         }
         */

         /* Initialize write and read routines. */    
         if ( fam->swap_bytes )
         {
            fam->write_funcs[M_STRING] = wrt_byte;
            fam->write_funcs[M_INT4] = wrt_4byte_swap;
            fam->write_funcs[M_INT8] = wrt_8byte_swap;
            fam->write_funcs[M_FLOAT4] = wrt_4byte_swap;
            fam->write_funcs[M_FLOAT8] = wrt_8byte_swap;
            fam->write_funcs[M_INT] = wrt_4byte_swap;
            fam->write_funcs[M_FLOAT] = wrt_4byte_swap;

            fam->read_funcs[M_STRING] = rd_byte;
            fam->read_funcs[M_INT4] = rd_4byte_swap;
            fam->read_funcs[M_INT8] = rd_8byte_swap;
            fam->read_funcs[M_FLOAT4] = rd_4byte_swap;
            fam->read_funcs[M_FLOAT8] = rd_8byte_swap;
            fam->read_funcs[M_INT] = rd_4byte_swap;
            fam->read_funcs[M_FLOAT] = rd_4byte_swap;
         }
         else
         {
            fam->write_funcs[M_STRING] = wrt_byte;
            fam->write_funcs[M_INT4] = wrt_4byte;
            fam->write_funcs[M_INT8] = wrt_8byte;
            fam->write_funcs[M_FLOAT4] = wrt_4byte;
            fam->write_funcs[M_FLOAT8] = wrt_8byte;
            fam->write_funcs[M_INT] = wrt_4byte;
            fam->write_funcs[M_FLOAT] = wrt_4byte;

            fam->read_funcs[M_STRING] = rd_byte;
            fam->read_funcs[M_INT4] = rd_4byte;
            fam->read_funcs[M_INT8] = rd_8byte;
            fam->read_funcs[M_FLOAT4] = rd_4byte;
            fam->read_funcs[M_FLOAT8] = rd_8byte;
            fam->read_funcs[M_INT] = rd_4byte;
            fam->read_funcs[M_FLOAT] = rd_4byte;
         }
         break;

      default:
         rval = UNKNOWN_PRECISION;
         break;
   }

   if ( rval == OK )
   {
      /* Initialize the state data I/O routines. */
      rval = set_state_data_io_routines( fam );
   }
#if TIMER
   stop = clock();
   cumalative =((double)(stop - start))/CLOCKS_PER_SEC;
   printf("Function set_default_io_routines()\nTime is: %f\n",cumalative);
#endif
   return rval;
}


/*****************************************************************
 * TAG( set_state_data_io_routines ) PRIVATE
 *
 * Initialize pointers to the state data i/o routines in the family
 * struct on the basis of target file precision and host architecture.
 */
Return_value
set_state_data_io_routines( Mili_family *fam )
{
   Return_value rval;
   size_t (**rf)();
   size_t (**wf)();

   rval = OK;
   rf = fam->state_read_funcs;
   wf = fam->state_write_funcs;

   /* Just copy the non-state I/O function references. */
   memcpy( rf, fam->read_funcs,
           QTY_PD_ENTRY_TYPES * sizeof( (void *) NULL ) );
   memcpy( wf, fam->write_funcs,
           QTY_PD_ENTRY_TYPES * sizeof( (void *) NULL ) );

   return rval;
}


/*****************************************************************
 * TAG( rd_byte, wrt_byte ) LOCAL
 *
 * Buffered I/O on character or byte data from/to a file.
 */
static size_t
rd_byte( FILE *file, void *data, size_t qty )
{
   return fread( data, 1, qty, file );
}

static size_t
wrt_byte( FILE *file, void *data, size_t qty )
{
   return fwrite( data, 1, qty, file );
}


/*****************************************************************
 * TAG( rd_4byte, wrt_4byte ) LOCAL
 *
 * Buffered I/O on four-byte chunks of un-translated data from/to a file.
 */
static size_t
rd_4byte( FILE *file, void *data, size_t qty )
{
   return fread( data, 4, qty, file );
}

static size_t
wrt_4byte( FILE *file, void *data, size_t qty )
{
   return fwrite( data, 4, qty, file );
}


/*****************************************************************
 * TAG( rd_4byte_swap, wrt_4byte_swap ) LOCAL
 *
 * Buffered I/O on four-byte chunks of un-translated data from/to
 * a file with byte-order swap.
 */
static size_t
rd_4byte_swap( FILE *file, void *data, size_t qty )
{
   return rd_bytes_swap( file, data, qty, 4 );
}

static size_t
wrt_4byte_swap( FILE *file, void *data, size_t qty )
{
   return wrt_bytes_swap( file, data, qty, 4 );
}


/*****************************************************************
 * TAG( rd_8byte, wrt_8byte ) LOCAL
 *
 * Buffered I/O on eight-byte chunks of untranslated data from/to a file.
 */
static size_t
rd_8byte( FILE *file, void *data, size_t qty )
{
   return fread( data, 8, qty, file );
}

static size_t
wrt_8byte( FILE *file, void *data, size_t qty )
{
   return fwrite( data, 8, qty, file );
}


/*****************************************************************
 * TAG( rd_8byte_swap, wrt_8byte_swap ) LOCAL
 *
 * Buffered I/O on eight-byte chunks of byte-swapped data from/to
 * a file.
 */
static size_t
rd_8byte_swap( FILE *file, void *data, size_t qty )
{
   return rd_bytes_swap( file, data, qty, 8 );
}

static size_t
wrt_8byte_swap( FILE *file, void *data, size_t qty )
{
   return wrt_bytes_swap( file, data, qty, 8 );
}


/*****************************************************************
 * TAG( rd_bytes_swap, wrt_bytes_swap ) LOCAL
 *
 * Buffered I/O on chunks of chunk_size byte-swapped data from/to 
 * a file.
 */
static size_t
rd_bytes_swap( FILE *file, void *data, size_t qty, size_t chunk_size )
{
   size_t nitems;
   unsigned char *rawdata;

   rawdata = NEW_N( unsigned char, qty * chunk_size, "swapbuf" );
   if (rawdata != NULL)
   {

      nitems = fread( (void *) rawdata, chunk_size, qty, file );

      swap_bytes( nitems, chunk_size, (void *) rawdata, data );

      free( rawdata );
   }
   else
   {
      nitems = 0;
   }

   return nitems;
}

static size_t
wrt_bytes_swap( FILE *file, void *data, size_t qty, size_t chunk_size )
{
   size_t nitems;
   unsigned char *swapdata;

   swapdata = NEW_N( unsigned char, qty * chunk_size, "swapbuf" );
   if (swapdata != NULL)
   {

      swap_bytes( qty, chunk_size, data, (void *) swapdata );

      nitems = fwrite( (void *) swapdata, chunk_size, qty, file );

      free( swapdata );
   }
   else
   {
      nitems = 0;
   }

   return nitems;
}
