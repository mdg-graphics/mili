
/* $Id: result_data.c,v 1.7.2.2 2013/10/25 16:49:03 oliver4 Exp $ */
/*
 * result_data.c - Routines for managing result data arrays.
 *
 *      Douglas E. Speck
 *      Lawrence Livermore National Laboratory
 *      Aug 1997
 *
 *
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - Sept 15, 2004: Added new option "damage_hide" which
 *                controls if damaged elements are displayed.
 *
 *  I. R. Corey - Jan 12, 2006: Added test for no free nodes.
 *
 *  I. R. Corey - Feb 02, 2006: Add a new capability to display meshless,
 *                particle-based results.
 *                See SRC#367.
 *
 *  I. R. Corey - Oct 26, 2006: Added new feature for enable/disable vis/invis
 *                that allow selection of an object type and material for that
 *                object.
 *                See SRC#421.
 *
 *  I. R. Corey - Oct 05, 2007:	Added new error indicator result option.
 *                See SRC#483.
 *
 *  I. R. Corey - Nov 05, 2007:	Added Node/Element labeling.
 *                See SRC#483.
 *
 *  I. R. Corey - Feb 19, 2008:	Fixed a problem with multiple counting of
 *                nodal result on material boundary. This affects all
 *                class types (hex_to_nodal, quad_to_nodal, etc.)
 *                See SRC#517.
 *
 *  I. R. Corey - June 16, 2009: Add exclusion capability for nodes.
 *                See SRC#539
 *
 *  09/29/2009 I. R. Corey  Fixed a problem with computing strains for
 *                          shared result with multiple Hex element sets.
 *                          See SRC#: 629
 *
 *  11/11/2009 I. R. Corey  update_nodal_min_max() gives incorrect max value
 *                          of 0 when values are all < 0.
 *
 *  02/11/2013 I. R. Corey  Fixed problem with calculating interp results
 *                   across multiple element sets or multiple sub-records.
 *                   See TeamForge#19431
 *
 *  02/25/2013 I. R. Corey Added class_name + hexid to zero volume warning.
 *
 *  04/07/2013 I. R. Fixed problem with function dump_result. Need to check
 *                   if the requested result is valid for the class.
 *                   See TeamForge#19800
 *
 *************************************************************************
 */

#include <stdlib.h>
#include "viewer.h"
#include "mdg.h"

#ifdef AIX
#ifndef MAXFLOAT
#define MAXFLOAT ((float)3.40282347e+38)
#endif
#endif

void update_min_max( Analysis * );
void update_nodal_min_max( Analysis * );
void init_nodal_min_max( Analysis * );
void find_min_max( int num_nodes, float *result, float *min, float *max,
                   int *min_index, int *max_index );

void dump_result( Analysis *analy, char * );

/*****************************************************************
 * TAG( derive_order )
 *
 * Set the order for evaluation and precedence for which mesh object
 * type has priority to determine the final value that goes into the
 * nodal result array.  Note that (1) we only include types that Griz
 * is likely to handle, (2) the priority setting will only impact
 * results which are supported on multiple superclasses, and (3) the
 * order of evaluation of mesh object _classes_ within a superclass is
 * not determined this information.
 */
static int derive_order[] =
{
    G_NODE, G_TRUSS, G_BEAM, G_TET, G_HEX, G_TRI, G_QUAD, G_SURFACE,
    G_PARTICLE, G_MAT, G_MESH, G_UNIT
};


/*****************************************************************
 * TAG( zero_vol_warn_once )
 *
 * Controls whether a warning about zero element volume is given.
 * Some meshes will incur this warning over many elements and the
 * warning becomes very redundant and time-consuming.
 */
static Bool_type zero_vol_warn_once;


/*****************************************************************
 * TAG( reset_zero_vol_warning )
 *
 * Enables the zero-volume-element warning.
 */
extern void
reset_zero_vol_warning()
{
    zero_vol_warn_once = TRUE;
}


/*****************************************************************
 * TAG( update_min_max )
 *
 * Update the state and global min/max for the current result
 * data.
 */
void
update_min_max( Analysis *analy )
{
    float *state_mm, *el_state_mm;
    int *el_state_id;
    char **el_state_class;
    int *el_state_sclass;
    int i;
    int *mm_nodes;
    char **mm_class;
    int *mm_sclass;

    state_mm = analy->state_mm;
    el_state_mm    = analy->elem_state_mm.object_minmax;
    el_state_id    = analy->elem_state_mm.object_id;
    el_state_class = analy->elem_state_mm.class_long_name;
    el_state_sclass = analy->elem_state_mm.sclass;

    mm_nodes      = analy->state_mm_nodes;
    mm_class      = analy->state_mm_class_long_name;
    mm_sclass     = analy->state_mm_sclass;

#ifdef OLD_UPDATE_MM
    /* Get rid of these local var's on cleanup. */
    result = analy->result;
    cnt = MESH( analy ).node_geom->qty;

    /* Update the state min/max for nodal/interpolated data. */
    state_mm[0] = result[0];
    mm_nodes[0] = 1;
    state_mm[1] = result[0];
    mm_nodes[1] = 1;

    for ( i = 1; i < cnt; i++ )
    {
        if ( result[i] < state_mm[0] )
        {
            state_mm[0] = result[i];
            mm_nodes[0] = i + 1;
            mm_class_long_name[0] = class_name;
            mm_class_long_name[1] = class_name;
            mm_sclass[0] = sclass;
            mm_sclass[1] = sclass;
        }
        else if ( result[i] > state_mm[1] )
        {
            state_mm[1] = result[i];
            mm_nodes[1] = i + 1;
        }
    }

    /*
     * For nodal results, save the class name and verify that the result
     * references only one class.  This should probably always be the case,
     * but if/when not, some re-design will be necessary to update the
     * min/max after each subrecord is processed and save the correct
     * class names.
     */
    p_r = analy->cur_result;
    if ( p_r->origin.is_node_result && analy->result_mod )
    {
        p_so = analy->srec_tree[p_r->srec_id].subrecs;
        indices = p_r->subrecs;
        cname = p_so[indices[0]].p_object_class->class_name;

        for ( i = 1; i < p_r->qty; i++ )
            if ( strcmp( cname, p_so[indices[i]].p_object_class->class_name )
                    != 0 )
            {
                popup_dialog( WARNING_POPUP, "%s\n%s\n%s",
                              "Result accessed multiple M_NODE classes.",
                              "Result min/max class name may be wrong.",
                              "Please contact the Methods Development Group." );
                break;
            }

        analy->state_mm_class[0] = analy->state_mm_class[1] = cname;
    }
#endif

    /* Update the state min/max for element/non-interpolated data. */
    analy->elem_state_mm = analy->tmp_elem_mm;

    /* Update the global min/max. */
    if ( analy->result_mod )
    {
        /* For a new result, always init the global mm from the state mm. */
        analy->global_mm[0] = state_mm[0];
        analy->global_mm_nodes[0] = mm_nodes[0];
        analy->global_mm_class_long_name[0] = mm_class[0];
        analy->global_mm_sclass[0] = mm_sclass[0];

        analy->global_mm[1] = state_mm[1];
        analy->global_mm_nodes[1] = mm_nodes[1];
        analy->global_mm_class_long_name[1] = mm_class[1];
        analy->global_mm_sclass[1] = mm_sclass[1];

        analy->elem_global_mm.object_minmax[0] = el_state_mm[0];
        analy->elem_global_mm.object_minmax[1] = el_state_mm[1];

        analy->elem_global_mm.object_id[0] = el_state_id[0];
        analy->elem_global_mm.object_id[1] = el_state_id[1];

        analy->elem_global_mm.class_long_name[0] = el_state_class[0];
        analy->elem_global_mm.class_long_name[1] = el_state_class[1];

        analy->elem_global_mm.sclass[0] = el_state_sclass[0];
        analy->elem_global_mm.sclass[1] = el_state_sclass[1];
    }
    else
    {
        /* Same result - only update global if state has new extreme(s). */
        if ( state_mm[0] < analy->global_mm[0] )
        {
            analy->global_mm[0] = state_mm[0];
            analy->global_mm_nodes[0] = mm_nodes[0];
            analy->global_mm_class_long_name[0] = mm_class[0];
            analy->global_mm_sclass[0] = mm_sclass[0];
        }
        if ( state_mm[1] > analy->global_mm[1] )
        {
            analy->global_mm[1] = state_mm[1];
            analy->global_mm_nodes[1] = mm_nodes[1];
            analy->global_mm_class_long_name[1] = mm_class[1];
            analy->global_mm_sclass[1] = mm_sclass[1];
        }

        /* Update the global minmax for element (pre-interpolated) results. */
        if ( el_state_mm[0] < analy->elem_global_mm.object_minmax[0] )
        {
            analy->elem_global_mm.object_minmax[0]  = el_state_mm[0];
            analy->elem_global_mm.object_id[0]      = el_state_id[0];
            analy->elem_global_mm.class_long_name[0] = el_state_class[0];
            analy->elem_global_mm.sclass[0]         = el_state_sclass[0];
        }
        if ( el_state_mm[1] > analy->elem_global_mm.object_minmax[1] )
        {
            analy->elem_global_mm.object_minmax[1]  = el_state_mm[1];
            analy->elem_global_mm.object_id[1]      = el_state_id[1];
            analy->elem_global_mm.class_long_name[1] = el_state_class[1];
            analy->elem_global_mm.sclass[1]         = el_state_sclass[1];
        }
    }

    /* Update the current min/max. */
    for ( i = 0; i < 2; i++ )
        if ( !analy->mm_result_set[i] )
        {
            if ( analy->use_global_mm )
                analy->result_mm[i] = analy->global_mm[i];
            else
                analy->result_mm[i] = analy->state_mm[i];
        }
}


/*****************************************************************
 * TAG( init_nodal_min_max )
 *
 * Initialize state min/max data cache's for nodal/interpolated data.
 */
void
init_nodal_min_max( Analysis *analy )
{
    float *result;
    Result *p_r;
    Subrec_obj *p_so;
    char *class_name;
    int ident=0, superclass, cur_idx;
    Bool_type indirect;
    MO_class_data *p_mo_class;

    result = NODAL_RESULT_BUFFER( analy );

    p_r = analy->cur_result;
    cur_idx = analy->result_index;
    indirect = p_r->indirect_flags[cur_idx];
    p_so = analy->srec_tree[p_r->srec_id].subrecs
           + p_r->subrecs[analy->result_index];
    /**/
    /* superclass = p_so->p_object_class->superclass; */

    superclass = p_r->superclasses[cur_idx];

    /*
     * Sanity check for object classes that don't ultimately generate nodal
     * data.
     */
    if ( superclass!=G_SURFACE && (superclass < G_NODE
                                   || superclass > G_PARTICLE) )
        return;

    if ( indirect )
    {
        /* Init using the first class of the appropriate superclass. */
        p_mo_class = ((MO_class_data **)
                      MESH_P( analy )->classes_by_sclass[superclass].list)[0];
        class_name = p_mo_class->long_name;

        /* Get a valid node from the first element's connectivities. */
        ident = p_mo_class->objects.elems->nodes[0];
    }
    else if ( superclass == G_NODE )
    {
        /* Direct nodal data. */

        class_name = p_so->p_object_class->long_name;
        superclass = p_so->p_object_class->superclass;

        if ( p_so->object_ids == NULL )
            ident = 0;
        else
            ident = p_so->object_ids[0];
    }
    else
    {
        /* Direct element data. */

        class_name = p_so->p_object_class->long_name;
        superclass = p_so->p_object_class->superclass;

        /* Init from the first node referenced in element connectivities. */
        if ( p_so->referenced_nodes!= NULL )
            ident = p_so->referenced_nodes[0];
        else ident = 0;
    }

    /* Initialize the state min/max for nodal/interpolated data. */
    analy->state_mm_class_long_name[0]  = class_name;
    analy->state_mm_class_long_name[1]  = class_name;

    analy->state_mm_sclass[0] = superclass;
    analy->state_mm_sclass[1] = superclass;

    /* analy->state_mm[0] = result[ident];
       analy->state_mm[1] = result[ident]; */

    analy->state_mm[0] = MAXFLOAT;
    analy->state_mm[1] = -MAXFLOAT;

    analy->state_mm_nodes[0] = ident + 1;
    analy->state_mm_nodes[1] = ident + 1;
}


/*****************************************************************
 * TAG( update_nodal_min_max )
 *
 * Update the state min/max for the current subrecord result
 * data as existing at or interpolated to the nodes.
 */
