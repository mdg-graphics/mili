/* $Id$ */
/*
 * stress.c - Routines for computing derived stress quantities.
 *
 *      Thomas Spelce
 *      Lawrence Livermore National Laboratory
 *      June 29 1992
 *
 ************************************************************************
 *
 * Modifications:
 *
 *  I. R. Corey - Feb 12, 2009: Modified limits check for calculating
 *                               invariant for the principle stresses.
 *                See SRC# 565.
 *
 ************************************************************************
 *
 */


#include "viewer.h"

#define STRESS_LEN 6
#define ONETHIRD .333333333333333333333333333333333333333
#define ONEHALF  .5


/************************************************************
 * TAG( compute_hex_stress )
 *
 * Computes the stress at nodes for hex elements.
 */
void
compute_hex_stress( Analysis *analy, float *resultArr, Bool_type interpolate )
{
    Ref_frame_type ref_frame;
    float *resultElem;
    float localMat[3][3];
    float (*sigma)[6];
    int idx, i;
    float *result_buf;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int obj_qty;
    int index;
    char primal_spec[32];
    char *primal_list[1];
    MO_class_data *p_mo_class;
    Subrec_obj *p_subrec;
    int elem_idx;
    int *object_ids;

    p_result = analy->cur_result;
    index = analy->result_index;
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    p_mo_class = p_subrec->p_object_class;
    primals = p_result->primals[index];
    obj_qty = p_subrec->subrec.qty_objects;
    object_ids = p_subrec->object_ids;
    resultElem = p_subrec->p_object_class->data_buffer;

    /*
     * Hack to get an index on [0,5] from name,
     * which is one of (in order): sx sy sz sxy syz szx.
     */
    idx = (int) p_result->name[1] - (int) 'x' + (( p_result->name[2] ) ? 3 : 0);

    /*
     * The result_id is SIGX, SIGY, SIGZ, SIGXY, SIGYZ, or SIGZX.
     * These can be loaded directly from the database.
     */

    ref_frame = analy->ref_frame;

    /*
     * If re-ordering or frame transformation is necessary, read into
     * a temp result buffer.
     */
    result_buf = ( ref_frame == LOCAL || object_ids )
                 ? analy->tmp_result[0] : resultElem;

    if ( ref_frame == GLOBAL )
    {
        if ( analy->do_tensor_transform )
        {
            if ( !transform_stress_strain( p_result->primals[index], 0, analy,
                                           analy->tensor_transform_matrix,
                                           result_buf ) )
            {
                popup_dialog( WARNING_POPUP,
                              "Stress/strain coord transformation failed." );
                return;
            }

            p_result->modifiers.use_flags.coord_transform = 1;
        }
        else
        {
            /* Build up component specification of vector result. */
            strcpy( primal_spec, p_result->primals[index][0] );
            sprintf( primal_spec + strlen( primal_spec ), "[%s]",
                     p_result->name );
            primal_list[0] = primal_spec;

            /* Read the database. */
            analy->db_get_results( analy->db_ident, analy->cur_state + 1,
                                   subrec, 1, primal_list,
                                   (void *) result_buf );
        }

        /* Re-order data if subrecord is non-proper. */
        if ( object_ids )
        {
            for ( i = 0; i < obj_qty; i++ )
                resultElem[object_ids[i]] = result_buf[i];
        }
    }
    else if ( ref_frame == LOCAL )
    {
        /* Need to transform global quantities to local quantities.
         * The full tensor must be transformed.
         */

        /* Load the tensors for the elements. */
        analy->db_get_results( analy->db_ident, analy->cur_state + 1,
                               subrec, 1, p_result->primals[index],
                               (void *)result_buf );

        sigma = (float (*)[6]) result_buf;

        if ( object_ids )
        {
            for ( i = 0; i < obj_qty; i++ )
            {
                elem_idx = object_ids[i];
                hex_g2l_mtx( analy, p_mo_class, elem_idx, 0, 1, 3, localMat );
                transform_tensors_1p( 1, sigma + i, localMat );
                resultElem[elem_idx] = sigma[i][idx];
            }
        }
        else
        {
            for ( i = 0; i < obj_qty; i++ )
            {
                hex_g2l_mtx( analy, p_mo_class, i, 0, 1, 3, localMat );
                transform_tensors_1p( 1, sigma + i, localMat );
                resultElem[i] = sigma[i][idx];
            }
        }
    }

    /* Update modifiers to indicate pertinent ones for this result. */
    p_result->modifiers.use_flags.use_ref_frame = 1;
    p_result->modifiers.ref_frame = analy->ref_frame;

    if ( interpolate )
        hex_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                      object_ids, analy );
}

/************************************************************
 * TAG( compute_press )
 *
 * Determine which function should be used to compute the
 * pressure and call it.
 */
void compute_press( Analysis *analy, float *resultArr, Bool_type interpolate )
{
    Result* p_result;
    int index;
    int srec, subrec;
    Subrec_obj* p_subrec;

    p_result = analy->cur_result;
    index = analy->result_index;
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;

    if( p_subrec->element_set != NULL ){
        compute_es_press(analy, resultArr, interpolate);
    }
    else{
        compute_hex_press(analy, resultArr, interpolate);
    }

}


/************************************************************
 * TAG( compute_hex_press )
 *
 * Computes the pressure at nodes given the current stress
 * tensor for an element.
 *
 * Requires a primal vector result "stress[sx, sy, sz, sxy, syz, szx]",
 * but only uses the first three components.
 */
void
compute_hex_press( Analysis *analy, float *resultArr, Bool_type interpolate )
{
    float *resultElem;
    float (*stress)[6];
    int i;
    float *result_buf;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int obj_qty;
    int index;
    int *object_ids;
    Subrec_obj *p_subrec;
    MO_class_data* p_mo_class;

    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    object_ids = p_subrec->object_ids;
    obj_qty = p_subrec->subrec.qty_objects;
    p_mo_class = p_subrec->p_object_class;
    resultElem = p_subrec->p_object_class->data_buffer;

    /* Just use analy->tmp_result as an extra long buffer. */
    result_buf = analy->tmp_result[0];

    /* Read the database. */
    analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 1,
                           primals, (void *) result_buf );

    /* Compute pressure. */
    stress = (float(*)[6]) result_buf;

    if ( object_ids )
        for ( i = 0; i < obj_qty; i++ )
            resultElem[object_ids[i]] = -(stress[i][0] + stress[i][1] + stress[i][2]) * ONETHIRD;
    else
        for ( i = 0; i < obj_qty; i++ )
            resultElem[i] = -(stress[i][0] + stress[i][1] + stress[i][2]) * ONETHIRD;

    if ( interpolate ){
        switch( p_mo_class->superclass)
        {
        case G_HEX:
            hex_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                        object_ids, analy );
            break;
        case G_QUAD:
            quad_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy, TRUE );
            break;
        case G_TRI:
            tri_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                          object_ids, analy, TRUE );
            break;
        case G_BEAM:
            beam_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy );
            break;
        case G_TRUSS:
            truss_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                            object_ids, analy );
            break;
        case G_TET:
            tet_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                            object_ids, analy );
            break;
        case G_PARTICLE:
            particle_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                               object_ids, analy );
            break;
        }
    }
}

