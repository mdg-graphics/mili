/* $Id: mc_silo_ti.c,v 1.8 2011/05/26 21:43:13 durrenberger1 Exp $ */

/*
 * Time Invarient Routines.
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

/*
************************************************************************
* Modifications:
*
*  I. R. Corey - January 25, 2008: Created for Mili Silo.
*                See SCR#480
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
Bool_type silo_ti_enable     = FALSE;
Bool_type silo_ti_only       = FALSE;       /* If true,then only TI files are read and written */
Bool_type silo_ti_data_found = FALSE;


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

int
mc_silo_ti_make_var_name( Famid fam_id, char *name, char *class, char *new_name );

Return_value
mc_silo_ti_read_scalar( Famid fam_id, char *name, void *p_value )
{
   Mili_family *fam;
   Htable_entry *phte;
   char new_name[128];
   Return_value status;

   fam = fam_list[fam_id];

   /* Exit if TI capability not requested */
   if (!silo_ti_enable)
      return( OK );

   if ( name == NULL || *name == '\0' )
      return INVALID_NAME;

   if ( fam->ti_param_table == NULL )
      return ENTRY_NOT_FOUND;

   status = mc_silo_ti_make_var_name( fam_id, name, NULL, new_name );

   status = ti_htable_search( fam->ti_param_table, new_name,
                              FIND_ENTRY, &phte );
   if ( phte == NULL )
      return status;

   return (ti_read_scalar( fam_id, (Param_ref *) phte->data, p_value ));
}


/*****************************************************************
 * TAG( ti_read_scalar ) PRIVATE
 *
 * Read a scalar numeric parameter from the referenced family.
 */

#ifdef AAA
Return_value
ti_read_scalar( Famid fam_id, Param_ref *p_pr, void *p_value )
{
   int file, entry_idx;
   long offset;
   Return_value rval;
   size_t nitems;
   int *entry;
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
   }
   if ( rval != OK )
      return rval;

   rval = ti_file_seek( fam_id, offset );
   if ( rval != OK )
      return rval;

   /* Read the data. */
   nitems = fam->read_funcs[numtype]( fam->ti_cur_file, p_value, 1 );

   return ( nitems == 1 ) ? OK : SHORT_READ;
}
#endif


/*****************************************************************
 * TAG( mc_silo_ti_wrt_scalar ) PUBLIC
 *
 * Write a TI numeric scalar into the referenced family.
 */
Return_value
mc_silo_ti_wrt_scalar( Famid fam_id, int type, char *name, void *p_value )
{
   Mili_family *fam;
   char new_name[128];
   int status;

   fam = fam_list[fam_id];

   /* Exit if TI capability not requested */
   if (!silo_ti_enable)
      return( OK );


   if ( name == NULL || *name == '\0' )
      return INVALID_NAME;
   else if ( fam->access_mode == 'r' )
      return BAD_ACCESS_TYPE;

   if ( fam->ti_param_table == NULL )
   {
      fam->ti_param_table = htable_create( DEFAULT_HASH_TABLE_SIZE );
      if (fam->ti_param_table == NULL)
      {
         return ALLOC_FAILED;
      }
   }

   status = mc_silo_ti_make_var_name( fam_id, name, NULL, new_name );

   return ti_write_scalar( fam_id, type, new_name, p_value, TI_PARAM );
}



/*****************************************************************
 * TAG( ti_write_scalar ) PRIVATE
 *
 * Write a scalar numeric parameter into the referenced family.
 */

