/* $Id$ */
/*
 * contour.c - Routines for contouring.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Jun 5 1992
 *
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
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - June 29, 2007: Initialized field contour_value to 0.
 *                Klocwork build #1 - Find (id) = 2
 ************************************************************************
 */

#include <stdlib.h>
#include <math.h>
#include "viewer.h"


#define CONTOUR_MAX_VERTS (3840)
#define THREE2RAD (0.0523598776)
#define MAX_CON_DL_FACTOR (128.0)

static void gen_hex_class_contours( Analysis *analy,
                                    MO_class_data *p_hex_class, float *c_vals );
static void gen_tet_class_contours( Analysis *analy,
                                    MO_class_data *p_tet_class, float *c_vals );
static void gen_quad_class_contours_3d( Analysis *analy,
                                        MO_class_data *p_quad_class,
                                        float *c_vals );
static void gen_quad_class_contours_2d( Analysis *analy,
                                        MO_class_data *p_quad_class,
                                        float *c_vals );
static void gen_tri_class_contours_3d( Analysis *analy,
                                       MO_class_data *p_tri_class,
                                       float *c_vals );
static void gen_tri_class_contours_2d( Analysis *analy,
                                       MO_class_data *p_tri_class,
                                       float *c_vals );
static Contour_obj *contour_poly( int, float *, float [4][3], float [4] );
static Contour_obj *contour_poly_level( float, float [4][3], float [4] );
static void trace_contour( Contour_obj *, float, float [4][3], float [4],
                           float *, float );


/************************************************************
 * TAG( set_contour_vals )
 *
 * Create a list of num evenly-distributed contour values.
 * Contour levels are stored as percentages.
 */
void
set_contour_vals( int num_contours, Analysis *analy )
{
    int i;

    if ( num_contours < 1 )
        return;

    if ( analy->contour_cnt > 0 )
        free( analy->contour_vals );

    analy->contour_vals = NEW_N( float, num_contours, "Contour levels" );
    for ( i = 0; i < num_contours; i++ )
        analy->contour_vals[i] = (i+1)/(float)(num_contours+1);
    analy->contour_cnt = num_contours;
}


/************************************************************
 * TAG( add_contour_val )
 *
 * Add a contour level to the list of contour vals (may be empty).
 * Contour levels are stored as percentages.
 */
void
add_contour_val( float val, Analysis *analy )
{
    float *c_vals;
    int i;

    if ( val < 0.0 || val > 1.0 )
        return;

    /*
     * Should use realloc, but this works.
     * The contour values are not stored in any order.
     */
    c_vals = NEW_N( float, analy->contour_cnt + 1, "Contour levels" );
    for ( i = 0; i < analy->contour_cnt; i++ )
        c_vals[i] = analy->contour_vals[i];
    c_vals[analy->contour_cnt] = val;
    if ( analy->contour_cnt > 0 )
        free( analy->contour_vals );
    analy->contour_vals = c_vals;
    analy->contour_cnt++;
}


/************************************************************
 * TAG( report_contour_vals )
 *
 * Write a report of the current contour values and their
 * associated result values.
 */
void
report_contour_vals( Analysis *analy )
{
    int i, cnt;
    float *c_vals, r_val, tmp, diff, base;
    Bool_type show_result_val, sorting;

    wrt_text( "Contour/Isosurface Values:\n" );

    if ( analy->contour_cnt < 1 )
    {
        wrt_text( "    (none)\n\n" );
        return;
    }
    else if ( analy->result_mm[0] >= analy->result_mm[1]
              || analy->cur_result == NULL )
        show_result_val = FALSE;
    else
    {
        show_result_val = TRUE;
        wrt_text( "    Current result: %s\n", analy->cur_result->title );
    }

    cnt = analy->contour_cnt;

    /*
     * Create a local copy of the contour values and sort them
     * in ascending order for user-friendly display.
     */
    c_vals = NEW_N( float, cnt, "Contour levels" );
    memcpy( c_vals, analy->contour_vals, cnt * sizeof( float ) );

    sorting = TRUE;
    while ( sorting )
    {
        sorting = FALSE;

        for ( i = 1; i < cnt; i++ )
        {
            if ( c_vals[i - 1] > c_vals[i] )
            {
                SWAP( tmp, c_vals[i - 1], c_vals[i] );
                sorting = TRUE;
            }
        }
    }

    /* Write column titles. */
    wrt_text( "        Percent" );
    if ( show_result_val )
        wrt_text( "     Result Value\n" );
    else
        wrt_text( "\n" );

    diff = analy->result_mm[1] - analy->result_mm[0];
    base = analy->result_mm[0];

    /* Write each contour percentage and result value. */
    for ( i = 0; i < cnt; i++ )
    {
        wrt_text( "    %2d.  %5.1f", i + 1, 100.0 * c_vals[i] );
        if ( show_result_val )
        {
            /* Convert from percentages to actual contour values. */
            r_val = diff * c_vals[i] + base;

            /* Apply units conversion if applicable. */
            if ( analy->perform_unit_conversion )
                r_val = (analy->conversion_scale * r_val)
                        + analy->conversion_offset;

            wrt_text( "       %10.3e\n", r_val );
        }
        else
            wrt_text( "\n" );
    }
    wrt_text( "\n" );

    free( c_vals );
}


