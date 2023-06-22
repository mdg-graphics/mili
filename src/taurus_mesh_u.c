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

 * Routines for managing unstructured mesh geometry.
 *
 */

#include <string.h>
#include "mili_internal.h"
#include "taurus_db.h"

#define QTY_NODE_TAGS (2)

#define OVERLAP(min, max, tmin, tmax) (!(tmax < min || tmin > max))

extern int taurus_class_data_mesh_id = -1;
extern Return_value taurus_non_state_file_open(Mili_family *fam, int index, char mode);
extern Return_value taurus_reset_class_data(Mili_family *, int, Bool_type);

static Mili_family *taurus_class_data_fam = NULL;
static int taurus_all_class_count;
static Mesh_object_class_data **taurus_class_data = NULL;
static Return_value add_geom(Mesh_descriptor *mesh, char *short_name, int start_mo, int stop_mo);

static Return_value taurus_load_nodes_def(Mili_family *fam, int file_idx, Dir_entry dir_ent, void *coords);
static Return_value taurus_load_conn_def(Mili_family *fam, int *file_idx, int superclass, Dir_entry dir_ent,
                                         void *conns, int *p_max_mat);
static int taurus_get_max_material(Famid fam_id, int mesh_id, char **short_name);
static void taurus_delete_mo_data(void *ptr_mo_data);

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

/*****************************************************************
 * TAG( taurus_make_umesh ) PUBLIC
 *
 * Perform error checking prior to creating a mesh.
 */
Return_value taurus_make_umesh(Famid fam_id, char *mesh_name, int dim, int *p_mesh_id)
{
    Mili_family *fam;
    int i;
    LONGLONG outbytes;
    int fidx;
    LONGLONG offset;
    char *mname;
    char pname[32];
    Param_ref *ppr;
    Htable_entry *phte;
    Mesh_descriptor *pmd;
    char *mesh_type = "mesh type";
    char *mesh_dims = "mesh dimensions";
    Return_value rval;

    fam = fam_list[fam_id];

    if ( dim != 2 && dim != 3 )
        return INVALID_DIMENSIONALITY;
    else if ( dim != fam->dimensions && fam->dimensions != 0 )
        return INVALID_DIMENSIONALITY;

    mname = mesh_name ? mesh_name : "";

    for ( i = 0; i < fam->qty_meshes; i++ )
        if ( strcmp(fam->meshes[i]->name, mname) == 0 )
            return NAME_CONFLICT;

    if ( fam->mesh_type == UNDEFINED )
    {
        fam->mesh_type = UNSTRUCTURED;
        outbytes = EXT_SIZE(fam, M_INT);
        rval = add_dir_entry(fam, MILI_PARAM, M_INT, SCALAR, 1, &mesh_type, fam->next_free_byte, outbytes);
        if ( rval != OK )
            return rval;

        /* Now create entry for the param table. */
        rval = htable_search(fam->param_table, "mesh type", ENTER_ALWAYS, &phte);
        if ( phte == NULL )
        {
            return rval;
        }
        ppr = NEW(Param_ref, "Param table entry - string");
        fidx = 0;
        ppr->file_index = fidx;
        ppr->entry_index = fam->directory[fidx].qty_entries - 1;
        phte->data = (void *)ppr;
    }
    else if ( fam->mesh_type != UNSTRUCTURED )
        return MESH_TYPE_CONFLICT;

    if ( fam->dimensions == 0 )
    {
        fam->dimensions = dim;
        offset = fam->next_free_byte;
        outbytes = EXT_SIZE(fam, M_INT);
        rval = add_dir_entry(fam, MILI_PARAM, M_INT, SCALAR, 1, &mesh_dims, offset, outbytes);
        if ( rval != OK )
            return rval;

        /* Now create entry for the param table. */
        rval = htable_search(fam->param_table, "mesh dimensions", ENTER_ALWAYS, &phte);
        if ( phte == NULL )
        {
            return rval;
        }
        ppr = NEW(Param_ref, "Param table entry - string");
        fidx = fam->cur_index;
        ppr->file_index = fidx;
        ppr->entry_index = fam->directory[0].qty_entries - 1;
        phte->data = (void *)ppr;
        fam->next_free_byte += 49 * sizeof(int);
        ;
    }

    fam->meshes = RENEW_N(Mesh_descriptor *, fam->meshes, fam->qty_meshes, 1, "Mesh pointer");

    pmd = NEW(Mesh_descriptor, "Mesh data");
    str_dup(&pmd->name, mesh_name);
    fam->meshes[fam->qty_meshes] = pmd;

    sprintf(pname, "mesh name %d", fam->qty_meshes);
    str_dup(&mname, pname);
    /* Add directory entry. Modifier is Mili data type. */
    rval = add_dir_entry(fam, MILI_PARAM, M_STRING, DONT_CARE, 1, &mname, DONT_CARE, strlen(mesh_name) + 1);
    if ( rval != OK )
        return rval;

    free(mname);

    /*
     * Create the geometry table and insert an entry for global data
     * since there will be no add_geom() call for global data.
     */
    pmd->mesh_data.umesh_data = htable_create(DEFAULT_HASH_TABLE_SIZE);
    *p_mesh_id = fam->qty_meshes;
    fam->qty_meshes++;
    return (Return_value)OK;
}

