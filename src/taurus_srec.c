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

 * Routines for managing state records and formats.
 *
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - April 30, 2010: Incorporated changes from Sebastian
 *                at IABG to support building under Windows.
 *                SCR #678.
 *
 ************************************************************************
 */

#include <string.h>
#include <sys/stat.h>
#include "taurus_db.h"
#include "mili_internal.h"

#define RANGE_QTY (100)
#define DFLT_BUF_COUNT 2

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

static Return_value set_metrics(Mili_family *fam, int data_org, Sub_srec *psubrec, LONGLONG *size);
static int make_srec(Mili_family *fam, int mesh_id);
static int svar_atom_qty(Svar *p_svar);
Return_value taurus_commit_srecs(Mili_family *fam);

/*****************************************************************
 * TAG( taurus_make_srec ) PUBLIC
 *
 * Interface to make_srec() which performs QC prior to instantiation.
 */
Return_value taurus_make_srec(Mili_family *fam, int mesh_id, int *p_srec_id)
{
    Mesh_descriptor *mesh;
    Mesh_object_class_data **pp_mocd;
    Mesh_object_class_data *p_mocd;
    int i, class_qty;
    Return_value rval;

    if ( mesh_id >= fam->qty_meshes )
        return NO_MESH;

    mesh = fam->meshes[mesh_id];

    /* Verify that there are no holes in defined node/element classes. */
    rval = htable_get_data(mesh->mesh_data.umesh_data, (void ***)&pp_mocd, &class_qty);
    if ( rval != OK )
    {
        return rval;
    }
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];

        if ( p_mocd->superclass != M_MESH )
            if ( p_mocd->blocks->block_qty != 1 )
            {
                free(pp_mocd);
                return INCOMPLETE_MESH_DEF;
            }
    }
    free(pp_mocd);

    *p_srec_id = make_srec(fam, mesh_id);

    return OK;
}

/*****************************************************************
 * TAG( make_srec ) LOCAL
 *
 * Create an uninitialized state record descriptor.
 */
static int make_srec(Mili_family *fam, int mesh_id)
{
    Srec *p_sr;
    int srec_id;
    /*
     * Grow arrays to hold a pointer to the new srec descriptor
     * and a pointer to its associated mesh descriptor.
     */
    fam->srecs = RENEW_N(Srec *, fam->srecs, fam->qty_srecs, 1, "Srec pointer");
    fam->srec_meshes = RENEW_N(Mesh_descriptor *, fam->srec_meshes, fam->qty_srecs, 1, "Srec mesh pointer");

    /* Allocate the state record descriptor. */
    p_sr = NEW(Srec, "State record format");

    /* Store addresses of the srec descriptor and its mesh descriptor. */
    fam->srecs[fam->qty_srecs] = p_sr;
    fam->srec_meshes[fam->qty_srecs] = fam->meshes[mesh_id];

    /* Ident is just the (previous) count. */
    srec_id = fam->qty_srecs;
    fam->qty_srecs++;

    return srec_id;
}

/*****************************************************************
 * TAG( taurus_def_subrec ) PUBLIC
 *
 * Define a state record sub-record, binding a collection of
 * mesh objects of one type to a set of state variables.
 *
 * Assumes unstructured mesh type (c/o mesh object id checking).
 */
