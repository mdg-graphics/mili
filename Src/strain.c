/* $Id$ */
/*
 * strain.c - Routines for computing derived strain quantities.
 *
 *      Thomas Spelce
 *      Lawrence Livermore National Laboratory
 *      Jun 25 1992
 *
 *
 * Modification History
 *
 * Date        By Whom      Description
 * ==========  ===========  ==============================================
 * 03/07/1995  S.C. Nevin   hex_principal_strain()
 *                          Change internal variables to double, and
 *                          change the zero tolerance to a smaller number.
 * 03/13/1995  S.C. Nevin   compute_hex_strain()
 *                          Fix strain rate calculation.  Must use the
 *                          velocity vector, not the velocity magnitude.
 *
 *
 * Copyright (c) 1992 Regents of the University of California
 */

#include <stdio.h>
#include <string.h>
#include "viewer.h"


static void compute_shell_strain();
static void hex_partial_deriv();
static void extract_strain_vec();
static void hex_principal_strain();
static void compute_hex_eff_strain();
static void compute_shell_eff_strain();


/************************************************************
 * TAG( compute_share_strain )
 *
 * Computes the strain at nodes for hex and shell elements.
 */
void
compute_share_strain( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    if ( analy->geom_p->bricks != NULL )
        compute_hex_strain( analy, resultArr );
    else
        memset( resultArr, 0, analy->geom_p->nodes->cnt * sizeof(float) );

    if ( analy->geom_p->shells != NULL )
    {
        if ( is_in_database( VAL_SHELL_EPSX_IN ) )
            compute_shell_strain( analy, resultArr );
        else
        {
            popup_dialog( INFO_POPUP, 
                          "No strain data present for shell elements.\n" );
            analy->show_shell_result = FALSE;
            if ( analy->geom_p->bricks == NULL )
                analy->result_id = VAL_NONE;
        }
    }
}


/************************************************************
 * TAG( compute_share_eff_strain )
 *
 * Computes the effective strain at nodes for hex and shell
 * elements.
 */
void
compute_share_eff_strain( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    if ( analy->geom_p->bricks != NULL )
    {
/**/
/*
        get_result( VAL_HEX_EPS_EFF, analy->cur_state, analy->hex_result );
        hex_to_nodal( analy->hex_result, resultArr, analy );
*/
	compute_hex_eff_strain( analy, resultArr );
    }
    else
        memset( resultArr, 0, analy->geom_p->nodes->cnt * sizeof(float) );

    if ( analy->geom_p->shells != NULL )
        compute_shell_eff_strain( analy, resultArr );
}


/************************************************************
 * TAG( compute_hex_strain )
 *
 * Computes the strain at nodes given the current geometry
 * and initial configuration.
 */