#ifdef AAA
Return_value
ti_write_scalar( Famid fam_id, int type, char *name, void *data, Dir_entry_type etype )
{
   size_t outbytes;
   Return_value rval;
   Param_ref *ppr;
   int fidx;
   Htable_entry *phte;
   int *entry;
   int prev_index;
   size_t write_ct;

   Mili_family *fam;

   fam = fam_list[fam_id];

   /* See if entry already exists. */
   rval = ti_htable_search( fam->ti_param_table, name, ENTER_MERGE, &phte );
   if ( rval == OK )
   {
      /* Ensure file is open and positioned for initial write. */
      rval = prep_for_new_data( fam, TI_DATA );

      if ( rval == OK )
      {
         /* Add entry into directory. */
         outbytes = EXT_SIZE( fam, type );
         rval = add_ti_dir_entry( fam, etype, type, SCALAR, 1, &name,
                                  fam->ti_next_free_byte, outbytes );

         if ( rval == OK )
         {
            /* Now create entry for the param table. */
            ppr = NEW( Param_ref, "Param table entry - scalar" );
            fidx = fam->ti_cur_index;
            ppr->file_index = fidx;
            ppr->entry_index = fam->ti_directory[fidx].qty_entries - 1;
            phte->data = (void *) ppr;

            /* Write the scalar. */
            write_ct = (fam->write_funcs[type])( fam->ti_cur_file, data, 1 );
            if (write_ct != 1)
            {
               free(ppr);
               return SHORT_READ;
            }

            fam->ti_next_free_byte += outbytes;
         }
      }
   }
   else if (rval == ENTRY_EXISTS)
   {
      /* Found name match.  OK to re-write if numeric type is same. */
      ppr = (Param_ref *) phte->data;
      entry = fam->ti_directory[ppr->file_index].dir_entries[ppr->entry_index];

      if ( entry[MODIFIER1_IDX] == type )
      {
         /* Save id of any extant open file for use below. */
         prev_index = fam->ti_cur_index;

         /* Open in append mode to avoid truncating file. */
         ti_file_seek( fam_id, (long) entry[OFFSET_IDX] );

         write_ct = (fam->write_funcs[type])( fam->ti_cur_file, data, (size_t) 1 );
         if (write_ct != 1)
         {
            return SHORT_READ;
         }

         /*
          * This was a re-write.  If the destination file wasn't already
          * open, close it to ensure changes are flushed to disk.
          */
         if ( prev_index != fam->ti_cur_index )
            ti_file_close( fam_id );
         else if ( fam->ti_cur_file != NULL )
            /* Leave file pointer at end of file. */
            fseek( fam->ti_cur_file, 0, SEEK_END );

         rval = OK;
      }
      else
         /* Type mismatch; re-write error. */
         rval = NUM_TYPE_MISMATCH;
   }

   return rval;
}
#endif


/*****************************************************************
 * TAG( mc_silo_ti_read_string ) PUBLIC
 *
 * Read a names TI string field from the referenced family.
 */
Return_value
mc_silo_ti_read_string( Famid fam_id, char *name, char *p_value )
{
   Mili_family *fam;
   Htable_entry *phte;
   char new_name[128];
   int status;
   Return_value rval;

   fam = fam_list[fam_id];

   /* Exit if TI capability not requested */
   if (!silo_ti_enable)
      return( OK );

   if ( name == NULL || *name == '\0' )
      return INVALID_NAME;

   if ( fam->ti_param_table == NULL )
      return ENTRY_NOT_FOUND;

   status = mc_silo_ti_make_var_name( fam_id, name, NULL, new_name );
   return OK;

   rval = ti_htable_search( fam->ti_param_table, new_name,
                            FIND_ENTRY, &phte );
   if ( phte == NULL )
      return rval;
   return OK;

   return (ti_read_string( fam_id, (Param_ref *) phte->data, p_value ));
}

#ifdef AAA
/*****************************************************************
 * TAG( ti_read_string ) PRIVATE
 *
 * Read a string parameter from the referenced family.
 */
Return_value
ti_read_string( Famid fam_id, Param_ref *p_pr, char *p_value )
{
   int file, entry_idx;
   long offset;
   Return_value rval;
   size_t nitems, length;
   int *entry;

   Mili_family *fam;
   fam = fam_list[fam_id];

   /* Get directory entry. */
   file      = p_pr->file_index;
   entry_idx = p_pr->entry_index;

   /* Get file offset and length of data. */
   entry = fam->ti_directory[file].dir_entries[entry_idx];

   if ( entry[MODIFIER1_IDX] != M_STRING )
      return NOT_STRING_TYPE;

   offset = entry[OFFSET_IDX];
   length = entry[LENGTH_IDX];

   if ( fam->ti_file_count >=0 )
   {
      rval = ti_file_open( fam->my_id, file, fam->access_mode );
   }
   if ( rval != OK )
      return rval;

   rval = ti_file_seek( fam_id, offset );
   if ( rval != OK )
      return rval;

   /* Read the data. */
   nitems = fam->read_char( fam->ti_cur_file, p_value, length );

   return ( nitems == length ) ? OK : SHORT_READ;
}
#endif

