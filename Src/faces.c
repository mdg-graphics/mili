/* $Id$ */
/*
 * faces.c - Routines for generating and modifying the mesh face tables.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Dec 27 1991
 *
 * Copyright (c) 1991 Regents of the University of California
 */

#include <values.h>
#include "viewer.h"


/* Local routines. */
static void double_table_size();
static int *check_match();
static Bool_type check_degen_face();
static void rough_cut();
static int test_plane_elem();
static void face_aver_norm();
static void shell_aver_norm();
static int check_for_triangle();
static void face_normals();
static void shell_normals();
static void get_brick_edges();
static void get_shell_edges();
static void edge_heapsort();
static int edge_node_cmp();
static void find_extents();

/* Default crease threshold angle is 22 degrees.
 * (This is the angle between the face normal and the _average_ node normal.)
 */
float crease_threshold = 0.927;

/* Default angle for explicit edge detection is 44 degrees. */
float explicit_threshold = 0.719;


/************************************************************
 * TAG( create_hex_adj )
 *
 * Compute volume element adjacency information and create
 * the element adjacency table.  The routine uses a bucket
 * sort with approximately linear complexity.
 */
void
create_hex_adj( analy )
Analysis *analy;
{
    Hex_geom *bricks; 
    Bool_type valid;
    int **hex_adj;
    int *node_tbl;
    int *face_tbl[7];
    int face_entry[6];
    int bcnt, ncnt, tbl_sz, free_ptr;
    int *idx, new_idx, tmp;
    int el, fc, elem1, face1, elem2, face2;
    int i, j;

    /*
     * The table face_tbl contains for each face entry
     *
     *     node1 node2 node3 node4 element face next
     *
     * The table hex_adj contains for each hex element
     *
     *     neighbor_elem1 n_elem2 n_elem3 n_elem4 n_elem5 n_elem6
     *
     * If a hex element has no neighbor adjacent to one of its faces,
     * that face neighbor is marked with a -1 in the hex_adj table.
     *
     * For algorithm see:
     *
     *      "Visual3 - A Software Environment for Flow Visualization,"
     *      Computer Graphics and Flow Visualization in Computational
     *      Fluid Dynamics, VKI Lecture Series #10, Brussels, Sept 16-20
     *      1991.
     */

    if ( analy->geom_p->bricks == NULL )
        return;

    bricks = analy->geom_p->bricks;
    hex_adj = analy->hex_adj;
    bcnt = bricks->cnt;
    ncnt = analy->geom_p->nodes->cnt;

    /* Arbitrarily set the table size. */
    if ( analy->num_blocks > 1 )
        tbl_sz = 6 * bcnt / 10;
    else
        tbl_sz = 6 * bcnt;

    /* Allocate the tables and initialize table pointers. */
    node_tbl = NEW_N( int, ncnt, "Node table" );
    for ( i = 0; i < ncnt; i++ )
        node_tbl[i] = -1;

    for ( i = 0; i < 7; i++ )
        face_tbl[i] = NEW_N( int, tbl_sz, "Face table" );
    for ( i = 0; i < tbl_sz; i++ )
        face_tbl[6][i] = i + 1;
    face_tbl[6][tbl_sz-1] = -1;
    free_ptr = 0;

    /*
     * Initialize the adjacency table so all neighbors are NULL.
     */
    for ( i = 0; i < bcnt; i++ )
        for ( j = 0; j < 6; j++ )
            hex_adj[j][i] = -1;

    /*
     * Table generation loop.
     */
    for ( el = 0; el < bcnt; el++ )
        for ( fc = 0; fc < 6; fc++ )
        {
            /* Get nodes for a face. */
            for ( i = 0; i < 4; i++ )
                face_entry[i] = bricks->nodes[ fc_nd_nums[fc][i] ][el];
            face_entry[4] = el;
            face_entry[5] = fc;

            /* Order the node numbers in ascending order. */
            for ( i = 0; i < 3; i++ )
                for ( j = i+1; j < 4; j++ )
                    if ( face_entry[i] > face_entry[j] )
                        SWAP( tmp, face_entry[i], face_entry[j] );

            /* Check for degenerate element faces. */
            if ( bricks->degen_elems )
            {
                valid = check_degen_face( face_entry );

                /* If face is degenerate (pt or line), skip it. */
                if ( !valid )
                    continue;
            }

            idx = check_match( face_entry, node_tbl, face_tbl );

            if ( *idx >= 0 )
            {
                /*
                 * The face is repeated, so make the two elements
                 * that share the face point to each other.
                 */
                elem1 = face_entry[4];
                face1 = face_entry[5];
                elem2 = face_tbl[4][ *idx ];
                face2 = face_tbl[5][ *idx ];
                hex_adj[ face1 ][ elem1 ] = elem2;
                hex_adj[ face2 ][ elem2 ] = elem1;

                /* Put the face entry on the free list. */
                tmp = face_tbl[6][*idx];
                face_tbl[6][*idx] = free_ptr;
                free_ptr = *idx;
                *idx = tmp;
            }
            else
            {
                /* Create a new face entry and append it to node list. */
                if ( free_ptr < 0 )
                    double_table_size( &tbl_sz, face_tbl, &free_ptr );
                new_idx = free_ptr;
                free_ptr = face_tbl[6][free_ptr];

                for ( i = 0; i < 6; i++ )
                    face_tbl[i][new_idx] = face_entry[i];
                face_tbl[6][new_idx] = -1;
                *idx = new_idx;
            }
        }

    /* Free the tables. */
    free( node_tbl );
    for ( i = 0; i < 7; i++ )
        free( face_tbl[i] );
}


/************************************************************
 * TAG( double_table_size )
 *
 * Expand the size of the face table to create more entries.
 */
static void
double_table_size( tbl_sz, face_tbl, free_ptr )
int *tbl_sz;
int **face_tbl;
int *free_ptr;
{
    int *new_tbl;
    int sz, i, j;

    sz = *tbl_sz;
    fprintf( stderr, "Expanding face table.\n" );
    for ( i = 0; i < 7; i++ )
    {
        new_tbl = NEW_N( int, 2*sz, "Face table" );
        for ( j = 0; j < sz; j++ )
            new_tbl[j] = face_tbl[i][j];
        free( face_tbl[i] );
        face_tbl[i] = new_tbl;
    }
    for ( i = sz; i < 2*sz; i++ )
        face_tbl[6][i] = i + 1;
    face_tbl[6][2*sz-1] = -1;
    *free_ptr = sz;
    *tbl_sz = sz*2;
}


/************************************************************
 * TAG( check_match )
 *
 * Check if a face entry already exists.
 */
static int *
check_match( face_entry, node_tbl, face_tbl )
int *face_entry;
int *node_tbl;
int **face_tbl;
{
    int *idx;

    idx = &node_tbl[face_entry[0]];
    while ( *idx >= 0 )
    {
        if ( face_entry[0] == face_tbl[0][*idx] &&
             face_entry[1] == face_tbl[1][*idx] &&
             face_entry[2] == face_tbl[2][*idx] &&
             face_entry[3] == face_tbl[3][*idx] )
            return idx;
        else
            idx = &face_tbl[6][*idx];
    }

    /* No match. */
    return idx;
}


/************************************************************
 * TAG( check_degen_face )
 *
 * Check whether an element face is degenerate.  Face may
 * degenerate to a triangle, a line segment, or a point.
 * If face is a triangle, the redundant node is set to -1.
 * (This is done to so that the sort and compression work
 * correctly for triangular faces.)  If the face is a
 * quadrilateral or triangle, the routine returns 1; otherwise
 * the routine returns 0.
 *
 * NOTE:
 *     The routine assumes that the face nodes have already
 * been sorted in ascending order.
 */
static Bool_type
check_degen_face( nodes )
int *nodes;
{
    int match_cnt, tmp;
    int i, j;

    for ( match_cnt = 0, i = 0; i < 3; i++ )
        if ( nodes[i] == nodes[i+1] )
            match_cnt++;

    /* Check for invalid face (point or line). */
    if ( match_cnt > 1 )
        return FALSE;

    /* Check for triangle face. */
    if ( match_cnt == 1 )
    {
        /* Get rid of the redundant node. */
        for ( i = 0; i < 3; i++ )
            if ( nodes[i] == nodes[i+1] )
            {
                nodes[i] = -1;
                break;
            }

        /* Re-order the nodes in ascending order. */
        for ( i = 0; i < 3; i++ )
            for ( j = i+1; j < 4; j++ )
                if ( nodes[i] > nodes[j] )
                    SWAP( tmp, nodes[i], nodes[j] );
    }

    /* Valid face. */
    return TRUE;
}


/************************************************************
 * TAG( set_hex_visibility )
 *
 * Set the visibility flag for each element.  Takes into
 * account element activity, invisible materials and roughcut
 * planes.
 */
