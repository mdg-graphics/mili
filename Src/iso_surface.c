/* $Id$ */
/*
 * iso_surface.c - Construct an isosurface for 3D unstructured cell data.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Mar 25 1992
 *
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - December 18, 2008: Added test for hidden mats in
 *                for cutting plane generation.
 *                See SRC#557
 *
 *  I. R. Corey - Dec 28, 2009: Fixed several problems releated to drawing
 *                ML particles.
 *                See SRC#648
 *
 ************************************************************************ */

/*
 * This code is derived from software authored by
 *
 *      Author           : Brian Cabral
 *      Date created     : November 17, 1988
 *      Algorithm        : see "Marching Cubes" 1987 SIGGRAPH proceedings
 *      Copyright        : Lawrence Livermore National Laboratory (c) 1988
 */

#include <stdlib.h>
#include "viewer.h"
#include "cell_cases.h"

static void hex_fine_cut( Analysis *analy, MO_class_data *p_elem_class,
                          float plane_pt[3], float plane_norm[3] );
/*
static void tet_fine_cut( Analysis *analy, MO_class_data *p_elem_class,
                          float plane_pt[3], float plane_norm[3] );
*/

void
particle_rough_cut( Analysis *analy, float *plane_ppt, float *norm, MO_class_data *p_particle_class,
                    GVec3D *nodes, unsigned char *particle_visib );
static void hex_isosurface( float isoval, Subrec_obj *p_so,
                            MO_class_data *p_hex_class, Analysis * analy );
/*****************************************************************
 * TAG( is_degen_triangle )
 *
 * Check whether a triangle polygon is degenerate (has repeated
 * vertices.)
 */
static Bool_type
is_degen_triangle( Triangle_poly *tri )
{
    int i;

    for ( i = 0; i < 3; i++ )
    {
        if ( APX_EQ( tri->vtx[i][0], tri->vtx[(i+1)%3][0] ) &&
                APX_EQ( tri->vtx[i][1], tri->vtx[(i+1)%3][1] ) &&
                APX_EQ( tri->vtx[i][2], tri->vtx[(i+1)%3][2] ) )
            return TRUE;
    }

    return FALSE;
}


/*****************************************************************
 * TAG( gen_isosurfs )
 *
 * Generate isosurface for a hex class result.
 */
void
gen_isosurfs( Analysis *analy )
{
    float iso_val, diff, base;
    int i, j, k, l, c_qty;
    Result *p_result;
    int subrec_id, sclass;
    Subrec_obj *p_so;
    Mesh_data *p_mesh;
    MO_class_data **p_mo_classes;
    void (*iso_funcs[])( float, Subrec_obj *, MO_class_data *, Analysis * ) =
    {
        NULL,       /* G_UNIT */
        NULL,       /* G_NODE */
        NULL,       /* G_TRUSS */
        NULL,       /* G_BEAM */
        NULL,       /* G_TRI */
        NULL,       /* G_QUAD */
        NULL,       /* tet_isosurface */
        NULL,       /* G_PYRAMID */
        NULL,       /* G_WEDGE */
        hex_isosurface,
        NULL,       /* G_MAT */
        NULL,       /* G_MESH */
        NULL,       /* G_SURFACE */
    };

    p_result = analy->cur_result;
    if ( p_result == NULL )
        return;

    DELETE_LIST( analy->isosurf_poly );

    if ( analy->show_isosurfs
            && analy->contour_cnt > 0
            && ( p_result->origin.is_node_result
                 || p_result->origin.is_elem_result ) )
    {
        check_interp_mode( analy );

        p_mesh = MESH_P( analy );

        /* Convert from percentages to actual contour values. */
        diff = analy->result_mm[1] - analy->result_mm[0];
        base = analy->result_mm[0];

        for ( i = 0; i < analy->contour_cnt; i++ )
        {
            iso_val = diff*analy->contour_vals[i] + base;

            /*
             * Take a peek at the first superclass to see if it's a
             * nodal result.  If it is, there's no point in looping
             * over the supporting subrecords as we just want to go
             * ahead and hit all volume element classes once.
             */

            sclass = p_result->superclasses[0];

            if ( sclass == G_NODE )
            {
                /*
                 * Generate isosurfaces from nodal data on all volume
                 * element classes.
                 *
                 * This could lead to problems if a database has nodal
                 * classes which don't bind all nodes referenced by the
                 * volume element classes in the mesh.
                 */
                for ( k = 0; k < QTY_SCLASS; k++ )
                {
                    if ( iso_funcs[k] != NULL )
                    {
                        c_qty = p_mesh->classes_by_sclass[k].qty;
                        p_mo_classes = (MO_class_data **)
                                       p_mesh->classes_by_sclass[k].list;

                        for ( l = 0; l < c_qty; l++ )
                            iso_funcs[k]( iso_val, NULL, p_mo_classes[l],
                                          analy );
                    }
                }
            }
            else
            {
                for ( j = 0; j < p_result->qty; j++ )
                {
                    sclass = p_result->superclasses[j];

                    if ( iso_funcs[sclass] == NULL )
                        continue;

                    subrec_id = p_result->subrecs[j];

                    if ( p_result->indirect_flags[j] )
                    {
                        /*
                         * For indirect results, generate isosurfaces on all
                         * classes with correct superclass.
                         */
                        c_qty = p_mesh->classes_by_sclass[sclass].qty;
                        p_mo_classes = (MO_class_data **)
                                       p_mesh->classes_by_sclass[sclass].list;

                        for ( k = 0; k < c_qty; k++ )
                            iso_funcs[sclass]( iso_val, NULL, p_mo_classes[k],
                                               analy );
                    }
                    else
                    {
                        /*
                         * For direct results, generate isosurfaces
                         * on volumetric elements bound to the j'th
                         * supporting subrecord.
                         */
                        p_so = analy->srec_tree[p_result->srec_id].subrecs
                               + subrec_id;
                        iso_funcs[sclass]( iso_val, p_so,
                                           p_so->p_object_class, analy );
                    }
                }
            }
        }
    }
}


