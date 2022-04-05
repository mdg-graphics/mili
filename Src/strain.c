/* $Id$ */
/*
 * strain.c - Routines for computing derived strain quantities.
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
 * 03/07/1995  S.C. Nevin   hex_principal_strain()
 *                          Change internal variables to double, and
 *                          change the zero tolerance to a smaller number.
 *
 * 03/13/1995  S.C. Nevin   compute_hex_strain()
 *                          Fix strain rate calculation.  Must use the
 *                          velocity vector, not the velocity magnitude.
 *
 * 09/13/2004  I. R. Corey  Fix an error in call to transform_tensors - the single
 *                          percision version was being called for double data.
 *
 * 03/29/2005 I. R. Corey   Added capability to compute hex strain for
 *                          timehistory databases.
 *                          See SRC#: 291
 *
 * 01/17/2008 I. R. Corey   Compute shell strain for primals (strain_in[ex]
 *                          and strain_out[ex]).
 *                          See SRC#: 514
 *
 * 01/17/2008 I. R. Corey   Compute rotations for Shell Strains correctly for
 *                          TH databases - need to get c ooords from TH
 *                          nodes.
 *                          See SRC#: 523
 *
 * 08/27/2008 I. R. Corey   Strain rotations not computed correctly for
 *                          data organized by object ids in rotate_quad_
 *                          result.
 *                          See SRC#: 546
 *
 * 11/10/2008 I. R. Corey   Fixed an indexing problem for strain rotations
 *                          when object ids are found in compute_shell_
 *                          strain.
 *                          See SRC#: 551
 *
 * 05/22/2009 I. R. Corey   Fixed a problem with loading object ids
 *                          for quad rotations.
 *                          See SRC#: 605
 *
 * 09/29/2009 I. R. Corey   Fixed a problem with computing strains for
 *                          shared result with multiple Hex element
 *                          sets.
 *                          See SRC#: 629
 *
 * 09/07/2011 I. R. Corey   Fixed a problem with computing hex strain
 *                          rate.
 *
 * 01/29/2013 I. R. Corey   Fixed a problem with computing hex strain
 *                          for TH databases.
 *
 *************************************************************************
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "viewer.h"

#define ONETHIRD .3333333333333333
#define ONEHALF .5

static void hex_partial_deriv( double [8], double [8], double [8], double [8],
                               double [8], double [8] );
static void extract_strain_vec( double *, double [9], Strain_type );
static void hex_principal_strain( double *, double * );
static void load_reference_node_pos( Analysis * );

static Result *velx = NULL, *vely = NULL, *velz = NULL;

static GVec3D2P *reference_nodes = NULL;

static float *strain_vel = NULL;
static double *strain_vel_2p = NULL;


/************************************************************
 * TAG( free_static_strain_data )
 *
 * Releases nodal velocity component Result structs.
 */
void
free_static_strain_data( void )
{
    int i;

    if ( velx != NULL )
    {
        for ( i = 0; i < 3; i++ )
            cleanse_result( velx + i );

        free( velx );

        velx = vely = velz = NULL;
    }

    if ( reference_nodes != NULL )
    {
        free( reference_nodes );

        reference_nodes = NULL;
    }

    if ( strain_vel != NULL )
    {
        free( strain_vel );

        strain_vel = NULL;
    }

    if ( strain_vel_2p != NULL )
    {
        free( strain_vel_2p );

        strain_vel_2p = NULL;
    }
}


/************************************************************
 * TAG( compute_hex_strain )
 *
 * Computes the strain at nodes given the current geometry
 * and initial configuration.
 */
