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

 * Gahl ("Great, another hash library")  header file.
 * Originated as Mili's hash.h 1.4.
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

#ifndef GAHL_H
#define GAHL_H

/*****************************************************************
 * TAG( Htable_entry )
 *
 * Hash table entry.
 */
typedef struct _htable_entry
{
    struct _htable_entry *next;
    struct _htable_entry *prev;
    char *key;
    void *data;
} Htable_entry;

/*****************************************************************
 * TAG( Hash_table )
 */
typedef struct _hash_table
{
    Htable_entry **table;
    int qty_entries;
    int size;
} Hash_table;

/*****************************************************************
 * TAG( Hash_action )
 *
 * Enumerates the possible actions performed by htable_search().
 */
typedef enum
{
    ENTER_UNIQUE,
    ENTER_MERGE,
    ENTER_ALWAYS,
    FIND_ENTRY
} Hash_action;

/* Public routines. */
Hash_table *htable_create(                 /* Create a hash table */
                          int table_size); /* Requested number of buckets in table */

Return_value htable_search(                       /* Search to insert/find an entry in the table */
                           Hash_table *table,     /* Hash table to operate on */
                           char *key,             /* Hash key */
                           Hash_action op,        /* Operation to perform */
                           Htable_entry **entry); /* (output) Destination for entry address */

Return_value ti_htable_search(                       /* Search to insert/find an entry in the TI hash table */
                              Hash_table *table,     /* Hash table to operate on */
                              char *key,             /* Hash key */
                              Hash_action op,        /* Operation to perform */
                              Htable_entry **entry); /* (output) Destination for entry address */

void htable_del_entry(                      /* Delete an entry from a hash table */
                      Hash_table *table,    /* Table to operate on */
                      Htable_entry *entry); /* Entry to delete */

Return_value htable_get_data(                   /* Generate array of pointers to table entries */
                             Hash_table *table, /* Table to operate on */
                             void ***data_ptrs, /* (output) Destination for array address */
                             int *num_entries); /* (output) Number of entries created */

Return_value htable_add_entry_data(                   /* Add data to an entry in the table */
                                   Hash_table *table, /* Table to operate on */
                                   char *key,         /* Hash key */
                                   Hash_action op,    /* Search operation to perform */
                                   void *entry_data); /* Destination for array address */

Return_value ti_htable_add_entry_data(                   /* Add data to an entry in the table */
                                      Hash_table *table, /* Table to operate on */
                                      char *key,         /* Hash key */
                                      Hash_action op,    /* Search operation to perform */
                                      void *entry_data); /* Destination for array address */

void htable_delete(                           /* Delete a hash table */
                   Hash_table *table,         /* Table to operate on */
                   void (*user_func)(void *), /* Function to free user-data from entries */
                   int free_data);            /* Permit/preclude freeing of "data" field */

void htable_prt_error(                  /* Print diagnostic for search return code */
                      int return_code); /* Return code from htable_search() call */

void htable_dump(                           /* Dump summary of hash table contents */
                 Hash_table *table,         /* Table to operate on */
                 int compress,              /* If non-zero, don't output empty buckets */
                 void (*user_data_dump)()); /* Optional routine to unpack user-data */

void htable_delete_wildcard_list(                       /* Free up storage for a wildcard result list */
                                 int list_count,        /* Number of entries in the list */
                                 char **wildcard_list); /* The list of matching entries */

int htable_search_wildcard(                                 /* Search for a list of entries based on two input keys */
                           Hash_table *table,               /* Table to operate on */
                           int list_len,                    /* Current list length */
                           Bool_type allow_duplicates,      /* Duplicate entries allowed if TRUE */
                           char *key1, char *key2,          /* Search keys */
                           char *key3, char **return_list); /* List of matching entries */

int htable_key_search(Hash_table *table, int list_len, char *key1, char **return_list);
#endif
