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

#include "mili_internal.h"
#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#endif

/*****************************************************************
 * TAG( fam_list )
 *
 * Dynamically allocated array of pointers to all currently open
 * MILI families.
 */
Mili_family **fam_list;


static void delete_tfile(Mili_family *fam)
{
    FILE *fp;
    if ( !fam->write_tfile )
    {
        if ( fam->time_state_file )
        {
            fp = fam->time_state_file;
        }
        else
        {
            fp = fopen(fam->time_file_name, "r");
        }
        if ( fp )
        {
            fclose(fp);
            fp = NULL;
            fam->time_state_file = NULL;
            unlink(fam->time_file_name);
        }
    }
    return;
}

/*****************************************************************
 * TAG( truncate_family ) LOCAL
 *
 * Prune from a family all states and files including and following
 * the state indicated by "st_index".
 */
static Return_value truncate_family(Mili_family *p_fam, int st_index)
{
    char fname[M_MAX_NAME_LEN];
    Return_value rval = OK;
    int status = 0;
    int file_qty = 0;
    int state_qty = 0;
    int i = 0;
    long offset = 0;
    int header[QTY_DIR_HEADER_FIELDS];
    int num_items = -1;
    char check = ' ';
    FILE *fp = NULL;

    if ( p_fam->state_qty == st_index )
    {
        return OK;
    }
    else if ( p_fam->state_qty < st_index )
    {
        return INVALID_FILE_STATE_INDEX;
    }

    offset = p_fam->state_map[st_index].offset;
    state_qty = p_fam->state_qty;

    /* Make sure any file that will be affected is closed. */
    if ( p_fam->cur_st_index >= p_fam->state_map[st_index].file )
    {
        rval = state_file_close(p_fam);
        if ( rval != OK )
        {
            return rval;
        }
    }

    /* Get quantity of state files remaining after truncation. */
    file_qty = p_fam->state_map[st_index].file;

    for ( i = p_fam->st_file_count - 1; i > file_qty; i-- )
    {
        make_fnam(STATE_DATA, p_fam, ST_FILE_SUFFIX(p_fam, i), fname);
        status = unlink(fname);
        if ( status != 0 )
        {
            rval = FAMILY_TRUNCATION_FAILED;
            break;
        }
    }

    p_fam->st_file_count = i + 1;

    if ( p_fam->state_map[st_index].file == 0 && st_index == 0 )
    {
        make_fnam(STATE_DATA, p_fam, ST_FILE_SUFFIX(p_fam, 0), fname);
        status = unlink(fname);
        if ( status != 0 )
        {
            return FAMILY_TRUNCATION_FAILED;
        }
    }
    else if ( p_fam->state_map[st_index].offset == 0 )
    {
        make_fnam(STATE_DATA, p_fam, ST_FILE_SUFFIX(p_fam, p_fam->state_map[st_index].file), fname);
        status = unlink(fname);
        if ( status != 0 )
        {
            return FAMILY_TRUNCATION_FAILED;
        }
        p_fam->st_file_count -= 1;
    }
    else
    {
        make_fnam(STATE_DATA, p_fam, ST_FILE_SUFFIX(p_fam, p_fam->state_map[st_index].file), fname);
#if defined(_WIN32) || defined(WIN32)
        status = 0;
#else
        status = truncate(fname, p_fam->state_map[st_index].offset);
#endif
        status = state_file_open(p_fam, p_fam->state_map[st_index].file, p_fam->access_mode);
    }

    /* Clean up the state maps, in mili file version < 3 these are in the A-file, thereafter they are in the T-file */
    if ( p_fam->char_header[DIR_VERSION_IDX] > 1 )
    {
        if ( p_fam->char_header[HDR_VERSION_IDX] > 2 && p_fam->write_tfile )
        {
            if ( p_fam->time_state_file == NULL )
            {
                p_fam->time_state_file = fopen(p_fam->time_file_name, "r+b");
            }
            fp = p_fam->time_state_file;
        }
        else
        {
            fp = fopen(p_fam->aFile, "r+b");
        }

        if ( fp )
        {
            if ( p_fam->char_header[HDR_VERSION_IDX] > 2 && p_fam->write_tfile )
            {
                offset = -1;
            }
            else
            {
                offset = -(QTY_DIR_HEADER_FIELDS)*EXT_SIZE(p_fam, M_INT);
            }
            fseek(fp, offset, SEEK_END);
            if ( status != 0 )
            {
                fclose(fp);
                return SEEK_FAILED;
            }
            if ( p_fam->char_header[HDR_VERSION_IDX] > 2 && p_fam->write_tfile )
            {
                num_items = p_fam->read_funcs[M_STRING](fp, &check, 1);
                if ( num_items != 1 || check != p_fam->state_end_marker )
                {
                    fclose(fp);
                    mc_print_error("Standalone state file corrupted ", CORRUPTED_FILE);
                }
            }
            else
            {
                num_items = p_fam->read_funcs[M_INT](fp, header, QTY_DIR_HEADER_FIELDS);
                if ( num_items != QTY_DIR_HEADER_FIELDS )
                {
                    fclose(fp);
                    return BAD_LOAD_READ;
                }
            }
            if ( p_fam->char_header[HDR_VERSION_IDX] > 2 && p_fam->write_tfile )
            {
                offset = 0;
                status = fseek(fp, offset, SEEK_SET);
            }
            else
            {
                offset -= header[QTY_STATES_IDX] * 20;
                status = fseek(fp, offset, SEEK_END);
            }

            if ( st_index > 0 )
            {
                status = fseek(fp, st_index * 20, SEEK_CUR);
            }
            offset = ftell(fp);

            fclose(fp);
            fp = NULL;
            if ( p_fam->char_header[HDR_VERSION_IDX] > 2 && p_fam->write_tfile )
            {
                p_fam->time_state_file = NULL;
            }
#if !(defined(_WIN32) || defined(WIN32))
            if ( p_fam->char_header[HDR_VERSION_IDX] > 2 && p_fam->write_tfile )
            {
                strcpy(fname, p_fam->time_file_name);
            }
            else
            {
                strcpy(fname, p_fam->aFile);
            }
            truncate(fname, offset);
#endif
            if ( p_fam->char_header[HDR_VERSION_IDX] > 2 && p_fam->write_tfile )
            {
                if ( p_fam->time_state_file == NULL )
                {
                    p_fam->time_state_file = fopen(p_fam->time_file_name, "r+b");
                }
                fp = p_fam->time_state_file;
                fseek(fp, 0, SEEK_END);
                p_fam->write_funcs[M_STRING](fp, &p_fam->state_end_marker, 1);
            }
            else
            {
                fp = fopen(p_fam->aFile, "r+b");
                fseek(fp, 0, SEEK_END);
                header[QTY_STATES_IDX] = st_index;
                num_items = p_fam->write_funcs[M_INT](fp, header, QTY_DIR_HEADER_FIELDS);
                if ( num_items != QTY_DIR_HEADER_FIELDS )
                {
                    fclose(fp);
                    return SHORT_WRITE;
                }
            }
            fclose(fp);
            p_fam->time_state_file = NULL;
        }
    }

    /* Clean up state_map*/
    if ( rval == OK )
    {
        if ( st_index == 0 )
        {
            memset((void *)fname, (int)0, 64);

            free(p_fam->state_map);
            p_fam->state_map = NULL;
            free(p_fam->file_map);
            p_fam->file_map = NULL;
            p_fam->state_qty = 0;
            p_fam->cur_st_file_size = 0;
            p_fam->file_st_qty = 0;
            p_fam->st_file_count = 0;
        }
        else
        {
            make_fnam(STATE_DATA, p_fam, ST_FILE_SUFFIX(p_fam, p_fam->state_map[st_index].file), fname);
            State_descriptor *temp = NEW_N(State_descriptor, st_index, "Shrunken state map on restart");

            memcpy((void *)temp, p_fam->state_map, st_index * sizeof(State_descriptor));
            free(p_fam->state_map);
            p_fam->state_map = temp;
            temp = NULL;
            if ( st_index > 0 && p_fam->state_map == NULL )
            {
                rval = ALLOC_FAILED;
            }

            if ( file_qty + 1 != p_fam->st_file_count )
            {
                p_fam->file_map = RENEW_N(State_file_descriptor, p_fam->file_map, p_fam->st_file_count, file_qty,
                                          "Shrunken file map on restart");
            }
            if ( p_fam->file_map == NULL )
            {
                rval = ALLOC_FAILED;
            }
            else
            {
                p_fam->state_qty = st_index;
                int file_st_idx = st_index;
                int file_st_cnt = 0;
                while( file_st_idx > 0 && p_fam->state_map[ file_st_idx - 1 ].file == p_fam->state_map[ st_index - 1 ].file )
                {
                    file_st_cnt += 1;
                    file_st_idx -= 1;
                }
                p_fam->file_st_qty = file_st_cnt;
                p_fam->file_map[p_fam->state_map[st_index - 1].file].state_qty = p_fam->file_st_qty;
            }
        }
    }

    return rval;
}
/*****************************************************************
 * TAG( set_state_map_on ) PUBLIC
 *
 * Turn on the or off the new state map files.  Function must be called at creation
 * time of the mili files. Cannot change once the run begins.
 *
 * @Famid fam_id Int value of the database to set
 * @param Bool_type on zero for off. Any other number for on default is off
 *
 */

