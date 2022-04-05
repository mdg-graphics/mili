/* $Id$ */
/*
 * frame.c - Routines for global to local reference frame transformations.
 *
 *      Thomas Spelce
 *      Lawrence Livermore National Laboratory
 *      Sept 24 1992
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

/************************************************************
 * TAG( transpose_tensors )
 *
 * Transpose the tensors in analy->tmp_result[0], storing a
 * single component into a result array (used to transform
 * all components back into analy->tmp_result[0...5]).
 */
static Bool_type
transpose_tensors( int qty, Analysis *analy, float *res_array )
{
    int comp_idx, i;
    float (*tens)[6];
    char *name;

    name = analy->cur_result->name;

    /*
     * Hack to get the correct tensor component index (component name
     * is one of sx, sy, sz, sxy, syz, szx, OR ex, ey, ez, exy, eyz, ezx,
     * OR gamxy, gamyz, gamzx).
     */
    if ( name[0] == 'e' || name[0] == 's' )
        comp_idx = (int) name[1] - (int) 'x' + (( name[2] ) ? 3 : 0);
    else if ( strncmp( name, "gam", 3 ) == 0 )
        comp_idx = (int) name[3] - (int) 'x' + 3;
    else
        return FALSE;

    /* Extract the requested component into the result array. */
    tens = (float (*)[6]) analy->tmp_result[0];
    for ( i = 0; i < qty; i++ )
        res_array[i] = tens[i][comp_idx];

    return TRUE;
}

/************************************************************
 * TAG( hex_g2l_mtx )
 *
 * Computes the matrix for transforming global stress to local
 * element stress for hexes.
 */

void
hex_g2l_mtx(Analysis *analy, MO_class_data *p_mo_class, int elem,
            int n1, int n2, int n3, float localMat[3][3] )
{
    float visual_info[3], vj[3], vk[3], vd[3];
    float x[8], y[8], z[8];
    float al1, al2;
    GVec3D *coords;
    int (*connects)[8];
    int i;

    connects = (int (*)[8]) p_mo_class->objects.elems->nodes;
    coords = analy->state_p->nodes.nodes3d;

    /*  Get the hex nodal coordinates for local reference frame  */
    for ( i = 0; i < 8; i++ )
    {
        x[i] = coords[ connects[elem][i] ][0];
        y[i] = coords[ connects[elem][i] ][1];
        z[i] = coords[ connects[elem][i] ][2];
    }

    /*
     *  "i" axis is the direction from node 1 to node 2
     */
    visual_info[0] = x[n2] - x[n1];
    visual_info[1] = y[n2] - y[n1];
    visual_info[2] = z[n2] - z[n1];

    /*
     *  Vector "D" is the direction from node 1 to node 4
     */
    vd[0] = x[n3] - x[n1];
    vd[1] = y[n3] - y[n1];
    vd[2] = z[n3] - z[n1];

    /*
     * Normalize the reference vectors
     */
    al1=1.0/sqrt(visual_info[0]*visual_info[0]+visual_info[1]*visual_info[1]+visual_info[2]*visual_info[2]);
    visual_info[0]=visual_info[0]*al1;
    visual_info[1]=visual_info[1]*al1;
    visual_info[2]=visual_info[2]*al1;

    al2=1.0/sqrt(vd[0]*vd[0]+vd[1]*vd[1]+vd[2]*vd[2]);
    vd[0]=vd[0]*al2;
    vd[1]=vd[1]*al2;
    vd[2]=vd[2]*al2;

    /*
     *  "k" axis is cross-product of "i" and vector "D"
     */
    VEC_CROSS( vk, visual_info, vd );

    /*
     *  "j" axis is cross-product of "k" and "i"
     */
    VEC_CROSS( vj, vk, visual_info );

    /* column vectors */
    for ( i = 0; i < 3; i++ )
    {
        localMat[i][0] =  visual_info[i];
        localMat[i][1] =  vj[i];
        localMat[i][2] =  vk[i];
    }

    return;

}

/************************************************************
 * TAG( global_to_local_mtx )
 *
 * Computes the matrix for transforming global stress to local
 * element stress for shells.
 */
