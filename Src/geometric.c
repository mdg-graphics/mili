/* $Id$ */
/*
 * geometric.c - Vectors, transformation matrices, transforms.
 *
 *      modified by Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Jan 28 1992
 *
 *      plane_three_pts() added 10/97 by Doug Speck, LLNL
 *
 */


/**
 ** sipp - SImple Polygon Processor
 **
 **  A general 3d graphic package
 **
 **  Copyright Jonas Yngvesson  (jonas-y@isy.liu.se) 1988/89/90/91
 **            Inge Wallin      (ingwa@isy.liu.se)         1990/91
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 1, or any later version.
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 ** You can receive a copy of the GNU General Public License from the
 ** Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **/


#include <math.h>
#include <stdlib.h>
#include "misc.h"
#include "geometric.h"


/*****************************************************************
 * TAG( vec_norm )
 *
 * Normalize a vector.
 */
void vec_norm( float *vec )
{
    float len;

    len = VEC_LENGTH( vec );
    if ( len == 0.0 )
        /*
        #ifdef DEBUG
                fprintf( stderr, "vec_norm(): Vector has length 0!\n" );
        #else
        */
        return;
    /*
    #endif DEBUG
    */
    else
        VEC_SCALE( vec, 1.0/len, vec );
}


/*****************************************************************
 * TAG( norm_three_pts )
 *
 * Compute a normal vector to a plane from three points in the
 * plane.  norm = cross( P3 - P2, P1 - P2 ).
 */
void norm_three_pts( float norm[3], float pt1[3], float pt2[3], float pt3[3] )
{
    float v1[3], v2[3];

    VEC_SUB( v1, pt3, pt2 );
    VEC_SUB( v2, pt1, pt2 );
    VEC_CROSS( norm, v1, v2 );
    vec_norm( norm );
}


/*****************************************************************
 * TAG( plane_three_pts )
 *
 * Compute A, B, C, and D coefficients for a plane from three
 * points in the plane.
 */
void plane_three_pts( float plane[4], float pt1[3], float pt2[3], float pt3[3] )
{
    float v1[3], v2[3];

    VEC_SUB( v1, pt3, pt2 );
    VEC_SUB( v2, pt1, pt2 );
    VEC_CROSS( plane, v1, v2 );
    vec_norm( plane );
    plane[3] = -VEC_DOT( pt1, plane );
}


/*****************************************************************
 * TAG( line_two_pts )
 *
 * Compute A, B, and C coefficients for a normalized 2D line from
 * two points on the line.
 */
void
line_two_pts( float line[3], float pt1[2], float pt2[2] )
{
    double len_inv, len;
    float perp_vec[2];

    perp_vec[0] = pt1[1] - pt2[1];
    perp_vec[1] = pt2[0] - pt1[0];

    len = VEC_LENGTH_2D( perp_vec );
    if ( len == 0.0 )
        return;

    len_inv = 1.0 / len;

    line[0] = len_inv * perp_vec[0];
    line[1] = len_inv * perp_vec[1];
    line[2] = -VEC_DOT_2D( pt1, line );
}


/*****************************************************************
 * TAG( sqr_dist_seg_to_line )
 *
 * Returns the square of the distance between a line segment and a line.
 * The line segment is specified by its two endpoints; the line is
 * specified by a point and a direction vector.
 */
