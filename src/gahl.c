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
 
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "misc.h"
#include "mili_enum.h"
#include "mili_internal.h"
#include "list.h"
#include "gahl.h"

#ifndef NULL
#define NULL (0)
#endif


/* Private helper functions */

/*****************************************************************
 * TAG( add_shift_hash ) LOCAL
 *
 * Transform a character key into a hash table index by summation
 * over the characters' ASCII values but with each character left-
 * shifted one bit more than the preceding character.  Mod the
 * sum with the table size.
 */
static int
add_shift_hash( char *key, int table_size )
{
   char *pc;
   int sidx;
   register int sum;

   sum = (int) *key;
   sidx = 1;
   for ( pc = key + 1; *pc; pc++ )
   {
      sum += ((int) *pc ) << sidx;
      sidx = (sidx + 1) % 8;
   }

   return ( sum % table_size );
}

/*****************************************************************
 * TAG( remove_text_patten ) LOCAL
 *
 * Replaces a text pattern in a strings with spaces.
 */
static void
remove_text_pattern ( char *string, char *pattern )
{
   char *pattern_ptr;
   Bool_type pattern_found = TRUE;
   int i;
   int len;

   len = strlen( pattern );

   while ( pattern_found )
   {
      pattern_ptr = strstr( string, pattern );
      if ( pattern_ptr )
      {
         for (i=0; i<len; i++)
         {
            pattern_ptr[i] = ' ';
         }
      }
      else
      {
         pattern_found = FALSE;
      }
   }
}

/* END Private helper functions */


/*****************************************************************
 * TAG( htable_create )
 *
 * Create a hash table.
 */
Hash_table *
htable_create( int table_size )
{
   Hash_table *pht = NEW( Hash_table, "Hash table struct" );
   if (pht != NULL)
   {
      pht->table = NEW_N( Htable_entry *, table_size,
                          "Hash table bucket array" );
      if (pht->table == NULL)
      {
         free(pht);
         pht = NULL;
      }
      else
      {
         pht->size = table_size;
      }
   }
   return pht;
}


/*****************************************************************
 * TAG( htable_search )
 *
 * Search a hash table for a key and perform the specified action.
 */
Return_value
htable_search( Hash_table *table, char *key, Hash_action op,
               Htable_entry **entry )
{
   
   int bucket;
   Htable_entry *phte = NULL, 
                *phte2 = NULL;
   Return_value rval =OK;

   if( table == NULL || key == NULL){
       return NULL_POINTER;
   }

   bucket = add_shift_hash( key, table->size );

   if( bucket > table->size )
   {
      /* Return Error. */
      *entry = NULL;
      return HASH_TABLE_OVERFLOW;
   }

   /**/
   /* Insertion sort on keys to improve average performance? */
   /* strcmp() would allow...                                */

   for ( phte = table->table[bucket]; phte != NULL; phte = phte->next )
   {
      if ( strcmp( phte->key, key ) == 0 )
      {
         break;
      }
   }

   rval = OK;

   switch ( op )
   {
      case ENTER_UNIQUE:
         
         if ( phte == NULL )
         {
            phte = NEW( Htable_entry, "Hash table entry" );
            if (phte == NULL)
            {
               rval = ALLOC_FAILED;
            }
            else
            {
               str_dup( &phte->key, key );
               if (phte->key == NULL)
               {
                  free(phte);
                  phte = NULL;
                  rval = ALLOC_FAILED;
               }
               else
               {
                  INSERT_ELEM( phte, table->table[bucket] );
                  table->qty_entries++;
               }
            }
            *entry = phte;
         }
         else
         {
            *entry = NULL;
            rval = ENTRY_EXISTS;
         }
         break;

      case ENTER_MERGE:
         if ( phte == NULL )
         {
            phte = NEW( Htable_entry, "Hash table entry" );
            if (phte == NULL)
            {
               rval = ALLOC_FAILED;
            }
            else
            {
               str_dup( &phte->key, key );
               if (phte->key == NULL)
               {
                  free(phte);
                  phte = NULL;
                  rval = ALLOC_FAILED;
               }
               else
               {
                  INSERT_ELEM( phte, table->table[bucket] );
                  table->qty_entries++;
               }
            }
         }
         else
         {
            rval =  ENTRY_EXISTS;
         }
         *entry = phte;
         break;

      case ENTER_ALWAYS:
         phte2 = NEW( Htable_entry, "Hash table entry" );
         
         if (phte2 == NULL)
         {
            rval = ALLOC_FAILED;
         }
         else
         {
            str_dup( &phte2->key, key );
            if (phte2->key == NULL)
            {
               free(phte2);
               phte2 = NULL;
               rval = ALLOC_FAILED;
            }
            else
            {
               if ( phte == NULL )
               {
                  INSERT_ELEM( phte2, table->table[bucket] );
               }
               else
               {
                  INSERT_BEFORE( phte2, phte,table->table[bucket] );
                  
               }
               table->qty_entries++;
            }
         }
         *entry = phte2;
         break;

      case FIND_ENTRY:
         if ( phte == NULL )
         {
            rval = ENTRY_NOT_FOUND;
         }
         *entry = phte;
         break;

      default:
         rval = UNKNOWN_HASH_ACTION;
         *entry = NULL;
         break;
   }

   return rval;
}