/************************************************************
 * TAG( compute_effstress )
 *
 * Determine which function should be used to compute the
 * Effective stress and call it.
 */
void compute_effstress( Analysis *analy, float *resultArr, Bool_type interpolate )
{
    Result* p_result;
    int index;
    int srec, subrec;
    Subrec_obj* p_subrec;

    p_result = analy->cur_result;
    index = analy->result_index;
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;

    if( p_subrec->element_set != NULL ){
        compute_es_effstress(analy, resultArr, interpolate);
    }
    else{
        compute_hex_effstress(analy, resultArr, interpolate);
    }
}


/************************************************************
 * TAG( compute_hex_effstress )
 *
 * Computes the effective stress at nodes.
 */
void
compute_hex_effstress( Analysis *analy, float *resultArr, Bool_type interpolate )
{
    float *resultElem;
    float *stress;
    float devStress[3];
    float pressure;
    int obj_qty;
    int i,j;
    float *result_buf;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int index, elem_idx;
    int *object_ids;
    Subrec_obj *p_subrec;
    MO_class_data* p_mo_class;

    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    object_ids = p_subrec->object_ids;
    obj_qty = p_subrec->subrec.qty_objects;
    p_mo_class = p_subrec->p_object_class;
    resultElem = p_subrec->p_object_class->data_buffer;

    /* Just use analy->tmp_result as an extra long buffer. */
    result_buf = analy->tmp_result[0];

    /* Read the database. */
    analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 1,
                           primals, (void *) result_buf );

    for ( i = 0, stress = result_buf; i < obj_qty; i++, stress += 6 )
    {
        pressure = -( stress[0] +
                      stress[1] +
                      stress[2] ) * ONETHIRD;

        for ( j = 0; j < 3; j++ )
            devStress[j] = stress[j] + pressure;

        elem_idx = ( object_ids ) ? object_ids[i] : i;

        /*
         * Calculate effective stress from deviatoric components.
         * Updated derivation avoids negative square root operand
         * (UNICOS, of course).
         */
        resultElem[elem_idx] = 0.5 * ( devStress[0]*devStress[0]
                                       + devStress[1]*devStress[1]
                                       + devStress[2]*devStress[2] )
                               + stress[3]*stress[3]
                               + stress[4]*stress[4]
                               + stress[5]*stress[5] ;

        resultElem[elem_idx] = sqrtf(3.0 * resultElem[elem_idx]);
    }

    if ( interpolate ){
        switch( p_mo_class->superclass)
        {
        case G_HEX:
            hex_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                        object_ids, analy );
            break;
        case G_QUAD:
            quad_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy, TRUE );
            break;
        case G_TRI:
            tri_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                          object_ids, analy, TRUE );
            break;
        case G_BEAM:
            beam_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy );
            break;
        case G_TRUSS:
            truss_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                            object_ids, analy );
            break;
        case G_TET:
            tet_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                            object_ids, analy );
            break;
        case G_PARTICLE:
            particle_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                               object_ids, analy );
            break;
        }
    }
}


/************************************************************
 * TAG( compute_principal_stress )
 *
 * Determine which function should be used to compute the
 * Principal stress and call it.
 */
void compute_principal_stress( Analysis *analy, float *resultArr, Bool_type interpolate )
{
    Result* p_result;
    int index;
    int srec, subrec;
    Subrec_obj* p_subrec;

    p_result = analy->cur_result;
    index = analy->result_index;
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;

    if( p_subrec->element_set != NULL ){
        compute_es_principal_stress(analy, resultArr, interpolate);
    }
    else{
        compute_hex_principal_stress(analy, resultArr, interpolate);
    }
}


/************************************************************
 * TAG( compute_hex_principal_stress )
 *
 * Computes the principal deviatoric stresses and principal
 * stresses.
 */
void
compute_hex_principal_stress( Analysis *analy, float *resultArr, Bool_type interpolate )
{
    float *resultElem;               /* Element results vector. */
    float  *hexStressFlt;            /* Ptr to element stresses. */

    double hexStress[6];
    double devStress[3];             /* Deviatoric stresses,
                                        only need diagonal terms. */
    double Invariant[3];             /* Invariants of tensor. */
    float  princStress[3];           /* Principal values. */
    double pressure;
    float alpha, angle, value;
    int i, j;
    float *result_buf;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int obj_qty;
    int index, res_index, elem_idx;
    int *object_ids;
    Subrec_obj *p_subrec;
    MO_class_data* p_mo_class;

    double limit_check=0.0;


    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    object_ids = p_subrec->object_ids;
    obj_qty = p_subrec->subrec.qty_objects;
    p_mo_class = p_subrec->p_object_class;
    resultElem = p_subrec->p_object_class->data_buffer;

    if ( strcmp( p_result->name, "pdev1" ) == 0 )
        res_index = 0;
    else if ( strcmp( p_result->name, "pdev2" ) == 0 )
        res_index = 1;
    else if ( strcmp( p_result->name, "pdev3" ) == 0 )
        res_index = 2;
    else if ( strcmp( p_result->name, "maxshr" ) == 0 )
        res_index = 3;
    else if ( strcmp( p_result->name, "prin1" ) == 0 )
        res_index = 4;
    else if ( strcmp( p_result->name, "prin2" ) == 0 )
        res_index = 5;
    else if ( strcmp( p_result->name, "prin3" ) == 0 )
        res_index = 6;

    /* Just use analy->tmp_result[0] as an extra long buffer. */
    result_buf = analy->tmp_result[0];

    /* Read the database. */
    analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 1,
                           primals, (void *) result_buf );

    /* Calculate deviatoric stresses. */
    for ( i = 0, hexStressFlt = result_buf; i < obj_qty; i++, hexStressFlt += 6 )
    {
        for ( j = 0; j < 6; j++ )
            hexStress[j] = hexStressFlt[j];

        /* Calculate pressure. */
        pressure =  - ( hexStress[0] +
                        hexStress[1] +
                        hexStress[2] ) * ONETHIRD;

        devStress[0] = hexStress[0] + pressure;
        devStress[1] = hexStress[1] + pressure;
        devStress[2] = hexStress[2] + pressure;

        /* Calculate invariants of deviatoric tensor.
         * Invariant[0] = 0.0 */
        Invariant[0] = devStress[0] + devStress[1] + devStress[2];
        Invariant[1] = 0.5 * ( devStress[0] * devStress[0]
                               + devStress[1] * devStress[1]
                               + devStress[2] * devStress[2] )
                       + hexStress[3] * hexStress[3]
                       + hexStress[4] * hexStress[4]
                       + hexStress[5] * hexStress[5];
        Invariant[2] = -devStress[0] * devStress[1] * devStress[2]
                       - 2.0 * hexStress[3] * hexStress[4]
                       * hexStress[5]
                       + devStress[0] * hexStress[4] * hexStress[4]
                       + devStress[1] * hexStress[5] * hexStress[5]
                       + devStress[2] * hexStress[3] * hexStress[3];


        /* Check to see if we can have non-zero divisor, if not
         * set principal stress to 0. */
        if (Invariant[1]>0.0)
            limit_check = fabs(Invariant[2]/Invariant[1]);
        else
            limit_check = 0.0;

        if ( limit_check >= 1e-12 )
        {
            alpha = -0.5*sqrt( (double)27.0/Invariant[1])*
                    Invariant[2]/Invariant[1];
            if ( alpha < 0 )
                alpha = MAX( alpha, -1.0 );
            else if ( alpha > 0 )
                alpha = MIN( alpha, 1.0 );
            angle = acos((double)alpha) * ONETHIRD;
            value = 2.0 * sqrt( (double)Invariant[1] * ONETHIRD);
            princStress[0] = value*cos((double)angle);
            angle = angle - 2.0*PI * ONETHIRD ;
            princStress[1] = value*cos((double)angle);
            angle = angle + 4.0*PI * ONETHIRD;
            princStress[2] = value*cos((double)angle);
        }
        else
        {
            princStress[0] = 0.0;
            princStress[1] = 0.0;
            princStress[2] = 0.0;
        }

        elem_idx = ( object_ids ) ? object_ids[i] : i;

        switch ( res_index )
        {
        case 0:
            resultElem[elem_idx] = princStress[0];
            break;
        case 1:
            resultElem[elem_idx] = princStress[1];
            break;
        case 2:
            resultElem[elem_idx] = princStress[2];
            break;
        case 3:
            resultElem[elem_idx] = (princStress[0] - princStress[2])  * ONEHALF;
            break;
        case 4:
            resultElem[elem_idx] = princStress[0] - pressure;
            break;
        case 5:
            resultElem[elem_idx] = princStress[1] - pressure;
            break;
        case 6:
            resultElem[elem_idx] = princStress[2] - pressure;
            break;
        default:
            popup_dialog( WARNING_POPUP,
                          "Principal stress result index is invalid." );
        }
    }

    if ( interpolate ){
        switch( p_mo_class->superclass)
        {
        case G_HEX:
            hex_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                        object_ids, analy );
            break;
        case G_QUAD:
            quad_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy, TRUE );
            break;
        case G_TRI:
            tri_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                          object_ids, analy, TRUE );
            break;
        case G_BEAM:
            beam_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy );
            break;
        case G_TRUSS:
            truss_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                            object_ids, analy );
            break;
        case G_TET:
            tet_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                            object_ids, analy );
            break;
        case G_PARTICLE:
            particle_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                               object_ids, analy );
            break;
        }
    }
}