void
compute_hex_strain( Analysis *analy, float *resultArr, Bool_type interpolate )
{
    double x[8],  y[8], z[8];        /* Initial element geom. */
    double xx[8], yy[8], zz[8];      /* Current element geom. */
    double xv[8], yv[8], zv[8];      /* Current node velocities. */
    double px[8], py[8], pz[8];      /* Global derivates dN/dx,dN/dy,dN/dz. */
    double F[9];                     /* Element deformation gradient. */
    double detF;                     /* Determinant of element
                                        deformation gradient. */
    double eps[6];                   /* Calculated strain. */
    float *resultElem;               /* Array for the element data. */
    double meanStrain;               /* Mean strain for an element. */
    Ref_frame_type ref_frame;
    float localMat[3][3];
    int i, j, k, l, m, n, nd;
    int (*connects)[8];
    Result *p_result;
    int class_qty, mat_qty, active_qty, elem_block_qty, qty;
    int res_index, start, stop;
    GVec3D *nodes3d, *onodes3d;
    float *nxv, *nyv, *nzv;          /* Nodal velocity arrays for rate calc. */
    double *nxv_2p, *nyv_2p, *nzv_2p; /* Double prec. arrays for rate calc. */
    GVec3D2P *nodes3d2p, *onodes3d2p;
    State_rec_obj *p_sro;
    float *tmp_data;
    int x_rval, y_rval, z_rval;
    MO_class_data *p_hex_class, *p_mat_class;
    Material_data *p_mats;
    unsigned char *disable_mat;
    int *active_mats;
    int cur_mat;
    Bool_type single_prec_pos, single_prec_vel;
    static Bool_type mixed_prec_warn = FALSE;
    Bool particle_class=FALSE;

    int status=OK;

    /* IRC: Added March 29, 2005 - Support strain
     *      calculations for timehistory databases
     */
    Bool_type map_timehist_coords = FALSE;
    int       elem_index;
    int       obj_cnt=0, obj_num=0, *obj_ids=NULL;
    GVec3D2P  *new_nodes;

    p_result = analy->cur_result;
    ref_frame = analy->ref_frame;

    if (!analy->stateDB)
        map_timehist_coords = TRUE;


    /* Determine a result index. */
    if ( strcmp( p_result->name, "ex" ) == 0 )
        res_index = 0;
    else if ( strcmp( p_result->name, "ey" ) == 0 )
        res_index = 1;
    else if ( strcmp( p_result->name, "ez" ) == 0 )
        res_index = 2;
    else if ( strcmp( p_result->name, "exy" ) == 0 )
        res_index = 3;
    else if ( strcmp( p_result->name, "eyz" ) == 0 )
        res_index = 4;
    else if ( strcmp( p_result->name, "ezx" ) == 0 )
        res_index = 5;
    else if ( strcmp( p_result->name, "pdstrn1" ) == 0 )
        res_index = 6;
    else if ( strcmp( p_result->name, "pdstrn2" ) == 0 )
        res_index = 7;
    else if ( strcmp( p_result->name, "pdstrn3" ) == 0 )
        res_index = 8;
    else if ( strcmp( p_result->name, "pshrstr" ) == 0 )
        res_index = 9;
    else if ( strcmp( p_result->name, "pstrn1" ) == 0 )
        res_index = 10;
    else if ( strcmp( p_result->name, "pstrn2" ) == 0 )
        res_index = 11;
    else if ( strcmp( p_result->name, "pstrn3" ) == 0 )
        res_index = 12;
    else if ( strcmp( p_result->name, "gamxy" ) == 0 )
        res_index = 13;
    else if ( strcmp( p_result->name, "gamyz" ) == 0 )
        res_index = 14;
    else if ( strcmp( p_result->name, "gamzx" ) == 0 )
        res_index = 15;

    /* Get node position data precision. */
    single_prec_pos = !(MESH_P( analy )->double_precision_nodpos);


    /* IRC: Added March 29, 2005 - Support strain
     *      calculations for timehistory databases
     */
    if (map_timehist_coords)
    {
        /* Save current result */
        p_result = analy->cur_result;

        status = load_hex_nodpos_timehist( analy, analy->cur_state+1, single_prec_pos,
                                           &obj_cnt, &obj_ids, &new_nodes);

        analy->cur_result = p_result;

        if (status!=OK)
            return;
    }


    /* Get nodal velocities if strain rate is desired. */
    if ( analy->strain_variety == RATE )
    {
        if ( velx == NULL )
        {
            /* Initialize velocity Result structs. */
            velx = NEW_N( Result, 3, "Vel Results for strain type rate" );
            vely = velx + 1;
            velz = velx + 2;

            x_rval = find_result( analy, ALL, TRUE, velx, "nodvel[vx]" );
            y_rval = find_result( analy, ALL, TRUE, vely, "nodvel[vy]" );
            z_rval = find_result( analy, ALL, TRUE, velz, "nodvel[vz]" );

            if ( !x_rval || !y_rval || !z_rval )
            {
                popup_dialog( INFO_POPUP, "%s\n%s",
                              "Velocity not available for strain rate",
                              "calculation; aborted." );
                for ( i = 0; i < 3; i++ )
                    cleanse_result( velx + i );
                free( velx );
                velx = vely = velz = NULL;

                return;
            }
        }
        else
        {
            /*
             * Alread have Results, so just update them.
             * We're assuming that if velocity was available in any state
             * format, it'll be available in all formats.
             */
            update_result( analy, velx );
            update_result( analy, vely );
            update_result( analy, velz );
        }

        /* Capture velocity precision. */
        single_prec_vel = ( ((Primal_result *)
                             velx->original_result)->var->num_type
                            == G_FLOAT4
                            ||
                            ((Primal_result *)
                             velx->original_result)->var->num_type
                            == G_FLOAT );

        if ( single_prec_pos )
        {
            if ( single_prec_vel )
            {
                /* No working buffer conflict; just use analy->tmp_result. */

                tmp_data = NODAL_RESULT_BUFFER( analy );

                nxv = analy->tmp_result[0];
                nyv = analy->tmp_result[1];
                nzv = analy->tmp_result[2];

                /* Load nodal velocities for current state. */

                analy->cur_result = velx;
                NODAL_RESULT_BUFFER( analy ) = nxv;
                load_result( analy, TRUE, FALSE, FALSE );

                analy->cur_result = vely;
                NODAL_RESULT_BUFFER( analy ) = nyv;
                load_result( analy, TRUE, FALSE, FALSE );

                analy->cur_result = velz;
                NODAL_RESULT_BUFFER( analy ) = nzv;
                load_result( analy, TRUE, FALSE, FALSE );

                analy->cur_result = p_result;
                NODAL_RESULT_BUFFER( analy ) = tmp_data;
            }
            else
            {
                if ( !mixed_prec_warn )
                {
                    popup_dialog( INFO_POPUP,
                                  "Strain RATE not available for single "
                                  "precision\nnode positions and double "
                                  "precision velocities; aborting.\n(This "
                                  "message will not repeat)" );
                    mixed_prec_warn = TRUE;
                }

                for ( i = 0; i < 3; i++ )
                    cleanse_result( velx + i );
                free( velx );
                velx = vely = velz = NULL;

                return;
            }
        }
        else
        {
            if ( single_prec_vel )
            {
                /* analy->tmp_result in use; allocate velocity buffers. */

                /* Save the current nodal data buffer. */
                tmp_data = NODAL_RESULT_BUFFER( analy );

                qty = MESH_P( analy )->node_geom->qty;

                if ( strain_vel == NULL )
                {
                    /* Allocate arrays for velocities into a static pointer. */
                    strain_vel = NEW_N( float, qty * 3,
                                        "Temp node vel buffers" );
                    if ( strain_vel == NULL )
                    {
                        popup_dialog( WARNING_POPUP, "Array allocation failed; "
                                      "aborting strain rate calculation." );
                        return;
                    }
                }

                nxv = strain_vel;
                nyv = nxv + qty;
                nzv = nyv + qty;

                /* Load nodal velocities for current state. */

                analy->cur_result = velx;
                NODAL_RESULT_BUFFER( analy ) = nxv;
                load_result( analy, TRUE, FALSE, FALSE );

                analy->cur_result = vely;
                NODAL_RESULT_BUFFER( analy ) = nyv;
                load_result( analy, TRUE, FALSE, FALSE );

                analy->cur_result = velz;
                NODAL_RESULT_BUFFER( analy ) = nzv;
                load_result( analy, TRUE, FALSE, FALSE );

                analy->cur_result = p_result;
                NODAL_RESULT_BUFFER( analy ) = tmp_data;
            }
            else
            {
                /* analy->tmp_result in use; allocate velocity buffers. */

                /* Save the current nodal data buffer. */
                tmp_data = NODAL_RESULT_BUFFER( analy );

                qty = MESH_P( analy )->node_geom->qty;

                if ( strain_vel_2p == NULL )
                {
                    /* Allocate arrays for velocities into a static pointer. */
                    strain_vel_2p = NEW_N( double, qty * 3,
                                           "Temp node vel buffers" );
                    if ( strain_vel_2p == NULL )
                    {
                        popup_dialog( WARNING_POPUP, "Array allocation failed; "
                                      "aborting strain rate calculation." );
                        return;
                    }
                }

                nxv_2p = strain_vel_2p;
                nyv_2p = nxv_2p + qty;
                nzv_2p = nyv_2p + qty;

                /* Load nodal velocities for current state. */

                /*
                 * Cast the double arrays to floats; the I/O library just
                 * needs an address referencing enough space to write to.
                 */

                analy->cur_result = velx;
                NODAL_RESULT_BUFFER( analy ) = (float *) nxv_2p;
                load_result( analy, TRUE, FALSE, FALSE );

                analy->cur_result = vely;
                NODAL_RESULT_BUFFER( analy ) = (float *) nyv_2p;
                load_result( analy, TRUE, FALSE, FALSE );

                analy->cur_result = velz;
                NODAL_RESULT_BUFFER( analy ) = (float *) nzv_2p;
                load_result( analy, TRUE, FALSE, FALSE );

                analy->cur_result = p_result;
                NODAL_RESULT_BUFFER( analy ) = tmp_data;
            }
        }

    }

    if ( single_prec_pos )
    {
        nodes3d = analy->state_p->nodes.nodes3d;
        onodes3d = MESH( analy ).node_geom->objects.nodes3d;
    }
    else
    {
        /*
         * Load and save node positions for the first state, which is the
         * reference state for this calculation in double precision.
         */
        if (map_timehist_coords)
        {
            if ( reference_nodes == NULL )
            {
                /* Save current result */
                p_result = analy->cur_result;

                status = load_hex_nodpos_timehist( analy, 1, single_prec_pos,
                                                   &obj_cnt, &obj_ids, &reference_nodes);
                if (status!=OK)
                    return;
            }
            onodes3d2p = reference_nodes;
        }
        else
        {
            if ( reference_nodes == NULL )
                load_reference_node_pos( analy );

            onodes3d2p = reference_nodes;

            /*
             * Read in the double precision coordinates at the current state.
             * Can't use analy->state_p->nodes because that is converted to single
             * precision immediately when read in at state change.
             */
            p_sro = analy->srec_tree + analy->state_p->srec_id;
            load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                         analy->cur_state + 1, FALSE,
                         (void *) analy->tmp_result[0] );
            nodes3d2p = (GVec3D2P *) analy->tmp_result[0];
        }
    }


    /* Prepare access to material data. */
    p_mat_class = ((MO_class_data **)
                   MESH( analy ).classes_by_sclass[G_MAT].list)[0];
    p_mats = p_mat_class->objects.materials;
    disable_mat = MESH( analy ).disable_material;
    mat_qty = analy->max_mesh_mat_qty;
    active_mats = NEW_N( int, mat_qty, "Active materials array" );

    /* Loop over all hex classes to generate results for enabled materials. */
    class_qty = MESH( analy ).classes_by_sclass[G_HEX].qty;

    for ( l = 0; l < class_qty; l++ )
    {
        p_hex_class = ((MO_class_data **)
                       MESH( analy ).classes_by_sclass[G_HEX].list)[l];

        particle_class = FALSE;
        if (is_particle_class( analy, p_hex_class->superclass, p_hex_class->short_name) )
            particle_class = TRUE;

        connects = (int (*)[8]) p_hex_class->objects.elems->nodes;
        resultElem = p_hex_class->data_buffer;

        /* Generate the list of valid, enabled materials in this class. */
        active_qty = 0;
        for ( m = 0; m < mat_qty; m++ )
        {
            /* If the material is of the hex class and it's enabled, save it. */
            if ( p_mats[m].elem_class == p_hex_class && !disable_mat[m] )
                active_mats[active_qty++] = m;
        }

        /* Loop over the active materials. */
        for ( m = 0; m < active_qty; m++ )
        {
            cur_mat = active_mats[m];
            elem_block_qty = p_mats[cur_mat].elem_block_qty;

            /* IRC: Added March 29, 2005 - Support strain
             *      calculations for timehistory databases
             */
            if (map_timehist_coords)
                elem_block_qty = 1;

            if ( class_qty>1 && elem_block_qty>1 )
            {
                elem_block_qty = 1;
            }

            /* Loop over the element blocks of the current active material. */
            for ( n = 0; n < elem_block_qty; n++ )
            {
                start = p_mats[cur_mat].elem_blocks[n][0];
                stop  = p_mats[cur_mat].elem_blocks[n][1];

                if ( class_qty>1 && stop>=p_hex_class->qty )
                {
                    start = 0;
                    stop  = p_hex_class->qty-1;
                }

                if ( obj_ids && obj_cnt>0 )
                {
                    for ( i = start; i <= stop; i++ )
                        resultElem[i] = 0.0;
                    start = 0;
                    stop  = obj_cnt-1;
                }

                /* Loop over the elements in the current element block. */
                for ( i = start; i <= stop; i++ )
                {
                    if ( particle_class )
                    {

                        elem_index = i;

                        if ( obj_ids )
                        {
                            if (map_timehist_coords && obj_ids[i] == -1)
                                continue;
                            else
                                elem_index = obj_ids[i];
                        }
                        resultElem[elem_index] = 0.0;
                    }

                    /*
                     * Since we're calculating volume element results from
                     * nodal data, loop over the hex element class, not the
                     * subrecord.
                     *
                     * Assumptions:
                     * - complete nodal data (at least with respect to the
                     * nodes referenced by the hex class) is available.  We
                     * don't read the subrecord directly, only the state_p
                     * position arrays and the node class initial position
                     * arrays, which are "proper" arrays for the class (or
                     * velocities from nodal data arrays which are also proper
                     * after the load_result() call), but the data must be
                     * available for this calculation to give correct results;
                     * in fact, the nodal subrecord should be complete and
                     * there should only be one, otherwise this function will
                     * be called multiple times, only to repeat the same
                     * calculation each time.
                     */

                    /*
                     * Strain cases:
                     *  - strain with single or double precision position inputs
                     *  - strain rate with double precision positions and
                     *    single precision velocities
                     *  - strain rate with double precision positions and
                     *    double precision velocities
                     *
                     * We haven't implemented rate calc with single precision
                     * positions and double precision velocities just because
                     * that's not likely to happen in the foreseeable future,
                     * but it would be easy enough to add that case below and
                     * where we're loading the data above.
                     */

                    if (map_timehist_coords && !obj_ids)
                        continue;

                    if ( analy->strain_variety != RATE )
                    {
                        if ( single_prec_pos )
                        {
                            /* Single precision positions. */
                            for ( j = 0; j < 8; j++ )
                            {
                                nd = connects[i][j];
                                if (map_timehist_coords)
                                {
                                    if ( obj_ids )
                                    {
                                        elem_index = obj_ids[i];
                                        nd = connects[elem_index][j];
                                    }
                                }
                                x[j] = (double) onodes3d[nd][0];
                                y[j] = (double) onodes3d[nd][1];
                                z[j] = (double) onodes3d[nd][2];

                                /* Obtain coordinates from timehistory subrecords */
                                if (map_timehist_coords)
                                {
                                    xx[j] = new_nodes[nd][0];
                                    yy[j] = new_nodes[nd][1];
                                    zz[j] = new_nodes[nd][2];
                                }
                                else
                                {
                                    xx[j] = (double) nodes3d[nd][0];
                                    yy[j] = (double) nodes3d[nd][1];
                                    zz[j] = (double) nodes3d[nd][2];
                                }
                            }
                        }
                        else
                        {
                            /* Double precision positions. */
                            for ( j = 0; j < 8; j++ )
                            {
                                nd = connects[i][j];
                                if (map_timehist_coords)
                                {
                                    if ( obj_ids )
                                    {
                                        elem_index = obj_ids[i];
                                        nd = connects[elem_index][j];
                                    }
                                }

                                x[j] = onodes3d2p[nd][0];
                                y[j] = onodes3d2p[nd][1];
                                z[j] = onodes3d2p[nd][2];

                                /* Obtain coordinates from timehistory subrecords */
                                if (map_timehist_coords)
                                {
                                    xx[j] = new_nodes[nd][0];
                                    yy[j] = new_nodes[nd][1];
                                    zz[j] = new_nodes[nd][2];
                                }
                                else
                                {
                                    xx[j] = nodes3d2p[nd][0];
                                    yy[j] = nodes3d2p[nd][1];
                                    zz[j] = nodes3d2p[nd][2];
                                }
                            }
                        }
                    }
                    else
                    {
                        /* Strain RATE calculation */

                        if ( single_prec_pos )
                        {
                            /* Single prec pos, must be single prec vel */
                            for ( j = 0; j < 8; j++ )
                            {
                                nd = connects[i][j];
                                if (map_timehist_coords)
                                {
                                    if ( obj_ids )
                                    {
                                        elem_index = obj_ids[i];
                                        nd = connects[elem_index][j];
                                    }
                                }

                                xv[j] = nxv[nd];
                                yv[j] = nyv[nd];
                                zv[j] = nzv[nd];

                                /* Obtain coordinates from timehistory subrecords */
                                if (map_timehist_coords)
                                {
                                    xx[j] = new_nodes[nd][0];
                                    yy[j] = new_nodes[nd][1];
                                    zz[j] = new_nodes[nd][2];
                                }
                                else
                                {
                                    xx[j] = nodes3d[nd][0];
                                    yy[j] = nodes3d[nd][1];
                                    zz[j] = nodes3d[nd][2];
                                }
                            }
                        }
                        else
                        {
                            if ( single_prec_vel )
                            {
                                /* Double prec pos, single prec vel */
                                for ( j = 0; j < 8; j++ )
                                {
                                    nd = connects[i][j];
                                    if (map_timehist_coords)
                                    {
                                        if ( obj_ids )
                                        {
                                            elem_index = obj_ids[i];
                                            nd = connects[elem_index][j];
                                        }
                                    }

                                    xv[j] = (double) nxv[nd];
                                    yv[j] = (double) nyv[nd];
                                    zv[j] = (double) nzv[nd];

                                    /* Obtain coordinates from timehistory subrecords */
                                    if (map_timehist_coords)
                                    {
                                        xx[j] = new_nodes[nd][0];
                                        yy[j] = new_nodes[nd][1];
                                        zz[j] = new_nodes[nd][2];
                                    }
                                    else
                                    {
                                        xx[j] = nodes3d2p[nd][0];
                                        yy[j] = nodes3d2p[nd][1];
                                        zz[j] = nodes3d2p[nd][2];
                                    }
                                }
                            }
                            else
                            {
                                /* Double prec pos, double prec vel */
                                for ( j = 0; j < 8; j++ )
                                {
                                    nd = connects[i][j];
                                    if (map_timehist_coords)
                                    {
                                        if ( obj_ids )
                                        {
                                            elem_index = obj_ids[i];
                                            nd = connects[elem_index][j];
                                        }
                                    }

                                    xv[j] = nxv_2p[nd];
                                    yv[j] = nyv_2p[nd];
                                    zv[j] = nzv_2p[nd];

                                    /* Obtain coordinates from timehistory subrecords */
                                    if (map_timehist_coords)
                                    {
                                        xx[j] = new_nodes[nd][0];
                                        yy[j] = new_nodes[nd][1];
                                        zz[j] = new_nodes[nd][2];
                                    }
                                    else
                                    {
                                        xx[j] = nodes3d2p[nd][0];
                                        yy[j] = nodes3d2p[nd][1];
                                        zz[j] = nodes3d2p[nd][2];
                                    }
                                }
                            }
                        }
                    }

                    memset( F, 0, sizeof(double)*9 );

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
                        detF = F[0]*F[4]*F[8] + F[1]*F[5]*F[6]
                               + F[2]*F[3]*F[7]
                               - F[2]*F[4]*F[6] - F[1]*F[3]*F[8]
                               - F[0]*F[5]*F[7];
                        detF = fabs( detF );
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
                        detF = F[0]*F[4]*F[8] + F[1]*F[5]*F[6]
                               + F[2]*F[3]*F[7]
                               - F[2]*F[4]*F[6] - F[1]*F[3]*F[8]
                               - F[0]*F[5]*F[7];
                        detF = fabs( detF );
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
                        detF = F[0]*F[4]*F[8] + F[1]*F[5]*F[6]
                               + F[2]*F[3]*F[7]
                               - F[2]*F[4]*F[6] - F[1]*F[3]*F[8]
                               - F[0]*F[5]*F[7];
                        detF = fabs( detF );
                        break;
                    }

                    extract_strain_vec( eps, F, analy->strain_variety );

                    if ( ref_frame == GLOBAL )
                    {
                        if ( analy->do_tensor_transform )
                            if ( res_index >= 0 && res_index <= 5 )
                                transform_tensors( 1, (double (*)[6]) &eps,
                                                   analy->tensor_transform_matrix );
                    }
                    else if ( ref_frame == LOCAL )
                    {
                        if ( res_index >= 0 && res_index <= 5 )
                        {
                            hex_g2l_mtx( analy, p_hex_class, i, 0, 1, 3,
                                         localMat );
                            transform_tensors( 1, (double (*)[6]) &eps,
                                               localMat );
                        }
                    }

                    elem_index = i;

                    if ( obj_ids )
                    {
                        if (map_timehist_coords && obj_ids[i] == -1)
                            continue;
                        else
                            elem_index = obj_ids[i];
                    }

                    switch( res_index )
                    {
                    case 0:
                        resultElem[elem_index] = (float) eps[0];
                        break;
                    case 1:
                        resultElem[elem_index] = (float) eps[1];
                        break;
                    case 2:
                        resultElem[elem_index] = (float) eps[2];
                        break;
                    case 3:
                        resultElem[elem_index] = (float) eps[3];
                        break;
                    case 4:
                        resultElem[elem_index] = (float) eps[4];
                        break;
                    case 5:
                        resultElem[elem_index] = (float) eps[5];
                        break;
                    case 6:
                        hex_principal_strain( eps, &meanStrain );
                        resultElem[elem_index] = (float) eps[0];
                        break;
                    case 7:
                        hex_principal_strain( eps, &meanStrain );
                        resultElem[elem_index] = (float) eps[1];
                        break;
                    case 8:
                        hex_principal_strain( eps, &meanStrain );
                        resultElem[elem_index] = (float) eps[2];
                        break;
                    case 9:
                        hex_principal_strain( eps, &meanStrain );
                        resultElem[elem_index] = (float) (eps[0] - eps[2]);
                        break;
                    case 10:
                        hex_principal_strain( eps, &meanStrain );
                        resultElem[elem_index] = (float) (eps[0] - meanStrain);
                        break;
                    case 11:
                        hex_principal_strain( eps, &meanStrain );
                        resultElem[elem_index] = (float) (eps[1] - meanStrain);
                        break;
                    case 12:
                        hex_principal_strain( eps, &meanStrain );
                        resultElem[elem_index] = (float) (eps[2] - meanStrain);
                        break;
                    case 13:
                        resultElem[elem_index] = (float) eps[3] * 2.0;
                        break;
                    case 14:
                        resultElem[elem_index] = (float) eps[4] * 2.0;
                        break;
                    case 15:
                        resultElem[elem_index] = (float) eps[5] * 2.0;
                        break;
                    default:
                        popup_dialog( WARNING_POPUP,
                                      "Unknown strain result requested!" );
                    }

                } /* elements in block loop */
            } /* element block in active material loop */
        } /* active material in hex class loop */

        if ( analy->ei_result || (interpolate && active_qty > 0) )
            /* Interpolate to the nodes by class and active materials. */
            hex_to_nodal_by_mat( resultElem, resultArr, p_hex_class,
                                 active_qty, active_mats, p_mats, analy );
    } /* hex class loop */

    free( active_mats );

    p_result->modifiers.use_flags.use_strain_variety = 1;
    p_result->modifiers.strain_variety = analy->strain_variety;
    if ( analy->do_tensor_transform
            && res_index >= 0 && res_index <= 5 )
        p_result->modifiers.use_flags.coord_transform = 1;
    if (  res_index >= 0 && res_index <= 5 )
    {
        p_result->modifiers.use_flags.use_ref_frame = 1;
        p_result->modifiers.ref_frame = analy->ref_frame;
    }

    if (!analy->stateDB && analy->cur_state>0)
    {
        free(obj_ids);
        free(new_nodes);
    }
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
hex_partial_deriv( double dNx[8], double dNy[8], double dNz[8], double coorX[8],
                   double coorY[8], double coorZ[8] )
{
    /* Local shape function derivatives evaluated at node points. */
    static double dN1[8] = { -.125, -.125, .125, .125,
                             -.125, -.125, .125, .125
                           };
    static double dN2[8] = { -.125, -.125, -.125, -.125,
                             .125, .125, .125, .125
                           };
    static double dN3[8] = { -.125, .125, .125, -.125,
                             -.125, .125, .125, -.125
                           };
    double jacob[9], invJacob[9], detJacob;
    int k;

    memset( jacob, 0, sizeof(double)*9 );
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

    if ( fabs( detJacob ) < 1.0e-20 )
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
 * Calculates the strain components as double precision.
 */
static void
extract_strain_vec( double *strain, double F[9], Strain_type s_type )
{
    switch ( s_type )
    {
    case INFINITESIMAL:
        strain[0] = F[0] - 1.0;
        strain[1] = F[4] - 1.0;
        strain[2] = F[8] - 1.0;
        strain[3] = 0.5 * ( F[1] + F[3] );
        strain[4] = 0.5 * ( F[5] + F[7] );
        strain[5] = 0.5 * ( F[2] + F[6] );
        break;
    case RATE:
        strain[0] = F[0];
        strain[1] = F[4];
        strain[2] = F[8];
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
 * If strain variety is RATE, uses central differences to
 * compute the rate of result "eeff", else just reads and
 * passes back primal data.
 */
void
compute_hex_eff_strain( Analysis *analy,float *resultArr, Bool_type interpolate )
{
    int cur_st, obj_qty, index, subrec, st_num, last_st;
    int i;
    float *e0, *e1, *e2, *resultElem;
    float  time0, time1, time2;
    double dt1_inv, dt2_inv;
    int *object_ids;
    Result *p_result;
    Subrec_obj *p_subrec;

    p_result = analy->cur_result;
    index = analy->result_index;
    subrec = p_result->subrecs[index];
    p_subrec = analy->srec_tree[p_result->srec_id].subrecs + subrec;
    cur_st = analy->cur_state;
    obj_qty = p_subrec->subrec.qty_objects;
    object_ids = p_subrec->object_ids;
    resultElem = p_subrec->p_object_class->data_buffer;

    e0 = ( object_ids == NULL ) ? resultElem : analy->tmp_result[2];

    if ( analy->strain_variety != RATE )
        analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec,
                               1, p_result->primals[index], (void *) e0 );
    else
    {
        e1 = analy->tmp_result[0];
        e2 = analy->tmp_result[1];

        if ( cur_st == 0 )
            /* Rate is zero at first state. */
            memset( e0, 0, obj_qty * sizeof( float ) );
        else
        {
            /* Calculate rate from backward difference. */

            analy->db_get_results( analy->db_ident, analy->cur_state,
                                   subrec, 1, p_result->primals[index],
                                   (void *) e0 );
            analy->db_get_results( analy->db_ident, analy->cur_state + 1,
                                   subrec, 1, p_result->primals[index],
                                   (void *) e1 );

            st_num = analy->cur_state;
            analy->db_query( analy->db_ident, QRY_STATE_TIME, (void *) &st_num,
                             NULL, (void *) &time0 );
            st_num++;
            analy->db_query( analy->db_ident, QRY_STATE_TIME, (void *) &st_num,
                             NULL, (void *) &time1 );

            dt1_inv = 1.0 / ((double) time1 - time0);

            for ( i = 0; i < obj_qty; i++ )
                e0[i] = ((double) e1[i] - e0[i]) * dt1_inv;

            /*
             * Need last state.  Don't use get_max_state() since we don't
             * need to consider any max state constraint the user may
             * have specified.
             */
            analy->db_query( analy->db_ident, QRY_QTY_STATES, NULL, NULL,
                             &last_st );
            last_st--;

            if ( cur_st != last_st )
            {
                /*
                 * Not at last state, so calculate rate from forward
                 * difference and average to get final value.
                 * (Time steps are not constant so this is not the
                 * simpler typical central difference expression.)
                 */
                analy->db_get_results( analy->db_ident, analy->cur_state + 2,
                                       subrec, 1, p_result->primals[index],
                                       (void *) e2 );

                st_num++;
                analy->db_query( analy->db_ident, QRY_STATE_TIME,
                                 (void *) &st_num, NULL, (void *) &time2 );
                dt2_inv = 1.0 / ((double) time2 - time1);

                for ( i = 0; i < obj_qty; i++ )
                    e0[i] = (e0[i] + ((double) e2[i] - e1[i]) * dt2_inv) * 0.5;
            }
        }
    }

    if ( object_ids )
    {
        for ( i = 0; i < obj_qty; i++ )
            resultElem[object_ids[i]] = e0[i];
    }

    if ( interpolate )
        hex_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                      object_ids, analy );

    /*
     * Strain type = RATE is not actually supported for this result, but
     * we perform the time derivative when that is set; set the
     * appropriate use flag.
     */
    if ( analy->strain_variety == RATE )
        p_result->modifiers.use_flags.time_derivative = 1;
}


/************************************************************
 * TAG( compute_hex_relative_volume )
 *
 * Computes the relative volume and the volumetric strain.
 *
 * Since this hex result is calculated from nodal primal data, all
 * the comments in compute_hex_strain() above apply.
 */
void
compute_hex_relative_volume( Analysis *analy, float *resultArr,
                             Bool_type interpolate )
{
    int i, j, l, m, n;
    double x[8], y[8], z[8];         /* Initial element geom. */
    double xx[8], yy[8], zz[8];      /* Current element geom. */
    double dNx[8], dNy[8], dNz[8];   /* Global derivates dN/dx, dN/dy, dN/dz. */
    double F[9];                     /* Element deformation gradient. */
    double detF;                     /* Determinant of element
                                       deformation gradient. */
    float *resultElem;              /* Array for the element data. */
    register Bool_type vol_strain;
    int (*connects)[8];
    Result *p_result;
    int nd, active_qty, mat_qty, class_qty, elem_block_qty;
    int start, stop;
    GVec3D *nodes3d, *onodes3d;
    GVec3D2P *nodes3d2p, *onodes3d2p;
    MO_class_data *p_hex_class, *p_mat_class;
    Material_data *p_mats;
    unsigned char *disable_mat;
    int *active_mats;
    int cur_mat;
    Bool_type single_prec_pos;
    State_rec_obj *p_sro;
    Bool particle_class = FALSE;

    /* Get node position data precision. */
    single_prec_pos = !(MESH_P( analy )->double_precision_nodpos);

    p_result = analy->cur_result;
    vol_strain = ( strcmp( p_result->name, "evol" ) == 0 ) ? TRUE : FALSE;

    if ( single_prec_pos )
    {
        nodes3d = analy->state_p->nodes.nodes3d;
        onodes3d = MESH( analy ).node_geom->objects.nodes3d;
    }
    else
    {
        /*
         * Load and save node positions for the first state, which is the
         * reference state for this calculation in double precision.
         */
        if ( reference_nodes == NULL )
            load_reference_node_pos( analy );

        onodes3d2p = reference_nodes;

        /*
         * Read in the double precision coordinates at the current state.
         * Can't use analy->state_p->nodes because that is converted to single
         * precision immediately when read in at state change.
         */
        p_sro = analy->srec_tree + analy->state_p->srec_id;
        load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                     analy->cur_state + 1, FALSE,
                     (void *) analy->tmp_result[0] );
        nodes3d2p = (GVec3D2P *) analy->tmp_result[0];
    }

    /* Prepare access to material data. */
    p_mat_class = ((MO_class_data **)
                   MESH( analy ).classes_by_sclass[G_MAT].list)[0];
    p_mats = p_mat_class->objects.materials;
    disable_mat = MESH( analy ).disable_material;
    mat_qty = analy->max_mesh_mat_qty;
    active_mats = NEW_N( int, mat_qty, "Active materials array" );

    /* Loop over all hex classes to generate results for enabled materials. */
    class_qty = MESH( analy ).classes_by_sclass[G_HEX].qty;

    for ( l = 0; l < class_qty; l++ )
    {
        p_hex_class = ((MO_class_data **)
                       MESH( analy ).classes_by_sclass[G_HEX].list)[l];

        particle_class = FALSE;
        if (is_particle_class( analy, p_hex_class->superclass, p_hex_class->short_name) )
            particle_class = TRUE;

        connects = (int (*)[8]) p_hex_class->objects.elems->nodes;
        resultElem = p_hex_class->data_buffer;

        /* Generate the list of valid, enabled materials in this class. */
        active_qty = 0;
        for ( m = 0; m < mat_qty; m++ )
        {
            /* If the material is of the hex class and it's enabled, save it. */
            if ( p_mats[m].elem_class == p_hex_class && !disable_mat[m] )
                active_mats[active_qty++] = m;
        }

        /* Loop over the active materials. */
        for ( m = 0; m < active_qty; m++ )
        {
            cur_mat = active_mats[m];
            elem_block_qty = p_mats[cur_mat].elem_block_qty;

            /* Loop over the element blocks of the current active material. */
            for ( n = 0; n < elem_block_qty; n++ )
            {
                start = p_mats[cur_mat].elem_blocks[n][0];
                stop = p_mats[cur_mat].elem_blocks[n][1];

                if ( class_qty>1 && stop>=p_hex_class->qty )
                {
                    start = 0;
                    stop  = p_hex_class->qty-1;
                }

                /* Loop over the elements in the current element block. */
                for ( i = start; i <= stop; i++ )
                {
                    if ( particle_class )
                    {
                        resultElem[i] = 0.0;
                        continue;
                    }

                    /* Get the initial and current geometries. */
                    if ( single_prec_pos )
                    {
                        for ( j = 0; j < 8; j++ )
                        {
                            nd = connects[i][j];
                            x[j] = (double) onodes3d[nd][0];
                            y[j] = (double) onodes3d[nd][1];
                            z[j] = (double) onodes3d[nd][2];
                            xx[j] = (double) nodes3d[nd][0];
                            yy[j] = (double) nodes3d[nd][1];
                            zz[j] = (double) nodes3d[nd][2];
                        }
                    }
                    else
                    {
                        for ( j = 0; j < 8; j++ )
                        {
                            nd = connects[i][j];
                            x[j] = onodes3d2p[nd][0];
                            y[j] = onodes3d2p[nd][1];
                            z[j] = onodes3d2p[nd][2];
                            xx[j] = nodes3d2p[nd][0];
                            yy[j] = nodes3d2p[nd][1];
                            zz[j] = nodes3d2p[nd][2];
                        }
                    }

                    memset(F, 0, sizeof(double)*9);

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
                    resultElem[i] = (float) (vol_strain ? log( detF ) : detF);

                } /* for element in block */
            } /* for element block in material */
        } /* for active material */

        if ( interpolate )
            /* Interpolate to the nodes by class and active materials. */
            hex_to_nodal_by_mat( resultElem, resultArr, p_hex_class,
                                 active_qty, active_mats, p_mats, analy );
    } /* for hex element class */

    free( active_mats );
}


