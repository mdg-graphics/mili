/* $Id$ */
/*
 * node.c - Routines for computing derived nodal quantities.
 *
 *      Thomas Spelce
 *      Lawrence Livermore National Laboratory
 *      Jun 25 1992
 *
  *************************************************************************
 *
 * Modification History
 *
 * Date        By Whom      Description
 * ==========  ===========  ==============================================
 * 05/23/2005  I. R. Corey  Added variable to identify results that must be
 *                          recalculated.
 *                          See SRC#: 316
 *
 * 07/19/2005  E. M. Pierce Changed nodal displacement algorithm to work
 *                          with TH databases.
 *                          See SRC#: 329
 *
 * 04/30/2008  I.R. Corey   Added new derived nodal result = dispr, radial
 *                          nodal displacement.
 *                          See SRC#: 532
 *
 * 12/05/2008 I. R. Corey   Add DP calculations for all derived nodal
 *                          results. See mili_db_set_results().
 *                          See SRC#: 556
 *************************************************************************
 */

#include <stdlib.h>
#include "viewer.h"
#include "misc.h"

#ifndef INFINITY
#define INFINITY        4.2535295865117308e37 /* (2^125) */
#endif                                                                                    

/* Reference pressure intensity for PING. */
static float ping_pr_ref = 1.0;

void *
create_node_array( Analysis *analy)
{
    Result *p_result;
    State_rec_obj *p_sro;
    Subrec_obj *p_subrec;
    int index;
    int subrec, srec;
    int *object_ids;
    int num_nodes;
    MO_class_data *p_node_class;
    Bool_type double_prec_pos = FALSE;
    void *tmp_nodesDp;

    p_result = analy->cur_result;
    index = analy->result_index;
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;

    object_ids = p_subrec->object_ids;

    p_node_class = MESH_P( analy )->node_geom;
    num_nodes    = p_node_class->qty;

    if (MESH_P( analy )->double_precision_nodpos)
    {
        tmp_nodesDp = (void *) NEW_N( double, num_nodes*analy->dimension,
                                      "Tmp DP node cache" );
        double_prec_pos = TRUE;
    }
    else
        tmp_nodesDp = (void *)NEW_N( float, num_nodes*analy->dimension,
                                     "Tmp DP node cache" );

    p_sro = analy->srec_tree + analy->state_p->srec_id;

    load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                 analy->cur_state + 1, double_prec_pos, tmp_nodesDp );

    return tmp_nodesDp;
}

/* ARGSUSED 2 */
/************************************************************
 * TAG( compute_node_displacement )
 *
 * Computes the displacement at nodes given the current geometry
 * and initial configuration.
 */
void
compute_node_displacement( Analysis *analy, float *resultArr,
                           Bool_type interpolate )
{
    int i, node_qty;
    GVec3D *nodes3d, *onodes3d;
    GVec2D *nodes2d, *onodes2d;

    int coord;

    /* EMP: Added July 19, 2005 - Support
     *      for timehistory databases
     */
    Result *p_result;
    State_rec_obj *p_sro;
    Subrec_obj *p_subrec;
    int obj_qty;
    int index;
    int subrec, srec;
    int *object_ids;
    int  node_idx;
    Bool_type single_prec_pos;
    Bool_type map_timehist_coords = FALSE;
    int       elem_index;
    int       obj_cnt, obj_num, *obj_ids;

    /* Added December 04, 2008: IRC
     *  Add logic to use DP nodal coordinates.
     */

    GVec3D2P *nodes3dDp, *onodes3dDp;
    GVec2D2P *nodes2dDp, *onodes2dDp;
    void *tmp_nodesDp=0;

    int  num_nodes;

    MO_class_data *p_node_class;


    p_result = analy->cur_result;
    index = analy->result_index;
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    node_qty = p_subrec->subrec.qty_objects;
    object_ids = p_subrec->object_ids;

    p_node_class = MESH_P( analy )->node_geom;
    num_nodes    = p_node_class->qty;

    if (analy->stateDB)
    {
        node_qty = MESH( analy ).node_geom->qty;

        if( analy->dimension == 3 )
            nodes3d = analy->state_p->nodes.nodes3d;
        else
            nodes2d = analy->state_p->nodes.nodes2d;

        if (MESH_P( analy )->double_precision_nodpos)
        {
            p_sro = analy->srec_tree + analy->state_p->srec_id;



            tmp_nodesDp = (void *)NEW_N( double, num_nodes*analy->dimension,
                                         "Tmp DP node cache for calc node disp" );
            load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                         analy->cur_state + 1, FALSE,
                         tmp_nodesDp );

            if( analy->dimension == 3 )
                nodes3dDp = (GVec3D2P *) tmp_nodesDp;
            else
                nodes2dDp = (GVec2D2P *) tmp_nodesDp;

        }
    }
    else
    {

        if (MESH_P( analy )->double_precision_nodpos)
            tmp_nodesDp = (void *) NEW_N( double, num_nodes*analy->dimension,
                                          "Tmp DP node cache for calc node disp" );
        else
            tmp_nodesDp = (void *)NEW_N( float, num_nodes*analy->dimension,
                                         "Tmp DP node cache for calc node disp" );

        p_sro = analy->srec_tree + analy->state_p->srec_id;

        load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                     analy->cur_state + 1, TRUE,
                     tmp_nodesDp );
        /*
                     (void *) analy->tmp_result[1] );
                     */

        if( analy->dimension == 3 )
        {
            if (MESH_P( analy )->double_precision_nodpos)
                nodes3dDp = (GVec3D2P *)tmp_nodesDp ;
            else
                nodes3d = (GVec3D *)tmp_nodesDp ;/*
            nodes3d = (GVec3D *) analy->tmp_result[1];*/
        }
        else
        {
            if (MESH_P( analy )->double_precision_nodpos)
                nodes2dDp = (GVec2D2P *)tmp_nodesDp ;
            else
                nodes2d = (GVec2D *)tmp_nodesDp ;
        }/*
            nodes2d = (GVec2D *) analy->tmp_result[1];*/

    }

    coord = (int) (analy->cur_result->name[4] - 'x'); /* 0, 1, or 2 */

    if ( analy->dimension == 3 )
    {
        if (MESH_P( analy )->double_precision_nodpos && analy->cur_ref_state_dataDp)
            onodes3dDp = (GVec3D2P *) analy->cur_ref_state_dataDp;
        else
            onodes3d = (GVec3D *) analy->cur_ref_state_data;

        for ( i = 0; i < node_qty; i++ )
        {
            node_idx = ( object_ids ) ? object_ids[i] : i;

            if (MESH_P( analy )->double_precision_nodpos && analy->cur_ref_state_dataDp)
                resultArr[node_idx] = nodes3dDp[node_idx][coord] - onodes3dDp[node_idx][coord];
            else
                resultArr[node_idx] = nodes3d[node_idx][coord] - onodes3d[node_idx][coord];
        }
    }
    else
    {
        if (MESH_P( analy )->double_precision_nodpos && analy->cur_ref_state_dataDp)
            onodes2dDp = (GVec2D2P *) analy->cur_ref_state_dataDp;
        else
            onodes2d = (GVec2D *) analy->cur_ref_state_data;

        for ( i = 0; i < node_qty; i++ )
        {
            node_idx = ( object_ids ) ? object_ids[i] : i;

            if (MESH_P( analy )->double_precision_nodpos && analy->cur_ref_state_dataDp)
                resultArr[node_idx] = nodes2dDp[i][coord] - onodes2dDp[node_idx][coord];
            else
                resultArr[node_idx] = nodes2d[i][coord] - onodes2d[node_idx][coord];
        }
    }

    analy->cur_result->modifiers.use_flags.use_ref_state = 1;
    analy->cur_result->modifiers.ref_state = analy->reference_state;

    if (tmp_nodesDp)
        free( tmp_nodesDp );
}


