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
 
 * Routines for managing state records and formats.
 *
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - March 20, 2009: Added support for WIN32 for use with
 *                with Visit.
 *                See SCR #587.
 *
 *  I. R. Corey - February 11, 2010: Fixed error in de-referencing agg_type
 *                in translate_reference().
 *                See SCR #661.
 *
 *  I. R. Corey - February 19, 2010: Changed default for max states per
 *                file - was too low for TH runs. Fixed problem with
 *                not initializing fam->bytes_per_file_current when
 *                state limit reached.
 *                SCR #663.
 *
 *  I. R. Corey - April 30, 2010: Incorporated changes from Sebastian
 *                at IABG to support building under Windows.
 *                SCR #678.
 *
 *  I. R. Corey - September 27, 2011: Added several new read functions to
 *                support regression testing tools.
 *
 ************************************************************************
 */

#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#endif
#include <string.h>
#include "mili_internal.h"
#include "eprtf.h"

#define RANGE_QTY (1000)
#define DFLT_BUF_COUNT 2

char *org_names[] =
{
   "Result", "Object"
};

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

extern Bool_type filelock_enable;

/*****************************************************************
 * TAG( fam_array_length )
 *
 * The length of fam_list.
 */
extern int fam_array_length;

Bool_type zero_subrec_notified =0;

static Return_value get_ro_svars( Mili_family *fam, int state,
                                  Sub_srec *p_subrec, int qty,
                                  Translated_ref *refs, void **p_out );
static Return_value get_oo_svars( Mili_family *fam, int state,
                                  Sub_srec *p_subrec, int qty,
                                  Translated_ref *refs, void **p_out );
static Return_value translate_reference( Sub_srec *p_subrec, char *result,
                                         Svar **pp_svar, 
                                         Translated_ref *p_tref );      
static Return_value map_subset_spec( Svar *p_svar, int indices[],
                                     int index_qty, char *component,
                                     int comp_index, char *sub_component, 
                                     int sub_comp_index, Translated_ref *p_tref );
static void (*get_data_dist_func( int data_type, int length,
                                  int cell_size ))();
static void distribute_all_4( void *p_input, int cell_qty, int cell_size,
                              int cell_offset, int length, void *p_output );
static void distribute_all_8( void *p_input, int cell_qty, int cell_size,
                              int cell_offset, int length, void *p_output );
static void distribute_scalar_4( void *p_input, int cell_qty, int cell_size,
                                 int cell_offset, int length, void *p_output );
static void distribute_scalar_8( void *p_input, int cell_qty, int cell_size,
                                 int cell_offset, int length, void *p_output );
static void distribute_vector_4( void *p_input, int cell_qty, int cell_size,
                                 int cell_offset, int length, void *p_output );
static void distribute_vector_8( void *p_input, int cell_qty, int cell_size,
                                 int cell_offset, int length, void *p_output );
static Return_value add_srec( Mili_family *fam, int index, Dir_entry dir_ent );
static Return_value def_subrec_from_db( Mili_family *fam, int srec_id,
                                        int *i_data, char *c_data,
                                        int *c_data_len );
static Return_value set_metrics( Mili_family *fam, int data_org,
                                 Sub_srec *psubrec,
                                 Mesh_object_class_data *p_mocd, LONGLONG *size );
static Return_value get_start_index( Mili_family *fam );
static Return_value truncate_family( Mili_family *p_fam, int st_index );
static int find_file_index(Famid fam_id,int *global_state_index);
static int svar_atom_qty( Svar *p_svar );


/*****************************************************************
 * TAG( set_subrec_check ) 
 *
 * Set the check for start of subrec check.
 */

Return_value
mc_set_subrec_check(Famid fam_id, Bool_type check)
{
   Mili_family *fam;
   
   fam = fam_list[fam_id];
   
   fam->subrec_start_check = check;
   
   return OK;
   
}

Return_value
mc_check_subrec_start(Famid fam_id, int srec_id)
{
   /*typedef struct _srec
{
   int qty_subrecs;
   Sub_srec **subrecs;
   LONGLONG size;
   Db_object_status status;
} Srec;*/
   Mili_family *fam;
   Srec *psr;
   LONGLONG offset;
   int count, pos, middle, top; 
   
   fam = fam_list[fam_id];
   if(!fam->subrec_start_check)
   {
      return OK;
   }
   
   psr = fam->srecs[srec_id];
   
   if( fam->cur_st_file == NULL)
   {
      return STATE_NOT_INSTATIATED;
   }
   
   offset = ftell(fam->cur_st_file) - fam->state_map[fam->state_qty-1].offset -8;
   if(offset == psr->size)
   {
       return OK;
   }
   count = psr->qty_subrecs;
   
   if(psr->subrecs[0]->offset == offset || psr->subrecs[count-1]->offset == offset)
   {
      return OK;
   }
   top = count -1;
   pos = 1;
   middle = (top+pos)/2;
   
   do
   {
      if(psr->subrecs[middle]->offset == offset)
      {
         return OK;
      }
      
      if(offset > psr->subrecs[middle]->offset  )
      {
         pos = middle+1;
      }else
      {
         top = middle-1; 
      }
      if (top == pos)
      {
         if(psr->subrecs[top]->offset == offset)
         {
            return OK;
         }
      }
      middle = (top + pos)/2;
   }while(pos < top);
   
   return SUBRECORD_ALIGN_ERROR;
}
/*****************************************************************
 * TAG( make_srec ) LOCAL
 *
 * Create an uninitialized state record descriptor.
 */
static Return_value
make_srec( Mili_family *fam, int mesh_id, int *srec_id )
{
   Srec *p_sr;
   Return_value rval;

   /*
    * Grow arrays to hold a pointer to the new srec descriptor
    * and a pointer to its associated mesh descriptor.
    */
   fam->srecs = RENEW_N( Srec *, fam->srecs, fam->qty_srecs, 1,
                         "Srec pointer" );
   fam->srec_meshes = RENEW_N( Mesh_descriptor *, fam->srec_meshes,
                               fam->qty_srecs, 1, "Srec mesh pointer" );

   /* Allocate the state record descriptor. */
   p_sr = NEW( Srec, "State record format" );

   if (fam->srecs != NULL && fam->srec_meshes != NULL && p_sr != NULL)
   {
      /* Store addresses of the srec descriptor and its mesh descriptor. */
      fam->srecs[fam->qty_srecs] = p_sr;
      fam->srec_meshes[fam->qty_srecs] = fam->meshes[mesh_id];

      /* Ident is just the (previous) count. */
      *srec_id = fam->qty_srecs;
      fam->qty_srecs++;
      rval = OK;
   }
   else
   {
      rval = ALLOC_FAILED;
   }

   return rval;
}



/*****************************************************************
 * TAG( mc_open_srec ) PUBLIC
 *
 * Interface to make_srec() which performs QC prior to instantiation.
 */
Return_value
mc_open_srec( Famid fam_id, int mesh_id, int *p_srec_id )
{
   Mili_family *fam;
   Mesh_descriptor *mesh;
   Mesh_object_class_data **pp_mocd;
   Mesh_object_class_data *p_mocd;
   int i, class_qty;
   Return_value rval;

   fam = fam_list[fam_id];

   if ( mesh_id >= fam->qty_meshes )
   {
      return NO_MESH;
   }

   mesh = fam->meshes[mesh_id];

   /* Verify that there are no holes in defined node/element classes. */
   rval = htable_get_data( mesh->mesh_data.umesh_data,
                           (void ***) &pp_mocd, &class_qty );
   if (rval != OK)
   {
      return rval;
   }
   for ( i = 0; i < class_qty; i++ )
   {
      p_mocd = pp_mocd[i];

      if ( p_mocd->superclass != M_MESH )
      {
         if ( p_mocd->blocks->block_qty != 1 )
         {
            free( pp_mocd );
            return INCOMPLETE_MESH_DEF;
         }
      }
   }
   free( pp_mocd );

   return make_srec( fam, mesh_id, p_srec_id );
}




/*****************************************************************
 * TAG( mc_close_srec ) PUBLIC
 *
 * Set condition of a state record format to preclude addition of
 * further subrecords and indicate availability for output.
 */
Return_value
mc_close_srec( Famid fam_id, int srec_id )
{
   Mili_family *fam;
   Srec *p_srec;

   fam = fam_list[fam_id];

   if ( fam->qty_meshes == 0 )
   {
      return NO_MESH;
   }
   if ( srec_id < 0 || srec_id >= fam->qty_srecs )
   {
      return INVALID_SREC_INDEX;
   }

   p_srec = fam->srecs[srec_id];

   if ( p_srec->qty_subrecs == 0 )
   {
      p_srec->status = OBJ_EMPTY;
   }
   else
   {
      p_srec->status = OBJ_CLOSED;

      /* Indicate there's data ready for output. */
      fam->non_state_ready = TRUE;
   }
   
   return mc_write_mili_metadata(fam_id);
   
}


/*****************************************************************
 * TAG( mc_def_subrec ) PUBLIC
 *
 * Define a state record sub-record, binding a collection of
 * mesh objects of one type to a set of state variables.
 *
 * Assumes unstructured mesh type (c/o mesh object id checking).
 */
Return_value
mc_def_subrec( Famid fam_id, int srec_id, char *subrec_name, int org,
               int qty_svars, char *svar_names, int name_stride,
               char *mclass, int format, int cell_qty, int *mo_ids, int *flag )
{
   int i,count=0,pos,error=0;
   Mili_family *fam;
   Return_value status;
   Srec *psr;
   int *pib, *bound, *psrc, *pdest;
   Sub_srec *psubrec;
   Hash_action op;
   Htable_entry *subrec_entry, *svar_entry, *class_entry;
   
   Svar **svars;
   char *sv_name;
   int blk_qty;
   int stype;
   LONGLONG sub_size;

   Mesh_object_class_data *p_mocd;
   Svar **ppsvars;
   int local_srec_id;
   int mesh_id;
   int superclass;
   Return_value rval;
   int svar_atoms;
   int *surface_flag;

   if ( format == M_BLOCK_OBJ_FMT ) {
      for (i=0; i<cell_qty; i++)
      {
         pos = i*2;
         if(mo_ids[pos] <=  mo_ids[pos+1])
         {
            count += mo_ids[pos+1]-mo_ids[pos]+1;
         }else
         {
            error = 1;
         }
      }
   } else {
      count = cell_qty;
   }
   if(error || count <=0)
   {
      if (!zero_subrec_notified)
      {
         fprintf(stderr,"Mili warning: Subrecord mili object ids must have a count >= 1.\n");
         fprintf(stderr,"Subrecord creation ignored.  This warning will not be repeated\n");
         zero_subrec_notified=1;
      }
      return OK;
   }

   fam = fam_list[fam_id];

   if ( fam->qty_meshes == 0 )
   {
      return NO_MESH;
   }
   
   if ( srec_id < 0 || srec_id >= fam->qty_srecs )
   {
      return INVALID_SREC_INDEX;
   }
   
   if ( fam->srecs[srec_id]->status != OBJ_OPEN )
   {
      return CANNOT_MODIFY_OBJ;
   }
   
   if ( qty_svars == 0 )
   {
      return NO_SVARS;
   }

   if ( org!=OBJECT_ORDERED && org!=RESULT_ORDERED )
   {
      return UNKNOWN_ORGANIZATION;
   }

   local_srec_id = srec_id;
   rval = mc_query_family( fam_id, SREC_MESH, &local_srec_id, NULL,
                           (void *)&mesh_id );

   if ( OK  !=  rval )
   {
      return rval;
   }

   rval = mc_query_family( fam_id, CLASS_SUPERCLASS, &mesh_id, mclass,
                           (void *)&superclass );

   if ( OK  !=  rval )
   {
      return rval;
   }

   if ( superclass == M_SURFACE )
   {
      if ( 1  !=  cell_qty )
      {
         return INVALID_DATA_TYPE;
      }
   }


   /* Create the subrecord hash table if it hasn't been. */
   if ( fam->subrec_table == NULL )
   {
      fam->subrec_table = htable_create( DEFAULT_HASH_TABLE_SIZE );
      if (fam->subrec_table == NULL)
      {
         return ALLOC_FAILED;
      }
   }

   /* Create an entry in the hash table. */
   if( strncmp( subrec_name, "sand", 4 ) == 0 )
   {
      op = ENTER_ALWAYS;
   }
   else
   {
      op = ENTER_UNIQUE;
   }

   status = htable_search( fam->subrec_table, subrec_name,
                           op, &subrec_entry );
   if ( subrec_entry == NULL )
   {
      return status;
   }

   /* Got into the hash table, so create the subrec. */
   psubrec = NEW( Sub_srec, "State subrecord" );
   if (psubrec == NULL)
   {
      return ALLOC_FAILED;
   }
   str_dup( &psubrec->name, subrec_name );
   if (psubrec->name == NULL)
   {
      return ALLOC_FAILED;
   }
   psubrec->organization = org;
   psubrec->qty_svars = qty_svars;
   svars = NEW_N( Svar *, qty_svars, "Svar array" );
   if (qty_svars > 0 && svars == NULL)
   {
      free(psubrec);
      return ALLOC_FAILED;
   }
   psubrec->svars = svars;
   subrec_entry->data = (void *) psubrec;

   /* Ensure all the referenced state variables exist. */
   for ( i = 0, sv_name = svar_names; i < qty_svars;
         sv_name += name_stride, i++ )
   {
      status = htable_search( fam->svar_table, sv_name,
                              FIND_ENTRY, &svar_entry );
      if (svar_entry == NULL)
      {
         /* Variable not found.  Free all resources and return. */
         delete_subrec( (void *) psubrec );
         htable_del_entry( fam->subrec_table, subrec_entry );
         return status;
      }
      else
      {
         /* Save reference to variable in subrec. */
         svars[i] = (Svar *) svar_entry->data;
      }
   }

   if ( org == OBJECT_ORDERED )
   {
      /* For object-ordered data, base types must all be the same. */
      for ( i = 1; i < qty_svars; i++ )
      {
         /* Compare data types of consecutive svars. */
         if ( *svars[i]->data_type != *svars[0]->data_type )
         {
            /* Bad news; clean-up and return. */
            delete_subrec( (void *) psubrec );
            htable_del_entry( fam->subrec_table, subrec_entry );
            return TOO_MANY_TYPES;
         }
      }
   }else
   {
      /* For result-ordered data,  agg_type cannot be vector array. */
      for ( i = 0; i < qty_svars; i++ )
      {
         /* Compare data types of consecutive svars. */
         if ( *svars[i]->agg_type == VEC_ARRAY )
         {
            /* Bad news; clean-up and return. */
            delete_subrec( (void *) psubrec );
            htable_del_entry( fam->subrec_table, subrec_entry );
            return SVAR_VEC_ARRAY_ORG_MISMATCH;
         }
      }
   }

   /* Get the referenced object class. */
   status = htable_search( fam->srec_meshes[srec_id]->mesh_data.umesh_data,
                           mclass, FIND_ENTRY, &class_entry );
   if ( class_entry == NULL )
   {
      delete_subrec( (void *) psubrec );
      htable_del_entry( fam->subrec_table, subrec_entry );
      return status;
   }

   p_mocd = (Mesh_object_class_data *) class_entry->data;
   stype = p_mocd->superclass;
   str_dup( &psubrec->mclass, mclass );
   
   if (psubrec->mclass == NULL)
   {
      return ALLOC_FAILED;
   }

   if ( stype != M_MESH )
   {
      /* If format is 1D array of element numbers, put into block format. */
      if ( format == M_LIST_OBJ_FMT )
      {
         status = list_to_blocks( cell_qty, mo_ids, &pib, &blk_qty );
         if (status != OK)
         {
            return status;
         }
         count = cell_qty;
      }
      else if ( format == M_BLOCK_OBJ_FMT )
      {
         /* Allocate space for the ident blocks in the subrecord. */
         pib = NEW_N( int, cell_qty * 2, "Subrec id blks" );
         if (cell_qty > 0 && pib == NULL)
         {
            return ALLOC_FAILED;
         }

         /* Copy the ident blocks, counting idents. */
         count = 0;
         bound = ((int *) mo_ids) + 2 * cell_qty;
         for ( psrc = (int *) mo_ids, pdest = pib;
               psrc < bound;
               pdest += 2, psrc += 2 )
         {
            pdest[0] = psrc[0];
            pdest[1] = psrc[1];
            count += pdest[1] - pdest[0] + 1;
         }

         blk_qty = cell_qty;
      }
      else
      {
         delete_subrec( (void *) psubrec );
         htable_del_entry( fam->subrec_table, subrec_entry );
         return UNKNOWN_ELEM_FMT;
      }

      /* Ensure the objects referenced in the id blocks are in the mesh. */
      status = check_object_ids( p_mocd->blocks, blk_qty, pib );
      if ( status != OK )
      {
         delete_subrec( (void *) psubrec );
         htable_del_entry( fam->subrec_table, subrec_entry );
         free( pib );
         return status;
      }

      psubrec->qty_id_blks = blk_qty;
      psubrec->mo_id_blks = pib;
      psubrec->mo_qty = count;
   }
   else
   {
      psubrec->mo_qty = 1;
   }


   /*
    * Processing of surface subrecord data, as required
    */

   local_srec_id = srec_id;
   rval = mc_query_family( fam_id, SREC_MESH, &local_srec_id, NULL,
                           (void *)&mesh_id );
   if ( OK  !=  rval )
   {
      return( rval );
   }

   rval = mc_query_family( fam_id, CLASS_SUPERCLASS, &mesh_id, mclass,
                           (void *) &superclass );
   if ( OK != rval )
   {
      return( rval );
   }

   if ( M_SURFACE == superclass )
   {
      /*
       * NOTE:  "flag" data is copied to "surface_flag" to relinquish
       *        user "control" over "flag's" (possibly) allocated memory.
       */

      ppsvars = psubrec->svars;
      svar_atoms = svar_atom_qty( *ppsvars );

      if ( flag == NULL )
      {
         /*
          * establish surface default mode basis of PER_OBJECT
          */

         surface_flag = NEW_N( int, svar_atoms,
                               "Surface variable display buffer" );
         if (svar_atoms > 0 && surface_flag == NULL)
         {
            return ALLOC_FAILED;
         }

         for ( i = 0; i < svar_atoms; i++ )
         {
            surface_flag[ i ] = PER_OBJECT;
         }
      }
      else
      {
         /*
          * test validity of surface "flag" parameters
          */
         for ( i = 0; i < svar_atoms; i++ )
         {
            if ( flag[ i ] != PER_OBJECT  &&  flag[ i ] != PER_FACET )
            {
               return INVALID_DATA_TYPE;
            }
         }

         surface_flag = NEW_N( int, svar_atoms,
                               "Surface variable display buffer" );
         if (svar_atoms > 0 && surface_flag == NULL)
         {
            return ALLOC_FAILED;
         }
         memcpy( (void *)surface_flag, (const void *)flag,
                 svar_atoms * sizeof( int ) );
      }

      psubrec->surface_variable_flag = surface_flag;
   }
   else
   {
      psubrec->surface_variable_flag = NULL;
   } /* if M_SURFACE */


   /* Calculate positioning metrics. */
   status = set_metrics( fam, org, psubrec, p_mocd, &sub_size );
   if (status != OK)
   {
      return status;
   }

   /* Init input queue for Object-ordered subrecords. */
   if ( org == OBJECT_ORDERED )
   {
      psubrec->ibuffer = create_buffer_queue( DFLT_BUF_COUNT, sub_size );
      if (psubrec->ibuffer == NULL)
      {
         return ALLOC_FAILED;
      }
   }

   /* Offset to subrecord is previous record size. */
   psr = fam->srecs[srec_id];
   psubrec->offset = psr->size;

   /* Update the record size. */
   psr->size += sub_size;

   /* Save a reference in order of its creation. */
   psr->subrecs = RENEW_N( Sub_srec *, psr->subrecs, psr->qty_subrecs, 1,
                           "Srec data order incr" );
   if (psr->subrecs == NULL)
   {
      return ALLOC_FAILED;
   }
   psr->subrecs[psr->qty_subrecs] = psubrec;
   psr->qty_subrecs++;

   return OK;
}