Return_value taurus_def_subrec(Mili_family *fam, int srec_id, char *subrec_name, int org, int qty_svars,
                               char **svar_names, char *mclass, int format, int cell_qty, int *mo_ids, Bool_type first)
{
    Return_value status;
    Srec *psr;
    int *pib, *bound, *psrc, *pdest;
    Sub_srec *psubrec;
    Hash_action op;
    Htable_entry *subrec_entry, *svar_entry, *class_entry;
    int count;
    Svar **svars;
    int i;
    char *sv_name;
    int blk_qty;
    int stype;
    LONGLONG sub_size;
    Mesh_object_class_data *p_mocd;

    if ( fam->qty_meshes == 0 )
        return NO_MESH;
    if ( srec_id < 0 || srec_id >= fam->qty_srecs )
        return INVALID_SREC_INDEX;
    if ( qty_svars == 0 )
        return NO_SVARS;

    /* Create the subrecord hash table if it hasn't been. */
    if ( fam->subrec_table == NULL )
        fam->subrec_table = htable_create(DEFAULT_HASH_TABLE_SIZE);

    /* Create an entry in the hash table. */
    if ( (strncmp(subrec_name, "sand", 4) == 0) )
        op = ENTER_ALWAYS;
    else
        op = ENTER_UNIQUE;

    status = (Return_value)htable_search(fam->subrec_table, subrec_name, op, &subrec_entry);
    if ( status != OK )
        return status;

    /* Got into the hash table, so create the subrec. */
    psubrec = NEW(Sub_srec, "State subrecord");
    str_dup(&psubrec->name, subrec_name);
    psubrec->organization = org;
    psubrec->qty_svars = qty_svars;
    svars = NEW_N(Svar *, qty_svars, "Svar array");
    psubrec->svars = svars;
    subrec_entry->data = (void *)psubrec;

    /* Ensure all the referenced state variables exist. */
    for ( i = 0; i < qty_svars; i++ )
    {
        sv_name = svar_names[i];
        status = (Return_value)htable_search(fam->svar_table, sv_name, FIND_ENTRY, &svar_entry);
        if ( status == OK ) /* Save reference to variable in subrec. */
            svars[i] = (Svar *)svar_entry->data;
        else
        {
            /* Variable not found.  Free all resources and return. */
            delete_subrec((void *)psubrec);
            htable_del_entry(fam->subrec_table, subrec_entry);
            return status;
        }
    }

    if ( org == OBJECT_ORDERED )
    {
        /* For object-ordered data, base types must all be the same. */
        for ( i = 1; i < qty_svars; i++ )
        {
            /* Compare data types of consecutive svars. */
            if ( *svars[i]->data_type != *svars[0]->data_type )
            {
                /* Bad news; clean-up and return. */
                delete_subrec((void *)psubrec);
                htable_del_entry(fam->subrec_table, subrec_entry);
                return TOO_MANY_TYPES;
            }
        }
    }

    /* Get the referenced object class. */
    status =
        (Return_value)htable_search(fam->srec_meshes[srec_id]->mesh_data.umesh_data, mclass, FIND_ENTRY, &class_entry);
    if ( class_entry == NULL )
    {
        delete_subrec((void *)psubrec);
        htable_del_entry(fam->subrec_table, subrec_entry);
        return NO_OBJECT_GROUP;
    }

    p_mocd = (Mesh_object_class_data *)class_entry->data;
    stype = p_mocd->superclass;
    str_dup(&psubrec->mclass, mclass);

    if ( stype != M_MESH )
    {
        /* If format is 1D array of element numbers, put into block format. */
        if ( format == M_LIST_OBJ_FMT )
        {
            status = list_to_blocks(cell_qty, mo_ids, &pib, &blk_qty);
            if ( status != OK )
            {
                return status;
            }
            count = cell_qty;
        }
        else if ( format == M_BLOCK_OBJ_FMT )
        {
            /* Allocate space for the ident blocks in the subrecord. */
            pib = NEW_N(int, cell_qty * 2, "Subrec id blks");

            /* Copy the ident blocks, counting idents. */
            count = 0;
            bound = ((int *)mo_ids) + 2 * cell_qty;
            for ( psrc = (int *)mo_ids, pdest = pib; psrc < bound; pdest += 2, psrc += 2 )
            {
                pdest[0] = psrc[0];
                pdest[1] = psrc[1];
                count += pdest[1] - pdest[0] + 1;
            }

            blk_qty = cell_qty;
        }
        else
        {
            delete_subrec((void *)psubrec);
            htable_del_entry(fam->subrec_table, subrec_entry);
            return UNKNOWN_ELEM_FMT;
        }

        /* Ensure the objects referenced in the id blocks are in the mesh. */
        status = check_object_ids(p_mocd->blocks, blk_qty, pib);
        if ( status != OK )
        {
            delete_subrec((void *)psubrec);
            htable_del_entry(fam->subrec_table, subrec_entry);
            free(pib);
            return status;
        }

        psubrec->qty_id_blks = blk_qty;
        psubrec->mo_id_blks = pib;
        psubrec->mo_qty = count;
    }
    else
    {
        psubrec->mo_qty = 1;
    }

    /* Calculate positioning metrics. */
    status = set_metrics(fam, org, psubrec, &sub_size);
    if ( status != OK )
    {
        return status;
    }

    /* Init input queue for Object-ordered subrecords. */
    if ( org == OBJECT_ORDERED )
    {
        psubrec->ibuffer = create_buffer_queue(DFLT_BUF_COUNT, sub_size);
        if ( psubrec->ibuffer == NULL )
        {
            return ALLOC_FAILED;
        }
    }

    /* Offset to subrecord is previous record size. */
    psr = fam->srecs[srec_id];
    if ( first )
        psr->size += EXT_SIZE(fam, M_FLOAT); /* Time is at start of state */
    psubrec->offset = psr->size;

    /* Update the record size. */
    psr->size += sub_size;

    /* Save a reference in order of its creation. */
    psr->subrecs = RENEW_N(Sub_srec *, psr->subrecs, psr->qty_subrecs, 1, "Srec data order incr");
    psr->subrecs[psr->qty_subrecs] = psubrec;
    psr->qty_subrecs++;

    return (Return_value)OK;
}

