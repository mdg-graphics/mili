/* $Id$ */
/*
 * frame.c - Routines for global to local reference frame transformations.
 *
 *      Thomas Spelce
 *      Lawrence Livermore National Laboratory
 *      Sept 24 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */

#include "viewer.h"


/************************************************************
 * TAG( global_to_local_mtx )
 *
 * Computes the matrix for transforming global stress to local
 * element stress for shells.
 */
void
global_to_local_mtx( analy, elem, localMat )
Analysis *analy;
int elem;
float localMat[3][3];
{
    Nodal *currentGeom;
    Shell_geom *shells;
    float x[4], y[4], z[4];
    float xn[3], yn[3], zn[3];
    int i;

    shells = analy->geom_p->shells;
    currentGeom = analy->state_p->nodes;

    /* Get the shell element geometry. */
    for ( i = 0; i < 4; i++ )
    {
        x[i] = currentGeom->xyz[0][ shells->nodes[i][elem] ];
        y[i] = currentGeom->xyz[1][ shells->nodes[i][elem] ];
        z[i] = currentGeom->xyz[2][ shells->nodes[i][elem] ];
    }

    /* Get average vectors for the sides of the element. */
    xn[0] = -x[0] + x[1] + x[2] - x[3];
    xn[1] = -y[0] + y[1] + y[2] - y[3];
    xn[2] = -z[0] + z[1] + z[2] - z[3];
    yn[0] = -x[0] - x[1] + x[2] + x[3];
    yn[1] = -y[0] - y[1] + y[2] + y[3];
    yn[2] = -z[0] - z[1] + z[2] + z[3];

    /* Get normal to element (assumes element is planar). */
    VEC_CROSS( zn, xn, yn );

    /* An orthonormal basis is what we need. */
    vec_norm( zn );
    vec_norm( yn );
    VEC_CROSS( xn, yn, zn)

    for ( i = 0; i < 3; i++ )    
    {
        localMat[i][0] =  xn[i];
        localMat[i][1] =  yn[i];
        localMat[i][2] =  zn[i];
    }
}

       
/************************************************************
 * TAG( transform_tensor )
 *
 * Transform the stress/strain tensor with a change-of-coordinates
 * transformation matrix.
 *
 *                  t
 *     sigma_new = A * sigma * A
 *
 * The tensor comes in as a vector.  The routine is hardcoded for
 * the above matrix multiply.
 */
void
transform_tensor( ten, mat )
float ten[6];
float mat[3][3];
{
    float new[6], sub1[3], sub2[3], sub3[3];
    int i;

    /* Compute common sub-expressions. */
    sub1[0] = ten[0]*mat[0][0] + ten[3]*mat[1][0] + ten[5]*mat[2][0];
    sub1[1] = ten[3]*mat[0][0] + ten[1]*mat[1][0] + ten[4]*mat[2][0];
    sub1[2] = ten[5]*mat[0][0] + ten[4]*mat[1][0] + ten[2]*mat[2][0];

    sub2[0] = ten[0]*mat[0][1] + ten[3]*mat[1][1] + ten[5]*mat[2][1];
    sub2[1] = ten[3]*mat[0][1] + ten[1]*mat[1][1] + ten[4]*mat[2][1];
    sub2[2] = ten[5]*mat[0][1] + ten[4]*mat[1][1] + ten[2]*mat[2][1];

    sub3[0] = ten[0]*mat[0][2] + ten[3]*mat[1][2] + ten[5]*mat[2][2];
    sub3[1] = ten[3]*mat[0][2] + ten[1]*mat[1][2] + ten[4]*mat[2][2];
    sub3[2] = ten[5]*mat[0][2] + ten[4]*mat[1][2] + ten[2]*mat[2][2];

    /* Get the transformed vector. */
    new[0] = mat[0][0]*sub1[0] + mat[1][0]*sub1[1] + mat[2][0]*sub1[2];
    new[1] = mat[0][1]*sub2[0] + mat[1][1]*sub2[1] + mat[2][1]*sub2[2];
    new[2] = mat[0][2]*sub3[0] + mat[1][2]*sub3[1] + mat[2][2]*sub3[2];
    new[3] = mat[0][0]*sub2[0] + mat[1][0]*sub2[1] + mat[2][0]*sub2[2];
    new[4] = mat[0][1]*sub3[0] + mat[1][1]*sub3[1] + mat[2][1]*sub3[2];
    new[5] = mat[0][0]*sub3[0] + mat[1][0]*sub3[1] + mat[2][0]*sub3[2];

    for ( i = 0; i < 6; i++ )
        ten[i] = new[i];
}

