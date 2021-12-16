/*
 * Utility routines.
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
*  I. R. Corey - April 4, 2006: Added new record type for time invarent
*                 data (TI). This data will be written to a new file
*                 trype and is not tied to a specific state.
*                 See SCR #298.
*
*  I. R. Corey - March 20, 2009: Added support for WIN32 for use with
*                with Visit.
*                See SCR #587.
*
*  I. R. Corey - May 17, 2010: Added function mc_countDomains() to
*                support Visit plug-in.
************************************************************************
*/

#ifdef _MSC_VER
#include <windows.h>   //this will give us tchar.h
#include <stdio.h>
#else
#include <dirent.h>
#endif
#include <string.h>
#include <ctype.h>
#include "mili_internal.h"
#include "sarray.h"
#include "mili_endian.h"

#ifndef NULL
#define NULL ((void *) 0)
#endif


static LONGLONG _mem_total = 0;

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
 * TAG( milisilo )
 *
 * Mili-Silo interface flag
 */
extern Bool_type milisilo;

static void to_base26( int num, Bool_type upper, char *base26_rep );



/************************************************************
 * TAG( determine_naming )
 * 
 * Determine if the es_ naming is a stress or strain.
 *
 * Return NULL if it is not a full set of the required parameters.
 *
 */
char *
mc_determine_naming( char *p_name , State_variable *p_sv)
{
   char *stresses[]={"sx","sy","sz","sxy","szx","syz"};
   char *strains[]={"ex","ey","ez","exy","ezx","eyz"};
   int int_array[6]={0};
   int i,j;
   int valid;
   char *return_value = NULL;
   
   if(p_sv->agg_type != VEC_ARRAY && p_sv->agg_type != VECTOR)
   {
       return NULL;
   }
   if(!strcmp(p_name,"strain") || !strcmp(p_name,"stress"))
   {
       return NULL;
   }
   /*lets check stresses first*/
   for(i=0;i<6;i++)
   {
      for(j=0;j<p_sv->vec_size;j++)
      {
          if(!int_array[i] && 
             !strcmp(stresses[i],p_sv->components[j]))
          {
              int_array[i] = 1;
              break;
          }
      }
   }
   
   valid = 1;
   
   for(i=0;i<6;i++)
   {
       if(!int_array[i])
       {
         valid = 0;
       }
   }
   
   if(valid)
   {
       return_value = (char*)malloc(7);
       strcpy(return_value,"stress");
       return return_value;   
   }
   
   /* Well we might as well check for strains if we made it this far. */
   for(i=0;i<6;i++)
   {
       int_array[i]=0;
   }
   
   for(i=0;i<6;i++)
   {
      for(j=0;j<p_sv->vec_size;j++)
      {
          if(!strcmp(strains[i],p_sv->components[j]))
          {
              int_array[i] = 1;
              break;
          }
      }
   }
   
   valid = 1;
   
   for(i=0;i<6;i++)
   {
       if(!int_array[i])
       {
         valid = 0;
       }
   }
   
   if(valid)
   {
       return_value = (char*)malloc(7);
       strcpy(return_value,"strain");
       return return_value;   
   }
   
   return NULL;
}


/*****************************************************************
 * TAG( get_mili_version ) PRIVATE
 *
 * Request Mili Library Version
 */
void
get_mili_version( char *mili_version_ptr )
{
   int len;

   len = strlen( MILI_VERSION );
   if ( len >= M_MAX_NAME_LEN )
   {
      len = M_MAX_NAME_LEN-2;
   }
   strncpy( mili_version_ptr, MILI_VERSION, len );
   mili_version_ptr[len] = '\0';
   return;

}
/*****************************************************************
 * TAG( mc_query_family ) PUBLIC
 *
 * Request information about a family.
 */
