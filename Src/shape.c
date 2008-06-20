/* $Id$ */
/*
 * shape.c - Routines related to finite element shape functions.
 *
 *      Tom Spelce
 *      Lawrence Livermore National Laboratory
 *      October 2, 1992
 *
 * Added pt_in_tet() and pt_in_tri() to support Mili db's.
 *      Doug Speck, 10/97
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
 */

#include "viewer.h"

static float determinant3x3( float mat[3][3] );
static Bool_type invert3x3( float mat[3][3], float inverse[3][3] );
static void hex_jacobian( float verts[8][3], float dN1[8], float dN2[8], 
                          float dN3[8], float Jac[3][3] );
/*
static void quad_jacobian( float verts[4][3], float dN1[4], float dN2[4], 
                           float dN3[4], float Jac[3][3] );
*/

/************************************************************
 * TAG( shape_fns_quad )
 *
 * Computes the 2D shape functions at (r,s) where -1 <= r,s >= 1.
 * The first vertex is at (-1,-1).
 */
void
shape_fns_quad( float r, float s, float h[4] )
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
 * TAG( shape_fns_surf)
 *
 * Computes the 2D shape functions at (r,s) where -1 <= r,s >= 1.
 * The first vertex is at (-1,-1).
 */
void
shape_fns_surf( float r, float s, float h[4] )
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
shape_derivs_2d( float r, float s, float dhdr[4], float dhds[4] )
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
 * TAG( shape_fns_hex )
 *
 * Computes the 3D shape functions at (r,s,t) where -1 <= r,s,t >= 1.
 */
void
shape_fns_hex( float r, float s, float t, float h[8] ) 
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
shape_derivs_3d( float r, float s, float t, float dhdr[8], float dhds[8], 
                 float dhdt[8] )
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
determinant3x3( float mat[3][3] )
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
invert3x3( float mat[3][3], float inverse[3][3] )
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
hex_jacobian( float verts[8][3], float dN1[8], float dN2[8], float dN3[8], 
              float Jac[3][3] )
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
quad_jacobian( float verts[4][3], float dN1[4], float dN2[4], float dN3[4], 
               float Jac[3][3] )
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
pt_in_hex( float verts[8][3], float pt[3], float solut[3] )
{
    float xi, eta, psi;
    float dxi, deta, dpsi;
    float N[8];
    float dN1[8], dN2[8], dN3[8];
    float F[3];
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
        shape_fns_hex( xi, eta, psi, N );
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
 * TAG( pt_in_tet )
 *
 * Calculate the volumetric coordinates of a point with 
 * respect to a tetrahedral element.  The element vertices 
 * are stored in verts; the point is stored in pt, and the 
 * result (r,s,t,u) is returned in solut.  The routine returns 
 * TRUE if the point is inside the element and FALSE otherwise.
 */
Bool_type
pt_in_tet( float verts[4][3], float pt[3], float solut[4] )
{
    float plane_031[4], plane_132[4], plane_023[4];
    float L0, L1, L2, L3;
    
    plane_three_pts( plane_031, verts[0], verts[3], verts[1] );
    plane_three_pts( plane_132, verts[1], verts[3], verts[2] );
    plane_three_pts( plane_023, verts[0], verts[2], verts[3] );
    
    L0 = (VEC_DOT( plane_132, pt ) + plane_132[3])
         / (VEC_DOT( plane_132, verts[0] ) + plane_132[3]);
    
    L1 = (VEC_DOT( plane_023, pt ) + plane_023[3])
         / (VEC_DOT( plane_023, verts[0] ) + plane_023[3]);
    
    L2 = (VEC_DOT( plane_031, pt ) + plane_031[3])
         / (VEC_DOT( plane_031, verts[0] ) + plane_031[3]);
    
    L3 = 1 - L0 - L1 - L2;
    
    solut[0] = L0;
    solut[1] = L1;
    solut[2] = L2;
    solut[3] = L3;
    
    return ( L0 <= 1 && L0 >= 0 
             && L1 <= 1 && L1 >= 0 
             && L2 <= 1 && L2 >= 0 
             && L3 <= 1 && L3 >= 0 );
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
pt_in_quad( float verts[4][3], float pt[2], float solut[2] )
{
    double b1, b2, L1, L2, L3, G1, G2, G3, a, b, c, eta, xi, tmp;

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


/************************************************************
 * TAG( pt_in_surf )
 *
 * For a specified point inside a 2D surface element,
 * calculate the element-centered coordinates of the point.
 * The element vertices are stored verts; the point is
 * stored in pt, and the result (r,s) is returned in solut.
 * The routine returns TRUE if the point is inside the element
 * and FALSE otherwise.
 */
Bool_type
pt_in_surf( float verts[4][3], float pt[2], float solut[2] )
{
    double b1, b2, L1, L2, L3, G1, G2, G3, a, b, c, eta, xi, tmp;

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


/************************************************************
 * TAG( pt_in_tri )
 *
 * Calculate the area coordinates of a point with 
 * respect to a triangular element.  The element vertices 
 * are stored in verts; the point is stored in pt, and the 
 * result (r,s,t) is returned in solut.  The routine returns 
 * TRUE if the point is inside the element and FALSE otherwise.
 */
Bool_type
pt_in_tri( float verts[3][3], float pt[2], float solut[3] )
{
    float line_12[3], line_20[3];
    float L0, L1, L2;
    
    line_two_pts( line_12, verts[1], verts[2] );
    line_two_pts( line_20, verts[2], verts[0] );
    
    L0 = (VEC_DOT_2D( line_12, pt ) + line_12[2])
         / (VEC_DOT_2D( line_12, verts[0] ) + line_12[2]);
    
    L1 = (VEC_DOT_2D( line_20, pt ) + line_20[2])
         / (VEC_DOT_2D( line_20, verts[0] ) + line_20[2]);
    
    L2 = 1 - L0 - L1;
    
    solut[0] = L0;
    solut[1] = L1;
    solut[2] = L2;
    
    return ( L0 <= 1 && L0 >= 0 
             && L1 <= 1 && L1 >= 0 
             && L2 <= 1 && L2 >= 0 );
}


