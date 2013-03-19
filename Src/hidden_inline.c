
/* $Id$ */

/* 
 ***************************************************************
 * hidden_inline.c - Create a hidden line drawing of a mesh.
 *
 * This is a new inline version of the standalone version 
 * of hidden.c. This version will run directly from GRIZ when
 * the command outhid is executed.
 ***************************************************************
 * 	Donald J. Dovey
 * 	Lawrence Livermore National Laboratory
 * 	Sep 16 1993
 *
 * Modified:
 *      I.R. Corey: 10/12/04: Added input and output filenames 
 *      to command line. Also added help option.
 *
 *
 * Copyright (c) 1993 Regents of the University of California
 ***************************************************************
 */

#include <time.h>
#include <stdlib.h>
#include "misc.h"
#include "list.h"
#include "geometric.h"

#define PS_ACTIVE_PATH_LIMIT (1500)

typedef double Real_type;

extern char *getenv();

/*
 * Edge fragment.
 */
typedef struct _Edge_frag
{
    struct _Edge_frag *next;
    struct _Edge_frag *prev;
    Real_type line_pt[3];
    Real_type line_dir[3];
    Real_type proj_pts[2][2];
    Real_type bounds[2];          /* Parametric endpoints of segment. */
} Edge_frag;


/*
 * Drawn edge.
 */
typedef struct _Drawn_edge
{
    struct _Drawn_edge *next;
    struct _Drawn_edge *prev;
    Real_type pts[2][2];
} Drawn_edge;


Bool_type perspective;
Real_type view_aspect;
Real_type view_eyepoint[3];
int node_cnt;
int one_poly_cnt;
int two_poly_cnt;
int line_seg_cnt;
Real_type *nodes[3];
Real_type *proj_nodes[2];
int *one_poly[4];                 /* One-sided polygons. */
int *two_poly[4];                 /* Two-sided polygons. */
int *line_seg[2];                 /* Line segments. */

int edge_cnt;
int poly_cnt;
int *edge[2];
int *hidden_poly[4];
Real_type *poly_min[2];
Real_type *poly_max[2];

Drawn_edge *draw_edge_list;
Bool_type landscape;
Bool_type portrait;

Bool_type verbose;

/* Routine declarations. */
static void read_token( FILE *infile, char *token, int max_length );
static int read_int( FILE *infile );
static Real_type read_real( FILE *infile );
static void read_hid_file( char *infile );
static void create_poly_table();
static void create_edge_table();
static void bbox_polys();
static void gen_hidden();
static void output_hidden( char *outfile );
static void hidden_vec_norm( Real_type *vec );
static void hidden_point_transform( Real_type *res, Real_type *vec, Transf_mat *mat);
static void griz_heapsort( int n, int *arrin[2], int *indx );
static int node_cmp( int indx1, int indx2, int *n_arr[2] );
static Real_type par_test_pt_plane( Real_type plane_pt[3], 
                                    Real_type plane_norm[3], 
                                    Real_type test_pt[3] );
static int clip_line_seg_plane( Real_type plane_pt[3], Real_type plane_norm[3], 
                                Real_type line_pt[3], Real_type line_dir[3], 
                                Real_type bounds[2] );
static void copy_frag( Edge_frag *fr_frag, Edge_frag *to_frag );
static void project_frag_ends( Edge_frag *frag );
static int shadow_edge_poly( int npoly, Edge_frag *frag, Edge_frag **nfrag2 );



/*****************************************************************
 * TAG( hidden_inline )
 * 
 * General purpose routine to read a token (delimited by white space)
 * from a file.
 */
void hidden_inline ( char *input_filename )
{
    char output_filename[64];

    landscape = FALSE;
    portrait  = FALSE;
    verbose   = FALSE;

    strcpy(output_filename, input_filename);
    strcat(output_filename, ".ps");

    read_hid_file(  input_filename );
    create_poly_table();
    create_edge_table();
    bbox_polys();
    gen_hidden();
    output_hidden( output_filename );
}

/*****************************************************************
 * TAG( read_token )
 * 
 * General purpose routine to read a token (delimited by white space)
 * from a file.
 */
static void
read_token( FILE *infile, char *token, int max_length )
{
    int i;
    char c;

    /* Eat up any preliminary white space. */
    c = getc( infile );
    while ( !feof( infile ) && isspace( c ) )
        c = getc( infile );

    /* Get the token. */
    i = 0;
    while ( !feof( infile ) && !isspace( c ) && i < max_length-1 )
    {
        token[i] = c;
        i++;
        c = getc( infile );
    }

    token[i] = '\0';
}


/*****************************************************************
 * TAG( read_int )
 *
 * Read an integer from a file.
 */
static int
read_int( FILE *infile )
{
    char token[80];
    int val;

    read_token( infile, token, 80 );
    sscanf( token, "%d", &val );
    return val;
}


/*****************************************************************
 * TAG( read_real )
 *
 * Read a real number from a file.
 */
static Real_type
read_real( FILE *infile )
{
    char token[80];
    Real_type val;

    read_token( infile, token, 80 );
    if ( sizeof( val ) == 8 )
        sscanf( token, "%lf", &val );
    else
        sscanf( token, "%lf", &val );

    return val;
}


/************************************************************
 * TAG( read_hid_file )
 *
 * Read in mesh data from a .hid file.
 */
