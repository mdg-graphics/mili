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

 * I/O store management routines
 *
 */

/*
************************************************************************
* Modifications:
*
*  I. R. Corey - January 8, 2007: Added functions for TI files.
*
*  I. R. Corey - December 20, 2007: Increased name allocation to 16000.
*                Fixed a bug with the reallocation of MemStore.
*                SCR#: 513
*
*  I. R. Corey - May 19, 2010: Incorporated more changes from
*                Sebastian at IABG to support building under Windows.
*                SCR #679.
*
************************************************************************
*/

#ifdef DEBUG_IOS
#define ALLOC_UNITS 100
#else
#define ALLOC_UNITS 16000
#endif

#include <stdio.h>
#include <string.h>
#include "mili_internal.h"

#ifdef DEBUG_IOS

main(int argc, char *argv[])
{
    FILE *ofile;
    char *pc, *pc1;
    int *pi, *pi1;
    int i;
    LONGLONG len;
    IO_mem_store *pioms;
    char *test1 = "0123456789012345678901234567890123456789";
    char *test2 = "B7e7t7w7e7e7n7 77777777t7h7e7 7l7i7n7e7s";
    Famid mfam;

    int num_written = 0;

    /* Open a Mili family just to get some initialization done. */
    i = Mopen_family_("mt", ".", "w", &mfam);

    ofile = fopen("iost", "wb");
    if ( ofile == NULL )
    {
        fprintf(stderr, "Unable to open file \"iost\".\n");
        exit(-1);
    }

    pioms = ios_create(M_STRING);

    len = strlen(test1);
    pc = ios_alloc(len + 1, pioms);
    strcpy(pc, test1);

    ios_output(mfam, ofile, pioms, &num_written);
    fflush(ofile);

    ios_str_dup(&pc, test1, pioms);

    ios_output(mfam, ofile, pioms, &num_written);
    fflush(ofile);

    ios_str_dup(&pc, test2, pioms);

    len = strlen(test2);
    for ( pc1 = pc; *pc1 != '\0'; pc1++ )
    {
        if ( *pc1 == '7' )
        {
            ios_unalloc(pc1, 1, pioms);
        }
    }

    ios_output(mfam, ofile, pioms, &num_written);
    fflush(ofile);

    pc = ios_alloc((LONGLONG)3 * ALLOC_UNITS, pioms);
    for ( pc1 = pc; pc1 < pc + 3 * ALLOC_UNITS; *pc1++ = '7' )
        ;
    ios_output(mfam, ofile, pioms, &num_written);
    fflush(ofile);

    ios_destroy(pioms);

    pioms = ios_create(M_INT);
    pi = ios_alloc((LONGLONG)128, pioms);
    for ( pi1 = pi, i = 0; pi1 < pi + 128; *pi1++ = i++ )
        ;
    ios_output(mfam, ofile, pioms, &num_written);
    fflush(ofile);

    ios_destroy(pioms);

    fclose(ofile);

    Mclose_family_(&mfam);

    exit(0);
}

#endif

/*****************************************************************
 * TAG( ios_create ) PRIVATE
 *
 * Allocate and initialize an I/O memory store.
 */
IO_mem_store *ios_create(int datatype)
{
    IO_mem_store *pioms = NULL;
    IO_mem_buffer *piomb = NULL;

    pioms = NEW(IO_mem_store, "IO mem store");
    if ( pioms == NULL )
    {
        return NULL;
    }
    pioms->data_buffers = NEW(IO_mem_buffer *, "Initial IO buffer list");
    if ( pioms->data_buffers == NULL )
    {
        free(pioms);
        return NULL;
    }
    piomb = NEW(IO_mem_buffer, "IO mem buffer");
    if ( piomb == NULL )
    {
        free(pioms->data_buffers);
        free(pioms);
        return NULL;
    }
    piomb->size = ALLOC_UNITS;
    pioms->data_buffers[0] = piomb;

    switch ( datatype )
    {
        case M_STRING:
            piomb->data = (void *)NEW_N(char, ALLOC_UNITS, "char io mem data");
            pioms->type = M_STRING;
            break;
        case M_INT:
            piomb->data = (void *)NEW_N(int, ALLOC_UNITS, "int io mem data");
            pioms->type = M_INT;
            break;
        case M_FLOAT:
            piomb->data = (void *)NEW_N(float, ALLOC_UNITS, "float io mem data");
            pioms->type = M_FLOAT;
            break;
    }
    if ( piomb->data == NULL )
    {
        free(piomb);
        free(pioms->data_buffers);
        free(pioms);
        return NULL;
    }

    pioms->type = datatype;

    return pioms;
}

