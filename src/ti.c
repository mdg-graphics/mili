/*
 * Time Invarient Routines.
 *
 */

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
 
************************************************************************
* Modifications:
*
*  I. R. Corey - September 30, 2006: Created for TI files.
*                See SCR#480
*
*  I. R. Corey - August 16, 2007: Added field for TI vars to note if
*                nodal or element result.
*                See SCR#480
*.
*  I. R. Corey - December 20, 2007: Increased name allocation to 16000.
*                Fixed a bug with the reallocation of MemStore.
*                SCR#: 513
*
*  I. R. Corey - February 02, 2009: Made TI file processing the
*                default.
*
************************************************************************
*/

#include <string.h>
#include "mili_internal.h"
#include "eprtf.h"

/*****************************************************************
 * Time Independent file functions:
 * Added May 2006 by I.R. Corey
 *****************************************************************
 */

/* Various TI state flags */
Bool_type ti_enable     = TRUE;
Bool_type ti_only       = FALSE;       /* If true,then only TI files are read and written */
Bool_type ti_data_found = FALSE;


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
/*
static Return_value ti_read_scalar( Famid fam_id, Param_ref *p_pr,
                                    void *p_value );
static Return_value ti_read_string( Famid fam_id, Param_ref *p_pr,
                                    char *p_value );
static Return_value ti_write_scalar( Famid fam_id, int type, char *name,
                                     void *data, Dir_entry_type etype );
static Return_value ti_write_string( Famid fam_id, char *name, char *value,
                                     Dir_entry_type etype );
static Return_value ti_write_array( Famid fam_id, int type, char *name,
                                    int order, int *dimensions, void *data,
                                    Dir_entry_type etype );

*/

/*****************************************************************
 * TAG( ti_file_seek ) PRIVATE
 *
 * Seek a TI family file.
 */
Return_value
ti_file_seek( Famid fam_id, LONGLONG offset )
{
   Mili_family *fam;
   int status;

   fam = fam_list[fam_id];

   /* Exit if TI capability not requested */
   if (!fam->ti_enable)
   {
      return OK;
   }

   /*
    * IRC: Aug 8, 2006 - changed test >= to > if ( offset >= fam->ti_next_free_byte )
    */
   if ( offset > fam->ti_next_free_byte )
   {
      return SEEK_FAILED;
   }

   status = fseek( fam->ti_cur_file, (long)offset, SEEK_SET );

   return ( status == 0 ) ? OK : SEEK_FAILED;
}

/*****************************************************************
 * TAG( ti_read_scalar ) LOCAL
 *
 * Read a scalar numeric parameter from the referenced family.
 */
static Return_value
ti_read_scalar( Famid fam_id, Param_ref *p_pr, void *p_value )
{
   
   int file, entry_idx;
   LONGLONG offset;
   Return_value rval;
   size_t nitems;
   LONGLONG *entry;
   int numtype;
   Mili_family *fam;

   fam = fam_list[fam_id];

   /* Get directory entry. */
   file = p_pr->file_index;
   entry_idx = p_pr->entry_index;

   /* Get file offset and length of data. */
   entry   = fam->ti_directory[file].dir_entries[entry_idx];
   offset  = entry[OFFSET_IDX];
   numtype = entry[MODIFIER1_IDX];

   if ( fam->ti_file_count >=0 )
   {
      rval = ti_file_open( fam->my_id, file, fam->access_mode );
      if ( rval != OK )
      {
         return rval;
      }
   }

   rval = ti_file_seek( fam_id, offset );
   if ( rval != OK )
   {
      return rval;
   }

   /* Read the data. */
   nitems = fam->read_funcs[numtype]( fam->ti_cur_file, p_value, 1 );

   return ( nitems == 1 ) ? OK : SHORT_READ;
}


/*****************************************************************
 * TAG( mc_ti_read_scalar ) PUBLIC
 *
 * Reads a TI numeric scalar from the referenced family.
 */
Return_value
mc_ti_read_scalar( Famid fam_id, char *name, void *p_value )
{
   Mili_family *fam = fam_list[fam_id];
   
   Htable_entry *phte;
   char new_name[M_MAX_NAME_LEN];
   Return_value status;

   if(fam->char_header[DIR_VERSION_IDX] >1){
      return mc_read_scalar(fam_id, name, p_value );
   }
   

   /* Exit if TI capability not requested */
   if (!ti_enable)
   {
      return( OK );
   }

   if ( name == NULL || *name == '\0' )
   {
      return INVALID_NAME;
   }

   if ( fam->ti_param_table == NULL )
   {
      return ENTRY_NOT_FOUND;
   }

   status = mc_ti_make_var_name( fam_id, name, NULL, new_name );
   if (status != OK)
   {
      return status;
   }

   status = ti_htable_search( fam->ti_param_table, new_name,
                              FIND_ENTRY, &phte );
   if ( phte == NULL )
   {
      return status;
   }

   return (ti_read_scalar( fam_id, (Param_ref *) phte->data, p_value ));
}





/*****************************************************************
 * TAG( mc_ti_wrt_scalar ) PUBLIC
 *
 * Write a TI numeric scalar into the referenced family.
 */
Return_value
mc_ti_wrt_scalar( Famid fam_id, int type, char *name, void *p_value )
{
   Mili_family *fam;
   char new_name[M_MAX_NAME_LEN];
   
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
   return write_scalar(fam,type,name,p_value,TI_PARAM);
}



/*****************************************************************
 * TAG( ti_read_string ) LOCAL
 *
 * Read a string parameter from the referenced family.
 */
