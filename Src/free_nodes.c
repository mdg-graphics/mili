/* $Id$ */
/*
 * free_nodes.c - Gathers and displays free node data
 *
 *      Bob Corey
 *      Lawrence Livermore National Laboratory
 *      Aug 31, 2005
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
 ************************************************************************
 * Modifications:
 *  I. R. Corey - Aug 31, 2005: Created
 *
 *  I. R. Corey - Feb 07, 2006: Add a new capability to compute
 *                momemtun for free node data and write the data
 *                to a text file - one file per timestep.
 *                See SRC#368.
 *
 *  I. R. Corey - March 03, 2006: Fixed a memory leak in the
 *                free node momemtun calculation function.
 *                See SRC#372.
 *
 *  I. R. Corey - March 14, 2006: Modified gather_free_nodes so that
 *                it handles the case where no free node data is
 *                present (fn_mass and fn_vol is null).
 *                See SRC#374
************************************************************************
 */

#include <stdlib.h>
#include "viewer.h"

#define NONE    -1
#define MASS     0
#define VOL      1
#define MOMENTUM 2


/*****************************************************************
 * TAG( compute_fnmass )
 * This function is called from the "show" command. If will return
 * the total mass for free nodes only.
 *
 */
void
compute_fnmass( Analysis *analy, float *resultArr,
                Bool_type interpolate )
{
    int   all_nodes = FALSE;
    int   i, index;
    int   subrec, srec;
    int   num_elements;
    int   num_nodes, node_index, *fn_list, num_fn;
    float *fn_mass, *result_buf,  mass_total = 0.;

    Result *p_result;
    Subrec_obj *p_subrec;

    p_result     = analy->cur_result;
    index        = analy->result_index + 1;
    subrec       = p_result->subrecs[index];
    srec         = p_result->srec_id;
    p_subrec     = analy->srec_tree[srec].subrecs + subrec;
    num_elements = p_subrec->subrec.qty_objects;

    result_buf = p_subrec->p_object_class->data_buffer;

    fn_mass = resultArr;

    num_nodes = gather_free_nodes(analy,     &num_fn, MASS,
                                  all_nodes, &fn_list,
                                  &fn_mass);

    for (node_index=0;
            node_index<num_nodes;
            node_index++)
    {

        if (fn_list[node_index] < 0)
        {
            fn_mass[node_index] = 0.0;
        }
        mass_total += fn_mass[node_index];
    }

    for (node_index=0;
            node_index<num_nodes;
            node_index++)
    {
        fn_mass[node_index] = mass_total;
    }

    result_buf[0] = mass_total;
    analy->tmp_elem_mm.object_minmax[0] = 0.;
    analy->tmp_elem_mm.object_minmax[1] = mass_total;

    if (fn_list!=NULL)
        free(fn_list);
    return;
}


/*****************************************************************
 * TAG( compute_anmass )
 * This function is called from the "show" command. If will return
 * the total mass for every node.
 *
 */
void
compute_anmass( Analysis *analy, float *resultArr,
                Bool_type interpolate )
{
    int   all_nodes = TRUE;
    int   i, index;
    int   subrec, srec;
    int   num_elements;
    int   num_nodes, node_index, *fn_list, num_fn;
    float *fn_mass, *result_buf,  mass_total = 0;

    Result *p_result;
    Subrec_obj *p_subrec;

    p_result     = analy->cur_result;
    index        = analy->result_index + 1;
    subrec       = p_result->subrecs[index];
    srec         = p_result->srec_id;
    p_subrec     = analy->srec_tree[srec].subrecs + subrec;
    num_elements = p_subrec->subrec.qty_objects;

    result_buf = p_subrec->p_object_class->data_buffer;

    fn_mass = resultArr;

    num_nodes = gather_free_nodes(analy,     &num_fn, MASS,
                                  all_nodes, &fn_list,
                                  &fn_mass);
    for (node_index=0;
            node_index<num_nodes;
            node_index++)
    {

        if (fn_list[node_index] < 0)
        {
            fn_mass[node_index] = 0.0;
        }
        mass_total += fn_mass[node_index];
    }

    for (node_index=0;
            node_index<num_nodes;
            node_index++)
    {
        fn_mass[node_index] = mass_total;
    }

    result_buf[0] = mass_total;
    analy->tmp_elem_mm.object_minmax[0] = 0.;
    analy->tmp_elem_mm.object_minmax[1] = mass_total;

    if (fn_list!=NULL)
        free(fn_list);
    return;
}