/*****************************************************************
 * TAG( ios_create_empty ) PRIVATE
 *
 * Allocate an empty I/O memory store.
 */
IO_mem_store *ios_create_empty()
{
    IO_mem_store *pioms = NULL;

    pioms = NEW(IO_mem_store, "IO mem store");
    if ( pioms != NULL )
    {
        pioms->current_index = -1;
    }

    return pioms;
}

/*****************************************************************
 * TAG( ios_input ) PRIVATE
 *
 * Load an I/O memory store from a file.
 */
Return_value ios_input(Mili_family *fam, int datatype, LONGLONG qty_units, IO_mem_store *pioms, void **new_data)
{
    IO_mem_buffer *piomb = NULL;
    LONGLONG rd_cnt;
    LONGLONG index;

    pioms->type = datatype;

    index = ++pioms->current_index;

    pioms->data_buffers = RENEW_N(IO_mem_buffer *, pioms->data_buffers, index, 1, "Load IO buffer list");
    if ( pioms->data_buffers == NULL )
    {
        return ALLOC_FAILED;
    }

    piomb = NEW(IO_mem_buffer, "IO mem buffer");
    if ( piomb == NULL )
    {
        free(pioms->data_buffers);
        return ALLOC_FAILED;
    }
    pioms->data_buffers[index] = piomb;
    piomb->size = qty_units;
    piomb->output = qty_units;

    switch ( datatype )
    {
        case M_STRING:
            piomb->data = (void *)NEW_N(char, qty_units, "char io mem data");
            if ( qty_units > 0 && pioms->data_buffers == NULL )
            {
                free(pioms->data_buffers);
                free(piomb);
                return ALLOC_FAILED;
            }
            rd_cnt = fam->read_funcs[M_STRING](fam->cur_file, piomb->data, qty_units);
            break;
        case M_INT:
            piomb->data = (void *)NEW_N(int, qty_units, "int io mem data");
            if ( qty_units > 0 && pioms->data_buffers == NULL )
            {
                free(pioms->data_buffers);
                free(piomb);
                return ALLOC_FAILED;
            }
            rd_cnt = fam->read_funcs[M_INT](fam->cur_file, piomb->data, qty_units);
            break;
        case M_FLOAT:
            piomb->data = (void *)NEW_N(float, qty_units, "float io mem data");
            if ( qty_units > 0 && pioms->data_buffers == NULL )
            {
                free(pioms->data_buffers);
                free(piomb);
                return ALLOC_FAILED;
            }
            rd_cnt = fam->read_funcs[M_FLOAT](fam->cur_file, piomb->data, qty_units);
            break;
        default:
            if ( mili_verbose )
            {
                fprintf(stderr, "Mili Warning in ios_input--unhandled data type.\n");
            }
            return UNKNOWN_DATA_TYPE; /* Fail status. */
    }

    piomb->used = rd_cnt;

    if ( new_data != NULL )
    {
        *new_data = piomb->data;
    }

    return ((rd_cnt == qty_units) ? OK : SHORT_READ);
}

/*****************************************************************
 * TAG( ti_ios_input ) PRIVATE
 *
 * Load an I/O memory store from a TI file.
 */