static Return_value
ti_read_string( Famid fam_id, Param_ref *p_pr, char *p_value )
{
   int file, entry_idx;
   LONGLONG offset;
   Return_value rval;
   size_t nitems, length;
   LONGLONG *entry;

   Mili_family *fam;
   fam = fam_list[fam_id];
   
   /* Get directory entry. */
   file      = p_pr->file_index;
   entry_idx = p_pr->entry_index;

   /* Get file offset and length of data. */
   entry = fam->ti_directory[file].dir_entries[entry_idx];

   if ( entry[MODIFIER1_IDX] != M_STRING )
   {
      return NOT_STRING_TYPE;
   }

   offset = entry[OFFSET_IDX];
   length = (size_t)entry[LENGTH_IDX];

   if ( fam->ti_file_count >=0 )
   {
      rval = ti_file_open( fam->my_id, file, fam->access_mode );
      if ( rval != OK )
      {
         return rval;
      }
   }

   rval = ti_file_seek( fam_id, offset );
   if ( rval != OK )
   {
      return rval;
   }

   /* Read the data. */
   nitems = fam->read_funcs[M_STRING]( fam->ti_cur_file, p_value, length );

   return ( nitems == length ) ? OK : SHORT_READ;
}

/*****************************************************************
 * TAG( mc_ti_read_string ) PUBLIC
 *
 * Read a names TI string field from the referenced family.
 */
Return_value
mc_ti_read_string( Famid fam_id, char *name, char *p_value )
{
   Mili_family *fam;
   Htable_entry *phte;
   char new_name[128];
   Return_value status;

   fam = fam_list[fam_id];
   if(fam->char_header[DIR_VERSION_IDX] >1){
      return mc_read_string(fam_id, name, p_value );
   }
   /* Exit if TI capability not requested */
   if (!ti_enable)
   {
      return( OK );
   }

   p_value[0] = '\0'; /* Set default to NULL string */

   if ( name == NULL || *name == '\0' )
   {
      return INVALID_NAME;
   }

   if ( fam->ti_param_table == NULL )
   {
      return ENTRY_NOT_FOUND;
   }

   status = mc_ti_make_var_name( fam_id, name, NULL, new_name );
   if (status != OK)
   {
      return status;
   }

   status = ti_htable_search( fam->ti_param_table, new_name,
                              FIND_ENTRY, &phte );
   if ( phte == NULL )
   {
      return status;
   }

   return (ti_read_string( fam_id, (Param_ref *) phte->data, p_value ));
}


/*****************************************************************
 * TAG( mc_ti_wrt_string ) PUBLIC
 *
 * Write a TI string parameter into the referenced family.
 */
Return_value
mc_ti_wrt_string( Famid fam_id, char *name, char *value )
{
   Mili_family *fam;
   char new_name[128];
   Return_value status;
   
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
   return write_string(fam, name,value, TI_PARAM);
   
}



Return_value 
read_htable_array(Famid fam_id, char *name, void **p_data,
                  int *num_items_read )
{
   Htable_entry *phte,*next;
   Hash_table *table;
   Param_ref *p_pr;
   Mili_family *fam = fam_list[fam_id];
   char new_name[128];
   Return_value status = OK;
   LONGLONG *dir_ent;
   int file, entry_idx, num_type, cell_qty;
   int group_num_type,i;
   LONGLONG offset;
   size_t nitems, length;
   int *idata = NULL;
   LONGLONG *lidata= NULL;
   float *fdata= NULL;
   double *ddata= NULL;
   int first_ppr = 1;
   int current_count = 0;
   *num_items_read = 0;
   
   table = fam->param_table;
   status = htable_search( table, name,FIND_ENTRY, &phte ); 
   
   next = phte;
   
   while(next){
      if(strcmp(name,next->key) != 0){
         next = next->next;
         continue;
      }
      p_pr = (Param_ref*)next->data;
      file = p_pr->file_index;
      entry_idx = p_pr->entry_index;
      dir_ent = fam->directory[file].dir_entries[entry_idx];
      
      /* Get file offset and length of data. */
      offset   = dir_ent[OFFSET_IDX];
      length   = (size_t)dir_ent[LENGTH_IDX];
      num_type = dir_ent[MODIFIER1_IDX];
      if(first_ppr){
         group_num_type = num_type;
         first_ppr=0;
      }
      if ( fam->file_count >=0 )
      {
         status = non_state_file_open( fam, file, fam->access_mode );
         if ( status != OK )
         {
            return status;
         }
      }

      status = non_state_file_seek( fam, offset );
      
      if ( status != OK )
      {
         return status;
      }
      
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
                                      (size_t) p_pr->rank );
      if ( nitems != (size_t)p_pr-> rank )
      {
         free( p_pr->dims );
         return SHORT_READ;
      }
      /* Calculate array size */
      for ( i = 0, cell_qty = 1; i < p_pr->rank; i++ )
      {
         if ( p_pr->dims[i]>0 )
         {
            cell_qty *= p_pr->dims[i];
         }
      }
      switch( num_type )
      {
         case M_INT:
         case M_INT4:
            if(idata == NULL){
               idata = NEW_N( int, cell_qty, "Param array bufr" );
            }else{
               idata = RENEW_N( int,idata, current_count, 
                                 cell_qty, "Param array bufr");
            }
            if (cell_qty > 0 && idata == NULL)
            {
               free( p_pr->dims );
               return ALLOC_FAILED;
            }
            
            nitems = (fam->read_funcs[M_INT])( fam->cur_file, idata+current_count,
                                            (size_t) cell_qty );
            length/=4;
            break;

         case M_INT8:
            if(lidata == NULL){
               lidata = NEW_N( LONGLONG, cell_qty, "Param array bufr" );
            }else{
               lidata = RENEW_N( LONGLONG,lidata, current_count, 
                                 cell_qty, "Param array bufr");
            }
            if (cell_qty > 0 && lidata == NULL)
            {
               free( p_pr->dims );
               return ALLOC_FAILED;
            }
            
            nitems = (fam->read_funcs[M_INT8])( fam->cur_file, lidata+current_count,
                                             (size_t) cell_qty );
            length/=8;
            break;

         case M_FLOAT:
         case M_FLOAT4:
            if(fdata == NULL){
               fdata = NEW_N( float, cell_qty, "Param array bufr" );
            }else{
               fdata = RENEW_N( float,fdata, current_count, 
                                 cell_qty, "Param array bufr");
            }
            if (cell_qty > 0 && fdata == NULL)
            {
               free( p_pr->dims );
               return ALLOC_FAILED;
            }
            
            nitems = (fam->read_funcs[M_FLOAT])( fam->cur_file, fdata+current_count,
                                              (size_t) cell_qty );
            length=(length/4)-2;
            break;

         case M_FLOAT8:
            if(ddata == NULL) {
               ddata = NEW_N( double, cell_qty, "Param array bufr" );
            }else{
               ddata = RENEW_N( double,ddata, current_count, 
                                 cell_qty, "Param array bufr");
            }
            if (cell_qty > 0 && ddata == NULL)
            {
               free( p_pr->dims );
               return ALLOC_FAILED;
            }
            
            nitems = (fam->read_funcs[M_FLOAT8])( fam->cur_file, ddata+current_count,
                                               (size_t) cell_qty );
            length/=8;
            break;

      }
      current_count += cell_qty; 
      next = next->next;
		free( p_pr->dims );
   
   }
   switch( num_type )
   {
      case M_INT:
      case M_INT4:
         p_data[0] = (void *)idata;
         break;
      case M_INT8:
         p_data[0] = (void *)lidata;
         break;
      case M_FLOAT:
      case M_FLOAT4:
         p_data[0] = (void *)fdata;
         break;
      case M_FLOAT8:
         p_data[0] = (void *)ddata;
         break;
   }
      
   *num_items_read = current_count;
   return status;

}