void
update_nodal_min_max( Analysis *analy )
{
    float *state_mm, *result;
    float val;
    int i, ident, ref_node_qty, cur_idx, class_qty;
    int *mm_nodes, *ref_nodes;
    Result *p_r;
    Subrec_obj *p_so;
    char **mm_class;
    int *mm_sclass;
    char *class_name;
    int superclass;
    Bool_type indirect;
    MO_class_data **mo_classes;
    MO_class_data *p_mo_class;
    Mesh_data *p_mesh;

    p_mesh = MESH_P( analy );
    p_r    = analy->cur_result;

    int exclude_cnt=0;

    cur_idx = analy->result_index;
    superclass = p_r->superclasses[cur_idx];

    /*
     * Sanity check for object classes that don't ultimately generate nodal
     * data.
     */
    if ( superclass!=G_SURFACE && (superclass < G_NODE
                                   || superclass > G_PARTICLE ) )
        return;

    indirect = p_r->indirect_flags[cur_idx];
    p_so = analy->srec_tree[p_r->srec_id].subrecs + p_r->subrecs[cur_idx];
    state_mm = analy->state_mm;
    mm_nodes = analy->state_mm_nodes;
    mm_class = analy->state_mm_class_long_name;
    mm_sclass = analy->state_mm_sclass;

    result = NODAL_RESULT_BUFFER( analy );

    float min, max;
    int min_index, max_index;
    find_min_max( p_mesh->node_geom->qty, result, &min, &max,
                  &min_index, &max_index );

    class_qty = 1;

    if ( indirect )
    {
        /* Prepare for traversing first class of appropriate superclass. */
        class_qty = MESH_P( analy )->classes_by_sclass[superclass].qty;
        mo_classes = (MO_class_data **)
                     MESH_P( analy )->classes_by_sclass[superclass].list;
        p_mo_class = mo_classes[0];
        class_name = mo_classes[0]->long_name;

        if ( p_mo_class->referenced_nodes == NULL )
        {
            /* Create a node list for the class. */
            create_elem_class_node_list( analy, p_mo_class,
                                         &p_mo_class->referenced_node_qty,
                                         &p_mo_class->referenced_nodes );
        }

        ref_nodes    = p_mo_class->referenced_nodes;
        ref_node_qty = p_mo_class->referenced_node_qty;
        superclass   = p_mo_class->superclass;
    }
    else if ( superclass == G_NODE )
    {
        class_name = p_so->p_object_class->long_name;
        superclass = p_so->p_object_class->superclass;

        if ( p_so->object_ids == NULL )
        {
            /* Traverse proper nodal subrecords directly. */

            ref_node_qty = MESH_P( analy )->node_geom->qty;

            for ( i = 1; i < ref_node_qty; i++ )
            {
                val = result[i];

                /* Skip excluded nodes */
                if ( p_mesh->disable_nodes )
                    if ( p_mesh->disable_nodes[i] )
                        continue;

                if ( val < state_mm[0] )
                {
                    state_mm[0]  = val;
                    mm_nodes[0]  = i + 1;
                    mm_class[0]  = class_name;
                    mm_sclass[0] = superclass;
                }
                if ( val > state_mm[1] )
                {
                    state_mm[1]  = val;
                    mm_nodes[1]  = i + 1;
                    mm_class[1]  = class_name;
                    mm_sclass[1] = superclass;
                }
            }

            return;
        }
        else
        {
            /*
             * Prepare for traversal with ident indirection for non-proper
             * nodal subrecord.
             */
            ref_nodes    = p_so->object_ids;
            ref_node_qty = p_so->subrec.qty_objects;
        }
    }
    else
    {
        /*
         * Traverse nodal data via referenced nodes list for element
         * subrecords.
         */
        class_name   = p_so->p_object_class->long_name;
        superclass   = p_so->p_object_class->superclass;
        ref_nodes    = p_so->referenced_nodes;
        ref_node_qty = p_so->ref_node_qty;
    }

    /* Traverse through appropriate nodal values to update min/max. */
    for ( i = 1; i < ref_node_qty; i++ )
    {
        ident = ref_nodes[i];
        val   = result[ident];

        /* Skip excluded nodes */
        if ( p_mesh->disable_nodes && !is_particle_class( analy, p_so->p_object_class->superclass, p_so->p_object_class->short_name ) )
            if ( p_mesh->disable_nodes[ident] || p_mesh->disable_nodes[ident]<0 )
            {
                exclude_cnt++;
                continue;
            }
        if ( p_mesh->disable_nodes )
            if ( p_mesh->disable_nodes[ident] || p_mesh->disable_nodes[ident]<0 )
            {
                exclude_cnt++;
                continue;
            }

        if ( val < state_mm[0] )
        {
            state_mm[0] = val;
            mm_nodes[0] = ident + 1;
            mm_class[0] = class_name;
            analy->state_mm_sclass[0] = superclass;
        }
        else if ( val > state_mm[1] )
        {
            state_mm[1] = val;
            mm_nodes[1] = ident + 1;
            mm_class[1] = class_name;
            analy->state_mm_sclass[1] = superclass;
        }
    }

    if ( indirect && class_qty > 1 )
    {
        /*
         * Multiple classes support this indirect result - must
         * traverse the rest to ascertain true min/max.
         */
        for ( i = 1; i < class_qty; i++ )
        {
            p_mo_class = mo_classes[i];
            class_name = p_mo_class->long_name;
            superclass = p_mo_class->superclass;

            if ( p_mo_class->referenced_nodes == NULL )
            {
                /* Create a node list for the class. */
                create_elem_class_node_list( analy, p_mo_class,
                                             &p_mo_class->referenced_node_qty,
                                             &p_mo_class->referenced_nodes );
            }

            ref_nodes = p_mo_class->referenced_nodes;
            ref_node_qty = p_mo_class->referenced_node_qty;

            for ( i = 1; i < ref_node_qty; i++ )
            {
                ident = ref_nodes[i];
                val   = result[ident];

                /* Skip excluded nodes */
                if ( p_mesh->disable_nodes && !is_particle_class( analy, p_so->p_object_class->superclass, p_so->p_object_class->short_name ) )
                    if ( p_mesh->disable_nodes[ident] || p_mesh->disable_nodes[ident]<1 )
                        continue;

                if ( val < state_mm[0] )
                {
                    state_mm[0]  = val;
                    mm_nodes[0]  = ident + 1;
                    mm_class[0]  = class_name;
                    mm_sclass[0] = superclass;
                }
                else if ( val > state_mm[1] )
                {
                    state_mm[1]  = val;
                    mm_nodes[1]  = ident + 1;
                    mm_class[1]  = class_name;
                    mm_sclass[1] = superclass;
                }
            }
        }
    }
}


/*****************************************************************
 * TAG( elem_get_minmax )
 *
 * Traverses element class result values to extract the min and max.
 *
 * Data in val_elem is assumed to be indexed by element id.
 */
void
elem_get_minmax( float *val_elem, int qty, Analysis *analy )
{
    Mesh_data *p_mesh;
    MO_class_data *p_elem_class;
    int i;
    int *elem_ids;
    char **classes;
    int *sclasses, sclass;
    char *class_long_name;
    Result *p_result;
    int idx, subrec_idx;
    Subrec_obj *p_so;
    int mesh, srec;
    int elem_qty;
    int elem_id;
    float val;
    float *mm_val, *activity;
    int *mats;

    Bool_type disable_obj = FALSE;

    p_result = analy->cur_result;
    srec = p_result->srec_id;
    idx = analy->result_index;
    subrec_idx = p_result->subrecs[idx];
    p_so = analy->srec_tree[srec].subrecs + subrec_idx;
    p_elem_class = p_so->p_object_class;
    class_long_name = p_elem_class->long_name;
    sclass = p_elem_class->superclass;
    elem_qty = p_so->subrec.qty_objects;
    analy->db_query( analy->db_ident, QRY_SREC_MESH, (void *) &srec, NULL,
                     (void *) &mesh );
    p_mesh = analy->mesh_table + mesh;
    mats = p_elem_class->objects.elems->mat;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_elem_class->elem_class_index]
               : NULL;

    if ( qty != 0 )
        elem_qty = qty;

    /* Prepare to extract elem min/max (values init'd in load_result()). */
    mm_val = analy->tmp_elem_mm.object_minmax;
    classes = analy->tmp_elem_mm.class_long_name;
    sclasses = analy->tmp_elem_mm.sclass;
    elem_ids = analy->tmp_elem_mm.object_id;


    /* Loop to extract state elem min/max. */
    for ( i = 0; i < elem_qty; i++ )
    {
        /* Get the elem identifier. */
        if ( qty == 0 )
            elem_id = p_so->object_ids ? p_so->object_ids[i] : i;
        else
            elem_id = i;

        /* Ignore inactive elements. */
        if ( !analy->free_nodes_enabled && activity && activity[elem_id] == 0.0 )
            continue;

        /* Ignore disabled materials. */
        if ( p_mesh->disable_material[ mats[elem_id] ] )
            continue;

        /* Ignore disabled objects */
        /* if ( disable_by_object_type( p_elem_class, mats[elem_id], i, analy, NULL ) )
        continue; */

        val = val_elem[elem_id];

        /* Test for new min and max. */
        if ( val < mm_val[0] )
        {
            mm_val[0] = val;
            elem_ids[0] = elem_id + 1;
            classes[0] = class_long_name;
            sclasses[0] = sclass;
        }

        if ( val > mm_val[1] )
        {
            mm_val[1] = val;
            elem_ids[1] = elem_id + 1;
            classes[1] = class_long_name;
            sclasses[1] = sclass;
        }
    }
}


/*****************************************************************
 * TAG( unit_get_minmax )
 *
 * Traverses unit class result values to extract the min and max.
 *
 * This routine could be generalized to work for similar non-element
 * mesh object classes with the introduction of a generic approach
 * to deal with material disability (which may or may not be
 * pertinent to non-element, non-nodal classes).
 *
 * Data in val_unit is assumed to be indexed by unit id.
 */
void
unit_get_minmax( float *val_unit, Analysis *analy )
{
    int i;
    int *unit_ids;
    char **classes;
    int *sclasses, sclass;
    char *class_long_name;
    Result *p_result;
    int idx, subrec_idx;
    Subrec_obj *p_so;
    int mesh, srec;
    int unit_qty;
    int unit_id;
    float val;
    float *mm_val;

    p_result = analy->cur_result;
    srec = p_result->srec_id;
    idx = analy->result_index;
    subrec_idx = p_result->subrecs[idx];
    p_so = analy->srec_tree[srec].subrecs + subrec_idx;
    class_long_name = p_so->p_object_class->long_name;
    sclass = p_so->p_object_class->superclass;
    unit_qty = p_so->subrec.qty_objects;
    analy->db_query( analy->db_ident, QRY_SREC_MESH, (void *) &srec, NULL,
                     (void *) &mesh );

    /* Prepare to extract unit min/max (values init'd in load_result()). */
    mm_val   = analy->tmp_elem_mm.object_minmax;
    classes  = analy->tmp_elem_mm.class_long_name;
    sclasses = analy->tmp_elem_mm.sclass;
    unit_ids = analy->tmp_elem_mm.object_id;

    /* Loop to extract state unit min/max. */
    for ( i = 0; i < unit_qty; i++ )
    {
        /* Get the unit identifier. */
        unit_id = p_so->object_ids ? p_so->object_ids[i] : i;

        val = val_unit[unit_id];

        /* Test for new min and max. */
        if ( val < mm_val[0] )
        {
            mm_val[0]   = val;
            unit_ids[0] = unit_id + 1;
            classes[0]  = class_long_name;
            sclasses[0] = sclass;
        }

        if ( val > mm_val[1] )
        {
            mm_val[1]   = val;
            unit_ids[1] = unit_id + 1;
            classes[1]  = class_long_name;
            sclasses[1] = sclass;
        }
    }
}


/*****************************************************************
 * TAG( mat_get_minmax )
 *
 * Traverses material result values to extract the min and max.
 *
 * This routine could be generalized to work for similar non-element
 * mesh object classes with the introduction of a generic approach
 * to deal with material disability (which may or may not be
 * pertinent to non-element, non-nodal classes).
 *
 * Data in val_mat is assumed to be indexed by material id.
 */
void
mat_get_minmax( float *val_mat, Analysis *analy )
{
    Mesh_data *p_mesh;
    int i;
    int *mat_ids;
    char **classes;
    int *sclasses, sclass;
    char *class_long_name;
    Result *p_result;
    int idx, subrec_idx;
    Subrec_obj *p_so;
    int mesh, srec;
    int mat_qty;
    int mat_id;
    float val;
    float *mm_val;

    p_result = analy->cur_result;
    srec = p_result->srec_id;
    idx = analy->result_index;
    subrec_idx = p_result->subrecs[idx];
    p_so = analy->srec_tree[srec].subrecs + subrec_idx;
    class_long_name = p_so->p_object_class->long_name;
    sclass = p_so->p_object_class->superclass;
    mat_qty = p_so->subrec.qty_objects;
    analy->db_query( analy->db_ident, QRY_SREC_MESH, (void *) &srec, NULL,
                     (void *) &mesh );
    p_mesh = analy->mesh_table + mesh;

    /* Prepare to extract material min/max (values init'd in load_result()). */
    mm_val = analy->tmp_elem_mm.object_minmax;
    classes = analy->tmp_elem_mm.class_long_name;
    sclasses = analy->tmp_elem_mm.sclass;
    mat_ids = analy->tmp_elem_mm.object_id;


    /* Loop to extract state material min/max. */
    for ( i = 0; i < mat_qty; i++ )
    {
        /* Get the material identifier. */
        mat_id = p_so->object_ids ? p_so->object_ids[i] : i;

        /* Ignore disabled materials. */
        if ( p_mesh->disable_material[mat_id] )
            continue;

        val = val_mat[mat_id];

        /* Test for new min and max. */
        if ( val < mm_val[0] )
        {
            mm_val[0]   = val;
            mat_ids[0]  = mat_id + 1;
            classes[0]  = class_long_name;
            sclasses[0] = sclass;
        }

        if ( val > mm_val[1] )
        {
            mm_val[1]   = val;
            mat_ids[1]  = mat_id + 1;
            classes[1]  = class_long_name;
            sclasses[1] = sclass;
        }
    }
}


/*****************************************************************
 * TAG( mesh_get_minmax )
 *
 * Traverses material result values to extract the min and max.
 *
 * This routine could be generalized to work for similar non-element
 * mesh object classes with the introduction of a generic approach
 * to deal with material disability (which may or may not be
 * pertinent to non-element, non-nodal classes).
 *
 * Data in val_mesh is assumed to be indexed by mesh id.
 */