/************************************************************
 * TAG( hex_principal_strain )
 *
 * Computes the principal strain for an element given
 * the strain tensor computed in one of the selected
 * reference configurations.
 */
static void
hex_principal_strain( double strainVec[6], double *avgStrain )
{
    double devStrain[3];
    double Invariant[3];
    double alpha, angle, value, avg_e;

    avg_e = - ( strainVec[0] + strainVec[1] + strainVec[2] )
            * ONETHIRD;

    devStrain[0] = strainVec[0] + avg_e;
    devStrain[1] = strainVec[1] + avg_e;
    devStrain[2] = strainVec[2] + avg_e;

    /* Calculate invariants of deviatoric tensor. */
    /* Invariant[0] = 0.0 */
    Invariant[0] = devStrain[0] + devStrain[1] + devStrain[2];
    Invariant[1] = -( devStrain[0] * devStrain[1] +
                      devStrain[1] * devStrain[2] +
                      devStrain[0] * devStrain[2] ) +
                   SQR( strainVec[3] ) + SQR( strainVec[4] ) +
                   SQR( strainVec[5] );
    Invariant[2] = -devStrain[0] * devStrain[1] * devStrain[2]
                   - 2.0 * strainVec[3] * strainVec[4] * strainVec[5]
                   + devStrain[0] * SQR( strainVec[4] )
                   + devStrain[1] * SQR( strainVec[5] )
                   + devStrain[2] * SQR( strainVec[3] ) ;

    /* Check to see if we can have non-zero divisor, if not
     * set principal stress to 0.
     */
    if ( Invariant[1] >= 1e-12 )
    {
        alpha = -0.5 * sqrt( 27.0 / Invariant[1] )
                * Invariant[2] / Invariant[1];
        alpha = BOUND( -1.0, alpha, 1.0 );
        angle = acos( alpha )  * ONETHIRD;
        value = 2.0 * sqrt( Invariant[1] * ONETHIRD );
        strainVec[0] = value * cos( angle );
        angle = angle - 2.0 * PI * ONETHIRD;
        strainVec[1] = value * cos( angle );
        angle = angle + 4.0 * PI * ONETHIRD;
        strainVec[2] = value * cos( angle );
    }
    else
    {
        strainVec[0] = 0.0;
        strainVec[1] = 0.0;
        strainVec[2] = 0.0;
    }

    *avgStrain = avg_e;
}

