/* $Id$ */
/* 
 * iso_surface.c - Construct an isosurface for 3D unstructured cell data.
 * 
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Mar 25 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */

/*
 * This code is derived from software authored by
 *
 *      Author		 : Brian Cabral
 *      Date created	 : November 17, 1988
 *      Algorithm	 : see "Marching Cubes" 1987 SIGGRAPH proceedings
 *      Copyright	 : Lawrence Livermore National Laboratory (c) 1988
 */

#include "viewer.h"
#include "cell_cases.h"

static Bool_type is_degen_triangle();
static void iso_surface();
static void fine_cut();


/*****************************************************************
 * TAG( is_degen_triangle )
 *
 * Check whether a triangle polygon is degenerate (has repeated
 * vertices.)
 */
static Bool_type
is_degen_triangle( tri )
Triangle_poly *tri;
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
 * Generate all isosurfaces.
 */
void
gen_isosurfs( analy )
Analysis *analy;
{
    float iso_val, diff, base;
    int i;
 
    DELETE_LIST( analy->isosurf_poly );

    if ( analy->geom_p->bricks != NULL && analy->show_hex_result &&
         analy->show_isosurfs && analy->contour_cnt > 0 )
    {
        /* Convert from percentages to actual contour values. */
        diff = analy->result_mm[1] - analy->result_mm[0];
        base = analy->result_mm[0];
        for ( i = 0; i < analy->contour_cnt; i++ )
        {
            iso_val = diff*analy->contour_vals[i] + base;
            iso_surface( iso_val, analy );
        }
    }
}


/*****************************************************************
 * TAG( iso_surface )
 *
 * Generate triangles approximating the isosurface for the given
 * isovalue and the currently displayed variable.
 */