/************************************************************
 * TAG( free_contours )
 *
 * Free the list of contour objects.
 */
void
free_contours( Analysis *analy )
{
    Contour_obj *cont_list, *cont;

    cont_list = analy->contours;
    while ( cont_list != NULL )
    {
        cont = cont_list;
        cont_list = cont_list->next;
        if ( cont->pts != NULL )
            free( cont->pts );
        free( cont );
    }
    analy->contours = NULL;
}


/************************************************************
 * TAG( gen_contours )
 *
 * Generate contours for display on the mesh.
 */
void
gen_contours( Analysis *analy )
{
    float diff, base;
    float *c_vals;
    int i, j, k, c_qty;
    Mesh_data *p_mesh;
    Result *p_res;
    MO_class_data *p_mo_class;
    MO_class_data **p_mo_classes;
    int srec_id, sub, sclass;
    void (**con_funcs)();
    void (*con_funcs_3d[])( Analysis *, MO_class_data *, float * ) =
    {
        NULL,       /* G_UNIT */
        NULL,       /* G_NODE */
        NULL,       /* G_TRUSS */
        NULL,       /* G_BEAM */
        gen_tri_class_contours_3d,
        gen_quad_class_contours_3d,
        gen_tet_class_contours,
        NULL,       /* G_PYRAMID */
        NULL,       /* G_WEDGE */
        gen_hex_class_contours,
        NULL,       /* G_MAT */
        NULL,       /* G_MESH */
        NULL,       /* G_SURFACE */
        NULL,       /* G_PARTICLE */
    };
    void (*con_funcs_2d[])( Analysis *, MO_class_data *, float * ) =
    {
        NULL,       /* G_UNIT */
        NULL,       /* G_NODE */
        NULL,       /* G_TRUSS */
        NULL,       /* G_BEAM */
        gen_tri_class_contours_2d,
        gen_quad_class_contours_2d,
        NULL,       /* G_TET */
        NULL,       /* G_PYRAMID */
        NULL,       /* G_WEDGE */
        NULL,       /* G_HEX */
        NULL,       /* G_MAT */
        NULL,       /* G_MESH */
        NULL,       /* G_SURFACE */
        NULL,       /* G_PARTICLE */
    };

    free_contours( analy );

    if ( !analy->show_contours
            || analy->contour_cnt < 1
            || analy->result_mm[0] >= analy->result_mm[1]
            || ( analy->cur_result->origin.is_elem_result != 1
                 && analy->cur_result->origin.is_node_result != 1 ) )
        return;

    check_interp_mode( analy );

    c_vals = NEW_N( float, analy->contour_cnt, "Temp contour values" );

    /* Convert from percentages to actual contour values. */
    diff = analy->result_mm[1] - analy->result_mm[0];
    base = analy->result_mm[0];
    for ( i = 0; i < analy->contour_cnt; i++ )
        c_vals[i] = diff*analy->contour_vals[i] + base;

    p_mesh = MESH_P( analy );
    p_res = analy->cur_result;
    srec_id = p_res->srec_id;

    /* Assign function pointers array by mesh dimensionality. */
    con_funcs = ( analy->dimension == 3 ) ? con_funcs_3d : con_funcs_2d;

    /* Process all supporting subrecords. */
    for ( i = 0; i < p_res->qty; i++ )
    {
        sub = p_res->subrecs[i];
        p_mo_class = analy->srec_tree[srec_id].subrecs[sub].p_object_class;
        sclass = p_res->superclasses[i];

        /* If subrecord contains element data... */
        if ( sclass != G_NODE )
        {
            /* ...call the appropriate element superclass contour function. */
            if ( con_funcs[sclass] != NULL )
                con_funcs[sclass]( analy, p_mo_class, c_vals );
        }
        else
        {
            /*
             *  ...else for nodal data, call all planar or volume element
             * superclass contour functions where those superclasses exist.
             */
            for ( j = 0; j < QTY_SCLASS; j++ )
            {
                if ( con_funcs[j] != NULL )
                {
                    c_qty = p_mesh->classes_by_sclass[j].qty;
                    p_mo_classes = (MO_class_data **)
                                   p_mesh->classes_by_sclass[j].list;

                    for ( k = 0; k < c_qty; k++ )
                        con_funcs[j]( analy, p_mo_classes[k], c_vals );
                }
            }
        }
    }

    free( c_vals );
}


