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
 
 * Parameter management routines.
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
 */

#include <string.h>
#include "mili_internal.h"
#include "eprtf.h"

#define ACELL_SPACE 3

static Return_value dump_param_array( LONGLONG (*read_funcs[])(), FILE *p_f,
                                      Dir_entry dir_ent, char *dir_string,
                                      Dump_control *p_dc,
                                      int head_indent, int body_indent );

static Return_value read_param_array_len( Mili_family *fam, Param_ref *p_pr,
      int *param_array_type, int *param_array_len );


extern
char *dtype_names[QTY_PD_ENTRY_TYPES + 1] =
{
   NULL,
   "String",
   "Float",
   "Float32",
   "Float64",
   "Integer",
   "Integer32",
   "Integer64"
};

extern Bool_type milisilo;

/*****************************************************************
 * TAG( fam_list )
 *
 * Dynamically allocated array of pointers to all currently open
 * MILI families.
 */
extern Mili_family **fam_list;

/*****************************************************************
 * TAG( fam_qty )
 *
 * The actual number of all currently open MILI families.
 */
extern int fam_qty;

/*****************************************************************
 * TAG( fam_array_length )
 *
 * The length of fam_list.
 */
extern int fam_array_length;


/*****************************************************************
 * TAG( mc_read_scalar ) PUBLIC
 *
 * Read a named scalar parameter from the referenced family.
 */
Return_value
mc_read_scalar( Famid fam_id, char *name, void *p_value )
{
   Mili_family *fam;
   Htable_entry *phte;
   Return_value rval;

   fam = fam_list[fam_id];

   if ( name == NULL || *name == '\0' )
   {
      return INVALID_NAME;
   }

   if ( fam->param_table == NULL )
   {
      return ENTRY_NOT_FOUND;
   }

   rval = htable_search( fam->param_table, name, FIND_ENTRY, &phte );
   if ( phte == NULL )
   {
      return rval;
   }

   return read_scalar( fam, (Param_ref *) phte->data, p_value );
}


/*****************************************************************
 * TAG( read_scalar ) PRIVATE
 *
 * Read a scalar numeric parameter from the referenced family.
 */
Return_value
read_scalar( Mili_family *fam, Param_ref *p_pr, void *p_value )
{
   int file, entry_idx;
   LONGLONG offset;
   Return_value rval;
   LONGLONG nitems;
   LONGLONG *entry;
   LONGLONG numtype;

   /* Get directory entry. */
   file = p_pr->file_index;
   entry_idx = p_pr->entry_index;

   /* Get file offset and length of data. */
   entry = fam->directory[file].dir_entries[entry_idx];
   offset = entry[OFFSET_IDX];
   numtype = entry[MODIFIER1_IDX];

   rval = non_state_file_open( fam, file, fam->access_mode );
   if ( rval != OK )
   {
      return rval;
   }

   rval = non_state_file_seek( fam, offset );
   if ( rval != OK )
   {
      return rval;
   }

   /* Read the data. */
   nitems = fam->read_funcs[numtype]( fam->cur_file, p_value, 1 );

   return ( nitems == 1 ) ? OK : SHORT_READ;
}


/*****************************************************************
 * TAG( mc_wrt_scalar ) PUBLIC
 *
 * Write a scalar numeric parameter into the referenced family.
 */
Return_value
mc_wrt_scalar( Famid fam_id, int type, char *name, void *p_value )
{
   Mili_family *fam;
   Return_value status;

#ifdef SILOENABLED
   if (milisilo)
   {
      status = mc_silo_wrt_scalar( fam_id, type, name, p_value );
      return ( status );
   }
#endif

   fam = fam_list[fam_id];

   if ( name == NULL || *name == '\0' )
   {
      return INVALID_NAME;
   }
   else if ( fam->access_mode == 'r' )
   {
      return BAD_ACCESS_TYPE;
   }

   if ( fam->param_table == NULL )
   {
      fam->param_table = htable_create( DEFAULT_HASH_TABLE_SIZE );
      if (fam->param_table == NULL)
      {
         return ALLOC_FAILED;
      }
   }   
   status = write_scalar( fam, type, name, p_value, APPLICATION_PARAM );
   return status;
}


/*****************************************************************
 * TAG( write_scalar ) PRIVATE
 *
 * Write a scalar numeric parameter into the referenced family.
 */
