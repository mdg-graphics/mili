/*
 * Routines for defining and organizing state variables.
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
 *  I. R. Corey - December 20, 2007: Increased name allocation to 16000.
 *                Fixed a bug with the reallocation of MemStore.
 *                SCR#: 513
 *
 *  I. R. Corey - March 20, 2009: Added support for WIN32 for use with
 *                with Visit.
 *                See SCR #587.
 ************************************************************************
 */


#include <string.h>
#include "mili_internal.h"
#include "eprtf.h"

#ifdef DEBUG
static int dump_tables;                        /* For debugging. */
static int compress_dump;                      /* For debugging. */
static void dump_svar( void *data );           /* For debugging. */
#endif

static char *agg_names[AGG_TYPE_CNT] =
{
   "Scalar", "Vector", "Array", "Vector array"
};

static Return_value create_svar( Mili_family *fam, char *name, Svar **ppsv );
static Return_value find_delete_svar( Mili_family *fam, char *name );
static Return_value get_svar_info(Famid fam_id, char *class_name,
                                  char *var_name, int *num_blocks, int *size,
                                  int *type, int *srec_id, int *sub_srec_id,
                                  char *agg_svar_name);

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
 * TAG( valid_svar_data ) PRIVATE
 *
 * Validate the data defining a state variable.  Does not
 * discover conflict with any extant state variable definition.
 */
Bool_type
valid_svar_data( Aggregate_type atype, char *name, int svar_type,
                 int vec_length, char **components, int rank, int *dims )
{
   int i;

   /* Make these checks for all variable types. */
   if ( ( atype < SCALAR || atype > VEC_ARRAY ) ||
         ( name == NULL || strlen( name ) == 0 ) ||
         ( svar_type != M_STRING &&
           svar_type != M_FLOAT  &&
           svar_type != M_FLOAT4 &&
           svar_type != M_FLOAT8 &&
           svar_type != M_INT    &&
           svar_type != M_INT4   &&
           svar_type != M_INT8 ) )
   {
      return FALSE;
   }

   /* For array types... */
   if ( atype == ARRAY || atype == VEC_ARRAY )
   {
      /* Array rank bounds. */
      if ( rank < 1 || rank > M_MAX_ARRAY_DIMS )
      {
         return FALSE;
      }

      /* Dimensions array. */
      if ( dims == NULL )
      {
         return FALSE;
      }

      /* Array dimension lower bound. */
      for ( i = 0; i < rank; i++ )
      {
         if ( dims[i] < 1 )
         {
            return FALSE;
         }
      }
   }

   /* For vector types... */
   if ( atype == VECTOR || atype == VEC_ARRAY )
   {
      /* Vector length lower bound. */
      if ( vec_length <= 0 )
      {
         return FALSE;
      }

      /* Components array. */
      if ( components == NULL )
      {
         return FALSE;
      }

      /* Component names. */
      for ( i = 0; i < vec_length; i++ )
      {
         if ( components[i] == NULL || strlen( components[i] ) == 0 )
         {
            return FALSE;
         }
      }
   }

   return TRUE;
}


/*****************************************************************
 * TAG( mc_def_svars ) PUBLIC
 *
 * Define multiple scalar state variables.
 */
Return_value
mc_def_svars( Famid fam_id, int qty, char *names, int name_stride,
              char *titles, int title_stride, int types[] )
{
   Mili_family *fam;
   char *p_name, *p_title;
   int i;
   Svar *psv;
   Return_value status;
   char *warning = "Mili warning - duplicate state variable ignored";
   int len;

   fam = fam_list[fam_id];
   p_name = names;
   p_title = titles;
   
   for ( i = 0; i < qty; i++, p_name += name_stride, p_title += title_stride )
   {
      if ( valid_svar_data( SCALAR, p_name, types[i], 0, NULL, 0, NULL ) )
      {
         status = create_svar( fam, p_name, &psv );
         if ( status == OK )
         {
            len = ios_str_dup( &psv->name, p_name, fam->svar_c_ios );
            if ( len == 0 )
            {
               return IOS_STR_DUP_FAILED;
            }

            if ( strlen( p_title ) > 0 )
            {
               len = ios_str_dup( &psv->title, p_title, fam->svar_c_ios );
            }
            else
            {
               len = ios_str_dup( &psv->title, p_name, fam->svar_c_ios );
            }
            if ( len == 0 )
            {
               return IOS_STR_DUP_FAILED;
            }

            psv->agg_type = (Aggregate_type *)
                            ios_alloc( (size_t) 1, fam->svar_i_ios );
            if (psv->agg_type == NULL)
            {
               return IOS_ALLOC_FAILED;
            }
            *psv->agg_type = SCALAR;

            psv->data_type = (int *) ios_alloc( (size_t) 1,
                                                fam->svar_i_ios );
            if ( psv->data_type == NULL )
            {
               return IOS_ALLOC_FAILED;
            }
            *psv->data_type = fam->external_type[types[i]];
         }
         else
         {
            /* Force a diagnostic for duplicates, but don't break. */
            if ( mili_verbose )
            {
               fprintf( stderr, "%s: %s\n", warning, p_name );
            }
         }
      }
      else
      {
         /* Delete previous (successful) entries.  Is this right? */
         for ( p_name -= name_stride; p_name >= names;
               p_name -= name_stride )
         {
            find_delete_svar( fam, p_name );
         }
         return INVALID_SVAR_DATA;
      }
   }

#ifdef DEBUG
   /* For debugging. */
   if ( dump_tables )
   {
      htable_dump( fam->svar_table, compress_dump, dump_svar );
   }
#endif

   return OK;
}


/*****************************************************************
 * TAG( mc_def_vec_svar ) PUBLIC
 *
 * Define a vector state variable.
 */