/*****************************************************************
 * TAG( set_metrics ) LOCAL
 *
 * Calculate lump sizes and offsets for a subrecord plus the
 * subrecord's total length.
 */
static Return_value set_metrics(Mili_family *fam, int data_org, Sub_srec *psubrec, LONGLONG *ret_size)
{
    int k, l;
    int count;
    LONGLONG size;
    int atom_size;
    int atoms;
    Svar **ppsvar, **bound;
    LONGLONG *lump_offsets, *lump_sizes;
    int *lump_atoms;
    Return_value status = OK;

    /* Skip it if already initialized. */
    if ( psubrec->lump_sizes != NULL )
    {
        if ( data_org == OBJECT_ORDERED )
        {
            size = psubrec->lump_sizes[0] * psubrec->mo_qty;
        }
        else
        {
            k = psubrec->qty_svars - 1;
            size = psubrec->lump_offsets[k] + psubrec->lump_sizes[k];
        }
    }
    else
    {
        /* Calculate lump sizes on basis of gross data organization. */
        if ( data_org == OBJECT_ORDERED )
        {
            /* For object ordering, only one lump size: atom count. */
            psubrec->lump_atoms = NEW(int, "Lump atom array (1)");
            psubrec->lump_sizes = NEW(LONGLONG, "Lump size array (1)");

            count = 0;
            size = 0;

            /* Traverse the state vars in the state vector. */
            atom_size = EXT_SIZE(fam, *psubrec->svars[0]->data_type);
            bound = psubrec->svars + psubrec->qty_svars;
            for ( ppsvar = psubrec->svars; ppsvar < bound; ppsvar++ )
            {
                /*
                 * Only need to distinguish array-valued vs. scalar
                 * since base-type must all be the same for object-ordered
                 * data.
                 */
                atoms = svar_atom_qty(*ppsvar);
                count += atoms;
                size += atoms * atom_size;
            }
            psubrec->lump_atoms[0] = count;
            psubrec->lump_sizes[0] = size;

            /* Calc subrec size. */
            size = psubrec->lump_sizes[0] * psubrec->mo_qty;
        }
        else if ( data_org == RESULT_ORDERED )
        {
            /*
             * For result-ordered data, lump size depends on quantity
             * of mesh objects and whether state var is scalar- or
             * array-valued.
             */

            /* Allocate space for sizes and offsets arrays. */
            psubrec->lump_atoms = NEW_N(int, psubrec->qty_svars, "Lump atom array");
            psubrec->lump_sizes = NEW_N(LONGLONG, psubrec->qty_svars, "Lump size array");
            psubrec->lump_offsets = NEW_N(LONGLONG, psubrec->qty_svars, "Lump offset array");
            lump_offsets = psubrec->lump_offsets;
            lump_sizes = psubrec->lump_sizes;
            lump_atoms = psubrec->lump_atoms;

            /* Traverse the svars and assign lump sizes, atom counts. */
            bound = psubrec->svars + psubrec->qty_svars;
            for ( ppsvar = psubrec->svars, l = 0; ppsvar < bound; l++, ppsvar++ )
            {
                atoms = svar_atom_qty(*ppsvar);
                lump_atoms[l] = psubrec->mo_qty * atoms;
                lump_sizes[l] = lump_atoms[l] * EXT_SIZE(fam, *(*ppsvar)->data_type);
            }

            /* Calculate offsets to each lump to speed I/O later. */
            lump_offsets[0] = 0;
            for ( k = 1; k < psubrec->qty_svars; k++ )
            {
                lump_offsets[k] = lump_offsets[k - 1] + lump_sizes[k - 1];
            }

            k = psubrec->qty_svars - 1;

            /* Calc subrec size. */
            size = lump_offsets[k] + lump_sizes[k];
        }
        else
        {
            size = 0;
            fprintf(stderr, "Error in set_metrics--unknown data organization");
            status = NOT_OK;
        }
    }
    *ret_size = size;
    return status;
}