/*****************************************************************
 * TAG( ti_read_array ) LOCAL
 *
 * Read a parameter array from the referenced family.
 */
static Return_value
ti_read_array( Famid fam_id, Param_ref *p_pr, void **p_data,
               int *num_items_read )
{
   int file, entry_idx;
   int cell_qty;
   int i;
   LONGLONG offset;
   size_t nitems, length;
   int num_type;
   LONGLONG *dir_ent;
   int *idata;
   LONGLONG *lidata;
   float *fdata;
   double *ddata;
   Return_value rval;
   Mili_family *fam;
   Param_ref *next_pr;
   
   fam = fam_list[fam_id];
   next_pr = p_pr;
   /*
   while(next_pr){
      next_pr=next_pr->next;
      
   }*/
   /* Get directory entry. */
   file = p_pr->file_index;
   entry_idx = p_pr->entry_index;
   
   dir_ent = fam->ti_directory[file].dir_entries[entry_idx];
   

   /* Get file offset and length of data. */
   offset   = dir_ent[OFFSET_IDX];
   length   = (size_t)dir_ent[LENGTH_IDX];
   num_type = dir_ent[MODIFIER1_IDX];
   
   if ( fam->ti_file_count >=0 )
   {
      rval = ti_file_open( fam->my_id, file, fam->access_mode );
      if ( rval != OK )
      {
         return rval;
      }
   }

   rval = ti_file_seek( fam_id, offset );
   
   if ( rval != OK )
   {
      return rval;
   }

   /* Read the data. */

   /* Read the array rank. */
   nitems = (fam->read_funcs[M_INT])( fam->ti_cur_file, &p_pr->rank, 1 );

   if ( nitems != 1 )
   {
      return SHORT_READ;
   }

   p_pr->dims = NEW_N( int, p_pr->rank, "Array dimensions bufr" );
   if (p_pr->rank > 0 && p_pr->dims == NULL)
   {
      return ALLOC_FAILED;
   }

   nitems = (fam->read_funcs[M_INT])( fam->ti_cur_file, p_pr->dims,
                                      (size_t) p_pr->rank );
   if ( nitems != (size_t)p_pr-> rank )
   {
      free( p_pr->dims );
      return SHORT_READ;
   }

   /* Calculate array size */
   for ( i = 0, cell_qty = 1; i < p_pr->rank; i++ )
   {
      if ( p_pr->dims[i]>0 )
      {
         cell_qty *= p_pr->dims[i];
      }
   }

   switch( num_type )
   {
      case M_INT:
      case M_INT4:
         idata = NEW_N( int, cell_qty, "Param array bufr" );
         if (cell_qty > 0 && idata == NULL)
         {
            free( p_pr->dims );
            return ALLOC_FAILED;
         }
         p_data[0] = (void *)idata;
         nitems = (fam->read_funcs[M_INT])( fam->ti_cur_file, idata,
                                            (size_t) cell_qty );
         length/=4;
         break;

      case M_INT8:
         lidata = NEW_N( LONGLONG, cell_qty, "Param array bufr" );
         if (cell_qty > 0 && lidata == NULL)
         {
            free( p_pr->dims );
            return ALLOC_FAILED;
         }
         p_data[0] = (void *)lidata;
         nitems = (fam->read_funcs[M_INT8])( fam->ti_cur_file, lidata,
                                             (size_t) cell_qty );
         length/=8;
         break;

      case M_FLOAT:
      case M_FLOAT4:
         fdata = NEW_N( float, cell_qty, "Param array bufr" );
         if (cell_qty > 0 && fdata == NULL)
         {
            free( p_pr->dims );
            return ALLOC_FAILED;
         }
         p_data[0] = (void *)fdata;
         nitems = (fam->read_funcs[M_FLOAT])( fam->ti_cur_file, fdata,
                                              (size_t) cell_qty );
         length=(length/4)-2;
         break;

      case M_FLOAT8:
         ddata = NEW_N( double, cell_qty, "Param array bufr" );
         if (cell_qty > 0 && ddata == NULL)
         {
            free( p_pr->dims );
            return ALLOC_FAILED;
         }
         p_data[0] = (void *)ddata;
         nitems = (fam->read_funcs[M_FLOAT8])( fam->ti_cur_file, ddata,
                                               (size_t) cell_qty );
         length/=8;
         break;

   }
   free( p_pr->dims );
   *num_items_read = nitems;

   return ( nitems == cell_qty ) ? OK : SHORT_READ;
}

