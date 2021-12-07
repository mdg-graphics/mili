/* */

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
 
 * Routines for defining and organizing state variables.
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

extern Return_value delete_svar_with_ios( Svar *psv, IO_mem_store *pcioms,
      IO_mem_store *piioms );
static Return_value taurus_create_svar( Mili_family *fam, char *name, Svar **ppsv );
static Return_value find_delete_svar( Mili_family *fam, char *name );
Return_value taurus_commit_svars( Mili_family *fam );

/*****************************************************************
 * TAG( taurus_def_svars ) PUBLIC
 *
 * Define scalar state variables.
 */
Return_value
taurus_def_svars( Mili_family *fam, int qty, char **names,
                  char **titles, int types[] )
{
   int i;
   int len;
   Svar *psv;
   Return_value status;
   char *warning = "Taurus warning - duplicate state variable ignored";

   for ( i = 0; i < qty; i++ )
   {
      if ( valid_svar_data( SCALAR, names[i], types[i], 0, NULL, 0, NULL ) )
      {
         status = taurus_create_svar( fam, names[i], &psv );
         if ( status == OK )
         {
            len = ios_str_dup( &psv->name, names[i], fam->svar_c_ios );
            if ( len == 0 )
               return IOS_STR_DUP_FAILED;

            if ( strlen( titles[i] ) > 0 )
               len = ios_str_dup( &psv->title, titles[i],
                                  fam->svar_c_ios );
            else
               len = ios_str_dup( &psv->title, names[i],
                                  fam->svar_c_ios );
            if ( len == 0 )
               return IOS_STR_DUP_FAILED;

            psv->agg_type = (Aggregate_type *)
                            ios_alloc( (LONGLONG) 1, fam->svar_i_ios );
            if ( psv->agg_type == NULL )
               return IOS_ALLOC_FAILED;
            *psv->agg_type = SCALAR;

            psv->data_type = (int *) ios_alloc( (LONGLONG) 1,
                                                fam->svar_i_ios );
            if ( psv->data_type == NULL )
               return IOS_ALLOC_FAILED;
            *psv->data_type = types[i];

         }
         else
            /* Force a diagnostic for duplicates, but don't break. */
            if ( mili_verbose )
               fprintf( stderr, "%s: %s\n", warning, names[i] );
      }
      else
      {
         /* Delete previous (successful) entries.  Is this right? */
         for ( i--; i>=0; i-- )
            find_delete_svar( fam, names[i] );
         return INVALID_SVAR_DATA;
      }
   }

   return (Return_value)OK;
}


/*****************************************************************
 * TAG( taurus_def_vec_svar ) PUBLIC
 *
 * Define a vector state variable.
 */