Return_value
write_scalar( Mili_family *fam, int type, char *name, void *data,
              Dir_entry_type etype )
{
   LONGLONG outbytes;
   Return_value rval;
   Param_ref *ppr;
   int fidx;
   Htable_entry *phte;
   LONGLONG *entry;
   int prev_index;
   LONGLONG write_ct;

   /* See if entry already exists. */
   rval = htable_search( fam->param_table, name, ENTER_MERGE, &phte );
   if (phte == NULL)
   {
      return rval;
   }
   if ( rval == OK )
   {
      /* Ensure file is open and positioned for initial write. */
      rval = prep_for_new_data( fam, NON_STATE_DATA );
      if ( rval == OK )
      {
         /* Add entry into directory. */
         outbytes = EXT_SIZE( fam, type );
         rval = add_dir_entry( fam, etype, type, SCALAR, 1, &name,
                               fam->next_free_byte, outbytes );

         if ( rval == OK )
         {
            /* Now create entry for the param table. */
            ppr = NEW( Param_ref, "Param table entry - scalar" );
            if (ppr == NULL)
            {
               return ALLOC_FAILED;
            }
            fidx = fam->cur_index;
            ppr->file_index = fidx;
            ppr->entry_index = fam->directory[fidx].qty_entries - 1;
            phte->data = (void *) ppr;

            /* Write the scalar. */
            write_ct = (fam->write_funcs[type])( fam->cur_file, data, 1 );
            if (write_ct != 1)
            {
               rval = SHORT_WRITE;
            }
            if(strncmp(name,"nproc", strlen("nproc"))==0)
            {
               fam->num_procs = *(int*)data;
            }

            fam->next_free_byte += outbytes;
         }
      }
   }
   else if (rval == ENTRY_EXISTS)
   {
      /* Found name match.  OK to re-write if numeric type is same. */
      ppr = (Param_ref *) phte->data;
      entry = fam->directory[ppr->file_index].dir_entries[ppr->entry_index];

      if ( entry[MODIFIER1_IDX] == type )
      {
         rval = OK;

         /* Save id of any extant open file for use below. */
         prev_index = fam->cur_index;

         /* Open in append mode to avoid truncating file. */
         rval = non_state_file_open( fam, ppr->file_index, 'a' );
         if (rval != OK)
         {
            return rval;
         }
         rval = non_state_file_seek( fam, entry[OFFSET_IDX] );
         if (rval != OK)
         {
            return rval;
         }

         write_ct = (fam->write_funcs[type])( fam->cur_file, data, (LONGLONG) 1 );
         if (write_ct != 1)
         {
            rval = SHORT_WRITE;
         }
         if(strncmp(name,"nproc", strlen("nproc"))==0)
         {
             fam->num_procs = *(int*)data;
         }
         /*
          * This was a re-write.  If the destination file wasn't already
          * open, close it to ensure changes are flushed to disk.
          */
         if ( prev_index != fam->cur_index )
         {
            rval = non_state_file_close( fam );
         }
         else if ( fam->cur_file != NULL )
         {
            /* Leave file pointer at end of file. */
            if (fseek( fam->cur_file, 0, SEEK_END ) != 0)
            {
               rval = SEEK_FAILED;
            }
         }
      }
      else
      {
         /* Type mismatch; re-write error. */
         rval = NUM_TYPE_MISMATCH;
      }
   }
   
   return rval;
}


/*******************************************************************
* mc_set_global_class_count(..)
* Method to specifically set the global counts for a speific class
* (i.e. shell, node, brick, ml ....)
*/


Return_value
mc_set_global_class_count(Famid fam_id, char* name, int global_count)
{
   Mili_family *fam;
   char *global_prefix = "GLOBAL_COUNT-";
   char global_name[128];
   
   fam = fam_list[fam_id];
   
   global_name[0] = '\0';
   
   strncat(global_name, global_prefix, 13);
   strcat(global_name, name);
   
   return write_scalar(fam,M_INT,(char*)global_name,(void*) &global_count, TI_PARAM);
   
}

Return_value
mc_get_global_class_count(Famid fam_id, char* name, int *global_count)
{
   char *global_prefix = "GLOBAL_COUNT-";
   char global_name[128];
   
   global_name[0] = '\0';
   
   strncat(global_name, global_prefix, 13);
   strcat(global_name, name);
   
   return mc_read_scalar(fam_id,(char*)global_name,(void*) global_count);
   
}


Return_value
mc_get_local_class_count(Famid fam_id, char* name, int *local_count)
{
   char *prefix = "LOCAL_COUNT-";
   char local_name[128];
   
   *local_count = 0;
   
   local_name[0] = '\0';
   
   strncat(local_name, prefix, 13);
   strcat(local_name, name);
   
   return mc_read_scalar(fam_id,(char*)local_name,(void*) local_count);
   
}
/*****************************************************************
 * TAG( mc_read_string ) PUBLIC
 *
 * Read a string parameter from the referenced family.
 */
Return_value
mc_read_string( Famid fam_id, char *name, char *p_value )
{
   Mili_family *fam;
   Htable_entry *phte;
   Return_value status;
   
#ifdef SILOENABLED
   if (milisilo)
   {
      status = mc_silo_read_string( fam_id, name, p_value );
      return ( status );
   }
#endif

   fam = fam_list[fam_id];

   p_value[0] = '\0'; /* Set default to empty string */

   if ( name == NULL || *name == '\0' )
   {
      return INVALID_NAME;
   }

   if ( fam->param_table == NULL )
   {
      return ENTRY_NOT_FOUND;
   }

   status = htable_search( fam->param_table, name, FIND_ENTRY, &phte );
   if ( phte == NULL )
   {
      return status;
   }

    status = mili_read_string( fam, (Param_ref *) phte->data, p_value );
    return status;
}


/*****************************************************************
 * TAG( mili_read_string ) PRIVATE
 *
 * Read a string parameter from the referenced family.
 */
Return_value
mili_read_string( Mili_family *fam, Param_ref *p_pr, char *p_value )
{
   int file, entry_idx;
   LONGLONG offset;
   Return_value rval;
   LONGLONG nitems, length;
   LONGLONG *entry;

   /* Get directory entry. */
   file = p_pr->file_index;
   entry_idx = p_pr->entry_index;

   /* Get file offset and length of data. */
   entry = fam->directory[file].dir_entries[entry_idx];

   if ( entry[MODIFIER1_IDX] != M_STRING )
   {
      return NOT_STRING_TYPE;
   }

   offset = entry[OFFSET_IDX];
   length = (LONGLONG)entry[LENGTH_IDX];

   rval = non_state_file_open( fam, file, fam->access_mode );
   if ( rval != OK )
   {
      return rval;
   }

   rval = non_state_file_seek( fam, offset );
   if ( rval != OK )
   {
      return rval;
   }

   /* Read the data. */
   nitems = fam->read_funcs[M_STRING]( fam->cur_file, p_value, length );

   return ( nitems == length ) ? OK : SHORT_READ;
}


/*****************************************************************
 * TAG( mc_wrt_string ) PUBLIC
 *
 * Write a string parameter into the referenced family.
 */