/*****************************************************************
 * TAG( ti_htable_search )
 *
 * Search a TI hash table for a key and perform the specified action.
 * TI table entries have metadata stored at the end of the name,
 * so some of the metadata fields (these fields start after the ++
 * tag in the name should not be counted in the search.
 *
 */
Return_value
ti_htable_search( Hash_table *table, char *key, Hash_action op,
                  Htable_entry **entry )
{
   int bucket;
   Htable_entry *phte, *phte2;
   int i;
   Return_value rval;

   char htable_name[256], temp_key[512];
   int  len;

   strcpy( temp_key, key );
   len = strlen( temp_key );

   for (i=0; i<len; i++)
   {
      if ( i<len-4 )
      {
         if ( temp_key[i]=='/' && temp_key[i+1]=='+' &&
               temp_key[i+2]=='+')
         {
            temp_key[i] = '\0';
            break;
         }
      }
   }

   bucket = add_shift_hash( key, table->size );

   if( bucket > table->size )
   {
      *entry = NULL;
      return HASH_TABLE_OVERFLOW;
      /* This should never occur - but return error if it does. */
   }


   strcpy (temp_key, key );

   /* Remove all Boolean key fields (TRUE-FALSE) */

   remove_text_pattern ( temp_key, "TRUE"  );
   remove_text_pattern ( temp_key, "FALSE" );

   /**/
   /* Insertion sort on keys to improve average performance? */
   /* strcmp() would allow...                                */


   for ( phte = table->table[bucket]; phte != NULL; phte = phte->next )
   {
      strcpy( htable_name, phte->key );

      remove_text_pattern ( htable_name, "TRUE"  );
      remove_text_pattern ( htable_name, "FALSE" );

      len = strlen( htable_name );

      if ( strcmp( htable_name, temp_key ) == 0 )
      {
         break;
      }
   }

   rval = OK;

   switch ( op )
   {
      case ENTER_UNIQUE:
         if ( phte == NULL )
         {
            phte = NEW( Htable_entry, "Hash table entry" );
            if (phte == NULL)
            {
               rval = ALLOC_FAILED;
            }
            else
            {
               str_dup( &phte->key, key );
               if (phte->key == NULL)
               {
                  free(phte);
                  phte = NULL;
                  rval = ALLOC_FAILED;
               }
               else
               {
                  INSERT_ELEM( phte, table->table[bucket] );
                  table->qty_entries++;
               }
            }
            *entry = phte;
         }
         else
         {
            *entry = NULL;
            rval = ENTRY_EXISTS;
         }
         break;

      case ENTER_MERGE:
         if ( phte == NULL )
         {
            phte = NEW( Htable_entry, "Hash table entry" );
            if (phte == NULL)
            {
               rval = ALLOC_FAILED;
            }
            else
            {
               str_dup( &phte->key, key );
               if (phte->key == NULL)
               {
                  free(phte);
                  phte = NULL;
                  rval = ALLOC_FAILED;
               }
               else
               {
                  INSERT_ELEM( phte, table->table[bucket] );
                  table->qty_entries++;
               }
            }
         }
         else
         {
            rval = ENTRY_EXISTS;
         }
         *entry = phte;
         break;

      case ENTER_ALWAYS:
         phte2 = NEW( Htable_entry, "Hash table entry" );
         if (phte2 == NULL)
         {
            rval = ALLOC_FAILED;
         }
         else
         {
            str_dup( &phte2->key, key );
            if (phte2->key == NULL)
            {
               free(phte2);
               phte2 = NULL;
               rval = ALLOC_FAILED;
            }
            else
            {
               if ( phte == NULL )
               {
                  INSERT_ELEM( phte2, table->table[bucket] );
               }
               else
               {
                  INSERT_BEFORE( phte2, phte ,table->table[bucket]);
                  
               }
               table->qty_entries++;
            }
         }
         *entry = phte2;
         break;

      case FIND_ENTRY:
         if ( phte == NULL )
         {
            rval = ENTRY_NOT_FOUND;
         }
         *entry = phte;
         break;

      default:
         rval = UNKNOWN_HASH_ACTION;
         *entry = NULL;
         break;
   }

   return rval;
}


