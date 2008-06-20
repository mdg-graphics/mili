/* $Id$ */

/* 
 * list.h - Definitions for list operations.
 * 
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Nov 15 1991
 *
 * 11/27/95 - Added NEXT and PREV macros; fixed bug in UNLINK
 *            for case where the unlinked element is first in
 *            the list ("prev" pointer for _new_ first element
 *            was not being reset to NULL).  (Doug Speck)
 *
 * 08/02/96 - Same fix as UNLINK (above) for DELETE.  (Doug Speck)
 *
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
 * TAG( INSERT_BEFORE )
 * 
 * Insert an element before another in a list.
 */
#define INSERT_BEFORE( new_elem, elem ) { if ( (elem)->prev != NULL ) {  \
                 (elem)->prev->next = (new_elem);  \
                 (new_elem)->prev = (elem)->prev; }  \
                 (new_elem)->next = (elem); (elem)->prev = (new_elem); }


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
                 else if ( (elem) != NULL ) {  (elem)->prev = (list);  \
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
 * 
 * DOES NOT HANDLE DYNAMICALLY ALLOCATED DATA IN THE ELEMENT.
 */
/* Original
#define DELETE( elem, list ) { if ( (list) == (elem) )  \
                                  (list) = (elem)->next;  \
                               else {  \
                                  (elem)->prev->next = (elem)->next;  \
                                  if ( (elem)->next != NULL )  \
                                     (elem)->next->prev = (elem)->prev; } \
                               free( elem ); } 
*/
#define DELETE( elem, list ) { if ( (list) == (elem) )  \
                                  (list) = (elem)->next;  \
                               else  \
                                  (elem)->prev->next = (elem)->next;  \
                               if ( (elem)->next != NULL )  \
                                  (elem)->next->prev = (elem)->prev;  \
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