/*****************************************************************
 * TAG( list_to_blocks ) PRIVATE
 *
 * Transform a 1D array of numbers into a 2D array of ranges,
 * maintaining the order of the numbers (2D array is managed as
 * a 1D array for convenience).
 *
 * Dynamically allocated 2D array must be freed by calling routine.
 */
Return_value
list_to_blocks( int size, int *list, int **dest, int *blk_cnt )
{
   struct range_list
   {
      struct range_list *next;
      struct range_list *prev;
      int ranges[RANGE_QTY][2];
   } *tmp_list, *prl;
   int *p_range;
   int i, j;
   int int_qty, remainder, cnt;
   int *p_blk_list, *pi;

   /*
    * Pass through the input list and save the ranges (in RANGE_QTY size
    * sets for efficiency) in a linked list.
    */
   *blk_cnt = 0;
   i = 0;
   j = 0;
   tmp_list = NEW( struct range_list, "Range list" );
   if (tmp_list == NULL)
   {
      return ALLOC_FAILED;
   }
   prl = tmp_list;
   p_range = &prl->ranges[0][0];
   int_qty = RANGE_QTY * 2;
   while ( i < size )
   {
      /* Get first number in range. */
      p_range[j] = list[i++];

      /* Scan through rest of range. */
      while ( i < size
              && list[i] == list[i - 1] + 1 )
      {
         i++;
      }

      /* Get last number in range. */
      p_range[j + 1] = list[i - 1];

      /*
       * Increment destination index and get a new linked-list buffer
       * if current one is full.
       */
      j += 2;
      if ( j >= int_qty && i < size )
      {
         prl = NEW( struct range_list, "Range list" );
         if (prl == NULL)
         {
            return ALLOC_FAILED;
         }
         APPEND( prl, tmp_list );
         p_range = &prl->ranges[0][0];
         *blk_cnt += j / 2;
         j = 0;
      }
   }
   if ( j != 0 )
   {
      *blk_cnt += j / 2;
   }

   /* Allocate a buffer to hold the block ranges contiguously. */
   p_blk_list = NEW_N( int, *blk_cnt * 2, "Block list" );
   if (*blk_cnt > 0 && p_blk_list == NULL)
   {
      return ALLOC_FAILED;
   }

   /* Traverse the linked-list and load the buffer. */
   remainder = *blk_cnt;
   i = 0;
   pi = p_blk_list;
   for ( prl = tmp_list; prl != NULL; prl = prl->next )
   {
      cnt = ( remainder < RANGE_QTY ) ? remainder : RANGE_QTY;
      for ( j = 0; j < cnt; j++ )
      {
         *pi++ = prl->ranges[j][0];
         *pi++ = prl->ranges[j][1];
      }
      remainder -= cnt;
   }

   /* Free the linked list. */
   DELETE_LIST( tmp_list );

   /* Pass back the block list. */
   *dest = p_blk_list;

   return OK;
}


/*****************************************************************
 * TAG( set_metrics ) LOCAL
 *
 * Calculate lump sizes and offsets for a subrecord plus the
 * subrecord's total length.
 */
static Return_value
set_metrics( Mili_family *fam, int data_org, Sub_srec *psubrec,
             Mesh_object_class_data *p_mocd, LONGLONG *size )
{
   int i, j, k, l;
   int superclass;
   int count;
   int atom_size;
   int atoms;
   Svar **ppsvar, **bound;
   LONGLONG *lump_offsets, *lump_sizes;
   int *lump_atoms;
   int total_atoms;
   int surface_facet_qty;
   int id;


   /* Skip it if already initialized. */
   if ( psubrec->lump_sizes != NULL )
   {
      if ( data_org == OBJECT_ORDERED )
      {
         *size = (LONGLONG) psubrec->lump_sizes[0] * psubrec->mo_qty;
         return OK;
      }
      else
      {
         k = psubrec->qty_svars - 1;
         *size = (LONGLONG) psubrec->lump_offsets[k] + psubrec->lump_sizes[k];
         return OK;
      }
   }

   count = 0;
   *size = 0;
   superclass = p_mocd->superclass;

   /* Calculate lump sizes on basis of gross data organization. */
   if ( data_org == OBJECT_ORDERED )
   {
      /* For object ordering, only one lump size: atom count. */
      psubrec->lump_atoms = NEW( int, "Lump atom array (1)" );
      psubrec->lump_sizes = NEW( LONGLONG, "Lump size array (1)" );
      if (psubrec->lump_atoms == NULL || psubrec->lump_sizes == NULL)
      {
         return ALLOC_FAILED;
      }

      /* Traverse the state vars in the state vector. */
      atom_size = EXT_SIZE( fam, *psubrec->svars[0]->data_type );
      bound = psubrec->svars + psubrec->qty_svars;
      for ( ppsvar = psubrec->svars, i = 0; ppsvar < bound; ppsvar++, i++ )
      {
         /*
          * Only need to distinguish array-valued vs. scalar
          * since base-type must all be the same for object-ordered
          * data.
          */
         atoms = svar_atom_qty( *ppsvar );
         total_atoms = 0;

         if ( ( superclass == M_SURFACE ) &&
               ( psubrec->surface_variable_flag != NULL ) )
         {
            switch( psubrec->surface_variable_flag[ i ] )
            {
               case PER_FACET:
                  for( j = 0; j < psubrec->mo_qty; j++ )
                  {
                     id = psubrec->mo_id_blks[ j ];
                     surface_facet_qty = p_mocd->surface_sizes[id - 1];
                     total_atoms += atoms * surface_facet_qty;
                  }
                  break;

               case PER_OBJECT:

                  for( j = 0; j < psubrec->mo_qty; j++ )
                  {
                     total_atoms += atoms;
                  }
                  break;

               default:

                  return( INVALID_DATA_TYPE );
                  break;
            }
         }
         else
         {
            total_atoms = atoms;
         }

         count += total_atoms;
         *size += total_atoms * atom_size;
      }
      psubrec->lump_atoms[0] = count;
      psubrec->lump_sizes[0] = *size;

      /* Calc subrec size. */
      if( superclass != M_SURFACE )
      {
         *size = psubrec->lump_sizes[0] * psubrec->mo_qty;
      }
      else
      {
         *size = psubrec->lump_sizes[0];
      }
   }
   else if ( data_org == RESULT_ORDERED )
   {
      /*
       * For result-ordered data, lump size depends on quantity
       * of mesh objects and whether state var is scalar- or
       * array-valued.
       */

      /* Allocate space for sizes and offsets arrays. */
      psubrec->lump_atoms = NEW_N( int, psubrec->qty_svars,
                                   "Lump atom array" );
      psubrec->lump_sizes = NEW_N( LONGLONG, psubrec->qty_svars,
                                   "Lump size array" );
      psubrec->lump_offsets = NEW_N( LONGLONG, psubrec->qty_svars,
                                     "Lump offset array" );
      if ((psubrec->qty_svars > 0) &&
            (psubrec->lump_atoms == NULL || psubrec->lump_sizes == NULL ||
             psubrec->lump_offsets == NULL))
      {
         return ALLOC_FAILED;
      }
      lump_offsets = psubrec->lump_offsets;
      lump_sizes = psubrec->lump_sizes;
      lump_atoms = psubrec->lump_atoms;

      /* Traverse the svars and assign lump sizes, atom counts. */
      bound = psubrec->svars + psubrec->qty_svars;
      for ( ppsvar = psubrec->svars, l = 0;
            ppsvar < bound;
            l++, ppsvar++ )
      {
         atoms = svar_atom_qty( *ppsvar );
         total_atoms = atoms;

         if ( ( superclass != M_SURFACE ) &&
               ( psubrec->surface_variable_flag != NULL ) )
         {
            switch( psubrec->surface_variable_flag[ l ] )
            {
               case PER_FACET:
                  for( j = 0; j < psubrec->mo_qty; j++ )
                  {
                     id = psubrec->mo_id_blks[ j ];
                     surface_facet_qty = p_mocd->surface_sizes[id - 1];
                     total_atoms += atoms * surface_facet_qty;
                  }
                  break;

               case PER_OBJECT:
                  for( j = 0; j < psubrec->mo_qty; j++ )
                  {
                     total_atoms += atoms;
                  }
                  break;

               default:

                  return INVALID_DATA_TYPE;
                  break;
            }
         }
         else
         {
            total_atoms = atoms;
         }

         lump_atoms[l] = psubrec->mo_qty * total_atoms;
         lump_sizes[l] = lump_atoms[l]
                         * EXT_SIZE( fam, *(*ppsvar)->data_type );
      }

      /* Calculate offsets to each lump to speed I/O later. */
      lump_offsets[0] = 0;
      for ( k = 1; k < psubrec->qty_svars; k++ )
      {
         lump_offsets[k] = lump_offsets[k - 1] + lump_sizes[k - 1];
      }

      k = psubrec->qty_svars - 1;

      /* Calc subrec size. */
      *size = lump_offsets[k] + lump_sizes[k];

   }
   /* else boo-boo */

   return OK;
}


/*****************************************************************
 * TAG( svar_atom_qty ) LOCAL
 *
 * Calculate the quantity of atoms in a state variable.
 */
static int
svar_atom_qty( Svar *p_svar )
{
   int qty;
   int k;
   int qty_svars;
   int base_size;

   qty = 1;
   base_size = 1;

   if ( *p_svar->agg_type == VECTOR )
   {
      qty = *p_svar->list_size;
   }
   else if ( *p_svar->agg_type == ARRAY)
   {
      for ( k = 0; k < *p_svar->order; k++ )
      {
         qty *= p_svar->dims[k];
      }
      
   }else if ( *p_svar->agg_type == VEC_ARRAY )
   {
      qty = 0;
      
      for ( k = 0; k < *p_svar->order; k++ )
      {
         base_size *= p_svar->dims[k];
      }
      
      qty_svars = *p_svar->list_size;
      
      for( k=0; k<qty_svars; k++)
      {
         if (p_svar->svars[k]->agg_type == SCALAR)
         {
            qty += base_size;
         }
         else 
         {
            qty += base_size *  svar_atom_qty(p_svar->svars[k]); 
         }         
      }    
   }
   

   return qty;
}


/*****************************************************************
 * TAG( mc_get_subrec_def ) PUBLIC
 *
 * Copy subrecord definition parameters to application struct.
 */
Return_value
mc_get_subrec_def( Famid fam_id, int srec_id, int subrec_id,
                   Subrecord *p_subrec )
{
   Sub_srec *p_ssrec;
   int id_cnt;
   int *p_isrc, *p_idest, *bound;
#ifdef CANT_USE_EXTANT_COUNT
   int mo_qty, blk_qty;
#endif
   Mili_family *fam;
   Htable_entry *p_hte;
   int i;
   int superclass;
   Return_value rval;

   fam = fam_list[fam_id];

   p_ssrec = fam->srecs[srec_id]->subrecs[subrec_id];

   str_dup( &p_subrec->name, p_ssrec->name );
   str_dup( &p_subrec->class_name, p_ssrec->mclass );
   if (p_subrec->name == NULL || p_subrec->class_name == NULL)
   {
      return ALLOC_FAILED;
   }
   p_subrec->organization = p_ssrec->organization;
   p_subrec->qty_svars = p_ssrec->qty_svars;
   p_subrec->svar_names = NEW_N( char *, p_subrec->qty_svars,
                                 "Subrecord svar name ptrs" );
   if (p_subrec->qty_svars > 0 && p_subrec->svar_names == NULL)
   {
      return ALLOC_FAILED;
   }
   for ( i = 0; i < p_subrec->qty_svars; i++ )
   {
      str_dup( p_subrec->svar_names + i, p_ssrec->svars[i]->name );
      if (p_subrec->svar_names[i] == NULL)
      {
         return ALLOC_FAILED;
      }
   }

   /* Get the superclass. */
   rval = htable_search( fam->srec_meshes[srec_id]->mesh_data.umesh_data,
                         p_ssrec->mclass, FIND_ENTRY, &p_hte );
   if (p_hte == NULL)
   {
      return rval;
   }
   superclass = ((Mesh_object_class_data *) p_hte->data)->superclass;
	p_subrec->superclass = superclass;
   /*
    * Currently, M_MESH subrecords are treated specially and don't record
    * mesh objects in blocks.  However, the Subrecord struct implies
    * that the information will exist, so we'll do (more) special processing
    * now to handle the M_MESH subrecord case.  The correct solution is
    * to stop treating M_MESH subrecords differently when they are defined
    * by an mc_def_subrec() call or when opening an existing db, but the
    * quick attempt at this failed for unknown reasons, so here we are...
    */

   rval = OK;
   switch ( superclass )
   {
      case M_UNIT:
      case M_NODE:
      case M_TRUSS:
      case M_BEAM:
      case M_TRI:
      case M_QUAD:
      case M_TET:
      case M_PYRAMID:
      case M_WEDGE:
      case M_HEX:
      case M_MAT:
      case M_PARTICLE:
         
         p_subrec->qty_blocks = p_ssrec->qty_id_blks;
         id_cnt = 2 * p_subrec->qty_blocks;
         p_subrec->mo_blocks = NEW_N( int, id_cnt,
                                      "Subrecord mo ident blocks" );
         if (id_cnt > 0 && p_subrec->mo_blocks == NULL)
         {
            return ALLOC_FAILED;
         }
         bound = p_ssrec->mo_id_blks + id_cnt;
#ifdef CANT_USE_EXTANT_COUNT
         mo_qty = 0;
#endif
         for ( p_isrc = p_ssrec->mo_id_blks, p_idest = p_subrec->mo_blocks;
               p_isrc < bound;
               p_idest += 2, p_isrc += 2 )
         {
            /* Copy block. */
            *p_idest = *p_isrc;
            p_idest[1] = p_isrc[1];
#ifdef CANT_USE_EXTANT_COUNT
            /* Block size (allow for decreasing idents). */
            blk_qty = p_isrc[1] - p_isrc[0] + 1;
            if ( p_isrc[1] < p_isrc[0] )
            {
               blk_qty = -(blk_qty - 1) + 1;
            }
            mo_qty += blk_qty;
         }

         p_subrec->qty_objects = mo_qty;
#else
         }

         p_subrec->qty_objects = p_ssrec->mo_qty;
#endif

         break;


      case M_MESH:

         p_subrec->qty_blocks = 1;
         p_subrec->mo_blocks = NEW_N( int, 2, "Subrecord mo ident block" );
         if (p_subrec->mo_blocks == NULL)
         {
            return ALLOC_FAILED;
         }
         p_subrec->mo_blocks[0] = p_subrec->mo_blocks[1] = 1;
         p_subrec->qty_objects = p_ssrec->mo_qty;

         break;


      case M_SURFACE:

         p_subrec->qty_blocks = 1;
         p_subrec->mo_blocks = NEW_N( int, 2, "Subrecord mo ident block" );
         if (p_subrec->mo_blocks == NULL)
         {
            return ALLOC_FAILED;
         }
         p_subrec->mo_blocks[0] = p_subrec->mo_blocks[1] = 1;
         p_subrec->qty_objects = p_ssrec->mo_qty;

         p_subrec->surface_variable_flag =
            NEW_N( int, p_subrec->qty_svars, "Surface variable flag" );
         if (p_subrec->qty_svars > 0 &&
             p_subrec->surface_variable_flag == NULL)
         {
            return ALLOC_FAILED;
         }

         break;


      default:

         rval = INVALID_CLASS_NAME;

         break;
   }

   return rval;
}