/************************************************************
 * TAG( compute_shell_stress )
 *
 * Computes the shell stress given the INNER/MIDDLE/OUTER flag
 * and the GLOBAL/LOCAL flag.  Defaults: MIDDLE, GLOBAL.
 */
void
compute_shell_stress( Analysis *analy, float *resultArr, Bool_type interpolate )
{
    Ref_surf_type ref_surf;
    Ref_frame_type ref_frame;
    float *resultElem;
    float localMat[3][3];
    float (*sigma)[6];
    int idx, i;
    float *result_buf;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int obj_qty;
    int index;
    char primal_spec[32];
    char *primal_list[1];
    MO_class_data *p_mo_class;
    Subrec_obj *p_subrec;
    int elem_idx;
    int *object_ids;

    Bool_type map_timehist_coords = FALSE;
    GVec3D2P      *new_nodes      = NULL;
    Bool_type es_result = FALSE;

    p_result = analy->cur_result;
    index = analy->result_index;
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    p_mo_class = p_subrec->p_object_class;
    primals = p_result->primals[index];
    obj_qty = p_subrec->subrec.qty_objects;
    object_ids = p_subrec->object_ids;
    resultElem = p_subrec->p_object_class->data_buffer;

    /*
     * Hack to get an index on [0,5] from name,
     * which is one of (in order): sx sy sz sxy syz szx.
     */
    idx = (int) p_result->name[1] - (int) 'x' + (( p_result->name[2] ) ? 3 : 0);

    /*
     * The result_id is SIGX, SIGY, SIGZ, SIGXY, SIGYZ, or SIGZX.
     * These can be loaded directly from the database.
     */

    ref_surf = analy->ref_surf;
    ref_frame = analy->ref_frame;

    /* Process this as an element set result if found */

    /*
        * Don't want to read in all the primals from the Result_candidate for
        * this result, so build up a new primals array with just the right one.
        */
    switch ( ref_surf )
    {
    case MIDDLE:
        strcpy( primal_spec, primals[1] );
        break;
    case INNER:
        strcpy( primal_spec, primals[0] );
        break;
    case OUTER:
        strcpy( primal_spec, primals[2] );
        break;
    }

    primal_list[0] = primal_spec;

    /* Update the result to record modifier(s) that affected the calculation. */
    p_result->modifiers.use_flags.use_ref_surface = 1;
    p_result->modifiers.ref_surf = ref_surf;

    /*
     * If re-ordering or frame transformation is necessary, read into
     * a temp result buffer.
     */
    result_buf = ( ref_frame == LOCAL || object_ids )
                 ? analy->tmp_result[0] : resultElem;

    if ( ref_frame == GLOBAL )
    {
        if ( analy->do_tensor_transform )
        {
            if ( !transform_stress_strain( primal_list, 0, analy,
                                           analy->tensor_transform_matrix,
                                           result_buf ) )
            {
                popup_dialog( WARNING_POPUP,
                              "Stress/strain coordinate transform failed." );
                return;
            }

            p_result->modifiers.use_flags.coord_transform = 1;
        }
        else
        {
            /* Augment primal vector name with component name. */
            if ( !es_result )
                sprintf( primal_spec + strlen( primal_spec ), "[%s]",
                         p_result->name );

            /* Read the database. */
            analy->db_get_results( analy->db_ident, analy->cur_state + 1,
                                   subrec, 1, primal_list,
                                   (void *) result_buf );
        }

        /* Re-order results if necessary. */
        if ( object_ids )
        {
            for ( i = 0; i < obj_qty; i++ )
                resultElem[object_ids[i]] = result_buf[i];
        }
    }
    else if ( ref_frame == LOCAL )
    {
        /* Need to transform global quantities to local quantities.
         * The full tensor must be transformed.
         */

        analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 1,
                               primal_list, (void *) result_buf );

        sigma = (float (*)[6]) result_buf;

        if ( object_ids )
        {
            for ( i = 0; i < obj_qty; i++ )
            {
                elem_idx = object_ids[i];
                if(p_mo_class->superclass == G_QUAD)
                {
                    global_to_local_mtx( analy, p_mo_class, elem_idx,
                                         map_timehist_coords, new_nodes,
                                         localMat );
                }
                else if(p_mo_class->superclass == G_TRI)
                {

                    global_to_local_tri_mtx( analy, p_mo_class, elem_idx,
                                             map_timehist_coords, new_nodes,
                                             localMat );
                }

                /*
                 * Corrects a bug in pre-merge MGriz which used elem_idx to
                 * index sigma array.
                 */
                transform_tensors_1p( 1, sigma + i, localMat );
                resultElem[elem_idx] = sigma[i][idx];
            }
        }
        else
        {
            for ( i = 0; i < obj_qty; i++ )
            {
                if(p_mo_class->superclass == G_QUAD)
                {
                    global_to_local_mtx( analy, p_mo_class, i,
                                         map_timehist_coords, new_nodes,
                                         localMat );
                }
                else if(p_mo_class->superclass == G_TRI)
                {

                    global_to_local_tri_mtx( analy, p_mo_class, i,
                                             map_timehist_coords, new_nodes,
                                             localMat );

                }
                transform_tensors_1p( 1, sigma + i, localMat );
                resultElem[i] = sigma[i][idx];
            }
        }
    }

    /* Update modifiers to indicate pertinent ones for this result. */
    p_result->modifiers.use_flags.use_ref_frame = 1;
    p_result->modifiers.ref_frame = analy->ref_frame;
    p_result->modifiers.use_flags.use_ref_surface = 1;
    p_result->modifiers.ref_surf = analy->ref_surf;

    if ( interpolate )
        if(p_mo_class->superclass == G_QUAD)
        {
            quad_to_nodal( resultElem, resultArr, p_mo_class, obj_qty, object_ids,
                           analy, TRUE );
        }
        else if(p_mo_class->superclass == G_TRI)
        {
            tri_to_nodal( resultElem, resultArr, p_mo_class, obj_qty, object_ids,
                          analy, TRUE );
        }
}