/*****************************************************************
 * TAG( hex_isosurface )
 *
 * Generate triangles approximating the isosurface for the given
 * isovalue and the currently displayed variable.
 */
static void
hex_isosurface( float isoval, Subrec_obj *p_so, MO_class_data *p_hex_class,
                Analysis * analy )
{
    float *activ;
    unsigned char vertex_mask;
    float v0, v1, v2, v3, v4, v5, v6, v7;
    float interp;
    float xyz[8][3];
    float *data_buffer;
    Triangle_poly *tri;
    int fcnt;
    int isocnt, el, fc, nd;
    int i, j, k, t, v;
    int (*connects)[8];
    GVec3D *coords;
    int hex_qty;
    Result *p_result;
    int hex_id;
    Visibility_data *p_vd;
    int *object_ids;

    p_result = analy->cur_result;
    if ( p_result == NULL )
        return;

    if ( p_so == NULL )
    {
        /* No subrec specified, so render over entire class. */
        hex_qty = p_hex_class->qty;
        object_ids = NULL;
    }
    else
    {
        /* Subrec is specified, so only render over its bound elements. */
        hex_qty = p_so->subrec.qty_objects;
        object_ids = p_so->object_ids;
    }

    connects = (int (*)[8]) p_hex_class->objects.elems->nodes;
    coords = analy->state_p->nodes.nodes3d;
    p_vd = p_hex_class->p_vis_data;
    activ = analy->state_p->sand_present
            ? analy->state_p->elem_class_sand[p_hex_class->elem_class_index]
            : NULL;
    data_buffer = NODAL_RESULT_BUFFER( analy );

    /*
     * Visit each hexahedral element in the data set.  For now we
     * assume the element is not degenerate.
     */
    for ( i = 0; i < hex_qty; i++ )
    {
        hex_id = object_ids ? object_ids[i] : i;

        /* Skip inactive elements. */
        if ( activ && activ[hex_id] == 0.0 )
            continue;

        /*
         * Clear the vertex "value <= threshold" mask.
         */
        vertex_mask = 0;

        /*
         * Check each vertices which make up cell (i, j) and then set
         * the corresponding bit in the vertex_mask if the value of
         * that vertex is less than or equal to the threshold value.
         *
         * A cube/cell is labeled thus:
         *
         *                     e8
         *           v0-------------------v4    ---
         *           | \                  | \     \
         *           |  \                 |  \     i x
         *           | e0\                | e4\     \
         *           |    \               |    \     \
         *           |     \        e9    |     \     \
         *           |      v1------------+-----v5    ---
         *         e3|      |           e7|      |     |
         *           |      |             |      |     |
         *           |      |             |      |     |
         *           |      |             |      |     |
         *           |    e1|   e11       |      |     |
         *           v3-----+------------v7    e5|     j y
         *             \    |              \     |     |
         *              \   |               \    |     |
         *             e2\  |              e6\   |     |
         *                \ |                 \  |     |
         *                 \|        e10       \ |     |
         *                  v2-------------------v6   ---
         *
         *                  |----------k---------|
         *                             z
         *
         * So that:
         *
         *    vertex list is
         *
         *    v0 = i,   j,   k
         *    v1 = i+1, j,   k
         *    v2 = i+1, j+1, k
         *    v3 = i,   j+1, k
         *    v4 = i,   j,   k+1
         *    v5 = i+1, j,   k+1
         *    v6 = i+1, j+1, k+1
         *    v7 = i,   j+1, k+1
         *
         *    edge list is
         *
         *    e0 = v0 - v1     e4 = v4 - v5      e8 = v0 - v4
         *    e1 = v1 - v2     e5 = v5 - v6      e9 = v1 - v5
         *    e2 = v2 - v3     e6 = v6 - v7     e10 = v2 - v6
         *    e3 = v3 - v0     e7 = v7 - v4     e11 = v3 - v7
         */
        v0 = data_buffer[ connects[hex_id][0] ];
        v1 = data_buffer[ connects[hex_id][1] ];
        v2 = data_buffer[ connects[hex_id][2] ];
        v3 = data_buffer[ connects[hex_id][3] ];
        v4 = data_buffer[ connects[hex_id][4] ];
        v5 = data_buffer[ connects[hex_id][5] ];
        v6 = data_buffer[ connects[hex_id][6] ];
        v7 = data_buffer[ connects[hex_id][7] ];

        if ( v0 <= isoval )
            vertex_mask |= 1;
        if ( v1 <= isoval )
            vertex_mask |= 2;
        if ( v2 <= isoval )
            vertex_mask |= 4;
        if ( v3 <= isoval )
            vertex_mask |= 8;
        if ( v4 <= isoval )
            vertex_mask |= 16;
        if ( v5 <= isoval )
            vertex_mask |= 32;
        if ( v6 <= isoval )
            vertex_mask |= 64;
        if ( v7 <= isoval )
            vertex_mask |= 128;

        if ( triangle_cases_count[vertex_mask] > 0 )
            for ( j = 0; j < 8; j++ )
                for ( k = 0; k < 3; k++ )
                    xyz[j][k] = coords[ connects[hex_id][j] ][k];

        /*
         * Compute the polygon(s) which approximate the iso-surface in
         * the current cell.
         *
         * This implementation of the "Marching Cubes" (see SIGGRAPH '87
         * proceedings) enumerates all 256 cases through a permutation
         * matrix that was constructed painfully by hand by Jeff Kalman.
         * Each case is an array of up to 4 triangles whose vertices are
         * specified by the cell edge number which is the edge that the
         * vertex lies on.
         */

        /* For each triangle in the triangle list for this case
         * (i.e. vertex_mask) compute each vertex.
         */
        for ( t = 0; t < triangle_cases_count[vertex_mask]; t++ )
        {
            tri = NEW( Triangle_poly, "Triangle polygon" );

            /* For each vertex of the triangle compute its value.
             */
            for ( v = 0; v < 3; v++ )
            {
                switch ( triangle_cases[vertex_mask][t][v] )
                {
                case 0:
                    interp = ( v1 - isoval ) /( v1 - v0 );
                    L_INTERP( tri->vtx[v], interp, xyz[1], xyz[0] );

                    /*
                    compute_gradient( i,     j,   k, g0 );
                    compute_gradient( i+1,   j,   k, g1 );
                    linear_interpolate( g1.x, g0.x, interp, normals[v].x );
                    linear_interpolate( g1.y, g0.y, interp, normals[v].y );
                    linear_interpolate( g1.z, g0.z, interp, normals[v].z );
                    cartesian_normalize( normals[v], normals[v] );
                    */
                    break;

                case 1:
                    interp = ( v2 - isoval ) / (v2 - v1 );
                    L_INTERP( tri->vtx[v], interp, xyz[2], xyz[1] );

                    /*
                    compute_gradient( i+1,   j,   k, g1 );
                    compute_gradient( i+1,   j+1, k, g2 );
                    linear_interpolate( g2.x, g1.x, interp, normals[v].x );
                    linear_interpolate( g2.y, g1.y, interp, normals[v].y );
                    linear_interpolate( g2.z, g1.z, interp, normals[v].z );
                    cartesian_normalize( normals[v], normals[v] );
                    */
                    break;

                case 2:
                    interp = ( v3 - isoval ) / (v3 - v2 );
                    L_INTERP( tri->vtx[v], interp, xyz[3], xyz[2] );

                    /*
                    compute_gradient( i+1,   j+1, k, g2 );
                    compute_gradient( i,     j+1, k, g3 );
                    linear_interpolate( g3.x, g2.x, interp, normals[v].x );
                    linear_interpolate( g3.y, g2.y, interp, normals[v].y );
                    linear_interpolate( g3.z, g2.z, interp, normals[v].z );
                    cartesian_normalize( normals[v], normals[v] );
                    */
                    break;

                case 3:
                    interp = ( v0 - isoval ) / (v0 - v3 );
                    L_INTERP( tri->vtx[v], interp, xyz[0], xyz[3] );

                    /*
                    compute_gradient( i,   j+1,   k, g3 );
                    compute_gradient( i,     j,   k, g0 );
                    linear_interpolate( g0.x, g3.x, interp, normals[v].x );
                    linear_interpolate( g0.y, g3.y, interp, normals[v].y );
                    linear_interpolate( g0.z, g3.z, interp, normals[v].z );
                    cartesian_normalize( normals[v], normals[v] );
                    */
                    break;

                case 4:
                    interp = ( v5 - isoval ) / ( v5 - v4 );
                    L_INTERP( tri->vtx[v], interp, xyz[5], xyz[4] );

                    /*
                    compute_gradient( i,     j, k+1, g4 );
                    compute_gradient( i+1,   j, k+1, g5 );
                    linear_interpolate( g5.x, g4.x, interp, normals[v].x );
                    linear_interpolate( g5.y, g4.y, interp, normals[v].y );
                    linear_interpolate( g5.z, g4.z, interp, normals[v].z );
                    cartesian_normalize( normals[v], normals[v] );
                    */
                    break;

                case 5:
                    interp = ( v6 - isoval ) / ( v6 - v5 );
                    L_INTERP( tri->vtx[v], interp, xyz[6], xyz[5] );

                    /*
                    compute_gradient( i+1,   j, k+1, g5 );
                    compute_gradient( i+1, j+1, k+1, g6 );
                    linear_interpolate( g6.x, g5.x, interp, normals[v].x );
                    linear_interpolate( g6.y, g5.y, interp, normals[v].y );
                    linear_interpolate( g6.z, g5.z, interp, normals[v].z );
                    cartesian_normalize( normals[v], normals[v] );
                    */
                    break;

                case 6:
                    interp = ( v7 - isoval ) / ( v7 - v6 );
                    L_INTERP( tri->vtx[v], interp, xyz[7], xyz[6] );

                    /*
                    compute_gradient( i+1, j+1, k+1, g6 );
                    compute_gradient(   i, j+1, k+1, g7 );
                    linear_interpolate( g7.x, g6.x, interp, normals[v].x );
                    linear_interpolate( g7.y, g6.y, interp, normals[v].y );
                    linear_interpolate( g7.z, g6.z, interp, normals[v].z );
                    cartesian_normalize( normals[v], normals[v] );
                    */
                    break;

                case 7:
                    interp = ( v4 - isoval ) / ( v4 - v7 );
                    L_INTERP( tri->vtx[v], interp, xyz[4], xyz[7]);

                    /*
                    compute_gradient( i,  j+1, k+1, g7 );
                    compute_gradient( i,    j, k+1, g4 );
                    linear_interpolate( g4.x, g7.x, interp, normals[v].x );
                    linear_interpolate( g4.y, g7.y, interp, normals[v].y );
                    linear_interpolate( g4.z, g7.z, interp, normals[v].z );
                    cartesian_normalize( normals[v], normals[v] );
                    */
                    break;

                case 8:
                    interp = ( v4 - isoval ) / ( v4 - v0 );
                    L_INTERP( tri->vtx[v], interp, xyz[4], xyz[0]);

                    /*
                    compute_gradient( i,     j, k,   g0 );
                    compute_gradient( i,     j, k+1, g4 );
                    linear_interpolate( g4.x, g0.x, interp, normals[v].x );
                    linear_interpolate( g4.y, g0.y, interp, normals[v].y );
                    linear_interpolate( g4.z, g0.z, interp, normals[v].z );
                    cartesian_normalize( normals[v], normals[v] );
                    */
                    break;

                case 9:
                    interp = ( v5 - isoval ) / ( v5 - v1 );
                    L_INTERP( tri->vtx[v], interp, xyz[5], xyz[1]);

                    /*
                    compute_gradient( i+1,   j,   k, g1 );
                    compute_gradient( i+1,   j, k+1, g5 );
                    linear_interpolate( g5.x, g1.x, interp, normals[v].x );
                    linear_interpolate( g5.y, g1.y, interp, normals[v].y );
                    linear_interpolate( g5.z, g1.z, interp, normals[v].z );
                    cartesian_normalize( normals[v], normals[v] );
                    */
                    break;

                case 10:
                    interp = ( v6 - isoval ) / ( v6 - v2 );
                    L_INTERP( tri->vtx[v], interp, xyz[6], xyz[2]);

                    /*
                    compute_gradient( i+1, j+1,   k, g2 );
                    compute_gradient( i+1, j+1, k+1, g6 );
                    linear_interpolate( g6.x, g2.x, interp, normals[v].x );
                    linear_interpolate( g6.y, g2.y, interp, normals[v].y );
                    linear_interpolate( g6.z, g2.z, interp, normals[v].z );
                    cartesian_normalize( normals[v], normals[v] );
                    */
                    break;

                case 11:
                    interp = ( v7 - isoval ) / ( v7 - v3 );
                    L_INTERP( tri->vtx[v], interp, xyz[7], xyz[3]);

                    /*
                    compute_gradient( i, j+1,   k, g3 );
                    compute_gradient( i, j+1, k+1, g7 );
                    linear_interpolate( g7.x, g3.x, interp, normals[v].x );
                    linear_interpolate( g7.y, g3.y, interp, normals[v].y );
                    linear_interpolate( g7.z, g3.z, interp, normals[v].z );
                    cartesian_normalize( normals[v], normals[v] );
                    */
                    break;

                } /* End case */
            } /* End for v */

            /* Handle degenerate elements. */
            if ( p_hex_class->objects.elems->has_degen
                    && is_degen_triangle( tri ) )
            {
                free( tri );
                continue;
            }

            /* Flag element the triangle came from for use in carpets. */
            tri->elem = hex_id;

            /* Assign the vertex values for display. */
            for ( k = 0; k < 3; k++ )
                tri->result[k] = isoval;

            /* Get the normal. */
            norm_three_pts( tri->norm, tri->vtx[0], tri->vtx[1], tri->vtx[2] );

            /*
             * Throw the triangle onto the isosurface polygon list.
             */
            INSERT( tri, analy->isosurf_poly );

        } /* End for t */
    } /* End for i */


    /*
     * Here, we correct a minor deficiency in the Marching Cubes
     * algorithm.  Visiting each external face in the mesh, we
     * check if three or more nodes on the face are exactly at the
     * isosurf value.  If so, a piece or all of the external face
     * becomes part of the isosurface.
     */
    fcnt = p_vd->face_cnt;

    for ( i = 0; i < fcnt; i++ )
    {
        el = p_vd->face_el[i];
        fc = p_vd->face_fc[i];

        for ( isocnt = 0, j = 0; j < 4; j++ )
        {
            nd = connects[el][ fc_nd_nums[fc][j] ];
            if ( data_buffer[nd] == isoval )
                isocnt++;
        }

        if ( isocnt > 2 )
        {
            if ( isocnt == 4 )
            {
                /* Do the first triangle. */
                tri = NEW( Triangle_poly, "Triangle polygon" );
                for ( j = 0; j < 3; j++ )
                {
                    nd = connects[el][ fc_nd_nums[fc][j] ];
                    for ( k = 0; k < 3; k++ )
                        tri->vtx[j][k] = coords[nd][k];
                    tri->result[j] = isoval;
                }
                norm_three_pts( tri->norm, tri->vtx[0],
                                tri->vtx[1], tri->vtx[2] );
                INSERT( tri, analy->isosurf_poly );

                /* Set up the second triangle. */
                v = 2;
            }
            else
            {
                /* Isocnt = 3, find the first vertex of the triangle. */
                for ( j = 0; j < 4; j++ )
                {
                    nd = connects[el][ fc_nd_nums[fc][j] ];
                    if ( data_buffer[nd] != isoval )
                        break;
                }
                v = j + 1;
            }

            tri = NEW( Triangle_poly, "Triangle polygon" );
            for ( j = 0; j < 3; j++ )
            {
                nd = connects[el][ fc_nd_nums[fc][(v+j)%4] ];
                for ( k = 0; k < 3; k++ )
                    tri->vtx[(v+j)%4][k] = coords[nd][k];
                tri->result[(v+j)%4] = isoval;
            }
            norm_three_pts( tri->norm, tri->vtx[0],
                            tri->vtx[1], tri->vtx[2] );

            /* Handle degenerate elements. */
            if ( p_hex_class->objects.elems->has_degen
                    && is_degen_triangle( tri ) )
                free( tri );
            else
            {
                INSERT( tri, analy->isosurf_poly );
            }
        }
    }
}