void
set_hex_visibility( analy ) 
Analysis *analy;
{
    Plane_obj *plane;
    int *hex_visib;
    int *materials;
    float *activity;
    Bool_type hidden_matls;
    int brick_cnt;
    int i;

    if ( analy->geom_p->bricks == NULL )
        return;

    hex_visib = analy->hex_visib;
    materials = analy->geom_p->bricks->mat;
    brick_cnt = analy->geom_p->bricks->cnt;
    activity = analy->state_p->activity_present ?
               analy->state_p->bricks->activity : NULL;

    /* Initialize all elements to be visible. */
    for ( i = 0; i < brick_cnt; i++ )
        hex_visib[i] = TRUE;

    /*
     * If brick is inactive (0.0) then it is invisible.
     */
    if ( activity )
    {
        for ( i = 0; i < brick_cnt; i++ )
            if ( activity[i] == 0.0 )
                hex_visib[i] = FALSE;
    }

    /*
     * If brick material is hidden then it is invisible.
     */
    hidden_matls = FALSE;
    for ( i = 0; i < analy->num_materials; i++ )
        if ( analy->hide_material[i] )
            hidden_matls = TRUE;

    if ( hidden_matls )
    {
        for ( i = 0; i < brick_cnt; i++ )
            if ( analy->hide_material[materials[i]] )
                hex_visib[i] = FALSE;
    }

    /*
     * Perform the roughcuts.
     */
    if ( analy->show_roughcut )
    {
        for ( plane = analy->cut_planes; plane != NULL; plane = plane->next )
            rough_cut( plane->pt, plane->norm, analy->geom_p->bricks,
                       analy->state_p->nodes, analy->hex_visib );
    }
}


/************************************************************
 * TAG( rough_cut )
 *
 * Display just the elements that are on one side of a cutting
 * plane.  Marks all elements on the normal side of the cutting
 * plane as invisible.
 *
 * ppt       | A point on the plane
 * norm      | Normal vector of plane.  Elements on the normal
 *           | side of the plane are not shown.
 * bricks    | Brick data
 * nodes     | Node data
 * hex_visib | Hex element visibility
 */
static void
rough_cut( ppt, norm, bricks, nodes, hex_visib )
float *ppt;
float *norm;
Hex_geom *bricks;
Nodal *nodes; 
Bool_type *hex_visib;
{
    int test, i;

    for ( i = 0; i < bricks->cnt; i++ )
    {
        /* If element is on exterior side of plane (in normal direction),
         * mark it as invisible.
         */ 
        test = test_plane_elem( ppt, norm, i, bricks, nodes );
        if ( test <= 0 )
            hex_visib[i] = FALSE;
    }
}


/************************************************************
 * TAG( test_plane_elem )
 *
 * Test how a brick element lies with respect to a plane.  Returns
 *     1 for interior (opposite normal),
 *     -1 for exterior,
 *     0 for intersection.
 * Plane is defined by a point on the plane and a normal vector.
 */
static int
test_plane_elem( ppt, norm, el, bricks, nodes )
float *ppt;
float *norm;
int el;
Hex_geom *bricks;
Nodal *nodes;
{
    int i, j, nnum, result;
    float nd[3], con, test;

    result = 0;
    con = VEC_DOT( norm, ppt );

    /* Blindly perform eight node tests.  Plug the node point into
     * the plane equation and look at the sign of the result.
     */
    for ( i = 0; i < 8; i++ )
    {
        nnum = bricks->nodes[i][el];
        for ( j = 0; j < 3; j++ )
            nd[j] = nodes->xyz[j][nnum];

        test = VEC_DOT( norm, nd ) - con;
        result += SIGN( test );
    }

    if ( result == -8 )
        return 1;
    else if ( result == 8 )
        return -1;
    else
        return 0;
}


/************************************************************
 * TAG( get_hex_faces )
 *
 * Regenerate the volume element face lists from the element
 * adjacency information.  Assumes that hex adjacency has been
 * computed and hex visibility has been set.
 */
void
get_hex_faces( analy )
Analysis *analy;
{
    Hex_geom *bricks; 
    int **hex_adj;
    Bool_type *hex_visib;
    int *face_el;
    int *face_fc;
    int bcnt, fcnt, m1, m2;
    int i, j;

    bricks = analy->geom_p->bricks;

    if ( bricks == NULL )
        return;

    bcnt = bricks->cnt;

    /* Free up old face lists. */
    if ( analy->face_el != NULL )
    {
        free( analy->face_el );
        free( analy->face_fc );
        for ( i = 0; i < 4; i++ )
            for ( j = 0; j < 3; j++ )
                free( analy->face_norm[i][j] );
    }

    /* Faster references. */
    hex_adj = analy->hex_adj;
    hex_visib = analy->hex_visib;

    /* Count the number of external faces. */
    for ( fcnt = 0, i = 0; i < bcnt; i++ )
        if ( hex_visib[i] )
            for ( j = 0; j < 6; j++ )
            {
                if ( hex_adj[j][i] < 0 || !hex_visib[ hex_adj[j][i] ] )
                    fcnt++;
                else if ( analy->translate_material )
                {
                    /* Material translations can expose faces. */
                    m1 = bricks->mat[i];
                    m2 = bricks->mat[ hex_adj[j][i] ];
                    if ( analy->mtl_trans[0][m1] != analy->mtl_trans[0][m2] ||
                         analy->mtl_trans[1][m1] != analy->mtl_trans[1][m2] ||
                         analy->mtl_trans[2][m1] != analy->mtl_trans[2][m2] )
                        fcnt++;
                }
            }
    analy->face_cnt = fcnt;

    /* Allocate new face lists. */
    analy->face_el = NEW_N( int, fcnt, "Face element table" );
    analy->face_fc = NEW_N( int, fcnt, "Face side table" );
    for ( i = 0; i < 4; i++ )
        for ( j = 0; j < 3; j++ )
            analy->face_norm[i][j] = NEW_N( float, fcnt, "Face normals" );

    /* Faster references. */
    face_el = analy->face_el;
    face_fc = analy->face_fc;

    /* Build the tables. */
    for ( fcnt = 0, i = 0; i < bcnt; i++ )
        if ( hex_visib[i] )
            for ( j = 0; j < 6; j++ )
            {
                if ( hex_adj[j][i] < 0 || !hex_visib[ hex_adj[j][i] ] )
                {
                    face_el[fcnt] = i;
                    face_fc[fcnt] = j;
                    fcnt++;
                }
                else if ( analy->translate_material )
                {
                    /* Material translations can expose faces. */
                    m1 = bricks->mat[i];
                    m2 = bricks->mat[ hex_adj[j][i] ];
                    if ( analy->mtl_trans[0][m1] != analy->mtl_trans[0][m2] ||
                         analy->mtl_trans[1][m1] != analy->mtl_trans[1][m2] ||
                         analy->mtl_trans[2][m1] != analy->mtl_trans[2][m2] )
                    {
                        face_el[fcnt] = i;
                        face_fc[fcnt] = j;
                        fcnt++;
                    }
                }
            }

   /*
    * Keep in mind that the face normals still need to be computed.
    * This routine allocates space for the face normals but does not
    * compute them.
    */
}


/************************************************************
 * TAG( reset_face_visibility )
 *
 * Reset the visibility of hex elements and then regenerate
 * the face lists.  Note that the face normals are not computed
 * by this routine and will need to be recomputed after the
 * routine is called.
 */
void
reset_face_visibility( analy )
Analysis *analy;
{
    set_hex_visibility( analy );
    get_hex_faces( analy );
}


/************************************************************
 * TAG( get_face_verts )
 *
 * Return the vertices of a brick element face.
 */
void
get_face_verts( elem, face, analy, verts )
int elem;
int face;
Analysis *analy;
float verts[4][3];
{
    Hex_geom *bricks;
    Nodal *nodes, *onodes;
    float orig;
    int nd, i, j;

    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;
    onodes = analy->geom_p->nodes;

    for ( i = 0; i < 4; i++ )
    {
        nd = bricks->nodes[ fc_nd_nums[face][i] ][elem];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < 3; j++ )
            {
                orig = onodes->xyz[j][nd];
                verts[i][j] = orig + analy->displace_scale[j]*
                              (nodes->xyz[j][nd] - orig);
            }
        }
        else
        {
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes->xyz[j][nd];
        }
    }
}


/************************************************************
 * TAG( get_hex_verts )
 *
 * Return the vertices of a hex element.
 */
void
get_hex_verts( elem, analy, verts )
int elem;
Analysis *analy;
float verts[][3];
{
    Hex_geom *bricks;
    Nodal *nodes, *onodes;
    float orig;
    int nd, i, j;

    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;
    onodes = analy->geom_p->nodes;

    for ( i = 0; i < 8; i++ )
    {
        nd = bricks->nodes[i][elem];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < 3; j++ )
            {
                orig = onodes->xyz[j][nd];
                verts[i][j] = orig + analy->displace_scale[j]*
                              (nodes->xyz[j][nd] - orig);
            }
        }
        else
        {
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes->xyz[j][nd];
        }
    }
}


/************************************************************
 * TAG( get_shell_verts )
 *
 * Return the vertices of a shell element.
 */
void
get_shell_verts( elem, analy, verts )
int elem;
Analysis *analy;
float verts[][3];
{
    Shell_geom *shells;
    Nodal *nodes, *onodes;
    float orig;
    int nd, i, j;

    shells = analy->geom_p->shells;
    nodes = analy->state_p->nodes;
    onodes = analy->geom_p->nodes;

    for ( i = 0; i < 4; i++ )
    {
        nd = shells->nodes[i][elem];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < 3; j++ )
            {
                orig = onodes->xyz[j][nd];
                verts[i][j] = orig + analy->displace_scale[j]*
                              (nodes->xyz[j][nd] - orig);
            }
        }
        else
        {
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes->xyz[j][nd];
        }
    }
}


