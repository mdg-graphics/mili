/* $Id$ */
/*
 * shape.c - Routines related to finite element shape functions.
 *
 *      Tom Spelce
 *      Lawrence Livermore National Laboratory
 *      October 2, 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */

#include "viewer.h"

static float determinant3x3();
static Bool_type invert3x3();
static void hex_jacobian();


/************************************************************
 * TAG( shape_fns_2d )
 *
 * Computes the 2D shape functions at (r,s) where -1 <= r,s >= 1.
 * The first vertex is at (-1,-1).
 */
void
shape_fns_2d( r, s, h )
float r;
float s;
float h[4];
{
    float rp, sp, rm, sm;

    rp = 1.0 + r;
    sp = 1.0 + s;
    rm = 1.0 - r;
    sm = 1.0 - s;

    h[0] = 0.25*rm*sm;
    h[1] = 0.25*rp*sm;
    h[2] = 0.25*rp*sp;
    h[3] = 0.25*rm*sp;
}


/************************************************************
 * TAG( shape_derivs_2d )
 *
 * Computes the partial derivatives of the 2D shape functions
 * at (r,s) where -1 <= r,s >= 1.  The first vertex is at (-1,-1).
 */
void
shape_derivs_2d( r, s, dhdr, dhds )
float r;
float s;
float dhdr[4];
float dhds[4];
{
    dhdr[0] = -0.25*(1.0-s);
    dhdr[1] = -dhdr[0];
    dhdr[2] = 0.25*(1.0+s);
    dhdr[3] = -dhdr[2];

    dhds[0] = -0.25*(1.0-r);
    dhds[1] = -0.25*(1.0+r);
    dhds[2] = -dhds[1];
    dhds[3] = -dhds[0];
}


/************************************************************
 * TAG( shape_fns_3d )
 *
 * Computes the 3D shape functions at (r,s,t) where -1 <= r,s,t >= 1.
 */
void
shape_fns_3d( r, s, t, h ) 
float r;
float s;
float t;
float h[8];
{
    float rp, sp, tp, rm, sm, tm;

    rp = 1.0 + r;
    sp = 1.0 + s;
    tp = 1.0 + t;
    rm = 1.0 - r;
    sm = 1.0 - s;
    tm = 1.0 - t;

    h[0] = 0.125*rm*sm*tm;
    h[1] = 0.125*rp*sm*tm;
    h[2] = 0.125*rp*sp*tm;
    h[3] = 0.125*rm*sp*tm;
    h[4] = 0.125*rm*sm*tp;
    h[5] = 0.125*rp*sm*tp;
    h[6] = 0.125*rp*sp*tp;
    h[7] = 0.125*rm*sp*tp;
}


/************************************************************
 * TAG( shape_derivs_3d )
 *
 * Computes the partial derivatives of the 3D shape functions
 * at (r,s,t) where -1 <= r,s,t >= 1.  
 */
void
shape_derivs_3d( r, s, t, dhdr, dhds, dhdt )
float r;  
float s;
float t;
float dhdr[8];
float dhds[8];
float dhdt[8];
{
    dhdr[0] = -0.125*(1.0-s)*(1.0-t);
    dhdr[1] =  0.125*(1.0-s)*(1.0-t);
    dhdr[2] =  0.125*(1.0+s)*(1.0-t);
    dhdr[3] = -0.125*(1.0+s)*(1.0-t); 
    dhdr[4] = -0.125*(1.0-s)*(1.0+t); 
    dhdr[5] =  0.125*(1.0-s)*(1.0+t); 
    dhdr[6] =  0.125*(1.0+s)*(1.0+t); 
    dhdr[7] = -0.125*(1.0+s)*(1.0+t); 

    dhds[0] = -0.125*(1.0-r)*(1.0-t);
    dhds[1] = -0.125*(1.0+r)*(1.0-t); 
    dhds[2] =  0.125*(1.0+r)*(1.0-t); 
    dhds[3] =  0.125*(1.0-r)*(1.0-t); 
    dhds[4] = -0.125*(1.0-r)*(1.0+t); 
    dhds[5] = -0.125*(1.0+r)*(1.0+t); 
    dhds[6] =  0.125*(1.0+r)*(1.0+t); 
    dhds[7] =  0.125*(1.0-r)*(1.0+t); 

    dhdt[0] = -0.125*(1.0-r)*(1.0-s);
    dhdt[1] = -0.125*(1.0+r)*(1.0-s); 
    dhdt[2] = -0.125*(1.0+r)*(1.0+s); 
    dhdt[3] = -0.125*(1.0-r)*(1.0+s); 
    dhdt[4] =  0.125*(1.0-r)*(1.0-s); 
    dhdt[5] =  0.125*(1.0+r)*(1.0-s); 
    dhdt[6] =  0.125*(1.0+r)*(1.0+s); 
    dhdt[7] =  0.125*(1.0-r)*(1.0+s); 
}