/************************************************************
 * TAG( compute_es_press )
 *
 * Computes the pressure at nodes.

*************************************************************/
void
compute_es_press( Analysis *analy, float *resultArr, Bool_type interpolate)
{
    float *resultElem;
    int i, rval;
    int ipt_index;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int obj_qty;
    int index;
    int *object_ids;
    Subrec_obj *p_subrec;
    MO_class_data* p_mo_class;
    Primal_result* primal_result;
    Htable_entry* p_hte;
    char ipt[5];
    char es_name[32];
    char es_primal[64];
    char** es_primals;
    char* stress_components[3] = {"sx", "sy", "sz"};
    float *(stresses)[3];

    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    object_ids = p_subrec->object_ids;
    obj_qty = p_subrec->subrec.qty_objects;
    resultElem = p_subrec->p_object_class->data_buffer;
    p_mo_class = p_subrec->p_object_class;
 
    /* allocate memory for the result_buf arrays to hold the three primals 
     * necessary to calculate pressure */
    for(i = 0; i < 3; i++)
    {
        stresses[i] = calloc(obj_qty, sizeof(float));
        if(stresses[i] == NULL)
        {
            popup_dialog(WARNING_POPUP, "Out of memory in function compute_es_press, exiting\n");
            parse_command("quit", analy);
        }
    }

    es_primals = (char **) malloc(2*sizeof(char *));
    if(es_primals == NULL)
    {
        popup_dialog(WARNING_POPUP, "Out of memory in function compute_es_pressure. Exiting\n");
        parse_command("quit", analy);
    }
    es_primals[0] = (char *) malloc(32*sizeof(char));
    if(es_primals[0] == NULL)
    {
        popup_dialog(WARNING_POPUP, "Out of memory in function compute_es_pressure. Exiting\n");
        parse_command("quit", analy);
    }
   
    // Look up element set and gather stress components
    rval = htable_search(analy->primal_results, primals[0], FIND_ENTRY, &p_hte);
    if(rval == OK){
        primal_result = (Primal_result*) p_hte->data;

        // Match primal result subrec to results subrecord
        // and get the element set name.
        for( i = 0; i < primal_result->qty_subrecs; i++ ){
            if( primal_result->subrecs[i] == p_subrec){
                strcpy(es_name, primal_result->original_names_per_subrec[i]);
                break;
            }
        }

        // Gather all the stress components.
        ipt_index = p_subrec->element_set->current_index + 1;
        for( i = 0; i < 3; i++ ){
            strcpy(es_primal, es_name);
            strcat(es_primal, "[");
            //stress componenet name here
            strcat(es_primal, stress_components[i] );
            strcat(es_primal, ",");
            sprintf(ipt, "%d", ipt_index);
            strcat(es_primal, ipt);
            strcat(es_primal, "]");
            es_primals[0] = es_primal;
            es_primals[1] = NULL;
            analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 1,
                                   es_primals, (void *) stresses[i] );
        }
    }

    /* all three result buffers should now be populated with the raw data needed to calculate pressure
     * compute pressure */
    if(object_ids)
    {
        for(i = 0; i < obj_qty; i++)
        {
            resultElem[object_ids[i]] = -( stresses[0][i]
                                         + stresses[1][i]
                                         + stresses[2][i] ) * ONETHIRD;
        }
    } else
    {
        for(i = 0; i < obj_qty; i++)
        {
            resultElem[i] = -( stresses[0][i]
                             + stresses[1][i]
                             + stresses[2][i] ) * ONETHIRD;
        }
    }

    if ( interpolate ){
        switch( p_mo_class->superclass)
        {
        case G_HEX:
            hex_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                        object_ids, analy );
            break;
        case G_QUAD:
            quad_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy, TRUE );
            break;
        case G_TRI:
            tri_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                          object_ids, analy, TRUE );
            break;
        case G_BEAM:
            beam_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy );
            break;
        case G_TRUSS:
            truss_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                            object_ids, analy );
            break;
        case G_TET:
            tet_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                            object_ids, analy );
            break;
        case G_PARTICLE:
            particle_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                               object_ids, analy );
            break;
        }
    }
}

/*******************************
* TAG( compute_shell_press )
 *
 * Computes the pressure at nodes.
 */
void
compute_shell_press( Analysis *analy, float *resultArr, Bool_type interpolate )
{
    Ref_surf_type ref_surf;
    float *resultElem;
    float (*shellPressure)[6];
    int i;
    float *result_buf;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int obj_qty;
    int index;
    int *object_ids;
    Subrec_obj *p_subrec;
    char *primal_list[1];
    MO_class_data *p_mo_class;

    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    object_ids = p_subrec->object_ids;
    obj_qty = p_subrec->subrec.qty_objects;
    resultElem = p_subrec->p_object_class->data_buffer;
    p_mo_class = p_subrec->p_object_class;
    ref_surf = analy->ref_surf;

    /*
     * Don't want to read in all the primals from the Result_candidate for
     * this result, so build up a new primals array with just the right one.
     */
    switch ( ref_surf )
    {
    case MIDDLE:
        primal_list[0] = primals[1];
        break;
    case INNER:
        primal_list[0] = primals[0];
        break;
    case OUTER:
        primal_list[0] = primals[2];
        break;
    }

    /* Just use analy->tmp_result as an extra long buffer. */
    result_buf = analy->tmp_result[0];

    /* Read the database. */
    analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 1,
                           primal_list, (void *) result_buf );

    shellPressure = (float (*)[6]) result_buf;

    /* Compute pressure. */
    if ( object_ids )
    {
        for ( i = 0; i < obj_qty; i++ )
        {
            resultElem[object_ids[i]] = -( shellPressure[i][0]
                                           + shellPressure[i][1]
                                           + shellPressure[i][2] ) * ONETHIRD ;
        }
    }
    else
    {
        for ( i = 0; i < obj_qty; i++ )
        {
            resultElem[i] = -( shellPressure[i][0]
                               + shellPressure[i][1]
                               + shellPressure[i][2] ) * ONETHIRD ;
        }
    }

    /* Update result to indicate that reference surface matters. */
    p_result->modifiers.use_flags.use_ref_surface = 1;
    p_result->modifiers.ref_surf = analy->ref_surf;

    if ( interpolate )
        if(p_mo_class->superclass == G_QUAD)
        {
            quad_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy, TRUE );
        }
        else if(p_mo_class->superclass == G_TRI)
        {

            tri_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                          object_ids, analy, TRUE );
        } 
}