/*****************************************************************
 * TAG( mc_silo_ti_wrt_string ) PUBLIC
 *
 * Write a TI string parameter into the referenced family.
 */
Return_value
mc_silo_ti_wrt_string( Famid fam_id, char *name, char *value )
{
   Mili_family *fam;
   char new_name[128];
   int status;

   if ( name == NULL || *name == '\0' )
      return INVALID_NAME;

   fam = fam_list[fam_id];

   /* Exit if TI capability not requested */
   if (!silo_ti_enable)
      return( OK );

   status = mc_silo_ti_make_var_name( fam_id, name, NULL, new_name );

   if ( fam->ti_param_table == NULL )
   {
      fam->ti_param_table = htable_create( DEFAULT_HASH_TABLE_SIZE );
      if (fam->ti_param_table == NULL)
      {
         return ALLOC_FAILED;
      }
   }

   return ti_write_string( fam_id, new_name, value, TI_PARAM );
}

#ifdef AAA
/*****************************************************************
 * TAG( ti_write_string ) PRIVATE
 *
 * Write a string parameter into the referenced family.
 */
Return_value
ti_write_string( Famid fam_id, char *name, char *value, Dir_entry_type etype )
{
   size_t c_qty, s_len;
   char *c_buf;
   char *pdest, *psrc;
   Return_value rval;
   Param_ref *ppr;
   int fidx;
   Htable_entry *phte;
   int *entry;
   int prev_index;

   Mili_family *fam;

   fam = fam_list[fam_id];

   /* Evaluate how many characters required for string. */
   s_len = strlen( value ) + 1;
   c_qty = ROUND_UP_INT( s_len, 4 );

   c_buf = value;

   rval = ti_htable_search( fam->ti_param_table, name, ENTER_MERGE, &phte );
   if ( rval == OK )
   {
      /* Ensure file is open and positioned. */
      rval = prep_for_new_data( fam, TI_DATA );
      if ( rval == OK )
      {
         if ( s_len != c_qty )
         {
            /* Allocate output buffer. */
            c_buf = NEW_N( char, c_qty, "App string char buf" );

            /* Copy the string into the output buffer. */
            for ( psrc = value, pdest = c_buf; *pdest++ = *psrc++; );
         }

         /* Add directory entry. Modifier is Mili data type. */
         rval = add_ti_dir_entry( fam, etype, M_STRING, DONT_CARE, 1, &name,
                                  fam->ti_next_free_byte, c_qty );

         if ( rval == OK )
         {
            /* Now create entry for the param table. */
            ppr = NEW( Param_ref, "Param table entry - string" );
            fidx = fam->ti_cur_index;
            ppr->file_index = fidx;
            ppr->entry_index = fam->ti_directory[fidx].qty_entries - 1;
            phte->data = (void *) ppr;

            /* Write the string. */
            (*fam->write_char)( fam->ti_cur_file, c_buf, c_qty );

            fam->ti_next_free_byte += c_qty;

            if ( c_buf != value )
               free( c_buf );
         }
      }
   }
   else if (rval = ENTRY_EXISTS)
   {
      /* Found name match.  OK to re-write if new length fits in old space. */
      ppr = (Param_ref *) phte->data;
      entry = fam->ti_directory[ppr->file_index].dir_entries[ppr->entry_index];

      if ( (size_t) entry[LENGTH_IDX] >= s_len )
      {
         /* Save id of any extant open file for use below. */
         prev_index = fam->ti_cur_index;

         /* Open in append mode to avoid truncating file. */
         ti_file_seek( fam->my_id, (long) entry[OFFSET_IDX] );

         /* Re-write the string. */
         (*fam->write_char)( fam->ti_cur_file, c_buf, s_len );

         if ( prev_index != fam->ti_cur_index )
            ti_file_close( fam->my_id );
         else if ( fam->ti_cur_file != NULL )
            /* Leave file pointer at end of file. */
            fseek( fam->ti_cur_file, 0, SEEK_END );

         rval = OK;
      }
      else
         /* Type mismatch; re-write error. */
         rval = STR_LEN_MISMATCH;
   }

   return rval;
}