static void
read_hid_file( char *filename )
{
    Transf_mat view_mat;
    char token[80];
    Real_type pt[3], npt[3];
    int i, j;
    FILE *infile;

    if ( ( infile = fopen(filename, "r") ) == NULL )
    {
        fprintf( stderr, "Couldn't open file %s.\n", filename );
        exit( 1 );
    }

    /*
     * GRIZ viewing parameters.
     *
     * The viewing plane is at z = 1.0.
     * The eyepoint is at ( 0.0, 0.0, d ) where d = 20.0 is the default.
     * The view direction is along the negative Z axis.
     * The viewport size is ( 2*view_aspect, 2*1.0 ) if view_aspect >= 1.0
     *     and ( 2*1.0, 2 / view_aspect ) if view_aspect < 1.0.
     * The viewport is centered about ( 0.0, 0.0 ).
     */

    /* Get "PERSPECTIVE" or "ORTHOGRAPHIC". */
    read_token( infile, token, 80 );
    if ( strcmp( token, "PERSPECTIVE" ) == 0 )
        perspective = TRUE;
    else
        perspective = FALSE;

    /* Get "VIEWPORT_ASPECT". */
    read_token( infile, token, 80 );
    view_aspect = read_real( infile );

    /* Get "VIEW_EYEPOINT". */
    read_token( infile, token, 80 );
    for ( i = 0; i < 3; i++ )
        view_eyepoint[i] = read_real( infile );

    /* Get "VIEW_MATRIX". */
    read_token( infile, token, 80 );
    for ( i = 0; i < 4; i++ )
        for ( j = 0; j < 4; j++ )
            view_mat.mat[i][j] = read_real( infile );

    /* Get nodes. */
    read_token( infile, token, 80 );
    node_cnt = read_int( infile );
    for ( i = 0; i < 3; i++ )
        nodes[i] = NEW_N( Real_type, node_cnt, "Nodes" );
    node_cnt = 0;
    read_token( infile, token, 80 );
    while ( strcmp( token, "v" ) == 0 )
    {
        for ( i = 0; i < 3; i++ )
            pt[i] = read_real( infile );

        hidden_point_transform( npt, pt, &view_mat );

        for ( i = 0; i < 3; i++ )
            nodes[i][node_cnt] = npt[i];

        node_cnt++;
        read_token( infile, token, 80 );
    }

    /* Get one-sided polygons. */
    one_poly_cnt = read_int( infile );
    read_token( infile, token, 80 );
    if ( one_poly_cnt > 0 )
    {
        for ( i = 0; i < 4; i++ )
            one_poly[i] = NEW_N( int, one_poly_cnt, "One-sided polygons" );
        one_poly_cnt = 0;
        while ( strcmp( token, "f" ) == 0 )
        {
            for ( i = 0; i < 4; i++ )
                one_poly[i][one_poly_cnt] = read_int( infile );
            one_poly_cnt++;
            read_token( infile, token, 80 );
        }
    }

    /* Get two-sided polygons. */
    two_poly_cnt = read_int( infile );
    read_token( infile, token, 80 );
    if ( two_poly_cnt > 0 )
    {
        for ( i = 0; i < 4; i++ )
            two_poly[i] = NEW_N( int, two_poly_cnt, "Two-sided polygons" );
        two_poly_cnt = 0;
        while ( strcmp( token, "f" ) == 0 )
        {
            for ( i = 0; i < 4; i++ )
                two_poly[i][two_poly_cnt] = read_int( infile );
            two_poly_cnt++;
            read_token( infile, token, 80 );
        }
    }

    /* Get line segments. */
    line_seg_cnt = read_int( infile );
    if ( line_seg_cnt > 0 )
    {
        for ( i = 0; i < 4; i++ )
            line_seg[i] = NEW_N( int, line_seg_cnt, "Line segments" );
        line_seg_cnt = 0;
        read_token( infile, token, 80 );
        while ( strcmp( token, "b" ) == 0 )
        {
            line_seg[0][line_seg_cnt] = read_int( infile );
            line_seg[1][line_seg_cnt] = read_int( infile );
            line_seg_cnt++;
            read_token( infile, token, 80 );
        }
    }

    if ( verbose )
    {
        fprintf( stderr, "\nHIDDEN\n" );
        fprintf( stderr, "Num nodes: %d\n", node_cnt );
        fprintf( stderr, "Num one-sided polys: %d\n", one_poly_cnt );
        fprintf( stderr, "Num two-sided polys: %d\n", two_poly_cnt );
        fprintf( stderr, "Num lines: %d\n", line_seg_cnt );
    }
}


/************************************************************
 * TAG( create_poly_table )
 *
 * Create the polygon table, eliminating back-facing polygons.
 */