Return_value
mc_query_family( Famid fam_id, int which, void *num_args, char *char_args,
                 void *p_info )
{
   Mili_family *fam;
   Return_value rval;
   int blk_size;
   int elem_qty;
   int sclass_qty;
   int srec_idx, subrec_idx;
   Sub_srec *p_ssr;
   int reqd_st;
   int add;
   Mesh_descriptor *p_md;
   int i, j;
   Hash_table *p_ht;
   Htable_entry *p_hte;
   int *int_args;
   State_descriptor *smap;
   float time;
   int qty;
   int *p_i;
   char *p_c;
   char *version;
   float *p_f;
   int mesh_id;
   int state_no, max_st;
   int id, idx;
   int length;
   int state;

   if ( INVALID_FAM_ID( fam_id ) )
   {
      return BAD_FAMILY;
   }
   else
   {
      fam = fam_list[fam_id];
   }

   rval = OK;

   /* Most queries expect integer numeric arguments. */
   int_args = (int *) num_args;

   switch( (Query_request_type) which )
   {
      case QTY_STATES:
         if ( fam->active_family )
         {
            rval = update_active_family( fam );
         }
         if (rval == OK)
         {
            *((int *) p_info) = fam->state_qty;
         }
         break;
      case QTY_DIMENSIONS:
         *((int *) p_info) = fam->dimensions;
         break;
      case QTY_MESHES:
         *((int *) p_info) = fam->qty_meshes;
         break;
      case MESH_NAME:
         mesh_id = int_args[0];
         if (mesh_id < 0 || mesh_id > fam->qty_meshes - 1)
         {
            rval = NO_MESH;
         }
         else
         {
            strcpy((char*)p_info, fam->meshes[mesh_id]->name);
         }
         break;
      case QTY_SREC_FMTS:
         *((int *) p_info) = fam->qty_srecs;
         break;
      case QTY_SUBRECS:
         srec_idx = *int_args;
         if ( srec_idx < 0 || srec_idx > fam->qty_srecs - 1 )
         {
            rval = INVALID_SREC_INDEX;
         }
         else
         {
            *((int *) p_info) = fam->srecs[*int_args]->qty_subrecs;
         }
         break;
      case QTY_SUBREC_SVARS:
         srec_idx = int_args[0];
         if ( srec_idx < 0 || srec_idx > fam->qty_srecs - 1 )
         {
            rval = INVALID_SREC_INDEX;
         }
         else
         {
            subrec_idx = int_args[1];
            if ( subrec_idx < 0 ||
                  subrec_idx > fam->srecs[srec_idx]->qty_subrecs - 1 )
            {
               rval = INVALID_SUBREC_INDEX;
            }
            else
            {
               p_ssr = fam->srecs[srec_idx]->subrecs[subrec_idx];
               *((int *) p_info) = p_ssr->qty_svars;
            }
         }
         break;
      case QTY_SVARS:
         if ( fam->svar_table != NULL )
         {
            *((int *) p_info) = fam->svar_table->qty_entries;
         }
         else
         {
            *((int *) p_info) = 0;
         }
         break;
      case QTY_NODE_BLKS:
         *((int *) p_info) = count_node_entries( fam, *int_args, char_args );
         break;
      case QTY_NODES_IN_BLK:
         rval = get_node_block_size( fam, int_args, char_args, &blk_size );
         if ( rval == OK )
         {
            *((int *) p_info) = blk_size;
         }
         break;
      case QTY_CLASS_IN_SCLASS:
         rval = get_class_qty( fam, int_args, &sclass_qty );
         if (rval == OK)
         {
            *((int *) p_info) = sclass_qty;
         }
         break;
      case QTY_ELEM_CONN_DEFS:
         *((int *) p_info) = count_elem_conn_defs( fam, *int_args,
                             char_args );
         break;
      case QTY_ELEMS_IN_DEF:
         rval = get_elem_qty_in_def( fam, int_args, char_args, &elem_qty );
         if ( rval == OK )
         {
            *((int *) p_info) = elem_qty;
         }
         break;
      case SREC_FMT_ID:
         reqd_st = *int_args;
         if ( fam->active_family && reqd_st > fam->state_qty )
         {
            rval = update_active_family( fam );
         }
         if (rval == OK)
         {
            if ( reqd_st < 1 || reqd_st > fam->state_qty )
            {
               rval = INVALID_STATE;
            }
            else
            {
               *((int *) p_info) = fam->state_map[reqd_st - 1].srec_format;
            }
         }
         break;
      case SERIES_SREC_FMTS:
         for ( i = 0; i < 2; i++ )
         {
            if ( int_args[i] < 1 || int_args[i] > fam->state_qty )
            {
               rval = INVALID_STATE;
               break;
            }
         }
         if ( rval == OK )
         {
            p_i = (int *) p_info;
            max_st = int_args[1];
            for ( i = int_args[0] - 1, idx = 0; i < max_st; i++ )
            {
               p_i[idx++] = fam->state_map[i].srec_format;
            }
         }
         break;
      case SUBREC_CLASS:
         srec_idx = int_args[0];
         if ( srec_idx < 0 || srec_idx > fam->qty_srecs - 1 )
         {
            rval = INVALID_SREC_INDEX;
         }
         else
         {
            subrec_idx = int_args[1];
            if ( subrec_idx < 0 ||
                  subrec_idx > fam->srecs[srec_idx]->qty_subrecs - 1 )
            {
               rval = INVALID_SUBREC_INDEX;
            }
            else
            {
               p_ssr = fam->srecs[srec_idx]->subrecs[subrec_idx];
               strcpy( (char*)p_info, p_ssr->mclass );
            }
         }
         break;
      case SREC_MESH:
         srec_idx = int_args[0];
         if ( srec_idx < 0 || srec_idx > fam->qty_srecs - 1 )
         {
            rval = INVALID_SREC_INDEX;
         }
         else
         {
            p_md = fam->srec_meshes[srec_idx];
            for ( i = 0; i < fam->qty_meshes; i++ )
            {
               if ( p_md == fam->meshes[i] )
               {
                  break;
               }
            }
            *((int *) p_info) = i;
         }
         break;
      case CLASS_SUPERCLASS:
         mesh_id = int_args[0];
         if ( mesh_id > fam->qty_meshes - 1 )
         {
            rval = NO_MESH;
         }
         else
         {
            p_ht = fam->meshes[mesh_id]->mesh_data.umesh_data;
            rval = htable_search( p_ht, char_args, FIND_ENTRY, &p_hte );
            if ( p_hte != NULL )
            {
               *((int *) p_info) =
                  ((Mesh_object_class_data *) p_hte->data)->superclass;
            }
         }
         break;
      case STATE_TIME:
         state_no = int_args[0];
         if ( state_no < 1 || state_no > fam->state_qty )
         {
            rval = INVALID_STATE;
         }
         else
         {
            *((float *) p_info) = fam->state_map[state_no - 1].time;
         }
         break;
      case SERIES_TIMES:
         state_no = int_args[0];
         max_st = int_args[1];
         if ( state_no < 1 || state_no > fam->state_qty ||
               max_st < 1 || max_st > fam->state_qty )
         {
            rval = INVALID_STATE;
            break;
         }
         state_no--;
         max_st--;
         p_f = (float *) p_info;
         add = ( ((max_st - state_no) & 0x80000000) == 0 ) ? 1 : -1;
         for ( i = state_no, j = 0; i != max_st; i += add, j++ )
         {
            p_f[j] = fam->state_map[i].time;
         }
         p_f[j] = fam->state_map[i].time;
         break;
      case MULTIPLE_TIMES:
         qty = int_args[0];
         for ( i = 1; i <= qty; i++ )
         {
            if ( int_args[i] < 1 || int_args[i] > fam->state_qty )
            {
               rval = INVALID_STATE;
               break;
            }
         }
         if ( rval == OK )
         {
            p_f = (float *) p_info;
            for ( i = 1; i <= qty; i++ )
            {
               p_f[i - 1] = fam->state_map[int_args[i] - 1].time;
            }
         }
         break;
      case TIME_OF_STATE:
         /* States are 1-based. */
         state = ((int *) num_args)[0];
         if (state < 1 || state > fam->state_qty) {
            return INVALID_STATE;
         }
         ((float*)p_info)[0] = fam->state_map[state-1].time;
         break;
      case STATE_OF_TIME:
         time = ((float *) num_args)[0];
         smap = fam->state_map;
         qty = fam->state_qty;
         p_i = (int *) p_info;
         if ( qty > 1 )
         {
            i = 0;
            while ( i < qty && time >= smap[i].time )
            {
               i++;
            }
            if ( i == 0 )
            {
               /* Time precedes first state time. */
               rval = INVALID_TIME;
            }
            else if ( i < qty )
            {
               /* The "usual" case, time somewhere in the middle. */
               p_i[0] = i;
               p_i[1] = i + 1;
            }
            else if ( time == smap[i - 1].time )
            {
               /* Time matches last state time. */
               p_i[0] = i - 1;
               p_i[1] = i;
            }
            else
            {
               /* Time exceeds last state time. */
               rval = INVALID_TIME;
            }
         }
         else if ( qty == 1 && time == smap[0].time )
         {
            p_i[0] = p_i[1] = 1;
         }
         else   /* qty == 0 -> no state data */
         {
            rval = INVALID_TIME;
         }
         break;
      case NEXT_STATE_LOC:
         p_i = (int *) p_info;
         rval = gen_next_state_loc( fam, int_args[0], int_args[1],
                                    p_i, p_i + 1 );
         break;
      case CLASS_EXISTS:
         mesh_id = int_args[0];
         length = strlen( char_args);
         if ( mesh_id < 0 || mesh_id > fam->qty_meshes - 1 )
         {
            rval = NO_MESH;
         }
         else if ( char_args[0] == '\0' )
         {
            rval = INVALID_CLASS_NAME;
         }
         else if (  length <= 0 )
         {
            rval = INVALID_CLASS_NAME;
         }
         else
         {
            p_ht = fam->meshes[int_args[0]]->mesh_data.umesh_data;
            rval = htable_search( p_ht, char_args, FIND_ENTRY, &p_hte );
            *((int *) p_info) = ( rval == OK );
         }
         break;
      case LIB_VERSION:
         /*****  Fixed to return the library version
                 used when the database was written *****/
         p_ht = fam->param_table;
         rval = htable_search( p_ht, char_args, FIND_ENTRY, &p_hte );
         if( p_hte != NULL )
         {
            version = (char *)p_hte->data;
            p_c = (char *) p_info;
            qty = strlen( version );
            if ( qty > M_MAX_NAME_LEN )
            {
               qty = M_MAX_NAME_LEN;
            }
            strncpy( p_c, MILI_VERSION, qty );
            p_c[qty + 1] = '\0';
         }
         break;
      case STATE_SIZE:
         srec_idx = *int_args;
         if ( srec_idx < 0 || srec_idx > fam->qty_srecs - 1 )
         {
            rval = INVALID_SREC_INDEX;
         }
         else
         {
            *((int *) p_info) = fam->srecs[srec_idx]->size;
         }
         break;
      case QTY_FACETS_IN_SURFACE:
         mesh_id = int_args[0];
         p_ht = fam->meshes[mesh_id]->mesh_data.umesh_data;
         rval = htable_search( p_ht, char_args, FIND_ENTRY, &p_hte );
         if ( p_hte != NULL )
         {
            id = int_args[1];
            *((int *) p_info) =
               ((Mesh_object_class_data *) p_hte->data)->surface_sizes[ id - 1 ];
         }
         break;
      default:
         rval = UNKNOWN_QUERY_TYPE;
         break;
   }

   return rval;
}


/*****************************************************************
 * TAG( mc_cleanse_subrec ) PUBLIC
 *
 * Convenience function to free dynamically allocated structures
 * from a Subrecord struct.
 */
Return_value
mc_cleanse_subrec( Subrecord *p_subrec )
{
   int i;

   free( p_subrec->name );
   free( p_subrec->class_name );
   if(p_subrec->mo_blocks != NULL) {
      free( p_subrec->mo_blocks );
   }
   
   for ( i = 0; i < p_subrec->qty_svars; i++ )
   {
      free( p_subrec->svar_names[i] );
   }
   if( p_subrec->svar_names != NULL) {
      free( p_subrec->svar_names );
   }

   return OK;
}


/*****************************************************************
 * TAG( mc_cleanse_st_variable ) PUBLIC
 *
 * Convenience function to free dynamically allocated structures
 * from a State_variable struct.
 */
Return_value
mc_cleanse_st_variable( State_variable *p_svar )
{
   int i;

   free( p_svar->short_name );
	p_svar->short_name = NULL;
   free( p_svar->long_name );
	p_svar->long_name = NULL;
	
   if ( p_svar->agg_type != 0 )
   {
      if ((p_svar->agg_type != SCALAR) && (p_svar->agg_type != VECTOR) &&
          (p_svar->dims != NULL)) {
         free( p_svar->dims );
			p_svar->dims = NULL;
      }

      if ( p_svar->agg_type == VEC_ARRAY || p_svar->agg_type == VECTOR )
      {
         for ( i = 0; i < p_svar->vec_size; i++ )
         {
            free( p_svar->components[i] );
				p_svar->components[i] = NULL;
            free( p_svar->component_titles[i] );
				p_svar->component_titles[i] = NULL;
         }

         free( p_svar->components );
			p_svar->components = NULL;
         free(p_svar->component_titles);
			p_svar->component_titles = NULL;
      }
   }

   return OK;
}


