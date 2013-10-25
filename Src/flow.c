/* $Id$ */
/*
 * flow.c - Fluid flow visualization techniques.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Jan 11 1993
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

#include <stdlib.h>
#include "viewer.h"
#include "draw.h"

#define MAX_TRACER_STEPS 5000

typedef void (*Vec_func)( double *, double *, float *[3], MO_class_data * );

/* Local routines. */
static Bool_type pt_in_hex_bbox( float verts[8][3], float pt[3] );
static Bool_type pt_in_tet_bbox( float verts[][3], float pt[3] );
static Bool_type pt_in_quad_bbox( float verts[4][3], float pt[2] );
static Bool_type pt_in_tri_bbox( float verts[3][3], float pt[2] );
static Bool_type pt_in_surf_bbox( float verts[4][3], float pt[2] );
static void init_trace_points( Trace_pt_def_obj **ptlist, Analysis *analy );
static Bool_type init_pt_in_hex_class( Trace_pt_def_obj *pt,
                                       MO_class_data *p_hex_class,
                                       Analysis *analy );
static Bool_type init_pt_in_tet_class( Trace_pt_def_obj *pt,
                                       MO_class_data *p_tet_class,
                                       Analysis *analy );
static Bool_type init_pt_in_quad_class( Trace_pt_def_obj *pt,
                                        MO_class_data *p_quad_class,
                                        Analysis *analy );
static Bool_type init_pt_in_tri_class( Trace_pt_def_obj *pt,
                                       MO_class_data *p_tri_class,
                                       Analysis *analy );
static Bool_type init_pt_in_surf_class( Trace_pt_def_obj *pt,
                                        MO_class_data *p_surf_class,
                                        Analysis *analy );
static Bool_type init_new_traces( Analysis *analy );
static void follow_particles( Trace_pt_obj *plist, float *vel[3],
                              Analysis *analy, Bool_type follow_mtl,
                              int point_cnt );
static void follow_particle_hex( Trace_pt_obj *pc, float *vel[3],
                                 Analysis *analy, Bool_type follow_mtl,
                                 int point_cnt );
static void follow_particle_tet( Trace_pt_obj *pc, float *vel[3],
                                 Analysis *analy, Bool_type follow_mtl,
                                 int point_cnt );
static void follow_particle_quad( Trace_pt_obj *pc, float *vel[3],
                                  Analysis *analy, Bool_type follow_mtl,
                                  int point_cnt );
static void follow_particle_tri( Trace_pt_obj *pc, float *vel[3],
                                 Analysis *analy, Bool_type follow_mtl,
                                 int point_cnt );
static void vel_at_time( float time, float *vel[3], Analysis *analy );
static void save_trace( Trace_pt_obj *p_trace, int dims, FILE *ofile );
/*** static ***/
void init_vec_points_hex( Vector_pt_obj **ptlist, int mesh_id,
                          Analysis *analy, Bool_type update_class );
static void init_vec_points_tet( Vector_pt_obj **ptlist, int mesh_id,
                                 Analysis *analy, Bool_type update_class );
static void init_vec_points_quad_2d( Vector_pt_obj **ptlist, int mesh_id,
                                     Analysis *analy );
static void init_vec_points_tri_2d( Vector_pt_obj **ptlist, int mesh_id,
                                    Analysis *analy );
static void init_vec_points_surf_2d( Vector_pt_obj **ptlist, int mesh_id,
                                     Analysis *analy );
static void calc_hex_vec_result( double *p_vmin, double *p_vmax,
                                 float *vec_comps[3],
                                 MO_class_data *p_hex_class );
static void calc_tet_vec_result( double *p_vmin, double *p_vmax,
                                 float *vec_comps[3],
                                 MO_class_data *p_tet_class );
static void calc_quad_vec_result( double *p_vmin, double *p_vmax,
                                  float *vec_comps[2],
                                  MO_class_data *p_quad_class );
static void calc_tri_vec_result( double *p_vmin, double *p_vmax,
                                 float *vec_comps[2],
                                 MO_class_data *p_tri_class );
static void calc_surf_vec_result( double *p_vmin, double *p_vmax,
                                  float *vec_comps[2],
                                  MO_class_data *p_surf_class );
/*
static float ramp_random();
static void rand_bary_pt( float tri_vert[3][3], float hex_vert[8][3],
                          float xi[3] );
static void gen_iso_carpet_points( Analysis *analy );
static void gen_grid_carpet_points( Analysis *analy );
static void real_heapsort( int n, float *arrin, int *indx );
*/

/* Local data. */
static char *trace_tag = "Particle trace";
static char *points_tag = "Points: ";
static char *color_tag = "Color: ";
static char *mtls_trav_tag = "Materials traversed: ";
static char *pos_his_hdr_tag =
    "Time         X              Y              Z";


/************************************************************
 * TAG( pt_in_hex_bbox )
 *
 * Test whether a point is within the bounding box of a brick
 * element.  Returns TRUE for within and FALSE for not in.
 */
static Bool_type
pt_in_hex_bbox( float verts[8][3], float pt[3] )
{
    int i, j;
    float min[3];
    float max[3];

    for ( i = 0; i < 3; i++ )
    {
        min[i] = verts[0][i];
        max[i] = verts[0][i];
    }

    for ( i = 1; i < 8; i++ )
        for ( j = 0; j < 3; j++ )
        {
            if ( verts[i][j] < min[j] )
                min[j] = verts[i][j];
            else if ( verts[i][j] > max[j] )
                max[j] = verts[i][j];
        }

    for ( i = 0; i < 3; i++ )
        if ( DEF_LT( pt[i], min[i] ) || DEF_GT( pt[i], max[i] ) )
            return FALSE;

    return TRUE;
}


/************************************************************
 * TAG( pt_in_tet_bbox )
 *
 * Test whether a point is within the bounding box of a tet
 * element.  Returns TRUE for within and FALSE for not in.
 */
static Bool_type
pt_in_tet_bbox( float verts[][3], float pt[3] )
{
    int i, j;
    float min[3];
    float max[3];

    for ( i = 0; i < 3; i++ )
    {
        min[i] = verts[0][i];
        max[i] = verts[0][i];
    }

    for ( i = 1; i < 4; i++ )
        for ( j = 0; j < 3; j++ )
        {
            if ( verts[i][j] < min[j] )
                min[j] = verts[i][j];
            else if ( verts[i][j] > max[j] )
                max[j] = verts[i][j];
        }

    for ( i = 0; i < 3; i++ )
        if ( DEF_LT( pt[i], min[i] ) || DEF_GT( pt[i], max[i] ) )
            return FALSE;

    return TRUE;
}


/************************************************************
 * TAG( pt_in_quad_bbox )
 *
 * Test whether a point is within the bounding box of a 2D
 * quadrilateral element.  Returns TRUE for within and FALSE
 * for not in.
 */
static Bool_type
pt_in_quad_bbox( float verts[4][3], float pt[2] )
{
    int i, j;
    float min[2];
    float max[2];

    min[0] = verts[0][0];
    max[0] = verts[0][0];
    min[1] = verts[0][1];
    max[1] = verts[0][1];

    for ( i = 1; i < 4; i++ )
        for ( j = 0; j < 2; j++ )
        {
            if ( verts[i][j] < min[j] )
                min[j] = verts[i][j];
            else if ( verts[i][j] > max[j] )
                max[j] = verts[i][j];
        }

    for ( i = 0; i < 2; i++ )
        if ( DEF_LT( pt[i], min[i] ) || DEF_GT( pt[i], max[i] ) )
            return FALSE;

    return TRUE;
}


/************************************************************
 * TAG( pt_in_tri_bbox )
 *
 * Test whether a point is within the bounding box of a 2D
 * triangular element.  Returns TRUE for within and FALSE
 * for not in.
 */
static Bool_type
pt_in_tri_bbox( float verts[3][3], float pt[2] )
{
    int i, j;
    float min[2];
    float max[2];

    min[0] = verts[0][0];
    max[0] = verts[0][0];
    min[1] = verts[0][1];
    max[1] = verts[0][1];

    for ( i = 1; i < 3; i++ )
        for ( j = 0; j < 2; j++ )
        {
            if ( verts[i][j] < min[j] )
                min[j] = verts[i][j];
            else if ( verts[i][j] > max[j] )
                max[j] = verts[i][j];
        }

    for ( i = 0; i < 2; i++ )
        if ( DEF_LT( pt[i], min[i] ) || DEF_GT( pt[i], max[i] ) )
            return FALSE;

    return TRUE;
}


/************************************************************
 * TAG( pt_in_surf_bbox )
 *
 * Test whether a point is within the bounding box of a 2D
 * surface element.  Returns TRUE for within and FALSE
 * for not in.
 */
static Bool_type
pt_in_surf_bbox( float verts[4][3], float pt[2] )
{
    int i, j;
    float min[2];
    float max[2];

    min[0] = verts[0][0];
    max[0] = verts[0][0];
    min[1] = verts[0][1];
    max[1] = verts[0][1];

    for ( i = 1; i < 4; i++ )
        for ( j = 0; j < 2; j++ )
        {
            if ( verts[i][j] < min[j] )
                min[j] = verts[i][j];
            else if ( verts[i][j] > max[j] )
                max[j] = verts[i][j];
        }

    for ( i = 0; i < 2; i++ )
        if ( DEF_LT( pt[i], min[i] ) || DEF_GT( pt[i], max[i] ) )
            return FALSE;

    return TRUE;
}


/*
 * SECTION( Particle tracing routines )
 */


/************************************************************
 * TAG( gen_trace_points )
 *
 * Generate a rake of point locations for particle tracing.
 */
void
gen_trace_points( int num, float start_pt[3], float end_pt[3], float color[3],
                  Analysis *analy )
{
    Trace_pt_def_obj *pt, *ptlist;
    float t;
    int i, j;

    ptlist = NULL;

    for ( i = 0; i < num; i++ )
    {
        pt = NEW( Trace_pt_def_obj, "Trace pt definition object" );
        if ( num == 1 )
            t = 0.0;
        else
            t = i / (float)( num - 1 );
        for ( j = 0; j < 3; j++ )
            pt->pt[j] = (1.0-t)*start_pt[j] + t*end_pt[j];
        VEC_COPY( pt->color, color );
        INSERT( pt, ptlist );
    }

    /* Locate the element that each point initially lies in. */
    init_trace_points( &ptlist, analy );

    if ( ptlist != NULL )
        APPEND( ptlist, analy->trace_pt_defs );
}


/************************************************************
 * TAG( init_trace_points )
 *
 * For each point in the list, find the element that the point
 * initially resides in.  Points that don't lie in any element
 * are deleted from the list.
 */
static void
init_trace_points( Trace_pt_def_obj **ptlist, Analysis *analy )
{
    Trace_pt_def_obj *pt, *nextpt;
    int i;
    Mesh_data *p_mesh;
    MO_class_data **hex_classes, **tet_classes;
    MO_class_data **quad_classes, **tri_classes, **surf_classes;
    int class_qty, quad_class_qty, tri_class_qty, surf_class_qty;
    Bool_type pt_done;

    p_mesh = MESH_P( analy );

    if ( analy->dimension == 3 )
    {
        /* First try to locate particles in hex classes, then tet classes. */

        for ( pt = *ptlist; pt != NULL; )
        {
            pt_done = FALSE;

            hex_classes = (MO_class_data **)
                          p_mesh->classes_by_sclass[G_HEX].list;
            class_qty = p_mesh->classes_by_sclass[G_HEX].qty;

            for ( i = 0; i < class_qty; i++ )
            {
                if ( init_pt_in_hex_class( pt, hex_classes[i], analy ) )
                {
                    NEXT( pt );
                    pt_done = TRUE;
                    break;
                }
            }

            if ( pt_done )
                continue;

            tet_classes = (MO_class_data **)
                          p_mesh->classes_by_sclass[G_TET].list;
            class_qty = p_mesh->classes_by_sclass[G_TET].qty;

            for ( i = 0; i < class_qty; i++ )
            {
                if ( init_pt_in_tet_class( pt, tet_classes[i], analy ) )
                {
                    NEXT( pt );
                    pt_done = TRUE;
                    break;
                }
            }

            if ( !pt_done )
            {
                /* The point wasn't in any 3D element, so delete it. */
                popup_dialog( INFO_POPUP,
                              "Point (%f %f %f) not in any element, deleted.\n",
                              pt->pt[0], pt->pt[1], pt->pt[2] );
                nextpt = pt->next;
                DELETE( pt, *ptlist );
                pt = nextpt;
            }
        }
    }
    else
    {
        /* Create 2D element adjacency tables (only needed for traces). */
        quad_classes = (MO_class_data **)
                       p_mesh->classes_by_sclass[G_QUAD].list;
        quad_class_qty = p_mesh->classes_by_sclass[G_QUAD].qty;
        for ( i = 0; i < quad_class_qty; i++ )
            create_quad_adj( quad_classes[i], analy );

        tri_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_TRI].list;
        tri_class_qty = p_mesh->classes_by_sclass[G_TRI].qty;
        for ( i = 0; i < tri_class_qty; i++ )
            create_tri_adj( tri_classes[i], analy );

        /* Create 2D element adjacency tables (only needed for traces). */
        surf_classes = (MO_class_data **)
                       p_mesh->classes_by_sclass[G_SURFACE].list;
        surf_class_qty = p_mesh->classes_by_sclass[G_SURFACE].qty;
        for ( i = 0; i < surf_class_qty; i++ )
            create_surf_adj( surf_classes[i], analy );

        /* 2D; locate particles in quad classes, tri classes,
         * then surface classes
         */
        for ( pt = *ptlist; pt != NULL; )
        {
            pt_done = FALSE;

            for ( i = 0; i < quad_class_qty; i++ )
            {
                if ( init_pt_in_quad_class( pt, quad_classes[i], analy ) )
                {
                    NEXT( pt );
                    pt_done = TRUE;
                    break;
                }
            }

            if ( pt_done )
                continue;

            for ( i = 0; i < tri_class_qty; i++ )
            {
                if ( init_pt_in_tri_class( pt, tri_classes[i], analy ) )
                {
                    NEXT( pt );
                    pt_done = TRUE;
                    break;
                }
            }

            for ( i = 0; i < surf_class_qty; i++ )
            {
                if ( init_pt_in_surf_class( pt, surf_classes[i], analy ) )
                {
                    NEXT( pt );
                    pt_done = TRUE;
                    break;
                }
            }

            if ( !pt_done )
            {
                /* The point wasn't in any 2D element, so delete it. */
                popup_dialog( INFO_POPUP,
                              "Point (%f %f %f) not in any element, deleted.\n",
                              pt->pt[0], pt->pt[1], pt->pt[2] );
                nextpt = pt->next;
                DELETE( pt, *ptlist );
                pt = nextpt;
            }
        }
    }
}


/************************************************************
 * TAG( init_pt_in_hex_class )
 *
 * Attempt to locate a point in an element of a hex mesh
 * object class.
 */