void
mesh_get_minmax( float *val_mesh, Analysis *analy )
{
    int *mesh_ids;
    char **classes;
    int *sclasses;
    Result *p_result;
    int idx, subrec_idx;
    Subrec_obj *p_so;
    int mesh, srec;
    float *mm_val;

    p_result = analy->cur_result;
    srec = p_result->srec_id;
    idx = analy->result_index;
    subrec_idx = p_result->subrecs[idx];
    p_so = analy->srec_tree[srec].subrecs + subrec_idx;
    analy->db_query( analy->db_ident, QRY_SREC_MESH, (void *) &srec, NULL,
                     (void *) &mesh );

    /* Prepare to extract material min/max (values init'd in load_result()). */
    mm_val = analy->tmp_elem_mm.object_minmax;
    classes = analy->tmp_elem_mm.class_long_name;
    sclasses = analy->tmp_elem_mm.sclass;
    mesh_ids = analy->tmp_elem_mm.object_id;


    mm_val[0] = mm_val[1] = val_mesh[0];
    mesh_ids[0] = mesh_ids[1] = 1;
    classes[0] = classes[1]   = p_so->p_object_class->long_name;
    sclasses[0] = sclasses[1] = p_so->p_object_class->superclass;
}


/*****************************************************************
 * TAG( hex_vol )
 *
 * Compute the approximate volume of a hex element using 1-point
 * integration.  (Taken from DYNA code, subroutine vlmass().)
 */
float
hex_vol( float x[8], float y[8], float z[8] )
{
    float aj[9], vol;

    /*
     * Jacobian matrix.
     */
    aj[0] = -x[0]-x[1]+x[2]+x[3]-x[4]-x[5]+x[6]+x[7];
    aj[3] = -x[0]-x[1]-x[2]-x[3]+x[4]+x[5]+x[6]+x[7];
    aj[6] = -x[0]+x[1]+x[2]-x[3]-x[4]+x[5]+x[6]-x[7];
    aj[1] = -y[0]-y[1]+y[2]+y[3]-y[4]-y[5]+y[6]+y[7];
    aj[4] = -y[0]-y[1]-y[2]-y[3]+y[4]+y[5]+y[6]+y[7];
    aj[7] = -y[0]+y[1]+y[2]-y[3]-y[4]+y[5]+y[6]-y[7];
    aj[2] = -z[0]-z[1]+z[2]+z[3]-z[4]-z[5]+z[6]+z[7];
    aj[5] = -z[0]-z[1]-z[2]-z[3]+z[4]+z[5]+z[6]+z[7];
    aj[8] = -z[0]+z[1]+z[2]-z[3]-z[4]+z[5]+z[6]-z[7];

    /*
     * Jacobian.
     */
    vol = aj[0]*aj[4]*aj[8]+aj[1]*aj[5]*aj[6]+aj[2]*aj[3]*aj[7]-
          aj[2]*aj[4]*aj[6]-aj[1]*aj[3]*aj[8]-aj[0]*aj[5]*aj[7];
    vol = 0.0156250 * vol;

    return vol;
}


/*****************************************************************
 * TAG( hex_vol_exact )
 *
 * Compute the exact volume of a hex element using 2-point integration.
 * (Taken from DYNA code, subroutine vlmffb().)
 */
float
hex_vol_exact( float x[8], float y[8], float z[8] )
{
    float a45, a24, a52, a16, a31, a63, a27, a74, a38, a81, a86, a57;
    float a6345, a5238, a8624, a7416, a5731, a8127;
    float px1, px2, px3, px4, px5, px6, px7, px8;
    float vol;

    a45 = z[3]-z[4];
    a24 = z[1]-z[3];
    a52 = z[4]-z[1];
    a16 = z[0]-z[5];
    a31 = z[2]-z[0];
    a63 = z[5]-z[2];
    a27 = z[1]-z[6];
    a74 = z[6]-z[3];
    a38 = z[2]-z[7];
    a81 = z[7]-z[0];
    a86 = z[7]-z[5];
    a57 = z[4]-z[6];
    a6345 = a63-a45;
    a5238 = a52-a38;
    a8624 = a86-a24;
    a7416 = a74-a16;
    a5731 = a57-a31;
    a8127 = a81-a27;
    px1 = y[1]*a6345+y[2]*a24-y[3]*a5238+y[4]*a8624+y[5]*a52+y[7]*a45;
    px2 = y[2]*a7416+y[3]*a31-y[0]*a6345+y[5]*a5731+y[6]*a63+y[4]*a16;
    px3 = y[3]*a8127-y[0]*a24-y[1]*a7416-y[6]*a8624+y[7]*a74+y[5]*a27;
    px4 = y[0]*a5238-y[1]*a31-y[2]*a8127-y[7]*a5731+y[4]*a81+y[6]*a38;
    px5 = -y[7]*a7416+y[6]*a86+y[5]*a8127-y[0]*a8624-y[3]*a81-y[1]*a16;
    px6 = -y[4]*a8127+y[7]*a57+y[6]*a5238-y[1]*a5731-y[0]*a52-y[2]*a27;
    px7 = -y[5]*a5238-y[4]*a86+y[7]*a6345+y[2]*a8624-y[1]*a63-y[3]*a38;
    px8 = -y[6]*a6345-y[5]*a57+y[4]*a7416+y[3]*a5731-y[2]*a74-y[0]*a45;
    vol = (px1*x[0]+px2*x[1]+px3*x[2]+px4*x[3]+px5*x[4]+
           px6*x[5]+px7*x[6]+px8*x[7])/12.0;

    return vol;
}


/*****************************************************************
 * TAG( hex_to_nodal )
 *
 * Convert brick element result data to nodal data.
 *
 * Data in val_hex is assumed to be indexed by element id.
 *
 *************************************************************************************************
 * Modifications:
 *  I. R. Corey April 22, 2004: Check for zero volume and set
 *  to 1 if zero.
 *
 *  I. R. Corey October 08, 2007: Added Error Indicator result
 *  calculation.
 *  SCR #487.
 *
 *  Notes on the EI algorithm:
 *
 *      The basic Zienkiewicz-Zhu solution estimator (Z-Z, for short) is calculated by assuming
 *      a smooth solution, interpolated by the nodes of the mesh. To calculate this estimator:
 *
 *      1. At a particular node I determine the elements E^J_I which are attached to node I.
 *
 *      2. For each element E^J_I calculate the weight W^J_I. The weight typically used is
 *         the volume of the element.
 *
 *      3. For each node I calculate the total weight W^I as
 *             o  W^I = sum_J [ W^J_I ].
 *
 *      4. For any particular scalar, vector, or tensor quantity a, evaluate it at the centroid
 *         of each element E^J_I, providing the value a^J_I. Note for single-point elements or
 *         single-point data files (e.g., what is currently read into Griz) this is the same as
 *         the element value which Griz already operates with.
 *
 *      5. Then the smoothed solution nodal estimate is
 *             o  a_n^I = (1/W^I) sum_J [ W^J_I a^J_I ]
 *
 *      6. At each element K, calculate the element-based smoothed estimate
 *             o  a_s^K = (1/N) sum_I [ a_n^I ], where N is the number of nodes attached to
 *         element K, and a_n^I are the nodal estimates of the exact solution calculated in step 5.
 *
 *      7. Then an indicator of the error in the quantity at each element K can be calculated via
 *             o  a_e^K = a^K - a_s^K
 *
 *      8. A global error norm indicator can be calculated by summing over the elements
 *             o  a_(e,g) = sum_K [ W^K norm(a_e^K) ], where norm(a_e^K) is the appropriate scalar,
 *         vector, tensor, or symmetric tensor norm.
 *
 *      9. Similarly, a global norm of the quantity itself can be calculated as
 *             o  a_g = sum_K [ W^K norm(a^K) ]
 *
 *      10. A global error indicator can then be calculated as
 *             o  a_r = a_(e,g)/a_g
 *
 *      11. A local refinement indicator (between 0 and 1) for element K can be calculated
 *             o a^K_r = [ W^K norm(a_e^K) ]/ a_(e,g)
 *
 **************************************************************************************************
 */