/************************************************************
 * TAG( compute_es_effstress )
 *
 * Computes the pressure at nodes.

*************************************************************/
void compute_es_effstress( Analysis *analy, float *resultArr, Bool_type interpolate )
{
    float *resultElem;
    float *stress;
    float devStress[3];
    float pressure;
    int obj_qty;
    int i, j, rval, ipt_index;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int index, elem_idx;
    int *object_ids;
    Subrec_obj *p_subrec;
    MO_class_data* p_mo_class;
    Primal_result* primal_result;
    Htable_entry* p_hte;
    char ipt[5];
    char es_name[32];
    char es_primal[64];
    char** es_primals;
    char* stress_components[6] = {"sx", "sy", "sz", "sxy", "syz", "szx"};
    float *(stresses)[6];

    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    object_ids = p_subrec->object_ids;
    obj_qty = p_subrec->subrec.qty_objects;
    resultElem = p_subrec->p_object_class->data_buffer;
    p_mo_class = p_subrec->p_object_class;
 
    /* allocate memory for the result_buf arrays to hold the three primals 
     * necessary to calculate pressure */
    for(i = 0; i < 6; i++)
    {
        stresses[i] = calloc(obj_qty, sizeof(float));
        if(stresses[i] == NULL)
        {
            popup_dialog(WARNING_POPUP, "Out of memory in function compute_es_press, exiting\n");
            parse_command("quit", analy);
        }
    }

    es_primals = (char **) malloc(2*sizeof(char *));
    if(es_primals == NULL)
    {
        popup_dialog(WARNING_POPUP, "Out of memory in function compute_es_pressure. Exiting\n");
        parse_command("quit", analy);
    }
    es_primals[0] = (char *) malloc(32*sizeof(char));
    if(es_primals[0] == NULL)
    {
        popup_dialog(WARNING_POPUP, "Out of memory in function compute_es_pressure. Exiting\n");
        parse_command("quit", analy);
    }
   
    // Look up element set and gather stress components
    rval = htable_search(analy->primal_results, primals[0], FIND_ENTRY, &p_hte);
    if(rval == OK){
        primal_result = (Primal_result*) p_hte->data;

        // Match primal result subrec to results subrecord
        // and get the element set name.
        for( i = 0; i < primal_result->qty_subrecs; i++ ){
            if( primal_result->subrecs[i] == p_subrec){
                strcpy(es_name, primal_result->original_names_per_subrec[i]);
                break;
            }
        }

        // Gather all the stress components.
        ipt_index = p_subrec->element_set->current_index + 1;
        for( i = 0; i < 6; i++ ){
            strcpy(es_primal, es_name);
            strcat(es_primal, "[");
            //stress componenet name here
            strcat(es_primal, stress_components[i] );
            strcat(es_primal, ",");
            sprintf(ipt, "%d", ipt_index);
            strcat(es_primal, ipt);
            strcat(es_primal, "]");
            es_primals[0] = es_primal;
            es_primals[1] = NULL;
            analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 1,
                                   es_primals, (void *) stresses[i] );
        }
    }

    for(i = 0; i < obj_qty; i++)
    {
        pressure = -( stresses[0][i] +
                      stresses[1][i] +
                      stresses[2][i] ) * ONETHIRD;

        /* calculate deviatoric components of stress tensor */
        for(j = 0; j < 3; j++)
            devStress[j] = stresses[j][i] + pressure;

        elem_idx = ( object_ids ) ? object_ids[i] : i;
 
        /* calculate effective stress from deviatoric components. */
        resultElem[elem_idx] = 0.5 * ( devStress[0] * devStress[0] +
                                      devStress[1] * devStress[1] +
                                      devStress[2] * devStress[2] )
                               +   stresses[3][i] * stresses[3][i]
                               +   stresses[4][i] * stresses[4][i]
                               +   stresses[5][i] * stresses[5][i];
        resultElem[elem_idx] = sqrtf(3.0 * resultElem[elem_idx]);
    }

    if ( interpolate ){
        switch( p_mo_class->superclass)
        {
        case G_HEX:
            hex_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                        object_ids, analy );
            break;
        case G_QUAD:
            quad_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy, TRUE );
            break;
        case G_TRI:
            tri_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                          object_ids, analy, TRUE );
            break;
        case G_BEAM:
            beam_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy );
            break;
        case G_TRUSS:
            truss_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                            object_ids, analy );
            break;
        case G_TET:
            tet_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                            object_ids, analy );
            break;
        case G_PARTICLE:
            particle_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                               object_ids, analy );
            break;
        }
    }
}

/************************************************************
 * TAG( compute_shell_effstress )
 *
 * Computes the effective stress at nodes.
 */
