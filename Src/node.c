/* $Id$ */
/*
 * node.c - Routines for computing derived nodal quantities.
 *
 *      Thomas Spelce
 *      Lawrence Livermore National Laboratory
 *      Jun 25 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */

#include "viewer.h"

/* Reference pressure intensity for PING. */
static float ping_pr_ref = 1.0;


/************************************************************
 * TAG( compute_node_displacement )
 *
 * Computes the displacement at nodes given the current geometry
 * and initial configuration.
 */
void
compute_node_displacement( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    Nodal *initGeom, *currentGeom;
    int i;

    initGeom = analy->geom_p->nodes;
    currentGeom = analy->state_p->nodes;
    
    if ( analy->result_id != VAL_NODE_DISPX
         && analy->result_id != VAL_NODE_DISPY
         && analy->result_id != VAL_NODE_DISPZ
         && analy->result_id != VAL_NODE_DISPMAG )
    {
        popup_dialog( WARNING_POPUP, 
		      "Unknown nodal displacement type requested!" );
	return;
    }
    
    for ( i = 0; i < initGeom->cnt; i++ )
    {
        switch ( analy->result_id )
        {
            case VAL_NODE_DISPX:
                resultArr[i] = currentGeom->xyz[0][i] - initGeom->xyz[0][i];
                break;
            case VAL_NODE_DISPY:
                resultArr[i] = currentGeom->xyz[1][i] - initGeom->xyz[1][i];
                break;
            case VAL_NODE_DISPZ:
                resultArr[i] = currentGeom->xyz[2][i] - initGeom->xyz[2][i];
                break;
            case VAL_NODE_DISPMAG:
                resultArr[i] = sqrt ( (double)
                    ( (currentGeom->xyz[0][i] - initGeom->xyz[0][i]) *
                      (currentGeom->xyz[0][i] - initGeom->xyz[0][i])
                    + (currentGeom->xyz[1][i] - initGeom->xyz[1][i]) *
                      (currentGeom->xyz[1][i] - initGeom->xyz[1][i])
                    + (currentGeom->xyz[2][i] - initGeom->xyz[2][i]) *
                      (currentGeom->xyz[2][i] - initGeom->xyz[2][i]) ) );
                break;
        }
    }
}


/************************************************************
 * TAG( compute_node_velocity )
 *
 * Computes the velocity at the nodes using a backwards difference
 * (unless the velocities are contained in the data).
 */
void
compute_node_velocity( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    Nodal *initGeom, *currentGeom;
    State *state_prev;
    float delta_t;
    float tmp[3];
    int i;
    
    if ( analy->result_id != VAL_NODE_VELX
         && analy->result_id != VAL_NODE_VELY
         && analy->result_id != VAL_NODE_VELZ
         && analy->result_id != VAL_NODE_VELMAG )
    {
        popup_dialog( WARNING_POPUP, 
		      "Unknown nodal velocity type requested!" );
	return;
    }

    initGeom = analy->geom_p->nodes;
    currentGeom = analy->state_p->nodes;

    if ( is_in_database( VAL_NODE_VELX ) )
    {
        /* Velocities are stored in the data.
         */
        switch ( analy->result_id )
        {
            case VAL_NODE_VELX:
            case VAL_NODE_VELY:
            case VAL_NODE_VELZ:
                get_result( analy->result_id, analy->cur_state, resultArr );
                break;
            case VAL_NODE_VELMAG:
                get_vec_result( VAL_NODE_VELX, analy->cur_state,
                                analy->tmp_result[0], analy->tmp_result[1],
                                analy->tmp_result[2] );

                for ( i = 0; i < initGeom->cnt; i++ )
                    resultArr[i] = sqrt((double)
                       ( (analy->tmp_result[0][i]*analy->tmp_result[0][i])
                       + (analy->tmp_result[1][i]*analy->tmp_result[1][i])
                       + (analy->tmp_result[2][i]*analy->tmp_result[2][i]) ));
        }
    }
    else
    {
        /* Read the previous coords in and calculate the velocities
         * using simple differences.
         */
        if ( analy->cur_state == 0 )
        {
            wrt_text( "Can't calculate velocity for time 0.0\n" );
            return;
        }

        state_prev = get_state( analy->cur_state-1, NULL );
        delta_t = analy->state_p->time - state_prev->time;

        for ( i = 0; i < initGeom->cnt; i++ )
        {
            switch ( analy->result_id )
            {
                case VAL_NODE_VELX:
                    resultArr[i] = (currentGeom->xyz[0][i] -
                                    state_prev->nodes->xyz[0][i]) / delta_t;
                    break;
                case VAL_NODE_VELY:
                    resultArr[i] = (currentGeom->xyz[1][i] -
                                    state_prev->nodes->xyz[1][i]) / delta_t;
                    break;
                case VAL_NODE_VELZ:
                    resultArr[i] = (currentGeom->xyz[2][i] -
                                    state_prev->nodes->xyz[2][i]) / delta_t;
                    break;
                case VAL_NODE_VELMAG:
                    tmp[0] = currentGeom->xyz[0][i] -
                             state_prev->nodes->xyz[0][i];
                    tmp[1] = currentGeom->xyz[1][i] -
                             state_prev->nodes->xyz[1][i];
                    tmp[2] = currentGeom->xyz[2][i] -
                             state_prev->nodes->xyz[2][i];
                    resultArr[i] = VEC_LENGTH( tmp ) / delta_t;
            }
        }
        fr_state( state_prev );
    }
}


