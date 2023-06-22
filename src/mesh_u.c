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
 ************************************************************************
 * Modifications:
 *  I. R. Corey - February 9, 2008: Added material numbers to end of label
 *  list which is needed by Xmilics.
 *
 *  I. R. Corey - March 20, 2009: Added support for WIN32 for use with
 *                with Visit.
 *                See SCR #587.
 ************************************************************************
 */

#include <string.h>
#ifndef _MSC_VER
#include <values.h>
#include <sys/time.h>
#endif
#include <time.h>
#include "mili_internal.h"
#include "eprtf.h"

#define QTY_NODE_TAGS (2)
#if TIMER
int timed = 0;
#endif
enum
{
    QTY_CONN_SURFACE_HEADER_FIELDS = 1
};

/* Keep this in sync with object superclass defines in mili.h*/
static const char *superclass_names[M_QTY_SUPERCLASS] = {"M_UNIT", "M_NODE", "M_TRUSS",   "M_BEAM",    "M_TRI",
                                                         "M_QUAD", "M_TET",  "M_PYRAMID", "M_WEDGE",   "M_HEX",
                                                         "M_MAT",  "M_MESH", "M_SURFACE", "M_PARTICLE"};

/**/
#define OVERLAP(min, max, tmin, tmax) (!(tmax < min || tmin > max))

typedef struct _label_block
{
    int start_of_block, end_of_block;
} Label_block;

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
 * TAG( milisilo )
 *
 * Mili-Silo interface flag
 */
extern Bool_type milisilo;

static Mili_family *class_data_fam = NULL;
static int class_data_mesh_id = -1;
static int all_class_count;
static Mesh_object_class_data **class_data = NULL;

static int SDTLIBCC mc_compare_labels(const void *label1, const void *label2);
static Return_value make_umesh(Mili_family *fam, Bool_type initial_create, char *mesh_name, int *p_mesh_id);
static Return_value update_nodes_def(Mili_family *fam, int mesh_id, char *class_name, int start, int stop,
                                     float *coords);
static void delete_umesh(Mesh_descriptor *mesh);
static Return_value add_mesh(Mili_family *fam, Dir_entry dir_ent, char *name_handle);
static Return_value load_nodes_def(Mili_family *fam, int file_idx, Dir_entry dir_ent, void *coords);
static Return_value load_conn_def(Mili_family *fam, int file_idx, Dir_entry dir_ent, void *conns);
static Return_value load_conn_def_surface(Mili_family *fam, int file_idx, Dir_entry dir_ent, void *conns);
static Return_value add_geom(Mesh_descriptor *mesh, char *short_name, int start_mo, int stop_mo);

/*****************************************************************
 * TAG( classEnum_to_className ) LOCAL
 *
 * Generate an Enum value for a class name (ASCII)
 */
Return_value classEnum_to_className(int classEnum, char *className)
{
    /* Convert Mili-class type to text string */
    Return_value rval = OK;
    if ( className != NULL && (classEnum >= M_UNIT && classEnum < M_QTY_SUPERCLASS) )
    {
        strcpy(className, superclass_names[classEnum]);
    }
    else
    {
        rval = INVALID_CLASS_NAME;
    }

    return rval;
}

/*****************************************************************
 * TAG( className_to_classEnum ) PRIVATE
 *
 * Generate an Enum value for a class name (ASCII)
 */
Return_value className_to_classEnum(char *className, int *classEnum)
{
    Return_value rval = OK;

    /* Convert text class name to Mili-class type */
    if ( !strcmp("M_NODE", className) || !strcmp("node", className) )
    {
        *classEnum = M_NODE;
    }
    else if ( !strcmp("M_TRUSS", className) || !strcmp("truss", className) )
    {
        *classEnum = M_TRUSS;
    }
    else if ( !strcmp("M_BEAM", className) || !strcmp("beam", className) )
    {
        *classEnum = M_BEAM;
    }
    else if ( !strcmp("M_QUAD", className) || !strcmp("shell", className) )
    {
        *classEnum = M_QUAD;
    }
    else if ( !strcmp("M_TET", className) || !strcmp("tet", className) )
    {
        *classEnum = M_TET;
    }
    else if ( !strcmp("M_HEX", className) || !strcmp("brick", className) )
    {
        *classEnum = M_HEX;
    }
    else if ( !strcmp("M_MAT", className) )
    {
        *classEnum = M_MAT;
    }
    else if ( !strcmp("M_SURFACE", className) )
    {
        *classEnum = M_SURFACE;
    }
    else if ( !strcmp("M_TRI", className) )
    {
        *classEnum = M_TRI;
    }
    else if ( !strcmp("M_UNIT", className) )
    {
        *classEnum = M_UNIT;
    }
    else if ( !strcmp("M_PARTICLE", className) )
    {
        *classEnum = M_PARTICLE;
    }

    else
    {
        *classEnum = -1;
        rval = INVALID_CLASS_NAME;
    }

    return rval;
}

/*****************************************************************
 * TAG( mc_is_particle_class ) PRIVATE
 *
 * Checks to see if this is a particle class and returns TRUE oi
 * FALSE. This is for old backward compatibility issues.
 *
 */
Bool_type mc_is_particle_class(Famid fam_id, char *class_name)
{
    int i = 0;
    int num_entries = 0;
    char *wildcard_list[1000];
    char particle_class_name[256];

    int status = OK;

    /* Load particle element names from list if available */
    num_entries = mc_ti_htable_search_wildcard(fam_id, 0, FALSE, "particle_element", "NULL", "NULL", wildcard_list);
    if ( num_entries > 0 )
    {
        for ( i = 0; i < num_entries; i++ )
        {
            status = mc_ti_read_string(fam_id, wildcard_list[i], particle_class_name);
            if ( status != OK )
            {
                continue;
            }
            if ( !strcmp(particle_class_name, class_name) )
            {
                htable_delete_wildcard_list(num_entries, wildcard_list);
                return TRUE;
            }
        }
    }
    htable_delete_wildcard_list(num_entries, wildcard_list);
    return FALSE;
}

/*****************************************************************
 * TAG( mc_make_umesh ) PUBLIC
 *
 * Perform error checking prior to creating a mesh.
 */
Return_value mc_make_umesh(Famid fam_id, char *mesh_name, int dim, int *p_mesh_id)
{
    Mili_family *fam;
    int i;
    char *mname;
    Return_value status;

#ifdef SILOENABLED
    if ( milisilo )
    {
        status = mc_silo_make_umesh(fam_id, mesh_name, dim, p_mesh_id);
        return (status);
    }
#endif

    fam = fam_list[fam_id];

    CHECK_WRITE_ACCESS(fam)

    if ( dim != 2 && dim != 3 )
    {
        return INVALID_DIMENSIONALITY;
    }
    else if ( dim != fam->dimensions && fam->dimensions != 0 )
    {
        return INVALID_DIMENSIONALITY;
    }

    mname = mesh_name ? mesh_name : "";

    for ( i = 0; i < fam->qty_meshes; i++ )
    {
        if ( strcmp(fam->meshes[i]->name, mname) == 0 )
        {
            return NAME_CONFLICT;
        }
    }

    if ( fam->mesh_type == UNDEFINED )
    {
        fam->mesh_type = UNSTRUCTURED;
        write_scalar(fam, M_INT, "mesh type", &fam->mesh_type, MILI_PARAM);
    }

    if ( fam->dimensions == 0 )
    {
        fam->dimensions = dim;
        write_scalar(fam, M_INT, "mesh dimensions", &fam->dimensions, MILI_PARAM);
    }

    status = make_umesh(fam, TRUE, mname, p_mesh_id);

    return status;
}

/*****************************************************************
 * TAG( make_umesh ) PUBLIC
 *
 * Create and initialize an unstructured mesh, making it the
 * current mesh.  Return its id in a pass-by-reference argument.
 */
static Return_value make_umesh(Mili_family *fam, Bool_type initial_create, char *mesh_name, int *p_mesh_id)
{
    Mesh_descriptor *pmd;
    char pname[32];
    Return_value rval = OK;

    fam->meshes = RENEW_N(Mesh_descriptor *, fam->meshes, fam->qty_meshes, 1, "Mesh pointer");
    if ( fam->meshes == NULL )
    {
        return ALLOC_FAILED;
    }

    pmd = NEW(Mesh_descriptor, "Mesh data");
    if ( pmd == NULL )
    {
        return ALLOC_FAILED;
    }
    str_dup(&pmd->name, mesh_name);
    if ( pmd->name == NULL )
    {
        return ALLOC_FAILED;
    }
    fam->meshes[fam->qty_meshes] = pmd;

    /*
     * Create the geometry table and insert an entry for global data
     * since there will be no add_geom() call for global data.
     */
    pmd->mesh_data.umesh_data = htable_create(DEFAULT_HASH_TABLE_SIZE);
    if ( pmd->mesh_data.umesh_data == NULL )
    {
        return ALLOC_FAILED;
    }

    /*
     * If mesh is being created by calling app, save mesh name to
     * family as string parameter; otherwise an existing mesh is
     * being read into memory from the family.
     */
    if ( initial_create )
    {
        sprintf(pname, "mesh name %d", fam->qty_meshes);
        rval = write_string(fam, pname, mesh_name, MILI_PARAM);
    }

    *p_mesh_id = fam->qty_meshes;
    fam->qty_meshes++;

    return rval;
}

/*****************************************************************
 * TAG( mc_get_class_info_by_index ) PRIVATE
 *
 * Retrieve the class info based on the index and mesh id
 */
Return_value mc_get_class_info_by_index(Mili_family *in, int *mesh_id, int *index, int *superclass, char *short_name,
                                        char *long_name)
{
    Return_value rval;

    if ( *mesh_id < 0 || *mesh_id > in->qty_meshes - 1 )
    {
        return NO_MESH;
    }

    if ( *index < 0 )
    {
        return NO_CLASS;
    }

    rval = reset_class_data(in, *mesh_id, FALSE);
    if ( rval != OK )
    {
        return rval;
    }

    if ( *index >= all_class_count )
    {
        return NO_CLASS;
    }
    *superclass = class_data[*index]->superclass;
    strcpy(short_name, class_data[*index]->short_name);
    strcpy(long_name, class_data[*index]->long_name);

    return OK;
}

/*****************************************************************
 * TAG( create_class_data ) PRIVATE
 *
 * Allocate and initialize a Mesh_object_class_data struct,
 * returning its address.
 */
Return_value create_class_data(int superclass, char *short_name, char *long_name, Mesh_object_class_data **pp_mocd)
{
    Mesh_object_class_data *p_mocd;

    *pp_mocd = NEW(Mesh_object_class_data, "Mesh object class");
    if ( *pp_mocd == NULL )
    {
        return ALLOC_FAILED;
    }
    p_mocd = *pp_mocd;

    str_dup(&p_mocd->long_name, long_name);
    str_dup(&p_mocd->short_name, short_name);
    if ( p_mocd->long_name == NULL || p_mocd->short_name == NULL )
    {
        if ( p_mocd->long_name != NULL )
        {
            free(p_mocd->long_name);
        }
        if ( p_mocd->short_name != NULL )
        {
            free(p_mocd->short_name);
        }
        free(p_mocd);
        return ALLOC_FAILED;
    }
    p_mocd->superclass = superclass;
    p_mocd->surface_sizes = NULL;

    /* Superclass-specific initializations. */
    switch ( superclass )
    {
        case M_UNIT:
        case M_NODE:
        case M_TRUSS:
        case M_BEAM:
        case M_TRI:
        case M_QUAD:
        case M_TET:
        case M_PYRAMID:
        case M_WEDGE:
        case M_HEX:
        case M_MAT:
        case M_MESH:
        case M_SURFACE:
        case M_PARTICLE:
            p_mocd->blocks = NEW(Block_list, "Class block list");
            if ( p_mocd->blocks == NULL )
            {
                if ( p_mocd->long_name != NULL )
                {
                    free(p_mocd->long_name);
                }
                if ( p_mocd->short_name != NULL )
                {
                    free(p_mocd->short_name);
                }
                free(p_mocd);
                return ALLOC_FAILED;
            }
            break;

        default:
            p_mocd->blocks = NULL;
            break;
    }

    return OK;
}

/*****************************************************************
 * TAG( mc_def_class ) PUBLIC
 *
 * Create an entry in the geometry table for a new mesh object
 * class.
 */
Return_value mc_def_class(Famid fam_id, int mesh_id, int superclass, char *short_name, char *long_name)
{
    Mili_family *fam;
    Hash_table *p_ht;
    Return_value rval;
    char *names[2];
    Mesh_object_class_data *mocd;

#ifdef SILOENABLED
    if ( milisilo )
    {
        rval = mc_silo_def_class(fam_id, mesh_id, superclass, short_name, long_name);
        return (rval);
    }
#endif

    fam = fam_list[fam_id];

    if ( mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }
    else
    {
        p_ht = fam->meshes[mesh_id]->mesh_data.umesh_data;
    }

    /* Create/init object class struct. */
    rval = create_class_data(superclass, short_name, long_name, &mocd);
    if ( rval != OK )
    {
        return rval;
    }

    /* Enter the class in the table. */
    rval = htable_add_entry_data(p_ht, short_name, ENTER_UNIQUE, mocd);
    if ( rval != OK && rval != ENTRY_EXISTS )
    {
        free(mocd->long_name);
        free(mocd->short_name);
        if ( mocd->blocks != NULL )
        {
            free(mocd->blocks);
        }
        free(mocd);
        return rval;
    }

    /*
     * Create directory entry.  A class def has only directory data,
     * but call prep_for_new_data() first to update fam->file_count in case a new
     * family file will be required to receive the actual directory entry
     * when ultimately written out.
     */
    rval = prep_for_new_data(fam, NON_STATE_DATA);
    if ( rval == OK )
    {
        names[0] = short_name;
        names[1] = long_name;
        rval = add_dir_entry(fam, CLASS_DEF, mesh_id, superclass, 2, names, (LONGLONG)DONT_CARE, (LONGLONG)DONT_CARE);
    }

    return rval;
}

/*****************************************************************
 * TAG( mc_def_class_idents ) PUBLIC
 *
 * Define object identifiers for objects in a particular class.
 * The class should be derived only from one of the superclasses
 * which represent objects of which there may be more than one
 * and which are not otherwise defined in another mc_def...()
 * call (as of this writing, M_UNIT or M_MAT).
 */
Return_value mc_def_class_idents(Famid fam_id, int mesh_id, char *short_name, int start, int stop)
{
    LONGLONG rec_size;
    Mesh_descriptor *mesh;
    Mili_family *fam;
    Return_value rval;
    int entry[QTY_IDENT_ENTRY_FIELDS];
    int superclass;
    int num_items_written;

#ifdef SILOENABLED
    if ( milisilo )
    {
        rval = mc_silo_def_class_idents(fam_id, mesh_id, short_name, start, stop);
        return (rval);
    }
#endif

    fam = fam_list[fam_id];

    CHECK_WRITE_ACCESS(fam)

    if ( mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }
    else
    {
        mesh = fam->meshes[mesh_id];
    }

    /* Keep track of objects defined. */
    rval = add_geom(mesh, short_name, start, stop);
    if ( rval != OK )
    {
        return rval;
    }

    /* Calculate the external record size. */
    rval = mc_query_family(fam_id, CLASS_SUPERCLASS, &mesh_id, short_name, (void *)&superclass);
    if ( rval != OK )
    {
        return rval;
    }

    rec_size = QTY_IDENT_ENTRY_FIELDS * EXT_SIZE(fam, M_INT);

    /* Ensure file is open and positioned. */
    rval = prep_for_new_data(fam, NON_STATE_DATA);
    if ( rval != OK )
    {
        return rval;
    }

    /* Register an entry in the directory. */
    rval = add_dir_entry(fam, CLASS_IDENTS, mesh_id, stop - start + 1, 1, &short_name, fam->next_free_byte, rec_size);
    if ( rval != OK )
    {
        return rval;
    }

    /* Load the entry array. */
    entry[IDENT_SUPERCLASS_IDX] = superclass;
    entry[START_IDENT_IDX] = start;
    entry[STOP_IDENT_IDX] = stop;

    /*
     * Write it out.
     */
    num_items_written = fam->write_funcs[M_INT](fam->cur_file, entry, (LONGLONG)QTY_IDENT_ENTRY_FIELDS);
    if ( num_items_written != QTY_IDENT_ENTRY_FIELDS )
    {
        return SHORT_WRITE;
    }

    fam->next_free_byte += rec_size;

    return OK;
}

/*****************************************************************
 * TAG( mc_def_nodes ) PUBLIC
 *
 * Define coordinates for a continuously-numbered sequence of nodes.
 * Logic assumes the sequence is ascending.
 */