void
hex_to_nodal( float *val_hex, float *val_nodal, MO_class_data *p_hex_class,
              int hex_qty, int *hex_ids, Analysis *analy )
{
    GVec3D *nodes;
    MO_class_data *p_node_geom;
    Mesh_data *p_mesh;
    float xx[8], yy[8], zz[8];
    float *hex_vols, *vol_sums, *activity, *mm_val;
    Bool_type *nodes_updated;
    float *val_nodal_input;
    Bool_type volume_average;
    int i, j, nd;
    int *el_id;
    char **classes;
    int *sclasses;
    int (*connects)[8];
    int *mats;
    int hex_id;

    Bool_type ignore_elem=FALSE;

    float temp_nodal;

    int num_nodes;

    /* Free Node & Particle Node variables */
    float *free_nodes_vol = NULL;
    int    fn_vol_found  = FALSE;
    int    status;
    Bool   particle_class = FALSE;

    /* Error Indicator Arrays */

    float *ei_elem, *ei_nodal, *ei_nodal_weight;
    float  ei_elem_val;

    /* Set the next line to FALSE to compare with TAURUS output. */
    volume_average = analy->vol_averaging;

    /*
     * ORIGINAL:
     *
    p_result = analy->cur_result;
    srec = p_result->srec_id;
    analy->db_query( analy->db_ident, QRY_SREC_MESH, (void *) &srec, NULL,
                     (void *) &mesh );
    p_mesh = analy->mesh_table + mesh;
     *
     * REPLACEMENT:
     */

    if ( is_particle_class( analy, p_hex_class->superclass, p_hex_class->short_name ) )
        particle_class = TRUE;

    p_mesh = MESH_P( analy );

    /* END REPLACEMENT */

    p_node_geom = p_mesh->node_geom;

    num_nodes   = p_node_geom->qty;
    connects    = (int (*) [8]) p_hex_class->objects.elems->nodes;
    mats        = p_hex_class->objects.elems->mat;

    nodes = analy->state_p->nodes.nodes3d;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_hex_class->elem_class_index]
               : NULL;

    hex_vols        = NEW_N( float, p_hex_class->qty, "Hex volumes" );
    vol_sums        = NEW_N( float, p_node_geom->qty, "Volume sums" );
    val_nodal_input = NEW_N( float, p_node_geom->qty, "Input val_nodal" );
    nodes_updated   = NEW_N( Bool_type, p_node_geom->qty, "List of updated nodes" );

    for ( i=0;
            i<p_node_geom->qty;
            i++ )
    {
        val_nodal_input[i] = val_nodal[i];
        nodes_updated[i]   = FALSE;
    }

    if ( analy->ei_result )
    {
        ei_elem         = NEW_N( float, hex_qty, "Error Indicator - Element" );
        ei_nodal        = NEW_N( float, num_nodes, "Error Indicator - Nodal" );
        ei_nodal_weight = NEW_N( float, num_nodes, "Error Indicator - Nodal Weight" );

        for ( i = 0; i < num_nodes; i++ )
        {
            ei_nodal[i]        = 0.0;
            ei_nodal_weight[i] = 0.0;
        }

        for ( i = 0;
                i <  hex_qty;
                i++ )
            ei_elem[i] = 0.0;
    }

    /* If free nodes are active then:
     *
     * Read the nodal volumes if volume scaling is enabled
     */
    if ( analy->free_nodes_enabled || (analy->particle_nodes_enabled && particle_class) )
    {
        status = mili_db_get_param_array(analy->db_ident,  "Nodal Volume", (void *) &free_nodes_vol);
        if (status==0)
        {
            fn_vol_found = TRUE;
        }
    }

    /*
     * Each node result is a volume-weighted average of the neighboring
     * element results.  The proper way to do this would be to interpolate
     * using the shape functions.
     */

    /* First loop to get the summed volumes. */
    for ( i = 0; i < hex_qty; i++ )
    {
        /* Get the element identifier. */
        hex_id = hex_ids ? hex_ids[i] : i;

        ignore_elem = FALSE;

        /* Ignore inactive elements. */
        if ( !particle_class && !fn_vol_found && activity && activity[hex_id] == 0.0 )
            continue;

        /* Ignore disabled materials. */
        if ( p_mesh->disable_material[ mats[hex_id] ] ||
                disable_by_object_type( p_hex_class, mats[hex_id], hex_id, analy, NULL ) )
            ignore_elem = TRUE;

        if ( particle_class && analy->particle_nodes_enabled &&
                !disable_by_object_type( p_hex_class, mats[hex_id], hex_id, analy, NULL ) )
            ignore_elem = FALSE;

        if (ignore_elem)
            continue;

        for ( j = 0; j < 8; j++ )
        {
            nd = connects[hex_id][j];
            xx[j] = nodes[nd][0];
            yy[j] = nodes[nd][1];
            zz[j] = nodes[nd][2];
        }

        if (fn_vol_found)
            hex_vols[hex_id] = free_nodes_vol[nd];
        else
        {
            hex_vols[hex_id] = hex_vol( xx, yy, zz );
        }

        if ( zero_vol_warn_once && hex_vols[hex_id] <= 0.0 )
        {
            popup_dialog( WARNING_POPUP,
                          "Element volume is zero or negative!\n"
                          "@ %s %d.\n(warning will not repeat)",
                          p_hex_class->short_name,
                          hex_id + 1 );
            zero_vol_warn_once = FALSE;
        }

        for ( j = 0; j < 8; j++ )
        {
            nd = connects[hex_id][j];
            nodes_updated[nd] = TRUE;
            if ( val_nodal_input[nd]!=0.0 )
                val_nodal[nd] = 0.0;

            if ( volume_average )
                vol_sums[nd] += hex_vols[hex_id];
            else
                vol_sums[nd] += 1.0;

            if ( analy->ei_result )
            {
                ei_nodal_weight[nd] += hex_vols[hex_id];
            }
        }
    }

    /* Prepare to extract element min/max (values init'd in load_result()). */
    mm_val  = analy->tmp_elem_mm.object_minmax;
    classes = analy->tmp_elem_mm.class_long_name;
    sclasses = analy->tmp_elem_mm.sclass;
    el_id   = analy->tmp_elem_mm.object_id;

    /* Loop to get the averaged values and extract state element min/max. */
    for ( i = 0; i < hex_qty; i++ )
    {
        /* Get the element identifier. */
        hex_id = hex_ids ? hex_ids[i] : i;

        ignore_elem = FALSE;

        /* Ignore inactive elements. */
        if ( !particle_class && !fn_vol_found && activity && activity[hex_id] == 0.0 )
            continue;

        /* Ignore disabled materials. */
        if ( p_mesh->disable_material[ mats[hex_id] ] ||
                disable_by_object_type( p_hex_class, mats[hex_id], hex_id, analy, NULL ) )
            ignore_elem = TRUE;

        if ( particle_class && analy->particle_nodes_enabled &&
                !disable_by_object_type( p_hex_class, mats[hex_id], hex_id, analy, NULL ) )
            ignore_elem = FALSE;

        if (ignore_elem)
            continue;

        /* Test for new min or max. */
        if ( val_hex[hex_id] < mm_val[0] )
        {
            mm_val[0]   = val_hex[hex_id];
            el_id[0]    = hex_id + 1;
            classes[0]  = p_hex_class->long_name;
            sclasses[0] = p_hex_class->superclass;
        }

        if ( val_hex[hex_id] > mm_val[1] )
        {
            mm_val[1]   = val_hex[hex_id];
            el_id[1]    = hex_id + 1;
            classes[1]  = p_hex_class->long_name;
            sclasses[1] = p_hex_class->superclass;
        }

        analy->result_active = TRUE; /* We have some valid result data */

        if ( analy->ei_result )
            ei_elem[hex_id] = 0.0;

        for ( j = 0; j < 8; j++ )
        {
            nd = connects[hex_id][j];
            /*
             * Divide hex_vol by vol_sum first to avoid precision errors
             * for models with very small dimensions.
             */

            /*
             * Check for zero volume and if zero set to 1
             * to avoid dividing by zero.
             */
            if ( vol_sums[nd]<=0.)
                vol_sums[nd] = 1.0;

            if ( particle_class )
                val_nodal[nd] = val_hex[hex_id];
            else if ( volume_average )
            {

                val_nodal[nd] += val_hex[hex_id]
                                 * ( hex_vols[hex_id] / vol_sums[nd] );
            }
            else
                val_nodal[nd] += val_hex[hex_id] / vol_sums[nd];

            if ( analy->ei_result )
            {
                ei_nodal[nd] += (hex_vols[hex_id] * val_hex[hex_id]);
                val_nodal[nd] = 0.0;
            }
        }
    }

    if ( analy->ei_result )
        for ( i = 0; i < num_nodes; i++ )
        {
            if ( ei_nodal_weight[i]!=0 )
                ei_nodal[i] = (1/ei_nodal_weight[i])*ei_nodal[i] ;
            else
                ei_nodal[i] = 0;
        }

    /* Compute an Error Indicator result from the selected (primal or derived) result */
    if ( analy->ei_result )
    {
        for ( i = 0; i < hex_qty; i++ )
        {
            /* Get the element identifier. */
            hex_id = hex_ids ? hex_ids[i] : i;

            /* Ignore inactive elements. */
            ignore_elem = FALSE;

            /* Ignore inactive elements. */
            if ( !particle_class && !fn_vol_found && activity && activity[hex_id] == 0.0 )
                continue;

            /* Ignore disabled materials. */
            if ( p_mesh->disable_material[ mats[hex_id] ] ||
                    disable_by_object_type( p_hex_class, mats[hex_id], hex_id, analy, NULL ) )
                ignore_elem = TRUE;

            if ( particle_class && analy->particle_nodes_enabled &&
                    !disable_by_object_type( p_hex_class, mats[hex_id], hex_id, analy, NULL ) )
                ignore_elem = FALSE;

            if (ignore_elem)
                continue;

            for ( j = 0; j < 8; j++ )
            {
                nd = connects[hex_id][j];
                temp_nodal = ei_nodal[nd];
                ei_elem[hex_id] += ei_nodal[nd];
            }
            ei_elem[hex_id] = ei_elem[hex_id]/8.0;
            ei_elem[hex_id] = val_hex[hex_id] - ei_elem[hex_id];

            val_hex[hex_id] = ei_elem[hex_id];
        }

        for ( i = 0; i < hex_qty; i++ )
        {
            /* Get the element identifier. */
            hex_id = hex_ids ? hex_ids[i] : i;

            /* Ignore inactive elements. */
            if ( !particle_class && !fn_vol_found && activity && activity[hex_id] == 0.0 )
                continue;

            /* Ignore disabled materials. */
            if ( p_mesh->disable_material[ mats[hex_id] ] ||
                    disable_by_object_type( p_hex_class, mats[hex_id], hex_id, analy, NULL ) )
                continue;

            for ( j = 0; j < 8; j++ )
            {
                nd = connects[hex_id][j];

                if ( analy->ei_result )
                {
                    val_nodal[nd] += ei_elem[hex_id];
                }
                else
                {
                    if ( volume_average )
                        val_nodal[nd] += (double)ei_elem[hex_id]
                                         * ( (double)hex_vols[hex_id] / vol_sums[nd] );
                    else if ( vol_sums[nd] != 0. )
                        val_nodal[nd] += ei_elem[hex_id] / vol_sums[nd];
                }
            }
        }

        analy->ei_error_norm += hex_vols[hex_id]*ei_elem[i];
        analy->ei_norm_qty   += hex_vols[hex_id]*ei_elem[i]*8;

        analy->ei_global_indicator      = 0;
        if ( analy->ei_norm_qty!= 0. )
            analy->ei_global_indicator = analy->ei_error_norm/analy->ei_norm_qty;

        analy->ei_refinement_indicator = 0.;
        analy->ei_global_indicator     = 0.;
        analy->ei_refinement_indicator = 0.;

        free( ei_elem );
        free( ei_nodal );
        free( ei_nodal_weight );
    }

    /* Average out nodes that were already computed with other subrecords
     * or element sets.
     */
    for ( i=0;
            i<p_node_geom->qty;
            i++ )
    {
        if ( !particle_class && nodes_updated[i] &&  val_nodal_input[i]!=0.0 )
        {
            val_nodal[i] = ((double)val_nodal[i]+val_nodal_input[i])/(double)2.0;
        }
    }

    free( hex_vols );
    free( vol_sums );
    free( nodes_updated );
    free( val_nodal_input );
}

/*****************************************************************
 * TAG( particle_to_nodal )
 *
 * Convert brick element result data to nodal data.
 *
 * Data in val_part is assumed to be indexed by element id.
 *
 *****************************************************************
 * Modifications:
 *  I. R. Corey February 22, 2012: Created.
 *
 ****************************************************************
 */
void
particle_to_nodal( float *val_part, float *val_nodal, MO_class_data *p_part_class,
                   int part_qty, int *part_ids, Analysis *analy )
{
    GVec3D *nodes;
    MO_class_data *p_node_geom;
    Mesh_data *p_mesh;
    float *activity, *mm_val;
    int i, j, nd;
    int *el_id, *sclasses;
    char **classes;
    int (*connects)[1];
    int *mats;
    int part_id;

    Bool_type ignore_elem=FALSE;

    float temp_nodal;

    int num_nodes=0;

    int status;

    p_mesh = MESH_P( analy );

    p_node_geom = p_mesh->node_geom;
    num_nodes   = p_node_geom->qty;
    connects    = (int (*) [1]) p_part_class->objects.elems->nodes;
    mats        = p_part_class->objects.elems->mat;
    sclasses    = analy->tmp_elem_mm.sclass;
    el_id       = analy->tmp_elem_mm.object_id;

    nodes    = analy->state_p->nodes.nodes3d;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_part_class->elem_class_index]
               : NULL;

    /* Prepare to extract element min/max (values init'd in load_result()). */
    mm_val  = analy->tmp_elem_mm.object_minmax;
    classes = analy->tmp_elem_mm.class_long_name;

    /* Loop to get the averaged values and extract state element min/max. */
    for ( i = 0;
            i < part_qty;
            i++ )
    {
        /* Get the element identifier. */
        part_id = part_ids ? part_ids[i] : i;

        ignore_elem = FALSE;

        /* Ignore inactive elements. */
        if (  activity && activity[part_id] == 0.0 )
            continue;

        /* Ignore disabled materials. */

        if ( analy->particle_nodes_enabled &&
                !disable_by_object_type( p_part_class, mats[part_id], part_id, analy, NULL ) )
            ignore_elem = FALSE;

        if (ignore_elem)
            continue;

        /* Test for new min or max. */
        if ( val_part[part_id] < mm_val[0] )
        {
            mm_val[0]   = val_part[part_id];
            classes[0]  = p_part_class->long_name;
        }

        if ( val_part[part_id] > mm_val[1] )
        {
            mm_val[1]   = val_part[part_id];
            el_id[1]    = part_id + 1;
            classes[1]  = p_part_class->long_name;
            sclasses[0] = p_part_class->superclass;
        }

        analy->result_active = TRUE; /* We have some valid result data */

        nd = connects[part_id][0];

        val_nodal[nd] = val_part[part_id];
    }
}


/*****************************************************************
 * TAG( hex_to_nodal_by_mat )
 *
 * Convert brick element result data to nodal data, but loop
 * first over materials that are enabled.
 *
 * Assumes any subsetting of the class' elements are factored into
 * active_mats and p_mats.
 *
 * Data in val_hex is assumed to be indexed by element id.
 *
 *****************************************************************
 * Modifications:
 *  I. R. Corey October 08, 2007: Added Error Indicator result
 *  calculation.
 *  SCR #487.
 *
 *  Notes on the EI algorithm: (see hex_to_nodal)
 *
 *****************************************************************
 */