Return_value mc_set_state_map_file_on(Famid fam_id, Bool_type on)
{
    Mili_family *fam;
    fam = fam_list[fam_id];
    if ( fam->access_mode == 'r' && fam->state_qty > 0 || fam->state_dirty || fam->char_header[HDR_VERSION_IDX] < 3 )
    {
        return MAP_FILE_CREATION_ERROR;
    }
    fam->write_tfile = on;

    if ( !on )
    {
        delete_tfile(fam);
    }
    else
    {
        if ( fam->time_state_file == NULL )
        {
            fam->time_state_file = fopen(fam->time_file_name, "r");
        }

        if ( fam->time_state_file )
        {
            fclose(fam->time_state_file);
            fam->time_state_file = fopen(fam->time_file_name, "r+b");
        }
        else
        {
            fam->time_state_file = fopen(fam->time_file_name, "w+b");
        }
    }
    return OK;
}

/*****************************************************************
 * TAG( rebuild_state_tfile )
 *
 * Rebuild state file based on plot files
 *
 */
Return_value rebuild_state_tfile(Mili_family *fam)
{
    if ( !fam->write_tfile )
    {
        return OK;
    }

    State_file_descriptor *p_fmap = NULL;
    int index;
    LONGLONG offset;
    LONGLONG (*readi)();
    LONGLONG (*readf)();
    float st_time;
    int srec_id;
    int state_qty;
    int hdr_size;
    Return_value rval;
    int wrt_size = 0;

    offset = 0;

    if ( !fam->time_state_file )
    {
        fam->time_state_file = fopen(fam->time_file_name, "w+");
        if ( fam->time_state_file == NULL )
        {
            return MAP_FILE_CREATION_ERROR;
        }
    }
    if ( fam->file_map )
    {
        free(fam->file_map);
    }
    fam->file_map = NULL;

    if ( fam->state_map )
    {
        free(fam->state_map);
    }
    fam->state_map = NULL;

    readi = fam->state_read_funcs[M_INT];
    readf = fam->state_read_funcs[M_FLOAT];

    hdr_size = EXT_SIZE(fam, M_INT) + EXT_SIZE(fam, M_FLOAT);

    rval = OK;
    index = 0;
    state_qty = 0;

    /* Count offset through mapped states in known last file. */
    offset = 0;

    fam->file_st_qty = 0;

    /* Traverse the state data files. */
    while ( state_file_open(fam, index, 'r') == OK )
    {
        if ( fam->file_map == NULL )
        {
            fam->file_map = NEW_N(State_file_descriptor, 1, "Allocating state file descriptor in rebuild_state_tfile");
            if ( !fam->file_map )
            {
                return ALLOC_FAILED;
            }
        }
        else
        {
            fam->file_map = RENEW_N(State_file_descriptor, p_fmap, index, 1,
                                    "Reallocating state file descriptor in rebuild_state_tfile");
            if ( !fam->file_map )
            {
                return ALLOC_FAILED;
            }
        }
        /* Traverse the state records within the current file. */
        while ( offset < fam->cur_st_file_size && seek_state_file(fam->cur_st_file, offset) == OK )
        {
            /* Read the header - time and format. */
            if ( readf(fam->cur_st_file, &st_time, 1) != 1 )
            {
                rval = SHORT_READ;
                break;
            }

            if ( readi(fam->cur_st_file, &srec_id, 1) != 1 )
            {
                rval = SHORT_READ;
                break;
            }

            /* Add a new entry in the state map. */
            if ( fam->state_map == NULL )
            {
                fam->state_map = NEW_N(State_descriptor, 1, "Allocate state descriptor in rebuild_state_tfile");
                if ( !fam->state_map )
                {
                    return ALLOC_FAILED;
                }
            }
            else
            {
                fam->state_map = RENEWC_N(State_descriptor, fam->state_map, state_qty, 1,
                                          "Reallocating state file descriptor in rebuild_state_tfile");
                if ( !fam->state_map )
                {
                    return ALLOC_FAILED;
                }
            }

            fam->state_map[state_qty].file = index;
            fam->state_map[state_qty].offset = offset;
            fam->state_map[state_qty].time = st_time;
            fam->state_map[state_qty].srec_format = srec_id;

            wrt_size = fam->write_funcs[M_INT](fam->time_state_file, (void *)&index, 1);
            if ( wrt_size != 1 )
            {
                rval = SHORT_WRITE;
            }
            wrt_size = fam->write_funcs[M_FLOAT8](fam->time_state_file, (void *)&offset, 1);
            if ( wrt_size != 1 )
            {
                rval = SHORT_WRITE;
            }
            wrt_size = fam->write_funcs[M_FLOAT](fam->time_state_file, (void *)&st_time, 1);
            if ( wrt_size != 1 )
            {
                rval = SHORT_WRITE;
            }
            wrt_size = fam->write_funcs[M_INT](fam->time_state_file, (void *)&srec_id, 1);
            if ( wrt_size != 1 )
            {
                rval = SHORT_WRITE;
            }
            if ( rval != OK )
            {
                return rval;
            }
            /* Update stuff... */
            offset += fam->srecs[srec_id]->size + hdr_size;

            fam->cur_st_offset = offset;
            state_qty++;
            fam->state_qty = state_qty;
            fam->file_st_qty++;
        }

        index++;

        if ( offset == 0 || rval != OK )
        {
            rval = state_file_close(fam);
            break;
        }
        fam->st_file_count = index;
    }

    if ( rval == OK )
    {
        index = fam->write_funcs[M_STRING](fam->time_state_file, "~", 1);
        if ( index != 1 )
        {
            rval = SHORT_WRITE;
        }
    }

    return rval;
}