void
compute_shell_effstress( Analysis *analy, float *resultArr,
                         Bool_type interpolate )
{
    float *resultElem;
    float devStress[3];
    float pressure;
    Ref_surf_type ref_surf;
    float *shellStress;
    int i, j;
    float *result_buf;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int obj_qty, out_idx;
    int index;
    int *object_ids;
    Subrec_obj *p_subrec;
    char *primal_list[1];
    double interm_result;
    MO_class_data *p_mo_class;

    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    object_ids = p_subrec->object_ids;
    obj_qty = p_subrec->subrec.qty_objects;
    resultElem = p_subrec->p_object_class->data_buffer;
    p_mo_class = p_subrec->p_object_class;

    ref_surf = analy->ref_surf;

    /*
     * Don't want to read in all the primals from the Result_candidate for
     * this result, so build up a new primals array with just the right one.
     */
    switch ( ref_surf )
    {
    case MIDDLE:
        primal_list[0] = primals[1];
        break;
    case INNER:
        primal_list[0] = primals[0];
        break;
    case OUTER:
        primal_list[0] = primals[2];
        break;
    }

    /* Just use analy->tmp_result as an extra long buffer. */
    result_buf = analy->tmp_result[0];

    /* Read the database. */
    analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 1,
                           primal_list, (void *) result_buf );

    /* Calculate pressure as intermediate variable needed for
     * determining deviatoric components.
     *
     * Since there's already a lot of computation inside the
     * loop, we'll allow the repeated determination of out_idx
     * source rather than having separate loops.
     */
    for ( i = 0, shellStress = result_buf; i < obj_qty; i++, shellStress += 6 )
    {
        pressure = -( shellStress[0] +
                      shellStress[1] +
                      shellStress[2] ) * ONETHIRD;

        /* Calculate deviatoric components of stress tensor. */
        for ( j = 0; j < 3; j++ )
            devStress[j] = shellStress[j] + pressure;

        /* Calculate effective stress from deviatoric components. */
        interm_result = 0.5 * ( devStress[0]*devStress[0]
                                + devStress[1]*devStress[1]
                                + devStress[2]*devStress[2] )
                        + shellStress[3] * shellStress[3]
                        + shellStress[4] * shellStress[4]
                        + shellStress[5] * shellStress[5];

        out_idx = ( object_ids == NULL ) ? i : object_ids[i];
        resultElem[out_idx] = sqrt( 3.0 * interm_result );
    }

    /* Update result to indicate that reference surface matters. */
    p_result->modifiers.use_flags.use_ref_surface = 1;
    p_result->modifiers.ref_surf = analy->ref_surf;

    if ( interpolate )
        if(p_mo_class->superclass == G_QUAD)
        {
            quad_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy, TRUE );
        }
        else if (p_mo_class->superclass == G_TRI)
        {

            tri_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                          object_ids, analy, TRUE );
        }
}

/************************************************************
 * TAG( compute_es_principal_stress )
 *
 * Computes the pressure at nodes.

*************************************************************/
void
compute_es_principal_stress( Analysis *analy, float *resultArr, Bool_type interpolate)
{
    float *resultElem;
    float pressure, interm_result;
    float Invariant[3];              /* Invariants of tensor. */
    float princStress[3];            /* Principal values. */
    float alpha, angle, value;
    float devStress[3];
    int i, j, out_idx;
    int rval, ipt_index;
    Result *p_result;
    char ** primals;
    int subrec, srec;
    int res_index, elem_idx;
    int obj_qty;
    int index;
    int *object_ids;
    Subrec_obj *p_subrec;
    MO_class_data* p_mo_class;
    Primal_result* primal_result;
    Htable_entry* p_hte;
    char ipt[5];
    char es_name[32];
    char es_primal[64];
    char** es_primals;
    char* stress_components[6] = {"sx", "sy", "sz", "sxy", "syz", "szx"};
    float *(stresses)[6];

    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    object_ids = p_subrec->object_ids;
    obj_qty = p_subrec->subrec.qty_objects;
    resultElem = p_subrec->p_object_class->data_buffer;
    p_mo_class = p_subrec->p_object_class;
 
    /* allocate memory for the result_buf arrays to hold the three primals 
     * necessary to calculate pressure */
    for(i = 0; i < 6; i++)
    {
        stresses[i] = calloc(obj_qty, sizeof(float));
        if(stresses[i] == NULL)
        {
            popup_dialog(WARNING_POPUP, "Out of memory in function compute_es_press, exiting\n");
            parse_command("quit", analy);
        }
    }

    es_primals = (char **) malloc(2*sizeof(char *));
    if(es_primals == NULL)
    {
        popup_dialog(WARNING_POPUP, "Out of memory in function compute_es_pressure. Exiting\n");
        parse_command("quit", analy);
    }
    es_primals[0] = (char *) malloc(32*sizeof(char));
    if(es_primals[0] == NULL)
    {
        popup_dialog(WARNING_POPUP, "Out of memory in function compute_es_pressure. Exiting\n");
        parse_command("quit", analy);
    }
   
    // Look up element set and gather stress components
    rval = htable_search(analy->primal_results, primals[0], FIND_ENTRY, &p_hte);
    if(rval == OK){
        primal_result = (Primal_result*) p_hte->data;

        // Match primal result subrec to results subrecord
        // and get the element set name.
        for( i = 0; i < primal_result->qty_subrecs; i++ ){
            if( primal_result->subrecs[i] == p_subrec){
                strcpy(es_name, primal_result->original_names_per_subrec[i]);
                break;
            }
        }

        // Gather all the stress components.
        ipt_index = p_subrec->element_set->current_index + 1;
        for( i = 0; i < 6; i++ ){
            strcpy(es_primal, es_name);
            strcat(es_primal, "[");
            //stress componenet name here
            strcat(es_primal, stress_components[i] );
            strcat(es_primal, ",");
            sprintf(ipt, "%d", ipt_index);
            strcat(es_primal, ipt);
            strcat(es_primal, "]");
            es_primals[0] = es_primal;
            es_primals[1] = NULL;
            analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 1,
                                   es_primals, (void *) stresses[i] );
        }
    }
 
    /* Map result onto a numeric index. */
    if ( strcmp( p_result->name, "pdev1" ) == 0 )
        res_index = 0;
    else if ( strcmp( p_result->name, "pdev2" ) == 0 )
        res_index = 1;
    else if ( strcmp( p_result->name, "pdev3" ) == 0 )
        res_index = 2;
    else if ( strcmp( p_result->name, "maxshr" ) == 0 )
        res_index = 3;
    else if ( strcmp( p_result->name, "prin1" ) == 0 )
        res_index = 4;
    else if ( strcmp( p_result->name, "prin2" ) == 0 )
        res_index = 5;
    else if ( strcmp( p_result->name, "prin3" ) == 0 )
        res_index = 6;

    /* Calculate deviatoric stresses. */
    for ( i = 0; i < obj_qty; i++ )
    {
        /* Calculate pressure as intermediate variable needed for
         * determining deviatoric components.
         */
        pressure = -( stresses[0][i] +
                      stresses[1][i] +
                      stresses[2][i] ) * ONETHIRD;

        /* Calculate deviatoric components of stress tensor. */
        for ( j = 0; j < 3; j++ )
            devStress[j] = stresses[j][i] + pressure;

        /* Calculate invariants of deviatoric tensor. */
        /* Invariant[0] = 0.0 */
        Invariant[0] = devStress[0] + devStress[1] + devStress[2];
        Invariant[1] = 0.5 * ( devStress[0] * devStress[0]
                               + devStress[1] * devStress[1]
                               + devStress[2] * devStress[2] )
                       + stresses[3][i] * stresses[3][i]
                       + stresses[4][i] * stresses[4][i]
                       + stresses[5][i] * stresses[5][i];
        Invariant[2] = -devStress[0] * devStress[1] * devStress[2]
                       - 2.0 * stresses[3][i] * stresses[4][i] * stresses[5][i]
                       + devStress[0] * stresses[4][i] * stresses[4][i]
                       + devStress[1] * stresses[5][i] * stresses[5][i]
                       + devStress[2] * stresses[3][i] * stresses[3][i];

        /* Check to see if we can have non-zero divisor, if not
         * set principal stress to 0.
         */
        if ( Invariant[1] >= 1e-7 )
        {
            alpha = -0.5 * sqrt( (double) 27.0 / Invariant[1] )
                    * Invariant[2]/Invariant[1];
            if ( alpha < 0 )
                alpha = MAX( alpha, -1.0 );
            else if ( alpha > 0 )
                alpha = MIN( alpha, 1.0 );
            angle = acos((double)alpha) * ONETHIRD;
            value = 2.0 * sqrt( (double)Invariant[1] * ONETHIRD);
            princStress[0] = value*cos((double)angle);
            angle = angle - 2.0*PI * ONETHIRD ;
            princStress[1] = value*cos((double)angle);
            angle = angle + 4.0*PI * ONETHIRD;
            princStress[2] = value*cos((double)angle);
        }
        else
        {
            princStress[0] = 0.0;
            princStress[1] = 0.0;
            princStress[2] = 0.0;
        }

        elem_idx = ( object_ids ) ? object_ids[i] : i;

        switch ( res_index )
        {
        case 0:
            resultElem[elem_idx] = princStress[0];
            break;
        case 1:
            resultElem[elem_idx] = princStress[1];
            break;
        case 2:
            resultElem[elem_idx] = princStress[2];
            break;
        case 3:
            resultElem[elem_idx] = (princStress[0] - princStress[2]) * ONEHALF;
            break;
        case 4:
            resultElem[elem_idx] = princStress[0] - pressure;
            break;
        case 5:
            resultElem[elem_idx] = princStress[1] - pressure;
            break;
        case 6:
            resultElem[elem_idx] = princStress[2] - pressure;
            break;
        }
    }

    if ( interpolate ){
        switch( p_mo_class->superclass)
        {
        case G_HEX:
            hex_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                        object_ids, analy );
            break;
        case G_QUAD:
            quad_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy, TRUE );
            break;
        case G_TRI:
            tri_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                          object_ids, analy, TRUE );
            break;
        case G_BEAM:
            beam_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy );
            break;
        case G_TRUSS:
            truss_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                            object_ids, analy );
            break;
        case G_TET:
            tet_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                            object_ids, analy );
            break;
        case G_PARTICLE:
            particle_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                               object_ids, analy );
            break;
        }
    }
}