/************************************************************
 * TAG( gen_hex_class_contours )
 *
 * Generate contours for faces of hex elements.
 */
void
gen_hex_class_contours( Analysis *analy, MO_class_data *p_hex_class,
                        float *c_vals )
{
    GVec3D *nodes;
    int (*connects)[8];
    int i, j, k;
    int el, face, face_cnt, nd;
    int *face_el, *face_fc;
    Visibility_data *p_vd;
    float verts[4][3], vert_vals[4];
    float *nodal_data;
    Contour_obj *cont;

    p_vd = p_hex_class->p_vis_data;
    face_el = p_vd->face_el;
    face_fc = p_vd->face_fc;
    face_cnt = p_vd->face_cnt;
    nodes = analy->state_p->nodes.nodes3d;
    connects = (int (*)[8]) p_hex_class->objects.elems->nodes;
    nodal_data = NODAL_RESULT_BUFFER( analy );

    /* Loop through faces, generating the contours on each visible face. */
    for( i = 0; i < face_cnt; i++ )
    {
        el = face_el[i];
        face = face_fc[i];

        for( j = 0; j < 4; j++ )
        {
            nd = connects[el][ fc_nd_nums[face][j] ];
            for( k = 0; k < 3; k++ )
                verts[j][k] = nodes[nd][k];
            vert_vals[j] = nodal_data[nd];
        }

        cont = contour_poly( analy->contour_cnt, c_vals, verts, vert_vals );
        if ( cont != NULL )
            APPEND( cont, analy->contours );
    }
}


/************************************************************
 * TAG( gen_tet_class_contours )
 *
 * Generate contours for faces of tet elements.
 */
void
gen_tet_class_contours( Analysis *analy, MO_class_data *p_tet_class,
                        float *c_vals )
{
#ifdef HAVE_TRI_CONTOUR
    GVec3D *nodes;
    int (*connects)[4];
    int i, j, k;
    int el, face, face_cnt, nd;
    int *face_el, *face_fc;
    Visibility_data *p_vd;
    float verts[3][3], vert_vals[3];
    float *nodal_data;
    Contour_obj *cont;

    p_vd = p_tet_class->p_vis_data;
    face_el = p_vd->face_el;
    face_fc = p_vd->face_fc;
    face_cnt = p_vd->face_cnt;
    nodes = analy->state_p->nodes.nodes3d;
    connects = (int (*)[4]) p_tet_class->objects.elems->nodes;
    nodal_data = NODAL_RESULT_BUFFER( analy );

    /* Loop through faces, generating the contours on each visible face. */
    for( i = 0; i < face_cnt; i++ )
    {
        el = face_el[i];
        face = face_fc[i];

        for( j = 0; j < 3; j++ )
        {
            nd = connects[el][ tet_fc_nd_nums[face][j] ];
            for( k = 0; k < 3; k++ )
                verts[j][k] = nodes[nd][k];
            vert_vals[j] = nodal_data[nd];
        }
        /*** Need a flag to indicate tri poly vs. quad poly (see alg paper) ***/
        cont = contour_poly( analy->contour_cnt, c_vals, verts, vert_vals );
        if ( cont != NULL )
            APPEND( cont, analy->contours );
    }
#endif
#ifndef HAVE_TRI_CONTOUR
    popup_dialog( INFO_POPUP,
                  "Contouring on tetrahedral elements is not implemented." );
#endif
}