void
hex_to_nodal_by_mat( float *val_hex, float *val_nodal,
                     MO_class_data *p_hex_class,
                     int active_mat_qty, int *active_mats,
                     Material_data *p_mats, Analysis *analy )
{
    GVec3D *nodes;
    MO_class_data *p_node_geom;
    Mesh_data *p_mesh;
    float xx[8], yy[8], zz[8];
    float *hex_vols, *vol_sums, *activity, *mm_val;
    Bool_type volume_average;
    int i, j, m, n, nd;
    int *el_id;
    char **classes;
    int *sclasses;
    int (*connects)[8];
    int hex_id, hex_ids;
    int elem_block_qty;
    int start, stop;
    int cur_mat;
    int *mats;
    int num_hexes, num_nodes;
    int hex_class_qty=0;
    float vol_term;

    /* Free Nodes variables */
    float *free_nodes_vol = NULL;
    int    fn_vol_found  = FALSE;
    int    status;
    Bool   particle_class = FALSE;

    /* Error Indicator Arrays */
    int   hex_qty = 0;
    float *ei_elem, *ei_nodal, *ei_nodal_weight;
    float ei_elem_val;

    if ( is_particle_class( analy, p_hex_class->superclass, p_hex_class->short_name ) )
        particle_class = TRUE;

    /* Set the next line to FALSE to compare with TAURUS output. */
    volume_average = TRUE;

    p_mesh = MESH_P( analy );
    p_node_geom = p_mesh->node_geom;
    num_hexes   = p_hex_class->qty;
    num_nodes   = p_node_geom->qty;
    connects = (int (*) [8]) p_hex_class->objects.elems->nodes;
    mats        = p_hex_class->objects.elems->mat;

    nodes = analy->state_p->nodes.nodes3d;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_hex_class->elem_class_index]
               : NULL;

    hex_vols = NEW_N( float, p_hex_class->qty, "Hex volumes" );
    vol_sums = NEW_N( float, p_node_geom->qty, "Volume sums" );

    if ( analy->ei_result )
    {
        ei_elem         = NEW_N( float, num_hexes, "Error Indicator - Element" );
        ei_nodal        = NEW_N( float, num_nodes, "Error Indicator - Nodal" );
        ei_nodal_weight = NEW_N( float, num_nodes, "Error Indicator - Nodal Weight" );

        for ( i = 0; i < num_nodes; i++ )
        {
            ei_nodal[i]        = 0.0;
            ei_nodal_weight[i] = 0.0;
        }

        for ( i = 0; i < num_hexes; i++ )
            ei_elem[i] = 0.0;
    }

    /* If free nodes are active then:
     *
     * Read the nodal volumes if volume scaling is enabled
     */
    if (analy->free_nodes_enabled || analy->particle_nodes_enabled)
    {
        status = mili_db_get_param_array(analy->db_ident,  "Nodal Volume", (void *) &free_nodes_vol);
        if (status==0)
        {
            fn_vol_found = TRUE;
        }
    }

    /*
     * Each node result is a volume-weighted average of the neighboring
     * element results.  The proper way to do this would be to interpolate
     * using the shape functions.
     */

    /* First loop to get the summed volumes. */

    hex_class_qty = MESH( analy ).classes_by_sclass[G_HEX].qty;

    /* Loop over the active materials. */

    if ( !particle_class )
        for ( m = 0; m < active_mat_qty; m++ )
        {
            cur_mat = active_mats[m];
            elem_block_qty = p_mats[cur_mat].elem_block_qty;

            /* Loop over the element blocks of the current active material. */
            for ( n = 0; n < elem_block_qty; n++ )
            {
                start = p_mats[cur_mat].elem_blocks[n][0];
                stop = p_mats[cur_mat].elem_blocks[n][1];

                if (  hex_class_qty>1 && stop>=p_hex_class->qty )
                {
                    start = 0;
                    stop  = p_hex_class->qty-1;
                }

                /* Loop over the elements in the current element block. */
                for ( hex_id = start; hex_id <= stop; hex_id++ )
                {
                    /* Ignore inactive elements. */
                    if ( !fn_vol_found && activity && activity[hex_id] == 0.0 )
                        continue;

                    /* Ignore disabled materials. */
                    if ( p_mesh->disable_material[ cur_mat ] ||
                            disable_by_object_type( p_hex_class, cur_mat, hex_id, analy, NULL ) )
                        continue;

                    hex_qty++;

                    analy->result_active = TRUE; /* We have some valid result data */

                    for ( j = 0; j < 8; j++ )
                    {
                        nd = connects[hex_id][j];
                        xx[j] = nodes[nd][0];
                        yy[j] = nodes[nd][1];
                        zz[j] = nodes[nd][2];
                    }

                    if (fn_vol_found)
                        hex_vols[hex_id] = free_nodes_vol[nd];
                    else
                        hex_vols[hex_id] = hex_vol( xx, yy, zz );

                    if ( zero_vol_warn_once && hex_vols[hex_id] <= 0.0 )
                    {
                        popup_dialog( WARNING_POPUP,
                                      "Element volume is zero or negative!\n"
                                      "%s %d\n(warning will not repeat)",
                                      p_mats[cur_mat].elem_class->long_name,
                                      hex_id + 1 );
                        zero_vol_warn_once = FALSE;
                    }

                    for ( j = 0; j < 8; j++ )
                    {
                        nd = connects[hex_id][j];

                        if ( volume_average )
                            vol_sums[nd] += hex_vols[hex_id];
                        else
                            vol_sums[nd] += 1.0;

                        if ( analy->ei_result )
                        {
                            ei_nodal_weight[nd] += hex_vols[hex_id];
                        }
                    }
                } /* for hex element in element block */
            } /* for element block */
        } /* for active material */

    /* Prepare to extract element min/max (values init'd in load_result()). */
    mm_val = analy->tmp_elem_mm.object_minmax;
    classes = analy->tmp_elem_mm.class_long_name;
    sclasses = analy->tmp_elem_mm.sclass;
    el_id = analy->tmp_elem_mm.object_id;

    /* Loop to get the averaged values and extract state element min/max. */

    /* Loop over the active materials. */
    for ( m = 0; m < active_mat_qty; m++ )
    {
        cur_mat = active_mats[m];
        elem_block_qty = p_mats[cur_mat].elem_block_qty;

        /* Loop over the element blocks of the current active material. */
        for ( n = 0; n < elem_block_qty; n++ )
        {
            start = p_mats[cur_mat].elem_blocks[n][0];
            stop = p_mats[cur_mat].elem_blocks[n][1];

            if ( hex_class_qty>1 && stop>=p_hex_class->qty )
            {
                start = 0;
                stop  = p_hex_class->qty-1;
            }

            /* Loop over the elements in the current element block. */
            for ( hex_id = start; hex_id <= stop; hex_id++ )
            {
                /* Ignore inactive elements. */
                if ( !analy->free_nodes_enabled && !analy->particle_nodes_enabled && activity && activity[hex_id] == 0.0 )
                    continue;

                /* Ignore disabled materials. */
                if ( p_mesh->disable_material[ cur_mat ] ||
                        disable_by_object_type( p_hex_class, cur_mat, hex_id, analy, NULL ) )
                    continue;

                /* Test for new min or max. */
                if ( val_hex[hex_id] < mm_val[0] )
                {
                    mm_val[0] = val_hex[hex_id];
                    el_id[0] = hex_id + 1;
                    classes[0] = p_hex_class->long_name;
                    sclasses[0] = p_hex_class->superclass;
                }

                if ( val_hex[hex_id] > mm_val[1] )
                {
                    mm_val[1] = val_hex[hex_id];
                    el_id[1] = hex_id + 1;
                    classes[1] = p_hex_class->long_name;
                    sclasses[1] = p_hex_class->superclass;
                }

                if ( !particle_class )
                    for ( j = 0; j < 8; j++ )
                    {
                        nd = connects[hex_id][j];

                        /*
                         * Divide hex_vol by vol_sum first to avoid precision
                         * errors for models with very small dimensions.
                         */
                        if ( volume_average )
                        {
                            if ( vol_sums[nd]>0.0 )
                                vol_term = hex_vols[hex_id] / vol_sums[nd];
                            else vol_term = hex_vols[hex_id];
                            if ( vol_term == 0.0 )
                                vol_term = 1,0;

                            val_nodal[nd] += val_hex[hex_id]
                                             * ( vol_term );
                        }
                        else
                        {
                            if ( vol_sums[nd]==0.0 )
                                val_nodal[nd] += val_hex[hex_id];
                            else
                                val_nodal[nd] += val_hex[hex_id] / vol_sums[nd];
                        }
                    }
                else
                {
                    nd = connects[hex_id][0];
                    val_nodal[nd] = val_hex[hex_id];
                }

            } /* for hex element in element block */
        } /* for element block */
    } /* for active material */


    /* Compute an Error Indicator result from the selected (primal or derived) result */
    if ( analy->ei_result && !particle_class )
    {
        /* Loop over the active materials. */
        for ( m = 0; m < active_mat_qty; m++ )
        {
            cur_mat        = active_mats[m];
            elem_block_qty = p_mats[cur_mat].elem_block_qty;

            /* Loop over the element blocks of the current active material. */
            for ( n = 0; n < elem_block_qty; n++ )
            {
                start = p_mats[cur_mat].elem_blocks[n][0];
                stop  = p_mats[cur_mat].elem_blocks[n][1];

                /* Loop over the elements in the current element block. */
                for ( hex_id = start; hex_id <= stop; hex_id++ )
                {
                    /* Ignore inactive elements. */
                    if ( !particle_class && !fn_vol_found && activity && activity[hex_id] == 0.0 )
                        continue;

                    /* Ignore disabled materials. */
                    if ( p_mesh->disable_material[ cur_mat ] ||
                            disable_by_object_type( p_hex_class, cur_mat, hex_id, analy, NULL ) )
                        continue;

                    for ( j = 0; j < 8; j++ )
                    {
                        nd = connects[hex_id][j];
                        vol_sums[nd] += hex_vols[hex_id];

                        ei_nodal[nd] += hex_vols[hex_id] * val_hex[hex_id];
                    }

                    ei_elem[hex_id] = (1.0/8.0)*ei_elem[hex_id];
                    if ( ei_elem[hex_id] <0.0 )
                        ei_elem[hex_id] = -ei_elem[hex_id];

                    val_hex[hex_id] = ei_elem[hex_id];

                    for ( j = 0; j < 8; j++ )
                    {
                        nd = connects[hex_id][j];
                        val_nodal[nd] = ei_elem[hex_id]*8 - ei_elem[hex_id];
                    }
                } /* Loop over hexs */
            } /* Loop over elem blocks */
        } /* Loop over mats */

        for ( i = 0; i < num_nodes; i++ )
        {
            if ( ei_nodal_weight[i]!=0 )
                ei_nodal[i] = (1/ei_nodal_weight[i])*ei_nodal[i] ;
            else
                ei_nodal[i] = 0;
        }

        /* Loop over the active materials. */
        for ( m = 0; m < active_mat_qty; m++ )
        {
            cur_mat = active_mats[m];
            elem_block_qty = p_mats[cur_mat].elem_block_qty;

            /* Loop over the element blocks of the current active material. */
            for ( n = 0; n < elem_block_qty; n++ )
            {
                start = p_mats[cur_mat].elem_blocks[n][0];
                stop  = p_mats[cur_mat].elem_blocks[n][1];

                /* Loop over the elements in the current element block. */
                for ( hex_id = start; hex_id <= stop; hex_id++ )
                {

                    /* Ignore inactive elements. */
                    if ( !particle_class && !fn_vol_found && activity && activity[hex_id] == 0.0 )
                        continue;

                    /* Ignore disabled materials. */
                    if ( p_mesh->disable_material[ mats[hex_id] ] ||
                            disable_by_object_type( p_hex_class, mats[hex_id], hex_id, analy, NULL ) )
                        continue;

                    for ( j = 0; j < 8; j++ )
                    {
                        nd = connects[hex_id][j];

                        if ( analy->ei_result )
                        {
                            val_nodal[nd] += ei_elem[hex_id];
                        }
                        else
                        {
                            if ( volume_average )
                                val_nodal[nd] += ei_elem[hex_id]
                                                 * ( hex_vols[hex_id] / vol_sums[nd] );
                            else if ( vol_sums[nd] != 0. )
                                val_nodal[nd] += ei_elem[hex_id] / vol_sums[nd];
                        }
                    }
                }
            }
        }

        analy->ei_error_norm           = 0.;
        analy->ei_norm_qty             = 0.;
        analy->ei_global_indicator     = 0.;
        analy->ei_refinement_indicator = 0.;

        mm_val[0] = MAXFLOAT;
        mm_val[1] = MINFLOAT;

        /* Calculate Global EI quantities */
        /* Loop over the active materials. */
        for ( m = 0; m < active_mat_qty; m++ )
        {
            cur_mat = active_mats[m];
            elem_block_qty = p_mats[cur_mat].elem_block_qty;

            /* Loop over the element blocks of the current active material. */
            for ( n = 0; n < elem_block_qty; n++ )
            {
                start = p_mats[cur_mat].elem_blocks[n][0];
                stop = p_mats[cur_mat].elem_blocks[n][1];

                /* Loop over the elements in the current element block. */
                for ( hex_id = start; hex_id <= stop; hex_id++ )
                {
                    /* Ignore inactive elements. */
                    if ( !analy->free_nodes_enabled && !analy->particle_nodes_enabled && activity && activity[hex_id] == 0.0 )
                        continue;

                    /* Ignore disabled materials. */
                    if ( p_mesh->disable_material[ mats[hex_id] ] ||
                            disable_by_object_type( p_hex_class, cur_mat, hex_id, analy, NULL ) )
                        continue;

                    for ( j = 0; j < 8; j++ )
                    {
                        nd = connects[hex_id][j];
                        vol_sums[nd] += hex_vols[hex_id];

                        ei_elem[hex_id] += ei_nodal[nd];
                        val_nodal[nd]    = ei_nodal[nd];
                    }

                    ei_elem[hex_id] = val_hex[hex_id] - (0.125)*ei_elem[hex_id];
                    val_hex[hex_id] = ei_elem[hex_id];

                    ei_elem_val = ei_elem[hex_id];

                    /* Update Min max using EI values */
                    if ( ei_elem_val < mm_val[0] )
                    {
                        mm_val[0]   = ei_elem_val;
                        el_id[0]    = hex_id + 1;
                        classes[0]  = p_hex_class->long_name;
                        sclasses[0] = p_hex_class->superclass;
                    }
                }

                if ( ei_elem_val > mm_val[1] )
                {
                    mm_val[1]   = ei_elem_val;
                    el_id[1]    = hex_id + 1;
                    classes[1]  = p_hex_class->long_name;
                    sclasses[1] = p_hex_class->superclass;
                }

                analy->ei_error_norm += hex_vols[hex_id]*ei_elem[i];
                analy->ei_norm_qty   += hex_vols[hex_id]*ei_elem[i]*8;

                analy->ei_global_indicator      = 0;
                if ( analy->ei_norm_qty!= 0. )
                    analy->ei_global_indicator = analy->ei_error_norm/analy->ei_norm_qty;

                analy->ei_refinement_indicator  = 0.;
            }
        }

        free( ei_elem );
        free( ei_nodal );
    }

    free( hex_vols );
    free( vol_sums );
}


/*****************************************************************
 * TAG( tet_vol )
 *
 * Returns the volume of a tetrahedron.
 */
float
tet_vol( float verts[4][3] )
{
    float v1[3], v2[3], vc[3];
    double vol;

    VEC_SUB( v1, verts[1], verts[0] );
    VEC_SUB( v2, verts[2], verts[0] );
    VEC_CROSS( vc, v1, v2 );

    vol = 0.1666667
          * ( (double) VEC_DOT( vc, verts[3] )
              - (double) VEC_DOT( vc, verts[0] ) );

    return (float) fabs( vol );
}


/*****************************************************************
 * TAG( tet_to_nodal )
 *
 * Convert tetrahedral element result data to nodal data.
 *
 * Data in val_tet is assumed to be indexed by element id.
 */