/*****************************************************************
 * TAG( gen_cuts )
 *
 * Generate the fine cut plane facets for all cut planes.
 */
void
gen_cuts( Analysis *analy )
{
    int srec_id, mesh_id;
    Mesh_data *p_mesh;
    static MO_class_data **mo_classes = NULL;
    static int class_qty = 0;
    static int prev_mesh_id = -1;
    static int prev_db_ident = 0;
    Plane_obj *plane;
    int i;
    int st_num;
    static Bool_type warn_once = TRUE;
    Classed_list *p_cl;
    Triangle_poly *p_tp;
    unsigned char *part_visib;

    for ( p_cl = analy->cut_poly_lists; p_cl != NULL; NEXT( p_cl ) )
    {
        p_tp = (Triangle_poly *) p_cl->list;
        DELETE_LIST( p_tp );
    }
    DELETE_LIST( analy->cut_poly_lists );

    if ( analy->show_cut || analy->show_particle_cut )
    {
        /* Get the current state record format and its mesh. */
        st_num = analy->cur_state + 1;
        analy->db_query( analy->db_ident, QRY_SREC_FMT_ID, &st_num, NULL,
                         &srec_id );

        if ( analy->last_state <= 0 )
            mesh_id = analy->cur_mesh_id;
        else
            mesh_id = analy->srec_tree[srec_id].subrecs[0].p_object_class->mesh_id;

        /* Update references to mesh classes if necessary. */
        if ( analy->db_ident != prev_db_ident || mesh_id != prev_mesh_id )
        {
            prev_db_ident = analy->db_ident;
            prev_mesh_id = mesh_id;

            p_mesh = analy->mesh_table + mesh_id;

            if ( mo_classes != NULL )
                free( mo_classes );
#ifdef NEWMILI
            htable_get_data( p_mesh->class_table,
                             (void ***) &mo_classes ,
                             &class_qty );
#else
            class_qty = htable_get_data( p_mesh->class_table,
                                         (void ***) &mo_classes);
#endif
        }

        for ( plane = analy->cut_planes; plane != NULL; plane = plane->next )
        {
            /* Generate cut planes where volume element types exist in mesh. */
            for ( i = 0; i < class_qty; i++ )
            {
                switch ( mo_classes[i]->superclass )
                {
                case G_HEX:
                    hex_fine_cut( analy, mo_classes[i], plane->pt,
                                  plane->norm );
                    if ( is_particle_class( analy, mo_classes[i]->superclass, mo_classes[i]->short_name ) )
                        reset_face_visibility( analy );
                    break;
                case G_TET:
                    /*

                        tet_fine_cut( analy, mo_classes[i], plane->pt,
                                  plane->norm );
                    */
                    break;

                case G_PYRAMID:
                case G_WEDGE:
                    /* not implemented */
                    if ( warn_once )
                    {
                        popup_dialog( INFO_POPUP, "%s\n%s%s",
                                      "Cut planes are not implemented on",
                                      "tetrahedral, pyramid or wedge ",
                                      "element types." );
                        warn_once = FALSE;
                    }
                    break;
                default:
                    ;/* Do nothing for non-volumetric mesh object types. */
                }
            }   /* for each class in mesh */
        }   /* for each cut plane */
    }   /* if show_cut */
}