Return_value mc_def_nodes(Famid fam_id, int mesh_id, char *short_name, int start_node, int stop_node, float *coords)
{
    LONGLONG rec_size;
    LONGLONG data_words;
    Mesh_descriptor *mesh;
    int tags[QTY_NODE_TAGS];
    Mili_family *fam;
    Return_value rval;
    int superclass;
    int num_items_written;

#ifdef SILOENABLED
    if ( milisilo )
    {
        rval = mc_silo_def_nodes(fam_id, mesh_id, short_name, start_node, stop_node, coords);
        return (rval);
    }
#endif

    fam = fam_list[fam_id];

    CHECK_WRITE_ACCESS(fam)

    if ( mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }
    else
    {
        mesh = fam->meshes[mesh_id];
    }

    /* Verify class is derived from M_NODE superclass. */
    rval = mc_query_family(fam_id, CLASS_SUPERCLASS, &mesh_id, short_name, (void *)&superclass);
    if ( rval != OK )
    {
        return rval;
    }
    if ( superclass != M_NODE )
    {
        return SUPERCLASS_CONFLICT;
    }

    data_words = (stop_node - start_node + 1) * fam->dimensions;
    rec_size = data_words * EXT_SIZE(fam, M_FLOAT) + QTY_NODE_TAGS * EXT_SIZE(fam, M_INT);

    /* Keep track of nodes defined and trap re-write requests. */
    rval = add_geom(mesh, short_name, start_node, stop_node);
    if ( rval != OK )
    {
        if ( rval == OBJECT_RANGE_OVERLAP )
        {
            /* Overwrite OK if appropriate conditions met. */
            rval = update_nodes_def(fam, mesh_id, short_name, start_node, stop_node, coords);
        }

        return rval;
    }

    /* Ensure file is open and positioned _before_ add_dir_entry(). */
    rval = prep_for_new_data(fam, NON_STATE_DATA);
    if ( rval != OK )
    {
        return rval;
    }

    /* Register an entry in the directory. */
    rval =
        add_dir_entry(fam, NODES, mesh_id, stop_node - start_node + 1, 1, &short_name, fam->next_free_byte, rec_size);
    if ( rval != OK )
    {
        return rval;
    }

    /* Write the bounding nodes id's first. */
    tags[0] = start_node;
    tags[1] = stop_node;
    num_items_written = fam->write_funcs[M_INT](fam->cur_file, tags, QTY_NODE_TAGS);
    if ( num_items_written != QTY_NODE_TAGS )
    {
        return SHORT_WRITE;
    }

    /* Now write the coordinates. */
    num_items_written = fam->write_funcs[M_FLOAT](fam->cur_file, coords, data_words);
    if ( num_items_written != data_words )
    {
        return SHORT_WRITE;
    }

    fam->next_free_byte += rec_size;

    return OK;
}

/*****************************************************************
 * TAG( mc_def_node_labels ) PUBLIC
 *
 * Define nodal labels - labels are optional and are
 * implemented using the TI interface.
 *
 */
Return_value mc_def_node_labels(Famid fam_id, int mesh_id, char *short_name, int qty, int *labels)
{
    int dims[3];
    Mili_family *fam;
    char label_name[128], superclass_name[32], label_descriptor[96];
    int temp_mesh_id = mesh_id, superclass;
    Return_value status;
    fam = fam_list[fam_id];

    CHECK_WRITE_ACCESS(fam)

    if ( mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }

    mc_ti_enable(fam_id);

    status = mc_ti_savedef_class(fam_id);
    if ( status != OK )
    {
        return status;
    }

    /* Determine the superclass */
    status = mc_query_family(fam_id, CLASS_SUPERCLASS, &temp_mesh_id, short_name, (void *)&superclass);
    if ( status != OK )
    {
        return status;
    }

    /* Convert superclass Enum to a name string */
    status = classEnum_to_className(superclass, superclass_name);
    if ( status != OK )
    {
        return status;
    }
    status = ti_make_label_description(mesh_id, (int)-1, superclass_name, short_name, label_descriptor);
    /*status = mc_ti_def_class( fam_id, mesh_id, state, (int) -1,
                              superclass_name, TRUE, TRUE,
                              short_name, short_name);
    */
    if ( status != OK )
    {
        return status;
    }

    dims[0] = qty;
    // dims[1] = 0;
    // dims[2] = 0;

    strcpy(label_name, "Node Labels");
    strcat(label_name, label_descriptor);
    status = mc_ti_wrt_array(fam_id, M_INT, (char *)label_name, 1, dims, (void *)labels);
    if ( status != OK )
    {
        return status;
    }

    status = mc_ti_restoredef_class(fam_id);

    return status;
}

/*****************************************************************
 * TAG( update_nodes_def ) LOCAL
 *
 * Determine if a range of nodes is a continuous subset of any
 * existing node definitions entry and overwrite if so.
 */
static Return_value update_nodes_def(Mili_family *fam, int mesh_id, char *class_name, int start, int stop,
                                     float *coords)
{
    int i, j, ent_qty, node_qty, name_cnt;
    LONGLONG read_cnt, data_words, write_cnt;
    Dir_entry *p_de;
    char **names;
    int range[QTY_NODE_TAGS];
    int sav_index;
    Bool_type searching;
    Return_value rval;
    LONGLONG offset;
    int stat;

    /*
     * Scan existing directory entries looking for a NODES entry that
     * contains the input node sequence.
     */

    /* For each non-state file... */
    rval = OBJECT_RANGE_OVERLAP; /* Start with condition that got us here. */
    sav_index = fam->cur_index;
    searching = TRUE;
    node_qty = stop - start + 1;
    for ( i = 0; i < fam->file_count && searching; i++ )
    {
        ent_qty = fam->directory[i].qty_entries;
        p_de = fam->directory[i].dir_entries;
        names = fam->directory[i].names;
        name_cnt = 0;

        /* For each directory entry in file... */
        for ( j = 0; j < ent_qty; j++ )
        {
            if ( (Dir_entry_type)p_de[j][TYPE_IDX] == NODES && p_de[j][MODIFIER1_IDX] == mesh_id &&
                 p_de[j][MODIFIER2_IDX] >= node_qty && strcmp(names[name_cnt], class_name) == 0 )
            {
                /* Found a potential winner; must seek and read to verify. */

                /* Open in append mode to avoid truncating file. */
                rval = non_state_file_open(fam, i, 'a');
                if ( rval != OK )
                {
                    searching = FALSE;
                    break;
                }
                rval = non_state_file_seek(fam, p_de[j][OFFSET_IDX]);
                if ( rval != OK )
                {
                    searching = FALSE;
                    break;
                }
                read_cnt = fam->read_funcs[M_INT](fam->cur_file, range, QTY_NODE_TAGS);

                if ( read_cnt != QTY_NODE_TAGS )
                {
                    searching = FALSE;
                    rval = SHORT_READ;
                    break;
                }
                else if ( start >= range[0] && stop <= range[1] )
                {
                    /* Update can occur. */

                    if ( range[0] != start )
                    {
                        offset = (start - range[0]) * EXT_SIZE(fam, M_FLOAT) * fam->dimensions;
                    }
                    else
                    {
                        offset = 0;
                    }

                    /*
                     * Seek to correct location to start overwriting.
                     * Call fseek directly rather than non_state_file_seek()
                     * so we can seek relative to current file ptr.
                     *
                     * Write fails (IRIX) with offset = 0 if we ignore seek.
                     */
                    stat = fseek(fam->cur_file, (long)offset, SEEK_CUR);

                    if ( stat != 0 )
                    {
                        searching = FALSE;
                        rval = SEEK_FAILED;
                        break;
                    }

                    /* Write out new coordinates. */
                    data_words = (stop - start + 1) * fam->dimensions;
                    write_cnt = fam->write_funcs[M_FLOAT](fam->cur_file, coords, data_words);

                    searching = FALSE;
                    rval = (write_cnt != data_words) ? SHORT_WRITE : OK;
                    break;
                }
                else if ( sav_index != fam->cur_index )
                {
                    rval = non_state_file_close(fam);
                    if ( rval != OK )
                    {
                        searching = FALSE;
                        break;
                    }
                }
            } /* if have a matching entry */

            /* Still hunting; prepare for next directory entry. */
            name_cnt += p_de[j][STRING_QTY_IDX];
        } /* for each entry in file... */
    }     /* for each non state file... */

    if ( sav_index == fam->cur_index && fam->cur_file != NULL )
    {
        /* Re-position file pointer to end of file to restore state. */
        if ( fseek(fam->cur_file, (long)0, SEEK_END) != 0 )
        {
            rval = SEEK_FAILED;
        }
    }
    else
    {
        rval = non_state_file_close(fam);
    }

    return rval;
}

/*****************************************************************
 * TAG( mc_def_conn_seq ) PUBLIC
 *
 * Define connectivities, materials _?_, and part numbers_?_ for a
 * globally numbered sequential list of elements of a given type.
 */
Return_value mc_def_conn_seq(Famid fam_id, int mesh_id, char *short_name, int start_el, int stop_el, int *data)
{
    LONGLONG rec_size;
    Mesh_descriptor *mesh;
    int *tags;
    Mili_family *fam;
    Return_value rval;
    LONGLONG ibuf_len, data_words;
    int superclass;
    int num_items_written;

#ifdef SILOENABLED
    if ( milisilo )
    {
        rval = mc_silo_def_conn_seq(fam_id, mesh_id, short_name, start_el, stop_el, data);
        return (rval);
    }
#endif

    fam = fam_list[fam_id];

    CHECK_WRITE_ACCESS(fam)

    ibuf_len = QTY_CONN_HEADER_FIELDS + 2;

    if ( mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }
    else
    {
        mesh = fam->meshes[mesh_id];
    }

    /* Keep track of elements defined. */
    rval = add_geom(mesh, short_name, start_el, stop_el);
    if ( rval != OK )
    {
        return rval;
    }

    /* Calculate the external record size. */
    rval = mc_query_family(fam_id, CLASS_SUPERCLASS, &mesh_id, short_name, (void *)&superclass);
    if ( rval != OK )
    {
        return rval;
    }
    data_words = (stop_el - start_el + 1) * conn_words[superclass];
    rec_size = (data_words + ibuf_len) * EXT_SIZE(fam, M_INT);

    /* Ensure file is open and positioned. */
    rval = prep_for_new_data(fam, NON_STATE_DATA);
    if ( rval != OK )
    {
        return rval;
    }

    /* Register an entry in the directory. */
    rval =
        add_dir_entry(fam, ELEM_CONNS, mesh_id, stop_el - start_el + 1, 1, &short_name, fam->next_free_byte, rec_size);
    if ( rval != OK )
    {
        return rval;
    }

    /*
     * Allocate and load the tags array.  Since there's only one block,
     * just use the tags array extended by two to pass the block.
     */
    tags = NEW_N(int, ibuf_len, "Conn tags buf");
    if ( ibuf_len > 0 && tags == NULL )
    {
        return ALLOC_FAILED;
    }
    tags[CONN_SUPERCLASS_IDX] = superclass;
    tags[QTY_BLOCKS_IDX] = 1;
    tags[QTY_BLOCKS_IDX + 1] = start_el;
    tags[QTY_BLOCKS_IDX + 2] = stop_el;

    /*
     * Write it out.
     */
    num_items_written = fam->write_funcs[M_INT](fam->cur_file, tags, ibuf_len);
    if ( num_items_written != ibuf_len )
    {
        return SHORT_WRITE;
    }
    num_items_written = fam->write_funcs[M_INT](fam->cur_file, data, data_words);
    if ( num_items_written != data_words )
    {
        return SHORT_WRITE;
    }

    fam->next_free_byte += rec_size;

    free(tags);

    return OK;
}

/*****************************************************************
 * TAG( mc_def_conn_arb ) PUBLIC
 *
 * Define connectivities, materials _?_, and part numbers_?_ for
 * an arbitrary list of elements of a given type.
 */
Return_value mc_def_conn_arb(Famid fam_id, int mesh_id, char *short_name, int el_qty, int *elem_ids, int *data)
{
    LONGLONG rec_size;
    LONGLONG data_words;
    Mesh_descriptor *mesh;
    int tags[QTY_CONN_HEADER_FIELDS];
    Mili_family *fam;
    Return_value rval;
    int *p_int_blk, *p_iblk, *ibound;
    int blk_qty;
    int superclass;
    int num_items_written;

    fam = fam_list[fam_id];

    CHECK_WRITE_ACCESS(fam)

    if ( mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }
    else
    {
        mesh = fam->meshes[mesh_id];
    }

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
        {
            free(p_int_blk);
            return rval;
        }
    }

    /* Calculate the external record size. */
    rval = mc_query_family(fam_id, CLASS_SUPERCLASS, &mesh_id, short_name, (void *)&superclass);
    if ( rval != OK )
    {
        free(p_int_blk);
        return rval;
    }
    data_words = el_qty * conn_words[superclass];
    rec_size = (data_words + QTY_CONN_HEADER_FIELDS + blk_qty * 2) * EXT_SIZE(fam, M_INT);

    /* Ensure file is open and positioned. */
    rval = prep_for_new_data(fam, NON_STATE_DATA);
    if ( rval != OK )
    {
        return rval;
    }

    /* Register an entry in the directory. */
    rval = add_dir_entry(fam, ELEM_CONNS, mesh_id, el_qty, 1, &short_name, fam->next_free_byte, rec_size);
    if ( rval != OK )
    {
        free(p_int_blk);
        return rval;
    }

    /* Load the tags array. */
    tags[CONN_SUPERCLASS_IDX] = superclass;
    tags[QTY_BLOCKS_IDX] = blk_qty;

    /*
     * Write it out...
     */
    num_items_written = fam->write_funcs[M_INT](fam->cur_file, tags, QTY_CONN_HEADER_FIELDS);
    if ( num_items_written != QTY_CONN_HEADER_FIELDS )
    {
        return SHORT_WRITE;
    }
    num_items_written = fam->write_funcs[M_INT](fam->cur_file, p_int_blk, (LONGLONG)blk_qty * 2);
    if ( num_items_written != (blk_qty * 2) )
    {
        return SHORT_WRITE;
    }
    num_items_written = fam->write_funcs[M_INT](fam->cur_file, data, data_words);
    if ( num_items_written != data_words )
    {
        return SHORT_WRITE;
    }

    fam->next_free_byte += rec_size;

    free(p_int_blk);

    return OK;
}

/*****************************************************************
 * TAG( mc_def_conn_arb_labels ) PUBLIC
 *
 * Define connectivities, materials _?_, and part numbers_?_ for
 * an arbitrary list of elements of a given type.
 *
 * This variation of the function also includes labels.
 */
Return_value mc_def_conn_arb_labels(Famid fam_id, int mesh_id, char *short_name, int el_qty, int *elem_ids, int *labels,
                                    int *data)
{
    Mili_family *fam;
    int i;
    int qty;
    int *labels_mats;
    Htable_entry *p_hte;

    Return_value rval;

    rval = mc_def_conn_arb(fam_id, mesh_id, short_name, el_qty, elem_ids, data);
    if ( rval == OK )
    {
        /* Include the material numbers with the labels - placed
         * at the end of the labels array.
         *

         * Determine the superclass for this class
         */

        fam = fam_list[fam_id];
        rval = htable_search(fam->meshes[mesh_id]->mesh_data.umesh_data, short_name, FIND_ENTRY, &p_hte);

        if ( p_hte != NULL )
        {
            qty = el_qty;
            ;

            labels_mats = NEW_N(int, qty * 2, "Labels with Materials");

            if ( qty > 0 && labels_mats == NULL )
            {
                return ALLOC_FAILED;
            }

            for ( i = 0; i < qty; i++ )
            {
                labels_mats[i] = elem_ids[i];
            }

            rval = mc_def_conn_labels(fam_id, mesh_id, short_name, el_qty, labels, labels_mats);

            free(labels_mats);
        }
    }

    return (rval);
}
/*****************************************************************
 * TAG( mc_def_conn_seq_labels ) PUBLIC
 *
 * Define connectivities, materials _?_, and part numbers_?_ for a
 * globally numbered sequential list of elements of a given type.
 *
 * This variation of the function also includes labels.
 */
Return_value mc_def_conn_seq_labels(Famid fam_id, int mesh_id, char *short_name, int start_el, int stop_el, int *labels,
                                    int *data)
{
    Mili_family *fam;
    int i;
    int qty;
    int *idents;
    Htable_entry *p_hte;
    int element_id;

    Return_value rval;

    rval = mc_def_conn_seq(fam_id, mesh_id, short_name, start_el, stop_el, data);
    if ( rval != OK )
    {
        return rval;
    }

    /* Include the material numbers with the labels - placed
     * at the end of the labels array.
     */

    /* Determine the superclass for this class */

    fam = fam_list[fam_id];
    rval = htable_search(fam->meshes[mesh_id]->mesh_data.umesh_data, short_name, FIND_ENTRY, &p_hte);
    if ( p_hte == NULL )
    {
        return rval;
    }

    qty = (stop_el - start_el) + 1;

    idents = NEW_N(int, qty, "Labels with materials");
    if ( qty > 0 && idents == NULL )
    {
        return ALLOC_FAILED;
    }

    element_id = start_el;
    for ( i = 0; i < qty; i++ )
    {
        idents[i] = element_id++;
    }

    rval = mc_def_conn_labels(fam_id, mesh_id, short_name, qty, labels, idents);

    free(idents);

    return rval;
}