void
tet_to_nodal( float *val_tet, float *val_nodal, MO_class_data *p_tet_class,
              int tet_qty, int *tet_ids, Analysis *analy )
{
    GVec3D *nodes;
    MO_class_data *p_node_geom;
    Mesh_data *p_mesh;
    float tet_verts[4][3];
    float *tet_vols, *vol_sums, *activity, *mm_val;
    float *val_nodal_input;
    Bool_type *nodes_updated;
    int i, j, k, nd;
    int *el_id;
    char **classes;
    int *sclasses;
    int (*connects)[4];
    int *mats;
    int tet_id;

    /*
     * NOTE:  Original coding as per "hex_to_nodal"
     */

    p_mesh = MESH_P( analy );

    p_node_geom = p_mesh->node_geom;
    connects = (int (*) [4]) p_tet_class->objects.elems->nodes;
    mats = p_tet_class->objects.elems->mat;
    nodes = analy->state_p->nodes.nodes3d;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_tet_class->elem_class_index]
               : NULL;

    tet_vols        = NEW_N( float, tet_qty, "Tet volumes" );
    vol_sums        = NEW_N( float, p_node_geom->qty, "Volume sums" );
    val_nodal_input = NEW_N( float, p_node_geom->qty, "Input val_nodal" );
    nodes_updated   = NEW_N( Bool_type, p_node_geom->qty, "List of updated nodes" );

    for ( i=0;
            i<p_node_geom->qty;
            i++ )
    {
        val_nodal_input[i] = val_nodal[i];
        nodes_updated[i]   = FALSE;
    }

    /*
     * Each node result is a volume-weighted average of the neighboring
     * element results.  The proper way to do this would be to interpolate
     * using the shape functions.
     */

    /* First loop to get the summed volumes. */
    for ( i = 0; i < tet_qty; i++ )
    {
        /* Get the element identifier. */
        tet_id = tet_ids ? tet_ids[i] : i;

        /* Ignore inactive elements. */
        if ( activity && activity[tet_id] == 0.0 )
            continue;

        /* Ignore disabled materials. */
        if ( p_mesh->disable_material[ mats[tet_id] ] )
            continue;

        for ( j = 0; j < 4; j++ )
        {
            nd = connects[tet_id][j];

            for ( k = 0; k < 3; k++ )
                tet_verts[j][k] = nodes[nd][k];
        }

        tet_vols[i] = tet_vol( tet_verts );

        if ( zero_vol_warn_once && tet_vols[i] <= 0.0 )
        {
            popup_dialog( WARNING_POPUP,
                          "Element volume is zero or negative!\n"
                          "(warning will not repeat for this database load)" );
            zero_vol_warn_once = FALSE;
        }

        for ( j = 0; j < 4; j++ )
        {
            nd = connects[tet_id][j];
            vol_sums[nd] += tet_vols[i];
            nodes_updated[nd] = TRUE;
            if ( val_nodal_input[nd]!=0.0 )
                val_nodal[nd] = 0.0;

        }
    }

    /* Prepare to extract element min/max (values init'd in load_result()). */
    mm_val = analy->tmp_elem_mm.object_minmax;
    classes = analy->tmp_elem_mm.class_long_name;
    sclasses = analy->tmp_elem_mm.sclass;
    el_id = analy->tmp_elem_mm.object_id;

    /* Loop to get the averaged values and extract state element min/max. */
    for ( i = 0; i < tet_qty; i++ )
    {
        /* Get the element identifier. */
        tet_id = tet_ids ? tet_ids[i] : i;

        /* Ignore inactive elements. */
        if ( activity && activity[tet_id] == 0.0 )
            continue;

        /* Ignore disabled materials. */
        if ( p_mesh->disable_material[ mats[tet_id] ] )
            continue;

        /* Test for new min or max. */
        if ( val_tet[tet_id] < mm_val[0] )
        {
            mm_val[0]   = val_tet[tet_id];
            el_id[0]    = tet_id + 1;
            classes[0]  = p_tet_class->long_name;
            sclasses[0] = p_tet_class->superclass;
        }

        if ( val_tet[tet_id] > mm_val[1] )
        {
            mm_val[1]   = val_tet[tet_id];
            el_id[1]    = tet_id + 1;
            classes[1]  = p_tet_class->long_name;
            sclasses[1] = p_tet_class->superclass;
        }

        for ( j = 0; j < 4; j++ )
        {
            nd = connects[tet_id][j];

            /*
             * Divide tet_vol by vol_sum first to avoid precision errors
             * for models with very small dimensions.
             */
            val_nodal[nd] += val_tet[tet_id]
                             * ( tet_vols[i] / vol_sums[nd] );
        }
    }

    /* Average out nodes that were already computed with other subrecords
     * element sets.
     */
    for ( i=0;
            i<p_node_geom->qty;
            i++ )
    {
        if ( nodes_updated[i] &&  val_nodal_input[i]!=0.0 )
        {
            val_nodal[i] = (val_nodal[i]+val_nodal_input[i])/2.0;
        }
    }

    free( tet_vols );
    free( vol_sums );
    free( nodes_updated );
    free( val_nodal_input );
}


/*****************************************************************
 * TAG( quad_to_nodal )
 *
 * Convert quad element result data to nodal data.  If the merge
 * flag is TRUE, nodes that don't belong to quad elements aren't
 * zeroed.
 *
 * Data in val_quad is assumed to be indexed by element id.
 */
void
quad_to_nodal( float *val_quad, float *val_nodal, MO_class_data *p_quad_class,
               int quad_qty, int *quad_ids, Analysis *analy,
               Bool_type merge )
{
    Mesh_data *p_mesh;
    MO_class_data *p_node_geom;
    float *activity, *mm_val, *val_nodal_input;
    Bool_type *nodes_updated;
    int *adj_cnt=NULL;
    int i, j, nd=0;
    int *el_id;
    char **classes;
    int *sclasses;
    int (*connects)[4];
    int *mats;
    int quad_id=0;

    double *val_nodal_dbl=NULL, val_dbl;

    /*
     * NOTE:  Original coding as per "hex_to_nodal"
     */

    p_mesh = MESH_P( analy );

    p_node_geom = p_mesh->node_geom;
    connects = (int (*) [4]) p_quad_class->objects.elems->nodes;
    mats = p_quad_class->objects.elems->mat;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_quad_class->elem_class_index]
               : NULL;

    adj_cnt         = NEW_N( int,    p_node_geom->qty, "Tmp quad result cnts" );
    val_nodal_dbl   = NEW_N( double, p_node_geom->qty, "Tmp quad result cnts" );
    val_nodal_input = NEW_N( float, p_node_geom->qty, "Input val_nodal" );
    nodes_updated   = NEW_N( Bool_type, p_node_geom->qty, "List of updated nodes" );

    for ( i=0;
            i<p_node_geom->qty;
            i++ )
    {
        val_nodal_input[i] = val_nodal[i];
        nodes_updated[i]   = FALSE;
    }

    if ( merge )
    {
        /* Zero selectively. */
        for ( i = 0; i < quad_qty; i++ )
        {
            /* Get the element identifier. */
            quad_id = quad_ids ? quad_ids[i] : i;

            if ( activity )
                if ( activity[quad_id] == 0.0 ||
                        p_mesh->disable_material[ mats[quad_id] ] ||
                        disable_by_object_type( p_quad_class, mats[quad_id], quad_id, analy, NULL ) )
                    continue;

            for ( j = 0; j < 4; j++ )
            {
                val_nodal[connects[quad_id][j]] = 0.0;
            }
        }
    }

    for ( i = 0;
            i < quad_qty;
            i++ )
    {
        /* Get the element identifier. */
        quad_id = quad_ids ? quad_ids[i] : i;

        if ( activity )
            if ( activity[quad_id] == 0.0 ||
                    p_mesh->disable_material[ mats[quad_id] ] ||
                    disable_by_object_type( p_quad_class, mats[quad_id], quad_id, analy, NULL ) )
                continue;

        for ( j = 0;
                j < 4;
                j++ )
        {
            nd = connects[quad_id][j];
            nodes_updated[nd] = TRUE;
        }
    }

    for ( i=0;
            i<p_node_geom->qty;
            i++ )
        if ( nodes_updated[i] && val_nodal_input[i]!=0.0 )
            val_nodal[i] = 0.0;

    for ( i = 0;
            i < p_node_geom->qty;
            i++ )
        val_nodal_dbl[i] = val_nodal[i];

    /* Prepare to extract element min/max (values init'd in load_result()). */
    mm_val = analy->tmp_elem_mm.object_minmax;
    classes = analy->tmp_elem_mm.class_long_name;
    sclasses = analy->tmp_elem_mm.sclass;
    el_id = analy->tmp_elem_mm.object_id;

    /*
     * Each node result is an average of the neighboring
     * element results.  Extract element min/max.
     */
    for ( i = 0; i < quad_qty; i++ )
    {
        /* Get the element identifier. */
        quad_id = quad_ids ? quad_ids[i] : i;

        /* Ignore inactive elements. */
        if ( activity )
            if ( activity[quad_id] == 0.0 )
                continue;

        /* Ignore disabled materials. */
        if ( p_mesh->disable_material[ mats[quad_id] ] ||
                disable_by_object_type( p_quad_class, mats[quad_id], quad_id, analy, NULL ) )
        {
            continue;
        }

        /* Test for new min or max. */
        if ( val_quad[quad_id] < mm_val[0] )
        {
            mm_val[0] = val_quad[quad_id];
            el_id[0] = quad_id + 1;
            classes[0] = p_quad_class->long_name;
            sclasses[0] = p_quad_class->superclass;
        }

        if ( val_quad[quad_id] > mm_val[1] )
        {
            mm_val[1] = val_quad[quad_id];
            el_id[1] = quad_id + 1;
            classes[1] = p_quad_class->long_name;
            sclasses[1] = p_quad_class->superclass;
        }

        for ( j = 0; j < 4; j++ )
        {
            nd = connects[quad_id][j];
            val_nodal_dbl[nd] += (double) val_quad[quad_id];
            adj_cnt[nd]++;
        }
    }

    for ( i = 0; i < p_node_geom->qty; i++ )
        if ( adj_cnt[i] != 0 )
        {
            val_dbl      = (double) val_nodal_dbl[i]/adj_cnt[i];
            val_nodal[i] = (float) val_dbl;
        }

    /* Average out nodes that were already computed with other subrecords
     * or element sets.
     */
    for ( i=0;
            i<p_node_geom->qty;
            i++ )
    {
        if ( nodes_updated[i] &&  val_nodal_input[i]!=0.0 )
        {
            val_nodal[i] = (val_nodal[i]+val_nodal_input[i])/2.0;
        }
    }

    free( adj_cnt );
    free( val_nodal_dbl );
    free( nodes_updated );
    free( val_nodal_input );
}


/*****************************************************************
 * TAG( surf_to_nodal )
 *
 * Convert surface result data to nodal data.  If the merge
 * flag is TRUE, nodes that don't belong to surface aren't
 * zeroed.
 *
 * Data in val_surf is assumed to be indexed by element id.
 */
void
surf_to_nodal( float *val_surf, float *val_nodal, MO_class_data *p_surf_class,
               int surf_qty, int *surf_ids, Analysis *analy,
               Bool_type merge )
{
    Mesh_data *p_mesh;
    MO_class_data *p_node_geom;
    float *activity, *mm_val;
    int *adj_cnt;
    int i, j, nd;
    int *el_id;
    char **classes;
    int *sclasses;
    int (*connects)[4];
    int facet_qty;
    int *mats;
    int surf_id;

    double *val_nodal_dbl=NULL, val_dbl=0.0;

    /*
     * NOTE:  Original coding as per "quad_to_nodal"
     */

    p_mesh = MESH_P( analy );

    p_node_geom = p_mesh->node_geom;
    connects = (int (*) [4]) p_surf_class->objects.elems->nodes;
    facet_qty= p_surf_class->objects.surfaces->facet_qty;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_surf_class->elem_class_index]
               : NULL;


    adj_cnt       = NEW_N( int,    p_node_geom->qty, "Tmp surf result cnts" );
    val_nodal_dbl = NEW_N( double, p_node_geom->qty, "Tmp surf double result values" );

    if ( merge )
    {
        /* Zero selectively. */
        for ( i = 0; i < facet_qty; i++ )
        {
            /* Get the element identifier. */
            surf_id = surf_ids ? surf_ids[i] : i;

            for ( j = 0; j < 4; j++ )
            {
                val_nodal[connects[surf_id][j]] = 0.0;
            }
        }
    }

    for ( i = 0;
            i < p_node_geom->qty;
            i++ )
        val_nodal_dbl[i] = val_nodal[i];

    /* Prepare to extract element min/max (values init'd in load_result()). */
    mm_val = analy->tmp_elem_mm.object_minmax;
    classes = analy->tmp_elem_mm.class_long_name;
    sclasses = analy->tmp_elem_mm.sclass;
    el_id = analy->tmp_elem_mm.object_id;

    /*
     * Each node result is an average of the neighboring
     * element results.  Extract element min/max.
     */
    for ( i = 0; i < facet_qty; i++ )
    {
        /* Get the element identifier. */
        surf_id = surf_ids ? surf_ids[i] : i;

        /* Ignore inactive elements. */
        if ( activity && activity[surf_id] == 0.0 )
            continue;

        /* Test for new min or max. */
        if ( val_surf[surf_id] < mm_val[0] )
        {
            mm_val[0] = val_surf[surf_id];
            el_id[0] = surf_id + 1;
            classes[0] = p_surf_class->long_name;
        }

        if ( val_surf[surf_id] > mm_val[1] )
        {
            mm_val[1] = val_surf[surf_id];
            el_id[1] = surf_id + 1;
            classes[1] = p_surf_class->long_name;
        }

        for ( j = 0; j < 4; j++ )
        {
            nd = connects[surf_id][j];
            val_nodal_dbl[nd] += (double) val_surf[surf_id];
            adj_cnt[nd]++;
        }
    }

    for ( i = 0; i < p_node_geom->qty; i++ )
        if ( adj_cnt[i] != 0 )
        {
            val_dbl      = (double) val_nodal_dbl[i]/adj_cnt[i];
            val_nodal[i] = (float) val_dbl;
        }

    free( adj_cnt );
    free( val_nodal_dbl );
}


/*****************************************************************
 * TAG( tri_to_nodal )
 *
 * Convert triangle element result data to nodal data.  If the merge
 * flag is TRUE, nodes that don't belong to tri elements aren't
 * zeroed.
 *
 * Data in val_tri is assumed to be indexed by element id.
 */