float
sqr_dist_seg_to_line( float seg_pt1[3], float seg_pt2[3], float line_pt[3],
                      float line_dir[3] )
{
    float v1[3], v2[3], pta[3], ptb[3];
    float v1p1, v2p2, v2p1, v1p2, v1v1, v2v2, v1v2, denom, s;

    /* First check if the lines are near parallel.
     * Just return dist pt to line in that case.
     */
    VEC_SUB( v1, seg_pt2, seg_pt1 );
    vec_norm( v1 );
    VEC_COPY( v2, line_dir );
    vec_norm( v2 );
    if ( APX_EQ( VEC_DOT( v1, v2 ), 1.0 ) )
    {
        VEC_COPY( pta, seg_pt1 );
    }
    else
    {
        /* Get the nearest point on the line segment. */
        VEC_SUB( v1, seg_pt2, seg_pt1 );
        v1p1 = VEC_DOT( v1, seg_pt1 );
        v2p2 = VEC_DOT( line_dir, line_pt );
        v2p1 = VEC_DOT( line_dir, seg_pt1 );
        v1p2 = VEC_DOT( v1, line_pt );
        v1v1 = VEC_DOT( v1, v1 );
        v2v2 = VEC_DOT( line_dir, line_dir );
        v1v2 = VEC_DOT( v1, line_dir );
        denom = v1v2*v1v2 - v1v1*v2v2;

        s = ( (v2p2 - v2p1)*v1v2 - (v1p2 - v1p1)*v2v2 ) / denom;

        if ( s < 0.0 )
        {
            VEC_COPY( pta, seg_pt1 );
        }
        else if ( s > 1.0 )
        {
            VEC_COPY( pta, seg_pt2 );
        }
        else
        {
            VEC_ADDS( pta, s, v1, seg_pt1 );
        }
    }

    near_pt_on_line( pta, line_pt, line_dir, ptb );
    VEC_SUB( v1, pta, ptb );
    return( VEC_DOT( v1, v1 ) );

    /*
     *  Line 1:  P1[3], V1[3] = P1b - P1, P = P1 + s*V1
     *  Line 2:  P2[3], V2[3] = P2b - P2, P = P2 + t*V2
     *
     *  v1p1 = VEC_DOT( V1, P1 );
     *  v2p2 = VEC_DOT( V2, P2 );
     *  v2p1 = VEC_DOT( V2, P1 );
     *  v1p2 = VEC_DOT( V1, P2 );
     *
     *  v1v1 = VEC_DOT( V1, V1 );
     *  v2v2 = VEC_DOT( V2, V2 );
     *  v1v2 = VEC_DOT( V1, V2 );
     *  denom = v1v2*v1v2 - v1v1*v2v2;
     *
     *  Parametric loc of intersection pt on line1 and line2:
     *
     *  s = ( (v2p2 - v2p1)*v1v2 - (v1p2 - v1p1)*v2v2 ) / denom;
     *  t = ( (v2p2 - v2p1)*v1v1 - (v1p2 - v1p1)*v1v2 ) / denom;
     *
     */
}


/*****************************************************************
 * TAG( near_pt_on_line )
 *
 * Returns the point on a line nearest to the specified point.
 * The line is specified by a point on the line and a direction
 * vector.  The result is returned in near_pt.
 */
void
near_pt_on_line( float pt[3], float line_pt[3], float line_dir[3],
                 float near_pt[3] )
{
    float v1[3], v2[3], v3[3];

    VEC_COPY( v1, line_dir );
    vec_norm( v1 );
    VEC_SUB( v2, pt, line_pt );
    /*
     * The component of vector A in the direction of vector B is
     *
     *     dot( A, B ) / length( B )
     *
     * Calculate component of v2 in direction of v1.
     */
    VEC_SCALE( v3, VEC_DOT( v2, v1 ), v1 );
    VEC_ADD( near_pt, line_pt, v3 );
}


/*****************************************************************
 * TAG( intersect_line_plane )
 *
 * Find the intersection of a line and a plane.  The routine
 * returns 0 if they do not intersect.  The intersection point is
 * returned in the last argument.
 */
int
intersect_line_plane( float line_pt[3], float line_dir[3], float plane_pt[3],
                      float plane_norm[3], float isect_pt[3] )
{
    float d1, d2, p1, p2, con1, con2, t;
    int i, i1, i2;

    /* Test for line parallel to plane. */
    if ( APX_EQ( VEC_DOT(line_dir, plane_norm), 0.0 ) )
        return( 0 );

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
    isect_pt[i] = (con1 + con2*line_pt[i] - p1 - p2) / (plane_norm[i] + con2);

    t = (isect_pt[i] - line_pt[i]) / line_dir[i];
    isect_pt[i1] = line_pt[i1] + t*line_dir[i1];
    isect_pt[i2] = line_pt[i2] + t*line_dir[i2];
    return( 1 );
}


/*****************************************************************
 * TAG( intersect_line_tri )
 *
 * Test whether a line intersects a triangular polygon.  Returns
 * 1 if the line intersects and 0 if not.  The intersection point
 * is returned in the last parameter.
 */