/************************************************************
 * TAG( load_reference_node_pos )
 *
 * Read node positions for the _strain_ reference state into
 * memory (not to be confused with the reference state for
 * displacement calculations).
 *
 * When/if double precision nodal positions in the mesh definition
 * are available, this logic and its utilization should be
 * updated to "do the right thing".
 */
static void
load_reference_node_pos( Analysis *analy )
{
    int i, rval, qty;
    int srec_id, mesh_id;
    Mesh_data *p_md;
    State_rec_obj *p_sro;

    /* Sanity check. */
    if ( reference_nodes != NULL )
    {
        popup_dialog( WARNING_POPUP, "Global pointer for node "
                      "position\nreference state non-NULL in "
                      "load_reference_node_pos();\naborting" );
        return;
    }

    /*
     * Get srec format and mesh_id for the first state, which,
     * however unlikely, could be different than those for the
     * current state.
     */
    i = 1;
    rval = analy->db_query( analy->db_ident, QRY_SREC_FMT_ID,
                            (void *) &i, NULL, (void *) &srec_id );
    if ( rval != 0 )
    {
        popup_dialog( WARNING_POPUP, "Database query (srec id) failed "
                      "during reference nodpos load; aborted" );
        return;
    }

    rval = analy->db_query( analy->db_ident, QRY_SREC_MESH,
                            (void *) &srec_id, NULL,
                            (void *) &mesh_id );
    if ( rval != 0 )
    {
        popup_dialog( WARNING_POPUP, "Database query (mesh id) failed "
                      "during reference nodpos load; aborted" );
        return;
    }

    p_sro = analy->srec_tree + srec_id;
    p_md = analy->mesh_table + mesh_id;

    qty = p_md->node_geom->qty;
    reference_nodes = NEW_N( GVec3D2P, qty,
                             "Double prec ref. nodpos buffer" );

    load_nodpos( analy, p_sro, p_md, analy->dimension, 1, FALSE,
                 (void *) reference_nodes );
}