/************************************************************
 * TAG( compute_shell_principal_stress )
 *
 * Computes the principal deviatoric stresses and principal
 * stresses.
 */
void
compute_shell_principal_stress( Analysis *analy, float *resultArr,
                                Bool_type interpolate )
{
    float *resultElem;               /* Element results vector. */
    float *shellStress;              /* Ptr to element stresses. */
    float devStress[3];              /* Deviatoric stresses,
                                        only need diagonal terms. */
    float Invariant[3];              /* Invariants of tensor. */
    float princStress[3];            /* Principal values. */
    float pressure;
    float alpha, angle, value;
    char *primal_list[1];
    Ref_surf_type ref_surf;
    int i, j;
    int obj_qty;
    float *result_buf;
    Result *p_result;
    char **primals;
    int subrec, srec;
    int index, res_index, elem_idx;
    Subrec_obj *p_subrec;
    int *object_ids;
    MO_class_data *p_mo_class;

    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    object_ids = p_subrec->object_ids;
    obj_qty = p_subrec->subrec.qty_objects;
    resultElem = p_subrec->p_object_class->data_buffer;
    p_mo_class = p_subrec->p_object_class;

    ref_surf = analy->ref_surf;

    /*
     * Don't want to read in all the primals from the Result_candidate for
     * this result, so build up a new primals array with just the right one.
     */
    switch ( ref_surf )
    {
    case MIDDLE:
        primal_list[0] = primals[1];
        break;
    case INNER:
        primal_list[0] = primals[0];
        break;
    case OUTER:
        primal_list[0] = primals[2];
        break;
    }


    /* Just use analy->tmp_result[0] as an extra long buffer. */
    result_buf = analy->tmp_result[0];

    /* Read the database. */
    analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 1,
                           primal_list, (void *) result_buf );

    /* Map result onto a numeric index. */
    if ( strcmp( p_result->name, "pdev1" ) == 0 )
        res_index = 0;
    else if ( strcmp( p_result->name, "pdev2" ) == 0 )
        res_index = 1;
    else if ( strcmp( p_result->name, "pdev3" ) == 0 )
        res_index = 2;
    else if ( strcmp( p_result->name, "maxshr" ) == 0 )
        res_index = 3;
    else if ( strcmp( p_result->name, "prin1" ) == 0 )
        res_index = 4;
    else if ( strcmp( p_result->name, "prin2" ) == 0 )
        res_index = 5;
    else if ( strcmp( p_result->name, "prin3" ) == 0 )
        res_index = 6;

    /* Calculate deviatoric stresses. */
    for ( i = 0, shellStress = result_buf; i < obj_qty; i++, shellStress += 6 )
    {
        /* Calculate pressure as intermediate variable needed for
         * determining deviatoric components.
         */
        pressure = -( shellStress[0] +
                      shellStress[1] +
                      shellStress[2] ) * ONETHIRD;

        /* Calculate deviatoric components of stress tensor. */
        for ( j = 0; j < 3; j++ )
            devStress[j] = shellStress[j] + pressure;

        /* Calculate invariants of deviatoric tensor. */
        /* Invariant[0] = 0.0 */
        Invariant[0] = devStress[0] + devStress[1] + devStress[2];
        Invariant[1] = 0.5 * ( devStress[0] * devStress[0]
                               + devStress[1] * devStress[1]
                               + devStress[2] * devStress[2] )
                       + shellStress[3] * shellStress[3]
                       + shellStress[4] * shellStress[4]
                       + shellStress[5] * shellStress[5];
        Invariant[2] = -devStress[0] * devStress[1] * devStress[2]
                       - 2.0 * shellStress[3] * shellStress[4] * shellStress[5]
                       + devStress[0] * shellStress[4] * shellStress[4]
                       + devStress[1] * shellStress[5] * shellStress[5]
                       + devStress[2] * shellStress[3] * shellStress[3];

        /* Check to see if we can have non-zero divisor, if not
         * set principal stress to 0.
         */
        if ( Invariant[1] >= 1e-7 )
        {
            alpha = -0.5 * sqrt( (double) 27.0 / Invariant[1] )
                    * Invariant[2]/Invariant[1];
            if ( alpha < 0 )
                alpha = MAX( alpha, -1.0 );
            else if ( alpha > 0 )
                alpha = MIN( alpha, 1.0 );
            angle = acos((double)alpha) * ONETHIRD;
            value = 2.0 * sqrt( (double)Invariant[1] * ONETHIRD);
            princStress[0] = value*cos((double)angle);
            angle = angle - 2.0*PI * ONETHIRD ;
            princStress[1] = value*cos((double)angle);
            angle = angle + 4.0*PI * ONETHIRD;
            princStress[2] = value*cos((double)angle);
        }
        else
        {
            princStress[0] = 0.0;
            princStress[1] = 0.0;
            princStress[2] = 0.0;
        }

        elem_idx = ( object_ids ) ? object_ids[i] : i;

        switch ( res_index )
        {
        case 0:
            resultElem[elem_idx] = princStress[0];
            break;
        case 1:
            resultElem[elem_idx] = princStress[1];
            break;
        case 2:
            resultElem[elem_idx] = princStress[2];
            break;
        case 3:
            resultElem[elem_idx] = (princStress[0] - princStress[2]) * ONEHALF;
            break;
        case 4:
            resultElem[elem_idx] = princStress[0] - pressure;
            break;
        case 5:
            resultElem[elem_idx] = princStress[1] - pressure;
            break;
        case 6:
            resultElem[elem_idx] = princStress[2] - pressure;
            break;
        }
    }

    /* Update result to indicate that reference surface matters. */
    p_result->modifiers.use_flags.use_ref_surface = 1;
    p_result->modifiers.ref_surf = analy->ref_surf;

    if ( interpolate )
        if(p_mo_class->superclass == G_QUAD)
        {
            quad_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                           object_ids, analy, TRUE );
        }
        else if(p_mo_class->superclass == G_TRI)
        {
            tri_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                          object_ids, analy, TRUE );
        }

}