Return_value mc_get_globals_and_locals(Famid fam_id, int mesh_id, char *class_name, int *global_ids, int *local_ids,
                                       int total_count)
{
    Mili_family *fam;
    Return_value status = OK;
    char new_name[128];
    char final_name[128];
    char final_local_name[128];
    int count = 0;
    int i, j;
    int param_array_type;
    int param_array_len;
    int current_size = 0;
    int *trash_pointer;
    int *trash_local_pointer;

    sprintf(new_name, "GLOBAL_IDS[/Mesh-%d/Sname-%s/DEF-", mesh_id, class_name);
    fam = fam_list[fam_id];

    count = htable_search_wildcard(fam->param_table, count, FALSE, new_name, "NULL", "NULL", NULL);
    for ( i = 0; i < count; i++ )
    {
        sprintf(final_name, "GLOBAL_IDS[/Mesh-%d/Sname-%s/DEF-%d]", mesh_id, class_name, i);
        sprintf(final_local_name, "GLOBAL_IDS_LOCAL_MAP[/Mesh-%d/Sname-%s/DEF-%d]", mesh_id, class_name, i);

        status = mc_read_param_array_len(fam_id, final_name, &param_array_type, &param_array_len);

        status = mc_read_param_array(fam_id, final_name, (void **)&trash_pointer);
        status = mc_read_param_array(fam_id, final_local_name, (void **)&trash_local_pointer);

        for ( j = 0; j < param_array_len; j++, current_size++ )
        {
            global_ids[current_size] = trash_pointer[j];
            local_ids[current_size] = trash_local_pointer[j];
        }

        free(trash_pointer);
        free(trash_local_pointer);
    }

    return status;
}
/*****************************************************************
 * TAG( mc_def_global_ids ) PUBLIC
 *
 * Define element global_ids -
 * Kevin Durrenberger
 */
Return_value mc_def_global_ids(Famid fam_id, int mesh_id, char *short_name, int qty, int *idents, int *global_ids)
{
    int dims[3];
    Mili_family *fam;
    Return_value status = OK;
    char label_prefix[32];
    char label_name[128];
    char new_name[128];

    Bool_type class_found = FALSE;
    int class_index = 0;
    int i;
    int cur_local_count = 0;
    char *fixed_local_count = "LOCAL_COUNT-";

    fam = fam_list[fam_id];

    CHECK_WRITE_ACCESS(fam)

    if ( mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }

    for ( i = 0; i < fam->num_global_id_classes; i++ )
    {
        if ( !strcmp(fam->global_id_class_list[i].mclass, short_name) )
        {
            class_found = TRUE;
            class_index = i;
        }
    }

    if ( i >= MAX_LABEL_CLASSES )
    {
        return TOO_MANY_LABELS;
    }

    if ( class_found )
    {
        fam->global_id_class_list[class_index].last_matid++;
    }
    else
    {
        str_dup(&(fam->global_id_class_list[fam->num_global_id_classes].mclass), short_name);
        fam->global_id_class_list[fam->num_global_id_classes].last_matid = 0;
        class_index = fam->num_global_id_classes;
        fam->num_global_id_classes++;
    }

    /* Write the labels and mats separately */
    dims[0] = qty;
    dims[1] = 0;
    dims[2] = 0;

    label_name[0] = '\0';
    label_prefix[0] = '\0';
    sprintf(new_name, "[/Mesh-%d/Sname-%s/DEF-%d]", mesh_id, short_name,
            fam->global_id_class_list[fam->num_global_id_classes - 1].last_matid);

    strncat(label_prefix, "GLOBAL_IDS", 11);

    strncat(label_name, label_prefix, 11);
    strcat(label_name, new_name);

    status = mc_ti_wrt_array(fam_id, M_INT, (char *)label_name, 1, dims, (void *)idents);
    if ( status != OK )
    {
        return status;
    }

    strncat(label_prefix, "_LOCAL_MAP", 10);
    label_name[0] = '\0';
    strncat(label_name, label_prefix, 21);
    strcat(label_name, new_name);
    status = mc_ti_wrt_array(fam_id, M_INT, (char *)label_name, 1, dims, (void *)global_ids);

    if ( status != OK )
    {
        return status;
    }

    label_name[0] = '\0';

    strcat(label_name, fixed_local_count);
    strcat(label_name, short_name);

    status = mc_read_scalar(fam_id, label_name, (void *)&cur_local_count);
    // htable_search(fam->param_table, label_name, FIND_ENTRY,&htable_entry);

    if ( status == OK )
    {
        cur_local_count += qty;
    }
    else
    {
        cur_local_count = qty;
    }

    status = write_scalar(fam, M_INT, label_name, &cur_local_count, TI_PARAM);

    return status;
}

/*****************************************************************
 * TAG( mc_def_conn_arb_labels ) PUBLIC
 *
 * Define connectivities, materials _?_, and part numbers_?_ for
 * an arbitrary list of elements of a given type.
 *
 * This variation of the function also includes labels.
 */
Return_value mc_def_conn_arb_labels_global_ids(Famid fam_id, int mesh_id, char *short_name, int el_qty, int *elem_ids,
                                               int *labels, int *data, int *global_ids)
{
    Mili_family *fam;
    int i;
    int qty;
    int *labels_mats;
    Htable_entry *p_hte;

    Return_value rval;

    rval = mc_def_conn_arb(fam_id, mesh_id, short_name, el_qty, elem_ids, data);
    if ( rval == OK )
    {
        /* Include the material numbers with the labels - placed
         * at the end of the labels array.
         *

         * Determine the superclass for this class
         */

        fam = fam_list[fam_id];
        rval = htable_search(fam->meshes[mesh_id]->mesh_data.umesh_data, short_name, FIND_ENTRY, &p_hte);

        if ( p_hte != NULL )
        {
            qty = el_qty;
            ;

            labels_mats = NEW_N(int, qty * 2, "Labels with Materials");

            if ( qty > 0 && labels_mats == NULL )
            {
                return ALLOC_FAILED;
            }

            for ( i = 0; i < qty; i++ )
            {
                labels_mats[i] = elem_ids[i];
            }

            rval = mc_def_conn_labels(fam_id, mesh_id, short_name, el_qty, labels, labels_mats);

            free(labels_mats);
        }
        mc_def_global_ids(fam_id, mesh_id, short_name, el_qty, elem_ids, global_ids);
    }

    return (rval);
}

Return_value mc_def_seq_global_ids(Famid fam_id, int mesh_id, char *short_name, int start_el, int stop_el,
                                   int *global_ids)
{
    int i, qty, element_id;
    int *element_ids;

    qty = (stop_el - start_el) + 1;

    element_ids = NEW_N(int, qty, "Element ids for globals");

    element_id = start_el;

    for ( i = 0; i < qty; i++ )
    {
        element_ids[i] = element_id++;
    }

    return mc_def_global_ids(fam_id, mesh_id, short_name, qty, element_ids, global_ids);
}
/*****************************************************************
 * TAG( mc_def_conn_seq_labels_global_ids ) PUBLIC
 *
 * Define connectivities, materials _?_, and part numbers_?_ for a
 * globally numbered sequential list of elements of a given type.
 *
 * This variation of the function also includes labels and global ids.
 */
Return_value mc_def_conn_seq_labels_global_ids(Famid fam_id, int mesh_id, char *short_name, int start_el, int stop_el,
                                               int *labels, int *data, int *global_ids)
{
    Return_value status;

    status = mc_def_conn_seq_labels(fam_id, mesh_id, short_name, start_el, stop_el, labels, data);

    if ( status == OK )
    {
        status = mc_def_seq_global_ids(fam_id, mesh_id, short_name, start_el, stop_el, global_ids);
    }

    return status;
}

Return_value mc_def_seq_labels(Famid fam_id, int mesh_id, char *short_name, int start_el, int stop_el, int *labels)
{
    int i, qty, element_id;
    int *element_ids;

    qty = (stop_el - start_el) + 1;

    element_ids = NEW_N(int, qty, "Generic Labels for arbitrary labels");

    element_id = start_el;

    for ( i = 0; i < qty; i++ )
    {
        element_ids[i] = element_id++;
    }
    // We send this to the functions
    return mc_def_conn_labels(fam_id, mesh_id, short_name, qty, labels, element_ids);
}
/*****************************************************************
 * TAG( mc_def_conn_labels ) PUBLIC
 *
 * Define element labels - labels are optional and are
 * implemented using the TI interface.
 *
 * DUE TO INITIAL ERRORS IN CREATING LABELS THE LABELS AND IDENTS ARE ACTUALLY PASSED IN REVERSED.
 * GRIZ AND XMILICS REQUIRE THIS AT THE MOMENT.
 */
Return_value mc_def_conn_labels(Famid fam_id, int mesh_id, char *short_name, int qty, int *labels, int *idents)
{
    int dims[3];
    Mili_family *fam;
    int temp_mesh_id = mesh_id, superclass;
    char label_name[128];
    char label_id_name[128];
    char label_descriptor[96];
    char superclass_name[64];
    int i;
    int state = 0;
    Return_value status;
    Bool_type class_found = FALSE;
    int class_index = 0;

    fam = fam_list[fam_id];

    CHECK_WRITE_ACCESS(fam)

    if ( mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }

    mc_ti_enable(fam_id);

    status = mc_ti_savedef_class(fam_id);
    if ( status != OK )
    {
        return status;
    }

    /* Determine the superclass */
    status = mc_query_family(fam_id, CLASS_SUPERCLASS, &temp_mesh_id, short_name, (void *)&superclass);

    if ( status != OK )
    {
        status = className_to_classEnum(short_name, &superclass);
        if ( status != OK )
        {
            return status;
        }
    }

    /* If we are writing out a set of materials for a single class then we
     * need to set the material number.
     */
    for ( i = 0; i < fam->num_label_classes; i++ )
    {
        if ( !strcmp(fam->label_class_list[i].mclass, short_name) )
        {
            class_found = TRUE;
            class_index = i;
        }
    }

    if ( i >= MAX_LABEL_CLASSES )
    {
        return TOO_MANY_LABELS;
    }

    if ( class_found )
    {
        fam->label_class_list[class_index].last_matid++;
    }
    else
    {
        str_dup(&(fam->label_class_list[fam->num_label_classes].mclass), short_name);
        fam->label_class_list[fam->num_label_classes].last_matid = 0;
        class_index = fam->num_label_classes;
        fam->num_label_classes++;
    }

    /* Convert superclass Enum to a name string */
    status = classEnum_to_className(superclass, superclass_name);
    if ( status != OK )
    {
        return status;
    }
    status = ti_make_label_description(mesh_id, fam->label_class_list[class_index].last_matid, superclass_name,
                                       short_name, label_descriptor);
    status = mc_ti_def_class(fam_id, mesh_id, state, fam->label_class_list[class_index].last_matid, superclass_name,
                             TRUE, FALSE, short_name, short_name);
    if ( status != OK )
    {
        return status;
    }

    /* Write the labels and mats separately */
    dims[0] = qty;
    dims[1] = 0;
    dims[2] = 0;

    strcpy(label_name, "Element Labels");
    strcat(label_name, label_descriptor);
    // status = mc_wrt_array( fam_id,  M_INT, (char *) label_name, 1, dims,
    //                           (void *)  labels );

    status = mc_ti_wrt_array(fam_id, M_INT, (char *)label_name, 1, dims, (void *)labels);

    if ( status != OK )
    {
        return status;
    }

    strcpy(label_id_name, "Element Labels-ElemIds");
    strcat(label_id_name, label_descriptor);

    status = mc_ti_wrt_array(fam_id, M_INT, (char *)label_id_name, 1, dims, (void *)idents);

    if ( status != OK )
    {
        return status;
    }

    status = mc_ti_restoredef_class(fam_id);
    if ( status != OK )
    {
        return status;
    }

    return status;
}

/*****************************************************************
 * TAG( mc_def_conn_surface ) PUBLIC
 *
 * Define connectivities, et cetera, for a
 * globally numbered sequential list of elements of a surface.
 */
Return_value mc_def_conn_surface(Famid fam_id, int mesh_id, char *short_name, int qty_of_facets, int *data,
                                 int *surface_id)
{
    Mili_family *fam;
    Mesh_descriptor *mesh;
    Hash_table *pht;
    Htable_entry *phte;
    Mesh_object_class_data *p_mocd;
    int rec_size, data_words;
    int tags[QTY_CONN_HEADER_FIELDS];
    int superclass;
    Return_value rval;
    int num_items_written;

    /* */

    fam = fam_list[fam_id];

    CHECK_WRITE_ACCESS(fam)

    if ( mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }
    else
    {
        mesh = fam->meshes[mesh_id];
    }

    /*
     * This section establishes the identifiers for each
     * instantiation of a particular type of surface, e.g.,
     * boundary surface, contact surface, rigid wall surface.
     *
     * The basis for identifying each surface will be its
     * order of being called via this procedure, "mf_def_conn_surface",
     * i.e., the first call will identify surface #1, ad infinitum.
     *
     * This section of code performs surface identification by
     * "counting" the number of particular surfaces already established
     * in the particular category of surface instantiation.  Information
     * such as this and the knowledge of "surface completeness" MAY be required
     * for use within GRIZ.  If so, uncomment this block of code.
     */

    /* Get pointer to the geometry hash table. */
    pht = mesh->mesh_data.umesh_data;

    /* Enter the class in the table or find it if it already exists. */
    rval = htable_search(pht, short_name, FIND_ENTRY, &phte);

    if ( phte == NULL )
    {
        return rval;
    }
    else
    {
        p_mocd = (Mesh_object_class_data *)phte->data;
    }

    if ( p_mocd->blocks->blocks == NULL )
    {
        *surface_id = 1;
    }
    else
    {
        *surface_id = p_mocd->blocks->blocks->stop + 1;
    }

    p_mocd->surface_sizes = RENEW_N(int, p_mocd->surface_sizes, *surface_id, 1, "No. of facets in surface");
    if ( p_mocd->surface_sizes == NULL )
    {
        return ALLOC_FAILED;
    }
    p_mocd->surface_sizes[*surface_id - 1] = qty_of_facets;

    /* Keep track of elements defined. */
    rval = add_geom(mesh, short_name, *surface_id, *surface_id);
    if ( rval != OK )
    {
        return rval;
    }

    rval = mc_query_family(fam_id, CLASS_SUPERCLASS, &mesh_id, short_name, (void *)&superclass);
    if ( rval != OK )
    {
        return rval;
    }

    data_words = qty_of_facets * conn_words[superclass];
    rec_size = (data_words + QTY_CONN_HEADER_FIELDS) * EXT_SIZE(fam, M_INT);

    /* Ensure file is open and positioned. */
    rval = prep_for_new_data(fam, NON_STATE_DATA);
    if ( rval != OK )
    {
        return rval;
    }

    /* Register an entry in the directory. */
    rval = add_dir_entry(fam, SURFACE_CONNS, mesh_id, qty_of_facets, 1, &short_name, fam->next_free_byte, rec_size);
    if ( rval != OK )
    {
        return rval;
    }

    /* Load the tags array. */
    tags[CONN_SUPERCLASS_IDX] = superclass;
    tags[QTY_BLOCKS_IDX] = qty_of_facets;

    /*
     * Write it out...
     */
    num_items_written = fam->write_funcs[M_INT](fam->cur_file, tags, QTY_CONN_HEADER_FIELDS);
    if ( num_items_written != QTY_CONN_HEADER_FIELDS )
    {
        return SHORT_WRITE;
    }
    num_items_written = fam->write_funcs[M_INT](fam->cur_file, data, data_words);
    if ( num_items_written != data_words )
    {
        return SHORT_WRITE;
    }

    fam->next_free_byte += rec_size;

    return OK;
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
#if TIMER
    clock_t start, stop, start1, stop1;
    double cumalative = 0.0;
    double cumalative1 = 0.0;
#endif
    Mesh_object_class_data *p_mocd;
    Return_value rval;
    Hash_table *pht;
    Htable_entry *phte;

    /* Get pointer to the geometry hash table. */
    pht = mesh->mesh_data.umesh_data;
#if TIMER
    start = clock();
#endif
    /* Enter the class in the table or find it if it already exists. */
    rval = htable_search(pht, short_name, FIND_ENTRY, &phte);
#if TIMER
    stop = clock();
    cumalative = (double)(1000 * (stop - start)) / CLOCKS_PER_SEC;
    if ( timed && cumalative > 0.5 )
    {
        printf("Function add_geom() htable_search Time is: %f ms\n", cumalative);
        printf("Long name: %s   -  Short Name: %s\n", p_mocd->long_name, p_mocd->short_name);
    }

    start = clock();
#endif
    /* Initialize the class if new. */
    if ( phte == NULL )
    {
        return rval;
    }
    else
    {
        p_mocd = (Mesh_object_class_data *)phte->data;
    }
#if TIMER
    stop = clock();
    cumalative = (double)(1000 * (stop - start)) / CLOCKS_PER_SEC;
    if ( timed && cumalative > 0.5 )
    {
        printf("Function add_geom() (Mesh_object_class_data *) phte->data Time is: %f ms\n", cumalative);
        printf("Long name: %s   -  Short Name: %s\n", p_mocd->long_name, p_mocd->short_name);
    }
    start = clock();
#endif
    rval = insert_range(p_mocd->blocks, start_mo, stop_mo);
#if TIMER
    stop = clock();
    cumalative = (double)(1000 * (stop - start)) / CLOCKS_PER_SEC;
    if ( timed && cumalative > 0.5 )
    {
        printf("Function add_geom() insert_range Opening time is: %f ms\n", cumalative);
        printf("Long name: %s   -  Short Name: %s\n", p_mocd->long_name, p_mocd->short_name);
    }

#endif
    return ((rval != OBJECT_RANGE_OVERLAP) ? OK : rval);
}