/*****************************************************************
 * TAG( compute_fnmom )
 * This function is called from the "show" command. If will return
 * the total mom for free nodes only.
 *
 */
void
compute_fnmoment( Analysis *analy, float *resultArr,
                  Bool_type interpolate )
{
    int   all_nodes = FALSE;
    int   i, index;
    int   state;
    int   subrec, srec;
    int   num_elements;
    int   num_nodes, node_count, node_index, *fn_list, num_fn;
    int   displayed_nodes = 0;
    int   mat;
    float *fn_mass, *result_buf,  mom_total = 0;

    float *velx, *vely, *velz, *velmag;

    /* Material mass and momentum */
    Bool  mats_used[200];
    float mass_by_mat[200], mass_by_mat_fn[200], mom_by_mat[200], mom_by_mat_fn[200];

    char fname[100];
    FILE *fp;

    Result *p_result;
    Subrec_obj *p_subrec;

    num_nodes = gather_free_nodes(analy,     &num_fn,  MASS,
                                  all_nodes, &fn_list, &resultArr);

    fn_mass = (float *) malloc(sizeof(float)*num_nodes);

    for (node_index=0;
            node_index<num_nodes;
            node_index++)
    {
        fn_mass[node_index] = resultArr[node_index];
    }

    /* Allocate space for velocity components and gather velocity */

    velx   = (float *) malloc(sizeof(float)*num_nodes);
    vely   = (float *) malloc(sizeof(float)*num_nodes);
    velz   = (float *) malloc(sizeof(float)*num_nodes);
    velmag = (float *) malloc(sizeof(float)*num_nodes);

    node_count = gather_nodal_velocity( analy, velx, vely, velz, velmag);

    p_result     = analy->cur_result;
    index        = analy->result_index + 1;
    subrec       = p_result->subrecs[index];
    srec         = p_result->srec_id;
    p_subrec     = analy->srec_tree[srec].subrecs + subrec;
    num_elements = p_subrec->subrec.qty_objects;
    state        = analy->cur_state+1;

    for (node_index=0;
            node_index<num_nodes;
            node_index++)
    {
        if (fn_list[node_index]!=0)
            mom_total += fn_mass[node_index]*velmag[node_index];
    }

    result_buf = p_subrec->p_object_class->data_buffer;


    /* Write out the data to a text file if requested */
    if (analy->fn_output_momentum==TRUE)
    {
        for (i=0;
                i<200;
                i++)
        {
            mats_used[i]       = 0;
            mass_by_mat[i]     = 0.;
            mass_by_mat_fn[i]  = 0.;
            mom_by_mat[i]      = 0.;
            mom_by_mat_fn[i]   = 0.;
        }

        for (node_index=0;
                node_index<num_nodes;
                node_index++)
        {
            mat = fn_list[node_index];
            if (mat<0)
                mat = -mat;

            if (mat!=0)
            {
                mats_used[mat] = 1;

                if (fn_list[node_index]<0)
                    mass_by_mat_fn[mat] += fn_mass[node_index];
                else
                    mass_by_mat[mat]    += fn_mass[node_index];

                if (fn_list[node_index]<0)
                    mom_by_mat_fn[mat]     += (fn_mass[node_index]*velmag[node_index]);
                else
                    mom_by_mat[mat]        += (fn_mass[node_index]*velmag[node_index]);
            }
        }

        for (node_index=0;
                node_index<num_nodes;
                node_index++)
            if (fn_list[node_index]!=0)
                displayed_nodes++;

        sprintf(fname, "MOMEMTUM-%4.4d", state);
        wrt_text("Writing Momemtum data for state %d to file: %s\n\n", state, fname);

        fp = fopen(fname, "w+");

        fprintf(fp,"\nNumber of Nodes Selected=%d\n\n", displayed_nodes);

        fprintf(fp,"\tMat\tMass Tot\tMass Tot(FN)\tMom Tot\t\tMom Tot(FN)\n");

        for (i=0;
                i<200;
                i++)
        {
            if (mats_used[i])
            {
                fprintf(fp,"\t%d\t%e\t%e\t%e\t%e\n", i, mass_by_mat[i], mass_by_mat_fn[i],
                        mom_by_mat[i], mom_by_mat_fn[i]);
            }
        }

        fprintf(fp,"\n\nNode\t\tMat\tMass\t\tux\t\tuy\t\tuz\t\tMag\n");

        for (i=0;
                i<num_nodes;
                i++)
        {
            mat = fn_list[i];
            if (mat!=0)
            {
                if (mat<1)
                    mat = -mat;
                if (fn_list[i]<0)
                    fprintf(fp,"%d\t  \t%d\t%e\t%e\t%e\t%e\t%e\n", i, mat, fn_mass[i], velx[i], vely[i], velz[i], velmag[i]);
                else
                    fprintf(fp,"%d\tFN\t%d\t%e\t%e\t%e\t%e\t%e\n", i, mat, fn_mass[i], velx[i], vely[i], velz[i], velmag[i]);
            }
        }
        fclose(fp);
    }


    free(fn_mass);
    free(velx);
    free(vely);
    free(velz);
    free(velmag);

    result_buf[0] = mom_total;
    analy->tmp_elem_mm.object_minmax[0] = 0.;
    analy->tmp_elem_mm.object_minmax[1] = mom_total;

    if (fn_list!=NULL)
        free(fn_list);

    return;
}