/************************************************************
 * TAG( gen_quad_class_contours_3d )
 *
 * Generate contours for faces of 3D quad elements.
 */
void
gen_quad_class_contours_3d( Analysis *analy, MO_class_data *p_quad_class,
                            float *c_vals )
{
    GVec3D *nodes;
    int (*connects)[4];
    int i, j, k;
    int quad_cnt, nd;
    float verts[4][3], vert_vals[4];
    float *nodal_data;
    Contour_obj *cont;
    float *activity;

    quad_cnt = p_quad_class->qty;
    nodes = analy->state_p->nodes.nodes3d;
    connects = (int (*)[4]) p_quad_class->objects.elems->nodes;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_quad_class->elem_class_index]
               : NULL;
    nodal_data = NODAL_RESULT_BUFFER( analy );

    /* Loop through quads, generating the contours. */
    for( i = 0; i < quad_cnt; i++ )
    {
        if ( activity && activity[i] == 0.0 )
            continue;

        for( j = 0; j < 4; j++ )
        {
            nd = connects[i][j];
            for( k = 0; k < 3; k++ )
                verts[j][k] = nodes[nd][k];
            vert_vals[j] = nodal_data[nd];
        }

        cont = contour_poly( analy->contour_cnt, c_vals, verts, vert_vals );
        if ( cont != NULL )
            APPEND( cont, analy->contours );
    }
}


/************************************************************
 * TAG( gen_quad_class_contours_2d )
 *
 * Generate contours for faces of 2D quad elements.
 */
void
gen_quad_class_contours_2d( Analysis *analy, MO_class_data *p_quad_class,
                            float *c_vals )
{
    GVec2D *nodes;
    int (*connects)[4];
    int i, j, k;
    int quad_cnt, nd;
    float verts[4][3], vert_vals[4]; /* contour_poly() takes 3rd dim */
    float *nodal_data;
    Contour_obj *cont;
    float *activity;

    quad_cnt = p_quad_class->qty;
    nodes = analy->state_p->nodes.nodes2d;
    connects = (int (*)[4]) p_quad_class->objects.elems->nodes;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_quad_class->elem_class_index]
               : NULL;
    nodal_data = NODAL_RESULT_BUFFER( analy );

    /* Loop through quads, generating the contours. */
    for( i = 0; i < quad_cnt; i++ )
    {
        if ( activity && activity[i] == 0.0 )
            continue;

        for( j = 0; j < 4; j++ )
        {
            nd = connects[i][j];
            for( k = 0; k < 2; k++ )
                verts[j][k] = nodes[nd][k];
            verts[j][2] = 0.0;
            vert_vals[j] = nodal_data[nd];
        }

        cont = contour_poly( analy->contour_cnt, c_vals, verts, vert_vals );
        if ( cont != NULL )
            APPEND( cont, analy->contours );
    }
}


/************************************************************
 * TAG( gen_tri_class_contours_3d )
 *
 * Generate contours for faces of 3D tri elements.
 */
void
gen_tri_class_contours_3d( Analysis *analy, MO_class_data *p_tri_class,
                           float *c_vals )
{
#ifdef HAVE_TRI_CONTOUR
    GVec3D *nodes;
    int (*connects)[3];
    int i, j, k;
    int tri_cnt;
    float verts[3][3], vert_vals[3];
    float *nodal_data;
    Contour_obj *cont;
    float *activity;

    tri_cnt = p_tri_class->qty;
    nodes = analy->state_p->nodes.nodes3d;
    connects = (int (*)[3]) p_tri_class->objects.elems->nodes;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_tri_class->elem_class_index]
               : NULL;
    nodal_data = NODAL_RESULT_BUFFER( analy );

    /* Loop through tris, generating the contours. */
    for( i = 0; i < tri_cnt; i++ )
    {
        if ( activity && activity[i] == 0.0 )
            continue;

        for( j = 0; j < 3; j++ )
        {
            nd = connects[i][j];
            for( k = 0; k < 3; k++ )
                verts[j][k] = nodes[nd][k];
            vert_vals[j] = nodal_data[nd];
        }

        /*** Need a flag to indicate tri poly vs. quad poly (see alg paper) ***/
        cont = contour_poly( analy->contour_cnt, c_vals, verts, vert_vals );
        if ( cont != NULL )
            APPEND( cont, analy->contours );
    }