#endif
/*****************************************************************
 * TAG( mc_silo_ti_read_array ) PUBLIC
 *
 * Read a TI array from the referenced family.
 */
Return_value
mc_silo_ti_read_array( Famid fam_id, char *name, void **p_data, int *num_items_read )
{
   Htable_entry *phte;
   Mili_family *fam;
   char new_name[128];
   Return_value status;

   fam = fam_list[fam_id];

   *num_items_read = 0;

   /* Exit if TI capability not requested */
   if (!silo_ti_enable)
      return( OK );

   if ( name == NULL || *name == '\0' )
      return INVALID_NAME;

   if ( fam->ti_param_table == NULL )
      return ENTRY_NOT_FOUND;

   status = mc_silo_ti_make_var_name( fam_id, name, NULL, new_name );

   status = ti_htable_search( fam->ti_param_table, new_name,
                              FIND_ENTRY, &phte );
   if ( phte == NULL )
      return status;

   return ti_read_array( fam_id, (Param_ref *) phte->data, p_data, num_items_read );
}

#ifdef AAA
/*****************************************************************
 * TAG( ti_read_array ) PRIVATE
 *
 * Read a parameter array from the referenced family.
 */
Return_value
ti_read_array( Famid fam_id, Param_ref *p_pr, void **p_data, int *num_items_read )
{
   int file, entry_idx;
   int cell_qty;
   int i;
   long offset;
   size_t nitems, length, length_array;
   int num_type;
   int *dir_ent;
   int *idata;
   LONGLONG *lidata;
   float *fdata;
   double *ddata;
   Return_value rval;
   Mili_family *fam;

   fam = fam_list[fam_id];

   /* Get directory entry. */
   file = p_pr->file_index;
   entry_idx = p_pr->entry_index;
   dir_ent = fam->ti_directory[file].dir_entries[entry_idx];

   /* Get file offset and length of data. */
   offset   = dir_ent[OFFSET_IDX];
   length   = dir_ent[LENGTH_IDX];
   num_type = dir_ent[MODIFIER1_IDX];

   if ( fam->ti_file_count >=0 )
   {
      rval = ti_file_open( fam->my_id, file, fam->access_mode );
   }
   if ( rval != OK )
      return rval;

   rval = ti_file_seek( fam_id, offset );
   if ( rval != OK )
      return rval;

   /* Read the data. */

   /* Read the array rank. */
   nitems = (fam->read_funcs[M_INT])( fam->ti_cur_file, &p_pr->rank, 1 );

   if ( nitems != 1 )
      return SHORT_READ;

   p_pr->dims = NEW_N( int, p_pr->rank, "Array dimensions bufr" );

   nitems = (fam->read_funcs[M_INT])( fam->ti_cur_file, p_pr->dims, (size_t) p_pr->rank );
   if ( nitems != (size_t)p_pr-> rank )
   {
      free( p_pr->dims );
      return SHORT_READ;
   }

   /* Calculate array size */
   for ( i = 0, cell_qty = 1; i < p_pr->rank; i++ )
   {
      if ( p_pr->dims[i]>0 )
         cell_qty *= p_pr->dims[i];
   }

   switch( num_type )
   {
      case M_INT:
      case M_INT4:
         idata = NEW_N( int, cell_qty, "Param array bufr" );
         p_data[0] = (void *)idata;
         nitems = (fam->read_funcs[M_INT])( fam->ti_cur_file, idata, (size_t) cell_qty );
         length/=4;
         break;

      case M_INT8:
         lidata = NEW_N( LONGLONG, cell_qty, "Param array bufr" );
         p_data[0] = (void *)lidata;
         nitems = (fam->read_funcs[M_INT8])( fam->ti_cur_file, lidata, (size_t) cell_qty );
         length/=8;
         break;

      case M_FLOAT:
      case M_FLOAT4:
         fdata = NEW_N( float, cell_qty, "Param array bufr" );
         p_data[0] = (void *)fdata;
         nitems = (fam->read_funcs[M_FLOAT])( fam->ti_cur_file, fdata, (size_t) cell_qty );
         length=(length/4)-2;
         break;

      case M_FLOAT8:
         ddata = NEW_N( double, cell_qty, "Param array bufr" );
         p_data[0] = (void *)ddata;
         nitems = (fam->read_funcs[M_FLOAT8])( fam->ti_cur_file, ddata, (size_t) cell_qty );
         length/=8;
         break;

   }

   *num_items_read = nitems;

   return ( nitems == cell_qty ) ? OK : SHORT_READ;
}
#endif