/************************************************************
 * TAG( compute_node_acceleration )
 *
 * Computes the acceleration at the nodes using a central difference
 * (unless the accelerations are contained in the data).
 */
void
compute_node_acceleration( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    Nodal *nodes, *nodes_p, *nodes_n;
    State *state_prev, *state_next;
    float delta_t, one_over_tsqr;
    float disp, dispf, dispb, disp_inc;
    int dir, i;
    
    if ( analy->result_id != VAL_NODE_ACCX
         && analy->result_id != VAL_NODE_ACCY
         && analy->result_id != VAL_NODE_ACCZ
         && analy->result_id != VAL_NODE_ACCMAG )
    {
        popup_dialog( WARNING_POPUP, 
		      "Unknown acceleration component requested!" );
	return;
    }

    nodes = analy->state_p->nodes;

    if ( is_in_database( VAL_NODE_ACCX ) )
    {
        /* Load accelerations straight from the database. */

        switch ( analy->result_id )
        {
            case VAL_NODE_ACCX:
            case VAL_NODE_ACCY:
            case VAL_NODE_ACCZ:
                get_result( analy->result_id, analy->cur_state, resultArr );
                break;
            case VAL_NODE_ACCMAG:
                get_result( VAL_NODE_ACCX, analy->cur_state,
                            analy->tmp_result[0] );
                get_result( VAL_NODE_ACCY, analy->cur_state,
                            analy->tmp_result[1] );
                get_result( VAL_NODE_ACCZ, analy->cur_state,
                            analy->tmp_result[2] );
                for ( i = 0; i < nodes->cnt; i++ )
                    resultArr[i] = sqrt((double)
                       ( (analy->tmp_result[0][i]*analy->tmp_result[0][i])
                       + (analy->tmp_result[1][i]*analy->tmp_result[1][i])
                       + (analy->tmp_result[2][i]*analy->tmp_result[2][i]) ));
                break;
        }
    }
    else
    {
        /* Calculate accelerations using central differences. */

        switch ( analy->result_id )
        {
            case VAL_NODE_ACCX:
                dir = 0;
                break;
            case VAL_NODE_ACCY:
                dir = 1;
                break;
            case VAL_NODE_ACCZ:
                dir = 2;
                break;
            case VAL_NODE_ACCMAG:
                dir = 3;
                break;
        }

        if ( analy->cur_state == 0 )
            state_prev = NULL;
        else
            state_prev = get_state( analy->cur_state-1, NULL );

        if ( analy->cur_state + 1 >= analy->num_states )
            state_next = NULL;
        else
            state_next = get_state( analy->cur_state+1, NULL );

        if ( state_prev == NULL )
        {
            /* No prev -- use forward difference. */
            delta_t = state_next->time - analy->state_p->time;
            one_over_tsqr = 1.0 / ( delta_t * delta_t );
            nodes_n = state_next->nodes;

            for ( i = 0; i < nodes->cnt; i++ )
            {
                if ( dir < 3 )
                {
                    disp_inc = nodes->xyz[dir][i] - nodes_n->xyz[dir][i];
                    disp = nodes->xyz[dir][i];
                    dispf = nodes_n->xyz[dir][i];
                }
                else
                {
                    disp_inc = sqrt( (double)
                                SQR(nodes->xyz[0][i] - nodes_n->xyz[0][i]) +
                                SQR(nodes->xyz[1][i] - nodes_n->xyz[1][i]) +
                                SQR(nodes->xyz[2][i] - nodes_n->xyz[2][i]) );
                    disp = sqrt( (double) SQR(nodes->xyz[0][i]) +
                                          SQR(nodes->xyz[1][i]) +
                                          SQR(nodes->xyz[2][i]) );
                    dispf = sqrt( (double) SQR(nodes_n->xyz[0][i]) +
                                           SQR(nodes_n->xyz[1][i]) +
                                           SQR(nodes_n->xyz[2][i]) );
                }
                resultArr[i] = (dispf - disp + disp_inc) * one_over_tsqr;
            }
        }
        else if ( state_next == NULL )
        {
            /* No next -- use backward difference. */
            delta_t = analy->state_p->time - state_prev->time;
            one_over_tsqr = 1.0 / ( delta_t * delta_t );
            nodes_p = state_prev->nodes;

            for ( i = 0; i < nodes->cnt; i++ )
            {
                if ( dir < 3 )
                {
                    disp_inc = nodes_p->xyz[dir][i] - nodes->xyz[dir][i];
                    disp = nodes->xyz[dir][i];
                    dispb = nodes_p->xyz[dir][i];
                }
                else
                {
                    disp_inc = sqrt( (double)
                                SQR(nodes_p->xyz[0][i] - nodes->xyz[0][i]) +
                                SQR(nodes_p->xyz[1][i] - nodes->xyz[1][i]) +
                                SQR(nodes_p->xyz[2][i] - nodes->xyz[2][i]) );
                    disp = sqrt( (double) SQR(nodes->xyz[0][i]) +
                                          SQR(nodes->xyz[1][i]) +
                                          SQR(nodes->xyz[2][i]) );
                    dispb = sqrt( (double) SQR(nodes_p->xyz[0][i]) +
                                           SQR(nodes_p->xyz[1][i]) +
                                           SQR(nodes_p->xyz[2][i]) );
                }
                resultArr[i] = (disp_inc - disp + dispb) * one_over_tsqr;
            }
        }
        else
        {
            /* Default -- use central difference. */
            delta_t = 0.5*(state_next->time - state_prev->time);
            one_over_tsqr = 1.0 / ( delta_t * delta_t );
            nodes_n = state_next->nodes;
            nodes_p = state_prev->nodes;

            for ( i = 0; i < nodes->cnt; i++ )
            {
                if ( dir < 3 )
                {
                    disp = nodes->xyz[dir][i];
                    dispf = nodes_n->xyz[dir][i];
                    dispb = nodes_p->xyz[dir][i];
                }
                else
                {
                    disp = sqrt( (double) SQR(nodes->xyz[0][i]) +
                                          SQR(nodes->xyz[1][i]) +
                                          SQR(nodes->xyz[2][i]) );
                    dispf = sqrt( (double) SQR(nodes_n->xyz[0][i]) +
                                           SQR(nodes_n->xyz[1][i]) +
                                           SQR(nodes_n->xyz[2][i]) );
                    dispb = sqrt( (double) SQR(nodes_p->xyz[0][i]) +
                                           SQR(nodes_p->xyz[1][i]) +
                                           SQR(nodes_p->xyz[2][i]) );
                }
                resultArr[i] = (dispf - 2.0*disp + dispb) * one_over_tsqr;
            }
        }

        fr_state( state_prev );
        fr_state( state_next );
    }
}