/************************************************************
 * TAG( get_beam_verts )
 *
 * Return the vertices of a beam element.
 */
Bool_type
get_beam_verts( elem, analy, verts )
int elem;
Analysis *analy;
float verts[][3];
{
    Beam_geom *beams;
    Nodal *nodes, *onodes;
    float orig;
    int nd, i, j, ncnt;

    beams = analy->geom_p->beams;
    nodes = analy->state_p->nodes;
    onodes = analy->geom_p->nodes;
    ncnt = nodes->cnt;

    for ( i = 0; i < 2; i++ )
    {
        nd = beams->nodes[i][elem];
	
	if ( nd < 0 || nd > ncnt - 1 )
	    return FALSE;

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < 3; j++ )
            {
                orig = onodes->xyz[j][nd];
                verts[i][j] = orig + analy->displace_scale[j]*
                              (nodes->xyz[j][nd] - orig);
            }
        }
        else
        {
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes->xyz[j][nd];
        }
    }
    
    return TRUE;
}


/************************************************************
 * TAG( get_node_vert )
 *
 * Return the (possibly displaced) position of a node.
 */
void
get_node_vert( node_num, analy, vert )
int node_num;
Analysis *analy;
float vert[3];
{
    Nodal *nodes, *onodes;
    float orig;
    int nd, i;

    nodes = analy->state_p->nodes;
    onodes = analy->geom_p->nodes;

    if ( analy->displace_exag )
    {
        /* Scale the node displacements. */
        for ( i = 0; i < 3; i++ )
        {
            orig = onodes->xyz[i][node_num];
            vert[i] = orig + analy->displace_scale[i]*
                      (nodes->xyz[i][node_num] - orig);
        }
    }
    else
    {
        for ( i = 0; i < 3; i++ )
            vert[i] = nodes->xyz[i][node_num];
    }
}


/************************************************************
 * TAG( face_aver_norm )
 *
 * Return the average normal of a brick face.
 */
static void
face_aver_norm( elem, face, analy, norm )
int elem;
int face;
Analysis *analy;
float *norm;
{
    float vert[4][3];
    float v1[3], v2[3];

    get_face_verts( elem, face, analy, vert );

    /*
     * Get an average normal by taking the cross-product of the
     * diagonals.  This handles triangular faces correctly.
     */
    VEC_SUB( v1, vert[2], vert[0] );
    VEC_SUB( v2, vert[3], vert[1] );
    VEC_CROSS( norm, v1, v2 );
    vec_norm( norm );
}


/************************************************************
 * TAG( shell_aver_norm )
 *
 * Return the average normal of a shell element.
 */
static void
shell_aver_norm( elem, analy, norm )
int elem;
Analysis *analy;
float *norm;
{
    float vert[4][3];
    float v1[3], v2[3];

    get_shell_verts( elem, analy, vert );

    /*
     * Get an average normal by taking the cross-product of the
     * diagonals.  This handles triangular faces correctly.
     */
    VEC_SUB( v1, vert[2], vert[0] );
    VEC_SUB( v2, vert[3], vert[1] );
    VEC_CROSS( norm, v1, v2 );
    vec_norm( norm );
}


/************************************************************
 * TAG( check_for_triangle )
 *
 * Check for a triangular face (i.e., one of the four face
 * nodes is repeated.)  if the face is triangular, reorder
 * the nodes so the repeated node is last.  The routine
 * returns the number of nodes for the face (3 or 4).
 */
static int
check_for_triangle( nodes )
int nodes[4];
{
    int shift, tmp;
    int i, j, k;

    for ( i = 0; i < 4; i++ )
    {
        if ( nodes[i] == nodes[(i+1)%4] )
        {
            if ( i == 3 )
                shift = 3;
            else
                shift = 2 - i;

            for ( j = 0; j < shift; j++ )
            {
                tmp = nodes[3];
                for ( k = 3; k > 0; k-- )
                    nodes[k] = nodes[k-1];
                nodes[0] = tmp;
            }

            return( 3 );
        }
    }

    return( 4 );
}


/************************************************************
 * TAG( face_normals )
 *
 * Recalculate the vertex normals for the hex element faces.
 */
static void
face_normals( analy )
Analysis *analy;
{
    Hex_geom *bricks;
    Nodal *nodes;
    float **node_norm;
    int *face_el;
    int *face_fc;
    int fcnt;
    float f_norm[3], n_norm[3];
    float dot;
    int i, j, k, el, face, nd, nds[4], num_nds;

    /*
     * Steps are as follows:
     *
     * 1.) Calculate face normals.
     * 2.) Calculate averaged node normals.
     * 3.) Plug either the average face normal or the average node normal
     *     into the face node normals.
     */

/*
 * Need to take into account element type.
 */

    fcnt = analy->face_cnt;
    face_el = analy->face_el;
    face_fc = analy->face_fc;
    node_norm = analy->node_norm;
    nodes = analy->state_p->nodes;
    bricks = analy->geom_p->bricks;

    /*
     * Compute an average normal for each face.
     */
    for ( i = 0; i < fcnt; i++ )
    {
        el = face_el[i];
        face = face_fc[i];

        face_aver_norm( el, face, analy, f_norm );

        /* Stick the average normal into the first vertex normal. */
        for ( k = 0; k < 3; k++ )
            analy->face_norm[0][k][i] = f_norm[k];

        /* If flat shading, put average into all four vertex normals. */
        if ( !analy->normals_smooth )
            for ( j = 1; j < 4; j++ )
                for ( k = 0; k < 3; k++ )
                    analy->face_norm[j][k][i] = f_norm[k];
    }

    /*
     * For flat shading, we're done so return.
     */
    if ( !analy->normals_smooth )
        return;

    /*
     * We must be smooth shading.  Zero out the node normals array.
     */
    for ( i = 0; i < 3; i++ )
        memset( node_norm[i], 0, nodes->cnt*sizeof( float ) );
 
    /*
     * Compute average normals at each of the nodes.
     */
    for ( i = 0; i < fcnt; i++ )
    {
        el = face_el[i];
        face = face_fc[i];

        for ( j = 0; j < 4; j++ )
            nds[j] = bricks->nodes[ fc_nd_nums[face][j] ][el];

        /* Check for triangular (degenerate) face. */
        num_nds = 4;
        if ( bricks->degen_elems )
            num_nds = check_for_triangle( nds );

        for ( j = 0; j < num_nds; j++ )
        {
            for ( k = 0; k < 3; k++ )
                node_norm[k][nds[j]] += analy->face_norm[0][k][i];
        }

        /*
         * Note that the node normals still need to be normalized.
         */
    }

    /*
     * Compare average normals at nodes with average normals on faces
     * to determine which normal to store for each face vertex.
     */
    for ( i = 0; i < fcnt; i++ )
    {
        el = face_el[i];
        face = face_fc[i];

        for ( k = 0; k < 3; k++ )
            f_norm[k] = analy->face_norm[0][k][i];

        for ( j = 0; j < 4; j++ )
        {
            nd = bricks->nodes[ fc_nd_nums[face][j] ][el];

            for ( k = 0; k < 3; k++ )
                n_norm[k] = node_norm[k][nd];
            vec_norm( n_norm );
 
            /*
             * Dot product test.
             */
            dot = VEC_DOT( f_norm, n_norm );
 
            /*
             * Magical constant.  An edge is detected when the angle
             * between normals is greater than a critical angle.
             */
            if ( dot < crease_threshold )
            {
                /* Edge detected, use face normal. */
                for ( k = 0; k < 3; k++ )
                    analy->face_norm[j][k][i] = f_norm[k];
            }
            else
            {
                /* No edge, use node normal. */
                for ( k = 0; k < 3; k++ )
                    analy->face_norm[j][k][i] = n_norm[k];
            }
        }
    }
}


/************************************************************
 * TAG( shell_normals )
 *
 * Recalculate the vertex normals for the shell elements.
 */
