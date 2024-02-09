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

 * Utility routines for libtaurus.
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
#include <ctype.h>
#include "mili_internal.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Declared external functions to avoid compiler warnings */
extern int taurus_count_node_entries(Mili_family *, int, char *);
extern Return_value taurus_get_class_qty(Mili_family *, int *, int *);
extern Return_value taurus_get_elem_qty_in_def(Mili_family *, int *, char *, int *);
extern Return_value taurus_get_node_block_size(Mili_family *, int *, char *, int *);
extern int taurus_count_elem_conn_defs(Mili_family *, int, char *);

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

void to_base26(int num, Bool_type upper, char *base26_rep);

/*****************************************************************
 * TAG( taurus_query_family ) PUBLIC
 *
 * Request information about a family.
 */
int taurus_query_family(Famid fam_id, int which, void *num_args, char *char_args, void *p_info)
{
    Mili_family *fam;
    int rval, status;
    int blk_size;
    int sclass_size;
    int elem_qty;
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
    float *p_f;
    int mesh_id;
    int state_no, max_st;
    int idx;

    if ( validate_fam_id(fam_id) == OK )
        fam = fam_list[fam_id];
    else
        return (int)BAD_FAMILY;

    rval = OK;

    /* Most queries expect integer numeric arguments. */
    int_args = (int *)num_args;

    switch ( (Query_request_type)which )
    {
        case QTY_STATES:
            if ( fam->active_family )
                update_active_family(fam);
            *((int *)p_info) = fam->state_qty;
            break;
        case QTY_DIMENSIONS:
            *((int *)p_info) = fam->dimensions;
            break;
        case QTY_MESHES:
            *((int *)p_info) = fam->qty_meshes;
            break;
        case QTY_SREC_FMTS:
            *((int *)p_info) = fam->qty_srecs;
            break;
        case QTY_SUBRECS:
            srec_idx = *int_args;
            if ( srec_idx < 0 || srec_idx > fam->qty_srecs - 1 )
                rval = (int)INVALID_SREC_INDEX;
            else
                *((int *)p_info) = fam->srecs[*int_args]->qty_subrecs;
            break;
        case QTY_SUBREC_SVARS:
            srec_idx = int_args[0];
            if ( srec_idx < 0 || srec_idx > fam->qty_srecs - 1 )
                rval = (int)INVALID_SREC_INDEX;
            else
            {
                subrec_idx = int_args[1];
                if ( subrec_idx < 0 || subrec_idx > fam->srecs[srec_idx]->qty_subrecs - 1 )
                    rval = (int)INVALID_SUBREC_INDEX;
                else
                {
                    p_ssr = fam->srecs[srec_idx]->subrecs[subrec_idx];
                    *((int *)p_info) = p_ssr->qty_svars;
                }
            }
            break;
        case QTY_SVARS:
            if ( fam->svar_table != NULL )
                *((int *)p_info) = fam->svar_table->qty_entries;
            else
                *((int *)p_info) = 0;
            break;
        case QTY_NODE_BLKS:
            *((int *)p_info) = taurus_count_node_entries(fam, *int_args, char_args);
            break;
        case QTY_NODES_IN_BLK:
            rval = taurus_get_node_block_size(fam, int_args, char_args, &blk_size);
            if ( rval == OK )
                *((int *)p_info) = blk_size;
            break;
        case QTY_CLASS_IN_SCLASS:
            rval = taurus_get_class_qty(fam, int_args, &sclass_size);
            if ( rval == OK )
            {
                *((int *)p_info) = sclass_size;
            }
            break;
        case QTY_ELEM_CONN_DEFS:
            *((int *)p_info) = taurus_count_elem_conn_defs(fam, *int_args, char_args);
            break;
        case QTY_ELEMS_IN_DEF:
            rval = taurus_get_elem_qty_in_def(fam, int_args, char_args, &elem_qty);
            if ( rval == OK )
                *((int *)p_info) = elem_qty;
            break;
        case SREC_FMT_ID:
            reqd_st = *int_args;
            if ( fam->active_family && reqd_st > fam->state_qty )
                update_active_family(fam);
            if ( reqd_st < 1 || reqd_st > fam->state_qty )
                rval = INVALID_STATE;
            else
                *((int *)p_info) = fam->state_map[reqd_st - 1].srec_format;
            break;
        case SUBREC_CLASS:
            srec_idx = int_args[0];
            if ( srec_idx < 0 || srec_idx > fam->qty_srecs - 1 )
                rval = (int)INVALID_SREC_INDEX;
            else
            {
                subrec_idx = int_args[1];
                if ( subrec_idx < 0 || subrec_idx > fam->srecs[srec_idx]->qty_subrecs - 1 )
                    rval = (int)INVALID_SUBREC_INDEX;
                else
                {
                    p_ssr = fam->srecs[srec_idx]->subrecs[subrec_idx];
                    strcpy(p_info, p_ssr->mclass);
                }
            }
            break;
        case SREC_MESH:
            srec_idx = int_args[0];
            if ( srec_idx < 0 || srec_idx > fam->qty_srecs - 1 )
                rval = (int)INVALID_SREC_INDEX;
            else
            {
                p_md = fam->srec_meshes[srec_idx];
                for ( i = 0; i < fam->qty_meshes; i++ )
                    if ( p_md == fam->meshes[i] )
                        break;
                *((int *)p_info) = i;
            }
            break;
        case CLASS_SUPERCLASS:
            mesh_id = int_args[0];
            if ( mesh_id > fam->qty_meshes - 1 )
                rval = NO_MESH;
            else
            {
                p_ht = fam->meshes[mesh_id]->mesh_data.umesh_data;
                status = htable_search(p_ht, char_args, FIND_ENTRY, &p_hte);
                if ( status == OK )
                    *((int *)p_info) = ((Mesh_object_class_data *)p_hte->data)->superclass;
                else
                    rval = status;
            }
            break;
        case STATE_TIME:
            state_no = int_args[0];
            if ( state_no < 1 || state_no > fam->state_qty )
                rval = INVALID_STATE;
            else
                *((float *)p_info) = fam->state_map[state_no - 1].time;
            break;
        case SERIES_TIMES:
            state_no = int_args[0];
            max_st = int_args[1];
            if ( state_no < 1 || state_no > fam->state_qty || max_st < 1 || max_st > fam->state_qty )
            {
                rval = INVALID_STATE;
                break;
            }
            state_no--;
            max_st--;
            p_f = (float *)p_info;
            add = ((max_st - state_no & 0x80000000) == 0) ? 1 : -1;
            for ( i = state_no, j = 0; i != max_st; i += add, j++ )
                p_f[j] = fam->state_map[i].time;
            p_f[j] = fam->state_map[i].time;
            break;
        case MULTIPLE_TIMES:
            qty = int_args[0];
            for ( i = 1; i <= qty; i++ )
                if ( int_args[i] < 1 || int_args[i] > fam->state_qty )
                {
                    rval = INVALID_STATE;
                    break;
                }
            if ( rval == OK )
            {
                p_f = (float *)p_info;
                for ( i = 1; i <= qty; i++ )
                    p_f[i - 1] = fam->state_map[int_args[i] - 1].time;
            }
            break;
        case SERIES_SREC_FMTS:
            for ( i = 0; i < 2; i++ )
                if ( int_args[i] < 1 || int_args[i] > fam->state_qty )
                {
                    rval = INVALID_STATE;
                    break;
                }
            if ( rval == OK )
            {
                p_i = (int *)p_info;
                max_st = int_args[1];
                for ( i = int_args[0] - 1, idx = 0; i < max_st; i++ )
                    p_i[idx++] = fam->state_map[i].srec_format;
            }
            break;
        case STATE_OF_TIME:
            time = ((float *)num_args)[0];
            smap = fam->state_map;
            qty = fam->state_qty;
            p_i = (int *)p_info;
            i = 0;
            while ( time > smap[i].time && i < qty )
                i++;
            if ( time == smap[i].time )
            {
                p_i[0] = p_i[1] = i + 1;
            }
            else if ( i == 0 || i == qty )
            {
                rval = INVALID_TIME;
            }
            else
            {
                p_i[0] = i;
                p_i[1] = i + 1;
            }
            break;
        default:
            rval = UNKNOWN_QUERY_TYPE;
            break;
    }

    return (int)rval;
}

/*****************************************************************
 * TAG( taurus_non_state_file_close ) PRIVATE
 *
 * Open a family file.
 */
Return_value taurus_non_state_file_close(Mili_family *fam, Famid fam_id)
{
    if ( fam->cur_file != NULL )
    {
        fclose(fam->cur_file);
        fam->cur_file = NULL;
        fam->cur_index = -1;
    }

    return OK;
}