Return_value ti_ios_input(Mili_family *fam, int datatype, LONGLONG qty_units, IO_mem_store *pioms, void **new_data)
{
    IO_mem_buffer *piomb = NULL;
    LONGLONG rd_cnt;
    LONGLONG index;

    pioms->type = datatype;

    index = ++pioms->current_index;

    pioms->data_buffers = RENEW_N(IO_mem_buffer *, pioms->data_buffers, index, 1, "Load IO buffer list");
    if ( pioms->data_buffers == NULL )
    {
        return ALLOC_FAILED;
    }

    piomb = NEW(IO_mem_buffer, "IO mem buffer");
    if ( piomb == NULL )
    {
        free(pioms->data_buffers);
        return ALLOC_FAILED;
    }
    pioms->data_buffers[index] = piomb;
    piomb->size = qty_units;
    piomb->output = qty_units;

    switch ( datatype )
    {
        case M_STRING:
            piomb->data = (void *)NEW_N(char, qty_units, "char io mem data");
            if ( qty_units > 0 && pioms->data_buffers == NULL )
            {
                free(pioms->data_buffers);
                free(piomb);
                return ALLOC_FAILED;
            }
            rd_cnt = fam->read_funcs[M_STRING](fam->ti_cur_file, piomb->data, qty_units);
            break;
        case M_INT:
            piomb->data = (void *)NEW_N(int, qty_units, "int io mem data");
            if ( qty_units > 0 && pioms->data_buffers == NULL )
            {
                free(pioms->data_buffers);
                free(piomb);
                return ALLOC_FAILED;
            }
            rd_cnt = fam->read_funcs[M_INT](fam->ti_cur_file, piomb->data, qty_units);
            break;
        case M_FLOAT:
            piomb->data = (void *)NEW_N(float, qty_units, "float io mem data");
            if ( qty_units > 0 && pioms->data_buffers == NULL )
            {
                free(pioms->data_buffers);
                free(piomb);
                return ALLOC_FAILED;
            }
            rd_cnt = fam->read_funcs[M_FLOAT](fam->ti_cur_file, piomb->data, qty_units);
            break;
        default:
            if ( mili_verbose )
            {
                fprintf(stderr, "Mili Warning in ti_ios_input--unhandled data type.\n");
            }
            return UNKNOWN_DATA_TYPE; /* Fail status. */
    }

    piomb->used = rd_cnt;

    if ( new_data != NULL )
    {
        *new_data = piomb->data;
    }

    return ((rd_cnt == qty_units) ? OK : SHORT_READ);
}

/*****************************************************************
 * TAG( ios_destroy ) PRIVATE
 *
 * Free all resources associated with an I/O store.
 */
void ios_destroy(IO_mem_store *pioms)
{
    LONGLONG i;
    IO_mem_buffer *piomb;

    for ( i = 0; i < pioms->current_index + 1; i++ )
    {
        piomb = pioms->data_buffers[i];
        if ( piomb->invalid != NULL )
        {
            Int_range *blist;

            blist = piomb->invalid->blocks;
            DELETE_LIST(blist);
            free(piomb->invalid);
        }
        free(piomb->data);
        free(piomb);
    }

    free(pioms->data_buffers);
    free(pioms);

    return;
}

/*****************************************************************
 * TAG( ios_alloc ) PRIVATE
 *
 * Allocate memory from a store.
 *
 * Parameter "qty" is the number of objects (chars, ints, or
 * floats) requested, not the number of bytes.
 */
