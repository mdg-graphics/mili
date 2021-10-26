//Chadway Cooper
//Sept 7, 2012



#include <stdio.h>
#include <iostream>
#include <string>
#include <math.h>

using namespace std;

#include "transformClass.h"

#define VEC_LENGTH(A)      ((float)sqrt((double)VEC_DOT(A, A)))
#define VEC_LENGTH_2D(A)   ((float)sqrt((double)VEC_DOT_2D(A, A)))
#define VEC_DOT(A, B)      ((A)[0]*(B)[0] + (A)[1]*(B)[1] + (A)[2]*(B)[2])
#define VEC_DOT_2D(A, B)   ((A)[0]*(B)[0] + (A)[1]*(B)[1])
#define VEC_SCALE(C, s, A)  { (C)[0] = (s) * (A)[0]; \
                              (C)[1] = (s) * (A)[1]; \
                              (C)[2] = (s) * (A)[2]; }
#define VEC_CROSS(C, A, B)   { (C)[0] = (A)[1]*(B)[2] - (A)[2]*(B)[1]; \
                               (C)[1] = (A)[2]*(B)[0] - (A)[0]*(B)[2]; \
                               (C)[2] = (A)[0]*(B)[1] - (A)[1]*(B)[0]; }

/*****************************************************************
 * TAG( vec_norm )
 * 
 * Normalize a vector.
 */
static void 
vec_norm( double *vec )
{
    double len;

    len = VEC_LENGTH( vec );


    if ( len == 0.0 )
        return;
    else
	VEC_SCALE( vec, 1.0/len, vec );
}



/************************************************************
 * TAG( hex_g2l_mtx )
 *
 * Computes the matrix for transforming global stress to local
 * element stress for hexes.
 */

void
transformClass::hex_g2l_mtx(double x[8], double y[8], double z[8],
            int n1, int n2, int n3, double localMat[3][3] )
{
    float vi[3], vj[3], vk[3], vd[3];
    float al1, al2;
    int i;

    /*
     *  "i" axis is the direction from node 1 to node 2
     */
    vi[0] = x[n2] - x[n1];
    vi[1] = y[n2] - y[n1];
    vi[2] = z[n2] - z[n1];

    /*
     *  Vector "D" is the direction from node 1 to node 4
     */
    vd[0] = x[n3] - x[n1];
    vd[1] = y[n3] - y[n1];
    vd[2] = z[n3] - z[n1];

    /*
     * Normalize the reference vectors
     */
    //al1=1.0/sqrt(vi[0]*vi[0]+vi[1]*vi[1]+vi[2]*vi[2]);
    vi[0]=vi[0]*al1;
    vi[1]=vi[1]*al1;
    vi[2]=vi[2]*al1;

    //al2=1.0/sqrt(vd[0]*vd[0]+vd[1]*vd[1]+vd[2]*vd[2]);
    vd[0]=vd[0]*al2;
    vd[1]=vd[1]*al2;
    vd[2]=vd[2]*al2;

    /*
     *  "k" axis is cross-product of "i" and vector "D"
     */
    VEC_CROSS( vk, vi, vd );

    /*
     *  "j" axis is cross-product of "k" and "i"
     */
    VEC_CROSS( vj, vk, vi );

    /* column vectors */
    for ( i = 0; i < 3; i++ )    
    {
        localMat[i][0] =  vi[i];
        localMat[i][1] =  vj[i];
        localMat[i][2] =  vk[i];
    }
    
    return;

}

