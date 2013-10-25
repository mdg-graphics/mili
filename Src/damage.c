/* $Id$ */
/*
 * damage.c - Computes damage value for each element.
 *
 *      Elsie Pierce &
 *      Bob Corey
 *      Lawrence Livermore National Laboratory
 *      June 22, 2004
 *
 ************************************************************************
 * Modifications:
 *  I. R. Corey - Sept 13, 2004: Add new option "damage_hide" which
 *   controls if damaged elements are displayed.
 *
 *  I. R. Corey - Nov 1, 2012: Changed cutoff variables from int to
 *    float.
 *
 ************************************************************************
 */

#include <stdlib.h>
#include "viewer.h"

extern void update_nodal_min_max( Analysis * );

/* Local routines. */
float *compute_damage(int   qty,
                      float vel_cutoff,float relVol_cutoff,float eeff_cutoff,
                      float *eeff, float *vel, float *relVol);

/*****************************************************************
 * TAG( compute_hex_damage )
 *
 */
void
compute_hex_damage( Analysis *analy, float *resultArr,
                    Bool_type interpolate )
{
    Bool_type single_prec_vel;
    char *vdir;
    char **classes;
    unsigned char *disable_mat;
    int el_id;
    int (*connects)[8];
    int *mats;
    int *active_mats;
    int cur_mat;
    int i, j, k, l, m, n, nd;
    int nplot_rval, eeff_rval;
    int nrvol_rval;
    int x_rval, y_rval, z_rval;
    float vel_cutoff, relVol_cutoff, eeff_cutoff;
    int hex_qty, hex_id;
    int *hex_ids, *object_ids;
    int class_qty, mat_qty, active_qty, elem_block_qty, node_qty;
    int index, start, stop;
    int subrec, srec;
    float *resultElem; /* Array for the element data. */
    float *val_hex1=NULL;
    float *val_hex2=NULL;
    float *vel_sums=NULL;
    float localMat[3][3];
    float *vel_1p = NULL;
    double *vel_2p = NULL;
    Material_data *p_mats;
    Mesh_data *p_mesh;
    MO_class_data **p_hex_classes;
    MO_class_data *p_hex_class, *p_mat_class;
    MO_class_data *p_node_geom;
    Result *nplot = NULL, *eeff = NULL;
    Result *nrvol = NULL;
    Result *velx = NULL, *vely = NULL, *velz = NULL;
    Result *p_result;
    State_rec_obj *p_sro;
    Subrec_obj *p_subrec;

    Visibility_data *p_vd;

    p_result = analy->cur_result;

    vdir          = analy->damage_params.vel_dir;
    vel_cutoff    = analy->damage_params.cut_off[0];
    relVol_cutoff = analy->damage_params.cut_off[1];
    eeff_cutoff   = analy->damage_params.cut_off[2];
    node_qty = MESH_P( analy )->node_geom->qty;

    /* Get nodal velocities. */
    if ( velx == NULL )
    {
        /* Initialize velocity Result structs. */
        velx = NEW_N( Result, 3, "Vel Results for damage calculation" );
        vely = velx + 1;
        velz = velx + 2;

        x_rval = find_result( analy, ALL, TRUE, velx, "nodvel[vx]" );
        y_rval = find_result( analy, ALL, TRUE, vely, "nodvel[vy]" );
        z_rval = find_result( analy, ALL, TRUE, velz, "nodvel[vz]" );

        if ( !x_rval || !y_rval || !z_rval )
        {
            popup_dialog( INFO_POPUP, "%s\n%s",
                          "Velocity not available for damage calculation",
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
         * Already have Results, so just update them.
         * We're assuming that if velocity was available in any state
         * format, it'll be available in all formats.
         */
        update_result( analy, velx );
        update_result( analy, vely );
        update_result( analy, velz );
    }

    /* Load nodal velocities for current state. */

    if( strcmp( vdir, "vx")==0 )
    {
        analy->cur_result = velx;
    }

    if( strcmp( vdir, "vy")==0 )
    {
        analy->cur_result = vely;
    }

    if( strcmp( vdir, "vz")==0 )
    {
        analy->cur_result = velz;
    }

    /* Capture velocity precision. */
    single_prec_vel = ( ((Primal_result *)
                         velx->original_result)->var->num_type
                        == G_FLOAT4
                        ||
                        ((Primal_result *)
                         velx->original_result)->var->num_type
                        == G_FLOAT );

    if ( single_prec_vel )
    {
        vel_1p = analy->tmp_result[0];
        NODAL_RESULT_BUFFER( analy ) = vel_1p;
        load_result( analy, FALSE, FALSE, FALSE );

    }
    else
    {
        if ( vel_2p == NULL )
        {
            /* Allocate arrays for velocities into a static pointer. */
            vel_2p = NEW_N( double, node_qty, "Temp node vel buffers" );
            if ( vel_2p == NULL )
            {
                popup_dialog( WARNING_POPUP, "Array allocation failed; "
                              "aborting damage calculation." );
                return;
            }
        }

        NODAL_RESULT_BUFFER( analy ) = (float *) vel_2p;
        load_result( analy, FALSE, FALSE, FALSE );
    }

    vel_sums = analy->tmp_result[3];

    /* Prepare access to material data. */
    p_mat_class = ((MO_class_data **)
                   MESH( analy ).classes_by_sclass[G_MAT].list)[0];
    p_mats = p_mat_class->objects.materials;
    disable_mat = MESH( analy ).disable_material;
    mat_qty = analy->max_mesh_mat_qty;
    active_mats = NEW_N( int, mat_qty, "Active materials array" );

    /* Loop over all hex classes to generate results for enabled materials. */
    class_qty = MESH( analy ).classes_by_sclass[G_HEX].qty;

    for ( l = 0;
            l < class_qty;
            l++ )
    {
        p_hex_class = ((MO_class_data **)
                       MESH( analy ).classes_by_sclass[G_HEX].list)[l];
        hex_qty = p_hex_class->qty;

        val_hex1 = NEW_N( float, node_qty, "Temp node vel buffers" );
        NODAL_RESULT_BUFFER( analy ) = val_hex1;

        nplot = NEW_N( Result, 1, "nplot Results for damage composite" );
        nplot_rval = find_result( analy, PRIMAL, TRUE, nplot, "nplot" );
        if ( nplot_rval )
        {
            analy->cur_result = nplot;
            load_result( analy, FALSE, FALSE, FALSE );
            for ( i=0;
                    i<hex_qty;
                    i++ )
                val_hex1[i] = p_hex_class->data_buffer[i];
        }
        else
        {
            cleanse_result( nplot );
            free(nplot);
            nplot = NULL;
            eeff = NEW_N( Result, 1, "eeff Results for damage composite" );
            eeff_rval = find_result( analy, PRIMAL, TRUE, eeff, "eeff" );

            analy->cur_result = eeff;
            load_result( analy, FALSE, FALSE, FALSE );
            for ( i=0;
                    i<hex_qty;
                    i++ )
                val_hex1[i] = p_hex_class->data_buffer[i];
        }

        val_hex2 = NEW_N( float, node_qty, "Temp node vel buffers" );
        NODAL_RESULT_BUFFER( analy ) = val_hex2;
        nrvol = NEW_N( Result, 1, "nrvol Results for damage composite" );
        nrvol_rval = find_result( analy, DERIVED, TRUE, nrvol, "relvol" );
        if ( nrvol_rval )
        {
            analy->cur_result = nrvol;
            load_result( analy, FALSE, FALSE, FALSE );
            for ( i=0;
                    i<hex_qty;
                    i++ )
                val_hex2[i] = p_hex_class->data_buffer[i];
        }
        else
        {
            popup_dialog( INFO_POPUP, "%s\n%s",
                          "Relative Volume not available for damage calculation",
                          "calculation; aborted." );
            cleanse_result( nrvol );
            free( nrvol );
            nrvol = NULL;

            if ( nplot_rval )
            {
                cleanse_result( nplot );
                free( nplot );
                nplot = NULL;
            }
            else
            {
                cleanse_result( eeff );
                free( eeff );
                eeff = NULL;
            }
            return;
        }
        connects = (int (*)[8]) p_hex_class->objects.elems->nodes;

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

                /* Loop over the elements in the current element block. */
                for ( i = start; i <= stop; i++ )
                {
                    vel_sums[i] = 0;

                    if ( single_prec_vel )
                    {
                        /* Single precision */
                        for ( j = 0; j < 8; j++ )
                        {
                            nd = connects[i][j];
                            vel_sums[i] += vel_1p[nd];
                        }
                        vel_sums[i] /= 8.0;
                    }
                    else
                    {
                        /* Double precision */
                        for ( j = 0; j < 8; j++ )
                        {
                            nd = connects[i][j];
                            vel_sums[i] += (float)vel_2p[nd];
                        }
                        vel_sums[i] /= 8.0;
                    }
                }
            }
        }

        resultElem = compute_damage(hex_qty, vel_cutoff, relVol_cutoff, eeff_cutoff,
                                    val_hex1, vel_sums, val_hex2);

        /* Update the classes data buffer */
        for ( i=0;
                i<hex_qty;
                i++ )
            p_hex_class->data_buffer[i] = resultElem[i];

        p_vd = p_hex_class->p_vis_data;

#ifdef DISABLE_DAMAGE_HISTORY
        /* Retain damage history from previous time steps */
        /* Disable for now */

        for ( i=0;
                i<hex_qty;
                i++)
        {
            /* The reset flag is set to TRUE is we do not want to retain
             * damage history. If set to TRUE then all of the elements will
             * start out as viewable - damage history will be erased.
             */
            if (analy->reset_damage_hide ||
                    analy->min_state == analy->cur_state)
                p_vd->visib_damage[i] = FALSE;

            if (!resultElem[i])
                resultElem[i] = p_vd->visib_damage[i];
        }
#endif

        if ( analy->reset_damage_hide )
            analy->reset_damage_hide = FALSE;

        if ( interpolate )
            /* Interpolate to the nodes by class and active materials. */
            hex_to_nodal_by_mat( resultElem, resultArr, p_hex_class,
                                 active_qty, active_mats, p_mats, analy );
        /*
        * Set the visibility for damaged elements.
         */

        /* Added 09/15/04: IRC - The field visib_damage is used to control the
         * display of damaged elements.
         */
#ifdef DISABLE_DAMAGE_HISTORY
        if (analy->damage_hide)
        {
            for ( i = 0;
                    i < hex_qty;
                    i++ )
                if ( resultElem[i] )
                {
                    p_vd->visib_damage[i] = TRUE;
                }
                else
                    p_vd->visib_damage[i] = FALSE;
        }
#endif
    }

    if (analy->damage_hide)
    {
        /* We need to redefine the faces that are visible here
         * since this function is usually called before the load
         * function.
         */
        reset_face_visibility( analy );
        if ( analy->dimension == 3 )
            compute_normals( analy );
    }

    NODAL_RESULT_BUFFER( analy ) = resultArr;
    analy->cur_result = p_result;

    /* Initialize temporary element min/max values. */
    init_mm_obj( &analy->tmp_elem_mm );
    elem_get_minmax( resultElem, 0, analy );
    update_min_max( analy );

    /* Releases nodal velocity component Result structs. */
    if ( val_hex1)
        free( val_hex1 );
    if ( val_hex2 )
        free( val_hex2 );

    if ( velx != NULL )
    {
        for ( i = 0; i < 3; i++ )
            cleanse_result( velx + i );

        free( velx );

        velx = vely = velz = NULL;
    }

    if( nplot != NULL )
        cleanse_result( nplot );
    else
        cleanse_result( eeff );

    if( nrvol != NULL )
        cleanse_result( nrvol );

    if ( vel_2p != NULL )
        free( vel_2p );

    free( active_mats );

    return;
}

/*****************************************************************
 * TAG( compute_damage )
 *
 * This function will compute for every element a damage value of
 * 0 or 1 where 0 is no damage and 1 is damage.
 */
float *compute_damage(int   qty,
                      float vel_cutoff,
                      float relVol_cutoff,
                      float eeff_cutoff,
                      float *eeff,
                      float *vel,
                      float *relVol)
{
    int    i;
    float *damage;

    /* Allocate space for the returned damage array */

    damage = (float *) malloc( qty * sizeof(float) );

    for (i=0;
            i<qty;
            i++)
    {

        /* Initialize with no damage */
        damage[i] = 0.;

        if (eeff_cutoff    < eeff[i] &&
                vel_cutoff    < vel[i] &&
                relVol_cutoff < relVol[i])
            damage[i] = 1.0; /* This element has damage */
    }

    return(damage);
}