/*****************************************************************
 * TAG( parse_int_list ) PRIVATE
 *
 * Parse a string containing a space- or comma-separated list of
 * integers.  Store the integers (as integer data) in a dynamically
 * allocated array.  Return the array and its size as
 * pass-by-reference arguments.
 */
Return_value
parse_int_list( char *list_string, int *count, int **iarray )
{
   int cnt, i;
   char *int_string;
   char *delimiters = ", ";
   struct int_node
   {
      struct int_node *next;
      int value;
   } *inp = NULL, *inlist = NULL;

   /* Parse the string and store the integers in a linked list. */
   int_string = strtok( list_string, delimiters );
   cnt = 0;
   while( int_string != NULL )
   {
      inp = (struct int_node *) calloc( 1, sizeof( struct int_node ) );
      if (inp == NULL)
      {
         return ALLOC_FAILED;
      }
      inp->value = atoi( int_string );
      inp->next = inlist;
      inlist = inp;

      cnt++;

      int_string = strtok( NULL, delimiters );
   };

   /*
    * Allocate space for the array then copy integers from the
    * linked-list into the array in their original order.
    */
   *iarray = (int *) calloc( cnt, sizeof( int ) );
   if (cnt > 0 && *iarray == NULL)
   {
      return ALLOC_FAILED;
   }

   for ( i = cnt - 1; i >= 0; i-- )
   {
      (*iarray)[i] = inlist->value;
      inp = inlist;
      inlist = inlist->next;
      free( inp );
   }

   *count = cnt;

   return OK;
}


/*****************************************************************
 * TAG( str_dup ) PRIVATE
 *
 * Duplicate a string with space allocation. Return length of
 * string duplicated.
 */
int
str_dup( char **dest, char *src )
{
   LONGLONG len;
   char *pd, *ps;

   len = strlen( src );
   *dest = (char *) calloc( len + 1, sizeof( char ) );

   if (*dest != NULL)
   {
      pd = *dest;
      ps = src;
      while ( (*pd++ = *ps++) );
   }

   return (int) len;
}


/*****************************************************************
 * TAG( str_dup_f2c ) PRIVATE
 *
 * Duplicate a FORTRAN CHARACTER parameter into a C string with
 * space allocation. Return length of C string.
 */
int
str_dup_f2c( char **dest, char *src, int ftn_len )
{
   char *p_c, *p_new;
   int c_len, i;

   for ( p_c = src + ftn_len - 1; p_c >= src; p_c-- )
   {
      if ( *p_c != ' ' )
      {
         break;
      }
   }

   if ( p_c < src )
   {
      *dest = NULL;
      return 0;
   }

   c_len = (int) (p_c - src + 1);
   p_new = NEW_N( char, c_len + 1, "FORTRAN character copy" );
   if (p_new == NULL)
   {
      *dest = NULL;
      return 0;
   }
   for ( i = 0; i < c_len; i++ )
   {
      p_new[i] = src[i];
   }

   *dest = p_new;

   return c_len;
}


/*****************************************************************
 * TAG( is_numeric ) PRIVATE
 *
 * Evaluate a string and return TRUE if each character is a member
 * of 0-9.
 */
int
is_numeric( char *ptest )
{
   char *p_c;

   for ( p_c = ptest; *p_c != (char) 0; p_c++ )
   {
      if ( !isdigit( (int) *p_c ) )
      {
         return FALSE;
      }
   }

   return ( p_c == ptest ) ? FALSE : TRUE;
}


/*****************************************************************
 * TAG( is_all_upper ) PRIVATE
 *
 * Evaluate a string and return TRUE if each character is a an
 * upper-case letter.
 */
int
is_all_upper( char *ptest )
{
   char *p_c;

   for ( p_c = ptest; *p_c; p_c++ )
   {
      if ( !isupper( (int) *p_c ) )
      {
         return FALSE;
      }
   }

   return ( p_c == ptest ) ? FALSE : TRUE;
}


/*****************************************************************
 * TAG( make_fnam ) PRIVATE
 *
 * Build a state or non-state filename.
 */
void
make_fnam( int ftype, Mili_family *fam, int fnum, char dest[] )
{
   char numtext[8];

   if( fam->db_type == TAURUS_DB_TYPE )
   {
      ftype = TAURUS_DATA;
   }

   switch( ftype )
   {
      case STATE_DATA:
         sprintf( dest, "%s%0*d", fam->root, fam->st_suffix_width, fnum );
         break;

      case NON_STATE_DATA:
         to_base26( fnum, TRUE, numtext );
         sprintf( dest, "%s%s", fam->root, numtext );
         break;

      case TAURUS_DATA:
         if ( fnum == 0 )
         {
            strcpy( dest, fam->root );
         }
         else if ( fnum < 100 )
         {
            sprintf( dest, "%s%02d", fam->root, fnum );
         }
         else
         {
            sprintf( dest, "%s%03d", fam->root, fnum );
         }
         break;

      case WRITE_LOCK:
         sprintf( dest, "%swlock", fam->root );
         break;

      case TI_DATA:
         to_base26( fnum, TRUE, numtext );
         sprintf( dest, "%s_TI_%s", fam->root, numtext );
         break;
   }
}


/*****************************************************************
 * TAG( to_base26 ) LOCAL
 *
 * Generate a base-26 string from an integer using upper- or
 * lower-case alphabet characters as numerals.
 */
static void
to_base26( int num, Bool_type upper, char *base26_rep )
{
   int pwr, n, next_n, i, last, mult, zero;

   /* Find the largest power of 26 that fits into num. */
   pwr = 0;
   n = 1;
   while ( (next_n = n * 26) - 1 < num )
   {
      n = next_n;
      pwr++;
   }

   /* Subtract out multiples of powers of 26 to generate base26_rep. */
   zero = upper ? (int) 'A' : (int) 'a';
   last = pwr;
   for ( i = 0; i <= last; i++ )
   {
      mult = num / n;
      base26_rep[i] = (char) (zero + mult);
      num -= mult * n;
      n /= 26;
   }

   base26_rep[last + 1] = '\0';
}


/*****************************************************************
 * TAG( get_file_index ) PRIVATE
 *
 * Extract the zero-based file index from a file name as a
 * base-ten integer.
 */
Return_value
get_file_index( int file_type, char *fam_root, char *file_name, int *index )
{
   char *p_test, *p_root, *p_c;
   int sum, n;

   /* Locate beginning of indexing characters. */

   /* Find last occurrence of family root in file_name. */
   p_test = strstr( file_name, fam_root );

   /* Error, the family root does not exist in file_name. */
   if (p_test == NULL)
   {
      return FILE_FAMILY_NAME_MISMATCH;
   }

   do
   {
      p_root = p_test;
      p_test = strstr( p_test + 1, fam_root );
   }
   while ( p_test != NULL );

   p_test = p_root + strlen( fam_root );

   if ( file_type == NON_STATE_DATA )
   {
      /* Convert text representation of base-26 numeral to base-10 integer. */

      /* Point to last of indexing characters. */
      for ( p_c = p_test + 1; *p_c; p_c++ );
      p_c--;

      n = 1;
      sum = 0;
      for ( ; p_c >= p_test; p_c-- )
      {
         sum += ((int) *p_c - (int) 'A') * n;
         n *= 26;
      }
      *index = sum;
   }
   else if ( file_type == STATE_DATA )
   {
      /* Convert text representation of base-10 numeral to integer. */
      *index = atoi( p_test );
   }
   else
   {
      /* Error. */
      return FILE_FAMILY_NAME_MISMATCH;
   }

   return OK;
}


/*****************************************************************
 * TAG( mc_get_mesh_id ) PUBLIC
 *
 * Map a mesh name into a mesh identifier.
 */
Return_value
mc_get_mesh_id( Famid fam_id, char *mesh_name, int *p_mesh_id )
{
   Mili_family *fam;
   int i;
   Return_value status = NO_MATCH;

#ifdef SILOENABLED
   if (milisilo)
   {
      status = mc_get_mesh_id( fam_id, mesh_name, p_mesh_id );
      return ( status );
   }
#endif

   fam = fam_list[fam_id];

   for ( i = 0; i < fam->qty_meshes; i++ )
   {
      if ( strcmp( fam->meshes[i]->name, mesh_name ) == 0 )
      {
         *p_mesh_id = i;
         status =  OK;
         break;
      }
   }

   return status;
}