/************************************************************
 * TAG( compute_node_radial_displacement )
 *
 * Computes the radial displacement at nodes given the current
 * geometry and initial configuration.
 */
void
compute_node_radial_displacement( Analysis *analy, float *resultArr,
                                  Bool_type interpolate )
{
    int i, node_qty;
    GVec3D *nodes3d, *onodes3d;
    GVec2D *nodes2d, *onodes2d;

    double dx, dy, dr;

    int coord_x, coord_y;

    Result *p_result;
    State_rec_obj *p_sro;
    Subrec_obj *p_subrec;
    int obj_qty;
    int index;
    int subrec, srec;
    int *object_ids;
    int  node_idx;
    Bool_type single_prec_pos;
    Bool_type map_timehist_coords = FALSE;
    int       elem_index;
    int       obj_cnt, obj_num, *obj_ids;

    /* Added December 04, 2008: IRC
     *  Add logic to use DP nodal coordinates.
     */

    GVec3D2P *nodes3dDp, *onodes3dDp;
    GVec2D2P *nodes2dDp, *onodes2dDp;

    int           num_nodes;
    void *tmp_nodesDp=0;
    /*
    double        *tmp_nodesDp=NULL;
    */
    MO_class_data *p_node_class;

    p_result = analy->cur_result;
    index = analy->result_index;
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    node_qty = p_subrec->subrec.qty_objects;
    object_ids = p_subrec->object_ids;

    p_node_class = MESH_P( analy )->node_geom;
    num_nodes    = p_node_class->qty;

    if (analy->stateDB)
    {
        node_qty = MESH( analy ).node_geom->qty;

        if( analy->dimension == 3 )
            nodes3d = analy->state_p->nodes.nodes3d;
        else
            nodes2d = analy->state_p->nodes.nodes2d;

        if (MESH_P( analy )->double_precision_nodpos)
        {
            p_sro = analy->srec_tree + analy->state_p->srec_id;


            tmp_nodesDp = (void *)NEW_N( double, num_nodes*analy->dimension,
                                         "Tmp DP node cache for calc node disp" );

            load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                         analy->cur_state + 1, FALSE, tmp_nodesDp );


            if( analy->dimension == 3 )
                nodes3dDp = (GVec3D2P *) tmp_nodesDp;
            else
                nodes2dDp = (GVec2D2P *) tmp_nodesDp;
        }
    }
    else
    {
        if (MESH_P( analy )->double_precision_nodpos)
            tmp_nodesDp = (void *) NEW_N( double, num_nodes*analy->dimension,
                                          "Tmp DP node cache for calc node disp" );
        else
            tmp_nodesDp = (void *)NEW_N( float, num_nodes*analy->dimension,
                                         "Tmp DP node cache for calc node disp" );

        p_sro = analy->srec_tree + analy->state_p->srec_id;


        load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                     analy->cur_state + 1,
                     ! (MESH_P( analy)->double_precision_nodpos),
                     tmp_nodesDp );

        if( analy->dimension == 3 )
        {
            if (MESH_P( analy )->double_precision_nodpos)
                nodes3dDp = (GVec3D2P *)tmp_nodesDp ;
            else
                nodes3d = (GVec3D *)tmp_nodesDp ;
        }
        else
        {
            if (MESH_P( analy )->double_precision_nodpos)
                nodes2dDp = (GVec2D2P *)tmp_nodesDp ;
            else
                nodes2d = (GVec2D *)tmp_nodesDp ;
        }
    }

    coord_x = 0;
    coord_y = 1;

    if ( analy->dimension == 3 )
    {
        if (MESH_P( analy )->double_precision_nodpos && analy->cur_ref_state_dataDp)
            onodes3dDp = (GVec3D2P *) analy->cur_ref_state_dataDp;
        else
            onodes3d = (GVec3D *) analy->cur_ref_state_data;

        for ( i = 0; i < node_qty; i++ )
        {
            node_idx = ( object_ids ) ? object_ids[i] : i;

            if (MESH_P( analy )->double_precision_nodpos && analy->cur_ref_state_dataDp)
            {
                dx = nodes3dDp[node_idx][coord_x] - onodes3dDp[node_idx][coord_x];
                dy = nodes3dDp[node_idx][coord_y] - onodes3dDp[node_idx][coord_y];
            }
            else
            {
                dx = nodes3d[node_idx][coord_x] - onodes3d[node_idx][coord_x];
                dy = nodes3d[node_idx][coord_y] - onodes3d[node_idx][coord_y];
            }

            dr = sqrt( (dx*dx)+(dy*dy) );
            resultArr[node_idx] = dr;
        }
    }
    else
    {
        if (MESH_P( analy )->double_precision_nodpos && analy->cur_ref_state_dataDp)
            onodes2dDp = (GVec2D2P *) analy->cur_ref_state_dataDp;
        else
            onodes2d = (GVec2D *) analy->cur_ref_state_data;

        for ( i = 0; i < node_qty; i++ )
        {
            node_idx = ( object_ids ) ? object_ids[i] : i;

            if (MESH_P( analy )->double_precision_nodpos && analy->cur_ref_state_dataDp)
            {
                dx = nodes2dDp[node_idx][coord_x] - onodes2dDp[node_idx][coord_x];
                dy = nodes2dDp[node_idx][coord_y] - onodes2dDp[node_idx][coord_y];
            }
            else
            {
                dx = nodes2d[node_idx][coord_x] - onodes2d[node_idx][coord_x];
                dy = nodes2d[node_idx][coord_y] - onodes2d[node_idx][coord_y];
            }

            dr = sqrt( (dx*dx)+(dy*dy) );
            resultArr[node_idx] = dr;
        }
    }

    analy->cur_result->modifiers.use_flags.use_ref_state = 1;
    analy->cur_result->modifiers.ref_state = analy->reference_state;

    if (tmp_nodesDp)
        free( tmp_nodesDp );
}


/* ARGSUSED 2 */
/************************************************************
 * TAG( compute_node_displacement_mag )
 *
 * Computes the magnitude of the displacement at nodes given the
 * current geometry and initial configuration.
 */