Bool_type
init_pt_in_hex_class( Trace_pt_def_obj *pt, MO_class_data *p_hex_class,
                      Analysis *analy )
{
    float verts[8][3];
    int i, j, k;
    int nd, el;
    Elem_block_obj *p_ebo;
    float xi[3];
    int (*connects)[8];
    GVec3D *nodes;

    connects = (int (*)[8]) p_hex_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes3d;
    p_ebo = p_hex_class->p_elem_block;

    for ( i = 0; i < p_ebo->num_blocks; i++ )
    {
        /* See if the point lies in the block's bounding box first. */
        if ( pt->pt[0] < p_ebo->block_bbox[0][0][i] ||
                pt->pt[1] < p_ebo->block_bbox[0][1][i] ||
                pt->pt[2] < p_ebo->block_bbox[0][2][i] ||
                pt->pt[0] > p_ebo->block_bbox[1][0][i] ||
                pt->pt[1] > p_ebo->block_bbox[1][1][i] ||
                pt->pt[2] > p_ebo->block_bbox[1][2][i] )
            continue;

        /* Test point against elements in block. */
        for ( el = p_ebo->block_lo[i]; el <= p_ebo->block_hi[i]; el++ )
        {
            for ( j = 0; j < 8; j++ )
            {
                nd = connects[el][j];
                for ( k = 0; k < 3; k++ )
                    verts[j][k] = nodes[nd][k];
            }

            /* Perform approximate inside test first. */
            if ( pt_in_hex_bbox( verts, pt->pt ) &&
                    pt_in_hex( verts, pt->pt, xi ) )
            {
                pt->elem = el;
                VEC_COPY( pt->xi, xi );
                pt->material = p_hex_class->objects.elems->mat[el];
                pt->mo_class = p_hex_class;
                return TRUE;
            }
        }
    }

    return FALSE;
}


/************************************************************
 * TAG( init_pt_in_tet_class )
 *
 * Attempt to locate a point in an element of a tet mesh
 * object class.
 */
Bool_type
init_pt_in_tet_class( Trace_pt_def_obj *pt, MO_class_data *p_tet_class,
                      Analysis *analy )
{
    float verts[4][3];
    int i, j, k;
    int nd, el;
    Elem_block_obj *p_ebo;
    float xi[4];
    int (*connects)[4];
    GVec3D *nodes;

    connects = (int (*)[4]) p_tet_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes3d;
    p_ebo = p_tet_class->p_elem_block;

    for ( i = 0; i < p_ebo->num_blocks; i++ )
    {
        /* See if the point lies in the block's bounding box first. */
        if ( pt->pt[0] < p_ebo->block_bbox[0][0][i] ||
                pt->pt[1] < p_ebo->block_bbox[0][1][i] ||
                pt->pt[2] < p_ebo->block_bbox[0][2][i] ||
                pt->pt[0] > p_ebo->block_bbox[1][0][i] ||
                pt->pt[1] > p_ebo->block_bbox[1][1][i] ||
                pt->pt[2] > p_ebo->block_bbox[1][2][i] )
            continue;

        /* Test point against elements in block. */
        for ( el = p_ebo->block_lo[i]; el <= p_ebo->block_hi[i]; el++ )
        {
            for ( j = 0; j < 4; j++ )
            {
                nd = connects[el][j];
                for ( k = 0; k < 3; k++ )
                    verts[j][k] = nodes[nd][k];
            }

            if ( pt_in_tet_bbox( verts, pt->pt ) &&
                    pt_in_tet( verts, pt->pt, xi ) )
            {
                pt->elem = el;
                pt->xi[0] = xi[0];
                pt->xi[1] = xi[1];
                pt->xi[2] = xi[2];
                pt->xi[3] = xi[3];
                pt->material = p_tet_class->objects.elems->mat[el];
                pt->mo_class = p_tet_class;
                return TRUE;
            }
        }
    }

    return FALSE;
}


/************************************************************
 * TAG( init_pt_in_quad_class )
 *
 * Attempt to locate a point in an element of a 2D quad mesh
 * object class.
 */
Bool_type
init_pt_in_quad_class( Trace_pt_def_obj *pt, MO_class_data *p_quad_class,
                       Analysis *analy )
{
    float verts[4][3];
    int i, j, k;
    int nd, el;
    Elem_block_obj *p_ebo;
    float xi[2];
    int (*connects)[4];
    GVec2D *nodes;

    connects = (int (*)[4]) p_quad_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes2d;
    p_ebo = p_quad_class->p_elem_block;

    for ( i = 0; i < p_ebo->num_blocks; i++ )
    {
        /* See if the point lies in the block's bounding box first. */
        if ( pt->pt[0] < p_ebo->block_bbox[0][0][i] ||
                pt->pt[1] < p_ebo->block_bbox[0][1][i] ||
                pt->pt[0] > p_ebo->block_bbox[1][0][i] ||
                pt->pt[1] > p_ebo->block_bbox[1][1][i] )
            continue;

        /* Test point against elements in block. */
        for ( el = p_ebo->block_lo[i]; el <= p_ebo->block_hi[i]; el++ )
        {
            for ( j = 0; j < 4; j++ )
            {
                nd = connects[el][j];
                for ( k = 0; k < 2; k++ )
                    verts[j][k] = nodes[nd][k];
            }

            if ( pt_in_quad_bbox( verts, pt->pt ) &&
                    pt_in_quad( verts, pt->pt, xi ) )
            {
                pt->elem = el;
                VEC_COPY_2D( pt->xi, xi );
                pt->material = p_quad_class->objects.elems->mat[el];
                pt->mo_class = p_quad_class;
                return TRUE;
            }
        }
    }

    return FALSE;
}


/************************************************************
 * TAG( init_pt_in_tri_class )
 *
 * Attempt to locate a point in an element of a 2D tri mesh
 * object class.
 */
Bool_type
init_pt_in_tri_class( Trace_pt_def_obj *pt, MO_class_data *p_tri_class,
                      Analysis *analy )
{
    float verts[3][3];
    int i, j, k;
    int nd, el;
    Elem_block_obj *p_ebo;
    float xi[3];
    int (*connects)[3];
    GVec2D *nodes;

    connects = (int (*)[3]) p_tri_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes2d;
    p_ebo = p_tri_class->p_elem_block;

    for ( i = 0; i < p_ebo->num_blocks; i++ )
    {
        /* See if the point lies in the block's bounding box first. */
        if ( pt->pt[0] < p_ebo->block_bbox[0][0][i] ||
                pt->pt[1] < p_ebo->block_bbox[0][1][i] ||
                pt->pt[0] > p_ebo->block_bbox[1][0][i] ||
                pt->pt[1] > p_ebo->block_bbox[1][1][i] )
            continue;

        /* Test point against elements in block. */
        for ( el = p_ebo->block_lo[i]; el <= p_ebo->block_hi[i]; el++ )
        {
            for ( j = 0; j < 3; j++ )
            {
                nd = connects[el][j];
                for ( k = 0; k < 2; k++ )
                    verts[j][k] = nodes[nd][k];
            }

            if ( pt_in_tri_bbox( verts, pt->pt ) &&
                    pt_in_tri( verts, pt->pt, xi ) )
            {
                pt->elem = el;
                VEC_COPY( pt->xi, xi );
                pt->material = p_tri_class->objects.elems->mat[el];
                pt->mo_class = p_tri_class;
                return TRUE;
            }
        }
    }

    return FALSE;
}


/************************************************************
 * TAG( init_pt_in_surf_class )
 *
 * Attempt to locate a point in an element of a 2D surf mesh
 * object class.
 */
Bool_type
init_pt_in_surf_class( Trace_pt_def_obj *pt, MO_class_data *p_surf_class,
                       Analysis *analy )
{
    float verts[4][3];
    int i, j, k;
    int nd, el;
    Elem_block_obj *p_ebo;
    float xi[2];
    int (*connects)[4];
    GVec2D *nodes;

    connects = (int (*)[4]) p_surf_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes2d;
    p_ebo = p_surf_class->p_elem_block;

    for ( i = 0; i < p_ebo->num_blocks; i++ )
    {
        /* See if the point lies in the block's bounding box first. */
        if ( pt->pt[0] < p_ebo->block_bbox[0][0][i] ||
                pt->pt[1] < p_ebo->block_bbox[0][1][i] ||
                pt->pt[0] > p_ebo->block_bbox[1][0][i] ||
                pt->pt[1] > p_ebo->block_bbox[1][1][i] )
            continue;

        /* Test point against elements in block. */
        for ( el = p_ebo->block_lo[i]; el <= p_ebo->block_hi[i]; el++ )
        {
            for ( j = 0; j < 4; j++ )
            {
                nd = connects[el][j];
                for ( k = 0; k < 2; k++ )
                    verts[j][k] = nodes[nd][k];
            }

            if ( pt_in_surf_bbox( verts, pt->pt ) &&
                    pt_in_surf( verts, pt->pt, xi ) )
            {
                pt->elem = el;
                VEC_COPY_2D( pt->xi, xi );
                /****  Surface Elements do not have a material
                                pt->material = p_surf_class->objects.elems->mat[el];
                *****/
                pt->mo_class = p_surf_class;
                return TRUE;
            }
        }
    }

    return FALSE;
}


/************************************************************
 * TAG( free_trace_list )
 *
 * Delete a list of particle trace points.
 */
void
free_trace_list( Trace_pt_obj **p_trace_list )
{
    Trace_pt_obj *p_tpo, *p_tpo2;

    for ( p_tpo = *p_trace_list; p_tpo != NULL; )
    {
        p_tpo2 = p_tpo;
        NEXT( p_tpo );

        free( p_tpo2->pts );
        free( p_tpo2->time );

        if ( p_tpo2->mtl_segments != NULL )
            DELETE_LIST( p_tpo2->mtl_segments );

        free( p_tpo2 );
    }

    *p_trace_list = NULL;
}


/************************************************************
 * TAG( free_trace_points )
 *
 * Delete the particle trace points.
 */
void
free_trace_points( Analysis *analy )
{
    free_trace_list( &analy->trace_pts );
    free_trace_list( &analy->new_trace_pts );

    DELETE_LIST( analy->trace_pt_defs );
}


/************************************************************
 * TAG( init_new_traces )
 *
 * Duplicate the "trace_pt_defs" list as a list of Trace_pt_obj
 * structures to prepare for a new trace integration.
 */
static Bool_type
init_new_traces( Analysis *analy )
{
    Trace_pt_def_obj *p_tpd;
    Trace_pt_obj *p_tp;

    if ( analy->new_trace_pts != NULL )
    {
        popup_dialog( WARNING_POPUP, "New traces already active!" );
        return FALSE;
    }

    for ( p_tpd = analy->trace_pt_defs; p_tpd != NULL; p_tpd = p_tpd->next )
    {
        p_tp = NEW( Trace_pt_obj, "Trace point object" );

        p_tp->elnum = p_tpd->elem;
        p_tp->last_elem = p_tpd->elem;
        p_tp->mtl_num = p_tpd->material;
        VEC_COPY( p_tp->pt, p_tpd->pt );
        VEC_COPY( p_tp->color, p_tpd->color );
        VEC_COPY( p_tp->xi, p_tpd->xi );
        p_tp->pts = NEW_N( float, 3*MAX_TRACER_STEPS, "Trace points" );
        p_tp->time = NEW_N( float, MAX_TRACER_STEPS, "Trace point times" );
        p_tp->mo_class = p_tpd->mo_class;

        APPEND( p_tp, analy->new_trace_pts );
    }

    return ( analy->new_trace_pts != NULL ) ? TRUE : FALSE;
}


/************************************************************
 * TAG( follow_particles )
 *
 * Using the current particle positions, find the element that
 * each particle lies in and the natural coordinates for the
 * particle location.  This routine "walks" from element to
 * element until it finds the enclosing element.  Then the
 * velocity of the particle is updated by interpolating from
 * the nodes of the enclosing element.
 */
static void
follow_particles( Trace_pt_obj *plist, float *vel[3], Analysis *analy,
                  Bool_type follow_mtl, int point_cnt )
{
    Trace_pt_obj *pc;
    static void (* follow_funcs[])() =
    {
        NULL, NULL, NULL, NULL,  /* Unit, node, truss, and beam classes N/A */
        follow_particle_tri,
        follow_particle_quad,
        follow_particle_tet,
        NULL, NULL,              /* Pyramid and wedge classes N/A */
        follow_particle_hex
    };

    for ( pc = plist; pc != NULL; pc = pc->next )
    {
        /* Check to see we've bounced out of the grid already. */
        if ( !pc->in_grid )
            continue;

        (follow_funcs[pc->mo_class->superclass])( pc, vel, analy, follow_mtl,
                point_cnt );

        /*
         * Without a mesh-global face table and integrated adjacency tables,
         * pc->in_grid should be checked here and out-of-grid particles
         * should be evaluated for inclusion in other classes to verify that
         * it really has left the mesh.  When (if) necessary, it would be
         * straightforward to modify init_trace_points() to serve this
         * purpose as well.
         */
    }
}


/************************************************************
 * TAG( follow_particle_hex )
 *
 * Using the current particle positions, find the element that
 * each particle lies in and the natural coordinates for the
 * particle location.  This routine "walks" from element to
 * element until it finds the enclosing element.  Then the
 * velocity of the particle is updated by interpolating from
 * the nodes of the enclosing element.
 */
static void
follow_particle_hex( Trace_pt_obj *pc, float *vel[3], Analysis *analy,
                     Bool_type follow_mtl, int point_cnt )
{
    Bool_type found;
    float verts[8][3];
    float xi[3];
    float h[8];
    int nd, el, face;
    int i, j;
    Trace_segment_obj *p_ts;
    static MO_class_data *p_hex_class = NULL;
    static int **hex_adj;
    static int (*connects)[8];
    static GVec3D *nodes;
    static int *mat;

    if ( pc->mo_class != p_hex_class )
    {
        p_hex_class = pc->mo_class;
        connects = (int (*)[8]) p_hex_class->objects.elems->nodes;
        nodes = analy->state_p->nodes.nodes3d;
        hex_adj = p_hex_class->p_vis_data->adjacency;
        mat = p_hex_class->objects.elems->mat;
    }

    /* Get the vertices of the element. */
    el = pc->elnum;
    for ( i = 0; i < 8; i++ )
    {
        nd = connects[el][i];
        for ( j = 0; j < 3; j++ )
            verts[i][j] = nodes[nd][j];
    }

    /* Test whether new point is inside this element. */
    if ( pt_in_hex( verts, pc->pt, xi ) )
    {
        VEC_COPY( pc->xi, xi );
        found = TRUE;
    }
    else
        found = FALSE;

    /*
     * Move till we find an element that the particle is inside of or
     * we bounce out of the grid.
     */
    while ( !found )
    {
        face = -1;
        if ( xi[0] > 1.0 && hex_adj[0][el] >= 0 )
            face = 0;
        else if ( xi[1] > 1.0 && hex_adj[1][el] >= 0 )
            face = 1;
        else if ( xi[0] < -1.0 && hex_adj[2][el] >= 0 )
            face = 2;
        else if ( xi[1] < -1.0 && hex_adj[3][el] >= 0 )
            face = 3;
        else if ( xi[2] > 1.0 && hex_adj[4][el] >= 0 )
            face = 4;
        else if ( xi[2] < -1.0 && hex_adj[5][el] >= 0 )
            face = 5;

        if ( face >= 0 )
        {
            /* Move to next element. */

            el = hex_adj[face][el];

            for ( i = 0; i < 8; i++ )
            {
                nd = connects[el][i];
                for ( j = 0; j < 3; j++ )
                    verts[i][j] = nodes[nd][j];
            }

            if ( pt_in_hex( verts, pc->pt, xi ) )
            {
                /* Found the desired element. */

                pc->elnum = el;
                VEC_COPY( pc->xi, xi );
                found = TRUE;
            }
        }
        else
        {
            /* We have bounced out of the grid. */

            pc->in_grid = FALSE;
            found = TRUE;

            /* Always add a material trace segment for the last segment. */
            p_ts = NEW( Trace_segment_obj, "Trace segment" );
            p_ts->material = pc->mtl_num;
            p_ts->last_pt_idx = point_cnt - 1;
            APPEND( p_ts, pc->mtl_segments );
        }
    }

    /*
    * If monitoring material transitions, check new material
    * against previous material and add a list entry for
    * last material's trace segment if there's a change.
    */
    if ( follow_mtl && ( el != pc->last_elem ) && pc->in_grid )
    {
        /*
         * "pc->elnum" is not suitable for this comparison because
         * it may be updated during either call to follow_particles
         * which occurs during a single position integration
         * calculation, and only the element validated during the
         * first call is guaranteed to be the correct element
         * for the current position.  However, this element may
         * actually have been identified during either of the calls
         * that occurred while the current position was being
         * integrated, so this test must happen outside of the
         * logic which identifies/validates the element number.  As
         * long as "follow_mtl" is TRUE only during the first
         * follow_particles call and pc->last_elem is only updated
         * when "follow_mtl" is TRUE, this logic will correctly
         * capture material transitions.
         */
        pc->last_elem = el;

        if ( mat[el] != pc->mtl_num )
        {
            p_ts = NEW( Trace_segment_obj, "Trace segment" );
            p_ts->material = pc->mtl_num;

            /*
             * If point_cnt is "i", we're validating the element for
             * point "i-1" and the last point for the PREVIOUS
             * material is "i-2".
             */
            p_ts->last_pt_idx = point_cnt - 2;
            APPEND( p_ts, pc->mtl_segments );
            pc->mtl_num = mat[el];
        }
    }

    /* Update the velocities at the particle. */
    if ( pc->in_grid )
    {
        shape_fns_hex( pc->xi[0], pc->xi[1], pc->xi[2], h );

        VEC_SET( pc->vec, 0.0, 0.0, 0.0 );
        for ( i = 0; i < 8; i++ )
        {
            nd = connects[pc->elnum][i];
            for ( j = 0; j < 3; j++ )
                pc->vec[j] += h[i]*vel[j][nd];
        }
    }
}