/*****************************************************************
 * TAG( svar_atom_qty ) LOCAL
 *
 * Calculate the quantity of atoms in a state variable.
 */
static int svar_atom_qty(Svar *p_svar)
{
    int qty;
    int k;

    qty = 1;

    if ( *p_svar->agg_type == VECTOR )
    {
        qty = *p_svar->list_size;
    }
    else if ( *p_svar->agg_type == ARRAY || *p_svar->agg_type == VEC_ARRAY )
    {
        for ( k = 0; k < *p_svar->order; k++ )
            qty *= p_svar->dims[k];

        if ( *p_svar->agg_type == VEC_ARRAY )
            qty *= *p_svar->list_size;
    }

    return qty;
}

/*****************************************************************
 * TAG( taurus_get_subrec_def ) PUBLIC
 *
 * Copy subrecord definition parameters to application struct.
 */
Return_value taurus_get_subrec_def(Famid fam_id, int srec_id, int subrec_id, Subrecord *p_subrec)
{
    Sub_srec *p_ssrec;
    int id_cnt;
    int *p_isrc, *p_idest, *bound;
    int mo_qty, blk_qty;
    Mili_family *fam;
    Htable_entry *p_hte;
    int i;
    int superclass;

    fam = fam_list[fam_id];

    p_ssrec = fam->srecs[srec_id]->subrecs[subrec_id];

    str_dup(&p_subrec->name, p_ssrec->name);
    str_dup(&p_subrec->class_name, p_ssrec->mclass);
    p_subrec->organization = p_ssrec->organization;
    p_subrec->qty_svars = p_ssrec->qty_svars;
    p_subrec->svar_names = NEW_N(char *, p_subrec->qty_svars, "Subrecord svar name ptrs");
    for ( i = 0; i < p_subrec->qty_svars; i++ )
        str_dup(p_subrec->svar_names + i, p_ssrec->svars[i]->name);

    /* Get the superclass. */
    htable_search(fam->srec_meshes[srec_id]->mesh_data.umesh_data, p_ssrec->mclass, FIND_ENTRY, &p_hte);
    if ( p_hte == NULL )
    {
        return NOT_OK;
    }
    superclass = ((Mesh_object_class_data *)p_hte->data)->superclass;

    /*
     * Currently, M_MESH subrecords are treated specially and don't record
     * mesh objects in blocks.  However, the Subrecord struct implies
     * that the information will exist, so we'll do (more) special processing
     * now to handle the M_MESH subrecord case.  The correct solution is
     * to stop treating M_MESH subrecords differently when they are defined
     * by an mc_def_subrec() call or when opening an existing db, but the
     * quick attempt at this failed for unknown reasons, so here we are...
     */

    if ( superclass != M_MESH )
    {
        p_subrec->qty_blocks = p_ssrec->qty_id_blks;
        id_cnt = 2 * p_subrec->qty_blocks;
        p_subrec->mo_blocks = NEW_N(int, id_cnt, "Subrecord mo ident blocks");
        bound = p_ssrec->mo_id_blks + id_cnt;
        mo_qty = 0;
        for ( p_isrc = p_ssrec->mo_id_blks, p_idest = p_subrec->mo_blocks; p_isrc < bound; p_idest += 2, p_isrc += 2 )
        {
            /* Copy block. */
            *p_idest = *p_isrc;
            p_idest[1] = p_isrc[1];

            /* Block size (allow for decreasing idents). */
            blk_qty = p_isrc[1] - p_isrc[0] + 1;
            if ( p_isrc[1] < p_isrc[0] )
                blk_qty = -(blk_qty - 1) + 1;
            mo_qty += blk_qty;
        }

        p_subrec->qty_objects = mo_qty;
    }
    else
    {
        p_subrec->qty_blocks = 1;
        p_subrec->mo_blocks = NEW_N(int, 2, "Subrecord mo ident block");
        p_subrec->mo_blocks[0] = p_subrec->mo_blocks[1] = 1;
        p_subrec->qty_objects = p_ssrec->mo_qty;
    }

    return (Return_value)OK;
}

