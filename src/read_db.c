
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
#include "mili_enum.h"


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
read_state_data( int state_num, Mili_analysis *in_db)
{
   Famid fam_id;
   Mili_family *fam;
   Srec *p_sr;
   int srec_id;
   int st_index;
   int idx;
   Return_value rval = OK;
   LONGLONG offset;

   fam_id = in_db->db_ident;
   fam = fam_list[fam_id];

   if( fam->qty_srecs == 0 ) {
      return rval;
   }
   
   if(fam->state_qty < state_num)
   {
      return INVALID_STATE;
   }
   
   st_index = state_num-1;
   
   rval = state_file_open(fam,fam->state_map[st_index].file,fam->access_mode);
   
   if(rval != OK)
   {
      mc_print_error("Error opening state file: ", rval);
   }
   offset = (fam->state_map[st_index].offset)+(LONGLONG)8;
   idx = fseek(fam->cur_st_file, offset, SEEK_SET);
   
   if(idx != 0)
   {
      return INVALID_STATE;
   }
   
   srec_id = fam->qty_srecs - 1;
   p_sr = fam->srecs[srec_id];
   
   if(in_db->result == NULL)
   {
      in_db->result = (float *)NEW_N( char, p_sr->size, "Results" );
   }
   idx = fread( (void *) in_db->result, 1, p_sr->size, fam->cur_st_file);
   
   
   if(idx == p_sr->size)
   {
      return OK;
   }
   else
   {
      return INVALID_STATE;
   }
}

