/* $Id$ */
/*
 * stress.c - Routines for computing derived stress quantities.
 *
 *      Thomas Spelce
 *      Lawrence Livermore National Laboratory
 *      Jun 29 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */

#include "viewer.h"


static void compute_hex_effstress();
static void compute_hex_principal_stress();
static void compute_shell_effstress();
static void compute_hex_press();
static void compute_shell_press();
static void compute_shell_stress();
static void compute_shell_principal_stress();


/************************************************************
 * TAG( compute_share_press )
 *
 * Computes the pressure at nodes for hex and shell elements.
 */
void 
compute_share_press( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    if ( analy->geom_p->bricks != NULL )
        compute_hex_press( analy, resultArr );
    else
        memset( resultArr, 0, analy->geom_p->nodes->cnt * sizeof(float) );

    if ( analy->geom_p->shells != NULL )
        compute_shell_press( analy, resultArr );
}


/************************************************************
 * TAG( compute_share_stress )
 *
 * Computes the stress at nodes for hex and shell elements.
 */
void
compute_share_stress( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    if ( analy->geom_p->bricks != NULL )
    {
        get_result( VAL_HEX_SIGX + (analy->result_id - VAL_SHARE_SIGX),
                    analy->cur_state, analy->hex_result );
        hex_to_nodal( analy->hex_result, resultArr, analy );
    }
    else
        memset( resultArr, 0, analy->geom_p->nodes->cnt * sizeof(float) );

    if ( analy->geom_p->shells != NULL )
        compute_shell_stress( analy, resultArr );
}


/************************************************************
 * TAG( compute_share_effstress )
 *
 * Computes the stress at nodes for hex and shell elements.
 */
void
compute_share_effstress( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    if ( analy->geom_p->bricks != NULL )
    {
        compute_hex_effstress( analy, resultArr );
    }
    else
        memset( resultArr, 0, analy->geom_p->nodes->cnt * sizeof(float) );

    if ( analy->geom_p->shells != NULL )
        compute_shell_effstress( analy, resultArr );
}


/************************************************************
 * TAG( compute_share_prin_stress )
 *
 * Computes the stress at nodes for hex and shell elements.
 */
void
compute_share_prin_stress( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    if ( analy->geom_p->bricks != NULL )
    {
        compute_hex_principal_stress( analy, resultArr );
    }
    else
        memset( resultArr, 0, analy->geom_p->nodes->cnt * sizeof(float) );

    if ( analy->geom_p->shells != NULL )
        compute_shell_principal_stress( analy, resultArr );
}


/************************************************************
 * TAG( compute_hex_press )
 *
 * Computes the pressure at nodes given the current stress
 * tensor for an element.
 */
static void 
compute_hex_press( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    Hex_geom *bricks;
    float *resultElem;
    float *hexPressure[3];
    int i;

    bricks = analy->geom_p->bricks;
    resultElem = analy->hex_result;

    for ( i = 0; i < 3; i++ )
    {
        get_result( VAL_HEX_SIGX+i, analy->cur_state, analy->tmp_result[i] ); 
        hexPressure[i] = analy->tmp_result[i];
    }

    for ( i = 0; i < bricks->cnt; i++ )
    {
        resultElem[i] = -( hexPressure[0][i] +
                           hexPressure[1][i] +
                           hexPressure[2][i] ) / 3.0 ;
    }

    hex_to_nodal( resultElem, resultArr, analy );
}


/************************************************************
 * TAG( compute_hex_effstress )
 *
 * Computes the effective stress at nodes.
 */
static void 
compute_hex_effstress( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    float *resultElem;
    float *hexStress[6];
    float devStress[3];
    float pressure;
    int brick_cnt;
    int i,j;

    brick_cnt = analy->geom_p->bricks->cnt;

    for ( i = 0; i < 6; i++ )
    {
        get_result( VAL_HEX_SIGX+i, analy->cur_state, analy->tmp_result[i] ); 
        hexStress[i] = analy->tmp_result[i];
    }

    resultElem = analy->hex_result;

    for ( i = 0; i < brick_cnt; i++ )
    {
        pressure = -( hexStress[0][i] + 
                      hexStress[1][i] +  
                      hexStress[2][i] ) / 3.0;

        for ( j = 0; j < 3; j++ )
            devStress[j] = hexStress[j][i] + pressure;

        /* 
	 * Calculate effective stress from deviatoric components.
	 * Updated derivation avoids negative square root operand
	 * (UNICOS, of course).
	 */
        resultElem[i] = 0.5 * ( devStress[0]*devStress[0] +
                                devStress[1]*devStress[1] +
                                devStress[2]*devStress[2] ) +
                        hexStress[3][i]*hexStress[3][i] +
                        hexStress[4][i]*hexStress[4][i] +
                        hexStress[5][i]*hexStress[5][i] ;

       resultElem[i] = sqrt( (double)(3.0*resultElem[i]) );
    }

    hex_to_nodal( resultElem, resultArr, analy );
}