Return_value
mc_wrt_string( Famid fam_id, char *name, char *value )
{
   Mili_family *fam;
   Return_value status;

#ifdef SILOENABLED
   if (milisilo)
   {
      status = mc_silo_wrt_string( fam_id, name, value );
      return ( status );
   }
#endif

   if ( name == NULL || *name == '\0' )
   {
      return INVALID_NAME;
   }

   fam = fam_list[fam_id];

   if ( fam->param_table == NULL )
   {
      fam->param_table = htable_create( DEFAULT_HASH_TABLE_SIZE );
      if (fam->param_table == NULL)
      {
         return ALLOC_FAILED;
      }
   }
   status = write_string( fam, name, value, APPLICATION_PARAM );
   return status;
}


/*****************************************************************
 * TAG( write_string ) PRIVATE
 *
 * Write a string parameter into the referenced family.
 */
Return_value
write_string( Mili_family *fam, char *name, char *value, Dir_entry_type etype )
{
   LONGLONG c_qty, s_len;
   char *c_buf;
   char *pdest, *psrc;
   Return_value rval;
   Param_ref *ppr;
   int fidx;
   Htable_entry *phte;
   LONGLONG *entry;
   int prev_index;
   int write_ct;

   /* Evaluate how many characters required for string. */
   s_len = strlen( value ) + 1;
   c_qty = ROUND_UP_INT( s_len, 8 );

   c_buf = value;

   rval = htable_search( fam->param_table, name, ENTER_MERGE, &phte );
   if (phte == NULL)
   {
      return rval;
   }
   if ( rval == OK )
   {
      /* Ensure file is open and positioned. */
      rval = prep_for_new_data( fam, NON_STATE_DATA );
      if ( rval == OK )
      {
         if ( s_len != c_qty )
         {
            /* Allocate output buffer. */
            c_buf = NEW_N( char, c_qty, "App string char buf" );
            if (c_qty > 0 && c_buf == NULL)
            {
               return ALLOC_FAILED;
            }

            /* Copy the string into the output buffer. */
            for ( psrc = value, pdest = c_buf; (*pdest++ = *psrc++); );
         }

         /* Add directory entry. Modifier is Mili data type. */
         rval = add_dir_entry( fam, etype, M_STRING, DONT_CARE, 1, &name,
                               fam->next_free_byte, c_qty );

         if ( rval == OK )
         {
            /* Now create entry for the param table. */
            ppr = NEW( Param_ref, "Param table entry - string" );
            if (ppr == NULL)
            {
               if ( c_buf != value )
               {
                  free( c_buf );
               }
               return ALLOC_FAILED;
            }
            fidx = fam->cur_index;
            ppr->file_index = fidx;
            ppr->entry_index = fam->directory[fidx].qty_entries - 1;
            phte->data = (void *) ppr;

            /* Write the string. */
            write_ct = fam->write_funcs[M_STRING]( fam->cur_file,
                                                   c_buf, c_qty );
            if (write_ct != c_qty)
            {
               if ( c_buf != value )
               {
                  free( c_buf );
               }
               return SHORT_WRITE;
            }

            fam->next_free_byte += c_qty;

            if ( c_buf != value )
            {
               free( c_buf );
            }
         }
      }
   }
   else if (rval == ENTRY_EXISTS)
   {
      /* Found name match.  OK to re-write if new length fits in old space. */
      ppr = (Param_ref *) phte->data;
      entry = fam->directory[ppr->file_index].dir_entries[ppr->entry_index];

      if ( (LONGLONG) entry[LENGTH_IDX] >= s_len )
      {
         /* Save id of any extant open file for use below. */
         prev_index = fam->cur_index;

         /* Open in append mode to avoid truncating file. */
         rval = non_state_file_open( fam, ppr->file_index, 'a' );
         if (rval != OK)
         {
            return rval;
         }
         rval = non_state_file_seek( fam, entry[OFFSET_IDX] );
         if (rval != OK)
         {
            return rval;
         }

         /* Re-write the string. */
         write_ct = fam->write_funcs[M_STRING]( fam->cur_file,
                                                c_buf, s_len );
         if (write_ct != s_len)
         {
            return SHORT_WRITE;
         }

         if ( prev_index != fam->cur_index )
         {
            rval = non_state_file_close( fam );
         }
         else if ( fam->cur_file != NULL )
         {
            /* Leave file pointer at end of file. */
            if (fseek( fam->cur_file, 0, SEEK_END ) != 0)
            {
               rval = SEEK_FAILED;
            }
         }
      }
      else
      {
         /* Type mismatch; re-write error. */
         rval = STR_LEN_MISMATCH;
      }
   }

   return rval;
}

/*****************************************************************
 * TAG( mc_read_param_array ) PUBLIC
 *
 * Read a parameter array from the referenced family.
 */
Return_value
mc_read_param_array( Famid fam_id, char *name, void **p_data )
{
   Mili_family *fam;
   Htable_entry *phte;
   Return_value status;

#ifdef SILOENABLED
   if (milisilo)
   {
      status = mc_silo_read_param_array( fam_id, name, p_data );
      return ( status );
   }
#endif

   fam = fam_list[fam_id];

   if ( name == NULL || *name == '\0' )
   {
      return INVALID_NAME;
   }

   if ( fam->param_table == NULL )
   {
      return ENTRY_NOT_FOUND;
   }

   status = htable_search( fam->param_table, name, FIND_ENTRY, &phte );
   if ( phte == NULL )
   {
      return status;
   }

   return read_param_array( fam, (Param_ref *) phte->data, p_data );
}

/*****************************************************************
 * TAG( mc_read_param_array_len ) PUBLIC
 *
 * Read the length of a parameter array from the referenced family.
 */