void
compute_node_displacement_mag( Analysis *analy, float *resultArr,
                               Bool_type interpolate )
{
    int i, node_qty;
    GVec3D *nodes3d, *onodes3d;
    GVec2D *nodes2d, *onodes2d;
    float dx, dy, dz;

    /* EMP: Added July 19, 2005 - Support
     *      for timehistory databases
     */
    Result *p_result;
    State_rec_obj *p_sro;
    Subrec_obj *p_subrec;
    int obj_qty;
    int index;
    int subrec, srec;
    int *object_ids;
    int  node_idx;

    /* Added December 04, 2008: IRC
     *  Add logic to use DP nodal coordinates.
     */

    GVec3D2P *nodes3dDp, *onodes3dDp;
    GVec2D2P *nodes2dDp, *onodes2dDp;

    int           num_nodes;
    void *tmp_nodesDp=0;
    /*
    double        *tmp_nodesDp=NULL;
    */
    MO_class_data *p_node_class;

    p_result = analy->cur_result;
    index = analy->result_index;
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    node_qty = p_subrec->subrec.qty_objects;
    object_ids = p_subrec->object_ids;

    p_node_class = MESH_P( analy )->node_geom;
    num_nodes    = p_node_class->qty;

    if (analy->stateDB)
    {
        node_qty = MESH( analy ).node_geom->qty;

        if( analy->dimension == 3 )
            nodes3d = analy->state_p->nodes.nodes3d;
        else
            nodes2d = analy->state_p->nodes.nodes2d;

        if (MESH_P( analy )->double_precision_nodpos)
        {
            p_sro = analy->srec_tree + analy->state_p->srec_id;

            tmp_nodesDp = (void *)NEW_N( double, num_nodes*analy->dimension,
                                         "Tmp DP node cache for calc node disp" );

            load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                         analy->cur_state + 1, FALSE,
                         tmp_nodesDp );

            if( analy->dimension == 3 )
                nodes3dDp = (GVec3D2P *) tmp_nodesDp;
            else
                nodes2dDp = (GVec2D2P *) tmp_nodesDp;
        }
    }
    else
    {
        if (MESH_P( analy )->double_precision_nodpos)
            tmp_nodesDp = (void *) NEW_N( double, num_nodes*analy->dimension,
                                          "Tmp DP node cache for calc node disp" );
        else
            tmp_nodesDp = (void *)NEW_N( float, num_nodes*analy->dimension,
                                         "Tmp DP node cache for calc node disp" );

        p_sro = analy->srec_tree + analy->state_p->srec_id;
        load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                     analy->cur_state + 1,
                     ! (MESH_P( analy)->double_precision_nodpos),
                     (void *) tmp_nodesDp );

        if( analy->dimension == 3 )
            if (MESH_P( analy )->double_precision_nodpos)
                nodes3dDp = (GVec3D2P *)tmp_nodesDp ;
            else
                nodes3d = (GVec3D *)tmp_nodesDp ;
        else if (MESH_P( analy )->double_precision_nodpos)
            nodes2dDp = (GVec2D2P *)tmp_nodesDp ;
        else
            nodes2d = (GVec2D *)tmp_nodesDp ;
    }

    if ( analy->dimension == 3 )
    {
        if (MESH_P( analy )->double_precision_nodpos && analy->cur_ref_state_dataDp)
            onodes3dDp = (GVec3D2P *) analy->cur_ref_state_dataDp;
        else
            onodes3d = (GVec3D *) analy->cur_ref_state_data;

        for ( i = 0; i < node_qty; i++ )
        {
            node_idx = ( object_ids ) ? object_ids[i] : i;

            if (MESH_P( analy )->double_precision_nodpos && analy->cur_ref_state_dataDp)
            {
                dx = nodes3dDp[node_idx][0] - onodes3dDp[node_idx][0];
                dy = nodes3dDp[node_idx][1] - onodes3dDp[node_idx][1];
                dz = nodes3dDp[node_idx][2] - onodes3dDp[node_idx][2];
            }
            else
            {
                dx = nodes3d[node_idx][0] - onodes3d[node_idx][0];
                dy = nodes3d[node_idx][1] - onodes3d[node_idx][1];
                dz = nodes3d[node_idx][2] - onodes3d[node_idx][2];
            }

            resultArr[node_idx] = sqrt( (double) dx * dx + dy * dy + dz * dz );
        }
    }
    else
    {
        onodes2d = (GVec2D *) analy->cur_ref_state_data;

        if (MESH_P( analy )->double_precision_nodpos && analy->cur_ref_state_dataDp)
            onodes2dDp = (GVec2D2P *) analy->cur_ref_state_dataDp;
        else
            onodes2d = (GVec2D *) analy->cur_ref_state_data;

        for ( i = 0; i < node_qty; i++ )
        {
            node_idx = ( object_ids ) ? object_ids[i] : i;

            if (MESH_P( analy )->double_precision_nodpos && analy->cur_ref_state_dataDp)
            {
                dx = nodes2dDp[node_idx][0] - onodes2dDp[node_idx][0];
                dy = nodes2dDp[node_idx][1] - onodes2dDp[node_idx][1];
            }
            else
            {
                dx = nodes2d[node_idx][0] - onodes2d[node_idx][0];
                dy = nodes2d[node_idx][1] - onodes2d[node_idx][1];
            }

            resultArr[node_idx] = sqrt( (double) dx * dx + dy * dy );
        }
    }

    analy->cur_result->modifiers.use_flags.use_ref_state = 1;
    analy->cur_result->modifiers.ref_state = analy->reference_state;

    if (tmp_nodesDp)
        free( tmp_nodesDp );
}

/************************************************************
 * TAG( compute_node_modaldisplacement_mag )
 *
 * Computes the magnitude of the displacement of a modal
 * analysis at nodes given the current geometry.
 */
void
compute_node_modaldisplacement_mag( Analysis *analy, float *resultArr,
                                    Bool_type interpolate )
{
    float (*modedisp)[3];
    double (*modedisp_dp)[3];
    float *modedispscalar;
    double *modedispscalar_dp;
    int i;
    double *result_buf;
    double dx, dy, dz;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int obj_qty;
    int index;
    int node_qty;
    int  node_idx;
    int *object_ids;
    State_variable sv;
    Subrec_obj *p_subrec;
    MO_class_data *p_node_class;
    char *primal_list[1];
    int rval=0;

    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    object_ids = p_subrec->object_ids;
    obj_qty = p_subrec->subrec.qty_objects;

    node_qty = MESH( analy ).node_geom->qty;

    rval = mc_get_svar_def( analy->db_ident, primals[0], &sv );

    /* Read the database. */
    primal_list[0] = strdup(primals[0]);

    result_buf = NEW_N( double, node_qty*sv.vec_size*2,
                        "Current Nodal Results" );

    analy->db_get_results( analy->db_ident, analy->cur_state+1, subrec, 1,
                           primal_list, (void *) result_buf );

    if ( sv.vec_size==3 )
        if ( sv.num_type==M_FLOAT )
        {
            modedisp     = (float (*)[3]) result_buf;
        }
        else
        {
            modedisp_dp     = (double (*)[3]) result_buf;
        }
    else if ( sv.num_type==M_FLOAT )
    {
        modedispscalar    = (float *) result_buf;
    }
    else
    {
        modedispscalar_dp = (double *) result_buf;
    }

    free( primal_list[0] );

    for ( i = 0;
            i < node_qty;
            i++ )
    {
        if ( sv.vec_size==3 )
            if ( sv.num_type==M_FLOAT )
            {
                dx =  modedisp[i][0];
                dy =  modedisp[i][1];
                dz =  modedisp[i][2];
            }
            else
            {
                dx =  modedisp_dp[i][0];
                dy =  modedisp_dp[i][1];
                dz =  modedisp_dp[i][2];
            }
        else if ( sv.num_type==M_FLOAT )
        {
            dx =  modedispscalar[i];
        }
        else
        {
            dx =  modedispscalar_dp[i];
        }

        if ( sv.vec_size==3 )
            resultArr[i] = sqrt( (double) dx * dx + dy * dy + dz * dz );
        else
            resultArr[i] = sqrt( (double) dx * dx );
    }

    free( result_buf );
}


