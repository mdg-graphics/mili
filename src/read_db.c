
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
 
 * read_db.c
 *
 *      Lawrence Livermore National
 *
 * Copyright (c) 1991, 2010 Lawrence Livermore National
 */

/*
 *                        L E G A L   N O T I C E
 *
 *                          (C) Copyright 1991, 2010
 */

/*
************************************************************************
* Modifications:
*
*  I. R. Corey - January 11, 2008: Ported from Xmilics to serve as
*  a general purpose Mili database reader..
*
*  I. R. Corey - April 30, 2010: Incorporated changes from Sebastian
*                at IABG to support building under Windows.
*                SCR #678.
*
************************************************************************
*/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <time.h>
#include "mili_internal.h"


/*****************************************************************
 * TAG( fam_list )
 *
 * Dynamically allocated array of pointers to all currently open
 * MILI families.
 */
extern Mili_family **fam_list;

/*****************************************************************
 * TAG( read_state_data ) LOCAL
 *
 * Read state data.
 *
 */
Return_value
read_state_data( int state_num, Mili_analysis *in_db )
{
   Famid fam_id;
   Mili_family *fam;
   void *result_buf;
   Srec *p_sr;
   Sub_srec *p_subrec;
   Svar *svar;
   int srec_id;
   int num_type;
   int atom_size;
   int precision, iprec;
   size_t state_size;
   int st;
   int subrec_qty, qty_svars;
   int i, j;
   int idx;
   int round;
   LONGLONG offset;
   char *p_c;
   char *primals[2];
   char primal_spec[32];
   Return_value rval;

   fam_id = in_db->db_ident;
   fam = fam_list[fam_id];

   if( fam->qty_srecs == 0 ) {
      return (Return_value)OK;
   }

   srec_id = fam->qty_srecs - 1;
   p_sr = fam->srecs[srec_id];

   precision = fam->precision_limit;

   rval = (Return_value)OK;
   state_size = (size_t)p_sr->size / EXT_SIZE( fam, M_FLOAT4 ) + 1;
   switch( precision ) {
      case PREC_LIMIT_SINGLE:
         if( in_db->result == NULL ) {
            in_db->result = NEW_N( float, state_size, "Results" );
         }
         break;

      case PREC_LIMIT_DOUBLE:
         if( in_db->result == NULL ) {
            in_db->result = (float *)NEW_N( double, state_size, "Results" );
         }
         break;

      default:
         rval = UNKNOWN_PRECISION;
   }
   if ( rval != OK ) {
      return rval;
   }

   st = state_num +1;

   iprec = 1;
   offset = 0;
   round  = 0;

   subrec_qty = p_sr->qty_subrecs;
   for( i = 0; i < subrec_qty; i++ ) {
      p_subrec = p_sr->subrecs[i];
      qty_svars = p_subrec->qty_svars;
      for( j = 0; j < qty_svars; j++ ) {
         svar = p_subrec->svars[j];
         num_type = *svar->data_type;
         atom_size = EXT_SIZE( fam, num_type );
         switch( num_type ) {
            case M_INT:
            case M_INT4:
            case M_FLOAT:
            case M_FLOAT4:
               iprec = 1;
               break;
            case M_INT8:
            case M_FLOAT8:
               iprec = 2;
               break;
         }

         strcpy( primal_spec, svar->name );
         primals[0] = primal_spec;
         primals[1] = NULL;
         p_c = (char *)(in_db->result  + offset);
         result_buf = (void *)p_c;
         /* Read the database. */
         mc_read_results( fam_id, st, i, 1, primals, result_buf );

         if( p_subrec->organization == RESULT_ORDERED ) {
            offset +=  p_subrec->lump_atoms[j]*iprec;
         } else {
            if( *svar->agg_type == SCALAR ) {
               offset +=  p_subrec->mo_qty*iprec;
            } else if( *svar->agg_type == VECTOR ) {
               offset += (*svar->list_size * p_subrec->mo_qty*iprec);
            } else if( *svar->agg_type == ARRAY )
               for( idx = 0; idx < *svar->order; idx ++ ) {
                  offset += (svar->dims[idx] * p_subrec->mo_qty*iprec);
               }
            else if( *svar->agg_type == VEC_ARRAY ) {
               /***** Need to check and make sure this is correct *****/
               for( idx = 0; idx < *svar->order; idx ++ ) {
                  offset += (svar->dims[idx]*svar->list_size[idx]*p_subrec->mo_qty*iprec);
               }
            }
         }
      }
   }

   state_file_close( fam );

   return (Return_value)OK;
}