Return_value
mc_def_vec_svar( Famid fam_id, int type, int size, char *name, char *title,
                 char **field_names, char **field_titles )
{
   Mili_family *fam;
   Svar *psv, *psv2;
   char *sv_name, *sv_title, *dumchar;
   Htable_entry *var_entry;
   int i;
   int len;
   Return_value status;

   fam = fam_list[fam_id];

   if ( !valid_svar_data( VECTOR, name, type, size, field_names, 0, NULL ) )
   {
      return INVALID_SVAR_DATA;
   }

   status = create_svar( fam, name, &psv );
   if ( status == OK )
   {
      len = ios_str_dup( &psv->name, name, fam->svar_c_ios );
      if ( len == 0 )
      {
         return IOS_STR_DUP_FAILED;
      }

      if ( strlen( title ) > 0 )
      {
         len = ios_str_dup( &psv->title, title, fam->svar_c_ios );
      }
      else
      {
         len = ios_str_dup( &psv->title, name, fam->svar_c_ios );
      }
      if ( len == 0 )
      {
         return IOS_STR_DUP_FAILED;
      }

      psv->agg_type = (Aggregate_type *) ios_alloc( (size_t) 1,
                      fam->svar_i_ios );
      if (psv->agg_type == NULL)
      {
         return IOS_ALLOC_FAILED;
      }
      *psv->agg_type = VECTOR;
      psv->data_type = (int *) ios_alloc( (size_t) 1, fam->svar_i_ios );
      if ( psv->data_type == NULL )
      {
         return IOS_ALLOC_FAILED;
      }
      *psv->data_type = fam->external_type[type];

      psv->list_size = (int *) ios_alloc( (size_t) 1, fam->svar_i_ios );
      if ( psv->list_size == NULL )
      {
         return IOS_ALLOC_FAILED;
      }
      *psv->list_size = size;

      /*
       * Save component names.
       * Names are concatenated in memory, so only keep address of first.
       */
      len = ios_str_dup( &psv->list_names, field_names[0], fam->svar_c_ios );
      if ( len == 0 )
      {
         return IOS_STR_DUP_FAILED;
      }
      for ( i = 1; i < size; i++ )
      {
         len = ios_str_dup( &dumchar, field_names[i], fam->svar_c_ios );
         if ( len == 0 )
         {
            return IOS_STR_DUP_FAILED;
         }
      }

      psv->svars = NEW_N( Svar *, size, "Svar array" );
      if (size > 0 && psv->svars == NULL)
      {
         return ALLOC_FAILED;
      }

      /*
       * Enter all fields as scalar state variables also, or use existing
       * ones if found.
       */
      for ( i = 0; i < size; i++ )
      {
         /* Could modify "create_svar()" to do merged entries... */

         sv_name = field_names[i];
         sv_title = field_titles[i];

         status = htable_search( fam->svar_table, sv_name,
                                 ENTER_MERGE, &var_entry );
         if (var_entry == NULL)
         {
            break;
         }
         
         if ( status != ENTRY_EXISTS )
         {
            psv2 = NEW( Svar, "State var" );
            if (psv2 == NULL)
            {
               return ALLOC_FAILED;
            }
            len = ios_str_dup( &psv2->name, sv_name, fam->svar_c_ios );
            if ( len == 0 )
            {
               return IOS_STR_DUP_FAILED;
            }
            if ( strlen( sv_title ) > 0 )
            {
               len = ios_str_dup( &psv2->title, sv_title,
                                  fam->svar_c_ios );
            }
            else
            {
               len = ios_str_dup( &psv2->title, sv_name,
                                  fam->svar_c_ios );
            }
            if ( len == 0 )
            {
               return IOS_STR_DUP_FAILED;
            }

            psv2->agg_type = (Aggregate_type *)
                             ios_alloc( (size_t) 1, fam->svar_i_ios );
            if ( psv2->agg_type == NULL )
            {
               return IOS_ALLOC_FAILED;
            }
            *psv2->agg_type = SCALAR;

            psv2->data_type = (int *) ios_alloc( (size_t) 1,
                                                 fam->svar_i_ios );
            if ( psv2->data_type == NULL )
            {
               return IOS_ALLOC_FAILED;
            }
            *psv2->data_type = fam->external_type[type];

            var_entry->data = (void *) psv2;
         }
         else
         {
            psv2 = (Svar *) var_entry->data;
            if ( *psv2->data_type != type || *psv2->agg_type != SCALAR )
            {
               /*
                * Found state var incompatible with list.  Err off.
                * Ugly!  Might leave new svar entries in table,
                * but they can't be deleted because it was ENTER_MERGE.
                * (Solution - maintain reference count in table entry.)
                */
               find_delete_svar( fam, name );
               status = MISMATCHED_LIST_FIELD;
               break;
            }
         }

         /* Save reference to the list field in the list svar struct. */
         psv->svars[i] = psv2;
      }
      /* Application only gets OK or failure. */
      if ( status == ENTRY_EXISTS )
      {
         status = OK;
      }
   }

   return status;
}


/*****************************************************************
 * TAG( mc_def_arr_svar ) PUBLIC
 *
 * Define an array state variable.
 */
Return_value
mc_def_arr_svar( Famid fam_id, int rank, int dims[], char *name,
                 char *title, int type )
{
   Mili_family *fam;
   Return_value status;
   Svar *psv;
   int len;

   fam = fam_list[fam_id];

   if ( !valid_svar_data( ARRAY, name, type, 0, NULL, rank, dims ) )
   {
      return INVALID_SVAR_DATA;
   }

   status = create_svar( fam, name, &psv );
   if ( status == OK )
   {
      len = ios_str_dup( &psv->name, name, fam->svar_c_ios );
      if ( len == 0 )
      {
         return IOS_STR_DUP_FAILED;
      }

      if ( strlen( title ) > 0 )
      {
         len = ios_str_dup( &psv->title, title, fam->svar_c_ios );
      }
      else
      {
         len = ios_str_dup( &psv->title, name, fam->svar_c_ios );
      }
      if ( len == 0 )
      {
         return IOS_STR_DUP_FAILED;
      }

      psv->agg_type = (Aggregate_type *)
                      ios_alloc( (size_t) 1, fam->svar_i_ios );
      if ( psv->agg_type == NULL )
      {
         return IOS_ALLOC_FAILED;
      }
      *psv->agg_type = ARRAY;

      psv->data_type = (int *) ios_alloc( (size_t) 1, fam->svar_i_ios );
      if ( psv->data_type == NULL )
      {
         return IOS_ALLOC_FAILED;
      }
      *psv->data_type = fam->external_type[type];

      psv->order = (int *) ios_alloc( (size_t) 1, fam->svar_i_ios );
      if ( psv->order == NULL )
      {
         return IOS_ALLOC_FAILED;
      }
      *psv->order = rank;

      psv->dims = (int *) ios_alloc( (size_t) rank, fam->svar_i_ios );
      if ( psv->dims == NULL )
      {
         return IOS_ALLOC_FAILED;
      }
      memcpy( (char *) psv->dims, (char *) dims, rank * sizeof( int ) );
   }

#ifdef DEBUG
   /* For debugging. */
   if ( dump_tables )
   {
      htable_dump( fam->svar_table, compress_dump, dump_svar );
   }
#endif

   return status;
}


