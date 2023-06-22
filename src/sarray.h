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

 * sarray.h - Header file for the StringArray library, containing
 *            functions and macros to use and manage StringArrays.
 *
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
 *
 *
 *
 * A StringArray is an extendable, relocatable, and randomly accessible
 * buffer of arbitrary length strings.  It uses a single contiguous
 * block of dynamically allocated memory.  The memory is mapped as a
 * set of control words followed by an array of offsets into the buffer
 * for each string, then the strings themselves.  The strings are
 * stored in the reverse order of their insertion starting at the high
 * end of the buffer and progressively working lower in memory as the
 * offsets array simultaneously grows from the low end of the buffer into
 * higher memory.  The integer type used for descriptors and offsets
 * determines the maximum addressable buffer size and can be reset from
 * the default (unsigned short) to an appropriate unsigned type prior
 * to compilation.  When a new string is added to a StringArray, it may
 * require a reallocation to extend memory and a subsequent copy operation
 * to push the strings to the top of the newly extended buffer.  Reallocations
 * acquire the smallest whole multiple of the initial StringArray size
 * which will accommodate the string causing the extension to occur.
 * An application should call free() on the StringArray to release its
 * memory when it is no longer needed.
 *
 * All StringArray functions but the string add and sort operations are
 * implemented as macros, and macro interfaces to the string add and sort
 * functions are provided for consistency.
 *
 * The data types and macros of interest to users are:
 *  Types:
 *      StringArray - a string array object.
 *      SAIntType - the integer type (should be unsigned) mapped into
 *          the StringArray buffer to manage its contents.
 *  Macros:
 *      SANEW( sa ) - initialize StringArray sa to a new StringArray of
 *          initial length DEFAULT_SIZE.
 *      SANEWS( sa, size ) - initialize StringArray sa to a new StringArray
 *          of initial length "size".
 *      SASTRING( sa, idx ) - returns pointer to string in StringArray
 *          sa indexed by idx.
 *      SAQTY( sa ) - returns quantity of strings in StringArray sa.
 *      SASORT( sa, a ) - sort the offsets in StringArray sa so that
 *          its strings are referenced in ascending lexicographic order
 *          (if "a" is non-zero) or descending lexicographic order
 *          ("a" is zero).
 *
 * Code example:
 *
 *      #include "sarray.h"
 *
 *      StringArray sa;
 *      int ival, i;
 *
 *      SANEW( sa );
 *
 *      sa_add( &sa, "pea" );
 *      sa_add( &sa, "bean" );
 *      sa_add( &sa, "lentil" );
 *
 *      SASORT( sa, 1 );
 *
 *      printf( "These are members (alphabetized) of the legume family:\n" );
 *      ival = SAQTY( sa );
 *      for ( i = 0; i < ival; i++ )
 *          printf( "%d. %s\n", i + 1, SASTRING( sa, i ) );
 *
 *      free( sa );
 *
 *
 * Function sa_add() returns a non-negative integer
 * on successful completion which is the inserted string's index in
 * the string array.  This index may be used as input to the SASTRING
 * macro to generate a character pointer to the string.  If an error
 * occurs upon string insertion, sa_add() returns a negative integer.
 * The interpretation of the error values is as follows:
 *      OVERSIZED_SARRAY  The new string required an extension which could not
 *                        be performed because the StringArray buffer would
 *                        exceed the maximum addressable length permitted by
 *                        the compiled SAIntType.
 *      ALLOC_FAILED      A reallocation attempt to extend the StringArray
 *                        failed.
 *
 *
 * Map of integers at start of StringArray buffer:
 *
 * 0________|1________|2________|3________|4...____________________________
 * Buffer   |String   |Used     |Initial  |Array of offsets to strings
 * length   |quantity |string   |buffer   |
 *          |         |storage  |length   |
 */

#ifndef STRINGARRAY
#define STRINGARRAY

#include "mili_enum.h"

#define DEFAULT_SIZE 1024

typedef unsigned short SAIntType;

typedef char *StringArray;

enum Reserved_ints
{
    SA_LEN_IDX = 0,
    SA_QTY_IDX,
    SA_SUSED_IDX,
    SA_ALLOC_SZ_IDX, /* _Always_ last, so the field it indexes will
                         immediately precede the offset array */
    RESERVED_INT_QTY
};

/* Function prototypes. */
Return_value sa_add(StringArray *, char *, int *);
void sa_sort(StringArray, int);
/*
int sa_remove_s( StringArray, char * );
int sa_remove_i( StringArray, int );
*/

/*****************************************************************
 * TAG( SAI )
 *
 * Cast a StringArray as a pointer to SAIntType for array-type
 * access to numeric descriptors at front of buffer.
 */
#define SAI(l) ((SAIntType *)(l))

/*****************************************************************
 * TAG( SALEN SAQTY SAUSED SAASIZE )
 *
 * Convenience macros to access the buffer length, string quantity,
 * length of buffer portion storing null-terminated strings, and initial
 * allocation size, respectively, of a StringArray.
 */
#define SALEN(l) SAI(l)[SA_LEN_IDX]
#define SAQTY(l) SAI(l)[SA_QTY_IDX]
#define SAUSED(l) SAI(l)[SA_SUSED_IDX]
#define SAASIZE(l) SAI(l)[SA_ALLOC_SZ_IDX]

/*****************************************************************
 * TAG( SANEWS )
 *
 * Allocate and initialize a new StringArray with a specified size.
 * Initialize the allocation size entry to the total buffer size.
 * The allocation size entry supports two functions: (1) subsequent
 * reallocations will be performed based on this size and (2) a
 * SASTRING reference to the 'qty - 1'th string when no strings are
 * present will return an address of the byte past the end of the
 * StringArray, which will be the right thing for the sa_add()
 * function's calculation of empty space available.
 */
#define SANEWS(h, s)                   \
    (h) = (StringArray)calloc((s), 1); \
    SALEN(h) = (s);                    \
    SAASIZE(h) = (s);

/*****************************************************************
 * TAG( SANEW )
 *
 * Allocate and initialize a new StringArray of default size.
 */
#define SANEW(h) SANEWS(h, DEFAULT_SIZE)

/*****************************************************************
 * TAG( SASTRING )
 *
 * Return the address of an indexed string in a StringArray.
 */
#define SASTRING(a, i) ((((StringArray)(a)) + ((SAIntType *)(a))[RESERVED_INT_QTY + (i)]))

/*****************************************************************
 * TAG( SASORT )
 *
 * Macro for sa_sort() function to sort the strings in a StringArray
 * in either ascending or descending lexicographic order.  If
 * parameter "a" is non-zero, the strings will be referenced in
 * ascending order following the sort.
 */
#define SASORT(s, a) sa_sort(s, a)

/*****************************************************************
 * TAG( SAREMS ) NOT IMPLEMENTED
 *
 * Macro for SARemoveS() function to remove a string from a
 * StringArray identified by value.

#define SAREMS( a, s ) sa_remove_s( (StringArray) (a), (char *) (s) )
 */

/*****************************************************************
 * TAG( SAREMI ) NOT IMPLEMENTED
 *
 * Macro for SARemoveI() function to remove a string from a
 * StringArray identified by index.

#define SAREMI( a, i ) sa_remove_i( (StringArray) (a), (int) (i) )
 */

#endif