static void
shell_normals( analy )
Analysis *analy;
{
    Shell_geom *shells;
    Nodal *nodes, *onodes;
    float **node_norm;
    float dot, f_norm[3], n_norm[3];
    float orig;
    int shell_cnt, num_nd;
    int i, j, k, nd;

    shells = analy->geom_p->shells;
    nodes = analy->state_p->nodes;
    onodes = analy->geom_p->nodes;
    node_norm = analy->node_norm;
    shell_cnt = shells->cnt;
 
    /*
     * Compute an average polygon normal for each shell element.
     */
    for ( i = 0; i < shell_cnt; i++ )
    {  
        shell_aver_norm( i, analy, f_norm );

        /* Stick the average normal into the first vertex normal. */
        for ( k = 0; k < 3; k++ )
            analy->shell_norm[0][k][i] = f_norm[k];

        /* If flat shading, put average into all four vertex normals. */
        if ( !analy->normals_smooth )
            for ( j = 1; j < 4; j++ )
                for ( k = 0; k < 3; k++ )
                    analy->shell_norm[j][k][i] = f_norm[k];
    }

    /*
     * For flat shading, we're done so return.
     */
    if ( !analy->normals_smooth )
        return;

    /*
     * We must be smooth shading.  Zero out the node normals array.
     */
    for ( i = 0; i < 3; i++ )
        memset( node_norm[i], 0, nodes->cnt*sizeof( float ) );
 
    /*
     * Compute average normals at each of the nodes.
     */
    for ( i = 0; i < shell_cnt; i++ )
    {
        num_nd = 4;

        /* Check for triangular shell element. */
        if ( shells->degen_elems &&
             shells->nodes[2][i] == shells->nodes[3][i] )
            num_nd = 3;

        for ( j = 0; j < num_nd; j++ )
        {
            nd = shells->nodes[j][i];
            for ( k = 0; k < 3; k++ )
                node_norm[k][nd] += analy->shell_norm[0][k][i];
        }
    }
 
    /*
     * Compare average normals at nodes with average normals on shells
     * to determine which normal to store for each shell vertex.
     */
    for ( i = 0; i < shell_cnt; i++ )
    {
        for ( k = 0; k < 3; k++ )
            f_norm[k] = analy->shell_norm[0][k][i];
 
        for ( j = 0; j < 4; j++ )
        {
            nd = shells->nodes[j][i];

            for ( k = 0; k < 3; k++ )
                n_norm[k] = node_norm[k][nd];
            vec_norm( n_norm );

            /*                  
             * Dot product test.
             */
/*
            Try this if adjacent shell elements have opposite
            directed normals (i.e., normals are inconsistent).

            dot = fabsf( VEC_DOT( f_norm, n_norm ) );
*/
            dot = VEC_DOT( f_norm, n_norm );

            /*
             * Magical constant.  An edge is detected when the angle
             * between normals is greater than a critical angle.
             */
            if ( dot < crease_threshold )
            {  
                /* Edge detected, use face normal. */
                for ( k = 0; k < 3; k++ )
                    analy->shell_norm[j][k][i] = f_norm[k];
            }
            else
            {
                /* No edge, use node normal. */
                for ( k = 0; k < 3; k++ )
                    analy->shell_norm[j][k][i] = n_norm[k];
            }
        }
    }
}


/************************************************************
 * TAG( compute_normals )
 *
 * Compute the vertex normals for the brick faces and shell
 * elements.
 */
void
compute_normals( analy ) 
Analysis *analy;
{
    if ( analy->geom_p->shells != NULL )
        shell_normals( analy );

    /* Want to leave face normals in node_norm array for use with
     * reference surfaces.  So do face normals after shell normals.
     */
    if ( analy->geom_p->bricks != NULL )
        face_normals( analy );
}


/************************************************************
 * TAG( bbox_nodes )
 *
 * Compute the bounding box of the nodes or just the nodes
 * referenced by the element types indicated by string
 * "types".  Lower and upper bounding coords are returned in 
 * bb_lo and bb_hi.
 *
 * If "types" is "v", check nodes referenced by all 
 * element types but only if that element's material is visible.
 *
 * String "types" must not be null and should not contain
 * 'n' or 'v' if it has 'h', 's', or 'b'.
 */
void
bbox_nodes( analy, types, use_geom, bb_lo, bb_hi )
Analysis *analy;
char *types;
Bool_type use_geom;
float bb_lo[3];
float bb_hi[3];
{
    Nodal *nodes;
    Hex_geom *bricks;
    Shell_geom *shells;
    Beam_geom *beams;
    float **xyz;
    int i, j;
    int cnt;
    char *pc;
    Bool_type el_nodes_ok;
    Bool_type *tested;
    
    nodes = ( use_geom ) ? analy->geom_p->nodes : analy->state_p->nodes;
    xyz = nodes->xyz;
    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;
    beams = analy->geom_p->beams;

    for ( i = 0; i < 3; i++ )
    {
        bb_lo[i] = MAXFLOAT;    
        bb_hi[i] = -MAXFLOAT;    
    }
    
    el_nodes_ok = FALSE;
    tested = NULL;
    for ( pc = types; *pc != '\0'; pc++ )
    {
	switch( *pc )
	{
	    case 'h':
	        /* Evaluate nodes referenced by hex elements. */
	        if ( tested == NULL )
		    tested = NEW_N( Bool_type, nodes->cnt, "Node visit flags" );
		if ( bricks != NULL )
		{
		    el_nodes_ok = TRUE;
                    find_extents( bricks->cnt, 8, bricks->nodes, 
		                  xyz, tested, NULL, 0, bb_lo, bb_hi );
		}
	        break;
	
	    case 's':
	        /* Evaluate nodes referenced by shell elements. */
	        if ( tested == NULL )
		    tested = NEW_N( Bool_type, nodes->cnt, "Node visit flags" );
		if ( shells != NULL )
		{
		    el_nodes_ok = TRUE;
                    find_extents( shells->cnt, 4, shells->nodes, 
		                  xyz, tested, NULL, 0, bb_lo, bb_hi );
		}
	        break;
	    
	    case 'b':
	        /* Evaluate nodes referenced by beam elements. */
	        if ( tested == NULL )
		    tested = NEW_N( Bool_type, nodes->cnt, "Node visit flags" );
		if ( beams != NULL )
		{
		    el_nodes_ok = TRUE;
                    find_extents( beams->cnt, 2, beams->nodes, 
		                  xyz, tested, NULL, 0, bb_lo, bb_hi );
		}
	        break;
	    
	    case 'v':
	        /* 
		 * Evaluate nodes referenced by all elements but constrained
		 * by material visibility.
		 */
	        if ( tested == NULL )
		    tested = NEW_N( Bool_type, nodes->cnt, "Node visit flags" );
		    
		if ( bricks != NULL )
		{
		    el_nodes_ok = TRUE;
                    find_extents( bricks->cnt, 8, bricks->nodes, xyz, tested, 
		                  analy->hide_material, bricks->mat, 
				  bb_lo, bb_hi );
		}
		
		if ( shells != NULL )
		{
		    el_nodes_ok = TRUE;
                    find_extents( shells->cnt, 4, shells->nodes, xyz, tested, 
		                  analy->hide_material, shells->mat, 
				  bb_lo, bb_hi );
		}
		
		if ( beams != NULL )
		{
		    el_nodes_ok = TRUE;
                    find_extents( beams->cnt, 2, beams->nodes, xyz, tested, 
		                  analy->hide_material, beams->mat, 
				  bb_lo, bb_hi );
		}
	        break;
	    
	    case 'n':
	        /* If nodes specified, don't bother with anything more. */
		el_nodes_ok = FALSE;
		for ( ; *pc++; );
	}
    }
    
    if ( tested != NULL )
        free( tested );
    
    if ( !el_nodes_ok )
    {
	cnt = nodes->cnt;
        for ( j = 1; j < cnt; j++ )
            for ( i = 0; i < 3; i++ )
            {
                if ( xyz[i][j] < bb_lo[i] )
                    bb_lo[i] = xyz[i][j];
                else if ( xyz[i][j] > bb_hi[i] )
                    bb_hi[i] = xyz[i][j];
            }
    }
}


/************************************************************
 * TAG( find_extents )
 *
 * Find geometric extents of nodes referenced by the passed
 * elements.  If material invisibility array "mat_invis" is non-NULL, 
 * only test the nodes of an element it its material is visible.
 */
static void
find_extents( qty_elems, qty_verts, conns, nodes, tested, mat_invis, elem_mat, 
              bb_lo, bb_hi )
int qty_elems;
int qty_verts;
int **conns;
float **nodes;
Bool_type tested[];
int *mat_invis;
int *elem_mat;
float bb_lo[3];
float bb_hi[3];
{
    int i, j, k, nd;
    Bool_type mat_fail;
    
    for ( j = 0; j < qty_elems; j++ )
    {
        mat_fail = ( mat_invis == NULL )
	           ? FALSE
		   : mat_invis[elem_mat[j]];
	
        for ( i = 0; i < qty_verts; i++ )
        {
            nd = conns[i][j];
	    if ( mat_fail || nd < 0 ) /* Should pass & test against node qty too. */
	        continue;
            if ( !tested[nd] )
            {
                tested[nd] = TRUE;
                for ( k = 0; k < 3; k++ )
                {
                    if ( nodes[k][nd] < bb_lo[k] )
                        bb_lo[k] = nodes[k][nd];
                    else if ( nodes[k][nd] > bb_hi[k] )
                        bb_hi[k] = nodes[k][nd];
                }
            }
        }
    }
}


/************************************************************
 * TAG( count_materials )
 *
 * Count the number of materials in the data.
 */
void
count_materials( analy )
Analysis *analy;
{
    Hex_geom *bricks;
    Shell_geom *shells;
    Beam_geom *beams;
    int i, mat_cnt;

    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;
    beams = analy->geom_p->beams;
    mat_cnt = -1;

    if ( bricks )
    {
        for ( i = 0; i < bricks->cnt; i++ )
        {
            if ( bricks->mat[i] > mat_cnt )
                mat_cnt = bricks->mat[i];
        }
    }
    if ( shells )
    {
        for ( i = 0; i < shells->cnt; i++ )
        {
            if ( shells->mat[i] > mat_cnt )
                mat_cnt = shells->mat[i];
        }
    }
    if ( beams )
    {
        for ( i = 0; i < beams->cnt; i++ )
        {
            if ( beams->mat[i] > mat_cnt )
                mat_cnt = beams->mat[i];
        }
    }

    analy->num_materials = mat_cnt + 1;
}


/************************************************************
 * TAG( face_matches_shell )
 *
 * For a specified face on a specified hex element, this routine
 * returns "TRUE" if there is a visible shell element which duplicates
 * the face.  Duplicates in the sense that it has the same four
 * nodes.  The routine doesn't check for vertices which match by
 * position but not by node number.
 */