/*****************************************************************
 * TAG( commit_srecs ) PRIVATE
 *
 * Write state record formats to disk.
 */
Return_value
commit_srecs( Mili_family *fam )
{
   LONGLONG i_qty, c_qty;
   int i, j, k, ii;
   Srec *psrec;
   Sub_srec *psubrec;
   Svar **ppsvars;
   int svar_atoms;
   int *i_data, *pi;
   char *c_data, *pc;
   int *bound, *pblk;
   LONGLONG outbytes;
   char *psrc;
   Return_value rval;
   LONGLONG write_ct;

   /* Only output fresh formats. */
   if ( fam->qty_srecs - 1 <= fam->commit_max )
   {
      return OK;
   }

   rval = prep_for_new_data( fam, NON_STATE_DATA );
   if ( rval != OK )
   {
      return rval;
   }

   for ( i = fam->commit_max + 1; i < fam->qty_srecs; i++ )
   {
      i_qty = QTY_SREC_HEADER_FIELDS; /* Srec_id, mesh_id, size, qty_subrecs */
      c_qty = 0;
      psrec = fam->srecs[i];

      if ( psrec->status != OBJ_CLOSED )
      {
         continue;
      }

      /* Traverse the subrecords and count data. */
      for ( j = 0; j < psrec->qty_subrecs; j++ )
      {
         psubrec = psrec->subrecs[j];

         i_qty += 3; /* org, qty_svars, qty_id_blks */
         i_qty += 2 * psubrec->qty_id_blks; /* id_blks */

         c_qty += strlen( psubrec->name ) + 1; /* subrec name */
         c_qty += strlen( psubrec->mclass ) + 1; /* class name */

         /* Traverse the state variables to count name lengths and quantity. */
         ppsvars = psubrec->svars;
         svar_atoms = 0;
         for ( k = 0; k < psubrec->qty_svars; k++ )
         {
            if( psubrec->surface_variable_flag != NULL)
            {
               svar_atoms += svar_atom_qty( *ppsvars );
            }
            c_qty += strlen( ppsvars[k]->name ) + 1; /* svar name. */
         }
         if( svar_atoms > 0 )
         {
            i_qty += ( svar_atoms + 1);
         }
      }

      /* Allocate output buffers. */
      i_data = NEW_N( int, i_qty, "Srec commit int buf" );
      if (i_qty > 0 && i_data == NULL)
      {
         return ALLOC_FAILED;
      }
      c_qty = ROUND_UP_INT( c_qty, 4 );
      c_data = NEW_N( char, c_qty, "Srec commit char buf" );
      if (c_qty > 0 && c_data == NULL)
      {
         return ALLOC_FAILED;
      }

      pi = i_data;
      pc = c_data;

      /* Copy the main tags for the srec into the int output buffer. */
      *pi++ = i; /* srec_id */
      for ( j = 0; j < fam->qty_meshes; j++ )
      {
         if ( fam->meshes[j] == fam->srec_meshes[i] )
         {
            break;
         }
      }
      *pi++ = j; /* mesh_id */
      *pi++ = (int) psrec->size;
      *pi++ = psrec->qty_subrecs;

      /* Traverse the subrecords and copy data into output buffers. */
      for ( j = 0; j < psrec->qty_subrecs; j++ )
      {
         psubrec = psrec->subrecs[j];

         /*
          * Copy organization, quantity state vars, and
          * quantity object ident blocks.
          */
         *pi++ = psubrec->organization;
         *pi++ = psubrec->qty_svars;
         *pi++ = psubrec->qty_id_blks;

         /* Copy the mesh object ident blocks. */
         bound = psubrec->mo_id_blks + 2 * psubrec->qty_id_blks;
         for ( pblk = psubrec->mo_id_blks; pblk < bound; pblk += 2 )
         {
            *pi++ = pblk[0];
            *pi++ = pblk[1];
         }

         /* Copy the surface variable flags */
         if( psubrec->surface_variable_flag != NULL)
         {
            *pi++ = svar_atoms;
            for( ii = 0; ii < svar_atoms; ii++ )
            {
               *pi++  = psubrec->surface_variable_flag[ii];
            }
         }

         /* Copy the subrecord name. */
         for ( psrc = psubrec->name; (*pc++ = *psrc++); );

         /* Copy the class name. */
         for ( psrc = psubrec->mclass; (*pc++ = *psrc++); );

         /* Copy the state variable names. */
         ppsvars = psubrec->svars;
         for ( k = 0; k < psubrec->qty_svars; k++ )
         {
            for ( psrc = ppsvars[k]->name; (*pc++ = *psrc++); );
         }
      }

      /* Create directory entry for the state record. */
      outbytes = c_qty + i_qty * EXT_SIZE( fam, M_INT );
      rval = add_dir_entry( fam, STATE_REC_DATA, (int) i_qty, (int) c_qty, 0,
                            NULL, fam->next_free_byte, outbytes );
      if ( rval != OK )
      {
         free(i_data);
         free(c_data);
         return rval;
      }

      /* Write int data to the file. */
      write_ct = fam->write_funcs[M_INT]( fam->cur_file, i_data, i_qty );
      if (write_ct != i_qty)
      {
         free(i_data);
         free(c_data);
         return SHORT_WRITE;
      }

      /* Write character data to the file. */
      write_ct = fam->write_funcs[M_STRING]( fam->cur_file, c_data, c_qty );
      if (write_ct != c_qty)
      {
         free(i_data);
         free(c_data);
         return SHORT_WRITE;
      }

      /* Mark the format to prevent further output. */
      psrec->status = OBJ_SAVED;

      fam->next_free_byte += outbytes;

      free( i_data );
      free( c_data );
   }

   fam->commit_max = fam->qty_srecs - 1;

   return OK;
}


/*****************************************************************
 * TAG( load_srec_formats ) PRIVATE
 *
 * Load state record formats from a Mili family into memory.
 *
 * Should only be called at db initialization, but after loading
 * svars and mesh definitions.
 */
Return_value
load_srec_formats( Mili_family *fam )
{
   int i, j;
   int ent_qty, fil_qty;
   Dir_entry *p_de;
   Return_value rval = OK;

   /* Traverse the directory and find state record entries. */

   fil_qty = fam->file_count;

   /* For each non-state file... */
   for ( i = 0; i < fil_qty; i++ )
   {
      /* For each directory entry in file... */
      ent_qty = fam->directory[i].qty_entries;
      p_de = fam->directory[i].dir_entries;

      for ( j = 0; j < ent_qty; j++ )
      {
         if ( p_de[j][TYPE_IDX] == STATE_REC_DATA )
         {
            rval = add_srec( fam, i, p_de[j] );
         }
         if (rval != OK)
         {
            return rval;
         }
      }
   }

   fam->commit_max = fam->qty_srecs - 1;

   return rval;
}


/*****************************************************************
 * TAG( add_srec ) LOCAL
 *
 * Load state record formats from a Mili family into memory.
 */
static Return_value
add_srec( Mili_family *fam, int index, Dir_entry dir_ent )
{
   int i, subrec_qty, i_qty, c_qty;
   int *i_data, *subrec_i_data;
   int p_srec_id;
   int superclass;
   int qty_surface_flags;
   char *c_data, *subrec_c_data;
   char *p_c;
   int srec_hdr[QTY_SREC_HEADER_FIELDS];
   int c_data_len;
   Htable_entry *subrec_entry, *class_entry;
   Sub_srec *p_subrec;
   Mesh_object_class_data *p_mocd;
   Return_value rval = OK;
   int read_ct;

   /* Seek to the srec entry in the family. */
   rval = non_state_file_open( fam, index, fam->access_mode );
   if (rval != OK)
   {
      return rval;
   }
   rval = non_state_file_seek( fam, dir_ent[OFFSET_IDX] );
   if (rval != OK)
   {
      return rval;
   }

   /* Read the srec data header. */
   read_ct = fam->read_funcs[M_INT]( fam->cur_file, srec_hdr,
                                     QTY_SREC_HEADER_FIELDS );
   if (read_ct != QTY_SREC_HEADER_FIELDS)
   {
      return SHORT_READ;
   }

   i_qty = dir_ent[MODIFIER1_IDX] - QTY_SREC_HEADER_FIELDS;
   c_qty = dir_ent[MODIFIER2_IDX];

   /* Allocate buffers for integer and character data. */
   i_data = NEW_N( int, i_qty, "Srec int data" );
   if (i_qty > 0 && i_data == NULL)
   {
      return ALLOC_FAILED;
   }
   c_data = NEW_N( char, c_qty, "Srec char data" );
   if (c_qty > 0 && c_data == NULL)
   {
      free(i_data);
      return ALLOC_FAILED;
   }

   /* Read subrecord format descriptions. */
   read_ct = fam->read_funcs[M_INT]( fam->cur_file, i_data, i_qty );
   if (read_ct != i_qty)
   {
      free(i_data);
      free(c_data);
      return SHORT_READ;
   }
   read_ct = fam->read_funcs[M_STRING]( fam->cur_file, c_data, c_qty );
   if (read_ct != c_qty)
   {
      free(i_data);
      free(c_data);
      return SHORT_READ;
   }

   /* Create uninitialized state record format descriptor. */
   rval = make_srec( fam, srec_hdr[SREC_PARENT_MESH_ID_IDX], &p_srec_id );
   if (rval != OK)
   {
      free(i_data);
      free(c_data);
      return rval;
   }


   subrec_qty = srec_hdr[SREC_QTY_SUBRECS_IDX];
   subrec_i_data = i_data;
   subrec_c_data = c_data;

   /* Define each subrecord. */
   for ( i = 0; i < subrec_qty; i++ )
   {
      rval = def_subrec_from_db( fam, srec_hdr[SREC_ID_IDX], subrec_i_data,
                                 subrec_c_data, &c_data_len );
      if (rval != OK)
      {
         break;
      }

      subrec_i_data += 3 + subrec_i_data[2] * 2;


      /* Need to go through all of this to make backward compatible */
      /**************************************************************/
      /* Get the subrecord entry. */
      p_c = subrec_c_data;
      rval = htable_search( fam->subrec_table, p_c, FIND_ENTRY, &subrec_entry );
      if (subrec_entry == NULL)
      {
         break;
      }
      p_subrec = (Sub_srec *)subrec_entry->data;

      /* Get the object class. */
      rval = htable_search( fam->srec_meshes[p_srec_id]->mesh_data.umesh_data,
                            p_subrec->mclass, FIND_ENTRY, &class_entry );
      if (class_entry == NULL)
      {
         break;
      }

      /* Get the superclass. */
      p_mocd = (Mesh_object_class_data *) class_entry->data;
      superclass = p_mocd->superclass;

      if( superclass == M_SURFACE )
      {
         qty_surface_flags = subrec_i_data[0];
         subrec_i_data += 1 + qty_surface_flags;
      }
      /**************************************************************/

      subrec_c_data += c_data_len;

   }

   /* Mark format to prevent modification. */
   if (rval == OK)
   {
      fam->srecs[srec_hdr[SREC_ID_IDX]]->status = OBJ_SAVED;
   }

   free( i_data );
   free( c_data );

   return rval;
}


/*****************************************************************
 * TAG( def_subrec_from_db ) LOCAL
 *
 * Load state record formats from a Mili family into memory.
 */
static Return_value
def_subrec_from_db( Mili_family *fam, int srec_id, int *i_data, char *c_data,
                    int *c_data_len )
{
   Srec *p_sr;
   char *p_c;
   int *p_i, *bound, *p_src, *p_dest;
   Hash_action op;
   Htable_entry *subrec_entry, *svar_entry, *class_entry;
   Sub_srec *p_subrec;
   int org, qty_svars, qty_id_blks;
   Svar **svars;
   int svar_atoms;
   int i, ii, count;
   LONGLONG sub_size;
   int superclass;
   Mesh_object_class_data *p_mocd;
   Return_value rval = OK;


   p_sr = fam->srecs[srec_id];
   p_c = c_data;
   p_i = i_data;

   org = *p_i++;
   qty_svars = *p_i++;
   qty_id_blks = *p_i++;

   /* Create the subrecord hash table if it hasn't been. */
   if ( fam->subrec_table == NULL )
   {
      fam->subrec_table = htable_create( DEFAULT_HASH_TABLE_SIZE );
      if (fam->subrec_table == NULL)
      {
         return ALLOC_FAILED;
      }
   }

   /* Create an entry in the hash table. */
   if( strncmp( p_c, "sand", 4 ) == 0 )
   {
      op = ENTER_ALWAYS;
   }
   else
   {
      op = ENTER_UNIQUE;
   }

   rval = htable_search( fam->subrec_table, p_c, op, &subrec_entry );
   if (subrec_entry == NULL)
   {
      return rval;
   }

   /* Create the subrec. */
   p_subrec = NEW( Sub_srec, "State subrecord" );
   if (p_subrec == NULL)
   {
      return ALLOC_FAILED;
   }
   str_dup( &p_subrec->name, p_c );
   if (p_subrec->name == NULL)
   {
      return ALLOC_FAILED;
   }
   p_subrec->organization = org;
   p_subrec->qty_svars = qty_svars;
   svars = NEW_N( Svar *, qty_svars, "Svar array" );
   if (qty_svars > 0 && svars == NULL)
   {
      free(p_subrec->name);
      free(p_subrec);
      return ALLOC_FAILED;
   }
   p_subrec->svars = svars;
   subrec_entry->data = (void *) p_subrec;

   /* Increment char ptr past subrec name to class name and copy. */
   p_c += strlen( p_c ) + 1;
   str_dup( &p_subrec->mclass, p_c );
   if (p_subrec->mclass == NULL)
   {
      return ALLOC_FAILED;
   }

   /* Increment char ptr past class name to first svar name. */
   p_c += strlen( p_c ) + 1;

   /* Get state variable references. */
   for ( i = 0; i < qty_svars; i++ )
   {
      rval = htable_search( fam->svar_table, p_c, FIND_ENTRY, &svar_entry );
      if (svar_entry == NULL)
      {
         free(svars);
         free(p_subrec->mclass);
         free(p_subrec->name);
         free(p_subrec);
         return rval;
      }
      svars[i] = (Svar *) svar_entry->data;

      p_c += strlen( p_c ) + 1;
   }

   /* Pass back length of character data processed. */
   *c_data_len = (int) (p_c - c_data);

   /* Get the object class. */
   rval = htable_search( fam->srec_meshes[srec_id]->mesh_data.umesh_data,
                         p_subrec->mclass, FIND_ENTRY, &class_entry );
   if (class_entry == NULL)
   {
      free(svars);
      free(p_subrec->mclass);
      free(p_subrec->name);
      free(p_subrec);
      return rval;
   }

   /* Get the superclass. */
   p_mocd = (Mesh_object_class_data *) class_entry->data;
   superclass = p_mocd->superclass;

   if ( superclass != M_MESH )
   {
      p_subrec->qty_id_blks = qty_id_blks;
      p_subrec->mo_id_blks = NEW_N( int, qty_id_blks * 2, "Subrec id blks" );
      if (qty_id_blks > 0 && p_subrec->mo_id_blks == NULL)
      {
         free(svars);
         free(p_subrec->mclass);
         free(p_subrec->name);
         free(p_subrec);
         return ALLOC_FAILED;
      }

      /* Copy the ident blocks, counting idents. */
      count = 0;
      bound = p_i + 2 * qty_id_blks;
      for ( p_src = p_i, p_dest = p_subrec->mo_id_blks;
            p_src < bound;
            p_dest += 2, p_src += 2 )
      {
         p_dest[0] = p_src[0];
         p_dest[1] = p_src[1];
         count += p_dest[1] - p_dest[0] + 1;
      }

      p_subrec->mo_qty = count;

      if ( superclass == M_SURFACE )
      {
         p_i += 2 * qty_id_blks;
         svar_atoms = *p_i++;
         p_subrec->surface_variable_flag =
            NEW_N( int, svar_atoms, "Surface flags" );
         if (svar_atoms > 0 && p_subrec->surface_variable_flag == NULL)
         {
            free(p_subrec->mo_id_blks);
            free(svars);
            free(p_subrec->mclass);
            free(p_subrec->name);
            free(p_subrec);
            return ALLOC_FAILED;
         }
         for( ii = 0; ii < svar_atoms; ii++ )
         {
            p_subrec->surface_variable_flag[ii] = *p_i++;
         }
      }
   }
   else
   {
      p_subrec->mo_qty = 1;
   }

   /* Calculate positioning metrics. */
   rval = set_metrics( fam, org, p_subrec, p_mocd, &sub_size );
   if (rval != OK)
   {
      free(p_subrec->surface_variable_flag);
      free(p_subrec->mo_id_blks);
      free(svars);
      free(p_subrec->mclass);
      free(p_subrec->name);
      free(p_subrec);
      return rval;
   }

   /* Create buffer queue for object-ordered subrecords. */
   if ( org == OBJECT_ORDERED )
   {
      p_subrec->ibuffer = create_buffer_queue( DFLT_BUF_COUNT, sub_size );
      if (p_subrec->ibuffer == NULL)
      {
         free(p_subrec->surface_variable_flag);
         free(p_subrec->mo_id_blks);
         free(svars);
         free(p_subrec->mclass);
         free(p_subrec->name);
         free(p_subrec);
         return ALLOC_FAILED;
      }
   }

   /* Offset to subrecord is previous record size. */
   p_subrec->offset = p_sr->size;

   /* Update the record size. */
   p_sr->size += sub_size;

   /* Save a reference in order of its creation. */
   p_sr->subrecs = RENEW_N( Sub_srec *, p_sr->subrecs, p_sr->qty_subrecs, 1,
                            "Srec data order incr" );
   if (p_sr->subrecs == NULL)
   {
      free(p_subrec->surface_variable_flag);
      free(p_subrec->mo_id_blks);
      free(svars);
      free(p_subrec->mclass);
      free(p_subrec->name);
      free(p_subrec);
      return ALLOC_FAILED;
   }
   p_sr->subrecs[p_sr->qty_subrecs] = p_subrec;
   p_sr->qty_subrecs++;

   return rval;
}