/************************************************************
 * TAG( follow_particle_tet )
 *
 * Using the current particle positions, find the element that
 * each particle lies in and the natural coordinates for the
 * particle location.  This routine "walks" from element to
 * element until it finds the enclosing element.  Then the
 * velocity of the particle is updated by interpolating from
 * the nodes of the enclosing element.
 */
static void
follow_particle_tet( Trace_pt_obj *pc, float *vel[3], Analysis *analy,
                     Bool_type follow_mtl, int point_cnt )
{
    Bool_type found;
    float verts[4][3];
    float xi[4];
    int nd, el, face;
    int i, j;
    Trace_segment_obj *p_ts;
    static MO_class_data *p_tet_class = NULL;
    static int **tet_adj;
    static int (*connects)[4];
    static GVec3D *nodes;
    static int *mat;

    if ( pc->mo_class != p_tet_class )
    {
        p_tet_class = pc->mo_class;
        connects = (int (*)[4]) p_tet_class->objects.elems->nodes;
        nodes = analy->state_p->nodes.nodes3d;
        tet_adj = p_tet_class->p_vis_data->adjacency;
        mat = p_tet_class->objects.elems->mat;
    }

    /* Get the vertices of the element. */
    el = pc->elnum;
    for ( i = 0; i < 4; i++ )
    {
        nd = connects[el][i];
        for ( j = 0; j < 3; j++ )
            verts[i][j] = nodes[nd][j];
    }

    /* Test whether new point is inside this element. */
    if ( pt_in_tet( verts, pc->pt, xi ) )
    {
        pc->xi[0] = xi[0];
        pc->xi[1] = xi[1];
        pc->xi[2] = xi[2];
        pc->xi[3] = xi[3];
        found = TRUE;
    }
    else
        found = FALSE;

    /*
     * Move till we find an element that the particle is inside of or
     * we bounce out of the grid.
     */
    while ( !found )
    {
        face = -1;
        if ( xi[0] < 0.0 && tet_adj[0][el] >= 0 )
            face = 0;
        else if ( xi[1] < 0.0 && tet_adj[1][el] >= 0 )
            face = 1;
        else if ( xi[2] < 0.0 && tet_adj[2][el] >= 0 )
            face = 2;
        else if ( xi[3] < 0.0 && tet_adj[3][el] >= 0 )
            face = 3;

        if ( face >= 0 )
        {
            /* Move to next element. */

            el = tet_adj[face][el];

            for ( i = 0; i < 4; i++ )
            {
                nd = connects[el][i];
                for ( j = 0; j < 3; j++ )
                    verts[i][j] = nodes[nd][j];
            }

            if ( pt_in_tet( verts, pc->pt, xi ) )
            {
                /* Found the desired element. */

                pc->elnum = el;
                pc->xi[0] = xi[0];
                pc->xi[1] = xi[1];
                pc->xi[2] = xi[2];
                pc->xi[3] = xi[3];
                found = TRUE;
            }
        }
        else
        {
            /* We have bounced out of the grid. */

            pc->in_grid = FALSE;
            found = TRUE;

            /* Always add a material trace segment for the last segment. */
            p_ts = NEW( Trace_segment_obj, "Trace segment" );
            p_ts->material = pc->mtl_num;
            p_ts->last_pt_idx = point_cnt - 1;
            APPEND( p_ts, pc->mtl_segments );
        }
    }

    /*
    * If monitoring material transitions, check new material
    * against previous material and add a list entry for
    * last material's trace segment if there's a change.
    */
    if ( follow_mtl && ( el != pc->last_elem ) && pc->in_grid )
    {
        /* See follow_particle_hex() above. */

        pc->last_elem = el;

        if ( mat[el] != pc->mtl_num )
        {
            p_ts = NEW( Trace_segment_obj, "Trace segment" );
            p_ts->material = pc->mtl_num;

            /*
             * If point_cnt is "i", we're validating the element for
             * point "i-1" and the last point for the PREVIOUS
             * material is "i-2".
             */
            p_ts->last_pt_idx = point_cnt - 2;
            APPEND( p_ts, pc->mtl_segments );
            pc->mtl_num = mat[el];
        }
    }

    /* Update the velocities at the particle. */
    if ( pc->in_grid )
    {
        VEC_SET( pc->vec, 0.0, 0.0, 0.0 );

        for ( i = 0; i < 4; i++ )
        {
            nd = connects[pc->elnum][i];
            for ( j = 0; j < 3; j++ )
                pc->vec[j] += pc->xi[i] * vel[j][nd];
        }
    }
}


/************************************************************
 * TAG( follow_particle_quad )
 *
 * Using the current particle positions, find the element that
 * each particle lies in and the natural coordinates for the
 * particle location.  This routine "walks" from element to
 * element until it finds the enclosing element.  Then the
 * velocity of the particle is updated by interpolating from
 * the nodes of the enclosing element.
 */
static void
follow_particle_quad( Trace_pt_obj *pc, float *vel[3], Analysis *analy,
                      Bool_type follow_mtl, int point_cnt )
{
    Bool_type found;
    float verts[4][3];
    float xi[2];
    float h[4];
    int nd, el, edge;
    int i, j;
    Trace_segment_obj *p_ts;
    static MO_class_data *p_quad_class = NULL;
    static int **quad_adj;
    static int (*connects)[4];
    static GVec2D *nodes;
    static int *mat;

    if ( pc->mo_class != p_quad_class )
    {
        p_quad_class = pc->mo_class;
        connects = (int (*)[4]) p_quad_class->objects.elems->nodes;
        nodes = analy->state_p->nodes.nodes2d;
        quad_adj = p_quad_class->p_vis_data->adjacency;
        mat = p_quad_class->objects.elems->mat;
    }

    /* Get the vertices of the element. */
    el = pc->elnum;
    for ( i = 0; i < 4; i++ )
    {
        nd = connects[el][i];
        for ( j = 0; j < 2; j++ )
            verts[i][j] = nodes[nd][j];
    }

    /* Test whether new point is inside this element. */
    if ( pt_in_quad( verts, pc->pt, xi ) )
    {
        VEC_COPY_2D( pc->xi, xi );
        found = TRUE;
    }
    else
        found = FALSE;

    /*
     * Move till we find an element that the particle is inside of or
     * we bounce out of the grid.
     */
    while ( !found )
    {
        edge = -1;
        if ( xi[0] > 1.0 && quad_adj[0][el] >= 0 )
            edge = 0;
        else if ( xi[1] > 1.0 && quad_adj[1][el] >= 0 )
            edge = 1;
        else if ( xi[0] < -1.0 && quad_adj[2][el] >= 0 )
            edge = 2;
        else if ( xi[1] < -1.0 && quad_adj[3][el] >= 0 )
            edge = 3;

        if ( edge >= 0 )
        {
            /* Move to next element. */

            el = quad_adj[edge][el];

            for ( i = 0; i < 4; i++ )
            {
                nd = connects[el][i];
                for ( j = 0; j < 2; j++ )
                    verts[i][j] = nodes[nd][j];
            }

            if ( pt_in_quad( verts, pc->pt, xi ) )
            {
                /* Found the desired element. */

                pc->elnum = el;
                VEC_COPY_2D( pc->xi, xi );
                found = TRUE;
            }
        }
        else
        {
            /* We have bounced out of the grid. */

            pc->in_grid = FALSE;
            found = TRUE;

            /* Always add a material trace segment for the last segment. */
            p_ts = NEW( Trace_segment_obj, "Trace segment" );
            p_ts->material = pc->mtl_num;
            p_ts->last_pt_idx = point_cnt - 1;
            APPEND( p_ts, pc->mtl_segments );
        }
    }

    /*
    * If monitoring material transitions, check new material
    * against previous material and add a list entry for
    * last material's trace segment if there's a change.
    */
    if ( follow_mtl && ( el != pc->last_elem ) && pc->in_grid )
    {
        /* See follow_particle_hex() above. */

        pc->last_elem = el;

        if ( mat[el] != pc->mtl_num )
        {
            p_ts = NEW( Trace_segment_obj, "Trace segment" );
            p_ts->material = pc->mtl_num;

            /*
             * If point_cnt is "i", we're validating the element for
             * point "i-1" and the last point for the PREVIOUS
             * material is "i-2".
             */
            p_ts->last_pt_idx = point_cnt - 2;
            APPEND( p_ts, pc->mtl_segments );
            pc->mtl_num = mat[el];
        }
    }

    /* Update the velocities at the particle. */
    if ( pc->in_grid )
    {
        shape_fns_quad( pc->xi[0], pc->xi[1], h );

        VEC_SET( pc->vec, 0.0, 0.0, 0.0 );
        for ( i = 0; i < 4; i++ )
        {
            nd = connects[pc->elnum][i];
            for ( j = 0; j < 2; j++ )
                pc->vec[j] += h[i]*vel[j][nd];
        }
    }
}


/************************************************************
 * TAG( follow_particle_tri )
 *
 * Using the current particle positions, find the element that
 * each particle lies in and the natural coordinates for the
 * particle location.  This routine "walks" from element to
 * element until it finds the enclosing element.  Then the
 * velocity of the particle is updated by interpolating from
 * the nodes of the enclosing element.
 */
static void
follow_particle_tri( Trace_pt_obj *pc, float *vel[3], Analysis *analy,
                     Bool_type follow_mtl, int point_cnt )
{
    Bool_type found;
    float verts[4][3];
    float xi[3];
    int nd, el, edge;
    int i, j;
    Trace_segment_obj *p_ts;
    static MO_class_data *p_tri_class = NULL;
    static int **tri_adj;
    static int (*connects)[3];
    static GVec2D *nodes;
    static int *mat;

    if ( pc->mo_class != p_tri_class )
    {
        p_tri_class = pc->mo_class;
        connects = (int (*)[3]) p_tri_class->objects.elems->nodes;
        nodes = analy->state_p->nodes.nodes2d;
        tri_adj = p_tri_class->p_vis_data->adjacency;
        mat = p_tri_class->objects.elems->mat;
    }

    /* Get the vertices of the element. */
    el = pc->elnum;
    for ( i = 0; i < 3; i++ )
    {
        nd = connects[el][i];
        for ( j = 0; j < 2; j++ )
            verts[i][j] = nodes[nd][j];
    }

    /* Test whether new point is inside this element. */
    if ( pt_in_tri( verts, pc->pt, xi ) )
    {
        VEC_COPY( pc->xi, xi );
        found = TRUE;
    }
    else
        found = FALSE;

    /*
     * Move till we find an element that the particle is inside of or
     * we bounce out of the grid.
     */
    while ( !found )
    {
        edge = -1;
        if ( xi[0] > 1.0 && tri_adj[1][el] >= 0 )
            edge = 1;
        else if ( xi[1] > 1.0 && tri_adj[2][el] >= 0 )
            edge = 2;
        else if ( xi[2] < -1.0 && tri_adj[0][el] >= 0 )
            edge = 0;

        if ( edge >= 0 )
        {
            /* Move to next element. */

            el = tri_adj[edge][el];

            for ( i = 0; i < 3; i++ )
            {
                nd = connects[el][i];
                for ( j = 0; j < 2; j++ )
                    verts[i][j] = nodes[nd][j];
            }

            if ( pt_in_tri( verts, pc->pt, xi ) )
            {
                /* Found the desired element. */

                pc->elnum = el;
                VEC_COPY( pc->xi, xi );
                found = TRUE;
            }
        }
        else
        {
            /* We have bounced out of the grid. */

            pc->in_grid = FALSE;
            found = TRUE;

            /* Always add a material trace segment for the last segment. */
            p_ts = NEW( Trace_segment_obj, "Trace segment" );
            p_ts->material = pc->mtl_num;
            p_ts->last_pt_idx = point_cnt - 1;
            APPEND( p_ts, pc->mtl_segments );
        }
    }

    /*
    * If monitoring material transitions, check new material
    * against previous material and add a list entry for
    * last material's trace segment if there's a change.
    */
    if ( follow_mtl && ( el != pc->last_elem ) && pc->in_grid )
    {
        /* See follow_particle_hex() above. */

        pc->last_elem = el;

        if ( mat[el] != pc->mtl_num )
        {
            p_ts = NEW( Trace_segment_obj, "Trace segment" );
            p_ts->material = pc->mtl_num;

            /*
             * If point_cnt is "i", we're validating the element for
             * point "i-1" and the last point for the PREVIOUS
             * material is "i-2".
             */
            p_ts->last_pt_idx = point_cnt - 2;
            APPEND( p_ts, pc->mtl_segments );
            pc->mtl_num = mat[el];
        }
    }

    /* Update the velocities at the particle. */
    if ( pc->in_grid )
    {
        VEC_SET( pc->vec, 0.0, 0.0, 0.0 );

        for ( i = 0; i < 3; i++ )
        {
            nd = connects[pc->elnum][i];
            for ( j = 0; j < 2; j++ )
                pc->vec[j] += pc->xi[i] * vel[j][nd];
        }
    }
}


/************************************************************
 * TAG( vel_at_time )
 *
 * Retrieve the nodal velocities for the specified time.
 */