/* ARGSUSED 2 */
/************************************************************
 * TAG( compute_node_velocity )
 *
 * Computes the velocity at the nodes using a backwards difference
 * (unless the velocities are contained in the data).
 */
void
compute_node_velocity( Analysis *analy, float *resultArr,
                       Bool_type interpolate )
{
    State2 *state_prev;
    float delta_t;
    float tmp[3];
    int i;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int obj_qty;
    int index;
    int *object_ids;
    Subrec_obj *p_subrec;
    float *result_buf;
    char *vcomps[] = { "vx", "vy", "vz" };
    char nbuf[16];
    char *new_primals[1];
    int coord, dim;
    GVec2D *vels2d, *pos2d_0, *pos2d_1;
    GVec3D *vels3d, *pos3d_0, *pos3d_1;

    /* Added December 04, 2008: IRC
     *  Add logic to use DP nodal coordinates.
     */

    GVec2D2P *pos2dDp_0, *pos2dDp_1;
    GVec3D2P *pos3dDp_0, *pos3dDp_1;

    int           num_nodes=0, tmp_state=0;
    double        *tmp_nodesDp=NULL, *tmp_nodesDp_lastState=NULL;
    MO_class_data *p_node_class;
    State_rec_obj *p_sro;

    dim = analy->dimension;
    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];

    p_node_class = MESH_P( analy )->node_geom;
    num_nodes    = p_node_class->qty;

    /* Determine if result is actually primal or derived from displacements. */
    if ( strcmp( primals[0], "nodvel" ) == 0  || strcmp( primals[0], "rotvel") == 0)
    {
        /* Velocities are stored in the data. */

        subrec = p_result->subrecs[index];
        srec = p_result->srec_id;
        p_subrec = analy->srec_tree[srec].subrecs + subrec;
        obj_qty = p_subrec->subrec.qty_objects;
        object_ids = p_subrec->object_ids;

        /*
         * If request is not for magnitude, augment primal spec with the
         * component and let I/O code extract it from the vector array in the
         * database.
         */
        if ((p_result->name[0] != 'r' && p_result->name[6] != 'm') && p_result->name[3] != 'm') 
        /* velmag vs. velx, vely, or velz  or rotvelmag vs rvx, rvy, rvz*/
        {
            coord = (int) (p_result->name[3] - 'x');
            sprintf( nbuf, "%s[%s]", primals[0], vcomps[coord] );
            new_primals[0] = nbuf;
            primals = (char **) new_primals;
            result_buf = ( object_ids == NULL ) ? resultArr
                         : analy->tmp_result[0];
        }
        else
            result_buf = analy->tmp_result[0];

        /* Read the database. */
        analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 1,
                               primals, (void *) result_buf );

        if ( p_result->name[3] == 'm'  || (p_result->name[0] == 'r' && p_result->name[6] == 'm'))
        {
            /* Calculate magnitude. */
            if ( dim == 2 )
            {
                vels2d = (float (*)[2]) result_buf;

                if ( object_ids == NULL )
                {
                    for ( i = 0; i < obj_qty; i++ )
                        resultArr[i] = sqrt( (double)
                                             (vels2d[i][0] * vels2d[i][0]
                                              + vels2d[i][1] * vels2d[i][1]) );
                }
                else
                {
                    for ( i = 0; i < obj_qty; i++ )
                        resultArr[object_ids[i]] = sqrt( (double)
                                                         (vels2d[i][0] * vels2d[i][0]
                                                          + vels2d[i][1] * vels2d[i][1]) );
                }
            }
            else
            {
                vels3d = (float (*)[3]) result_buf;

                if ( object_ids == NULL )
                {
                    for ( i = 0; i < obj_qty; i++ )
                        resultArr[i] = sqrt( (double)
                                             (vels3d[i][0] * vels3d[i][0]
                                              + vels3d[i][1] * vels3d[i][1]
                                              + vels3d[i][2] * vels3d[i][2]) );
                }
                else
                {
                    for ( i = 0; i < obj_qty; i++ )
                        resultArr[object_ids[i]] = sqrt( (double)
                                                         (vels3d[i][0] * vels3d[i][0]
                                                          + vels3d[i][1] * vels3d[i][1]
                                                          + vels3d[i][2] * vels3d[i][2]) );
                }
            }
        }
        else if ( object_ids != NULL )
        {
            /* Not magnitude result, but data needs reordering. */
            reorder_float_array( obj_qty, object_ids, 1, result_buf,
                                 resultArr );
        }
    }
    else
    {
        /* Read the previous coords in and calculate the velocities
         * using simple differences.  Reordering, if necessary, is
         * handled in db_get_state().
         */
        if ( analy->cur_state == 0 )
        {
            for ( i = 0; i < obj_qty; i++ )
            {
                resultArr[i] = 0.0;
            }
        }
        else
        {
            obj_qty = MESH_P( analy )->node_geom->qty;

            analy->db_get_state( analy, analy->cur_state - 1, NULL, &state_prev,
                                 NULL );
            delta_t = analy->state_p->time - state_prev->time;

            if ( dim == 2 )
            {
                pos2d_0 = state_prev->nodes.nodes2d;
                pos2d_1 = analy->state_p->nodes.nodes2d;

                if (MESH_P( analy )->double_precision_nodpos)
                {
                    p_sro = analy->srec_tree + analy->state_p->srec_id;

                    tmp_nodesDp = NEW_N( double, num_nodes*analy->dimension,
                                         "Tmp DP node cache for calc node velocity" );

                    load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                                 analy->cur_state + 1, FALSE,
                                 (void *) tmp_nodesDp );

                    tmp_nodesDp_lastState = NEW_N( double, num_nodes*analy->dimension,
                                                   "TmpLS DP node cache for calc node velocity" );

                    tmp_state = analy->cur_state;
                    if ( tmp_state == 0 )
                        tmp_state = 1;

                    load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                                 tmp_state, FALSE,
                                 (void *) tmp_nodesDp_lastState );

                    pos2dDp_0 = (GVec2D2P *) tmp_nodesDp_lastState;
                    pos2dDp_1 = (GVec2D2P *) tmp_nodesDp;
                }

                if ( p_result->name[3] != 'm' ) /* velmag vs. velx, vely, or velz */
                {
                    coord = (int) (p_result->name[3] - 'x');

                    for ( i = 0; i < obj_qty; i++ )
                    {
                        if (MESH_P( analy )->double_precision_nodpos && pos2dDp_0)
                            resultArr[i] = (pos2dDp_1[i][coord] - pos2dDp_0[i][coord])
                                           / delta_t;
                        else
                            resultArr[i] = (pos2d_1[i][coord] - pos2d_0[i][coord])
                                           / delta_t;
                    }
                }
                else
                {
                    for ( i = 0; i < obj_qty; i++ )
                    {
                        if (MESH_P( analy )->double_precision_nodpos && pos2dDp_0)
                        {
                            tmp[0] = pos2dDp_1[i][0] - pos2dDp_0[i][0];
                            tmp[1] = pos2dDp_1[i][1] - pos2dDp_0[i][1];
                        }
                        else
                        {
                            tmp[0] = pos2d_1[i][0] - pos2d_0[i][0];
                            tmp[1] = pos2d_1[i][1] - pos2d_0[i][1];
                        }

                        resultArr[i] = VEC_LENGTH_2D( tmp ) / delta_t;
                    }
                }
            }
            else
            {
                if (MESH_P( analy )->double_precision_nodpos)
                {
                    p_sro = analy->srec_tree + analy->state_p->srec_id;

                    tmp_nodesDp = NEW_N( double, num_nodes*analy->dimension,
                                         "Tmp DP node cache for calc node velocity" );

                    load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                                 analy->cur_state, FALSE,
                                 (void *) tmp_nodesDp );

                    tmp_nodesDp_lastState = NEW_N( double, num_nodes*analy->dimension,
                                                   "TmpLS DP node cache for calc node velocity" );

                    tmp_state = analy->cur_state;
                    if ( tmp_state == 0 )
                        tmp_state = 1;

                    load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                                 tmp_state -1, FALSE,
                                 (void *) tmp_nodesDp_lastState );

                    pos3dDp_0 = (GVec3D2P *) tmp_nodesDp_lastState;
                    pos3dDp_1 = (GVec3D2P *) tmp_nodesDp;
                }

                pos3d_0 = state_prev->nodes.nodes3d;
                pos3d_1 = analy->state_p->nodes.nodes3d;

                if ( p_result->name[3] != 'm' ) /* velmag vs. velx, vely, or velz */
                {
                    coord = (int) (p_result->name[3] - 'x');

                    for ( i = 0; i < obj_qty; i++ )
                    {
                        if (MESH_P( analy )->double_precision_nodpos && pos3dDp_0)
                            resultArr[i] = (pos3dDp_1[i][coord] - pos3dDp_0[i][coord])
                                           / delta_t;
                        else
                            resultArr[i] = (pos3d_1[i][coord] - pos3d_0[i][coord])
                                           / delta_t;
                    }
                }
                else
                {
                    for ( i = 0; i < obj_qty; i++ )
                    {

                        if (MESH_P( analy )->double_precision_nodpos && pos3dDp_0)
                        {
                            tmp[0] = pos3dDp_1[i][0] - pos3dDp_0[i][0];
                            tmp[1] = pos3dDp_1[i][1] - pos3dDp_0[i][1];
                            tmp[2] = pos3dDp_1[i][2] - pos3dDp_0[i][2];
                        }
                        else
                        {
                            tmp[0] = pos3d_1[i][0] - pos3d_0[i][0];
                            tmp[1] = pos3d_1[i][1] - pos3d_0[i][1];
                            tmp[2] = pos3d_1[i][2] - pos3d_0[i][2];
                        }

                        resultArr[i] = VEC_LENGTH( tmp ) / delta_t;
                    }
                }
            }
        }

        if ( analy->cur_state != 0 )
            fr_state2( state_prev, analy );
    }

    if (tmp_nodesDp)
        free( tmp_nodesDp );
    if (tmp_nodesDp_lastState)
        free( tmp_nodesDp_lastState );
}