void
tri_to_nodal( float *val_tri, float *val_nodal, MO_class_data *p_tri_class,
              int tri_qty, int *tri_ids, Analysis *analy, Bool_type merge )
{
    Mesh_data *p_mesh;
    MO_class_data *p_node_geom;
    float *activity, *mm_val;
    int *adj_cnt;
    int i, j, nd;
    int *el_id;
    char **classes;
    int *sclasses;
    int (*connects)[3];
    int *mats;
    int tri_id=0;
    Bool_type *nodes_updated;
    float *val_nodal_input;

    double *val_nodal_dbl=NULL, val_dbl=0.0;

    /*
     * NOTE: Original coding as per "hex_to_nodal"
     */

    p_mesh = MESH_P( analy );

    p_node_geom = p_mesh->node_geom;
    connects = (int (*) [3]) p_tri_class->objects.elems->nodes;
    mats = p_tri_class->objects.elems->mat;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_tri_class->elem_class_index]
               : NULL;

    adj_cnt       = NEW_N( int, p_node_geom->qty, "Tmp tri result cnts" );
    val_nodal_dbl = NEW_N( double, p_node_geom->qty, "Tmp tri double result values" );
    val_nodal_input = NEW_N( float, p_node_geom->qty, "Input val_nodal" );
    nodes_updated   = NEW_N( Bool_type, p_node_geom->qty, "List of updated nodes" );

    for ( i=0;
            i<p_node_geom->qty;
            i++ )
    {
        val_nodal_input[i] = val_nodal[i];
        nodes_updated[i]   = FALSE;
    }

    if ( merge )
    {
        /* Zero selectively. */
        for ( i = 0;
                i < tri_qty;
                i++ )
        {
            /* Get the element identifier. */
            tri_id = tri_ids ? tri_ids[i] : i;

            if ( (activity && activity[tri_id] == 0.0) ||
                    p_mesh->disable_material[ mats[tri_id] ] )
                continue;

            for ( j = 0; j < 3; j++ )
            {
                val_nodal[connects[tri_id][j]] = 0.0;
            }
        }
    }

    for ( i = 0;
            i < tri_qty;
            i++ )
    {
        /* Get the element identifier. */
        tri_id = tri_ids ? tri_ids[i] : i;

        if ( (activity && activity[tri_id] == 0.0) ||
                p_mesh->disable_material[ mats[tri_id] ] )
            continue;

        for ( j = 0;
                j < 3;
                j++ )
        {
            nd = connects[tri_id][j];
            nodes_updated[nd] = TRUE;
        }
    }

    for ( i = 0;
            i < p_node_geom->qty;
            i++ )
        val_nodal_dbl[i] = val_nodal[i];

    /* Prepare to extract element min/max (values init'd in load_result()). */
    mm_val = analy->tmp_elem_mm.object_minmax;
    classes = analy->tmp_elem_mm.class_long_name;
    sclasses = analy->tmp_elem_mm.sclass;
    el_id = analy->tmp_elem_mm.object_id;

    /*
     * Each node result is an average of the neighboring
     * element results.  Extract element min/max.
     */
    for ( i = 0; i < tri_qty; i++ )
    {
        /* Get the element identifier. */
        tri_id = tri_ids ? tri_ids[i] : i;

        /* Ignore inactive elements. */
        if ( activity && activity[tri_id] == 0.0 )
            continue;

        /* Ignore disabled materials. */
        if ( p_mesh->disable_material[ mats[tri_id] ] )
            continue;

        /* Test for new min or max. */
        if ( val_tri[tri_id] < mm_val[0] )
        {
            mm_val[0]   = val_tri[tri_id];
            el_id[0]    = tri_id + 1;
            classes[0]  = p_tri_class->long_name;
            sclasses[0] = p_tri_class->superclass;
        }

        if ( val_tri[tri_id] > mm_val[1] )
        {
            mm_val[1]   = val_tri[tri_id];
            el_id[1]    = tri_id + 1;
            classes[1]  = p_tri_class->long_name;
            sclasses[1] = p_tri_class->superclass;
        }

        for ( j = 0; j < 3; j++ )
        {
            nd = connects[tri_id][j];
            val_nodal_dbl[nd] += (double) val_tri[tri_id];
            adj_cnt[nd]++;
        }
    }

    for ( i = 0; i < p_node_geom->qty; i++ )
        if ( adj_cnt[i] != 0 )
        {
            val_dbl      = (double) val_nodal_dbl[i]/adj_cnt[i];
            val_nodal[i] = (float) val_dbl;
        }

    /* Average out nodes that were already computed with other subrecords
     * or element sets.
     */
    for ( i=0;
            i<p_node_geom->qty;
            i++ )
    {
        if ( nodes_updated[i] && val_nodal_input[i]!=0.0 )
        {
            val_nodal[i] = (val_nodal[i]+val_nodal_input[i])/2.0;
        }
    }

    free( adj_cnt );
    free( val_nodal_dbl );
    free( nodes_updated );
    free( val_nodal_input );
}


/*****************************************************************
 * TAG( beam_to_nodal )
 *
 * Convert beam element result data to nodal data.
 *
 * Data in val_beam is assumed to be indexed by element id.
 */
void
beam_to_nodal( float *val_beam, float *val_nodal, MO_class_data *p_beam_class,
               int beam_qty, int *beam_ids, Analysis *analy )
{
    Mesh_data *p_mesh;
    MO_class_data *p_node_geom;
    float *activity, *mm_val;
    int *adj_cnt;
    int i, nd;
    int *el_id;
    char **classes;
    int *sclasses;
    int (*connects)[3];
    int *mats;
    int beam_id;

    double *val_nodal_dbl=NULL, val_dbl=0.0;

    /*
     * NOTE:  Original coding as per "hex_to_nodal"
     */

    p_mesh = MESH_P( analy );

    p_node_geom = p_mesh->node_geom;
    connects = (int (*) [3]) p_beam_class->objects.elems->nodes;
    mats = p_beam_class->objects.elems->mat;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_beam_class->elem_class_index]
               : NULL;

    adj_cnt       = NEW_N( int, p_node_geom->qty, "Tmp beam result cnts" );
    val_nodal_dbl = NEW_N( double, p_node_geom->qty, "Tmp beam double result values" );

    /* Prepare to extract element min/max (values init'd in load_result()). */
    mm_val = analy->tmp_elem_mm.object_minmax;
    classes = analy->tmp_elem_mm.class_long_name;
    sclasses = analy->tmp_elem_mm.sclass;
    el_id = analy->tmp_elem_mm.object_id;

    for ( i = 0;
            i < p_node_geom->qty;
            i++ )
        val_nodal_dbl[i] = val_nodal[i];

    /*
     * Each node result is an average of the neighboring
     * element results.
     */
    for ( i = 0; i < beam_qty; i++ )
    {
        /* Get the element identifier. */
        beam_id = beam_ids ? beam_ids[i] : i;

        /* Ignore inactive elements. */
        if ( activity && activity[beam_id] == 0.0 )
            continue;

        /* Ignore disabled materials. */
        if ( p_mesh->disable_material[ mats[beam_id] ] ||
                disable_by_object_type( p_beam_class, mats[beam_id], beam_id, analy, NULL ) )
            continue;

        /* Test for new min or max. */
        if ( val_beam[beam_id] < mm_val[0] )
        {
            mm_val[0]   = val_beam[beam_id];
            el_id[0]    = beam_id + 1;
            classes[0]  = p_beam_class->long_name;
            sclasses[0] = p_beam_class->superclass;
        }

        if ( val_beam[beam_id] > mm_val[1] )
        {
            mm_val[1]   = val_beam[beam_id];
            el_id[1]    = beam_id + 1;
            classes[1]  = p_beam_class->long_name;
            sclasses[1] = p_beam_class->superclass;
        }

        /* Sum the result at each node. */
        nd = connects[beam_id][0];
        val_nodal[nd] += val_beam[beam_id];
        adj_cnt[nd]++;

        nd = connects[beam_id][1];
        val_nodal_dbl[nd] += (double) val_beam[beam_id];
        adj_cnt[nd]++;
    }

    /* Average the result at each node. */
    for ( i = 0; i < p_node_geom->qty; i++ )
        if ( adj_cnt[i] != 0 )
        {
            val_dbl      = (double) val_nodal_dbl[i]/adj_cnt[i];
            val_nodal[i] = (float) val_dbl;
        }

    free( adj_cnt );
    free( val_nodal_dbl );
}


/*****************************************************************
 * TAG( truss_to_nodal )
 *
 * Convert truss element result data to nodal data.
 *
 * Data in val_truss is assumed to be indexed by element id.
 */
void
truss_to_nodal( float *val_truss, float *val_nodal,
                MO_class_data *p_truss_class, int truss_qty, int *truss_ids,
                Analysis *analy )
{
    Mesh_data *p_mesh;
    MO_class_data *p_node_geom;
    float *activity, *mm_val;
    int *adj_cnt;
    int i, nd;
    int *el_id;
    char **classes;
    int *sclasses;
    int (*connects)[2];
    int *mats;
    int truss_id;

    double *val_nodal_dbl=NULL, val_dbl=0.0;
    Bool_type *nodes_updated;
    float *val_nodal_input;

    /*
     * NOTE:  Original coding as per "hex_to_nodal"
     */

    p_mesh = MESH_P( analy );

    p_node_geom = p_mesh->node_geom;
    connects = (int (*) [2]) p_truss_class->objects.elems->nodes;
    mats = p_truss_class->objects.elems->mat;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_truss_class->elem_class_index]
               : NULL;

    adj_cnt       = NEW_N( int, p_node_geom->qty, "Tmp truss result cnts" );
    val_nodal_dbl = NEW_N( double, p_node_geom->qty, "Tmp truss double result values" );

    /* Prepare to extract element min/max (values init'd in load_result()). */
    mm_val = analy->tmp_elem_mm.object_minmax;
    classes = analy->tmp_elem_mm.class_long_name;
    sclasses = analy->tmp_elem_mm.sclass;
    el_id = analy->tmp_elem_mm.object_id;
    val_nodal_input = NEW_N( float, p_node_geom->qty, "Input val_nodal" );
    nodes_updated   = NEW_N( Bool_type, p_node_geom->qty, "List of updated nodes" );

    for ( i=0;
            i<p_node_geom->qty;
            i++ )
    {
        val_nodal_input[i] = val_nodal[i];
        nodes_updated[i]   = FALSE;
    }

    for ( i = 0;
            i < truss_qty;
            i++ )
    {
        truss_id = truss_ids ? truss_ids[i] : i;
        nd = connects[truss_id][0];
        if ( val_nodal_input[nd]!=0.0 )
            val_nodal[nd] = 0.0;
    }

    for ( i = 0;
            i < p_node_geom->qty;
            i++ )
        val_nodal_dbl[i] = val_nodal[i];

    /*
     * Each node result is an average of the neighboring
     * element results.
     */
    for ( i = 0;
            i < truss_qty;
            i++ )
    {
        /* Get the element identifier. */
        truss_id = truss_ids ? truss_ids[i] : i;

        /* Ignore inactive elements. */
        if ( activity && activity[truss_id] == 0.0 )
            continue;

        /* Ignore disabled materials. */
        if ( p_mesh->disable_material[ mats[truss_id] ] ||
                disable_by_object_type( p_truss_class, mats[truss_id], truss_id, analy, NULL ) )
            continue;

        /* Test for new min or max. */
        if ( val_truss[truss_id] < mm_val[0] )
        {
            mm_val[0]   = val_truss[truss_id];
            el_id[0]    = truss_id + 1;
            classes[0]  = p_truss_class->long_name;
            sclasses[0] = p_truss_class->superclass;
        }

        if ( val_truss[truss_id] > mm_val[1] )
        {
            mm_val[1]   = val_truss[truss_id];
            el_id[1]    = truss_id + 1;
            classes[1]  = p_truss_class->long_name;
            sclasses[1] = p_truss_class->superclass;
        }

        /* Sum the result at each node. */
        nd = connects[truss_id][0];

        val_nodal[nd] += val_truss[truss_id];
        adj_cnt[nd]++;

        nd = connects[truss_id][1];
        val_nodal_dbl[nd] += (double) val_truss[truss_id];
        adj_cnt[nd]++;
    }

    /* Average the result at each node. */
    for ( i = 0;
            i < p_node_geom->qty;
            i++ )
        if ( adj_cnt[i] != 0 )
        {
            val_dbl      = (double) val_nodal_dbl[i]/adj_cnt[i];
            val_nodal[i] = (float) val_dbl;
        }

    /* Average out nodes that were already computed with other subrecords
     * or element sets.
     */
    for ( i=0;
            i<p_node_geom->qty;
            i++ )
    {
        if ( nodes_updated[i] &&  val_nodal_input[i]!=0.0 )
        {
            val_nodal[i] = (val_nodal[i]+val_nodal_input[i])/2.0;
        }
    }

    free( adj_cnt );
    free( val_nodal_dbl );
    free( nodes_updated );
    free( val_nodal_input );
}


/*****************************************************************
 * TAG( init_mm_obj )
 *
 * Initialize an Minmax_obj.
 */
void
init_mm_obj( Minmax_obj *p_mmo )
{
    static Minmax_obj mm_init =
    {
        NULL,
        NULL,
        { { '\0' }, INFINITESIMAL, MIDDLE, GLOBAL, 0 },
#ifndef AIX
        { MAXFLOAT, -MAXFLOAT },
        { MAXFLOAT, -MAXFLOAT },
#else
        {
            ((float)3.40282347e+38),
            ((float)-3.40282347e+38)
        },
        {
            ((float)3.40282347e+38),
            ((float)-3.40282347e+38)
        },
#endif
        { -1, -1 },
        { NULL, NULL }
    };

    *p_mmo = mm_init;
}


/*****************************************************************
 * TAG( load_result )
 *
 * Load the result variable for display.  The update parameter
 * should be TRUE if result min/max is to be updated, FALSE if
 * not.
 */