/*****************************************************************
 * TAG( insert_range ) PRIVATE
 *
 * Insert sort a range specification in a list of range spec's.
 * Sort by range start value in ascending order.  If new range
 * is numerically contiguous to an extant range, merge.
 */
Return_value insert_range(Block_list *p_bl, int start, int stop)
{
    Int_range *pir, *pir2;
    Return_value rval;

    pir = p_bl->blocks;
    pir2 = NULL;
    while ( pir )
    {
        /*
         * No overlaps, so now traverse to insert a new range.
         */
        if ( OVERLAP(pir->start, pir->stop, start, stop) )
        {
            return OBJECT_RANGE_OVERLAP;
        }

        if ( pir->start < start )
        {
            /* Can't insert yet; continue. */
            /* Save previous node in case we walk off the end. */
            pir2 = pir;
            pir = pir->next;
        }
        else
        {
            if ( pir->start - 1 == stop )
            {
                if ( pir2 && pir2->stop + 1 == start )
                {
                    pir2->stop = pir->stop;
                    pir2->next = pir->next;
                    if ( pir2->next )
                    {
                        pir2->next->prev = pir2;
                    }
                    p_bl->block_qty--;
                    free(pir);
                    pir = pir2;
                    rval = OBJECT_RANGE_COLLAPSE;
                }
                else
                {
                    pir->start = start;
                    rval = OBJECT_RANGE_MERGE;
                }
            }
            else
            {
                if ( pir->prev && pir->prev->stop + 1 == start )
                {
                    pir->prev->stop = stop;
                    rval = OBJECT_RANGE_MERGE;
                }
                else
                {
                    pir2 = NEW(Int_range, "Object id range");
                    if ( pir2 == NULL )
                    {
                        return ALLOC_FAILED;
                    }
                    pir2->start = start;
                    pir2->stop = stop;

                    pir2->next = pir;
                    pir2->prev = pir->prev;

                    if ( pir->prev )
                    {
                        pir->prev->next = pir2;
                    }
                    else
                    {
                        p_bl->blocks = pir2;
                    }

                    pir->prev = pir2;

                    p_bl->block_qty++;
                    rval = OBJECT_RANGE_INSERT;
                }
            }
            break;
        }
    }

    if ( pir == NULL )
    {
        if ( pir2 != NULL && pir2->stop + 1 == start )
        {
            pir2->stop = stop;
            rval = OBJECT_RANGE_MERGE;
        }
        else
        {
            /* Reached end of list. Insert new. */
            pir = NEW(Int_range, "Object id range");
            if ( pir == NULL )
            {
                return ALLOC_FAILED;
            }
            pir->start = start;
            pir->stop = stop;
            pir->next = NULL;
            if ( pir2 == NULL )
            {
                p_bl->blocks = pir;
                pir->prev = NULL;
            }
            else
            {
                pir2->next = pir;
                pir->prev = pir2;
            }
            p_bl->block_qty++;
            rval = OBJECT_RANGE_INSERT;
        }
    }

    p_bl->object_qty += stop - start + 1;

    return rval;
}

/*****************************************************************
 * TAG( check_object_ids ) PRIVATE
 *
 *
 */
Return_value check_object_ids(Block_list *mo_group, int qty_id_blks, int *mo_id_blks)
{
    Return_value rval;
    Int_range *pir;
    int *pi, *bound;

    /* Do until an id block is not fully contained in existing mesh def's. */
    bound = mo_id_blks + qty_id_blks * 2;
    rval = OK;
    for ( pi = mo_id_blks; pi < bound && rval == OK; pi += 2 )
    {
        /*
         * Assume a block is not contained, then loop through and check
         * until it is shown to be contained in the mesh.
         */
        pir = mo_group->blocks;
        rval = OBJECT_NOT_IN_MESH;
        while ( pir != NULL )
        {
            if ( pi[0] < pir->start || pi[1] > pir->stop )
            {
                pir = pir->next;
            }
            else
            {
                rval = OK;
                break;
            }
        }
    }

    return rval;
}

/*****************************************************************
 * TAG( delete_mo_data ) PRIVATE
 *
 * Delete a Mesh_object_class_data struct and its block list.  API designed
 * for compatibility with hashing utility function htable_delete().
 */
void delete_mo_data(void *ptr_mo_data)
{
    Mesh_object_class_data *p_mocd;

    p_mocd = (Mesh_object_class_data *)ptr_mo_data;

    free(p_mocd->long_name);
    free(p_mocd->short_name);

    DELETE_LIST(p_mocd->blocks->blocks);
    free(p_mocd->blocks);

    if ( NULL != p_mocd->surface_sizes )
    {
        free(p_mocd->surface_sizes);
    }

    free(p_mocd);
}

/*****************************************************************
 * TAG( delete_umesh ) LOCAL
 *
 * Delete an unstructured mesh descriptor and all referenced memory.
 */
static void delete_umesh(Mesh_descriptor *mesh)
{
    free(mesh->name);
    htable_delete(mesh->mesh_data.umesh_data, delete_mo_data, TRUE);
    free(mesh);
}

/*****************************************************************
 * TAG( delete_meshes ) PRIVATE
 *
 * Delete all in-memory meshes for a family.
 */
void delete_meshes(Mili_family *fam)
{
    void (*mdel_func)() = NULL;
    int i;

    switch ( fam->mesh_type )
    {
        case UNSTRUCTURED:
            mdel_func = delete_umesh;
            break;
        case UNDEFINED:
            break;
    }

    if ( mdel_func != NULL )
    {
        for ( i = 0; i < fam->qty_meshes; i++ )
        {
            (*mdel_func)(fam->meshes[i]);
        }
    }

    free(fam->meshes);
    fam->meshes = NULL;
}

/*****************************************************************
 * TAG( mc_load_nodes ) PUBLIC
 *
 * Read coordinates for all nodes of a M_NODE object class.
 *
 * This routine has been initially designed to support loading
 * both nodal coordinates and element connectivities, but only
 * the nodal functionality is implemented due to unresolved
 * data format compatibility issues (i.e., Griz wants to have
 * material and part number arrays as separate arrays, but we're
 * only passing one data pointer in and would like to maintain
 * one-to-one argument lists with the FORTRAN API).
 */
Return_value mc_load_nodes(Famid fam_id, int mesh_id, char *short_name, void *data)
{
    Mili_family *fam;
    int fil_qty, ent_qty, name_cnt;
    int i, j;
    Dir_entry *p_de;
    char **names;
    Return_value rval;
    Htable_entry *p_hte;
    Dir_entry_type etype;
    Return_value (*load_func)();

    rval = validate_fam_id(fam_id);
    if ( rval != OK )
    {
        return rval;
    }
    fam = fam_list[fam_id];

    if ( mesh_id < 0 || mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }

    /* Initialize for node coord load or element connectivity load. */
    rval = htable_search(fam->meshes[mesh_id]->mesh_data.umesh_data, short_name, FIND_ENTRY, &p_hte);
    if ( p_hte == NULL )
    {
        return rval;
    }
    else if ( ((Mesh_object_class_data *)p_hte->data)->superclass < M_NODE ||
              ((Mesh_object_class_data *)p_hte->data)->superclass > M_HEX )
    {
        return INVALID_CLASS_OPERATION;
    }
    else
    {
        if ( ((Mesh_object_class_data *)p_hte->data)->superclass == M_NODE )
        {
            etype = NODES;
            load_func = load_nodes_def;
        }
        else
        {
            etype = ELEM_CONNS;
            load_func = load_conn_def;
        }
    }

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
            if ( (Dir_entry_type)p_de[j][TYPE_IDX] == etype && p_de[j][MODIFIER1_IDX] == mesh_id &&
                 strcmp(names[name_cnt], short_name) == 0 )
            {
                /* Found an appropriate entry - load it into memory. */
                rval = load_func(fam, i, p_de[j], data);
                if ( rval != OK )
                {
                    return rval;
                }
            }

            name_cnt += p_de[j][STRING_QTY_IDX];
        }
    }

    return OK;
}

/*****************************************************************
 * TAG( mc_load_conns ) PUBLIC
 *
 * Read connectivities for all elements of a particular class.
 *
 */
Return_value mc_load_conns(Famid fam_id, int mesh_id, char *short_name, int *conns, int *mats, int *parts)
{
    Mili_family *fam;
    int fil_qty, ent_qty, name_cnt;
    int i, j;
    Dir_entry *p_de;
    char **names;
    Return_value rval;
    Htable_entry *p_hte;
    void *dest_ptrs[3];

    rval = validate_fam_id(fam_id);
    if ( rval != OK )
    {
        return rval;
    }
    fam = fam_list[fam_id];

    if ( mesh_id < 0 || mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }

    /* Initialize for element connectivity load. */
    rval = htable_search(fam->meshes[mesh_id]->mesh_data.umesh_data, short_name, FIND_ENTRY, &p_hte);
    if ( p_hte == NULL )
    {
        return rval;
    }
    else if ( ((Mesh_object_class_data *)p_hte->data)->superclass < M_TRUSS ||
              ((Mesh_object_class_data *)p_hte->data)->superclass > M_PARTICLE )
    {
        return INVALID_CLASS_OPERATION;
    }

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
                rval = load_conn_def(fam, i, p_de[j], (void *)dest_ptrs);
                if ( rval != OK )
                {
                    return rval;
                }
            }

            name_cnt += p_de[j][STRING_QTY_IDX];
        }
    }

    return OK;
}

/*****************************************************************
 * TAG( mc_load_conn_labels ) PUBLIC
 *
 * Read connectivity labels for all elements of a particular class.
 *
 * Return the blocking of non-contigious sets of labels.
 */

Return_value mc_load_conn_labels(Famid fam_id, int mesh_id, char *short_name, int qty, int *num_blocks,
                                 int **block_range, int *element_list, int *labels)
{
    Mili_family *fam;
    int i, j;
    int num_items_read, dummy;
    Return_value status;

    char sname[M_MAX_NAME_LEN];

    int *block_range_ptr;

    int *labels_ptr = NULL, *labels_elem_ptr = NULL;
    int total_qty_read = 0;
    int label_index = 0;

    char **wildcard_list_labels;
    int num_entries_labels = 0;

    char mili_version[M_MAX_NAME_LEN], host[M_MAX_NAME_LEN], arch[M_MAX_NAME_LEN], timestamp[M_MAX_NAME_LEN],
        xmilics_version[M_MAX_NAME_LEN];
    char *major, *minor;
    char id_prefix[32];
    char *wildcard_list_suffix;
    char wildcard_list_id[M_MAX_NAME_LEN];
    Hash_table *table;

    Bool_type particle_class = FALSE;

    *num_blocks = 0;
    *block_range = NULL;

    status = validate_fam_id(fam_id);
    if ( status != OK )
    {
        return status;
    }
    fam = fam_list[fam_id];
    if ( fam->char_header[DIR_VERSION_IDX] > 1 )
    {
        table = fam->param_table;
    }
    else
    {
        table = fam->ti_param_table;
    }
    if ( !fam->ti_enable )
    {
        return OK;
    }

    status = mc_ti_savedef_class(fam_id);
    if ( status != OK )
    {
        return status;
    }

    strcpy(sname, "Sname-");
    strncat(sname, short_name, M_MAX_NAME_LEN - 7);
    strcat(sname, "/");

    /* Load the list of Label names and id names */
    num_entries_labels = htable_search_wildcard(table, 0, FALSE, "Element Labels[", sname, "NULL", NULL);
    if ( num_entries_labels == 0 )
    {
        particle_class = mc_is_particle_class(fam_id, short_name);
        if ( particle_class )
        {
            strcpy(sname, "Sname-");
            strncat(sname, "ml", M_MAX_NAME_LEN - 7);
            num_entries_labels = htable_search_wildcard(table, 0, FALSE, "Element Labels[", sname, "NULL", NULL);
            /* Try all older naming convensions */
            if ( num_entries_labels == 0 )
                num_entries_labels =
                    htable_search_wildcard(table, 0, FALSE, "Element Labels[", "Sname-particle", "NULL", NULL);
            if ( num_entries_labels == 0 )
                num_entries_labels =
                    htable_search_wildcard(table, 0, FALSE, "Element Labels[", "Sname-particle-elem", "NULL", NULL);
            if ( num_entries_labels == 0 )
            {
                *num_blocks = 0;
                *block_range = NULL;
                return OK;
            }
        }
    } /*( num_entries_labels==0 )*/

    if ( num_entries_labels == 0 )
    {
        *num_blocks = 0;
        *block_range = NULL;
        return OK;
    }

    wildcard_list_labels = NEW_N(char *, num_entries_labels, "Element Labels");
    if ( num_entries_labels > 0 && wildcard_list_labels == NULL )
    {
        return ALLOC_FAILED;
    }

    num_entries_labels =
        htable_search_wildcard(table, 0, FALSE, "Element Labels[", sname, "NULL", wildcard_list_labels);

    /* Determine if this is a pre 8.1 database to figure out the naming of
     * the ids. */
    status = mc_get_metadata(fam_id, mili_version, host, arch, timestamp, xmilics_version);
    if ( status != OK )
    {
        return status;
    }

    /* If there was no version string assume an ancient version of Mili.
     * Otherwise parse the string. */
    if ( strlen(mili_version) == 0 )
    {
        strcpy(id_prefix, "Element Labels-Mats");
    }
    else
    {
        major = strtok(mili_version, "_");
        major[0] = '0';
        minor = strtok(NULL, "_");
        if ( atoi(major) < 8 )
        {
            strcpy(id_prefix, "Element Labels-Mats");
        }
        else if ( atoi(major) == 8 && atoi(minor) == 0 )
        {
            strcpy(id_prefix, "Element Labels-Mats");
        }
        else
        {
            strcpy(id_prefix, "Element Labels-ElemIds");
        }
    }

    status = mc_ti_undef_class(fam_id);
    if ( status != OK )
    {
        return status;
    }

    for ( i = 0; i < num_entries_labels; i++ )
    {
        status = mc_ti_read_array(fam_id, wildcard_list_labels[i], (void **)&labels_ptr, &num_items_read);
        if ( status != OK )
        {
            htable_delete_wildcard_list(num_entries_labels, wildcard_list_labels);
            free(wildcard_list_labels);
            if ( labels_ptr )
            {
                free(labels_ptr);
            }
            return status;
        }

        wildcard_list_suffix = strstr(wildcard_list_labels[i], "[");
        strcpy(wildcard_list_id, id_prefix);
        strcat(wildcard_list_id, wildcard_list_suffix);

        status = mc_ti_read_array(fam_id, wildcard_list_id, (void **)&labels_elem_ptr, &dummy);
        if ( status != OK )
        {
            if ( !strcmp(id_prefix, "Element Labels-Mats") )
                strcpy(wildcard_list_id, "Element Labels-ElemIds");
            if ( !strcmp(id_prefix, "Element Labels-ElemIds") )
                strcpy(wildcard_list_id, "Element Labels-Mats");
            strcat(wildcard_list_id, wildcard_list_suffix);

            status = mc_ti_read_array(fam_id, wildcard_list_id, (void **)&labels_elem_ptr, &dummy);
        }

        if ( status != OK || dummy != num_items_read )
        {
            htable_delete_wildcard_list(num_entries_labels, wildcard_list_labels);
            free(wildcard_list_labels);
            if ( labels_ptr )
            {
                free(labels_ptr);
            }
            if ( labels_elem_ptr )
            {
                free(labels_elem_ptr);
            }
            if ( status != OK )
            {
                return status;
            }
            else
            {
                return CONN_LABEL_ID_MISMATCH;
            }
        }
        for ( j = 0; j < num_items_read; j++ )
        {
            labels[label_index] = labels_ptr[j];
            element_list[label_index] = labels_elem_ptr[j];
            label_index++;
        }
        total_qty_read += num_items_read;

        if ( labels_ptr )
        {
            free(labels_ptr);
            labels_ptr = NULL;
        }
        if ( labels_elem_ptr )
        {
            free(labels_elem_ptr);
            labels_elem_ptr = NULL;
        }
    }

    status = mc_ti_restoredef_class(fam_id);
    if ( status != OK )
    {
        return status;
    }

    htable_delete_wildcard_list(num_entries_labels, wildcard_list_labels);
    free(wildcard_list_labels);

    if ( num_entries_labels == 0 )
    {
        *num_blocks = 0;
        *block_range = NULL;
        return OK;
    }
    /* Convert 1D list to 2D block list. */
    status = list_to_blocks(total_qty_read, labels, &block_range_ptr, num_blocks);
    if ( status != OK )
    {
        return status;
    }

    *block_range = block_range_ptr;

    return OK;
}