/*****************************************************************
 * TAG( my_calloc ) PRIVATE
 *
 * Calloc instrumented to keep track of allocations.
 */
char *
my_calloc( int cnt, long size, char *descr )
{
   char *arr;

   _mem_total += cnt*size;

#ifdef DEBUG_MEM
   fprintf( stderr, "Allocating memory for %s: %d bytes, total %d K.\n",
            descr, cnt*size, _mem_total/1024 );
#endif

   arr = (char *)calloc( cnt, size );
   if ( arr == NULL )
   {
      fprintf( stderr, "Memory allocation failed. Bye.\n" );
   }

   return arr;
}


/*****************************************************************
 * TAG( my_realloc ) PRIVATE
 *
 * Realloc and clear new memory.  Warning - call
 * syntax enables caller to have a dangling reference if return
 * value is not assigned to argument passed into "ptr".
 */
void *
my_realloc( void *ptr, long size, long add, char *descr )
{
   long new_size;

   new_size = size + add;

   ptr = realloc( ptr, new_size );

   if ( ptr != NULL && add > 0 )
   {
      memset( (char *) ptr + size, (int) '\0', add );
   }
   else
   {
      if ( mili_verbose )
      {
         fprintf( stderr, "Mili - realloc failed for \"%s\".\n", descr );
      }
   }
   
#ifdef DEBUG_MEM
   _mem_total += add;

   fprintf( stderr, "Reallocating memory for %s: %d bytes, total %d K.\n",
            descr, add, _mem_total / 1024 );
#endif
   return ptr;
}


/*****************************************************************
 * TAG( mili_recalloc ) PRIVATE
 *
 * Realloc if possible with clear of new memory.  Warning - call
 * syntax enables caller to have a dangling reference if return
 * value is not assigned to argument passed into "ptr".
 */
void *
mili_recalloc( void *ptr, long size, long add, char *descr )
{
   unsigned char *new;
   long new_size;

   new_size = size + add;

   new = (unsigned char *) realloc( ptr, new_size );

   if ( new == NULL )
   {
      if ( mili_verbose )
      {
         fprintf( stderr, "Mili - realloc failed for \"%s\".\n", descr );
      }
   }

   memset( new + size, 0, add );

#ifdef DEBUG_MEM
   _mem_total += add;

   fprintf( stderr, "Reallocating memory for %s: %d bytes, total %d K.\n",
            descr, add, _mem_total / 1024 );
#endif

   return (void *) new;
}


/*****************************************************************
 * TAG( mc_set_buffer_qty ) PUBLIC
 *
 * Set the quantity of input buffers for one or all mesh object
 * classes in a mesh.
 */
Return_value
mc_set_buffer_qty( Famid fam_id, int mesh_id, char *class_name, int buf_qty )
{
   Mili_family *fam;
   Return_value rval, rval_accum;
   Mesh_descriptor *p_mesh;
   Bool_type reset_all_classes;
   Srec *p_srec;
   Sub_srec *p_subrec;
   int i, j;

   rval = validate_fam_id( fam_id );
   if ( rval != OK )
   {
      return rval;
   }
   fam = fam_list[fam_id];

   if ( mesh_id < 0 || mesh_id > fam->qty_meshes - 1 )
   {
      return NO_MESH;
   }
   p_mesh = fam->meshes[mesh_id];

   if ( class_name == NULL || *class_name == '\0' )
   {
      reset_all_classes = TRUE;
   }
   else
   {
      reset_all_classes = FALSE;
   }

   /*
    * Traverse all state record formats for the mesh and identify
    * subrecords bound to a requested class.
    */
   rval_accum = OK;
   for ( i = 0; i < fam->qty_srecs; i++ )
   {
      if ( fam->srec_meshes[i] == p_mesh )
      {
         /* This format belongs to specified mesh. */

         p_srec = fam->srecs[i];

         for ( j = 0; j < p_srec->qty_subrecs; j++ )
         {
            p_subrec = p_srec->subrecs[j];

            if ( p_subrec->organization == OBJECT_ORDERED &&
                  ( reset_all_classes ||
                    strcmp( class_name, p_subrec->mclass ) == 0 ) )
            {
               /* Have class match; re-init buffer queue. */
               rval = init_buffer_queue( p_subrec->ibuffer, buf_qty,
                                         p_subrec->ibuffer->buffer_size );
               if ( rval != OK )
               {
                  rval_accum = rval;
               }
            }
         }
      }
   }

   return rval_accum;
}


/*****************************************************************
 * TAG( create_buffer_queue ) PRIVATE
 *
 * Allocate and initialize a Buffer_queue struct.
 */
Buffer_queue *
create_buffer_queue( int buf_qty, long length )
{
   Buffer_queue *p_bq;
   Return_value rval;

   p_bq = NEW( Buffer_queue, "New buffer queue" );
   if (p_bq == NULL)
   {
      return NULL;
   }

   rval = init_buffer_queue( p_bq, buf_qty, length );
   if ( rval != OK )
   {
      delete_buffer_queue( p_bq );
      return NULL;
   }

   return p_bq;
}


/*****************************************************************
 * TAG( init_buffer_queue ) PRIVATE
 *
 * (Re-)Initialize a buffer queue.
 */