/*****************************************************************
 * TAG( load_static_map ) PRIVATE
 *
 * Build file/offset map of state-data records in family.
 */
Return_value load_static_maps(Mili_family *fam, Bool_type initial_build, Bool_type force_reopen_state_file)
{
    Return_value rval = OK;
    long offset = 0;
    int status, nitems, state_count;
    int header[QTY_DIR_HEADER_FIELDS];
    int cur_state;
    FILE *fp = NULL;

    int file, srec_id;
    LONGLONG file_offset;
    float time;
    int file_count = -1;
    char check;

    if ( fam->char_header[HDR_VERSION_IDX] > 2 && fam->write_tfile )
    {
        if ( fam->time_state_file == NULL )
        {
            fam->time_state_file = fopen(fam->time_file_name, "r+b");
        }
        fp = fam->time_state_file;
    }
    else
    {
        if ( fam->access_mode == 'r' )
        {
            fp = fopen(fam->aFile, "r+b");
        }
        else
        {
            fp = fopen(fam->aFile, "r+b");
        }
    }
    if ( fp )
    {
        if ( fam->char_header[HDR_VERSION_IDX] > 2 && fam->write_tfile )
        {
            offset = -1;
        }
        else
        {
            offset = -(QTY_DIR_HEADER_FIELDS)*EXT_SIZE(fam, M_INT);
        }
        status = fseek(fp, offset, SEEK_END);
        if ( status != 0 )
        {
            fclose(fp);
            return SEEK_FAILED;
        }
        if ( fam->char_header[HDR_VERSION_IDX] > 2 && fam->write_tfile )
        {
            nitems = fam->read_funcs[M_STRING](fp, &check, 1);
            if ( nitems != 1 || check != fam->state_end_marker )
            {
                fclose(fp);
                fp = fam->time_state_file = NULL;
                if ( rebuild_state_tfile(fam) != OK )
                {
                    mc_print_error("Standalone state file corrupted ", CORRUPTED_FILE);
                    return CORRUPTED_FILE;
                }
                fp = fam->time_state_file;
            }
            // seek to last byte of the file '~' as confirmed above
            fseek(fp, offset, SEEK_END);
            if ( status != 0 )
            {
                fclose(fp);
                return SEEK_FAILED;
            }
            // take the offset into the file and divide by state map descriptor size in bytes to get state map count
            state_count = ftell(fp) / 20;
            // seek to the start of the file to parse state maps
            fseek(fp, 0, SEEK_SET);

            status = ftell(fp);
            if ( status != 0 )
            {
                fclose(fp);
                return SEEK_FAILED;
            }
        }
        else
        {
            nitems = fam->read_funcs[M_INT](fp, header, QTY_DIR_HEADER_FIELDS);
            if ( nitems != QTY_DIR_HEADER_FIELDS )
            {
                fclose(fp);
                return BAD_LOAD_READ;
            }
            state_count = header[QTY_STATES_IDX];
            offset -= state_count * 20;
            status = fseek(fp, offset, SEEK_END);
            if ( status != 0 )
            {
                fclose(fp);
                return SEEK_FAILED;
            }
        }
        fam->state_qty = state_count;

        // parse state maps
        fam->state_map = NEW_N(State_descriptor, state_count, "State map descriptors");
        for ( cur_state = 0; cur_state < state_count; cur_state++ )
        {
            fam->read_funcs[M_INT](fp, &file, 1);
            fam->read_funcs[M_INT8](fp, &file_offset, 1);
            fam->read_funcs[M_FLOAT](fp, &time, 1);
            fam->read_funcs[M_INT](fp, &srec_id, 1);

            fam->state_map[cur_state].file = file;
            fam->state_map[cur_state].offset = file_offset;
            fam->state_map[cur_state].time = time;
            fam->state_map[cur_state].srec_format = srec_id;

            if ( file > file_count )
            {
                file_count = file;
                fam->st_file_count = file + 1;
                fam->file_map = RENEWC_N(State_file_descriptor, fam->file_map, file, 1, "New file map entry");
            }
            fam->file_map[file].state_qty++;
        }

        fclose(fp);
        fam->time_state_file = NULL;
    }

    if ( force_reopen_state_file )
    {
        if ( fam->cur_st_file != NULL )
            state_file_close(fam);

        rval = state_file_open(fam, fam->state_map[ fam->state_qty-1 ].file, fam->access_mode);
        if ( rval != OK )
        {
            return rval;
        }
    }

    if ( state_count > 0 && fam->access_mode != 'r')
    {
        rval = state_file_open(fam, fam->st_file_count - 1, fam->access_mode);
        if ( rval != OK )
        {
            return rval;
        }
    }
    return rval;
}


