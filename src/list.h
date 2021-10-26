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
 
 * list.h - Definitions for list operations.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Nov 15 1991
 *
 ************************************************************************
 * Modifications:
 * 11/27/95 - Added NEXT and PREV macros; fixed bug in UNLINK
 *            for case where the unlinked element is first in
 *            the list ("prev" pointer for _new_ first element
 *            was not being reset to NULL).  (Doug Speck)
 *
 * 08/02/96 - Same fix as UNLINK (above) for DELETE.  (Doug Speck)
 *
 *  I. R. Corey - March 20, 2009: Added support for WIN32 for use with
 *                with Visit.
 *                See SCR #587.
 ************************************************************************
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
#define INSERT_ELEM( elem, list ) { if ( (list) != NULL )  \
                                  { (elem)->next = (list);  \
                                    (list)->prev = (elem); }  \
                               (list) = (elem); }


/*****************************************************************
 * TAG( INSERT_BEFORE )
 *
 * Insert an element before another in a list.
 */
#define INSERT_BEFORE( new_elem, elem,list ) { if ( (elem)->prev != NULL ) {  \
                 (elem)->prev->next = (new_elem);  \
                 (new_elem)->prev = (elem)->prev; }  else {\
                 (list) = (new_elem); } \
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
#define DELETE_ELEM( elem, list ) { if ( (list) == (elem) )  \
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
                       { (list) = (list)->next; free( (list)->prev ); (list)->prev = NULL;}  \
                       free( (list) ); (list) = NULL; } }


#endif /* LIST_H */