static void
vel_at_time( float time, float *vel[3], Analysis *analy )
{
    float *vec1, *vec2;
    float *vels_1, *vels_2;
    float t1, t2, interp;
    int node_cnt, tmp_state, state1, state2;
    int i, j;
    int idx1, idx2;
    int srec_id1, srec_id2;
    int subrec1, subrec2;
    int rval;
    Result *res1, *tmp_res;
    char *vel_comps[3] =
    {
        "velx", "vely", "velz"
    };
    char *vel_vec_comps[3] =
    {
        "nodvel[vx]", "nodvel[vy]", "nodvel[vz]"
    };
    Bool_type have_primal;
    int dims;
    int bound_states[2];

    tmp_res = analy->cur_result;
    tmp_state = analy->cur_state;

    dims = analy->dimension;
    node_cnt = MESH( analy ).node_geom->qty;

    /* Find the states which sandwich the desired time. */
    rval = analy->db_query( analy->db_ident, QRY_STATE_OF_TIME, (void *) &time,
                            NULL, (void *) bound_states );
    if ( rval != OK )
    {
        popup_dialog( WARNING_POPUP,
                      "Bad time request (%f) in vel_at_time().", time );
        return;
    }
    state1 = bound_states[1];
    analy->db_query( analy->db_ident, QRY_STATE_TIME, bound_states + 1, NULL,
                     (void *) &t1 );
    state2 = bound_states[0];
    analy->db_query( analy->db_ident, QRY_STATE_TIME, bound_states, NULL,
                     (void *) &t2 );

    interp = (time-t1) / (t2-t1);

    /*
     * If nodal velocities are primal results, load all components per
     * state with one direct call to db_get_results(), otherwise get
     * individually with compute_node_velocity() (i.e., the old way).
     */

    have_primal = FALSE;

    analy->db_query( analy->db_ident, QRY_SREC_FMT_ID, &state1, NULL,
                     (void *) &srec_id1 );
    subrec1 = analy->srec_tree[srec_id1].node_vel_subrec;

    analy->db_query( analy->db_ident, QRY_SREC_FMT_ID, &state2, NULL,
                     (void *) &srec_id2 );
    subrec2 = analy->srec_tree[srec_id2].node_vel_subrec;

    if ( subrec1 != -1 && subrec2 != -1 )
    {
        vels_1 = NEW_N( float, dims * node_cnt, "Tmp velocities buffer 1" );
        vels_2 = NEW_N( float, dims * node_cnt, "Tmp velocities buffer 2" );

        if ( vels_1 != NULL && vels_2 != NULL )
        {
            analy->db_get_results( analy->db_ident, state1, subrec1, dims,
                                   vel_vec_comps, (void *) vels_1 );
            analy->db_get_results( analy->db_ident, state2, subrec2, dims,
                                   vel_vec_comps, (void *) vels_2 );

            for ( i = 0; i < dims; i++ )
            {
                vec1 = vels_1 + i * node_cnt;
                vec2 = vels_2 + i * node_cnt;

                for ( j = 0; j < node_cnt; j++ )
                    vel[i][j] = (1.0 - interp) * vec1[j] + interp * vec2[j];
            }

            have_primal = TRUE;
        }

        if ( vels_1 != NULL )
            free( vels_1 );
        if ( vels_2 != NULL )
            free( vels_2 );
    }

    if ( !have_primal )
    {
        /*
         * Problem: need a Result to specify which component compute_node_velocity()
         * needs to get, but if "vecx", "vecy", and/or "vecz" aren't in the db as
         * scalars, we have no useful way of interacting with compute_node_velocity().
         * And if they are in the db, we don't need compute_node_velocity().
         *
         * Maybe we determine if scalar components are present and if not, call
         * a different function that knows to derive velocity from positions.
         */
        tmp_res = analy->cur_result;
        tmp_state = analy->cur_state;
        vec1 = analy->tmp_result[0];
        vec2 = analy->tmp_result[1];

        res1 = NEW( Result, "Temp vel result1" );

        subrec1 = analy->srec_tree[srec_id1].node_pos_subrec;
        subrec2 = analy->srec_tree[srec_id2].node_pos_subrec;

        for ( i = 0; i < dims; i++ )
        {
            find_result( analy, ALL, TRUE, res1, vel_comps[i] );
            analy->cur_result = res1;

            /*
             * Find the result index for the subrecord containing positions.
             * (Assumes all components are found in same subrecord.)
             */
            if ( i == 0 )
            {
                for ( idx1 = 0; idx1 < res1->qty; idx1++ )
                    if ( res1->subrecs[idx1] == subrec1 )
                        break;

                if ( srec_id2 != srec_id1 )
                {
                    for ( idx2 = 0; idx2 < res1->qty; idx2++ )
                        if ( res1->subrecs[idx2] == subrec2 )
                            break;
                }
                else
                    idx2 = idx1;
            }

            analy->result_index = idx1;
            analy->cur_state = state1;
            /*            compute_node_velocity( analy, vec1 ); */

            /* Find the result index for the subrecord containing positions. */
            analy->result_index = idx2;
            analy->cur_state = state2;
            /*            compute_node_velocity( analy, vec2 ); */

            for ( j = 0; j < node_cnt; j++ )
                vel[i][j] = (1.0 - interp) * vec1[j] + interp * vec2[j];

            cleanse_result( res1 );
        }

        free( res1 );
    }

    analy->cur_result = tmp_res;
    analy->cur_state = tmp_state;
}


/************************************************************
 * TAG( particle_trace )
 *
 * Trace out particle paths.
 */
void
particle_trace( float t_zero, float t_stop, float delta_t, Analysis *analy,
                Bool_type static_field )
{
    Trace_pt_obj *pc, *p_tp;
    Trace_segment_obj *p_ts;
    float *vel[3];
    float t, beg_t, end_t;
    int node_cnt, cnt;
    int i;
    int state;
    int qty_states;
    int bound_states[2];
    int dims;

    analy->show_traces = TRUE;
    /*
        analy->mesh_view_mode = RENDER_NONE;
    */
    analy->show_edges = TRUE;

    dims = analy->dimension;

    /* Bounds check on start time. */
    analy->db_query( analy->db_ident, QRY_QTY_STATES, NULL, NULL,
                     (void *) &qty_states );
    state = 1;
    analy->db_query( analy->db_ident, QRY_STATE_TIME, (void *) &state, NULL,
                     (void *) &beg_t );
    state = qty_states;
    analy->db_query( analy->db_ident, QRY_STATE_TIME, (void *) &state, NULL,
                     (void *) &end_t );
    t_zero = BOUND( beg_t, t_zero, end_t );

    /* Initialize particles. */
    if ( !init_new_traces( analy ) )
        return;
    for ( pc = analy->new_trace_pts; pc != NULL; NEXT( pc ) )
    {
        for ( i = 0; i < dims; i++ )
            pc->pts[i] = pc->pt[i];
        pc->time[0] = t_zero;
        pc->cnt = 1;
        pc->in_grid = TRUE;
    }

    /* Move to the start state; this is to take care of other display stuff.
     * Ie vectors, isosurfs, & etc should get updated during the animation.
     */
    analy->db_query( analy->db_ident, QRY_STATE_OF_TIME, (void *) &t_zero, NULL,
                     (void *) bound_states );
    analy->cur_state = bound_states[0] - 1;
    change_state( analy );

    /* When necessary to calculate delta_t, do so differently for static and
     * dynamic flow fields.
     */
    if ( static_field )
    {
        if ( delta_t <= 0.0 || delta_t > t_stop )
            delta_t = t_stop * 0.01;
        wrt_text( "Static field particle trace, using delta t of %f.\n",
                  delta_t );
    }
    else
    {
        t_stop = BOUND( beg_t, t_stop, end_t );

        /* Guess delta_t if a value has not been provided. */
        if ( delta_t <= 0.0 )
            delta_t = 0.3 * (end_t - beg_t) / qty_states;
        wrt_text( "Particle trace, using delta t of %f.\n", delta_t );
    }

    /* Setup. */
    node_cnt = MESH( analy ).node_geom->qty;
    for ( i = 0; i < dims; i++ )
        vel[i] = NEW_N( float, node_cnt, "Velocities" );

    /* Initialize nodal velocities. */
    t = ( static_field ) ? 0.0 : t_zero;
    vel_at_time( t_zero, vel, analy );

    /* Integration loop. */
    cnt = 1;
    while ( cnt < MAX_TRACER_STEPS && analy->new_trace_pts != NULL )
    {
        /*
         * We use second-order Runge-Kutta integration to integrate
         * the particle velocities over time, which generates the
         * the particle paths.
         */

        if ( t + delta_t > t_stop )
            break;

        follow_particles( analy->new_trace_pts, vel, analy, TRUE, cnt );

        /* Get P1. */
        pc = analy->new_trace_pts;
        while ( pc != NULL )
        {
            if ( pc->in_grid )
            {
                for ( i = 0; i < dims; i++ )
                    pc->pt[i] += delta_t*pc->vec[i];
                NEXT( pc );
            }
            else
            {
                /* Remove from "under construction" list. */
                p_tp = pc;
                NEXT( pc );
                UNLINK( p_tp, analy->new_trace_pts );
                APPEND( p_tp, analy->trace_pts );
            }
        }

        t += delta_t;
        if ( !static_field ) vel_at_time( t, vel, analy );

        follow_particles( analy->new_trace_pts, vel, analy, FALSE, cnt );

        /* Get P2 and Pn+1. */
        pc = analy->new_trace_pts;
        while ( pc != NULL )
        {
            if ( pc->in_grid )
            {
                for ( i = 0; i < dims; i++ )
                {
                    pc->pt[i] = 0.5*( pc->pt[i] + delta_t * pc->vec[i] +
                                      pc->pts[(cnt-1) * dims + i] );
                    pc->pts[cnt * dims + i] = pc->pt[i];
                }
                pc->time[cnt] = ( static_field ) ? t_zero : t;
                pc->cnt++;
                NEXT( pc );
            }
            else
            {
                /* Remove from "under construction" list. */
                p_tp = pc;
                NEXT( pc );
                UNLINK( p_tp, analy->new_trace_pts );
                APPEND( p_tp, analy->trace_pts );
            }
        }

        cnt++;

        /* Redraw the display. */
        analy->state_p->time = ( static_field ) ? t_zero : t;
        analy->update_display( analy );
    }

    if ( cnt >= MAX_TRACER_STEPS )
    {
        popup_dialog( WARNING_POPUP,
                      "Exhausted room for ptrace; limit to %d steps.",
                      MAX_TRACER_STEPS );
    }

    /* Terminate material segment lists; free unused trace memory. */
    for ( pc = analy->new_trace_pts; pc != NULL; NEXT( pc ) )
    {
        /*
         * Add last material segment for particles which
         * terminated within the grid.
         */
        p_ts = NEW( Trace_segment_obj, "Last trace segment" );
        p_ts->material = pc->mtl_num;
        p_ts->last_pt_idx = pc->cnt - 1;
        APPEND( p_ts, pc->mtl_segments );

        /*
         * Could trim unused "pts" and "time" array space here to what
         * was actually used, but a new ptrace command with different
         * parameters would necessitate keeping track of current allocations
         * or free/realloc's on every ptrace command.
         */
    }

    /* Add remaining new traces to existing traces. */
    if ( analy->new_trace_pts != NULL )
    {
        APPEND( analy->new_trace_pts, analy->trace_pts );
        analy->new_trace_pts = NULL;
    }

    /* Free up memory. */
    for ( i = 0; i < dims; i++ )
        free( vel[i] );

    if ( static_field )
        /* Leave time at static field trace time. */
        change_time( t_zero, analy );
    else
    {
        /* Move to the last state. */
        analy->db_query( analy->db_ident, QRY_STATE_OF_TIME, (void *) &t,
                         NULL, (void *) bound_states );
        state = bound_states[1] - 1;

        if ( state != analy->cur_state )
        {
            analy->cur_state = state;
            change_state( analy );
        }
        else
            analy->db_query( analy->db_ident, QRY_STATE_TIME,
                             (void *) (bound_states + 1), NULL,
                             (void *) &analy->state_p->time );
    }
}


/************************************************************
 * TAG( find_next_subtrace )
 *
 * Find the first contiguous sequence of steps of a particle
 * trace subset for which all steps are in materials that
 * aren't disabled for particle traces.  When "init" is true,
 * initialize the trace subset to those trace steps bounded
 * by "start" and "start + qty".  Thereafter, consider only
 * steps in the initial subset which have not belonged to a
 * previous subtrace.
 *
 * The net effect of a sequence of calls to find_next_subtrace
 * is to partition a trace subset into a disconnected sequence
 * of trace subsets separated by materials for which traces
 * are disabled.
 */
Bool_type
find_next_subtrace( Bool_type init, int skip, int qty, Trace_pt_obj *pt,
                    Analysis *analy, int *new_skip, int *new_qty )
{
    static int start, end;
    static int *vis_map;
    int i, stop;
    Bool_type disabled;
    Trace_segment_obj *p_seg;
    int idum;

    /*
     * On initialization, build an array of flags, one per trace step
     * point, which are FALSE when the point lies in a material which
     * is trace-disabled.
     */
    if ( init )
    {
        end = skip + qty;

        vis_map = NEW_N( Bool_type, end, "Trace vis map" );

        i = 0;
        for ( p_seg = pt->mtl_segments; p_seg != NULL; NEXT( p_seg ) )
        {
            disabled = is_in_iarray( p_seg->material, analy->trace_disable_qty,
                                     analy->trace_disable, &idum );
            for ( ; i <= p_seg->last_pt_idx && i < end; i++ )
                vis_map[i] = ( disabled ) ? FALSE : TRUE;
        }

        /*
         * If the trace is under construction, the length will be longer
         * than the steps accounted for in the material segments list.
         * Pick up the "tail" steps here.
         */
        if ( end > i )
        {
            disabled = is_in_iarray( pt->mtl_num, analy->trace_disable_qty,
                                     analy->trace_disable, &idum );
            for ( ; i < end; i++ )
                vis_map[i] = ( disabled ) ? FALSE : TRUE;
        }

        start = skip;
    }

    /* Find the first step after (or equal to) "start" which is visible. */
    while ( start < end && !vis_map[start] )
        start++;

    if ( start == end )
    {
        /* No visible subtrace left. */
        free( vis_map );
        vis_map = NULL;
        return FALSE;
    }

    /*
     * Traverse only _CONTIGUOUS_ visible steps after "start" and
     * find the last one.
     */
    stop = start + 1;
    while ( stop < end && vis_map[stop] )
        stop++;

    if ( stop == (start + 1) )
    {
        /* No visible subtrace left. */
        free( vis_map );
        vis_map = NULL;
        return FALSE;
    }

    /* Return the end points of the visible subtrace. */
    *new_skip = start;
    *new_qty = stop - start;

    /* Prepare for next iteration. */
    start = stop;

    return TRUE;
}


/*****************************************************************
 * TAG( save_particle_traces )
 *
 * Output ASCII file of particle traces.
 */
void
save_particle_traces( Analysis *analy, char *filename )
{
    Trace_pt_obj *p_tp;
    FILE *ofile;
    int tcnt;

    if ( analy->trace_pts == NULL )
        return;

    if ( (ofile = fopen( filename, "w" )) == NULL )
    {
        popup_dialog( WARNING_POPUP,
                      "Unable to open particle trace output file." );
        return;
    }

    write_preamble( ofile );

    /* Count and output the number of traces. */
    tcnt = 0;
    for ( p_tp = analy->trace_pts; p_tp != NULL; NEXT( p_tp ) )
        tcnt++;
    fprintf( ofile, "# Quantity particle traces: %d\n", tcnt );

    for ( p_tp = analy->trace_pts; p_tp != NULL; NEXT( p_tp ) )
        save_trace( p_tp, analy->dimension, ofile );

    fclose( ofile );

    return;
}