int
intersect_line_tri( float verts[3][3], float line_pt[3], float line_dir[3],
                    float isect_pt[3] )
{
    float norm[3];
    float v1[3], v2[3], v3[3];
    int i;

    norm_three_pts( norm, verts[0], verts[1], verts[2] );

    /* Intersect the line with the plane of the polygon. */
    if ( intersect_line_plane( line_pt, line_dir, verts[0], norm, isect_pt ) )
    {
        for ( i = 0; i < 3; i++ )
        {
            VEC_SUB( v1, isect_pt, verts[i] );
            VEC_SUB( v2, verts[(i+1)%3], verts[i] );
            VEC_CROSS( v3, v1, v2 );
            if ( VEC_DOT( v3, norm ) > 0.0 )
                return( 0 );
        }
        return( 1 );
    }
    else
        return( 0 );
}


/*****************************************************************
 * TAG( intersect_line_quad )
 *
 * Test whether a line intersects a quadrilateral polygon.  Returns
 * 1 if the line intersects and 0 if not.  The intersection point
 * is returned in the last parameter.
 */
int
intersect_line_quad( float verts[4][3], float line_pt[3], float line_dir[3],
                     float isect_pt[3] )
{
    float tverts[3][3];
    int i, j, ii;

    /* Break into two triangles and test each of those. */
    for ( i = 0; i < 2; i++ )
    {
        ii = i*2;

        for ( j = 0; j < 3; j++ )
        {
            VEC_COPY( tverts[j], verts[(ii+j)%4] );
        }

        if ( intersect_line_tri( tverts, line_pt, line_dir, isect_pt ) )
            return( 1 );
    }
    return( 0 );
}


/*****************************************************************
 * TAG( area_of_triangle )
 *
 * Returns the area of a triangular polygon.
 */
float
area_of_triangle( float verts[3][3] )
{
    float v1[3], v2[3], vc[3];
    float area;

    VEC_SUB( v1, verts[1], verts[0] );
    VEC_SUB( v2, verts[2], verts[0] );
    VEC_CROSS( vc, v1, v2 );
    area = 0.5 * VEC_LENGTH( vc );

    return area;
}


/*****************************************************************
 * TAG( area_of_quad )
 *
 * Returns the area of a quadrilateral polygon.
 */
float
area_of_quad( float verts[4][3] )
{
    float v1[3], v2[3], vc[3];
    float area;

    /* I'm sure there's a better way to calculate this. */

    VEC_SUB( v1, verts[1], verts[0] );
    VEC_SUB( v2, verts[3], verts[0] );
    VEC_CROSS( vc, v1, v2 );
    area = 0.5 * VEC_LENGTH( vc );

    VEC_SUB( v1, verts[3], verts[2] );
    VEC_SUB( v2, verts[1], verts[2] );
    VEC_CROSS( vc, v1, v2 );
    area += 0.5 * VEC_LENGTH( vc );

    return area;
}


/*****************************************************************
 * TAG( ident_matrix )
 *
 * Identity matrix.
 */
Transf_mat   ident_matrix = {{        /* Unit tranfs. matrix */
        { 1.0, 0.0, 0.0, 0.0 },
        { 0.0, 1.0, 0.0, 0.0 },
        { 0.0, 0.0, 1.0, 0.0 },
        { 0.0, 0.0, 0.0, 1.0 }
    }
};


/*****************************************************************
 * TAG( transf_mat_create )
 *
 * Allocate a new matrix, and if INITMAT != NULL copy the contents
 * of INITMAT to the new matrix,  otherwise copy the identity matrix
 * to the new matrix.
 */
Transf_mat *
transf_mat_create( Transf_mat *initmat )
{
    Transf_mat  *mat;

    mat = (Transf_mat *) malloc(sizeof(Transf_mat));
    if (initmat != NULL)
        mat_copy( mat, initmat );
    else
        mat_copy( mat, &ident_matrix );

    return mat;
}


/*****************************************************************
 * TAG( transf_mat_destruct )
 *
 */
void
transf_mat_destruct( Transf_mat *mat )
{
    free( mat );
}


/*****************************************************************
 * TAG( mat_copy )
 *
 * Copy matrix b into matrix a.
 */
void
mat_copy( Transf_mat *a, Transf_mat *b )
{
    int i, j;

    for ( i = 0; i < 4; i++ )
        for ( j = 0; j < 4; j++ )
            a->mat[i][j] = b->mat[i][j];
}


/*****************************************************************
 * TAG( mat_to_array )
 *
 * Copy matrix into a 1D array in row-major order.
 */
void
mat_to_array( Transf_mat *mat, float array[16] )
{
    int i, j;

    for ( i = 0; i < 4; i++ )
        for ( j = 0; j < 4; j++ )
            array[i*4 + j] = mat->mat[i][j];
}