Return_value
mc_read_param_array_len( Famid fam_id, char *name, int *param_array_type, int *param_array_len )
{
   Mili_family *fam;
   Htable_entry *phte;
   Return_value rval;
   int value =0;

#ifdef SILOENABLED
   if (milisilo)
   {
      return mc_silo_read_param_array_len( fam_id, name );
   }
#endif

   fam = fam_list[fam_id];

   if ( name == NULL || *name == '\0' )
   {
      return INVALID_NAME;
   }

   if ( fam->param_table == NULL )
   {
      return ENTRY_NOT_FOUND;
   }

   rval = htable_search( fam->param_table, name, FIND_ENTRY, &phte );
   if ( phte == NULL )
   {
      return rval;
   }
   while (phte != NULL)
   {
      if(strcmp(name,phte->key) == 0)
      {
         rval = read_param_array_len(fam, (Param_ref *)phte->data, param_array_type, &value);
         if(rval == OK)
         {
            *param_array_len += value;
         }else 
         {
            break;
         }
      }
      phte = phte->next;
   }
   return rval;
}


/*****************************************************************
 * TAG( read_param_array ) PRIVATE
 *
 * Read a parameter array from the referenced family.
 */
Return_value
read_param_array( Mili_family *fam, Param_ref *p_pr, void **p_data )
{
   int file, entry_idx;
   int cell_qty;
   int i;
   LONGLONG offset;
   LONGLONG nitems, length;
   int num_type;
   LONGLONG *dir_ent;
   int *idata;
   LONGLONG *lidata;
   float *fdata;
   double *ddata;
   Return_value rval;


   /* Get directory entry. */
   file = p_pr->file_index;
   entry_idx = p_pr->entry_index;
   dir_ent = fam->directory[file].dir_entries[entry_idx];

   /* Get file offset and length of data. */
   offset   = dir_ent[OFFSET_IDX];
   length   = (LONGLONG)dir_ent[LENGTH_IDX];
   num_type = dir_ent[MODIFIER1_IDX];

   rval = non_state_file_open( fam, file, fam->access_mode );
   if ( rval != OK )
   {
      return rval;
   }

   rval = non_state_file_seek( fam, offset );
   
   if ( rval != OK )
   {
      return rval;
   }

   /* Read the data. */

   /* Read the array rank.*/
   nitems = (fam->read_funcs[M_INT])( fam->cur_file, &p_pr->rank, 1 );

   if ( nitems != 1 )
   {
      return SHORT_READ;
   }

   p_pr->dims = NEW_N( int, p_pr->rank, "Array dimensions bufr" );
   if (p_pr->rank > 0 && p_pr->dims == NULL)
   {
      return ALLOC_FAILED;
   }

   nitems = (fam->read_funcs[M_INT])( fam->cur_file, p_pr->dims,
                                      (LONGLONG) p_pr->rank );
   if ( nitems != (LONGLONG)p_pr-> rank )
   {
      free( p_pr->dims );
      return SHORT_READ;
   }

   /* Calculate array size */
   for ( i = 0, cell_qty = 1; i < p_pr->rank; i++ )
   {
      cell_qty *= p_pr->dims[i];
   }

   switch( num_type )
   {
      case M_INT:
      case M_INT4:
         idata = NEW_N( int, cell_qty, "Param array bufr" );
         if (cell_qty > 0 && idata == NULL)
         {
            free(p_pr->dims);
            return ALLOC_FAILED;
         }
         p_data[0] = (void *)idata;
         nitems = (fam->read_funcs[M_INT])( fam->cur_file, idata,
                                            (LONGLONG) cell_qty );
         length/=4;
         break;

      case M_INT8:
         lidata = NEW_N( LONGLONG, cell_qty, "Param array bufr" );
         if (cell_qty > 0 && lidata == NULL)
         {
            free(p_pr->dims);
            return ALLOC_FAILED;
         }
         p_data[0] = (void *)lidata;
         nitems = (fam->read_funcs[M_INT8])( fam->cur_file, lidata,
                                             (LONGLONG) cell_qty );
         length/=8;
         break;

      case M_FLOAT:
      case M_FLOAT4:
         fdata = NEW_N( float, cell_qty, "Param array bufr" );
         if (cell_qty > 0 && fdata == NULL)
         {
            free(p_pr->dims);
            return ALLOC_FAILED;
         }
         p_data[0] = (void *)fdata;
         nitems = (fam->read_funcs[M_FLOAT])( fam->cur_file, fdata,
                                              (LONGLONG) cell_qty );
         length=(length/4)-2;
         break;

      case M_FLOAT8:
         ddata = NEW_N( double, cell_qty, "Param array bufr" );
         if (cell_qty > 0 && ddata == NULL)
         {
            free(p_pr->dims);
            return ALLOC_FAILED;
         }
         p_data[0] = (void *)ddata;
         nitems = (fam->read_funcs[M_FLOAT8])( fam->cur_file, ddata,
                                               (LONGLONG) cell_qty );
         length/=8;
         break;

   }


   return ( nitems == cell_qty ) ? OK : SHORT_READ;
}

/*****************************************************************
 * TAG( read_param_array_len ) LOCAL
 *
 * Read the length of a  parameter array from the referenced family.
 */