/*****************************************************************
 * TAG( mc_read_results ) PUBLIC
 *
 * Read state data by subrecord, returning a sequence of result-
 * ordered arrays in the caller's data buffer.
 */
Return_value
mc_read_results( Famid fam_id, int state, int subrec_id, int qty,
                 char **results, void *data )
{
   Mili_family *fam;
   int st;
   Srec *p_sr;
   Sub_srec *p_subrec;
   int i;
   Svar *p_sv;
   char *p_c;
   void **p_output = NULL;
   int atom_size;
   Translated_ref *p_trans_refs;
   Return_value rval;

   fam = fam_list[fam_id];
   st = state - 1;

   if ( state > fam->state_qty )
   {
      if ( fam->active_family )
      {
         rval = update_active_family( fam );
         if (rval != OK)
         {
            return rval;
         }
         else if ( state > fam->state_qty )
         {
            return INVALID_STATE;
         }
      }
      else
      {
         return INVALID_STATE;
      }
   }

   p_sr = fam->srecs[(fam->state_map + st)->srec_format];
   p_trans_refs = NEW_N( Translated_ref, qty,
                         "Translated svar ref array for read" );
   if (qty > 0 && p_trans_refs == NULL)
   {
      return ALLOC_FAILED;
   }

   /* Validate and record the subrecord. */
   if ( subrec_id < 0 || subrec_id > p_sr->qty_subrecs - 1 )
   {
      return INVALID_INDEX;
   }
   p_subrec = p_sr->subrecs[subrec_id];

   p_output = NEW_N( void *, qty, "Result array output pointers" );
   if (qty > 0 && p_output == NULL)
   {
      return ALLOC_FAILED;
   }

   /* QC the requested results and save state variable references. */
   p_c = (char *) data;
   rval = OK;
   for ( i = 0; i < qty; i++ )
   {
      /* Parse, consolidate the result reference. */
      rval = translate_reference( p_subrec, results[i], &p_sv,
                                  p_trans_refs + i );

      if ( rval != OK )
      {
         free( p_output );
         free( p_trans_refs );
         return rval;
      }

      /*
       * Align current output destination address for size of data type
       * of current result and save the (possibly) adjusted destination
       * address; increment output address to end of current result
       * in preparation for next result.
       */
      atom_size = internal_sizes[*p_sv->data_type];

      p_output[i] = (void *) p_c;
      if ( i < qty - 1 )
      {
         p_c += p_trans_refs[i].reqd_qty * atom_size * p_subrec->mo_qty;
      }
   }

   if ( p_subrec->organization == RESULT_ORDERED )
   {
      rval = get_ro_svars( fam, st, p_subrec, qty, p_trans_refs, p_output );
   }
   else // Object ordered
   {
      rval = get_oo_svars( fam, st, p_subrec, qty, p_trans_refs, p_output );
   }

   free( p_output );
   free( p_trans_refs );

   return rval;
}


/*****************************************************************
 * TAG( translate_reference ) LOCAL
 *
 * Parse a result specification string, possibly including a subset
 * specification of an aggregate svar, and put it into a canonical
 * form (a Translated_ref struct).
 */
static Return_value
translate_reference( Sub_srec *p_subrec, char *result, Svar **pp_svar,
                     Translated_ref *p_tref )
{
   char spec[512];
   char *p_name;
   char *p_c_idx;
   char component[M_MAX_NAME_LEN + 1];
   char secondary_component[M_MAX_NAME_LEN + 1];
   int indices[M_MAX_ARRAY_DIMS];
   int index_qty=0, 
       comp_index=0,
       secondary_comp_index=0;
   
   char *delimiters = "[ , ]";
   int qty_svars, last;
   Svar *p_svar;
   Return_value rval;
   Translated_ref tr;
   int i;
   Bool_type have_component,
             have_second_component;

   strcpy( spec, result );
   p_name = strtok( spec, "[" );
   if (p_name == NULL)
   {
      return BAD_NAME_READ;
   }

   /* Trim any trailing blanks. */
   last = strlen( p_name ) - 1;
   while ( p_name[last] == ' ' && last > -1 )
   {
      last--;
   }
   p_name[last + 1] = '\0';

   index_qty = 0;
   component[0] = '\0';
   secondary_component[0] = '\0';

   /* Parse to extract subset specification of aggregate svar. */
   have_component = FALSE;
   have_second_component = FALSE;

   p_c_idx = strtok( NULL, delimiters );
   
   while ( p_c_idx != NULL )
   {
      if ( !is_numeric( p_c_idx ) )
      {
         if ( have_component && have_second_component)
         {
            /* Only one component name allowed. */
            return MALFORMED_SUBSET;
         }
         /* Must be a vector component short name. */
         if ( !have_component )
         {
            strcpy( component, p_c_idx );
            have_component = TRUE;
         }else // Must have a vector within the vector array
         {
            strcpy( secondary_component,  p_c_idx );
            have_second_component = TRUE;
         }         
      }
      else
      {
         indices[index_qty] = atoi( p_c_idx ) - 1;
         index_qty++;
      }

      p_c_idx = strtok( NULL, delimiters );
   }

   /* Find the svar indicated by the name. */
   qty_svars = p_subrec->qty_svars;
      
   for ( i = 0; i < qty_svars; i++ )
   {
      if ( strcmp( p_subrec->svars[i]->name, p_name ) == 0 )
      {
         break;
      }
   }
   if ( i == qty_svars )
   {
      return UNBOUND_NAME;
   }
      
   p_svar = p_subrec->svars[i];

   /* Verify component name if it exists. */
   if ( have_component )
   {
      if ( *p_svar->agg_type != VECTOR && *p_svar->agg_type != VEC_ARRAY )
      {
         return MALFORMED_SUBSET;
      }

      for ( comp_index = 0; comp_index < *p_svar->list_size; comp_index++ )
      {
         // This will give us the svar at the first level. In the 
         // older version this was always a SCALAR value.  We now
         // have the ability to have a VECTOR component here.  
   
         if ( strcmp( p_svar->svars[comp_index]->name, component ) == 0 )
         {
            if(have_second_component)
            {
               for(secondary_comp_index=0 ; 
                   secondary_comp_index< *p_svar->svars[comp_index]->list_size;
                   secondary_comp_index++)
               {
                  if ( strcmp( p_svar->svars[comp_index]->svars[secondary_comp_index]->name, 
                       secondary_component ) == 0 )
                  {
                     break;
                  }
                  
               } 
               if ( secondary_comp_index  == *p_svar->svars[comp_index]->list_size )
               {
                  return MALFORMED_SUBSET;
               }
            }
            break;
         }
      }
      if ( comp_index  == *p_svar->list_size )
      {
         return MALFORMED_SUBSET;
      }
   }

   /*
    * Load the Translated_ref struct.
    */

   /* Make a dummy copy so p_tref can be returned unchanged on failure. */
   tr = *p_tref;

   p_tref->atom_qty = svar_atom_qty( p_svar );
   p_tref->index = i;
   strcpy( p_tref->name, p_name );

   if ( index_qty == 0 && !have_component )
   {
      p_tref->reqd_qty = p_tref->atom_qty;
      p_tref->offset = 0;
   }
   else if ( *p_svar->agg_type != SCALAR )
   {
      /* Turn the subset specification into a size and offset. */
      rval = map_subset_spec( p_svar, indices, index_qty, component,
                              comp_index, secondary_component, 
                              secondary_comp_index, p_tref );
      if ( rval != OK )
      {
         *p_tref = tr;
         return rval;
      }
   }
   else
   {
      /* Requested subset of a scalar variable! */
      *p_tref = tr;
      return MALFORMED_SUBSET;
   }

   *pp_svar = p_svar;

   return OK;
}


/*****************************************************************
 * TAG( map_subset_spec ) LOCAL
 *
 * Turn an aggregate svar subset specification into a size and offset.
 */
static Return_value
map_subset_spec( Svar *p_svar, int indices[], int index_qty, char *component,
                 int comp_index, char *sub_component, 
                 int sub_comp_index, Translated_ref *p_tref )
{
   int i, j;
   int svar_dims[M_MAX_ARRAY_DIMS + 1];
   int vec_array_comp_offset[M_MAX_ARRAY_DIMS + 1];
   int svar_dim_sizes[M_MAX_ARRAY_DIMS + 1];
   int svar_rank;
   int dim_size;
   int spec_indices[M_MAX_ARRAY_DIMS + 1];
   int spec_rank;
   int offset;
   int have_subcomponent = 0;
   Svar *sub_p_svar = 0;
   
   if(*sub_component != '\0' )
   {
      have_subcomponent = 1;
      sub_p_svar = p_svar->svars[comp_index];
   }

   /* First reduce aggregate svar to an array def. */
   switch ( *p_svar->agg_type )
   {
      case VECTOR:
         svar_rank = 1;
         svar_dims[0] = *p_svar->list_size;
         svar_dim_sizes[0] = 1;
         break;

      case ARRAY:
         svar_rank = *p_svar->order;
         for ( i = svar_rank - 1, dim_size = 1; i > -1; i-- )
         {
            svar_dims[i] = p_svar->dims[i];
            svar_dim_sizes[i] = dim_size;
            dim_size *= svar_dims[i];
         }
         break;

      case VEC_ARRAY:
         /*
          * First verify that if a component is specified, all vector
          * array dimensions have been specified also.
          */
         if ( *component != '\0' )
         {
            if ( index_qty != *p_svar->order )
            {
               return MALFORMED_SUBSET;
            }
         }

         //Adding one for the number of vector components
         svar_rank = *p_svar->order + 1;  

         svar_dims[svar_rank - 1] = 0;
         for( i = 0; i < *p_svar->list_size; i++ ){
            vec_array_comp_offset[i] = svar_dims[svar_rank-1];
            switch(*p_svar->svars[i]->agg_type){
               case SCALAR:
                  svar_dims[svar_rank-1] += 1;
                  break;
               case ARRAY:
                  for( j = 0; j < *p_svar->svars[i]->order; j++){
                     svar_dims[svar_rank-1] += p_svar->svars[i]->dims[j];
                  }
                  break;
               case VECTOR:
                  svar_dims[svar_rank-1] += *p_svar->svars[i]->list_size;
                  break;
            }
         }
         svar_dim_sizes[svar_rank - 1] = 1;

         for ( i = svar_rank - 2, dim_size = svar_dims[svar_rank - 1]; i > -1; i-- )
         {
            svar_dims[i] = p_svar->dims[i];
            svar_dim_sizes[i] = dim_size;
            dim_size *= svar_dims[i];
         }
         break;

      default:
         return INVALID_AGG_TYPE; /* Should never get here. */
         break;
   }

   /* Now turn the subset specification into an array reference. */

   spec_rank = 0;
   i = 0;
   if ( index_qty > 0 )
   {
      spec_rank = index_qty;
      for ( ; i < index_qty; i++ )
      {
         spec_indices[i] = indices[i];
      }
   }

   if ( *component != '\0' )
   {
      spec_rank++;
      if(*p_svar->agg_type == VEC_ARRAY)
         spec_indices[i] = vec_array_comp_offset[comp_index];
      else 
         spec_indices[i] = comp_index;
      // Offset to subcomponent if necessary
      if(*sub_component != '\0')
      {
        spec_indices[i] += sub_comp_index;
      }
   }

   /* Make sure the specification is within bounds. */

   if ( spec_rank > svar_rank )
   {
      return MALFORMED_SUBSET;
   }

   for ( i = 0; i < spec_rank; i++ )
   {
      if ( spec_indices[i] < 0 || spec_indices[i] > svar_dims[i] - 1 )
      {
         return MALFORMED_SUBSET;
      }
   }

   /* Get the size and offset. */
   p_tref->reqd_qty = svar_dim_sizes[spec_rank - 1];
   for ( i = 0, offset = 0; i < spec_rank; i++ )
   {
      offset += spec_indices[i] * svar_dim_sizes[i];
   }
   p_tref->offset = offset;

   return OK;
}


/*****************************************************************
 * TAG( get_ro_svars ) LOCAL
 *
 * Get result arrays from a result-ordered database subrecord.
 */
static Return_value
get_ro_svars( Mili_family *fam, int state, Sub_srec *p_subrec, int qty,
              Translated_ref *refs, void **p_out )
{
   State_descriptor *p_sd;
   LONGLONG offset;
   LONGLONG read_cnt, 
            new_read_atoms, 
            read_atoms, 
            length;
   int file_num;
   LONGLONG buf_pos;
   int i, j;
   int idx;
   int data_type;
   char *p_tmp;
   char *p_obuf;
   Bool_type subset;
   Return_value rval;
   void (*dist_func)();
   int p_tmp_size;

   /* Loop over the requested state variables. */
   rval = OK;
   for ( i = 0; i < qty; i++ )
   {
      if ( refs[i].done )
      {
         continue;
      }

      /* Get data type of current variable request. */
      idx = refs[i].index;
      data_type = *p_subrec->svars[idx]->data_type;

      /*
       * If a subset of an aggregate svar is requested, allocate
       * a temporary buffer to read from file into.  Otherwise
       * read directly into application data buffer.
       */
      subset = ( refs[i].reqd_qty != refs[i].atom_qty );

      if ( subset )
      {
         /* Subset of an aggregate type requested. */
         p_tmp_size = p_subrec->lump_atoms[idx] * internal_sizes[data_type];
         p_tmp = NEW_N( char, p_tmp_size, "Tmp ro res buf" );
         if (p_tmp_size > 0 && p_tmp == NULL)
         {
            rval = ALLOC_FAILED;
            break;
         }
         p_obuf = p_tmp;
      }
      else
      {
         p_obuf = (char *) p_out[i];
      }

      /* Seek to result array. */
      p_sd = fam->state_map + state;
      rval = state_file_open( fam, p_sd->file, fam->access_mode );
      if (rval != OK)
      {
         break;
      }
      offset = p_sd->offset
               + EXT_SIZE( fam, M_INT ) + EXT_SIZE( fam, M_FLOAT )
               + p_subrec->offset
               + p_subrec->lump_offsets[idx];
      if(  fam->db_type == TAURUS_DB_TYPE )
      {
         file_num = p_sd->file;
         while( offset >= fam->cur_st_file_size )
         {
            offset -= fam->cur_st_file_size;
            file_num++;
         }
         if( file_num != p_sd->file )
         {
            rval = state_file_close( fam );
            if (rval != OK)
            {
               break;
            }
            rval = state_file_open( fam, file_num, fam->access_mode );
            if (rval != OK)
            {
               break;
            }
         }
      }
      rval = seek_state_file( fam->cur_st_file, offset );
      if (rval != OK)
      {
         break;
      }

      /* Read data. */
      read_atoms = (LONGLONG) p_subrec->lump_atoms[idx];
      if(  fam->db_type == TAURUS_DB_TYPE )
      {
         length = read_atoms;
         buf_pos = 0;
         while( length > 0 )
         {
            if( offset + length * EXT_SIZE( fam, data_type )
                  <= fam->cur_st_file_size )
            {
               read_cnt = fam->state_read_funcs[data_type]( fam->cur_st_file,
                          p_obuf+buf_pos, length );
               if ( read_cnt < length )
               {
                  rval = SHORT_READ;
                  break;
               }
               length = 0;
            }
            else
            {
               new_read_atoms = (LONGLONG)( fam->cur_st_file_size - offset )
                                / EXT_SIZE( fam, data_type );
               read_cnt = fam->state_read_funcs[data_type]( fam->cur_st_file,
                          p_obuf+buf_pos, new_read_atoms );
               if ( read_cnt < new_read_atoms )
               {
                  rval = SHORT_READ;
                  break;
               }
               offset = 0;
               file_num++;;
               rval = state_file_close( fam );
               if (rval != OK)
               {
                  break;
               }
               rval = state_file_open( fam, file_num, fam->access_mode );
               if (rval != OK)
               {
                  break;
               }
               length -= new_read_atoms;
               buf_pos += ( new_read_atoms * EXT_SIZE( fam, data_type ) );
            }
         }
      }
      else
      {
         read_cnt = fam->state_read_funcs[data_type]( fam->cur_st_file,
                    p_obuf, read_atoms );
         if ( read_cnt != (LONGLONG) read_atoms )
         {
            rval = SHORT_READ;
         }
      }

      if ( rval != OK )
      {
         if (subset)
         {
            free( p_tmp );
         }
         break;
      }

      /*
       * Get all subsets of current svar among current requests
       * to avoid re-accessing the disk.
       *
       * If current request is not a subset, it will have been read
       * directly into the application output buffer so start searching
       * with the next requested variable.
       */
      for ( j = subset ? i : i + 1; j < qty; j++ )
      {
         if ( strcmp( refs[j].name, refs[i].name ) == 0 && !refs[j].done )
         {
            /* Found a match with what's been read in. */

            idx = refs[j].index;
            data_type = *p_subrec->svars[idx]->data_type;

            /* Get appropriate function to move data to output buffer. */
            dist_func = get_data_dist_func( data_type, refs[i].reqd_qty,
                                            refs[i].atom_qty );
            if ( dist_func != NULL )
            {
               /* Move data. */
               dist_func( (void *) p_obuf, p_subrec->mo_qty,
                          refs[j].atom_qty, refs[j].offset,
                          refs[j].reqd_qty, p_out[j] );
               refs[j].done = TRUE;
            }
            else
            {
               rval = INVALID_DATA_TYPE;
               break;
            }
         }
      }

      if ( subset )
      {
         free( p_tmp );
      }

      if ( rval != OK )
      {
         break;
      }
   }

   return rval;
}