/************************************************************
 * TAG( compute_shell_strain )
 *
 * Computes the shell strain given the INNER/MIDDLE/OUTER flag
 * and the GLOBAL/LOCAL flag.  Defaults: MIDDLE, GLOBAL.
 */
void
compute_shell_strain( Analysis *analy, float *resultArr, Bool_type interpolate )
{
    Ref_surf_type ref_surf;
    Ref_frame_type ref_frame;
    float *resultElem, *res1, *res2;
    float eps[6];
    double (*p_tensors)[6];
    float localMat[3][3];
    int comp_idx, i, j, obj_qty, obj_id, obj_index,
        index;
    int srec, subrec;
    char **comp_names;
    char **primals;
    Htable_entry *p_hte;
    MO_class_data *p_quad_class;
    Result *p_result;
    Subrec_obj *p_subrec;
    int *object_ids;
    char *primal_list[2];
    char primal1[32], primal2[32];
    Bool_type engr_strain;

    /* IRC: Added February 22, 2008 - Support strain
     *      calculations for timehistory databases
     */
    Bool_type map_timehist_coords = FALSE;
    int       obj_cnt, obj_num, *obj_ids=NULL;
    GVec3D2P  *new_nodes;
    Bool_type single_prec_pos;

    /*
     * The result_id is EPSX, EPSY, EPSZ, EPSXY, EPSYZ, or EPSZX.
     * These can be loaded directly from the database.
     */

    p_result = analy->cur_result;
    index = analy->result_index;
    primals = p_result->primals[index];
    subrec = p_result->subrecs[index];
    srec = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    p_quad_class = p_subrec->p_object_class;
    object_ids = p_subrec->object_ids;
    obj_qty = p_subrec->subrec.qty_objects;
    resultElem = p_quad_class->data_buffer;

    if (!analy->stateDB)
        map_timehist_coords = TRUE;

    ref_surf = analy->ref_surf;
    ref_frame = analy->ref_frame;

    /* Determine a result index. */
    if ( strcmp( p_result->name, "ex" ) == 0 )
    {
        comp_idx = 0;
        engr_strain = FALSE;
    }
    else if ( strcmp( p_result->name, "ey" ) == 0 )
    {
        comp_idx = 1;
        engr_strain = FALSE;
    }
    else if ( strcmp( p_result->name, "ez" ) == 0 )
    {
        comp_idx = 2;
        engr_strain = FALSE;
    }
    else if ( strcmp( p_result->name, "exy" ) == 0 )
    {
        comp_idx = 3;
        engr_strain = FALSE;
    }
    else if ( strcmp( p_result->name, "eyz" ) == 0 )
    {
        comp_idx = 4;
        engr_strain = FALSE;
    }
    else if ( strcmp( p_result->name, "ezx" ) == 0 )
    {
        comp_idx = 5;
        engr_strain = FALSE;
    }
    else if ( strcmp( p_result->name, "gamxy" ) == 0 )
    {
        comp_idx = 3;
        engr_strain = TRUE;
    }
    else if ( strcmp( p_result->name, "gamyz" ) == 0 )
    {
        comp_idx = 4;
        engr_strain = TRUE;
    }
    else if ( strcmp( p_result->name, "gamzx" ) == 0 )
    {
        comp_idx = 5;
        engr_strain = TRUE;
    }

    /* Get state variable to provide array of component names. */
    htable_search( analy->st_var_table, primals[0], FIND_ENTRY,
                   &p_hte );
    comp_names = ((State_variable *) p_hte->data)->components;

    primal_list[0] = primal1;
    primal_list[1] = primal2;

    /* IRC: Added March 29, 2005 - Support strain
     *      calculations for timehistory databases
     */
    if (map_timehist_coords)
    {
        /* Save current result */
        p_result = analy->cur_result;

        /* Get node position data precision. */
        single_prec_pos = !(MESH_P( analy )->double_precision_nodpos);

        load_quad_nodpos_timehist( analy, analy->cur_state+1, single_prec_pos,
                                   &obj_cnt, &obj_ids, &new_nodes);

        analy->cur_result = p_result;
    }

    if ( ref_frame == GLOBAL )
    {
        if ( analy->do_tensor_transform )
        {
            switch ( ref_surf )
            {
            case MIDDLE:
                /* Get transformed inner surface strain. */
                res1 = NEW_N( float, obj_qty, "Temp shell stress" );
                transform_stress_strain( primals, 0, analy,
                                         analy->tensor_transform_matrix,
                                         res1 );

                /* Get transformed outer surface strain. */
                res2 = analy->tmp_result[6];
                transform_stress_strain( primals, 1, analy,
                                         analy->tensor_transform_matrix,
                                         res2 );

                if ( object_ids == NULL )
                    for ( i = 0; i < obj_qty; i++ )
                        resultElem[i] = 0.5 * ( (double) res1[i] + res2[i]);
                else
                {
                    for ( i = 0; i < obj_qty; i++ )
                        resultElem[object_ids[i]] = 0.5
                                                    * ( (double) res1[i]
                                                        + res2[i]);
                }

                free( res1 );

                break;
            case INNER:
                res2 = ( object_ids == NULL )
                       ? resultElem : analy->tmp_result[6];
                transform_stress_strain( primals, 0, analy,
                                         analy->tensor_transform_matrix,
                                         res2 );
                break;
            case OUTER:
                res2 = ( object_ids == NULL )
                       ? resultElem : analy->tmp_result[6];
                transform_stress_strain( primals, 1, analy,
                                         analy->tensor_transform_matrix,
                                         res2 );
                break;
            }
        }
        else
        {
            switch ( ref_surf )
            {
            case MIDDLE:
                sprintf( primal1, "%s[%s]", primals[0],
                         comp_names[comp_idx] );
                sprintf( primal2, "%s[%s]", primals[1],
                         comp_names[comp_idx] );
                res1 = analy->tmp_result[0];
                res2 = res1 + obj_qty;
                analy->db_get_results( analy->db_ident,
                                       analy->cur_state + 1, subrec,
                                       2, primal_list, res1 );

                if ( object_ids == NULL )
                    for ( i = 0; i < obj_qty; i++ )
                        resultElem[i] = 0.5 * ((double) res1[i] + res2[i]);
                else
                {
                    for ( i = 0; i < obj_qty; i++ )
                        resultElem[object_ids[i]] = 0.5
                                                    * ( (double) res1[i]
                                                        + res2[i]);
                }
                break;
            case INNER:
                sprintf( primal1, "%s[%s]", primals[0],
                         comp_names[comp_idx] );
                res2 = ( object_ids == NULL )
                       ? resultElem : analy->tmp_result[6];
                analy->db_get_results( analy->db_ident,
                                       analy->cur_state + 1, subrec,
                                       1, primal_list, res2 );
                break;
            case OUTER:
                sprintf( primal1, "%s[%s]", primals[1],
                         comp_names[comp_idx] );
                res2 = ( object_ids == NULL )
                       ? resultElem : analy->tmp_result[6];
                analy->db_get_results( analy->db_ident,
                                       analy->cur_state + 1, subrec,
                                       1, primal_list, res2 );
                break;
            }
        }

        /* Put data into class order if necessary. */
        if ( ref_surf != MIDDLE && object_ids != NULL )
            for ( i = 0; i < obj_qty; i++ )
                resultElem[object_ids[i]] = res2[i];
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
            /*
             * Get individual components sequentially.  Could get
             * whole vectors in one access, but would need to allocate
             * another large array to have both INNER and OUTER surface
             * results in memory simultaneously.
             */
            for ( i = 0; i < 6; i++ )
            {
                res1 = analy->tmp_result[i];
                res2 = res1 + obj_qty;

                sprintf( primal1, "%s[%s]", primals[0], comp_names[i] );
                sprintf( primal2, "%s[%s]", primals[1], comp_names[i] );

                analy->db_get_results( analy->db_ident,
                                       analy->cur_state + 1, subrec,
                                       2, primal_list, res1 );
                for ( j = 0; j < obj_qty; j++ )
                    res1[j] = 0.5 * ((double) res1[j] + res2[j]);
            }

            /*
             * Transform MIDDLE result here.  Above logic creates six
             * sequential, result-ordered component arrays in
             * analy->tmp_result.  Logic below for INNER and OUTER
             * surfaces leaves the components object-ordered which can
             * be transposed without the copying step performed here.
             */
            obj_index = 0;
            for ( i = 0;
                    i < obj_qty;
                    i++ )
            {
                obj_id = ( object_ids == NULL ) ? i : object_ids[i];
                if ( map_timehist_coords )
                {
                    for ( j = 0; j < 6; j++ )
                        eps[j] = analy->tmp_result[j][i];
                }
                else
                {
                    for ( j = 0; j < 6; j++ )
                        eps[j] = analy->tmp_result[j][i];
                }

                global_to_local_mtx( analy, p_quad_class, obj_id,
                                     map_timehist_coords, new_nodes,
                                     localMat );
                transform_tensors_1p( 1, (float (*)[6]) &eps, localMat );
                resultElem[obj_id] = eps[comp_idx];
            }
            break;
        case INNER:
            /* Get individual components sequentially.  Could get
                 * whole vectors in one access, but would need to allocate
                 * another large array to have both INNER and OUTER surface
                 * results in memory simultaneously.
                 */

            for ( i = 0; i < 6; i++ )
            {
                res1 = analy->tmp_result[i];
                res2 = res1 + obj_qty;

                sprintf( primal1, "%s[%s]", primals[0], comp_names[i] );

                analy->db_get_results( analy->db_ident,
                                       analy->cur_state + 1, subrec,
                                       1, primal_list, res1 );
            }

            /*
             * Transform INNER result here.  Above logic creates six
             * sequential, result-ordered component arrays in
             * analy->tmp_result.
             */
            obj_index = 0;
            for ( i = 0;
                    i < obj_qty;
                    i++ )
            {
                obj_id = ( object_ids == NULL ) ? i : object_ids[i];

                if ( map_timehist_coords )
                {
                    for ( j = 0; j < 6; j++ )
                        eps[j] = analy->tmp_result[j][i];
                }
                else
                {
                    for ( j = 0; j < 6; j++ )
                        eps[j] = analy->tmp_result[j][i];
                }

                global_to_local_mtx( analy, p_quad_class, obj_id,
                                     map_timehist_coords, new_nodes,
                                     localMat );
                transform_tensors_1p( 1, (float (*)[6]) &eps, localMat );
                resultElem[obj_id] = eps[comp_idx];
            }

            /* OLD inner - IRC analy->db_get_results( analy->db_ident,
                                       analy->cur_state + 1, subrec,
                                       1, primals, analy->tmp_result[0] ); */
            break;
        case OUTER:
            /* Get individual components sequentially.  Could get
             * whole vectors in one access, but would need to allocate
             * another large array to have both INNER and OUTER surface
             * results in memory simultaneously.
             */
            for ( i = 0; i < 6; i++ )
            {
                res1 = analy->tmp_result[i];
                res2 = res1 + obj_qty;

                sprintf( primal1, "%s[%s]", primals[1], comp_names[i] );

                analy->db_get_results( analy->db_ident,
                                       analy->cur_state + 1, subrec,
                                       1, primal_list, res1 );
            }

            /*
             * Transform OUTER result here.  Above logic creates six
             * sequential, result-ordered component arrays in
             * analy->tmp_result
             */
            obj_index = 0;
            for ( i = 0;
                    i < obj_qty;
                    i++ )
            {
                obj_id = ( object_ids == NULL ) ? i : object_ids[i];
                if ( map_timehist_coords )
                {
                    for ( j = 0; j < 6; j++ )
                        eps[j] = analy->tmp_result[j][i];
                }
                else
                {
                    for ( j = 0; j < 6; j++ )
                        eps[j] = analy->tmp_result[j][i];
                }

                global_to_local_mtx( analy, p_quad_class, obj_id,
                                     map_timehist_coords, new_nodes,
                                     localMat );
                transform_tensors_1p( 1, (float (*)[6]) &eps, localMat );
                resultElem[obj_id] = eps[comp_idx];
            }
            break;
        }

        if ( ref_surf != MIDDLE )
        {
            /* Transform INNER and OUTER surface results. */

            /* p_tensors = (double (*)[6]) analy->tmp_result[0];

            obj_id = 0;
            for ( i = 0; i < obj_qty; i++ )
            {
                obj_id = ( object_ids == NULL ) ? i : object_ids[i];

                global_to_local_mtx( analy, p_quad_class, obj_id,
                             map_timehist_coords, new_nodes,
            	     localMat );
                transform_tensors( 1, p_tengriz4ssors + i, localMat );

                resultElem[obj_id] = p_tensors[i][comp_idx];
            } */
        }
    }

    /* Perform the Engineering Strain conversion if necessary. */
    if ( engr_strain )
    {
        /*
         * If data was from a "non-proper" subrecord, the object quantity
         * must be updated from the subrecord size to the class size
         * because it's now in class-order.
         */
        if ( object_ids != NULL )
            obj_qty = p_quad_class->qty;

        for ( i = 0; i < obj_qty; i++ )
            resultElem[i] = (double) 2.0 * resultElem[i];
    }

    /* Update modifiers to indicate pertinent ones for this result. */
    p_result->modifiers.use_flags.use_ref_frame = 1;
    p_result->modifiers.ref_frame = analy->ref_frame;
    p_result->modifiers.use_flags.use_ref_surface = 1;
    p_result->modifiers.ref_surf = analy->ref_surf;

    if ( interpolate )
        quad_to_nodal( resultElem, resultArr, p_quad_class, obj_qty, object_ids,
                       analy, TRUE );

    p_result->modifiers.use_flags.use_ref_surface = 1;
    p_result->modifiers.ref_surf = ref_surf;
    p_result->modifiers.use_flags.use_ref_frame = 1;
    p_result->modifiers.ref_frame = ref_frame;
    if ( ref_frame == GLOBAL && analy->do_tensor_transform )
        p_result->modifiers.use_flags.coord_transform = 1;
}