static Return_value
read_param_array_len( Mili_family *fam, Param_ref *p_pr, int *param_array_type, int *param_array_len )
{
   int file, entry_idx;
   int i;
   LONGLONG offset;
   LONGLONG nitems;
   int num_type;
   LONGLONG *dir_ent;
   Return_value rval;


   /* Get directory entry. */
   file = p_pr->file_index;
   entry_idx = p_pr->entry_index;
   dir_ent = fam->directory[file].dir_entries[entry_idx];

   /* Get file offset and length of data. */
   offset   = dir_ent[OFFSET_IDX];
   num_type = dir_ent[MODIFIER1_IDX];
   *param_array_type = num_type;
   rval = non_state_file_open( fam, file, fam->access_mode );
   if ( rval != OK )
   {
      return rval;
   }

   rval = non_state_file_seek( fam, offset );
   if ( rval != OK )
   {
      return rval;
   }

   /* Read the data. */

   /* Read the array rank. */
   nitems = (fam->read_funcs[M_INT])( fam->cur_file, &p_pr->rank, 1 );

   if ( nitems != 1 )
   {
      return SHORT_READ;
   }

   p_pr->dims = NEW_N( int, p_pr->rank, "Array dimensions bufr" );
   if (p_pr->rank > 0 && p_pr->dims == NULL)
   {
      return ALLOC_FAILED;
   }

   nitems = (fam->read_funcs[M_INT])( fam->cur_file, p_pr->dims,
                                      (LONGLONG) p_pr->rank );
   if ( nitems != (LONGLONG)p_pr-> rank )
   {
      free( p_pr->dims );
      return SHORT_READ;
   }

   /* Calculate array size */
   for ( i = 0, *param_array_len = 1; i < p_pr->rank; i++ )
   {
      *param_array_len *= p_pr->dims[i];
   }

   return rval;
}


/*****************************************************************
 * TAG( mc_wrt_array ) PUBLIC
 *
 * Write an array parameter into the referenced family.
 */
Return_value
mc_wrt_array( Famid fam_id, int type, char *name, int order,
              int *dimensions, void *data )
{
   Mili_family *fam;
   Return_value status;

#ifdef SILOENABLED
   if (milisilo)
   {
      status = mc_silo_wrt_array( fam_id, type, name, order,
                                  dimensions, data );
      return ( status );
   }
#endif

   if ( name == NULL || *name == '\0' )
   {
      return INVALID_NAME;
   }

   fam = fam_list[fam_id];

   if ( fam->param_table == NULL )
   {
      fam->param_table = htable_create( DEFAULT_HASH_TABLE_SIZE );
      if (fam->param_table == NULL)
      {
         return ALLOC_FAILED;
      }
   }

   status =write_array( fam, type, name, order, dimensions, data,
                       APPLICATION_PARAM );
   return status;
}


/*****************************************************************
 * TAG( write_array ) PRIVATE
 *
 * Write a parameter array into the referenced family.
 */
Return_value
write_array( Mili_family *fam, int type, char *name, int order,
             int *dimensions, void *data, Dir_entry_type etype )
{
   int i;
   int atoms;
   int *i_buf;
   LONGLONG outbytes;
   Return_value rval;
   Param_ref *ppr;
   int fidx;
   LONGLONG write_ct;

   /* Ensure file is open and positioned. */
   if ( (rval = prep_for_new_data( fam, NON_STATE_DATA )) != OK )
   {
      return rval;
   }

   /* Calc number of entries in array. */
   for ( i = 0, atoms = 1; i < order; i++ )
   {
      atoms *= dimensions[i];
   }

   /* Load integer descriptors into an output buffer. */
   i_buf = NEW_N( int, order + 1, "App array descr buf" );
   if (order + 1 > 0 && i_buf == NULL)
   {
      return ALLOC_FAILED;
   }
   i_buf[0] = order;
   for ( i = 0; i < order; i++ )
   {
      i_buf[i + 1] = dimensions[i];
   }

   /* Add entry into directory. */
   outbytes = ((order + 1) * EXT_SIZE( fam, M_INT ))
              + atoms * EXT_SIZE( fam, type );
   rval = add_dir_entry( fam, etype, type, ARRAY, 1, &name,
                         fam->next_free_byte, outbytes );
   if ( rval != OK )
   {
      free(i_buf);
      return rval;
   }

   /* Write it out... */
   write_ct = fam->write_funcs[M_INT]( fam->cur_file, i_buf, order + 1 );
   if (write_ct != (order + 1))
   {
      free(i_buf);
      return SHORT_WRITE;
   }
   write_ct = (fam->write_funcs[type])( fam->cur_file, data, (LONGLONG) atoms );
   if (write_ct != atoms)
   {
      free(i_buf);
      return SHORT_WRITE;
   }

   fam->next_free_byte += outbytes;

   free( i_buf );

   /* Now create entry for the param table. */
   ppr = NEW( Param_ref, "Param table entry - array" );
   if (ppr == NULL)
   {
      return ALLOC_FAILED;
   }
   fidx = fam->cur_index;
   ppr->file_index = fidx;
   ppr->entry_index = fam->directory[fidx].qty_entries - 1;
   rval = htable_add_entry_data( fam->param_table, name, ENTER_ALWAYS, ppr );
   if (rval != OK)
   {
      free(ppr);
      return rval;
   }

   return OK;
}

/*****************************************************************
 * TAG( mc_get_app_parameter_count ) PUBLIC
 *
 * Get the number of MILI_PARAMS.
 */
int
mc_get_app_parameter_count(Famid fam_id)
{
	Htable_entry *entry;
   Param_ref *pref;
	int i;
	int count = 0;
	int num_copied = 0;
	int status;
	char **list=NULL;
   int dir_index;
	File_dir *p_fd;
	Mili_family *fam;
	fam = fam_list[fam_id];
	
	count = htable_search_wildcard( fam->param_table,0,FALSE,
	                                "*", "NULL", "NULL",list);
   list=(char**) malloc( count*sizeof(char *));
   count = htable_search_wildcard(fam->param_table,0, FALSE,
		                               "*", "NULL","NULL",list );
	for ( i = 0; i < count; i++ )
   {
		if(strstr(list[i],"Labels[") != NULL ||
			strstr(list[i],"Labels-ElemIds[") != NULL)
		{
			continue;
		}
		status = htable_search( fam->param_table, list[i], 
                              FIND_ENTRY, &entry );
		if(status != OK)
        {
           continue;
        }
        pref = (Param_ref*) entry->data;
		dir_index = pref->entry_index;
		p_fd= fam->directory + pref->file_index;
		if(p_fd->dir_entries[dir_index][TYPE_IDX] == APPLICATION_PARAM ||
		   p_fd->dir_entries[dir_index][TYPE_IDX] == TI_PARAM)
		{
			num_copied++;
		}
	}
   htable_delete_wildcard_list(count,list);
	free(list);
	return num_copied;
	
}