/*****************************************************************
 * TAG( update_static_map ) PRIVATE
 *
 * Write the State_descriptor to file.
 *  Before mili file version 3, this writes to the A-file
 *  Starting in mili file version 3, this writes to the T-file
 */

Return_value update_static_map(Famid fam_id, State_descriptor *p_sd)
{
    /**
    We will need to seek to the end of the file minus the header size.
    Read in the header data.
    Write the timestep information.
    Increment the  QTY_STATES_IDX value by one. (prior to mili file version 3)
    Rewrite the header the header information.
    Then flush the non-state file.
    */
    Mili_family *fam = fam_list[fam_id];
    long offset = 0;

    int status = 0;
    int num_items = -1;
    int num_written = 0;
    int header[QTY_DIR_HEADER_FIELDS];
    int state_count = 0;
    char check = ' ';

    Return_value rval = OK;
    FILE *fp = NULL;

    if ( fam->char_header[HDR_VERSION_IDX] > 2 && fam->write_tfile )
    {
        if ( fam->time_state_file == NULL )
        {
            fam->time_state_file = fopen(fam->time_file_name, "r+b");
        }
        fp = fam->time_state_file;
    }
    else
    {
        fp = fopen(fam->aFile, "r+b");
    }

    if ( fp )
    {
        if ( fam->char_header[HDR_VERSION_IDX] > 2 && fam->write_tfile )
        {
            fseek(fp, 0, SEEK_END);
            if ( ftell(fp) )
            {
                offset = -1;
            }
            else
            {
                offset = 0;
            }
        }
        else
        {
            offset = -(QTY_DIR_HEADER_FIELDS)*EXT_SIZE(fam, M_INT);
        }
        status = fseek(fp, offset, SEEK_END);
        if ( status != 0 )
        {
            fclose(fp);
            return SEEK_FAILED;
        }
        // Write it the tfile instead of the afile
        if ( fam->char_header[HDR_VERSION_IDX] > 2 && fam->write_tfile )
        {
            state_count = ftell(fp) / 20;
            if ( state_count > 0 )
            {
                num_items = fam->read_funcs[M_STRING](fp, &check, 1);

                if ( num_items != 1 || check != fam->state_end_marker )
                {
                    // Can try a repair at this point if needed.
                    fclose(fp);

                    mc_print_error("Standalone state file corrupted ", CORRUPTED_FILE);
                }

                offset = -1;
            }
        }
        else
        {
            num_items = fam->read_funcs[M_INT](fp, header, QTY_DIR_HEADER_FIELDS);
            if ( num_items != QTY_DIR_HEADER_FIELDS )
            {
                fclose(fp);
                return BAD_LOAD_READ;
            }
            header[QTY_STATES_IDX]++;
            state_count = header[QTY_STATES_IDX];
        }

        // Reset the point to current start of the header information
        status = fseek(fp, offset, SEEK_END);
        fam->write_funcs[M_INT](fp, &(p_sd->file), 1);
        fam->write_funcs[M_INT8](fp, &(p_sd->offset), 1);
        fam->write_funcs[M_FLOAT](fp, &(p_sd->time), 1);
        fam->write_funcs[M_INT](fp, &(p_sd->srec_format), 1);

        if ( fam->char_header[HDR_VERSION_IDX] > 2 && fam->write_tfile )
        {
            fam->write_funcs[M_STRING](fp, &fam->state_end_marker, 1);
        }
        else
        {
            num_written = fam->write_funcs[M_INT](fp, header, QTY_DIR_HEADER_FIELDS);
            if ( num_written != QTY_DIR_HEADER_FIELDS )
            {
                return SHORT_WRITE;
            }
            if ( fp )
            {
                fclose(fp);
            }
        }
        fp = NULL;
        state_count = state_count + 1;
        mc_wrt_scalar(fam_id, M_INT, "state_count", (void *)&state_count);
    }
    else
    {
        rval = NO_A_FILE_FOR_STATEMAP;
    }
    return rval;
}