Bool_type
face_matches_shell( el, face, analy ) 
int el;
int face;
Analysis *analy;
{
    Shell_geom *shells;
    Hex_geom *bricks;
    float *activity;
    int *materials;
    int fnodes[4], snodes[4], tmp;
    int i, j, k, match;

    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;
    materials = shells->mat;
    activity = analy->state_p->activity_present ?
               analy->state_p->shells->activity : NULL;

    /* Get the face nodes & order in ascending order. */
    for ( j = 0; j < 4; j++ )
        fnodes[j] = bricks->nodes[ fc_nd_nums[face][j] ][el];

    for ( j = 0; j < 3; j++ )
        for ( k = j+1; k < 4; k++ )
            if ( fnodes[j] > fnodes[k] )
                SWAP( tmp, fnodes[j], fnodes[k] );

    for ( i = 0; i < shells->cnt; i++ )
    {
        /* Skip this shell if it is hidden or inactive. */
        if ( (activity && activity[i] == 0.0) ||
             analy->hide_material[materials[i]] )
            continue;

        /* Get the shell nodes & order in ascending order. */
        for ( j = 0; j < 4; j++ )
            snodes[j] = shells->nodes[j][i];

        for ( j = 0; j < 3; j++ )
            for ( k = j+1; k < 4; k++ )
                if ( snodes[j] > snodes[k] )
                    SWAP( tmp, snodes[j], snodes[k] );

        /* Test for a match. */
        for ( match = 0, j = 0; j < 4; j++ )
            if ( fnodes[j] == snodes[j] )
                match++; 
        if ( match == 4 )
            return( TRUE );
    }

    return( FALSE );
}


/************************************************************
 * TAG( select_item )
 *
 * Select a node or element from a screen pick.  The position
 * of the pick is in screen coordinates.  The item_type is
 *
 *     0 == Node
 *     1 == Beam element
 *     2 == Shell element
 *     3 == Hex element
 */
int
select_item( item_type, posx, posy, find_only, analy )
int item_type;
int posx;
int posy;
Bool_type find_only;
Analysis *analy;
{
    static char *elem_strs[] = { "node", "beam", "shell", "brick" };
    Nodal *nodes, *onodes;
    Hex_geom *bricks;
    Shell_geom *shells;
    Beam_geom *beams;
    float *onsurf;
    float line_pt[3], line_dir[3], pt[3], near_pt[3], vec[3], v1[3], v2[3];
    float verts[4][3];
    float orig, dist, near_dist;
    Bool_type first, verts_ok;
    char comstr[80];
    int el, fc, nd, near_num;
    int i, j, k;
    int idum;

    nodes = analy->state_p->nodes;
    onodes = analy->geom_p->nodes;
    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;
    beams = analy->geom_p->beams;

    /* Get a ray in the scene from the pick point. */
    get_pick_ray( posx, posy, line_pt, line_dir, analy );
    vec_norm( line_dir );

    if ( item_type == 0 )
    {
        /* Picking a node. */

        /* Mark all nodes that are on the surface of the mesh. */
        onsurf = analy->tmp_result[0];
        for ( i = 0; i < nodes->cnt; i++ )
            onsurf[i] = 0.0;

        for ( i = 0; i < analy->face_cnt; i++ )
        {
            el = analy->face_el[i];
            fc = analy->face_fc[i];

            face_aver_norm( el, fc, analy, vec );

            /* Test for backfacing face. */
            if ( VEC_DOT( vec, line_dir ) > 0 ) 
                continue;

            for ( j = 0; j < 4; j++ )
            {
                nd = bricks->nodes[ fc_nd_nums[fc][j] ][el];
                onsurf[nd] = 1.0;
            }
        }
        if ( shells != NULL )
        {
            for ( i = 0; i < shells->cnt; i++ )
            {
                /* Skip if this material is invisible. */
                if ( analy->hide_material[shells->mat[i]] )
                    continue;

                for ( j = 0; j < 4; j++ )
                    onsurf[shells->nodes[j][i]] = 1.0;
            }
        }
        if ( beams != NULL )
        {
            for ( i = 0; i < beams->cnt; i++ )
            {
                /* Skip if this material is invisible. */
                if ( analy->hide_material[beams->mat[i]] )
                    continue;

                for ( j = 0; j < 2; j++ )
                    onsurf[beams->nodes[j][i]] = 1.0;
            }
        }

        /* Choose node whose distance from line is smallest. */
        first = TRUE;
        for ( i = 0; i < nodes->cnt; i++ )
        {
            if ( onsurf[i] == 1.0 )
            {
                if ( analy->displace_exag )
                {
                    /* Include displacement scaling. */
                    for ( j = 0; j < 3; j++ )
                    {
                        orig = onodes->xyz[j][i];
                        pt[j] = orig + analy->displace_scale[j]*
                                (nodes->xyz[j][i] - orig);
                    }
                }
                else
                {
                    for ( j = 0; j < 3; j++ )
                        pt[j] = nodes->xyz[j][i];
                }
                near_pt_on_line( pt, line_pt, line_dir, near_pt );

                VEC_SUB( vec, pt, near_pt );
                dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                if ( first )
                {
                    near_num = i;
                    near_dist = dist;
                    first = FALSE;
                }
                else if ( dist < near_dist )
                {
                    near_num = i;
                    near_dist = dist;
                }
            }
        }
    }
    else if ( item_type == 1 )
    {
        /* Picking a beam element. */

        if ( beams == NULL )
        {
            parse_command( "clrhil", env.curr_analy );
            wrt_text( "No beam hit\n" );
            return 0;
        }

        first = TRUE;
        for ( i = 0; i < beams->cnt; i++ )
        {
            /* Skip if this material is invisible. */
            if ( analy->hide_material[beams->mat[i]] )
                continue;

            verts_ok = get_beam_verts( i, analy, verts );
	    if ( !verts_ok )
	    {
		wrt_text( "Warning - beam %d ignored; invalid node(s).\n", 
		          i + 1 );
		continue;
	    }

            dist = sqr_dist_seg_to_line( verts[0], verts[1],
                                         line_pt, line_dir );

            if ( first || (dist < near_dist) )
            {
                near_num = i;
                near_dist = dist;
                first = FALSE;
            }
        }

        /* If we didn't hit an element. */
        if ( first )
        {
            parse_command( "clrhil", env.curr_analy );
            wrt_text( "No beam hit\n" );
            return 0;
        }
    }
    else if ( item_type == 2 )
    {
        /* Picking a shell element. */

        if ( shells == NULL )
        {
            parse_command( "clrhil", env.curr_analy );
            wrt_text( "No shell hit\n" );
            return 0;
        }

        first = TRUE;
        for ( i = 0; i < shells->cnt; i++ )
        {
            /* Skip if this material is invisible. */
            if ( analy->hide_material[shells->mat[i]] )
                continue;

            get_shell_verts( i, analy, verts );

            if ( intersect_line_quad( verts, line_pt, line_dir, pt ) )
            {
                VEC_SUB( vec, pt, line_pt );
                dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                if ( first || (dist < near_dist) )
                {
                    near_num = i;
                    near_dist = dist;
                    first = FALSE;
                }
            }
        }

        /* If we didn't hit an element. */
        if ( first )
        {
            parse_command( "clrhil", env.curr_analy );
            wrt_text( "No shell hit\n" );
            return 0;
        }
    }
    else if ( item_type == 3 )
    {
        /* Picking a hex element. */

        if ( bricks == NULL )
        {
            parse_command( "clrhil", env.curr_analy );
            wrt_text( "No brick hit\n" );
            return 0;
        }

        first = TRUE;
        for ( i = 0; i < analy->face_cnt; i++ )
        {
            el = analy->face_el[i];
            fc = analy->face_fc[i];

            get_face_verts( el, fc, analy, verts );

            VEC_SUB( v1, verts[2], verts[0] );
            VEC_SUB( v2, verts[3], verts[1] );
            VEC_CROSS( vec, v1, v2 );
            vec_norm( vec );

            /* Test for backfacing face. */
            if ( VEC_DOT( vec, line_dir ) > 0.0 ) 
                continue;

            if ( intersect_line_quad( verts, line_pt, line_dir, pt ) )
            {
                VEC_SUB( vec, pt, line_pt );
                dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                if ( first || (dist < near_dist) )
                {
                    near_num = el;
                    near_dist = dist;
                    first = FALSE;
                }
            }
        }

        /* If we didn't hit an element. */
        if ( first )
        {
            parse_command( "clrhil", env.curr_analy );
            wrt_text( "No brick hit\n" );
            return 0;
        }
    }
    else
        popup_dialog( WARNING_POPUP, "Illegal button type." );

    if ( !find_only )
    {
        /* Hilite or select the picked element or node. */
        if ( analy->mouse_mode == MOUSE_HILITE )
	    if ( analy->hilite_num == near_num )
		sprintf( comstr, "clrhil" );
	    else
		sprintf( comstr, "hilite %s %d", elem_strs[item_type],
			 near_num + 1 );
        else if ( analy->mouse_mode == MOUSE_SELECT )
	{
	    if ( is_in_iarray( near_num, analy->num_select[item_type], 
	                       analy->select_elems[item_type], &idum ) )
	        sprintf( comstr, "clrsel %s %d", elem_strs[item_type], 
		         near_num + 1 );
	    else
		sprintf( comstr, "select %s %d", elem_strs[item_type],
			 near_num + 1 );
	}

        parse_command( comstr, env.curr_analy );
    }
    
    return near_num + 1;
}