/*****************************************************************
 * TAG( mat_translate )
 *
 * Set MAT to the transformation matrix that represents the
 * concatenation between the previous transformation in MAT
 * and a translation along the vector described by DX, DY and DZ.
 *
 * [a  b  c  0]   [ 1  0  0  0]     [ a     b     c    0]
 * [d  e  f  0]   [ 0  1  0  0]     [ d     e     f    0]
 * [g  h  i  0] * [ 0  0  1  0]  =  [ g     h     i    0]
 * [j  k  l  1]   [Tx Ty Tz  1]     [j+Tx  k+Ty  l+Tz  1]
 */
void
mat_translate( Transf_mat *mat,  float dx,  float dy,  float dz )
{
    mat->mat[3][0] += dx;
    mat->mat[3][1] += dy;
    mat->mat[3][2] += dz;
}


/*****************************************************************
 * TAG( mat_trans )
 *
 * Pre-multiply by the specified translation.
 *
 * [          ]   [a  b  c  0]   [            ]
 * [    T     ]   [d  e  f  0]   [            ]
 * [          ] * [g  h  i  0] = [            ]
 * [          ]   [j  k  l  1]   [            ]
 */
void
mat_trans( Transf_mat *mat,  float dx,  float dy,  float dz )
{
    mat->mat[3][0] += dx*mat->mat[0][0]+dy*mat->mat[1][0]+dz*mat->mat[2][0];
    mat->mat[3][1] += dx*mat->mat[0][1]+dy*mat->mat[1][1]+dz*mat->mat[2][1];
    mat->mat[3][2] += dx*mat->mat[0][2]+dy*mat->mat[1][2]+dz*mat->mat[2][2];
}


/*****************************************************************
 * TAG( mat_rot )
 *
 * Pre-multiply the transformation matrix with the rotation matrix
 * for the specified rotation.
 *
 * [          ]   [a  b  c  0]   [            ]
 * [    R     ]   [d  e  f  0]   [            ]
 * [          ] * [g  h  i  0] = [            ]
 * [          ]   [j  k  l  1]   [            ]
 */
void
mat_rot( Transf_mat *mat, float ang, int axis )
{
    Transf_mat rmat;
    float   cosang;
    float   sinang;
    int      i, j;

    cosang = cos((double)ang);
    sinang = sin((double)ang);
    if (fabs((double)cosang) < 1.0e-15)
    {
        cosang = 0.0;
    }
    if (fabs((double)sinang) < 1.0e-15)
    {
        sinang = 0.0;
    }

    for ( i = 0; i < 4; i++ )
        for ( j = 0; j < 3; j++ )
            if ( i == j )
                rmat.mat[i][j] = 1.0;
            else
                rmat.mat[i][j] = 0.0;

    if ( axis == 0 )
    {
        rmat.mat[1][1] = cosang;
        rmat.mat[1][2] = sinang;
        rmat.mat[2][1] = -sinang;
        rmat.mat[2][2] = cosang;
    }
    else if ( axis == 1 )
    {
        rmat.mat[0][0] = cosang;
        rmat.mat[0][2] = -sinang;
        rmat.mat[2][0] = sinang;
        rmat.mat[2][2] = cosang;
    }
    else
    {
        rmat.mat[0][0] = cosang;
        rmat.mat[0][1] = sinang;
        rmat.mat[1][0] = -sinang;
        rmat.mat[1][1] = cosang;
    }

    mat_mul( mat, &rmat, mat );
}


/*****************************************************************
 * TAG( mat_rotate_x )
 *
 * Set MAT to the transformation matrix that represents the
 * concatenation between the previous transformation in MAT
 * and a rotation with the angle ANG around the X axis.
 *
 * [a  b  c  0]   [1   0   0  0]     [a  b*Ca-c*Sa  b*Sa+c*Ca  0]
 * [d  e  f  0]   [0  Ca  Sa  0]     [d  e*Ca-f*Sa  e*Sa+f*Ca  0]
 * [g  h  i  0] * [0 -Sa  Ca  0]  =  [g  h*Ca-i*Sa  h*Sa+i*Ca  0]
 * [j  k  l  1]   [0   0   0  1]     [j  k*Ca-l*Sa  k*Se+l*Ca  1]
 */