/************************************************************
 * TAG( compute_shell_eff_strain )
 *
 * Computes the shell effective plastic strain given the
 * INNER/MIDDLE/OUTER flag.  Default: MIDDLE.
 */
void
compute_shell_eff_strain( Analysis *analy, float *resultArr,
                          Bool_type interpolate )
{
    Ref_surf_type ref_surf;
    int cur_st, obj_qty, index, subrec, st_num, last_st;
    int i;
    float *e0, *e1, *e2, *resultElem;
    float time0, time1, time2;
    double dt1_inv, dt2_inv;
    int *object_ids;
    Result *p_result;
    Subrec_obj *p_subrec;
    char **primals;

    p_result = analy->cur_result;
    index = analy->result_index;
    subrec = p_result->subrecs[index];
    p_subrec = analy->srec_tree[p_result->srec_id].subrecs + subrec;
    cur_st = analy->cur_state;
    obj_qty = p_subrec->subrec.qty_objects;
    object_ids = p_subrec->object_ids;
    ref_surf = analy->ref_surf;
    resultElem = p_subrec->p_object_class->data_buffer;

    /* Assume primals are ordered "eeff_mid", "eeff_in", "eeff_out". */
    switch ( ref_surf )
    {
    case MIDDLE:
        primals = p_result->primals[index];
        break;
    case INNER:
        primals = p_result->primals[index] + 1;
        break;
    case OUTER:
        primals = p_result->primals[index] + 2;
        break;
    }

    e0 = ( object_ids == NULL ) ? resultElem : analy->tmp_result[2];

    if ( analy->strain_variety != RATE )
        analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec,
                               1, primals, (void *) e0 );
    /*                               1, p_result->primals[index], (void *) e0 ); */
    else
    {
        e1 = analy->tmp_result[0];
        e2 = analy->tmp_result[1];

        if ( cur_st == 0 )
            /* Rate is zero at first state. */
            memset( e0, 0, obj_qty * sizeof( float ) );
        else
        {
            /* Calculate rate from backward difference. */

            analy->db_get_results( analy->db_ident, analy->cur_state,
                                   subrec, 1, primals, (void *) e0 );
            analy->db_get_results( analy->db_ident, analy->cur_state + 1,
                                   subrec, 1, primals, (void *) e1 );

            st_num = analy->cur_state;
            analy->db_query( analy->db_ident, QRY_STATE_TIME, (void *) &st_num,
                             NULL, (void *) &time0 );
            st_num++;
            analy->db_query( analy->db_ident, QRY_STATE_TIME, (void *) &st_num,
                             NULL, (void *) &time1 );

            dt1_inv = 1.0 / ((double) time1 - time0);

            for ( i = 0; i < obj_qty; i++ )
                e0[i] = ((double) e1[i] - e0[i]) * dt1_inv;

            /*
             * Need last state.  Don't use get_max_state() since we don't
             * need to consider any max state constraint the user may
             * have specified.
             */
            analy->db_query( analy->db_ident, QRY_QTY_STATES, NULL, NULL,
                             &last_st );
            last_st--;

            if ( cur_st != last_st )
            {
                /*
                 * Not at last state, so calculate rate from forward
                 * difference and average to get final value.
                 * (Time steps are not constant so this is not the
                 * simpler typical central difference expression.)
                 */
                analy->db_get_results( analy->db_ident, analy->cur_state + 2,
                                       subrec, 1, primals, (void *) e2 );

                st_num++;
                analy->db_query( analy->db_ident, QRY_STATE_TIME,
                                 (void *) &st_num, NULL, (void *) &time2 );
                dt2_inv = 1.0 / ((double) time2 - time1);

                for ( i = 0; i < obj_qty; i++ )
                    e0[i] = 0.5 * (e0[i] + ((double) e2[i] - e1[i]) * dt2_inv);
            }
        }

    }

    if ( object_ids )
    {
        for ( i = 0; i < obj_qty; i++ )
            resultElem[object_ids[i]] = e0[i];
    }

    if ( interpolate )
        quad_to_nodal( resultElem, resultArr, p_subrec->p_object_class, obj_qty,
                       object_ids, analy, TRUE );

    /* Set appropriate modifiers. */
    p_result->modifiers.use_flags.use_ref_surface = 1;
    p_result->modifiers.ref_surf = ref_surf;
    /*
     * Strain type = RATE is not actually supported for this result, but
     * we perform the time derivative when that is set; set the
     * appropriate use flag.
     */
    if ( analy->strain_variety == RATE )
        p_result->modifiers.use_flags.time_derivative = 1;
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
static Bool_type new_beam_parameters = TRUE;