/*****************************************************************
 * TAG( mc_ti_read_array ) PUBLIC
 *
 * Read a TI array from the referenced family.
 */
Return_value
mc_ti_read_array( Famid fam_id, char *name, void **p_data,
                  int *num_items_read )
{
   Htable_entry *phte = NULL;
   Hash_table *table = NULL;
   Mili_family *fam;
   char new_name[128];
   Return_value status;

   
   fam = fam_list[fam_id];
   
   *num_items_read = 0;

   /* Exit if TI capability not requested */
   if (!ti_enable)
   {
      return( OK );
   }
   
   if ( name == NULL || *name == '\0' )
   {
      return INVALID_NAME;
   }
   if(fam->char_header[DIR_VERSION_IDX] >1){
      return read_htable_array(fam_id, name,p_data,num_items_read);
   }
   
   table = fam->ti_param_table;
   
   if ( table == NULL )
   {
      return ENTRY_NOT_FOUND;
   }
   
   status = ti_htable_search( table, name,FIND_ENTRY, &phte );   
   if ( phte == NULL )
   {
      status = mc_ti_make_var_name( fam_id, name, NULL, new_name );
      if (status != OK)
      {
         return status;
      }

      status = ti_htable_search( table, new_name,FIND_ENTRY, &phte );
      if ( phte == NULL )
      {
         return status;
      }
   }
   
   return ti_read_array( fam_id, (Param_ref *) phte->data,
                         p_data, num_items_read );
}


/*****************************************************************
 * TAG( ti_write_array ) LOCAL
 *
 * Write a parameter array into the referenced family.
 */
static Return_value
ti_write_array( Famid fam_id, int type, char *name, int order,
                int *dimensions, void *data, Dir_entry_type etype )
{
   int i;
   int atoms;
   int *i_buf;
   size_t outbytes;
   Return_value rval = OK;
   Param_ref *ppr;
   int fidx;
   size_t write_ct;

   Mili_family *fam;
   fam = fam_list[fam_id];

   /* Ensure file is open and positioned. */
   if ( (rval = prep_for_new_data( fam, TI_DATA )) != OK )
   {
      return rval;
   }

   /* Calc number of entries in array. */
   for ( i = 0, atoms = 1; i < order; i++ )
   {
      if ( dimensions[i]>0 )
      {
         atoms *= dimensions[i];
      }
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
   rval = add_ti_dir_entry( fam, etype, type, ARRAY, 1, &name,
                            fam->ti_next_free_byte, outbytes );
   if ( rval != OK )
   {
      return rval;
   }

   /* Write it out... */
   write_ct = fam->write_funcs[M_INT]( fam->ti_cur_file, i_buf, order + 1 );
   if (write_ct != (order + 1))
   {
      return SHORT_WRITE;
   }
   write_ct = (fam->write_funcs[type])( fam->ti_cur_file, data,
                                        (size_t) atoms );
   if (write_ct != atoms)
   {
      return SHORT_WRITE;
   }

   fam->ti_next_free_byte += outbytes;

   free( i_buf );

   /* Now create entry for the param table. */
   ppr = NEW( Param_ref, "Param table entry - array" );
   if (ppr == NULL)
   {
      return ALLOC_FAILED;
   }
   fidx = fam->ti_cur_index;
   ppr->file_index = fidx;
   ppr->entry_index = fam->ti_directory[fidx].qty_entries - 1;
   rval = ti_htable_add_entry_data( fam->ti_param_table, name,
                                    ENTER_ALWAYS, ppr );
   if (rval != OK)
   {
      free(ppr);
   }

   return rval;
}



/*****************************************************************
 * TAG( mc_ti_wrt_array ) PUBLIC
 *
 * Write a TI array into the referenced family.
 */
Return_value
mc_ti_wrt_array( Famid fam_id, int type, char *name, int order,
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
                        TI_PARAM );
   return status;
                             
}



/*****************************************************************
 * TAG( mc_ti_def_class ) PUBLIC
 *
 * Define class information for a new TI Class.
 */