Return_value
init_buffer_queue( Buffer_queue *p_bq, int buf_qty, long length )
{
   unsigned char **p_tmp_dat, **p_swap_dat;
   int *p_tmp_st;
   int cnt;
   int i, j;
   Bool_type new_memory;
   int save, gone;
   Return_value rval;
   int alloc_sz = 0;

   /*
    * Attempt to allocate memory first if necessary to capture any
    * allocation failures before modifying existing structures.
    *
    * Note that if changing a buffer size, this will result in both
    * the old buffers and the new ones being allocated
    * simultaneously, and so could conceivably cause an alloc failure
    * unnecessarily.
    */

   rval = OK;
   new_memory = FALSE;
   if ( buf_qty > p_bq->buffer_count || p_bq->buffer_size != length )
   {
      cnt = buf_qty - p_bq->buffer_count;
      alloc_sz = cnt;
      p_tmp_dat = NEW_N( unsigned char *, cnt, "Temp input buffer array" );
      if (cnt > 0 && p_tmp_dat == NULL)
      {
         return  ALLOC_FAILED;
			
      }
      else
      {
         for ( i = 0; i < cnt; i++ )
         {
            p_tmp_dat[i] = NEW_N( unsigned char, (LONGLONG)length, "Input buffer" );

            if ( length > 0 && p_tmp_dat[i] == NULL )
            {
               /* Failure - clean-up and return. */
               for ( j = 0; j < i; j++ )
               {
                  free( p_tmp_dat[j] );
               }
               free( p_tmp_dat );
               return ALLOC_FAILED;
            }
         }

         new_memory = TRUE;
      }
   }

   if ( rval != OK )
   {
      return rval;
   }

   /* If buffer already used and buffer size changes or no buffering... */
   if ( p_bq->buffer_count != 0 &&
         ( p_bq->buffer_size != length || buf_qty == 0 ) )
   {
      /* Free everything to start from scratch. */

      for ( i = 0; i < p_bq->buffer_count; i++ )
      {
         free( p_bq->data_buffers[i] );
      }
      free( p_bq->data_buffers );
      free( p_bq->state_numbers );
      p_bq->buffer_count = 0;
   }

   if ( p_bq->buffer_count == 0 && buf_qty != 0 )
   {
      /* A clean Buffer_queue - create the queue. */

      p_bq->data_buffers = (void **) NEW_N( float *, buf_qty,
                                            "Input buffer queue" );
      p_bq->state_numbers = NEW_N( int, buf_qty, "Input queue states" );
      if ((buf_qty > 0) &&
            (p_bq->data_buffers == NULL || p_bq->state_numbers == NULL))
      {
         if (alloc_sz > 0)
         {
            for (i = 0; i < alloc_sz; i++)
            {
               free( p_tmp_dat[i] );
            }
            free( p_tmp_dat );
         }
         return ALLOC_FAILED;
      }
      for ( i = 0; i < buf_qty; i++ )
      {
         p_bq->data_buffers[i] = p_tmp_dat[i];
         p_bq->state_numbers[i] = -1;
      }
      p_bq->new_count = buf_qty;
      p_bq->buffer_count = buf_qty;
      p_bq->buffer_size = length;
      p_bq->recent = -1;
   }
   else if ( p_bq->buffer_count > buf_qty )
   {
      /* Shrink the queue */

      /* Free the excess data buffers. */
      for ( i = 0, gone = p_bq->recent; i < p_bq->buffer_count - buf_qty;
            i++ )
      {
         gone = (gone + 1) % p_bq->buffer_count;
         free( p_bq->data_buffers[gone] );

         if ( p_bq->state_numbers[gone] == -1 )
         {
            p_bq->new_count--;
         }
      }

      /* Migrate remaining buffers to low end of queue. */

      p_swap_dat = NEW_N( unsigned char *, buf_qty, "Temp input buf array" );
      p_tmp_st = NEW_N( int, buf_qty, "Temp buffer state array" );
      if ((buf_qty > 0) && (p_swap_dat == NULL || p_tmp_st == NULL))
      {
         if (alloc_sz > 0)
         {
            for (i = 0; i < alloc_sz; i++)
            {
               free( p_tmp_dat[i] );
            }
            free( p_tmp_dat );
         }
         return ALLOC_FAILED;
      }

      /* Move the keepers to temporary storage. */
      for ( i = 0, save = gone; i < buf_qty; i++ )
      {
         save = (save + 1) % p_bq->buffer_count;

         p_swap_dat[i] = p_bq->data_buffers[save];
         p_tmp_st[i] = p_bq->state_numbers[save];
      }

      /* Now move them back to the Buffer_queue. */
      for ( i = 0; i < buf_qty; i++ )
      {
         p_bq->data_buffers[i] = p_swap_dat[i];
         p_bq->state_numbers[i] = p_tmp_st[i];
      }

      free( p_swap_dat );
      free( p_tmp_st );

      /* Free up unused ends of queue arrays. */
      p_bq->data_buffers = RENEW_N( void *, p_bq->data_buffers,
                                    p_bq->buffer_count,
                                    buf_qty - p_bq->buffer_count,
                                    "Reduced input buffer queue" );
      p_bq->state_numbers = RENEW_N( int, p_bq->state_numbers,
                                     p_bq->buffer_count,
                                     buf_qty - p_bq->buffer_count,
                                     "Reduced input queue states" );
   }
   else if ( p_bq->buffer_count < buf_qty )
   {
      /* Extend the queue. */

      cnt = buf_qty - p_bq->buffer_count;

      p_bq->data_buffers = RENEW_N( void *, p_bq->data_buffers,
                                    p_bq->buffer_count, cnt,
                                    "Extended input buffer queue" );
      p_bq->state_numbers = RENEW_N( int, p_bq->state_numbers,
                                     p_bq->buffer_count, cnt,
                                     "Extended input queue states" );
      if ((cnt > 0) &&
            (p_bq->data_buffers == NULL || p_bq->state_numbers == NULL))
      {
         if (alloc_sz > 0)
         {
            for (i = 0; i < alloc_sz; i++)
            {
               free( p_tmp_dat[i] );
            }
            free( p_tmp_dat );
         }
         return ALLOC_FAILED;
      }

      /* Move the previously allocated new buffers to the Buffer_queue. */
      for ( i = p_bq->buffer_count; i < buf_qty; i++ )
      {
         p_bq->data_buffers[i] = p_tmp_dat[i - p_bq->buffer_count];
         p_bq->state_numbers[i] = -1;
      }

      p_bq->buffer_count = buf_qty;
      p_bq->new_count += cnt;
   }
   /*
    * else
    *     Request is for same quantity and size of buffers as already
    *     exists, so do nothing.
    */

   if ( new_memory )
   {
      free( p_tmp_dat );
   }

   return rval;
}


/*****************************************************************
 * TAG( delete_buffer_queue ) PRIVATE
 *
 * Free memory resources allocated to a Buffer_queue.
 */
void
delete_buffer_queue( Buffer_queue *p_bq )
{
   int i;

   /* Sanity check. */
   if ( p_bq == NULL )
   {
      return;
   }

   if ( p_bq->data_buffers != NULL )
   {
      for ( i = 0; i < p_bq->buffer_count; i++ )
      {
         free( p_bq->data_buffers[i] );
      }

      free( p_bq->data_buffers );
   }

   if ( p_bq->state_numbers != NULL )
   {
      free( p_bq->state_numbers );
   }

   free( p_bq );
}

/************************************************************
 * TAG( find_proc_count )
 *
 * This function is to get the number of processor for family 
 * that is being opened.  This was mainly added for backward 
 * compatibility for plot files that do not contain the parameter
 * nproc which Mili checks for now as part of the startup process.
 *
 */
int
find_proc_count(Famid fam_id)
{
    LONGLONG rlen,
           flen;
    int count = 0,
        found_A = 0,
        rval,
        multifiles_found = 0,
        single_file = 0,
        start_check,
        numbers=0;
    
    Mili_family *family;
    rval = validate_fam_id( fam_id );
    
    if ( rval != OK )
    {
        return rval;
    }
    
    family = fam_list[fam_id];
	
    //Just make sure if the database does have the nproc
    //then we do not need to go through everything else.
    rval = mc_read_scalar( fam_id, "nproc", &count );
    if(rval == OK)
    {
        return count;
    }
    
    rlen = strlen(family->file_root);
    
#ifndef _MSC_VER
    
    DIR *dirp;
    struct dirent   *entry;
    
    if((dirp = opendir(family->path)) == NULL) {
        
        return -1;
    }
    
    do{
        if((entry = readdir(dirp)) != NULL) 
        {
            if ((strcmp(entry->d_name, ".")== 0) ||
                (strcmp(entry->d_name, "..") == 0)) {
                    continue;
            }
            
            flen = strlen(entry->d_name);
        
            if ((strncmp(family->file_root,entry->d_name,rlen) ==0))
            {
                
                if(!(entry->d_name[flen-1]== 'A'))
                {
                    continue;
                }
                
                //We need to check on a few different scenarios
                
                //Let's see if this is just a single file.
                if(flen == rlen+1)
                {
                    count++;
                    found_A=1;
                    single_file = 1;
                    continue;
                }
                
                // If it was not a single file then check that the next 
                // letter is a digit
                if(!(entry->d_name[rlen]>='0' && entry->d_name[rlen]<='9'))
                {
                    continue;
                }
                
                //Check for how many processor files.           
		        for(start_check = strlen(family->file_root); start_check< strlen(entry->d_name) && 
                                                             !found_A; start_check++)
                {
			        if(entry->d_name[start_check] == 'A' && start_check == strlen(entry->d_name)-1 )
                    {
				        
                        found_A=1;
			        }
			
			        if(entry->d_name[start_check]>='0' && entry->d_name[start_check]<='9')
                    {
                        numbers++;
			        }
		        }
		        if(found_A && numbers>0)
                {
      	            count++;
                    multifiles_found=1;
                    found_A = 0;
		        }
            }
            numbers =0;
        }
        
        
        
    
    }while(entry != NULL);
    
   
    
#else
    //   WIN32_FIND_DATA fd; 
    WIN32_FIND_DATAA fd; 
    HANDLE dirHandle;
    //   TCHAR szDir[MAX_PATH+1]; 
    char szDir[MAX_PATH+1];  
    int index;
    char TrailStr[3];
    
    // Load var szDir
    TrailStr[0]='\\';
    TrailStr[1]='*';
    TrailStr[2]='\0';
    strcpy(szDir,family->path);        //Begin string szDir with the path
    strcat(szDir,TrailStr);	  //Add "\*" to the end

//Searches a directory for the first file matching szDir (Ex: szDir=.\*)
    dirHandle = FindFirstFileA(szDir, &fd); 
   
    if ( dirHandle == INVALID_HANDLE_VALUE )
    {
        printf("\nfind_proc_count : FindFirstFile status (%d)\n", GetLastError());  
        printf("              szDir=%s \n",szDir);  
        return 0;
    }
    
    do
    {
        if(strncmp(fd.cFileName, family->fam_root, rlen) == 0)
        {
             if ((strcmp(fd.cFileName, ".")== 0) ||
                (strcmp(fd.cFileName, "..") == 0)) {
                    continue;
            }
            
            flen = strlen(fd.cFileName);
        
            if ((strncmp(family->file_root,fd.cFileName,rlen) ==0))
            {
                
                if(!(fd.cFileName[flen-1]== 'A'))
                {
                    continue;
                }
                
                //We need to check on a few different scenarios
                
                //Let's see if this is just a single file.
                if(flen == rlen+1)
                {
                    count++;
                    found_A=1;
                    single_file = 1;
                    continue;
                }
                
                // If it was not a single file then check that the next 
                // letter is a digit
                if(!(fd.cFileName[rlen]>='0' && fd.cFileName[rlen]<='9'))
                {
                    continue;
                }
                
                //Check for how many processor files.           
		        for(start_check = rlen; start_check< strlen(fd.cFileName) && 
                                                             !found_A; start_check++)
                {
			        if(fd.cFileName[start_check] == 'A' && start_check == strlen(fd.cFileName)-1 )
                    {
				        
                        found_A=1;
			        }
			
			        if(fd.cFileName[start_check]>='0' && fd.cFileName[start_check]<='9')
                    {
                        numbers++;
			        }
		        }
		        if(found_A && numbers>0)
                {
      	            count++;
                    multifiles_found=1;
                    found_A = 0;
		        }
            }
            numbers =0;
            
        }
    }
    while (FindNextFileA(dirHandle, &fd));   //Continues a file search from a previous call to the FindFirstFile function

#endif
    
    if(multifiles_found && single_file)
    {
        count--;
    }else if(single_file && count>1)
    {
        //This should not happen
        count =1;
    }
    
    return count;
}