/*****************************************************************
 * TAG( taurus_def_class ) PUBLIC
 *
 * Create an entry in the geometry table for a new mesh object
 * class.
 */
Return_value taurus_def_class(Famid fam_id, int mesh_id, int superclass, char *short_name, char *long_name)
{
    Mili_family *fam;
    Hash_table *p_ht;
    Htable_entry *p_hte;
    Return_value rval;
    char *names[2];

    fam = fam_list[fam_id];

    if ( mesh_id > fam->qty_meshes - 1 )
        return NO_MESH;
    else
        p_ht = fam->meshes[mesh_id]->mesh_data.umesh_data;

    /* Enter the class in the table. */
    rval = (Return_value)htable_search(p_ht, short_name, ENTER_UNIQUE, &p_hte);

    if ( rval != OK )
        return rval;

    /* Create/init object class struct. */
    rval = create_class_data(superclass, short_name, long_name, (Mesh_object_class_data **)&p_hte->data);

    /* Create directory entry.  A class def has only directory data. */
    if ( rval == OK )
    {
        names[0] = short_name;
        names[1] = long_name;
        rval = add_dir_entry(fam, CLASS_DEF, mesh_id, superclass, 2, names, (LONGLONG)DONT_CARE, (LONGLONG)DONT_CARE);
    }

    return rval;
}

/*****************************************************************
 * TAG( taurus_def_class_idents ) PUBLIC
 *
 * Define object identifiers for objects in a particular class.
 * The class should be derived only from one of the superclasses
 * which represent objects of which there may be more than one
 * and which are not otherwise defined in another mc_def...()
 * call (as of this writing, M_UNIT or M_MAT).
 */
Return_value taurus_def_class_idents(Famid fam_id, int mesh_id, char *short_name, int start, int stop)
{
    LONGLONG rec_size;
    Mesh_descriptor *mesh;
    Mili_family *fam;
    Return_value rval;

    fam = fam_list[fam_id];

    if ( mesh_id > fam->qty_meshes - 1 )
        return NO_MESH;
    else
        mesh = fam->meshes[mesh_id];

    /* Keep track of objects defined. */
    rval = add_geom(mesh, short_name, start, stop);
    if ( rval != OK )
        return rval;

    /* Calculate the external record size. */
    rec_size = QTY_IDENT_ENTRY_FIELDS * EXT_SIZE(fam, M_INT);

    /* Register an entry in the directory. */
    rval = add_dir_entry(fam, CLASS_IDENTS, mesh_id, stop - start + 1, 1, &short_name, fam->next_free_byte, rec_size);

    return rval;
}

/*****************************************************************
 * TAG( taurus_def_nodes ) PUBLIC
 *
 * Define coordinates for a continuously-numbered sequence of nodes.
 */
Return_value taurus_def_nodes(Famid fam_id, int mesh_id, char *short_name, int start_node, int stop_node)
{
    LONGLONG data_words;
    Mesh_descriptor *mesh;
    LONGLONG offset, rec_size;
    Mili_family *fam;
    Return_value rval;
    Hash_table *p_ht;
    Htable_entry *p_hte;
    int superclass;

    fam = fam_list[fam_id];

    if ( mesh_id > fam->qty_meshes - 1 )
        return NO_MESH;
    else
        mesh = fam->meshes[mesh_id];

    /* Verify class is derived from M_NODE superclass. */
    p_ht = fam->meshes[mesh_id]->mesh_data.umesh_data;
    rval = (Return_value)htable_search(p_ht, short_name, FIND_ENTRY, &p_hte);
    if ( rval == OK )
        superclass = ((Mesh_object_class_data *)p_hte->data)->superclass;
    else
        return rval;

    if ( superclass != M_NODE )
        return SUPERCLASS_CONFLICT;

    data_words = (stop_node - start_node + 1) * fam->dimensions;
    rec_size = data_words * EXT_SIZE(fam, M_FLOAT);

    /* Keep track of nodes defined. */
    rval = add_geom(mesh, short_name, start_node, stop_node);
    if ( rval != OK )
        return rval;

    /* Register an entry in the directory. */
    offset = fam->next_free_byte;
    rval = add_dir_entry(fam, NODES, mesh_id, stop_node - start_node + 1, 1, &short_name, offset, rec_size);
    if ( rval != OK )
        return rval;

    fam->next_free_byte += rec_size;

    return (Return_value)OK;
}