/************************************************************
 * TAG( compute_node_temperature )
 *
 * Compute temperature results at nodes. 
 */
void
compute_node_temperature( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    if ( is_in_database( analy->result_id ) )
        get_result( analy->result_id, analy->cur_state, resultArr );
    else
    {
        wrt_text( "No temperature data present.\n" );
        analy->result_id = VAL_NONE;
        analy->show_hex_result = FALSE;
        analy->show_shell_result = FALSE;
        return;
    }
}


/************************************************************
 * TAG( compute_node_pr_intense )
 *
 * Computes pressure intensity at the nodes.  This derived
 * variable is for PING.
 *
 * Algorithm:
 *
 *     I = 10.0 log_10[ ( press / p_ref )^2 ]
 *
 *     where p_ref is a constant to be input by the user.
 */
void
compute_node_pr_intense( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    double psr;
    int cnt, i;

    cnt = analy->geom_p->nodes->cnt;

    /* Pressure is actually stored in Velocity Y for PING. */
    get_result( VAL_NODE_VELY, analy->cur_state, resultArr );

    for ( i = 0; i < cnt; i++ )
    {
        if ( resultArr[i] == 0.0 )
            resultArr[i] = -INFINITY;
        else
        {
            psr = resultArr[i] / ping_pr_ref;
            resultArr[i] = 10.0 * log10( psr * psr );
        }
    }
}