/*****************************************************************
 * TAG( htable_del_entry )
 *
 * Delete an entry from a hash table.  Caller is responsible for
 * managing memory referenced by the data field prior to this call.
 */
void
htable_del_entry( Hash_table *table, Htable_entry *entry )
{
   int bucket;

   if(!table || !entry)
   {
      return;
   } 

   bucket = add_shift_hash( entry->key, table->size );
   if( bucket > table->size )
   {
      /* This should never occur - but if it does return an error. */
   }

   free( entry->key );
   DELETE_ELEM( entry, table->table[bucket] );
   table->qty_entries--;
}


/*****************************************************************
 * TAG( htable_delete )
 *
 * Delete a hash table.  If data referenced by entry does itself
 * reference dynamically allocated memory, caller should provide
 * a user function that handles deallocation of both the data and
 * the memory it references.  This should be a void function that
 * takes a single argument, a pointer to void.
 */
void
htable_delete( Hash_table *table, void (*user_func)(void *), int free_data )
{
   int i;
   Htable_entry *phte, *phte2;
   int tsize;

   if(!table)
   {
      return;
   } 

   tsize = table->size;

   for ( i = 0; i < tsize; i++ )
   {
      phte = table->table[i];
      if ( phte != NULL )
      {
         do
         {
            phte2 = phte->next;
            free( phte->key );
            if ( user_func != NULL )
            {
               (*user_func)( phte->data );
            }
            else if ( free_data && phte->data != NULL )
            {
               free( phte->data );
            }
            free( phte );
            phte = phte2;
         }
         while ( phte != NULL );
      }
   }
   free( table->table );
   free( table );
}


/*****************************************************************
 * TAG( htable_add_entry_data )
 *
 * Add the supplied data to the entry associated with key.
 */
Return_value
htable_add_entry_data( Hash_table *table, char *key,
                       Hash_action op, void *entry_data )
{
   Htable_entry *phte;
   Return_value rval = OK;

   rval = htable_search(table, key, op, &phte);
   if (phte != NULL)
   {
      if (op == ENTER_MERGE && phte->data != NULL)
      {
         free(phte->data);
      }
      phte->data = entry_data;
   }
   return rval;
}


/*****************************************************************
 * TAG( ti_htable_add_entry_data )
 *
 * Add the supplied data to the entry associated with key.
 */
Return_value
ti_htable_add_entry_data( Hash_table *table, char *key,
                          Hash_action op, void *entry_data )
{
   Htable_entry *phte;
   Return_value rval;

   rval = ti_htable_search(table, key, op, &phte);
   if (phte != NULL)
   {
      if (op == ENTER_MERGE && phte->data != NULL)
      {
         free(phte->data);
      }
      phte->data = entry_data;
   }
   return rval;
}


/*****************************************************************
 * TAG( htable_get_data )
 *
 * Create an array of pointers to the data blocks in a hash table.
 * Return the number of entries in the table.
 *
 * Dynamically allocated space must be freed by caller.
 */