/*****************************************************************
 * TAG( taurus_def_conn_seq ) PUBLIC
 *
 * Define connectivities, materials _?_, and part numbers_?_ for a
 * globally numbered sequential list of elements of a given type.
 */
Return_value taurus_def_conn_seq(Mili_family *fam, int mesh_id, char *short_name, int start_el, int stop_el)
{
    LONGLONG rec_size, data_words;
    Mesh_descriptor *mesh;
    Return_value rval;
    Hash_table *p_ht;
    Htable_entry *p_hte;
    int superclass;

    if ( mesh_id > fam->qty_meshes - 1 )
        return NO_MESH;
    else
        mesh = fam->meshes[mesh_id];

    /* Keep track of elements defined. */
    rval = add_geom(mesh, short_name, start_el, stop_el);
    if ( rval != OK )
        return rval;

    /* Calculate the external record size. */
    p_ht = fam->meshes[mesh_id]->mesh_data.umesh_data;
    rval = (Return_value)htable_search(p_ht, short_name, FIND_ENTRY, &p_hte);
    if ( rval == OK )
        superclass = ((Mesh_object_class_data *)p_hte->data)->superclass;
    if ( rval != OK )
        return rval;
    data_words = (stop_el - start_el + 1) * (conn_words[superclass] - 1);
    /* Taurus data base writes out six (6) ints */
    if ( superclass == M_BEAM )
        data_words = (stop_el - start_el + 1) * 6;
    rec_size = (data_words)*EXT_SIZE(fam, M_INT);

    /* Register an entry in the directory. */
    rval =
        add_dir_entry(fam, ELEM_CONNS, mesh_id, stop_el - start_el + 1, 1, &short_name, fam->next_free_byte, rec_size);
    if ( rval != OK )
        return rval;

    fam->next_free_byte += rec_size;

    return (Return_value)OK;
}

/*****************************************************************
 * TAG( taurus_def_conn_arb ) PUBLIC
 *
 * Define connectivities, materials _?_, and part numbers_?_ for
 * an arbitrary list of elements of a given type.
 */
Return_value taurus_def_conn_arb(Mili_family *fam, int mesh_id, char *short_name, int el_qty, int *elem_ids)
{
    LONGLONG rec_size, data_words;
    Mesh_descriptor *mesh;
    Return_value rval;
    int *p_int_blk, *p_iblk, *ibound;
    int blk_qty;
    Hash_table *p_ht;
    Htable_entry *p_hte;
    int superclass;

    if ( mesh_id > fam->qty_meshes - 1 )
        return NO_MESH;
    else
        mesh = fam->meshes[mesh_id];

    /* Convert 1D list to 2D block list. */
    rval = list_to_blocks(el_qty, elem_ids, &p_int_blk, &blk_qty);
    if ( rval != OK )
    {
        return rval;
    }

    /* Keep track of elements defined. */
    ibound = p_int_blk + blk_qty * 2;
    for ( p_iblk = p_int_blk; p_iblk < ibound; p_iblk += 2 )
    {
        rval = add_geom(mesh, short_name, *p_iblk, *(p_iblk + 1));
        /* Ugly.  Failure will leave list partially added to geometry. */
        if ( rval != OK )
            return rval;
    }

    /* Calculate the external record size. */
    p_ht = fam->meshes[mesh_id]->mesh_data.umesh_data;
    rval = (Return_value)htable_search(p_ht, short_name, FIND_ENTRY, &p_hte);
    if ( rval == OK )
        superclass = ((Mesh_object_class_data *)p_hte->data)->superclass;
    if ( rval != OK )
        return rval;
    data_words = el_qty * conn_words[superclass];
    rec_size = (data_words + QTY_CONN_HEADER_FIELDS + blk_qty * 2) * EXT_SIZE(fam, M_INT);

    /* Register an entry in the directory. */
    rval = add_dir_entry(fam, ELEM_CONNS, mesh_id, el_qty, 1, &short_name, fam->next_free_byte, rec_size);
    if ( rval != OK )
        return rval;

    fam->next_free_byte += rec_size;

    free(p_int_blk);

    return (Return_value)OK;
}

/*****************************************************************
 * TAG( add_geom ) LOCAL
 *
 * Record id's of newly declared objects as a range.  Merge
 * new ranges with old ones which are contiguous.  No id can
 * appear twice.
 */