/************************************************************
 * TAG( determinant3x3 )
 *
 * Returns the determinant of a 3x3 matrix.
 */
static float 
determinant3x3( mat )
float mat[3][3];
{
    float val;

    val = mat[0][0]*(mat[1][1]*mat[2][2] - mat[1][2]*mat[2][1]) -
          mat[0][1]*(mat[1][0]*mat[2][2] - mat[1][2]*mat[2][0]) +
          mat[0][2]*(mat[1][0]*mat[2][1] - mat[1][1]*mat[2][0]);

    return( val > 0.0 ? val : -1.0 );
}


/************************************************************
 * TAG( invert3x3 )
 *
 * Computes the inverse of a 3x3 matrix.  Returns FALSE if
 * the matrix can't be inverted.
 */
static Bool_type
invert3x3( mat, inverse )
float mat[3][3];
float inverse[3][3];
{
    float detM;

    /* Compute the determinant. */
    detM = 1.0 / determinant3x3( mat );

    if ( detM <= 0 )
        return( FALSE );
 
    /* Compute inverse. */
    inverse[0][0] =  detM*( mat[1][1]*mat[2][2] - mat[1][2]*mat[2][1] );
    inverse[1][0] = -detM*( mat[1][0]*mat[2][2] - mat[1][2]*mat[2][0] );
    inverse[2][0] =  detM*( mat[1][0]*mat[2][1] - mat[1][1]*mat[2][0] );
    inverse[0][1] = -detM*( mat[0][1]*mat[2][2] - mat[0][2]*mat[2][1] );
    inverse[1][1] =  detM*( mat[0][0]*mat[2][2] - mat[0][2]*mat[2][0] );
    inverse[2][1] = -detM*( mat[0][0]*mat[2][1] - mat[0][1]*mat[2][0] );
    inverse[0][2] =  detM*( mat[0][1]*mat[1][2] - mat[0][2]*mat[1][1] );
    inverse[1][2] = -detM*( mat[0][0]*mat[1][2] - mat[0][2]*mat[1][0] );
    inverse[2][2] =  detM*( mat[0][0]*mat[1][1] - mat[0][1]*mat[1][0] );

    return( TRUE );
}


/************************************************************
 * TAG( hex_jacobian )
 *
 * Computes the Jacobian of the shape functions for a
 * hexahedral element.
 */
static void
hex_jacobian( verts, dN1, dN2, dN3, Jac )
float verts[8][3];      /* Vertex positions. */
float dN1[8];           /* Shape function derivatives. */
float dN2[8];
float dN3[8];
float Jac[3][3];        /* Return the Jacobian matrix. */
{
    int i, j;

    for ( i = 0; i < 3; i++ )
        for ( j = 0; j < 3; j++ )
            Jac[i][j] = 0.0;

    for ( i = 0; i < 8; i++ )
    {
        Jac[0][0] += verts[i][0]*dN1[i];
        Jac[0][1] += verts[i][0]*dN2[i];
        Jac[0][2] += verts[i][0]*dN3[i];
        Jac[1][0] += verts[i][1]*dN1[i];
        Jac[1][1] += verts[i][1]*dN2[i];
        Jac[1][2] += verts[i][1]*dN3[i];
        Jac[2][0] += verts[i][2]*dN1[i];
        Jac[2][1] += verts[i][2]*dN2[i];
        Jac[2][2] += verts[i][2]*dN3[i];
    }
}