/*****************************************************************
 * TAG( mc_def_vec_arr_svar ) PUBLIC
 *
 * Define a vector array state variable.
 */
Return_value
mc_def_vec_arr_svar( Famid fam_id, int rank, int dims[], char *name,
                     char *title, int size, char **field_names,
                     char **field_titles, int type )
{
   Mili_family *fam;
   Svar *psv, *psv2;
   char *sv_name, *sv_title, *dumchar;
   Htable_entry *var_entry;
   int i;
   int len;
   int *dumint;  //adding this as the idiots who devoped this originally were ignorant
   Return_value status;

   fam = fam_list[fam_id];

   if ( !valid_svar_data( VEC_ARRAY, name, type, size,
                          field_names, rank, dims ) )
   {
      return INVALID_SVAR_DATA;
   }

   status = create_svar( fam, name, &psv );
   if ( status == OK )
   {
      len = ios_str_dup( &psv->name, name, fam->svar_c_ios );
      if ( len == 0 )
      {
         return IOS_STR_DUP_FAILED;
      }

      if ( strlen( title ) > 0 )
      {
         len = ios_str_dup( &psv->title, title, fam->svar_c_ios );
      }
      else
      {
         len = ios_str_dup( &psv->title, name, fam->svar_c_ios );
      }
      if ( len == 0 )
      {
         return IOS_STR_DUP_FAILED;
      }

      psv->agg_type = (Aggregate_type *) ios_alloc( (size_t) 1,
                      fam->svar_i_ios );
      if ( psv->agg_type == NULL )
      {
         return IOS_ALLOC_FAILED;
      }
      *psv->agg_type = VEC_ARRAY;

      psv->data_type = (int *) ios_alloc( (size_t) 1, fam->svar_i_ios );
      if ( psv->data_type == NULL )
      {
         return IOS_ALLOC_FAILED;
      }
      *psv->data_type = fam->external_type[type];

      psv->order = (int *) ios_alloc( (size_t) 1, fam->svar_i_ios );
      if ( psv->order == NULL )
      {
         return IOS_ALLOC_FAILED;
      }
      *psv->order = rank;

      psv->dims = (int *) ios_alloc( (size_t) rank, fam->svar_i_ios );
      if ( psv->dims == NULL )
      {
         return IOS_ALLOC_FAILED;
      }
      memcpy( (char *) psv->dims, (char *) dims, rank * sizeof( int ) );

      psv->list_size = (int *) ios_alloc( (size_t) 1, fam->svar_i_ios );
      if ( psv->list_size == NULL )
      {
         return IOS_ALLOC_FAILED;
      }
      *psv->list_size = size;

      /*
       * Save component names.
       * Names are concatenated in memory, so only keep address of first.
       *
       * Note (1/9/03, DES) - the above is not necessarily true, given that
       * any string allocation in the IO Store could cause a new buffer
       * allocation if not enough room remains in the current Store buffer.
       * However, there do not appear to be any accesses of this data
       * except by the IO Store functions, so whether or not the strings
       * are contiguous in memory is hidden from other code.
       */
      len = ios_str_dup( &psv->list_names, field_names[0], fam->svar_c_ios );
      if ( len == 0 )
      {
         return IOS_STR_DUP_FAILED;
      }
      for ( i = 1; i < size; i++ )
      {
         len = ios_str_dup( &dumchar, field_names[i], fam->svar_c_ios );
         if ( len == 0 )
         {
            return IOS_STR_DUP_FAILED;
         }
      }

      psv->svars = NEW_N( Svar *, size, "Svar array" );
      if (size > 0 && psv->svars == NULL)
      {
         return ALLOC_FAILED;
      }

      /*
       * Enter all fields as scalar state variables also, or use existing
       * ones if found.
       */
      for ( i = 0; i < size; i++ )
      {
         /* Could modify "create_svar()" to do merged entries... */

         sv_name = field_names[i];
         sv_title = field_titles[i];

         status = htable_search( fam->svar_table, sv_name,
                                 ENTER_MERGE, &var_entry );
         if (var_entry == NULL)
         {
            break;
         }
         if ( status != ENTRY_EXISTS )
         {
            psv2 = NEW( Svar, "State var" );
            if (psv2 == NULL)
            {
               return ALLOC_FAILED;
            }
            len = ios_str_dup( &psv2->name, sv_name, fam->svar_c_ios );
            if ( len == 0 )
            {
               return IOS_STR_DUP_FAILED;
            }

            if ( strlen( sv_title ) > 0 )
            {
               len = ios_str_dup( &psv2->title, sv_title,
                                  fam->svar_c_ios );
            }
            else
            {
               len = ios_str_dup( &psv2->title, sv_name,
                                  fam->svar_c_ios );
            }
            if ( len == 0 )
            {
               return IOS_STR_DUP_FAILED;
            }

            psv2->agg_type = (Aggregate_type *)
                             ios_alloc( (size_t) 1, fam->svar_i_ios );
            if ( psv2->agg_type == NULL )
            {
               return IOS_ALLOC_FAILED;
            }
            *psv2->agg_type = SCALAR;

            psv2->data_type = (int *) ios_alloc( (size_t) 1,
                                                 fam->svar_i_ios );
            if ( psv2->data_type == NULL )
            {
               return IOS_ALLOC_FAILED;
            }
            *psv2->data_type = fam->external_type[type];

            var_entry->data = (void *) psv2;
         }
         else
         {
            psv2 = (Svar *) var_entry->data;
            if ( *psv2->data_type != type )
            {
               /*
                * Found state var incompatible with list.  Err off.
                * Ugly!  Might leave new svar entries in table,
                * but they can't be deleted because it was ENTER_MERGE.
                * (Solution - maintain reference count in table entry.)
                */
               find_delete_svar( fam, name );
               status = MISMATCHED_LIST_FIELD;
               break;
            }            
         }

         /* Save reference to the list field in the list svar struct. */
         psv->svars[i] = psv2;
      }
      /* Application only gets OK or failure. */
      if ( status == ENTRY_EXISTS )
      {
         status = OK;
      }
   }

   return status;
}


/*****************************************************************
 * TAG( create_svar ) LOCAL
 *
 * Create a new state variable.
 */