static void
create_poly_table()
{
    Real_type verts[4][3];
    Real_type v1[3], v2[3], v3[3];
    int tmp;
    int i, j, k;

    /* Cull back-facing one-sided polygons. */
    for ( poly_cnt = 0, i = 0; i < one_poly_cnt; i++ )
    {
        for ( j = 0; j < 4; j++ )
            for ( k = 0; k < 3; k++ )
                verts[j][k] = nodes[k][one_poly[j][i]];

        VEC_SUB( v1, verts[0], verts[1] );
        VEC_SUB( v2, verts[2], verts[1] );
        VEC_CROSS( v3, v2, v1 );

        if ( perspective )
        {
            VEC_SUB( v1, view_eyepoint, verts[1] );
        }
        else
        {
            VEC_SET( v1, 0.0, 0.0, 1.0 );
        }

        if( VEC_DOT( v3, v1 ) < 0.0 )
        {
            /* Discard this polygon (mark it for discarding). */
            one_poly[0][i] = -1;
        }
        else
            poly_cnt++;
    }

    poly_cnt += two_poly_cnt;

    /* Make all the two-sided polygons face forward. */
    for ( i = 0; i < two_poly_cnt; i++ )
    {
        for ( j = 0; j < 4; j++ )
            for ( k = 0; k < 3; k++ )
                verts[j][k] = nodes[k][two_poly[j][i]];

        VEC_SUB( v1, verts[0], verts[1] );
        VEC_SUB( v2, verts[2], verts[1] );
        VEC_CROSS( v3, v2, v1 );

        if ( perspective )
        {
            VEC_SUB( v1, view_eyepoint, verts[1] );
        }
        else
        {
            VEC_SET( v1, 0.0, 0.0, 1.0 );
        }

        if( VEC_DOT( v3, v1 ) < 0.0 )
        {
            /* Reverse the node order. */
            SWAP( tmp, two_poly[0][i], two_poly[3][i] );
            SWAP( tmp, two_poly[1][i], two_poly[2][i] );
        }
    }

    /* Fill the poly array with one & two-sided polygons. */
    for ( i = 0; i < 4; i++ )
        hidden_poly[i] = NEW_N( int, poly_cnt, "Polygons" );

    for ( poly_cnt = 0, i = 0; i < one_poly_cnt; i++ )
    {
        if ( one_poly[0][i] > -1 )
        {
            for ( j = 0; j < 4; j++ )
                hidden_poly[j][poly_cnt] = one_poly[j][i];
            poly_cnt++;
        }
    }

    for ( i = 0; i < two_poly_cnt; i++, poly_cnt++ )
        for ( j = 0; j < 4; j++ )
            hidden_poly[j][poly_cnt] = two_poly[j][i];

    /* Not needed anymore. */
    if ( one_poly_cnt > 0 )
        for ( i = 0; i < 4; i++ )
            free( one_poly[i] );
    if ( two_poly_cnt > 0 )
        for ( i = 0; i < 4; i++ )
            free( two_poly[i] );


    /* Check for degenerate (triangular) faces. */
    for ( i = 0; i < poly_cnt; i++ )
    {
        if( hidden_poly[0][i] == hidden_poly[1][i] )
        {
            SWAP( tmp, two_poly[0][i], two_poly[2][i] );
            SWAP( tmp, two_poly[1][i], two_poly[3][i] );
        }
        else if( hidden_poly[1][i] == hidden_poly[2][i] )
        {
            SWAP( tmp, two_poly[0][i], two_poly[3][i] );
            SWAP( tmp, two_poly[1][i], two_poly[3][i] );
        }
    }

    if ( verbose )
        fprintf( stderr, "Total (reduced) polygons: %d\n", poly_cnt );
}


/************************************************************
 * TAG( create_edge_table )
 *
 * Create edge list, sort it, and delete redundant edges.
 */
static void
create_edge_table()
{
    int *or_edge[2];
    int or_edge_cnt;
    int *ord;
    int tmp;
    int i, j;

    or_edge_cnt = poly_cnt*4 + line_seg_cnt;
    or_edge[0] = NEW_N( int, or_edge_cnt, "Unreduced edges" );
    or_edge[1] = NEW_N( int, or_edge_cnt, "Unreduced edges" );

    or_edge_cnt = 0;
    for ( i = 0; i < poly_cnt; i++ )
        for ( j = 0; j < 4; j++ )
        {
            or_edge[0][or_edge_cnt] = hidden_poly[j][i];
            or_edge[1][or_edge_cnt] = hidden_poly[(j+1)%4][i];
            or_edge_cnt++;
        }

    for ( i = 0; i < line_seg_cnt; i++ )
    {
        or_edge[0][or_edge_cnt] = line_seg[0][i];
        or_edge[1][or_edge_cnt] = line_seg[1][i];
        or_edge_cnt++;
    }

    /*
     * Sort each entry.  Then sort whole table.
     */
    for ( i = 0; i < or_edge_cnt; i++ )
    {
        if ( or_edge[0][i] > or_edge[1][i] )
            SWAP( tmp, or_edge[0][i], or_edge[1][i] );
    }

    ord = NEW_N( int, or_edge_cnt, "Griz_Heapsort ordering table" );
    griz_heapsort( or_edge_cnt, or_edge, ord );


    /*
     * Build the reduced table.
     */
    for ( i = 1, edge_cnt = 1; i < or_edge_cnt; i++ )
        if ( node_cmp( ord[i-1], ord[i], or_edge ) != 0 )
            ++edge_cnt;

    edge[0] = NEW_N( int, edge_cnt, "Edges" );
    edge[1] = NEW_N( int, edge_cnt, "Edges" );

    /* Pick up the first edge. */
    edge[0][0] = or_edge[0][ord[0]];
    edge[1][0] = or_edge[1][ord[0]];

    for ( i = 1, j = 1; i < or_edge_cnt; i++ )
        if ( node_cmp( ord[i-1], ord[i], or_edge ) != 0 )
        {
            /* New edge. */
            edge[0][j] = or_edge[0][ord[i]];
            edge[1][j] = or_edge[1][ord[i]];
            j++;
        }

    free( ord );
    free( or_edge[0] );
    free( or_edge[1] );

    /* Not needed anymore. */
    if ( line_seg_cnt > 0 )
    {
        free( line_seg[0] );
        free( line_seg[1] );
    }

    if ( verbose )
        fprintf( stderr, "Total (reduced) edges: %d\n\n", edge_cnt );
}