void
global_to_local_mtx( Analysis *analy, MO_class_data *p_mo_class, int elem,
                     Bool_type map_timehist_coords, GVec3D2P *new_nodes,
                     float localMat[3][3] )
{
    float x[4], y[4], z[4];
    float xn[3], yn[3], zn[3];
    int i;
    int nd;
    GVec3D *coords;
    int (*connects)[4];
    int index;

    connects = (int (*)[4]) p_mo_class->objects.elems->nodes;
    coords = analy->state_p->nodes.nodes3d;

    /* Get the quad element geometry. */
    for ( i = 0; i < 4; i++ )
    {
        if ( !map_timehist_coords )
        {
            x[i] = coords[ connects[elem][i] ][0];
            y[i] = coords[ connects[elem][i] ][1];
            z[i] = coords[ connects[elem][i] ][2];
        }
        else
        {
            nd = connects[elem][i];

            x[i] = new_nodes[nd][0];
            y[i] = new_nodes[nd][1];
            z[i] = new_nodes[nd][2];
        }
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
 * TAG( global_to_local_tri_mtx )
 *
 * Computes the matrix for transforming global stress to local
 * element stress for shells with a superclass of G_TRI.
 */
void
global_to_local_tri_mtx( Analysis *analy, MO_class_data *p_mo_class, int elem,
                         Bool_type map_timehist_coords, GVec3D2P *new_nodes,
                         float localMat[3][3] )
{
    float x[3], y[3], z[3];
    float xn[3], yn[3], zn[3];
    int i;
    int nd;
    GVec3D *coords;
    int (*connects)[3];
    int index;

    connects = (int (*)[3]) p_mo_class->objects.elems->nodes;
    coords = analy->state_p->nodes.nodes3d;

    /* Get the quad element geometry. */
    for ( i = 0; i < 3; i++ )
    {
        if ( !map_timehist_coords )
        {
            x[i] = coords[ connects[elem][i] ][0];
            y[i] = coords[ connects[elem][i] ][1];
            z[i] = coords[ connects[elem][i] ][2];
        }
        else
        {
            nd = connects[elem][i];

            x[i] = new_nodes[nd][0];
            y[i] = new_nodes[nd][1];
            z[i] = new_nodes[nd][2];
        }
    }

    /* Get average vectors for the sides of the element. */
    xn[0] = -x[0] + x[1];
    xn[1] = -y[0] + y[1];
    xn[2] = -z[0] + z[1];
    yn[0] = -x[0] - x[1] + x[2] + x[2];
    yn[1] = -y[0] - y[1] + y[2] + y[2];
    yn[2] = -z[0] - z[1] + z[2] + z[2];

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
 * Transform a double-precision input array of stress/strain
 * tensors with a change-of-coordinates transformation matrix.
 *
 *                  t
 *     sigma_new = A * sigma * A
 *
 * The tensor comes in as a vector.  The routine is hardcoded for
 * the above matrix multiply.
 */
void
transform_tensors( int qty, double (*tensors)[6], float mat[][3] )
{
    double new[6], sub1[3], sub2[3], sub3[3];
    double *p_ten;
    int i, j;

    for ( i = 0; i < qty; i++ )
    {
        p_ten = tensors[i];

        /* Compute common sub-expressions. */
        sub1[0] = p_ten[0]*mat[0][0] + p_ten[3]*mat[1][0]
                  + p_ten[5]*mat[2][0];
        sub1[1] = p_ten[3]*mat[0][0] + p_ten[1]*mat[1][0]
                  + p_ten[4]*mat[2][0];
        sub1[2] = p_ten[5]*mat[0][0] + p_ten[4]*mat[1][0]
                  + p_ten[2]*mat[2][0];

        sub2[0] = p_ten[0]*mat[0][1] + p_ten[3]*mat[1][1]
                  + p_ten[5]*mat[2][1];
        sub2[1] = p_ten[3]*mat[0][1] + p_ten[1]*mat[1][1]
                  + p_ten[4]*mat[2][1];
        sub2[2] = p_ten[5]*mat[0][1] + p_ten[4]*mat[1][1]
                  + p_ten[2]*mat[2][1];

        sub3[0] = p_ten[0]*mat[0][2] + p_ten[3]*mat[1][2]
                  + p_ten[5]*mat[2][2];
        sub3[1] = p_ten[3]*mat[0][2] + p_ten[1]*mat[1][2]
                  + p_ten[4]*mat[2][2];
        sub3[2] = p_ten[5]*mat[0][2] + p_ten[4]*mat[1][2]
                  + p_ten[2]*mat[2][2];

        /* Get the transformed vector. */
        new[0] = mat[0][0]*sub1[0] + mat[1][0]*sub1[1] + mat[2][0]*sub1[2];
        new[1] = mat[0][1]*sub2[0] + mat[1][1]*sub2[1] + mat[2][1]*sub2[2];
        new[2] = mat[0][2]*sub3[0] + mat[1][2]*sub3[1] + mat[2][2]*sub3[2];
        new[3] = mat[0][0]*sub2[0] + mat[1][0]*sub2[1] + mat[2][0]*sub2[2];
        new[4] = mat[0][1]*sub3[0] + mat[1][1]*sub3[1] + mat[2][1]*sub3[2];
        new[5] = mat[0][0]*sub3[0] + mat[1][0]*sub3[1] + mat[2][0]*sub3[2];

        for ( j = 0; j < 6; j++ )
            p_ten[j] = new[j];
    }
}


/************************************************************
 * TAG( transform_tensors_1p )
 *
 * Transform a single-precision input array of stress/strain
 * tensors with a change-of-coordinates transformation matrix.
 *
 *                  t
 *     sigma_new = A * sigma * A
 *
 * The tensor comes in as a vector.  The routine is hardcoded for
 * the above matrix multiply.
 */
void
transform_tensors_1p( int qty, float (*tensors)[6], float mat[][3] )
{
    double new[6], sub1[3], sub2[3], sub3[3];
    float *p_ten;
    int i, j;

    for ( i = 0; i < qty; i++ )
    {
        p_ten = tensors[i];

        /* Compute common sub-expressions. */
        sub1[0] = (double) p_ten[0]*mat[0][0] + (double) p_ten[3]*mat[1][0]
                  + (double) p_ten[5]*mat[2][0];
        sub1[1] = (double) p_ten[3]*mat[0][0] + (double) p_ten[1]*mat[1][0]
                  + (double) p_ten[4]*mat[2][0];
        sub1[2] = (double) p_ten[5]*mat[0][0] + (double) p_ten[4]*mat[1][0]
                  + (double) p_ten[2]*mat[2][0];

        sub2[0] = (double) p_ten[0]*mat[0][1] + (double) p_ten[3]*mat[1][1]
                  + (double) p_ten[5]*mat[2][1];
        sub2[1] = (double) p_ten[3]*mat[0][1] + (double) p_ten[1]*mat[1][1]
                  + (double) p_ten[4]*mat[2][1];
        sub2[2] = (double) p_ten[5]*mat[0][1] + (double) p_ten[4]*mat[1][1]
                  + (double) p_ten[2]*mat[2][1];

        sub3[0] = (double) p_ten[0]*mat[0][2] + (double) p_ten[3]*mat[1][2]
                  + (double) p_ten[5]*mat[2][2];
        sub3[1] = (double) p_ten[3]*mat[0][2] + (double) p_ten[1]*mat[1][2]
                  + (double) p_ten[4]*mat[2][2];
        sub3[2] = (double) p_ten[5]*mat[0][2] + (double) p_ten[4]*mat[1][2]
                  + (double) p_ten[2]*mat[2][2];

        /* Get the transformed vector. */
        new[0] = mat[0][0]*sub1[0] + mat[1][0]*sub1[1] + mat[2][0]*sub1[2];
        new[1] = mat[0][1]*sub2[0] + mat[1][1]*sub2[1] + mat[2][1]*sub2[2];
        new[2] = mat[0][2]*sub3[0] + mat[1][2]*sub3[1] + mat[2][2]*sub3[2];
        new[3] = mat[0][0]*sub2[0] + mat[1][0]*sub2[1] + mat[2][0]*sub2[2];
        new[4] = mat[0][1]*sub3[0] + mat[1][1]*sub3[1] + mat[2][1]*sub3[2];
        new[5] = mat[0][0]*sub3[0] + mat[1][0]*sub3[1] + mat[2][0]*sub3[2];

        for ( j = 0; j < 6; j++ )
            p_ten[j] = new[j];
    }
}


/************************************************************
 * TAG( transform_stress_strain )
 *
 * Transform single-precision stress or strain tensors for hex or shell
 * elements at the current state according to the currently
 * defined coordinate transformation matrix.
 *
 * The transformed components are left in analy->tmp_result.
 */
Bool_type
transform_stress_strain( char **primals, int primal_index, Analysis *analy,
                         float trans_matrix[3][3], float *res_array )
{
    int qty;
    int rval;
    Result *p_res;
    int subr_id;

    p_res = analy->cur_result;
    subr_id = p_res->subrecs[analy->result_index];

    /* Load the tensors for the elements in the current subrecord. */
    rval = analy->db_get_results( analy->db_ident, analy->cur_state + 1,
                                  subr_id, 1, primals + primal_index,
                                  (void *) analy->tmp_result[0] );
    if ( rval != OK )
        return FALSE;

    /* Do it... */
    qty = analy->srec_tree[p_res->srec_id].subrecs[subr_id].subrec.qty_objects;

    transform_tensors_1p( qty, (float (*)[6]) analy->tmp_result[0],
                          trans_matrix );

    /* Extract requested component. */
    return transpose_tensors( qty, analy, res_array );
}