void
compute_hex_strain( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    Nodal *initGeom, *currentGeom;
    Hex_geom *bricks;
    Result_type tmp_id;
    float x[8], y[8], z[8];          /* Initial element geom. */
    float xx[8], yy[8], zz[8];       /* Current element geom. */
    float xv[8], yv[8], zv[8];       /* Current node velocities. */
    float px[8], py[8], pz[8];       /* Global derivates dN/dx,dN/dy,dN/dz. */
    float F[9];                      /* Element deformation gradient. */
    float detF;                      /* Determinant of element 
                                        deformation gradient. */
    float eps[6];                    /* Calculated strain. */
    float *resultElem;               /* Array for the element data. */
    float meanStrain;                /* Mean strain for an element. */
    int i, j, k, nd;
    float *nxv, *nyv, *nzv;          /* Nodal velocity arrays for rate calc. */

    bricks = analy->geom_p->bricks;
    initGeom = analy->geom_p->nodes;
    currentGeom = analy->state_p->nodes;

    resultElem = analy->hex_result;
    
    /* Get nodal velocities if strain rate is desired. */
    if ( analy->strain_variety == RATE )
    {
	nxv = analy->tmp_result[0];
	nyv = analy->tmp_result[1];
	nzv = analy->tmp_result[2];
	
	tmp_id = analy->result_id;
	
	analy->result_id = VAL_NODE_VELX;
	compute_node_velocity( analy, nxv );
	
	analy->result_id = VAL_NODE_VELY;
	compute_node_velocity( analy, nyv );
	
	analy->result_id = VAL_NODE_VELZ;
	compute_node_velocity( analy, nzv );

	analy->result_id = tmp_id;
    }

    for ( i = 0; i < bricks->cnt; i++ )
    {
        /* Get the initial and current geometries. */
        for ( j = 0; j < 8; j++ )
        {
            nd = bricks->nodes[j][i];
            x[j] = initGeom->xyz[0][nd];
            y[j] = initGeom->xyz[1][nd];
            z[j] = initGeom->xyz[2][nd];
            xx[j] = currentGeom->xyz[0][nd];
            yy[j] = currentGeom->xyz[1][nd];
            zz[j] = currentGeom->xyz[2][nd];
        }

        /* Nodal velocities needed for strain rate calculation. */
        if ( analy->strain_variety == RATE )
        {
           for ( j = 0; j < 8; j++ )
           {
                nd = bricks->nodes[j][i];
                xv[j] = nxv[nd];
                yv[j] = nyv[nd];
                zv[j] = nzv[nd];
           }
        }

        memset( F, 0, sizeof(float)*9 );

        switch ( analy->strain_variety )
        {
            case INFINITESIMAL: 
            case GREEN_LAGRANGE: 
                hex_partial_deriv( px, py, pz, x, y, z);
                for ( k = 0; k < 8; k++ )
                {
                    F[0] = F[0] + px[k]*xx[k];
                    F[1] = F[1] + py[k]*xx[k];
                    F[2] = F[2] + pz[k]*xx[k];
                    F[3] = F[3] + px[k]*yy[k];
                    F[4] = F[4] + py[k]*yy[k];
                    F[5] = F[5] + pz[k]*yy[k];
                    F[6] = F[6] + px[k]*zz[k];
                    F[7] = F[7] + py[k]*zz[k];
                    F[8] = F[8] + pz[k]*zz[k];
                }
                detF = F[0]*F[4]*F[8] + F[1]*F[5]*F[6] + F[2]*F[3]*F[7] 
                       - F[2]*F[4]*F[6] - F[1]*F[3]*F[8] - F[0]*F[5]*F[7];
                detF = fabs( (double)detF );
                break;

            case ALMANSI: 
                hex_partial_deriv( px, py, pz, xx, yy, zz );
                for ( k = 0; k < 8; k++ )
                {
                    F[0] = F[0] + px[k]*x[k];
                    F[1] = F[1] + py[k]*x[k];
                    F[2] = F[2] + pz[k]*x[k];
                    F[3] = F[3] + px[k]*y[k];
                    F[4] = F[4] + py[k]*y[k];
                    F[5] = F[5] + pz[k]*y[k];
                    F[6] = F[6] + px[k]*z[k];
                    F[7] = F[7] + py[k]*z[k];
                    F[8] = F[8] + pz[k]*z[k];
                }
                detF = F[0]*F[4]*F[8] + F[1]*F[5]*F[6] + F[2]*F[3]*F[7] 
                       - F[2]*F[4]*F[6] - F[1]*F[3]*F[8] - F[0]*F[5]*F[7];
                detF = fabs( (double)detF );
                break;

            case RATE: 
                hex_partial_deriv( px, py, pz, xx, yy, zz );
                for ( k = 0; k < 8; k++ )
                {
                    F[0] = F[0] + px[k]*xv[k];
                    F[1] = F[1] + py[k]*xv[k];
                    F[2] = F[2] + pz[k]*xv[k];
                    F[3] = F[3] + px[k]*yv[k];
                    F[4] = F[4] + py[k]*yv[k];
                    F[5] = F[5] + pz[k]*yv[k];
                    F[6] = F[6] + px[k]*zv[k];
                    F[7] = F[7] + py[k]*zv[k];
                    F[8] = F[8] + pz[k]*zv[k];
                }
                detF = F[0]*F[4]*F[8] + F[1]*F[5]*F[6] + F[2]*F[3]*F[7]
                       - F[2]*F[4]*F[6] - F[1]*F[3]*F[8] - F[0]*F[5]*F[7];
                detF = fabs( (double)detF );
                break;
        }

        extract_strain_vec( eps, F, analy->strain_variety );
        
        if ( analy->do_tensor_transform )
        {
            if ( analy->tensor_transform_matrix != NULL
                 && analy->result_id >= VAL_SHARE_EPSX 
                 && analy->result_id <= VAL_SHARE_EPSZX )
                transform_tensors( 1, eps, analy->tensor_transform_matrix );
        }
            
        switch( analy->result_id )
        {
            case VAL_SHARE_EPSX:
                resultElem[i] = eps[0];        
                break;
            case VAL_SHARE_EPSY:
                resultElem[i] = eps[1];        
                break;
            case VAL_SHARE_EPSZ:
                resultElem[i] = eps[2];        
                break;
            case VAL_SHARE_EPSXY:
                resultElem[i] = eps[3];        
                break;
            case VAL_SHARE_EPSYZ:
                resultElem[i] = eps[4];        
                break;
            case VAL_SHARE_EPSZX:
                resultElem[i] = eps[5];        
                break;
            case VAL_HEX_EPS_PD1:
                hex_principal_strain( eps, &meanStrain );
                resultElem[i] = eps[0];
                break;
            case VAL_HEX_EPS_PD2:
                hex_principal_strain( eps, &meanStrain );
                resultElem[i] = eps[1];
                break;
            case VAL_HEX_EPS_PD3:
                hex_principal_strain( eps, &meanStrain );
                resultElem[i] = eps[2];
                break;
            case VAL_HEX_EPS_MAX_SHEAR:
                hex_principal_strain( eps, &meanStrain );
                resultElem[i] = eps[0] - eps[2];
                break;
            case VAL_HEX_EPS_P1:
                hex_principal_strain( eps, &meanStrain );
                resultElem[i] = eps[0] - meanStrain;
                break;
            case VAL_HEX_EPS_P2:
                hex_principal_strain( eps, &meanStrain );
                resultElem[i] = eps[1] - meanStrain;
                break;
            case VAL_HEX_EPS_P3:
                hex_principal_strain( eps, &meanStrain );
                resultElem[i] = eps[2] - meanStrain;
                break;
            default:
                popup_dialog( WARNING_POPUP, 
		              "Unknown strain result type requested!" );
        }
    }

    hex_to_nodal( resultElem, resultArr, analy );
}