void transformClass::G2LMatrixShell(double x[4], double y[4], double z[4], double localMat[3][3]){
    double xn[3], yn[3], zn[3];
    int i;
    int nd;

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
    //need 
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

bool transformClass::TransformStressStrain( double input[][6], double trans_matrix[3][3], double *res_array)
{
    int qty = 1;
    int rval;
    //Result *p_res;
    int subr_id;
    
    TransformTensors( qty, input,trans_matrix );
    
    /* Extract requested component. */
    return TransposeTensors( qty, "sy" , input, res_array );
}

double* transformClass::TransformTensors(int qty, double tensors[][6], double mat[][3])
{
    double newMat[6], sub1[3], sub2[3], sub3[3];
    double *p_ten;
    int i, j;

    for ( i = 0; i < qty; i++ )
    {
        p_ten = tensors[i];

        /* Compute common sub-expressions. */
        sub1[0] = (double) p_ten[0]*mat[0][0] + (double) p_ten[3]*mat[1][0] + (double) p_ten[5]*mat[2][0];
        sub1[1] = (double) p_ten[3]*mat[0][0] + (double) p_ten[1]*mat[1][0] + (double) p_ten[4]*mat[2][0];
        sub1[2] = (double) p_ten[5]*mat[0][0] + (double) p_ten[4]*mat[1][0] + (double) p_ten[2]*mat[2][0];

        sub2[0] = (double) p_ten[0]*mat[0][1] + (double) p_ten[3]*mat[1][1] + (double) p_ten[5]*mat[2][1];
        sub2[1] = (double) p_ten[3]*mat[0][1] + (double) p_ten[1]*mat[1][1] + (double) p_ten[4]*mat[2][1];
        sub2[2] = (double) p_ten[5]*mat[0][1] + (double) p_ten[4]*mat[1][1] + (double) p_ten[2]*mat[2][1];

        sub3[0] = (double) p_ten[0]*mat[0][2] + (double) p_ten[3]*mat[1][2] + (double) p_ten[5]*mat[2][2];
        sub3[1] = (double) p_ten[3]*mat[0][2] + (double) p_ten[1]*mat[1][2] + (double) p_ten[4]*mat[2][2];
        sub3[2] = (double) p_ten[5]*mat[0][2] + (double) p_ten[4]*mat[1][2] + (double) p_ten[2]*mat[2][2];

        /* Get the transformed vector. */
        newMat[0] = mat[0][0]*sub1[0] + mat[1][0]*sub1[1] + mat[2][0]*sub1[2];
        newMat[1] = mat[0][1]*sub2[0] + mat[1][1]*sub2[1] + mat[2][1]*sub2[2];
        newMat[2] = mat[0][2]*sub3[0] + mat[1][2]*sub3[1] + mat[2][2]*sub3[2];
        newMat[3] = mat[0][0]*sub2[0] + mat[1][0]*sub2[1] + mat[2][0]*sub2[2];
        newMat[4] = mat[0][1]*sub3[0] + mat[1][1]*sub3[1] + mat[2][1]*sub3[2];
        newMat[5] = mat[0][0]*sub3[0] + mat[1][0]*sub3[1] + mat[2][0]*sub3[2];

        for ( j = 0; j < 6; j++ )
            p_ten[j] = newMat[j];
    }
    return p_ten;
}

bool transformClass::TransposeTensors(int qty, char* name, double tensor[][6], double *res_array){
    int comp_idx, i;
    //float (*tens)[6];

    /* 
     * Hack to get the correct tensor component index (component name 
     * is one of sx, sy, sz, sxy, syz, szx, OR ex, ey, ez, exy, eyz, ezx,
     * OR gamxy, gamyz, gamzx).
     */
    if ( name[0] == 'e' || name[0] == 's' )
        comp_idx = (int) name[1] - (int) 'x' + (( name[2] ) ? 3 : 0);
    else if (name[0] == 'g' && name[1] == 'a' && name[2] == 'm')//strncmp( name, "gam", 3 ) == 0 )
        comp_idx = (int) name[3] - (int) 'x' + 3;
    else
        return false;

    /* Extract the requested component into the result array. */
    for ( i = 0; i < qty; i++ )
        res_array[i] = tensor[i][comp_idx];
    
    return true;
}

//to be implemented
void
SetTransMat(int node1, int node2, int node3)
{

}
  