/*****************************************************************
 * TAG( mc_get_node_label_info ) PUBLIC
 *
 * Get information necessary to load node labels for all nodes of a
 * particular class.
 *
 */
Return_value mc_get_node_label_info(Famid fam_id, int mesh_id, char *short_name, int *num_blocks, int *num_labels)
{
    Mili_family *fam;
    int i;
    int num_items_read;
    int state = 0;
    Return_value status;
    int v1_file = 0;

    char label_name[64];
    char label_descriptor[96];

    int superclass;
    char superclass_name[M_MAX_NAME_LEN];

    /* Label Blocking data */
    int block_qty;

    int *labels_ptr = NULL;
    int *local_temp_labels;

    *num_blocks = 0;
    *num_labels = 0;

    status = validate_fam_id(fam_id);
    if ( status != OK )
    {
        return status;
    }
    fam = fam_list[fam_id];

    if ( !fam->ti_enable )
    {
        return OK;
    }
    if ( fam->char_header[DIR_VERSION_IDX] <= 1 )
    {
        v1_file = 1;
    }

    if ( v1_file )
    {
        status = mc_ti_savedef_class(fam_id);
        if ( status != OK )
        {
            return status;
        }
    }

    /* Determine the superclass */
    status = mc_query_family(fam_id, CLASS_SUPERCLASS, &mesh_id, short_name, (void *)&superclass);
    if ( status != OK )
    {
        return status;
    }

    /* Convert superclass Enum to a name string */
    status = classEnum_to_className(superclass, superclass_name);
    if ( status != OK )
    {
        return status;
    }

    if ( v1_file )
    {
        status = mc_ti_def_class(fam_id, mesh_id, state, 0, superclass_name, TRUE, TRUE, short_name, short_name);
    }
    else
    {
        status = ti_make_label_description(mesh_id, (int)-1, superclass_name, short_name, label_descriptor);
    }
    if ( status != OK )
    {
        return status;
    }

    if ( v1_file )
    {
        status = mc_ti_read_array(fam_id, "Node Labels", (void **)&labels_ptr, &num_items_read);
    }
    else
    {
        strcpy(label_name, "Node Labels");
        strcat(label_name, label_descriptor);
        status = mc_ti_read_array(fam_id, label_name, (void **)&labels_ptr, &num_items_read);
    }
    if ( status != OK )
    {
        return status;
    }

    if ( v1_file )
    {
        status = mc_ti_restoredef_class(fam_id);

        if ( status != OK )
        {
            return status;
        }
    }
    if ( num_items_read == 0 )
    {
        if ( labels_ptr )
        {
            free(labels_ptr);
        }
        return OK;
    }

    local_temp_labels = NEW_N(int, num_items_read, "Temp node labels - sorted");
    if ( num_items_read > 0 && local_temp_labels == NULL )
    {
        return ALLOC_FAILED;
    }

    for ( i = 0; i < num_items_read; i++ )
    {
        local_temp_labels[i] = labels_ptr[i];
    }

    /* Determine the number of blocks */
    /* Sort the labels */

    qsort(local_temp_labels, num_items_read, sizeof(int), mc_compare_labels);

    block_qty = 1;
    for ( i = 0; i < num_items_read - 2; ++i )
    {
        if ( local_temp_labels[i + 1] != local_temp_labels[i] + 1 )
        {
            block_qty++;
        }
    }
    if ( local_temp_labels[num_items_read - 1] != local_temp_labels[num_items_read - 2] + 1 )
    {
        block_qty++;
    }
    if ( local_temp_labels )
    {
        free(local_temp_labels);
    }
    if ( labels_ptr )
    {
        free(labels_ptr);
    }

    *num_blocks = block_qty;
    *num_labels = num_items_read;

    return status;
}

/*****************************************************************
 * TAG( mc_load_node_labels ) PUBLIC
 *
 * Read node labels for all nodes of a particular class.
 *
 */
Return_value mc_load_node_labels(Famid fam_id, int mesh_id, char *short_name, int *num_blocks, int **block_range,
                                 int *labels)
{
    Mili_family *fam;
    int i;
    int num_items_read;
    Return_value status;

    int superclass, temp_mesh_id = mesh_id;
    char superclass_name[M_MAX_NAME_LEN];
    char label_name[64];
    char label_descriptor[96];
    /* Label Blocking data */
    int block_qty, block_range_index = 0, *block_range_ptr;
    int start_of_block = 0, end_of_block = 0;

    int *labels_ptr = NULL, *temp_labels = NULL;

    char *labels_var = NULL;
    Bool_type labels_found = FALSE;

    *num_blocks = 0;
    *block_range = NULL;

    status = validate_fam_id(fam_id);
    if ( status != OK )
    {
        return status;
    }
    fam = fam_list[fam_id];

    if ( !fam->ti_enable )
    {
        return OK;
    }

    status = mc_ti_savedef_class(fam_id);
    if ( status != OK )
    {
        return status;
    }

    /* Determine the superclass */
    status = mc_query_family(fam_id, CLASS_SUPERCLASS, &temp_mesh_id, short_name, (void *)&superclass);
    if ( status != OK )
    {
        return status;
    }

    /* Convert superclass Enum to a name string */
    status = classEnum_to_className(superclass, superclass_name);
    if ( status != OK )
    {
        return status;
    }
    status = ti_make_label_description(mesh_id, (int)-1, superclass_name, short_name, label_descriptor);
    /*
    status = mc_ti_def_class( fam_id, mesh_id, state, 0,
                              superclass_name, TRUE, TRUE,
                              short_name, short_name);
    */
    if ( status != OK )
    {
        return status;
    }
    strcpy(label_name, "Node Labels");
    strcat(label_name, label_descriptor);
    status = mc_ti_read_array(fam_id, label_name, (void **)&labels_ptr, &num_items_read);
    if ( status != OK )
    {
        labels_found = mc_ti_find_var_name(fam_id, "Node Labels", short_name, &labels_var);
        if ( labels_found )
            status = mc_ti_read_array(fam_id, labels_var, (void **)&labels_ptr, &num_items_read);
        if ( labels_var != NULL )
        {
            free(labels_var);
        }
        if ( status != OK )
        {
            return status;
        }
    }

    status = mc_ti_restoredef_class(fam_id);
    if ( status != OK )
    {
        return status;
    }
    if ( num_items_read == 0 )
    {
        if ( labels_ptr )
        {
            free(labels_ptr);
        }
        return OK;
    }

    for ( i = 0; i < num_items_read; i++ )
    {
        labels[i] = labels_ptr[i];
    }

    if ( labels_ptr )
    {
        free(labels_ptr);
    }

    /* Determine the number of blocks */
    /* Sort the labels */

    temp_labels = NEW_N(int, num_items_read, "Temp node labels - sorted");
    if ( num_items_read > 0 && temp_labels == NULL )
    {
        return ALLOC_FAILED;
    }

    for ( i = 0; i < num_items_read; i++ )
    {
        temp_labels[i] = labels[i];
    }

    qsort(temp_labels, num_items_read, sizeof(int), mc_compare_labels);

    block_qty = 1;
    for ( i = 0; i < num_items_read - 2; i++ )
    {
        if ( temp_labels[i + 1] != temp_labels[i] + 1 )
        {
            block_qty++;
        }
    }
    if ( temp_labels[num_items_read - 1] != temp_labels[num_items_read - 2] + 1 )
    {
        block_qty++;
    }

    /* Construct the blocking data */
    block_range_ptr = (int *)malloc(block_qty * 2 * sizeof(int));
    if ( block_qty > 0 && block_range_ptr == NULL )
    {
        return ALLOC_FAILED;
    }

    start_of_block = temp_labels[0];
    end_of_block = temp_labels[0];

    for ( i = 0; i < num_items_read - 2; i++ )
    {
        if ( end_of_block + 1 != temp_labels[i + 1] )
        {
            block_range_ptr[block_range_index++] = start_of_block;
            block_range_ptr[block_range_index++] = end_of_block;

            start_of_block = temp_labels[i + 1];
            end_of_block = temp_labels[i + 1];
        }
        else
        {
            end_of_block++;
        }
    }

    /* Include last label just read in */
    if ( temp_labels[num_items_read - 1] == end_of_block + 1 )
    {
        end_of_block++;
        block_range_ptr[block_range_index++] = start_of_block;
        block_range_ptr[block_range_index++] = end_of_block;
    }
    else
    {
        /* Special case when last label is all by itself */
        block_range_ptr[block_range_index++] = start_of_block;
        block_range_ptr[block_range_index++] = end_of_block;

        block_range_ptr[block_range_index++] = temp_labels[num_items_read - 1];
        block_range_ptr[block_range_index++] = temp_labels[num_items_read - 1];
    }

    if ( temp_labels )
    {
        free(temp_labels);
    }

    *block_range = block_range_ptr;
    *num_blocks = block_qty;

    return status;
}

/*****************************************************************
 * TAG( mc_load_surface ) PUBLIC
 *
 * Read connectivities for all nodes of a surface class.
 *
 */
Return_value mc_load_surface(Famid fam_id, int mesh_id, int surf, char *short_name, void *conns)
{
    Mili_family *fam;
    int fil_qty, ent_qty, name_cnt;
    int i, j;
    int this_surf;
    Dir_entry *p_de;
    char **names;
    Return_value rval;
    Htable_entry *p_hte;

    rval = validate_fam_id(fam_id);
    if ( rval != OK )
    {
        return rval;
    }
    fam = fam_list[fam_id];

    if ( mesh_id < 0 || mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }

    /* Initialize for element connectivity load. */
    rval = htable_search(fam->meshes[mesh_id]->mesh_data.umesh_data, short_name, FIND_ENTRY, &p_hte);
    if ( p_hte == NULL )
    {
        return rval;
    }
    else if ( M_SURFACE != ((Mesh_object_class_data *)p_hte->data)->superclass )
    {
        return INVALID_CLASS_OPERATION;
    }

    fil_qty = fam->file_count;

    /* For each non-state file... */
    for ( i = 0; i < fil_qty; i++ )
    {
        ent_qty = fam->directory[i].qty_entries;
        p_de = fam->directory[i].dir_entries;
        names = fam->directory[i].names;
        name_cnt = 0;
        this_surf = 0;

        /* For each directory entry in file... */
        for ( j = 0; j < ent_qty; j++ )
        {
            if ( (Dir_entry_type)p_de[j][TYPE_IDX] == SURFACE_CONNS && p_de[j][MODIFIER1_IDX] == mesh_id &&
                 strcmp(names[name_cnt], short_name) == 0 )
            {
                if ( surf == this_surf )
                {
                    /* Found an appropriate entry - load it into memory. */

                    rval = load_conn_def_surface(fam, i, p_de[j], conns);

                    if ( rval != OK )
                    {
                        return rval;
                    }
                }
                this_surf += 1;
            }

            name_cnt += p_de[j][STRING_QTY_IDX];
        }
    }

    return OK;
}

/*****************************************************************
 * TAG( load_conn_def ) LOCAL
 *
 * Read element connectivity family entry into memory.
 */
static Return_value load_conn_def(Mili_family *fam, int file_idx, Dir_entry dir_ent, void *dest_ptrs)
{
    int econn_hdr[QTY_CONN_HEADER_FIELDS];
    int *elem_blks, *ebuf;
    int *p_src, *bound;
    int blk_qty, elem_qty, word_qty, conn_qty;
    int superclass;
    int offset, mat_offset;
    LONGLONG read_cnt;
    int *conns, *conns_dest, *mats, *mats_dest, *parts, *parts_dest;
    int i, j;
    Return_value rval;

    conns = ((int **)dest_ptrs)[0];
    mats = ((int **)dest_ptrs)[1];
    parts = ((int **)dest_ptrs)[2];

    /* Seek to the connectivity entry in the family. */
    rval = non_state_file_open(fam, file_idx, fam->access_mode);
    if ( rval != OK )
    {
        return rval;
    }
    rval = non_state_file_seek(fam, dir_ent[OFFSET_IDX]);
    if ( rval != OK )
    {
        return rval;
    }

    /* Read the nodal data header (start and stop id's). */
    read_cnt = fam->read_funcs[M_INT](fam->cur_file, econn_hdr, QTY_CONN_HEADER_FIELDS);
    if ( read_cnt != QTY_CONN_HEADER_FIELDS )
    {
        return SHORT_READ;
    }

    blk_qty = econn_hdr[QTY_BLOCKS_IDX];
    elem_blks = NEW_N(int, blk_qty * 2, "Conn elem blocks");
    if ( blk_qty > 0 && elem_blks == NULL )
    {
        return ALLOC_FAILED;
    }
    read_cnt = fam->read_funcs[M_INT](fam->cur_file, elem_blks, blk_qty * 2);
    if ( read_cnt != (blk_qty * 2) )
    {
        return SHORT_READ;
    }

    /* Allocate temporary buffer and read element connectivities. */
    superclass = econn_hdr[CONN_SUPERCLASS_IDX];
    elem_qty = dir_ent[MODIFIER2_IDX];
    word_qty = conn_words[superclass];
    conn_qty = word_qty - 2;
    mat_offset = word_qty - 1;
    ebuf = NEW_N(int, elem_qty *word_qty, "Elem conns buf");
    if ( elem_qty * word_qty > 0 && ebuf == NULL )
    {
        free(elem_blks);
        return ALLOC_FAILED;
    }
    read_cnt = fam->read_funcs[M_INT](fam->cur_file, ebuf, elem_qty * word_qty);
    if ( read_cnt != (elem_qty * word_qty) )
    {
        return SHORT_READ;
    }

    /* Loop over the blocks defined in this family entry. */
    bound = NULL;
    p_src = ebuf;
    for ( i = 0; i < blk_qty; i++ )
    {
        /*
         * Calculate the offset into the class's connectivity data for the
         * current element block.
         */
        offset = *(elem_blks + i * 2) - 1;

        /* Initialize pointers into the destination data arrays. */
        conns_dest = conns + (offset * conn_qty);
        mats_dest = mats + offset;
        parts_dest = parts + offset;

        /* Calculate element quantity for the current block. */
        elem_qty = *(elem_blks + i * 2 + 1) - *(elem_blks + i * 2) + 1;

        /* Calculate bounding address for current block's data in input buf. */
        bound = p_src + elem_qty * word_qty;

        /* Copy connectivities, material, and part into appropriate buffers. */
        for ( ; p_src < bound; p_src += word_qty )
        {
            for ( j = 0; j < conn_qty; j++ )
            {
                *(conns_dest + j) = *(p_src + j) - 1;
            }
            conns_dest += conn_qty;
            *mats_dest++ = *(p_src + conn_qty) - 1;
            *parts_dest++ = *(p_src + mat_offset) - 1;
        }

        /* Next block's data starts at current block's bound. */
        p_src = bound;
    }

    free(elem_blks);
    free(ebuf);

    return OK;
}

/*****************************************************************
 * TAG( load_conn_def_surface ) LOCAL
 *
 * Read element connectivity family entry into memory.
 */
static Return_value load_conn_def_surface(Mili_family *fam, int file_idx, Dir_entry dir_ent, void *dest_ptr)
{
    int econn_hdr[QTY_CONN_HEADER_FIELDS];
    int read_cnt;
    int facet_qty;
    int *conns;
    int i;
    Return_value rval;

    rval = OK;
    conns = (int *)dest_ptr;

    /* Seek to the connectivity entry in the family. */
    rval = non_state_file_open(fam, file_idx, fam->access_mode);
    if ( rval != OK )
    {
        return rval;
    }
    rval = non_state_file_seek(fam, dir_ent[OFFSET_IDX]);
    if ( rval != OK )
    {
        return rval;
    }

    /* Read the connectivity data header. */
    read_cnt = fam->read_funcs[M_INT](fam->cur_file, econn_hdr, QTY_CONN_HEADER_FIELDS);
    if ( read_cnt != QTY_CONN_HEADER_FIELDS )
    {
        return SHORT_READ;
    }

    /* Allocate temporary buffer and read element connectivities. */
    facet_qty = dir_ent[MODIFIER2_IDX];
    read_cnt = fam->read_funcs[M_INT](fam->cur_file, conns, facet_qty * 4);
    if ( read_cnt != (facet_qty * 4) )
    {
        return SHORT_READ;
    }

    for ( i = 0; i < facet_qty * 4; i++ )
    {
        *(conns + i) -= 1;
    }

    return rval;
}