/*****************************************************************
 * TAG( save_trace )
 *
 * Output ASCII file of particle traces.
 *
 * Modifications to the format of this output require changes to
 * read_particle_traces() as well!!!
 */
static void
save_trace( Trace_pt_obj *p_trace, int dims, FILE *ofile )
{
    Trace_segment_obj *p_ts;
    float *p_xyz, *p_time;
    int i, j, mcnt;

    fprintf( ofile, "#\n#                  %s\n#\n", trace_tag );
    fprintf( ofile, "# Points: %d\n", p_trace->cnt );
    fprintf( ofile, "# %s%5.3f  %5.3f  %5.3f\n", color_tag,
             p_trace->color[0], p_trace->color[1], p_trace->color[2] );

    mcnt = 0;
    for ( p_ts = p_trace->mtl_segments; p_ts != NULL; NEXT( p_ts ) )
        mcnt++;

    fprintf( ofile, "# %s%d\n", mtls_trav_tag, mcnt );
    fprintf( ofile, "#      Material   Last point\n" );
    for ( p_ts = p_trace->mtl_segments; p_ts != NULL; NEXT( p_ts ) )
        fprintf( ofile, "#       %3d        %d\n", p_ts->material + 1,
                 p_ts->last_pt_idx );

    fprintf( ofile, "# Position history:\n" );
    fprintf( ofile, "#        %s\n", pos_his_hdr_tag );

    p_xyz = p_trace->pts;
    p_time = p_trace->time;
    for ( i = 0; i < p_trace->cnt; i++ )
    {
        fprintf( ofile, "    %8.6e", *p_time );
        for ( j = 0; j < dims; j++ )
            fprintf( ofile, " %9.6e", p_xyz[j] );
        fprintf( ofile, "\n" );
        p_time++;
        p_xyz += dims;
    }
}


/*****************************************************************
 * TAG( read_particle_traces )
 *
 * Read the particle trace file.
 *
 * The format read MUST be kept current with output from
 * save_particle_traces() and save_trace().
 */
void
read_particle_traces( Analysis *analy, char *filename )
{
    FILE *ifile;
    char *p_c;
    Trace_pt_obj *p_tp;
    Trace_segment_obj *p_ts;
    int i, j, mcnt;
    float *p_time, *p_xyz;
    char txtline[80];
    int dims;

    if ( (ifile = fopen( filename, "r" )) == NULL )
    {
        popup_dialog( WARNING_POPUP, "Unable to open file \"%s\".", filename );
        return;
    }

    dims = analy->dimension;

    /* Locate each particle trace dataset and load a Trace_pt_obj. */
    while ( find_text( ifile, trace_tag ) != NULL )
    {
        p_tp = NEW( Trace_pt_obj, "Trace obj from file" );

        /* Quantity of points in trace. */
        p_tp->cnt = atoi( find_text( ifile, points_tag ) + strlen( points_tag) );

        /* RGB values for trace color. */
        p_c = find_text( ifile, color_tag ) + strlen( color_tag );
        for ( i = 0; i < 3; i++ )
            p_tp->color[i] = (float) strtod( p_c, &p_c );

        /* Traversed materials list. */
        mcnt = atoi( find_text( ifile, mtls_trav_tag )
                     + strlen( mtls_trav_tag ) );
        /* Swallow the column titles. */
        fgets( txtline, 80, ifile );

        /* Now read each material and its last point. */
        for ( i = 0; i < mcnt; i++ )
        {
            p_ts = NEW( Trace_segment_obj, "Trace seg from file" );
            fscanf( ifile, "# %d %d\n", &p_ts->material,
                    &p_ts->last_pt_idx );
            p_ts->material--;
            APPEND( p_ts, p_tp->mtl_segments );
        }

        /* Find the position history. */
        p_c = find_text( ifile, pos_his_hdr_tag );

        /* Allocate arrays to hold the position history. */
        p_tp->pts = NEW_N( float, dims * p_tp->cnt, "Trace points from file" );
        p_tp->time = NEW_N( float, p_tp->cnt, "Trace point times from file" );

        /* Read the position history. */
        p_xyz = p_tp->pts;
        p_time = p_tp->time;
        for ( i = 0; i < p_tp->cnt; i++ )
        {
            fscanf( ifile, " %f", p_time );
            for ( j = 0; j < dims; j++ )
                fscanf( ifile, " %f", p_xyz + j );
            fscanf( ifile, "\n" );
            p_time++;
            p_xyz += dims;
        }

        /* Store the Trace_pt_obj. */
        APPEND( p_tp, analy->trace_pts );
    }

    fclose( ifile );

    /* Enable trace rendering. */
    analy->show_traces = TRUE;
}


/*
 * SECTION( Vector grid routines )
 */


/************************************************************
 * TAG( delete_vec_points )
 *
 * Free vector grid points.
 */
void
delete_vec_points( Analysis *analy )
{
    int i, j;
    int class_qty;
    MO_class_data **p_mo_classes;
    Vector_pt_obj *p_vp;

    if ( !analy->have_grid_points )
        return;

    for ( i = 0; i < analy->mesh_qty; i++ )
    {
#ifdef NEWMILI
        htable_get_data( analy->mesh_table[i].class_table,
                         (void ***) &p_mo_classes ,
                         &class_qty );
#else
        class_qty = htable_get_data( analy->mesh_table[i].class_table,
                                     (void ***) &p_mo_classes );
#endif

        for ( j = 0; j < class_qty; j++ )
        {
            if ( p_mo_classes[j]->vector_pts != NULL )
            {
                p_vp = p_mo_classes[j]->vector_pts;
                DELETE_LIST( p_vp );
                p_mo_classes[j]->vector_pts = NULL;
                p_mo_classes[j]->vector_pt_qty = 0;
            }
        }

        free( p_mo_classes );
    }

    analy->have_grid_points = FALSE;
}


/************************************************************
 * TAG( gen_vec_points )
 *
 * Generate a grid of point locations at which to display
 * vectors.
 *
 * dim              - (1, 2 or 3) Generate a linear, planar
 *                    or volume grid.
 * size             - Number of values in each direction.
 * start_pt, end_pt - Boundaries of region.
 */
void
gen_vec_points( int dim, int size[3], float start_pt[3], float end_pt[3],
                Analysis *analy )
{
    Vector_pt_obj *pt, *ptlist;
    float t;
    int ind1, ind2, inds;
    int i, j, k;

    ptlist = NULL;
    switch ( dim )
    {
    case 1:
        for ( i = 0; i < size[0]; i++ )
        {
            pt = NEW( Vector_pt_obj, "Vector point object" );

            if ( size[0] == 1 )
                t = 0.0;
            else
                t = i / (float)( size[0] - 1 );
            for ( j = 0; j < 3; j++ )
                pt->pt[j] = (1.0-t)*start_pt[j] + t*end_pt[j];
            INSERT( pt, ptlist );
        }
        break;
    case 2:
        ind1 = 0;
        ind2 = 2;
        inds = 1;
        if ( start_pt[0] == end_pt[0] )
        {
            ind1 = 1;
            inds = 0;
        }
        else if ( start_pt[2] == end_pt[2] )
        {
            ind2 = 1;
            inds = 2;
        }
        for ( i = 0; i < size[0]; i++ )
            for ( j = 0; j < size[1]; j++ )
            {
                pt = NEW( Vector_pt_obj, "Vector point object" );

                t = i / (float)( size[0] - 1 );
                pt->pt[ind1] = (1.0-t)*start_pt[ind1] + t*end_pt[ind1];

                t = j / (float)( size[1] - 1 );
                pt->pt[ind2] = (1.0-t)*start_pt[ind2] + t*end_pt[ind2];

                pt->pt[inds] = start_pt[inds];

                INSERT( pt, ptlist );
            }
        break;
    case 3:
        for ( i = 0; i < size[0]; i++ )
            for ( j = 0; j < size[1]; j++ )
                for ( k = 0; k < size[2]; k++ )
                {
                    pt = NEW( Vector_pt_obj, "Vector point object" );

                    t = i / (float)( size[0] - 1 );
                    pt->pt[0] = (1.0-t)*start_pt[0] + t*end_pt[0];

                    t = j / (float)( size[1] - 1 );
                    pt->pt[1] = (1.0-t)*start_pt[1] + t*end_pt[1];

                    t = k / (float)( size[2] - 1 );
                    pt->pt[2] = (1.0-t)*start_pt[2] + t*end_pt[2];

                    INSERT( pt, ptlist );
                }
        break;
    default:
        popup_dialog( WARNING_POPUP,
                      "gen_vec_points(): bad switch statement value." );
    }

    /* For each mesh, locate the element that each vector point lies in. */
    for ( i = 0; i < analy->mesh_qty; i++ )
    {
        if ( analy->dimension == 3 )
        {
            init_vec_points_hex( &ptlist, i, analy, TRUE );
            init_vec_points_tet( &ptlist, i, analy, TRUE );
        }
        else
        {
            init_vec_points_quad_2d( &ptlist, i, analy );
            init_vec_points_tri_2d( &ptlist, i, analy );
            init_vec_points_surf_2d( &ptlist, i, analy );
        }
    }

    /*
     * All points located within elements have been moved to lists
     * in the respective element classes.  Eliminate any leftovers.
     */
    if ( ptlist != NULL )
        DELETE_LIST( ptlist );

    /* Update the display vectors if needed. */
    update_vec_points( analy );
}


/************************************************************
 * TAG( init_vec_points_hex )
 *
 * For each point in the list, find which if any hex element
 * and class it initially lies in, and move the object to the
 * vector point list for that class.
 */
/***
static void
***/
void
init_vec_points_hex( Vector_pt_obj **ptlist, int mesh_id, Analysis *analy, Bool_type update_class )
{
    Vector_pt_obj *pt;
    Bool_type found;
    float verts[8][3];
    float *lo[3], *hi[3];
    float xi[3];
    float val;
    int i, j, k, l, el, nd;
    Mesh_data *p_mesh;
    MO_class_data *p_hex_class;
    int (*connects)[8];
    GVec3D *nodes;
    Elem_block_obj *p_ebo;
    Vector_pt_obj *mov_pt;

    p_mesh = analy->mesh_table + mesh_id;
    for ( l = 0; l < p_mesh->classes_by_sclass[G_HEX].qty; l++ )
    {
        p_hex_class = ((MO_class_data **)
                       p_mesh->classes_by_sclass[G_HEX].list)[l];

        connects = (int (*)[8]) p_hex_class->objects.elems->nodes;
        nodes = analy->state_p->nodes.nodes3d;

        /* Compute a bounding box for each element in advance. */
        for ( i = 0; i < 3; i++ )
        {
            lo[i] = analy->tmp_result[i];
            hi[i] = analy->tmp_result[i+3];
        }

        for ( i = 0; i < p_hex_class->qty; i++ )
        {
            nd = connects[i][0];
            for ( k = 0; k < 3; k++ )
            {
                lo[k][i] = nodes[nd][k];
                hi[k][i] = nodes[nd][k];
            }

            for ( j = 1; j < 8; j++ )
            {
                nd = connects[i][j];
                for ( k = 0; k < 3; k++ )
                {
                    val = nodes[nd][k];
                    if ( val < lo[k][i] )
                        lo[k][i] = val;
                    else if ( val > hi[k][i] )
                        hi[k][i] = val;
                }
            }
        }

        /* Now do the point tests. */
        p_ebo = p_hex_class->p_elem_block;
        for ( pt = *ptlist; pt != NULL; )
        {
            found = FALSE;

            for ( i = 0; i < p_ebo->num_blocks; i++ )
            {
                /* See if the point lies in the block's bounding box first. */
                if ( pt->pt[0] < p_ebo->block_bbox[0][0][i] ||
                        pt->pt[1] < p_ebo->block_bbox[0][1][i] ||
                        pt->pt[2] < p_ebo->block_bbox[0][2][i] ||
                        pt->pt[0] > p_ebo->block_bbox[1][0][i] ||
                        pt->pt[1] > p_ebo->block_bbox[1][1][i] ||
                        pt->pt[2] > p_ebo->block_bbox[1][2][i] )
                    continue;

                /* Test point against elements in block. */
                for ( el = p_ebo->block_lo[i]; el <= p_ebo->block_hi[i]; el++ )
                {
                    /* Perform approximate inside (bbox) test first. */
                    if ( pt->pt[0] >= lo[0][el] && pt->pt[0] <= hi[0][el] &&
                            pt->pt[1] >= lo[1][el] && pt->pt[1] <= hi[1][el] &&
                            pt->pt[2] >= lo[2][el] && pt->pt[2] <= hi[2][el] )
                    {
                        for ( j = 0; j < 8; j++ )
                        {
                            nd = connects[el][j];
                            for ( k = 0; k < 3; k++ )
                                verts[j][k] = nodes[nd][k];
                        }

                        if ( pt_in_hex( verts, pt->pt, xi ) )
                        {
                            pt->elnum = el;
                            VEC_COPY( pt->xi, xi );

                            mov_pt = pt;
                            NEXT( pt );

                            if ( TRUE == update_class )
                            {
                                UNLINK( mov_pt, *ptlist );
                                INSERT( mov_pt, p_hex_class->vector_pts );
                                p_hex_class->vector_pt_qty++;
                            }

                            found = TRUE;
                            break;
                        }
                    }
                }

                if ( found )
                    break;
            }

            if ( !found )
                NEXT( pt );
        }
    }
}


/************************************************************
 * TAG( init_vec_points_tet )
 *
 * For each point in the list, find which if any tet element
 * and class it initially lies in, and move the object to the
 * vector point list for that class.
 */