/*************************************************************
 * TAG( griz_heapsort )
 *
 * Heap sort, from Numerical_Recipes_in_C.  The node list array
 * arrin and the sorted ordering is returned in the indx array.
 */
static void
griz_heapsort( int n, int *arrin[2], int *indx )
{
    int l, j, ir, indxt, i;

    /*
     * Note that the expected array is indexed 0 to n-1, so we
     * decrement each use of the index in the algorithm to compensate
     * for Numerical Recipes' stupid indexing scheme.
     */
    for ( j = 0; j < n; j++ )
        indx[j] = j;

    l = n/2 + 1;
    ir = n;

    for (;;)
    {
        if ( l > 1 )
            indxt = indx[(--l)-1];
        else
        {
            indxt = indx[ir-1];
            indx[ir-1] = indx[0];
            if ( --ir == 1 )
            {
                indx[0] = indxt;
                return;
            }
        }

        i = l;
        j = 2*l;
        while ( j <= ir )
        {
            if ( j < ir && node_cmp( indx[j-1], indx[j], arrin ) < 0 )
                j++;
            if ( node_cmp( indxt, indx[j-1], arrin ) < 0 )
            {
                indx[i-1] = indx[j-1];
                j += (i=j);
            }
            else
                j = ir + 1;
        }
        indx[i-1] = indxt;
    }
}


/************************************************************
 * TAG( node_cmp )
 *
 * Compare two sets of two nodes in an array.
 */
static int
node_cmp( int indx1, int indx2, int *n_arr[2] )
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
 * TAG( bbox_polys )
 *
 * Calculate 2D bounding boxes for the projected polygons.
 * The 2D bboxes are used for initial edge/poly intersection
 * testing.
 */
static void
bbox_polys()
{
    Real_type d, z, x, y;
    int i, j;

    /* Project all nodes to 2D. */
    proj_nodes[0] = NEW_N( Real_type, node_cnt, "Projected nodes" );
    proj_nodes[1] = NEW_N( Real_type, node_cnt, "Projected nodes" );

    if ( perspective )
    {
        d = view_eyepoint[2] - 1.0;
        for ( i = 0; i < node_cnt; i++ )
        {
            z = view_eyepoint[2] - nodes[2][i];
            proj_nodes[0][i] = nodes[0][i] * d / z;
            proj_nodes[1][i] = nodes[1][i] * d / z;
        }
    }
    else
    {
        for ( i = 0; i < node_cnt; i++ )
        {
            proj_nodes[0][i] = nodes[0][i];
            proj_nodes[1][i] = nodes[1][i];
        }
    }

    /* Calculate 2D bounding box for each polygon. */
    poly_min[0] = NEW_N( Real_type, poly_cnt, "Polygon bbox" );
    poly_min[1] = NEW_N( Real_type, poly_cnt, "Polygon bbox" );
    poly_max[0] = NEW_N( Real_type, poly_cnt, "Polygon bbox" );
    poly_max[1] = NEW_N( Real_type, poly_cnt, "Polygon bbox" );

    for ( i = 0; i < poly_cnt; i++ )
    {
        poly_min[0][i] = proj_nodes[0][hidden_poly[0][i]];
        poly_max[0][i] = proj_nodes[0][hidden_poly[0][i]];
        poly_min[1][i] = proj_nodes[1][hidden_poly[0][i]];
        poly_max[1][i] = proj_nodes[1][hidden_poly[0][i]];

        for ( j = 1; j < 4; j++ )
        {
            x = proj_nodes[0][hidden_poly[j][i]];
            y = proj_nodes[1][hidden_poly[j][i]];

            if ( x < poly_min[0][i] )
                poly_min[0][i] = x;
            else if ( x > poly_max[0][i] )
                poly_max[0][i] = x;

            if ( y < poly_min[1][i] )
                poly_min[1][i] = y;
            else if ( y > poly_max[1][i] )
                poly_max[1][i] = y;
        }
    }
}


/*****************************************************************
 * TAG( hidden_vec_norm )
 * 
 * Normalize a vector.
 */
static void 
hidden_vec_norm( Real_type *vec )
{
    Real_type len;

    len = VEC_LENGTH( vec );
    if ( len == 0.0 )
        return;
    else
	VEC_SCALE( vec, 1.0/len, vec );
}


/*****************************************************************
 * TAG( hidden_point_transform )
 *
 * Transform the Point3d VEC with the transformation matrix MAT, and
 * put the result into the vector *RES.
 *
 *               [a  b  c  0]
 *               [d  e  f  0]
 * [x  y  z  1]  [g  h  i  0]  =  [ax+dy+gz+j  bx+ey+hz+k  cx+fy+iz+l  1]
 *               [j  k  l  1]
 */
static void
hidden_point_transform( Real_type *res, Real_type *vec, Transf_mat *mat)
{
    res[0] = mat->mat[0][0] * vec[0] + mat->mat[1][0] * vec[1] 
        + mat->mat[2][0] * vec[2] + mat->mat[3][0];
    res[1] = mat->mat[0][1] * vec[0] + mat->mat[1][1] * vec[1] 
        + mat->mat[2][1] * vec[2] + mat->mat[3][1];
    res[2] = mat->mat[0][2] * vec[0] + mat->mat[1][2] * vec[1] 
        + mat->mat[2][2] * vec[2] + mat->mat[3][2];
}