/*****************************************************************
 * TAG( compute_fnvol )
 * This function is called from the "show" command. If will return
 * the total volfor all free nodes.
 *
 */
void
compute_fnvol( Analysis *analy, float *resultArr,
               Bool_type interpolate )
{
    int   all_nodes = FALSE;
    int   i, index;
    int   subrec, srec;
    int   num_elements;
    int   num_nodes, node_index, *fn_list, num_fn;
    float *fn_mass, *result_buf,  vol_total = 0;

    Result *p_result;
    Subrec_obj *p_subrec;

    p_result     = analy->cur_result;
    index        = analy->result_index + 1;
    subrec       = p_result->subrecs[index];
    srec         = p_result->srec_id;
    p_subrec     = analy->srec_tree[srec].subrecs + subrec;
    num_elements = p_subrec->subrec.qty_objects;

    result_buf = p_subrec->p_object_class->data_buffer;

    fn_mass = resultArr;

    num_nodes = gather_free_nodes(analy,     &num_fn, VOL,
                                  all_nodes, &fn_list,
                                  &fn_mass);
    for (node_index=0;
            node_index<num_nodes;
            node_index++)
    {

        if (fn_list[node_index] < 0)
        {
            fn_mass[node_index] = 0.0;
        }
        vol_total += fn_mass[node_index];
    }

    for (node_index=0;
            node_index<num_nodes;
            node_index++)
    {
        fn_mass[node_index] = vol_total;
    }

    result_buf[0] = vol_total;
    analy->tmp_elem_mm.object_minmax[0] = 0.;
    analy->tmp_elem_mm.object_minmax[1] = vol_total;

    if (fn_list!=NULL)
        free(fn_list);
    return;
}


/*************************************************************
 * TAG( gather_free_nodes )
 *
 * This function will gather and return the free node mass and
 * volume data.
 *************************************************************/