/*****************************************************************
 * TAG( load_nodes_def ) LOCAL
 *
 * Read nodal coordinates family entry into memory.
 */
static Return_value load_nodes_def(Mili_family *fam, int file_idx, Dir_entry dir_ent, void *coords)
{
    int node_hdr[QTY_NODE_TAGS];
    LONGLONG float_qty, read_cnt;
    float *dest;
    Return_value rval;

    /* Seek to the nodal data entry in the family. */
    rval = non_state_file_open(fam, file_idx, fam->access_mode);
    if ( rval != OK )
    {
        return rval;
    }
    rval = non_state_file_seek(fam, dir_ent[OFFSET_IDX]);
    if ( rval != OK )
    {
        return rval;
    }

    /* Read the nodal data header (start and stop id's). */
    read_cnt = fam->read_funcs[M_INT](fam->cur_file, node_hdr, QTY_NODE_TAGS);
    if ( read_cnt != QTY_NODE_TAGS )
    {
        return SHORT_READ;
    }

    /* Calculate destination in coords buffer for this node block. */
    dest = ((float *)coords) + ((node_hdr[0] - 1) * fam->dimensions);

    /* Calculate quantity of floats to read. */
    float_qty = (node_hdr[1] - node_hdr[0] + 1) * fam->dimensions;

    /* Read coordinates from file. */
    read_cnt = fam->read_funcs[M_FLOAT](fam->cur_file, dest, float_qty);

    rval = (read_cnt == float_qty) ? OK : SHORT_READ;

    return rval;
}

/*****************************************************************
 * TAG( get_class_qty ) PRIVATE
 *
 * Find the number of classes defined for a superclass.
 */
Return_value get_class_qty(Mili_family *fam, int *modifiers, int *class_qty)
{
    int mesh_id, superclass;
    int i;
    Return_value rval;

    mesh_id = modifiers[0];
    superclass = modifiers[1];

    *class_qty = 0;
    rval = reset_class_data(fam, mesh_id, FALSE);
    if ( rval != OK )
    {
        return rval;
    }
    if ( superclass == M_ALL )
    {
        *class_qty = all_class_count;
    }
    else
    {
        for ( i = 0; i < all_class_count; i++ )
        {
            if ( class_data[i]->superclass == superclass )
            {
                (*class_qty)++;
            }
        }
    }

    return OK;
}

/*****************************************************************
 * TAG( count_elem_conn_classes ) PRIVATE
 *
 * Count the number of element connectivity definition blocks for
 * a particular mesh object class.
 */
void get_elem_conn_classes_names(Mili_family *fam, int mesh_id, int *count, char ***out_names)
{
    int fil_qty, ent_qty, name_cnt, found = 0;
    int qty_conn_entries;
    int i, j, k;
    Dir_entry *p_de;
    char **names;
    char **ret_names;
    int ret_count = 0;
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
            if ( (Dir_entry_type)p_de[j][TYPE_IDX] == ELEM_CONNS && p_de[j][MODIFIER1_IDX] == mesh_id )
            {
                qty_conn_entries++;
            }
            name_cnt += p_de[j][STRING_QTY_IDX];
        }
    }
    ret_names = (char **)malloc(sizeof(char *) * qty_conn_entries);

    for ( i = 0; i < fil_qty; i++ )
    {
        ent_qty = fam->directory[i].qty_entries;
        p_de = fam->directory[i].dir_entries;
        names = fam->directory[i].names;
        name_cnt = 0;
        /* For each directory entry in file... */
        for ( j = 0; j < ent_qty; j++ )
        {
            if ( (Dir_entry_type)p_de[j][TYPE_IDX] == ELEM_CONNS && p_de[j][MODIFIER1_IDX] == mesh_id )
            {
                found = 0;
                for ( k = 0; k < ret_count; k++ )
                {
                    if ( strcmp(names[name_cnt], ret_names[k]) == 0 )
                    {
                        found = 1;
                        break;
                    }
                }
                if ( !found )
                {
                    ret_names[ret_count] = malloc(sizeof(char) * strlen(names[name_cnt]) + 1);
                    ret_names[ret_count][0] = '\0';
                    strcpy(ret_names[ret_count], names[name_cnt]);
                    ret_count++;
                }
            }
            name_cnt += p_de[j][STRING_QTY_IDX];
        }
    }
    *count = ret_count;
    *out_names = ret_names;
}

/*****************************************************************
 * TAG( count_elem_conn_defs ) PRIVATE
 *
 * Count the number of element connectivity definition blocks for
 * a particular mesh object class.
 */
int count_elem_conn_defs(Mili_family *fam, int mesh_id, char *short_name)
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
                 strcmp(names[name_cnt], short_name) == 0 )
            {
                qty_conn_entries += p_de[j][MODIFIER2_IDX];
            }

            name_cnt += p_de[j][STRING_QTY_IDX];
        }
    }
    return qty_conn_entries;
}

/*****************************************************************
 * TAG( get_elem_qty_in_def ) PRIVATE
 *
 * Get the number of elements defined in a directory ELEM_CONNS
 * entry for a specified mesh and object class.
 * The current directory traversal state is saved so that a
 * sequential series of calls with steadily increasing entry
 * index can be accommodated without redundantly searching the
 * directory from the beginning at each call.
 */
Return_value get_elem_qty_in_def(Mili_family *fam, int *int_args, char *short_name, int *elem_qty)
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
    {
        return NO_MESH;
    }

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
                {
                    econn_entry_cnt++;
                }
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

    return ((searching ? INVALID_INDEX : OK));
}

/*****************************************************************
 * TAG( count_node_entries ) PRIVATE
 *
 * Count the number of nodal coordinate entries for a particular
 * nodal class and mesh of a family.
 */
int count_node_entries(Mili_family *fam, int mesh_id, char *short_name)
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
                 strcmp(names[name_cnt], short_name) == 0 )
            {
                qty_node_entries++;
            }

            name_cnt += p_de[j][STRING_QTY_IDX];
        }
    }

    return qty_node_entries;
}

/*****************************************************************
 * TAG( mc_get_class_idents ) PUBLIC
 *
 * Returns the superclass for a class name.
 */
Return_value mc_get_class_idents(Famid fam_id, int mesh_id, char *class_name, int *num_blocks, int **idents)

{
    Mili_family *fam;
    int i, j;
    int *temp_idents = NULL;
    Return_value rval;

    rval = validate_fam_id(fam_id);
    if ( rval != OK )
    {
        return rval;
    }
    fam = fam_list[fam_id];

    if ( mesh_id < 0 || mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }

    rval = reset_class_data(fam, mesh_id, FALSE);
    if ( rval != OK )
    {
        return rval;
    }

    /*
     * Search through classes for specified class name and return the
     * class type or superclass.
     */
    for ( i = 0; i < all_class_count; i++ )
    {
        if ( !strcmp(class_data[i]->short_name, class_name) )
        {
            *num_blocks = class_data[i]->blocks->block_qty;
            if ( class_data[i]->blocks->block_qty > 0 )
            {
                temp_idents = NEW_N(int, class_data[i]->blocks->block_qty * 2, "Class idents");
                if ( class_data[i]->blocks->block_qty > 0 && temp_idents == NULL )
                {
                    rval = ALLOC_FAILED;
                    break;
                }
                for ( j = 0; j < class_data[i]->blocks->block_qty; j++ )
                {
                    temp_idents[j * 2] = class_data[i]->blocks->blocks[j].start;
                    temp_idents[(j * 2) + 1] = class_data[i]->blocks->blocks[j].stop;
                }
                *idents = temp_idents;
                break;
            }
        }
    }

    return rval;
}

/*****************************************************************
 *
 * MERGE WITH get_elem_qty_in_def()!
 *
 * TAG( get_node_block_size ) PRIVATE
 *
 * Get the number of nodes defined in a directory NODES entry.
 * The current directory traversal state is saved so that a
 * sequential series of calls with steadily increasing block
 * index can be accommodated without redundantly searching the
 * directory from the beginning at each call.
 */