/*****************************************************************
 * TAG( mc_silo_ti_wrt_array ) PUBLIC
 *
 * Write a TI array into the referenced family.
 */
Return_value
mc_silo_ti_wrt_array( Famid fam_id, int type, char *name, int order,
                      int *dimensions, void *data )
{
   Mili_family *fam;
   char new_name[128];
   int  status;

   /* Always force the order to be 3 for TI files */

   int ti_order = 3;

   if ( name == NULL || *name == '\0' )
      return INVALID_NAME;

   fam = fam_list[fam_id];

   /* Exit if TI capability not requested */
   if (!silo_ti_enable)
      return( OK );

   if ( fam->ti_param_table == NULL )
   {
      fam->ti_param_table = htable_create( DEFAULT_HASH_TABLE_SIZE );
      if (fam->ti_param_table == NULL)
      {
         return ALLOC_FAILED;
      }
   }

   status = mc_silo_ti_make_var_name( fam_id, name, NULL, new_name );

   return ti_write_array( fam_id, type, new_name, ti_order, dimensions, data,
                          TI_PARAM );
}


#ifdef AAA
/*****************************************************************
 * TAG( ti_write_array ) PRIVATE
 *
 * Write a parameter array into the referenced family.
 */
Return_value
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
      return rval;

   /* Calc number of entries in array. */
   for ( i = 0, atoms = 1; i < order; i++ )
      if ( dimensions[i]>0 )
         atoms *= dimensions[i];

   /* Load integer descriptors into an output buffer. */
   i_buf = NEW_N( int, order + 1, "App array descr buf" );
   i_buf[0] = order;
   for ( i = 0; i < order; i++ )
      i_buf[i + 1] = dimensions[i];

   /* Add entry into directory. */
   outbytes = ((order + 1) * EXT_SIZE( fam, M_INT ))
              + atoms * EXT_SIZE( fam, type );
   rval = add_ti_dir_entry( fam, etype, type, ARRAY, 1, &name,
                            fam->ti_next_free_byte, outbytes );
   if ( rval != OK )
      return rval;

   /* Write it out... */
   (*fam->write_int)( fam->ti_cur_file, i_buf, order + 1 );
   write_ct  (fam->write_funcs[type])( fam->ti_cur_file, data, (size_t) atoms );
   if (write_ct != atoms)
   {
      return SHORT_READ;
   }

   fam->ti_next_free_byte += outbytes;

   free( i_buf );

   /* Now create entry for the param table. */
   ppr = NEW( Param_ref, "Param table entry - array" );
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
#endif


/*****************************************************************
 * TAG( mc_silo_ti_def_class ) PUBLIC
 *
 * Define class information for a new TI Class.
 */
int
mc_silo_ti_def_class( Famid fam_id, int meshid, int state, int  matid,
                      char *superclass,
                      Bool_type meshvar, Bool_type nodal,
                      char *short_name, char *long_name )
{
   Mili_family *fam;

   fam = fam_list[fam_id];

   if ( meshid >=0 ) /* Ignore fields that are <0 */
      fam->ti_meshid          = meshid;

   if ( matid >=0 )
      fam->ti_matid           = matid;

   if ( state >=0 )
      fam->ti_state           = state;

   if ( meshvar >= 0 )
      fam->ti_meshvar         = meshvar;

   if ( nodal >=0 )
      fam->ti_nodal           = nodal;

   if ( superclass != NULL )
      strcpy(fam->ti_superclass_name, superclass);

   if ( short_name != NULL )
      strcpy(fam->ti_short_name,      short_name);

   if ( long_name != NULL )
      strcpy(fam->ti_long_name,       long_name);

   return OK;
}


/*****************************************************************
 * TAG( mc_set_ti_class ) PUBLIC
 *
 * Define class information for a new TI Class.
 */