#endif
#ifndef HAVE_TRI_CONTOUR
    popup_dialog( INFO_POPUP,
                  "Contouring on triangular elements is not implemented." );
#endif
}


/************************************************************
 * TAG( gen_tri_class_contours_2d )
 *
 * Generate contours for faces of 2D tri elements.
 */
void
gen_tri_class_contours_2d( Analysis *analy, MO_class_data *p_tri_class,
                           float *c_vals )
{
#ifdef HAVE_TRI_CONTOUR
    GVec2D *nodes;
    int (*connects)[3];
    int i, j, k;
    int tri_cnt;
    float verts[3][3], vert_vals[3]; /* contour_poly() takes 3rd dim */
    float *nodal_data;
    Contour_obj *cont;
    float *activity;

    tri_cnt = p_tri_class->qty;
    nodes = analy->state_p->nodes.nodes2d;
    connects = (int (*)[3]) p_tri_class->objects.elems->nodes;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_tri_class->elem_class_index]
               : NULL;
    nodal_data = NODAL_RESULT_BUFFER( analy );

    /* Loop through tris, generating the contours. */
    for( i = 0; i < tri_cnt; i++ )
    {
        if ( activity && activity[i] == 0.0 )
            continue;

        for( j = 0; j < 3; j++ )
        {
            nd = connects[i][j];
            for( k = 0; k < 2; k++ )
                verts[j][k] = nodes[nd][k];
            verts[j][2] = 0.0;
            vert_vals[j] = nodal_data[nd];
        }

        /*** Need a flag to indicate tri poly vs. quad poly (see alg paper) ***/
        cont = contour_poly( analy->contour_cnt, c_vals, verts, vert_vals );
        if ( cont != NULL )
            APPEND( cont, analy->contours );
    }
#endif
#ifndef HAVE_TRI_CONTOUR
    popup_dialog( INFO_POPUP,
                  "Contouring on triangular elements is not implemented." );
#endif
}


/************************************************************
 * TAG( contour_poly )
 *
 * Generate contours for multiple contour levels over a single
 * quadrilateral.  Returns a list of contour objects.
 */
static Contour_obj *
contour_poly( int num_contours, float *contour_vals, float verts[4][3],
              float vert_vals[4] )
{
    Contour_obj *cont, *poly_cont;
    int i;

    poly_cont = NULL;

    for ( i = 0; i < num_contours; i++ )
    {
        cont = contour_poly_level( contour_vals[i], verts, vert_vals );
        if ( cont != NULL )
            APPEND( cont, poly_cont );
    }

    return poly_cont;
}


/************************************************************
 * TAG( contour_poly_level )
 *
 * Generate a contour for one contour level over a single
 * quadrilateral.  Returns a list of contour objects.
 */