static Return_value add_geom(Mesh_descriptor *mesh, char *short_name, int start_mo, int stop_mo)
{
    Mesh_object_class_data *p_mocd;
    Return_value rval;
    Hash_table *pht;
    Htable_entry *phte;

    /* Get pointer to the geometry hash table. */
    pht = mesh->mesh_data.umesh_data;

    /* Enter the class in the table or find it if it already exists. */
    rval = (Return_value)htable_search(pht, short_name, FIND_ENTRY, &phte);

    /* Initialize the class if new. */
    if ( rval != OK )
        return NO_CLASS;
    else
        p_mocd = (Mesh_object_class_data *)phte->data;

    rval = insert_range(p_mocd->blocks, start_mo, stop_mo);

    return (Return_value)((rval != OBJECT_RANGE_OVERLAP) ? OK : rval);
}

/*****************************************************************
 * TAG( taurus_delete_mo_data ) PRIVATE
 *
 * Delete a Mesh_object_class_data struct and its block list.  API designed
 * for compatibility with hashing utility function htable_delete().
 */
static void taurus_delete_mo_data(void *ptr_mo_data)
{
    Mesh_object_class_data *p_mocd;

    p_mocd = (Mesh_object_class_data *)ptr_mo_data;

    free(p_mocd->long_name);
    free(p_mocd->short_name);

    DELETE_LIST(p_mocd->blocks->blocks);
    free(p_mocd->blocks);

    free(p_mocd);
}

/*****************************************************************
 * TAG( taurus_delete_mesh ) PRIVATE
 *
 * Delete mesh for a Taurus file family.
 */
extern void taurus_delete_mesh(Mili_family *fam)
{
    htable_delete(fam->meshes[0]->mesh_data.umesh_data, taurus_delete_mo_data, TRUE);
    fam->meshes[0]->mesh_data.umesh_data = NULL;
    free(fam->meshes[0]->name);
    free(fam->meshes[0]);
    free(fam->meshes);
    fam->meshes = NULL;
}

/*****************************************************************
 * TAG( taurus_load_nodes ) PUBLIC
 *
 * Read coordinates for all nodes of a M_NODE object class.
 */
Return_value taurus_load_nodes(Famid fam_id, int mesh_id, char *short_name, void *data)
{
    Mili_family *fam;
    int fil_qty, ent_qty, name_cnt;
    int i, j;
    Dir_entry *p_de;
    char **names;
    Return_value rval;
    Htable_entry *p_hte;

    if ( validate_fam_id(fam_id) == OK )
        fam = fam_list[fam_id];
    else
        return BAD_FAMILY;

    if ( mesh_id < 0 || mesh_id > fam->qty_meshes - 1 )
        return NO_MESH;

    /* Initialize for node coord load or element connectivity load. */
    rval = (Return_value)htable_search(fam->meshes[mesh_id]->mesh_data.umesh_data, short_name, FIND_ENTRY, &p_hte);
    if ( rval != OK )
        return rval;
    else if ( ((Mesh_object_class_data *)p_hte->data)->superclass != M_NODE )
        return INVALID_CLASS_OPERATION;

    fil_qty = fam->file_count;

    /* For each non-state file... */
    for ( i = 0; i < fil_qty; i++ )
    {
        ent_qty = fam->directory[i].qty_entries;
        p_de = fam->directory[i].dir_entries;
        names = fam->directory[i].names;
        name_cnt = 0;

        /* For each directory entry in file... */
        for ( j = 0; j < ent_qty; j++ )
        {
            if ( (Dir_entry_type)p_de[j][TYPE_IDX] == NODES && p_de[j][MODIFIER1_IDX] == mesh_id &&
                 strcmp(names[name_cnt], short_name) == 0 )
            {
                /* Found an appropriate entry - load it into memory. */
                rval = taurus_load_nodes_def(fam, i, p_de[j], data);
                if ( rval != OK )
                    return rval;
            }

            name_cnt += p_de[j][STRING_QTY_IDX];
        }
    }

    return (Return_value)OK;
}

/*****************************************************************
 * TAG( taurus_load_conns ) PUBLIC
 *
 * Read connectivities for all elements of a particular class.
 *
 */