/************************************************************
 * TAG( get_mesh_edges )
 *
 * Generate the mesh edge list for currently displayed
 * bricks and shells.  Doesn't do real edge detection.
 */
void
get_mesh_edges( analy )
Analysis *analy;
{
    if ( analy->m_edges_cnt > 0 )
    {
        free( analy->m_edges[0] );
        free( analy->m_edges[1] );
        free( analy->m_edge_mtl );
    }
    analy->m_edges_cnt = 0;

    if ( analy->geom_p->bricks != NULL )
        get_brick_edges( analy );
    if ( analy->geom_p->shells != NULL )
        get_shell_edges( analy );
}


/************************************************************
 * TAG( get_brick_edges )
 *
 * Generate the mesh edge list for the currently displayed
 * faces.  Looks for boundaries of material regions and
 * for crease edges which exceed a threshold angle.
 */
static void
get_brick_edges( analy )
Analysis *analy;
{
    Hex_geom *bricks;
    float norm[3], norm_next[3], dot;
    int *edge_tbl[3];
    int *m_edges[2], *m_edge_mtl;
    int *new_edges[2], *new_edge_mtl;
    int *ord;
    int *mat;
    int el, fc, el_next, fc_next, node1, node2, tmp;
    int fcnt, ecnt, m_edges_cnt;
    int i, j;

    if ( analy->face_cnt == 0 )
        return;

    bricks = analy->geom_p->bricks;

    /* Create the edge table. */
    fcnt = analy->face_cnt;
    ecnt = fcnt*4;
    for ( i = 0; i < 3; i++ )
        edge_tbl[i] = NEW_N( int, ecnt, "Brick edge table" );

    /*
     * The edge_tbl contains for each edge
     *
     *     node1 node2 face
     */
    for ( i = 0; i < fcnt; i++ )
    {
        el = analy->face_el[i];
        fc = analy->face_fc[i];

        for ( j = 0; j < 4; j++ )
        {
            node1 = bricks->nodes[ fc_nd_nums[fc][j] ][el];
            node2 = bricks->nodes[ fc_nd_nums[fc][(j+1)%4] ][el];

            /* Order the node numbers in ascending order. */
            if ( node1 > node2 )
                SWAP( tmp, node1, node2 );

            edge_tbl[0][i*4 + j] = node1;
            edge_tbl[1][i*4 + j] = node2;
            edge_tbl[2][i*4 + j] = i;
        }
    }

    /* Sort the edge table. */
    ord = NEW_N( int, ecnt, "Sort ordering table" );
    edge_heapsort( ecnt, edge_tbl, ord );

    /* Put boundary edges in the edge list. */
    m_edges[0] = NEW_N( int, ecnt, "Brick edges" );
    m_edges[1] = NEW_N( int, ecnt, "Brick edges" );
    m_edge_mtl = NEW_N( int, ecnt, "Brick edge materials" );
    m_edges_cnt = 0;
    mat = bricks->mat;

    /* For brick faces, all edges should be in the sorted list twice. */
    for ( i = 0; i < ecnt - 1; i++ )
    {
        j = edge_tbl[2][ord[i]];
        el = analy->face_el[j];
        fc = analy->face_fc[j];

        if ( edge_node_cmp( ord[i], ord[i+1], edge_tbl ) == 0 )
        {
            j = edge_tbl[2][ord[i+1]];
            el_next = analy->face_el[j];
            fc_next = analy->face_fc[j];

            if ( mat[el] != mat[el_next] )
            {
                /* Edge detected for material boundary. */
                m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                m_edge_mtl[m_edges_cnt] = mat[el];
                m_edges_cnt++;
            }
            else
            {
                /* Get face normals for both faces. */
                face_aver_norm( el, fc, analy, norm );
                face_aver_norm( el_next, fc_next, analy, norm_next );
                vec_norm( norm );
                vec_norm( norm_next );

                /*
                 * Dot product test.
                 */
                dot = VEC_DOT( norm, norm_next );

                /*
                 * Magical constant.  An edge is detected when the angle
                 * between normals is greater than a critical angle.
                 */
                if ( dot < explicit_threshold )
                {
                    m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                    m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                    m_edge_mtl[m_edges_cnt] = mat[el];
                    m_edges_cnt++;
                }
            }

            /* Skip past the redundant edge. */
            i++;
        }
        else
            popup_dialog( WARNING_POPUP, 
	                  "Unexpected behavior in brick edge detection." );
    }

    /* Check for unused last edge. */
    if ( i < ecnt )
        popup_dialog( WARNING_POPUP, 
	              "Unexpected behavior in brick edge detection." );

    for ( i = 0; i < 3; i++ )
        free( edge_tbl[i] );
    free( ord );

    /* Copy the brick edges into analy's arrays. */
    analy->m_edges_cnt = m_edges_cnt;
    analy->m_edges[0] = NEW_N( int, m_edges_cnt, "Mesh edges" );
    analy->m_edges[1] = NEW_N( int, m_edges_cnt, "Mesh edges" );
    analy->m_edge_mtl = NEW_N( int, m_edges_cnt, "Mesh edge material" );

    for ( i = 0; i < m_edges_cnt; i++ )
    {
        analy->m_edges[0][i] = m_edges[0][i];
        analy->m_edges[1][i] = m_edges[1][i];
        analy->m_edge_mtl[i] = m_edge_mtl[i];
    }

    free( m_edges[0] );
    free( m_edges[1] );
    free( m_edge_mtl );
}


/************************************************************
 * TAG( old_get_brick_edges )
 *
 * Generate the mesh edge list for the currently displayed
 * faces.  This is sort of a rough way of picking out some
 * of the edges of hex elements.  It doesn't do real edge
 * detection.
 */
static void
old_get_brick_edges( analy )
Analysis *analy;
{
    Hex_geom *bricks;
    int **hex_adj;
    int *hex_visib;
    int nd, vis[6], vis_cnt, edge_cnt;
    int m1, m2;
    int i, j;

    bricks = analy->geom_p->bricks;
    hex_adj = analy->hex_adj;
    hex_visib = analy->hex_visib;
    edge_cnt = 0;

    /*
     * Loop through the brick elements.  At each brick, check whether
     * each of its faces is visible.  Then go through the edges:  if
     * the faces on both sides of an edge are visible, then the edge is
     * added to the edge list for display.  The first pass just counts
     * edges; the second pass actually stores them.
     */
    for ( i = 0; i < bricks->cnt; i++ )
    {
        if ( hex_visib[i] )
        {
            vis_cnt = 0;
            for ( j = 0; j < 6; j++ )
            {  
                if ( hex_adj[j][i] < 0 || !hex_visib[ hex_adj[j][i] ] )
                {
                    vis[j] = TRUE;
                    vis_cnt++;
                }
                else if ( analy->translate_material )
                {
                    /* Material translations can expose faces. */
                    m1 = bricks->mat[i];
                    m2 = bricks->mat[ hex_adj[j][i] ];
                    if ( analy->mtl_trans[0][m1] != analy->mtl_trans[0][m2] ||
                         analy->mtl_trans[1][m1] != analy->mtl_trans[1][m2] ||
                         analy->mtl_trans[2][m1] != analy->mtl_trans[2][m2] )
                    {
                        vis[j] = TRUE;
                        vis_cnt++;
                    }
                    else
                        vis[j] = FALSE;
                }
                else
                    vis[j] = FALSE;
            }

            if ( vis_cnt > 1 )
                for ( j = 0; j < 12; j++ )
                    if ( vis[ edge_face_nums[j][0] ] &&
                         vis[ edge_face_nums[j][1] ] )
                        edge_cnt++;
        }
    }

    analy->m_edges_cnt = edge_cnt;
    analy->m_edges[0] = NEW_N( int, edge_cnt, "Mesh edges" );
    analy->m_edges[1] = NEW_N( int, edge_cnt, "Mesh edges" );
    analy->m_edge_mtl = NEW_N( int, edge_cnt, "Mesh edge material" );
    edge_cnt = 0;

    for ( i = 0; i < bricks->cnt; i++ )
    {
        if ( hex_visib[i] )
        {
            vis_cnt = 0;
            for ( j = 0; j < 6; j++ )
            {  
                if ( hex_adj[j][i] < 0 || !hex_visib[ hex_adj[j][i] ] )
                {
                    vis[j] = TRUE;
                    vis_cnt++;  
                }   
                else if ( analy->translate_material )
                {
                    /* Material translations can expose faces. */
                    m1 = bricks->mat[i];
                    m2 = bricks->mat[ hex_adj[j][i] ];
                    if ( analy->mtl_trans[0][m1] != analy->mtl_trans[0][m2] ||
                         analy->mtl_trans[1][m1] != analy->mtl_trans[1][m2] ||
                         analy->mtl_trans[2][m1] != analy->mtl_trans[2][m2] )
                    {
                        vis[j] = TRUE;
                        vis_cnt++;
                    }
                    else
                        vis[j] = FALSE;
                }
                else
                    vis[j] = FALSE;
            }
 
            if ( vis_cnt > 1 )
                for ( j = 0; j < 12; j++ )
                    if ( vis[ edge_face_nums[j][0] ] &&
                         vis[ edge_face_nums[j][1] ] )
                    {
                        analy->m_edge_mtl[edge_cnt] = bricks->mat[i];
                        nd = bricks->nodes[ edge_node_nums[j][0] ][i];
                        analy->m_edges[0][edge_cnt] = nd;
                        nd = bricks->nodes[ edge_node_nums[j][1] ][i];
                        analy->m_edges[1][edge_cnt] = nd;
                        edge_cnt++; 
                    }
        }
    }
}