/************************************************************
 * TAG( par_test_pt_plane )
 *
 * Plug a point into a plane equation.  Returns a positive
 * number if the point lies on the normal side of the plane,
 * a negative number if the point lies on the side opposite
 * the normal, and zero if the point is in the plane.
 */
static Real_type
par_test_pt_plane( Real_type plane_pt[3], Real_type plane_norm[3], 
                   Real_type test_pt[3] )
{
    return( VEC_DOT( plane_norm, test_pt ) -
            VEC_DOT( plane_norm, plane_pt ) );
}


/************************************************************
 * TAG( clip_line_seg_plane )
 *
 * Intersect a line segment and a plane and clip away any of
 * the line segment that is on the normal side of the plane.
 * The parametric bounds of the line segment are adjusted
 * accordingly.
 *
 * Returns 0 if the whole line segment is clipped away,
 * otherwise returns 1.
 */
static int
clip_line_seg_plane( Real_type plane_pt[3], Real_type plane_norm[3], 
                     Real_type line_pt[3], Real_type line_dir[3], 
                     Real_type bounds[2] )
{
    Real_type pt1[3], pt2[3];
    Real_type t1, t2, d1, d2, p1, p2, con1, con2, isect, t;
    int i1, i2, i;

    VEC_ADDS( pt1, bounds[0], line_dir, line_pt );
    VEC_ADDS( pt2, bounds[1], line_dir, line_pt );
    
    t1 = par_test_pt_plane( plane_pt, plane_norm, pt1 );
    t2 = par_test_pt_plane( plane_pt, plane_norm, pt2 );

    /* Line segment completely on outside of plane. */
/*    if ( !DEF_LT( t1, 0.0 ) && !DEF_LT( t2, 0.0 ) ) */
    if ( !DEF_LT( t1, 0.0 ) && !DEF_LT( t2, 0.0 ) )
        return( 0 );

    /* 
     * Calculate intersection only if end points are on opposite
     * sides of the plane.
     */
    if ( t1 * t2 < 0.0 )
    {
        /* Intersect the line and plane. */

        /* Test for line parallel to plane. */
        if ( APX_EQ( VEC_DOT(line_dir, plane_norm), 0.0 ) )
            return( 1 );

        /* Find largest component of line direction. */
        if ( line_dir[0] > line_dir[1] && line_dir[0] > line_dir[2] )
            i = 0;
        else if ( line_dir[1] > line_dir[0] && line_dir[1] > line_dir[2] )
            i = 1;
        else
            i = 2;
        i1 = (i+1)%3;
        i2 = (i+2)%3;

        /* Calculate intersection point. */
        d1 = line_dir[i1]*plane_norm[i1];
        d2 = line_dir[i2]*plane_norm[i2];
        p1 = line_pt[i1]*plane_norm[i1];
        p2 = line_pt[i2]*plane_norm[i2];
        con1 = VEC_DOT( plane_norm, plane_pt );
        con2 = ( d1 + d2 ) / line_dir[i];
        isect = (con1 + con2*line_pt[i] - p1 - p2) / (plane_norm[i] + con2);
        t = (isect - line_pt[i]) / line_dir[i];

        if ( t > bounds[0] && t < bounds[1] )
        {
            if ( t1 < 0.0 && t2 > 0.0 )
                bounds[1] = t;
            else if ( t2 < 0.0 && t1 > 0.0 )
                bounds[0] = t;
        }
    }

    return( 1 );
}


/************************************************************
 * TAG( copy_frag )
 *
 * Copy one edge fragment structure to another.
 */
static void
copy_frag( Edge_frag *fr_frag, Edge_frag *to_frag )
{
    VEC_COPY( to_frag->line_pt, fr_frag->line_pt );
    VEC_COPY( to_frag->line_dir, fr_frag->line_dir );
    to_frag->proj_pts[0][0] = fr_frag->proj_pts[0][0];
    to_frag->proj_pts[0][1] = fr_frag->proj_pts[0][1];
    to_frag->proj_pts[1][0] = fr_frag->proj_pts[1][0];
    to_frag->proj_pts[1][1] = fr_frag->proj_pts[1][1];
    to_frag->bounds[0] = fr_frag->bounds[0];
    to_frag->bounds[1] = fr_frag->bounds[1];
}


/************************************************************
 * TAG( project_frag_ends )
 *
 * Project the two endpoints of an edge fragment to 2D.
 */
static void
project_frag_ends( Edge_frag *frag )
{
    Real_type end1[3], end2[3];
    Real_type d, z;

    VEC_ADDS( end1, frag->bounds[0], frag->line_dir, frag->line_pt );
    VEC_ADDS( end2, frag->bounds[1], frag->line_dir, frag->line_pt );

    if ( perspective )
    {
        d = view_eyepoint[2] - 1.0;

        z = view_eyepoint[2] - end1[2];
        frag->proj_pts[0][0] = end1[0] * d / z;
        frag->proj_pts[0][1] = end1[1] * d / z;

        z = view_eyepoint[2] - end2[2];
        frag->proj_pts[1][0] = end2[0] * d / z;
        frag->proj_pts[1][1] = end2[1] * d / z;
    }
    else
    {
        frag->proj_pts[0][0] = end1[0];
        frag->proj_pts[0][1] = end1[1];

        frag->proj_pts[1][0] = end2[0];
        frag->proj_pts[1][1] = end2[1];
    }
}