void *ios_alloc(LONGLONG qty, IO_mem_store *pioms)
{
    void *mem_address = NULL;
    IO_mem_buffer *piomb = NULL;
    Bool_type alloc_data;
    LONGLONG new_size = 0;

    /* Sanity check. */
    if ( qty == 0 )
    {
        return NULL;
    }

    /* Get current mem buffer. */
    piomb = pioms->data_buffers[pioms->current_index];

    /* If current can't contain new allocation, prepare to get new buffer. */
    if ( piomb->used + qty > piomb->size )
    {
        new_size = ALLOC_UNITS;
        while ( new_size < (LONGLONG)qty )
        {
            new_size += ALLOC_UNITS;
        }

        pioms->data_buffers =
            RENEW_N(IO_mem_buffer *, pioms->data_buffers, pioms->current_index + 1, 1, "Grow IO data buffer list");
        if ( pioms->data_buffers == NULL )
        {
            /* Uh oh; punt. */
            return NULL;
        }
        pioms->current_index++;
        piomb = NEW(IO_mem_buffer, "Additional IO mem buffer");
        if ( piomb == NULL )
        {
            /* Ouch.  We've assigned space to point to a buffer we can't
             * allocate.
             */
            return NULL;
        }
        piomb->size = new_size;
        pioms->data_buffers[pioms->current_index] = piomb;

        alloc_data = TRUE;
    }
    else
    {
        alloc_data = FALSE;
    }

    /* Obtain new buffer if needed and calculate memory address. */
    switch ( pioms->type )
    {
        case M_STRING:
            if ( alloc_data )
            {
                piomb->data = NEW_N(char, new_size, "Char IO buffer");
                if ( piomb->data == NULL )
                {
                    return NULL;
                }
            }
            mem_address = ((void *)((char *)piomb->data + piomb->used));
            break;

        case M_INT:
            if ( alloc_data )
            {
                piomb->data = NEW_N(int, new_size, "Int IO buffer");
                if ( piomb->data == NULL )
                {
                    return NULL;
                }
            }
            mem_address = ((void *)((int *)piomb->data + piomb->used));
            break;

        case M_FLOAT:
            if ( alloc_data )
            {
                piomb->data = NEW_N(float, new_size, "Float IO buffer");
                if ( piomb->data == NULL )
                {
                    return NULL;
                }
            }
            mem_address = ((void *)((float *)piomb->data + piomb->used));
            break;
    }

    /* Track allocation. */
    piomb->used += (LONGLONG)qty;

    return mem_address;
}

/*****************************************************************
 * TAG( ios_unalloc ) PRIVATE
 *
 * Tag previously allocated IO memory as invalid for output.
 *
 * Should probably return a status.
 */
Return_value ios_unalloc(void *pmem, int qty_units, IO_mem_store *pioms)
{
    int unit_size;
    LONGLONG start, stop;
    IO_mem_buffer *piomb = NULL;
    IO_mem_buffer **ppiomb, **bound;
    Return_value rval;

    switch ( pioms->type )
    {
        case M_STRING:
            unit_size = 1;
            break;
        case M_INT:
            unit_size = sizeof(int);
            break;
        case M_FLOAT:
            unit_size = sizeof(float);
            break;
        default:
            if ( mili_verbose )
            {
                fprintf(stderr, "Mili Warning in ios_unalloc--unhandled data type.\n");
            }
            return UNKNOWN_DATA_TYPE; /* Fail status. */
    }

    /*
     * Locate IO mem buffer in which returned data falls.
     *
     * Assume returned block will always lie within one buffer
     * (i.e., app won't "batch" unalloc's and return larger
     * blocks than were allocated).
     */
    bound = pioms->data_buffers + pioms->current_index + 1;
    for ( ppiomb = pioms->data_buffers; ppiomb < bound; ppiomb++ )
    {
        if ( pmem >= (*ppiomb)->data && (char *)pmem < (char *)(*ppiomb)->data + (*ppiomb)->used * unit_size )
        {
            piomb = *ppiomb;
            break;
        }
    }

    if ( ppiomb == bound || piomb == NULL )
    {
        if ( mili_verbose )
        {
            fprintf(stderr, "Mili - Cannot locate IO buffer memory to unallocate.\n");
        }
        return IOS_NO_VIABLE_BUFFER; /* Fail status. */
    }

    /*
     * Refinement - if unallocated memory is at the end of the current
     * memory buffer, it could be re-used.
     */

    /* Check if unallocated memory has already been output. */
    if ( (char *)pmem < (char *)piomb->data + piomb->output * unit_size )
    {
        if ( mili_verbose )
        {
            fprintf(stderr, "Mili Warning - unallocated memory already written.\n");
        }
    }

    /* Calculate unit indices for range of invalid units. */
    start = (int)(((unsigned long)((char *)pmem - (char *)piomb->data)) / unit_size);
    stop = start + qty_units - 1;

    /* Create/extend invalid units list. */
    if ( piomb->invalid == NULL )
    {
        piomb->invalid = NEW(Block_list, "Invalid ios data list");
    }

    rval = insert_range(piomb->invalid, (int)start, (int)stop);
    if ( rval == OBJECT_RANGE_OVERLAP )
    {
        if ( mili_verbose )
        {
            fprintf(stderr,
                    "Mili - Unable to unallocate IO buffer "
                    "memory.\n");
        }
        return IOS_NO_VIABLE_BUFFER; /* Fail status. */
    }

    return OK; /* Success. */
}