Return_value
htable_get_data( Hash_table *htable, void ***data_ptrs, int *num_entries )
{
   int i, j;
   void **pv;
   Htable_entry *phte;
   int tsize;

   /* Pass through the table and collect the data pointers. */
   pv = NEW_N( void *, htable->qty_entries, "Htable entry data ptrs" );
   if (htable->qty_entries > 0 && pv == NULL)
   {
      *num_entries = 0;
      return ALLOC_FAILED;
   }
   j = 0;
   tsize = htable->size;
   for ( i = 0; i < tsize; i++ )
   {
      phte = htable->table[i];
      while ( phte != NULL )
      {
         pv[j++] = phte->data;
         phte = phte->next;
      }
   }
   *data_ptrs = pv;
   *num_entries = htable->qty_entries;

   return OK;
}


/*****************************************************************
 * TAG( htable_dump )
 *
 * Unpack and dump the entries in a hash table.
 */
void
htable_dump( Hash_table *table, int compress, void (*user_data_dump)() )
{
   Htable_entry **hta;
   int empties;
   int tsize;
   int i;
   Htable_entry *phte;

   tsize = table->size;

   printf( "Table size = %d; quantity entries = %d.\n", table->size,
           table->qty_entries );

   empties = 0;

   for ( i = 0, hta = table->table; i < tsize; i++ )
   {
      if ( ( empties == 0 || !compress) ||
            ( compress && empties > 0 && hta[i] != NULL ) )
      {
         printf( "\n%4d", i + 1 );
      }

      if ( hta[i] != NULL )
      {
         empties = 0;

         printf( "  key=\"%s\" data: ", hta[i]->key );
         (*user_data_dump)( hta[i]->data );

         phte = hta[i]->next;
         if ( phte != NULL )
         {
            do
            {
               printf( "\n      key=\"%s\" data: ", phte->key );
               (*user_data_dump)( phte->data );
               phte = phte->next;
            }
            while ( phte != NULL );
         }
      }
      else if ( compress && empties == 1 )
      {
         printf( "..." );
         empties++;
      }
      else
      {
         empties++;
      }
   }
   printf( "\n" );
}


/*****************************************************************
 * TAG( htable_prt_error )
 *
 * Print diagnostic associated with a return code from htable_search().
 */
void
htable_prt_error( int return_code )
{
   switch( (Return_value) return_code )
   {
      case OK:
         /* Do nothing. */
         break;
      case (Return_value)ENTRY_EXISTS:
         fprintf( stderr, "Named entry already exists.\n" );
         break;
      case (Return_value)ENTRY_NOT_FOUND:
         fprintf( stderr, "Named entry not found.\n" );
         break;
      case (Return_value)UNKNOWN_HASH_ACTION:
         fprintf( stderr, "Unknown hash action.\n" );
         break;
      default:
         fprintf( stderr, "Unknown return code.\n" );
         break;
   }
}

/*****************************************************************
 * TAG( htable_delete_wildcard_list )
 *
 * Free space associated with a list of names generated from
 * a wildcard search.
 */
void
htable_delete_wildcard_list( int list_count, char **wildcard_list )
{
   int i;

   for ( i=0; i<list_count; i++)
   {
      free( wildcard_list[i] );
		wildcard_list[i] = NULL;
   }
}


/*****************************************************************
 * TAG( htable_search_wildcard )
 *
 * Search a hash table for a key pattern with up to
 * to input key1 ( key1 && key2 ). If key1=="*"
 * all hash entries are returned.
 */