Return_value get_node_block_size(Mili_family *fam, int *modifiers, char *short_name, int *blk_size)
{
    Mili_family *last_fam = NULL;
    static int mesh_id = -1;
    static int blk_index = -1;
    static int i, j;
    static int node_entry_cnt, name_cnt;
    int fil_qty, ent_qty;
    int mi;
    Dir_entry *p_de;
    Bool_type searching;
    char **names;
    static char last_class[128] = {'\0'};

    mi = modifiers[0];
    if ( fam == NULL || mi < 0 || mi > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }

    if ( fam != last_fam || mi != mesh_id || modifiers[1] < node_entry_cnt )
    {
        /* This is not a sequential successor of a previous search; re-init. */
        last_fam = fam;
        mesh_id = mi;
        strcpy(last_class, short_name);
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
                 strcmp(names[name_cnt], short_name) == 0 )
            {
                if ( node_entry_cnt == blk_index )
                {
                    /* Found desired entry.  Get block size and get out. */
                    *blk_size = p_de[j][MODIFIER2_IDX];
                    searching = FALSE;
                    break;
                }
                else
                {
                    node_entry_cnt++;
                }
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

    return ((searching ? INVALID_INDEX : OK));
}

/*****************************************************************
 * TAG( mc_get_class_info ) PUBLIC
 *
 * Get the name(s) and number of objects in a class identified by
 * an index number.
 */
Return_value mc_get_class_info(Famid fam_id, int mesh_id, int superclass, int class_index, char *short_name,
                               char *long_name, int *object_qty)
{
    Mili_family *fam;
    int i, j;
    Return_value rval;

    rval = validate_fam_id(fam_id);
    if ( rval != OK )
    {
        return rval;
    }
    fam = fam_list[fam_id];

    if ( mesh_id < 0 || mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }

    if ( class_index < 0 )
    {
        return NO_CLASS;
    }

    rval = reset_class_data(fam, mesh_id, FALSE);
    if ( rval != OK )
    {
        return rval;
    }

    /*
     * Search through classes for defined superclass to find the
     * "class_index'th" class occurrence
     */
    j = -1;
    for ( i = 0; i < all_class_count && j != class_index; i++ )
    {
        if ( class_data[i]->superclass == superclass )
        {
            j++;
        }
    }

    if ( j != class_index )
    {
        return NO_CLASS;
    }
    else
    {
        strcpy(short_name, class_data[i - 1]->short_name);
        strcpy(long_name, class_data[i - 1]->long_name);
        *object_qty = class_data[i - 1]->blocks->object_qty;
    }

    return OK;
}

/*****************************************************************
 * TAG( mc_get_class_info_by_name ) PUBLIC
 *
 * Get the name(s) and start/stop identifiers in a class identified by
 * an index number.
 */
Return_value mc_get_class_info_by_name(Famid fam_id, int *mesh_id, const char *in_short_name, int *superclass,
                                       int *class_index, char *in_long_name, int *num_elems)
{
    int i, j, cur_mesh, nmeshes;
    int args[2];
    int ngroups;
    int nelems;
    char short_name[1024];
    char long_name[1024];
    Return_value status;

    in_long_name[0] = '\0';
    status = mc_query_family(fam_id, QTY_MESHES, NULL, NULL, (void *)&nmeshes);
    if ( status != OK )
    {
        return status;
    }
    for ( cur_mesh = 0; cur_mesh < nmeshes; cur_mesh++ )
    {
        for ( i = 0; i < M_QTY_SUPERCLASS; i++ )
        {
            args[0] = cur_mesh;
            args[1] = i;

            ngroups = 0;

            status = mc_query_family(fam_id, QTY_CLASS_IN_SCLASS, (void *)args, NULL, (void *)&ngroups);
            if ( status != OK )
            {
                return status;
            }
            for ( j = 0; j < ngroups; j++ )
            {
                nelems = 0;
                status = mc_get_class_info(fam_id, cur_mesh, i, j, short_name, long_name, &nelems);
                if ( status != OK )
                {
                    return status;
                }
                if ( strcmp(short_name, in_short_name) == 0 )
                {
                    *superclass = i;
                    *class_index = j;
                    strcat(in_long_name, long_name);
                    *num_elems = nelems;
                    *mesh_id = cur_mesh;
                    return OK;
                }
            }
        }
    }
    return INVALID_CLASS_NAME;
}
/*****************************************************************
 * TAG( mc_get_simple_class_info ) PUBLIC
 *
 * Get the name(s) and start/stop identifiers in a class identified by
 * an index number.
 */
Return_value mc_get_simple_class_info(Famid fam_id, int mesh_id, int superclass, int class_index, char *short_name,
                                      char *long_name, int *start_ident, int *stop_ident)
{
    Mili_family *fam;
    int i, j;
    Return_value rval;

    rval = validate_fam_id(fam_id);
    if ( rval != OK )
    {
        return rval;
    }
    fam = fam_list[fam_id];

    if ( mesh_id < 0 || mesh_id > fam->qty_meshes - 1 )
    {
        return NO_MESH;
    }

    if ( class_index < 0 )
    {
        return NO_CLASS;
    }

    rval = reset_class_data(fam, mesh_id, FALSE);
    if ( rval != OK )
    {
        return rval;
    }

    /*
     * Search through classes for defined superclass to find the
     * "class_index'th" class occurrence
     */
    j = -1;
    for ( i = 0; i < all_class_count && j != class_index; i++ )
    {
        if ( class_data[i]->superclass == superclass )
        {
            j++;
        }
    }

    if ( j != class_index )
    {
        return NO_CLASS;
    }
    else
    {
        strcpy(short_name, class_data[i - 1]->short_name);
        strcpy(long_name, class_data[i - 1]->long_name);
        *start_ident = class_data[i - 1]->blocks->blocks->start;
        *stop_ident = class_data[i - 1]->blocks->blocks->stop;
    }

    return OK;
}

/*****************************************************************
 * TAG( reset_class_data ) PRIVATE
 *
 * Load global variables for object class queries.
 */
Return_value reset_class_data(Mili_family *fam, int mesh_id, Bool_type force_init)
{
    Mesh_descriptor *p_md;
    Return_value rval = OK;

    if ( force_init )
    {
        class_data_fam = NULL;
        class_data_mesh_id = -1;
        all_class_count = 0;
        if ( class_data != NULL )
        {
            free(class_data);
            class_data = NULL;
        }
    }
    else if ( fam != class_data_fam || mesh_id != class_data_mesh_id )
    {
        if ( class_data != NULL )
        {
            free(class_data);
        }

        class_data_fam = fam;
        class_data_mesh_id = mesh_id;
        p_md = fam->meshes[mesh_id];
        rval = htable_get_data(p_md->mesh_data.umesh_data, (void ***)&class_data, &all_class_count);
    }
    return rval;
}

/*****************************************************************
 * TAG( init_meshes ) PRIVATE
 *
 * Load all meshes defined in a family.
 *
 * Should be called only at family initialization.
 */
Return_value init_meshes(Mili_family *fam)
{
#if TIMER
    clock_t start, stop, start1, stop1, start2, stop2;
    double cumalative = 0.0;
    double cumalative1 = 0.0;
    double cumalative2 = 0.0;
    start = clock();
#endif

    int fil_qty, ent_qty;
    Dir_entry *p_de;
    Dir_entry_type etype;
    int conn_hdr[QTY_CONN_HEADER_FIELDS];
    int node_hdr[QTY_NODE_TAGS];
    int ident_entry[QTY_IDENT_ENTRY_FIELDS];
    char **names;
    int mod1;
    Return_value rval;
    char pname[132];
    int i, j, k;
    int blk_qty, name_cnt;
    int *elem_blks;
    Hash_table *p_ht;
    Htable_entry *p_hte;
    int surface_id;
    Mesh_object_class_data *p_mocd;
    int read_ct;

    rval = htable_search(fam->param_table, "mesh type", FIND_ENTRY, &p_hte);
    if ( rval == ENTRY_NOT_FOUND )
    {
        return OK; /* Not found -> no meshes. */
    }
    else if ( rval == OK )
    {
        rval = read_scalar(fam, (Param_ref *)p_hte->data, &fam->mesh_type);
        if ( rval != OK )
        {
            return rval;
        }
    }
    else
    {
        return rval;
    }

    rval = htable_search(fam->param_table, "mesh dimensions", FIND_ENTRY, &p_hte);
    if ( p_hte == NULL )
    {
        return rval;
    }
    rval = read_scalar(fam, (Param_ref *)p_hte->data, &fam->dimensions);
    if ( rval != OK )
    {
        return rval;
    }

    /*
     * Traverse family directory to assess node and element def's.
     * Use a temporary hash table to store data by class.
     */

    fil_qty = fam->file_count;

    /* Init mesh name parameter for first mesh. */
    sprintf(pname, "mesh name %d", fam->qty_meshes);

    /*
     * Traverse the directories once to prepare for reading mesh
     * data from the file.
     */

    /* For each non-state file... */
    for ( i = 0; i < fil_qty; i++ )
    {
        /* For each directory entry in file... */
        ent_qty = fam->directory[i].qty_entries;
        p_de = fam->directory[i].dir_entries;
        names = fam->directory[i].names;
        name_cnt = 0;
        for ( j = 0; j < ent_qty; j++ )
        {
#if TIMER
            start1 = clock();
#endif
            etype = (Dir_entry_type)p_de[j][TYPE_IDX];
            mod1 = p_de[j][MODIFIER1_IDX];

            /*
             * Look for new mesh indicators (i.e., mesh names)
             * and mesh geometry definitions.  The name will
             * always be encountered first so the mesh will have
             * been created before any of it's geometry is
             * encountered.
             */
            switch ( etype )
            {
                case NODES:
#if TIMER
                    start2 = clock();
#endif
                    /* Seek to the nodal data entry in the family. */
                    rval = non_state_file_open(fam, i, fam->access_mode);
                    if ( rval != OK )
                    {
                        return rval;
                    }
                    rval = non_state_file_seek(fam, p_de[j][OFFSET_IDX]);
                    if ( rval != OK )
                    {
                        return rval;
                    }

                    /* Read the nodal data header (start and stop id's). */
                    read_ct = fam->read_funcs[M_INT](fam->cur_file, node_hdr, QTY_NODE_TAGS);
                    if ( read_ct != QTY_NODE_TAGS )
                    {
                        return SHORT_READ;
                    }

                    /*
                     * This call maintains consistency in internal structures
                     * with their state at original node geometry definition.
                     */
                    rval = add_geom(fam->meshes[mod1], names[name_cnt], node_hdr[0], node_hdr[1]);
                    if ( rval != OK )
                    {
                        return rval;
                    }
#if TIMER
                    stop2 = clock();
                    cumalative2 = (double)(1000 * (stop2 - start2)) / CLOCKS_PER_SEC;
                    printf("Read of init_mesh NODES timing is: %f ms\n", cumalative2);
#endif
                    break;

                case ELEM_CONNS:
#if TIMER
                    start2 = clock();
#endif
                    /* Seek to the connectivity data entry in the family. */
                    rval = non_state_file_open(fam, i, fam->access_mode);
                    if ( rval != OK )
                    {
                        return rval;
                    }
                    rval = non_state_file_seek(fam, p_de[j][OFFSET_IDX]);
                    if ( rval != OK )
                    {
                        return rval;
                    }

                    /* Read the connectivity data header. */
                    read_ct = fam->read_funcs[M_INT](fam->cur_file, conn_hdr, QTY_CONN_HEADER_FIELDS);
                    if ( read_ct != QTY_CONN_HEADER_FIELDS )
                    {
                        return SHORT_READ;
                    }

                    /* Read the first elem/last elem block pairs. */
                    blk_qty = conn_hdr[QTY_BLOCKS_IDX];
                    elem_blks = NEW_N(int, blk_qty * 2, "Conn elem blocks");
                    if ( blk_qty > 0 && elem_blks == NULL )
                    {
                        return ALLOC_FAILED;
                    }
                    read_ct = fam->read_funcs[M_INT](fam->cur_file, elem_blks, blk_qty * 2);
                    if ( read_ct != (blk_qty * 2) )
                    {
                        free(elem_blks);
                        return SHORT_READ;
                    }

#if TIMER
                    start1 = clock();
                    timed = 1;
#endif

                    /*
                     * This call maintains consistency in internal structures
                     * with their state at original connectivity definition.
                     */
                    for ( k = 0; k < blk_qty; k++ )
                    {
                        rval = add_geom(fam->meshes[mod1], names[name_cnt], *(elem_blks + k * 2),
                                        *(elem_blks + k * 2 + 1));
                        if ( rval != OK )
                        {
                            return rval;
                        }
                    }
#if TIMER
                    timed = 0;
                    stop1 = clock();
                    cumalative1 = (double)(1000 * (stop1 - start1)) / CLOCKS_PER_SEC;
                    printf("Read of non-state files() ELEM_CONNS add_geom() timing is: %f ms\n", cumalative1);
#endif
#if TIMER
                    start1 = clock();
#endif
                    free(elem_blks);
#if TIMER
                    stop1 = clock();
                    cumalative1 = (double)(1000 * (stop1 - start1)) / CLOCKS_PER_SEC;
                    printf("Read of non-state files() ELEM_CONNS free( elem_blks ) timing is: %f ms\n", cumalative1);
#endif
#if TIMER
                    stop2 = clock();
                    cumalative2 = (double)(1000 * (stop2 - start2)) / CLOCKS_PER_SEC;
                    printf("Read of init_mesh ELEM_CONNS timing is: %f ms\n", cumalative2);
#endif
                    break;

                case CLASS_IDENTS:
#if TIMER
                    start2 = clock();
#endif
                    /* Seek to the class identifier entry in the family. */
                    rval = non_state_file_open(fam, i, fam->access_mode);
                    if ( rval != OK )
                    {
                        return rval;
                    }
                    rval = non_state_file_seek(fam, p_de[j][OFFSET_IDX]);
                    if ( rval != OK )
                    {
                        return rval;
                    }

                    /* Read the class identifier entry. */
                    read_ct = fam->read_funcs[M_INT](fam->cur_file, ident_entry, QTY_IDENT_ENTRY_FIELDS);
                    if ( read_ct != QTY_IDENT_ENTRY_FIELDS )
                    {
                        return SHORT_READ;
                    }

                    rval = add_geom(fam->meshes[mod1], names[name_cnt], ident_entry[START_IDENT_IDX],
                                    ident_entry[STOP_IDENT_IDX]);
                    if ( rval != OK )
                    {
                        return rval;
                    }
#if TIMER
                    stop2 = clock();
                    cumalative2 = (double)(1000 * (stop2 - start2)) / CLOCKS_PER_SEC;
                    printf("Read of init_mesh CLASS_IDENTS timing is: %f ms\n", cumalative2);
#endif
                    break;

                case STATE_VAR_DICT:
                    break;

                case STATE_REC_DATA:
                    break;

                case MILI_PARAM:
#if TIMER
                    start2 = clock();
#endif
                    /*
                     * For this directory entry type, the Modifier1 directory
                     * entry holds the parameter type.
                     */
                    if ( mod1 != M_STRING )
                    {
                        break;
                    }
                    if ( strcmp(pname, names[name_cnt]) == 0 )
                    {
                        /* Initialize the mesh. */
                        rval = add_mesh(fam, p_de[j], pname);
                        if ( rval != OK )
                        {
                            return rval;
                        }

                        /* Get ready for another mesh. */
                        sprintf(pname, "mesh name %d", fam->qty_meshes);
                    }
#if TIMER
                    stop2 = clock();
                    cumalative2 = (double)(1000 * (stop2 - start2)) / CLOCKS_PER_SEC;
                    printf("Read of init_mesh MILI_PARAM timing is: %f ms\n", cumalative2);
#endif
                    break;

                case APPLICATION_PARAM:
                    break;

                case CLASS_DEF:
#if TIMER
                    start2 = clock();
#endif
                    /*
                     * This call maintains consistency in internal structures
                     * with their state at original class definition.
                     */
                    rval = create_class_data(p_de[j][MODIFIER2_IDX], names[name_cnt], names[name_cnt + 1], &p_mocd);
                    if ( rval != OK )
                    {
                        return rval;
                    }

                    /* Enter the class in the geometry hash table. */
                    p_ht = fam->meshes[mod1]->mesh_data.umesh_data;
                    rval = htable_add_entry_data(p_ht, names[name_cnt], ENTER_UNIQUE, p_mocd);
                    if ( rval != OK && rval != ENTRY_EXISTS )
                    {
                        mili_delete_mo_class_data(p_mocd);
                        return rval;
                    }
#if TIMER
                    stop2 = clock();
                    cumalative2 = (double)(1000 * (stop2 - start2)) / CLOCKS_PER_SEC;
                    printf("Read of init_mesh CLASS_DEF timing is: %f ms\n", cumalative2);
#endif
                    break;

                case SURFACE_CONNS:
#if TIMER
                    start2 = clock();
#endif
                    p_ht = fam->meshes[mod1]->mesh_data.umesh_data;
                    rval = htable_search(p_ht, names[name_cnt], FIND_ENTRY, &p_hte);
                    if ( p_hte == NULL )
                    {
                        return rval;
                    }
                    p_mocd = (Mesh_object_class_data *)p_hte->data;
                    if ( NULL == p_mocd->blocks->blocks )
                    {
                        surface_id = 1;
                    }
                    else
                    {
                        surface_id = p_mocd->blocks->blocks->stop + 1;
                    }

                    p_mocd->surface_sizes =
                        RENEW_N(int, p_mocd->surface_sizes, surface_id, 1, "No. of facets in surface");
                    if ( p_mocd->surface_sizes == NULL )
                    {
                        return ALLOC_FAILED;
                    }
                    p_mocd->surface_sizes[surface_id - 1] = p_de[j][MODIFIER2_IDX];
                    rval = add_geom(fam->meshes[mod1], names[name_cnt], surface_id, surface_id);
                    if ( rval != OK )
                    {
                        return rval;
                    }
#if TIMER
                    stop2 = clock();
                    cumalative2 = (double)(1000 * (stop2 - start2)) / CLOCKS_PER_SEC;
                    printf("Read of init_mesh SURFACE_CONNS timing is: %f ms\n", cumalative2);
#endif
                    break;
                case TI_PARAM:
                case QTY_DIR_ENTRY_TYPES:
                    break;
            }
#if TIMER
            stop1 = clock();
            cumalative1 = ((double)(stop1 - start1)) / CLOCKS_PER_SEC;
            /*printf("Read of non-state files() timing is: %f\n",cumalative1);*/
#endif
            name_cnt += p_de[j][STRING_QTY_IDX];
        }
    }

    /**/
#if TIMER
    stop = clock();
    cumalative = ((double)(stop - start)) / CLOCKS_PER_SEC;
    printf("Function init_meshes() Opening time is: %f\n", cumalative);
#endif
    return OK;
}

/*****************************************************************
 * TAG( add_mesh ) LOCAL
 *
 * Add a mesh to a family.
 */
static Return_value add_mesh(Mili_family *fam, Dir_entry dir_ent, char *name_handle)
{
    char *mname;
    int mesh_id;
    Htable_entry *p_hte;
    Return_value rval = OK;

    /* Read the mesh name from the family. */
    rval = htable_search(fam->param_table, name_handle, FIND_ENTRY, &p_hte);

    if ( p_hte != NULL )
    {
        rval = OK;

        mname = NEW_N(char, (LONGLONG)dir_ent[LENGTH_IDX], "Mesh name buf");
        if ( dir_ent[LENGTH_IDX] > 0 && mname == NULL )
        {
            rval = ALLOC_FAILED;
        }
        else
        {
            rval = mili_read_string(fam, (Param_ref *)p_hte->data, mname);

            if ( rval == OK )
            {
                /* Now create the mesh structures... */
                rval = make_umesh(fam, FALSE, mname, &mesh_id);
            }

            free(mname);
        }
    }

    return rval;
}

/*****************************************************************
 * TAG( dump_nodes ) PRIVATE
 *
 * Dump nodal entry to stdout.
 */
Return_value dump_nodes(Mili_family *fam, FILE *p_f, Dir_entry dir_ent, char **dir_strings, Dump_control *p_dc,
                        int head_indent, int body_indent)
{
    LONGLONG offset;
    int node_header[QTY_NODE_TAGS];
    int hi, bi;
    int status;
    int dims, qty;
    float *fbuf, *p_l, *p_r;
    int i, j;
    int lcnt, off;
    int num_written;
    Return_value rval;
    LONGLONG read_ct;

    offset = dir_ent[OFFSET_IDX];
    hi = head_indent + 1;
    bi = body_indent + 1;

    /* Seek and read the nodal data header. */
    status = fseek(p_f, (long)offset, SEEK_SET);
    if ( status != 0 )
    {
        return SEEK_FAILED;
    }

    read_ct = fam->read_funcs[M_INT](p_f, node_header, QTY_NODE_TAGS);
    if ( read_ct < QTY_NODE_TAGS )
    {
        return SHORT_READ;
    }

    /* Calculate the number of dimensions in the mesh. */
    qty = node_header[1] - node_header[0] + 1;
    dims = dir_ent[LENGTH_IDX] / (EXT_SIZE(fam, M_FLOAT) * qty);

    /* Dump listing header for nodal data. */
    rval = eprintf(&num_written, "%*tNODE DEFS:%*tClass: %s;%*t@%ld;%*tIdents: %d-%d\n", hi, hi + 13, dir_strings[0],
                   hi + 44, offset, hi + 56, node_header[0], node_header[1]);
    if ( rval != OK )
    {
        return rval;
    }

    /* Return if only header dump asked for. */
    if ( p_dc->brevity == DV_HEADERS )
    {
        return OK;
    }

    if ( p_dc->brevity == DV_SHORT )
    {
        /* For "short" output, only dump coords of first and last node. */

        /* Allocate a buffer and read in first node coords. */
        fbuf = NEW_N(float, dims, "Nodal coords dump bufr");
        if ( dims > 0 && fbuf == NULL )
        {
            return ALLOC_FAILED;
        }
        read_ct = fam->read_funcs[M_FLOAT](p_f, fbuf, dims);
        if ( read_ct < dims )
        {
            free(fbuf);
            return SHORT_READ;
        }

        /* Dump... */
        off = bi;
        for ( j = 0; j < dims; j++ )
        {
            rval = eprintf(&num_written, "%*t%+.3e", off, *(fbuf + j));
            if ( rval != OK )
            {
                free(fbuf);
                return rval;
            }
            off = 2;
        }

        /* Read and dump second node coords. */
        if ( qty > 1 )
        {
            status = fseek(p_f, (qty - 2) * dims * EXT_SIZE(fam, M_FLOAT), SEEK_CUR);
            if ( status != 0 )
            {
                free(fbuf);
                return SEEK_FAILED;
            }
            read_ct = fam->read_funcs[M_FLOAT](p_f, fbuf, dims);
            if ( read_ct < dims )
            {
                free(fbuf);
                return SHORT_READ;
            }
            printf(" ... ");
            off = 1;
            for ( j = 0; j < dims; j++ )
            {
                rval = eprintf(&num_written, "%*t%+.3e", off, *(fbuf + j));
                if ( rval != OK )
                {
                    free(fbuf);
                    return rval;
                }
                off = 2;
            }
        }

        putchar((int)'\n');

        free(fbuf);
    }
    else
    {
        /*
         * Dump all nodes' coords for long output in two columns in a row-
         * major fashion (first-to-middle nodes left column, middle-to-last
         * nodes right column).
         */
        fbuf = NEW_N(float, dims *qty, "Nodal coords dump bufr");
        if ( dims * qty > 0 && fbuf == NULL )
        {
            return ALLOC_FAILED;
        }
        read_ct = fam->read_funcs[M_FLOAT](p_f, fbuf, dims * qty);
        if ( read_ct < dims * qty )
        {
            free(fbuf);
            return SHORT_READ;
        }
        lcnt = (qty + (qty & 0x1)) / 2;
        p_l = fbuf;
        p_r = fbuf + lcnt * dims;
        for ( i = 0; i < lcnt; i++ )
        {
            off = bi;
            for ( j = 0; j < dims; j++ )
            {
                rval = eprintf(&num_written, "%*t%+.3e", off, *p_l++);
                if ( rval != OK )
                {
                    free(fbuf);
                    return rval;
                }
                off = 2;
            }

            if ( (i + 1) * 2 <= qty )
            {
                off = 6;
                for ( j = 0; j < dims; j++ )
                {
                    rval = eprintf(&num_written, "%*t%+.3e", off, *p_r++);
                    if ( rval != OK )
                    {
                        free(fbuf);
                        return rval;
                    }
                    off = 2;
                }
            }

            putchar((int)'\n');
        }

        free(fbuf);
    }

    return OK;
}

/*****************************************************************
 * TAG( dump_elem_conns ) PRIVATE
 *
 * Dump elem connectivity entry to stdout.
 */
Return_value dump_elem_conns(Mili_family *fam, FILE *p_f, Dir_entry dir_ent, char **dir_strings, Dump_control *p_dc,
                             int head_indent, int body_indent)
{
    LONGLONG offset;
    int elem_header[QTY_CONN_HEADER_FIELDS];
    int hi, bi;
    int status;
    int superclass, qty_blks, qty_wds, elem_qty;
    int i, j, k;
    int off;
    int *blks, *p_i, *p_elem;
    int *conn;
    int num_written;
    Return_value rval;
    LONGLONG read_ct;

    offset = dir_ent[OFFSET_IDX];
    hi = head_indent + 1;
    bi = body_indent + 1;
    off = 2;

    /* Seek and read the element connectivity data header. */
    status = fseek(p_f, (long)offset, SEEK_SET);
    if ( status != 0 )
    {
        return SEEK_FAILED;
    }

    read_ct = fam->read_funcs[M_INT](p_f, elem_header, QTY_CONN_HEADER_FIELDS);
    if ( read_ct < QTY_CONN_HEADER_FIELDS )
    {
        return SHORT_READ;
    }

    /* Unpack the header. */
    qty_blks = elem_header[QTY_BLOCKS_IDX];
    superclass = elem_header[CONN_SUPERCLASS_IDX];

    /* Dump listing header for connectivity data. */
    rval = eprintf(&num_written, "%*tELEM DEFS:%*tClass: %s (%s);%*t@%ld;%*tQty blocks: %d\n", hi, hi + 13,
                   dir_strings[0], superclass_names[superclass], hi + 44, offset, hi + 56, qty_blks);
    if ( rval != OK )
    {
        return rval;
    }

    /* Return if only header dump asked for. */
    if ( p_dc->brevity == DV_HEADERS )
    {
        return OK;
    }

    qty_wds = conn_words[superclass];
    if ( p_dc->brevity == DV_SHORT )
    {
        conn = NEW_N(int, qty_wds, "Conn bufr");
        if ( qty_wds > 0 && conn == NULL )
        {
            return ALLOC_FAILED;
        }
    }

    /* Read the element block ranges. */
    blks = NEW_N(int, qty_blks * 2, "Conn blocks");
    if ( qty_blks > 0 && blks == NULL )
    {
        if ( p_dc->brevity == DV_SHORT )
        {
            free(conn);
        }
        return ALLOC_FAILED;
    }
    read_ct = fam->read_funcs[M_INT](p_f, blks, qty_blks * 2);
    if ( read_ct < qty_blks * 2 )
    {
        if ( p_dc->brevity == DV_SHORT )
        {
            free(conn);
        }
        free(blks);
        return SHORT_READ;
    }

    for ( i = 0, p_i = blks; i < qty_blks; p_i += 2, i++ )
    {
        rval = eprintf(&num_written, "%*tBlock %d; Ident range: %d - %d\n", bi, i + 1, *p_i, *(p_i + 1));
        if ( rval != OK )
        {
            if ( p_dc->brevity == DV_SHORT )
            {
                free(conn);
            }
            free(blks);
            return rval;
        }

        elem_qty = *(p_i + 1) - *p_i + 1;

        if ( p_dc->brevity == DV_SHORT )
        {
            read_ct = fam->read_funcs[M_INT](p_f, conn, qty_wds);
            if ( read_ct < qty_wds )
            {
                free(conn);
                free(blks);
                return SHORT_READ;
            }

            rval =
                eprintf(&num_written, "%*tm%d p%d%*t%-5d", bi, conn[qty_wds - 2], conn[qty_wds - 1], bi + 10, conn[0]);
            if ( rval != OK )
            {
                free(conn);
                free(blks);
                return rval;
            }

            for ( j = 1; j < qty_wds - 2; j++ )
            {
                rval = eprintf(&num_written, "%*t%-5d", off, conn[j]);
                if ( rval != OK )
                {
                    free(conn);
                    free(blks);
                    return rval;
                }
            }

            putchar((int)'\n');

            if ( elem_qty > 1 )
            {
                status = fseek(p_f, (elem_qty - 2) * qty_wds * EXT_SIZE(fam, M_INT), SEEK_CUR);
                if ( status != 0 )
                {
                    free(conn);
                    free(blks);
                    return SEEK_FAILED;
                }

                read_ct = fam->read_funcs[M_INT](p_f, conn, qty_wds);
                if ( read_ct < qty_wds )
                {
                    free(conn);
                    free(blks);
                    return SHORT_READ;
                }

                rval = eprintf(&num_written, "%*tm%d p%d%*t%-5d", bi, conn[qty_wds - 2], conn[qty_wds - 1], bi + 10,
                               conn[0]);
                if ( rval != OK )
                {
                    free(conn);
                    free(blks);
                    return rval;
                }

                for ( j = 1; j < qty_wds - 2; j++ )
                {
                    rval = eprintf(&num_written, "%*t%-5d", off, conn[j]);
                    if ( rval != OK )
                    {
                        free(conn);
                        free(blks);
                        return rval;
                    }
                }

                putchar((int)'\n');
            }
        }
        else
        {
            conn = NEW_N(int, elem_qty *qty_wds, "Conns bufr");
            if ( elem_qty * qty_wds > 0 && conn == NULL )
            {
                free(blks);
                return ALLOC_FAILED;
            }
            read_ct = fam->read_funcs[M_INT](p_f, conn, elem_qty * qty_wds);
            if ( read_ct < elem_qty * qty_wds )
            {
                free(conn);
                free(blks);
                return SHORT_READ;
            }

            for ( k = 0, p_elem = conn; k < elem_qty; p_elem += qty_wds, k++ )
            {
                rval = eprintf(&num_written, "%*tm%d p%d%*t%-5d", bi, p_elem[qty_wds - 2], p_elem[qty_wds - 1], bi + 10,
                               p_elem[0]);
                if ( rval != OK )
                {
                    free(conn);
                    free(blks);
                    return rval;
                }

                for ( j = 1; j < qty_wds - 2; j++ )
                {
                    rval = eprintf(&num_written, "%*t%-5d", off, p_elem[j]);
                    if ( rval != OK )
                    {
                        free(conn);
                        free(blks);
                        return rval;
                    }
                }

                putchar((int)'\n');
            }

            free(conn);
        }
    }

    free(blks);
    if ( p_dc->brevity == DV_SHORT )
    {
        free(conn);
    }

    return OK;
}

/*****************************************************************
 * TAG( dump_surface_conns ) PRIVATE
 *
 * Dump surface connectivity entry to stdout.
 */
Return_value dump_surface_conns(Mili_family *fam, FILE *p_f, Dir_entry dir_ent, char **dir_strings, Dump_control *p_dc,
                                int head_indent, int body_indent)
{
    LONGLONG offset;
    int econn_header[QTY_CONN_HEADER_FIELDS];
    int hi, bi;
    int status;
    int superclass, qty_facets, qty_wds;
    int j, k;
    int off;
    int *p_elem;
    int *conn;
    int num_written;
    Return_value rval;
    LONGLONG read_ct;

    offset = dir_ent[OFFSET_IDX];
    hi = head_indent + 1;
    bi = body_indent + 1;
    off = 2;

    /* Seek and read the element connectivity data header. */
    status = fseek(p_f, (long)offset, SEEK_SET);
    if ( status != 0 )
    {
        return SEEK_FAILED;
    }

    read_ct = fam->read_funcs[M_INT](p_f, econn_header, QTY_CONN_HEADER_FIELDS);
    if ( read_ct < QTY_CONN_HEADER_FIELDS )
    {
        return SHORT_READ;
    }

    /* Unpack the header. */
    qty_facets = econn_header[QTY_BLOCKS_IDX];
    superclass = econn_header[CONN_SUPERCLASS_IDX];

    /* Dump listing header for connectivity data. */
    rval = eprintf(&num_written, "%*tSURF DEFS:%*tClass: %s (%s);%*t@%ld;%*tQty facets: %d\n", hi, hi + 13,
                   dir_strings[0], superclass_names[superclass], hi + 44, offset, hi + 56, qty_facets);
    if ( rval != OK )
    {
        return rval;
    }

    /* Return if only header dump asked for. */
    if ( p_dc->brevity == DV_HEADERS )
    {
        return OK;
    }

    qty_wds = conn_words[superclass];

    if ( p_dc->brevity == DV_SHORT )
    {
        conn = NEW_N(int, qty_wds, "Conns bufr");
        if ( qty_wds > 0 && conn == NULL )
        {
            return ALLOC_FAILED;
        }
        read_ct = fam->read_funcs[M_INT](p_f, conn, qty_wds);
        if ( read_ct < qty_wds )
        {
            free(conn);
            return SHORT_READ;
        }

        rval = eprintf(&num_written, "%*t%*t%-5d", bi, bi + 10, conn[0]);
        if ( rval != OK )
        {
            free(conn);
            return rval;
        }

        for ( j = 1; j < qty_wds; j++ )
        {
            rval = eprintf(&num_written, "%*t%-5d", off, conn[j]);
            if ( rval != OK )
            {
                free(conn);
                return rval;
            }
        }

        putchar((int)'\n');

        if ( qty_facets > 1 )
        {
            status = fseek(p_f, (qty_facets - 2) * qty_wds * EXT_SIZE(fam, M_INT), SEEK_CUR);
            if ( status != 0 )
            {
                return SEEK_FAILED;
            }
            read_ct = fam->read_funcs[M_INT](p_f, conn, qty_wds);
            if ( read_ct < qty_wds )
            {
                free(conn);
                return SHORT_READ;
            }

            rval = eprintf(&num_written, "%*t%*t%-5d", bi, bi + 10, conn[0]);
            if ( rval != OK )
            {
                free(conn);
                return rval;
            }

            for ( j = 1; j < qty_wds; j++ )
            {
                rval = eprintf(&num_written, "%*t%-5d", off, conn[j]);
                if ( rval != OK )
                {
                    free(conn);
                    return rval;
                }
            }

            putchar((int)'\n');
        }
        free(conn);
    }
    else
    {
        conn = NEW_N(int, qty_facets *qty_wds, "Conns bufr");
        if ( qty_facets * qty_wds > 0 && conn == NULL )
        {
            return ALLOC_FAILED;
        }
        read_ct = fam->read_funcs[M_INT](p_f, conn, qty_facets * qty_wds);
        if ( read_ct < qty_facets * qty_wds )
        {
            free(conn);
            return SHORT_READ;
        }

        for ( k = 0, p_elem = conn; k < qty_facets; p_elem += qty_wds, k++ )
        {
            rval = eprintf(&num_written, "%*t%*t%-5d", bi, bi + 10, p_elem[0]);
            if ( rval != OK )
            {
                free(conn);
                return rval;
            }

            for ( j = 1; j < qty_wds; j++ )
            {
                rval = eprintf(&num_written, "%*t%-5d", off, p_elem[j]);
                if ( rval != OK )
                {
                    free(conn);
                    return rval;
                }
            }

            putchar((int)'\n');
        }

        free(conn);
    }

    return OK;
}

/*****************************************************************
 * TAG( dump_class_idents ) PRIVATE
 *
 * Dump a class identifier entry.
 */
Return_value dump_class_idents(Mili_family *fam, FILE *p_f, Dir_entry dir_ent, char **dir_strings, Dump_control *p_dc,
                               int head_indent, int body_indent)
{
    LONGLONG offset;
    int ident_entry[QTY_IDENT_ENTRY_FIELDS];
    int hi;
    int superclass;
    int status;
    int start, stop;
    int num_written;
    Return_value rval;
    LONGLONG read_ct;

    offset = dir_ent[OFFSET_IDX];
    hi = head_indent + 1;

    /* Seek and read the class ident entry. */
    status = fseek(p_f, (long)offset, SEEK_SET);
    if ( status != 0 )
    {
        return SEEK_FAILED;
    }

    read_ct = fam->read_funcs[M_INT](p_f, ident_entry, QTY_IDENT_ENTRY_FIELDS);
    if ( read_ct < QTY_IDENT_ENTRY_FIELDS )
    {
        return SHORT_READ;
    }

    /* Unpack the entry. */
    superclass = ident_entry[IDENT_SUPERCLASS_IDX];
    start = ident_entry[START_IDENT_IDX];
    stop = ident_entry[STOP_IDENT_IDX];

    /* Dump entry. */
    rval = eprintf(&num_written, "%*tIDENTS:%*tClass: %s (%s);%*t@%ld;%*tIdents: %d-%d\n", hi, hi + 13, dir_strings[0],
                   superclass_names[superclass], hi + 44, offset, hi + 56, start, stop);

    return rval;
}

/************************************************************
 * TAG( mc_compare_labels ) LOCAL
 *
 * Used by the qsort routine to sort Mili Labels.
 */
static int SDTLIBCC mc_compare_labels(const void *label1, const void *label2)
{
    int *label1_int = (int *)label1;
    int *label2_int = (int *)label2;

    if ( *label1_int < *label2_int )
    {
        return -1;
    }

    if ( *label1_int > *label2_int )
    {
        return 1;
    }

    /*if ( *label1_int == *label2_int )*/
    return 0;
}

/*****************************************************************
 * TAG( mc_get_conn_count ) PUBLIC
 *
 * Read element connectivity family entry into memory.
 */
int mc_get_conn_count(int superclass)
{
    if ( superclass < 0 || superclass >= M_QTY_SUPERCLASS )
    {
        return (0);
    }
    return (class_conns[superclass]);
}

/*****************************************************************
 * TAG( mc_get_unique_node_count ) PUBLIC
 *
 * Given a list of nodes with duplicates, count the number of unique nodes
 * in the list and return the count.
 */
Return_value mc_get_unique_node_count(int *nodes, int qty, int *count)
{
    int i;
    int max_node = -1;
    int *temp_buff;
    Return_value rval = OK;

    *count = 0;
    /* Determine the highest node number - these may be global node numbers and
     * not in 1-n order!.
     */

    for ( i = 0; i < qty; i++ )
    {
        if ( nodes[i] > max_node )
        {
            max_node = nodes[i] + 1;
        }
    }

    if ( max_node <= 0 )
    {
        return NOT_OK;
    }

    temp_buff = NEW_N(int, max_node, "Unique Node List");
    if ( max_node > 0 && temp_buff == NULL )
    {
        return ALLOC_FAILED;
    }

    for ( i = 0; i < max_node; i++ )
    {
        if ( nodes[i] >= max_node )
        {
            free(temp_buff);
            return NOT_OK; /* Node out of range. */
        }
        temp_buff[nodes[i]]++;
    }

    /* Now total up the number of nodes - add to count of temp_buff[i]>0 */
    for ( i = 0; i < max_node; i++ )
    {
        if ( temp_buff[i] > 0 )
        {
            (*count)++;
        }
    }

    free(temp_buff);
    return rval;
}

/*****************************************************************
 * TAG( mili_delete_mo_class_data ) PRIVATE
 *
 * Delete mili_mo_class_data struct and referenced memory.
 * Designed as a hash table data deletion function for htable_delete().
 */
void mili_delete_mo_class_data(void *p_data)
{
    Mili_mo_class_data *p_mocd;
    Mili_elem_data *p_ed;
    Mili_material_data *p_mtld;
    float *p_nd;
    int sclass;

    p_mocd = (Mili_mo_class_data *)p_data;
    if ( p_mocd == NULL )
    {
        return;
    }

    sclass = p_mocd->superclass;
    switch ( sclass )
    {
        case M_NODE:
            p_nd = p_mocd->objects.nodes;
            if ( p_nd != NULL )
            {
                free(p_nd);
            }
            break;

        case M_UNIT:
            /* do nothing */
            break;

        case M_MAT:
            p_mtld = p_mocd->objects.materials;
            if ( p_mtld != NULL )
            {
                free(p_mtld);
            }
            break;

        case M_TRUSS:
        case M_BEAM:
        case M_TRI:
        case M_QUAD:
        case M_TET:
        case M_PYRAMID:
        case M_WEDGE:
        case M_HEX:
        case M_PARTICLE:
            p_ed = p_mocd->objects.elems;
            if ( p_ed != NULL )
            {
                if ( p_ed->nodes != NULL )
                {
                    free(p_ed->nodes);
                }
                if ( p_ed->mat != NULL )
                {
                    free(p_ed->mat);
                }
                if ( p_ed->part != NULL )
                {
                    free(p_ed->part);
                }
                free(p_ed);
            }
            break;
    }

    free(p_mocd->short_name);
    free(p_mocd->long_name);

    free(p_mocd);
}

/*****************************************************************
 * TAG( mc_blocks_to_list )
 *
 * Expand identifiers implied by an array of first/last blocks
 * into explicit identifiers.
 */
void mc_blocks_to_list(int qty_blocks, int *mo_blocks, int *list_count, int *list, int decrement_indices)
{
    int i, j;
    int increment, ident;
    int *block, *list_entry;
    int start, stop;

    list_entry = list;
    *list_count = 0;

    for ( i = 0, block = mo_blocks; i < qty_blocks; i++, block += 2 )
    {
        if ( block[0] < block[1] )
        {
            increment = 1;
            start = block[0];
            stop = block[1];
        }
        else
        {
            increment = -1;
            start = block[1];
            stop = block[0];
        }

        ident = block[0];
        if ( decrement_indices )
            ident--;

        for ( j = start; j <= stop; j++, ident += increment )
        {
            *list_entry++ = ident;
            (*list_count)++;
        }
    }
}