/************************************************************
 * TAG( shadow_edge_poly )
 *
 * Calculate the fragment of an edge that is in the shadow
 * volume of the specified polygon.  Then subtract the
 * shadow fragment from the original edge.  The result can
 * be 0, 1 or 2 fragments.  This routine returns the number
 * of fragments.  If one fragment is returned, it is returned
 * in frag.  If two fragments are returned, the second is
 * allocated and comes back in nfrag2.
 */
static int
shadow_edge_poly( int npoly, Edge_frag *frag, Edge_frag **nfrag2 )
{
    Real_type shadow_bounds[2];
    Real_type verts[4][3];
    Real_type v1[3], v2[3], v3[3];
    int i, j;

    /* Copy fragment bounds into shadow fragment bounds. */
    shadow_bounds[0] = frag->bounds[0];
    shadow_bounds[1] = frag->bounds[1];

    /* Get polygon vertices. */
    for ( i = 0; i < 4; i++ )
        for ( j = 0; j < 3; j++ )
            verts[i][j] = nodes[j][hidden_poly[i][npoly]];

    /* Clip shadow edge against the four side planes. */
    for ( i = 0; i < 4; i++ )
    {
        /* Check for degenerate edge. */
        if ( hidden_poly[i][npoly] == hidden_poly[(i+1)%4][npoly] )
            continue;

        VEC_SUB( v1, verts[(i+1)%4], verts[i] );
        if ( perspective )
        {
            VEC_SUB( v2, view_eyepoint, verts[i] );
        }
        else
        {
            VEC_SET( v2, 0.0, 0.0, 1.0 );
        }
        VEC_CROSS( v3, v1, v2 );

        /* If clipped, just return the edge. */
        if ( !clip_line_seg_plane( verts[i], v3, frag->line_pt,
                                   frag->line_dir, shadow_bounds ) )
            return 1;
    }

    /* Clip shadow edge against polygon plane. */
    VEC_SUB( v1, verts[2], verts[1] );
    VEC_SUB( v2, verts[0], verts[1] );
    VEC_CROSS( v3, v1, v2 );

    if ( !clip_line_seg_plane( verts[1], v3, frag->line_pt,
                               frag->line_dir, shadow_bounds ) )
        return 1;

    /*
     * Subtract shadowed fragment from edge fragment.
     */

    if ( !DEF_GT( shadow_bounds[1], shadow_bounds[0] ) )
    {
        /* Just return original fragment. */
        return 1;
    }

    if ( APX_EQ( shadow_bounds[0], frag->bounds[0] ) )
    {
        if ( DEF_LT( shadow_bounds[1], frag->bounds[1] ) )
        {
            frag->bounds[0] = shadow_bounds[1];
            project_frag_ends( frag );
            return 1;
        }
        else
        {
            /* Whole fragment is obscured. */
            return 0;
        }
    }
    else if ( APX_EQ( shadow_bounds[1], frag->bounds[1] ) )
    {
        frag->bounds[1] = shadow_bounds[0];
        project_frag_ends( frag );
        return 1;
    }
    else
    {
        /* Form two fragments. */
        *nfrag2 = NEW( Edge_frag, "Edge fragment" );
        copy_frag( frag, *nfrag2 );
        frag->bounds[1] = shadow_bounds[0];
        (*nfrag2)->bounds[0] = shadow_bounds[1];
        project_frag_ends( frag );
        project_frag_ends( *nfrag2 );

        return 2;
    }
}


/************************************************************
 * TAG( gen_hidden )
 *
 * Generate hidden lines.
 */