/*****************************************************************
 * TAG( mili_scandir ) PRIVATE
 *
 * Build a filtered, sorted list of file names from a directory.
 */
Return_value
mili_scandir( char *path, char *root, StringArray *p_sarr )
{
#ifndef _MSC_VER
   DIR *p_dir;
   struct dirent *p_dent;
   LONGLONG rlen;
   int index;
   Return_value rval;

   rlen = strlen( root );

   /*
    * Read the file names from the directory and load the StringArray
    * with those that begin with the root name.
    */
   p_dir = opendir( path );
   if ( p_dir == NULL )
   {
      return DIR_OPEN_FAILED;
   }

   while( (p_dent = readdir( p_dir )) )
   {
      if ( strncmp( p_dent->d_name, root, rlen ) == 0 )
      {
         rval = sa_add( p_sarr, p_dent->d_name, &index );
         if (rval != OK)
         {
            return rval;
         }
      }
   }
   if (closedir( p_dir ) != 0)
   {
      return DIR_CLOSE_FAILED;
   }
#else  //Windows
/* TCHAR is a Microsoft-specific typedef for either 
	 char (1-byte character: ANSI) or 
	 wchar_t (a wide character) (2-byte character: Unicode).  
   The preproc macro _UNICODE controls which one is used  
   Windows uses Unicode by default (after 2008) (preproc macro _UNICODE is set) */
/* Since we don't care about non-English languages and it is messy to convert types (var path is char),
   it is easiest to just use "char" and call ANSI functions (appended with "A") directly 
      TCHAR ==> char
      WIN32_FIND_DATA ==> WIN32_FIND_DATAA 
      FindFirstFile ==> FindFirstFileA
	  FindNextFile  ==> FindNextFileA
   See www.codeproject.com/Articles/76252/What-are-TCHAR-WCHAR-LPSTR-LPWSTR-LPCTSTR-etc for details */

//   WIN32_FIND_DATA fd; 
   WIN32_FIND_DATAA fd; 
   HANDLE dirHandle;
//   TCHAR szDir[MAX_PATH+1]; 
   char szDir[MAX_PATH+1];  
   LONGLONG rlen;
   int index;
   Return_value rval;
   char TrailStr[3];

   rlen = strlen( root );

// Load var szDir
   TrailStr[0]='\\';
   TrailStr[1]='*';
   TrailStr[2]='\0';
   strcpy(szDir,path);        //Begin string szDir with the path
   strcat(szDir,TrailStr);	  //Add "\*" to the end

   /*
    * Read the file names from the directory and load the StringArray
    * with those that begin with the root name.
    */
//Searches a directory for the first file matching szDir (Ex: szDir=.\*)
   dirHandle = FindFirstFileA(szDir, &fd); 
// printf ("FindFirstFile status (%d)\n", GetLastError());  //3: The system cannot find the path specified. //#Debug
// printf("\nmili_scandir: szDir=%s \n",szDir);  //#Debug

   if ( dirHandle == INVALID_HANDLE_VALUE )
   {
      printf("\nmili_scandir: FindFirstFile status (%d)\n", GetLastError());  
      printf("              szDir=%s \n",szDir);  
      return DIR_OPEN_FAILED;
   }

//
   do
   {
      if (strncmp(fd.cFileName, root, rlen) == 0)
      {
         rval = sa_add(p_sarr, fd.cFileName, &index);
         if (rval != OK)
         {
            return rval;
         }
      }
   }
   while (FindNextFileA(dirHandle, &fd));   //Continues a file search from a previous call to the FindFirstFile function

   FindClose(dirHandle);   //Close the search handle

#endif

   /* Sort the list of filenames */

   sa_sort( *p_sarr, 1 );

   return OK;
}              //end mili_scandir

/*****************************************************************
 * TAG( mc_countDomains ) Public
 *
 * Returns a count of number of Mili Domains (number of A files
 * for a root name.
 */
Return_value
mc_countDomains( char *path, char *root, int *domCount )
{
   StringArray sarr;
   int i, len=0, qty=0;
   char fname[256];
   Return_value rval;

   SANEW( sarr );
   if (sarr == NULL)
   {
      return ALLOC_FAILED;
   }

   rval = mili_scandir( path, root, &sarr );
   if (rval != OK)
   {
      return rval;
   }

   qty = SAQTY( sarr );

   if ( qty == 0 )
   {
      free( sarr );
      *domCount = 0;
      return OK;
   }

   for ( i = 0; i < qty; i++ )
   {
      strcpy( fname, SASTRING( sarr, i ) );

      len = strlen(fname);
      if (len < 4)
      {
         continue;
      }
      if ( !isdigit(fname[len-1]) )
      {
         if ( fname[len-1]=='_' &&       /* Ignore TI_A files */
               fname[len-2]=='I' &&
               fname[len-3]=='T' )
         {
            continue;
         }

         if ( isdigit(fname[len-2]) &&  /* Parallel case */
               isdigit(fname[len-3]) &&
               isdigit(fname[len-4]) )
         {
            (*domCount)++;
         }
      }
   }
   free( sarr );

   if ( *domCount==0 )
   {
      *domCount = 1;
   }

   return OK;
}

/*****************************************************************
 * TAG( swap_bytes ) PRIVATE
 *
 * Byte-swap qty fields each of size field_size from the source array
 * into the destination array.
 */
void
swap_bytes( int qty, long field_size,
            void *p_source, void *p_destination )
{
   int i;
   unsigned char *p_src, *p_dest, *bound;

   for ( bound = ((unsigned char *) p_source) + qty * field_size,
         p_src = p_source,
         p_dest = p_destination;
         p_src < bound;
         p_src += field_size,
         p_dest += field_size )
   {
      for (i = 0; i < field_size; ++i)
      {
         *(p_dest + i) = *(p_src + field_size - 1 - i);
      }
   }
}


/*****************************************************************
 * TAG( mc_print_error ) PUBLIC
 *
 * Print a diagnostic message associated with a mili function
 * return value.
 *
 * Keep current with typedef Return_value in mili_enum.h.
 */