static Contour_obj *
contour_poly_level( float contour_v, float verts[4][3], float vert_v[4] )
{
    Contour_obj *cont, *cp2, *poly_cont, *start_cont;
    float eps, ratio;
    int i, ii, v1, v2, ns, side[4];
    float *next;
    float factor;
    Bool_type matched;

    /*
     * Handle the degenerate cases (sides at contour value).
     */
    for ( ns = 0, i = 0; i < 4; i++ )
        if ( vert_v[i] == contour_v && vert_v[(i+1)%4] == contour_v )
        {
            ns++;
            side[i] = 1;
        }
        else
            side[i] = 0;

    if ( ns == 1 )
    {
        cont = NEW( Contour_obj, "Contour object" );
        cont->contour_value = 0.0;

        if ( side[0] == 1 )
            v1 = 2;
        else if ( side[1] == 1 )
            v1 = 3;
        else if ( side[2] == 1 )
            v1 = 0;
        else
            v1 = 1;
        v2 = (v1+1)%4;

        if ( (vert_v[v1] < contour_v && vert_v[v2] < contour_v) ||
                (vert_v[v1] > contour_v && vert_v[v2] > contour_v) )
        {
            cont->pts = NEW_N( float, 2*3, "Contour points" );
            cont->cnt = 2;
            for ( i = 0; i < 3; i++ )
            {
                cont->pts[i] = verts[v1][i];
                cont->pts[3+i] = verts[v2][i];
            }
        }
        else
        {
            ratio = (vert_v[v2]-contour_v)/(vert_v[v2]-vert_v[v1]);
            cont->pts = NEW_N( float, 3*3, "Contour points" );
            cont->cnt = 3;
            for ( i = 0; i < 3; i++ )
            {
                cont->pts[i] = verts[(v2+1)%4][i];
                cont->pts[3+i] = ratio*verts[v1][i] + (1.0-ratio)*verts[v2][i];
                cont->pts[6+i] = verts[(v2+2)%4][i];
            }
        }
    }
    else if ( ns == 2 )
    {
        cont = NEW( Contour_obj, "Contour object" );
        cont->contour_value = 0.0;

        for ( i = 0; i < 4; i++ )
            if ( vert_v[i] != contour_v )
                break;
        v1 = (i+1)%4;
        v2 = (i+3)%4;
        cont->pts = NEW_N( float, 2*3, "Contour points" );
        cont->cnt = 2;
        for ( i = 0; i < 3; i++ )
        {
            cont->pts[i] = verts[v1][i];
            cont->pts[3+i] = verts[v2][i];
        }
    }
    else if ( ns == 3 || ns == 4 )
        return NULL;

    if ( ns > 0 )
    {
        cont->contour_value = contour_v;
        return cont;
    }

    /*
     * Now handle the non-degenerate cases.
     */

    /* Get contour intersections on each side. */
    start_cont = NULL;
    for ( ns = 0, i = 0; i < 4; i++ )
    {
        ii = (i+1)%4;

        if ( (contour_v >= vert_v[i] && contour_v < vert_v[ii]) ||
                (contour_v <= vert_v[i] && contour_v > vert_v[ii]) )
        {
            /* Add this start point to the start_list. */
            cont = NEW( Contour_obj, "Contour object" );
            cont->contour_value = 0.0;

            /* Get the starting point (r,s). */
            ratio = (contour_v-vert_v[i]) / (vert_v[ii]-vert_v[i]);
            switch ( i )
            {
            case 0:
                cont->start[0] = ratio*2.0 - 1.0;
                cont->start[1] = -1.0;
                break;
            case 1:
                cont->start[0] = 1.0;
                cont->start[1] = ratio*2.0 - 1.0;
                break;
            case 2:
                cont->start[0] = 1.0 - ratio*2.0;
                cont->start[1] = 1.0;
                break;
            case 3:
                cont->start[0] = -1.0;
                cont->start[1] = 1.0 - ratio*2.0;
                break;
            }
            cont->contour_value = contour_v;

#ifdef DEBUG
            if ( cont->start[0] < -1.0 || cont->start[0] > 1.0 ||
                    cont->start[1] < -1.0 || cont->start[1] > 1.0 )
                popup_dialog( WARNING_POPUP,
                              "Contouring -- (r,s) start point is out of bounds!" );
#endif

            APPEND( cont, start_cont );
            ns++;
        }
    }

    if ( ns == 0 )
        return NULL;

    /* Epsilon for matching contour endpoints (in r,s). */
    eps = 2.0 / 20;

    /*
     * Loop through the list, tracing the contours and tossing
     * redundant ones.
     */
    poly_cont = NULL;
    for ( cont = start_cont, factor = 1.0; cont != NULL; cont = start_cont )
    {
        /*
         * If two intersections found, assume they're the same contour
         * and pass the second as the end point for use in step-size
         * calculation.
         */
        next = ( ns == 2 && cont->next ) ? cont->next->start : NULL;

        trace_contour( cont, contour_v, verts, vert_v, next, factor );

        if ( cont->cnt < 2 )
        {
            if ( cont->pts )
                free( cont->pts );
            /* free( cont ); */
            DELETE( cont, start_cont );
        }
        else
        {
            matched = FALSE;

            /* Delete redundant contour if there is one. */
            for ( cp2 = cont->next; cp2 != NULL; cp2 = cp2->next )
            {
                if ( fabs((double)(cont->end[0] - cp2->start[0])) < eps &&
                        fabs((double)(cont->end[1] - cp2->start[1])) < eps )
                {
                    DELETE( cp2, start_cont );
                    matched = TRUE;
                }
            }

            if ( ns == 2 && !matched && factor <= MAX_CON_DL_FACTOR )
            {
                /*
                                wrt_text( "Contour end merge failed - decreasing step.\n" );
                */
                cont->cnt = 0;
                free( cont->pts );
                cont->pts = NULL;
                cont->start[0] = cont->start[1] = 0.0;
                cont->end[0] = cont->end[1] = 0.0;
                factor *= 2.0;
            }
            else
            {
                if ( factor > MAX_CON_DL_FACTOR )
                    wrt_text( "Minimum contour step-size reached.\n\n" );
                UNLINK( cont, start_cont );
                APPEND( cont, poly_cont );
            }
        }
    }

    return poly_cont;
}