/*****************************************************************
 * TAG( taurus_get_states_per_file )
 *
 * Open a family of MDG plot files.  Returns -1 if unsuccessful,
 * otherwise returns the number of states in the family.  Note
 * that if the plot file has valid geometry but no states, the
 * routine returns 0.
 */
int taurus_get_states_per_file(Mili_family *fam, int ctl[], int *file_sz)
{
    struct stat statbuf;
    int num_files = 0, geom_sz = 0, state_sz = 0, num_states = 0;
    int sum = 0, sumsav = 0, max_st = 0;
    int ndim = 0, numnp = 0, icode = 0, nglbv = 0, it = 0, iu = 0, iv = 0, ia = 0, ixd = 0, xnd = 0, nvqty = 0;
    int nel8 = 0, nv3d = 0, nel2 = 0, nv1d = 0;
    int nel4 = 0, nv2d = 0, activ = 0;
    int nv3dact = 0, nv2dact = 0, nv1dact = 0;
    char fname[128];
    int i = 0;

    /*
     * Loop to get the number of files.  Then get the length of each
     * file.
     */
    i = 0;
    make_fnam(TAURUS_DATA, fam, i, fname);
    while ( stat(fname, &statbuf) != -1 )
    {
        i++;
        make_fnam(TAURUS_DATA, fam, i, fname);
    }
    num_files = i;

    for ( i = 0; i < num_files; i++ )
    {
        make_fnam(TAURUS_DATA, fam, i, fname);
        stat(fname, &statbuf);
        file_sz[i] = (int)statbuf.st_size;
    }

    if ( ctl[0] == 4 )
    {
        ctl[0] = 3;
    }
    else

        ndim = ctl[0];
    numnp = ctl[1];
    icode = ctl[2];
    nglbv = ctl[3];
    it = ctl[4];
    iu = ctl[5];
    iv = ctl[6];
    ia = ctl[7];
    nel8 = ctl[8];
    nv3d = ctl[12];
    nel2 = ctl[13];
    nv1d = ctl[15];
    nel4 = ctl[16];
    nv2d = ctl[18];
    activ = ctl[20];
    ixd = ctl[24];

    /* If activity data present, include it with the rest of the variables. */
    nv1dact = nv1d;
    nv2dact = nv2d;
    nv3dact = nv3d;
    if ( activ >= 1000 && activ <= 1005 )
    {
        if ( nel2 > 0 )
            nv1dact++;
        if ( nel4 > 0 )
            nv2dact++;
        if ( nel8 > 0 )
            nv3dact++;
    }

    /* Check extra data flags to account for additional data. */
    xnd = (ixd & K_EPSILON_MASK) ? 2 : 0; /* Two add'l var's if flag set. */
    xnd += (ixd & A2_MASK) ? 1 : 0;       /* One add'l var if flag set. */

    /*
     * Calculate number of nodal variable fields in state record.
     * Assume A2 will never be present without k and epsilon.
     */
    nvqty = ((icode == 1) ? 1 : it) + xnd + ndim * (iu + iv + ia);

    geom_sz = (ndim * numnp + 9 * nel8 + 5 * nel4 + 6 * nel2) * EXT_SIZE(fam, M_FLOAT);
    state_sz = (nvqty * numnp + nel8 * nv3dact + nel4 * nv2dact + nel2 * nv1dact + nglbv + 1) * EXT_SIZE(fam, M_FLOAT);

    /*
     * Calculate the maximum possible number of states.  When(ever) we can
     * forget IRIX 4.x compatibility, type sum as "long long" (64 bit) and
     * go back to previous code for max_st calculation to avoid overflow
     * checking.
     */
    max_st = 0;
    for ( i = 0, sum = -geom_sz; i < num_files; i++ )
    {
        sumsav = sum;
        sum += file_sz[i];
        if ( sum < sumsav ) /* oops, overflow */
        {
            max_st += sumsav / state_sz;
            sum = sumsav % state_sz + file_sz[i];
        }
    }
    max_st += sum / state_sz;
    num_states = max_st / num_files;

    fam->st_file_count = num_files;
    fam->states_per_file = num_states;
    fam->state_qty = max_st;

    return state_sz;
}