Return_value taurus_load_conns(Famid fam_id, int mesh_id, char *short_name, int *conns, int *mats, int *parts)
{
    Mili_family *fam;
    int fil_qty, ent_qty, name_cnt;
    int i, j;
    Dir_entry *p_de;
    char **names;
    Return_value rval;
    int superclass;
    Htable_entry *p_hte;
    void *dest_ptrs[3];
    int max_mat, cum_max_mat, current_max;
    char *max_mat_short_name;

    if ( validate_fam_id(fam_id) == OK )
        fam = fam_list[fam_id];
    else
        return BAD_FAMILY;

    if ( mesh_id < 0 || mesh_id > fam->qty_meshes - 1 )
        return NO_MESH;

    /* Initialize for node coord load or element connectivity load. */
    rval = (Return_value)htable_search(fam->meshes[mesh_id]->mesh_data.umesh_data, short_name, FIND_ENTRY, &p_hte);
    if ( rval == OK )
        superclass = ((Mesh_object_class_data *)p_hte->data)->superclass;
    if ( rval != OK )
        return rval;

    if ( superclass < M_TRUSS || superclass > M_HEX )
        return INVALID_CLASS_OPERATION;

    fil_qty = fam->file_count;

    /*
     * Set up to pass the output buffer addresses via one pointer argument
     * to keep interface to load_conn_def() looking like the interface to
     * load_nodes_def() to support potential future merger.
     */
    dest_ptrs[0] = (void *)conns;
    dest_ptrs[1] = (void *)mats;
    dest_ptrs[2] = (void *)parts;

    /* For each non-state file... */
    cum_max_mat = 0;
    for ( i = 0; i < fil_qty; i++ )
    {
        ent_qty = fam->directory[i].qty_entries;
        p_de = fam->directory[i].dir_entries;
        names = fam->directory[i].names;
        name_cnt = 0;

        /* For each directory entry in file... */
        for ( j = 0; j < ent_qty; j++ )
        {
            if ( (Dir_entry_type)p_de[j][TYPE_IDX] == ELEM_CONNS && p_de[j][MODIFIER1_IDX] == mesh_id &&
                 strcmp(names[name_cnt], short_name) == 0 )
            {
                /* Found an appropriate entry - load it into memory. */
                rval = taurus_load_conn_def(fam, &i, superclass, p_de[j], (void *)dest_ptrs, &max_mat);
                if ( rval != OK )
                    return rval;

                if ( max_mat > cum_max_mat )
                    cum_max_mat = max_mat;
            }

            name_cnt += p_de[j][STRING_QTY_IDX];
        }
    }

    /*
     * Update the material class if in loading connectivities we've
     * detected a material number higher than that specified in the
     * control words (or previously loaded connectivities).
     */
    current_max = taurus_get_max_material(fam_id, mesh_id, &max_mat_short_name);
    if ( cum_max_mat > current_max )
        taurus_def_class_idents(fam_id, mesh_id, max_mat_short_name, current_max + 1, cum_max_mat);

    return (Return_value)OK;
}

/*****************************************************************
 * TAG( taurus_get_max_material ) PRIVATE
 *
 * Get the largest material number currently specified in a mesh.
 */
static int taurus_get_max_material(Famid fam_id, int mesh_id, char **found_short_name)
{
    int int_args[2];
    int qty_classes, start_ident, stop_ident;
    char long_name[M_MAX_NAME_LEN], short_name[M_MAX_NAME_LEN];
    static char save_name[M_MAX_NAME_LEN];
    int j;
    int max_mat;

    int_args[0] = mesh_id;
    int_args[1] = M_MAT;
    mc_query_family(fam_id, QTY_CLASS_IN_SCLASS, (void *)int_args, NULL, (void *)&qty_classes);

    max_mat = 0;
    for ( j = 0; j < qty_classes; j++ )
    {
        mc_get_simple_class_info(fam_id, mesh_id, M_MAT, j, short_name, long_name, &start_ident, &stop_ident);
        if ( stop_ident > max_mat )
        {
            strcpy(save_name, short_name);
            max_mat = stop_ident;
        }
    }

    *found_short_name = save_name;

    return max_mat;
}

/*****************************************************************
 * TAG( taurus_load_conn_def ) LOCAL
 *
 * Read element connectivity family entry into memory.
 */