/************************************************************
 * TAG( hex_partial_deriv )
 *
 * Computes the the partial derivative of the 8 brick shape
 * functions with respect to each coordinate axis.  The
 * coordinates of the brick are passed in coorX,Y,Z, and
 * the partial derivatives are returned in dNx,y,z.
 */
static void
hex_partial_deriv( dNx, dNy, dNz, coorX, coorY, coorZ )
float dNx[8];
float dNy[8];
float dNz[8];
float coorX[8];
float coorY[8];
float coorZ[8];
{
    /* Local shape function derivatives evaluated at node points. */
    static float dN1[8] = { -.125, -.125, .125, .125,
                            -.125, -.125, .125, .125 };
    static float dN2[8] = { -.125, -.125, -.125, -.125,
                            .125, .125, .125, .125 };
    static float dN3[8] = { -.125, .125, .125, -.125,
                            -.125, .125, .125, -.125};
    float jacob[9], invJacob[9], detJacob;
    int k;

    memset( jacob, 0, sizeof(float)*9 );
    for ( k = 0; k < 8; k++ )
    {
        jacob[0] += dN1[k]*coorX[k];
        jacob[1] += dN1[k]*coorY[k];
        jacob[2] += dN1[k]*coorZ[k];
        jacob[3] += dN2[k]*coorX[k];
        jacob[4] += dN2[k]*coorY[k];
        jacob[5] += dN2[k]*coorZ[k];
        jacob[6] += dN3[k]*coorX[k];
        jacob[7] += dN3[k]*coorY[k];
        jacob[8] += dN3[k]*coorZ[k];
    }  
    detJacob =   jacob[0]*jacob[4]*jacob[8] + jacob[1]*jacob[5]*jacob[6] 
               + jacob[2]*jacob[3]*jacob[7] - jacob[2]*jacob[4]*jacob[6] 
               - jacob[1]*jacob[3]*jacob[8] - jacob[0]*jacob[5]*jacob[7];

    if ( fabs( (double)detJacob ) < 1.0e-20 )
    {
        popup_dialog( WARNING_POPUP, 
	              "Element is degenerate! Result is invalid!" );
        detJacob = 1.0;
    }

    /* Develop inverse of mapping. */ 
    detJacob = 1.0 / detJacob;

    /* Cofactors of the jacobian matrix. */
    invJacob[0] = detJacob * (  jacob[4]*jacob[8] - jacob[5]*jacob[7] );
    invJacob[1] = detJacob * ( -jacob[1]*jacob[8] + jacob[2]*jacob[7] );
    invJacob[2] = detJacob * (  jacob[1]*jacob[5] - jacob[2]*jacob[4] );
    invJacob[3] = detJacob * ( -jacob[3]*jacob[8] + jacob[5]*jacob[6] );
    invJacob[4] = detJacob * (  jacob[0]*jacob[8] - jacob[2]*jacob[6] );
    invJacob[5] = detJacob * ( -jacob[0]*jacob[5] + jacob[2]*jacob[3] );
    invJacob[6] = detJacob * (  jacob[3]*jacob[7] - jacob[4]*jacob[6] );
    invJacob[7] = detJacob * ( -jacob[0]*jacob[7] + jacob[1]*jacob[6] );
    invJacob[8] = detJacob * (  jacob[0]*jacob[4] - jacob[1]*jacob[3] );

    /* Partials dN(k)/dx, dN(k)/dy, dN(k)/dz. */
    for ( k = 0; k < 8; k++ )
    {
        dNx[k] = invJacob[0]*dN1[k] + invJacob[1]*dN2[k] + invJacob[2]*dN3[k];
        dNy[k] = invJacob[3]*dN1[k] + invJacob[4]*dN2[k] + invJacob[5]*dN3[k];
        dNz[k] = invJacob[6]*dN1[k] + invJacob[7]*dN2[k] + invJacob[8]*dN3[k];
    }
}