void
compute_beam_axial_strain( Analysis *analy, float *resultArr,
                           Bool_type interpolate )
{
    float *resultElem, *res_array;
    static char d_str[64] = "Ax beam strain, dia =             ";
    static char ym_str[32] = ", Young's Mod =              ";
    int res_index, i, index, subrec, obj_qty;
    int *object_ids;
    double coeff, axf_coeff, mom_coeff;
    float *force, *moment;
    Result *p_result;
    Subrec_obj *p_subrec;
    MO_class_data *p_mo_class;

    p_result = analy->cur_result;
    index = analy->result_index;
    subrec = p_result->subrecs[index];
    p_subrec = analy->srec_tree[p_result->srec_id].subrecs + subrec;
    obj_qty = p_subrec->subrec.qty_objects;
    object_ids = p_subrec->object_ids;
    p_mo_class = p_subrec->p_object_class;
    resultElem = p_subrec->p_object_class->data_buffer;

    if ( new_beam_parameters )
    {
        sprintf( strchr( d_str, '=') + 2, "%.1e\0", diameter);
        sprintf( strchr( ym_str, '=') + 2, "%.1e.\n\0", youngs_modulus);
        strcat( d_str, ym_str);
        wrt_text( d_str );
        wrt_text( "\n" );
        new_beam_parameters = FALSE;
    }

    /* Determine a result index. */
    if ( strcmp( p_result->name, "saep" ) == 0 )
        res_index = 0;
    else if ( strcmp( p_result->name, "saem" ) == 0 )
        res_index = 1;
    else if ( strcmp( p_result->name, "taep" ) == 0 )
        res_index = 2;
    else if ( strcmp( p_result->name, "taem" ) == 0 )
        res_index = 3;

    force = analy->tmp_result[0];
    moment = force + obj_qty;

    res_array = ( object_ids == NULL ) ? resultElem : analy->tmp_result[2];

    analy->db_get_results( analy->db_ident, analy->cur_state + 1, subrec,
                           2, p_result->primals[index], (void *) force );

    coeff = 4.0 / ( PI * (double) youngs_modulus );
    axf_coeff = 1.0 / ( (double) diameter * diameter );
    mom_coeff = 8.0 / ( (double) diameter * diameter * diameter );

    switch( res_index )
    {
    case 0:
    case 2:
        for ( i = 0; i < obj_qty; i++ )
            res_array[i] = coeff * (axf_coeff * force[i]
                                    + mom_coeff * moment[i] );
        break;

    case 1:
    case 3:
        for ( i = 0; i < obj_qty; i++ )
            res_array[i] = coeff * (axf_coeff * force[i]
                                    - mom_coeff * moment[i] );
        break;
    }

    if ( object_ids )
    {
        for ( i = 0; i < obj_qty; i++ )
            resultElem[object_ids[i]] = res_array[i];
    }

    if ( interpolate )
        switch( p_mo_class->superclass ){
        case G_TRUSS:
            truss_to_nodal( resultElem, resultArr, p_subrec->p_object_class,
                            obj_qty, object_ids, analy );
            break;
        case G_BEAM:
            beam_to_nodal( resultElem, resultArr, p_subrec->p_object_class,
                        obj_qty, object_ids, analy );
            break;
        }
}