static void
iso_surface( isoval, analy )
float isoval;
Analysis *analy;
{
    Hex_geom *bricks;
    Nodal *nodes;
    float *activ;
    unsigned char vertex_mask; 
    float v0, v1, v2, v3, v4, v5, v6, v7;
    float interp;
    float xyz[8][3];
    Triangle_poly *tri;
    int fcnt;
    int isocnt, el, fc, nd;
    int i, j, k, t, v;

    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;
    activ = analy->state_p->activity_present ?
            analy->state_p->bricks->activity : NULL;

    /*
     * Visit each hexahedral element in the data set.  For now we
     * assume the element is not degenerate.
     */
    for ( i = 0; i < bricks->cnt; i++ )
    {
        /* Skip inactive elements. */
        if ( activ && activ[i] == 0.0 )
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
        v0 = analy->result[ bricks->nodes[0][i] ];
        v1 = analy->result[ bricks->nodes[1][i] ];
        v2 = analy->result[ bricks->nodes[2][i] ];
        v3 = analy->result[ bricks->nodes[3][i] ];
        v4 = analy->result[ bricks->nodes[4][i] ];
        v5 = analy->result[ bricks->nodes[5][i] ];
        v6 = analy->result[ bricks->nodes[6][i] ];
        v7 = analy->result[ bricks->nodes[7][i] ];

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
                    xyz[j][k] = nodes->xyz[k][ bricks->nodes[j][i] ];

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
            if ( bricks->degen_elems && is_degen_triangle( tri ) )
            {
                free( tri );
                continue;
            }

            /* Flag element the triangle came from for use in carpets. */
            tri->elem = i;

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
    fcnt = analy->face_cnt;

    for ( i = 0; i < fcnt; i++ )
    {
        el = analy->face_el[i];
        fc = analy->face_fc[i];

        for ( isocnt = 0, j = 0; j < 4; j++ )
        {
            nd = bricks->nodes[ fc_nd_nums[fc][j] ][el];
            if ( analy->result[nd] == isoval )
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
                    nd = bricks->nodes[ fc_nd_nums[fc][j] ][el];
                    for ( k = 0; k < 3; k++ )
                        tri->vtx[j][k] = nodes->xyz[k][nd];
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
                    nd = bricks->nodes[ fc_nd_nums[fc][j] ][el];
                    if ( analy->result[nd] != isoval )
                        break;
                }
                v = j + 1;
            }

            tri = NEW( Triangle_poly, "Triangle polygon" );
            for ( j = 0; j < 3; j++ )
            {
                nd = bricks->nodes[ fc_nd_nums[fc][(v+j)%4] ][el];
                for ( k = 0; k < 3; k++ )
                    tri->vtx[(v+j)%4][k] = nodes->xyz[k][nd];
                tri->result[(v+j)%4] = isoval;
            }
            norm_three_pts( tri->norm, tri->vtx[0],
                            tri->vtx[1], tri->vtx[2] );

            /* Handle degenerate elements. */
            if ( bricks->degen_elems && is_degen_triangle( tri ) )
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
gen_cuts( analy )
Analysis *analy;
{
    Plane_obj *plane;
 
    DELETE_LIST( analy->cut_poly ); 

    if ( analy->geom_p->bricks != NULL && analy->show_cut )
    {
        for ( plane = analy->cut_planes; plane != NULL; plane = plane->next )
            fine_cut( analy, plane->pt, plane->norm );
    }
}


/*****************************************************************
 * TAG( fine_cut )
 *
 * Uses the Marching Cubes algorithm to generate a cutting
 * plane through the mesh.  The result value is displayed
 * on the intersection of the plane and the mesh.
 */
static void
fine_cut( analy, plane_pt, plane_norm )
Analysis *analy;
float plane_pt[3];
float plane_norm[3];
{
    Hex_geom *bricks;
    Nodal *nodes;
    float *activ;
    unsigned char vertex_mask; 
    float v0, v1, v2, v3, v4, v5, v6, v7;
    float interp, leng;
    float xyz[8][3];
    float tmp[3], val;
    float *result;
    Triangle_poly *tri;
    int i, j, k, t, v;

    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;
    result = analy->result;
    activ = analy->state_p->activity_present ?
            analy->state_p->bricks->activity : NULL;

    /*
     * Visit each hexahedral element in the data set.  For now we
     * assume the element is not degenerate.
     */
    for ( i = 0; i < bricks->cnt; i++ )
    {
        /* Skip inactive elements. */
        if ( activ && activ[i] == 0.0 )
            continue;

        /*
         * Clear the vertex "value <= threshold" mask.
         */
        vertex_mask = 0;

        for ( j = 0; j < 8; j++ )
            for ( k = 0; k < 3; k++ )
                xyz[j][k] = nodes->xyz[k][ bricks->nodes[j][i] ];

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

        /* For each triangle in the triangle list for this case 
         * (i.e. vertex_mask) compute each vertex.
         */
        for ( t = 0; t < triangle_cases_count[vertex_mask]; t++ )
        {
            tri = NEW( Triangle_poly, "Triangle polygon" );
            VEC_SET( tri->norm, plane_norm[0], plane_norm[1], plane_norm[2] );
            tri->mat = bricks->mat[i];

            /* Flag element the triangle came from for use in carpets. */
            tri->elem = i;

            /* For each vertex of the triangle compute its value.
             */
            for ( v = 0; v < 3; v++ )
            {
                switch ( triangle_cases[vertex_mask][t][v] )
                {
                    case 0: 
                        interp = v1 / ( v1 + v0 );
                        L_INTERP( tri->vtx[v], interp, xyz[1], xyz[0] );
                        tri->result[v] =
                               (1.0-interp)*result[ bricks->nodes[1][i] ] +
                               interp*result[ bricks->nodes[0][i] ];
                        break;

                    case 1:
                        interp = v2 / (v2 + v1 );
                        L_INTERP( tri->vtx[v], interp, xyz[2], xyz[1] );
                        tri->result[v] =
                               (1.0-interp)*result[ bricks->nodes[2][i] ] +
                               interp*result[ bricks->nodes[1][i] ];
                        break;

                    case 2:
                        interp = v3 / (v3 + v2 );
                        L_INTERP( tri->vtx[v], interp, xyz[3], xyz[2] );
                        tri->result[v] =
                               (1.0-interp)*result[ bricks->nodes[3][i] ] +
                               interp*result[ bricks->nodes[2][i] ];
                        break;

                    case 3:
                        interp = v0 / (v0 + v3 );
                        L_INTERP( tri->vtx[v], interp, xyz[0], xyz[3] );
                        tri->result[v] =
                               (1.0-interp)*result[ bricks->nodes[0][i] ] +
                               interp*result[ bricks->nodes[3][i] ];
                        break;

                    case 4:
                        interp = v5 / ( v5 + v4 );
                        L_INTERP( tri->vtx[v], interp, xyz[5], xyz[4] );
                        tri->result[v] =
                               (1.0-interp)*result[ bricks->nodes[5][i] ] +
                               interp*result[ bricks->nodes[4][i] ];
                        break;

                    case 5:
                        interp = v6 / ( v6 + v5 );
                        L_INTERP( tri->vtx[v], interp, xyz[6], xyz[5] );
                        tri->result[v] =
                               (1.0-interp)*result[ bricks->nodes[6][i] ] +
                               interp*result[ bricks->nodes[5][i] ];
                        break;

                    case 6:
                        interp = v7 / ( v7 + v6 );
                        L_INTERP( tri->vtx[v], interp, xyz[7], xyz[6] );
                        tri->result[v] =
                               (1.0-interp)*result[ bricks->nodes[7][i] ] +
                               interp*result[ bricks->nodes[6][i] ];
                        break;

                    case 7:
                        interp = v4 / ( v4 + v7 );
                        L_INTERP( tri->vtx[v], interp, xyz[4], xyz[7]);
                        tri->result[v] =
                               (1.0-interp)*result[ bricks->nodes[4][i] ] +
                               interp*result[ bricks->nodes[7][i] ];
                        break;

                    case 8:
                        interp = v4 / ( v4 + v0 );
                        L_INTERP( tri->vtx[v], interp, xyz[4], xyz[0]);
                        tri->result[v] =
                               (1.0-interp)*result[ bricks->nodes[4][i] ] +
                               interp*result[ bricks->nodes[0][i] ];
                        break;

                    case 9:
                        interp = v5 / ( v5 + v1 );
                        L_INTERP( tri->vtx[v], interp, xyz[5], xyz[1]);
                        tri->result[v] =
                               (1.0-interp)*result[ bricks->nodes[5][i] ] +
                               interp*result[ bricks->nodes[1][i] ];
                        break;

                    case 10:
                        interp = v6 / ( v6 + v2 );
                        L_INTERP( tri->vtx[v], interp, xyz[6], xyz[2]);
                        tri->result[v] =
                               (1.0-interp)*result[ bricks->nodes[6][i] ] +
                               interp*result[ bricks->nodes[2][i] ];
                        break;

                    case 11:
                        interp = v7 / ( v7 + v3 );
                        L_INTERP( tri->vtx[v], interp, xyz[7], xyz[3]);
                        tri->result[v] =
                               (1.0-interp)*result[ bricks->nodes[7][i] ] +
                               interp*result[ bricks->nodes[3][i] ];
                        break;

                } /* End case */
            } /* End for v */

            /* Handle degenerate elements. */
            if ( bricks->degen_elems && is_degen_triangle( tri ) )
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
            INSERT( tri, analy->cut_poly );

        } /* End for t */
    } /* End for i */
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

    area = 0.0;
    cnt = 0;
    for ( tri = analy->isosurf_poly; tri != NULL; tri = tri->next, cnt++ )
        area += area_of_triangle( tri->vtx );
    for ( tri = analy->cut_poly; tri != NULL; tri = tri->next, cnt++ )
        area += area_of_triangle( tri->vtx );

    if ( cnt > 0 )
        return ( area / cnt );
    else
        return 0.0;
}