/************************************************************
 * TAG( extract_strain_vec ) 
 *
 * Calculates the strain components.
 */
static void
extract_strain_vec( strain, F, s_type )
float *strain;
float F[9];
Strain_type s_type;
{
    switch ( s_type ) 
    {
        case INFINITESIMAL:
        case RATE:
            strain[0] = F[0] - 1.0;
            strain[1] = F[4] - 1.0; 
            strain[2] = F[8] - 1.0; 
            strain[3] = 0.5 * ( F[1] + F[3] );
            strain[4] = 0.5 * ( F[5] + F[7] );
            strain[5] = 0.5 * ( F[2] + F[6] );
            break;
        case GREEN_LAGRANGE:
            strain[0] = 0.5*(F[0]*F[0] + F[3]*F[3] + F[6]*F[6] - 1.0);
            strain[1] = 0.5*(F[1]*F[1] + F[4]*F[4] + F[7]*F[7] - 1.0);
            strain[2] = 0.5*(F[2]*F[2] + F[5]*F[5] + F[8]*F[8] - 1.0);
            strain[3] = 0.5*(F[0]*F[1] + F[3]*F[4] + F[6]*F[7] );
            strain[4] = 0.5*(F[1]*F[2] + F[4]*F[5] + F[7]*F[8] );
            strain[5] = 0.5*(F[0]*F[2] + F[3]*F[5] + F[6]*F[8] );
            break;
        case ALMANSI:
            strain[0] = -0.5*(F[0]*F[0] + F[3]*F[3] + F[6]*F[6] - 1.0);
            strain[1] = -0.5*(F[1]*F[1] + F[4]*F[4] + F[7]*F[7] - 1.0);
            strain[2] = -0.5*(F[2]*F[2] + F[5]*F[5] + F[8]*F[8] - 1.0);
            strain[3] = -0.5*(F[0]*F[1] + F[3]*F[4] + F[6]*F[7] );
            strain[4] = -0.5*(F[1]*F[2] + F[4]*F[5] + F[7]*F[8] );
            strain[5] = -0.5*(F[0]*F[2] + F[3]*F[5] + F[6]*F[8] );
            break;
    }
}

 
/************************************************************
 * TAG( compute_hex_eff_strain )
 *
 * 
 */
