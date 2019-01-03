/* $Id: mc_silo_param.c,v 1.8 2011/05/26 21:43:13 durrenberger1 Exp $ */

/*
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

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  22-Aug-2007 IRC Initial version.                            */
/*                                                              */
/****************************************************************/

/*  Include Files  */
#include "silo.h"
#include "mili_internal.h"
#include "MiliSilo_Internal.h"
#include "SiloLib.h"

#include <sys/types.h>

/*****************************************************************
 * TAG( mc_silo_read_scalar ) PUBLIC
 *
 * Read a named scalar parameter from the referenced family.
 */
Return_value
mc_silo_read_scalar( Famid famid, char *name, void *p_value )
{
   Mili_family *fam;
   Htable_entry *phte;
   Return_value status = OK;


#ifdef AAA

   if ( name == NULL || *name == '\0' )
      return INVALID_NAME;

   if ( fam->param_table == NULL )
      return ENTRY_NOT_FOUND;

   status = htable_search( fam->param_table, name, FIND_ENTRY, &phte );

#endif

   return ( status );
}


/*****************************************************************
 * TAG( silo_read_scalar ) PRIVATE
 *
 * Read a scalar numeric parameter from the referenced family.
 */
Return_value
silo_read_scalar( Mili_family *fam, Param_ref *p_pr, void *p_value )
{
   int file, entry_idx;
   long offset;
   Return_value rval;
   size_t nitems;
   int *entry;
   int numtype;

   /* Get directory entry. */
   file = p_pr->file_index;
   entry_idx = p_pr->entry_index;

   /* Get file offset and length of data. */
   entry = fam->directory[file].dir_entries[entry_idx];
   offset = entry[OFFSET_IDX];
   numtype = entry[MODIFIER1_IDX];

   rval = non_state_file_open( fam, file, fam->access_mode );
   if ( rval != OK )
      return rval;

   rval = non_state_file_seek( fam, offset );
   if ( rval != OK )
      return rval;

   /* Read the data. */
   nitems = fam->read_funcs[numtype]( fam->cur_file, p_value, 1 );

   return ( nitems == 1 ) ? OK : SHORT_READ;
}


/*****************************************************************
 * TAG( mc_silo_wrt_scalar ) PUBLIC
 *
 * Write a scalar numeric parameter into the referenced family.
 */