static Return_value
create_svar( Mili_family *fam, char *name, Svar **ppsv )
{
   Hash_action op;
   Htable_entry *phte;
   Return_value status;
   Svar *psv;

   if ( fam->svar_table == NULL )
   {
      /* Create hash table to hold state var def's. */
      fam->svar_table = htable_create( DEFAULT_HASH_TABLE_SIZE );
      if (fam->svar_table == NULL)
      {
         return ALLOC_FAILED;
      }
      /* Create I/O stores to hold character and integer data for def's. */
      fam->svar_c_ios = ios_create( M_STRING );
      if (fam->svar_c_ios == NULL)
      {
         return IOS_ALLOC_FAILED;
      }
      fam->svar_i_ios = ios_create( M_INT );
      if (fam->svar_i_ios == NULL)
      {
         return IOS_ALLOC_FAILED;
      }
      /*
       * Initially, and after any svar output, allocate a header from
       * the integer output buffer so the header will be first in a
       * subsequent output operation.
       */
      fam->svar_hdr = (int *) ios_alloc( (size_t) QTY_SVAR_HEADER_FIELDS,
                                         fam->svar_i_ios );
      if (fam->svar_hdr == NULL)
      {
         return IOS_ALLOC_FAILED;
      }
   }

   if ( strncmp( name, "sand", 4 ) == 0 )
   {
      op = ENTER_ALWAYS;
   }else
   {
      op = ENTER_UNIQUE;
   }
   
   status = htable_search( fam->svar_table, name, op, &phte );

   if ( phte != NULL )
   {
      psv = NEW( Svar, "State var" );
      if (psv == NULL)
      {
         status = ALLOC_FAILED;
      }      
      phte->data = (void *) psv;
      *ppsv = psv;
   }
   else
   {
      *ppsv = NULL;
   }

   return status;
}


/*****************************************************************
 * TAG( mc_get_svar_def ) PUBLIC
 *
 * Copy state variable definition into application struct.
 */
Return_value
mc_get_svar_def( Famid fam_id, char *svar_name, State_variable *p_stvar )
{
   Mili_family *fam;
   Htable_entry *p_hte;
   Return_value rval;
   Svar *p_sv;
   int i;

   fam = fam_list[fam_id];

   rval = htable_search( fam->svar_table, svar_name, FIND_ENTRY, &p_hte );
   if ( p_hte == NULL )
   {
      return rval;
   }
   else
   {
      p_sv = (Svar *) p_hte->data;
   }

   /* Copy handles. */
   str_dup( &p_stvar->short_name, p_sv->name );
   str_dup( &p_stvar->long_name, p_sv->title );
   if (p_stvar->short_name == NULL || p_stvar->long_name == NULL)
   {
      return ALLOC_FAILED;
   }

   /* Numeric type and aggregate type. */
   p_stvar->num_type = *p_sv->data_type;
   p_stvar->agg_type = *p_sv->agg_type;

   /* For arrays and vector arrays... */

   p_stvar->rank     = 1;
   p_stvar->vec_size = 1;
   if ( p_stvar->agg_type != SCALAR )
   {
      if ( p_stvar->agg_type != VECTOR )
      {
         /* Copy rank. */
         p_stvar->rank = *p_sv->order;

         /* Copy dimension sizes. */
         p_stvar->dims = NEW_N( int, p_stvar->rank,
                                "State_variable arr dims" );
         if (p_stvar->rank > 0 && p_stvar->dims == NULL)
         {
            return ALLOC_FAILED;
         }
         for ( i = 0; i < p_stvar->rank; i++ )
         {
            p_stvar->dims[i] = p_sv->dims[i];
         }
      }

      /* For vector arrays... */
      if ( p_stvar->agg_type == VEC_ARRAY || p_stvar->agg_type == VECTOR )
      {
         /* Copy vector size. */
         p_stvar->vec_size = *p_sv->list_size;

         /* Copy component names. */
         p_stvar->components = NEW_N( char *, p_stvar->vec_size,
                                      "Vector array component names" );
         if (p_stvar->vec_size > 0 && p_stvar->components == NULL)
         {
            return ALLOC_FAILED;
         }

         p_stvar->component_titles = NEW_N( char *, p_stvar->vec_size,
                                            "Vector array component names" );
         if (p_stvar->vec_size > 0 && p_stvar->component_titles == NULL)
         {
            return ALLOC_FAILED;
         }
         for ( i = 0; i < p_stvar->vec_size; i++ )
         {
            str_dup( p_stvar->components + i, p_sv->svars[i]->name );
            str_dup( p_stvar->component_titles + i, p_sv->svars[i]->title );
            if (p_stvar->components[i] == NULL ||
                  p_stvar->component_titles[i] == NULL)
            {
               return ALLOC_FAILED;
            }
         }
      }
   }

   return OK;
}


/*****************************************************************
 * TAG( find_delete_svar ) LOCAL
 *
 * Delete a state variable.
 */
static Return_value
find_delete_svar( Mili_family *fam, char *name )
{
   Htable_entry *phte;
   Return_value rval;

   rval = htable_search( fam->svar_table, name, FIND_ENTRY, &phte );
   if ( phte != NULL )
   {
      rval = delete_svar_with_ios( (Svar *) phte->data, fam->svar_c_ios,
                                   fam->svar_i_ios );
      htable_del_entry( fam->svar_table, phte );
   }

   return rval;
}


/*****************************************************************
 * TAG( commit_svars ) PRIVATE
 *
 * Commit in-memory state variable defs to disk.
 *
 * Assumes file is opened and positioned properly.
 */