Return_value
mc_ti_def_class( Famid fam_id, int meshid, int state, int  matid,
                 char *superclass,
                 Bool_type meshvar, Bool_type nodal,
                 char *short_name, char *long_name )
{
   Mili_family *fam;

   fam = fam_list[fam_id];

   if ( meshid >=0 )   /* Ignore fields that are <0 */
   {
      fam->ti_meshid          = meshid;
   }

   if ( matid >=0 )
   {
      fam->ti_matid           = matid;
   }

   if ( state >=0 )
   {
      fam->ti_state           = state;
   }

   if ( meshvar >= 0 )
   {
      fam->ti_meshvar         = meshvar;
   }

   if ( nodal >=0 )
   {
      fam->ti_nodal           = nodal;
   }

   if ( superclass != NULL )
   {
      strcpy(fam->ti_superclass_name, superclass);
   }

   if ( short_name != NULL )
   {
      strcpy(fam->ti_short_name,      short_name);
   }

   if ( long_name != NULL )
   {
      strcpy(fam->ti_long_name,       long_name);
   }

   return OK;
}


/*****************************************************************
 * TAG( mc_ti_set_class ) PUBLIC
 *
 * Define class information for a new TI Class.
 */
Return_value
mc_ti_set_class( Famid fam_id, int meshid, int state, int matid,
                 char *superclass,
                 Bool_type meshvar, Bool_type nodal,
                 char *short_name, char *long_name )
{
   Return_value stat;
   stat = mc_ti_def_class( fam_id, meshid, state, matid, superclass,
                           meshvar, nodal,
                           short_name, long_name );
   return stat;
}

/*****************************************************************
 * TAG( ti_make_label_description ) PUBLIC
 *
 * Make a TI name with embedded class info
 */
Return_value
ti_make_label_description( int meshid,int mat_id,char *superclass,
                 char *short_name,  char *new_name)
{
   sprintf(new_name,
           "[/Mesh-%d/Sname-%s/Scls-%s/Mat-%d/]",
           meshid, short_name,superclass,mat_id);
   

   return OK;
}

/*****************************************************************
 * TAG( mc_ti_undef_class ) PUBLIC
 *
 * Reset class descriptor information for TI data..
 */
Return_value
mc_ti_undef_class( Famid fam_id )
{
   Mili_family *fam;

   fam = fam_list[fam_id];

   fam->ti_meshid          = -1;
   fam->ti_matid           = -1;
   fam->ti_state           = -1;
   fam->ti_meshvar         = -1;
   fam->ti_nodal           = -1;
   strcpy(fam->ti_superclass_name, "");
   strcpy(fam->ti_short_name, "");
   strcpy(fam->ti_long_name,  "");

   return OK;
}


/*****************************************************************
 * TAG( mc_ti_savedef_class ) PUBLIC
 *
 * Save the class descriptor information for TI data..
 */
Return_value
mc_ti_savedef_class( Famid fam_id )
{
   Mili_family *fam;

   fam = fam_list[fam_id];

   fam->ti_saved_meshid  = fam->ti_meshid;
   fam->ti_saved_matid   = fam->ti_matid ;
   fam->ti_saved_state   = fam->ti_state;
   fam->ti_saved_meshvar = fam->ti_meshvar;
   fam->ti_saved_nodal   = fam->ti_nodal;

   strcpy(fam->ti_saved_superclass_name, fam->ti_superclass_name);
   strcpy(fam->ti_saved_short_name,      fam->ti_short_name);
   strcpy(fam->ti_saved_long_name,       fam->ti_long_name);

   return OK;
}


/*****************************************************************
 * TAG( mc_ti_restoredef_class ) PUBLIC
 *
 * Restore the saved class descriptor information for TI data..
 */
Return_value
mc_ti_restoredef_class( Famid fam_id )
{
   Mili_family *fam;

   fam = fam_list[fam_id];

   fam->ti_meshid  = fam->ti_saved_meshid;
   fam->ti_matid   = fam->ti_saved_matid ;
   fam->ti_state   = fam->ti_saved_state;
   fam->ti_meshvar = fam->ti_saved_meshvar;
   fam->ti_nodal   = fam->ti_saved_nodal;

   strcpy(fam->ti_superclass_name, fam->ti_saved_superclass_name);
   strcpy(fam->ti_short_name,      fam->ti_saved_short_name);
   strcpy(fam->ti_long_name,       fam->ti_saved_long_name);

   return OK;
}


/*****************************************************************
 * TAG( mc_ti_enable ) PUBLIC
 *
 * Enable TI data capability
 */
void
mc_ti_enable(  Famid fam_id )

{
   ti_enable = TRUE;
}

/*****************************************************************
 * TAG( mc_ti_enable_only ) PUBLIC
 *
 * Enable TI ONLY data capability
 */
void
mc_ti_enable_only( Famid fam_id )

{
   ti_only = TRUE;
}

/*****************************************************************
 * TAG( mc_ti_disable ) PUBLIC
 *
 * Disable TI data capability
 */
void
mc_ti_disable( Famid fam_id )

{
   ti_enable     = FALSE;
   ti_only       = FALSE;
   ti_data_found = FALSE;
}

/*****************************************************************
 * TAG( mc_ti_disable_only ) PUBLIC
 *
 * Disable TI ONLY data capability.
 */
void
mc_ti_disable_only( Famid fam_id )

{
   ti_only = FALSE;
}

/*****************************************************************
 * TAG( mc_ti_data_found ) PUBLIC
 *
 * Set TI data found flag.
 */
void
mc_ti_data_found ( Famid fam_id )

{
   ti_data_found = TRUE;
}

/*****************************************************************
 * TAG( mc_ti_data_not_found ) PUBLIC
 *
 * Set TI data found flag to FALSE.
 */
void
mc_ti_data_not_found ( Famid fam_id )

{
   ti_data_found = FALSE;
}

/*****************************************************************
 * TAG( mc_ti_check_if_data_found ) PUBLIC
 *
 * Check if have TI data in this family
 */