Return_value
mc_silo_wrt_scalar( Famid famid, int type, char *name, void *p_value )
{
   Mili_family *fam;
   int status;

#ifdef AAA

   if ( name == NULL || *name == '\0' )
      return INVALID_NAME;
   else if ( fam->access_mode == 'r' )
      return BAD_ACCESS_TYPE;

   if ( fam->param_table == NULL )
   {
      fam->param_table = htable_create( DEFAULT_HASH_TABLE_SIZE );
      if (fam->param_table == NULL)
      {
         return ALLOC_FAILED;
      }
      +

      return write_scalar( fam, type, name, p_value, APPLICATION_PARAM );
#endif
      return ( status );
   }


   /*****************************************************************
    * TAG( silo_write_scalar ) PRIVATE
    *
    * Write a scalar numeric parameter into the referenced family.
    */
   Return_value
   silo_write_scalar( Mili_family *fam, int type, char *name, void *data,
                      Dir_entry_type etype )
   {
      size_t outbytes;
      Return_value rval;
      Param_ref *ppr;
      int fidx;
      Htable_entry *phte;
      int *entry;
      int prev_index;
      size_t write_ct;

      /* See if entry already exists. */
      rval = htable_search( fam->param_table, name, ENTER_MERGE, &phte );
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
               fidx = fam->cur_index;
               ppr->file_index = fidx;
               ppr->entry_index = fam->directory[fidx].qty_entries - 1;
               phte->data = (void *) ppr;

               /* Write the scalar. */
               write_ct = (fam->write_funcs[type])( fam->cur_file, data, 1 );
               if (write_ct != 1)
               {
                  free(ppr);
                  rval = SHORT_READ;
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
            /* Save id of any extant open file for use below. */
            prev_index = fam->cur_index;

            /* Open in append mode to avoid truncating file. */
            non_state_file_open( fam, ppr->file_index, 'a' );
            non_state_file_seek( fam, (long) entry[OFFSET_IDX] );

            write_ct = (fam->write_funcs[type])( fam->cur_file, data, (size_t) 1 );
            if (write_ct != 1)
            {
               rval = SHORT_READ;
            }

            /*
             * This was a re-write.  If the destination file wasn't already
             * open, close it to ensure changes are flushed to disk.
             */
            if ( prev_index != fam->cur_index )
               non_state_file_close( fam );
            else if ( fam->cur_file != NULL )
               /* Leave file pointer at end of file. */
               fseek( fam->cur_file, 0, SEEK_END );

            rval = OK;
         }
         else
            /* Type mismatch; re-write error. */
            rval = NUM_TYPE_MISMATCH;
      }

      return rval;
   }


   /*****************************************************************
    * TAG( mc_silo_read_string ) PUBLIC
    *
    * Read a string parameter from the referenced Silo file.
    */
   Return_value
   mc_silo_read_string( Famid famid, char *name, char *p_value )
   {
      Mili_family *fam;
      Htable_entry *phte;
      Return_value status = OK;

#ifdef AAA
      if ( name == NULL || *name == '\0' )
         return INVALID_NAME;

      if ( fam->param_table == NULL )
         return ENTRY_NOT_FOUND;

      status = htable_search( fam->param_table, name, FIND_ENTRY, &phte );
      if ( phte == NULL )
         return status;

      return read_string( fam, (Param_ref *) phte->data, p_value );
#endif

      return ( status );
   }


   /*****************************************************************
    * TAG( silo_read_string ) PRIVATE
    *
    * Read a string parameter from the referenced family.
    */
   Return_value
   silo_read_string( Mili_family *fam, Param_ref *p_pr, char *p_value )
   {
      int file, entry_idx;
      long offset;
      Return_value rval;
      size_t nitems, length;
      int *entry;

      /* Get directory entry. */
      file = p_pr->file_index;
      entry_idx = p_pr->entry_index;

      /* Get file offset and length of data. */
      entry = fam->directory[file].dir_entries[entry_idx];

      if ( entry[MODIFIER1_IDX] != M_STRING )
         return NOT_STRING_TYPE;

      offset = entry[OFFSET_IDX];
      length = entry[LENGTH_IDX];

      rval = non_state_file_open( fam, file, fam->access_mode );
      if ( rval != OK )
         return rval;

      rval = non_state_file_seek( fam, offset );
      if ( rval != OK )
         return rval;

      /* Read the data. */
      nitems = fam->read_char( fam->cur_file, p_value, length );

      return ( nitems == length ) ? OK : SHORT_READ;
   }


   /*****************************************************************
    * TAG( mc_silo_wrt_string ) PUBLIC
    *
    * Write a string parameter into the referenced family.
    */
   int
   mc_silo_wrt_string( Famid famid, char *name, char *value )
   {
      int len;
      int status;

      int    cycle=0, dims=1;
      double time=0.0;

      if ( name == NULL || *name == '\0' )
         return INVALID_NAME;

      len = strlen(value);

      status = SiloLib_Write_Var (famid,
                                  SILO_PLOTFILE,
                                  name,
                                  time,
                                  cycle,
                                  dims,
                                  len,
                                  DB_CHAR,
                                  value) ;

      return ( status );
   }


   /*****************************************************************
    * TAG( mc_silo_read_param_array ) PUBLIC
    *
    * Read a parameter array from the referenced family.
    */
   Return_value
   mc_silo_read_param_array( Famid famid, char *name, void **p_data )
   {
      Mili_family *fam;
      Htable_entry *phte;
      Return_value status = OK;

#ifdef AAA
      if ( name == NULL || *name == '\0' )
         return INVALID_NAME;

      if ( fam->param_table == NULL )
         return ENTRY_NOT_FOUND;

      status = htable_search( fam->param_table, name, FIND_ENTRY, &phte );
      if ( phte == NULL )
         return status;

      return read_param_array( fam, (Param_ref *) phte->data, p_data );
#endif

      return ( status );
   }


   /*****************************************************************
    * TAG( silo_read_param_array ) PRIVATE
    *
    * Read a parameter array from the referenced family.
    */
   Return_value
   silo_read_param_array( Mili_family *fam, Param_ref *p_pr, void **p_data )
   {
      int file, entry_idx;
      int cell_qty;
      int i;
      long offset;
      size_t nitems, length;
      int num_type;
      int *dir_ent;
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
      length   = dir_ent[LENGTH_IDX];
      num_type = dir_ent[MODIFIER1_IDX];

      rval = non_state_file_open( fam, file, fam->access_mode );
      if ( rval != OK )
         return rval;

      rval = non_state_file_seek( fam, offset );
      if ( rval != OK )
         return rval;

      /* Read the data. */

      /* Read the array rank. */
      nitems = (fam->read_funcs[M_INT])( fam->cur_file, &p_pr->rank, 1 );

      if ( nitems != 1 )
         return SHORT_READ;

      p_pr->dims = NEW_N( int, p_pr->rank, "Array dimensions bufr" );

      nitems = (fam->read_funcs[M_INT])( fam->cur_file, p_pr->dims, (size_t) p_pr->rank );
      if ( nitems != (size_t)p_pr-> rank )
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
            p_data[0] = (void *)idata;
            nitems = (fam->read_funcs[M_INT])( fam->cur_file, idata, (size_t) cell_qty );
            length/=4;
            break;

         case M_INT8:
            lidata = NEW_N( LONGLONG, cell_qty, "Param array bufr" );
            p_data[0] = (void *)lidata;
            nitems = (fam->read_funcs[M_INT8])( fam->cur_file, lidata, (size_t) cell_qty );
            length/=8;
            break;

         case M_FLOAT:
         case M_FLOAT4:
            fdata = NEW_N( float, cell_qty, "Param array bufr" );
            p_data[0] = (void *)fdata;
            nitems = (fam->read_funcs[M_FLOAT])( fam->cur_file, fdata, (size_t) cell_qty );
            length=(length/4)-2;
            break;

         case M_FLOAT8:
            ddata = NEW_N( double, cell_qty, "Param array bufr" );
            p_data[0] = (void *)ddata;
            nitems = (fam->read_funcs[M_FLOAT8])( fam->cur_file, ddata, (size_t) cell_qty );
            length/=8;
            break;

      }


      return ( nitems == cell_qty ) ? OK : SHORT_READ;
   }

   /*****************************************************************
    * TAG( mc_silo_wrt_array ) PUBLIC
    *
    * Write an array parameter into the referenced family.
    */
   int
   mc_silo_wrt_array( Famid famid, int type, char *name, int order,
                      int *dimensions, void *data )
   {
      Mili_family *fam;
      int status;

      if ( name == NULL || *name == '\0' )
         return INVALID_NAME;

#ifdef AAA

      return write_array( fam, type, name, order, dimensions, data,
                          APPLICATION_PARAM );
#endif

      return ( status );
   }

   /*****************************************************************
    * TAG( silo_write_array ) PRIVATE
    *
    * Write a parameter array into the referenced family.
    */
   Return_value
   silo_write_array( Mili_family *fam, int type, char *name, int order,
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

      /* Ensure file is open and positioned. */
      if ( (rval = prep_for_new_data( fam, NON_STATE_DATA )) != OK )
         return rval;

      /* Calc number of entries in array. */
      for ( i = 0, atoms = 1; i < order; i++ )
         atoms *= dimensions[i];

      /* Load integer descriptors into an output buffer. */
      i_buf = NEW_N( int, order + 1, "App array descr buf" );
      i_buf[0] = order;
      for ( i = 0; i < order; i++ )
         i_buf[i + 1] = dimensions[i];

      /* Add entry into directory. */
      outbytes = ((order + 1) * EXT_SIZE( fam, M_INT ))
                 + atoms * EXT_SIZE( fam, type );
      rval = add_dir_entry( fam, etype, type, ARRAY, 1, &name,
                            fam->next_free_byte, outbytes );
      if ( rval != OK )
         return rval;

      /* Write it out... */
      (*fam->write_int)( fam->cur_file, i_buf, order + 1 );
      write_ct = (fam->write_funcs[type])( fam->cur_file, data, (size_t) atoms );
      if (write_ct != atoms)
      {
         return SHORT_READ;
      }

      fam->next_free_byte += outbytes;

      free( i_buf );

      /* Now create entry for the param table. */
      ppr = NEW( Param_ref, "Param table entry - array" );
      fidx = fam->cur_index;
      ppr->file_index = fidx;
      ppr->entry_index = fam->directory[fidx].qty_entries - 1;
      rval = htable_add_entry_data( fam->param_table, name, ENTER_ALWAYS, ppr );
      if (rval != OK)
      {
         free(ppr);
      }

      return rval;
   }


   /* End of mc_silo_param.c */
