/* $Id$ */
/* 
 * list.h - Definitions for list operations.
 * 
 * 	Donald J. Dovey
 *	Lawrence Livermore National Laboratory
 *	Nov 15 1991
 *
 * 11/27/95 - Added NEXT and PREV macros; fixed bug in UNLINK
 *            for case where the unlinked element is first in
 *            the list ("prev" pointer for _new_ first element
 *            was not being reset to NULL).  (Doug Speck)
 *
 * Copyright 1991 (c) Regents of the University of California
 */

#ifndef LIST_H
#define LIST_H

/* 
 * These routines assume that the next and prev members of an element
 * are set to NULL by default.
 */

/*****************************************************************
 * TAG( NEXT )
 * 
 * Increment a list pointer to the next list member.
 */
#define NEXT( elem ) ( (elem) = (elem)->next )


/*****************************************************************
 * TAG( PREV )
 * 
 * Decrement a list pointer to the previous list member.
 */
#define PREV( elem ) ( (elem) = (elem)->prev )


/*****************************************************************
 * TAG( INIT_PTRS )
 * 
 * Set the next, prev pointers of an element to NULL.
 */
#define INIT_PTRS( elem ) { (elem)->next = NULL; (elem)->prev = NULL; }


/*****************************************************************
 * TAG( INSERT )
 * 
 * Insert an element at the beginning of a list.
 */
#define INSERT( elem, list ) { if ( (list) != NULL )  \
                                  { (elem)->next = (list);  \
                                    (list)->prev = (elem); }  \
                               (list) = (elem); }


/*****************************************************************
 * TAG( INSERT_AFTER )
 * 
 * Insert an element after another in a list.
 */
#define INSERT_AFTER( new_elem, elem ) { if ( (elem)->next != NULL ) {  \
                 (elem)->next->prev = (new_elem);  \
                 (new_elem)->next = (elem)->next; }  \
                 (new_elem)->prev = (elem); (elem)->next = (new_elem); }


/*****************************************************************
 * TAG( APPEND )
 * 
 * Append an element or list of elements to another.
 */
#define APPEND( elem, list ) { if ( (list) == NULL ) (list) = (elem);  \
                 else {  (elem)->prev = (list);  \
                         while( (elem)->prev->next != NULL )  \
                            (elem)->prev = (elem)->prev->next;  \
                         (elem)->prev->next = (elem);  } }


/*****************************************************************
 * TAG( UNLINK )
 * 
 * Unlink an element from a list.
 */
/* Original
#define UNLINK( elem, list ) { if ( (list) == (elem) )  \
                                  (list) = (elem)->next;  \
                               else {  \
                                  (elem)->prev->next = (elem)->next;  \
                                  if ( (elem)->next != NULL )  \
                                     (elem)->next->prev = (elem)->prev; } \
                               (elem)->next = NULL; (elem)->prev = NULL; }
*/
#define UNLINK( elem, list ) { if ( (list) == (elem) )  \
                                  (list) = (elem)->next;  \
                               else  \
                                  (elem)->prev->next = (elem)->next;  \
                               if ( (elem)->next != NULL )  \
                                  (elem)->next->prev = (elem)->prev;  \
                               (elem)->next = NULL; (elem)->prev = NULL; }


/*****************************************************************
 * TAG( DELETE )
 * 
 * Delete an element from a list.
 */
#define DELETE( elem, list ) { if ( (list) == (elem) )  \
                                  (list) = (elem)->next;  \
                               else {  \
                                  (elem)->prev->next = (elem)->next;  \
                                  if ( (elem)->next != NULL )  \
                                     (elem)->next->prev = (elem)->prev; } \
                               free( elem ); } 


/*****************************************************************
 * TAG( DELETE_LIST )
 * 
 * Delete all of the elements in a list.
 */
#define DELETE_LIST( list ) { if ( (list) != NULL ) {  \
                       while ( (list)->next != NULL )  \
                       { (list) = (list)->next; free( (list)->prev ); }  \
                       free( (list) ); (list) = NULL; } }


#endif /* LIST_H */