static void 
compute_hex_eff_strain( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    int cur_st, cnt;
    int i;
    float *e0, *e1, *e2;
    float dt1_inv, dt2_inv;
    
    cur_st = analy->cur_state;
    cnt = analy->geom_p->bricks->cnt;
    
    if ( analy->strain_variety != RATE )
        get_result( VAL_HEX_EPS_EFF, cur_st, analy->hex_result );
    else
    {
	e0 = analy->hex_result;
	e1 = analy->tmp_result[0];
	e2 = analy->tmp_result[1];
	
	if ( cur_st == 0 )
	    /* Rate is zero at first state. */
	    memset( analy->hex_result, 0, cnt * sizeof( float ) );
	else
	{
	    /* Calculate rate from backward difference. */
	    
            get_result( VAL_HEX_EPS_EFF, cur_st - 1, e0 );
            get_result( VAL_HEX_EPS_EFF, cur_st, e1 );
	    
	    dt1_inv = 1.0 / (analy->state_times[cur_st]
	                     - analy->state_times[cur_st - 1]);
	    
	    for ( i = 0; i < cnt; i++ )
		e0[i] = (e1[i] - e0[i]) * dt1_inv;

	    if ( cur_st != analy->num_states - 1 )
	    {
	        /*
		 * Not at last state, so calculate rate from forward
		 * difference and average to get final value. 
		 * (Time steps are not constant so this is not the
		 * simpler typical central difference expression.)
		 */
                get_result( VAL_HEX_EPS_EFF, cur_st + 1, e2 );
		
	        dt2_inv = 1.0 / (analy->state_times[cur_st + 1]
	                         - analy->state_times[cur_st]);
		
		for ( i = 0; i < cnt; i++ )
		{
		    e0[i] += (e2[i] - e1[i]) * dt2_inv;
		    e0[i] *= 0.5;
		}
	    }
	}
	
    }
    
    hex_to_nodal( analy->hex_result, resultArr, analy );
}

 
/************************************************************
 * TAG( compute_hex_relative_volume )
 *
 * Computes the strain at nodes given the current geometry
 * and initial configuration.
 */
void 
compute_hex_relative_volume( analy, resultArr )
Analysis *analy;
float *resultArr;
{

    Nodal *initGeom, *currentGeom;
    Hex_geom *bricks;
    int i,j;
    float x[8], y[8], z[8];         /* Initial element geom. */
    float xx[8], yy[8], zz[8];      /* Current element geom. */
    float dNx[8], dNy[8], dNz[8];   /* Global derivates dN/dx, dN/dy, dN/dz. */
    float F[9];                     /* Element deformation gradient. */
    float detF;                     /* Determinant of element 
                                       deformation gradient. */
    float *resultElem;              /* Array for the element data. */
    register Bool_type vol_strain;

    bricks = analy->geom_p->bricks;
    initGeom    = analy->geom_p->nodes;
    currentGeom = analy->state_p->nodes;
    vol_strain = ( analy->result_id == VAL_HEX_VOL_STRAIN ) ? TRUE : FALSE;

    resultElem = analy->hex_result;

    for ( i = 0; i < bricks->cnt; i++ )
    {
        /* Get the initial and current geometries. */
        for ( j = 0; j < 8; j++ )
        {
            x[j] = initGeom->xyz[0][ bricks->nodes[j][i] ];
            y[j] = initGeom->xyz[1][ bricks->nodes[j][i] ];
            z[j] = initGeom->xyz[2][ bricks->nodes[j][i] ];
            xx[j] = currentGeom->xyz[0][ bricks->nodes[j][i] ];
            yy[j] = currentGeom->xyz[1][ bricks->nodes[j][i] ];
            zz[j] = currentGeom->xyz[2][ bricks->nodes[j][i] ];
        }

        memset(F, 0, sizeof(float)*9);

        hex_partial_deriv( dNx, dNy, dNz, x, y, z );
        for ( j = 0; j < 8; j++ )
        {
            F[0] = F[0] + dNx[j]*xx[j];
            F[1] = F[1] + dNy[j]*xx[j];
            F[2] = F[2] + dNz[j]*xx[j];
            F[3] = F[3] + dNx[j]*yy[j];
            F[4] = F[4] + dNy[j]*yy[j];
            F[5] = F[5] + dNz[j]*yy[j];
            F[6] = F[6] + dNx[j]*zz[j];
            F[7] = F[7] + dNy[j]*zz[j];
            F[8] = F[8] + dNz[j]*zz[j];
        }
        detF =  F[0]*F[4]*F[8] + F[1]*F[5]*F[6] + F[2]*F[3]*F[7] 
                -F[2]*F[4]*F[6] - F[1]*F[3]*F[8] - F[0]*F[5]*F[7];
        
	/*
	 * Foolishly assuming detF will never be negative, we won't
	 * test to ensure it's positive as required by log().
	 */
        resultElem[i] = vol_strain ? log( detF ) : detF;
    } 

    hex_to_nodal( resultElem, resultArr, analy );
}