/*****************************************************************
 * TAG( mc_get_app_parameter_names ) PUBLIC
 *
 * Get the MILI_PARAMS names.
 */
void mc_get_app_parameter_names(Famid fam_id, char **names, int in_size)
{
	Htable_entry *entry;
   Param_ref *pref;
	int i;
	int count = 0;
	int num_copied = 0;
	File_dir *p_fd;
	int status;
	char **list=NULL;
   int dir_index;
	Mili_family *fam;
	fam = fam_list[fam_id];
	
	count = htable_search_wildcard( fam->param_table,0,FALSE,
	                                "*", "NULL", "NULL",list);
   list=(char**) malloc( count*sizeof(char *));
   count = htable_search_wildcard(fam->param_table,0, FALSE,
		                               "*", "NULL","NULL",list );
	for ( i = 0; i < count; i++ )
   {
		if(strstr(list[i],"Labels[") != NULL ||
			strstr(list[i],"Labels-ElemIds[") != NULL)
		{
			continue;
		}
		
		status = htable_search( fam->param_table, list[i], 
                              FIND_ENTRY, &entry );
		if(status != OK)
        {
           continue; 
        }
        pref = (Param_ref*) entry->data;
		dir_index = pref->entry_index;
		p_fd= fam->directory + pref->file_index;
		if(p_fd->dir_entries[dir_index][TYPE_IDX] == APPLICATION_PARAM ||
		   p_fd->dir_entries[dir_index][TYPE_IDX] == TI_PARAM)
		{
			names[num_copied]= NEW_N(char, strlen(list[i])+1,"Allocating Name in get_app_parameter_names");      
			strcpy(names[num_copied],list[i]);
			num_copied++;
		}
	}
   htable_delete_wildcard_list(count,list);
	free(list);
}

/*****************************************************************
 * TAG( dump_param ) PRIVATE
 *
 * Dump param entries from file to stdout.
 */
Return_value
dump_param( Mili_family *fam, FILE *p_f, Dir_entry dir_ent,
            char **dir_strings, Dump_control *p_dc,
            int head_indent, int body_indent )
{
   LONGLONG offset;
   int outlen;
   int status;
   LONGLONG nitems, length;
   int num_type;
   int i;
   char *cbuf;
   char *lead_char;
   char *param_name;
   int int_param;
   LONGLONG long_int_param;
   float float_param;
   double double_param;
   LONGLONG (*rfunc)();
   int hi, bi;
   Return_value rval;

   offset = dir_ent[OFFSET_IDX];

   status = fseek( p_f, (long)offset, SEEK_SET );
   if ( status != 0 )
   {
      return SEEK_FAILED;
   }

   /* If array type, process elsewhere. */
   if ( dir_ent[MODIFIER2_IDX] == ARRAY )
   {
      return dump_param_array( fam->read_funcs, p_f, dir_ent, dir_strings[0],
                               p_dc, head_indent, body_indent );
   }

   num_type = dir_ent[MODIFIER1_IDX];
   rfunc = fam->read_funcs[num_type];
   param_name = ( dir_ent[TYPE_IDX] == MILI_PARAM )
                ? "MILI PARAM" : "APP PARAM";
   cbuf = NULL;
   hi = head_indent;
   bi = body_indent;

   switch( num_type )
   {
      case M_STRING:
         cbuf = NEW_N( char, (LONGLONG)dir_ent[LENGTH_IDX], "Dump char input buf" );
         if (dir_ent[LENGTH_IDX] > 0 && cbuf == NULL)
         {
            return ALLOC_FAILED;
         }
         length = (LONGLONG)dir_ent[LENGTH_IDX];
         nitems = rfunc( p_f, cbuf, length );
         if ( nitems != length )
         {
            free( cbuf );
            return SHORT_READ;
         }

         if ( !p_dc->include_external &&
               ( strcmp( dir_strings[0], "host name" ) == 0 ||
                 strcmp( dir_strings[0], "host type" ) == 0 ||
                 strcmp( dir_strings[0], "date" ) == 0 ) )
         {
            break;
         }

         if ( p_dc->formatted )
         {
            rval = eprintf( &outlen, "%*t%s:%*t%s \"%s\";%*t@%ld;",
                            hi + 1, param_name,
                            hi + 14, dtype_names[num_type],
                            dir_strings[0], hi + 45, offset );
            if (rval != OK)
            {
               free(cbuf);
               return rval;
            }

            if ( length <= 20 )
            {
               rval = eprintf( &outlen, "%*t=\"%s\"\n",
                               hi + 57 - outlen, cbuf );
               if (rval != OK)
               {
                  free(cbuf);
                  return rval;
               }
            }
            else
            {
               lead_char = "\"";
               for ( i = 0; cbuf[i]; i++ )
               {
                  if ( i % 60 == 0 )
                  {
                     rval = eprintf( &outlen, "\n%*t%s",
                                     bi + 2, lead_char );
                     if (rval != OK)
                     {
                        free(cbuf);
                        return rval;
                     }
                     lead_char = " ";
                  }
                  putchar( (int) cbuf[i] );
               }
               printf( "\"\n" );
            }
         }
         break;

      case M_INT:
      case M_INT4:
         nitems = rfunc( p_f, &int_param, (LONGLONG) 1 );
         if ( nitems != 1 )
         {
            if ( cbuf != NULL )
            {
               free( cbuf );
            }
            return SHORT_READ;
         }

         if ( p_dc->formatted )
         {
            rval = eprintf( &outlen, "%*t%s:%*t%s \"%s\";%*t@%ld;%*t=%d\n",
                            hi + 1, param_name,
                            hi + 14, dtype_names[num_type], dir_strings[0],
                            hi + 45, offset,
                            hi + 57, int_param );
            if (rval != OK)
            {
               return rval;
            }
         }
         break;

      case M_INT8:
         nitems = rfunc( p_f, &long_int_param, (LONGLONG) 1 );
         if ( nitems != 1 )
         {
            if ( cbuf != NULL )
            {
               free( cbuf );
            }
            return SHORT_READ;
         }

         if ( p_dc->formatted )
         {
            rval = eprintf( &outlen,
                            "%*t%s:%*t%s \"%s\";%*t@%ld;%*t=%ld\n",
                            hi + 1, param_name,
                            hi + 14, dtype_names[num_type], dir_strings[0],
                            hi + 45, offset,
                            hi + 57, long_int_param );
            if (rval != OK)
            {
               return rval;
            }
         }
         break;

      case M_FLOAT:
      case M_FLOAT4:
         nitems = rfunc( p_f, &float_param, (LONGLONG) 1 );
         if ( nitems != 1 )
         {
            if ( cbuf != NULL )
            {
               free( cbuf );
            }
            return SHORT_READ;
         }
         double_param = float_param;

         if ( p_dc->formatted )
         {
            rval = eprintf( &outlen,
                            "%*t%s:%*t%s \"%s\";%*t@%ld;%*t=%+13.6e\n",
                            hi + 1, param_name,
                            hi + 14, dtype_names[num_type], dir_strings[0],
                            hi + 45, offset,
                            hi + 57, double_param );
            if (rval != OK)
            {
               return rval;
            }
         }
         break;

      case M_FLOAT8:
         nitems = rfunc( p_f, &double_param, (LONGLONG) 1 );
         if ( nitems != 1 )
         {
            if ( cbuf != NULL )
            {
               free( cbuf );
            }
            return SHORT_READ;
         }

         if ( p_dc->formatted )
         {
            rval = eprintf( &outlen, "%*t%s:%*t%s \"%s\";%*t@%ld;%*t=%+19.12e\n",
                            hi + 1, param_name,
                            hi + 14, dtype_names[num_type], dir_strings[0],
                            hi + 45, offset,
                            hi + 57, double_param );
            if (rval != OK)
            {
               return rval;
            }
         }
         break;
   }

   if ( cbuf )
   {
      free( cbuf );
   }

   return OK;
}