void
mc_print_error( char *preamble, int rval )
{
   LONGLONG preamble_len;
   char *pre;
   char *many_scalars = "Attempt to define too many scalar state variables.";
   char *no_spx_grp = "Mesh object group not found.";
   char *too_many_types = "Mixed numeric types for object-ordered data.";
   char *not_applicable = "Attempted action not applicable.";
   char *list_mismatch = "Referenced state variable incompatible with list.";
   char *lock_open_fail = "Unable to open write-lock file.";
   char *lock_name_fail = "Unable to lock name in write-lock file.";
   char *bad_access = "Specified access type not permitted.";
   char *bad_bin_fmt = "File family has incompatible binary format.";
   char *bad_load_rd = "Read file failed during directory load.";
   char *not_in_fam = "Attempted open on file not in family.";
   char *bad_scheme = "Unknown state file partitioning scheme.";
   char *holy_mesh = "Cannot format state records for incomplete mesh.";
   char *not_string = "Found parameter is not string type.";
   char *malformed = "Malformed or inappropriate result subset specification.";
   char *unbound = "Named state variable not bound to subrecord.";
   char *bad_trunc = "State file truncate or unlink failed.";
   char *type_mismatch = "Numeric type mismatch for scalar param re-write.";
   char *len_mismatch = "String length mismatch for string param re-write.";
   char *cannot_mod = "Db object not open for modification.";
   char *dir_wrt_conflict =
      "Append access precluded by directory version mismatch.";
   char *hdr_wrt_conflict =
      "Append access precluded by header version mismatch.";
   char *zero_size = "Cannot pass a zero number for the size of object";   

   if ( preamble != NULL && *preamble != '\0' )
   {
      preamble_len = strlen( preamble ) + 3;
      pre = NEW_N( char, preamble_len, "Error preamble bufr" );
      sprintf( pre, "%s: ", preamble );
   }
   else
   {
      pre = NEW_N( char, 1, "Error preamble bufr" );
      pre[0] = '\0';
   }

   switch( rval )
   {
      case OK:
         /* Do nothing. */
         break;
      case (int)ZERO_SIZE_ERROR:
          fprintf( stderr, "%sMili - %s\n", pre, zero_size );
          break;
      case (int)TOO_MANY_SCALARS:
         fprintf( stderr, "%sMili - %s\n", pre, many_scalars );
         break;
      case (int)TOO_MANY_LIST_FIELDS:
         fprintf( stderr, "%sMili - Too many fields in list.\n", pre );
         break;
      case ENTRY_EXISTS:
         fprintf( stderr, "%sMili - Named entry already exists.\n", pre );
         break;
      case ENTRY_NOT_FOUND:
         fprintf( stderr, "%sMili - Named entry not found.\n", pre );
         break;
      case HASH_TABLE_OVERFLOW:
         fprintf( stderr, "%sMili - Hash table overflow.\n", pre );
         break;
      case UNKNOWN_HASH_ACTION:
         fprintf( stderr, "%sMili - Unknown hash action.\n", pre );
         break;
      case (int)BAD_FAMILY:
         fprintf( stderr, "%sMili - Invalid family identifier.\n", pre );
         break;
      case (int)UNKNOWN_OS:
         fprintf( stderr, "%sMili - Host type unknown.\n", pre );
         break;
      case (int)UNKNOWN_PRECISION:
         fprintf( stderr, "%sMili - Invalid family precision.\n", pre );
         break;
      case (int)OPEN_FAILED:
         fprintf( stderr, "%sMili - File open failed.\n", pre );
         break;
      case (int)FAMILY_NOT_FOUND:
         fprintf( stderr, "%sMili - Family file not found.\n", pre );
         break;
      case (int)NO_MESH:
         fprintf( stderr, "%sMili - Mesh not found.\n", pre );
         break;
      case (int)NO_CLASS:
         fprintf( stderr, "%sMili - Object class not found.\n", pre );
         break;
      case (int)NO_OBJECT_GROUP:
         fprintf( stderr, "%sMili - %s\n", pre, no_spx_grp );
         break;
      case (int)OBJECT_RANGE_OVERLAP:
         fprintf( stderr, "%sMili - Mesh object range overlap.\n", pre );
         break;
      case (int)OBJECT_RANGE_MERGE:
         fprintf( stderr, "%sMili - Mesh object ranges merged.\n", pre );
         break;
      case (int)OBJECT_RANGE_INSERT:
         fprintf( stderr, "%sMili - New mesh object range inserted.\n", pre );
         break;
      case (int)OBJECT_RANGE_COLLAPSE:
         fprintf( stderr, "%sMili - Mesh object ranges collapsed.\n", pre );
         break;
      case (int)OBJECT_NOT_IN_MESH:
         fprintf( stderr, "%sMili - Object not defined in mesh.\n", pre );
         break;
      case (int)NAME_CONFLICT:
         fprintf( stderr, "%sMili - Named mesh already exists.\n", pre );
         break;
      case (int)NO_MATCH:
         fprintf( stderr, "%sMili - Mesh name unknown.\n", pre );
         break;
      case (int)MESH_TYPE_CONFLICT:
         fprintf( stderr, "%sMili - Multiple mesh types illegal.\n", pre );
         break;
      case (int)SUPERCLASS_CONFLICT:
         fprintf( stderr, "%sMili - Class has other superclass.\n", pre );
         break;
      case (int)TOO_MANY_TYPES:
         fprintf( stderr, "%sMili - %s\n", pre, too_many_types );
         break;
      case (int)TOO_MANY_LABELS:
         fprintf( stderr, "%sMili - Too many labels created: current imit is %d\n", pre,  MAX_LABEL_CLASSES);
         break;
      case (int)UNKNOWN_FAMILY_FILE_TYPE:
         fprintf( stderr, "%sMili - Unknown file type.\n", pre );
         break;
      case (int)NOT_APPLICABLE:
         fprintf( stderr, "%sMili - %s\n", pre, not_applicable );
         break;
      case (int)INVALID_SUFFIX_WIDTH:
         fprintf( stderr, "%sMili - Suffix width too small.\n", pre );
         break;
      case (int)MEMBER_DELETE_FAILED:
         fprintf( stderr, "%sMili - Unable to delete family member.\n",
                  pre );
         break;
      case (int)UNKNOWN_DATA_TYPE:
         fprintf( stderr, "%sMili - Unknown data type.\n", pre );
         break;
      case (int)INVALID_DATA_TYPE:
         fprintf( stderr, "%sMili - Invalid data type.\n", pre );
         break;
      case (int)INVALID_NAME:
         fprintf( stderr, "%sMili - Invalid object name.\n", pre );
         break;
      case (int)ORGANIZATION_READ_ONLY:
         fprintf( stderr, "%sMili - Cannot modify organization.\n", pre );
         break;
      case (int)UNKNOWN_ORGANIZATION:
         fprintf( stderr, "%sMili - Invalid organization.\n", pre );
         break;
      case (int)MISMATCHED_LIST_FIELD:
         fprintf( stderr, "%sMili - %s\n", pre, list_mismatch );
         break;
      case (int)UNKNOWN_ELEM_FMT:
         fprintf( stderr, "%sMili - Unknown element format.\n", pre );
         break;
      case (int)INVALID_DIMENSIONALITY:
         fprintf( stderr, "%sMili - Invalid dimensionality.\n", pre );
         break;
      case (int)LOCK_OPEN_FAILED:
         fprintf( stderr, "%sMili - %s\n", pre, lock_open_fail );
         break;
      case (int)LOCK_EXISTS:
         fprintf( stderr, "%sMili - Write access denied.\n", pre );
         break;
      case (int)LOCK_NAME_FAILED:
         fprintf( stderr, "%sMili - %s\n", pre, lock_name_fail );
         break;
      case (int)BAD_ACCESS_TYPE:
         fprintf( stderr, "%sMili - %s\n", pre, bad_access );
         break;
      case (int)INCOMPATIBLE_BINARY_FORMAT:
         fprintf( stderr, "%sMili - %s\n", pre, bad_bin_fmt );
         break;
      case (int)TOO_LATE:
         fprintf( stderr, "%sMili - Suffix width already set.\n", pre );
         break;
      case (int)BAD_LOAD_READ:
         fprintf( stderr, "%sMili - %s\n", pre, bad_load_rd );
         break;
      case (int)BAD_NAME_READ:
         fprintf( stderr, "%sMili - Lock file read failed.\n", pre );
         break;
      case (int)SHORT_READ:
         fprintf( stderr, "%sMili - Incomplete read from file.\n", pre );
         break;
      case (int)SHORT_WRITE:
         fprintf( stderr, "%sMili - Incomplete write to file.\n", pre );
         break;
      case (int)NULL_POINTER:
         fprintf( stderr, "%sMili - Null pointer encountered.\n", pre );
         break;
      case (int)NOT_IN_FAMILY:
         fprintf( stderr, "%sMili - %s\n", pre, not_in_fam );
         break;
      case (int)UNKNOWN_PARTITION_SCHEME:
         fprintf( stderr, "%sMili - %s\n", pre, bad_scheme );
         break;
      case (int)SEEK_FAILED:
         fprintf( stderr, "%sMili - File seek failed.\n", pre );
         break;
      case (int)INCOMPLETE_MESH_DEF:
         fprintf( stderr, "%sMili - %s\n", pre, holy_mesh );
         break;
      case (int)NOT_STRING_TYPE:
         fprintf( stderr, "%sMili - %s\n", pre, not_string );
         break;
      case (int)INVALID_INDEX:
         fprintf( stderr, "%sMili - Invalid index.\n", pre );
         break;
      case (int)INVALID_SREC_INDEX:
         fprintf( stderr, "%sMili - Invalid state rec format id.\n", pre );
         break;
      case (int)INVALID_SUBREC_INDEX:
         fprintf( stderr, "%sMili - Invalid subrecord index.\n", pre );
         break;
      case (int)INVALID_CLASS_OPERATION:
         fprintf( stderr, "%sMili - Invalid class for operation.\n", pre );
         break;
      case (int)INVALID_STATE:
         fprintf( stderr, "%sMili - Invalid state number.\n", pre );
         break;
      case (int)INVALID_RESULT:
         fprintf( stderr, "%sMili - Invalid result name.\n", pre );
         break;
      case (int)INVALID_TIME:
         fprintf( stderr, "%sMili - Invalid time (out of bounds).\n", pre );
         break;
      case (int)MALFORMED_SUBSET:
         fprintf( stderr, "%sMili - %s\n", pre, malformed );
         break;
      case (int)UNBOUND_NAME:
         fprintf( stderr, "%sMili - %s\n", pre, unbound );
         break;
      case (int)INVALID_AGG_TYPE:
         fprintf( stderr, "%sMili - Invalid aggregate type.\n", pre );
         break;
      case (int)UNKNOWN_QUERY_TYPE:
         fprintf( stderr, "%sMili - Unknown query type.\n", pre );
         break;
      case (int)ALLOC_FAILED:
         fprintf( stderr, "%sMili - Memory allocation failed.\n", pre );
         break;
      case (int)FAMILY_TRUNCATION_FAILED:
         fprintf( stderr, "%sMili - %s\n", pre, bad_trunc );
         break;
      case (int)NUM_TYPE_MISMATCH:
         fprintf( stderr, "%sMili - %s\n", pre, type_mismatch );
         break;
      case (int)STR_LEN_MISMATCH:
         fprintf( stderr, "%sMili - %s\n", pre, len_mismatch );
         break;
      case (int)INVALID_FILE_NAME_INDEX:
         fprintf( stderr, "%sMili - Invalid file name index.\n", pre );
         break;
      case (int)INVALID_FILE_STATE_INDEX:
         fprintf( stderr, "%sMili - Invalid file state index.\n", pre );
         break;
      case (int)CANNOT_MODIFY_OBJ:
         fprintf( stderr, "%sMili - %s\n", pre, cannot_mod );
         break;
      case (int)DIR_OPEN_FAILED:
         fprintf( stderr, "%sMili - Unable to open directory.\n", pre );
         break;
      case (int)DIR_ZERO_COUNT:
         fprintf( stderr, "%sMili - Directory count is zero.\n", pre );
         break;
      case (int)NO_SVARS:
         fprintf( stderr, "%sMili - State variable qty is zero.\n", pre );
         break;
      case (int)DIRECTORY_WRITE_CONFLICT:
         fprintf( stderr, "%sMili - %s\n", pre, dir_wrt_conflict );
         break;
      case (int)HEADER_WRITE_CONFLICT:
         fprintf( stderr, "%sMili - %s\n", pre, hdr_wrt_conflict );
         break;
      case (int)BAD_CONTROL_STRING:
         fprintf( stderr, "%sMili - Family open control string format "
                  "error.\n", pre );
         break;
      case (int)MULTIPLE_HEADER_READ:
         fprintf( stderr, "%sMili - Attempt to re-read family header.\n",
                  pre );
         break;
      case (int)MAPPED_OLD_HEADER:
         fprintf( stderr, "%sMili - Mapped old header format into new "
                  "format.\n", pre );
         break;
      case (int)TAURUS_INCOMPLETE_GEOM:
         fprintf( stderr, "%sMili(Taurus) - Incomplete geometry data.\n",
                  pre );
         break;
      case (int)INVALID_CLASS_NAME:
         fprintf( stderr, "%sMili - Invalid class name specification.\n",
                  pre );
         break;
      case (int)IOS_NO_VIABLE_BUFFER:
         fprintf( stderr, "%sMili - No viable buffer found in IO Store.\n",
                  pre );
         break;
      case (int)IOS_ALLOC_FAILED:
         fprintf( stderr, "%sMili - Error allocating memory from IO "
                  "Store.\n", pre );
         break;
      case (int)IOS_STR_DUP_FAILED:
         fprintf( stderr, "%sMili - String copy into IO Store failed.\n",
                  pre );
         break;
      case (int)INVALID_SVAR_DATA:
         fprintf( stderr, "%sMili - Invalid state variable definition "
                  "data.\n", pre );
         break;
      case (int)INVALID_DIR_ENTRY_DATA:
         fprintf( stderr, "%sMili - Invalid directory entry data.\n", pre );
         break;
      case (int)TOO_MANY_MATERIALS:
         fprintf( stderr, "%sMili - Bad material number in wildcard.\n",
                  pre );
         break;
      case (int)NO_MATERIALS:
         fprintf( stderr, "%sMili - No valid material numbers found.\n",
                  pre );
         break;
      case (int)CORRUPTED_FILE:
         fprintf( stderr, "%sMili - Likely corrupted file encountered.\n",
                  pre );
         break;
      case (int)EPRINTF_FAILED:
         fprintf( stderr, "%sMili - Extended printf failed.\n", pre );
         break;
      case (int)EFPRINTF_FAILED:
         fprintf( stderr, "%sMili - Extended fprintf failed.\n", pre );
         break;
      case (int)ESPRINTF_FAILED:
         fprintf( stderr, "%sMili - Extended sprintf failed.\n", pre );
         break;
      case (int)IOS_NO_CHAR_BUF:
         fprintf( stderr, "%sMili - Null IO Store character buffer.\n",
                  pre );
         break;
      case (int)IOS_OUT_OF_SYNC:
         fprintf( stderr, "%sMili - Actual/expected data in IO store out "
                  "of sync.\n", pre );
         break;
      case (int)IOS_INVALID_DATA:
         fprintf( stderr, "%sMili - Invalid data found in IO store.\n",
                  pre );
         break;
      case (int)BAD_HOST_NAME:
         fprintf( stderr, "%sMili - Unable to get host name from system.\n",
                  pre );
         break;
      case (int)BAD_SYSTEM_INFO:
         fprintf( stderr, "%sMili - Unable to get system information.\n",
                  pre );
         break;
      case (int)BAD_USER_ID:
         fprintf( stderr, "%sMili - Unable to get user ID.\n", pre );
         break;
      case (int)UNABLE_TO_FLUSH_FILE:
         fprintf( stderr, "%sMili - Unable to flush file.\n", pre );
         break;
      case (int)UNABLE_TO_STAT_FILE:
         fprintf( stderr, "%sMili - Unable to get file status.\n", pre );
         break;
      case (int)UNABLE_TO_CLOSE_FILE:
         fprintf( stderr, "%sMili - Unable to close file.\n", pre );
         break;
      case (int)OVERSIZED_SARRAY:
         fprintf( stderr, "%sMili - StingArray buffer exceeds maximum "
                  "addressable length.\n", pre );
         break;
      case (int)FILE_FAMILY_NAME_MISMATCH:
         fprintf( stderr, "%sMili - Family root does not exist in file "
                  "name.\n", pre );
         break;
      case (int)DIR_CLOSE_FAILED:
         fprintf( stderr, "%sMili - Unable to close directory stream.\n",
                  pre );
         break;
      case (int)DATA_PRECISION_MISMATCH:
         fprintf( stderr, "%sMili - Writing to lower or reading from higher "
                  "precision database than hardware.\n", pre );
         break;
      case (int)INVALID_SVAR_AGG_TYPE:
         fprintf( stderr, "%sMili - Unknown state variable aggregate type.\n",
                  pre );
         break;
      case (int)SVAR_VEC_ARRAY_ORG_MISMATCH:
         fprintf( stderr, "%sMili - Cannot combine Vector Arrays with Result Order Subrecord.\n",
                  pre );
         break;
      case (int)INVALID_VISIT_JSON_FILE:
         fprintf( stderr, "%sMili - Cannot Unable to open  the json formatted .mili file.\n",
                  pre );
         break;
      case (int)NO_A_FILE_FOR_STATEMAP:
         fprintf( stderr, "%sMili - The A file to output state map info is missing.\n",
                  pre );
         break;
      default:
         fprintf( stderr, "%sUnknown return status.\n", pre );
         break;
   }

   free( pre );
}