/*****************************************************************
 * TAG( hex_fine_cut )
 *
 * Uses the Marching Cubes algorithm to generate a cutting
 * plane through the mesh.  The result value is displayed
 * on the intersection of the plane and the mesh.
 */
static void
hex_fine_cut( Analysis *analy, MO_class_data *p_elem_class,
              float plane_pt[3], float plane_norm[3] )
{
    float *activ;
    unsigned char vertex_mask;
    float v0, v1, v2, v3, v4, v5, v6, v7;
    float interp;
    float xyz[8][3];
    float tmp[3], val;
    float *result;
    Triangle_poly *tri, *p_tp;
    int i, j, k, t, v;
    int (*connects)[8];
    GVec3D *coords;
    int *mats;
    int hex_qty;
    Classed_list *p_cl;
    int matl, mesh_idx;
    int *p_source_index;
    Bool_type do_result_interpolation;
    Mesh_data *p_mesh;

    unsigned char *hide_mtl=NULL, *hide_brick=NULL;

    hex_qty = p_elem_class->qty;
    connects = (int (*)[8]) p_elem_class->objects.elems->nodes;
    mats = p_elem_class->objects.elems->mat;
    coords = analy->state_p->nodes.nodes3d;
    p_mesh = MESH_P( analy );

    hide_mtl   = p_mesh->hide_material;
    hide_brick = p_mesh->hide_brick_elem;

    if ( result_has_superclass( analy->cur_result, G_MAT, analy ) )
    {
#ifdef OLD_RES_BUFFERS
        result = analy->mat_result;
#endif
        result = ((MO_class_data **)
                  p_mesh->classes_by_sclass[G_MAT].list)[0]->data_buffer;
        p_source_index = &matl;
        do_result_interpolation = FALSE;
    }
    else if ( result_has_superclass( analy->cur_result, G_MESH, analy ) )
    {
#ifdef OLD_RES_BUFFERS
        result = &analy->mesh_result;
#endif
        result = ((MO_class_data **)
                  p_mesh->classes_by_sclass[G_MESH].list)[0]->data_buffer;
        mesh_idx = 0;
        p_source_index = &mesh_idx;
        do_result_interpolation = FALSE;
    }
    else
    {
        result = NODAL_RESULT_BUFFER( analy );
        do_result_interpolation = TRUE;
    }

    activ = analy->state_p->sand_present
            ? analy->state_p->elem_class_sand[p_elem_class->elem_class_index]
            : NULL;

    for ( p_cl = analy->cut_poly_lists; p_cl != NULL; NEXT( p_cl ) )
        if ( p_cl->mo_class == p_elem_class )
            break;
    if ( p_cl == NULL )
    {
        p_cl = NEW( Classed_list, "New cut class list" );
        INSERT( p_cl, analy->cut_poly_lists );
    }
    p_tp = (Triangle_poly *) p_cl->list;

    /*
     * Visit each hexahedral element in the data set.  For now we
     * assume the element is not degenerate.
     */
    for ( i = 0; i < hex_qty; i++ )
    {
        /* Skip inactive elements. */
        if ( activ && activ[i] == 0.0 )
            continue;

        /* Skip hidden materials */
        matl = mats[i];
        if ( hide_mtl[matl] || hide_brick[i] )
            continue;

        if ( hide_brick[i])
            matl = mats[i];

        /*
          * Clear the vertex "value <= threshold" mask.
          */
        vertex_mask = 0;

        for ( j = 0; j < 8; j++ )
            for ( k = 0; k < 3; k++ )
                xyz[j][k] = coords[ connects[i][j] ][k];

        /*
         * Calculate the distance between each node point and the cutting
         * plane.  The sign of the distance tells you on which side of
         * the plane the node lies.  A positive result means the node
         * is on the normal side of the plane.  This calculation assumes
         * the plane normal vector has been normalized to length 1.
         */
        v0 = VEC_DOT( plane_norm, xyz[0] ) - VEC_DOT( plane_norm, plane_pt );
        v1 = VEC_DOT( plane_norm, xyz[1] ) - VEC_DOT( plane_norm, plane_pt );
        v2 = VEC_DOT( plane_norm, xyz[2] ) - VEC_DOT( plane_norm, plane_pt );
        v3 = VEC_DOT( plane_norm, xyz[3] ) - VEC_DOT( plane_norm, plane_pt );
        v4 = VEC_DOT( plane_norm, xyz[4] ) - VEC_DOT( plane_norm, plane_pt );
        v5 = VEC_DOT( plane_norm, xyz[5] ) - VEC_DOT( plane_norm, plane_pt );
        v6 = VEC_DOT( plane_norm, xyz[6] ) - VEC_DOT( plane_norm, plane_pt );
        v7 = VEC_DOT( plane_norm, xyz[7] ) - VEC_DOT( plane_norm, plane_pt );

        if ( v0 <= 0.0 )
            vertex_mask |= 1;
        if ( v1 <= 0.0 )
            vertex_mask |= 2;
        if ( v2 <= 0.0 )
            vertex_mask |= 4;
        if ( v3 <= 0.0 )
            vertex_mask |= 8;
        if ( v4 <= 0.0 )
            vertex_mask |= 16;
        if ( v5 <= 0.0 )
            vertex_mask |= 32;
        if ( v6 <= 0.0 )
            vertex_mask |= 64;
        if ( v7 <= 0.0 )
            vertex_mask |= 128;

        /*
         * Get the absolute values of the distances.
         */
        v0 = v0 < 0.0 ? -v0 : v0;
        v1 = v1 < 0.0 ? -v1 : v1;
        v2 = v2 < 0.0 ? -v2 : v2;
        v3 = v3 < 0.0 ? -v3 : v3;
        v4 = v4 < 0.0 ? -v4 : v4;
        v5 = v5 < 0.0 ? -v5 : v5;
        v6 = v6 < 0.0 ? -v6 : v6;
        v7 = v7 < 0.0 ? -v7 : v7;

        matl = mats[i];

        if ( !do_result_interpolation )
            val = result[*p_source_index];

        /* For each triangle in the triangle list for this case
         * (i.e. vertex_mask) compute each vertex.
         */

        for ( t = 0; t < triangle_cases_count[vertex_mask]; t++ )
        {
            tri = NEW( Triangle_poly, "Triangle polygon" );
            VEC_SET( tri->norm, plane_norm[0], plane_norm[1], plane_norm[2] );
            tri->mat = matl;

            /* Flag element the triangle came from for use in carpets. */
            tri->elem = i;

            if ( do_result_interpolation )
            {
                /* For each vertex of the triangle compute its value.  */

                for ( v = 0; v < 3; v++ )
                {
                    switch ( triangle_cases[vertex_mask][t][v] )
                    {
                    case 0:
                        interp = v1 / ( v1 + v0 );
                        L_INTERP( tri->vtx[v], interp, xyz[1], xyz[0] );
                        tri->result[v] =
                            (1.0-interp)*result[ connects[i][1] ] +
                            interp*result[ connects[i][0] ];
                        break;

                    case 1:
                        interp = v2 / (v2 + v1 );
                        L_INTERP( tri->vtx[v], interp, xyz[2], xyz[1] );
                        tri->result[v] =
                            (1.0-interp)*result[ connects[i][2] ] +
                            interp*result[ connects[i][1] ];
                        break;

                    case 2:
                        interp = v3 / (v3 + v2 );
                        L_INTERP( tri->vtx[v], interp, xyz[3], xyz[2] );
                        tri->result[v] =
                            (1.0-interp)*result[ connects[i][3] ] +
                            interp*result[ connects[i][2] ];
                        break;

                    case 3:
                        interp = v0 / (v0 + v3 );
                        L_INTERP( tri->vtx[v], interp, xyz[0], xyz[3] );
                        tri->result[v] =
                            (1.0-interp)*result[ connects[i][0] ] +
                            interp*result[ connects[i][3] ];
                        break;

                    case 4:
                        interp = v5 / ( v5 + v4 );
                        L_INTERP( tri->vtx[v], interp, xyz[5], xyz[4] );
                        tri->result[v] =
                            (1.0-interp)*result[ connects[i][5] ] +
                            interp*result[ connects[i][4] ];
                        break;

                    case 5:
                        interp = v6 / ( v6 + v5 );
                        L_INTERP( tri->vtx[v], interp, xyz[6], xyz[5] );
                        tri->result[v] =
                            (1.0-interp)*result[ connects[i][6] ] +
                            interp*result[ connects[i][5] ];
                        break;

                    case 6:
                        interp = v7 / ( v7 + v6 );
                        L_INTERP( tri->vtx[v], interp, xyz[7], xyz[6] );
                        tri->result[v] =
                            (1.0-interp)*result[ connects[i][7] ] +
                            interp*result[ connects[i][6] ];
                        break;

                    case 7:
                        interp = v4 / ( v4 + v7 );
                        L_INTERP( tri->vtx[v], interp, xyz[4], xyz[7]);
                        tri->result[v] =
                            (1.0-interp)*result[ connects[i][4] ] +
                            interp*result[ connects[i][7] ];
                        break;

                    case 8:
                        interp = v4 / ( v4 + v0 );
                        L_INTERP( tri->vtx[v], interp, xyz[4], xyz[0]);
                        tri->result[v] =
                            (1.0-interp)*result[ connects[i][4] ] +
                            interp*result[ connects[i][0] ];
                        break;

                    case 9:
                        interp = v5 / ( v5 + v1 );
                        L_INTERP( tri->vtx[v], interp, xyz[5], xyz[1]);
                        tri->result[v] =
                            (1.0-interp)*result[ connects[i][5] ] +
                            interp*result[ connects[i][1] ];
                        break;

                    case 10:
                        interp = v6 / ( v6 + v2 );
                        L_INTERP( tri->vtx[v], interp, xyz[6], xyz[2]);
                        tri->result[v] =
                            (1.0-interp)*result[ connects[i][6] ] +
                            interp*result[ connects[i][2] ];
                        break;

                    case 11:
                        interp = v7 / ( v7 + v3 );
                        L_INTERP( tri->vtx[v], interp, xyz[7], xyz[3]);
                        tri->result[v] =
                            (1.0-interp)*result[ connects[i][7] ] +
                            interp*result[ connects[i][3] ];
                        break;

                    } /* End case */
                } /* End for v */
            }
            else
            {
                /* For each vertex of the triangle assign its value.  */

                for ( v = 0; v < 3; v++ )
                {
                    switch ( triangle_cases[vertex_mask][t][v] )
                    {
                    case 0:
                        interp = v1 / ( v1 + v0 );
                        L_INTERP( tri->vtx[v], interp, xyz[1], xyz[0] );
                        tri->result[v] = val;
                        break;

                    case 1:
                        interp = v2 / (v2 + v1 );
                        L_INTERP( tri->vtx[v], interp, xyz[2], xyz[1] );
                        tri->result[v] = val;
                        break;

                    case 2:
                        interp = v3 / (v3 + v2 );
                        L_INTERP( tri->vtx[v], interp, xyz[3], xyz[2] );
                        tri->result[v] = val;
                        break;

                    case 3:
                        interp = v0 / (v0 + v3 );
                        L_INTERP( tri->vtx[v], interp, xyz[0], xyz[3] );
                        tri->result[v] = val;
                        break;

                    case 4:
                        interp = v5 / ( v5 + v4 );
                        L_INTERP( tri->vtx[v], interp, xyz[5], xyz[4] );
                        tri->result[v] = val;
                        break;

                    case 5:
                        interp = v6 / ( v6 + v5 );
                        L_INTERP( tri->vtx[v], interp, xyz[6], xyz[5] );
                        tri->result[v] = val;
                        break;

                    case 6:
                        interp = v7 / ( v7 + v6 );
                        L_INTERP( tri->vtx[v], interp, xyz[7], xyz[6] );
                        tri->result[v] = val;
                        break;

                    case 7:
                        interp = v4 / ( v4 + v7 );
                        L_INTERP( tri->vtx[v], interp, xyz[4], xyz[7]);
                        tri->result[v] = val;
                        break;

                    case 8:
                        interp = v4 / ( v4 + v0 );
                        L_INTERP( tri->vtx[v], interp, xyz[4], xyz[0]);
                        tri->result[v] = val;
                        break;

                    case 9:
                        interp = v5 / ( v5 + v1 );
                        L_INTERP( tri->vtx[v], interp, xyz[5], xyz[1]);
                        tri->result[v] = val;
                        break;

                    case 10:
                        interp = v6 / ( v6 + v2 );
                        L_INTERP( tri->vtx[v], interp, xyz[6], xyz[2]);
                        tri->result[v] = val;
                        break;

                    case 11:
                        interp = v7 / ( v7 + v3 );
                        L_INTERP( tri->vtx[v], interp, xyz[7], xyz[3]);
                        tri->result[v] = val;
                        break;

                    } /* End case */
                } /* End for v */
            }

            /* Handle degenerate elements. */
            if ( p_elem_class->objects.elems->has_degen
                    && is_degen_triangle( tri ) )
            {
                free( tri );
                continue;
            }

            /*
             * Test whether the ordering of the vertices matches the plane
             * normal.  If not, reverse the order of the triangle vertices.
             */
            norm_three_pts( tmp, tri->vtx[0], tri->vtx[1], tri->vtx[2] );
            if ( VEC_DOT( tmp, plane_norm ) < 0.0 )
            {
                VEC_COPY( tmp, tri->vtx[0] );
                VEC_COPY( tri->vtx[0], tri->vtx[1] );
                VEC_COPY( tri->vtx[1], tmp );
                val = tri->result[0];
                tri->result[0] = tri->result[1];
                tri->result[1] = val;
            }

            /*
             * Throw the triangle onto the cutting plane polygon list.
             */
            INSERT( tri, p_tp );

        } /* End for t */
    } /* End for i */

    if ( p_tp != NULL )
    {
        p_cl->mo_class = p_elem_class;
        p_cl->list = (void *) p_tp;
    }
    else
        DELETE( p_cl, analy->cut_poly_lists );
}


/************************************************************
 * TAG( get_tri_average_area )
 *
 * Returns the average area of the triangular facets in the
 * current cut planes and isosurfaces.
 */
float
get_tri_average_area( analy )
Analysis *analy;
{
    Triangle_poly *tri;
    float area;
    int cnt;
    Classed_list *p_cl;

    area = 0.0;
    cnt = 0;
    for ( tri = analy->isosurf_poly; tri != NULL; tri = tri->next, cnt++ )
        area += area_of_triangle( tri->vtx );
    for ( p_cl = analy->cut_poly_lists; p_cl != NULL; NEXT( p_cl ) )
        for ( tri = (Triangle_poly *) p_cl->list; tri != NULL;
                NEXT( tri ), cnt++ )
            area += area_of_triangle( tri->vtx );

    if ( cnt > 0 )
        return ( area / cnt );
    else
        return 0.0;
}