static void
init_vec_points_tet( Vector_pt_obj **ptlist, int mesh_id, Analysis *analy, Bool_type update_class )
{
    Vector_pt_obj *pt;
    Bool_type found;
    float verts[4][3];
    float *lo[3], *hi[3];
    float xi[4];
    float val;
    int i, j, k, l, el, nd;
    Mesh_data *p_mesh;
    MO_class_data *p_tet_class;
    int (*connects)[4];
    GVec3D *nodes;
    Elem_block_obj *p_ebo;
    Vector_pt_obj *mov_pt;

    p_mesh = analy->mesh_table + mesh_id;
    for ( l = 0; l < p_mesh->classes_by_sclass[G_TET].qty; l++ )
    {
        p_tet_class = ((MO_class_data **)
                       p_mesh->classes_by_sclass[G_TET].list)[l];

        connects = (int (*)[4]) p_tet_class->objects.elems->nodes;
        nodes = analy->state_p->nodes.nodes3d;

        /* Compute a bounding box for each element in advance. */
        for ( i = 0; i < 3; i++ )
        {
            lo[i] = analy->tmp_result[i];
            hi[i] = analy->tmp_result[i+3];
        }

        for ( i = 0; i < p_tet_class->qty; i++ )
        {
            nd = connects[i][0];
            for ( k = 0; k < 3; k++ )
            {
                lo[k][i] = nodes[nd][k];
                hi[k][i] = nodes[nd][k];
            }

            for ( j = 1; j < 4; j++ )
            {
                nd = connects[k][j];
                for ( k = 0; k < 3; k++ )
                {
                    val = nodes[nd][k];
                    if ( val < lo[k][i] )
                        lo[k][i] = val;
                    else if ( val > hi[k][i] )
                        hi[k][i] = val;
                }
            }
        }

        /* Now do the point tests. */
        p_ebo = p_tet_class->p_elem_block;
        for ( pt = *ptlist; pt != NULL; )
        {
            found = FALSE;

            for ( i = 0; i < p_ebo->num_blocks; i++ )
            {
                /* See if the point lies in the block's bounding box first. */
                if ( pt->pt[0] < p_ebo->block_bbox[0][0][i] ||
                        pt->pt[1] < p_ebo->block_bbox[0][1][i] ||
                        pt->pt[2] < p_ebo->block_bbox[0][2][i] ||
                        pt->pt[0] > p_ebo->block_bbox[1][0][i] ||
                        pt->pt[1] > p_ebo->block_bbox[1][1][i] ||
                        pt->pt[2] > p_ebo->block_bbox[1][2][i] )
                    continue;

                /* Test point against elements in block. */
                for ( el = p_ebo->block_lo[i]; el <= p_ebo->block_hi[i]; el++ )
                {
                    /* Perform approximate inside (bbox) test first. */
                    if ( pt->pt[0] >= lo[0][el] && pt->pt[0] <= hi[0][el] &&
                            pt->pt[1] >= lo[1][el] && pt->pt[1] <= hi[1][el] &&
                            pt->pt[2] >= lo[2][el] && pt->pt[2] <= hi[2][el] )
                    {
                        for ( j = 0; j < 4; j++ )
                        {
                            nd = connects[el][j];
                            for ( k = 0; k < 3; k++ )
                                verts[j][k] = nodes[nd][k];
                        }

                        if ( pt_in_tet( verts, pt->pt, xi ) )
                        {
                            pt->elnum = el;
                            pt->xi[0] = xi[0];
                            pt->xi[1] = xi[1];
                            pt->xi[2] = xi[2];
                            pt->xi[3] = xi[3];

                            mov_pt = pt;
                            NEXT( pt );

                            if ( update_class )
                            {
                                UNLINK( mov_pt, *ptlist );
                                INSERT( mov_pt, p_tet_class->vector_pts );
                                p_tet_class->vector_pt_qty++;
                            }

                            found = TRUE;
                            break;
                        }
                    }
                }

                if ( found )
                    break;
            }

            if ( !found )
                NEXT( pt );
        }
    }
}


/************************************************************
 * TAG( init_vec_points_quad_2d )
 *
 * For each point in the list, find which if any quad element
 * and class it initially lies in, and move the object to the
 * vector point list for that class.
 */
static void
init_vec_points_quad_2d( Vector_pt_obj **ptlist, int mesh_id, Analysis *analy )
{
    Vector_pt_obj *pt;
    Bool_type found;
    float verts[4][3];
    float xi[2];
    int i, j, k, l, el, nd;
    Mesh_data *p_mesh;
    MO_class_data *p_quad_class;
    int (*connects)[4];
    GVec2D *nodes;
    Elem_block_obj *p_ebo;
    Vector_pt_obj *mov_pt;

    p_mesh = analy->mesh_table + mesh_id;
    for ( l = 0; l < p_mesh->classes_by_sclass[G_QUAD].qty; l++ )
    {
        p_quad_class = ((MO_class_data **)
                        p_mesh->classes_by_sclass[G_QUAD].list)[l];

        connects = (int (*)[4]) p_quad_class->objects.elems->nodes;
        nodes = analy->state_p->nodes.nodes2d;
        p_ebo = p_quad_class->p_elem_block;

        for ( pt = *ptlist; pt != NULL; )
        {
            found = FALSE;

            for ( i = 0; i < p_ebo->num_blocks; i++ )
            {
                /* See if the point lies in the block's bounding box first. */
                if ( pt->pt[0] < p_ebo->block_bbox[0][0][i] ||
                        pt->pt[1] < p_ebo->block_bbox[0][1][i] ||
                        pt->pt[0] > p_ebo->block_bbox[1][0][i] ||
                        pt->pt[1] > p_ebo->block_bbox[1][1][i] )
                    continue;

                /* Test point against elements in block. */
                for ( el = p_ebo->block_lo[i]; el <= p_ebo->block_hi[i]; el++ )
                {
                    for ( j = 0; j < 4; j++ )
                    {
                        nd = connects[el][j];
                        for ( k = 0; k < 2; k++ )
                            verts[j][k] = nodes[nd][k];
                    }

                    /* Perform approximate inside test first. */
                    if ( pt_in_quad_bbox( verts, pt->pt ) &&
                            pt_in_quad( verts, pt->pt, xi ) )
                    {
                        pt->elnum = el;
                        pt->xi[0] = xi[0];
                        pt->xi[1] = xi[1];

                        mov_pt = pt;
                        NEXT( pt );
                        UNLINK( mov_pt, *ptlist );
                        INSERT( mov_pt, p_quad_class->vector_pts );
                        p_quad_class->vector_pt_qty++;

                        found = TRUE;
                        break;
                    }
                }

                if ( found )
                    break;
            }

            if ( !found )
                NEXT( pt );
        }
    }
}


/************************************************************
 * TAG( init_vec_points_tri_2d )
 *
 * For each point in the list, find which if any tri element
 * and class it initially lies in, and move the object to the
 * vector point list for that class.
 */
static void
init_vec_points_tri_2d( Vector_pt_obj **ptlist, int mesh_id, Analysis *analy )
{
    Vector_pt_obj *pt;
    Bool_type found;
    float verts[3][3];
    float xi[3];
    int i, j, k, l, el, nd;
    Mesh_data *p_mesh;
    MO_class_data *p_tri_class;
    int (*connects)[3];
    GVec2D *nodes;
    Elem_block_obj *p_ebo;
    Vector_pt_obj *mov_pt;

    p_mesh = analy->mesh_table + mesh_id;
    for ( l = 0; l < p_mesh->classes_by_sclass[G_TRI].qty; l++ )
    {
        p_tri_class = ((MO_class_data **)
                       p_mesh->classes_by_sclass[G_TRI].list)[l];

        connects = (int (*)[3]) p_tri_class->objects.elems->nodes;
        nodes = analy->state_p->nodes.nodes2d;
        p_ebo = p_tri_class->p_elem_block;

        for ( pt = *ptlist; pt != NULL; )
        {
            found = FALSE;

            for ( i = 0; i < p_ebo->num_blocks; i++ )
            {
                /* See if the point lies in the block's bounding box first. */
                if ( pt->pt[0] < p_ebo->block_bbox[0][0][i] ||
                        pt->pt[1] < p_ebo->block_bbox[0][1][i] ||
                        pt->pt[0] > p_ebo->block_bbox[1][0][i] ||
                        pt->pt[1] > p_ebo->block_bbox[1][1][i] )
                    continue;

                /* Test point against elements in block. */
                for ( el = p_ebo->block_lo[i]; el <= p_ebo->block_hi[i]; el++ )
                {
                    for ( j = 0; j < 3; j++ )
                    {
                        nd = connects[el][j];
                        for ( k = 0; k < 2; k++ )
                            verts[j][k] = nodes[nd][k];
                    }

                    /* Perform approximate inside test first. */
                    if ( pt_in_tri_bbox( verts, pt->pt ) &&
                            pt_in_tri( verts, pt->pt, xi ) )
                    {
                        pt->elnum = el;
                        pt->xi[0] = xi[0];
                        pt->xi[1] = xi[1];
                        pt->xi[2] = xi[2];

                        mov_pt = pt;
                        NEXT( pt );
                        UNLINK( mov_pt, *ptlist );
                        INSERT( mov_pt, p_tri_class->vector_pts );
                        p_tri_class->vector_pt_qty++;

                        found = TRUE;
                        break;
                    }
                }

                if ( found )
                    break;
            }

            if ( !found )
                NEXT( pt );
        }
    }
}


/************************************************************
 * TAG( init_vec_points_surf_2d )
 *
 * For each point in the list, find which if any surface element
 * and class it initially lies in, and move the object to the
 * vector point list for that class.
 */
static void
init_vec_points_surf_2d( Vector_pt_obj **ptlist, int mesh_id, Analysis *analy )
{
    Vector_pt_obj *pt;
    Bool_type found;
    float verts[4][3];
    float xi[2];
    int i, j, k, l, el, nd;
    Mesh_data *p_mesh;
    MO_class_data *p_surf_class;
    int (*connects)[4];
    GVec2D *nodes;
    Elem_block_obj *p_ebo;
    Vector_pt_obj *mov_pt;

    p_mesh = analy->mesh_table + mesh_id;
    for ( l = 0; l < p_mesh->classes_by_sclass[G_SURFACE].qty; l++ )
    {
        p_surf_class = ((MO_class_data **)
                        p_mesh->classes_by_sclass[G_SURFACE].list)[l];

        connects = (int (*)[4]) p_surf_class->objects.elems->nodes;
        nodes = analy->state_p->nodes.nodes2d;
        p_ebo = p_surf_class->p_elem_block;

        for ( pt = *ptlist; pt != NULL; )
        {
            found = FALSE;

            for ( i = 0; i < p_ebo->num_blocks; i++ )
            {
                /* See if the point lies in the block's bounding box first. */
                if ( pt->pt[0] < p_ebo->block_bbox[0][0][i] ||
                        pt->pt[1] < p_ebo->block_bbox[0][1][i] ||
                        pt->pt[0] > p_ebo->block_bbox[1][0][i] ||
                        pt->pt[1] > p_ebo->block_bbox[1][1][i] )
                    continue;

                /* Test point against elements in block. */
                for ( el = p_ebo->block_lo[i]; el <= p_ebo->block_hi[i]; el++ )
                {
                    for ( j = 0; j < 4; j++ )
                    {
                        nd = connects[el][j];
                        for ( k = 0; k < 2; k++ )
                            verts[j][k] = nodes[nd][k];
                    }

                    /* Perform approximate inside test first. */
                    if ( pt_in_surf_bbox( verts, pt->pt ) &&
                            pt_in_surf( verts, pt->pt, xi ) )
                    {
                        pt->elnum = el;
                        pt->xi[0] = xi[0];
                        pt->xi[1] = xi[1];

                        mov_pt = pt;
                        NEXT( pt );
                        UNLINK( mov_pt, *ptlist );
                        INSERT( mov_pt, p_surf_class->vector_pts );
                        p_surf_class->vector_pt_qty++;

                        found = TRUE;
                        break;
                    }
                }

                if ( found )
                    break;
            }

            if ( !found )
                NEXT( pt );
        }
    }
}


/**/
/* Needs to be updated to allow for indirect results!!! */
/************************************************************
 * TAG( update_vec_points )
 *
 * For each point in the vector point list, update the displayed
 * vector at that point.
 *
 * New notes: Vectors are only supported by node- and element-
 * based results.  The struct analy->vector_srec_map is pre-
 * calculated to determine which subrecords of each srec support
 * the specified vector components.  If the derivation of
 * analy->vector_srec_map changes to permit cross-class
 * combinations of results, this logic may need to be updated.
 *
 * Out-of-date, but touches on some pertinent issues:
 * Notes: Considering that non-element-based results (i.e., nodal,
 * material, mesh results) require elements to "house" the vector
 * grid points, those vector component results are gathered over
 * the entire mesh using load_results() and it is left to the user
 * to know that all components are actually available where the
 * vector grid points exist.  A more user-friendly policy might
 * demand some constraints dictating that non-element-based
 * subrecord bindings align physically on element-class boundaries.
 * This touches on the historical issue of nodes' treatment as
 * independent from materials/elements that reference them.
 * For vectors with any element-based results as components,
 * before doing any I/O or result computation for the element-based
 * results we verify that a subrecord supports all element-based
 * results required by the vector and that the class bound to the
 * subrecord has vector points in it.  This limits I/O and computation
 * to subrecord granularity, on subrecords where it's guaranteed to be
 * useful.
 */
void
update_vec_points( Analysis *analy )
{
    float *vec_data[3];
    double vmin, vmax;
    int i, j, node_cnt, node_ref_qty;
    int *refd_nodes;
    Mesh_data *p_mesh;
    int dimension;
    int qty_subrecs, qty_classes;
    int srec_id;
    int *subrecs;
    Subrec_obj *p_subrec;
    Vec_func vec_funcs[QTY_SCLASS] =
    {
        NULL, NULL, NULL, NULL, /* G_UNIT, G_NODE, G_TRUSS, G_BEAM */

        calc_tri_vec_result,    /* G_TRI */
        calc_quad_vec_result,   /* G_QUAD */
        calc_tet_vec_result,    /* G_TET */

        NULL, NULL,             /* G_PYRAMID, G_WEDGE */

        calc_hex_vec_result,    /* G_HEX */

        NULL, NULL,             /* G_MAT, G_MESH */
        calc_surf_vec_result    /* G_SURFACE */
    };

    Result *tmp_res;
    float *tmp_res_data;
    Bool_type convert, nodal_data;
    int node;
    MO_class_data *p_mo_class, *p_node_class;
    MO_class_data **mo_classes;
    float scale, offset;
    List_head *srec_map;

    if ( analy->show_vectors == FALSE )
        return;

    p_mesh = MESH_P( analy );
    p_node_class = p_mesh->node_geom;
    node_cnt = p_node_class->qty;

    dimension = analy->dimension;

    if ( dimension == 3 )
    {
        if ( p_mesh->classes_by_sclass[G_HEX].qty == 0
                && p_mesh->classes_by_sclass[G_TET].qty == 0 )
            return;
    }
    else /* dimension is 2 */
    {
        if ( p_mesh->classes_by_sclass[G_QUAD].qty == 0
                && p_mesh->classes_by_sclass[G_TRI].qty == 0
                && p_mesh->classes_by_sclass[G_SURFACE].qty == 0)
            return;
    }

    /* Allocate vector component arrays. */
    for ( i = 0; i < dimension; i++ )
        vec_data[i] = NEW_N( float, node_cnt, "Vec result" );

    /* Init vector min and max. */
    vmin = MAXFLOAT;
    vmax = -MAXFLOAT;

    /* Init for units conversion. */
    convert = analy->perform_unit_conversion;
    if ( convert )
    {
        scale = analy->conversion_scale;
        offset = analy->conversion_offset;
    }

    /* Save references to current result and result array. */
    tmp_res = analy->cur_result;
    tmp_res_data = p_node_class->data_buffer;

    /*
     * Since we're getting results by subrecord, explicitly update
     * the vector component Result structs beforehand.
     */
    for ( i = 0; i < dimension; i++ )
    {
        if ( analy->vector_result[i] == NULL )
            continue;
        analy->cur_result = analy->vector_result[i];
        update_result( analy, analy->cur_result );
    }

    /*
     * Get results according to vector srec map.
     */

    srec_map = analy->vector_srec_map;
    srec_id = analy->state_p->srec_id;
    qty_subrecs = srec_map[srec_id].qty;
    subrecs = (int *) srec_map[srec_id].list;

    for ( i = 0; i < qty_subrecs; i++ )
    {
        for ( j = 0; j < dimension; j++ )
        {
            /* Load each result from current subrecord. */
            analy->cur_result = analy->vector_result[j];
            p_node_class->data_buffer = vec_data[j];
            load_subrecord_result( analy, subrecs[i], FALSE, TRUE );
        }

        if ( convert )
        {
            p_subrec = analy->srec_tree[srec_id].subrecs + subrecs[i];

            if ( p_subrec->p_object_class->superclass == G_NODE
                    && p_subrec->object_ids == NULL )
            {
                for ( j = 0; j < node_cnt; j++ )
                {
                    vec_data[0][j] = vec_data[0][j] * scale + offset;
                    vec_data[1][j] = vec_data[1][j] * scale + offset;
                    if ( dimension == 3 )
                        vec_data[2][j] = vec_data[2][j] * scale + offset;
                }
            }
            else
            {
                /*
                 * Need indirection either because a nodal subrecord was
                 * non-proper or it's an element subrecord.
                 */
                if ( p_subrec->p_object_class->superclass == G_NODE )
                {
                    /* Nodal data. Use object_ids array. */
                    node_ref_qty = p_subrec->subrec.qty_objects;
                    refd_nodes = p_subrec->object_ids;
                }
                else
                {
                    /* Element data.  Use referenced_nodes array. */
                    node_ref_qty = p_subrec->ref_node_qty;
                    refd_nodes = p_subrec->referenced_nodes;
                }

                for ( j = 0; j < node_ref_qty; j++ )
                {
                    node = refd_nodes[j];

                    vec_data[0][node] = vec_data[0][node] * scale + offset;
                    vec_data[1][node] = vec_data[1][node] * scale + offset;
                    if ( dimension == 3 )
                        vec_data[2][node] = vec_data[2][node] * scale + offset;
                }
            }
        }
    }

    analy->cur_result = tmp_res;
    p_node_class->data_buffer = tmp_res_data;

    /*
     * For nodal-result-based vectors, just update vectors over all
     * classes that have vector points defined on them.  For element-
     * result-based vectors, update vectors on the classes that
     * have subrecords supporting the vector components.
     *
     * Find the first non-NULL result and use it to indicate nodal vs.
     * element result, since it's highly unlikely we'll have any result
     * supported on both nodal and element subrecords.
     */

    nodal_data = FALSE;
    for ( i = 0; i < dimension; i++ )
    {
        if ( analy->vector_result[i] != NULL )
            if ( analy->vector_result[i]->origin.is_node_result )
                nodal_data = TRUE;
    }

    if ( nodal_data )
    {
        /*
         * Render nodal-result-based vectors on any classes that can and
         * do have vector points defined on them.
         */
        for ( i = 0; i < QTY_SCLASS; i++ )
        {
            if ( vec_funcs[i] == NULL )
                continue;

            qty_classes = p_mesh->classes_by_sclass[i].qty;
            mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[i].list;

            for ( j = 0; j < qty_classes; j++ )
            {
                if ( mo_classes[j]->vector_pts == NULL )
                    continue;

                vec_funcs[i]( &vmin, &vmax, vec_data, mo_classes[j] );
            }
        }
    }
    else
    {
        for ( i = 0; i < qty_subrecs; i++ )
        {
            p_subrec = analy->srec_tree[srec_id].subrecs + subrecs[i];
            p_mo_class = p_subrec->p_object_class;

            if ( p_mo_class->vector_pts != NULL
                    &&  vec_funcs[p_mo_class->superclass] != NULL )
                vec_funcs[p_mo_class->superclass]( &vmin, &vmax, vec_data,
                                                   p_mo_class );
        }
    }

    analy->vec_min_mag = sqrt( vmin );
    analy->vec_max_mag = sqrt( vmax );

    for ( i = 0; i < dimension; i++ )
        free( vec_data[i] );
}