/************************************************************
 * TAG( compute_hex_principal_stress )
 *
 * Computes the principal deviatoric stresses and principal
 * stresses.
 */
static void
compute_hex_principal_stress( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    Hex_geom *bricks;
    float *resultElem;               /* Element results vector. */
    float *hexStress[6];             /* Ptr to element stresses. */
    float devStress[3];              /* Deviatoric stresses,
                                        only need diagonal terms. */
    float Invariant[3];              /* Invariants of tensor. */
    float princStress[3];            /* Principal values. */
    float pressure;
    float alpha, angle, value;
    int i, j;

    bricks = analy->geom_p->bricks;
    for ( i = 0; i < 6; i++ )
    {
        get_result( VAL_HEX_SIGX+i, analy->cur_state, analy->tmp_result[i] ); 
        hexStress[i] = analy->tmp_result[i];
    }
    resultElem = analy->hex_result;

    /* Calculate deviatoric stresses. */
    for ( i = 0; i < bricks->cnt; i++ )
    {
        /* Calculate pressure. */
        pressure =  - ( hexStress[0][i] +
                        hexStress[1][i] + 
                        hexStress[2][i] ) / 3.0;

        devStress[0] = hexStress[0][i] + pressure;
        devStress[1] = hexStress[1][i] + pressure;
        devStress[2] = hexStress[2][i] + pressure;

        /* Calculate invariants of deviatoric tensor. */
        /* Invariant[0] = 0.0 */
        Invariant[0] = devStress[0] + devStress[1] + devStress[2];
        Invariant[1] = 0.5 * ( devStress[0]*devStress[0] 
                               + devStress[1]*devStress[1] 
                               + devStress[2]*devStress[2] ) 
                       + hexStress[3][i]*hexStress[3][i] 
                       + hexStress[4][i]*hexStress[4][i] 
                       + hexStress[5][i]*hexStress[5][i];
        Invariant[2] = -devStress[0]*devStress[1]*devStress[2] -
                       2.0*hexStress[3][i]*hexStress[4][i]*hexStress[5][i] +
                       devStress[0]*hexStress[4][i]*hexStress[4][i] +
                       devStress[1]*hexStress[5][i]*hexStress[5][i] +
                       devStress[2]*hexStress[3][i]*hexStress[3][i];

        /* Check to see if we can have non-zero divisor, if not 
         * set principal stress to 0.
         */
        if ( Invariant[1] >= 1e-7 )
        {
            alpha = -0.5*sqrt( (double)27.0/Invariant[1])*
                              Invariant[2]/Invariant[1];
            if ( alpha < 0 ) 
                alpha = MAX( alpha, -1.0 );
            else if ( alpha > 0 )
                alpha = MIN( alpha, 1.0 );
            angle = acos((double)alpha) / 3.0;
            value = 2.0 * sqrt( (double)Invariant[1]/3.0);
            princStress[0] = value*cos((double)angle);
            angle = angle - 2.0*M_PI/3.0;
            princStress[1] = value*cos((double)angle);
            angle = angle + 4.0*M_PI/3.0;
            princStress[2] = value*cos((double)angle);
        }
        else
        {
            princStress[0] = 0.0;
            princStress[1] = 0.0;
            princStress[2] = 0.0;
        }

        switch ( analy->result_id )
        {
            case VAL_SHARE_SIG_PD1 :
                resultElem[i] = princStress[0];
                break;
            case VAL_SHARE_SIG_PD2 :
                resultElem[i] = princStress[1];
                break;
            case VAL_SHARE_SIG_PD3 :
                resultElem[i] = princStress[2];
                break;
            case VAL_SHARE_SIG_MAX_SHEAR :
                resultElem[i] = (princStress[0] - princStress[2]) / 2.0;
                break;
            case VAL_SHARE_SIG_P1 :
                resultElem[i] = princStress[0] - pressure;
                break;
            case VAL_SHARE_SIG_P2 :
                resultElem[i] = princStress[1] - pressure;
                break;
            case VAL_SHARE_SIG_P3 :
                resultElem[i] = princStress[2] - pressure;
                break;
        }
    }

    hex_to_nodal( resultElem, resultArr, analy );
}