/*****************************************************************
 * TAG( get_oo_svars ) LOCAL
 *
 * Get result arrays from an object-ordered database subrecord.
 */
static Return_value
get_oo_svars( Mili_family *fam, int state, Sub_srec *p_subrec, int qty,
              Translated_ref *refs, void **p_out )
{
   Buffer_queue *p_bq;
   Bool_type read_from_file;
   int i, j, next, idx;
   int data_type;
   State_descriptor *p_sd;
   void *ibuf;
   int ibuf_len;
   LONGLONG offset;
   void (*dist_func)();
   LONGLONG read_cnt, 
            read_atoms;
   int file_num;
   LONGLONG length, 
          new_read_atoms;
   LONGLONG buf_pos;
   int obj_vec_size, obj_vec_offset;
   int srec_id, superclass;
   int id, qty_facets;
   Mesh_object_class_data *p_mocd;
   Htable_entry *class_entry;
   Return_value rval;

   srec_id = fam->state_map[state].srec_format;
   rval = htable_search( fam->srec_meshes[srec_id]->mesh_data.umesh_data,
                         p_subrec->mclass, FIND_ENTRY, &class_entry );
   if (class_entry == NULL)
   {
      return rval;
   }

   p_mocd = (Mesh_object_class_data *) class_entry->data;
   superclass = p_mocd->superclass;

   p_bq = p_subrec->ibuffer;
   data_type = *p_subrec->svars[refs[0].index]->data_type;
   /* Get an input buffer. */
   if ( p_bq->buffer_count != 0 )
   {
      /* Attempt to match requested state among buffered states. */
      for ( i = 0; i < p_bq->buffer_count; i++ )
      {
         if ( p_bq->state_numbers[i] == state )
         {
            break;
         }
      }

      if ( i == p_bq->buffer_count )
      {
         /*
          * State not in memory - prepare to read it into next buffer
          * in queue.
          */

         read_from_file = TRUE;

         /* Check for unused buffers first. */
         if ( p_bq->new_count > 0 )
         {
            /* Find an unused buffer. */
            for ( i = 0; i < p_bq->buffer_count; i++ )
            {
               if ( p_bq->state_numbers[i] == -1 )
               {
                  break;
               }
            }

            next = i;
            p_bq->new_count--;
         }
         else
         {
            /* No unused buffers, so overwrite the one after "recent". */
            next = (p_bq->recent + 1) % p_bq->buffer_count;
         }

         /* Update pool. */
         p_bq->state_numbers[next] = state;
         p_bq->recent = next;

         /* Assign buffer. */
         ibuf = p_bq->data_buffers[next];
      }
      else
      {
         /* Found requested state in memory. */
         read_from_file = FALSE;
         ibuf = p_bq->data_buffers[i];
      }
   }
   else
   {
      /*
       * No buffering; allocate temporary buffer. All must have same
       * numeric type for object-ordered data.
       */
      read_from_file = TRUE;
      ibuf_len = p_subrec->lump_atoms[0] * internal_sizes[data_type]
                 * p_subrec->mo_qty;

      ibuf = (void *) NEW_N( char, ibuf_len, "Temp input buffer" );
      if (ibuf_len > 0 && ibuf == NULL)
      {
         return ALLOC_FAILED;
      }
   }

   /* Input from file if necessary. */
   if ( read_from_file )
   {
      p_sd = fam->state_map + state;
      rval = state_file_open( fam, p_sd->file, fam->access_mode );
      if (rval != OK)
      {
         return rval;
      }
      offset = p_sd->offset
               + EXT_SIZE( fam, M_INT ) + EXT_SIZE( fam, M_FLOAT )
               + p_subrec->offset;
      if(  fam->db_type == TAURUS_DB_TYPE )
      {
         file_num = p_sd->file;
         while( offset >= fam->cur_st_file_size )
         {
            offset -= fam->cur_st_file_size;
            file_num++;
         }
         if( file_num != p_sd->file )
         {
            rval = state_file_close( fam );
            if (rval != OK)
            {
               return rval;
            }
            rval = state_file_open( fam, file_num, fam->access_mode );
            if (rval != OK)
            {
               return rval;
            }
         }
      }
      rval = seek_state_file( fam->cur_st_file, offset );
      if (rval != OK)
      {
         return rval;
      }

      /* Read data. */
      if ( M_SURFACE == superclass )
      {
         read_atoms = p_subrec->lump_atoms[0];
      }
      else
      {
         read_atoms = p_subrec->lump_atoms[0] * p_subrec->mo_qty;
      }

      if(  fam->db_type == TAURUS_DB_TYPE )
      {
         length = read_atoms;
         buf_pos = 0;
         while( length > 0 )
         {
            if( offset + length * EXT_SIZE( fam, data_type )
                  <= fam->cur_st_file_size )
            {
               read_cnt = fam->state_read_funcs[data_type]( fam->cur_st_file,
                          (char *) ibuf+buf_pos,
                          length );
               if ( read_cnt < (LONGLONG) length )
               {
                  if ( p_bq->buffer_count == 0 )
                  {
                     free( ibuf );
                  }
                  return SHORT_READ;
               }
               length = 0;
            }
            else
            {
               new_read_atoms = (LONGLONG)( fam->cur_st_file_size - offset )
                                / EXT_SIZE( fam, data_type );
               read_cnt = fam->state_read_funcs[data_type]( fam->cur_st_file,
                          (char *) ibuf+buf_pos,
                          new_read_atoms );
               if ( read_cnt < new_read_atoms )
               {
                  if ( p_bq->buffer_count == 0 )
                  {
                     free( ibuf );
                  }
                  return SHORT_READ;
               }
               offset = 0;
               file_num++;
               rval = state_file_close( fam );
               if (rval != OK)
               {
                  return rval;
               }
               rval = state_file_open( fam, file_num, fam->access_mode );
               if (rval != OK)
               {
                  return rval;
               }
               length -= new_read_atoms;
               buf_pos += ( new_read_atoms * EXT_SIZE( fam, data_type ));
            }
         }
      }
      else
      {
         read_cnt = fam->state_read_funcs[data_type]( fam->cur_st_file,
                    (char *) ibuf,
                    read_atoms );
         if ( read_cnt != read_atoms )
         {
            if ( p_bq->buffer_count == 0 )
            {
               free( ibuf );
            }
            return SHORT_READ;
         }
      }

   }

   /* Loop over requested svars and extract each from subrecord. */
   rval = OK;
   for ( i = 0; i < qty; i++ )
   {
      qty_facets = 1;
      if( superclass == M_SURFACE )
      {
         if ( p_subrec->surface_variable_flag != NULL )
         {
            switch( p_subrec->surface_variable_flag[ i ] )
            {
               case PER_FACET:

                  id = p_subrec->mo_id_blks[ 0 ];
                  qty_facets = p_mocd->surface_sizes[ id - 1 ];
                  break;

               case PER_OBJECT:

                  qty_facets = 1;
                  break;

               default:

                  return INVALID_DATA_TYPE;
            }
         }
      }

      idx = refs[i].index;
      data_type = *p_subrec->svars[idx]->data_type;
      obj_vec_size = p_subrec->lump_atoms[0];

      /* Calculate offset to current svar in object vector. */
      for ( j = 0, obj_vec_offset = 0; j < idx; j++ )
      {
         obj_vec_offset += (svar_atom_qty( p_subrec->svars[j] )*qty_facets );
      }

      /* Get appropriate function to move data to output buffer. */
      dist_func = get_data_dist_func( data_type, refs[i].reqd_qty,
                                      obj_vec_size );
      if ( dist_func != NULL )
      {
         /* Move data. */
         dist_func( (void *) ibuf, p_subrec->mo_qty*qty_facets,
                    obj_vec_size/qty_facets, refs[i].offset + obj_vec_offset,
                    refs[i].reqd_qty, p_out[i] );
      }
      else
      {
         rval = INVALID_DATA_TYPE;
         break;
      }
   }

   if ( p_bq->buffer_count == 0 )
   {
      free( ibuf );
   }

   return rval;
}


/*****************************************************************
 * TAG( get_data_dist_func ) LOCAL
 *
 * Get result arrays from an object-ordered database subrecord.
 */
static void
(*get_data_dist_func( int data_type, int length, int cell_size ))()
{
   if ( internal_sizes[data_type] == 4 )
   {
      if ( length == cell_size )
      {
         return distribute_all_4;
      }
      else if ( length == 1 )
      {
         return distribute_scalar_4;
      }
      else
      {
         return distribute_vector_4;
      }
   }
   else if ( internal_sizes[data_type] == 8 )
   {
      if ( length == cell_size )
      {
         return distribute_all_8;
      }
      else if ( length == 1 )
      {
         return distribute_scalar_8;
      }
      else
      {
         return distribute_vector_8;
      }
   }
   else
   {
      return NULL;
   }
}


/*****************************************************************
 * TAG( distribute_all_4 ) LOCAL
 *
 * Copy all values from input buffer to output buffer in four-byte
 * chunks.
 */
static void
distribute_all_4( void *p_input, int cell_qty, int cell_size,
                  int cell_offset, int length, void *p_output )
{
   int *p_in, *p_out;
   int i, total;

   p_in = (int *) p_input;
   p_out = (int *) p_output;
   total = cell_qty * cell_size;

   for ( i = 0; i < total; i++ )
   {
      *(p_out + i) = *(p_in + i);
   }
}


/*****************************************************************
 * TAG( distribute_all_8 ) LOCAL
 *
 * Copy all values from input buffer to output buffer in eight-byte
 * chunks.
 */
static void
distribute_all_8( void *p_input, int cell_qty, int cell_size,
                  int cell_offset, int length, void *p_output )
{
   double *p_in, *p_out;
   int i, total;

   p_in = (double *) p_input;
   p_out = (double *) p_output;
   total = cell_qty * cell_size;

   for ( i = 0; i < total; i++ )
   {
      *(p_out + i) = *(p_in + i);
   }
}


/*****************************************************************
 * TAG( distribute_scalar_4 ) LOCAL
 *
 * Distribute scalar four-byte quantity from input buffer to
 * output buffer.
 */
static void
distribute_scalar_4( void *p_input, int cell_qty, int cell_size,
                     int cell_offset, int length, void *p_output )
{
   int *p_in, *p_out;
   int i;

   p_in = (int *) p_input + cell_offset;
   p_out = (int *) p_output;

   for ( i = 0; i < cell_qty; i++ )
   {
      *(p_out + i) = *(p_in + i * cell_size);
   }
}


/*****************************************************************
 * TAG( distribute_scalar_8 ) LOCAL
 *
 * Distribute scalar eight-byte quantity from input buffer to
 * output buffer.
 */
static void
distribute_scalar_8( void *p_input, int cell_qty, int cell_size,
                     int cell_offset, int length, void *p_output )
{
   double *p_in, *p_out;
   int i;

   p_in = (double *) p_input + cell_offset;
   p_out = (double *) p_output;

   for ( i = 0; i < cell_qty; i++ )
   {
      *(p_out + i) = *(p_in + i * cell_size);
   }
}


/*****************************************************************
 * TAG( distribute_vector_4 ) LOCAL
 *
 * Distribute vector four-byte quantities from input buffer to
 * output buffer.
 */
static void
distribute_vector_4( void *p_input, int cell_qty, int cell_size,
                     int cell_offset, int length, void *p_output )
{
   int *p_in, *p_out;
   int i, j;

   p_in = (int *) p_input + cell_offset;
   p_out = (int *) p_output;

   for ( i = 0; i < cell_qty; i++ )
   {
      for ( j = 0; j < length; j++ )
      {
         *(p_out + j) = *(p_in + j);
      }

      p_out += length;
      p_in += cell_size;
   }
}


/*****************************************************************
 * TAG( distribute_vector_8 ) LOCAL
 *
 * Distribute vector eight-byte quantities from input buffer to
 * output buffer.
 */
static void
distribute_vector_8( void *p_input, int cell_qty, int cell_size,
                     int cell_offset, int length, void *p_output )
{
   double *p_in, *p_out;
   int i, j;

   p_in = (double *) p_input + cell_offset;
   p_out = (double *) p_output;

   for ( i = 0; i < cell_qty; i++ )
   {
      for ( j = 0; j < length; j++ )
      {
         *(p_out + j) = *(p_in + j);
      }

      p_out += length;
      p_in += cell_size;
   }
}


/*****************************************************************
 * TAG( mc_wrt_stream ) PUBLIC
 *
 * Write state data to disk as a sequential stream of words
 * of a single data type.  With this interface, application
 * bears responsibility for sequencing multiple calls per state
 * properly, since no disk seeking is performed.
 */
Return_value
mc_wrt_stream( Famid fam_id, int type, LONGLONG qty, void *data )
{
   LONGLONG write_ct;
   LONGLONG byte_ct;

   write_ct = (fam_list[fam_id]->state_write_funcs[type])( fam_list[fam_id]->cur_st_file,
              data, qty );
   if (write_ct != qty)
   {
      return SHORT_WRITE;
   }

   byte_ct = mc_calc_bytecount( type, qty );
   if (byte_ct == 0)
   {
      return INVALID_DATA_TYPE;
   }
   fam_list[fam_id]->cur_st_file_size += byte_ct;

   return OK;
}


/*****************************************************************
 * TAG( mc_wrt_subrec ) PUBLIC
 *
 * Write state sub-record data to disk.  All "lumps" written in one
 * call are assumed to have the same data type.  For object-ordered
 * data, start and stop indicate order numbers for the objects
 * represented in the subrecord.  For result-ordered data, start and
 * stop indicate order numbers of the state variables (results) in
 * the state vector bound to the subrecord.
 */
Return_value
mc_wrt_subrec( Famid fam_id, char *subrec_name, int start, int stop,
               void *data )
{
   Mili_family *fam;
   Sub_srec *psubrec;
   Htable_entry *subrec_entry;
   LONGLONG *lump_offsets;
   LONGLONG loc, qty;
   int srec_id, mesh_id, superclass;
   int start_i, stop_i;
   int type;
   Return_value rval;
   LONGLONG write_ct;
   int byte_ct;

   fam = fam_list[fam_id];
   rval = htable_search( fam->subrec_table, subrec_name,
                         FIND_ENTRY, &subrec_entry );
   if ( subrec_entry == NULL )
   {
      return rval;
   }

   srec_id = fam->qty_srecs - 1;
   rval = mc_query_family( fam_id, SREC_MESH, &srec_id, NULL,
                           (void *)&mesh_id );
   if ( OK  !=  rval )
   {
      return( rval );
   }

   psubrec = (Sub_srec *) subrec_entry->data;
   rval = mc_query_family( fam_id, CLASS_SUPERCLASS, &mesh_id, psubrec->mclass,
                           (void *)&superclass );
   if ( rval != OK )
   {
      return rval;
   }

   rval = mc_query_family( fam_id, CLASS_SUPERCLASS, &mesh_id, psubrec->mclass,
                           (void *)&superclass );
   if ( rval != OK )
   {
      return rval;
   }

   start_i = start - 1;
   type = *psubrec->svars[start_i]->data_type;

   if ( psubrec->organization == OBJECT_ORDERED )
   {
      /* Calc file offset. */
      loc = fam->cur_st_offset + psubrec->offset
            + psubrec->lump_sizes[0] * start_i;
      /* Calc atom count. */
      if( superclass != M_SURFACE )
      {
         qty = psubrec->lump_atoms[0] * (stop - start + 1);
      }
      else
      {
         qty = psubrec->lump_atoms[0];
      }
   }
   else
   {
      stop_i = stop - 1;
      lump_offsets = psubrec->lump_offsets;

      /* Calc file offset. */
      loc = fam->cur_st_offset + psubrec->offset
            + lump_offsets[start_i];
      /* Calc atom count. */
      qty = (lump_offsets[stop_i] - lump_offsets[start_i]
             + psubrec->lump_sizes[stop_i])
            / EXT_SIZE( fam, type );
   }

   rval = seek_state_file( fam->cur_st_file, loc );
   if (rval != OK)
   {
      return rval;
   }

   write_ct = (fam->state_write_funcs[type])( fam->cur_st_file, 
               data, qty );
   if (write_ct != qty)
   {
      return SHORT_WRITE;
   }

   byte_ct = mc_calc_bytecount( type, qty );
   if (byte_ct == 0)
   {
      return INVALID_DATA_TYPE;
   }
   fam_list[fam_id]->cur_st_file_size += byte_ct;

   return OK;
}

/*****************************************************************
 * TAG( load_static_map ) PRIVATE
 *
 * Build file/offset map of state-data records in family.
 */