Return_value
commit_svars( Mili_family *fam )
{
   size_t int_qty, 
          char_qty, 
          round_qty, 
          outbytes;
   int num_written;
   Return_value rval;

   if ( fam->svar_table == NULL )
   {
      return OK;
   }

   rval = prep_for_new_data( fam, NON_STATE_DATA );
   if ( rval != OK )
   {
      return rval;
   }

   int_qty = ios_get_fresh( fam->svar_i_ios );

   /*
    * Only output if something in addition to header words has
    * been allocated from buffer.
    */
   if ( (int_qty - QTY_SVAR_HEADER_FIELDS) > 0 )
   {
      /* Get amount of character data for output. Round up if necessary. */
      char_qty = ios_get_fresh( fam->svar_c_ios );
      round_qty = ROUND_UP_INT( char_qty, 4 );
      
      if ( round_qty != char_qty )
      {
         if (ios_alloc( round_qty - char_qty, fam->svar_c_ios ) == NULL)
         {
            return IOS_ALLOC_FAILED;
         }
         char_qty = round_qty;
      }

      /* Fill the header. */
      fam->svar_hdr[SVAR_QTY_INT_WORDS_IDX] = (int) int_qty;
      fam->svar_hdr[SVAR_QTY_BYTES_IDX] = (int) char_qty;

      /* Add directory entry. */
      outbytes = char_qty + int_qty * EXT_SIZE( fam, M_INT );
      rval = add_dir_entry( fam, STATE_VAR_DICT, DONT_CARE, DONT_CARE, 0,
                            NULL, fam->next_free_byte, outbytes );
      if ( rval != OK )
      {
         return rval;
      }

      /* Write out the data. */
      rval = ios_output( fam, fam->cur_file, fam->svar_i_ios, &num_written );
      if (rval != OK)
      {
         return rval;
      }
      rval = ios_output( fam, fam->cur_file, fam->svar_c_ios, &num_written );
      if (rval != OK)
      {
         return rval;
      }

      /* Update for subsequent file operations. */
      fam->next_free_byte += outbytes;
      fam->svar_hdr = ios_alloc( (size_t) QTY_SVAR_HEADER_FIELDS,
                                 fam->svar_i_ios );
      if (fam->svar_hdr == NULL)
      {
         return IOS_ALLOC_FAILED;
      }
   }

   return OK;
}


/*****************************************************************
 * TAG( delete_svar ) PRIVATE
 *
 * Function to delete Svar struct; for use by htable_delete().
 * This function does not call ios_unalloc, but assumes another
 * function will (more efficiently) release that memory directly
 * from the I/O store.
 */
void
delete_svar( void *data )
{
   Svar *p_svar;

   p_svar = (Svar *) data;

   if ( p_svar->svars != NULL )
   {
      free( p_svar->svars );
   }

   free( p_svar );
}


/*****************************************************************
 * TAG( delete_svar_with_ios ) PRIVATE
 *
 * Delete Svar struct with calls to ios_unalloc().
 *
 * Undo "batch" unalloc's.
 */
Return_value
delete_svar_with_ios( Svar *psv, IO_mem_store *pcioms, IO_mem_store *piioms )
{
   char *p_name;
   int name_len, rank;
   int i;
   Return_value rval;

   /* Delete name and title. */
   rval = ios_unalloc( psv->name, (int) strlen( psv->name ) + 1, pcioms );
   if (rval != OK)
   {
      return rval;
   }
   rval = ios_unalloc( psv->title, (int) strlen( psv->title ) + 1, pcioms );
   if (rval != OK)
   {
      return rval;
   }

   /* Delete integers that always exist. */
   rval = ios_unalloc( psv->agg_type, 1, piioms );
   if (rval != OK)
   {
      return rval;
   }
   rval = ios_unalloc( psv->data_type, 1, piioms );
   if (rval != OK)
   {
      return rval;
   }

   if ( *psv->agg_type == VEC_ARRAY || *psv->agg_type == VECTOR )
   {
      /* Arrays have order allocated after data_type. */
      if ( *psv->agg_type == VEC_ARRAY )
      {
         rank = *psv->order;
         rval = ios_unalloc( psv->order, 1, piioms );
         if (rval != OK)
         {
            return rval;
         }
      }

      /* Free each component name individually. */
      p_name = psv->list_names;
      for ( i = 0; i < *psv->list_size; i++ )
      {
         name_len = (int) strlen( p_name ) + 1;
         rval = ios_unalloc( p_name, name_len, pcioms );
         if (rval != OK)
         {
            return rval;
         }
         p_name += name_len;
      }

      /* Free list size descriptor. */
      rval = ios_unalloc( psv->list_size, 1, piioms );
      if (rval != OK)
      {
         return rval;
      }

      /* Free component state variable pointers. */
      free( psv->svars );
   }

   if ( *psv->agg_type == VEC_ARRAY || *psv->agg_type == ARRAY )
   {
      rank = *psv->order;
		/* Arrays have order allocated after data_type. */
      if ( *psv->agg_type == ARRAY )
      {
         rval = ios_unalloc( psv->order, 1, piioms );
         if (rval != OK)
         {
            return rval;
         }
      }

      /* For array types, free dimensions. */
      rval = ios_unalloc( psv->dims, rank, piioms );
      if (rval != OK)
      {
         return rval;
      }
   }

   free( psv );
   return OK;
}

/*****************************************************************
 * TAG( input_svars ) LOCAL
 *
 * Read an STATE_VAR_DICT object from a non-state file and load the
 * described state variables into the family's state variable
 * dictionary.
 */