/************************************************************
 * TAG( compute_shell_stress )
 *
 * Computes the shell stress given the INNER/MIDDLE/OUTER flag
 * and the GLOBAL/LOCAL flag.  Defaults: MIDDLE, GLOBAL.
 */
static void
compute_shell_stress( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    Result_type result_id;
    Ref_surf_type ref_surf;
    Ref_frame_type ref_frame;
    float *resultElem;
    float localMat[3][3];
    float sigma[6];
    int base, idx, shell_cnt, i, j;

    /* 
     * The result_id is SIGX, SIGY, SIGZ, SIGXY, SIGYZ, or SIGZX.
     * These can be loaded directly from the database.
     */

    ref_surf = analy->ref_surf;
    ref_frame = analy->ref_frame;
    idx = analy->result_id - VAL_SHARE_SIGX;

    switch ( ref_surf )
    {
        case MIDDLE:
            base = VAL_SHELL_SIGX_MID;
            break;
        case INNER:
            base = VAL_SHELL_SIGX_IN;
            break;
        case OUTER:
            base = VAL_SHELL_SIGX_OUT;
            break;
    }

    resultElem = analy->shell_result;

    if ( ref_frame == GLOBAL )
    {
        get_result( base + idx, analy->cur_state, resultElem );
    }
    else if ( ref_frame == LOCAL )
    {
        /* Need to transform global quantities to local quantities.
         * The full tensor must be transformed.
         */

        for ( i = 0; i < 6; i++ )
            get_result( base + i, analy->cur_state, analy->tmp_result[i] );

        shell_cnt = analy->geom_p->shells->cnt;

        for ( i = 0; i < shell_cnt; i++ )
        {
            for ( j = 0; j < 6; j++ )
               sigma[j] = analy->tmp_result[j][i];
            global_to_local_mtx( analy, i, localMat );
            transform_tensor( sigma, localMat );

            resultElem[i] = sigma[idx];
        }
    }

    shell_to_nodal( resultElem, resultArr, analy, TRUE );
}


/************************************************************
 * TAG( compute_shell_press )
 *
 * Computes the pressure at nodes.
 */
static void
compute_shell_press( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    Ref_surf_type ref_surf;
    float *resultElem;
    float *shellPressure[3];
    int shell_cnt, i;

    shell_cnt = analy->geom_p->shells->cnt;
    ref_surf = analy->ref_surf;

    /* Load pointers to the original component arrays. */
    for ( i = 0; i < 3; i++ )
    {
        switch ( ref_surf )
        {
            case MIDDLE:
                get_result( VAL_SHELL_SIGX_MID + i,
                            analy->cur_state, analy->tmp_result[i] );
                break;
            case INNER:
                get_result( VAL_SHELL_SIGX_IN + i,
                            analy->cur_state, analy->tmp_result[i] );
                break;
            case OUTER:
                get_result( VAL_SHELL_SIGX_OUT + i,
                            analy->cur_state, analy->tmp_result[i] );
                break;
        }
        shellPressure[i] = analy->tmp_result[i];
    }

    resultElem = analy->shell_result;

    /* Compute pressure. */
    for ( i = 0; i < shell_cnt; i++ )
    {
        resultElem[i] = -( shellPressure[0][i] +
                           shellPressure[1][i] +
                           shellPressure[2][i] ) / 3.0 ;
    }

    shell_to_nodal( resultElem, resultArr, analy, TRUE );
}


/************************************************************
 * TAG( compute_shell_effstress )
 *
 * Computes the effective stress at nodes.
 */