Return_value
load_static_maps( Mili_family *fam, Bool_type initial_build )
{
   Return_value rval= OK;
   long offset;
   int status,nitems,state_count; 
   int header[QTY_DIR_HEADER_FIELDS];
   int cur_state;
   FILE *fp = NULL; 
   
   int file,srec_id;
   LONGLONG file_offset;
   float time;
   int file_count=-1;
   offset = 0;
   char fname[1024]; 
   
   strcpy(fname,fam->aFile);
   
   if(fam->access_mode == 'r')
   {
      fp = fopen(fname,"rb");
   }else
   {
      fp = fopen(fname,"r+b");
   }
   if(fp) {
      
      offset = -(QTY_DIR_HEADER_FIELDS) * EXT_SIZE( fam, M_INT );
      status = fseek( fp, offset, SEEK_END );
      if ( status != 0 )
      {
         fclose(fp);;
         return SEEK_FAILED;
      }
      nitems = fam->read_funcs[M_INT]( fp, header, QTY_DIR_HEADER_FIELDS );
      if ( nitems != QTY_DIR_HEADER_FIELDS )
      {
         fclose( fp );
         return BAD_LOAD_READ;
      }
      state_count = header[QTY_STATES_IDX];
      fam->state_qty = state_count;
      offset -= state_count * 20;
      status = fseek( fp, offset, SEEK_END );
      if ( status != 0 )
      {
         fclose(fp);
         return SEEK_FAILED;
      }
      fam->state_map = NEW_N(State_descriptor,state_count,"State map descriptors");
      for(cur_state = 0; cur_state < state_count; cur_state++) {
      
         fam->read_funcs[M_INT](fp,&file,1);
         fam->read_funcs[M_INT8](fp,&file_offset,1);
         fam->read_funcs[M_FLOAT](fp,&time,1);
         fam->read_funcs[M_INT](fp,&srec_id,1);
         fam->state_map[cur_state].file = file;
         fam->state_map[cur_state].offset = file_offset;
         fam->state_map[cur_state].time = time;
         fam->state_map[cur_state].srec_format= srec_id;
         if(file >file_count){
            file_count=file;
            fam->st_file_count = file+1;
            fam->file_map = RENEWC_N( State_file_descriptor, fam->file_map,
                                  file, 1, "New file map entry" );
         }
         fam->file_map[file].state_qty++;
      }
      
      fclose(fp);
       
   }
   if(state_count >0)
   {
       rval = state_file_open( fam, fam->st_file_count-1,
                               fam->access_mode );
       if ( rval != OK )
       {
           return rval;
       }
   }
   
   
   return rval;
}

/*****************************************************************
 * TAG( mc_reload_states ) PUBLIC
 *
 * External access for codes to call and reload the database times
 */
Return_value
mc_reload_states( Famid famid)
{
    return load_static_maps(fam_list[famid],0);
}

/*****************************************************************
 * TAG( update_static_map ) PRIVATE
 *
 * Write the State_descriptor to the A statefile.
 */
Return_value 
update_static_map(Famid fam_id,State_descriptor* p_sd) {
   /**
   We will need to seek to the end of the file minus the header size. 
   Read in the header data.
   Write the timestep information.
   Increment the  QTY_STATES_IDX value by one.
   Rewrite the header the header information.
   Then flush the non-state file.
   */
   Mili_family *fam = fam_list[fam_id];
   long offset = 0;
   int status,
       nitems,
       num_written;
   int header[QTY_DIR_HEADER_FIELDS];
   Return_value rval = OK; 
   FILE *fp = NULL;
   char fname[1024]; 
   
   strcpy(fname,fam->aFile);
   
   fp = fopen(fname,"r+b");

   if(fp)
   {
      offset = -(QTY_DIR_HEADER_FIELDS) * EXT_SIZE( fam, M_INT );
      status = fseek( fp, offset, SEEK_END );
      if ( status != 0 )
      {
         fclose(fp);
         return SEEK_FAILED;
      }
      nitems = fam->read_funcs[M_INT]( fp, header, QTY_DIR_HEADER_FIELDS );
      if ( nitems != QTY_DIR_HEADER_FIELDS )
      {
         fclose( fp );
         return BAD_LOAD_READ;
      }
      header[QTY_STATES_IDX]++;
      
      // Reset the point to current start of the header information
      status = fseek( fp, offset, SEEK_END );
      fam->write_funcs[M_INT](fp,&(p_sd->file),1);
      fam->write_funcs[M_INT8](fp,&(p_sd->offset),1);
      fam->write_funcs[M_FLOAT](fp,&(p_sd->time),1);
      fam->write_funcs[M_INT](fp,&(p_sd->srec_format),1);
      num_written = 
            fam->write_funcs[M_INT]( fp, header, QTY_DIR_HEADER_FIELDS );
      fclose(fp);
      if (num_written != QTY_DIR_HEADER_FIELDS)
      {
         return SHORT_WRITE;
      }
      mc_wrt_scalar(fam_id,M_INT,"state_count",
                    (void*)&header[QTY_STATES_IDX]);
   }else
   {
       rval = NO_A_FILE_FOR_STATEMAP;
   }
   
   return rval;
}
/*****************************************************************
 * TAG( mc_end_state ) PUBLIC
 *
 * Close a state and update the mapping in the A file.
 */
Return_value
mc_end_state( Famid fam_id, int srec_id)
{
   Mili_family *fam;
   int state_qty;
   State_descriptor *p_sd;
   Return_value rval = OK;
   LONGLONG position =0, target =0;
   
   if ( INVALID_FAM_ID( fam_id ) )
   {
      return BAD_FAMILY;
   }

   fam = fam_list[fam_id];

   if ( srec_id < 0 || srec_id >= fam->qty_srecs )
   {
      return INVALID_SREC_INDEX;
   }
   if(fam->cur_st_file == NULL)
   {
      
      rval = state_file_open( fam, fam->st_file_count-1,
                               fam->access_mode );
      if ( rval != OK )
      {
          return rval;
      }
      fseek( fam->cur_st_file, 0, SEEK_END );
   }
   position = ftell(fam->cur_st_file);
   target = fam->state_map[fam->state_qty-1].offset + 
            fam->srecs[0]->size + sizeof(int)*2;
   
   if(position != target)
   {
      return position <target ? INVALID_SR_OFFSET_UNDER:INVALID_SR_OFFSET_OVER;
   }
   /* Add a new entry in the state map. */
   state_qty = fam->state_qty;
   if(fam->char_header[DIR_VERSION_IDX]>1){
      fam->state_dirty = 0;
      if(!(fam->state_closed) && state_qty >0){
         p_sd = fam->state_map + (state_qty-1);
         rval =  update_static_map(fam_id,p_sd);
      }
   }
   if(rval == OK)
   {
       fam->state_closed = 1;
       mc_update_visit_file(fam_id);
   }
   return rval;
}
/*****************************************************************
 * TAG( mc_new_state ) PUBLIC
 *
 * Set db to receive data for a new state.
 */
Return_value
mc_new_state( Famid fam_id, int srec_id, float time, int *p_file_suffix,
              int *p_file_state_index )
{
   Mili_family *fam;
   Return_value rval;
   int state_qty;
   State_descriptor *p_sd;
   long write_ct;

   if ( INVALID_FAM_ID( fam_id ) )
   {
      return BAD_FAMILY;
   }

   fam = fam_list[fam_id];

   if ( srec_id < 0 || srec_id >= fam->qty_srecs )
   {
      return INVALID_SREC_INDEX;
   }

   
   rval = prep_for_new_data( fam, STATE_DATA );
   if ( rval != OK )
   {
      return rval;
   }

   /*
    * Do this after prep_for_new_data(), since prep_for_new_data() advances
    * the current state offset (file pointer) according to the format of the
    * state record preceding this one, as identified by fam->cur_srec_id.
    */
   fam->cur_srec_id = srec_id;

   /* Write state record header. */
   write_ct = (*fam->state_write_funcs[M_FLOAT])(fam->cur_st_file, &time, 1);
   if (write_ct != 1)
   {
      return SHORT_READ;
   }
   write_ct= (*fam->state_write_funcs[M_INT])(fam->cur_st_file, &srec_id, 1);
   if (write_ct != 1)
   {
      return SHORT_READ;
   }

   /* Update byte count */
   fam->cur_st_file_size += 8; /* time + srec_id */

   /* Add a new entry in the state map. */
   state_qty = fam->state_qty;
   if(fam->char_header[DIR_VERSION_IDX]>1){
      /* Set the dirty flag just in case the code closes the 
       * database without calling close_state()
       */
      fam->state_dirty = 1;
      
      /* If we failed to close the previous state do so now. */
      if(!(fam->state_closed) && state_qty >0)
      {
         p_sd = fam->state_map + state_qty -1;
         rval = update_static_map(fam_id,p_sd);
         if(rval)
         {
            return rval;
         }
      }
   }
   
   fam->state_qty++;
   
   if(fam->state_map == NULL)
   {
      fam->state_map = NEW_N( State_descriptor, fam->state_qty ,"Recreating state descriptor");
   }else
   {
      fam->state_map = RENEW_N( State_descriptor, fam->state_map, state_qty, 1,
                                "Addl state descr" );
   }
   
   if (fam->state_map == NULL)
   {
      return ALLOC_FAILED;
   }
   p_sd = fam->state_map + state_qty;
   p_sd->file = fam->cur_st_index;
   p_sd->offset = fam->cur_st_offset;
   p_sd->time = time;
   p_sd->srec_format = srec_id;
   
   fam->written_st_qty++;
   fam->file_st_qty++;
   fam->file_map[fam->st_file_count - 1].state_qty = fam->file_st_qty;

   /*
    * Position the current state file offset to the end of
    * data that Mili writes so it will be positioned properly
    * for application data I/O that needs it.
    */
   fam->cur_st_offset += EXT_SIZE( fam, M_FLOAT ) + EXT_SIZE( fam, M_INT );

   *p_file_suffix = ST_FILE_SUFFIX( fam, fam->cur_st_index );
   *p_file_state_index = fam->file_st_qty - 1;
   fam->state_closed = 0;
   return OK;
}


/*****************************************************************
 * TAG( find_state_file_index ) LOCAL
 *
 * This function finds the file index based on the global state number.
 * It will return the state file index and set the global_state_index
 * to the file_state_index;
 */
static int find_file_index(Famid fam_id, int *global_state_index)
{
   int global_index = *global_state_index;

   int file_index = 0,
       current_count,
       i,j;

   Mili_family *fam;
   fam = fam_list[fam_id];

   /* Sanity check - no states in family. */
   if ( fam->st_file_count != 0 )
   {
      /* Can't permit a gap in state sequence. */
      if ( global_index <=  fam->state_qty )
      {
         current_count = fam->state_qty;
         for ( i = fam->st_file_count-1;
               i>0 &&  current_count > global_index; i-- )
         {
            current_count -= fam->file_map[i].state_qty;
         }
         /*Check if we need to subtract out the previous states quantities*/
         file_index = i;
         for(j = i-1; j>=0; j--)
         {
            *global_state_index -= fam->file_map[j].state_qty;
         }
      }
   }
   return file_index;

}

int 
find_state_index(Mili_family *fam, int file_index, int local_index)
{
    int index =0;
    State_descriptor *state_map = fam->state_map;
    
    while(state_map[index].file < file_index)
    {
        index++;
    }
    
    return index + local_index;
}
/*****************************************************************
 * TAG( mc_restart_at_state ) PUBLIC
 *
 * Set db to receive data by overwriting at the specified state.
 */
Return_value
mc_restart_at_state( Famid fam_id, int file_name_index, int state_index )
{
   Mili_family *fam;
   Return_value rval = OK;

   fam = fam_list[fam_id];
   
   CHECK_WRITE_ACCESS( fam )
   
   if(file_name_index > 0)
   {
       state_index = find_state_index(fam, file_name_index, state_index);
   }
   
   /* Can't permit a gap in state sequence. */
   if ( state_index > fam->state_qty )
   {
      return INVALID_FILE_STATE_INDEX;
   }
   if(state_index < fam->state_qty)
   {
       rval = truncate_family( fam, state_index);
   }
   
   return rval;
}


/*****************************************************************
 * TAG( mc_restart_at_file ) PUBLIC
 *
 * Set db to receive data by overwriting at the specified file.
 */
Return_value
mc_restart_at_file( Famid fam_id, int file_name_index )
{
   Mili_family *fam;
   State_file_descriptor *p_sfd;
   int i;
   int state_index, file_index;
   Return_value rval = OK;

   fam = fam_list[fam_id];

   CHECK_WRITE_ACCESS( fam )

   /* Sanity check - no states in family. */
   if ( fam->st_file_count == 0 )
   {
      fam->st_file_index_offset = file_name_index;
      return OK;
   }

   /* Can't restart with a name that creates a gap in the name sequence. */
   if ( file_name_index > fam->st_file_index_offset + fam->st_file_count )
   {
      return INVALID_FILE_NAME_INDEX;
   }

   /* Suffix must be greater than or equal to zero. */
   file_index = file_name_index - fam->st_file_index_offset;
   if ( file_index < 0 )
   {
      return INVALID_FILE_NAME_INDEX;
   }

   /* Specifying restart on the next sequential member is just an append. */
   if ( file_index == fam->st_file_count )
   {
      if ( fam->cur_st_file != NULL )
      {
         rval = state_file_close( fam );
      }

      return rval;
   }

   /* Get the index of the first state in the indexed file. */
   p_sfd = fam->file_map;
   state_index = 0;
   for ( i = 0; i < file_index && i < fam->st_file_count; i++ )
   {
      state_index += p_sfd[i].state_qty;
   }

   /* Remove existing state records and files at and after "state_index". */
   rval = truncate_family ( fam, state_index );

   return rval;
}

/*****************************************************************
 * TAG( mc_rewrite_subrec ) PUBLIC
 *
 * Rewrite a subrecord at a given timestep. This timestep must
 * have already been written. This function calls mc_wrt_subrec.
 */
Return_value
mc_rewrite_subrec( Famid fid, char *subrec_name, int start, int stop, void *data, int st_index )
{
   State_descriptor *sd;
   LONGLONG saved_st_offset;
   LONGLONG saved_st_file_size;
   LONGLONG replace_st_offset;
   Return_value rval = OK;
   int status;

   Htable_entry *subrec_entry;
   Sub_srec *psubrec;
   int found;
   Buffer_queue *bq;
 

   if ( INVALID_FAM_ID( fid ) )
   {
      return BAD_FAMILY;
   }

   if ( st_index > fam_list[fid]->state_qty || st_index < 0 )
   {
      return INVALID_STATE;
   }
   
   /* Save current state offset and current state file size */
   saved_st_offset = fam_list[fid]->cur_st_offset;
   saved_st_file_size = fam_list[fid]->cur_st_file_size;

   /* Get offset of this timestep from State_descriptor */
   sd = fam_list[fid]->state_map;
   replace_st_offset = sd[st_index-1].offset;

   /* Add 8 bytes for data at beginning of state files */
   replace_st_offset += ( EXT_SIZE( fam_list[fid], M_INT ) + EXT_SIZE( fam_list[fid], M_FLOAT ) );

   /* Set offset to state to change */
   fam_list[fid]->cur_st_offset = replace_st_offset;

   
   /* Rewrite the subrecord */
   status = mc_wrt_subrec( fid, subrec_name, start, stop, data );
   if ( status != 0 )
   {
     rval = status;
   }
   
   /* Set subrec ibuffer buffer_count to 0 to prevent buffer read */
   rval = htable_search( fam_list[fid]->subrec_table, subrec_name,
			 FIND_ENTRY, &subrec_entry );
   if(subrec_entry == NULL)
     {
       //An error at this point since this should have been caught already in mc_wrt_subrec()
       return INVALID_SUBREC_INDEX;
     }
   psubrec = (Sub_srec*) subrec_entry->data;
 
   bq = psubrec->ibuffer;
 
   bq->buffer_count = 0;

   /* Reset current state offset and current state file size */
   fam_list[fid]->cur_st_offset = saved_st_offset;
   fam_list[fid]->cur_st_file_size = saved_st_file_size;

   return rval;
}



/*****************************************************************
 * TAG( truncate_family ) LOCAL
 *
 * Prune from a family all states and files including and following
 * the state indicated by "st_index".
 */