void
mat_rotate_x( Transf_mat *mat, float ang )
{
    float   cosang;
    float   sinang;
    float   tmp;
    int      i;

    cosang = cos((double)ang);
    sinang = sin((double)ang);
    if (fabs((double)cosang) < 1.0e-15)
    {
        cosang = 0.0;
    }
    if (fabs((double)sinang) < 1.0e-15)
    {
        sinang = 0.0;
    }
    for (i = 0; i < 4; ++i)
    {
        tmp = mat->mat[i][1];
        mat->mat[i][1] = mat->mat[i][1] * cosang
                         - mat->mat[i][2] * sinang;
        mat->mat[i][2] = tmp * sinang + mat->mat[i][2] * cosang;
    }
}


/*****************************************************************
 * TAG( mat_rotate_y )
 *
 * Set MAT to the transformation matrix that represents the
 * concatenation between the previous transformation in MAT
 * and a rotation with the angle ANG around the Y axis.
 *
 * [a  b  c  0]   [Ca   0 -Sa  0]     [a*Ca+c*Sa  b  -a*Sa+c*Ca  0]
 * [d  e  f  0] * [ 0   1   0  0]  =  [d*Ca+f*Sa  e  -d*Sa+f*Ca  0]
 * [g  h  i  0]   [Sa   0  Ca  0]     [g*Ca+i*Sa  h  -g*Sa+i*Ca  0]
 * [j  k  l  1]   [ 0   0   0  1]     [j*Ca+l*Sa  k  -j*Sa+l*Ca  1]
 */

void
mat_rotate_y( Transf_mat *mat, float ang )
{
    float   cosang;
    float   sinang;
    float   tmp;
    int      i;

    cosang = cos((double)ang);
    sinang = sin((double)ang);
    if (fabs((double)cosang) < 1.0e-15)
    {
        cosang = 0.0;
    }
    if (fabs((double)sinang) < 1.0e-15)
    {
        sinang = 0.0;
    }
    for (i = 0; i < 4; ++i)
    {
        tmp = mat->mat[i][0];
        mat->mat[i][0] = mat->mat[i][0] * cosang
                         + mat->mat[i][2] * sinang;
        mat->mat[i][2] = -tmp * sinang + mat->mat[i][2] * cosang;
    }
}


/*****************************************************************
 * TAG( mat_rotate_z )
 *
 * Set MAT to the transformation matrix that represents the
 * concatenation between the previous transformation in MAT
 * and a rotation with the angle ANG around the Z axis.
 *
 * [a  b  c  0]   [ Ca  Sa   0  0]     [a*Ca-b*Sa  a*Sa+b*Ca  c  0]
 * [d  e  f  0]   [-Sa  Ca   0  0]     [d*Ca-e*Sa  d*Sa+e*Ca  f  0]
 * [g  h  i  0] * [  0   0   1  0]  =  [g*Ca-h*Sa  g*Sa+h*Ca  i  0]
 * [j  k  l  1]   [  0   0   0  1]     [j*Ca-k*Sa  j*Sa+k*Ca  l  0]
 */
void
mat_rotate_z( Transf_mat *mat, float ang )
{
    float   cosang;
    float   sinang;
    float   tmp;
    int      i;

    cosang = cos((double)ang);
    sinang = sin((double)ang);
    if (fabs((double)cosang) < 1.0e-15)
    {
        cosang = 0.0;
    }
    if (fabs((double)sinang) < 1.0e-15)
    {
        sinang = 0.0;
    }
    for (i = 0; i < 4; ++i)
    {
        tmp = mat->mat[i][0];
        mat->mat[i][0] = mat->mat[i][0] * cosang
                         - mat->mat[i][1] * sinang;
        mat->mat[i][1] = tmp * sinang + mat->mat[i][1] * cosang;
    }
}


/*****************************************************************
 * TAG( mat_rotate )
 *
 * Set MAT to the transformation matrix that represents the
 * concatenation between the previous transformation in MAT
 * and a rotation with the angle ANG around the line represented
 * by the point POINT and the vector VECTOR.
 */