static Return_value
input_svars( Mili_family *fam, int file_index, LONGLONG file_offset )
{
   IO_mem_store *pioms;
   int *header;
   int *svar_i, *p_i, *bound;
   char *svar_c, *p_c;
   Svar *p_sv, *p_sv2;
   int i, new_qty;
   Htable_entry *phte;
   Return_value status;

   /* Set storage to receive file data. */
   if ( fam->svar_table == NULL )
   {
      fam->svar_table = htable_create( DEFAULT_HASH_TABLE_SIZE );
      if (fam->svar_table == NULL)
      {
         return ALLOC_FAILED;
      }
      fam->svar_i_ios = ios_create_empty();
      if (fam->svar_i_ios == NULL)
      {
         return IOS_ALLOC_FAILED;
      }
      fam->svar_c_ios = ios_create_empty();
      if (fam->svar_c_ios == NULL)
      {
         return IOS_ALLOC_FAILED;
      }
   }
   pioms = fam->svar_c_ios;

   /* Open and position the file. */
   status = non_state_file_open( fam, file_index, fam->access_mode );
   if (status != OK)
   {
      return status;
   }
   
   status = non_state_file_seek( fam, file_offset );
   
   if (status != OK)
   {
      return status;
   }

   /* Load the data into the IO Stores. */
   status = ios_input( fam, M_INT, (size_t) QTY_SVAR_HEADER_FIELDS,
                       fam->svar_i_ios, (void **) &header );
   if (status != OK)
   {
      return status;
   }
   status = ios_input( fam, M_INT, (size_t) header[SVAR_QTY_INT_WORDS_IDX] - QTY_SVAR_HEADER_FIELDS,
                       fam->svar_i_ios, (void **) &svar_i );
   if (status != OK)
   {
      return status;
   }
   status = ios_input( fam, M_STRING, (size_t) header[SVAR_QTY_BYTES_IDX],
                       fam->svar_c_ios, (void **) &svar_c );
   if (status != OK)
   {
      return status;
   }

   /* Parse the svar descriptors and load them into the table. */
   bound = svar_i + header[SVAR_QTY_INT_WORDS_IDX] - QTY_SVAR_HEADER_FIELDS;
   p_i = svar_i;
   if (pioms != NULL)
   {
      status = ios_traverse_init( pioms, 1 );
      if (status != OK)
      {
         return status;
      }
   }
   while( p_i < bound )
   {
      status = ios_string_traverse( pioms, &p_c );
      if (status != OK)
      {
         return status;
      }
      status = create_svar( fam, p_c, &p_sv );
      if (p_sv == NULL)
      {
         return status;
      }

      /* All types have these. */
      p_sv->name = p_c;
      status = ios_string_traverse( pioms, &p_sv->title );
      if (status != OK)
      {
         return status;
      }
      p_sv->agg_type = (Aggregate_type *) p_i++;
      p_sv->data_type = p_i++;

      /* Get agg_type-specific descriptors. */
      switch ( *p_sv->agg_type )
      {
         case ARRAY:
            p_sv->order = p_i++;
            p_sv->dims = p_i;
            p_i += *p_sv->order;
            break;

         case VECTOR:
         case VEC_ARRAY:
            if ( *p_sv->agg_type == VEC_ARRAY )
            {
               p_sv->order = p_i++;
               p_sv->dims = p_i;
               p_i += *p_sv->order;
            }

            p_sv->list_size = p_i++;

            /*
             * Find any components that have already been entered as
             * scalar state variables.  The component definition data
             * for un-entered components will immediately follow the
             * vector component list.
             */
            status = ios_string_traverse( pioms, &p_sv->list_names );
            if (status != OK)
            {
               return status;
            }
            p_c = p_sv->list_names;

            /* Vectors/vec arrays require scalars for each component. */

            p_sv->svars = NEW_N( Svar *, *p_sv->list_size, "Vec comp's" );
            if (*p_sv->list_size > 0 && p_sv->svars == NULL)
            {
               return ALLOC_FAILED;
            }

            new_qty = 0;
            for ( i = 0; i < *p_sv->list_size; i++ )
            {
               /* Already have first name. */
               if ( i > 0 )
               {
                  status = ios_string_traverse( pioms, &p_c );
                  if (status != OK)
                  {
                     return status;
                  }
               }

               status = htable_search( fam->svar_table, p_c,
                                       ENTER_MERGE, &phte );
               if (phte == NULL)
               {
                  return status;
               }
               if ( phte->data == NULL )
               {
                  /*
                   * Create the Svar to fill p_sv->svars, but initialize
                   * it when traversing the component data after the
                   * vector svar component list.
                   */
                  phte->data = (void *) NEW( Svar, "Input vec svar" );
                  
                  if (phte->data == NULL)
                  {
                     return ALLOC_FAILED;
                  }
                  new_qty++;
               }
               p_sv->svars[i] = (Svar *) phte->data;
            }

            /* Now create component svars that weren't found. */
            for ( i = 0; i < new_qty; i++ )
            {
               status = ios_string_traverse( pioms, &p_c );
               if (status != OK)
               {
                  return status;
               }
               status = htable_search( fam->svar_table, p_c,
                                       FIND_ENTRY, &phte );
               if (phte == NULL)
               {
                  return status;
               }

               p_sv2 = (Svar *) phte->data;
               p_sv2->name = p_c;
               status = ios_string_traverse( pioms, &p_sv2->title );
               if (status != OK)
               {
                  return status;
               }
               p_sv2->agg_type = (Aggregate_type *) p_i++;
               p_sv2->data_type = p_i++;
            }
            break;
         default: 
            break;
      }
   }
   return OK;
}

/*****************************************************************
 * TAG( load_svars ) PRIVATE
 *
 * Load state variable definitions from a Mili database.
 */
Return_value
load_svars( Mili_family *fam )
{
   File_dir *p_fd;
   int i, j;
   LONGLONG *entry;
   Return_value rval;

   /*
    * Search the family directory for STATE_VAR_DICT entries and
    * load the contents of each into the state variable hash table.
    */
   for ( i = 0; i < fam->file_count; i++ )
   {
      p_fd = fam->directory + i;
      for ( j = 0; j < p_fd->qty_entries; j++ )
      {
         entry = p_fd->dir_entries[j];

         if ( entry[TYPE_IDX] == STATE_VAR_DICT )
         {
            /* Svar dictionary data found. */
            rval = input_svars( fam, i, entry[OFFSET_IDX] );
            if (rval != OK)
            {
               return rval;
            }
         }
      }
   }

   /*
    * If this process has write access, allocate space for an svar entry
    * header now that all extant svar entries have been read in.  This
    * is to prepare for any subsequent new svar definitions.
    */
   if ( fam->svar_table != NULL &&
         ( fam->access_mode == 'w' || fam->access_mode == 'a' ) )
   {
      fam->svar_hdr = (int *)ios_alloc( (size_t) QTY_SVAR_HEADER_FIELDS,
                                        fam->svar_i_ios );
      if (fam->svar_hdr == NULL)
      {
         return IOS_ALLOC_FAILED;
      }
   }

   return OK;
}



/*****************************************************************
 * TAG( dump_state_var_dict ) PRIVATE
 *
 * Dump nodal entry to stdout.
 */