/************************************************************
 * TAG( trace_contour )
 *
 * Trace out a contour on a quadrilateral face, beginning with
 * the specified starting point.
 *
 * cont       | The contour object in which contour is returned.
 *            | Should contain the starting point in (r,s).
 * contour_v  | Contour value
 * verts      | Quadrilateral vertices
 * vert_v     | Vertex values
 * next       | Assumed endpoint for contour on quadilateral face.
 *            | May be null.
 * factor     | A correction factor the calling program may use to
 *            | scale down the step size.
 *
 * Algorithm: See "An Improved Method for Contouring on Isoparametric
 *            Surfaces", W. H. Gray and J. E. Akin, International
 *            Journal for Numerical Methods in Engineering, Vol. 14,
 *            pp 451-472 (1979).
 */
static void
trace_contour( Contour_obj *cont, float contour_v, float verts[4][3],
               float vert_v[4], float *next, float factor )
{
    float c_verts[CONTOUR_MAX_VERTS][3];
    float r_old, s_old, r_new, s_new, val, sgn;
    float h[4], dhdr[4], dhds[4], dvdr, dvds, delta, dl;
    Bool_type quit;
    int i, j, cnt;
    float dirvec[2], linvec[2];
    float arclen, vlen, dotp, steps;
    double rot;

    /* Get starting point. */
    r_old = cont->start[0];
    s_old = cont->start[1];
    shape_fns_quad( r_old, s_old, h );
    for ( i = 0; i < 3; i++ )
        c_verts[0][i] = verts[0][i]*h[0] + verts[1][i]*h[1] +
                        verts[2][i]*h[2] + verts[3][i]*h[3];

    cnt = 1;
    quit = FALSE;

    /*
     * Loop to trace out the contour.
     */
    while ( cnt < CONTOUR_MAX_VERTS && !quit )
    {
        shape_derivs_2d( r_old, s_old, dhdr, dhds );
        dvdr = vert_v[0]*dhdr[0] + vert_v[1]*dhdr[1] +
               vert_v[2]*dhdr[2] + vert_v[3]*dhdr[3];
        dvds = vert_v[0]*dhds[0] + vert_v[1]*dhds[1] +
               vert_v[2]*dhds[2] + vert_v[3]*dhds[3];

        if ( cnt == 1 )
        {
            /*
             * Guesstimate the integration step-size necessary based on
             * the distance to the end point and the azimuth from the start
             * direction to the direction of the end point, if the end point
             * is available, else punt.
             *
             * Alternative idea - calc curvature at start point and set dl
             * directly based on that.
             */

            /* If an (assumed) endpoint exists... */
            if ( next != NULL )
            {
                dirvec[0] = dvds;
                dirvec[1] = -dvdr; /* Rotate the gradient 90 degrees. */
                vlen = VEC_LENGTH_2D( dirvec );
                dirvec[0] /= vlen;
                dirvec[1] /= vlen;

                VEC_SUB_2D( linvec, next, cont->start );
                vlen = VEC_LENGTH_2D( linvec );
                linvec[0] /= vlen;
                linvec[1] /= vlen;

                dotp = VEC_DOT_2D( linvec, dirvec );
                dotp = (float) fabs( (double) dotp );

                if ( dotp >= 0.99999 ) /* arccos(.99999) = 0.25 degrees */
                    /* Straight shot - one step (when factor is 1). */
                    dl = vlen * 1.0001 / factor;
                else
                {
                    /*
                     * Calculate number of steps as one plus
                     * one_per_3_degrees of azimuth (times factor).
                     */
                    rot = acos( dotp );
                    steps = (ceil( rot / THREE2RAD ) + 1.0) * factor;

                    /*
                     * Assume a circular arc to estimate dl (bad???).
                     * Angle subtended by arc is twice the azimuth angle.
                     */
                    arclen = (rot / sin( rot )) * vlen;
                    dl = arclen / steps;
                }
            }
            else
            {
                /*
                                wrt_text( "Unpaired contour intersection of element.\n" );
                */
                dl = 0.1 / factor;
            }

            /*
             * Make sure we are walking along the contour in the correct
             * direction.  Want to walk into the quadrilateral, not away
             * from it.
             */
            if ( s_old == -1.0 )
            {
                if ( dvdr > 0.0 )
                    dl = -dl;
            }
            else if ( r_old == 1.0 )
            {
                if ( dvds > 0.0 )
                    dl = -dl;
            }
            else if ( s_old == 1.0 )
            {
                if ( dvdr < 0.0 )
                    dl = -dl;
            }
            else if ( r_old == -1.0 )
            {
                if ( dvds < 0.0 )
                    dl = -dl;
            }
        }

        /* Get the new position (r,s). */
        delta = sqrt( (double)( dvdr*dvdr + dvds*dvds ) );
        r_new = r_old + dl*dvds/delta;
        s_new = s_old - dl*dvdr/delta;

        /* Apply the correction in the normal direction. */
        shape_fns_quad( r_new, s_new, h );
        val = vert_v[0]*h[0] + vert_v[1]*h[1] +
              vert_v[2]*h[2] + vert_v[3]*h[3];
        r_new = r_new - dvdr*(val - contour_v)/(delta*delta);
        s_new = s_new - dvds*(val - contour_v)/(delta*delta);

        /* Check for (r,s) outside the quadrilateral. */
        if ( r_new < -1.0 || r_new > 1.0 )
        {
            if ( r_new < -1.0 )
                sgn = -1.0;
            else
                sgn = 1.0;
            s_new = (s_new-s_old)*(sgn-r_old)/(r_new-r_old) + s_old;
            r_new = sgn;
            quit = TRUE;
        }
        if ( s_new < -1.0 || s_new > 1.0 )
        {
            if ( s_new < -1.0 )
                sgn = -1.0;
            else
                sgn = 1.0;
            r_new = (r_new-r_old)*(sgn-s_old)/(s_new-s_old) + r_old;
            s_new = sgn;
            quit = TRUE;
        }

        /* Check for staying in one place. */
        if ( APX_EQ( s_new, s_old ) && APX_EQ( r_new, r_old ) )
            break;

        /* Save the point. */
        shape_fns_quad( r_new, s_new, h );
        for ( i = 0; i < 3; i++ )
            c_verts[cnt][i] = verts[0][i]*h[0] + verts[1][i]*h[1] +
                              verts[2][i]*h[2] + verts[3][i]*h[3];

        cnt++;

        /* Prepare for next step. */
        r_old = r_new;
        s_old = s_new;
    }

    /* Store (r,s) for the last point. */
    cont->end[0] = r_old;
    cont->end[1] = s_old;

    /* Check for a degenerate contour. */
    if ( cnt < 2 || ( cnt == 2 && APX_EQ(cont->start[0],cont->end[0]) &&
                      APX_EQ(cont->start[1],cont->end[1]) ) )
    {
        cont->cnt = 0;
    }
    else
    {
        /* Allocate storage for the contour points. */
        cont->cnt = cnt;
        cont->pts = NEW_N( float, 3*cnt, "Contour points" );
        for ( i = 0; i < cnt; i++ )
            for ( j = 0; j < 3; j++ )
                cont->pts[i*3+j] = c_verts[i][j];
    }
}