void
mat_rotate( Transf_mat *mat, float *point, float *vector, float ang )
{
    extern double hypot( double, double ); /* Have the #include, but still
                                              get a warning without this. */
    float   ang2;
    float   ang3;
    float   hyp;

    if ( APX_EQ(vector[1], 0.0) && APX_EQ(vector[0], 0.0) )
        ang2 = 0.0;
    else
        ang2 = atan2((double)vector[1], (double)vector[0]);

    hyp = hypot((double)vector[0], (double)vector[1]);
    if ( APX_EQ(hyp, 0.0) && APX_EQ(vector[2], 0.0) )
        ang3 = 0.0;
    else
        ang3 = atan2(hyp, (double)vector[2]);

    mat_translate(mat, -point[0], -point[1], -point[2]);
    mat_rotate_z(mat, -ang2);
    mat_rotate_y(mat, -ang3);
    mat_rotate_z(mat, ang);
    mat_rotate_y(mat, ang3);
    mat_rotate_z(mat, ang2);
    mat_translate(mat, point[0], point[1], point[2]);
}


/*****************************************************************
 * TAG( mat_rotate_axis_vec )
 *
 * Set MAT to the transformation matrix that represents the
 * concatenation between the previous transformation in MAT
 * and a rotation of the specified axis into the specified
 * vector, or vice versa.
 *
 * Arguments:
 *    axis_to_vec - TRUE if rotating axis to vector, FALSE if
 *                  rotating vector to axis.
 */
void
mat_rotate_axis_vec( Transf_mat *mat, int axis, float *vec,
                     Bool_type axis_to_vec )
{
    Transf_mat tmat;
    float min_val;
    float q[3], x[3], y[3], z[3];
    int i, min_axis;

    mat_copy( &tmat, &ident_matrix );

    /* Choose the axis that is most nearly perpendicular to the
     * given vector.
     */
    min_axis = 0;
    min_val = fabs( (double)vec[0] );
    for ( i = 1; i < 3; i++ )
        if ( fabs( (double)vec[i] ) < min_val )
        {
            min_val = fabs( (double)vec[i] );
            min_axis = i;
        }

    VEC_SET( q, 0.0, 0.0, 0.0 );
    q[min_axis] = 1.0;

    switch ( axis )
    {
    case 0:
        /* Rotate the X axis. */
        VEC_SET( x, vec[0], vec[1], vec[2] );
        vec_norm( x );
        VEC_CROSS( y, x, q );
        vec_norm( y );
        VEC_CROSS( z, x, y );
        vec_norm( z );
        break;

    case 1:
        /* Rotate the Y axis. */
        VEC_SET( y, vec[0], vec[1], vec[2] );
        vec_norm( y );
        VEC_CROSS( x, y, q );
        vec_norm( x );
        VEC_CROSS( z, x, y );
        vec_norm( z );
        break;

    case 2:
        /* Rotate the Z axis. */
        VEC_SET( z, vec[0], vec[1], vec[2] );
        vec_norm( z );
        VEC_CROSS( y, z, q );
        vec_norm( y );
        VEC_CROSS( x, y, z );
        vec_norm( x );
        break;
    }

    if ( axis_to_vec )
    {
        for ( i = 0; i < 3; i++ )
            tmat.mat[0][i] = x[i];
        for ( i = 0; i < 3; i++ )
            tmat.mat[1][i] = y[i];
        for ( i = 0; i < 3; i++ )
            tmat.mat[2][i] = z[i];
    }
    else
    {
        for ( i = 0; i < 3; i++ )
            tmat.mat[i][0] = x[i];
        for ( i = 0; i < 3; i++ )
            tmat.mat[i][1] = y[i];
        for ( i = 0; i < 3; i++ )
            tmat.mat[i][2] = z[i];
    }

    mat_mul( mat, mat, &tmat );
}


/*****************************************************************
 * TAG( mat_rotate_vec_vec )
 *
 * Set MAT to the transformation matrix that represents the
 * concatenation between the previous transformation in MAT
 * and a rotation of vector 1 into vector 2.
 */
void
mat_rotate_vec_vec( Transf_mat *mat, float *vec1, float *vec2 )
{
    /* Rotate vec 1 to the X axis, then rotate X axis to vec 2. */
    mat_rotate_axis_vec( mat, 0, vec1, FALSE );
    mat_rotate_axis_vec( mat, 0, vec2, TRUE );
}


/*****************************************************************
 * TAG( invert_rot_mat )
 *
 * Invert a rotation matrix.  Simply need to transpose it.
 */
void
invert_rot_mat( Transf_mat *inv_mat, Transf_mat *rot_mat )
{
    int i, j;

    for ( i = 0; i < 4; i++ )
        for ( j = 0; j < 4; j++ )
            inv_mat->mat[j][i] = rot_mat->mat[i][j];
}


