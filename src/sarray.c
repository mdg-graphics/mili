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
 */

#include <string.h>
#include <stdlib.h>
#ifdef DBGSA
#include <stdio.h>
#endif
#include "mili_internal.h"
#include "sarray.h"

static unsigned long long max =
   (CLONGLONGCONST(1)<<(sizeof( SAIntType ) * 8)) - 1;

static StringArray sort_sarr;
static int sort_coefficient;

static int SDTLIBCC sort_sa_offsets( const void *elem1, const void *elem2 );
#ifdef DBGSA
static void usage();
static void print_sarr( StringArray sa );
#endif


/*****************************************************************
 * TAG( sa_add ) PRIVATE
 *
 * Add a new string to a string heap.  Extend buffer through
 * reallocation and move the strings if necessary.  Add the new
 * string offset.
 *
 * Function sa_add() returns the string's index in the StringArray
 * (zero-based) or a negative value if it cannot complete the
 * insertion.
 */
Return_value
sa_add( StringArray *p_sarr, char *str, int *qty )
{
   StringArray sarr;
   LONGLONG slen, i_used, add, avail;
   LONGLONG blen, newlen, growth;
   char *p_src, *p_dest, *p_stop;
   int i;
   SAIntType *p_i;

   sarr = *p_sarr;
   slen = strlen( str ) + 1;
   *qty = SAQTY( sarr );
   i_used = RESERVED_INT_QTY + *qty;
   p_stop = sarr + SALEN( sarr ) - SAUSED( sarr );
   add = slen + sizeof( SAIntType );
   avail = p_stop - (sarr + i_used * sizeof( SAIntType ));

   /* Extend the buffer if necessary. */
   if ( avail < add )
   {      
      ldiv_t dres;

      /* Calculate required new space in multiples of the initial size. */
      blen = SALEN( sarr );
      dres = ldiv( add - avail, (LONGLONG) SAASIZE( sarr ) );
      growth = SAASIZE( sarr ) * (dres.quot + (( dres.rem > 0 ) ? 1 : 0));
      newlen = blen + growth;

      if ( newlen > max )
      {
         return OVERSIZED_SARRAY;
      }

      sarr = (StringArray) realloc( (void *) sarr, newlen );
      if ( newlen > 0 && sarr == NULL )
      {
         return ALLOC_FAILED;
      }
      else if ( sarr != *p_sarr )
      {
         /* StringArray was moved during realloc - update necessary var's. */
         *p_sarr = sarr;
         p_stop = sarr + SALEN( sarr ) - SAUSED( sarr );
#ifdef DBGSA
         printf( "StringArray relocated during realloc.\n" );
#endif
      }

      /* Save new length. */
      SALEN( sarr ) = newlen;

      /* Move the strings. */
      for ( p_src = sarr + blen - 1, p_dest = sarr + newlen - 1;
            p_src >= p_stop;
            *p_dest-- = *p_src-- );

      /* Adjust the offsets. */
      for ( i = 0, p_i = SAI( sarr ) + RESERVED_INT_QTY; i < *qty; i++ )
      {
         p_i[i] += growth;
      }

      /*
       * Update reference to the low end of the string data to its
       * post-move location.
       */
      p_stop += growth;

#ifdef DBGSA
      printf( "StringArray extended from %d to %d bytes.\n", blen, newlen );
#endif
   }

   /* Add the new string, its offset, and increment the string count. */
   p_stop -= slen;
   strcpy( p_stop, str );
   SAI( sarr )[i_used] = (SAIntType) (p_stop - sarr);
   SAQTY( sarr ) += 1;
   SAUSED( sarr ) += slen;

   return OK;
}


/*****************************************************************
 * TAG( sort_sa_offsets ) LOCAL
 *
 * Comparison function for qsort that provides StringArray
 * lexicographic sorting via the strcmp() function.
 */
static int
sort_sa_offsets( const void *elem1, const void *elem2 )
{
   return strcmp( sort_sarr + *((SAIntType *) elem1),
                  sort_sarr + *((SAIntType *) elem2) )
          * sort_coefficient;
}


/*****************************************************************
 * TAG( sa_sort ) PRIVATE
 *
 * Sort the offsets in a StringArray so that the strings are
 * referenced in lexicographic ascending or descending order,
 * based on parameter "ascending" being non-zero or zero,
 * respectively.
 */
void
sa_sort( StringArray sarr, int ascending )
{
   int qty;

   qty = SAQTY( sarr );

   /* Sanity check */
   if ( qty < 2 )
   {
      return;
   }

   /* Initialize file scope variables for access by the sorting function. */
   sort_sarr = sarr;
   sort_coefficient = ascending ? 1 : -1;

   /* Sort 'em */
   qsort( SAI( sarr ) + RESERVED_INT_QTY, qty, sizeof( SAIntType ),
          sort_sa_offsets );
}


#ifdef DBGSA
static void
usage()
{
   printf( "Usage: a.out <StringArray size>\n" );
}


static void
print_sarr( StringArray sa )
{
   int i, qty;

   qty = SAQTY( sa );

   printf( "StringArray size: %d\n", SALEN( sa ) );
   printf( "String quantity: %d\n", qty );
   for ( i = 0; i < qty; i++ )
   {
      printf( "%d. %s\n", i, SASTRING( sa, i ) );
   }
}


void
main( int argc, char *argv[] )
{
   char p_input[128];
   char *rd;
   int do_more;
   StringArray sarr;
   int sa_size;
   Return_value status;
   int index;

   if ( argc == 1 )
   {
      usage();
      exit( 0 );
   }

   sa_size = atol( argv[1] );
   if ( sa_size < RESERVED_INT_QTY * sizeof( SAIntType ) )
   {
      fprintf( stderr, "Buffer size must be greater %d.\n",
               RESERVED_INT_QTY * sizeof( SAIntType ) );
      exit( 1 );
   }

   SANEWS( sarr, sa_size );
   if ( sarr == NULL )
   {
      fprintf( stderr, "Unable to allocate StringArray.\n" );
      exit( 2 );
   }

   rd = p_input;
   do_more = 1;

   printf( "q[uit], d[ump], s[ort] or <text string>: " );
   rd = gets( p_input );

   while( do_more && rd == p_input )
   {
      if ( p_input[0] == 'd' && p_input[1] == '\0' )
      {
         print_sarr( sarr );
      }
      else if ( p_input[0] == 's' && p_input[1] == '\0' )
      {
         SASORT( sarr, 1 );
      }
      else if ( p_input[0] == 'q' && p_input[1] == '\0' )
      {
         do_more = 0;
      }
      else
      {
         status = sa_add( &sarr, p_input, &index );
         if ( status != OK )
         {
            if ( status == OVERSIZED_SARRAY )
            {
               fprintf( stderr,
                        "StringArray - unable to add string \"%s\".\n",
                        p_input );
            }
            else if ( status == ALLOC_FAILED )
            {
               fprintf( stderr,
                        "StringArray - failed to grow buffer.\n" );
            }

            free( sarr );
            exit( 3 );
         }
         else
         {
            printf( "Added at %d - \"%s\"\n", index, p_input );
         }
      }

      if ( do_more )
      {
         printf( "q[uit], d[ump], s[ort], or <text string>: " );
         rd = gets( p_input );
      }
   }

   free( sarr );
   exit( 0 );
}
#endif

