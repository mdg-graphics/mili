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
   float st_time;
   int qty_states;
   int states_per_file;
   int num_files;
   int width;
   int p_file_suffix;
   int p_file_state_index;
   int srec_id;
   LONGLONG result_size;
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

   srec_id = fam->qty_srecs -1;
   
   st_time = out_db->state_times[state_num-1];

   p_file_suffix = ST_FILE_SUFFIX( fam, fam->cur_st_index );
   
   p_file_state_index = fam->file_st_qty - 1;

   rval = (Return_value)mc_new_state( fam_id, srec_id, st_time,
                                      &p_file_suffix, &p_file_state_index );

   p_sr = fam->srecs[srec_id];
   
   result_size = p_sr->size;
   
   //mc_wrt_stream( Famid fam_id, int type, int qty, void *data )
   rval = mc_wrt_stream(fam_id, M_STRING, result_size, out_db->result);
   
   if(rval != OK)
   {
      fprintf(stderr, "Error writing state %d. Aborting combine.\n", state_num);
      mc_print_error( "write_state_data(...): ",rval);
      return rval;
   }
   
   rval = mc_end_state(fam_id, srec_id);
   
   return rval;
   
   
}