static Return_value
truncate_family( Mili_family *p_fam, int st_index )
{
   char fname[M_MAX_NAME_LEN];
   Return_value rval;
   int status;
   int file_qty, state_qty, remain_states;
   int cur_st_index, cur_file;
   int i;
   long offset;
   int header[QTY_DIR_HEADER_FIELDS];
   int count;
   FILE *fp = NULL;
   
   if(p_fam->state_qty == st_index)
   {
      return OK;
   }else if(p_fam->state_qty < st_index)
   {
      return INVALID_FILE_STATE_INDEX;
   }
      
   offset = p_fam->state_map[st_index].offset;
   state_qty = p_fam->state_qty;
 
   /* Make sure any file that will be affected is closed. */
   if ( p_fam->cur_st_index >= p_fam->state_map[st_index].file )
   {
      rval = state_file_close( p_fam );
      if (rval != OK)
      {
         return rval;
      }
   }

   /* Get quantity of state files remaining after truncation. */
   file_qty = p_fam->state_map[st_index].file;
   
   for(i = p_fam->st_file_count-1; i > file_qty; i--)
   {
      make_fnam( STATE_DATA, p_fam, ST_FILE_SUFFIX( p_fam, i ),
                 fname );
      status = unlink( fname );
      if ( status != 0 )
      {
         rval = FAMILY_TRUNCATION_FAILED;
         break;
      }
   }
   
   p_fam->st_file_count = i+1;
   
   if(p_fam->state_map[st_index].file == 0 && st_index == 0)
   {
      make_fnam( STATE_DATA, p_fam, ST_FILE_SUFFIX( p_fam, 0 ),
                 fname ); 
      status = unlink( fname );
      if ( status != 0 )
      {
         return FAMILY_TRUNCATION_FAILED;
      }
   }else if( p_fam->state_map[st_index].offset == 0 )
   {
      make_fnam( STATE_DATA, p_fam, ST_FILE_SUFFIX( p_fam, 
                 p_fam->state_map[st_index].file ), fname ); 
      status = unlink( fname );
      if ( status != 0 )
      {
         return FAMILY_TRUNCATION_FAILED;
      }
      p_fam->st_file_count -= 1;
   }else
   {
      make_fnam( STATE_DATA, p_fam, ST_FILE_SUFFIX( p_fam, 
                 p_fam->state_map[st_index].file ),
                 fname );
      #if defined(_WIN32) || defined(WIN32)
         status = 0;
      #else
         status = truncate( fname, p_fam->state_map[st_index].offset );
      #endif
      status = state_file_open(p_fam, p_fam->state_map[st_index].file, 
                               p_fam->access_mode);
   }
   

      
   /* Clean up the A file to remove the states from the state maps contained there*/
   if(p_fam->char_header[DIR_VERSION_IDX]>1) {
      fp = fopen(p_fam->aFile,"r+b");
      
      if(fp)
      {
         offset = -(QTY_DIR_HEADER_FIELDS) * EXT_SIZE( p_fam, M_INT );
         if ( status != 0 )
         {
            fclose(fp);
            return SEEK_FAILED;
         }
         fseek( fp, offset, SEEK_END );
         count = p_fam->read_funcs[M_INT]( fp, header, QTY_DIR_HEADER_FIELDS );
         if ( count != QTY_DIR_HEADER_FIELDS )
         {
             fclose( fp );
             return BAD_LOAD_READ;
         }
         
         offset -= header[QTY_STATES_IDX]*20;
         status = fseek(fp, offset, SEEK_END);
         if(st_index >0)
         {
             status = fseek(fp, st_index*20 ,SEEK_CUR);
         }
         offset = ftell(fp);
         
         fclose(fp);
#if !(defined(_WIN32) || defined(WIN32))
         truncate(p_fam->aFile,offset);
#endif
         fp = fopen(p_fam->aFile, "r+b");
         fseek( fp, 0, SEEK_END );
         header[QTY_STATES_IDX] = st_index;
         count = p_fam->write_funcs[M_INT]( fp, header, QTY_DIR_HEADER_FIELDS );
         if ( count != QTY_DIR_HEADER_FIELDS )
         {
             fclose(fp);
             return SHORT_WRITE;
         }
         fclose(fp);         
      }
      
   }
   
   /* Clean up state_map*/
   if ( rval == OK )
   {
      if ( st_index == 0 )
      {
         memset( (void *) fname, (int) 0, 64 );

         free( p_fam->state_map );
         p_fam->state_map = NULL;
         free( p_fam->file_map );
         p_fam->file_map = NULL;
         p_fam->state_qty = 0;
         p_fam->cur_st_file_size = 0;
         p_fam->file_st_qty = 0;
         p_fam->st_file_count = 0;
      }
      else
      {
         make_fnam( STATE_DATA, p_fam,
                    ST_FILE_SUFFIX( p_fam, p_fam->state_map[st_index].file ),
                    fname );

         State_descriptor *temp = NEW_N(State_descriptor,
                                     st_index,
                                     "Shrunken state map on restart" );
         
         memcpy((void*) temp, p_fam->state_map,st_index*sizeof(State_descriptor));
         free(p_fam->state_map);
         p_fam->state_map = temp;
         temp = NULL;
         if (st_index > 0 && p_fam->state_map == NULL)
         {
            rval = ALLOC_FAILED;
         }

         if ( file_qty+1 != p_fam->st_file_count )
         {
            p_fam->file_map = RENEW_N( State_file_descriptor,
                                       p_fam->file_map,
                                       p_fam->st_file_count, file_qty,
                                       "Shrunken file map on restart" );
         }
         if (p_fam->file_map == NULL)
         {
            rval = ALLOC_FAILED;
         }
         else
         {
            p_fam->state_qty = st_index;
            p_fam->file_st_qty = ((p_fam->state_map[st_index-1].offset)/(p_fam->srecs[0]->size+8))+1; 
            p_fam->file_map[p_fam->state_map[st_index-1].file].state_qty = p_fam->file_st_qty;
            
         }
      }
   }

   return rval;
}


/*****************************************************************
 * TAG( delete_subrec ) PRIVATE
 *
 * Delete a subrecord descriptor.
 */
void
delete_subrec( void *ptr_subrec )
{
   Sub_srec *psubrec;

   psubrec = (Sub_srec *) ptr_subrec;

   /* Delete the name. */
   if ( psubrec->name != NULL )
   {
      free( psubrec->name );
   }

   /* Delete the class name. */
   if ( psubrec->mclass != NULL )
   {
      free( psubrec->mclass );
   }

   /* Delete the svar references. */
   if ( psubrec->svars != NULL )
   {
      free( psubrec->svars );
   }

   /* Delete the blocks. */
   if ( psubrec->mo_id_blks != NULL )
   {
      free( psubrec->mo_id_blks );
   }

   /* Delete lump_atoms array. */
   if ( psubrec->lump_atoms != NULL )
   {
      free( psubrec->lump_atoms );
   }

   /* Delete lump_sizes array. */
   if ( psubrec->lump_sizes != NULL )
   {
      free( psubrec->lump_sizes );
   }

   /* Delete lump_offsets array. */
   if ( psubrec->lump_offsets != NULL )
   {
      free( psubrec->lump_offsets );
   }

   /* Delete surface_variable_flag array. */
   if ( psubrec->surface_variable_flag != NULL )
   {
      free( psubrec->surface_variable_flag );
   }

   /* Delete the input buffer queue. */
   delete_buffer_queue( psubrec->ibuffer );
   /*
       if ( psubrec->ibuffer != NULL )
       {
           p_bq = psubrec->ibuffer;

           for ( i = 0; i < p_bq->buffer_count; i++ ) {
               free( p_bq->data_buffers[i] );
           }
           free( p_bq->data_buffers );

           if ( p_bq->state_numbers != NULL ) {
               free( p_bq->state_numbers );
           }

           free( p_bq );
       }
   */

   /* Delete the subrec entry. */
   free( psubrec );
}


/*****************************************************************
 * TAG( delete_srecs ) PRIVATE
 *
 * Delete all state record descriptors from a family.
 */
void
delete_srecs( Mili_family *fam )
{
   int i;

   /* For each state record descriptor... */
   for ( i = 0; i < fam->qty_srecs; i++ )
   {
      free( fam->srecs[i]->subrecs );
      free( fam->srecs[i] );
   }

   free( fam->srecs );
   free( fam->srec_meshes );

   /* Delete subrec descriptors. */
   htable_delete( fam->subrec_table, delete_subrec, TRUE );

   fam->srecs = NULL;
   fam->srec_meshes = NULL;
   fam->subrec_table = NULL;
}


/*****************************************************************
 * TAG( build_state_map ) PRIVATE
 *
 * Build file/offset map of state-data records in family.
 */
Return_value
build_state_map( Mili_family *fam, Bool_type initial_build )
{
   State_file_descriptor **p_fmap;
   State_descriptor **p_smap;
   int index;
   LONGLONG offset;
   LONGLONG (*readi)();
   LONGLONG (*readf)();
   float st_time;
   int srec_id;
   int state_qty;
   int hdr_size;
   Return_value rval;
   State_descriptor *p_sd;
   Bool_type initial;
   Bool_type has_state_record = TRUE;

   offset = 0;
   if(fam->char_header[DIR_VERSION_IDX]>1){
      return load_static_maps(fam,initial_build);
   }

   p_fmap = &fam->file_map;
   p_smap = &fam->state_map;
   readi = fam->state_read_funcs[M_INT];
   readf = fam->state_read_funcs[M_FLOAT];
   hdr_size = EXT_SIZE( fam, M_INT ) + EXT_SIZE( fam, M_FLOAT );
   rval = OK;
   initial = initial_build;
   if(!fam->srecs){
      has_state_record = FALSE;
   }
   if ( initial_build )
   {
      rval = get_start_index( fam );
      if (rval != OK)
      {
         return rval;
      }
      index = 0;
      state_qty = 0;

      /*        fam->st_file_count = 1; */
   }
   else
   {
      index = fam->st_file_count - 1;
      state_qty = fam->state_qty;

      /* Count offset through mapped states in known last file. */
      offset = 0;
      for ( p_sd = fam->state_map + fam->state_qty - 1;

            ( (LONGLONG) p_sd >= (LONGLONG) fam->state_map ) && p_sd->file == index;

            p_sd-- )
      {
         offset += fam->srecs[p_sd->srec_format]->size + hdr_size;
      }
   }
   if(fam->hide_states)
   {
      fam->st_file_count =0;
   }
   else
   {

      /* Traverse the state data files. */
      while ( state_file_open( fam, index, 'r' ) == OK )
      {
         if ( initial )
         {
            offset = 0;
            fam->file_st_qty = 0;

            (*p_fmap) = RENEWC_N( State_file_descriptor, *p_fmap,
                                  index+1, 1, "New file map entry" );
            if (*p_fmap == NULL)
            {
               return ALLOC_FAILED;
            }
         }
         else
         {
            initial = TRUE;
         }

         /* Traverse the state records within the current file. */
         while ( offset < fam->cur_st_file_size &&
                 seek_state_file( fam->cur_st_file, offset ) == OK )
         {
            /* Read the header - time and format. */
            if ( readf( fam->cur_st_file, &st_time, 1 ) != 1 )
            {
               rval = SHORT_READ;
               break;
            }

            if ( readi( fam->cur_st_file, &srec_id, 1 ) != 1 )
            {
               rval = SHORT_READ;
               break;
            }

            /* Add a new entry in the state map. */
            *p_smap = RENEW_N( State_descriptor, *p_smap, state_qty, 1,
                               "Addl state descr" );
            if (*p_smap == NULL)
            {
               rval = ALLOC_FAILED;
               break;
            }

            (*p_smap)[state_qty].file = index;
            (*p_smap)[state_qty].offset = offset;
            (*p_smap)[state_qty].time = st_time;
            (*p_smap)[state_qty].srec_format = srec_id;

            /* Update stuff... */
            if(has_state_record){
               offset += fam->srecs[srec_id]->size + hdr_size;
            }else{
               offset += hdr_size;
            }
            fam->cur_st_offset = offset;
            state_qty++;
            fam->state_qty = state_qty;
            fam->file_st_qty++;
         }

         /* Prepare for next state data file in family or exit. */

         (*p_fmap)[index].state_qty = fam->file_st_qty;
         index++;

         if ( offset == 0 || rval != OK )
         {
            rval = state_file_close( fam );
            break;
         }
      }

      fam->st_file_count = index;
   }

   /*
    * Always return OK because there don't have to be any states,
    * although this will mask an inability to read states that do exist.
    */
   return rval;
}


/*****************************************************************
 * TAG( get_start_index ) LOCAL
 *
 * Determine beginning file index by searching directory.
 */
static Return_value
get_start_index( Mili_family *fam )
{
   int qty, rootlen;
   int i;
   int index, min_index;
   Bool_type init;
   StringArray sarr;
   Return_value rval;

   SANEW( sarr );
   if (sarr == NULL)
   {
      return ALLOC_FAILED;
   }

   rval = mili_scandir( fam->path, fam->file_root, &sarr );
   if (rval != OK)
   {
      return rval;
   }
   qty = SAQTY( sarr );

   if ( qty <= 0 )
   {
      min_index = 0;
   }
   else
   {
      /*
       * Even though the entries are now sorted alphanumerically, it's
       * possible (however unlikely) that there could be an occurrence
       * with files like "data02" and "data1" where "data02" would appear
       * first in the sorted list, so don't just pick the first entry
       * in the list but search through all of the state data files
       * allowed by select_fam().
       */

      rootlen = (int) strlen( fam->file_root );
      init = TRUE;
      for ( i = 0; i < qty; i++ )
      {
         if ( is_numeric( SASTRING( sarr, i ) + rootlen ) )
         {
            index = atoi( SASTRING( sarr, i ) + rootlen );

            if ( init )
            {
               min_index = index;
               init = FALSE;
            }
            else if ( index < min_index )
            {
               min_index = index;
            }
         }
      }

      if ( init )
      {
         min_index = 0;
      }
   }

   free( sarr );

   fam->st_file_index_offset = min_index;

   return OK;
}


/*****************************************************************
 * TAG( dump_state_rec_data ) PRIVATE
 *
 * Dump nodal entry to stdout.
 */
Return_value
dump_state_rec_data( Mili_family *fam, FILE *p_f, Dir_entry dir_ent,
                     char **dir_strings, Dump_control *p_dc,
                     int head_indent, int body_indent )
{
   LONGLONG offset;
   int status;
   int srec_hdr[QTY_SREC_HEADER_FIELDS];
   int *i_data, *subrec_i_data;
   int *p_i;
   int qty_surface_flags;
   char *c_data, *subrec_c_data, *cbound;
   char *p_snam, *p_cnam, *p_vnam;
   char cbuf[64];
   int i_qty, c_qty;
   int subrec_qty;
   int p_srec_id;
   int superclass;
   int i, hi, bi;
   int svar_qty, blk_qty, vcnt, left, off, indent3;
   int idx, j, rows;
   Mesh_object_class_data *p_mocd;
   Htable_entry *class_entry;
   char *short_subr_hdr =
      "%*t%d) \"%s\"%*tOrg:%s;%*tClass:%s;%*tVars:%d;%*tBlocks:%d\n";
   int outlen;
   Return_value rval = OK;
   int read_ct;

   offset = dir_ent[OFFSET_IDX];
   hi = head_indent + 1;
   bi = body_indent + 1;
   indent3 = bi + bi - hi;

   /* Seek and read the element connectivity data header. */
   status = fseek( p_f, offset, SEEK_SET );
   if ( status != 0 )
   {
      return SEEK_FAILED;
   }

   read_ct = fam->read_funcs[M_INT]( p_f, srec_hdr, QTY_SREC_HEADER_FIELDS );
   if (read_ct != QTY_SREC_HEADER_FIELDS)
   {
      return SHORT_READ;
   }

   subrec_qty = srec_hdr[SREC_QTY_SUBRECS_IDX];
   p_srec_id = fam->cur_srec_id;

   /* Dump listing header for state record data. */
   rval = eprintf( &outlen, "%*tSTATE REC:%*tQty subrecords: %d;%*t@%ld;\n",
                   hi, hi + 13, subrec_qty, hi + 44, offset );
   if (rval != OK)
   {
      return rval;
   }

   /* Return if only header dump asked for. */
   if ( p_dc->brevity == DV_HEADERS )
   {
      return OK;
   }

   i_qty = dir_ent[MODIFIER1_IDX] - QTY_SREC_HEADER_FIELDS;
   c_qty = dir_ent[MODIFIER2_IDX];

   /* Allocate buffers for integer and character data. */
   i_data = NEW_N( int, i_qty, "Srec dump int data" );
   if (i_qty > 0 && i_data == NULL)
   {
      return ALLOC_FAILED;
   }
   c_data = NEW_N( char, c_qty, "Srec dump char data" );
   if (c_qty > 0 && c_data == NULL)
   {
      return ALLOC_FAILED;
   }
   cbound = c_data + c_qty;

   /* Read subrecord format descriptions. */
   read_ct = fam->read_funcs[M_INT]( p_f, i_data, i_qty );
   if (read_ct != i_qty)
   {
      free(i_data);
      free(c_data);
      return SHORT_READ;
   }
   read_ct = fam->read_funcs[M_STRING]( p_f, c_data, c_qty );
   if (read_ct != c_qty)
   {
      free(i_data);
      free(c_data);
      return SHORT_READ;
   }

   subrec_i_data = i_data;
   subrec_c_data = c_data;

   /* Define each subrecord. */
   for ( i = 0; i < subrec_qty; i++ )
   {
      svar_qty = subrec_i_data[1];
      blk_qty = subrec_i_data[2];

      p_snam = subrec_c_data;
      ADVANCE_STRING( subrec_c_data, cbound );
      p_cnam = subrec_c_data;
      ADVANCE_STRING( subrec_c_data, cbound );

      /* Output subrecord header. */
      if ( p_dc->brevity == DV_SHORT )
      {
         rval = eprintf( &outlen, short_subr_hdr,
                         bi, i + 1, p_snam,
                         bi + 20, org_names[subrec_i_data[0]],
                         bi + 32, p_cnam,
                         bi + 54, svar_qty,
                         bi + 63, blk_qty );
         if (rval != OK)
         {
            free(i_data);
            free(c_data);
            return rval;
         }
         subrec_i_data += 3 + subrec_i_data[2] * 2;
         /* Need to go through all of this to make backward compatible */
         /**************************************************************/

         /* Get the object class. */
         rval = htable_search( fam->srec_meshes[p_srec_id]->mesh_data.umesh_data,
                               p_cnam, FIND_ENTRY, &class_entry );
         if (class_entry == NULL)
         {
            break;
         }

         /* Get the superclass. */
         p_mocd = (Mesh_object_class_data *) class_entry->data;
         superclass = p_mocd->superclass;

         if( superclass == M_SURFACE )
         {
            qty_surface_flags = subrec_i_data[0];
            subrec_i_data += 1 + qty_surface_flags;
         }
         /**************************************************************/
         for ( j = 0; j < svar_qty; j++ )
         {
            ADVANCE_STRING( subrec_c_data, cbound );
         }
         continue;
      }
      else
      {
         rval = eprintf( &outlen, "%*t%d) \"%s\"%*tOrg: %s;%*tClass: %s;\n",
                         bi, i + 1, p_snam,
                         bi + 30, org_names[subrec_i_data[0]],
                         bi + 45, p_cnam );
         if (rval != OK)
         {
            free(i_data);
            free(c_data);
            return rval;
         }
      }

      /* Output state variable names. */
      rval = esprintf( cbuf, "%*tState vars (%d): %s ", indent3, svar_qty,
                       subrec_c_data );
      if (rval != OK)
      {
         free(i_data);
         free(c_data);
         return rval;
      }
      printf( "%s",cbuf );
      ADVANCE_STRING( subrec_c_data, cbound );
      left = 75 - (int) strlen( cbuf );
      vcnt = 1;
      off = 1;
      while ( vcnt < svar_qty )
      {
         for ( ; left > 0 && vcnt < svar_qty; left -= strlen( p_vnam ) + 1 )
         {
            p_vnam = subrec_c_data;
            rval = eprintf( &outlen, "%*t%s ", off, p_vnam );
            if (rval != OK)
            {
               free(i_data);
               free(c_data);
               return rval;
            }
            ADVANCE_STRING( subrec_c_data, cbound );
            vcnt++;
            off = 1;
         }
         putchar( (int) '\n' );
         left = 75 - indent3;
         off = indent3;
      }
      if ( svar_qty == 1 )
      {
         putchar( (int) '\n' );
      }

      /* Output ident blocks. */
      if ( strcmp( p_cnam, "Global" ) != 0 )
      {
         rval = eprintf( &outlen,
                         "%*tIdent blocks in column-major order:\n",
                         indent3 );
         if (rval != OK)
         {
            free(i_data);
            free(c_data);
            return rval;
         }
      }
      p_i = subrec_i_data + 3;
      rows = blk_qty / 3;
      if ( blk_qty % 3 > 0 )
      {
         rows++;
      }
      for ( j = 0; j < rows; j++ )
      {
         idx = j * 2;

         rval = eprintf( &outlen, "%*t%8d - %-8d",
                         indent3, p_i[idx], p_i[idx + 1] );
         if (rval != OK)
         {
            free(i_data);
            free(c_data);
            return rval;
         }
         idx += rows * 2;
         if ( idx < blk_qty * 2 )
         {
            rval = eprintf( &outlen, "%*t%8d - %-8d",
                            3, p_i[idx], p_i[idx + 1] );
            if (rval != OK)
            {
               free(i_data);
               free(c_data);
               return rval;
            }
            idx += rows * 2;

            if ( idx < blk_qty * 2 )
            {
               rval = eprintf( &outlen, "%*t%8d - %-8d",
                               3, p_i[idx], p_i[idx + 1] );
               if (rval != OK)
               {
                  free(i_data);
                  free(c_data);
                  return rval;
               }
               idx += rows * 2;
            }
         }

         putchar( (int) '\n' );
      }

      subrec_i_data += 3 + subrec_i_data[2] * 2;
      /* Need to go through all of this to make backward compatible */
      /**************************************************************/

      /* Get the object class. */
      rval = htable_search( fam->srec_meshes[p_srec_id]->mesh_data.umesh_data,
                            p_cnam, FIND_ENTRY, &class_entry );
      if (class_entry == NULL)
      {
         break;
      }

      /* Get the superclass. */
      p_mocd = (Mesh_object_class_data *) class_entry->data;
      superclass = p_mocd->superclass;

      if( superclass == M_SURFACE )
      {
         qty_surface_flags = subrec_i_data[0];
         subrec_i_data += 1 + qty_surface_flags;
      }
      /**************************************************************/

   }

   free( i_data );
   free( c_data );

   return rval;
}