int
mc_silo_ti_set_class( Famid fam_id, int meshid, int state, int matid,
                      char *superclass,
                      Bool_type meshvar, Bool_type nodal,
                      char *short_name, char *long_name )
{
   Return_value stat;
   stat = mc_silo_ti_def_class( fam_id, meshid, state, matid, superclass,
                                meshvar, nodal,
                                short_name, long_name );
   return OK;
}


/*****************************************************************
 * TAG( mc_silo_ti_undef_class ) PUBLIC
 *
 * Reset class descriptor information for TI data..
 */
int
mc_silo_ti_undef_class( Famid fam_id )
{
   Return_value stat;
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
 * TAG( mc_silo_ti_savedef_class ) PUBLIC
 *
 * Save the class descriptor information for TI data..
 */
int
mc_silo_ti_savedef_class( Famid fam_id )
{
   Return_value stat;
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
 * TAG( mc_silo_ti_restoredef_class ) PUBLIC
 *
 * Restore the saved class descriptor information for TI data..
 */
int
mc_silo_ti_restoredef_class( Famid fam_id )
{
   Return_value stat;
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
 * TAG( mc_silo_ti_enable ) PUBLIC
 *
 * Enable TI data capability
 */
void
mc_silo_ti_enable(  Famid fam_id )

{
   Mili_family *fam;

   silo_ti_enable = TRUE;
}

/*****************************************************************
 * TAG( mc_silo_ti_enable_only ) PUBLIC
 *
 * Enable TI ONLY data capability
 */
void
mc_silo_ti_enable_only( Famid fam_id )

{
   Mili_family *fam;

   silo_ti_only = TRUE;
}

/*****************************************************************
 * TAG( mc_silo_ti_disable ) PUBLIC
 *
 * Disable TI data capability
 */
void
mc_silo_ti_disable( Famid fam_id )

{
   Mili_family *fam;

   silo_ti_enable = FALSE;
   silo_ti_only   = FALSE;
}

/*****************************************************************
 * TAG( mc_silo_ti_disable_only ) PUBLIC
 *
 * Disable TI ONLY data capability.
 */
void
mc_silo_ti_disable_only( Famid fam_id )

{
   Mili_family *fam;

   silo_ti_only = FALSE;
}

/*****************************************************************
 * TAG( mc_silo_ti_data_found ) PUBLIC
 *
 * Set TI data found flag.
 */
void
mc_silo_ti_data_found ( Famid fam_id )

{
   Mili_family *fam;

   silo_ti_data_found = TRUE;
}

/*****************************************************************
 * TAG( mc_silo_ti_check_if_data_found ) PUBLIC
 *
 * Check if have TI data in this family
 */
Bool_type
mc_silo_ti_check_if_data_found ( Famid fam_id )

{
   Mili_family *fam;

   return ( silo_ti_data_found );
}

/*****************************************************************
 * TAG( mc_silo_ti_make_var_name ) PRIVATE
 *
 * Make a TI name with embedded class info
 */
int
mc_silo_ti_make_var_name( Famid fam_id, char *name, char *class, char *new_name )
{
   Mili_family *fam;
   char node_or_elem[16], meshvar[16];
   char temp_class[64];

   fam = fam_list[fam_id];

   if ( class != NULL )
      strcpy( temp_class, class );
   else
      strcpy( temp_class, fam->ti_short_name );

   if ( fam->ti_meshvar )
      strcpy(meshvar, "TRUE");
   else
      strcpy(meshvar, "FALSE");

   if ( fam->ti_nodal )
      strcpy(node_or_elem, "TRUE");
   else
      strcpy(node_or_elem, "FALSE");

   if ( strlen(fam->ti_superclass_name)==0 || strlen(fam->ti_short_name)==0 )
      strcpy( new_name, name );
   else
      sprintf(new_name, "%s[/Mesh-%d/Sname-%s/++/IsMvar-%s/IsNod-%s/Scls-%s/Mat-%d/St-%d]",
              name, fam->ti_meshid, temp_class,
              meshvar, node_or_elem, fam->ti_superclass_name,
              fam->ti_matid, fam->ti_state );

   return OK;
}


/*****************************************************************
 * TAG( mc_silo_ti_get_class )
 *
 * Search a hash table for an entry and return the fields type and
 * length.
 */
void
mc_silo_ti_get_class( Famid fam_id, int meshid, char *name, char *class,
                      int *state, int *matid,
                      int *superclass,
                      Bool_type *isMeshvar, Bool_type *isNodal,
                      int *datatype, int *datalength )
{
   mc_silo_ti_get_data_attributes( fam_id, meshid, name, class,
                                   state, matid,
                                   superclass,
                                   isMeshvar, isNodal,
                                   datatype, datalength );
}


/*****************************************************************
 * TAG( mc_silo_ti_get_data_attributes )
 *
 * Search a hash table for an entry and return the fields type and
 * length.
 */
Return_value
mc_silo_ti_get_data_attributes( Famid fam_id, int meshid,
                                char *name, char *this_class,
                                int *state, int *matid,
                                int *superclass,
                                Bool_type *isMeshvar, Bool_type *isNodal,
                                int *datatype, int *datalength )
{
   Htable_entry *phte;
   Param_ref *p_pr;
   int       i;
   int       *entry, entry_idx, file;
   int       len_bytes;
   Mili_family *fam;
   char new_name[128], save_name[128], *class;
   int order=3;
   Return_value status;

   fam = fam_list[fam_id];

   status = mc_silo_ti_make_var_name( fam_id, name, this_class, new_name );

   status = ti_htable_search( fam->ti_param_table, new_name,
                              FIND_ENTRY, &phte );

   if ( phte==NULL )
      return status;

   strcpy( save_name, new_name );

   p_pr = ( Param_ref * ) phte->data;

   file      = p_pr->file_index;
   entry_idx = p_pr->entry_index;

   /* Get the type and length of the data. */
   entry        = fam->ti_directory[file].dir_entries[entry_idx];
   len_bytes    = entry[LENGTH_IDX];
   *datatype    = entry[MODIFIER1_IDX];


   /* Get the Mesh Variable flag from the name */

   class = strstr(new_name, "IsMvar" );
   *isMeshvar = FALSE;

   if ( class )
   {
      strcpy(new_name, class);
      for ( i=0;
            i<strlen(new_name);
            i++)
         if ( new_name[i]=='T' && new_name[i+1]=='R')
         {
            *isMeshvar = TRUE;
            break;
         }
   }

   /* Get the nodal/element flag from the name */

   strcpy (new_name, save_name );
   class = strstr(new_name, "IsNod" );
   *isNodal = FALSE;

   if ( class )
   {
      strcpy(new_name, class);
      for ( i=0;
            i<strlen(new_name);
            i++)
         if ( new_name[i]=='T' && new_name[i+1]=='R')
         {
            *isNodal = TRUE;
            break;
         }
   }

   /* Get the Mesh-Id from the name */

   strcpy (new_name, save_name );
   class = strstr(new_name, "St-" );
   if ( class )
   {
      strcpy(new_name, class);
      for ( i=0;
            i<strlen(new_name);
            i++)
         if ( new_name[i]=='/' )
         {
            new_name[i]='\0'; /* Terminate name at end of class name */
            *state = atoi(&class[6]);
            break;
         }
   }

   /* Get the Mat-Id from the name */

   strcpy (new_name, save_name );
   class = strstr(new_name, "Mat-" );
   if ( class )
   {
      strcpy(new_name, class);
      for ( i=0;
            i<strlen(new_name);
            i++)
         if ( new_name[i]=='/' )
         {
            new_name[i]='\0'; /* Terminate name at end of class name */
            *matid = atoi(&class[4]);
            break;
         }
   }

   /* Get the superclass from the name */

   strcpy (new_name, save_name );
   class = strstr(new_name, "M_" );
   if ( class )
   {
      strcpy(new_name, class);
      for ( i=0;
            i<strlen(new_name);
            i++)
         if ( new_name[i]=='/' )
         {
            new_name[i]='\0'; /* Terminate name at end of class name */
            break;
         }
   }

   /* Convert text class name to Mili-class type */
   if ( !strcmp("M_NODE", new_name) )
      *superclass = M_NODE;
   if ( !strcmp("M_TRUSS", new_name) )
      *superclass = M_TRUSS;
   if ( !strcmp("M_BEAM", new_name) )
      *superclass = M_BEAM;
   if ( !strcmp("M_QUAD", new_name) )
      *superclass = M_QUAD;
   if ( !strcmp("M_TET", new_name) )
      *superclass = M_TET;
   if ( !strcmp("M_HEX", new_name) )
      *superclass = M_HEX;
   if ( !strcmp("M_MAT", new_name) )
      *superclass = M_MAT;
   if ( !strcmp("M_SURFACE", new_name) )
      *superclass = M_SURFACE;
   if ( !strcmp("M_TRI", new_name) )
      *superclass = M_TRI;
   if ( !strcmp("M_UNIT", new_name) )
      *superclass = M_UNIT;

   switch (*datatype)
   {
      case (M_INT):
         *datalength = len_bytes/4;
         break;

      case (M_INT8):
         *datalength = len_bytes/8;
         break;

      case (M_STRING):
         *datalength = len_bytes;
         break;

      case (M_FLOAT):
      case (M_FLOAT4):
         *datalength = len_bytes/4;
         break;

      case (M_FLOAT8):
         *datalength = len_bytes/8;
         break;
   }

   if ( *datalength >2 )
      datalength -= (order+1);

   return(OK);
}

/*****************************************************************
 * TAG( mc_silo_ti_get_superclass )
 *
 * return the fields super
 * class type
 */
int
mc_silo_ti_get_superclass_from_name( char *name )
{
   char new_name[128], *class;
   int i;
   int superclass;

   strcpy( new_name, name );

   /* Get the superclass from the name */

   class = strstr(new_name, "M_" );
   if ( class )
   {
      strcpy(new_name, class);
      for ( i=0;
            i<strlen(new_name);
            i++)
         if ( new_name[i]=='/' )
         {
            new_name[i]='\0'; /* Terminate name at end of class name */
            break;
         }
   }

   /* Convert text class name to Mili-class type */
   if ( !strcmp("M_NODE", new_name) )
      superclass = M_NODE;
   if ( !strcmp("M_TRUSS", new_name) )
      superclass = M_TRUSS;
   if ( !strcmp("M_BEAM", new_name) )
      superclass = M_BEAM;
   if ( !strcmp("M_QUAD", new_name) )
      superclass = M_QUAD;
   if ( !strcmp("M_TET", new_name) )
      superclass = M_TET;
   if ( !strcmp("M_HEX", new_name) )
      superclass = M_HEX;
   if ( !strcmp("M_MAT", new_name) )
      superclass = M_MAT;
   if ( !strcmp("M_SURFACE", new_name) )
      superclass = M_SURFACE;
   if ( !strcmp("M_TRI", new_name) )
      superclass = M_TRI;
   if ( !strcmp("M_UNIT", new_name) )
      superclass = M_UNIT;

   return( superclass );
}


/*****************************************************************
 * TAG( mc_silo_ti_get_data_len )
 *
 * Search a hash table for an entry and return the fields type and
 * length.
 */
Return_value
mc_silo_ti_get_data_len( Famid fam_id, char *name, int *datatype, int *datalength )
{
   Htable_entry *phte;
   Param_ref *p_pr;
   int       i;
   int       *entry, entry_idx, file;
   int       len_bytes;
   Mili_family *fam;
   int order=3;
   Return_value rval;

   fam = fam_list[fam_id];

   rval = ti_htable_search( fam->ti_param_table, name, FIND_ENTRY, &phte );

   if ( phte==NULL )
      return rval;

   p_pr = ( Param_ref * ) phte->data;

   file      = p_pr->file_index;
   entry_idx = p_pr->entry_index;

   /* Get the type and length of the data. */
   entry        = fam->ti_directory[file].dir_entries[entry_idx];
   len_bytes    = entry[LENGTH_IDX];
   *datatype    = entry[MODIFIER1_IDX];


   switch (*datatype)
   {
      case (M_INT):
         *datalength =  len_bytes/4;
         break;

      case (M_INT8):
         *datalength = len_bytes/8;
         break;

      case (M_STRING):
         *datalength = len_bytes;
         break;

      case (M_FLOAT):
      case (M_FLOAT4):
         *datalength = len_bytes/8;
         break;

      case (M_FLOAT8):
         *datalength = len_bytes/16;
         break;
   }

   if ( *datalength >2 )
      *datalength -= (order+1);

   return(OK);
}




/* End of mc_silo_ti.c */