/*****************************************************************
 * TAG( ios_str_dup ) PRIVATE
 *
 * Convenience routine - duplicate a string into an I/O store.
 */
int ios_str_dup(char **ppcopy, char *pstring, IO_mem_store *pioms)
{
    LONGLONG len_with_null;
    char *pc, *psrc, *pdest;

    /* Count the string with its terminator. */
    for ( pc = pstring; *pc++; )
        ;
    len_with_null = pc - pstring;

    /* Allocate memory. */
    pc = (char *)ios_alloc(len_with_null, pioms);
    if ( pc == NULL )
    {
        return 0;
    }

    /* Copy the string. */
    psrc = pstring;
    pdest = pc;
    for ( ; (*pdest++ = *psrc++); )
        ;

    /* Pass back the address of the copy. */
    if ( ppcopy != NULL )
    {
        *ppcopy = pc;
    }

    return (int)(len_with_null - 1);
}

/*****************************************************************
 * TAG( ios_output ) PRIVATE
 *
 * Write fresh data in an I/O store to disk.
 */
Return_value ios_output(Mili_family *fam, FILE *ofile, IO_mem_store *pioms, int *num_items_written)
{
    IO_mem_buffer *piomb;
    LONGLONG unit_size, start_unit, i;
    LONGLONG (*ofunc)();
    int dtype;
    LONGLONG write_ct;

    *num_items_written = 0;

    if ( pioms == NULL )
    {
        return OK;
    }

    dtype = pioms->type;

    /* Get unit size. */
    switch ( dtype )
    {
        case M_STRING:
            unit_size = sizeof(char);
            break;
        case M_INT:
            unit_size = sizeof(int);
            break;
        case M_FLOAT:
            unit_size = sizeof(float);
            break;
        default:
            if ( mili_verbose )
            {
                fprintf(stderr, "Mili Warning in ios_output--unhandled data type.\n");
            }
            return UNKNOWN_DATA_TYPE; /* Fail status. */
    }

    /* Get appropriate unit write function. */
    ofunc = fam->write_funcs[dtype];

    for ( i = pioms->current_output_index; i < pioms->current_index + 1; i++ )
    {
        piomb = pioms->data_buffers[i];

        if ( piomb->invalid == NULL )
        {
            /* No invalid units - write all fresh data out. */
            write_ct = (*ofunc)(ofile, (char *)piomb->data + piomb->output * unit_size, piomb->used - piomb->output);
            if ( write_ct != piomb->used - piomb->output )
            {
                return SHORT_WRITE;
            }

            *num_items_written += piomb->used - piomb->output;
        }
        else
        {
            Int_range *pir;

            /*
             * Development note - If we assume an I/O buffer will only
             * be written to disk once, then invalid range blocks can
             * be discarded as they are encountered during output,
             * making processing on subsequent output operations simpler.
             * If a buffer might be written more than once, then the
             * invalid range blocks must be maintained.
             *
             * Assume latter (ios_unalloc() allows invalidation of
             * previously written memory).
             */

            /* Write fresh data segments preceding each invalid range. */
            start_unit = piomb->output;
            for ( pir = piomb->invalid->blocks; pir != NULL; pir = pir->next )
            {
                if ( (LONGLONG)pir->start > start_unit )
                {
                    /* There's fresh data preceding this invalid range. */
                    write_ct = (*ofunc)(ofile, (char *)piomb->data + start_unit * unit_size,
                                        (LONGLONG)pir->start - start_unit);
                    if ( write_ct != pir->start - start_unit )
                    {
                        return SHORT_WRITE;
                    }
                    *num_items_written += pir->start - start_unit;

                    start_unit = pir->stop + 1;
                }
            }

            if ( start_unit < piomb->used )
            {
                /* There's fresh data following the last invalid range. */
                write_ct = (*ofunc)(ofile, (char *)piomb->data + start_unit * unit_size, piomb->used - start_unit);
                if ( write_ct != piomb->used - start_unit )
                {
                    return SHORT_WRITE;
                }
                *num_items_written += piomb->used - start_unit;
            }
        }

        piomb->output = piomb->used;
    }

    pioms->current_output_index = pioms->current_index;

    return OK;
}