/************************************************************
 * TAG( set_diameter )
 *
 * Set static global variable diameter for use in beam strain
 * calculation.
 */
void
set_diameter( float diam )
{
    diameter = diam;
    new_beam_parameters = TRUE;
}


/************************************************************
 * TAG( set_youngs_mod )
 *
 * Set static global variable youngs_modulus for use in beam strain
 * calculation.
 */
void
set_youngs_mod( float youngs_mod )
{
    youngs_modulus = youngs_mod;
    new_beam_parameters = TRUE;
}

/************************************************************
 * TAG( is_primal_quad_strain_result )
 *
 *
 * This function will return TRUE if this result is a
 * primal QUAD strain result.
 */
Bool_type
is_primal_quad_strain_result( char *result_name )
{
    if ( strstr( result_name, "strain_in[" ) || strstr( result_name, "strain_out[" ) )
        return ( TRUE );
    else
        return ( FALSE );
}


/************************************************************
 * TAG( rotate_quad_result )
 *
 *
 * This function will calculate a rotated result.
 */
void
rotate_quad_result( Analysis *analy, char *primal,
                    int single_obj_id, float *result )
{
    Htable_entry *p_hte;
    Result     *p_result;
    Subrec_obj *p_subrec;
    MO_class_data *p_quad_class;
    int srec, subrec;
    int i, j, index;
    Ref_frame_type ref_frame;
    int obj_id, obj_qty, *object_ids=NULL;
    float eps[6];                   /* Calculated strain. */
    int comp_idx;
    float localMat[3][3];
    Bool_type engr_strain;
    float *temp_result, *res1[6];
    char primal_main[32], *primal_list[2], *primals[2], primal1[32];
    char *comp_names[10];
    GVec3D2P *new_nodes            = NULL;
    Bool_type  map_timehist_coords = FALSE;
    Bool_type single_prec_pos;
    int status;

    ref_frame = analy->ref_frame;

    /* Exit if we are not looging a local reference frame */
    if ( ref_frame == GLOBAL )
        return;

    if (!analy->stateDB)
        map_timehist_coords = TRUE;

    p_result     = analy->cur_result;
    index        = analy->result_index;
    subrec       = p_result->subrecs[index];
    srec         = p_result->srec_id;
    p_subrec     = analy->srec_tree[srec].subrecs + subrec;
    object_ids   = p_subrec->object_ids;
    p_quad_class = p_subrec->p_object_class;

    if (map_timehist_coords)
    {
        /* Get node position data precision. */
        single_prec_pos = !(MESH_P( analy )->double_precision_nodpos);

        /* Save current result */
        p_result = analy->cur_result;

        load_quad_nodpos_timehist( analy, analy->cur_state+1, single_prec_pos,
                                   &obj_qty, &object_ids, &new_nodes);

        if ( new_nodes==NULL)
        {
            popup_dialog( WARNING_POPUP, "Node Position Required for strain rotation calculation."
                          "Aborting strain rate rotation." );
            return;
        }

        analy->cur_result = p_result;
    }

    obj_qty = p_subrec->subrec.qty_objects;

    p_result->modifiers.use_flags.use_strain_variety = 0;
    p_result->modifiers.ref_frame                    = analy->ref_frame;
    p_result->modifiers.use_flags.use_ref_frame      = 1;

    strcpy( primal_main, p_result->name );

    /* Strip off the sub-name [name] to form the primal name */
    for ( i=0;
            i<strlen(primal_main);
            i++)
        if ( primal_main[i]=='[' )
            primal_main[i]='\0';

    comp_names[0] = (char *) strdup("ex");
    comp_names[1] = (char *) strdup("ey");
    comp_names[2] = (char *) strdup("ez");
    comp_names[3] = (char *) strdup("exy");
    comp_names[4] = (char *) strdup("eyz");
    comp_names[5] = (char *) strdup("ezx");

    /* Determine a result index. */
    if ( strstr( p_result->name, "[ex]" ) )
    {
        comp_idx = 0;
        engr_strain = FALSE;
    }
    else if ( strcmp( p_result->name, "[ey]" ) )
    {
        comp_idx = 1;
        engr_strain = FALSE;
    }
    else if ( strstr( p_result->name, "[ez]" )  )
    {
        comp_idx = 2;
        engr_strain = FALSE;
    }
    else if ( strstr( p_result->name, "[exy]" )  )
    {
        comp_idx = 3;
        engr_strain = FALSE;
    }
    else if ( strstr( p_result->name, "[eyz]" ) )
    {
        comp_idx = 4;
        engr_strain = FALSE;
    }
    else if ( strstr( p_result->name, "[ezx]" ) )
    {
        comp_idx = 5;
        engr_strain = FALSE;
    }
    else if ( strstr( p_result->name, "[gamxy]" ) )
    {
        comp_idx = 3;
        engr_strain = TRUE;
    }
    else if ( strstr( p_result->name, "[gamyz]" ) )
    {
        comp_idx = 4;
        engr_strain = TRUE;
    }
    else if ( strstr( p_result->name, "[gamzx]" ) )
    {
        comp_idx = 5;
        engr_strain = TRUE;
    }

    if ( engr_strain )
    {
        free (comp_names[3]);
        comp_names[3] = (char *) strdup("gamxy");

        free (comp_names[4]);
        comp_names[4] = (char *) strdup("gamyz");

        free (comp_names[5]);
        comp_names[5] = (char *) strdup("gamzx");
    }

    primals[0]     = (char *) strdup(primal);
    primal_list[0] = primal1;
    primal_list[1] = primal1;

    for ( i = 0;
            i < 6;
            i++ )
    {
        sprintf( primal1, "%s[%s]", primal_main, comp_names[i] );

        res1[i] = analy->tmp_result[i];
        analy->db_get_results( analy->db_ident,
                               analy->cur_state + 1, subrec,
                               1, primal_list, res1[i] );

        /* Map object data into full result array */
        if ( object_ids )
        {
            temp_result = NEW_N( float, obj_qty, "Temp shell stress" );
            for ( j = 0;
                    j < obj_qty;
                    j++ )
                temp_result[j] = res1[i][j];

            for ( j = 0;
                    j < obj_qty;
                    j++ )
            {
                res1[i][object_ids[j]] = temp_result[j];
            }
            free( temp_result );
        }
    }

    if ( single_obj_id>=0 )
    {
        for ( j = 0;
                j < 6;
                j++ )
            eps[j] = res1[j][single_obj_id];

        global_to_local_mtx( analy, p_quad_class, single_obj_id,
                             map_timehist_coords, new_nodes,
                             localMat );
        transform_tensors_1p( 1, (float (*)[6]) &eps, localMat );

        *result = eps[comp_idx];
        if ( engr_strain )
            *result*=2.0;
    }
    else
        for (i=0;
                i < obj_qty;
                i++ )
        {
            obj_id = ( object_ids == NULL ) ? i : object_ids[i];

            for ( j = 0;
                    j < 6;
                    j++ )
                eps[j] = res1[j][obj_id];

            global_to_local_mtx( analy, p_quad_class, obj_id,
                                 map_timehist_coords, new_nodes,
                                 localMat );

            transform_tensors_1p( 1, (float (*)[6]) &eps, localMat );

            result[obj_id] = eps[comp_idx];
            if ( engr_strain )
                result[obj_id]*=2.0;
        }
}