static Return_value taurus_load_conn_def(Mili_family *fam, int *file_idx, int superclass, Dir_entry dir_ent,
                                         void *dest_ptrs, int *p_max_mat)
{
    int *ebuf;
    int *p_src;
    int elem_qty, word_qty, conn_qty;
    int max_mat;
    LONGLONG read_cnt, int_qty;
    LONGLONG not_valid_qty;
    int *conns, *conns_dest, *mats, *parts;
    int i, j;
    LONGLONG offset;

    conns = ((int **)dest_ptrs)[0];
    conns_dest = conns;
    mats = ((int **)dest_ptrs)[1];
    parts = ((int **)dest_ptrs)[2];

    /* Allocate temporary buffer and read element connectivities. */
    elem_qty = dir_ent[MODIFIER2_IDX];
    word_qty = conn_words[superclass] - 1;
    conn_qty = word_qty - 1;
    if ( superclass == M_BEAM )
    {
        word_qty = 6;
        conn_qty = 3;
    }
    int_qty = elem_qty * word_qty;
    ebuf = NEW_N(int, int_qty, "Elem conns buf");
    p_src = ebuf;

    offset = dir_ent[OFFSET_IDX];
    taurus_non_state_file_open(fam, *file_idx, fam->access_mode);
    while ( offset > fam->cur_file_size )
    {
        offset -= fam->cur_file_size;
        non_state_file_close(fam);
        *file_idx += 1;
        taurus_non_state_file_open(fam, *file_idx, fam->access_mode);
    }
    non_state_file_seek(fam, offset);

    while ( int_qty > 0 )
    {
        if ( offset + int_qty * EXT_SIZE(fam, M_INT) <= fam->cur_file_size )
        {
            /* Read coordinates from file. */
            read_cnt = fam->read_funcs[M_INT](fam->cur_file, p_src, int_qty);
            if ( read_cnt != int_qty )
            {
                return SHORT_READ;
            }
            break;
        }
        else
        {
            /* Read coordinates from file. */
            read_cnt = fam->read_funcs[M_INT](fam->cur_file, p_src, int_qty);
            if ( read_cnt != int_qty )
            {
                return SHORT_READ;
            }
            /*
             * Check to make sure that all the data read in is valid.
             * the mazimum file size for a taurus file is a multiple of
             * 262144 'words'.  Any data beyond this is garbage and
             * needs to be ignored.
             */
            not_valid_qty = (offset / EXT_SIZE(fam, M_INT) + read_cnt) % 262144;
            if ( not_valid_qty > 0 )
                read_cnt -= not_valid_qty;
            /*
             * Close the current non-state file and prepare to read the
             * more coordinate date fromthe next file
             */
            p_src += read_cnt;
            int_qty -= read_cnt;
            *file_idx += 1;
            offset = 0;
            non_state_file_close(fam);
            taurus_non_state_file_open(fam, *file_idx, fam->access_mode);
            non_state_file_seek(fam, offset);
        }
    }

    max_mat = 0;
    p_src = ebuf;
    for ( i = 0; i < elem_qty; i++ )
    {
        /* Copy connectivities, material, and part into appropriate buffers. */
        for ( j = 0; j < conn_qty; j++ )
        {
            *(conns_dest + j) = *(p_src + j) - 1;
        }
        mats[i] = *(p_src + word_qty - 1) - 1;
        if ( mats[i] > max_mat )
            max_mat = mats[i];
        parts[i] = 1;
        conns_dest += conn_qty;
        p_src += word_qty;
    }

    free(ebuf);

    /* Return max material number. */
    *p_max_mat = max_mat + 1;

    return (Return_value)OK;
}

/*****************************************************************
 * TAG( taurus_load_nodes_def ) LOCAL
 *
 * Read nodal coordinates family entry into memory.
 */
static Return_value taurus_load_nodes_def(Mili_family *fam, int file_idx, Dir_entry dir_ent, void *coords)
{
    LONGLONG offset;
    float *dest;
    LONGLONG read_cnt, float_qty, byte_qty;

    /* Calculate destination in coords buffer for this node block. */
    dest = ((float *)coords);

    /* Calculate quantity of floats to read. */
    float_qty = (dir_ent[MODIFIER2_IDX]) * fam->dimensions;
    byte_qty = float_qty * sizeof(float);

    /* Seek to the nodal data entry in the family. */
    offset = dir_ent[OFFSET_IDX];
    taurus_non_state_file_open(fam, file_idx, fam->access_mode);
    while ( offset > fam->cur_file_size )
    {
        offset -= fam->cur_file_size;
        non_state_file_close(fam);
        file_idx += 1;
        taurus_non_state_file_open(fam, file_idx, fam->access_mode);
    }
    non_state_file_seek(fam, offset);

    while ( float_qty > 0 )
    {
        if ( offset + byte_qty <= fam->cur_file_size )
        {
            /* Read coordinates from file. */
            read_cnt = fam->read_funcs[M_FLOAT](fam->cur_file, dest, float_qty);
            break;
        }
        else
        {
            /* Read coordinates from file. */
            read_cnt = fam->read_funcs[M_FLOAT](fam->cur_file, dest, float_qty);

            /*
             * Close the current non-state file and prepare to read the
             * more coordinate date fromthe next file
             */
            dest += read_cnt;
            float_qty -= read_cnt;
            byte_qty = float_qty * sizeof(float);
            file_idx += 1;
            offset = 0;
            non_state_file_close(fam);
            taurus_non_state_file_open(fam, file_idx, fam->access_mode);
            non_state_file_seek(fam, offset);
        }
    }

    return (Return_value)((read_cnt == float_qty) ? OK : SHORT_READ);
}