static void
compute_shell_effstress( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    Shell_state *shells;
    float *resultElem;
    float *shellStress[6];
    float devStress[3];
    float pressure;
    int shell_cnt, i, j;

    if ( analy->state_p->activity_present )
        shell_cnt = analy->state_p->shells->cnt;
    else
	shell_cnt = analy->geom_p->shells->cnt;

    /* Load the pointers to the component arrays. */
    for ( i = 0; i < 6; i++ )
    {
        switch ( analy->ref_surf )
        {
            case MIDDLE:
                get_result( VAL_SHELL_SIGX_MID + i,
                            analy->cur_state, analy->tmp_result[i] );
                break;
            case INNER:
                get_result( VAL_SHELL_SIGX_IN + i,
                            analy->cur_state, analy->tmp_result[i] );
                break;
            case OUTER:
                get_result( VAL_SHELL_SIGX_OUT + i,
                            analy->cur_state, analy->tmp_result[i] );
                break;
        }
        shellStress[i] = analy->tmp_result[i];
    }

    resultElem = analy->shell_result;

    /* Calculate pressure as intermediate variable needed for
     * determining deviatoric components.
     */
    for ( i = 0; i < shell_cnt; i++ )
    {
        pressure = -( shellStress[0][i] +
                      shellStress[1][i] +
                      shellStress[2][i] ) / 3.0;

        /* Calculate deviatoric components of stress tensor. */
        for ( j = 0; j < 3; j++ )
            devStress[j] = shellStress[j][i] + pressure;

        /* 
	 * Calculate effective stress from deviatoric components.
	 * Updated derivation avoids negative square root operand
	 * (UNICOS, of course).
	 */
        resultElem[i] = 0.5 * ( devStress[0]*devStress[0] +
                                devStress[1]*devStress[1] +
                                devStress[2]*devStress[2] ) +
                        shellStress[3][i]*shellStress[3][i] +
                        shellStress[4][i]*shellStress[4][i] +
                        shellStress[5][i]*shellStress[5][i] ;
        resultElem[i] = sqrt( (double)(3.0*resultElem[i]) );
    }
    
    shell_to_nodal( resultElem, resultArr, analy, TRUE );
}


/************************************************************
 * TAG( compute_shell_principal_stress )
 *
 * Computes the principal deviatoric stresses and principal
 * stresses.
 */
static void
compute_shell_principal_stress( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    Shell_geom *shells;
    float *resultElem;               /* Element results vector. */
    float *shellStress[6];           /* Ptr to element stresses. */
    float devStress[3];              /* Deviatoric stresses,
                                        only need diagonal terms. */
    float Invariant[3];              /* Invariants of tensor. */
    float princStress[3];            /* Principal values. */
    float pressure;
    float alpha, angle, value;
    int i, j;
    int shell_cnt;


    if ( analy->state_p->activity_present )
        shell_cnt = analy->state_p->shells->cnt;
    else
	shell_cnt = analy->geom_p->shells->cnt;

    /* Load the pointers to the component arrays. */
    for ( i = 0; i < 6; i++ )
    {
        switch ( analy->ref_surf )
        {
            case MIDDLE:
                get_result( VAL_SHELL_SIGX_MID + i,
                            analy->cur_state, analy->tmp_result[i] );
                break;
            case INNER:
                get_result( VAL_SHELL_SIGX_IN + i,
                            analy->cur_state, analy->tmp_result[i] );
                break;
            case OUTER:
                get_result( VAL_SHELL_SIGX_OUT + i,
                            analy->cur_state, analy->tmp_result[i] );
                break;
        }
        shellStress[i] = analy->tmp_result[i];
    }

    resultElem = analy->shell_result;

    /* Calculate deviatoric stresses. */
    for ( i = 0; i < shell_cnt; i++ )
    {
        /* Calculate pressure as intermediate variable needed for
         * determining deviatoric components.
         */
        pressure = -( shellStress[0][i] +
                      shellStress[1][i] +
                      shellStress[2][i] ) / 3.0;

        /* Calculate deviatoric components of stress tensor. */
        for ( j = 0; j < 3; j++ )
            devStress[j] = shellStress[j][i] + pressure;

        /* Calculate invariants of deviatoric tensor. */
        /* Invariant[0] = 0.0 */
        Invariant[0] = devStress[0] + devStress[1] + devStress[2];
        Invariant[1] = 0.5 * ( devStress[0] * devStress[0]
                               + devStress[1] * devStress[1]
                               + devStress[2] * devStress[2] )
                       + shellStress[3][i] * shellStress[3][i]
                       + shellStress[4][i] * shellStress[4][i]
                       + shellStress[5][i] * shellStress[5][i];
        Invariant[2] = -devStress[0] * devStress[1] * devStress[2]
                       - 2.0 * shellStress[3][i] * shellStress[4][i] * shellStress[5][i]
                       + devStress[0] * shellStress[4][i] * shellStress[4][i]
                       + devStress[1] * shellStress[5][i] * shellStress[5][i]
                       + devStress[2] * shellStress[3][i] * shellStress[3][i];

        /* Check to see if we can have non-zero divisor, if not 
         * set principal stress to 0.
         */
        if ( Invariant[1] >= 1e-7 )
        {
            alpha = -0.5*sqrt( (double)27.0/Invariant[1])*
                              Invariant[2]/Invariant[1];
            if ( alpha < 0 ) 
                alpha = MAX( alpha, -1.0 );
            else if ( alpha > 0 )
                alpha = MIN( alpha, 1.0 );
            angle = acos((double)alpha) / 3.0;
            value = 2.0 * sqrt( (double)Invariant[1]/3.0);
            princStress[0] = value*cos((double)angle);
            angle = angle - 2.0*M_PI/3.0;
            princStress[1] = value*cos((double)angle);
            angle = angle + 4.0*M_PI/3.0;
            princStress[2] = value*cos((double)angle);
        }
        else
        {
            princStress[0] = 0.0;
            princStress[1] = 0.0;
            princStress[2] = 0.0;
        }

        switch ( analy->result_id )
        {
            case VAL_SHARE_SIG_PD1 :
                resultElem[i] = princStress[0];
                break;
            case VAL_SHARE_SIG_PD2 :
                resultElem[i] = princStress[1];
                break;
            case VAL_SHARE_SIG_PD3 :
                resultElem[i] = princStress[2];
                break;
            case VAL_SHARE_SIG_MAX_SHEAR :
                resultElem[i] = (princStress[0] - princStress[2]) / 2.0;
                break;
            case VAL_SHARE_SIG_P1 :
                resultElem[i] = princStress[0] - pressure;
                break;
            case VAL_SHARE_SIG_P2 :
                resultElem[i] = princStress[1] - pressure;
                break;
            case VAL_SHARE_SIG_P3 :
                resultElem[i] = princStress[2] - pressure;
                break;
        }
    }

    shell_to_nodal( resultElem, resultArr, analy, TRUE );
}