Bool_type
mc_ti_check_if_data_found ( Famid fam_id )

{
   return ( ti_data_found );
}

/*****************************************************************
 * TAG( mc_ti_make_var_name ) PUBLIC
 *
 * Make a TI name with embedded class info
 */
Return_value
mc_ti_make_var_name( Famid fam_id, char *name, char *class, char *new_name )
{
   Mili_family *fam;
   char node_or_elem[16], meshvar[16];
   char temp_class[M_MAX_NAME_LEN];

   fam = fam_list[fam_id];

   if ( class != NULL )
   {
      strcpy( temp_class, class );
   }
   else
   {
      strcpy( temp_class, fam->ti_short_name );
   }

   if ( fam->ti_meshvar )
   {
      strcpy(meshvar, "TRUE");
   }
   else
   {
      strcpy(meshvar, "FALSE");
   }

   if ( fam->ti_nodal )
   {
      strcpy(node_or_elem, "TRUE");
   }
   else
   {
      strcpy(node_or_elem, "FALSE");
   }

   if ( strlen(fam->ti_superclass_name)==0 ||
         strlen(fam->ti_short_name)==0 )
   {
      strcpy( new_name, name );
   }
   else
   {
      sprintf(new_name,
              "%s[/Mesh-%d/Sname-%s/++/IsMvar-%s/IsNod-%s/Scls-%s/Mat-%d/St-%d]",
              name, fam->ti_meshid, temp_class,
              meshvar, node_or_elem, fam->ti_superclass_name,
              fam->ti_matid, fam->ti_state );
   }

   return OK;
}


/*****************************************************************
 * TAG( mc_ti_find_var_name ) PUBLIC
 *
 * Make a TI name with embedded class info
 */
Bool_type
mc_ti_find_var_name( Famid fam_id, char *name, char *class, char **var_name )
{
   Mili_family *fam;

   char sname[M_MAX_NAME_LEN], vname[M_MAX_NAME_LEN];

   int i=0;
   int num_entries=0;

   char *p_name, *p_class;

   char **wildcard_list;

   Hash_table *table;
   
   fam = fam_list[fam_id];
   
   if(fam->char_header[DIR_VERSION_IDX] >1){
      table = fam->param_table;
   }else{
      table = fam->ti_param_table;
   }
   strcpy( vname, name );
   strcat( vname, "[" );
   strcpy( sname,  "Sname-" );
   strcat( sname, class );

   /* Find the variable with this name and class */
   num_entries = htable_search_wildcard(table, 0, FALSE,
					vname, sname,
					"NULL", NULL );
   
   if ( num_entries==0 )
        return FALSE;

   wildcard_list = NEW_N(char *, num_entries,
                                "WC List" );

   num_entries = htable_search_wildcard(table, 0, FALSE,
                                        vname, sname,
                                        "NULL", wildcard_list);

   for ( i=0;i<num_entries;i++ ) 
   {
      p_name  = strstr(wildcard_list[i], vname );
      p_class = strstr(wildcard_list[i], vname );
	   if ( p_name && p_class ) 
      {
	   }
	   *var_name = strdup( wildcard_list[i] );
	   break;
   }

   htable_delete_wildcard_list(num_entries, wildcard_list);

   if ( mc_ti_verify_var_name( fam_id, *var_name ) )
        return TRUE;
   else
        return FALSE;
}


/*****************************************************************
 * TAG( mc_ti_verify_var_name ) PUBLIC
 *
 * Checks for the existance if a TI hash table entry.
 */
Bool_type
mc_ti_verify_var_name( Famid fam_id, char *name )
{
   Htable_entry *phte;
   Mili_family *fam;
   int status = OK;
   Hash_table *table;
   
   fam = fam_list[fam_id];
   if(fam->char_header[DIR_VERSION_IDX] >1){
      table = fam->param_table;
   }else{
      table = fam->ti_param_table;
   }
   status = ti_htable_search( table, name, FIND_ENTRY, &phte );

   if ( phte == NULL )
   {
        return FALSE;
   }
   return TRUE;
}

/*****************************************************************
 * TAG( mc_ti_get_class ) PUBLIC
 *
 * Search a hash table for an entry and return the fields type and
 * length.
 */
Return_value
mc_ti_get_class( Famid fam_id, int meshid, char *name, char *class,
                 int *state, int *matid,
                 int *superclass,
                 Bool_type *isMeshvar, Bool_type *isNodal,
                 int *datatype, int *datalength )
{
   return mc_ti_get_data_attributes( fam_id, meshid, name, class,
                                     state, matid,
                                     superclass,
                                     isMeshvar, isNodal,
                                     datatype, datalength );
}


/*****************************************************************
 * TAG( mc_ti_get_data_attributes ) PUBLIC
 *
 * Search a hash table for an entry and return the fields type and
 * length.
 */