int find_state_index(Mili_family *fam, int file_index, int local_index)
{
    int index = 0;
    State_descriptor *state_map = fam->state_map;

    while ( state_map[index].file < file_index )
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
Return_value mc_restart_at_state(Famid fam_id, int file_name_index, int state_index)
{
    Mili_family *fam;
    Return_value rval = OK;

    fam = fam_list[fam_id];

    CHECK_WRITE_ACCESS(fam)

    if ( file_name_index > 0 )
    {
        state_index = find_state_index(fam, file_name_index, state_index);
    }

    /* Can't permit a gap in state sequence. */
    if ( state_index > fam->state_qty )
    {
        return INVALID_FILE_STATE_INDEX;
    }
    if ( state_index < fam->state_qty )
    {
        rval = truncate_family(fam, state_index);
    }

    return rval;
}

/*****************************************************************
 * TAG( mc_restart_at_file ) PUBLIC
 *
 * Set db to receive data by overwriting at the specified file.
 */
Return_value mc_restart_at_file(Famid fam_id, int file_name_index)
{
    Mili_family *fam;
    State_file_descriptor *p_sfd;
    int i;
    int state_index, file_index;
    Return_value rval = OK;

    fam = fam_list[fam_id];

    CHECK_WRITE_ACCESS(fam)

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
            rval = state_file_close(fam);
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
    rval = truncate_family(fam, state_index);

    return rval;
}