/************************************************************
 * TAG( calc_hex_vec_result )
 *
 * Calculate vector results at arbitrary points in a hexahedral
 * element grid by interpolating the nodal point component data.
 */
void
calc_hex_vec_result( double *p_vmin, double *p_vmax, float *vec_comps[3],
                     MO_class_data *p_hex_class )
{
    Vector_pt_obj *pt;
    float h[8];
    int i, j;
    int (*connects)[8];
    int nd;
    double vmag;

    connects = (int (*)[8]) p_hex_class->objects.elems->nodes;

    for ( pt = p_hex_class->vector_pts; pt != NULL; NEXT( pt ) )
    {
        VEC_SET( pt->vec, 0.0, 0.0, 0.0 );

        /* 3D data. */
        shape_fns_hex( pt->xi[0], pt->xi[1], pt->xi[2], h );

        for ( i = 0; i < 8; i++ )
        {
            nd = connects[pt->elnum][i];
            for ( j = 0; j < 3; j++ )
                pt->vec[j] += h[i]*vec_comps[j][nd];
        }

        vmag = VEC_DOT( pt->vec, pt->vec );

        if ( vmag > *p_vmax )
            *p_vmax = vmag;
        if ( vmag < *p_vmin )
            *p_vmin = vmag;
    }
}


/************************************************************
 * TAG( calc_tet_vec_result )
 *
 * Calculate vector results at arbitrary points in a tetrahedral
 * element grid by interpolating the nodal point component data.
 */
void
calc_tet_vec_result( double *p_vmin, double *p_vmax, float *vec_comps[3],
                     MO_class_data *p_tet_class )
{
    Vector_pt_obj *pt;
    int i, j;
    int (*connects)[4];
    int nd;
    double vmag;

    connects = (int (*)[4]) p_tet_class->objects.elems->nodes;

    for ( pt = p_tet_class->vector_pts; pt != NULL; NEXT( pt ) )
    {
        VEC_SET( pt->vec, 0.0, 0.0, 0.0 );

        for ( i = 0; i < 4; i++ )
        {
            nd = connects[pt->elnum][i];
            for ( j = 0; j < 3; j++ )
                pt->vec[j] += pt->xi[i] * vec_comps[j][nd];
        }

        vmag = VEC_DOT( pt->vec, pt->vec );

        if ( vmag > *p_vmax )
            *p_vmax = vmag;
        if ( vmag < *p_vmin )
            *p_vmin = vmag;
    }
}


/************************************************************
 * TAG( calc_quad_vec_result )
 *
 * Calculate vector results at arbitrary points in a 2D quadrilateral
 * element grid by interpolating the nodal point component data.
 */
void
calc_quad_vec_result( double *p_vmin, double *p_vmax, float *vec_comps[2],
                      MO_class_data *p_quad_class )
{
    Vector_pt_obj *pt;
    float h[4];
    int i;
    int (*connects)[4];
    int nd;
    double vmag;

    connects = (int (*)[4]) p_quad_class->objects.elems->nodes;

    for ( pt = p_quad_class->vector_pts; pt != NULL; NEXT( pt ) )
    {
        VEC_SET( pt->vec, 0.0, 0.0, 0.0 );

        /* 2D data. */
        shape_fns_quad( pt->xi[0], pt->xi[1], h );

        for ( i = 0; i < 4; i++ )
        {
            nd = connects[pt->elnum][i];
            pt->vec[0] += h[i] * vec_comps[0][nd];
            pt->vec[1] += h[i] * vec_comps[1][nd];
        }

        vmag = VEC_DOT_2D( pt->vec, pt->vec );

        if ( vmag > *p_vmax )
            *p_vmax = vmag;
        if ( vmag < *p_vmin )
            *p_vmin = vmag;
    }
}


/************************************************************
 * TAG( calc_tri_vec_result )
 *
 * Calculate vector results at arbitrary points in a 2D triangle
 * element grid by interpolating the nodal point component data.
 */
void
calc_tri_vec_result( double *p_vmin, double *p_vmax, float *vec_comps[2],
                     MO_class_data *p_tri_class )
{
    Vector_pt_obj *pt;
    int i;
    int (*connects)[3];
    int nd;
    double vmag;

    connects = (int (*)[3]) p_tri_class->objects.elems->nodes;

    for ( pt = p_tri_class->vector_pts; pt != NULL; NEXT( pt ) )
    {
        VEC_SET( pt->vec, 0.0, 0.0, 0.0 );

        for ( i = 0; i < 3; i++ )
        {
            nd = connects[pt->elnum][i];
            pt->vec[0] += pt->xi[i] * vec_comps[0][nd];
            pt->vec[1] += pt->xi[i] * vec_comps[1][nd];
        }

        vmag = VEC_DOT_2D( pt->vec, pt->vec );

        if ( vmag > *p_vmax )
            *p_vmax = vmag;
        if ( vmag < *p_vmin )
            *p_vmin = vmag;
    }
}


/************************************************************
 * TAG( calc_surf_vec_result )
 *
 * Calculate vector results at arbitrary points in a 2D surface
 * element grid by interpolating the nodal point component data.
 */
void
calc_surf_vec_result( double *p_vmin, double *p_vmax, float *vec_comps[2],
                      MO_class_data *p_surf_class )
{
    Vector_pt_obj *pt;
    float h[4];
    int i;
    int (*connects)[4];
    int nd;
    double vmag;

    connects = (int (*)[4]) p_surf_class->objects.elems->nodes;

    for ( pt = p_surf_class->vector_pts; pt != NULL; NEXT( pt ) )
    {
        VEC_SET( pt->vec, 0.0, 0.0, 0.0 );

        /* 2D data. */
        shape_fns_surf( pt->xi[0], pt->xi[1], h );

        for ( i = 0; i < 4; i++ )
        {
            nd = connects[pt->elnum][i];
            pt->vec[0] += h[i] * vec_comps[0][nd];
            pt->vec[1] += h[i] * vec_comps[1][nd];
        }

        vmag = VEC_DOT_2D( pt->vec, pt->vec );

        if ( vmag > *p_vmax )
            *p_vmax = vmag;
        if ( vmag < *p_vmin )
            *p_vmin = vmag;
    }
}


#ifdef HAVE_VECTOR_CARPETS

/*
 * SECTION( Vector carpet routines )
 */


/************************************************************
 * TAG( ramp_random )
 *
 * Generate a random coordinate with a linear ramped
 * probability distribution.
 */
static float
ramp_random()
{
    float g, h;

    /* Probability distribution is a linear ramp with probability
     * of one for val == 0 and probability of zero for val == 1.
     * That is, Probability(V) = 1 - V where V is in (0,1).
     */
    while ( TRUE )
    {
        g = drand48();
        h = drand48();

        if ( h > g )
            return g;
    }
}


/************************************************************
 * TAG( rand_bary_pt )
 *
 * Given a triangle embedded in an element, generate a random
 * point on the triangle and return the natural coordinates of
 * the point with respect to the element.
 */
static void
rand_bary_pt( float tri_vert[3][3], float hex_vert[8][3], float xi[3] )
{
    float r, s, t, pt[3];
    int i;

    /* Get barycentric coordinates of point on triangle. */
    r = ramp_random();
    s = (1.0 - r)*drand48();
    t = 1.0 - r - s;

    /* Get physical coordinates of point. */
    for ( i = 0; i < 3; i++ )
        pt[i] = r*tri_vert[0][i] + s*tri_vert[1][i] + t*tri_vert[2][i];

    /* Get natural coordinates of point on element. */
    if ( !pt_in_hex( hex_vert, pt, xi ) )
    {
        xi[0] = BOUND( -1.0, xi[0], 1.0 );
        xi[1] = BOUND( -1.0, xi[1], 1.0 );
        xi[2] = BOUND( -1.0, xi[2], 1.0 );
    }
}


/************************************************************
 * TAG( gen_carpet_points )
 *
 * Generate carpet points for all triangles on the cut planes
 * and on the isosurfaces if displayed, otherwise generate
 * carpet on the grid itself.
 */
void
gen_carpet_points( Analysis *analy )
{
    int i;

    /* Free any existing carpet points. */
    if ( analy->vol_carpet_cnt > 0 )
    {
        for ( i = 0; i < 3; i++ )
            free( analy->vol_carpet_coords[i] );
        free( analy->vol_carpet_elem );
        analy->vol_carpet_cnt = 0;
    }
    if ( analy->shell_carpet_cnt > 0 )
    {
        for ( i = 0; i < 2; i++ )
            free( analy->shell_carpet_coords[i] );
        free( analy->shell_carpet_elem );
        analy->shell_carpet_cnt = 0;
    }

    if ( !analy->show_carpet )
        return;

    if ( analy->isosurf_poly != NULL || analy->cut_poly_lists != NULL ||
            analy->ref_polys != NULL || analy->reg_dim[0] > 0 )
        gen_iso_carpet_points( analy );
    else
        gen_grid_carpet_points( analy );
}


/************************************************************
 * TAG( gen_iso_carpet_points )
 *
 * Generate carpet points for all triangles on the cut planes
 * and on the isosurfaces if displayed.
 */
static void
gen_iso_carpet_points( Analysis *analy )
{
    Hex_geom *bricks;
    Nodal *nodes;
    Triangle_poly *tri;
    float area;
    float verts[8][3], xi[3];
    float *carpet_coords[3], *ftmp;
    float vecs_per_area, vec_num, remain;
    int *carpet_elem, *itmp;
    int cnt, vec_cnt, max_cnt;
    int nd, i, j;
    Classed_list *p_cl;

    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;
    vecs_per_area = 1.0 / ( analy->vec_cell_size * analy->vec_cell_size );

    /* Estimate the number of points we're going to generate. */
    area = 0.0;
    for ( tri = analy->isosurf_poly; tri != NULL; tri = tri->next )
        area += area_of_triangle( tri->vtx );
    for ( p_cl = analy->cut_poly_lists; p_cl != NULL; NEXT( p_cl ) )
        for ( tri = (Triangle_poly *) p_cl->list; tri != NULL; NEXT( tri ) )
            area += area_of_triangle( tri->vtx );

    if ( area == 0.0 )
        return;

    max_cnt = (int)(1.3 * area * vecs_per_area);

    for ( i = 0; i < 3; i++ )
        carpet_coords[i] = NEW_N( float, max_cnt, "Iso carpet coords" );
    carpet_elem = NEW_N( int, max_cnt, "Iso carpet elem" );

    /* Loop through triangles to generate the points. */
    cnt = 0;
    for ( tri = analy->isosurf_poly; tri != NULL; tri = tri->next )
    {
        /* Get point count. */
        vec_num = vecs_per_area * area_of_triangle( tri->vtx );
        vec_cnt = (int) vec_num;
        remain = vec_num - vec_cnt;
        if ( drand48() < remain )
            ++vec_cnt;

        if ( cnt + vec_cnt > max_cnt )
        {
            popup_dialog( WARNING_POPUP,
                          "Not enough room in iso carpet array." );
            break;
        }

        /* Get element vertices. */
        for ( i = 0; i < 8; i++ )
        {
            nd = bricks->nodes[i][tri->elem];
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes->xyz[j][nd];
        }

        /* Generate the points and throw them on the list. */
        for ( i = 0; i < vec_cnt; i++ )
        {
            rand_bary_pt( tri->vtx, verts, xi );

            for ( j = 0; j < 3; j++ )
                carpet_coords[j][cnt] = xi[j];
            carpet_elem[cnt] = tri->elem;
            cnt++;
        }
    }
    for ( p_cl = analy->cut_poly_lists; p_cl != NULL; NEXT( p_cl ) )
        for ( tri = (Triangle_poly *) p_cl->list; tri != NULL; NEXT( tri ) )
        {
            /* Get point count. */
            vec_num = vecs_per_area * area_of_triangle( tri->vtx );
            vec_cnt = (int) vec_num;
            remain = vec_num - vec_cnt;
            if ( drand48() < remain )
                ++vec_cnt;

            if ( cnt + vec_cnt > max_cnt )
            {
                popup_dialog( WARNING_POPUP,
                              "Not enough room in iso carpet array." );
                break;
            }

            /* Get element vertices. */
            for ( i = 0; i < 8; i++ )
            {
                nd = bricks->nodes[i][tri->elem];
                for ( j = 0; j < 3; j++ )
                    verts[i][j] = nodes->xyz[j][nd];
            }

            /* Generate the points and throw them on the list. */
            for ( i = 0; i < vec_cnt; i++ )
            {
                rand_bary_pt( tri->vtx, verts, xi );

                for ( j = 0; j < 3; j++ )
                    carpet_coords[j][cnt] = xi[j];
                carpet_elem[cnt] = tri->elem;
                cnt++;
            }
        }

    /* Resize the point arrays. */
    analy->vol_carpet_cnt = cnt;

    for ( i = 0; i < 3; i++ )
    {
        ftmp = NEW_N( float, cnt, "Iso carpet coords" );
        memcpy( ftmp, carpet_coords[i], cnt*sizeof(float) );
        free( carpet_coords[i] );
        analy->vol_carpet_coords[i] = ftmp;
    }

    itmp = NEW_N( int, max_cnt, "Iso carpet elem" );
    memcpy( itmp, carpet_elem, cnt*sizeof(int) );
    free( carpet_elem );
    analy->vol_carpet_elem = itmp;
}