Return_value
mc_ti_get_data_attributes( Famid fam_id, int meshid, char *name,
                           char *this_class, int *state, int *matid,
                           int *superclass,
                           Bool_type *isMeshvar, Bool_type *isNodal,
                           int *datatype, int *datalength )
{
   Htable_entry *phte;
   Param_ref *p_pr;
   unsigned int i;
   LONGLONG       *entry;
	int  entry_idx, file;
   int       len_bytes;
   Mili_family *fam;
   char *mili_class, new_name[256];
   int order=3;
   Return_value status;

   fam = fam_list[fam_id];

   status = ti_htable_search( fam->ti_param_table, name, FIND_ENTRY, &phte );

   if ( phte == NULL )
   {
      return status;
   }

   p_pr = ( Param_ref * ) phte->data;

   file      = p_pr->file_index;
   entry_idx = p_pr->entry_index;

   /* Get the type and length of the data. */
   entry        = fam->ti_directory[file].dir_entries[entry_idx];
   len_bytes    = entry[LENGTH_IDX];
   *datatype    = entry[MODIFIER1_IDX];


   /* Get the Mesh Variable flag from the name */

   mili_class = strstr(name, "IsMvar" );
   *isMeshvar = FALSE;

   if ( mili_class )
   {
      strcpy(new_name, mili_class);
      for ( i=0; i<strlen(new_name); i++)
      {
         if ( new_name[i]=='T' && new_name[i+1]=='R')
         {
            *isMeshvar = TRUE;
            break;
         }
      }
   }

   /* Get the nodal/element flag from the name */

   mili_class = strstr(name, "IsNod" );
   *isNodal = FALSE;

   if ( mili_class )
   {
      strcpy(new_name, mili_class);
      for ( i=0; i<strlen(new_name); i++)
      {
         if ( new_name[i]=='T' && new_name[i+1]=='R')
         {
            *isNodal = TRUE;
            break;
         }
      }
   }

   /* Get the Mesh-Id from the name */

   mili_class = strstr(name, "St-" );
   if ( mili_class )
   {
      strcpy(new_name, mili_class);
      for ( i=0; i<strlen(new_name); i++)
      {
         if ( new_name[i]=='/' )
         {
            new_name[i]='\0'; /* Terminate name at end of class name */
            *state = atoi(&mili_class[6]);
            break;
         }
      }
   }

   /* Get the Mat-Id from the name */

   mili_class = strstr(name, "Mat-" );
   if ( mili_class )
   {
      strcpy(new_name, mili_class);
      for ( i=0; i<strlen(new_name); i++)
      {
         if ( new_name[i]=='/' )
         {
            new_name[i]='\0'; /* Terminate name at end of class name */
            *matid = atoi(&mili_class[4]);
            break;
         }
      }
   }

   /* Get the superclass from the name */

   mili_class = strstr(name, "M_" );
   if ( mili_class )
   {
      strcpy(new_name, mili_class);
      for ( i=0; i<strlen(new_name); i++)
      {
         if ( new_name[i]=='/' )
         {
            new_name[i]='\0'; /* Terminate name at end of class name */
            break;
         }
      }
   }

   /* Convert text class name to Mili-class type */
   status = className_to_classEnum(new_name,superclass);
   if(status != OK)
   {
      return INVALID_CLASS_NAME;
   }
   
   switch (*datatype)
   {
      case (M_INT):
      case (M_FLOAT):
      case (M_FLOAT4):
         *datalength = len_bytes/4;
         break;

      case (M_INT8):
      case (M_FLOAT8):
         *datalength = len_bytes/8;
         break;

      case (M_STRING):
         *datalength = len_bytes;
         break;

      default:
         return INVALID_DATA_TYPE;
         break;
   }

   if ( *datalength >2  && *datatype != M_STRING)
   {
      datalength -= (order+1);
   }

   return OK;
}

/*****************************************************************
 * TAG( mc_ti_get_superclass_from_name ) PUBLIC
 *
 * return the fields super
 * class type
 */
int
mc_ti_get_superclass_from_name( char *name )
{
   char new_name[128], *class;
   unsigned int i;
   int superclass = M_INVALID_LABEL;
   Return_value status;
   /* Get the superclass from the name */

   class = strstr(name, "M_" );
   if ( class )
   {
      new_name[0] = '\0';
      strncat(new_name, class, 127);
      for ( i=0; i<strlen(new_name); i++)
      {
         if ( new_name[i]=='/' )
         {
            new_name[i]='\0'; /* Terminate name at end of class name */
            break;
         }
      }

      /* Convert text class name to Mili-class type */
      status = className_to_classEnum(new_name,&superclass);
		if(status != OK)
		{
			mc_print_error("Name to enum conversion",status);
		}
      
   }

   return( superclass );
}


/*****************************************************************
 * TAG( mc_ti_get_material_from_name ) PUBLIC
 *
 * Return the fields material number.
 */
int
mc_ti_get_material_from_name( char *name )
{
   char new_name[M_MAX_NAME_LEN], *mat_ptr=NULL;
   unsigned int i;
   int mat_number;

   /* Get the superclass from the name */

   mat_ptr = strstr(name, "Mat-" );
   if ( mat_ptr )
   {
      new_name[0] = '\0';
      strncat(new_name, (char *)(mat_ptr+4), M_MAX_NAME_LEN-1);
      for ( i=0; i<strlen(new_name); i++)
      {
         if ( new_name[i]=='/' )
         {
            new_name[i]='\0'; /* Terminate name at end of class name */
            break;
         }
      }
      mat_number = atoi( new_name );
      return( mat_number );
   }
   return( -1 );
}


/*****************************************************************
 * TAG( mc_ti_get_data_len ) PUBLIC
 *
 * Search a hash table for an entry and return the fields type and
 * length.
 */