/************************************************************
 * TAG( set_pr_ref )
 *
 * Set the reference pressure for pressure intensity
 * computation.
 */
void
set_pr_ref( pr_ref )
float pr_ref;
{
    ping_pr_ref = pr_ref;
}


/************************************************************
 * TAG( compute_node_helicity )
 *
 * Computes helicity at the nodes.  This 3D-only scalar is the
 * dot-product of the vorticity and the velocity.
 */
void
compute_node_helicity( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    int cnt, i;
    float *u, *v, *w, *vort_x, *vort_y, *vort_z;

    cnt = analy->geom_p->nodes->cnt;
    u = analy->tmp_result[0];
    v = analy->tmp_result[1];
    w = analy->tmp_result[2];
    vort_x = analy->tmp_result[3];
    vort_y = analy->tmp_result[4];
    vort_z = analy->tmp_result[5];

    get_result( VAL_NODE_VELX, analy->cur_state, u );
    get_result( VAL_NODE_VELY, analy->cur_state, v );
    get_result( VAL_NODE_VELZ, analy->cur_state, w );
    get_result( VAL_NODE_ACCX, analy->cur_state, vort_x );
    get_result( VAL_NODE_ACCY, analy->cur_state, vort_y );
    get_result( VAL_NODE_ACCZ, analy->cur_state, vort_z );

    for ( i = 0; i < cnt; i++ )
    {
        resultArr[i] = u[i] * vort_x[i]
		       + v[i] * vort_y[i]
		       + w[i] * vort_z[i];
    }
}


/************************************************************
 * TAG( compute_node_enstrophy )
 *
 * Computes enstrophy at the nodes.  This 3D-only scalar is 
 * one-half the square of the vorticity.
 */
void
compute_node_enstrophy( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    int cnt, i;
    float *vort_x, *vort_y, *vort_z;

    cnt = analy->geom_p->nodes->cnt;
    vort_x = analy->tmp_result[0];
    vort_y = analy->tmp_result[1];
    vort_z = analy->tmp_result[2];

    get_result( VAL_NODE_ACCX, analy->cur_state, vort_x );
    get_result( VAL_NODE_ACCY, analy->cur_state, vort_y );
    get_result( VAL_NODE_ACCZ, analy->cur_state, vort_z );

    for ( i = 0; i < cnt; i++ )
    {
        resultArr[i] = (SQR( vort_x[i] ) + SQR( vort_y[i] ) + SQR( vort_z[i] ))
	               / 2.0;
    }
}


/************************************************************
 * TAG( compute_vector_component )
 *
 * Computes the length of the component of the currently
 * defined vector result in the specified direction at each 
 * node.
 */
void
compute_vector_component( analy, resultArr )
Analysis *analy;
float *resultArr;
{
    float *vec_comp[3];
    float vec[3];
    float *tmp_res;
    int i, j, tmp_id, node_qty, dim;
    
    tmp_id = analy->result_id;
    tmp_res = analy->result;
    node_qty = analy->geom_p->nodes->cnt;
    dim = analy->dimension;
    
    /* Read in the vector components. */
    for ( i = 0; i < dim; i++ )
    {
        vec_comp[i] = analy->tmp_result[i];

        if ( analy->vec_id[i] != VAL_NONE )
        {
            analy->result_id = analy->vec_id[i];
            analy->result = vec_comp[i];
            
            /* 
             * Recursively call load_result() so that element-based
             * components will be interpolated back to nodes.
             */
            load_result( analy, FALSE );
        }
        else
            memset( vec_comp[i], 0, node_qty * sizeof( float ) );
    }
    analy->result_id = tmp_id;
    analy->result = tmp_res;
    
    /* 
     * Dot each nodal vector with the direction unit vector to
     * generate the magnitude of its component in that direction.
     */
    if ( dim == 3 )
    {
        for ( i = 0; i < node_qty; i++ )
        {
            for ( j = 0; j < 3; j++ )
                vec[j] = vec_comp[j][i];
            
            resultArr[i] = VEC_DOT( vec, analy->dir_vec );
        }
    }
    else
    {
        for ( i = 0; i < node_qty; i++ )
        {
            for ( j = 0; j < 2; j++ )
                vec[j] = vec_comp[j][i];
            
            resultArr[i] = VEC_DOT_2D( vec, analy->dir_vec );
        }
    }
}