/*****************************************************************
 * TAG( mc_get_subrec_cnt ) PUBLIC
 *
 * Returns the number of sub-records that contains the name svar.
 *
 */
Return_value
mc_get_subrec_cnt( Famid fam_id, int staterec_id, char *svar_name, int *cnt )
{
    Subrecord p_subrec;
    int subrec_qty=0;
    int i=0, 
        j=0, 
        k=0, 
        status=0,
        temp_count=0;
    
    i = staterec_id;

    /* Get subrecord count for this state record. */
    status = mc_query_family( fam_id, QTY_SUBRECS, (void *) &i, NULL, 
			      &subrec_qty );

    if(status != OK)
    {
       return status;
    }
    for ( j=0;j< subrec_qty; j++ ) 
    {
      /* Get sub-record binding */
      status = mc_get_subrec_def( fam_id, i, j, &p_subrec );
      for (k=0;k<p_subrec.qty_svars;k++)
      {
         if (!strcmp(p_subrec.svar_names[k], svar_name)) {
		      temp_count++;
		      break;
	      }
      }
    }
    *cnt = temp_count;
    return OK;
}

/*****************************************************************
 * TAG( mc_get_subrec_ids ) PUBLIC
 *
 * Returns the number of sub-records that contains the name svar.
 *
 */
Return_value
mc_get_subrec_ids( Famid fam_id, int staterec_id, char *svar_name, 
		   int *cnt, int **sids )
{
    Subrecord subrec;
    int subrec_qty=0;
    int i=0, j=0, k=0, id_cnt=0, status=0;
    int *p_sids=NULL;

    i = staterec_id;

    /* Get subrecord count for this state record. */
    status = mc_query_family( fam_id, QTY_SUBRECS, (void *) &i, NULL, 
			      &subrec_qty );
    if(status != OK)
    {
       return status;
    }
    p_sids = (int *) malloc( sizeof(int)*subrec_qty);

    /* Load the Subrecord definitions into an array of Subrecords */
    *cnt=0; 
    for ( j=0; 
	  j< subrec_qty; 
	  j++ ) {
      
          /* Get sub-record binding */
          status = mc_get_subrec_def( fam_id, i, j, &subrec );
          for (k=0; 
	       k<subrec.qty_svars;
	       k++)
	      if (!strcmp(subrec.svar_names[k], svar_name)) {
		  (*cnt)++;
		   p_sids[id_cnt++] = j;
		   break;
	      }
	  
    }
    *sids = p_sids;
    return OK;
}

/*****************************************************************
 * TAG( mc_get_subrec_fields) PUBLIC
 *
 * Returns all scalar sub-record fields..
 *
 */
Return_value
mc_get_subrec_fields( Famid fam_id, int staterec_id, int sid, 
		      char *name, char *mclass, 
		      int *qty_svars, int *organization,
		      int *qty_objects, int *qty_blocks )
{
    Subrecord subrec     ;
    Return_value status=OK;

    status = mc_get_subrec_def( fam_id,  staterec_id, sid, &subrec );
    if ( status==OK ) {
         name       = subrec.name;
         mclass      = subrec.class_name;
         *qty_svars = subrec.qty_svars;
         *qty_svars = subrec.qty_svars;
         *organization = subrec.organization;
         *qty_objects  = subrec.qty_objects;
         *qty_blocks   = subrec.qty_blocks;
    }
    return status;    
}


/*****************************************************************
 * TAG( mc_get_svars) PUBLIC
 *
 * Returns all scalar sub-record fields..
 *
 */
Return_value
mc_get_svars( Famid fam_id, int staterec_id,
	      int *num_svars, char ***svar_names )
{
    Subrecord subrec;
    char **p_svar_names;
    int subrec_qty=0;

    int svar_qty=0, svar_index=0;
    Bool_type svar_found=FALSE;

    int i=0, j=0, k=0;
    Return_value status=OK;
 
    /* Get subrecord count for this state record. */
    status = mc_query_family( fam_id, QTY_SUBRECS, (void *) &staterec_id, NULL, 
			      &subrec_qty );

    for ( i=0; 
	  i< subrec_qty; 
	  i++ ) {
      
          /* Get sub-record binding */
          status = mc_get_subrec_def( fam_id, staterec_id, i, &subrec );
	  if ( status != OK )
	       return status;

          svar_qty += subrec.qty_svars;
    }

    p_svar_names = (char **) malloc( sizeof(char *)*svar_qty);
   
    for ( i=0; 
	  i< subrec_qty; 
	  i++ ) {
      
          /* Get sub-record binding */
          status = mc_get_subrec_def( fam_id, staterec_id, i, &subrec );
          for (j=0; 
	       j<subrec.qty_svars;
	       j++) {
	       svar_found = FALSE;
               for (k=0; 
	            k<svar_index;
		    k++) {
		    if ( !strcmp(subrec.svar_names[j], p_svar_names[k]) ) {
		         svar_found = TRUE;
			 break;
		    }
		    
	       }
	       if ( !svar_found ) 
		    p_svar_names[svar_index++] = strdup(subrec.svar_names[j]);  
	  }
    }

    *svar_names = (char **) p_svar_names;
    *num_svars = svar_index;
    return status;    
}

/*****************************************************************
 * TAG( mc_def_subrec_from_subrec ) PUBLIC
 *
 * Define a state record sub-record, binding a collection of
 * mesh objects of one type to a set of state variables.
 *
 * Assumes unstructured mesh type (c/o mesh object id checking).
 */
Return_value
mc_def_subrec_from_subrec( Famid fam_id, int srec_id,Subrecord *in_subrec){
   
	
	Mili_family *fam;
   Return_value status;
   Srec *psr;
   int *pib;
   Sub_srec *psubrec;
   Hash_action op;
   Htable_entry *subrec_entry, *svar_entry, *class_entry;
   Svar **svars;
   int i;
   int stype;
   LONGLONG sub_size;

   Mesh_object_class_data *p_mocd;
   Svar **ppsvars;
   int local_srec_id;
   int mesh_id;
   int superclass;
   Return_value rval;
   int svar_atoms;
   int *surface_flag;

   fam = fam_list[fam_id];

   if ( fam->qty_meshes == 0 )
   {
      return NO_MESH;
   }
   if ( srec_id < 0 || srec_id >= fam->qty_srecs )
   {
      return INVALID_SREC_INDEX;
   }
   if ( fam->srecs[srec_id]->status != OBJ_OPEN )
   {
      return CANNOT_MODIFY_OBJ;
   }
   if ( in_subrec->qty_svars == 0 )
   {
      return NO_SVARS;
   }

   if ( in_subrec->organization!=OBJECT_ORDERED && 
         in_subrec->organization!=RESULT_ORDERED )
   {
      return UNKNOWN_ORGANIZATION;
   }

   local_srec_id = srec_id;
   rval = mc_query_family( fam_id, SREC_MESH, &local_srec_id, NULL,
                           (void *)&mesh_id );

   if ( OK  !=  rval )
   {
      return rval;
   }

   rval = mc_query_family( fam_id, CLASS_SUPERCLASS, &mesh_id, 
                           in_subrec->class_name,
                           (void *)&superclass );

   if ( OK  !=  rval )
   {
      return rval;
   }

   


   /* Create the subrecord hash table if it hasn't been. */
   if ( fam->subrec_table == NULL )
   {
      fam->subrec_table = htable_create( DEFAULT_HASH_TABLE_SIZE );
      if (fam->subrec_table == NULL)
      {
         return ALLOC_FAILED;
      }
   }

   /* Create an entry in the hash table. */
   if( strncmp( in_subrec->name, "sand", 4 ) == 0 )
   {
      op = ENTER_ALWAYS;
   }
   else
   {
      op = ENTER_UNIQUE;
   }

   status = htable_search( fam->subrec_table, in_subrec->name,
                           op, &subrec_entry );
   if ( subrec_entry == NULL )
   {
      return status;
   }

   /* Got into the hash table, so create the subrec. */
   psubrec = NEW( Sub_srec, "State subrecord" );
   if (psubrec == NULL)
   {
      return ALLOC_FAILED;
   }
   str_dup( &psubrec->name, in_subrec->name );
   if (psubrec->name == NULL)
   {
      return ALLOC_FAILED;
   }
   psubrec->organization = in_subrec->organization;
   psubrec->qty_svars = in_subrec->qty_svars;
   svars = NEW_N( Svar *, in_subrec->qty_svars, "Svar array" );
   if (in_subrec->qty_svars > 0 && svars == NULL)
   {
      free(psubrec);
      return ALLOC_FAILED;
   }
   psubrec->svars = svars;
   subrec_entry->data = (void *) psubrec;

   /* Ensure all the referenced state variables exist. */
   for ( i = 0; i < in_subrec->qty_svars;i++ )
   {
      status = htable_search( fam->svar_table, in_subrec->svar_names[i],
                              FIND_ENTRY, &svar_entry );
      if (svar_entry == NULL)
      {
         /* Variable not found.  Free all resources and return. */
         delete_subrec( (void *) psubrec );
         htable_del_entry( fam->subrec_table, subrec_entry );
         return status;
      }
      else
      {
         /* Save reference to variable in subrec. */
         svars[i] = (Svar *) svar_entry->data;
      }
   }

   if ( in_subrec->organization == OBJECT_ORDERED )
   {
      /* For object-ordered data, base types must all be the same. */
      for ( i = 1; i < in_subrec->qty_svars; i++ )
      {
         /* Compare data types of consecutive svars. */
         if ( *svars[i]->data_type != *svars[0]->data_type )
         {
            /* Bad news; clean-up and return. */
            delete_subrec( (void *) psubrec );
            htable_del_entry( fam->subrec_table, subrec_entry );
            return TOO_MANY_TYPES;
         }
      }
   }

   /* Get the referenced object class. */
   status = htable_search( fam->srec_meshes[srec_id]->mesh_data.umesh_data,
                           in_subrec->class_name, FIND_ENTRY, &class_entry );
   if ( class_entry == NULL )
   {
      delete_subrec( (void *) psubrec );
      htable_del_entry( fam->subrec_table, subrec_entry );
      return status;
   }

   p_mocd = (Mesh_object_class_data *) class_entry->data;
   stype = p_mocd->superclass;
   str_dup( &psubrec->mclass, in_subrec->class_name );
   if (psubrec->mclass == NULL)
   {
      return ALLOC_FAILED;
   }
   if (in_subrec->qty_blocks > 0) {
      pib = NEW_N( int, in_subrec->qty_blocks * 2, "Subrec id blks" );
      if (in_subrec->qty_blocks > 0 && pib == NULL)
      {
         return ALLOC_FAILED;
      }

      /* Copy the ident blocks, counting idents. */
      for ( i=0 ; i<in_subrec->qty_blocks ;i++)
      {
         pib[i*2] = in_subrec->mo_blocks[i*2];
         pib[i*2+1] = in_subrec->mo_blocks[i*2+1];
      }
      psubrec->qty_id_blks = in_subrec->qty_blocks;
      psubrec->mo_id_blks = pib;
      psubrec->mo_qty = in_subrec->qty_objects;
      if ( stype != M_MESH ) {
         status = check_object_ids( p_mocd->blocks, in_subrec->qty_blocks, pib );
         if ( status != OK )
         {
            delete_subrec( (void *) psubrec );
            htable_del_entry( fam->subrec_table, subrec_entry );
            free( pib );
            return status;
         }
      }
   }
   


   /*
    * Processing of surface subrecord data, as required
    */

   local_srec_id = srec_id;
   rval = mc_query_family( fam_id, SREC_MESH, &local_srec_id, NULL,
                           (void *)&mesh_id );
   if ( OK  !=  rval )
   {
      return( rval );
   }

   rval = mc_query_family( fam_id, CLASS_SUPERCLASS, &mesh_id, in_subrec->class_name,
                           (void *) &superclass );
   if ( OK != rval )
   {
      return( rval );
   }

   if ( M_SURFACE == superclass )
   {
      /*
       * NOTE:  "flag" data is copied to "surface_flag" to relinquish
       *        user "control" over "flag's" (possibly) allocated memory.
       */

      ppsvars = psubrec->svars;
      svar_atoms = svar_atom_qty( *ppsvars );

      if ( in_subrec->surface_variable_flag == NULL )
      {
         /*
          * establish surface default mode basis of PER_OBJECT
          */

         surface_flag = NEW_N( int, svar_atoms,
                               "Surface variable display buffer" );
         if (svar_atoms > 0 && surface_flag == NULL)
         {
            return ALLOC_FAILED;
         }

         for ( i = 0; i < svar_atoms; i++ )
         {
            surface_flag[ i ] = PER_OBJECT;
         }
      }
      else
      {
         /*
          * test validity of surface "flag" parameters
          */
         for ( i = 0; i < svar_atoms; i++ )
         {
            if ( in_subrec->surface_variable_flag[ i ] != PER_OBJECT  &&  
                  in_subrec->surface_variable_flag[ i ] != PER_FACET )
            {
               return INVALID_DATA_TYPE;
            }
         }

         surface_flag = NEW_N( int, svar_atoms,
                               "Surface variable display buffer" );
         if (svar_atoms > 0 && surface_flag == NULL)
         {
            return ALLOC_FAILED;
         }
         memcpy( (void *)surface_flag, (const void *)in_subrec->surface_variable_flag,
                 svar_atoms * sizeof( int ) );
      }

      psubrec->surface_variable_flag = surface_flag;
   }
   else
   {
      psubrec->surface_variable_flag = NULL;
   } /* if M_SURFACE */


   /* Calculate positioning metrics. */
   status = set_metrics( fam, in_subrec->organization, psubrec, p_mocd, &sub_size );
   if (status != OK)
   {
      return status;
   }

   /* Init input queue for Object-ordered subrecords. */
   if ( in_subrec->organization == OBJECT_ORDERED )
   {
      psubrec->ibuffer = create_buffer_queue( DFLT_BUF_COUNT, sub_size );
      if (psubrec->ibuffer == NULL)
      {
         return ALLOC_FAILED;
      }
   }

   /* Offset to subrecord is previous record size. */
   psr = fam->srecs[srec_id];
   psubrec->offset = psr->size;

   /* Update the record size. */
   psr->size += sub_size;

   /* Save a reference in order of its creation. */
   psr->subrecs = RENEW_N( Sub_srec *, psr->subrecs, psr->qty_subrecs, 1,
                           "Srec data order incr" );
   if (psr->subrecs == NULL)
   {
      return ALLOC_FAILED;
   }
   psr->subrecs[psr->qty_subrecs] = psubrec;
   psr->qty_subrecs++;

   return OK;
}