/************************************************************
 * TAG( quad_jacobian )
 *
 * Computes the Jacobian of the shape functions for a
 * quadrilateral element.
 */
/* Currently unused. */
/*
static void
quad_jacobian( verts, dN1, dN2, dN3, Jac )
float verts[4][3];      * Vertex positions. *
float dN1[4];           * Shape function derivatives. *
float dN2[4];
float dN3[4];
float Jac[3][3];        * Return the Jacobian matrix. *
{
    int i, j;

    for ( i = 0; i < 3; i++ )
        for ( j = 0; j < 3; j++ )
            Jac[i][j] = 0.0;

    for ( i = 0; i < 4; i++ )
    {
        Jac[0][0] += verts[i][0]*dN1[i];
        Jac[0][1] += verts[i][0]*dN2[i];
        Jac[0][2] += verts[i][0]*dN3[i];
        Jac[1][0] += verts[i][1]*dN1[i];
        Jac[1][1] += verts[i][1]*dN2[i];
        Jac[1][2] += verts[i][1]*dN3[i];
        Jac[2][0] += verts[i][2]*dN1[i];
        Jac[2][1] += verts[i][2]*dN2[i];
        Jac[2][2] += verts[i][2]*dN3[i];
    }
}
*/


/************************************************************
 * TAG( pt_in_hex )
 *
 * For a specified point inside a brick element, calculate the
 * element-centered coordinates of the point.  The element
 * vertices are stored verts; the point is stored in pt, and
 * the result (r,s,t) is returned in solut.  The routine
 * returns TRUE if the point is inside the element and FALSE
 * otherwise.
 */
Bool_type
pt_in_hex( verts, pt, solut )
float verts[8][3];
float pt[3];
float solut[3];
{
    float xi, eta, psi;
    float dxi, deta, dpsi;
    float N[8];
    float dN1[8], dN2[8], dN3[8];
    float F[3];
    float xmin, xmax, ymin, ymax, zmin, zmax;
    float Jac[3][3];
    float invJac[3][3];
    int itercnt;
    int i, j;

    static float eps = 1.0e-5;          /* Iteration tolerance. */
    itercnt = 0;

    /* Initial guess. */
    xi = 0.0;
    eta = 0.0;
    psi = 0.0;

    /* Set the initial values to be greater than eps. */
    dxi = deta = dpsi = 1.0;

    while ( ( fabs((double)dxi) >= eps || fabs((double)deta) >= eps ||
              fabs((double)dpsi) >= eps ) && itercnt <= 20 )
    {
        /* Compute the shape functions and their derivatives. */
        shape_fns_3d( xi, eta, psi, N );
        shape_derivs_3d( xi, eta, psi, dN1, dN2, dN3 );

        F[0] = F[1] = F[2] = 0.0;
        for ( i = 0; i < 8; i++ )
            for ( j = 0; j < 3; j++ )
                F[j] += verts[i][j]*N[i];

        /* Calculate the error difference from the real value. */
        for ( i = 0; i < 3; i++ )
            F[i] = - ( F[i] - pt[i] );

        /* Quit the loop if the error differences are near zero. */
/*
        if ( APX_EQ(F[0],0.0) && APX_EQ(F[1],0.0) && APX_EQ(F[2],0.0) )
            break;
*/

        /* Compute the Jacobian transformation. */
        hex_jacobian( verts, dN1, dN2, dN3, Jac );
        if ( !invert3x3( Jac, invJac ) )
        {
            popup_dialog( WARNING_POPUP, "Singular element!" );
            return( FALSE ); 
        }

        /* Solve the the deltas. */
        dxi = 0.0;
        deta  = 0.0;
        dpsi  = 0.0;

        for ( i = 0; i < 3; i++ ) 
        {
            dxi += invJac[0][i]*F[i];
            deta  += invJac[1][i]*F[i];
            dpsi  += invJac[2][i]*F[i];
        }

        xi = xi + dxi;
        eta = eta + deta;
        psi = psi + dpsi;

        itercnt++;
    }

    /* We use approximate inclusion below because the Newton iteration
     * above only gets to within epsilon of the correct value.
     */
    if ( xi >= -1.0001 && xi <= 1.0001 &&
         eta >= -1.0001 && eta <= 1.0001 &&
         psi >= -1.0001 && psi <= 1.0001 )
    {
/*
wrt_text( "Natural coordinates: %f %f %f\n", xi, eta, psi );
wrt_text( "Iteration converged in %d iterations\n", itercnt );
*/
        solut[0] = BOUND( -1.0, xi, 1.0 );
        solut[1] = BOUND( -1.0, eta, 1.0 );
        solut[2] = BOUND( -1.0, psi, 1.0 );
        return TRUE;
    }
    else
    {
/*
wrt_text( "Natural coordinates: %f %f %f -- Out of Range\n",
 xi, eta, psi );
*/
        solut[0] = xi;
        solut[1] = eta;
        solut[2] = psi;
        return FALSE;
    }
}