/*****************************************************************
 * TAG( ios_get_fresh ) PRIVATE
 *
 * Return the quantity of un-written units in an I/O store.
 */
LONGLONG
ios_get_fresh(IO_mem_store *pioms)
{
    IO_mem_buffer *piomb;
    int i, char_cnt = 0;

    if ( pioms == NULL )
    {
        return 0;
    }

    /* Single I/O Store Buffer */
    if ( pioms->current_index == 0 )
    {
        piomb = pioms->data_buffers[pioms->current_index];
        char_cnt = piomb->used - piomb->output;
    }
    else
    {
        /* Multiple I/O Store Buffers */
        for ( i = 0; i <= pioms->current_index; i++ )
        {
            piomb = pioms->data_buffers[i];
            char_cnt += piomb->used - piomb->output;
        }
    }

    return (char_cnt);
}

#ifdef IOSGETDATA
/*****************************************************************
 * TAG( ios_get_data ) PRIVATE
 *
 * Initialize an empty I/O memory store from a file.
 *
 * INCOMPLETE
 */
void *ios_get_data(IO_mem_store *pioms, int unit_offset)
{
    IO_mem_buffer **ppiomb, **bound;
    IO_mem_buffer *piomb;
    int rem, sum;

    bound = pioms->data_buffers + pioms->current_index + 1;
    rem = unit_offset;
    sum = 0;

    for ( ppiomb = pioms->data_buffers; ppiomb < bound; ppiomb++ )
    {
        piomb = *ppiomb;

        sum += piomb->used;
        if ( rem < sum )
        {
            switch ( pioms->type )
            {
                case M_STRING:
                    break;
                case M_INT:
                    break;
                case M_FLOAT:
                    break;
            }
        }
    }
}
#endif

/*****************************************************************
 * TAG( ios_traverse_init ) PRIVATE
 *
 * Ready an IO store for traversal through its data.  Parameter
 * "last" is a flag, not an index, indicating (when > zero) to
 * start with the last valid buffer in the store.
 */
Return_value ios_traverse_init(IO_mem_store *pioms, int last)
{
    if ( pioms == NULL )
    {
        return NULL_POINTER;
    }
    else if ( last > 0 )
    {
        LONGLONG idx = pioms->current_index;

        pioms->traverse_index = idx;
        if ( pioms->data_buffers[idx] == NULL )
        {
            if ( idx == 0 )
            {
                return IOS_NO_VIABLE_BUFFER;
            }
            idx--;
        }
        pioms->traverse_remain = pioms->data_buffers[idx]->used;
        pioms->traverse_next = (char *)pioms->data_buffers[idx]->data;

        return OK;
    }
    else
    {
        if ( pioms->data_buffers[0] == NULL )
        {
            return IOS_NO_VIABLE_BUFFER;
        }
        pioms->traverse_index = 0;
        pioms->traverse_remain = pioms->data_buffers[0]->used;
        pioms->traverse_next = (char *)pioms->data_buffers[0]->data;

        return OK;
    }
}