/*****************************************************************
 * TAG( mat_scale )
 *
 * Set MAT to the transformation matrix that represents the
 * concatenation between the previous transformation in MAT
 * and a scaling with the scaling factors XSCALE, YSCALE and ZSCALE
 *
 * [a  b  c  0]   [Sx  0  0  0]     [a*Sx  b*Sy  c*Sz  0]
 * [d  e  f  0]   [ 0 Sy  0  0]     [d*Sx  e*Sy  f*Sz  0]
 * [g  h  i  0] * [ 0  0 Sz  0]  =  [g*Sx  h*Sy  i*Sz  0]
 * [j  k  l  1]   [ 0  0  0  1]     [j*Sx  k*Sy  l*Sz  1]
 */
void
mat_scale( Transf_mat *mat, float xscale, float yscale, float zscale )
{
    int   i;

    for (i = 0; i < 4; ++i)
    {
        mat->mat[i][0] *= xscale;
        mat->mat[i][1] *= yscale;
        mat->mat[i][2] *= zscale;
    }
}


/*****************************************************************
 * TAG( mat_mirror_plane )
 *
 * Set MAT to the transformation matrix that represents the
 * concatenation between the previous transformation in MAT
 * and a mirroring in the plane defined by the point POINT
 * and the normal vector NORM.
 */
void
mat_mirror_plane( Transf_mat *mat, float *point, float *norm )
{
    Transf_mat   tmp;
    float   factor;

    /* The first thing we do is to make a transformation matrix */
    /* for mirroring through a plane with the same normal vector */
    /* as our, but through the origin instead. */
    factor = 2.0 / (norm[0] * norm[0] + norm[1] * norm[1]
                    + norm[2] * norm[2]);

    /* The diagonal elements. */
    tmp.mat[0][0] = 1 - factor * norm[0] * norm[0];
    tmp.mat[1][1] = 1 - factor * norm[1] * norm[1];
    tmp.mat[2][2] = 1 - factor * norm[2] * norm[2];

    /* The rest of the matrix */
    tmp.mat[1][0] = tmp.mat[0][1] = -factor * norm[0] * norm[1];
    tmp.mat[2][0] = tmp.mat[0][2] = -factor * norm[0] * norm[2];
    tmp.mat[2][1] = tmp.mat[1][2] = -factor * norm[1] * norm[2];
    tmp.mat[3][0] = tmp.mat[3][1] = tmp.mat[3][2] = 0.0;

    /* Do the actual transformation. This is done in 3 steps: */
    /* 1) Translate the plane so that it goes through the origin. */
    /* 2) Do the actual mirroring. */
    /* 3) Translate it all back to the starting position. */
    mat_translate(mat, -point[0], -point[1], -point[2]);
    mat_mul(mat, mat, &tmp);
    mat_translate(mat, point[0], point[1], point[2]);
}


/*****************************************************************
 * TAG( mat_mul )
 *
 * Multiply the Matrix A with the Matrix B, and store the result
 * into the Matrix RES. It is possible for RES to point to the
 * same Matrix as either A or B since the result is stored into
 * a temporary during computation.
 *
 * [a b c 0]  [A B C 0]     [aA+bD+cG    aB+bE+cH    aC+bF+cI    0]
 * [d e f 0]  [D E F 0]     [dA+eD+fG    dB+eE+fH    dC+eF+fI    0]
 * [g h i 0]  [G H I 0]  =  [gA+hD+iG    gB+hE+iH    gC+hF+iI    0]
 * [j k l 1]  [J K L 1]     [jA+kD+lG+J  jB+kE+lH+K  jC+kF+lI+L  1]
 */
void
mat_mul( Transf_mat *res, Transf_mat *a, Transf_mat *b )
{
    Transf_mat   tmp;
    int      i;

    for (i = 0; i < 4; ++i)
    {
        tmp.mat[i][0] = a->mat[i][0] * b->mat[0][0]
                        + a->mat[i][1] * b->mat[1][0]
                        + a->mat[i][2] * b->mat[2][0];
        tmp.mat[i][1] = a->mat[i][0] * b->mat[0][1]
                        + a->mat[i][1] * b->mat[1][1]
                        + a->mat[i][2] * b->mat[2][1];
        tmp.mat[i][2] = a->mat[i][0] * b->mat[0][2]
                        + a->mat[i][1] * b->mat[1][2]
                        + a->mat[i][2] * b->mat[2][2];
    }

    tmp.mat[3][0] += b->mat[3][0];
    tmp.mat[3][1] += b->mat[3][1];
    tmp.mat[3][2] += b->mat[3][2];

    mat_copy(res, &tmp);
}