/* ARGSUSED 2 */
/************************************************************
 * TAG( compute_node_acceleration )
 *
 * Computes the acceleration at the nodes using a central difference
 * (unless the accelerations are contained in the data).
 */
void
compute_node_acceleration( Analysis *analy, float *resultArr,
                           Bool_type interpolate )
{
    State2 *state_prev, *state_next;
    float delta_t, one_over_tsqr;
    float disp, dispf, dispb, disp_inc;
    int i, max_state;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int obj_qty;
    int index;
    int dim;
    int coord;
    char nbuf[16];
    char *new_primals[1];
    int *object_ids;
    Subrec_obj *p_subrec;
    float *result_buf;
    char *acomps[] = { "ax", "ay", "az" };
    GVec2D *acc2d, *pos2d_0, *pos2d_1, *pos2d_2;
    GVec3D *acc3d, *pos3d_0, *pos3d_1, *pos3d_2;

    dim = analy->dimension;
    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];

    /* Determine if result is actually primal or derived from displacements. */
    if ( strcmp( primals[0], "nodacc" ) == 0 )
    {
        /* Accelerations are stored in the data. */

        subrec = p_result->subrecs[index];
        srec = p_result->srec_id;
        p_subrec = analy->srec_tree[srec].subrecs + subrec;
        obj_qty = p_subrec->subrec.qty_objects;
        object_ids = p_subrec->object_ids;

        /*
         * If request is not for magnitude, augment primal spec with the
         * component and let I/O code extract it from the vector array in the
         * database.
         */
        if ( p_result->name[3] != 'm' ) /* accmag vs. accx, accy, or accz */
        {
            coord = (int) (p_result->name[3] - 'x');
            sprintf( nbuf, "%s[%s]", primals[0], acomps[coord] );
            new_primals[0] = nbuf;
            primals = (char **) new_primals;
            result_buf = ( object_ids == NULL ) ? resultArr
                         : analy->tmp_result[0];
        }
        else
            result_buf = analy->tmp_result[0];

        /* Read the database. */
        analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 1,
                               primals, (void *) result_buf );

        if ( p_result->name[3] == 'm' )
        {
            /* Calculate magnitude. */
            if ( dim == 2 )
            {
                acc2d = (float (*)[2]) result_buf;

                if ( object_ids == NULL )
                {
                    for ( i = 0; i < obj_qty; i++ )
                        resultArr[i] = sqrt( (double)
                                             (acc2d[i][0] * acc2d[i][0]
                                              + acc2d[i][1] * acc2d[i][1]) );
                }
                else
                {
                    for ( i = 0; i < obj_qty; i++ )
                        resultArr[object_ids[i]] = sqrt( (double)
                                                         (acc2d[i][0] * acc2d[i][0]
                                                          + acc2d[i][1] * acc2d[i][1]) );
                }
            }
            else
            {
                acc3d = (float (*)[3]) result_buf;

                if ( object_ids == NULL )
                {
                    for ( i = 0; i < obj_qty; i++ )
                        resultArr[i] = sqrt( (double)
                                             (acc3d[i][0] * acc3d[i][0]
                                              + acc3d[i][1] * acc3d[i][1]
                                              + acc3d[i][2] * acc3d[i][2]) );
                }
                else
                {
                    for ( i = 0; i < obj_qty; i++ )
                        resultArr[object_ids[i]] = sqrt( (double)
                                                         (acc3d[i][0] * acc3d[i][0]
                                                          + acc3d[i][1] * acc3d[i][1]
                                                          + acc3d[i][2] * acc3d[i][2]) );
                }
            }
        }
        else if ( object_ids != NULL )
        {
            /* Not magnitude result, but data needs reordering. */
            reorder_float_array( obj_qty, object_ids, 1, result_buf,
                                 resultArr );
        }
    }
    else
    {
        /* Calculate accelerations using differences. */

        if ( analy->cur_state == 0 )
            state_prev = NULL;
        else
            analy->db_get_state( analy, analy->cur_state - 1, NULL, &state_prev,
                                 NULL );

        max_state = get_max_state( analy );
        if ( analy->cur_state + 1 > max_state )
            state_next = NULL;
        else
            analy->db_get_state( analy, analy->cur_state + 1, NULL, &state_next,
                                 NULL );


        obj_qty = MESH_P( analy )->node_geom->qty;

        if ( dim == 2 )
        {
            pos2d_1 = analy->state_p->nodes.nodes2d;

            if ( state_prev == NULL )
            {
                /* No prev -- use forward difference. */
                delta_t = state_next->time - analy->state_p->time;
                one_over_tsqr = 1.0 / ( delta_t * delta_t );

                pos2d_2 = state_next->nodes.nodes2d;

                if ( p_result->name[3] != 'm' ) /* accmag vs acc[x,y, or z] */
                {
                    /* 2D acceleration x or y component. */

                    coord = (int) (p_result->name[3] - 'x');

                    for ( i = 0; i < obj_qty; i++ )
                    {
                        disp_inc = pos2d_1[i][coord] - pos2d_2[i][coord];
                        disp = pos2d_1[i][coord];
                        dispf = pos2d_2[i][coord];
                        resultArr[i] = (dispf - disp + disp_inc)
                                       * one_over_tsqr;
                    }
                }
                else
                {
                    /* 2D acceleration magnitude */

                    for ( i = 0; i < obj_qty; i++ )
                    {
                        disp_inc = sqrt( (double)
                                         SQR(pos2d_1[i][0] - pos2d_2[i][0])
                                         + SQR(pos2d_1[i][1] - pos2d_2[i][1]) );
                        disp = sqrt( (double) SQR(pos2d_1[i][0])
                                     + SQR(pos2d_1[i][1]) );
                        dispf = sqrt( (double) SQR(pos2d_2[i][0])
                                      + SQR(pos2d_2[i][1]) );
                        resultArr[i] = (dispf - disp + disp_inc)
                                       * one_over_tsqr;
                    }
                }
            }
            else if ( state_next == NULL )
            {
                /* No next -- use backward difference. */

                delta_t = analy->state_p->time - state_prev->time;
                one_over_tsqr = 1.0 / ( delta_t * delta_t );

                pos2d_0 = state_prev->nodes.nodes2d;

                if ( p_result->name[3] != 'm' ) /* accmag vs acc[x,y, or z] */
                {
                    /* 2D acceleration x or y component. */

                    coord = (int) (p_result->name[3] - 'x');

                    for ( i = 0; i < obj_qty; i++ )
                    {
                        disp_inc = pos2d_0[i][coord] - pos2d_1[i][coord];
                        disp = pos2d_1[i][coord];
                        dispb = pos2d_0[i][coord];
                        resultArr[i] = (disp_inc - disp + dispb)
                                       * one_over_tsqr;
                    }
                }
                else
                {
                    /* 2D acceleration magnitude */

                    for ( i = 0; i < obj_qty; i++ )
                    {
                        disp_inc = sqrt( (double)
                                         SQR(pos2d_0[i][0] - pos2d_1[i][0])
                                         + SQR(pos2d_0[i][1] - pos2d_1[i][1]) );
                        disp = sqrt( (double) SQR(pos2d_1[i][0])
                                     + SQR(pos2d_1[i][1]) );
                        dispb = sqrt( (double) SQR(pos2d_0[i][0])
                                      + SQR(pos2d_0[i][1]) );
                        resultArr[i] = (disp_inc - disp + dispb)
                                       * one_over_tsqr;
                    }
                }
            }
            else
            {
                /* Default -- use central difference. */
                delta_t = 0.5 * (state_next->time - state_prev->time);
                one_over_tsqr = 1.0 / ( delta_t * delta_t );

                pos2d_2 = state_next->nodes.nodes2d;
                pos2d_0 = state_prev->nodes.nodes2d;

                if ( p_result->name[3] != 'm' ) /* accmag vs acc[x,y, or z] */
                {
                    /* 2D acceleration x or y component. */

                    coord = (int) (p_result->name[3] - 'x');

                    for ( i = 0; i < obj_qty; i++ )
                    {
                        disp = pos2d_1[i][coord];
                        dispf = pos2d_2[i][coord];
                        dispb = pos2d_0[i][coord];
                        resultArr[i] = (dispf - 2.0*disp + dispb)
                                       * one_over_tsqr;
                    }
                }
                else
                {
                    /* 2D acceleration magnitude. */

                    for ( i = 0; i < obj_qty; i++ )
                    {
                        disp = sqrt( (double) SQR(pos2d_1[i][0])
                                     + SQR(pos2d_1[i][1]) );
                        dispf = sqrt( (double) SQR(pos2d_2[i][0])
                                      + SQR(pos2d_2[i][1]) );
                        dispb = sqrt( (double) SQR(pos2d_0[i][0])
                                      + SQR(pos2d_0[i][1]) );
                        resultArr[i] = (dispf - 2.0*disp + dispb)
                                       * one_over_tsqr;
                    }
                }
            }
        }
        else
        {
            /*
             * 3D database.
             */

            pos3d_1 = analy->state_p->nodes.nodes3d;

            if ( state_prev == NULL )
            {
                /* No prev -- use forward difference. */
                delta_t = state_next->time - analy->state_p->time;
                one_over_tsqr = 1.0 / ( delta_t * delta_t );

                pos3d_2 = state_next->nodes.nodes3d;

                if ( p_result->name[3] != 'm' ) /* accmag vs acc[x,y, or z] */
                {
                    /* 3D acceleration x, y, or z component. */

                    coord = (int) (p_result->name[3] - 'x');

                    for ( i = 0; i < obj_qty; i++ )
                    {
                        disp_inc = pos3d_1[i][coord] - pos3d_2[i][coord];
                        disp = pos3d_1[i][coord];
                        dispf = pos3d_2[i][coord];
                        resultArr[i] = (dispf - disp + disp_inc)
                                       * one_over_tsqr;
                    }
                }
                else
                {
                    /* 3D acceleration magnitude */
                    for ( i = 0; i < obj_qty; i++ )
                    {
                        disp_inc = sqrt( (double)
                                         SQR(pos3d_1[i][0] - pos3d_2[i][0])
                                         + SQR(pos3d_1[i][1] - pos3d_2[i][1])
                                         + SQR(pos3d_1[i][2] - pos3d_2[i][2]) );
                        disp = sqrt( (double) SQR(pos3d_1[i][0])
                                     + SQR(pos3d_1[i][1])
                                     + SQR(pos3d_1[i][2]) );
                        dispf = sqrt( (double) SQR(pos3d_2[i][0])
                                      + SQR(pos3d_2[i][1])
                                      + SQR(pos3d_2[i][2]) );
                        resultArr[i] = (dispf - disp + disp_inc)
                                       * one_over_tsqr;
                    }
                }
            }
            else if ( state_next == NULL )
            {
                /* No next -- use backward difference. */

                delta_t = analy->state_p->time - state_prev->time;
                one_over_tsqr = 1.0 / ( delta_t * delta_t );

                pos3d_0 = state_prev->nodes.nodes3d;

                if ( p_result->name[3] != 'm' ) /* accmag vs acc[x,y, or z] */
                {
                    /* 3D acceleration x, y, or z component. */

                    coord = (int) (p_result->name[3] - 'x');

                    for ( i = 0; i < obj_qty; i++ )
                    {
                        disp_inc = pos3d_0[i][coord] - pos3d_1[i][coord];
                        disp = pos3d_1[i][coord];
                        dispb = pos3d_0[i][coord];
                        resultArr[i] = (disp_inc - disp + dispb)
                                       * one_over_tsqr;
                    }
                }
                else
                {
                    /* 3D acceleration magnitude */

                    for ( i = 0; i < obj_qty; i++ )
                    {
                        disp_inc = sqrt( (double)
                                         SQR(pos3d_0[i][0] - pos3d_1[i][0])
                                         + SQR(pos3d_0[i][1] - pos3d_1[i][1])
                                         + SQR(pos3d_0[i][1] - pos3d_1[i][2]) );
                        disp = sqrt( (double) SQR(pos3d_1[i][0])
                                     + SQR(pos3d_1[i][1])
                                     + SQR(pos3d_1[i][2]) );
                        dispb = sqrt( (double) SQR(pos3d_0[i][0])
                                      + SQR(pos3d_0[i][1])
                                      + SQR(pos3d_0[i][2]) );
                        resultArr[i] = (disp_inc - disp + dispb)
                                       * one_over_tsqr;
                    }
                }
            }
            else
            {
                /* Default -- use central difference. */
                delta_t = 0.5 * (state_next->time - state_prev->time);
                one_over_tsqr = 1.0 / ( delta_t * delta_t );

                pos3d_2 = state_next->nodes.nodes3d;
                pos3d_0 = state_prev->nodes.nodes3d;

                if ( p_result->name[3] != 'm' ) /* accmag vs acc[x,y, or z] */
                {
                    /* 3D acceleration x, y, or z component. */

                    coord = (int) (p_result->name[3] - 'x');

                    for ( i = 0; i < obj_qty; i++ )
                    {
                        disp = pos3d_1[i][coord];
                        dispf = pos3d_2[i][coord];
                        dispb = pos3d_0[i][coord];
                        resultArr[i] = (dispf - 2.0*disp + dispb)
                                       * one_over_tsqr;
                    }
                }
                else
                {
                    /* 3D acceleration magnitude */

                    for ( i = 0; i < obj_qty; i++ )
                    {
                        disp = sqrt( (double) SQR(pos3d_1[i][0])
                                     + SQR(pos3d_1[i][1])
                                     + SQR(pos3d_1[i][2]) );
                        dispf = sqrt( (double) SQR(pos3d_2[i][0])
                                      + SQR(pos3d_2[i][1])
                                      + SQR(pos3d_2[i][2]) );
                        dispb = sqrt( (double) SQR(pos3d_0[i][0])
                                      + SQR(pos3d_0[i][1])
                                      + SQR(pos3d_0[i][2]) );
                        resultArr[i] = (dispf - 2.0*disp + dispb)
                                       * one_over_tsqr;
                    }
                }
            }
        }

        if ( state_prev != NULL )
            fr_state2( state_prev, analy );
        if ( state_next != NULL )
            fr_state2( state_next, analy );
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
compute_node_pr_intense( Analysis *analy, float *resultArr,
                         Bool_type interpolate )
{
    double psr;
    int obj_qty, i;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int index;
    int *object_ids;
    Subrec_obj *p_subrec;
    float *result_buf;

    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    obj_qty = p_subrec->subrec.qty_objects;
    object_ids = p_subrec->object_ids;

    result_buf = ( object_ids == NULL ) ? resultArr : analy->tmp_result[0];

    /* Read the database. */
    analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 1,
                           primals, (void *) result_buf );

    if ( object_ids == NULL )
    {
        for ( i = 0; i < obj_qty; i++ )
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
    else
    {
        for ( i = 0; i < obj_qty; i++ )
        {
            if ( result_buf[i] == 0.0 )
                resultArr[object_ids[i]] = -INFINITY;
            else
            {
                psr = result_buf[i] / ping_pr_ref;
                resultArr[object_ids[i]] = 10.0 * log10( psr * psr );
            }
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
set_pr_ref( float pr_ref )
{
    ping_pr_ref = pr_ref;
}


/*
   This demonstrates new conditions which must be dealt with under MGriz.

   First, it requires a result which may be derived or primal.  The initial
   set_results logic only allows for dependencies upon primal results,
   meaning this calculation can only occur if nodal velocities are primal.
   In fact, this calculation shouldn't care how velocities are
   arrived at and show should be enabled even if nodal velocities are
   not primal but are derived from displacements.

   Second, when/if it calls a result which is itself derived (although in the
   implementation shown below we only allow for a primal acceleration
   result), and both use analy->tmp_results, and in particular if this
   calculation calls multiple derivable results, intermediate results
   stored in analy->tmp_results could be overwritten, corrupting the
   calculation.  Solution - implement tmp_results management as a
   specialized memory manager which ensures a buffer can only be assigned
   once and performs allocation if required.

   Note this calculation also requires a primal result (vorticity)
   which is "silently" being written by the analysis code into the hard-coded
   acceleration entry of the Taurus db.  Under MGriz, vorticity will be
   assumed to exist as the vector variable "nodvort".
*/

/************************************************************
 * TAG( compute_node_helicity )
 *
 * Computes helicity at the nodes.  This 3D-only scalar is the
 * dot-product of the vorticity and the velocity.
 *
 * Assumes vector primals for both velocity and vorticity.
 */
void
compute_node_helicity( Analysis *analy, float *resultArr,
                       Bool_type interpolate )
{
    int obj_qty, i;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int index;
    int *object_ids;
    Subrec_obj *p_subrec;
    float (*vel)[3], (*vort)[3];

    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    obj_qty = p_subrec->subrec.qty_objects;
    object_ids = p_subrec->object_ids;

    vel = (float (*)[3]) analy->tmp_result[0];
    vort = vel + obj_qty;

    /* Read the database. */
    analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 2,
                           primals, (void *) analy->tmp_result[0] );

    /* Calculate helicity. */
    if ( object_ids == NULL )
    {
        for ( i = 0; i < obj_qty; i++ )
            resultArr[i] = VEC_DOT( vel[i], vort[i] );
    }
    else
    {
        for ( i = 0; i < obj_qty; i++ )
            resultArr[object_ids[i]] = VEC_DOT( vel[i], vort[i] );
    }
}


/************************************************************
 * TAG( compute_node_enstrophy )
 *
 * Computes enstrophy at the nodes.  This 3D-only scalar is
 * one-half the square of the vorticity.
 */
void
compute_node_enstrophy( Analysis *analy, float *resultArr,
                        Bool_type interpolate )
{
    int obj_qty, i;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int index;
    int *object_ids;
    Subrec_obj *p_subrec;
    float (*vort)[3];

    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    obj_qty = p_subrec->subrec.qty_objects;
    object_ids = p_subrec->object_ids;

    vort = (float (*)[3]) analy->tmp_result[0];

    /* Read the database. */
    analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 1,
                           primals, (void *) vort );

    /* Calculate enstrophy. */
    if ( object_ids == NULL )
    {
        for ( i = 0; i < obj_qty; i++ )
            resultArr[i] = VEC_DOT( vort[i], vort[i] ) / 2.0;
    }
    else
    {
        for ( i = 0; i < obj_qty; i++ )
            resultArr[object_ids[i]] = VEC_DOT( vort[i], vort[i] ) / 2.0;;
    }
}


/************************************************************
 * TAG( check_compute_vector_component )
 *
 * Verifies that conditions necessary for
 * compute_vector_component() are set appropriately.
 */
Bool_type
check_compute_vector_component( Analysis *analy )
{
    int i;
    Bool_type need_vector, need_direction;
    static Bool_type warn_once_mm_source = TRUE;

    /* Verify that a vector result is defined. */
    for ( i = 0; i < analy->dimension; i++ )
        if ( analy->vector_result[i] != NULL )
            break;

    need_vector = ( i == analy->dimension );

    /* Verify that a direction vector is defined. */
    for ( i = 0; i < analy->dimension; i++ )
        if ( analy->dir_vec[i] != 0.0 )
            break;

    need_direction = ( i == analy->dimension );

    if ( need_vector || need_direction )
    {
        popup_dialog( INFO_POPUP, "%s\n%s\n%s\n%s",
                      "Result \"Projected Vector Magnitude\" (pvmag)",
                      "requires both a vector quantity (\"vec <x> <y> [<z>]\")",
                      "and a direction (\"dir3n\", \"dir3p\", or \"dirv\").",
                      "to be defined." );
        return FALSE;
    }

    /* Check and warn about min/max source. */
    if ( analy->use_global_mm && warn_once_mm_source )
    {
        popup_dialog( WARNING_POPUP, "%s\n%s\n%s\n%s\n\n%s",
                      "Because min/max caching is not sensitive to",
                      "changes in either the current direction vector",
                      "or vector result, \"switch mstat\" should be",
                      "in effect for the result \"pvmag\".",
                      "This notice will not repeat." );
        warn_once_mm_source = FALSE;
    }

    return TRUE;
}


/************************************************************
 * TAG( compute_vector_component )
 *
 * Computes the length of the component of the currently
 * defined vector result in the specified direction at each
 * node.
 */
void
compute_vector_component( Analysis *analy, float *resultArr,
                          Bool_type interpolate )
{
    int obj_qty, node_qty, i, j;
    Result *p_result, *p_temp_res;
    int subrec, srec;
    int index, dim, node_idx;
    int *object_ids, *node_ids;
    Subrec_obj *p_subrec;
    float *result_buf;
    int *vec_subrecs;
    int vec_subrec_qty;
    float *vec_comp[3];
    Bool_type direct;
    float vec[3];
    MO_class_data *p_node_class;

    p_result = analy->cur_result;
    index = analy->result_index;
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    obj_qty = p_subrec->subrec.qty_objects;
    object_ids = p_subrec->object_ids;
    dim = analy->dimension;
    p_node_class = MESH_P( analy )->node_geom;

    /*
     * Check the vector availability map to see if the defined vector
     * is available on the current subrecord.
     */
    vec_subrecs = (int *) analy->vector_srec_map[srec].list;
    vec_subrec_qty = analy->vector_srec_map[srec].qty;
    for ( i = 0; i < vec_subrec_qty; i++ )
        if ( vec_subrecs[i] == subrec )
            break;

    if ( i == vec_subrec_qty )
        return;

    /* Verify that this result can be computed (need node or elem data). */
    direct = TRUE;
    node_qty = p_node_class->qty;
    if ( p_result->origin.is_node_result )
    {
        if ( object_ids != NULL )
        {
            /*
             * Non-proper nodal subrecord - just use object_ids as
             * referenced nodes list.
             */
            direct = FALSE;
            node_ids = p_subrec->object_ids;
            node_qty = obj_qty;
        }
    }
    else if ( p_subrec->referenced_nodes != NULL )
    {
        direct = FALSE;
        node_ids = p_subrec->referenced_nodes;
        node_qty = p_subrec->ref_node_qty;
    }

    /* Save current result and data array prior to loading components. */
    result_buf = p_node_class->data_buffer;
    p_temp_res = analy->cur_result;

    /**/
    /*
     * Allocate data arrays and load vector component arrays.  Need results
     * interpolated back to nodes, so these must be Nodal-class-sized
     * arrays.
     *
     * This is a good reason to implement memory management of
     * analy->tmp_result buffers to minimize allocations.
     */
    for ( i = 0; i < dim; i++ )
    {
        vec_comp[i] = NEW_N( float, node_qty, "Vector component array" );
        p_node_class->data_buffer = vec_comp[i];
        analy->cur_result = analy->vector_result[i];

        load_subrecord_result( analy, subrec, FALSE, TRUE );
    }

    p_node_class->data_buffer = result_buf;
    analy->cur_result = p_temp_res;

    /*
     * Here at last is the actual measly calculation.
     *
     * Dot each nodal vector with the direction unit vector to
     * generate the magnitude of its component in that direction.
     */
    for ( i = 0; i < node_qty; i++ )
    {
        node_idx = direct ? i : node_ids[i];

        for ( j = 0; j < dim; j++ )
            vec[j] = vec_comp[j][node_idx];

        resultArr[node_idx] = ( dim == 3 )
                              ? VEC_DOT( vec, analy->dir_vec )
                              : VEC_DOT_2D( vec, analy->dir_vec );
    }

    for ( i = 0; i < dim; i++ )
        free( vec_comp[i] );

    analy->cur_result->must_recompute = TRUE;
}


/*****************************************************************
 * TAG( get_class_nodes )
 *
 * This function will return the list of nodes for all elements
 * in a specified class in connectivity order with duplicates if
 * for elements that share nodes.
 */
void
get_class_nodes ( int superclass, Mesh_data *p_mesh,
                  int *num_nodes, int *node_list )
{
    MO_class_data **mo_classes=NULL;
    MO_class_data *p_node_geom=NULL, *p_class=NULL;

    int i=0, j=0, k=0;
    int nd, elem_id=0, qty_classes=0, qty_elems=0, qty_nodes=0, node_index=0;
    int num_connects=0;
    int *all_nodes=0;

    num_connects = qty_connects[superclass];
    int (*connects)[num_connects];

    p_node_geom = p_mesh->node_geom;
    qty_nodes   = p_node_geom->qty;
    qty_classes = p_mesh->classes_by_sclass[superclass].qty;
    mo_classes  = (MO_class_data **) p_mesh->classes_by_sclass[superclass].list;
    connects    = (int (*) [num_connects]) p_class->objects.elems->nodes;

    /* First count the number of nodes */

    for ( i = 0;
            i < qty_classes;
            i++ )
    {
        p_class   =  mo_classes[i];
        qty_elems = p_class->qty;
        qty_nodes += qty_elems*num_connects;
    }


    all_nodes  = NEW_N( int, qty_nodes, "Node list for a class" );
    *num_nodes = qty_nodes;

    if ( *num_nodes == 0 )
    {
        *all_nodes = 0;
        return;
    }

    for ( i = 0;
            i < qty_classes;
            i++ )
    {
        p_class   =  mo_classes[i];
        qty_elems = p_class->qty;
        connects  = (int (*) [num_connects]) p_class->objects.elems->nodes;

        for ( j = 0;
                j < qty_elems;
                j++ )
        {

            elem_id = j;
            for ( k = 0;
                    k < num_connects;
                    k++ )
            {
                nd = connects[elem_id][k];
                all_nodes[node_index++] = nd;
            }
        }
    }
}