/*****************************************************************
 * TAG( taurus_get_class_qty ) PRIVATE
 *
 * Find the number of classes defined for a superclass.
 */
Return_value taurus_get_class_qty(Mili_family *fam, int *modifiers, int *class_qty)
{
    int mesh_id, superclass;
    int i;
    Return_value rval;

    mesh_id = modifiers[0];
    superclass = modifiers[1];

    *class_qty = 0;
    rval = taurus_reset_class_data(fam, mesh_id, FALSE);
    if ( rval != OK )
    {
        return rval;
    }

    for ( i = 0; i < taurus_all_class_count; i++ )
    {
        if ( taurus_class_data[i]->superclass == superclass )
            (*class_qty)++;
    }

    return OK;
}

/*****************************************************************
 * TAG( taurus_count_elem_conn_defs ) PRIVATE
 *
 * Count the number of element connectivity definition blocks for
 * a particular mesh object class.
 */
int taurus_count_elem_conn_defs(Mili_family *fam, int mesh_id, char *class_name)
{
    int fil_qty, ent_qty, name_cnt;
    int qty_conn_entries;
    int i, j;
    Dir_entry *p_de;
    char **names;

    qty_conn_entries = 0;
    fil_qty = fam->file_count;

    /* For each non-state file... */
    for ( i = 0; i < fil_qty; i++ )
    {
        ent_qty = fam->directory[i].qty_entries;
        p_de = fam->directory[i].dir_entries;
        names = fam->directory[i].names;
        name_cnt = 0;

        /* For each directory entry in file... */
        for ( j = 0; j < ent_qty; j++ )
        {
            if ( (Dir_entry_type)p_de[j][TYPE_IDX] == ELEM_CONNS && p_de[j][MODIFIER1_IDX] == mesh_id &&
                 strcmp(names[name_cnt], class_name) == 0 )
                qty_conn_entries++;

            name_cnt += p_de[j][STRING_QTY_IDX];
        }
    }

    return qty_conn_entries;
}

/*****************************************************************
 * TAG( taurus_get_elem_qty_in_def ) PRIVATE
 *
 * Get the number of elements defined in a directory ELEM_CONNS
 * entry for a specified mesh and object class.
 * The current directory traversal state is saved so that a
 * sequential series of calls with steadily increasing entry
 * index can be accommodated without redundantly searching the
 * directory from the beginning at each call.
 */
Return_value taurus_get_elem_qty_in_def(Mili_family *fam, int *int_args, char *short_name, int *elem_qty)
{
    Mili_family *last_fam = NULL;
    static int mesh_id = -1;
    static int econn_index = -1;
    static int i, j;
    static int econn_entry_cnt, name_cnt;
    static char last_class[128] = {'\0'};
    int fil_qty, ent_qty;
    int mi;
    Dir_entry *p_de;
    char **names;
    Bool_type searching;

    mi = int_args[0];
    if ( fam == NULL || mi < 0 || mi > fam->qty_meshes - 1 )
        return NO_MESH;

    if ( fam != last_fam || mi != mesh_id || int_args[1] < econn_entry_cnt || strcmp(last_class, short_name) != 0 )
    {
        /* This is not a sequential successor of a previous search;
         * re-initialize. */
        last_fam = fam;
        mesh_id = mi;
        strcpy(last_class, short_name);
        i = 0;
        j = 0;
        econn_entry_cnt = 0;
        name_cnt = 0;
    }

    econn_index = int_args[1];
    fil_qty = fam->file_count;
    searching = TRUE;

    /* For each non-state file... */
    while ( searching && i < fil_qty )
    {
        ent_qty = fam->directory[i].qty_entries;
        p_de = fam->directory[i].dir_entries;
        names = fam->directory[i].names;

        /* For each directory entry in file... */
        for ( ; j < ent_qty; j++ )
        {
            if ( (Dir_entry_type)p_de[j][TYPE_IDX] == ELEM_CONNS && p_de[j][MODIFIER1_IDX] == mesh_id &&
                 strcmp(names[name_cnt], short_name) == 0 )
            {
                if ( econn_entry_cnt == econn_index )
                {
                    /* Found desired entry.  Get elem count and get out. */
                    *elem_qty = p_de[j][MODIFIER2_IDX];
                    searching = FALSE;
                    break;
                }
                else
                    econn_entry_cnt++;
            }

            name_cnt += p_de[j][STRING_QTY_IDX];
        }

        if ( searching )
        {
            j = 0;
            i++;
            name_cnt = 0;
        }
    }

    return (Return_value)((searching ? INVALID_INDEX : OK));
}