/*****************************************************************
 * TAG( point_transform )
 *
 * Transform the Point3d VEC with the transformation matrix MAT, and
 * put the result into the vector *RES.
 *
 *               [a  b  c  0]
 *               [d  e  f  0]
 * [x  y  z  1]  [g  h  i  0]  =  [ax+dy+gz+j  bx+ey+hz+k  cx+fy+iz+l  1]
 *               [j  k  l  1]
 */
void
point_transform( float *res, float *vec, Transf_mat *mat)
{
    res[0] = (double) mat->mat[0][0] * vec[0]
             + (double) mat->mat[1][0] * vec[1]
             + (double) mat->mat[2][0] * vec[2]
             + (double) mat->mat[3][0];
    res[1] = (double) mat->mat[0][1] * vec[0]
             + (double) mat->mat[1][1] * vec[1]
             + (double) mat->mat[2][1] * vec[2]
             + (double) mat->mat[3][1];
    res[2] = (double) mat->mat[0][2] * vec[0]
             + (double) mat->mat[1][2] * vec[1]
             + (double) mat->mat[2][2] * vec[2]
             + (double) mat->mat[3][2];
}


/*****************************************************************
 * TAG( mat_to_angles )
 *
 * Decompose a rotation matrix into three successive rotations
 *
 * [Rx][Ry][Rz]
 *
 * Note that the conversion is not unique when the "Ry" rotation
 * is a multiple of PI radians.
 */
void
mat_to_angles( Transf_mat *mat, float *angles )
{
    float cy, tmp;

    /*
     * We wish to decompose this into [Rx][Ry][Rz].  If the rotation
     * about X is by an angle a, about Y by b and about Z by c, the
     * matrix now looks like
     * [ cos(b)*cos(c)          cos(b)*sin(c)           -sin(b)         ]
     * [                                                                ]
     * [ sin(a)*sin(b)*cos(c)   sin(a)*sin(b)*sin(c)    sin(a)*cos(b)   ]
     * [   -cos(a)*sin(c)           +cos(a)*cos(c)                      ]
     * [                                                                ]
     * [ cos(a)*sin(b)*cos(c)   cos(a)*sin(b)*sin(c)    cos(a)*cos(b)   ]
     * [   +sin(a)*sin(c)           -sin(a)*cos(c)                      ]
     *
     * Thus, b = arcsin(-mat[0][2]), then, if cos(b) != 0
     * a can easily be derived from mat[1][2] and mat[2][2],
     * and c from mat[0][1] and mat[0][0].
     * If cos(b) == 0, the matrix reduces to
     * [ 0              0               +/-1  ]
     * [ sin(a +/- c)   cos(a +/- c)     0    ]
     * [ cos(a +/- c)   -sin(a +/- c)    0    ]
     *
     * In which case, we can arbitrarily set c to 0 and easily derive
     * a from mat[1][0] and mat[1][1].
     */

    tmp = mat->mat[0][2];
    tmp = BOUND( -1.0, tmp, 1.0 );

    angles[1] = asin( - (double)tmp );
    cy = cos( (double)angles[1] );
    if ( ! APX_EQ( cy, 0.0 ) )
    {
        angles[0] = atan2( (double)(mat->mat[1][2]/cy),
                           (double)(mat->mat[2][2]/cy) );
        angles[2] = atan2( (double)(mat->mat[0][1]/cy),
                           (double)(mat->mat[0][0]/cy) );
    }
    else
    {
        angles[0] = atan2( (double)mat->mat[1][0], (double)mat->mat[1][1] );
        angles[2] = 0;
    }
}


/*****************************************************************
 * TAG( test_pt_plane )
 *
 * Test how a point lies with respect to a plane.  Returns
 *       1 for interior (opposite plane normal)
 *       -1 for exterior
 *       0 for point on the plane
 * The plane is defined by a point and a normal vector.
 */
int
test_pt_plane( testpt, ppt, pnorm )
float testpt[3];
float ppt[3];
float pnorm[3];
{
    return( - SIGN( VEC_DOT( pnorm, testpt ) - VEC_DOT( pnorm, ppt ) ) );
}

