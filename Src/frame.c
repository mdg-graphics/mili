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


static Bool_type transpose_tensors();


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
 * TAG( transform_tensors )
 *
 * Transform an array of stress/strain tensors with a 
 * change-of-coordinates transformation matrix.
 *
 *                  t
 *     sigma_new = A * sigma * A
 *
 * The tensor comes in as a vector.  The routine is hardcoded for
 * the above matrix multiply.
 */
void
transform_tensors( qty, tensors, mat )
int qty;
float *tensors;
float mat[][3];
{
    float new[6], sub1[3], sub2[3], sub3[3];
    float *p_ten;
    int i, j;

    p_ten = tensors;
    for ( i = 0; i < qty; i++ )
    {
        /* Compute common sub-expressions. */
        sub1[0] = p_ten[0]*mat[0][0] + p_ten[3]*mat[1][0] + p_ten[5]*mat[2][0];
        sub1[1] = p_ten[3]*mat[0][0] + p_ten[1]*mat[1][0] + p_ten[4]*mat[2][0];
        sub1[2] = p_ten[5]*mat[0][0] + p_ten[4]*mat[1][0] + p_ten[2]*mat[2][0];

        sub2[0] = p_ten[0]*mat[0][1] + p_ten[3]*mat[1][1] + p_ten[5]*mat[2][1];
        sub2[1] = p_ten[3]*mat[0][1] + p_ten[1]*mat[1][1] + p_ten[4]*mat[2][1];
        sub2[2] = p_ten[5]*mat[0][1] + p_ten[4]*mat[1][1] + p_ten[2]*mat[2][1];

        sub3[0] = p_ten[0]*mat[0][2] + p_ten[3]*mat[1][2] + p_ten[5]*mat[2][2];
        sub3[1] = p_ten[3]*mat[0][2] + p_ten[1]*mat[1][2] + p_ten[4]*mat[2][2];
        sub3[2] = p_ten[5]*mat[0][2] + p_ten[4]*mat[1][2] + p_ten[2]*mat[2][2];

        /* Get the transformed vector. */
        new[0] = mat[0][0]*sub1[0] + mat[1][0]*sub1[1] + mat[2][0]*sub1[2];
        new[1] = mat[0][1]*sub2[0] + mat[1][1]*sub2[1] + mat[2][1]*sub2[2];
        new[2] = mat[0][2]*sub3[0] + mat[1][2]*sub3[1] + mat[2][2]*sub3[2];
        new[3] = mat[0][0]*sub2[0] + mat[1][0]*sub2[1] + mat[2][0]*sub2[2];
        new[4] = mat[0][1]*sub3[0] + mat[1][1]*sub3[1] + mat[2][1]*sub3[2];
        new[5] = mat[0][0]*sub3[0] + mat[1][0]*sub3[1] + mat[2][0]*sub3[2];

        for ( j = 0; j < 6; j++ )
            p_ten[j] = new[j];
        
        /* Increment to point to the next element's tensor. */
        p_ten += 6;
    }
}


/************************************************************
 * TAG( transform_stress_strain )
 *
 * Transform stress or strain tensors for hex or shell 
 * elements at the current state according to the currently 
 * defined coordinate transformation matrix.
 *
 * The transformed components are left in analy->tmp_result.
 */
Bool_type
transform_stress_strain( which, single_component, analy )
int which;
Bool_type single_component;
Analysis *analy;
{
    int qty;
    Bool_type rval;

    /* Load the tensors for all elements from the database. */
    rval = get_tensor_result( analy->cur_state, which, 
                              analy->tmp_result[0] );
    if ( !rval )
        return FALSE;
    
    qty = ( which == VAL_HEX_SIGX ) ? analy->geom_p->bricks->cnt
                                    : analy->geom_p->shells->cnt;
    
    /* Do it... */
    transform_tensors( qty, analy->tmp_result[0],
                       analy->tensor_transform_matrix );
    
    /*
     * Re-order tensors into separate component arrays (i.e., x,
     * y, z, xy, yz, zx) before returning.
     */
    return transpose_tensors( which, single_component, analy );
}


/************************************************************
 * TAG( transpose_tensors )
 *
 * Transpose the tensors in analy->tmp_result[0], storing back
 * into analy->tmp_result[0...5].
 */
static Bool_type
transpose_tensors( which, single_component, analy )
int which;
Bool_type single_component;
Analysis *analy;
{
    int qty, idx, i;
    float *p_tmp, *p_t0, *p_t1, *p_t2, *p_t3, *p_t4, *p_t5;
    float (*tens)[6];
    double *p_dsrc, *p_dbound, *p_ddest;
    
    qty = ( which == VAL_HEX_SIGX ) ? analy->geom_p->bricks->cnt
                                    : analy->geom_p->shells->cnt;
    
    /* 
     * If only a single component is required, transpose back into
     * the correct element result array, otherwise copy and transpose
     * back into analy->tmp_result;
     */
    
    if ( single_component )
    {
        tens = (float (*)[6]) analy->tmp_result[0];

        if ( which == VAL_HEX_SIGX )
        {
            p_t0 = analy->hex_result;
            idx = analy->result_id - VAL_SHARE_SIGX;
        }
        else
        {
            p_t0 = analy->shell_result;
            idx = analy->result_id - VAL_SHARE_EPSX;
        }
                                         
        /* Extract the requested component into the result array. */
        for ( i = 0; i < qty; i++ )
            p_t0[i] = tens[i][idx];
    }
    else
    {
        p_tmp = NEW_N( float, qty * 6, "Temp tensors array" );
        if ( p_tmp == NULL )
            return FALSE;
    
        /* Copy array of tensors to temp storage with double-sized moves. */
        p_dsrc = (double *) analy->tmp_result[0];
        p_dbound = (double *) (analy->tmp_result[0] + qty * 6);
        p_ddest = (double *) p_tmp;
        for ( ; p_dsrc < p_dbound; *p_ddest++ = *p_dsrc++ );
        
        /* Assign local pointers. */
        p_t0 = analy->tmp_result[0];
        p_t1 = analy->tmp_result[1];
        p_t2 = analy->tmp_result[2];
        p_t3 = analy->tmp_result[3];
        p_t4 = analy->tmp_result[4];
        p_t5 = analy->tmp_result[5];
        
        /* Now transpose back into analy->tmp_result. */
        
        tens = (float (*)[6]) p_tmp;
        for ( i = 0; i < qty; i++ )
        {
            p_t0[i] = tens[i][0];
            p_t1[i] = tens[i][1];
            p_t2[i] = tens[i][2];
            p_t3[i] = tens[i][3];
            p_t4[i] = tens[i][4];
            p_t5[i] = tens[i][5];
        }
    
        free( p_tmp );
    }
    
    return TRUE;
}