Return_value
dump_state_var_dict( Mili_family *fam, FILE *p_f, Dir_entry dir_ent,
                     char **dir_strings, Dump_control *p_dc,
                     int head_indent, int body_indent )
{
   LONGLONG offset;
   int hi, bi, off;
   int qty_ints, qty_bytes;
   int status;
   char *cdata, *p_c, *cbound;
   int *idata, *p_i, *ibound;
   int svar_header[QTY_SVAR_HEADER_FIELDS];
   Aggregate_type agg;
   int dtype, rank, list_size;
   char *name, *title;
   char handle[64];
   char *p_tail;
   int i;
   int outlen;
   Return_value rval;
   size_t read_ct;

   hi = head_indent + 1;
   bi = body_indent + 1;
   offset = dir_ent[OFFSET_IDX];
	off = 0;
	list_size = 0;
#ifdef HAVE_EPRINT
   rval = eprintf( &outlen, "%*tVAR DICT:%*t@%ld;\n", hi, hi + 44, offset );
   if (rval != OK)
   {
      return rval;
   }
#else
#endif

   /* Return if only header dump asked for. */
   if ( p_dc->brevity == DV_HEADERS || p_dc->brevity == DV_SHORT )
   {
      return OK;
   }

   /* Seek to header and read. */
   status = fseek( p_f, (long)offset, SEEK_SET );
   if ( status != 0 )
   {
      return SEEK_FAILED;
   }
   read_ct = fam->read_funcs[M_INT](p_f, svar_header, QTY_SVAR_HEADER_FIELDS);
   if (read_ct != QTY_SVAR_HEADER_FIELDS)
   {
      return SHORT_READ;
   }
   qty_ints = svar_header[SVAR_QTY_INT_WORDS_IDX] - QTY_SVAR_HEADER_FIELDS;
   qty_bytes = svar_header[SVAR_QTY_BYTES_IDX];

   /* Allocate integer and character data buffers. */
   idata = NEW_N( int, qty_ints, "Svar dict ints" );
   if (qty_ints > 0 && idata == NULL)
   {
      return ALLOC_FAILED;
   }
   cdata = NEW_N( char, qty_bytes, "Svar dict chars" );
   if (qty_bytes > 0 && cdata == NULL)
   {
      return ALLOC_FAILED;
   }

   /* Read integer, then character data. */
   read_ct = fam->read_funcs[M_INT]( p_f, idata, qty_ints );
   if (read_ct != qty_ints)
   {
      return SHORT_READ;
   }
   read_ct = fam->read_funcs[M_STRING]( p_f, cdata, qty_bytes );
   if (read_ct != qty_bytes)
   {
      return SHORT_READ;
   }

   /* While more variables exist, traverse buffers and output. */
   ibound = idata + qty_ints;
   p_i = idata;
   cbound = cdata + qty_bytes;
   p_c = cdata;
   while ( p_i < ibound )
   {
      agg = (Aggregate_type) *p_i++;
      dtype = *p_i++;
      name = p_c;
      ADVANCE_STRING( p_c, cbound );
      title = p_c;
      ADVANCE_STRING( p_c, cbound );

      if ( agg != SCALAR )
      {
         rank = *p_i++;

         if ( agg != VECTOR )
         {
            /* Build array declaration string. */
            sprintf( handle, "%s", agg_names[agg] );
            p_tail = handle + strlen( agg_names[agg] );
            for ( i = 0; i < rank; i++ )
            {
               sprintf( p_tail, "[%d]", *p_i++ );
               while ( *(++p_tail) );
            }
         }
         else
         {
            sprintf( handle, "%s", agg_names[agg] );
            p_tail = handle + strlen( agg_names[agg] );
            sprintf( p_tail, "[%d]", rank );
            off = bi - hi;
         }

         /* Append vec size for vector arrays. */
         if ( agg == VEC_ARRAY )
         {
            list_size = *p_i++;
            sprintf( p_tail, "x[%d]", list_size );
            off = bi - hi;
         }
      }

      switch( agg )
      {
         case SCALAR:
#ifdef HAVE_EPRINT
            rval = eprintf( &outlen, "%*t%s \"%s\"%*t%s%*t%s\n",
                            bi, name, title, bi + 35, dtype_names[dtype],
                            bi + 45, agg_names[agg] );
            if (rval != OK)
            {
               free(idata);
               free(cdata);
               return rval;
            }
#else
#endif
            break;

         case VECTOR:
#ifdef HAVE_EPRINT
            rval = eprintf( &outlen, "%*t%s \"%s\"%*t%s%*t%s\n",
                            bi, name, title, bi + 35, dtype_names[dtype],
                            bi + 45, handle );
            if (rval != OK)
            {
               free(idata);
               free(cdata);
               return rval;
            }
#else
#endif
            for ( i = 0; i < rank; i++ )
            {
               name = p_c;
               ADVANCE_STRING( p_c, cbound );
               if ( i == 0 )
               {
                  rval = eprintf( &outlen, "%*tComponents: %s",
                                  bi + off, name );
                  if (rval != OK)
                  {
                     free(idata);
                     free(cdata);
                     return rval;
                  }
               }
               else
               {
                  printf( " %s", name );
               }
            }
            putchar( (int) '\n' );
            break;

         case ARRAY:
#ifdef HAVE_EPRINT
            rval = eprintf( &outlen, "%*t%s \"%s\"%*t%s%*t%s\n",
                            bi, name, title, bi + 35, dtype_names[dtype],
                            bi + 45, handle );
            if (rval != OK)
            {
               free(idata);
               free(cdata);
               return rval;
            }
#else
#endif
            break;

         case VEC_ARRAY:
#ifdef HAVE_EPRINT
            rval = eprintf( &outlen, "%*t\"%s\" \"%s\"%*t%s%*t%s\n",
                            bi, name, title, bi + 35, dtype_names[dtype],
                            bi + 45, handle );
            if (rval != OK)
            {
               free(idata);
               free(cdata);
               return rval;
            }
#else
#endif
            for ( i = 0; i < list_size; i++ )
            {
               name = p_c;
               ADVANCE_STRING( p_c, cbound );
               if ( i == 0 )
               {
                  rval = eprintf( &outlen, "%*tComponents: %s",
                                  bi + off, name );
                  if (rval != OK)
                  {
                     free(idata);
                     free(cdata);
                     return rval;
                  }
               }
               else
               {
                  printf( " %s", name );
               }
            }
            putchar( (int) '\n' );
            break;
         default:
            break;
      }
   }

   /* Clean up. */
   free( idata );
   free( cdata );

   return OK;
}

#ifdef DEBUG
/*****************************************************************
 * TAG( dump_svar ) LOCAL
 *
 * For debugging.
 */
static void
dump_svar( void *data )
{
   int i;
   Svar *psv = (Svar *) data;
   char *agg_types[] = {"SCALAR", "VECTOR", "ARRAY", "VEC_ARRAY" };

   printf( "name=\"%s\" title=\"%s\" data_type=%d agg_type=%s, order=%d dims=",
           psv->name, psv->title, *psv->data_type, agg_types[*psv->agg_type],
           *psv->order );

   if ( psv->dims == NULL )
   {
      printf( "NULL" );
   }
   else
   {
      printf( "%d", psv->dims[0] );
      for ( i = 1; i < *psv->order; i++ )
      {
         printf( ",%d", psv->dims[i] );
      }
   }
}
#endif