static void
gen_hidden()
{
    Edge_frag *frag, *frag_list, *new_frag;
    Drawn_edge *draw_edge;
    Bool_type done;
    Real_type view_clip_pt[4][3];
    Real_type view_clip_norm[4][3];
    Real_type edge_verts[2][3];
    Real_type v1[3], v2[3];
    unsigned int clip_flag[2];
    int num_frags;
    int i, j, k;

    /*
     * Setup four view-volume clip planes.
     */
    if ( view_aspect >= 1.0 )
    {
        VEC_SET( view_clip_pt[0], view_aspect, 0.0, 1.0 );    /* Right. */
        VEC_SET( view_clip_pt[1], -view_aspect, 0.0, 1.0 );   /* Left. */
        VEC_SET( view_clip_pt[2], 0.0, 1.0, 1.0 );            /* Upper. */
        VEC_SET( view_clip_pt[3], 0.0, -1.0, 1.0 );           /* Lower. */
    }
    else
    {
        VEC_SET( view_clip_pt[0], 1.0, 0.0, 1.0 );
        VEC_SET( view_clip_pt[1], -1.0, 0.0, 1.0 );
        VEC_SET( view_clip_pt[2], 0.0, 1.0/view_aspect, 1.0 );
        VEC_SET( view_clip_pt[3], 0.0, -1.0/view_aspect, 1.0 );
    }

    if ( perspective )
    {
        VEC_SET( v1, 0.0, 1.0, 0.0 );
        VEC_SUB( v2, view_eyepoint, view_clip_pt[0] );
        VEC_CROSS( view_clip_norm[0], v1, v2 );
        VEC_SET( v1, 0.0, -1.0, 0.0 );
        VEC_SUB( v2, view_eyepoint, view_clip_pt[1] );
        VEC_CROSS( view_clip_norm[1], v1, v2 );
        VEC_SET( v1, -1.0, 0.0, 0.0 );
        VEC_SUB( v2, view_eyepoint, view_clip_pt[2] );
        VEC_CROSS( view_clip_norm[2], v1, v2 );
        VEC_SET( v1, 1.0, 0.0, 0.0 );
        VEC_SUB( v2, view_eyepoint, view_clip_pt[3] );
        VEC_CROSS( view_clip_norm[3], v1, v2 );

        for ( i = 0; i < 4; i++ )
            hidden_vec_norm( view_clip_norm[i] );
    }
    else
    {
        VEC_SET( view_clip_norm[0], 1.0, 0.0, 0.0 );
        VEC_SET( view_clip_norm[1], -1.0, 0.0, 0.0 );
        VEC_SET( view_clip_norm[2], 0.0, 1.0, 0.0 );
        VEC_SET( view_clip_norm[3], 0.0, -1.0, 0.0 );
    }

    /*
     * Loop through edges.
     */
    draw_edge_list = NULL;

    for ( i = 0; i < edge_cnt; i++ )
    {
        if ( verbose && i%1000 == 0 && i > 0 )
            fprintf( stderr, "Edge %d\n", i );

        /* Allocate a new edge structure. */
        frag = NEW( Edge_frag, "Edge fragment" );

        for ( j = 0; j < 2; j++ )
        {
            for ( k = 0; k < 3; k++ )
                edge_verts[j][k] = nodes[k][edge[j][i]];
            for ( k = 0; k < 2; k++ )
                frag->proj_pts[j][k] = proj_nodes[k][edge[j][i]];
        }

        VEC_COPY( frag->line_pt, edge_verts[0] );
        VEC_SUB( frag->line_dir, edge_verts[1], edge_verts[0] );
        frag->bounds[0] = 0.0;
        frag->bounds[1] = 1.0;

        /* Clip edge to view volume. */
        done = FALSE;
        for ( j = 0; j < 4; j++ )
            if ( !clip_line_seg_plane( view_clip_pt[j], view_clip_norm[j],
                                       frag->line_pt, frag->line_dir,
                                       frag->bounds ) )
            {
                done = TRUE;
                break;
            }

        if ( done )
        {
            free( frag );
            continue;
        }

        /* Get the new 2D endpoints. */
        if ( frag->bounds[0] != 0.0 || frag->bounds[1] != 1.0 )
            project_frag_ends( frag );

        frag_list = frag;

        /*
         * Loop through polygons.
         */
        for ( j = 0; j < poly_cnt; j++ )
        {
            /*
             * Test whether edge is an edge of the polygon, for
             * trivial reject.
             */
            for ( done = FALSE, k = 0; k < 4; k++ )
                if ( ( edge[0][i] == hidden_poly[k][j] &&
                       edge[1][i] == hidden_poly[(k+1)%4][j] ) ||
                     ( edge[1][i] == hidden_poly[k][j] &&
                       edge[0][i] == hidden_poly[(k+1)%4][j] ) )
                    done = TRUE;
            if ( done )
                continue;

            /*
             * Loop through visible fragments of edge.
             */
            for ( frag = frag_list; frag != NULL; )
            {
                /* 
                 * Test edge fragment against bbox of projected poly for
                 * trivial reject.
                 */
                clip_flag[0] = 0;
                clip_flag[1] = 0;

                for ( k = 0; k < 2; k++ )
                {
                    if ( frag->proj_pts[k][0] <= poly_min[0][j] )
                        clip_flag[k] = clip_flag[k] | 1;
                    else if ( frag->proj_pts[k][0] >= poly_max[0][j] )
                        clip_flag[k] = clip_flag[k] | 2;
                    if ( frag->proj_pts[k][1] <= poly_min[1][j] )
                        clip_flag[k] = clip_flag[k] | 4;
                    else if ( frag->proj_pts[k][1] >= poly_max[1][j] )
                        clip_flag[k] = clip_flag[k] | 8;
                }

                if ( clip_flag[0] & clip_flag[1] )
                {
                    frag = frag->next;
                    continue;
                }

                /*
                 * Calculate the segment of the line inside the shadow
                 * volume behind the polygon.  Subtract this segment
                 * from the fragment, generating 0, 1 or 2 fragments
                 * as a result.  Append any fragments to the fragment
                 * list.
                 */
                num_frags = shadow_edge_poly( j, frag, &new_frag );

                if ( num_frags == 0 )
                {
                    /* Delete fragment from list. */
                    new_frag = frag->next;
                    DELETE( frag, frag_list );
                    frag = new_frag;
                }
                else  /* num_frags == 1 or 2 */
                {
                    frag = frag->next;
                    if ( num_frags == 2 )
                        INSERT( new_frag, frag_list );
                }
            }

            /* If no visible fragments left, quit the loop early. */
            if ( frag_list == NULL )
                break;

        } /* Loop through polygons. */

        /* Convert all visible fragments into drawn edges. */
        for ( frag = frag_list; frag != NULL; frag = frag->next )
        {
            draw_edge = NEW( Drawn_edge, "Draw edge" );
            draw_edge->pts[0][0] = frag->proj_pts[0][0];
            draw_edge->pts[0][1] = frag->proj_pts[0][1];
            draw_edge->pts[1][0] = frag->proj_pts[1][0];
            draw_edge->pts[1][1] = frag->proj_pts[1][1];
            INSERT( draw_edge, draw_edge_list );
        }
        DELETE_LIST( frag_list );

    } /* Loop through edges. */

    /*
     * Don't need this stuff anymore.  Don't really need to free
     * it either; I'm just being retentive.
     */
    for ( i = 0; i < 2; i++ )
    {
        free( edge[i] );
        free( poly_min[i] );
        free( poly_max[i] );
    }
    for ( i = 0; i < 4; i++ )
        free( hidden_poly[i] );
}