Return_value
taurus_def_vec_svar(Mili_family *fam, int type, int size, char *name,
                    char *title, char **field_names, char **field_titles )
{
   Svar *psv, *psv2;
   char *sv_name, *sv_title, *dumchar;
   Htable_entry *var_entry;
   int i;
   int len;
   Return_value status;

   if ( !valid_svar_data( VECTOR, name, type, size, field_names, 0, NULL ) )
      return INVALID_SVAR_DATA;

   status = taurus_create_svar( fam, name, &psv );
   if ( status == OK )
   {
      len = ios_str_dup( &psv->name, name, fam->svar_c_ios );
      if ( len == 0 )
         return IOS_STR_DUP_FAILED;

      if ( strlen( title ) > 0 )
         len = ios_str_dup( &psv->title, title, fam->svar_c_ios );
      else
         len = ios_str_dup( &psv->title, name, fam->svar_c_ios );
      if ( len == 0 )
         return IOS_STR_DUP_FAILED;

      psv->agg_type = (Aggregate_type *) ios_alloc( (LONGLONG) 1,
                      fam->svar_i_ios );
      if ( psv->agg_type == NULL )
         return IOS_ALLOC_FAILED;
      *psv->agg_type = VECTOR;

      psv->data_type = (int *) ios_alloc( (LONGLONG) 1, fam->svar_i_ios );
      if ( psv->data_type == NULL )
         return IOS_ALLOC_FAILED;
      *psv->data_type = type;

      psv->list_size = (int *) ios_alloc( (LONGLONG) 1, fam->svar_i_ios );
      if ( psv->list_size == NULL )
         return IOS_ALLOC_FAILED;
      *psv->list_size = size;

      /*
       * Save component names.
       * Names are concatenated in memory, so only keep address of first.
       */
      len = ios_str_dup( &psv->list_names, field_names[0], fam->svar_c_ios );
      if ( len == 0 )
         return IOS_STR_DUP_FAILED;

      for ( i = 1; i < size; i++ )
      {
         len = ios_str_dup( &dumchar, field_names[i], fam->svar_c_ios );
         if ( len == 0 )
            return IOS_STR_DUP_FAILED;
      }

      psv->svars = NEW_N( Svar *, size, "Svar array" );

      /*
       * Enter all fields as scalar state variables also, or use existing
       * ones if found.
       */
      for ( i = 0; i < size; i++ )
      {
         /* Could modify "create_svar()" to do merged entries... */

         sv_name = field_names[i];
         sv_title = field_titles[i];

         status = (Return_value) htable_search( fam->svar_table, sv_name,
                                                ENTER_MERGE, &var_entry );
         if (var_entry == NULL)
         {
            break;
         }
         if ( status != ENTRY_EXISTS )
         {
            psv2 = NEW( Svar, "State var" );
            len = ios_str_dup( &psv2->name, sv_name, fam->svar_c_ios );
            if ( len == 0 )
               return IOS_STR_DUP_FAILED;

            if ( strlen( sv_title ) > 0 )
               len = ios_str_dup( &psv2->title, sv_title,
                                  fam->svar_c_ios );
            else
               len = ios_str_dup( &psv2->title, sv_name, fam->svar_c_ios );
            if ( len == 0 )
               return IOS_STR_DUP_FAILED;

            psv2->agg_type = (Aggregate_type *)
                             ios_alloc( (LONGLONG) 1, fam->svar_i_ios );
            if ( psv2->agg_type == NULL )
               return IOS_ALLOC_FAILED;
            *psv2->agg_type = SCALAR;

            psv2->data_type = (int *) ios_alloc( (LONGLONG) 1,
                                                 fam->svar_i_ios );
            if ( psv2->data_type == NULL )
               return IOS_ALLOC_FAILED;
            *psv2->data_type = type;

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
         status = (Return_value)OK;
   }

   return status;
}


/*****************************************************************
 * TAG( taurus_create_svar ) LOCAL
 *
 * Create a new state variable.
 */
static Return_value
taurus_create_svar( Mili_family *fam, char *name, Svar **ppsv )
{
   Hash_action op;
   Htable_entry *phte;
   Return_value status;
   Svar *psv;

   if ( fam->svar_table == NULL )
   {
      /* Create hash table to hold state var def's. */
      fam->svar_table = htable_create( DEFAULT_HASH_TABLE_SIZE );
      /* Create I/O stores to hold character and integer data for def's. */
      fam->svar_c_ios = ios_create( M_STRING );
      if (fam->svar_c_ios == NULL)
      {
         return ALLOC_FAILED;
      }
      fam->svar_i_ios = ios_create( M_INT );
      if (fam->svar_i_ios == NULL)
      {
         return ALLOC_FAILED;
      }
      /*
       * Initially, and after any svar output, allocate a header from
       * the integer output buffer so the header will be first in a
       * subsequent output operation.
       */
      fam->svar_hdr = (int *) ios_alloc( (LONGLONG) QTY_SVAR_HEADER_FIELDS,
                                         fam->svar_i_ios );
      if (fam->svar_hdr == NULL)
      {
         return IOS_ALLOC_FAILED;
      }
   }


   if( strncmp( name, "sand", 4 ) == 0 )
      op = ENTER_ALWAYS;
   else
      op = ENTER_UNIQUE;

   status = (Return_value) htable_search( fam->svar_table, name, op,
                                          &phte );

   if ( status == OK )
   {
      psv = NEW( Svar, "State var" );
      phte->data = (void *) psv;
      *ppsv = psv;
   }
   else
      *ppsv = NULL;

   return status;
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

   rval = (Return_value) htable_search( fam->svar_table, name, FIND_ENTRY,
                                        &phte );
   if ( rval == OK )
   {
      rval = delete_svar_with_ios( (Svar *) phte->data, fam->svar_c_ios,
                                   fam->svar_i_ios );
      htable_del_entry( fam->svar_table, phte );
   }

   return rval;
}

/*****************************************************************
 * TAG( taurus_commit_svars ) PRIVATE
 *
 * Commit in-memory state variable defs to disk.
 *
 * Assumes file is opened and positioned properly.
 */
Return_value
taurus_commit_svars( Mili_family *fam )
{
   LONGLONG int_qty, char_qty, round_qty;
   void *p_v;
   Return_value rval;

   if ( fam->svar_table == NULL )
      return OK;

   int_qty = ios_get_fresh( fam->svar_i_ios );

   if ( int_qty - QTY_SVAR_HEADER_FIELDS > 0 )
   {
      /* Get amount of character data. Round up if necessary. */
      char_qty = ios_get_fresh( fam->svar_c_ios );
      round_qty = ROUND_UP_INT( char_qty, 4 );
      if ( round_qty != char_qty )
      {
         p_v = ios_alloc( round_qty - char_qty, fam->svar_c_ios );
         if ( p_v == NULL )
            return IOS_ALLOC_FAILED;

         char_qty = round_qty;
      }

      /* Fill the header. */
      fam->svar_hdr[SVAR_QTY_INT_WORDS_IDX] = (int) int_qty;
      fam->svar_hdr[SVAR_QTY_BYTES_IDX] = (int)(int)  char_qty;

      /* Add directory entry. */
      rval = add_dir_entry( fam, STATE_VAR_DICT, DONT_CARE, DONT_CARE, 0,
                            NULL, (LONGLONG) DONT_CARE, (LONGLONG) DONT_CARE );
      if ( rval != OK )
         return rval;

      /* Update for subsequent file operations. */
      fam->svar_hdr = ios_alloc( (LONGLONG) QTY_SVAR_HEADER_FIELDS,
                                 fam->svar_i_ios );
      if ( fam->svar_hdr == NULL )
         return IOS_ALLOC_FAILED;

   }

   return OK;
}