/*****************************************************************
 * TAG( taurus_build_state_map ) PRIVATE
 *
 * Build file/offset map of state-data records in family.
 */
Return_value taurus_build_state_map(Mili_family *fam, Bool_type initial_build, int ctl[], int srec_id)
{
    State_descriptor **p_smap;
    int index, end_geom_idx;
    int ndim, numnp, nel8, nel4, nel2;
    LONGLONG offset;
    LONGLONG (*readf)();
    float st_time;
    int state_qty;
    State_descriptor *p_sd;
    int *file_sz;
    int state_size;
    long remainder;

    ndim = ctl[0];
    numnp = ctl[1];
    nel8 = ctl[8];
    nel2 = ctl[13];
    nel4 = ctl[16];

    p_smap = &fam->state_map;
    readf = fam->state_read_funcs[M_FLOAT];
    if ( initial_build )
    {
        index = 0;
        state_qty = 0;
        /* We will be in trouble if there are more than 10000 files. */
        file_sz = NEW_N(int, 10000, "File_sz array");
        taurus_get_states_per_file(fam, ctl, file_sz);
        /*  Take into account the geometry data in the first file */
        offset = 64 * EXT_SIZE(fam, M_INT) + (ndim * numnp + 9 * nel8 + 5 * nel4 + 6 * nel2) * EXT_SIZE(fam, M_FLOAT);

        /* Loop through the files to get the first file with
         * actual state information
         */
        while ( index < fam->st_file_count && offset >= (LONGLONG)file_sz[index] )
        {
            offset = offset - file_sz[index];
            index++;
        }

        /* Something's wrong if we traversed all the files. */
        if ( index == fam->st_file_count )
        {
            free(file_sz);
            return TAURUS_INCOMPLETE_GEOM;
        }
        else
            end_geom_idx = index;
    }
    else
    {
        index = fam->st_file_count - 1;
        state_qty = fam->state_qty;

        /*  Take into account the geometry data in the first file */
        offset = 64 * EXT_SIZE(fam, M_INT) + (ndim * numnp + 9 * nel8 + 5 * nel4 + 6 * nel2) * EXT_SIZE(fam, M_FLOAT);
        /* Count offset through mapped states in known last file. */
        for ( p_sd = fam->state_map + fam->state_qty - 1;
              ((LONGLONG)p_sd >= (LONGLONG)fam->state_map) && p_sd->file == index; p_sd-- )
            offset += fam->srecs[p_sd->srec_format]->size;
    }

    /* Traverse the state data files. */
    state_size = (int)fam->srecs[srec_id]->size;
    state_qty = 0;
    st_time = 0.0;
    while ( state_file_open(fam, index, 'r') == OK )
    {
        /* Traverse the state records within the current file. */
        while ( (offset == 0 || fam->cur_st_file_size - offset >= (LONGLONG)state_size) &&
                seek_state_file(fam->cur_st_file, offset) == OK )
        {
            /* Read the time. */
            if ( readf(fam->cur_st_file, &st_time, 1) != 1 )
                break;

            /* End of data in file? */
            if ( st_time < 0 )
                break;

            /* Add a new entry in the state map. */
            *p_smap = RENEW_N(State_descriptor, *p_smap, state_qty, 1, "Addl state descr");

            (*p_smap)[state_qty].file = index;
            (*p_smap)[state_qty].offset = offset;
            (*p_smap)[state_qty].time = st_time;
            (*p_smap)[state_qty].srec_format = srec_id;

            state_qty++;
            fam->state_qty = state_qty;
            fam->file_st_qty++;

            offset += state_size;
        }

        /* Prepare for next file... */
        if ( st_time < 0 || (LONGLONG)state_size <= fam->cur_st_file_size || index == end_geom_idx )
        {
            /* End of valid data in file; try next. */
            state_file_close(fam);
            offset = 0;
            index++;
        }
        else
        {
            /*
             * State must straddle files.  Loop through files to get past
             * rest of state.
             */

            remainder = (int)((LONGLONG)state_size - fam->cur_st_file_size);

            do
            {
                state_file_close(fam);
                index++;
            } while ( state_file_open(fam, index, 'r') == OK && (remainder -= fam->cur_st_file_size) > 0 );

            if ( remainder <= 0 )
            {
                /*
                 * Last file with a partial state will never start a new state.
                 */
                state_file_close(fam);
                index++;
                offset = 0;
            }
        }
    }

    for ( state_qty = 0; state_qty < fam->state_qty; state_qty++ )
    {
        (*p_smap)[state_qty].offset -= (EXT_SIZE(fam, M_INT) + EXT_SIZE(fam, M_FLOAT));
    }

    fam->st_file_count = index;

    if ( initial_build )
    {
        free(file_sz);
    }

    /*
     * Always return OK because there don't have to be any states,
     * although this will mask an inability to read states that do exist.
     */
    return OK;
}