int
htable_search_wildcard( Hash_table *table, int list_len,
                        Bool_type allow_duplicates,
                        char *key1, char *key2, char *key3,
                        char **return_list )
{
   Htable_entry *phte;

   int i, j;
   int return_list_count = list_len;
   int tsize;

   Bool_type dup_match=FALSE, 
             key1_match = TRUE,
             key2_match = TRUE, 
             key3_match = TRUE;

   if ( table==NULL )
   {
      return (0);
   }

   tsize = table->size;

   for ( i = 0; i < tsize; i++ )
   {
      phte = table->table[i];
      if ( phte != NULL )
      {
         do
         {
            key1_match = FALSE;
            key2_match = FALSE;
            key3_match = FALSE;

            if ( strstr( phte->key, key1 ) != NULL )
            {
               key1_match = TRUE;
            }

            if ( strlen(key2) > 0 )
            {
               if ( strstr( phte->key, key2 ) != NULL )
               {
                  key2_match = TRUE;
               }
            }

            if ( strlen(key3) > 0 )
            {
               if ( strstr( phte->key, key3 ) != NULL )
               {
                  key3_match = TRUE;
               }
            }

            /* If key2 NULL then an automatic match */
            if ( strlen(key2) == 0 || strcmp( "NULL", key2 ) == 0 )
            {
               key2_match = TRUE;
            }

            /* If key3 NULL then an automatic match */
            if ( strlen(key3) == 0 || strcmp( "NULL", key3 ) == 0 )
            {
               key3_match = TRUE;
            }

            /* Get everything !!! */
            if ( strcmp( "*", key1 ) == 0 || strcmp( "NULL", key1 ) == 0 )
            {
               key1_match = TRUE;
               key2_match = TRUE;
               key3_match = TRUE;
            }

            if ( !return_list && key1_match && key2_match &&  key3_match )
            {
               return_list_count++; /* Just return a count */
            }
            else
            {
               if ( key1_match && key2_match &&  key3_match )
               {
                  dup_match = FALSE;

                  if ( !allow_duplicates )
                  {
                     for ( j=0; j<return_list_count; j++)
                     {
                        if ( !strcmp(return_list[j], phte->key ))
                        {
                           dup_match = TRUE;
                        }
                     }
                  }

                  if ( !dup_match || allow_duplicates )
                  {
                     str_dup( &return_list[return_list_count],
                              phte->key );
                     if (return_list[return_list_count] == NULL)
                     {
                        /* It would be better if the function
                         * returned a Return_value and the returned
                         * count was in the arg list.  But that is
                         * a big change as this thing is used
                         * pervasively thoughout the code.  At least
                         * this will prevent a core dump.
                         */
                        return 0;
                     }
                     ++return_list_count;
                  }
               }
            }

            phte = phte->next;
         }
         while ( phte != NULL );
      }
   }

   if ( return_list_count <0 )
   {
      return_list_count = 0;
   }

   return return_list_count;
}

/**
*  TAG( getMatchType )
*  This function will return the match type that 
*  we need to try and match.
*  
*  types are absolute (no star)= 1
*  begining match ( something*)= 2
*  end match (*something)      = 3
*  middle match (*something*)  = 4
*  inner match  (some*thing)   = 5
*/
static int
getMatchType(char* input)
{
   int end_target;
   int i;  // counter
   int found=0; //used when searching for inner "*"
   
   //Check to make sure input is valid
   if(!input 
      || strlen(input) == 0  
      || strcmp(input, "NULL")==0) 
   {
      return -1;
   }
   
   end_target = strlen(input)-1;
   
   if(input[0] != '*' && input[end_target] != '*')
   {
      // Check if "*" appears in middle of string
      for(i = 1; i < end_target; i++)
      {
         if(input[i] == '*')
         {
            //if already 1 inner *, return -1
            if(found)
               return -1;
            found = 1;
         }
      }
        
      if(found)
         return 5;
      else
         return 1;
   }
   else if(input[0] == '*'  && input[end_target] == '*')
   {
      return 4;
   }
   else if(input[0] == '*')
   {
      return 3;
   }
   else
   {
      return 2;
   }
} 


/*****************************************************************
 * TAG( str_begins_with )
 *
 * This function will check the beginning of str for substr
 */
Bool_type
str_begins_with(char* str, char* substr){
   Bool_type result = FALSE;
   if(strncmp(str,substr,strlen(substr)) == 0){
      result = TRUE;
   }
   return result;
}


/*****************************************************************
 * TAG( str_ends_with )
 *
 * This function will check the beginning of str for substr
 */
Bool_type
str_ends_with(char* str, char* substr){
   Bool_type result = FALSE;
   if(strcmp(str + strlen(str) - strlen(substr),substr) == 0){
      result = TRUE;      
   }
   return result;
}