/*****************************************************************
 * TAG( Mset_hash_dump ) PRIVATE
 *
 * For debugging.
 */
void
Mset_hash_dump_( int *compress )
{
#ifdef DEBUG
   dump_tables = TRUE;
   compress_dump = *compress;
#endif
}


/*****************************************************************
 * TAG( mc_get_svar_info ) PUBLIC
 *
 * Returns the amount and type of data associated with the named svar
 * associated with the given class.
 */
static Return_value
get_svar_info(Famid fam_id, char *class_name, char *var_name, int *num_blocks,
              int *size, int *type, int *srec_id, int *sub_srec_id,
              char *agg_svar_name)
{
   Mili_family *fam;
   int i, j, k, l;
   Srec *this_srec;
   Sub_srec *this_sub_srec;
   Svar *this_svar, *target_svar;
   Return_value rval;
   Bool_type found = FALSE;

   rval = validate_fam_id(fam_id);
   if (rval != OK) {
      return rval;
   }

   fam = fam_list[fam_id];

   /* Search through state records. */
   for (i = 0; i < fam->qty_srecs && !found; ++i) {
      this_srec = fam->srecs[i];

      /* Search through state sub-records. */
      for (j = 0; j < this_srec->qty_subrecs && !found; ++j) {
         this_sub_srec = this_srec->subrecs[j];

         /* Check for state sub-record class name match. */
         if (strcmp(class_name, this_sub_srec->mclass) == 0) {

            /* Search through state variables. */
            for (k = 0; k < this_sub_srec->qty_svars && !found; ++k) {
               this_svar = this_sub_srec->svars[k];

               /* Check if this is the right state variable. */
               if (strcmp(var_name, this_svar->name) == 0) {
                  /* Found it! */
                  found = TRUE;
                  *srec_id = i;
                  *sub_srec_id = j;
                  *num_blocks = this_sub_srec->qty_id_blks;
                  strcpy(agg_svar_name, "\0");
                  target_svar = this_svar;
               }
               else if (this_svar->list_size > 0) {
                  /* Look at the state var's state vars. */
                  for (l = 0; l < *this_svar->list_size && !found; ++l) {

                     /* Check if this is the right state variable. */
                     if (strcmp(var_name, this_svar->svars[l]->name) == 0) {
                        /* Found it! */
                        found = TRUE;
                        *srec_id = i;
                        *sub_srec_id = j;
                        *num_blocks = this_sub_srec->qty_id_blks;
                        strcpy(agg_svar_name, this_svar->name);
                        target_svar = this_svar->svars[l];
                     }
                  }
               }
            }
         }
      }
   }

   if (found) {
      *type = *target_svar->data_type;
      if (*target_svar->agg_type == SCALAR) {
         *size = this_sub_srec->mo_qty;
      }
      else if (*target_svar->agg_type == VECTOR) {
         *size = *target_svar->list_size * this_sub_srec->mo_qty;
      }
      else if (*target_svar->agg_type == VEC_ARRAY) {
         *size = *target_svar->list_size * this_sub_srec->mo_qty;
         for (i = 0; i < *target_svar->order; ++i) {
            *size *= target_svar->dims[i];
         }
      }
      else if (*target_svar->agg_type == ARRAY) {
         *size = this_sub_srec->mo_qty;
         for (i = 0; i < *target_svar->order; ++i) {
            *size *= target_svar->dims[i];
         }
      }
      else {
         *size = 0;
         rval = INVALID_SVAR_AGG_TYPE;
      }
   }
   else {
      *srec_id = -1;
      *sub_srec_id = -1;
      *num_blocks = 0;
      *type = 0;
      *size = 0;
      strcpy(agg_svar_name, "\0");
      rval = NO_SVARS;
   }

   return rval;
}


/*****************************************************************
 * TAG( mc_get_svar_size ) PUBLIC
 *
 * Returns the amount and type of data associated with the named svar
 * associated with the given class.
 */
Return_value
mc_get_svar_size(Famid fam_id, char *class_name, char *var_name,
                 int *num_blocks, int *size, int *type)
{
   int srec_id, sub_srec_id;
   char agg_svar_name[512];

   return get_svar_info(fam_id, class_name, var_name, num_blocks,
                        size, type, &srec_id, &sub_srec_id, agg_svar_name);
}


/************************************************************
 * TAG( mc_get_svar_mo_ids_on_class ) PUBLIC
 *
 * Read and return the mesh object ids for a specified class and variable.
 */
Return_value
mc_get_svar_mo_ids_on_class(Famid fam_id, char *class_name,
                            char *svar_name, int *blocks)
{
   char agg_svar_name[512];
   int i, num_blocks, size, type, srec_id, sub_srec_id, block_len;
   Return_value rval;
   const int *blk_src;

   rval = get_svar_info(fam_id, class_name, svar_name, &num_blocks, &size,
                        &type, &srec_id, &sub_srec_id, agg_svar_name);
   if (rval != OK) {
      return rval;
   }

   /* Fill the blocks. */
   block_len = 2*num_blocks;
   blk_src = fam_list[fam_id]->srecs[srec_id]->subrecs[sub_srec_id]->mo_id_blks;
   for (i = 0; i < block_len; ++i) {
      blocks[i] = blk_src[i];
   }

   return OK;
}


/************************************************************
 * TAG( mc_get_svar_on_class ) PUBLIC
 *
 * Read and return a result for a specified class and variable.
 */
Return_value
mc_get_svar_on_class(Famid fam_id, int state, char *class_name,
                     char *svar_name, void *result)
{
   char agg_svar_name[512];
   int num_blocks, size, type, srec_id, sub_srec_id;
   Return_value rval;
   char *read_results_names[2];

   rval = get_svar_info(fam_id, class_name, svar_name, &num_blocks, &size,
                        &type, &srec_id, &sub_srec_id, agg_svar_name);
   if (rval != OK) {
      return rval;
   }

   if (strlen(agg_svar_name) == 0) {
      read_results_names[0] = svar_name;
   }
   else {
      strcat(agg_svar_name, "[");
      strcat(agg_svar_name, svar_name);
      strcat(agg_svar_name, "]");
      read_results_names[0] = agg_svar_name;
   }
   read_results_names[1] = NULL;

   /* Read the requested result for the specified class */
   rval = mc_read_results(fam_id, state, sub_srec_id,
                          1, read_results_names, result);

   if (rval != OK) {
      return rval;
   }

   return OK;
}