int
gather_free_nodes( Analysis *analy,  int *num_fn, int mass_flag,
                   int   all_nodes,
                   int   **fn_list_ptr,
                   float **free_nodes_data)
{
    MO_class_data *p_element_class,
                  *p_node_class,
                  *p_mo_class,
                  **mo_classes;

    Mesh_data     *p_mesh;

    List_head     *p_lh;

    char *cname;

    int   nd;

    float *activity, *temp_activity;
    float *data_array;
    float **sand_arrays;

    int  elem_qty;
    int  node_index, node_num, num_nodes, node_qty;
    int  class_index, class_qty;
    int  conn_qty;
    int  vert_cnt;

    int i, j, k, l;
    int dim, obj_qty;

    int  *connects;

    int  free_nodes_found   = FALSE;
    int  fn_flag;
    int  *fn_list, fn_count = 0, fn_data_found=TRUE;

    float *fn_mass_ptr = NULL;
    float *fn_mass_tmp = NULL;
    float *fn_vol_ptr  = NULL;
    float *fn_vol_tmp  = NULL;

    int  status;

    /* Variables related to materials */
    unsigned char *disable_mtl, *hide_mtl;
    int  *mat, mat_num;

    p_mesh      = MESH_P( analy );
    disable_mtl = p_mesh->disable_material;
    hide_mtl    = p_mesh->hide_material;

    dim        = analy->dimension;
    data_array = NODAL_RESULT_BUFFER( analy );

    p_node_class = MESH_P( analy )->node_geom;
    num_nodes    = p_node_class->qty;

    /* Free node list is an array of node length used to identify
     * the free nodes - free_nodes_list[i] is set to 1 if node is free.
     */
    fn_list = NEW_N( int , num_nodes, "fn_list" );
    memset( fn_list, 0, num_nodes );

    temp_activity = NEW_N( float , num_nodes, "temp_activity_list" );
    memset( temp_activity, 1, num_nodes );

    fn_mass_ptr = *free_nodes_data;
    fn_vol_ptr  = *free_nodes_data;

    /* If mass scaling option is enabled, then look for the nodal masses */
    if (mass_flag==MASS)
    {
        status = mili_db_get_param_array(analy->db_ident,  "Nodal Mass",   (void *) &fn_mass_tmp);
    }

    /* Read the nodal volumes if volume scaling is enabled */
    if (mass_flag==VOL)
        status = mili_db_get_param_array(analy->db_ident,  "Nodal Volume", (void *) &fn_vol_tmp);


    /* Make sure that this database has free node mass or volume data */
    if (fn_mass_tmp==NULL && fn_vol_tmp==NULL)
        fn_data_found = FALSE;

    fn_count = 0;
    *num_fn  = 0;

    /* Loop over each element superclass. */
    for ( i = 0; i < QTY_SCLASS; i++ )
    {
        if ( !IS_ELEMENT_SCLASS( i ) )
            continue;

        p_lh = p_mesh->classes_by_sclass + i;

        /* If classes exist from current superclass... */
        if ( p_lh->qty > 0 )
        {
            mo_classes = (MO_class_data **)
                         p_mesh->classes_by_sclass[i].list;

            sand_arrays = analy->state_p->elem_class_sand;
            node_qty    = qty_connects[i];
            conn_qty    = ( i == G_BEAM ) ? node_qty - 1 : node_qty;

            /* Loop over each class. */
            for ( j = 0;
                    j < p_lh->qty;
                    j++ )
            {
                p_mo_class = mo_classes[j];
                connects   = p_mo_class->objects.elems->nodes;
                mat        = p_mo_class->objects.elems->mat;

                if ( analy->state_p->sand_present )
                    activity = sand_arrays[p_mo_class->elem_class_index];
                else
                    activity = temp_activity;

                /* Loop over each element */
                for ( k = 0;
                        k < p_mo_class->qty;
                        k++ )
                {
                    mat_num = mat[k];
                    fn_flag = activity[k];
                    if ( all_nodes )
                        fn_flag = 0;

                    if (!disable_mtl[mat_num] && !hide_mtl[mat_num]
                            && fn_flag == 0)
                        for ( l = 0;
                                l < conn_qty;
                                l++ )
                        {
                            nd = connects[k * node_qty + l];
                            free_nodes_found = TRUE;
                            fn_list[nd]      = mat_num+1;
                            fn_count++;
                        }
                }

                for ( k = 0;
                        k < p_mo_class->qty;
                        k++ )
                {
                    mat_num = mat[k];
                    fn_flag = activity[k];
                    if ( all_nodes )
                        fn_flag = 0;

                    if ( !disable_mtl[mat_num] && !hide_mtl[mat_num] &&
                            fn_flag != 0 )
                        for ( l = 0;
                                l < conn_qty;
                                l++ )
                        {
                            nd = connects[k * node_qty + l];
                            fn_list[nd] = -(mat_num+1);
                        }
                }
            }
        }
    }

    *fn_list_ptr = fn_list;
    *num_fn      = fn_count;

    for ( i = 0;
            i < num_nodes;
            i++ )
        fn_mass_ptr[i] = 0.0;

    if (fn_data_found)
        if (fn_count>0)
            for ( i = 0;
                    i < num_nodes;
                    i++ )
            {
                if (mass_flag==MASS)
                {
                    fn_mass_ptr[i] = fn_mass_tmp[i];
                }
                else
                    fn_vol_ptr[i]  = fn_vol_tmp[i];
            }


    if (fn_mass_tmp!=NULL)
        free(fn_mass_tmp);

    if (fn_vol_tmp!=NULL)
        free(fn_vol_tmp);

    free(temp_activity);

    return(num_nodes);
}