/*****************************************************************
 * TAG( dump_param_array ) LOCAL
 *
 * Dump param array entry from file to stdout.
 */
static Return_value
dump_param_array( LONGLONG (*read_funcs[])(), FILE *p_f, Dir_entry dir_ent,
                  char *dir_string, Dump_control *p_dc,
                  int head_indent, int body_indent )
{
   int rank, cell_qty;
   int i;
   LONGLONG offset;
   int *dims;
   int *idata;
   LONGLONG *lidata;
   float *fdata;
   double *ddata;
   LONGLONG nitems;
   LONGLONG (*rd_i_func)();
   char *param_name;
   int num_type;
   char handle[64];
   char *p_tail;
   char *f_fmt = "%*t%.6g%s%.6g%s%.6g ... %.6g%s%.6g%s%.6g";
   char *d_fmt = "%*t%.12lg%s%.12lg%s%.12lg ... %.12lg%s%.12lg%s%.12lg";
   int hi;
   int off;
   char cell_sp[ACELL_SPACE];
   int outlen;
   Return_value rval;

   num_type = dir_ent[MODIFIER1_IDX];
   offset = dir_ent[OFFSET_IDX];
   param_name = ( dir_ent[TYPE_IDX] == MILI_PARAM )
                ? "MILI PARAM" : "APP PARAM";
   hi = head_indent;

   rd_i_func = read_funcs[M_INT];

   /* Read the array rank. */
   nitems = rd_i_func( p_f, &rank, 1 );

   if ( nitems != 1 )
   {
      return SHORT_READ;
   }

   dims = NEW_N( int, rank, "Array dimensions bufr" );
   if (rank > 0 && dims == NULL)
   {
      return ALLOC_FAILED;
   }

   nitems = rd_i_func( p_f, dims, (LONGLONG) rank );
   if ( nitems != (LONGLONG) rank )
   {
      free( dims );
      return SHORT_READ;
   }

   /* Initialize string of spaces. */
   for ( i = 0; i < ACELL_SPACE - 1; i++ )
   {
      cell_sp[i] = ' ';
   }
   cell_sp[ACELL_SPACE - 1] = '\0';

   /* Calculate array size and build array declaration string. */
   sprintf( handle, "%s", dir_string);
   p_tail = handle + strlen( dir_string );
   for ( i = 0, cell_qty = 1; i < rank; i++ )
   {
      cell_qty *= dims[i];

      sprintf( p_tail, "[%d]", dims[i] );
      while ( *(++p_tail) );
   }

   switch( num_type )
   {
      case M_INT:
      case M_INT4:
         idata = NEW_N( int, cell_qty, "Param array bufr" );
         if (cell_qty > 0 && idata == NULL)
         {
            return ALLOC_FAILED;
         }
         nitems = (read_funcs[M_INT])( p_f, idata, (LONGLONG) cell_qty );
         if (nitems != cell_qty)
         {
            free(idata);
            return SHORT_READ;
         }

         if ( p_dc->formatted )
         {
            rval = eprintf( &outlen, "%*t%s:%*t%s \"%s\";%*t@%ld;\n",
                            hi + 1, param_name,
                            hi + 14, dtype_names[num_type], handle,
                            hi + 45, offset );
            if (rval != OK)
            {
               free(idata);
               return rval;
            }

            if ( cell_qty < 7 )
            {
               off = body_indent + 1;
               for ( i = 0; i < cell_qty; i++ )
               {
                  rval = eprintf( &outlen, "%*t%d", off, idata[i] );
                  if (rval != OK)
                  {
                     free(idata);
                     return rval;
                  }
                  off = ACELL_SPACE;
               }
            }
            else
            {
               rval = eprintf( &outlen,
                               "%*t%d%s%d%s%d ... %d%s%d%s%d",
                               body_indent + 1, idata[0],
                               cell_sp, idata[1],
                               cell_sp, idata[2],
                               idata[cell_qty - 3],
                               cell_sp, idata[cell_qty - 2],
                               cell_sp, idata[cell_qty - 1] );
               if (rval != OK)
               {
                  free(idata);
                  return rval;
               }
            }
            printf( "\n" );
         }
         break;

      case M_INT8:
         lidata = NEW_N( LONGLONG, cell_qty, "Param array bufr" );
         if (cell_qty > 0 && lidata == NULL)
         {
            return ALLOC_FAILED;
         }
         nitems = (read_funcs[M_INT8])( p_f, lidata, (LONGLONG) cell_qty );
         if (nitems != cell_qty)
         {
            free(lidata);
            return SHORT_READ;
         }

         if ( p_dc->formatted )
         {
            rval = eprintf( &outlen, "%*t%s:%*t%s \"%s\";%*t@%ld;\n",
                            hi + 1, param_name,
                            hi + 14, dtype_names[num_type], handle,
                            hi + 45, offset );
            if (rval != OK)
            {
               free(lidata);
               return rval;
            }

            if ( cell_qty < 7 )
            {
               off = body_indent + 1;
               for ( i = 0; i < cell_qty; i++ )
               {
                  rval = eprintf( &outlen, "%*t%ld", off, lidata[i] );
                  if (rval != OK)
                  {
                     free(lidata);
                     return rval;
                  }
                  off = ACELL_SPACE;
               }
            }
            else
            {
               rval = eprintf( &outlen,
                               "%*t%ld%s%ld%s%ld ... %ld%s%ld%s%ld",
                               body_indent + 1, lidata[0],
                               cell_sp, lidata[1],
                               cell_sp, lidata[2],
                               lidata[cell_qty - 3],
                               cell_sp, lidata[cell_qty - 2],
                               cell_sp, lidata[cell_qty - 1] );
               if (rval != OK)
               {
                  free(lidata);
                  return rval;
               }
            }
            printf( "\n" );
         }
         break;

      case M_FLOAT:
      case M_FLOAT4:
         fdata = NEW_N( float, cell_qty, "Param array bufr" );
         if (cell_qty > 0 && fdata == NULL)
         {
            return ALLOC_FAILED;
         }
         nitems = (read_funcs[M_FLOAT])( p_f, fdata, (LONGLONG) cell_qty );
         if (nitems != cell_qty)
         {
            free(fdata);
            return SHORT_READ;
         }

         if ( p_dc->formatted )
         {
            rval = eprintf( &outlen, "%*t%s:%*t%s \"%s\";%*t@%ld;\n",
                            hi + 1, param_name,
                            hi + 14, dtype_names[num_type], handle,
                            hi + 45, offset );
            if (rval != OK)
            {
               free(fdata);
               return rval;
            }

            if ( cell_qty < 7 )
            {
               off = body_indent + 1;
               for ( i = 0; i < cell_qty; i++ )
               {
                  rval = eprintf( &outlen, "%*t%13.6g", off, fdata[i] );
                  if (rval != OK)
                  {
                     free(fdata);
                     return rval;
                  }
                  off = ACELL_SPACE;
               }
            }
            else
            {
               rval = eprintf( &outlen, f_fmt,
                               body_indent + 1, (double) fdata[0],
                               cell_sp, (double) fdata[1],
                               cell_sp, (double) fdata[2],
                               (double) fdata[cell_qty - 3],
                               cell_sp, (double) fdata[cell_qty - 2],
                               cell_sp, (double) fdata[cell_qty - 1] );
               if (rval != OK)
               {
                  free(fdata);
                  return rval;
               }
            }
            printf( "\n" );
         }
         break;

      case M_FLOAT8:
         ddata = NEW_N( double, cell_qty, "Param array bufr" );
         if (cell_qty > 0 && ddata == NULL)
         {
            return ALLOC_FAILED;
         }
         nitems = (read_funcs[M_FLOAT8])( p_f, ddata, (LONGLONG) cell_qty );
         if (nitems != cell_qty)
         {
            free(ddata);
            return SHORT_READ;
         }

         if ( p_dc->formatted )
         {
            rval = eprintf( &outlen, "%*t%s:%*t%s \"%s\";%*t@%ld;\n",
                            hi + 1, param_name,
                            hi + 14, dtype_names[num_type], handle,
                            hi + 45, offset );
            if (rval != OK)
            {
               free(ddata);
               return rval;
            }

            if ( cell_qty < 7 )
            {
               off = body_indent + 1;
               for ( i = 0; i < cell_qty; i++ )
               {
                  rval = eprintf( &outlen, "%*t%13.6lg", off, ddata[i] );
                  if (rval != OK)
                  {
                     free(ddata);
                     return rval;
                  }
                  off = ACELL_SPACE;
               }
            }
            else
            {
               rval = eprintf( &outlen, d_fmt,
                               body_indent + 1, ddata[0],
                               cell_sp, ddata[1],
                               cell_sp, ddata[2],
                               ddata[cell_qty - 3],
                               cell_sp, ddata[cell_qty - 2],
                               cell_sp, ddata[cell_qty - 1] );
               if (rval != OK)
               {
                  free(ddata);
                  return rval;
               }
            }
            printf( "\n" );
         }
         break;

   }

   free( dims );

   return OK;
}