Return_value
mc_ti_get_data_len( Famid fam_id, char *name, int *datatype, int *datalength )
{
   Htable_entry *phte, *temp_phte ;
   Hash_table *table;
   Param_ref *p_pr;
   LONGLONG  *entry;
	int entry_idx, file;
   int       len_bytes;
   Mili_family *fam;
   int order=3;
   Return_value status;
   *datalength = 0;
   fam = fam_list[fam_id];
   if(fam->char_header[DIR_VERSION_IDX] >1){
      table = fam->param_table;
   }else{
      table = fam->ti_param_table;
   }
   status = ti_htable_search( table, name, FIND_ENTRY, &phte );

   if ( phte == NULL )
   {
      return status;
   }
   while(phte != NULL){
      p_pr = ( Param_ref * ) phte->data;

      file      = p_pr->file_index;
      entry_idx = p_pr->entry_index;

      /* Get the type and length of the data. */
      if(fam->char_header[DIR_VERSION_IDX] >1){
         entry        = fam->directory[file].dir_entries[entry_idx];
      }else{
         entry        = fam->ti_directory[file].dir_entries[entry_idx];
      }
      len_bytes    = entry[LENGTH_IDX];
      *datatype    = entry[MODIFIER1_IDX];


      switch (*datatype)
      {
         case (M_INT):
            *datalength +=  len_bytes/fam->external_size[M_INT];
            break;

         case (M_INT8):
            *datalength += len_bytes/fam->external_size[M_INT8];
            break;

         case (M_STRING):
            *datalength += len_bytes;
            break;

         case (M_FLOAT):
         case (M_FLOAT4):
            *datalength += len_bytes/fam->external_size[M_FLOAT];
            break;

         case (M_FLOAT8):
            *datalength += len_bytes/fam->external_size[M_FLOAT8];
            break;

         default:
            return INVALID_DATA_TYPE;
            break;
      }
      phte = phte->next;
   }
   if ( *datalength >2 && *datatype != M_STRING)
   {
      *datalength -= (order+1);
   }
   

   return OK;
}


/*****************************************************************
 * TAG( mc_ti_get_metadata_from_name ) PUBLIC
 *
 * Extract embedded metadata fields from a TI variable name.
 */
Return_value
mc_ti_get_metadata_from_name( char *name, char *class,
                              int *meshid, int *state, int *matid,
                              int *superclass,
                              Bool_type *isMeshvar, Bool_type *isNodal )
{
   unsigned int       i;
   char new_name[128], *name_ptr;
   Return_value rval;

   /* Get the class name */

   /* Initialize return variables */
   *state      = 0;
   *matid      = 0;
   *superclass = 0;
   *isMeshvar  = FALSE;
   *isNodal    = FALSE;

   name_ptr = strstr(name, "Sname-" );
   if ( name_ptr )
   {
      new_name[0] = '\0';
      strncat(new_name, name_ptr, 127);
      for ( i=0; i<strlen(new_name); i++)
      {
         if ( new_name[i]=='/' )
         {
            new_name[i]='\0'; /* Terminate name at end of name_ptr name */
            strcpy(class, &new_name[6]);
            break;
         }
      }
   }

   /* Get the superclass name */

   name_ptr = strstr(name, "Scls-" );
   if ( name_ptr )
   {
      new_name[0] = '\0';
      strncat(new_name, name_ptr, 127);
      for ( i=0; i<strlen(new_name); i++)
      {
         if ( new_name[i]=='/' || new_name[i] ==']')
         {
            new_name[i]='\0'; /* Terminate name at end of name_ptr name */
            rval = className_to_classEnum( &new_name[5], superclass );
            if (rval != OK)
            {
               return rval;
            }
            break;
         }
      }
   }

   /* Get the Mesh Variable flag from the name */

   name_ptr = strstr(name, "IsMvar" );
   *isMeshvar = FALSE;

   if ( name_ptr )
   {
      new_name[0] = '\0';
      strncat(new_name, name_ptr, 127);
      for ( i=0; i<strlen(new_name); i++)
      {
         if ( new_name[i]=='T' && new_name[i+1]=='R')
         {
            *isMeshvar = TRUE;
            break;
         }
      }
   }

   /* Get the nodal/element flag from the name */

   name_ptr = strstr(name, "IsNod" );
   *isNodal = FALSE;

   if ( name_ptr )
   {
      new_name[0] = '\0';
      strncat(new_name, name_ptr, 127);
      for ( i=0; i<strlen(new_name); i++)
      {
         if ( new_name[i]=='T' && new_name[i+1]=='R')
         {
            *isNodal = TRUE;
            break;
         }
      }
   }

   /* Get the Mesh-Id from the name */

   name_ptr = strstr(name, "Mesh-");
   if (name_ptr)
   {
      new_name[0] = '\0';
      strncat(new_name, name_ptr, 127);
      for (i = 0; i < strlen(new_name); ++i)
      {
         if (new_name[i] == '/')
         {
            new_name[i]='\0'; /* Terminate name at end of name_ptr name */
            *meshid = atoi(&new_name[5]);
            break;
         }
      }
   }

   /* Get the State-Id from the name */

   name_ptr = strstr(name, "St-" );
   if ( name_ptr )
   {
      new_name[0] = '\0';
      strncat(new_name, name_ptr, 127);
      for ( i=0; i<strlen(new_name); i++)
      {
         if ( new_name[i]==']' )
         {
            new_name[i]='\0'; /* Terminate name at end of name_ptr name */
            *state = atoi(&new_name[3]);
            break;
         }
      }
   }

   /* Get the Mat-Id from the name */

   name_ptr = strstr(name, "Mat-" );
   if ( name_ptr )
   {
      new_name[0] = '\0';
      strncat(new_name, name_ptr, 127);
      for ( i=0; i<strlen(new_name); i++)
      {
         if ( new_name[i]=='/' )
         {
            new_name[i]='\0'; /* Terminate name at end of name_ptr name */
            *matid = atoi(&new_name[4]);
            break;
         }
      }
   }

   return OK;
}

/* End of ti.c */