/************************************************************
 * TAG( gen_grid_carpet_points )
 *
 * Generate carpet points for all volume and shell elements
 * in the grid.
 */
static void
gen_grid_carpet_points( Analysis *analy )
{
    Hex_geom *bricks;
    Shell_geom *shells;
    Nodal *nodes;
    float area, vol, csize;
    float xx[8], yy[8], zz[8];
    float qverts[4][3];
    float *carpet_coords[3], *ftmp;
    float vec_num, vec_cnt, remain;
    int *carpet_elem, *itmp;
    int cnt, max_cnt;
    int nd, i, j, k;

    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;
    nodes = analy->state_p->nodes;
    csize = analy->vec_cell_size;

    /* Loop through elements to generate the points. */
    if ( bricks != NULL )
    {
        /* Estimate the number of points we're going to generate. */
        vol = 0.0;
        for ( i = 0; i < bricks->cnt; i++ )
        {
            for ( j = 0; j < 8; j++ )
            {
                nd = bricks->nodes[j][i];
                xx[j] = nodes->xyz[0][nd];
                yy[j] = nodes->xyz[1][nd];
                zz[j] = nodes->xyz[2][nd];
            }
            vol += hex_vol( xx, yy, zz );
        }
        max_cnt = (int)( 1.5 * vol / (csize*csize*csize) ) + 1;
        for ( i = 0; i < 3; i++ )
            carpet_coords[i] = NEW_N( float, max_cnt, "Carpet coords" );
        carpet_elem = NEW_N( int, max_cnt, "Carpet elem" );

        cnt = 0;
        for ( i = 0; i < bricks->cnt; i++ )
        {
            for ( j = 0; j < 8; j++ )
            {
                nd = bricks->nodes[j][i];
                xx[j] = nodes->xyz[0][nd];
                yy[j] = nodes->xyz[1][nd];
                zz[j] = nodes->xyz[2][nd];
            }
            vol = hex_vol( xx, yy, zz );

            /* Get point count. */
            vec_num = vol / (csize*csize*csize);
            vec_cnt = (int) vec_num;
            remain = vec_num - vec_cnt;
            if ( drand48() < remain )
                ++vec_cnt;

            if ( cnt + vec_cnt > max_cnt )
            {
                popup_dialog( WARNING_POPUP,
                              "Not enough room in carpet array." );
                break;
            }

            /* Generate the points and throw them on the list. */
            for ( j = 0; j < vec_cnt; j++ )
            {
                for ( k = 0; k < 3; k++ )
                    carpet_coords[k][cnt] = 2.0 * drand48() - 1.0;
                carpet_elem[cnt] = i;
                cnt++;
            }
        }

        /* Resize the point arrays. */
        analy->vol_carpet_cnt = cnt;

        for ( i = 0; i < 3; i++ )
        {
            ftmp = NEW_N( float, cnt, "Carpet coords" );
            memcpy( ftmp, carpet_coords[i], cnt*sizeof(float) );
            free( carpet_coords[i] );
            analy->vol_carpet_coords[i] = ftmp;
        }
        itmp = NEW_N( int, max_cnt, "Carpet elem" );
        memcpy( itmp, carpet_elem, cnt*sizeof(int) );
        free( carpet_elem );
        analy->vol_carpet_elem = itmp;
    }

    if ( shells != NULL )
    {
        /* Estimate the number of points we're going to generate. */
        area = 0.0;
        for ( i = 0; i < shells->cnt; i++ )
        {
            for ( j = 0; j < 4; j++ )
            {
                nd = shells->nodes[j][i];
                for ( k = 0; k < 3; k++ )
                    qverts[j][k] = nodes->xyz[k][nd];
            }
            area += area_of_quad( qverts );
        }
        max_cnt = (int)( 1.3 * area / (csize*csize) ) + 1;
        for ( i = 0; i < 2; i++ )
            carpet_coords[i] = NEW_N( float, max_cnt, "Carpet coords" );
        carpet_elem = NEW_N( int, max_cnt, "Carpet elem" );

        cnt = 0;
        for ( i = 0; i < shells->cnt; i++ )
        {
            for ( j = 0; j < 4; j++ )
            {
                nd = shells->nodes[j][i];
                for ( k = 0; k < 3; k++ )
                    qverts[j][k] = nodes->xyz[k][nd];
            }
            area = area_of_quad( qverts );

            /* Get point count. */
            vec_num = area / (csize*csize);
            vec_cnt = (int) vec_num;
            remain = vec_num - vec_cnt;
            if ( drand48() < remain )
                ++vec_cnt;

            if ( cnt + vec_cnt > max_cnt )
            {
                popup_dialog( WARNING_POPUP,
                              "Not enough room in carpet array." );
                break;
            }

            /* Generate the points and throw them on the list. */
            for ( j = 0; j < vec_cnt; j++ )
            {
                for ( k = 0; k < 2; k++ )
                    carpet_coords[k][cnt] = 2.0 * drand48() - 1.0;
                carpet_elem[cnt] = i;
                cnt++;
            }
        }

        /* Resize the point arrays. */
        analy->shell_carpet_cnt = cnt;

        for ( i = 0; i < 2; i++ )
        {
            ftmp = NEW_N( float, cnt, "Carpet coords" );
            memcpy( ftmp, carpet_coords[i], cnt*sizeof(float) );
            free( carpet_coords[i] );
            analy->shell_carpet_coords[i] = ftmp;
        }
        itmp = NEW_N( int, max_cnt, "Carpet elem" );
        memcpy( itmp, carpet_elem, cnt*sizeof(int) );
        free( carpet_elem );
        analy->shell_carpet_elem = itmp;
    }
}


/************************************************************
 * TAG( sort_carpet_points )
 *
 * Sort the carpet points in order from nearest to the viewer
 * to farthest from the viewer.
 */
void
sort_carpet_points( Analysis *analy, int cnt, float **carpet_coords,
                    int *carpet_elem, int *index, Bool_type is_hex )
{
    Hex_geom *bricks;
    Shell_geom *shells;
    Nodal *nodes;
    Transf_mat view_mat;
    float *dist;
    float eyept[3], vecpt[3], diff[3];
    float r, s, t, h[8];
    int el, i, j, k;

    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;
    nodes = analy->state_p->nodes;

    /* Get the inverse viewing transformation matrix. */
    inv_view_transf_mat( &view_mat );

    /* Inverse transform the eyepoint into world coordinates. */
    point_transform( eyept, v_win->look_from, &view_mat );

    /* Calculate the square of the distance from the eye for all vec pts. */
    dist = NEW_N( float, cnt, "Carpet sort tmp" );

    if ( is_hex )
    {
        /* Points were generated in hex elements. */
        for ( i = 0; i < cnt; i++ )
        {
            /* Get the natural coordinates of the vector point. */
            r = carpet_coords[0][i];
            s = carpet_coords[1][i];
            t = carpet_coords[2][i];
            shape_fns_hex( r, s, t, h );

            /* Get the physical coordinates of the vector point. */
            el = carpet_elem[i];
            VEC_SET( vecpt, 0.0, 0.0, 0.0 );
            for ( j = 0; j < 8; j++ )
                for ( k = 0; k < 3; k++ )
                    vecpt[k] += h[j]*nodes->xyz[k][ bricks->nodes[j][el] ];

            /* Calculate square of distance. */
            VEC_SUB( diff, vecpt, eyept );
            dist[i] = VEC_DOT( diff, diff );
        }
    }
    else
    {
        /* Points were generated in shell elements. */
        for ( i = 0; i < cnt; i++ )
        {
            /* Get the natural coordinates of the vector point. */
            r = carpet_coords[0][i];
            s = carpet_coords[1][i];
            shape_fns_quad( r, s, h );

            /* Get the physical coordinates of the vector point. */
            el = carpet_elem[i];
            VEC_SET( vecpt, 0.0, 0.0, 0.0 );
            for ( j = 0; j < 4; j++ )
                for ( k = 0; k < 3; k++ )
                    vecpt[k] += h[j]*nodes->xyz[k][ shells->nodes[j][el] ];

            /* Calculate square of distance. */
            VEC_SUB( diff, vecpt, eyept );
            dist[i] = VEC_DOT( diff, diff );
        }
    }

    /* Sort distances from smallest to largest. */
    real_heapsort( cnt, dist, index );
}


/*************************************************************
 * TAG( real_heapsort )
 *
 * Heap sort, from Numerical_Recipes_in_C.  The array of floats
 * in arrin is sorted in ascending order and the sorted ordering
 * is returned in the indx array.
 */
static void
real_heapsort( int n, float *arrin, int *indx )
{
    int l, j, ir, indxt, i;

    for ( j = 0; j < n; j++ )
        indx[j] = j;

    l = n/2;
    ir = n - 1;

    for (;;)
    {
        if ( l > 0 )
        {
            --l;
            indxt = indx[l];
        }
        else
        {
            indxt = indx[ir];
            indx[ir] = indx[0];
            --ir;
            if ( ir == 0 )
            {
                indx[0] = indxt;
                return;
            }
        }

        i = l;
        j = 2*l + 1;
        while ( j <= ir )
        {
            if ( j < ir && arrin[indx[j]] < arrin[indx[j+1]] )
                j++;
            if ( arrin[indxt] < arrin[indx[j]] )
            {
                indx[i] = indx[j];
                i = j;
                j = 2*j + 1;
            }
            else
                j = ir + 1;
        }
        indx[i] = indxt;
    }
}


/************************************************************
 * TAG( gen_reg_carpet_points )
 *
 * Generate carpet points for jittered regular grid.
 */
void
gen_reg_carpet_points( Analysis *analy )
{
    Hex_geom *bricks;
    Nodal *nodes;
    Bool_type found;
    float verts[8][3], pt[3], xi[3];
    float *lo[3], *hi[3];
    float cell_size, val;
    float *reg_carpet_coords[3];
    int *reg_carpet_elem;
    int *reg_dim;
    int ii, jj, kk, el, nd, idx, cnt;
    int i, j, k;

    /* Handle 3D grids only for now. */
    if ( analy->geom_p->bricks == NULL )
        return;

    /* Free any existing regular carpet points. */
    if ( analy->reg_dim[0] > 0 )
    {
        for ( i = 0; i < 3; i++ )
            free( analy->reg_carpet_coords[i] );
        free( analy->reg_carpet_elem );
        analy->reg_dim[0] = 0;
    }

    if ( !analy->show_carpet || analy->reg_cell_size == 0.0 )
        return;

    /* Get grid dimensions and allocate space for grid. */
    cell_size = analy->reg_cell_size;
    reg_dim = analy->reg_dim;
    for ( i = 0; i < 3; i++ )
        reg_dim[i] = (int)( (analy->bbox[1][i] - analy->bbox[0][i]) /
                            cell_size );
    cnt = reg_dim[0] * reg_dim[1] * reg_dim[2];

    for ( i = 0; i < 3; i++ )
        reg_carpet_coords[i] = NEW_N( float, cnt, "Reg carpet coords" );
    reg_carpet_elem = NEW_N( int, cnt, "Reg carpet elem" );

    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;

    /* Compute a bounding box for each element in advance. */
    for ( i = 0; i < 3; i++ )
    {
        lo[i] = analy->tmp_result[i];
        hi[i] = analy->tmp_result[i+3];
    }

    for ( i = 0; i < bricks->cnt; i++ )
    {
        nd = bricks->nodes[0][i];
        for ( k = 0; k < 3; k++ )
        {
            lo[k][i] = nodes->xyz[k][nd];
            hi[k][i] = nodes->xyz[k][nd];
        }

        for ( j = 1; j < 8; j++ )
        {
            nd = bricks->nodes[j][i];
            for ( k = 0; k < 3; k++ )
            {
                val = nodes->xyz[k][nd];
                if ( val < lo[k][i] )
                    lo[k][i] = val;
                else if ( val > hi[k][i] )
                    hi[k][i] = val;
            }
        }
    }

    /* Now do the point inclusion tests. */
    for ( ii = 0; ii < reg_dim[0]; ii++ )
        for ( jj = 0; jj < reg_dim[1]; jj++ )
            for ( kk = 0; kk < reg_dim[2]; kk++ )
            {
                idx = ii*reg_dim[1]*reg_dim[2] + jj*reg_dim[2] + kk;

                /* Calculate the point position. */
                pt[0] = analy->bbox[0][0] + cell_size*( ii + drand48() );
                pt[1] = analy->bbox[0][1] + cell_size*( jj + drand48() );
                pt[2] = analy->bbox[0][2] + cell_size*( kk + drand48() );

                found = FALSE;
                for ( i = 0; i < analy->num_blocks; i++ )
                {
                    /* See if point lies in block's bound box. */
                    if ( pt[0] < analy->block_bbox[0][0][i] ||
                            pt[1] < analy->block_bbox[0][1][i] ||
                            pt[2] < analy->block_bbox[0][2][i] ||
                            pt[0] > analy->block_bbox[1][0][i] ||
                            pt[1] > analy->block_bbox[1][1][i] ||
                            pt[2] > analy->block_bbox[1][2][i] )
                        continue;

                    /* Test point against elements in block. */
                    for ( el = analy->block_lo[i];
                            el <= analy->block_hi[i];
                            el++ )
                    {
                        /* Perform approximate inside (bbox) test first. */
                        if ( pt[0] >= lo[0][el] && pt[0] <= hi[0][el] &&
                                pt[1] >= lo[1][el] && pt[1] <= hi[1][el] &&
                                pt[2] >= lo[2][el] && pt[2] <= hi[2][el] )
                        {
                            for ( j = 0; j < 8; j++ )
                            {
                                nd = bricks->nodes[j][el];
                                for ( k = 0; k < 3; k++ )
                                    verts[j][k] = nodes->xyz[k][nd];
                            }

                            if ( pt_in_hex( verts, pt, xi ) )
                            {
                                reg_carpet_elem[idx] = el;
                                for ( k = 0; k < 3; k++ )
                                    reg_carpet_coords[k][idx] = xi[k];
                                found = TRUE;
                                break;
                            }
                        }
                    }

                    if ( found )
                        break;
                }

                /* If the point wasn't in any bricks, mark it as out. */
                if ( !found )
                    reg_carpet_elem[idx] = -1;
            }

    analy->reg_carpet_elem = reg_carpet_elem;
    for ( i = 0; i < 3; i++ )
        analy->reg_carpet_coords[i] = reg_carpet_coords[i];
}

#endif

