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
 
 * write_db.c
 *
 *      Lawrence Livermore National Laboratory
 *
 * Copyright (c) 1991
 */

/*
************************************************************************
* Modifications:
*
*  I. R. Corey - September 23, 2008: Ported from Xmilics to serve as
*  a general purpose Mili database reader..
*
*  I. R. Corey - March 20, 2009: Added support for WIN32 for use with
*                with Visit.
*                See SCR #587.
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
#else
#include <io.h>
#endif

#include <time.h>
#include "mili_internal.h"

#define QTY_NODE_TAGS (2)



/*****************************************************************
 * TAG( fam_list )
 *
 * Dynamically allocated array of pointers to all currently open
 * MILI families.
 */
extern Mili_family **fam_list;

/*****************************************************************
 * TAG( write_state_data ) LOCAL
 *
 * Write state datea to file.
 *
 */
Return_value
write_state_data( int state_num, Mili_analysis *out_db )
{
   Famid fam_id;
   Mili_family *fam;
   Srec *p_sr;
   Sub_srec *psubrec;
   float st_time;
   int qty_states;
   int states_per_file;
   int num_files;
   int width;
   int j, k;
   int p_file_suffix;
   int p_file_state_index;
   int srec_id;
   int iprec;
   int num_type;
   int atom_size;
   int subrec_qty;
   int result_size;
   int round;
   LONGLONG offset;
   void *state_out;
   char *p_c;
   Return_value rval;

   fam_id = out_db->db_ident;
   rval = validate_fam_id( fam_id );
   if ( rval != OK ) {
      return rval;
   }

   fam = fam_list[fam_id];

   if ( rval != OK ) {
      return rval;
   }

   if ( (fam->partition_scheme == STATE_COUNT ) && (state_num == 0 ) ) {
      rval = mc_query_family( fam_id, QTY_STATES, NULL, NULL,
                              &qty_states );
      states_per_file = fam->states_per_file;
      if(states_per_file) {
         num_files = qty_states/states_per_file;
      } else {
         num_files = 1;
      }
      /*
       * Default state file suffix width is 2.
       * This allows 100 files to be written.
       * For more files we need to increase the suffix width.
       */
      if( ( num_files > 100 ) && ( num_files <= 1000 ) ) {
         width = 3;
         mc_suffix_width( fam_id, width );
      } else if( ( num_files > 1000 ) && ( num_files <= 10000 ) ) {
         width = 4;
         mc_suffix_width( fam_id, width );
      }
   }

   srec_id = fam->qty_srecs -1;
   p_sr = fam->srecs[srec_id];
   subrec_qty = p_sr->qty_subrecs;

   st_time = out_db->state_times[state_num];

   p_file_suffix = ST_FILE_SUFFIX( fam, fam->cur_st_index );
   p_file_state_index = fam->file_st_qty - 1;

   rval = (Return_value)mc_new_state( fam_id, srec_id, st_time,
                                      &p_file_suffix, &p_file_state_index );

   iprec = 1;
   offset = 0;
   round  = 0;
   p_c = (char *)(out_db->result + offset);
   for( j = 0; j < subrec_qty; j++ ) {
      psubrec = p_sr->subrecs[j];
      offset= psubrec->offset/ EXT_SIZE(fam,M_FLOAT);
      if (  psubrec->organization == RESULT_ORDERED ) {
         for( k = 0; k < psubrec->qty_svars; k++ ) {
            num_type = *psubrec->svars[k]->data_type;
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

            atom_size = EXT_SIZE(fam, num_type );
            p_c = (char *)(out_db->result + offset) ;
            state_out = (void *)p_c;
            result_size =  psubrec->lump_atoms[k];
            rval = (Return_value)mc_wrt_stream( fam_id, num_type,
                                                result_size, state_out );
            offset += result_size*iprec;
         }
      } else {
         /*  Note:  If a subrecord is organized as m_object_ordered,
          *  the data types of all state variables bound to the
          *  subrecord definition must be the same.
          */
         if ( !strcmp( psubrec->name, "node") ) {
            num_type = 1;
         }

         num_type = *psubrec->svars[0]->data_type;
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

         atom_size = EXT_SIZE(fam, num_type );

         p_c = (char *)(out_db->result + offset) ;
         state_out = (void *)p_c;
         result_size = psubrec->lump_atoms[0] * psubrec->mo_qty;
         rval = (Return_value)mc_wrt_stream( fam_id, num_type,
                                             result_size, state_out );         
      }
   }

   return rval;
}