/*****************************************************************
 * TAG( taurus_count_node_entries ) PRIVATE
 *
 * Count the number of nodal coordinate entries for a particular
 * nodal class and mesh of a family.
 */
int taurus_count_node_entries(Mili_family *fam, int mesh_id, char *class_name)
{
    int fil_qty, ent_qty, name_cnt;
    int qty_node_entries;
    int i, j;
    Dir_entry *p_de;
    char **names;

    qty_node_entries = 0;
    fil_qty = fam->file_count;

    /* For each non-state file... */
    for ( i = 0; i < fil_qty; i++ )
    {
        ent_qty = fam->directory[i].qty_entries;
        p_de = fam->directory[i].dir_entries;
        names = fam->directory[i].names;
        name_cnt = 0;

        /* For each directory entry in file... */
        for ( j = 0; j < ent_qty; j++ )
        {
            if ( (Dir_entry_type)p_de[j][TYPE_IDX] == NODES && p_de[j][MODIFIER1_IDX] == mesh_id &&
                 strcmp(names[name_cnt], class_name) == 0 )
                qty_node_entries++;

            name_cnt += p_de[j][STRING_QTY_IDX];
        }
    }

    return qty_node_entries;
}

/*****************************************************************
 *
 * MERGE WITH get_elem_qty_in_def()!
 *
 * TAG( taurus_get_node_block_size ) PRIVATE
 *
 * Get the number of nodes defined in a directory NODES entry.
 * The current directory traversal state is saved so that a
 * sequential series of calls with steadily increasing block
 * index can be accommodated without redundantly searching the
 * directory from the beginning at each call.
 */
Return_value taurus_get_node_block_size(Mili_family *fam, int *modifiers, char *class_name, int *blk_size)
{
    Mili_family *last_fam = NULL;
    static int mesh_id = -1;
    static int blk_index = -1;
    static int i, j;
    static int node_entry_cnt;
    int fil_qty, ent_qty, name_cnt;
    int mi;
    Dir_entry *p_de;
    Bool_type searching;
    char **names;
    static char last_class[128] = {'\0'};

    mi = modifiers[0];
    if ( fam == NULL || mi < 0 || mi > fam->qty_meshes - 1 )
        return NO_MESH;

    if ( fam != last_fam || mi != mesh_id || modifiers[1] < node_entry_cnt )
    {
        /* This is not a sequential successor of a previous search;
         * re-initialize. */
        last_fam = fam;
        mesh_id = mi;
        strcpy(last_class, class_name);
        i = 0;
        j = 0;
        node_entry_cnt = 0;
        name_cnt = 0;
    }

    blk_index = modifiers[1];
    fil_qty = fam->file_count;
    searching = TRUE;

    /* For each non-state file... */
    while ( searching && i < fil_qty )
    {
        ent_qty = fam->directory[i].qty_entries;
        p_de = fam->directory[i].dir_entries;
        names = fam->directory[i].names;

        /* For each directory entry in file... */
        for ( ; j < ent_qty; j++ )
        {
            if ( (Dir_entry_type)p_de[j][TYPE_IDX] == NODES && p_de[j][MODIFIER1_IDX] == mesh_id &&
                 strcmp(names[name_cnt], class_name) == 0 )
            {
                if ( node_entry_cnt == blk_index )
                {
                    /* Found desired entry.  Get block size and get out. */
                    *blk_size = p_de[j][MODIFIER2_IDX];
                    searching = FALSE;
                    break;
                }
                else
                    node_entry_cnt++;
            }
        }

        if ( searching )
        {
            j = 0;
            i++;
            name_cnt = 0;
        }
    }

    return (Return_value)((searching ? INVALID_INDEX : OK));
}

/*****************************************************************
 * TAG( taurus_reset_class_data ) LOCAL
 *
 * Load global variables for object class queries.
 */
Return_value taurus_reset_class_data(Mili_family *fam, int mesh_id, Bool_type force_init)
{
    Mesh_descriptor *p_md;
    Return_value rval = OK;

    if ( force_init )
    {
        taurus_class_data_fam = NULL;
        taurus_class_data_mesh_id = -1;
        taurus_all_class_count = 0;
        if ( taurus_class_data != NULL )
        {
            free(taurus_class_data);
            taurus_class_data = NULL;
        }
    }
    else if ( fam != taurus_class_data_fam || mesh_id != taurus_class_data_mesh_id )
    {
        if ( taurus_class_data != NULL )
            free(taurus_class_data);

        taurus_class_data_fam = fam;
        taurus_class_data_mesh_id = mesh_id;
        p_md = fam->meshes[mesh_id];
        rval = htable_get_data(p_md->mesh_data.umesh_data, (void ***)&taurus_class_data, &taurus_all_class_count);
    }
    return rval;
}
