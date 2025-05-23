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

 * Routines for managing the TI file directory, which is written at
 * the end of a file.
 *
 */

/*
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - December 20, 2007: Increased name allocation to 16000.
 *                Fixed a bug with the reallocation of MemStore.
 *                SCR#: 513
 *

 ************************************************************************
 */

#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mili_internal.h"

/* Prototypes */
static Bool_type valid_ti_dir_entry_data(Dir_entry_type etype, int string_qty, char **strings);

/*****************************************************************
 * TAG( valid_ti_dir_entry_data ) LOCAL
 *
 * Validate the data for a directory entry.
 *
 * Should add etype-specific tests of modifier fields...
 */
static Bool_type valid_ti_dir_entry_data(Dir_entry_type etype, int string_qty, char **strings)
{
    int i;

    if ( etype < NODES || etype >= QTY_DIR_ENTRY_TYPES )
    {
        return FALSE;
    }

    for ( i = 0; i < string_qty; i++ )
    {
        if ( strings[i] == NULL || strlen(strings[i]) == 0 )
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*****************************************************************
 * TAG( add_ti_dir_entry ) PRIVATE
 *
 * Add an entry into the directory.
 */
Return_value add_ti_dir_entry(Mili_family *fam, Dir_entry_type etype, int modifier1, int modifier2, int string_qty,
                              char **strings, LONGLONG offset, LONGLONG length)
{
    LONGLONG *entry;
    File_dir *p_fd;
    int qty_ent, qty_nam;
    int i;
    int len;

    /* Check the input data before doing anything. */
    if ( !valid_ti_dir_entry_data(etype, string_qty, strings) )
    {
        return INVALID_DIR_ENTRY_DATA;
    }

    /*
     * Get the File_dir that heads the array of Dir_entry's for the
     * current non-state file.
     */

    p_fd = fam->ti_directory + fam->ti_file_count - 1;

    /* Grow the array of Dir_entry's for the current non-state file. */

    qty_ent = p_fd->qty_entries;
    qty_nam = p_fd->qty_names;

    p_fd->dir_entries = RENEW_N(Dir_entry, p_fd->dir_entries, qty_ent, 1, "Addl file dir entry");
    if ( p_fd->dir_entries == NULL )
    {
        return ALLOC_FAILED;
    }

    if ( string_qty > 0 )
    {
        p_fd->names = RENEW_N(char *, p_fd->names, qty_nam, string_qty, "Addl name ptr");
        if ( string_qty > 0 && p_fd->names == NULL )
        {
            return ALLOC_FAILED;
        }
    }

    entry = p_fd->dir_entries[qty_ent];
    p_fd->qty_entries++;
    p_fd->qty_names += string_qty;

    /* Load the new Dir_entry. */
    entry[TYPE_IDX] = etype;
    entry[MODIFIER1_IDX] = modifier1;
    entry[MODIFIER2_IDX] = modifier2;
    entry[STRING_QTY_IDX] = string_qty;
    entry[OFFSET_IDX] = offset;
    entry[LENGTH_IDX] = length;

    if ( string_qty > 0 )
    {
        if ( p_fd->name_data == NULL )
        {
            p_fd->name_data = ios_create(M_STRING);
            if ( p_fd->name_data == NULL )
            {
                return IOS_ALLOC_FAILED;
            }
        }

        for ( i = 0; i < string_qty; i++ )
        {
            len = ios_str_dup(p_fd->names + qty_nam + i, strings[i], p_fd->name_data);
            if ( len == 0 )
            {
                return IOS_STR_DUP_FAILED;
            }
        }
    }

    return OK;
}

/*****************************************************************
 * TAG( commit_ti_dir ) PRIVATE
 *
 * Write a non-state file's directory to the file and delete its
 * memory.
 *
 * Assumes file is opened and positioned properly.
 */
Return_value commit_ti_dir(Mili_family *fam)
{
    LONGLONG words;
    int dir_header[QTY_DIR_HEADER_FIELDS];
    File_dir *p_fd;
    LONGLONG char_qty, round_qty;
    void *pmem;
    int num_written;
    Return_value rval;

    p_fd = fam->ti_directory + fam->ti_file_count - 1;

    if ( p_fd->qty_entries == 0 )
    {
        return OK;
    }

    /* Get amount of character data for output. Round up if necessary. */
    char_qty = ios_get_fresh(p_fd->name_data);
    round_qty = ROUND_UP_INT(char_qty, 4);
    if ( round_qty != char_qty )
    {
        pmem = ios_alloc(round_qty - char_qty, p_fd->name_data);
        if ( pmem == NULL )
        {
            return IOS_ALLOC_FAILED;
        }
    }

    p_fd->commit_count = fam->ti_commit_count + 1;

    /*
     * Store in the header the quantity of bytes occupied by object names.
     *
     * Must use the rounded-up quantity because this field must provide the
     * negative offset from the start of directory integer data to the
     * __beginning__ of the preceding object name data.
     */
    dir_header[NAMES_LEN_IDX] = (int)round_qty;
    /* Copy commit count into buffer. */
    dir_header[COMMIT_COUNT_IDX] = p_fd->commit_count;
    /* Copy entry count into buffer. */
    dir_header[QTY_ENTRIES_IDX] = p_fd->qty_entries;

    /*
     * Write out the names, the entries, then the header, so that the header
     * will always be at the end of the file.
     */
    rval = ios_output(fam, fam->ti_cur_file, p_fd->name_data, &num_written);
    if ( rval != OK )
    {
        return rval;
    }

    words = QTY_ENTRY_FIELDS * p_fd->qty_entries;
    num_written = fam->write_funcs[M_INT](fam->ti_cur_file, p_fd->dir_entries, words);
    if ( num_written != words )
    {
        return SHORT_WRITE;
    }
    if ( fam->char_header[DIR_VERSION_IDX] == 1 )
    {
        num_written = fam->write_funcs[M_INT](fam->ti_cur_file, dir_header, (LONGLONG)QTY_DIR_HEADER_FIELDS - 1);
        if ( num_written != QTY_DIR_HEADER_FIELDS - 1 )
        {
            return SHORT_WRITE;
        }
        fam->ti_next_free_byte += (words + QTY_DIR_HEADER_FIELDS - 1) * EXT_SIZE(fam, M_INT);
    }
    else
    {
        num_written = fam->write_funcs[M_INT](fam->ti_cur_file, dir_header, (LONGLONG)QTY_DIR_HEADER_FIELDS);
        if ( num_written != QTY_DIR_HEADER_FIELDS )
        {
            return SHORT_WRITE;
        }
        fam->ti_next_free_byte += (words + QTY_DIR_HEADER_FIELDS) * EXT_SIZE(fam, M_INT);
    }

    return OK;
}

/*****************************************************************
 * TAG( delete_ti_dir ) PRIVATE
 *
 * Delete an in-memory non-state file directory.
 */
void delete_ti_dir(Mili_family *fam)
{
    File_dir *p_fd, *bound;

    /* Sanity check. */
    if ( fam->ti_directory == NULL )
    {
        return;
    }

    bound = fam->ti_directory + fam->ti_file_count;
    for ( p_fd = fam->ti_directory; p_fd < bound; p_fd++ )
    {
        if ( p_fd->dir_entries )
        {
            free(p_fd->dir_entries);
        }
        if ( p_fd->names )
        {
            free(p_fd->names);
        }
        if ( p_fd->name_data )
        {
            ios_destroy(p_fd->name_data);
        }
    }

    free(fam->ti_directory);

    fam->ti_directory = NULL;
}

/*****************************************************************
 * TAG( load_ti_directories ) PRIVATE
 *
 * Traverse non-state files in family and read in directories.
 *
 * Should only be called at initialization of an existing family.
 *
 * This routine is hard-coded to the current (initial) design
 * of directories.  A new design would require abstraction of the
 * directory handling routines.
 */
Return_value load_ti_directories(Mili_family *fam)
{
    int fcnt;
    int fnum;
    int qty_ent, qty_nam;
    FILE *p_f;
    char fname[M_MAX_NAME_LEN];
    int header[QTY_DIR_HEADER_FIELDS];
    int nnames;
    LONGLONG nitems, nbytes, offset;
    File_dir *p_fd;
    Dir_entry *p_de;
    TempDir_entry *temp_p_de;
    int fd;
    Return_value rval;
    Bool_type active;
    IO_mem_store *pioms;
    int i, j;
    Param_ref *ppr;
    Htable_entry *phte;
    Dir_entry_type etype;
    char *p_name;
    int status;

    Bool_type dir_read = FALSE;

    active = fam->active_family;

    /*
     * If there is a process writing to the family; get the name
     * of the last complete non-state file from the lock file.
     */
    if ( active )
    {
        rval = get_name_lock(fam, NON_STATE_DATA);

        if ( rval != OK )
        {
            return rval;
        }

        fd = fam->lock_file_descriptor;

        if ( lseek(fd, 1, SEEK_SET) == -1 )
        {
            return SEEK_FAILED;
        }
        nbytes = read(fd, (void *)fname, M_MAX_NAME_LEN);
        if ( nbytes != 256 )
        {
            return SHORT_READ;
        }
        else if ( fname[0] == '\0' )
        {
            /*
             * Apparently another process has write access to the
             * database but hasn't committed anything yet.
             */
            fam->ti_file_count = 0;
            return OK;
        }
        else
        {
            fname[nbytes] = '\0';
        }

        rval = free_name_lock(fam, NON_STATE_DATA);
        if ( rval != OK )
        {
            return rval;
        }

        rval = get_file_index(NON_STATE_DATA, fam->file_root, fname, &fnum);
        if ( rval != OK )
        {
            return rval;
        }
    }
    else
    {
        fnum = 0;
    }

    /*
     * Loop through all the available non-state files.
     */

    fcnt = 0;
    make_fnam(TI_DATA, fam, fcnt, fname);

    while ( ((p_f = fopen(fname, "rb")) != NULL) && (!active || fcnt <= fnum) )
    {
        fam->ti_cur_file = p_f;

        /* Seek to end of file and read directory "header". */
        if ( fam->char_header[DIR_VERSION_IDX] == 1 )
        {
            offset = -(QTY_DIR_HEADER_FIELDS - 1) * EXT_SIZE(fam, M_INT);
        }
        else
        {
            offset = -QTY_DIR_HEADER_FIELDS * EXT_SIZE(fam, M_INT);
        }
        status = fseek(p_f, offset, SEEK_END);
        if ( status != 0 )
        {
            fclose(p_f);
            return SEEK_FAILED;
        }
        if ( fam->char_header[DIR_VERSION_IDX] == 1 )
        {
            nitems = fam->read_funcs[M_INT](p_f, header, QTY_DIR_HEADER_FIELDS - 1);
            if ( nitems != QTY_DIR_HEADER_FIELDS - 1 )
            {
                fclose(p_f);
                return BAD_LOAD_READ;
            }
        }
        else
        {
            nitems = fam->read_funcs[M_INT](p_f, header, QTY_DIR_HEADER_FIELDS);
            if ( nitems != QTY_DIR_HEADER_FIELDS )
            {
                fclose(p_f);
                return BAD_LOAD_READ;
            }
        }

        qty_ent = header[QTY_ENTRIES_IDX];

        /* Allocate storage for current non-state file's directory. */
        fam->ti_directory = RENEW_N(File_dir, fam->ti_directory, fcnt, 1, "Load dir File_dir");
        if ( fam->ti_directory == NULL )
        {
            fclose(p_f);
            return ALLOC_FAILED;
        }
        p_fd = fam->ti_directory + fcnt;
        p_de = NEW_N(Dir_entry, qty_ent, "Load dir entries");
        if ( qty_ent > 0 && p_de == NULL )
        {
            free(fam->ti_directory);
            fam->ti_directory = NULL;
            fclose(p_f);
            return ALLOC_FAILED;
        }

        /* Seek to beginning of directory entries and read all at once. */
        offset -= qty_ent * QTY_ENTRY_FIELDS * EXT_SIZE(fam, M_INT);
        status = fseek(p_f, offset, SEEK_END);
        if ( status != 0 )
        {
            free(fam->ti_directory);
            fam->ti_directory = NULL;
            free(p_de);
            fclose(p_f);
            return SEEK_FAILED;
        }
        status = fseek(p_f, offset, SEEK_END);

        temp_p_de = NEW_N(TempDir_entry, qty_ent, "Load dir entries");
        nitems = fam->read_funcs[M_INT](p_f, temp_p_de, QTY_ENTRY_FIELDS * qty_ent);
        for ( i = 0; i < qty_ent; i++ )
        {
            for ( j = 0; j < QTY_ENTRY_FIELDS; j++ )
            {
                p_de[i][j] = (LONGLONG)temp_p_de[i][j];
            }
        }
        free(temp_p_de);
        if ( nitems != (LONGLONG)qty_ent * QTY_ENTRY_FIELDS )
        {
            free(fam->ti_directory);
            fam->ti_directory = NULL;
            free(p_de);
            fclose(p_f);
            return BAD_LOAD_READ;
        }

        /* Calculate quantity of strings in directory. */
        qty_nam = 0;
        for ( i = 0; i < qty_ent; i++ )
        {
            qty_nam += p_de[i][STRING_QTY_IDX];
        }

        /* Fill in the file's File_dir. */
        p_fd->dir_entries = p_de;
        p_fd->commit_count = header[COMMIT_COUNT_IDX];
        p_fd->qty_entries = qty_ent;
        p_fd->qty_names = qty_nam;
        p_fd->names = NEW_N(char *, qty_nam, "File_dir entry names");
        if ( qty_nam > 0 && p_fd->names == NULL )
        {
            free(fam->ti_directory);
            fam->ti_directory = NULL;
            free(p_de);
            fclose(p_f);
            return ALLOC_FAILED;
        }

        if ( fam->ti_commit_count < header[COMMIT_COUNT_IDX] )
        {
            fam->ti_commit_count = header[COMMIT_COUNT_IDX];
        }

        /*
         * Now seek and read entry names if they exist.  Store references
         * to parameters in the param table.
         */
        nitems = header[NAMES_LEN_IDX];
        if ( nitems > 0 )
        {
            offset -= nitems;
            status = fseek(p_f, offset, SEEK_END);
            if ( status != 0 )
            {
                free(p_fd->names);
                free(fam->ti_directory);
                fam->ti_directory = NULL;
                free(p_de);
                fclose(p_f);
                return SEEK_FAILED;
            }

            /* Names go in an IO Store. */
            pioms = ios_create_empty();
            if ( pioms == NULL )
            {
                free(p_fd->names);
                free(fam->ti_directory);
                fam->ti_directory = NULL;
                free(p_de);
                fclose(p_f);
                return IOS_ALLOC_FAILED;
            }
            ti_ios_input(fam, M_STRING, nitems, pioms, NULL);
            p_fd->name_data = pioms;
            rval = ios_traverse_init(pioms, 1);
            if ( rval != OK )
            {
                free(p_fd->names);
                free(fam->ti_directory);
                fam->ti_directory = NULL;
                free(p_de);
                fclose(p_f);
                return rval;
            }

            /* Create the param table if necessary. */
            if ( fam->ti_param_table == NULL )
            {
                fam->ti_param_table = htable_create(DEFAULT_HASH_TABLE_SIZE);
                if ( fam->ti_param_table == NULL )
                {
                    free(p_fd->names);
                    free(fam->ti_directory);
                    fam->ti_directory = NULL;
                    free(p_de);
                    fclose(p_f);
                    return ALLOC_FAILED;
                }
            }

            /* Traverse through the entries for this file's directory. */
            nnames = 0;
            for ( i = 0; i < qty_ent; i++ )
            {
                /* If entry has associated strings, copy them */
                p_name = NULL;
                for ( j = 0; j < p_de[i][STRING_QTY_IDX]; j++ )
                {
                    /* Get next name from directory name data. */
                    rval = ios_string_traverse(pioms, &p_name);
                    if ( rval != OK )
                    {
                        free(p_fd->names);
                        free(fam->ti_directory);
                        fam->ti_directory = NULL;
                        free(p_de);
                        fclose(p_f);
                        return rval;
                    }

                    p_fd->names[nnames + j] = p_name;
                }

                nnames += p_de[i][STRING_QTY_IDX];

                /* If entry is a parameter... */
                etype = (Dir_entry_type)p_de[i][TYPE_IDX];
                if ( etype == TI_PARAM )
                {
                    if ( p_name == NULL )
                    {
                        free(p_fd->names);
                        free(fam->ti_directory);
                        fam->ti_directory = NULL;
                        free(p_de);
                        fclose(p_f);
                        return NOT_OK;
                    }
                    /* All parameters will have names. */
                    rval = htable_search(fam->ti_param_table, p_name, ENTER_ALWAYS, &phte);
                    if ( phte == NULL )
                    {
                        free(p_fd->names);
                        free(fam->ti_directory);
                        fam->ti_directory = NULL;
                        free(p_de);
                        fclose(p_f);
                        return rval;
                    }
                    /* Create an entry in the param table. */
                    ppr = NEW(Param_ref, "Param table entry - string");
                    if ( ppr == NULL )
                    {
                        free(p_fd->names);
                        free(fam->ti_directory);
                        fam->ti_directory = NULL;
                        free(p_de);
                        fclose(p_f);
                        return ALLOC_FAILED;
                    }
                    ppr->file_index = fcnt;
                    ppr->entry_index = i;
                    phte->data = (void *)ppr;
                }
            }
        }

        dir_read = TRUE;

        /* Prepare to access next file in family. */
        fclose(p_f);
        fcnt++;
        fam->ti_file_count = fcnt;
        make_fnam(TI_DATA, fam, fcnt, fname);
    }

    fam->ti_cur_file = NULL;

    if ( p_f != NULL )
    {
        fclose(p_f);
    }

    if ( dir_read )
    {
        return OK;
    }
    else
    {
        return NOT_OK;
    }
}