/************************************************************
 * TAG( hex_principal_strain )
 *
 * Computes the principal strain for an element given
 * the strain tensor computed in one of the selected
 * reference configurations.
 */
static void
hex_principal_strain( strainVec, avgStrain )
float strainVec[6];
float *avgStrain;
{
    double devStrain[3];
    double Invariant[3];
    double alpha, angle, value;

    *avgStrain = - ( strainVec[0] + strainVec[1] + strainVec[2] ) / 3.0;

    devStrain[0] = strainVec[0] + *avgStrain;
    devStrain[1] = strainVec[1] + *avgStrain;
    devStrain[2] = strainVec[2] + *avgStrain;

    /* Calculate invariants of deviatoric tensor. */
    /* Invariant[0] = 0.0 */
    Invariant[0] = devStrain[0] + devStrain[1] + devStrain[2];
    Invariant[1] = -( devStrain[0]*devStrain[1] +
                      devStrain[1]*devStrain[2] +
                      devStrain[0]*devStrain[2] ) +
                      SQR(strainVec[3]) + SQR(strainVec[4]) +
                      SQR(strainVec[5]);
    Invariant[2] = -devStrain[0]*devStrain[1]*devStrain[2] -
                    2.0*strainVec[3]*strainVec[4]*strainVec[5] +
                    devStrain[0]*SQR(strainVec[4]) +
                    devStrain[1]*SQR(strainVec[5]) +
                    devStrain[2]*SQR(strainVec[3]) ;

    /* Check to see if we can have non-zero divisor, if not
     * set principal stress to 0.
     */
    if ( Invariant[1] >= 1e-12 )
    {
        alpha = -0.5*sqrt((double)27.0/Invariant[1])*
                          Invariant[2]/Invariant[1];
        alpha = BOUND( -1.0, alpha, 1.0 );
        angle = acos((double)alpha) / 3.0;
        value = 2.0 * sqrt((double)Invariant[1]/3.0);
        strainVec[0] = value*cos((double)angle);
        angle = angle - 2.0*M_PI/3.0;
        strainVec[1] = value*cos((double)angle);
        angle = angle + 4.0*M_PI/3.0;
        strainVec[2] = value*cos((double)angle);
    }
    else
    {
        strainVec[0] = 0.0;
        strainVec[1] = 0.0;
        strainVec[2] = 0.0;
    }
}


/************************************************************
 * TAG( compute_shell_strain )
 *
 * Computes the shell strain given the INNER/MIDDLE/OUTER flag
 * and the GLOBAL/LOCAL flag.  Defaults: MIDDLE, GLOBAL.
 */