/************************************************************
 * TAG( get_shell_edges )
 *
 * Generate the edge list for shell elements.  Looks for the
 * boundaries of the shell regions, and adds those edges to
 * the list.
 */
static void
get_shell_edges( analy )
Analysis *analy;
{
    Shell_geom *shells;
    Bool_type vis_this, vis_next;
    float *activity;
    float norm_this[3], norm_next[3], dot;
    int *edge_tbl[3];
    int *m_edges[2], *m_edge_mtl;
    int *new_edges[2], *new_edge_mtl;
    int *ord;
    int *mat;
    int node1, node2, tmp, el_this, el_next;
    int i, j, scnt, ecnt, m_edges_cnt;

    shells = analy->geom_p->shells;
    activity = analy->state_p->activity_present ?
               analy->state_p->shells->activity : NULL;

    /* Create the edge table. */
    scnt = shells->cnt;
    ecnt = scnt*4;
    for ( i = 0; i < 3; i++ )
        edge_tbl[i] = NEW_N( int, ecnt, "Shell edge table" );

    /*
     * The edge_tbl contains for each edge
     *     node1 node2 element
     */
    for ( i = 0; i < scnt; i++ )
    {
        for ( j = 0; j < 4; j++ )
        {
            node1 = shells->nodes[j][i];
            node2 = shells->nodes[(j+1)%4][i];

            /* Order the node numbers in ascending order. */
            if ( node1 > node2 )
                SWAP( tmp, node1, node2 );

            edge_tbl[0][i*4 + j] = node1;
            edge_tbl[1][i*4 + j] = node2;
            edge_tbl[2][i*4 + j] = i;
        }
    }

    /* Sort the edge table. */
    ord = NEW_N( int, ecnt, "Sort ordering table" );
    edge_heapsort( ecnt, edge_tbl, ord );

    /* Put boundary edges in the edge list. */
    m_edges[0] = NEW_N( int, ecnt, "Shell edges" );
    m_edges[1] = NEW_N( int, ecnt, "Shell edges" );
    m_edge_mtl = NEW_N( int, ecnt, "Shell edge materials" );
    m_edges_cnt = 0;
    mat = shells->mat;

    for ( i = 0; i < ecnt - 1; i++ )
    {
        el_this = edge_tbl[2][ord[i]];
        vis_this = TRUE;
        if ( ( activity && activity[el_this] == 0.0 )
	     || analy->hide_material[mat[el_this]] )
            vis_this = FALSE;

        if ( edge_node_cmp( ord[i], ord[i+1], edge_tbl ) == 0 )
        {
            el_next = edge_tbl[2][ord[i+1]];
            vis_next = TRUE;
            if ( ( activity && activity[el_next] == 0.0 )
	         || analy->hide_material[mat[el_this]] )
                vis_next = FALSE;

            if ( ( vis_this && vis_next && mat[el_this] != mat[el_next] ) ||
                 ( vis_this && !vis_next ) || ( !vis_this && vis_next ) )
            {
                /* Edge detected for material or surface boundary. */
                m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                if ( !vis_next )
                    m_edge_mtl[m_edges_cnt] = mat[el_this];
                else if ( !vis_this )
                    m_edge_mtl[m_edges_cnt] = mat[el_next];
                else
                    m_edge_mtl[m_edges_cnt] = mat[el_this];
                m_edges_cnt++;
            }
            else if ( vis_this && vis_next )
            {
                /* Get face normals for both faces. */
                shell_aver_norm( el_this, analy, norm_this );
                shell_aver_norm( el_next, analy, norm_next );
                vec_norm( norm_this );
                vec_norm( norm_next );

                /*
                 * Dot product test.
                 */
                dot = VEC_DOT( norm_this, norm_next );

                /*
                 * Magical constant.  An edge is detected when the angle
                 * between normals is greater than a critical angle.
		 * 
		 * Need absolute value of dot product for shells because
		 * normals can be parallel but opposite.
                 */
                if ( fabs( dot ) < explicit_threshold )
                {
                    m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                    m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                    m_edge_mtl[m_edges_cnt] = mat[el_this];
                    m_edges_cnt++;
                }
            }

            /* Skip past the redundant edge. */
            i++;
        }
        else if ( vis_this )
        {
            m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
            m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
            m_edge_mtl[m_edges_cnt] = mat[el_this];
            m_edges_cnt++;
        }
    }

    /* Pick up the last edge. */
    if ( i < ecnt )
    {
        if ( !( (activity && activity[edge_tbl[2][ord[i]]] == 0.0)
	        || analy->hide_material[mat[el_this]] ) )
        {
            m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
            m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
            m_edge_mtl[m_edges_cnt] = mat[ edge_tbl[2][ord[i]] ];
            m_edges_cnt++;
        }
    }

    for ( i = 0; i < 3; i++ )
        free( edge_tbl[i] );
    free( ord );

    /* Copy the shell edges into analy's arrays. */
    if ( m_edges_cnt > 0 )
    {
        ecnt = analy->m_edges_cnt + m_edges_cnt;
        new_edges[0] = NEW_N( int, ecnt, "Mesh edges" );
        new_edges[1] = NEW_N( int, ecnt, "Mesh edges" );
        new_edge_mtl = NEW_N( int, ecnt, "Mesh edge materials" );

        for ( i = 0; i < m_edges_cnt; i++ )
        {
            new_edges[0][i] = m_edges[0][i];
            new_edges[1][i] = m_edges[1][i];
            new_edge_mtl[i] = m_edge_mtl[i];
        }

        for ( i = 0, ecnt = m_edges_cnt; i < analy->m_edges_cnt; i++, ecnt++ )
        {
            new_edges[0][ecnt] = analy->m_edges[0][i];
            new_edges[1][ecnt] = analy->m_edges[1][i];
            new_edge_mtl[ecnt] = analy->m_edge_mtl[i];
        }

        if ( analy->m_edges_cnt > 0 )
        {
            free( analy->m_edges[0] );
            free( analy->m_edges[1] );
            free( analy->m_edge_mtl );
        }

        analy->m_edges_cnt = ecnt;
        analy->m_edges[0] = new_edges[0];
        analy->m_edges[1] = new_edges[1];
        analy->m_edge_mtl = new_edge_mtl;
    }

    free( m_edges[0] );
    free( m_edges[1] );
    free( m_edge_mtl );
}


/*************************************************************
 * TAG( edge_heapsort )
 *
 * Heap sort, from Numerical_Recipes_in_C.  The node list array
 * is arrin and the sorted ordering is returned in the indx array.
 */
static void
edge_heapsort( n, arrin, indx )
int n;
int *arrin[3];
int *indx;
{
    int l, j, ir, indxt, i;

    for ( j = 0; j < n; j++ )
        indx[j] = j;

    l = n/2;
    ir = n - 1;

    for (;;)
    {
    	if ( l > 0 )
        {
            --l;
            indxt = indx[l];
        }
	else
        {
	    indxt = indx[ir];
    	    indx[ir] = indx[0];
            --ir;
	    if ( ir == 0 )
            {
		indx[0] = indxt;
		return;
	    }
        }

	i = l;
        j = 2*l + 1;
	while ( j <= ir ) 
        {
	    if ( j < ir && edge_node_cmp( indx[j], indx[j+1], arrin ) < 0 )
                j++;
	    if ( edge_node_cmp( indxt, indx[j], arrin ) < 0 )
            {
		indx[i] = indx[j];
                i = j;
                j = 2*j + 1;
	    }
	    else
                j = ir + 1;
	}
	indx[i] = indxt;
    }
}


/************************************************************
 * TAG( edge_node_cmp )
 *
 * Compare two sets of nodes in an array.  Called by the sorting
 * routine to sort shell edges.  Assumes that the node numbers
 * are stored in the first two entries of n_arr.
 */
static int
edge_node_cmp( indx1, indx2, n_arr )
int indx1, indx2;
int *n_arr[3];
{
    int i;

    for ( i = 0; i < 2; i++ )
    {
        if ( n_arr[i][indx1] < n_arr[i][indx2] )
            return -1;
        else if ( n_arr[i][indx1] > n_arr[i][indx2] )
            return 1;
    }

    /* Entries match.
     */
    return 0;
}


/************************************************************
 * TAG( create_elem_blocks )
 *
 * Group elements into smaller blocks in order to make face
 * table generation and vector drawing routines more efficient.
 */