/************************************************************
 * TAG( output_hidden )
 *
 * Output hidden lines to a Postscript file.
 */
static void
output_hidden( char *filename )
{
    Drawn_edge *draw_edge;
    time_t time_int;
    struct tm *tm_struct;
    char date[20];
    char *user_name;
    Real_type ps_viewport[2][2];
    Real_type hid_vp_x, hid_vp_y;
    Real_type scale_x, scale_y, scale;
    Real_type ps_ctr[2];
    Real_type tmp;
    int i, j;
    int pointcount;

    FILE *outfile;

    
    if ( ( outfile = fopen(filename, "w+") ) == NULL )
    {
        fprintf( stderr, "Couldn't open output file %s.\n", filename );
        exit( 1 );
    }


    if ( draw_edge_list == NULL )
    {
        fprintf( stderr, "No hidden lines generated!!!\n" );
        exit( 0 );
    }

    /* Get today's date and user's name. */
    time_int = time( NULL );
    tm_struct = gmtime( &time_int );
    strftime( date, 19, "%D", tm_struct );
    user_name = getenv( "LOGNAME" );

    /* Retrieve the HIDDEN viewport dimensions. */
    if ( view_aspect >= 1.0 )
    {
        hid_vp_x = 2.0 * view_aspect;
        hid_vp_y = 2.0;
    }
    else
    {
        hid_vp_x = 2.0;
        hid_vp_y = 2.0 / view_aspect;
    }

    /* Rotate image for landscape output. */
    if ( landscape || (view_aspect > 1.0 && !portrait) )
    {
        for ( draw_edge = draw_edge_list;
              draw_edge != NULL;
              draw_edge = draw_edge->next )
        {
            for ( i = 0; i < 2; i++ )
            {
                /*
                 * Rotate about Z by -90 degrees:
                 *     x' = y
                 *     y' = -x
                 */
                tmp = draw_edge->pts[i][0];
                draw_edge->pts[i][0] = draw_edge->pts[i][1];
                draw_edge->pts[i][1] = -tmp;
            }
        }
        SWAP( tmp, hid_vp_x, hid_vp_y );
    }

    /* Set the Postscript viewport. */
    ps_viewport[0][0] = 40.0;
    ps_viewport[0][1] = 50.0;
    ps_viewport[1][0] = 560.0;
    ps_viewport[1][1] = 750.0;
    ps_ctr[0] = 0.5 * ( ps_viewport[0][0] + ps_viewport[1][0] );
    ps_ctr[1] = 0.5 * ( ps_viewport[0][1] + ps_viewport[1][1] );

    scale_x = ( ps_viewport[1][0] - ps_viewport[0][0] ) / hid_vp_x;
    scale_y = ( ps_viewport[1][1] - ps_viewport[0][1] ) / hid_vp_y;
    scale = ( scale_x < scale_y ? scale_x : scale_y );

    /*
     * Scale and translate the hidden lines into the Postscript
     * viewport.
     */
    for ( draw_edge = draw_edge_list;
          draw_edge != NULL;
          draw_edge = draw_edge->next )
    {
        for ( i = 0; i < 2; i++ )
            for ( j = 0; j < 2; j++ )
                draw_edge->pts[i][j] = draw_edge->pts[i][j] * scale +
                                       ps_ctr[j];
    }

    /*
     * Output the transformed lines to the file.
     */
    fprintf( outfile, "%%!PS-Adobe-2.0\n" );
    fprintf( outfile, "%%%%Title: LLNL hidden line drawing from HIDDEN\n" );
    fprintf( outfile, "%%%%Creator: %s\n", user_name );
    fprintf( outfile, "%%%%Creation date: %s\n", date );
    fprintf( outfile, "%%%%EndComments\n" );
    fprintf( outfile, "initgraphics 1 setlinecap " );
    fprintf( outfile, "1 setlinejoin 0 setlinewidth\n" );
    fprintf( outfile, "/m {moveto} def /l {lineto} def\n" );
    fprintf( outfile, "%%%%Page:  1\n" );
    fprintf( outfile, "newpath\n" );

    for ( draw_edge = draw_edge_list, pointcount = 0;
          draw_edge != NULL;
          draw_edge = draw_edge->next )
    {
        fprintf( outfile, " %.2f %.2f m  %.2f %.2f l\n",
                 draw_edge->pts[0][0], draw_edge->pts[0][1],
                 draw_edge->pts[1][0], draw_edge->pts[1][1] );
        pointcount++;
	
        /*
         * if the point count exceeds the path limit, stroke it
         * and initiate a new path
         */
        if ( pointcount == PS_ACTIVE_PATH_LIMIT - 1 )
	{
            pointcount = 0;
            fprintf( outfile, "stroke newpath\n" );
        }
    }

    fprintf( outfile, "stroke showpage\n" );
}