/*****************************************************************
 * TAG( ios_int_traverse ) PRIVATE
 *
 * Return the current traversal address of a valid (used) 1D
 * array of integer data of the given size such that the array is
 * completely contained within contiguous valid data of the IO
 * store.
 *
 * Needs to be fixed to account for invalid data in the store.
 */
Return_value ios_int_traverse(IO_mem_store *pioms, LONGLONG size, int **pp_data)
{
    IO_mem_buffer *piomb;

    if ( pioms->traverse_remain >= size )
    {
        pioms->traverse_remain -= size;
        *pp_data = (int *)pioms->traverse_next;
        pioms->traverse_next += size * sizeof(int);
    }
    else if ( pioms->traverse_index < pioms->current_index )
    {
        do
        {
            pioms->traverse_index++;
            piomb = pioms->data_buffers[pioms->traverse_index];
        } while ( (piomb == NULL || /* conceivable but _way_ unlikely */
                   piomb->used < size) &&
                  pioms->traverse_index < pioms->current_index );

        if ( pioms->traverse_index < pioms->current_index )
        {
            if ( piomb->invalid != NULL )
            {
                if ( mili_verbose )
                {
                    fprintf(stderr,
                            "Mili - Invalid data encountered in "
                            "IO Store.\n");
                }
                return IOS_INVALID_DATA;
            }

            if ( piomb->used >= size )
            {
                *pp_data = (int *)piomb->data;
                pioms->traverse_remain = piomb->used - size;
                pioms->traverse_next = (char *)*pp_data + size * sizeof(int);
            }
            else
            {
                return IOS_INVALID_DATA;
            }
        }
        else
        {
            return IOS_INVALID_DATA;
        }
    }
    else
    {
        return IOS_OUT_OF_SYNC;
    }

    return OK;
}

/*****************************************************************
 * TAG( ios_string_traverse ) PRIVATE
 *
 * Return the address of the next valid (used) null-terminated
 * string in a character IO store.
 *
 * Needs to be fixed to account for invalid data in the store.
 */
Return_value ios_string_traverse(IO_mem_store *pioms, char **pp_data)
{
    LONGLONG slen, rem, idx;

    *pp_data = pioms->traverse_next;

    if ( *pp_data != NULL )
    {
        slen = strlen(*pp_data) + 1;
        rem = pioms->traverse_remain;

        if ( slen > rem || slen == 1 )
        {
            if ( mili_verbose )
            {
                fprintf(stderr, "Mili - IO Store traversal out of sync.\n");
            }
            return IOS_OUT_OF_SYNC; /* Error - shouldn't happen if store holds _strings_ */
        }

        if ( slen == rem )
        {
            /* Last string in this buffer. */

            if ( pioms->traverse_index == pioms->current_index )
            {
                /* No more data. */
                pioms->traverse_next = NULL;
            }
            else
            {
                while ( ++(pioms->traverse_index) < pioms->current_index )
                {
                    if ( pioms->data_buffers[pioms->traverse_index] != NULL )
                    {
                        break;
                    }
                }
                idx = pioms->traverse_index;

                if ( idx < pioms->current_index )
                {
                    pioms->traverse_next = (char *)pioms->data_buffers[idx]->data;
                    pioms->traverse_remain = pioms->data_buffers[idx]->size;

                    if ( pioms->data_buffers[idx]->invalid != NULL )
                    {
                        if ( mili_verbose )
                        {
                            fprintf(stderr,
                                    "Mili - Invalid data encountered "
                                    "in IO store.\n");
                        }
                        return IOS_INVALID_DATA;
                    }
                }
                else
                {
                    pioms->traverse_next = NULL;
                    *pp_data = NULL;
                    if ( mili_verbose )
                    {
                        fprintf(stderr,
                                "Mili - NULL buffers encountered"
                                " in IO store.\n");
                    }
                    return IOS_INVALID_DATA;
                }
            }
        }
        else
        {
            pioms->traverse_next += slen;
            pioms->traverse_remain -= slen;
        }
        return OK;
    }
    else
    {
        return IOS_NO_CHAR_BUF;
    }
}