static void
compute_shell_strain( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    Ref_surf_type ref_surf;
    Ref_frame_type ref_frame;
    float *resultElem, *res1, *res2;
    float eps[6];
    float localMat[3][3];
    int idx, cstate, shell_cnt, in, out, i, j;

    /*
     * The result_id is EPSX, EPSY, EPSZ, EPSXY, EPSYZ, or EPSZX.
     * These can be loaded directly from the database.
     */

    ref_surf = analy->ref_surf;
    ref_frame = analy->ref_frame;
    shell_cnt = analy->geom_p->shells->cnt;
    idx = analy->result_id - VAL_SHARE_EPSX;
    cstate = analy->cur_state;
    in = VAL_SHELL_EPSX_IN;
    out = VAL_SHELL_EPSX_OUT;

    resultElem = analy->shell_result;

    if ( ref_frame == GLOBAL )
    {
        if ( analy->do_tensor_transform )
        {
            switch ( ref_surf )
            {
                case MIDDLE:
                    /* Get transformed inner surface strain. */
                    transform_stress_strain( in, FALSE, analy );
                    
                    /* Save copy of requested inner surface strain. */
                    res1 = NEW_N( float, shell_cnt, "Temp shell stress" );
                    res2 = analy->tmp_result[idx];
                    for ( i = 0; i < shell_cnt; i++ )
                        res1[i] = res2[i];
                    
                    /* Get transformed outer surface strain. */
                    transform_stress_strain( out, analy );
                    /* "res2" now references outer surface strain. */

                    for ( i = 0; i < shell_cnt; i++ )
                        resultElem[i] = 0.5*(res1[i] + res2[i]);
                    
                    free( res1 );

                    break;
                case INNER:
                    transform_stress_strain( in, TRUE, analy );
                    break;
                case OUTER:
                    transform_stress_strain( out, TRUE, analy );
                    break;
            }
        }
        else
        {
            switch ( ref_surf )
            {
                case MIDDLE:
                    res1 = analy->tmp_result[1];
                    res2 = analy->tmp_result[2];
                    get_result( in + idx, cstate, res1 );
                    get_result( out + idx, cstate, res2 );
                    for ( i = 0; i < shell_cnt; i++ )
                        resultElem[i] = 0.5*(res1[i] + res2[i]);
                    break;
                case INNER:
                    get_result( in + idx, cstate, resultElem );
                    break;
                case OUTER:
                    get_result( out + idx, cstate, resultElem );
                    break;
            }
        }
    }
    else if ( ref_frame == LOCAL )
    {
        /* Need to transform global quantities to local quantities.
         * The full tensor must be transformed.
         */
/**/
/* 
 * These might benefit from being updated with calls to
 * get_tensor_result() then transpose_tensors().
 */

        switch ( ref_surf )
        {
            case MIDDLE:
                res2 = NEW_N( float, shell_cnt, "Tmp shell stress" );
                for ( i = 0; i < 6; i++ )
                {
                    get_result( in + i, cstate, analy->tmp_result[i] );
                    res1 = analy->tmp_result[i];
                    get_result( out + i, cstate, res2 );
                    for ( j = 0; j < shell_cnt; j++ )
                        res1[j] = 0.5*(res1[j] + res2[j]);
                }
                free( res2 );
                break;
            case INNER:
                for ( i = 0; i < 6; i++ )
                    get_result( in + i, cstate, analy->tmp_result[i] );
                break;
            case OUTER:
                for ( i = 0; i < 6; i++ )
                    get_result( out + i, cstate, analy->tmp_result[i] );
                break;
        }

        for ( i = 0; i < shell_cnt; i++ )
        {
            for ( j = 0; j < 6; j++ )
                eps[j] = analy->tmp_result[j][i];
            global_to_local_mtx( analy, i, localMat );
            transform_tensors( 1, eps, localMat );

            resultElem[i] = eps[idx];
        }
    }

    shell_to_nodal( resultElem, resultArr, analy, TRUE );
}


/************************************************************
 * TAG( compute_shell_eff_strain )
 *
 * Computes the shell effective plastic strain given the
 * INNER/MIDDLE/OUTER flag.  Default: MIDDLE.
 */