/*****************************************************************
 * TAG( taurus_commit_srecs ) PRIVATE
 *
 *
 */
Return_value taurus_commit_srecs(Mili_family *fam)
{
    int i_qty, c_qty;
    int i, j, k;
    Srec *psrec;
    Sub_srec *psubrec;
    Svar **ppsvars;
    Return_value rval;

    for ( i = fam->commit_max + 1; i < fam->qty_srecs; i++ )
    {
        /* Srec_id, mesh_id, size, qty_subrecs */
        i_qty = QTY_SREC_HEADER_FIELDS;
        c_qty = 0;
        psrec = fam->srecs[i];

        /* Traverse the subrecords and count data. */
        for ( j = 0; j < psrec->qty_subrecs; j++ )
        {
            psubrec = psrec->subrecs[j];

            i_qty += 3;                        /* org, qty_svars, qty_id_blks */
            i_qty += 2 * psubrec->qty_id_blks; /* id_blks */

            c_qty += (int)strlen(psubrec->name) + 1;   /* subrec name */
            c_qty += (int)strlen(psubrec->mclass) + 1; /* class name */

            /* Traverse the state variables to count name lengths. */
            ppsvars = psubrec->svars;
            for ( k = 0; k < psubrec->qty_svars; k++ )
                c_qty += (int)strlen(ppsvars[k]->name) + 1; /* svar name. */
        }

        psrec->status = OBJ_SAVED;

        fam->commit_max = fam->qty_srecs - 1;
    }

    rval = add_dir_entry(fam, STATE_REC_DATA, i_qty, c_qty, 0, NULL, (LONGLONG)DONT_CARE, (LONGLONG)DONT_CARE);

    return rval;
}