/************************************************************
 * TAG( compute_shell_surface_stress )
 *
 * Computes shell surface stresses at nodes.
 */
void
compute_shell_surface_stress( Analysis *analy, float *resultArr,
                              Bool_type interpolate )
{
    float *resultElem;
    float *thickness;
    float one_over_t, one_over_t2;
    float sx, sy, sxy, sigef[2];      /* Vars for eff surface stress. */
    int obj_qty, i;
    int out, index, nidx, bidx, res_index;
    char *primal_list[3];
    GVec3D *bending_resultant, *normal_resultant;
    Result *p_result;
    char **primals;
    int subrec, srec;
    Subrec_obj *p_subrec;
    int *object_ids;

    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    object_ids = p_subrec->object_ids;
    obj_qty = p_subrec->subrec.qty_objects;
    resultElem = p_subrec->p_object_class->data_buffer;

    /* Map result onto a numeric index. */
    if ( strcmp( p_result->name, "surf1" ) == 0 )
        res_index = 0;
    else if ( strcmp( p_result->name, "surf2" ) == 0 )
        res_index = 1;
    else if ( strcmp( p_result->name, "surf3" ) == 0 )
        res_index = 2;
    else if ( strcmp( p_result->name, "surf4" ) == 0 )
        res_index = 3;
    else if ( strcmp( p_result->name, "surf5" ) == 0 )
        res_index = 4;
    else if ( strcmp( p_result->name, "surf6" ) == 0 )
        res_index = 5;
    else if ( strcmp( p_result->name, "eff1" ) == 0 )
        res_index = 6;
    else if ( strcmp( p_result->name, "eff2" ) == 0 )
        res_index = 7;
    else if ( strcmp( p_result->name, "effmax" ) == 0 )
        res_index = 8;

    /* Initialize primal results request list. */
    primal_list[0] = primals[0];
    primal_list[1] = primals[2];
    primal_list[2] = primals[3];

    /* Initialize access pointers. */
    bending_resultant = (GVec3D *) analy->tmp_result[0];
    normal_resultant = bending_resultant + obj_qty;
    thickness = (float *) (normal_resultant + obj_qty);

    /* Initialize normal and bending result component indices. */
    switch ( res_index )
    {
    case 0:
    case 1:
        bidx = 0;
        nidx = 0;
        break;
    case 2:
    case 3:
        bidx = 1;
        nidx = 1;
        break;
    case 4:
    case 5:
        bidx = 2;
        nidx = 2;
        break;
    }

    /* Read the database. */
    /*analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 3,
                           primal_list, (void *) analy->tmp_result[0] ); */
    analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec, 2,
                           primal_list, (void *) analy->tmp_result[0] );

    /* Calculate the value given the normal and bending components
     * and the element thickness.
     */
    for ( i = 0; i < obj_qty; i++ )
    {
        one_over_t = 1.0 / thickness[i];
        one_over_t2 = one_over_t / thickness[i];

        out = ( object_ids == NULL ) ? i : object_ids[i];

        switch ( res_index )
        {
        case 0:
        case 2:
        case 4:
            resultElem[out] = normal_resultant[i][nidx]*one_over_t +
                              6.0*bending_resultant[i][bidx]*one_over_t2;
            break;
        case 1:
        case 3:
        case 5:
            resultElem[out] = normal_resultant[i][nidx]*one_over_t -
                              6.0*bending_resultant[i][bidx]*one_over_t2;
            break;
        case 6:
            /* These are the upper surface values. */
            sx = normal_resultant[i][0]*one_over_t +
                 6.0*bending_resultant[i][0]*one_over_t2 ;
            sy = normal_resultant[i][1]*one_over_t +
                 6.0*bending_resultant[i][1]*one_over_t2 ;
            sxy = normal_resultant[i][2]*one_over_t +
                  6.0*bending_resultant[i][2]*one_over_t2 ;
            resultElem[out] = sqrt( (double)
                                    (sx*sx - sx*sy + sy*sy + 3.0*sxy*sxy));
            break;
        case 7:
            /* These are the lower surface values. */
            sx = normal_resultant[i][0]*one_over_t -
                 6.0*bending_resultant[i][0]*one_over_t2 ;
            sy = normal_resultant[i][1]*one_over_t -
                 6.0*bending_resultant[i][1]*one_over_t2 ;
            sxy = normal_resultant[i][2]*one_over_t -
                  6.0*bending_resultant[i][2]*one_over_t2 ;
            resultElem[out] = sqrt( (double)
                                    (sx*sx - sx*sy + sy*sy + 3.0*sxy*sxy));
            break;
        case 8:
            /* Calc upper surface first. */
            sx = normal_resultant[i][0]*one_over_t +
                 6.0*bending_resultant[i][0]*one_over_t2 ;
            sy = normal_resultant[i][1]*one_over_t +
                 6.0*bending_resultant[i][1]*one_over_t2 ;
            sxy = normal_resultant[i][2]*one_over_t +
                  6.0*bending_resultant[i][2]*one_over_t2 ;
            sigef[0] = sqrt( (double)
                             (sx*sx - sx*sy + sy*sy + 3.0*sxy*sxy));

            /* Now calc lower surface. */
            sx = normal_resultant[i][0]*one_over_t -
                 6.0*bending_resultant[i][0]*one_over_t2 ;
            sy = normal_resultant[i][1]*one_over_t -
                 6.0*bending_resultant[i][1]*one_over_t2 ;
            sxy = normal_resultant[i][2]*one_over_t -
                  6.0*bending_resultant[i][2]*one_over_t2 ;
            sigef[1] = sqrt( (double)
                             (sx*sx - sx*sy + sy*sy + 3.0*sxy*sxy));

            /* Return maximum value. */
            resultElem[out] = MAX( sigef[0], sigef[1] );
            break;
        }
    }

    if ( interpolate )
        quad_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                       object_ids, analy, FALSE );
}