static void
compute_shell_eff_strain( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    Ref_surf_type ref_surf;
    float *resultElem;
    int cur_st, cnt;
    int i;
    float *e1, *e2;
    float dt1_inv, dt2_inv;
    int result_id;

    ref_surf = analy->ref_surf;
    resultElem = analy->shell_result;
    
    cur_st = analy->cur_state;
    cnt = analy->geom_p->shells->cnt;

    switch ( ref_surf )
    {
        case MIDDLE:
            result_id = VAL_SHELL_EPS_EFF_MID;
            break;
        case INNER:
            result_id = VAL_SHELL_EPS_EFF_IN;
            break;
        case OUTER:
            result_id = VAL_SHELL_EPS_EFF_OUT;
            break;
    }
    
    if ( analy->strain_variety != RATE )
        get_result( result_id, cur_st, resultElem );
    else
    {
	e1 = analy->tmp_result[0];
	e2 = analy->tmp_result[1];
	
	if ( cur_st == 0 )
	    /* Rate is zero at first state. */
	    memset( resultElem, 0, cnt * sizeof( float ) );
	else
	{
	    /* Calculate rate from backward difference. */
	    
            get_result( result_id, cur_st - 1, resultElem );
            get_result( result_id, cur_st, e1 );
	    
	    dt1_inv = 1.0 / (analy->state_times[cur_st]
	                     - analy->state_times[cur_st - 1]);
	    
	    for ( i = 0; i < cnt; i++ )
		resultElem[i] = (e1[i] - resultElem[i]) * dt1_inv;

	    if ( cur_st != analy->num_states - 1 )
	    {
	        /*
		 * Not at last state, so calculate rate from forward
		 * difference and average to get final value. 
		 * (Time steps are not constant so this is not the
		 * simpler typical central difference expression.)
		 */
                get_result( result_id, cur_st + 1, e2 );
		
	        dt2_inv = 1.0 / (analy->state_times[cur_st + 1]
	                         - analy->state_times[cur_st]);
		
		for ( i = 0; i < cnt; i++ )
		{
		    resultElem[i] += (e2[i] - e1[i]) * dt2_inv;
		    resultElem[i] *= 0.5;
		}
	    }
	}
	
    }

    shell_to_nodal( resultElem, resultArr, analy, TRUE );
}


/************************************************************
 * TAG( compute_beam_axial_strain )
 *
 * Computes [S|T] [+|-] axial beam strains given the diameter and
 * youngs_modulus values.
 */

static float
    diameter = 1.0,
    youngs_modulus = 1.0;

void
compute_beam_axial_strain( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    float *resultElem;
    static char d_str[64] = "Ax beam strain, dia =             ",
	 *ym_str = ", Young's Mod =              ";
    int r_idx, i;
    float coeff, axf_coeff, mom_coeff;
    float *force, *moment;
    Result_type r_type;

    resultElem = analy->beam_result;

    sprintf( strchr( d_str, '=') + 2, "%.1e\0", diameter);
    sprintf( strchr( ym_str, '=') + 2, "%.1e.\n\0", youngs_modulus);
    strcat( d_str, ym_str);
    wrt_text( d_str );

    r_idx = resultid_to_index[analy->result_id];
    r_type = (Result_type) trans_result[r_idx][0];

    force = analy->tmp_result[0];
    get_result( VAL_BEAM_AX_FORCE, analy->cur_state, force );

    moment = analy->tmp_result[1];
    switch( r_type )
    {
        case VAL_BEAM_S_AX_STRN_P:
	case VAL_BEAM_S_AX_STRN_M:
	    get_result( VAL_BEAM_S_MOMENT, analy->cur_state, moment );
	    break;
        
	case VAL_BEAM_T_AX_STRN_P:
	case VAL_BEAM_T_AX_STRN_M:
	    get_result( VAL_BEAM_T_MOMENT, analy->cur_state, moment );
	    break;
    }

    coeff = 4.0 / ( PI * youngs_modulus );
    axf_coeff = 1.0 / ( diameter * diameter );
    mom_coeff = 8.0 / ( diameter * diameter * diameter );

    switch( r_type )
    {
        case VAL_BEAM_S_AX_STRN_P:
	case VAL_BEAM_T_AX_STRN_P:
	    for ( i = 0; i < analy->geom_p->beams->cnt; i++ )
		resultElem[i] = coeff * (axf_coeff * force[i]
					 + mom_coeff * moment[i] );
	    break;
        
	case VAL_BEAM_S_AX_STRN_M:
	case VAL_BEAM_T_AX_STRN_M:
	    for ( i = 0; i < analy->geom_p->beams->cnt; i++ )
		resultElem[i] = coeff * (axf_coeff * force[i]
					 - mom_coeff * moment[i] );
	    break;
    }

    beam_to_nodal( resultElem, resultArr, analy );
 }


/************************************************************
 * TAG( set_diameter )
 *
 * Set static global variable diameter for use in beam strain
 * calculation.
 */
void
set_diameter( diam )
float diam;
{
    diameter = diam;
}


/************************************************************
 * TAG( set_youngs_mod )
 *
 * Set static global variable youngs_modulus for use in beam strain
 * calculation.
 */
void
set_youngs_mod( youngs_mod )
float youngs_mod;
{
    youngs_modulus = youngs_mod;
}