static int
match(char* target, char* key, int match_type)
{
   // types are absolute (no star)= 1
   // begining match ( something*)= 2
   // end match (*something)      = 3
   // middle match (*something*)  = 4
   // inner match  (some*thing)   = 5
   int target_length;
   int index_to_move;
   int i;
   char temp[128];
   char temp2[128];
   temp[0] = '\0';
   
   
   switch(match_type){
   
      case 1:
         if(strcmp(target,key) == 0)
         {
            return 1;
         }else
         {
            return 0;
         }
         break;
      case 2:
         strcpy(temp2,target);
         strcpy(temp,key);
         temp2[strlen(temp)-1]='\0';
         temp[strlen(temp)-1]='\0';
         
         if(strcmp(temp2,temp) == 0)
         {
            return 1;
         }else
         {
            return 0;
         }
         break;
      case 3:
         strcpy(temp, key+1);
         strcpy(temp2, target);
         target_length = strlen(temp2);
         index_to_move = target_length - strlen(temp);
         
         if (strcmp( temp2+index_to_move, temp) == 0)
         {
             return 1;
         }else
         {
             return 0;
         } 
         break;
      case 4:
         strncpy(temp, key+1, strlen(key)-1);
         temp[strlen(temp)-1]='\0';
         if(strstr(target,temp))
         {
            return 1;
         }else
         {
            return 0;
         }
         break;
      case 5:
         // Find index of * in key
         for (i = 0; i < strlen(key); i++ )
         {
            if(key[i] == '*')
            {
                index_to_move = i;
            }
         }
         //copy key from 0 up to i into temp
         //copy key from i+1 to end into temp2
         strncpy(temp,key,index_to_move);
         strncpy(temp2,key+index_to_move+1, strlen(key)-index_to_move-1);
         temp[index_to_move] = '\0';
         temp2[strlen(key)-index_to_move-1] = '\0';

         if(str_begins_with(target,temp) && str_ends_with(target,temp2))
         {
            return 1;
         }
         else
         {
            return 0;
         }
         break;
      default:
         return 0;
   } 
}
/*****************************************************************
 * TAG( htable_key_search )
 *
 * Search a hash table for a key pattern with up to
 * to input key1.  This search allows for search similar
 * ls in that a '*' character has the following effects
 * 
 * "xxx*"  matches anything starting with an xxx
 * "*xxx"  matches anything ending with xxx
 * "*xxx*" matches anything containing xxx
 * "xxx"   matches exactly xxx thus strcmp() must return zero.
 *
 * If return_list is NULL then the function returns the count of the matches.
 */
int
htable_key_search( Hash_table *table, int list_len,
                        char *key1, char **return_list )
{
   Htable_entry *phte;

   int i;
   int return_list_count = list_len;
   int tsize;

   Bool_type key1_match = TRUE;
             
   int key1_match_type;
       
   if ( table==NULL )
   {
      return (0);
   }
   
   key1_match_type = getMatchType(key1);
   
   if(key1_match_type<1)
   {
      return (0);
   }
   
   tsize = table->size;

   for ( i = 0; i < tsize; i++ )
   {
      phte = table->table[i];
      if ( phte != NULL )
      {
         do
         {
            // types are absolute (no star)= 1
            // begining match ( something*)= 2
            // end match (*something)      = 3
            // middle match (*something*)  = 4
            // inner match (some*thing)    = 5
            
            key1_match = FALSE;

            key1_match = match(phte->key, key1, key1_match_type);
            
            if ( !return_list && key1_match )
            {
               return_list_count++; /* Just return a count */
            }
            else
            {
               if ( key1_match)
               {
                  str_dup( &return_list[return_list_count],
                           phte->key );
                  if (return_list[return_list_count] == NULL)
                  {
                     /* It would be better if the function
                      * returned a Return_value and the returned
                      * count was in the arg list.  But that is
                      * a big change as this thing is used
                      * pervasively thoughout the code.  At least
                      * this will prevent a core dump.
                      */
                      return return_list_count;
                  }
                  ++return_list_count;                  
               }
            }

            phte = phte->next;
         }
         while ( phte != NULL );
      }
   }

   if ( return_list_count <0 )
   {
      return_list_count = 0;
   }

   return return_list_count;
}