/************************************************************
 * TAG( pt_in_quad )
 *
 * For a specified point inside a 2D quadrilateral element,
 * calculate the element-centered coordinates of the point.
 * The element vertices are stored verts; the point is
 * stored in pt, and the result (r,s) is returned in solut.
 * The routine returns TRUE if the point is inside the element
 * and FALSE otherwise.
 */
Bool_type
pt_in_quad( verts, pt, solut )
float verts[4][2];
float pt[2];
float solut[2];
{
    float b1, b2, L1, L2, L3, G1, G2, G3, a, b, c, eta, xi, tmp;

    L1 = ( -verts[0][0] + verts[1][0] + verts[2][0] - verts[3][0] );
    L2 = ( -verts[0][0] - verts[1][0] + verts[2][0] + verts[3][0] );
    L3 = (  verts[0][0] - verts[1][0] + verts[2][0] - verts[3][0] );

    G1 = ( -verts[0][1] + verts[1][1] + verts[2][1] - verts[3][1] );
    G2 = ( -verts[0][1] - verts[1][1] + verts[2][1] + verts[3][1] );
    G3 = (  verts[0][1] - verts[1][1] + verts[2][1] - verts[3][1] );

    b1 = 4.0*pt[0] - ( verts[0][0] + verts[1][0] + verts[2][0] + verts[3][0] );
    b2 = 4.0*pt[1] - ( verts[0][1] + verts[1][1] + verts[2][1] + verts[3][1] );

    a = G2*L3 - G3*L2;
    b = G3*b1 + G2*L1 - G1*L2 - b2*L3;
    c = G1*b1 - b2*L1;

    /* Solve quadratic equation. */
    if ( a == 0.0 )
    {
        if ( b == 0.0 )
            return FALSE;
        else
            eta = -c / b;
    }
    else
    {
        if ( b < 0.0 )
            eta = ( -b - sqrt( b*b - 4.0*a*c ) ) / (2.0*a);
        else
            eta = ( -b + sqrt( b*b - 4.0*a*c ) ) / (2.0*a);
    }

    tmp = L1 + L3*eta;
    if ( tmp != 0.0 )
        xi = ( b1 - L2*eta ) / tmp;

    if ( DEF_LT( xi, -1.0 ) || DEF_GT( xi, 1.0 ) ||
         DEF_LT( eta, -1.0 ) || DEF_GT( eta, 1.0 ) )
    {
        solut[0] = xi;
        solut[1] = eta;
        return FALSE;
    }

    solut[0] = BOUND( -1.0, xi, 1.0 );
    solut[1] = BOUND( -1.0, eta, 1.0 );
    return TRUE;
}