void
create_elem_blocks( analy )
Analysis *analy;
{
    Nodal *nodes;
    Hex_geom *bricks;
    Shell_geom *shells;
    int block_sz, block_cnt;
    int lo, hi, nd;
    int i, j, k, m;

    nodes = analy->geom_p->nodes;

    if ( analy->dimension == 3 && analy->geom_p->bricks != NULL )
    {
        bricks = analy->geom_p->bricks;

        if ( bricks->cnt > 5000 )
        {
            block_sz = 2 * (int) sqrt( (double) bricks->cnt );
            block_cnt = bricks->cnt / block_sz;
            if ( bricks->cnt % block_sz > 0 )
                block_cnt++;
        }
        else
        {
            /* For small grids, just use one big block. */
            block_cnt = 1;
            block_sz = bricks->cnt;
        }

        /* Allocate block tables. */
        analy->num_blocks = block_cnt;
        analy->block_lo = NEW_N( int, block_cnt, "Blocks low elem" );
        analy->block_hi = NEW_N( int, block_cnt, "Blocks high elem" );
        for ( i = 0; i < 2; i++ )
            for ( j = 0; j < 3; j++ )
                analy->block_bbox[i][j] =
                        NEW_N( float, block_cnt, "Block bboxes" );

        /* Get low and high element of each block. */
        lo = 0;
        hi = block_sz - 1;
        for ( i = 0; i < block_cnt; i++ )
        {
            if ( hi >= bricks->cnt )
                hi = bricks->cnt - 1;

            analy->block_lo[i] = lo;
            analy->block_hi[i] = hi;

            lo = hi + 1;
            hi = lo + block_sz - 1;
        }

        /* Get a bbox for all nodes in each block. */
        for ( i = 0; i < block_cnt; i++ )
        {
            /* Initialize min and max. */
            nd = bricks->nodes[0][ analy->block_lo[i] ];
            for ( k = 0; k < 3; k++ )
            {
                analy->block_bbox[0][k][i] = nodes->xyz[k][nd];
                analy->block_bbox[1][k][i] = nodes->xyz[k][nd];
            }

            for ( j = analy->block_lo[i]; j <= analy->block_hi[i]; j++ )
            {
                for ( m = 0; m < 8; m++ )
                {
                    nd = bricks->nodes[m][j];
                    for ( k = 0; k < 3; k++ )
                    {
                        if ( nodes->xyz[k][nd] < analy->block_bbox[0][k][i] )
                            analy->block_bbox[0][k][i] = nodes->xyz[k][nd];
                        if ( nodes->xyz[k][nd] > analy->block_bbox[1][k][i] )
                            analy->block_bbox[1][k][i] = nodes->xyz[k][nd];
                    }
                }
            }
        }
    }
    else if ( analy->dimension == 2 && analy->geom_p->shells != NULL )
    {
        /*
         * Blocking for 2D shell data is currently only used to
         * speed up the 2D vector drawing routines.
         */

        shells = analy->geom_p->shells;

        if ( shells->cnt > 1000 )
        {
            block_sz = (int) sqrt( (double) shells->cnt );
            block_cnt = shells->cnt / block_sz;
            if ( shells->cnt % block_sz > 0 )
                block_cnt++;
        }
        else
        {
            /* For small grids, just use one big block. */
            block_cnt = 1;
            block_sz = shells->cnt;
        }

        /* Allocate block tables. */
        analy->num_blocks = block_cnt;
        analy->block_lo = NEW_N( int, block_cnt, "Blocks low elem" );
        analy->block_hi = NEW_N( int, block_cnt, "Blocks high elem" );
        for ( i = 0; i < 2; i++ )
            for ( j = 0; j < 3; j++ )
                analy->block_bbox[i][j] =
                        NEW_N( float, block_cnt, "Block bboxes" );

        /* Get low and high element of each block. */
        lo = 0;
        hi = block_sz - 1;
        for ( i = 0; i < block_cnt; i++ )
        {
            if ( hi >= shells->cnt )
                hi = shells->cnt - 1;

            analy->block_lo[i] = lo;
            analy->block_hi[i] = hi;

            lo = hi + 1;
            hi = lo + block_sz - 1;
        }

        /* Get a bbox for all nodes in each block. */
        for ( i = 0; i < block_cnt; i++ )
        {
            /* Initialize min and max. */
            nd = shells->nodes[0][ analy->block_lo[i] ];
            for ( k = 0; k < 2; k++ )
            {
                analy->block_bbox[0][k][i] = nodes->xyz[k][nd];
                analy->block_bbox[1][k][i] = nodes->xyz[k][nd];
            }

            for ( j = analy->block_lo[i]; j <= analy->block_hi[i]; j++ )
            {
                for ( m = 0; m < 4; m++ )
                {
                    nd = shells->nodes[m][j];
                    for ( k = 0; k < 2; k++ )
                    {
                        if ( nodes->xyz[k][nd] < analy->block_bbox[0][k][i] )
                            analy->block_bbox[0][k][i] = nodes->xyz[k][nd];
                        if ( nodes->xyz[k][nd] > analy->block_bbox[1][k][i] )
                            analy->block_bbox[1][k][i] = nodes->xyz[k][nd];
                    }
                }
            }
        }
    }
/*
fprintf( stderr, "\nNumber of blocks: %d\n\n", analy->num_blocks );
*/
}


/************************************************************
 * TAG( write_ref_file )
 *
 * Write out the current face table (or a subset thereof)
 * as a reference surface file.
 */
void
write_ref_file( tokens, token_cnt, analy )
char tokens[MAXTOKENS][TOKENLENGTH];
int token_cnt;
Analysis *analy;
{
    FILE *ofile;
    int i, j, k;
    int **nodes;
    int fc, el;
    float *p_f;
    float verts[4][3];
    float *xyz_cons[3];
    float con;
    float *constraints;
    int xyz_qtys[3];
    static char xyz_chars[] = { 'x', 'y', 'z' };
    int c_qty, out_qty;

    if ( token_cnt == 1 )
    {
	popup_dialog( USAGE_POPUP, "outref <filename> [x|y|z <coord>]..." );
	return;
    }
    
    nodes = analy->geom_p->bricks->nodes;
    
    ofile = fopen( tokens[1], "w" );
    if ( ofile == NULL )
    {
	popup_dialog( WARNING_POPUP, "Unable to open file \"%s\".", tokens[1] );
	return;
    }
    
    if ( token_cnt == 2 )
    {
	/* No constraints; dump all external faces. */
	
	/* Face count. */
	fprintf( ofile, "%d\n", analy->face_cnt );
	    
	for ( i = 0; i < analy->face_cnt; i++ )
	{
	    el = analy->face_el[i];
	    fc = analy->face_fc[i];
    
	    fprintf( ofile, "%d %d %d %d\n", 
		     nodes[ fc_nd_nums[fc][0] ][el] + 1, 
		     nodes[ fc_nd_nums[fc][1] ][el] + 1, 
		     nodes[ fc_nd_nums[fc][2] ][el] + 1, 
		     nodes[ fc_nd_nums[fc][3] ][el] + 1 );
	}
    }
    else
    {
	/* Extract constraints. */
	c_qty = (token_cnt - 2) / 2;
	if ( c_qty * 2 + 2 != token_cnt )
	{
	    popup_dialog( USAGE_POPUP, "outref <filename> [x|y|z <coord>]..." );
	    return;
	}
	
	constraints = NEW_N( float, c_qty, "Outref constraint list" );
	
	/* Loop to find x, y, and z constraints sequentially. */
	p_f = constraints;
	for ( i = 0; i < 3; i++ )
	{
	    xyz_cons[i] = p_f;
	    xyz_qtys[i] = 0;
	    
	    /* Scan entire constraints list for current coord constraints. */
	    for ( j = 2; j < token_cnt; j += 2 )
	        if ( tokens[j][0] == xyz_chars[i] )
		{
		    /* Found one.  Save constraint value. */
		    *p_f++ = (float) atof( tokens[j + 1] );
		    xyz_qtys[i]++;
		}
	}
	
	if ( p_f - constraints != c_qty )
	{
	    free( constraints );
	    popup_dialog( USAGE_POPUP, "outref <filename> [x|y|z <coord>]..." );
	    return;
	}
	
	/*
	 * Now write the file.
	 */
	
	/* Write a blank first line since we don't know the count yet. */
	fwrite( "          \n", 1, 11, ofile );
	
	/* Loop over external faces. */
	out_qty = 0;
	for ( i = 0; i < analy->face_cnt; i++ )
	{
	    el = analy->face_el[i];
	    fc = analy->face_fc[i];
	    
            get_face_verts( el, fc, analy, verts );
	    
	    /* For each dimension... */
	    for ( j = 0; j < 3; j++ )
	    {
	        /* For each constraint for this dimension... */
		for ( k = 0; k < xyz_qtys[j]; k++ )
		{
		    con = xyz_cons[j][k];
		    
		    /* Compare coord for all four nodes. */
		    if ( fabs( verts[0][j] - con ) < 0.001
		         && fabs( verts[1][j] - con ) < 0.001
		         && fabs( verts[2][j] - con ) < 0.001
		         && fabs( verts[3][j] - con ) < 0.001 )
		    {
			/* Face matches constraint; write it out. */
			fprintf( ofile, "%d %d %d %d\n", 
				 nodes[ fc_nd_nums[fc][0] ][el] + 1, 
				 nodes[ fc_nd_nums[fc][1] ][el] + 1, 
				 nodes[ fc_nd_nums[fc][2] ][el] + 1, 
				 nodes[ fc_nd_nums[fc][3] ][el] + 1 );
			
			out_qty++;
		    }
		}
	    }
    
	}
	
	/* Now overwrite the first line with the correct face count. */
	fseek( ofile, 0, SEEK_SET );
	fprintf( ofile, "%d", out_qty );
    }
    
    free( constraints );
    
    fclose( ofile );
}