void
load_result( Analysis *analy, Bool_type update, Bool_type interpolate, Bool_type extreme_result )
{
    Result *p_r;
    float *data_buffer;
    int qty;
    int i, j;
    Bool_type first;
    int idcnt=0;
    int num_nodes=0;

    int        particle_count, *particle_nodes;
    int        subrec=0, srec=0;
    Subrec_obj *p_subrec;

    if ( extreme_result==FALSE && analy->extreme_result == TRUE )
        return;

    p_r = analy->cur_result;

    if ( p_r != NULL )
    {
        update_result( analy, analy->cur_result );

        if ( env.result_timing && !env.timing )
        {
            wrt_text( "\nTiming for result computation...\n" );
            check_timing( 0 );
        }

        /* Initialize temporary element min/max values. */
        init_mm_obj( &analy->tmp_elem_mm );

        /* Clear the result array. */
        data_buffer = NODAL_RESULT_BUFFER( analy );
        memset( data_buffer, 0,
                MESH( analy ).node_geom->qty * sizeof( float ) );

        /* Clear the applicable object class arrays. */
        prep_object_class_buffers( analy, analy->cur_result );

        /*
         * Loop through supporting references for the current result
         * looking for superclasses in the prescribed order for
         * result derivation.
         */

        first = TRUE;
        qty = sizeof( derive_order ) / sizeof( derive_order[0] );

        analy->pn_nodal_result  = FALSE;
        analy->pn_ref_nodes[0]  = NULL;
        analy->pn_buffer_ptr[0] = NULL;

        for ( i = 0; i < qty; i++ )
        {
            for ( j = 0;
                    j < p_r->qty;
                    j++ )
            {
                /**/
                /* Set the subrecord references in the Derived_result in the appropriate
                   derivation order and avoid the outer (i) loop and this test.
                */
                if ( p_r->superclasses[j] == derive_order[i] )
                {
                    analy->result_index = j;

                    subrec   = p_r->subrecs[j];
                    srec     = p_r->srec_id;
                    p_subrec = analy->srec_tree[srec].subrecs + subrec;

                    p_r->result_funcs[j]( analy, data_buffer, interpolate );

                    /* Get a pointer to the result buffer for the Particle object */
                    if ( analy->particle_nodes_enabled )
                    {
                        subrec   = p_r->subrecs[analy->result_index];
                        srec     = p_r->srec_id;
                        p_subrec = analy->srec_tree[srec].subrecs + subrec;

                        if ( is_particle_class( analy, p_subrec->p_object_class->superclass, p_subrec->p_object_class->short_name ) )
                        {
                            analy->pn_nodal_result = TRUE;
                            analy->pn_node_ptr[0]  = data_buffer;

                            analy->pn_ref_nodes[0]      = p_subrec->referenced_nodes;
                            analy->pn_ref_node_count[0] = p_subrec->ref_node_qty;
                        }
                    }

                    if ( update )
                    {
                        if ( first )
                        {
                            init_nodal_min_max( analy );
                            first = FALSE;
                        }
                        update_nodal_min_max( analy );
                    }
                } /* if ( p_r->superclasses[j] == derive_order[i] ) */
            } /* for j */
        }

        if ( env.result_timing && !env.timing )
            check_timing( 1 );

        if ( update )
            update_min_max( analy );

        if (analy->previous_state > analy->cur_state ||
                analy->min_state == analy->cur_state)
            analy->reset_damage_hide = TRUE;
        analy->previous_state    = analy->cur_state;
    }
}


/*****************************************************************
 * TAG( load_subrecord_result )
 *
 * Load the result variable from the specified subrecord for
 * display.  The update parameter should be TRUE if result min/max
 * is to be updated, FALSE if not.  Unlike load_result(), does
 * NOT clear the destination array prior to generating the result.
 *
 * Returns FALSE if the result is non-NULL but isn't supported on the
 * specified subrecord, otherwise TRUE. (Returning TRUE for a NULL
 * result was useful for the application that initially required
 * this function, but this may need to be revisited.)
 */
Bool_type
load_subrecord_result( Analysis *analy, int subrec_id, Bool_type update,
                       Bool_type interpolate )
{
    Result *p_r;
    int qty;
    int *subrecs;
    int j;

    p_r = analy->cur_result;

    if ( p_r != NULL )
    {
        qty = p_r->qty;
        subrecs = p_r->subrecs;

        /* Initialize temporary element min/max values. */
        init_mm_obj( &analy->tmp_elem_mm );

        for ( j = 0; j < qty; j++ )
        {
            if ( subrecs[j] == subrec_id )
            {
                analy->result_index = j;

                p_r->result_funcs[j]( analy, NODAL_RESULT_BUFFER( analy ),
                                      interpolate );

                if ( update )
                {
                    init_nodal_min_max( analy );
                    update_nodal_min_max( analy );
                    /**/
                    /*
                     * if update true but interpolate false, add call here
                     * to get state elem min/max since ...to_nodal functions
                     * will not have been called?
                     */
                }

                break;
            }
        }

        if ( j == qty )
            return FALSE;

        if ( update )
            update_min_max( analy );
    }

    return TRUE;
}


/************************************************************
 * TAG( get_free_nodes )
 *
 * This function will return a list of free nodes.
 */

int *get_free_nodes( Analysis *analy )
{
    MO_class_data *p_element_class,
                  *p_node_class,
                  *p_mo_class,
                  **mo_classes;

    Mesh_data     *p_mesh;

    List_head     *p_lh;

    int  *free_nodes_list;
    int   nd;

    float *activity, active_element=0.;
    float *data_array;
    float verts[3], leng[3];
    float **sand_arrays;

    int  elem_qty;
    int  node_index, node_num, num_nodes, node_qty, num_fn=0;
    int  class_index, class_qty;
    int  conn_qty;

    int  i, j, k, l;
    int  dim, obj_qty;

    int  *connects;

    /* Variables related to materials */
    unsigned char *disable_mtl;
    int  *mat, mat_num;

    p_mesh      = MESH_P( analy );
    disable_mtl = p_mesh->disable_material;

    data_array = NODAL_RESULT_BUFFER( analy );

    p_node_class = MESH_P( analy )->node_geom;
    num_nodes    = p_node_class->qty;

    /* Free node list is an array of node length used to identify
     * the free nodes - free_nodes_list[i] isset to 1 if node is free.
     */
    free_nodes_list = NEW_N( int , num_nodes, "free_nodes_list" );
    memset( free_nodes_list, 0, num_nodes );


    /* Loop over each element superclass. */
    for ( i = 0;
            i < QTY_SCLASS;
            i++ )
    {
        if ( !IS_ELEMENT_SCLASS( i ) )
            continue;

        p_lh = p_mesh->classes_by_sclass + i;

        /* If classes exist from current superclass... */
        if ( p_lh->qty > 0 )
        {
            mo_classes = (MO_class_data **)
                         p_mesh->classes_by_sclass[i].list;

            sand_arrays = analy->state_p->elem_class_sand;
            node_qty    = qty_connects[i];
            conn_qty    = ( i == G_BEAM ) ? node_qty - 1 : node_qty;

            /* Loop over each class. */
            for ( j = 0;
                    j < p_lh->qty;
                    j++ )
            {
                p_mo_class = mo_classes[j];
                connects   = p_mo_class->objects.elems->nodes;
                mat        = p_mo_class->objects.elems->mat;
                if ( sand_arrays )
                    activity = sand_arrays[p_mo_class->elem_class_index];
                else activity = NULL;


                /* Loop over each element */
                for ( k = 0;
                        k < p_mo_class->qty;
                        k++ )
                {
                    mat_num = mat[k];
                    if ( activity )
                        active_element = activity[k];
                    else active_element = 0.0;

                    if (!disable_mtl[mat_num] && active_element == 0.0)
                        for ( l = 0; l < conn_qty; l++ )
                        {
                            nd = connects[k * node_qty + l];
                            free_nodes_list[nd] = 1;
                            num_fn++;
                        }
                }

            }
        }
    }

    if (num_fn==0)
    {
        free(free_nodes_list);
        free_nodes_list = NULL;
    }

    return (free_nodes_list);
}

/************************************************************
 * TAG( find_min_max )
 *
 * This function will return the min and max values + the
 * indexes for a nodal result.
 */

void find_min_max( int num_nodes,  float *result, float *min, float *max,
                   int *min_index, int *max_index )
{
    int i;

    *min = MAXFLOAT;
    *max = -MAXFLOAT;

    for ( i=0;
            i<num_nodes;
            i++ )
    {
        if ( result[i] < *min )
        {
            *min = result[i];
            *min_index = i;
        }
        if ( result[i] > *max )
        {
            *max = result[i];
            *max_index = i;
        }
    }
}

/*****************************************************************
 * TAG( dump_result )
 *
 * This function will dump results data to a txt file named after
 * the result_name.
 */
void
dump_result( Analysis *analy, char *fname_input )
{
    FILE  *fp;
    int   i,j,k;
    char  fname[128], state_string[10];
    float *data_buffer=NULL, *data_result=NULL;
    int   *data_sclass=NULL, *data_mat=NULL;
    int   *data_ids=NULL;
    Bool_type *data_hide=NULL;
    int   result_index=0;
    int   class_qty=0;
    int   class_type;
    int   num_nodes=0;
    int   num_objects=1;
    int   superclass=0;
    MO_class_data *p_node_class, *p_class;
    Result *p_r;
    Subrec_obj *p_subrec;
    char *class_names[100];
    int  *class_name_index;
    int  class_index=0, class_label_index=0;
    Bool class_found=FALSE;

    int *mat=NULL;
    Mesh_data *p_mesh;
    unsigned char *hide_mat;

    p_mesh = MESH_P( analy );
    hide_mat    = p_mesh->hide_material;

    p_r = analy->cur_result;
    if ( !p_r )
        return;

    if ( fname_input )
        strcpy( fname, fname_input );
    else
    {
        strcpy( fname, "ResultData-" );
        strcat( fname, analy->cur_result->name );
        sprintf( state_string, "_State-%d", analy->cur_state+1 );
        strcat( fname, state_string );
    }
    fp = fopen(fname, "w" );
    fprintf( fp, "* Global-Id\tClass-Name\tClass-Id\tMaterial\tElemType\t\tValue\n" );
    /* Count number of result objects for array allocation */
    for ( i = 0;
            i < p_r->qty;
            i++ )
    {
        superclass = p_r->superclasses[i];
        class_qty = MESH( analy ).classes_by_sclass[superclass].qty;

        for ( j = 0;
                j < class_qty;
                j++ )
        {
            p_class = ((MO_class_data **)
                       MESH( analy ).classes_by_sclass[superclass].list)[j];
            num_objects+=p_class->qty;
        }
    }

    p_node_class = MESH_P( analy )->node_geom;
    num_nodes = p_node_class->qty;
    data_sclass = NEW_N( int , num_objects, "data_sclass" );
    memset( data_sclass, -1, num_objects );
    data_ids = NEW_N( int , num_objects, "data_ids" );
    memset( data_ids, -1, num_objects );
    data_hide = NEW_N( Bool_type, num_objects, "data_hide" );
    memset( data_hide, FALSE, num_objects );
    data_mat = NEW_N( int , num_objects, "data_mat" );
    memset( data_mat, 0, num_objects );
    data_result = NEW_N( float , num_objects, "data_result" );
    memset( data_result, 0.0, num_objects );
    class_name_index = NEW_N( int , num_objects, "class_name_index" );
    memset( class_name_index, -1, num_objects );

    for ( i = 0;
            i < p_r->qty;
            i++ )
    {
        superclass = p_r->superclasses[i];
        class_qty = MESH( analy ).classes_by_sclass[superclass].qty;

        for ( j = 0;
                j < class_qty;
                j++ )
        {
            p_class = ((MO_class_data **)
                       MESH( analy ).classes_by_sclass[superclass].list)[j];

            mat = p_class->objects.elems->mat;

            if (!result_has_class( analy->cur_result, p_class, analy ) )
                continue;

            /* Only process class one time - do not allow duplicates */
            class_found = FALSE;
            for ( k=0;
                    k<class_index;
                    k++ )
                if ( !strcmp( class_names[k], p_class->short_name ) )
                    class_found = TRUE;
            if ( class_found )
                continue;

            for ( k=0;
                    k<p_class->qty;
                    k++ )
            {

                if ( p_class->superclass!=M_NODE )
                {
                    data_mat[result_index] = mat[k]+1;
                    if ( hide_mat[mat[k]] )
                        data_hide[result_index] = TRUE;
                    else
                        data_hide[result_index] = FALSE;
                }
                else
                {
                    data_mat[result_index]  = -1;
                    data_hide[result_index] = FALSE;
                }

                data_sclass[result_index] = p_class->superclass;
                data_ids[result_index]    = k+ 1;
                class_label_index = get_class_label_index( p_class, k+1 );
                data_result[result_index] = p_class->data_buffer[class_label_index-1];
                class_name_index[result_index] = class_index;
                if ( data_sclass[i]>0 && data_hide[i]==FALSE )
                {
                    fprintf( fp, "\t%d\t\t%s\t\t%d\t\t%d\t%d\t\t%22.15e\n", p_class->labels[result_index].label_num, 
                    p_class->short_name, data_ids[result_index],
                    data_mat[result_index], data_sclass[result_index], data_result[result_index] );
                }
                result_index++;
            }
            class_names[class_index++] = strdup( p_class->short_name  );
        }
    }

    fclose( fp );

    free( data_sclass );
    free( data_ids );
    free( data_result );
    free( data_hide );
    free( data_mat );

    free( class_name_index );
    for ( i=0;
            i<class_index;
            i++ )
        free( class_names[i] );
}