/************************************************************
 * TAG( compute_shell_surface_stress )
 *
 * Computes shell surface stresses at nodes.
 */
void
compute_shell_surface_stress( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    float *tmp_result[6];
    float *resultElem;
    float *bending_resultant[3];      /* Three slots for eff stress calc. */
    float *normal_resultant[3];
    float *thickness;
    float one_over_t, one_over_t2;
    float sx, sy, sxy, sigef[2];      /* Vars for eff surface stress. */
    int shell_cnt, i;

    shell_cnt = analy->geom_p->shells->cnt;
    for ( i = 0; i < 6; i++ )
        tmp_result[i] = analy->tmp_result[i];

    /* Set up the pointers to the individual component arrays. */
    switch ( analy->result_id )
    {
        case VAL_SHELL_SURF1:
        case VAL_SHELL_SURF2:
            get_result( VAL_SHELL_RES6, analy->cur_state, tmp_result[0] );
            get_result( VAL_SHELL_RES1, analy->cur_state, tmp_result[1] );
            normal_resultant[0] = tmp_result[0];
            bending_resultant[0]= tmp_result[1];
            break;
        case VAL_SHELL_SURF3:
        case VAL_SHELL_SURF4:
            get_result( VAL_SHELL_RES7, analy->cur_state, tmp_result[0] );
            get_result( VAL_SHELL_RES2, analy->cur_state, tmp_result[1] );
            normal_resultant[0] = tmp_result[0];
            bending_resultant[0]= tmp_result[1];
            break;
        case VAL_SHELL_SURF5:
        case VAL_SHELL_SURF6:
            get_result( VAL_SHELL_RES8, analy->cur_state, tmp_result[0] );
            get_result( VAL_SHELL_RES3, analy->cur_state, tmp_result[1] );
            normal_resultant[0] = tmp_result[0];
            bending_resultant[0]= tmp_result[1];
            break;
        case VAL_SHELL_EFF1:
        case VAL_SHELL_EFF2:
        case VAL_SHELL_EFF3:
            get_result( VAL_SHELL_RES6, analy->cur_state, tmp_result[0] );
            get_result( VAL_SHELL_RES1, analy->cur_state, tmp_result[1] );
            get_result( VAL_SHELL_RES7, analy->cur_state, tmp_result[2] );
            get_result( VAL_SHELL_RES2, analy->cur_state, tmp_result[3] );
            get_result( VAL_SHELL_RES8, analy->cur_state, tmp_result[4] );
            get_result( VAL_SHELL_RES3, analy->cur_state, tmp_result[5] );
            normal_resultant[0] = tmp_result[0];
            bending_resultant[0]= tmp_result[1];
            normal_resultant[1] = tmp_result[2];
            bending_resultant[1]= tmp_result[3];
            normal_resultant[2] = tmp_result[4];
            bending_resultant[2]= tmp_result[5];
            break;
    }
    thickness = NEW_N( float, shell_cnt, "Tmp shell thickness" );
    get_result( VAL_SHELL_THICKNESS, analy->cur_state, thickness );

    resultElem = analy->shell_result;

    /* Calculate the value given the normal and bending components
     * and the element thickness.
     */
    for ( i = 0; i < shell_cnt; i++ )
    {
        one_over_t = 1.0 / thickness[i];
        one_over_t2 = one_over_t / thickness[i];
        switch ( analy->result_id )
        {
            case VAL_SHELL_SURF1:
            case VAL_SHELL_SURF3:
            case VAL_SHELL_SURF5:
                resultElem[i] = normal_resultant[0][i]*one_over_t +
                                6.0*bending_resultant[0][i]*one_over_t2;
                break;
            case VAL_SHELL_SURF2:
            case VAL_SHELL_SURF4:
            case VAL_SHELL_SURF6:
                resultElem[i] = normal_resultant[0][i]*one_over_t -
                                6.0*bending_resultant[0][i]*one_over_t2;
                break;
            case VAL_SHELL_EFF1:
                /* These are the upper surface values. */
                sx = normal_resultant[0][i]*one_over_t +
                     6.0*bending_resultant[0][i]*one_over_t2 ;
                sy = normal_resultant[1][i]*one_over_t +
                     6.0*bending_resultant[1][i]*one_over_t2 ;
                sxy = normal_resultant[2][i]*one_over_t +
                      6.0*bending_resultant[2][i]*one_over_t2 ;
                resultElem[i] = sqrt( (double)
                                     (sx*sx - sx*sy + sy*sy + 3.0*sxy*sxy));
                break;
            case VAL_SHELL_EFF2:
                /* These are the lower surface values. */
                sx = normal_resultant[0][i]*one_over_t -
                     6.0*bending_resultant[0][i]*one_over_t2 ;
                sy = normal_resultant[1][i]*one_over_t -
                     6.0*bending_resultant[1][i]*one_over_t2 ;
                sxy = normal_resultant[2][i]*one_over_t -
                      6.0*bending_resultant[2][i]*one_over_t2 ;
                resultElem[i] = sqrt( (double)
                                     (sx*sx - sx*sy + sy*sy + 3.0*sxy*sxy));
                break;
            case VAL_SHELL_EFF3:
                /* Calc upper surface first. */
                sx = normal_resultant[0][i]*one_over_t +
                     6.0*bending_resultant[0][i]*one_over_t2 ;
                sy = normal_resultant[1][i]*one_over_t +
                     6.0*bending_resultant[1][i]*one_over_t2 ;
                sxy = normal_resultant[2][i]*one_over_t +
                      6.0*bending_resultant[2][i]*one_over_t2 ;
                sigef[0] = sqrt( (double)
                                (sx*sx - sx*sy + sy*sy + 3.0*sxy*sxy));

                /* Now calc lower surface. */
                sx = normal_resultant[0][i]*one_over_t -
                     6.0*bending_resultant[0][i]*one_over_t2 ;
                sy = normal_resultant[1][i]*one_over_t -
                     6.0*bending_resultant[1][i]*one_over_t2 ;
                sxy = normal_resultant[2][i]*one_over_t -
                      6.0*bending_resultant[2][i]*one_over_t2 ;
                sigef[1] = sqrt( (double)
                                (sx*sx - sx*sy + sy*sy + 3.0*sxy*sxy));

                /* Return maximum value. */
                resultElem[i] = MAX( sigef[0], sigef[1] );
                break;
       }
    }

    shell_to_nodal( resultElem, resultArr, analy, FALSE );

    free( thickness );
}