/*************************************************************
 * TAG( gather_nodal_velocity )
 *
 * This function will gather and return the nodal velocity
 * components for all nodes.
 *************************************************************/
int
gather_nodal_velocity( Analysis *analy,
                       float *velx, float *vely,
                       float *velz, float *velmag)

{
    Result *p_result, *res_velx = NULL, *res_vely = NULL, *res_velz = NULL;
    Bool_type single_prec_vel;

    int i;
    int num_nodes;
    int x_rval, y_rval, z_rval;

    float  *vel_1p = NULL;
    double *vel_2p = NULL;

    p_result = analy->cur_result;

    /* Initialize velocity Result structs. */
    res_velx = NEW_N( Result, 3, "Vel Results for free nodes" );
    res_vely = res_velx + 1;
    res_velz = res_velx + 2;

    x_rval = find_result( analy, ALL, TRUE, res_velx, "nodvel[vx]" );
    y_rval = find_result( analy, ALL, TRUE, res_vely, "nodvel[vy]" );
    z_rval = find_result( analy, ALL, TRUE, res_velz, "nodvel[vz]" );

    update_result( analy, res_velx );
    update_result( analy, res_vely );
    update_result( analy, res_velz );

    analy->cur_result = res_velx;
    num_nodes = MESH_P( analy )->node_geom->qty;
    vel_1p = analy->tmp_result[0];
    vel_2p = NEW_N( double, num_nodes, "Temp node vel buffers" );

    /* Capture velocity precision. */
    single_prec_vel = ( ((Primal_result *)
                         res_velx->original_result)->var->num_type
                        == G_FLOAT4
                        ||
                        ((Primal_result *)
                         res_velx->original_result)->var->num_type
                        == G_FLOAT );

    /* Vel[x] */
    analy->cur_result = res_velx;
    if ( single_prec_vel )
    {
        NODAL_RESULT_BUFFER( analy ) = vel_1p;
        load_result( analy, FALSE, FALSE, FALSE );

    }
    else
    {
        NODAL_RESULT_BUFFER( analy ) = (float *) vel_2p;
        load_result( analy, FALSE, FALSE, FALSE );
    }
    for (i=0;
            i<num_nodes;
            i++)
        if ( single_prec_vel )
            velx[i] = vel_1p[i];
        else
            velx[i] = vel_2p[i];

    /* Vel[y] */
    analy->cur_result = res_vely;
    if ( single_prec_vel )
    {
        NODAL_RESULT_BUFFER( analy ) = vel_1p;
        load_result( analy, FALSE, FALSE, FALSE );

    }
    else
    {
        NODAL_RESULT_BUFFER( analy ) = (float *) vel_2p;
        load_result( analy, FALSE, FALSE, FALSE );
    }
    for (i=0;
            i<num_nodes;
            i++)
        if ( single_prec_vel )
            vely[i] = vel_1p[i];
        else
            vely[i] = vel_2p[i];

    /* Vel[z] */
    analy->cur_result = res_velz;
    if ( single_prec_vel )
    {
        NODAL_RESULT_BUFFER( analy ) = vel_1p;
        load_result( analy, FALSE, FALSE, FALSE );

    }
    else
    {
        NODAL_RESULT_BUFFER( analy ) = (float *) vel_2p;
        load_result( analy, FALSE, FALSE, FALSE );
    }
    for (i=0;
            i<num_nodes;
            i++)
    {
        if ( single_prec_vel )
            velz[i] = vel_1p[i];
        else
            velz[i] = vel_2p[i];

        /* We have the x,y,z components, so computer the mag */
        velmag[i] = sqrt((double)((velx[i]*velx[i]) +
                                  (vely[i]*vely[i]) +
                                  (velz[i]*velz[i])));
    }

    analy->cur_result = p_result;

    return(num_nodes);
}

