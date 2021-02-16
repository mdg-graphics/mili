/* faces.c - Routines for generating and modifying the mesh face tables.
*
*      Donald J. Dovey
*      Lawrence Livermore National Laboratory
*      Dec 27 1991
*
************************************************************************
* Modifications:
*  I. R. Corey - Sept 15, 2004: Added new option "damage_hide" which
*  controls if damaged elements are displayed.
*
*  I. R. Corey - Jan   7, 2005: Added logic to support selecting objects
*                for failed elements when the free_nodes option is enabled.
*                See SRC#: 286
*
*  I. R. Corey - Jan  12, 2006: Added test for no free nodes.
*                See SRC#: 286
*
*  I. R. Corey - Aug  14, 2007: Added call to re-compute idx (pointer
*
*  I. R. Corey - April 23, 2009: Fixed problem with qty not being inc-
*                creased when the first set of edges is largest.
*                See SRC#: 599
*
*  I. R. Corey - Dec 28, 2009: Fixed several problems releated to drawing
*                ML particles.
*                See SRC#648
************************************************************************
*/

#include <stdlib.h>
#include "viewer.h"
#include "mdg.h"
/*#ifndef DUMP_TABLES
#define DUMP_TABLES
#endif*/ 

/* Local routines. */
static void load_hex_adj( MO_class_data *p_mocd, int *node_tbl,
                          int *face_tbl[7], int *p_tbl_sz,
                          int start, int stop );
static void double_table_size( int *tbl_sz, int **face_tbl, int *free_ptr,
                               int row_qty );
static int *check_match( int *face_entry, int *node_tbl, int **face_tbl, int nodes_per_face );
static Bool_type check_degen_face( int *nodes );
static void hex_rough_cut( float *ppt, float *norm, MO_class_data *p_hex_class,
                           GVec3D *nodes, unsigned char *hex_visib );
static void tet_rough_cut( float *ppt, float *norm, MO_class_data *p_tet_class,
                           GVec3D *nodes, unsigned char *tet_visib );
static int test_plane_hex_elem( float *, float *, int, int[][8], GVec3D *); 
static int test_plane_tet_elem( float *, float *, int, int [][4], GVec3D * );
/* static void face_aver_norm(); */
static void tet_face_avg_norm( int, int, MO_class_data *, Analysis *,
                               float * );
static void hex_face_avg_norm( int, int, MO_class_data *, Analysis *,
                               float * );
static void pyramid_face_avg_norm( int, int, MO_class_data *, Analysis *,
                                   float * );
/* static void shell_aver_norm(); */
static void tri_avg_norm( int, MO_class_data *, Analysis *, float * );
static void quad_avg_norm( int, MO_class_data *, Analysis *, float * );
static void surface_avg_norm( int, MO_class_data *, Analysis *, float * );
static int check_for_triangle( int nodes[4] );
/* static void face_normals(); */
static void tet_face_normals( Mesh_data *, Analysis * );
static void hex_face_normals( Mesh_data *, Analysis * );
static void pyramid_face_normals( Mesh_data *, Analysis * );
/* static void shell_normals(); */
static void quad_normals( Mesh_data *, Analysis * );
static void surface_normals( Mesh_data *, Analysis * );
static void tri_normals( Mesh_data *, Analysis * );
static void select_hex( Analysis *analy, Mesh_data *p_mesh,
                        MO_class_data *p_mo_class, float line_pt[3],
                        float line_dir[3], int *p_near );

static void select_pyramid( Analysis *analy, Mesh_data *p_mesh,
                            MO_class_data *p_mo_class, float line_pt[3],
                            float line_dir[3], int *p_near );
void
select_meshless_elem( Analysis *analy, Mesh_data *p_mesh,
                      MO_class_data *p_ml_class, int near_node,
                      int *p_near );
void
select_meshless_node( Analysis *analy, Mesh_data *p_mesh,
                      MO_class_data *p_ml_class, int elem_id,
                      int *p_near );

static void select_tet( Analysis *analy, Mesh_data *p_mesh,
                        MO_class_data *p_mo_class, float line_pt[3],
                        float line_dir[3], int *p_near );
static void select_planar( Analysis *analy, Mesh_data *p_mesh,
                           MO_class_data *p_mo_class, float line_pt[3],
                           float line_dir[3], int *p_near );
static void select_surf_planar( Analysis *analy, Mesh_data *p_mesh,
                                MO_class_data *p_mo_class, float line_pt[3],
                                float line_dir[3], int *p_near );
static void select_linear( Analysis *analy, Mesh_data *p_mesh,
                           MO_class_data *p_mo_class, float line_pt[3],
                           float line_dir[3], int *p_near );

static void select_node( Analysis *analy, Mesh_data *p_mesh, Bool_type ml_node,
                         float line_pt[3],
                         float line_dir[3], int *p_near );

static void select_particle( Analysis *analy, Mesh_data *p_mesh,
                             MO_class_data *p_mo_class, float line_pt[3],
                             float line_dir[3], int *p_near );

static void edge_heapsort( int n, int *arrin[3], int *indx, int *face_el,
                           int *mat, float *mtl_trans[3] );
static int edge_compare( int, int, int *[3], int *, int *, float *[3] );
static Edge_list_obj * get_hex_class_edges( float *[3], MO_class_data *,
        Analysis * );
static Edge_list_obj * get_pyramid_class_edges( float *[3], MO_class_data *,
        Analysis * );
static Edge_list_obj * get_tet_class_edges( float *[3], MO_class_data *,
        Analysis * );
static Edge_list_obj * get_quad_class_edges( float *[3], MO_class_data *,
        Analysis * );
static Edge_list_obj * get_tri_class_edges( float *[3], MO_class_data *,
        Analysis * );
static Edge_list_obj * create_compressed_edge_list( int *[2], int *, int );
static Edge_list_obj * merge_and_free_edge_lists( Edge_list_obj *,
        Edge_list_obj * );
static void find_extents( MO_class_data *, int, float *, Bool_type [],
                          unsigned char *, float [], float [] );

extern float calc_particle_radius( Analysis *analy, float scale_factor_input );

/* Default crease threshold angle is 22 degrees.
 * (This is the angle between the face normal and the _average_ node normal.)
 */
float crease_threshold = 0.927;

/* Default angle for explicit edge detection is 44 degrees. Note that
 * cos(44) = 0.719 which is the explicit_threshold*/
float explicit_threshold = 0.719;

/* Used in load_hex_adj(). */
static int free_ptr;


/**/
/* Check if Dyna has anything to say about tet face definition */
/*****************************************************************
 * TAG( tet_fc_nd_nums )
 *
 * Table which gives the nodes corresponding to each face of a
 * tet element.
 */
int tet_fc_nd_nums[4][3] = { {0, 1, 3},
    {1, 2, 3},
    {2, 0, 3},
    {0, 2, 1}
};


/*****************************************************************
 * TAG( tet_edge_node_nums )
 *
 * Table which gives the numbers of the nodes at the ends of
 * each edge on a tetrahedral element.
 */
int tet_edge_node_nums[6][2] = { {0,1}, {0,2}, {0,3}, {1,2}, {1,3}, {2,3} };

/**/
/* Check if Dyna has anything to say about pyramid face definition */
/*****************************************************************
 * TAG( pyramid_fc_nd_nums )
 *
 * Table which gives the nodes corresponding to each face of a
 * pyramid element.
 *
 */
int pyramid_fc_nd_nums[5][4] = { {0, 3, 2, 1},
    {0, 1, 4, 4},
    {3, 0, 4, 4},
    {1, 2, 4, 4},
    {2, 3, 4, 4}
};

/*****************************************************************
 * TAG( pyramid_edge_node_nums )
 *
 * Table which gives the numbers of the nodes at the ends of
 * each edge on a pyramid element.
 */
int pyramid_edge_node_nums[8][2] = { {0,1}, {0,3}, {1,2}, {2,3},
    {0,4}, {1,4}, {2,4}, {3,4}
};

/*****************************************************************
 * TAG( fc_nd_nums )
 *
 * Table which gives the nodes corresponding to each face of a
 * brick element.  The faces are numbered as follows.
 *
 *     +X  1
 *     +Y  2
 *     -X  3
 *     -Y  4
 *     +Z  5
 *     -Z  6
 *
 * The nodes are numbered as specified in the DYNA3D manual.
 *
 *     (i,j,k)  node#           (i,j,k)  node#
 *    -------------------------------------------
 *       000      1               001      5
 *       100      2               101      6
 *       110      3               111      7
 *       010      4               011      8
 *
 */
int fc_nd_nums[6][4] = { {1, 2, 6, 5},
    {2, 3, 7, 6},
    {0, 4, 7, 3},
    {1, 5, 4, 0},
    {4, 5, 6, 7},
    {0, 3, 2, 1}
};


/*****************************************************************
 * TAG( edge_face_nums )
 *
 * Table which gives the numbers of the faces which are adjacent
 * to each edge on a hexahedral element.
 */
int edge_face_nums[12][2] = { {2,3}, {0,3}, {0,1}, {1,2}, {3,5}, {1,5},
    {1,4}, {3,4}, {2,5}, {2,4}, {4,0}, {0,5}
};


/*****************************************************************
 * TAG( edge_node_nums )
 *
 * Table which gives the numbers of the nodes at the ends of
 * each edge on a hexahedral element.
 */
int edge_node_nums[12][2] = { {0,4}, {1,5}, {2,6}, {3,7}, {0,1}, {3,2},
    {7,6}, {4,5}, {0,3}, {4,7}, {5,6}, {1,2}
};


#ifdef DUMP_TABLES
static FILE *dump_file = NULL;
static int node_tbl_size = 0;


/************************************************************
 * TAG( start_table_dump )
 *
 * Prepare for dumping tables.
 */
static void
start_table_dump( int node_qty )
{
    node_tbl_size = node_qty;
    dump_file = fopen( "table_f", "w" );
}


/************************************************************
 * TAG( stop_table_dump )
 *
 *
 */
static void
stop_table_dump()
{
    fclose( dump_file );
}


/************************************************************
 * TAG( dump_face_table )
 *
 * Dump the face table to a file.
 */
static void
dump_face_table( int *face_tbl[7], int size )
{
    int i, j;

    for ( i = 0; i < size; i++ )
    {
        fprintf( dump_file, "%4d: ", i );
        for ( j = 0; j < 7; j++ )
            fprintf( dump_file, "  %5d", face_tbl[j][i] );
        fprintf( dump_file, "\n" );
    }
}


/************************************************************
 * TAG( dump_node_table )
 *
 * Dump the node table to a file.
 */
#define NODES_PER_ROW 8
static void
dump_node_table( int *node_tbl, int size )
{
    int i, j, idx, rows;

    rows = size / NODES_PER_ROW;
    if ( size % NODES_PER_ROW )
        rows++;

    for ( i = 0; i < rows; i++ )
    {
        for ( j = 0; j < NODES_PER_ROW; j++ )
        {
            idx = j * rows + i;
            if ( idx < size )
                fprintf( dump_file, "%5d:%-6d ", idx, node_tbl[idx] );
            else
                break;
        }
        fprintf( dump_file, "\n" );
    }
}


/************************************************************
 * TAG( dump_hex_adj_table )
 *
 * Dump the hex element adjacency table to a file.
 */
static void
dump_hex_adj_table( int *hex_adj[6], int size )
{
    int i, j;

    for ( i = 0; i < size; i++ )
    {
        fprintf( dump_file, "%4d: ", i );
        for ( j = 0; j < 6; j++ )
            fprintf( dump_file, "  %5d", hex_adj[j][i] );
        fprintf( dump_file, "\n" );
    }
}


/************************************************************
 * TAG( dump_tables )
 *
 * Dump the hex element adjacency table to a file.
 */
static void
dump_tables( int elem, int face,
             int *node_tbl, int node_size,
             int *face_tbl[7], int face_tbl_size,
             int *hex_adj[6], int hex_size )
{
    if ( elem == -1 )
        fprintf( dump_file, "initial values\n\n" );
    else
        fprintf( dump_file, "elem %d    face %d\n\n", elem, face );

    dump_node_table( node_tbl, node_size );
    fprintf( dump_file, "\n" );

    dump_face_table( face_tbl, face_tbl_size );
    fprintf( dump_file, "\n" );

    dump_hex_adj_table( hex_adj, hex_size );
    fprintf( dump_file, "\n" );
    fflush(dump_file);
}
#endif


/************************************************************
 * TAG( update_hex_adj )
 *
 * Re-load adjacency tables for hex classes.
 */
void
update_hex_adj( Analysis *analy )
{
    int i, j;
    Mesh_data *p_mesh;
    int class_qty;
    MO_class_data **p_hex_classes;

    for ( i = 0; i < analy->mesh_qty; i++ )
    {
        p_mesh = analy->mesh_table + i;

        class_qty = p_mesh->classes_by_sclass[G_HEX].qty;
        p_hex_classes = (MO_class_data **)
                        p_mesh->classes_by_sclass[G_HEX].list;

        for ( j = 0; j < class_qty; j++ )
            create_hex_adj( p_hex_classes[j], analy );
    }
}


/************************************************************
 * TAG( create_hex_adj )
 *
 * Create element adjacency table for a hex class.  Dependent
 * on "hex_overlap" boolean value (which for best performance
 * and most meshes should be FALSE), coincident elements of
 * different materials may have their adjacency configured
 * such that each material instance is allowed to provide
 * external faces independently.  The performance hit in
 * permitting this is that each coincident face is rendered,
 * meaning all but the last rendering of each face is
 * wasted effort.
 */
void
create_hex_adj( MO_class_data *p_mocd, Analysis *analy )
{
    int **hex_adj;
    int *node_tbl;
    int *face_tbl[7];
    int hcnt, ncnt, tbl_sz;
    int i, j, k;
    Mesh_data *p_mesh;
    MO_class_data *p_mat_class;
    Material_data *p_mats;
    int qty_mats;
    Int_2tuple *blocks;

    /*
     * The table face_tbl contains for each face entry
     *
     *     node1 node2 node3 node4 element face next
     *
     * The table hex_adj contains for each hex element
     *
     *     neighbor_elem1 n_elem2 n_elem3 n_elem4 n_elem5 n_elem6
     *
     * If a hex element has no neighbor adjacent to one of its faces,
     * that face neighbor is marked with a -1 in the hex_adj table.
     *
     * For algorithm see:
     *
     *      "Visual3 - A Software Environment for Flow Visualization,"
     *      Computer Graphics and Flow Visualization in Computational
     *      Fluid Dynamics, VKI Lecture Series #10, Brussels, Sept 16-20
     *      1991.
     *
     * "The face table to be constructed has seven entries for each face.  The first four
     * entries are the indices of the four nodes that define the face.  The elements supported
     * have either quadrilateral or triangular faces.  The fifth contains the element number. 
     * The sixth entry is the face number and the seventh entry is a pointer to the next face
     * in the list which contains the same minimum
     * node index (a -1 indicates the end of the list).  This produces a 'threaded list' based
     * on the lowest numbered node of each face.  This list can be quickly processed and a face
     * can be found without extensive searching.  In addition to the face table there is a
     * node table with one entry per node which points to the first face which involved that node.
     *
     * When the initialization process begins, the node table is all -1 and the face table is all zeros.  They
     * are filled up progressively by processing all of the elements in the domain.  Each element 
     * has four to six faces that have pointers to the three or four nodes which define the faces.
     * For each face the process is as follows:
     *
     * Let n1, n2, n3, and optionally n4 be the indices of the nodes that define the new face.
     * Nmin is the minimum of the node numbers. The node table is used to see if there is already
     * a face in the face table which involves node Nmin.  If not, then this new face is added
     * to the face table by setting the first four entries equal to n1, n2, n3, and n4 and the
     * fifth to the element index and the sixth to the face number.  The node table entry for Nmin is
     * adjusted to point to the new face.
     *
     * If there is an entry in the node table, then all the nodes of that entry in the table are
     * compared to n1, n2, n3, and n4.  If these match, then the table face is the same as the new
     * face. The appropriate entries in the adjacency tables for the current face/element and the
     * face/element stored in the table entry are set to point to each other.  The sixth entry of
     * the face table is also set to indicate that the face is complete.
     *
     * If the table face does not match then the whole process is repeated with the next face in
     * the table which uses the node Nmin, by using the pointer in entry seven. This continues until
     * one either finds a match for the new face, or runs out of faces which use the node Nmin. In
     * the latter case the new face does not currently exist in the face table and so it is added.
     * Also, the seventh entry of the face that used to be the last involving node Nmin is set to
     * point to the new face in the table which is now the last entry.
     *
     * When all the elements have been processed, those entries in the  
     */

    hex_adj = p_mocd->p_vis_data->adjacency;
    hcnt = p_mocd->qty;
    ncnt = analy->mesh_table[p_mocd->mesh_id].node_geom->qty;

    /* Arbitrarily set the table size. */
    if ( p_mocd->p_elem_block->num_blocks > 1 )
        tbl_sz = 6 * hcnt / 10;
    else
        tbl_sz = 6 * hcnt;

    /*
     * Initialize the adjacency table so all neighbors are NULL.
     */
    for ( i = 0; i < hcnt; i++ )
        for ( j = 0; j < 6; j++ )
            hex_adj[j][i] = -1;

    /* Allocate the tables. */
    node_tbl = NEW_N( int, ncnt, "Node table" );
    for ( i = 0; i < 7; i++ )
        face_tbl[i] = NEW_N( int, tbl_sz, "Face table" );

#ifdef DUMP_TABLES 
    start_table_dump( ncnt );
#endif 

    /* Set this outside of load_hex_adj. */
    free_ptr = 0;

    if ( analy->hex_overlap )
    {
        /*
         * Process elements by material, skipping materials for which the
         * elements don't belong to the current hex class.
         */

        /* Get material data. */
        p_mesh = analy->mesh_table + p_mocd->mesh_id;
        p_mat_class = ((MO_class_data **)
                       (p_mesh->classes_by_sclass[G_MAT].list))[0];
        qty_mats = p_mat_class->qty;
        p_mats = p_mat_class->objects.materials;

        for ( k = 0; k < qty_mats; k++ )
        {
            if ( p_mats[k].elem_class != p_mocd )
                continue;

            /* Initialize the tables. */
            for ( i = 0; i < ncnt; i++ )
                node_tbl[i] = -1;
            for ( i = 0; i < tbl_sz; i++ )
                face_tbl[6][i] = i + 1;
            face_tbl[6][tbl_sz-1] = -1;

            /* Get the element blocks for the current material. */
            blocks = p_mats[k].elem_blocks;

            /* Loop over element blocks. */
            for ( i = 0; i < p_mats[k].elem_block_qty; i++ )
            {
                /* Do the work... */
                load_hex_adj( p_mocd, node_tbl, face_tbl, &tbl_sz, blocks[i][0],
                              blocks[i][1] );
            }

        }
    }
    else
    {
        /* Normal approach - don't distinguish materials. */

        /* Initialize the tables. */
        for ( i = 0; i < ncnt; i++ )
            node_tbl[i] = -1;
        for ( i = 0; i < tbl_sz; i++ )
            face_tbl[6][i] = i + 1;
        face_tbl[6][tbl_sz-1] = -1;

        /* Do the work... */
        load_hex_adj( p_mocd, node_tbl, face_tbl, &tbl_sz, 0, hcnt - 1 );
    }

#ifdef DUMP_TABLES
    stop_table_dump();
#endif

    /* Free the tables. */
    free( node_tbl );
    for ( i = 0; i < 7; i++ )
        free( face_tbl[i] );
}


/************************************************************
 * TAG( load_hex_adj )
 *
 * Compute volume element adjacency information and create
 * the element adjacency table.  The routine uses a bucket
 * sort with approximately linear complexity.
 */
static void
load_hex_adj( MO_class_data *p_mocd, int *node_tbl, int *face_tbl[7],
              int *p_tbl_sz, int start, int stop )
{
    int (*connects)[8];
    Bool_type valid;
    int **hex_adj;
    int face_entry[6];
    int tmp_faces[6];
    int *idx, new_idx, tmp;
    int el, fc, elem1, face1, elem2, face2;
    int i, j;
    FILE *fp;

    /*
     * The table face_tbl contains for each face entry
     *
     *     node1 node2 node3 node4 element face next
     *
     * The table hex_adj contains for each hex element
     *
     *     neighbor_elem1 n_elem2 n_elem3 n_elem4 n_elem5 n_elem6
     *
     * If a hex element has no neighbor adjacent to one of its faces,
     * that face neighbor is marked with a -1 in the hex_adj table.
     *
     * For algorithm see:
     *
     *      "Visual3 - A Software Environment for Flow Visualization,"
     *      Computer Graphics and Flow Visualization in Computational
     *      Fluid Dynamics, VKI Lecture Series #10, Brussels, Sept 16-20
     *      1991.
     * "The face table to be constructed has seven entries for each face.  The first four
     * entries are the indices of the four nodes that define the face.  The elements supported
     * have either quadrilateral or triangular faces.  The fifth contains the element number. 
     * The sixth entry is the face number and the seventh entry is a pointer to the next face
     * in the list which contains the same minimum
     * node index (a -1 indicates the end of the list).  This produces a 'threaded list' based
     * on the lowest numbered node of each face.  This list can be quickly processed and a face
     * can be found without extensive searching.  In addition to the face table there is a
     * node table with one entry per node which points to the first face which involved that node.
     *
     * When the initialization process begins, the node table is all -1 and the face table is all zeros.  They
     * are filled up progressively by processing all of the elements in the domain.  Each element 
     * has four to six faces that have pointers to the three or four nodes which define the faces.
     * For each face the process is as follows:
     *
     * Let n1, n2, n3, and optionally n4 be the indices of the nodes that define the new face.
     * Nmin is the minimum of the node numbers. The node table is used to see if there is already
     * a face in the face table which involves node Nmin.  If not, then this new face is added
     * to the face table by setting the first four entries equal to n1, n2, n3, and n4 and the
     * fifth to the element index and the sixth to the face number.  The node table entry for Nmin is
     * adjusted to point to the new face.
     *
     * If there is an entry in the node table, then all the nodes of that entry in the table are
     * compared to n1, n2, n3, and n4.  If these match, then the table face is the same as the new
     * face. The appropriate entries in the adjacency tables for the current face/element and the
     * face/element stored in the table entry are set to point to each other.  The sixth entry of
     * the face table is also set to indicate that the face is complete.
     *
     * If the table face does not match then the whole process is repeated with the next face in
     * the table which uses the node Nmin, by using the pointer in entry seven. This continues until
     * one either finds a match for the new face, or runs out of faces which use the node Nmin. In
     * the latter case the new face does not currently exist in the face table and so it is added.
     * Also, the seventh entry of the face that used to be the last involving node Nmin is set to
     * point to the new face in the table which is now the last entry.
     *
     * When all the elements have been processed, those entries in the  
     */

    connects = (int (*)[8]) p_mocd->objects.elems->nodes;
    hex_adj = p_mocd->p_vis_data->adjacency;

#ifdef DUMP_TABLES
    dump_tables( -1, -1,
                 node_tbl, node_tbl_size,
                 face_tbl, *p_tbl_sz,
                 hex_adj, p_mocd->qty );
    fp = fopen("face_entry", "w");
    fprintf(fp, "nd1\tnd2\tnd3\tnd4\tel\tfc\n");
#endif

    /* Loop over elements. */
    for ( el = start; el <= stop; el++ )
    {
        for ( fc = 0; fc < 6; fc++ )
        {

            /* Get nodes for a face. */
            for ( i = 0; i < 4; i++ )
                face_entry[i] = connects[el][ fc_nd_nums[fc][i] ];
            face_entry[4] = el;
            face_entry[5] = fc;

            /* Order the node numbers in ascending order. */
            for ( i = 0; i < 3; i++ )
                for ( j = i+1; j < 4; j++ )
                    if ( face_entry[i] > face_entry[j] )
                        SWAP( tmp, face_entry[i], face_entry[j] );

            /* Check for degenerate element faces. */
            if ( p_mocd->objects.elems->has_degen )
            {
#ifdef DUMP_TABLES
                for(i = 0; i < 6; i++)
                {
                    tmp_faces[i] = face_entry[i];
                }
                for(i = 1; i < 3; i++)
                {
                    if(tmp_faces[i] == tmp_faces[i-1])
                    {
                        tmp_faces[i] = tmp_faces[i+1];
                    }
                }

                 
                fprintf(fp, "%d\t%d\t%d\t%d\t%d\t%d\n", tmp_faces[0], tmp_faces[1],
                                                        tmp_faces[2], tmp_faces[3],
                                                        tmp_faces[4], tmp_faces[5]);
#endif 
                valid = check_degen_face( face_entry );

                /* If face is degenerate (pt or line), skip it. */
                if ( !valid )
                {
                    if(free_ptr >= 0)
                    {
                        new_idx = free_ptr;
                        free_ptr = face_tbl[6][free_ptr];
                        if(new_idx == free_ptr - 1)
                        { 
                            face_tbl[6][new_idx] = -1;
                        }
                        /*if(node_tbl[face_entry[0]] == -1 && (new_idx == free_ptr - 1))
                        {
                            node_tbl[face_entry[0]] = new_idx;
                        }*/
                    }
                    hex_adj[fc][el] = -2;
                    continue;
                }
            }

            idx = check_match( face_entry, node_tbl, face_tbl, 4 );

            if ( *idx >= 0 )
            {
                /*
                 * The face is repeated, so make the two elements
                 * that share the face point to each other.
                 */
                elem1 = face_entry[4];
                face1 = face_entry[5];
                elem2 = face_tbl[4][ *idx ];
                face2 = face_tbl[5][ *idx ];
                hex_adj[ face1 ][ elem1 ] = elem2;
                hex_adj[ face2 ][ elem2 ] = elem1;

                /* Put the face entry on the free list. */
                tmp = face_tbl[6][*idx];
                face_tbl[6][*idx] = free_ptr;
                free_ptr = *idx;
                *idx = tmp;
            }
            else
            {
                /* Create a new face entry and append to node list. */
                if ( free_ptr < 0 )
                {
                    double_table_size( p_tbl_sz, face_tbl, &free_ptr, 7 );

                    /* IRC - Aug. 14, 2007 - Added call to re-compute address for idx */
                    idx = check_match( face_entry, node_tbl, face_tbl, 4 );
                }

                new_idx = free_ptr;
                free_ptr = face_tbl[6][free_ptr];

                for ( i = 0; i < 6; i++ )
                    face_tbl[i][new_idx] = face_entry[i];
                face_tbl[6][new_idx] = -1;
                *idx = new_idx;
            }

#ifdef DUMP_TABLES
            dump_tables( el, fc,
                         node_tbl, node_tbl_size,
                         face_tbl, *p_tbl_sz,
                         hex_adj, p_mocd->qty );
#endif

        }
    }
#ifdef DUMP_TABLES
    fclose(fp);
#endif

}


/************************************************************
 * TAG( create_tet_adj )
 *
 * Compute tetrahedral element adjacency information and create
 * the element adjacency table.  The routine uses a bucket
 * sort with approximately linear complexity.
 */
void
create_tet_adj( MO_class_data *p_mocd, Analysis *analy )
{
    int (*connects)[4];
    Bool_type has_degen;
    int **tet_adj;
    int *node_tbl;
    int *face_tbl[6];
    int face_entry[5];
    int tcnt, ncnt, tbl_sz, free_ptr;
    int *idx, new_idx, tmp;
    int el, fc, elem1, face1, elem2, face2;
    int i, j;

    /*
     * The table face_tbl contains for each face entry
     *
     *     node1 node2 node3 element face next
     *
     * The table tet_adj contains for each tet element
     *
     *     neighbor_elem1 n_elem2 n_elem3 n_elem4
     *
     * If a tet element has no neighbor adjacent to one of its faces,
     * that face neighbor is marked with a -1 in the tet_adj table.
     *
     * For algorithm see:
     *
     *      "Visual3 - A Software Environment for Flow Visualization,"
     *      Computer Graphics and Flow Visualization in Computational
     *      Fluid Dynamics, VKI Lecture Series #10, Brussels, Sept 16-20
     *      1991.
     */

    connects = (int (*)[4]) p_mocd->objects.elems->nodes;
    tet_adj = p_mocd->p_vis_data->adjacency;
    tcnt = p_mocd->qty;
    ncnt = analy->mesh_table[p_mocd->mesh_id].node_geom->qty;
    has_degen = p_mocd->objects.elems->has_degen;

    /* Arbitrarily set the table size. */
    if ( p_mocd->p_elem_block->num_blocks > 1 )
        tbl_sz = 4 * tcnt / 10;
    else
        tbl_sz = 4 * tcnt;

    /* Allocate the tables and initialize table pointers. */
    node_tbl = NEW_N( int, ncnt, "Node table" );
    for ( i = 0; i < ncnt; i++ )
        node_tbl[i] = -1;

    for ( i = 0; i < 6; i++ )
        face_tbl[i] = NEW_N( int, tbl_sz, "Face table" );
    for ( i = 0; i < tbl_sz; i++ )
        face_tbl[5][i] = i + 1;
    face_tbl[5][tbl_sz-1] = -1;
    free_ptr = 0;

    /*
     * Initialize the adjacency table so all neighbors are NULL.
     */
    for ( i = 0; i < tcnt; i++ )
        for ( j = 0; j < 4; j++ )
            tet_adj[j][i] = -1;

    /*
     * Table generation loop.
     */
    for ( el = 0; el < tcnt; el++ )
        for ( fc = 0; fc < 4; fc++ )
        {
            /* Get nodes for a face. */
            for ( i = 0; i < 3; i++ )
                face_entry[i] = connects[el][ tet_fc_nd_nums[fc][i] ];
            face_entry[3] = el;
            face_entry[4] = fc;

            /* Order the node numbers in ascending order. */
            for ( i = 0; i < 2; i++ )
                for ( j = i+1; j < 3; j++ )
                    if ( face_entry[i] > face_entry[j] )
                        SWAP( tmp, face_entry[i], face_entry[j] );

            /* Check for degenerate element faces. */
            if ( has_degen )
            {
                /* If face is degenerate (pt or line), skip it. */
                if ( face_entry[0] == face_entry[1]
                        || face_entry[1] == face_entry[2] )
                    continue;
            }

            /* Search chain for first node and seek a face match. */
            idx = &node_tbl[face_entry[0]];
            while ( *idx >= 0 )
            {
                if ( face_entry[0] == face_tbl[0][*idx]
                        && face_entry[1] == face_tbl[1][*idx]
                        && face_entry[2] == face_tbl[2][*idx] )
                {
                    break;
                }
                else
                {
                    idx = &face_tbl[5][*idx];
                }
            }

            if ( *idx >= 0 )
            {
                /*
                 * The face is repeated, so make the two elements
                 * that share the face point to each other.
                 */
                elem1 = face_entry[3];
                face1 = face_entry[4];
                elem2 = face_tbl[3][ *idx ];
                face2 = face_tbl[4][ *idx ];
                tet_adj[ face1 ][ elem1 ] = elem2;
                tet_adj[ face2 ][ elem2 ] = elem1;

                /* Put the face entry on the free list. */
                tmp = face_tbl[5][*idx];
                face_tbl[5][*idx] = free_ptr;
                free_ptr = *idx;
                *idx = tmp;
            }
            else
            {
                /* Create a new face entry and append it to node list. */
                if ( free_ptr < 0 )
                {
                    double_table_size( &tbl_sz, face_tbl, &free_ptr, 6 );
                    idx = check_match( face_entry, node_tbl, face_tbl, 3 );
                }
                new_idx = free_ptr;
                free_ptr = face_tbl[5][free_ptr];

                for ( i = 0; i < 5; i++ )
                    face_tbl[i][new_idx] = face_entry[i];
                face_tbl[5][new_idx] = -1;
                *idx = new_idx;
            }
        }

    /* Free the tables. */
    free( node_tbl );
    for ( i = 0; i < 6; i++ )
        free( face_tbl[i] );
}


/************************************************************
 * TAG( create_quad_adj )
 *
 * Compute quadrilateral element adjacency information and create
 * the element adjacency table.  The routine uses a bucket
 * sort with approximately linear complexity.
 */
void
create_quad_adj( MO_class_data *p_mocd, Analysis *analy )
{
    int (*connects)[4];
    Bool_type has_degen;
    int **quad_adj;
    int *node_tbl;
    int *edge_tbl[5];
    int edge_entry[4];
    int qcnt, ncnt, tbl_sz, free_ptr;
    int *idx, new_idx, tmp;
    int el, edg, elem1, edge1, elem2, edge2;
    int i, j, k;
    static int quad_edg_nd_nums[4][2] =
    {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }
    };

    /*
     * The table edge_tbl contains for each edge entry
     *
     *     node1 node2 element edge next
     *
     * The table quad_adj contains for each quad element
     *
     *     neighbor_elem1 n_elem2 n_elem3 n_elem4
     *
     * If a quad element has no neighbor adjacent to one of its edges,
     * that edge neighbor is marked with a -1 in the quad_adj table.
     *
     * For algorithm see:
     *
     *      "Visual3 - A Software Environment for Flow Visualization,"
     *      Computer Graphics and Flow Visualization in Computational
     *      Fluid Dynamics, VKI Lecture Series #10, Brussels, Sept 16-20
     *      1991.
     */

    if ( p_mocd->p_vis_data->adjacency != NULL )
        return;

    quad_adj = NEW_N( int *, 4, "Quad adjacency array" );
    for ( k = 0; k < 4; k++ )
        quad_adj[k] = NEW_N( int, p_mocd->qty, "Quad adjacency" );
    p_mocd->p_vis_data->adjacency = quad_adj;
    connects = (int (*)[4]) p_mocd->objects.elems->nodes;
    qcnt = p_mocd->qty;
    ncnt = analy->mesh_table[p_mocd->mesh_id].node_geom->qty;
    has_degen = p_mocd->objects.elems->has_degen;

    /* Arbitrarily set the table size. */
    if ( p_mocd->p_elem_block->num_blocks > 1 )
        tbl_sz = 4 * qcnt / 10;
    else
        tbl_sz = 4 * qcnt;

    /* Allocate the tables and initialize table pointers. */
    node_tbl = NEW_N( int, ncnt, "Node table" );
    for ( i = 0; i < ncnt; i++ )
        node_tbl[i] = -1;

    for ( i = 0; i < 5; i++ )
        edge_tbl[i] = NEW_N( int, tbl_sz, "Edge table" );
    for ( i = 0; i < tbl_sz; i++ )
        edge_tbl[4][i] = i + 1;
    edge_tbl[4][tbl_sz-1] = -1;
    free_ptr = 0;

    /*
     * Initialize the adjacency table so all neighbors are NULL.
     */
    for ( i = 0; i < qcnt; i++ )
        for ( j = 0; j < 4; j++ )
            quad_adj[j][i] = -1;

    /*
     * Table generation loop.
     */
    for ( el = 0; el < qcnt; el++ )
        for ( edg = 0; edg < 4; edg++ )
        {
            /* Get nodes for a edge. */
            edge_entry[0] = connects[el][ quad_edg_nd_nums[edg][0] ];
            edge_entry[1] = connects[el][ quad_edg_nd_nums[edg][1] ];
            edge_entry[2] = el;
            edge_entry[3] = edg;

            /* Order the node numbers in ascending order. */
            if ( edge_entry[0] > edge_entry[1] )
                SWAP( tmp, edge_entry[0], edge_entry[1] );

            /* Check for degenerate element edges. */
            if ( has_degen )
            {
                /* If edge is degenerate (pt), skip it. */
                if ( edge_entry[0] == edge_entry[1] )
                    continue;
            }

            /* Search chain for first node and seek a edge match. */
            idx = &node_tbl[edge_entry[0]];
            while ( *idx >= 0 )
            {
                if ( edge_entry[0] != edge_tbl[0][*idx]
                        || edge_entry[1] != edge_tbl[1][*idx] )
                    idx = &edge_tbl[4][*idx];
            }

            if ( *idx >= 0 )
            {
                /*
                 * The edge is repeated, so make the two elements
                 * that share the edge point to each other.
                 */
                elem1 = edge_entry[2];
                edge1 = edge_entry[3];
                elem2 = edge_tbl[2][ *idx ];
                edge2 = edge_tbl[3][ *idx ];
                quad_adj[ edge1 ][ elem1 ] = elem2;
                quad_adj[ edge2 ][ elem2 ] = elem1;

                /* Put the edge entry on the free list. */
                tmp = edge_tbl[4][*idx];
                edge_tbl[4][*idx] = free_ptr;
                free_ptr = *idx;
                *idx = tmp;
            }
            else
            {
                /* Create a new edge entry and append it to node list. */
                if ( free_ptr < 0 )
                    double_table_size( &tbl_sz, edge_tbl, &free_ptr, 5 );
                new_idx = free_ptr;
                free_ptr = edge_tbl[4][free_ptr];

                for ( i = 0; i < 4; i++ )
                    edge_tbl[i][new_idx] = edge_entry[i];
                edge_tbl[4][new_idx] = -1;
                *idx = new_idx;
            }
        }

    /* Free the tables. */
    free( node_tbl );
    for ( i = 0; i < 5; i++ )
        free( edge_tbl[i] );
}



/************************************************************
 * TAG( create_surf_adj )
 *
 * Compute quadrilateral element adjacency information and create
 * the element adjacency table.  The routine uses a bucket
 * sort with approximately linear complexity.
 */
void
create_surf_adj( MO_class_data *p_mocd, Analysis *analy )
{
    int (*connects)[4];
    Bool_type has_degen;
    int **surf_adj;
    int *node_tbl;
    int *edge_tbl[5];
    int edge_entry[4];
    int qcnt, ncnt, tbl_sz, free_ptr;
    int *idx, new_idx, tmp;
    int el, edg, elem1, edge1, elem2, edge2;
    int i, j, k;
    static int surf_edg_nd_nums[4][2] =
    {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }
    };

    /*
     * The table edge_tbl contains for each edge entry
     *
     *     node1 node2 element edge next
     *
     * The table surf_adj contains for each surf element
     *
     *     neighbor_elem1 n_elem2 n_elem3 n_elem4
     *
     * If a surf element has no neighbor adjacent to one of its edges,
     * that edge neighbor is marked with a -1 in the surf_adj table.
     *
     * For algorithm see:
     *
     *      "Visual3 - A Software Environment for Flow Visualization,"
     *      Computer Graphics and Flow Visualization in Computational
     *      Fluid Dynamics, VKI Lecture Series #10, Brussels, Sept 16-20
     *      1991.
     */

    if ( p_mocd->p_vis_data->adjacency != NULL )
        return;

    surf_adj = NEW_N( int *, 4, "surf adjacency array" );
    for ( k = 0; k < 4; k++ )
        surf_adj[k] = NEW_N( int, p_mocd->qty, "Surf adjacency" );
    p_mocd->p_vis_data->adjacency = surf_adj;
    connects = (int (*)[4]) p_mocd->objects.elems->nodes;
    qcnt = p_mocd->qty;
    ncnt = analy->mesh_table[p_mocd->mesh_id].node_geom->qty;
    has_degen = p_mocd->objects.elems->has_degen;

    /* Arbitrarily set the table size. */
    if ( p_mocd->p_elem_block->num_blocks > 1 )
        tbl_sz = 4 * qcnt / 10;
    else
        tbl_sz = 4 * qcnt;

    /* Allocate the tables and initialize table pointers. */
    node_tbl = NEW_N( int, ncnt, "Node table" );
    for ( i = 0; i < ncnt; i++ )
        node_tbl[i] = -1;

    for ( i = 0; i < 5; i++ )
        edge_tbl[i] = NEW_N( int, tbl_sz, "Edge table" );
    for ( i = 0; i < tbl_sz; i++ )
        edge_tbl[4][i] = i + 1;
    edge_tbl[4][tbl_sz-1] = -1;
    free_ptr = 0;

    /*
     * Initialize the adjacency table so all neighbors are NULL.
     */
    for ( i = 0; i < qcnt; i++ )
        for ( j = 0; j < 4; j++ )
            surf_adj[j][i] = -1;

    /*
     * Table generation loop.
     */
    for ( el = 0; el < qcnt; el++ )
        for ( edg = 0; edg < 4; edg++ )
        {
            /* Get nodes for a edge. */
            edge_entry[0] = connects[el][ surf_edg_nd_nums[edg][0] ];
            edge_entry[1] = connects[el][ surf_edg_nd_nums[edg][1] ];
            edge_entry[2] = el;
            edge_entry[3] = edg;

            /* Order the node numbers in ascending order. */
            if ( edge_entry[0] > edge_entry[1] )
                SWAP( tmp, edge_entry[0], edge_entry[1] );

            /* Check for degenerate element edges. */
            if ( has_degen )
            {
                /* If edge is degenerate (pt), skip it. */
                if ( edge_entry[0] == edge_entry[1] )
                    continue;
            }

            /* Search chain for first node and seek a edge match. */
            idx = &node_tbl[edge_entry[0]];
            while ( *idx >= 0 )
            {
                if ( edge_entry[0] != edge_tbl[0][*idx]
                        || edge_entry[1] != edge_tbl[1][*idx] )
                    idx = &edge_tbl[4][*idx];
            }

            if ( *idx >= 0 )
            {
                /*
                 * The edge is repeated, so make the two elements
                 * that share the edge point to each other.
                 */
                elem1 = edge_entry[2];
                edge1 = edge_entry[3];
                elem2 = edge_tbl[2][ *idx ];
                edge2 = edge_tbl[3][ *idx ];
                surf_adj[ edge1 ][ elem1 ] = elem2;
                surf_adj[ edge2 ][ elem2 ] = elem1;

                /* Put the edge entry on the free list. */
                tmp = edge_tbl[4][*idx];
                edge_tbl[4][*idx] = free_ptr;
                free_ptr = *idx;
                *idx = tmp;
            }
            else
            {
                /* Create a new edge entry and append it to node list. */
                if ( free_ptr < 0 )
                    double_table_size( &tbl_sz, edge_tbl, &free_ptr, 5 );
                new_idx = free_ptr;
                free_ptr = edge_tbl[4][free_ptr];

                for ( i = 0; i < 4; i++ )
                    edge_tbl[i][new_idx] = edge_entry[i];
                edge_tbl[4][new_idx] = -1;
                *idx = new_idx;
            }
        }

    /* Free the tables. */
    free( node_tbl );
    for ( i = 0; i < 5; i++ )
        free( edge_tbl[i] );
}


/************************************************************
 * TAG( create_tri_adj )
 *
 * Compute triangle element adjacency information and create
 * the element adjacency table.  The routine uses a bucket
 * sort with approximately linear complexity.
 */
void
create_tri_adj( MO_class_data *p_mocd, Analysis *analy )
{
    int (*connects)[3];
    Bool_type has_degen;
    int **tri_adj;
    int *node_tbl;
    int *edge_tbl[5];
    int edge_entry[4];
    int tcnt, ncnt, tbl_sz, free_ptr;
    int *idx, new_idx, tmp;
    int el, edg, elem1, edge1, elem2, edge2;
    int i, j, k;
    static int tri_edg_nd_nums[3][2] =
    {
        { 0, 1 }, { 1, 2 }, { 2, 0 }
    };

    /*
     * The table edge_tbl contains for each edge entry
     *
     *     node1 node2 element edge next
     *
     * The table tri_adj contains for each tri element
     *
     *     neighbor_elem1 n_elem2 n_elem3
     *
     * If a tri element has no neighbor adjacent to one of its edges,
     * that edge neighbor is marked with a -1 in the tri_adj table.
     *
     * For algorithm see:
     *
     *      "Visual3 - A Software Environment for Flow Visualization,"
     *      Computer Graphics and Flow Visualization in Computational
     *      Fluid Dynamics, VKI Lecture Series #10, Brussels, Sept 16-20
     *      1991.
     */

    if ( p_mocd->p_vis_data->adjacency != NULL )
        return;

    tri_adj = NEW_N( int *, 3, "Tri adjacency array" );
    for ( k = 0; k < 3; k++ )
        tri_adj[k] = NEW_N( int, p_mocd->qty, "Tri adjacency" );
    p_mocd->p_vis_data->adjacency = tri_adj;
    connects = (int (*)[3]) p_mocd->objects.elems->nodes;
    tcnt = p_mocd->qty;
    ncnt = analy->mesh_table[p_mocd->mesh_id].node_geom->qty;
    has_degen = p_mocd->objects.elems->has_degen;

    /* Arbitrarily set the table size. */
    if ( p_mocd->p_elem_block->num_blocks > 1 )
        tbl_sz = 3 * tcnt / 10;
    else
        tbl_sz = 3 * tcnt;

    /* Allocate the tables and initialize table pointers. */
    node_tbl = NEW_N( int, ncnt, "Node table" );
    for ( i = 0; i < ncnt; i++ )
        node_tbl[i] = -1;

    for ( i = 0; i < 5; i++ )
        edge_tbl[i] = NEW_N( int, tbl_sz, "Edge table" );
    for ( i = 0; i < tbl_sz; i++ )
        edge_tbl[4][i] = i + 1;
    edge_tbl[4][tbl_sz-1] = -1;
    free_ptr = 0;

    /*
     * Initialize the adjacency table so all neighbors are NULL.
     */
    for ( i = 0; i < tcnt; i++ )
        for ( j = 0; j < 3; j++ )
            tri_adj[j][i] = -1;

    /*
     * Table generation loop.
     */
    for ( el = 0; el < tcnt; el++ )
        for ( edg = 0; edg < 3; edg++ )
        {
            /* Get nodes for a edge. */
            edge_entry[0] = connects[el][ tri_edg_nd_nums[edg][0] ];
            edge_entry[1] = connects[el][ tri_edg_nd_nums[edg][1] ];
            edge_entry[2] = el;
            edge_entry[3] = edg;

            /* Order the node numbers in ascending order. */
            if ( edge_entry[0] > edge_entry[1] )
                SWAP( tmp, edge_entry[0], edge_entry[1] );

            /* Check for degenerate element edges. */
            if ( has_degen )
            {
                /* If edge is degenerate (pt), skip it. */
                if ( edge_entry[0] == edge_entry[1] )
                    continue;
            }

            /* Search chain for first node and seek a edge match. */
            idx = &node_tbl[edge_entry[0]];
            while ( *idx >= 0 )
            {
                if ( edge_entry[0] != edge_tbl[0][*idx]
                        || edge_entry[1] != edge_tbl[1][*idx] )
                    idx = &edge_tbl[4][*idx];
            }

            if ( *idx >= 0 )
            {
                /*
                 * The edge is repeated, so make the two elements
                 * that share the edge point to each other.
                 */
                elem1 = edge_entry[2];
                edge1 = edge_entry[3];
                elem2 = edge_tbl[2][ *idx ];
                edge2 = edge_tbl[3][ *idx ];
                tri_adj[ edge1 ][ elem1 ] = elem2;
                tri_adj[ edge2 ][ elem2 ] = elem1;

                /* Put the edge entry on the free list. */
                tmp = edge_tbl[4][*idx];
                edge_tbl[4][*idx] = free_ptr;
                free_ptr = *idx;
                *idx = tmp;
            }
            else
            {
                /* Create a new edge entry and append it to node list. */
                if ( free_ptr < 0 )
                    double_table_size( &tbl_sz, edge_tbl, &free_ptr, 5 );
                new_idx = free_ptr;
                free_ptr = edge_tbl[4][free_ptr];

                for ( i = 0; i < 4; i++ )
                    edge_tbl[i][new_idx] = edge_entry[i];
                edge_tbl[4][new_idx] = -1;
                *idx = new_idx;
            }
        }

    /* Free the tables. */
    free( node_tbl );
    for ( i = 0; i < 5; i++ )
        free( edge_tbl[i] );
}



/************************************************************
 * TAG( create_pryamid_adj )
 *
 * Compute pyramidal element adjacency information and create
 * the element adjacency table.  The routine uses a bucket
 * sort with approximately linear complexity.
 */
void
create_pyramid_adj( MO_class_data *p_mocd, Analysis *analy )
{
    int (*connects)[5];
    Bool_type has_degen;
    int **pyramid_adj;
    int *node_tbl;
    int *face_tbl[7];
    int face_entry[6];
    int pcnt, ncnt, tbl_sz, free_ptr;
    int *idx, new_idx, tmp;
    int el, fc, elem1, face1, elem2, face2;
    int i, j;

    /*
     * The table face_tbl contains for each face entry
     *
     *     node1 node2 node3 node4 element face next
     *
     * The table pyramid_adj contains for each pyramid element

     *     neighbor_elem1 n_elem2 n_elem3 n_elem4 n_elem5
     *
     * If a pyramid element has no neighbor adjacent to one of its faces,
     * that face neighbor is marked with a -1 in the pyramid_adj table.
     *
     * For algorithm see:
     *
     *      "Visual3 - A Software Environment for Flow Visualization,"
     *      Computer Graphics and Flow Visualization in Computational
     *      Fluid Dynamics, VKI Lecture Series #10, Brussels, Sept 16-20
     *      1991.
     */

    connects = (int (*)[5]) p_mocd->objects.elems->nodes;
    pyramid_adj = p_mocd->p_vis_data->adjacency;
    pcnt = p_mocd->qty;
    ncnt = analy->mesh_table[p_mocd->mesh_id].node_geom->qty;
    has_degen = p_mocd->objects.elems->has_degen;

    /* Arbitrarily set the table size. */
    if ( p_mocd->p_elem_block->num_blocks > 1 )
        tbl_sz = 5 * pcnt / 10;
    else
        tbl_sz = 5 * pcnt;

    /* Allocate the tables and initialize table pointers. */
    node_tbl = NEW_N( int, ncnt, "Node table" );
    for ( i = 0; i < ncnt; i++ )
        node_tbl[i] = -1;

    for ( i = 0; i < 7; i++ )
        face_tbl[i] = NEW_N( int, tbl_sz, "Face table" );
    for ( i = 0; i < tbl_sz; i++ )
        face_tbl[6][i] = i + 1;
    face_tbl[6][tbl_sz-1] = -1;
    free_ptr = 0;
    /*
     * Initialize the adjacency table so all neighbors are NULL.
     */
    for ( i = 0; i < pcnt; i++ )
        for ( j = 0; j < 5; j++ )
            pyramid_adj[j][i] = -1;

    /*
     * Table generation loop.
     */
    for ( el = 0; el < pcnt; el++ )
        for ( fc = 0; fc < 5; fc++ )
        {
            /* Get nodes for a face. */
            for ( i = 0; i < 4; i++ )
                face_entry[i] = connects[el][ pyramid_fc_nd_nums[fc][i] ];
            face_entry[4] = el;
            face_entry[5] = fc;

            /* Order the node numbers in ascending order. */
            for ( i = 0; i < 3; i++ )
                for ( j = i+1; j < 4; j++ )
                    if ( face_entry[i] > face_entry[j] )
                        SWAP( tmp, face_entry[i], face_entry[j] );

            /* Check for degenerate element faces. */
            if ( has_degen )
            {
                /* If face is degenerate (pt or line), skip it. */
                if ( face_entry[0] == face_entry[1]
                        || face_entry[1] == face_entry[2] )
                    continue;
            }

            /* Search chain for first node and seek a face match. */
            idx = &node_tbl[face_entry[0]];
            while ( *idx >= 0 )
            {
                if ( face_entry[0] == face_tbl[0][*idx]
                        && face_entry[1] == face_tbl[1][*idx]
                        && face_entry[2] == face_tbl[2][*idx]
                        && face_entry[3] == face_tbl[3][*idx] )
                    break;
                {

                    idx = &face_tbl[6][*idx];
                }
            }

            if ( *idx >= 0 )
            {
                /*
                 * The face is repeated, so make the two elements
                 * that share the face point to each other.
                 */
                elem1 = face_entry[3];
                face1 = face_entry[4];
                elem2 = face_tbl[3][ *idx ];
                face2 = face_tbl[4][ *idx ];
                pyramid_adj[ face1 ][ elem1 ] = elem2;
                pyramid_adj[ face2 ][ elem2 ] = elem1;

                /* Put the face entry on the free list. */
                tmp = face_tbl[5][*idx];
                face_tbl[5][*idx] = free_ptr;
                free_ptr = *idx;
                *idx = tmp;
            }
            else
            {
                /* Create a new face entry and append it to node list. */
                if ( free_ptr < 0 )
                    double_table_size( &tbl_sz, face_tbl, &free_ptr, 6 );
                new_idx = free_ptr;
                free_ptr = face_tbl[6][free_ptr];

                for ( i = 0; i < 6; i++ )
                    face_tbl[i][new_idx] = face_entry[i];
                face_tbl[6][new_idx] = -1;
                *idx = new_idx;
            }
        }

    /* Free the tables. */
    free( node_tbl );
    for ( i = 0; i < 6; i++ )
        free( face_tbl[i] );
}


/************************************************************
 * TAG( double_table_size )
 *
 * Expand the size of the face table to create more entries.
 */
static void
double_table_size( int *tbl_sz, int **face_tbl, int *free_ptr, int row_qty )
{
    int *new_tbl;
    int sz, i, j;

    sz = *tbl_sz;
    fprintf( stderr, "Expanding face table.\n" );
    for ( i = 0; i < row_qty; i++ )
    {
        new_tbl = NEW_N( int, 2*sz, "Face table" );
        for ( j = 0; j < sz; j++ )
            new_tbl[j] = face_tbl[i][j];
        free( face_tbl[i] );
        face_tbl[i] = new_tbl;
    }
    for ( i = sz; i < 2*sz; i++ )
        face_tbl[row_qty - 1][i] = i + 1;
    face_tbl[row_qty - 1][2*sz-1] = -1;
    *free_ptr = sz;
    *tbl_sz = sz*2;
}


/************************************************************
 * TAG( check_match )
 *
 * Check if a face entry already exists.
 */
static int *
check_match( int *face_entry, int *node_tbl, int **face_tbl, int nds_per_face )
{
    int *idx;
    int i;
    int tmp_faces1[4];
    int tmp_faces2[4];
    int repeated = 0;
    int last_idx = 5;

    if(nds_per_face == 4)
    {
        last_idx = 6;
    }

    for(i = 1; i < nds_per_face; i++)
    {
        if(face_entry[i] == face_entry[i - 1])
        {
            repeated = 1;
            break;
        }
    }

    idx = &node_tbl[face_entry[0]];

    if(repeated > 0)
    {
        if(*idx >= 0)
        {
            for(i = 0; i < nds_per_face; i++)
            {
                tmp_faces1[i] = face_entry[i];
                tmp_faces2[i] = face_tbl[i][*idx]; 
            }

            for(i = 1; i < nds_per_face - 1; i++)
            {
                if(tmp_faces1[i] == tmp_faces1[i-1])
                {
                    tmp_faces1[i] = tmp_faces1[i+1];
                }
                if(tmp_faces2[i] == tmp_faces2[i-1])
                {
                    tmp_faces2[i] = tmp_faces2[i+1];
                }
            }
        }
        while( *idx >= 0)
        {
            if( (nds_per_face == 4 && tmp_faces1[0] == tmp_faces2[0] &&
                tmp_faces1[1] == tmp_faces2[1] &&
                tmp_faces1[2] == tmp_faces2[2] &&
                tmp_faces1[3] == tmp_faces2[3]) || (nds_per_face == 3 && 
                tmp_faces1[0] == tmp_faces2[0] &&
                tmp_faces1[1] == tmp_faces2[1] &&
                tmp_faces1[2] == tmp_faces2[2] )) 
                
            {
                return idx;
            } else
            {
                idx = &face_tbl[last_idx][*idx];
                if(*idx >= 0)
                {
                    for(i = 0; i < nds_per_face; i++)
                    {
                        tmp_faces2[i] = face_tbl[i][*idx];
                    }
                    for(i = 1; i < nds_per_face - 1; i++)
                    {
                        if(tmp_faces2[i] == tmp_faces2[i-1])
                        {
                            tmp_faces2[i] = tmp_faces2[i+1];
                        }
                    }
                }
            }
        }
        return idx;
    }

    while ( *idx >= 0 )
    {
        if ( (nds_per_face == 4 && face_entry[0] == face_tbl[0][*idx] &&
                face_entry[1] == face_tbl[1][*idx] &&
                face_entry[2] == face_tbl[2][*idx] &&
                face_entry[3] == face_tbl[3][*idx])
              || (nds_per_face == 3 && face_entry[0] == face_tbl[0][*idx] &&
                face_entry[1] == face_tbl[1][*idx] &&
                face_entry[2] == face_tbl[2][*idx]
                ))
            return idx;
        else
            idx = &face_tbl[last_idx][*idx];
    }

    /* No match. */

    return idx;
}


/************************************************************
 * TAG( check_degen_face )
 *
 * Check whether an element face is degenerate.  Face may
 * degenerate to a triangle, a line segment, or a point.
 * If the face is a quadrilateral or triangle, the routine
 * returns 1; otherwise the routine returns 0.
 *
 * NOTE:
 *     The routine assumes that the face nodes have already
 * been sorted in ascending order.
 */
static Bool_type
check_degen_face( int *nodes )
{
    int match_cnt, tmp;
    int i, j;

    for ( match_cnt = 0, i = 0; i < 3; i++ )
        if ( nodes[i] == nodes[i+1] )
            match_cnt++;

    /* Check for invalid face (point or line). */
    if ( match_cnt > 1 )
        return FALSE;

    /* Check for triangle face. */
    if ( match_cnt == 1 )
    {
        /* Get rid of the redundant node. */
        for ( i = 0; i < 3; i++ )
            if ( nodes[i] == nodes[i+1] )
                break;

        /* Re-order the nodes in ascending order. */
        for ( i = 0; i < 3; i++ )
            for ( j = i+1; j < 4; j++ )
                if ( nodes[i] > nodes[j] )
                    SWAP( tmp, nodes[i], nodes[j] );
    }

    /* Valid face. */
    return TRUE;
}


/************************************************************
 * TAG( free_vis_data )
 *
 * Free Visibility_data allocations.
 */
void
free_vis_data( Visibility_data **pp_vis_data, int superclass )
{
    Visibility_data *p_vd;
    int face_qty;
    int i, j;

    switch ( superclass )
    {
    case G_TRI:
        face_qty = 0;
        break;
    case G_QUAD:
        face_qty = 0;
        break;
    case G_TET:
        face_qty = 4;
        break;
    case G_HEX:
        face_qty = 6;
        break;
    case G_SURFACE:
        face_qty = 0;
        break;
    default:
        popup_dialog( WARNING_POPUP,
                      "Un-implemented superclass in free_vis_data()." );
        return;
    }

    p_vd = *pp_vis_data;

    if ( p_vd->adjacency != NULL )
    {
        for ( i = 0; i < face_qty; i++ )
            free( p_vd->adjacency[i] );

        free( p_vd->adjacency );
    }

    if ( p_vd->visib != NULL )
        free( p_vd->visib );

    if ( p_vd->face_el != NULL )
        free( p_vd->face_el );

    if ( p_vd->face_fc != NULL )
        free( p_vd->face_fc );

    if ( p_vd->face_norm != NULL )
    {
        for ( i = 0; i < 4; i++ )
            for ( j = 0; j < 3; j++ )
                if ( p_vd->face_norm[i][j] != NULL )
                    free( p_vd->face_norm[i][j] );
    }

    free( p_vd );

    *pp_vis_data = NULL;
}


/************************************************************
 * TAG( set_tet_visibility )
 *
 * Set the visibility flag for each tetrahedral element.  Takes into
 * account element activity, invisible materials and roughcut
 * planes.
 */
void
set_tet_visibility( MO_class_data *p_tet_class, Analysis *analy )
{
    Plane_obj *plane;
    unsigned char *tet_visib, *hide_mtl;
    int *materials;
    float *activity;
    Bool_type hidden_matls;
    int tet_qty, mtl_qty;
    int i;

    tet_visib = p_tet_class->p_vis_data->visib;
    hide_mtl = analy->mesh_table[p_tet_class->mesh_id].hide_material;
    materials = p_tet_class->objects.elems->mat;
    tet_qty = p_tet_class->qty;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_tet_class->elem_class_index]
               : NULL;

    /* Initialize all elements to be visible. */
    for ( i = 0; i < tet_qty; i++ )
        tet_visib[i] = TRUE;

    if ( analy->draw_wireframe_trans )
        return;

    /*
     * If tet element is inactive (0.0) then it is invisible.
     */
    if ( activity )
    {
        for ( i = 0; i < tet_qty; i++ )
        {
            if ( activity[i] == 0.0 )
                tet_visib[i] = FALSE;
            if ( activity[i] == 0.0 && (analy->show_deleted_elements || analy->show_only_deleted_elements) )
                tet_visib[i] = TRUE;
            if ( activity[i] != 0.0 && analy->show_only_deleted_elements )
                tet_visib[i] = FALSE;
        }
    }

    /*
     * If tet element material is hidden then it is invisible.
     */
    hidden_matls = FALSE;
    mtl_qty = analy->mesh_table[p_tet_class->mesh_id].material_qty;
    for ( i = 0; i < mtl_qty; i++ )
        if ( hide_mtl[i] )
            hidden_matls = TRUE;

    if ( hidden_matls )
    {
        for ( i = 0; i < tet_qty; i++ )
            if ( hide_mtl[materials[i]] )
                tet_visib[i] = FALSE;
    }

    /*
     * Perform the roughcuts.
     */
    if ( analy->show_roughcut )
    {
        for ( plane = analy->cut_planes; plane != NULL; plane = plane->next )
            tet_rough_cut( plane->pt, plane->norm, p_tet_class,
                           analy->state_p->nodes.nodes3d, tet_visib );
    }
}

/************************************************************
* TAG( init_hex_visibility )
*
* Set the visibility flag for each element.  Takes into
* account element activity, invisible materials and roughcut
* planes.
*/
void
init_hex_visibility( MO_class_data *p_hex_class, Analysis *analy )
{
    int hex_qty;
    int i;
    unsigned char *hex_visib_damage;

    hex_visib_damage = p_hex_class->p_vis_data->visib_damage;
    hex_qty          = p_hex_class->qty;

    /* Initialize all elements to be visible. */
    for ( i = 0; i < hex_qty; i++ )
        hex_visib_damage[i] = FALSE;
}


/************************************************************
* TAG( init_particle_visibility )
*
* Set the visibility flag for each particle.
*/
void
init_particle_visibility( MO_class_data *p_particle_class, Analysis *analy )
{
    int particle_qty;
    int mat, *all_mats;
    int i;
    unsigned char *particle_visib;
    Bool_type     *p_mats;
    Mesh_data     *p_mesh;

    p_mesh             = MESH_P( analy );
    p_mats             = p_mesh->particle_mats;

    particle_visib     = p_particle_class->p_vis_data->visib;
    particle_qty       = p_particle_class->qty;
    all_mats           = p_particle_class->objects.elems->mat;

    /* Initialize all elements to be visible. */
    for ( i = 0; i < particle_qty; i++ )
    {
        particle_visib[i]     = TRUE;
        mat                   = all_mats[i];
        p_mats[mat]           = TRUE;
    }
}


/************************************************************
* TAG( set_hex_visibility )
*
* Set the visibility flag for each element.  Takes into
* account element activity, invisible materials and roughcut
* planes.
*/
void
set_hex_visibility( MO_class_data *p_hex_class, Analysis *analy )
{
    Plane_obj *plane;
    unsigned char *hex_visib, *hex_visib_damage, *hide_mtl;
    int *materials;
    float *activity;
    Bool_type hidden_matls;
    int hex_qty, mtl_qty;
    int i;

    Bool_type hide_obj=FALSE;

    int hide_qty = 0;
    int damage_count = 0;

    hex_visib        = p_hex_class->p_vis_data->visib;
    hex_visib_damage = p_hex_class->p_vis_data->visib_damage;

    /*hide_qty = analy->mesh_table[p_hex_class->mesh_id].hide_brick_elem_qty +
               analy->mesh_table[p_hex_class->mesh_id].hide_shell_elem_qty +
               analy->mesh_table[p_hex_class->mesh_id].hide_truss_elem_qty +
               analy->mesh_table[p_hex_class->mesh_id].hide_beam_elem_qty; */

    for(i = 0; i < MESH(analy).qty_class_selections; i++)
    {
        hide_qty += MESH(analy).by_class_select[i].hide_class_elem_qty;
    }

    if ( hide_qty>0 )
        hide_obj=TRUE;

    hide_mtl = analy->mesh_table[p_hex_class->mesh_id].hide_material;
    materials = p_hex_class->objects.elems->mat;
    hex_qty = p_hex_class->qty;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_hex_class->elem_class_index]
               : NULL;

    /* Initialize all elements to be visible. */
    for ( i = 0; i < hex_qty; i++ )
        hex_visib[i] = TRUE;

    if ( analy->draw_wireframe_trans )
        return;

    /*
     * If hex element is inactive (0.0) then it is invisible.
     */
    if ( activity )
    {
        for ( i = 0; i < hex_qty; i++ )
        {
            if ( activity[i] == 0.0 )
                hex_visib[i] = FALSE;
            if ( activity[i] == 0.0 && (analy->show_deleted_elements || analy->show_only_deleted_elements) )
                hex_visib[i] = TRUE;
            if ( activity[i] != 0.0 && analy->show_only_deleted_elements )
                hex_visib[i] = FALSE;
        }
    }

    /*
     * If hex element material is hidden then it is invisible.
     */
    hidden_matls = FALSE;
    mtl_qty = analy->mesh_table[p_hex_class->mesh_id].material_qty;
    for ( i = 0; i < mtl_qty; i++ )
        if ( hide_mtl[i] )
            hidden_matls = TRUE;

    if ( hidden_matls || hide_obj )
    {
        for ( i = 0; i < hex_qty; i++ )
            if ( hide_mtl[materials[i]]
                    || hide_by_object_type( p_hex_class, materials[i], i, analy, NULL ) )
                hex_visib[i] = FALSE;
    }

    /*
     * Perform the roughcuts.
     */
    if ( analy->show_roughcut )
    {
        for ( plane = analy->cut_planes; plane != NULL; plane = plane->next )
            hex_rough_cut( plane->pt, plane->norm, p_hex_class,
                           analy->state_p->nodes.nodes3d, hex_visib );
    }

    /*
     * Set the visibility for damaged elements.
     */

    /* Added 09/15/04: IRC - The field visib_damage is used to control the
     * display of damaged elements.
     */
    if (analy->damage_hide && analy->damage_result)
    {
        hex_visib_damage = p_hex_class->p_vis_data->visib_damage;
        for ( i = 0; i < hex_qty; i++ )
            if ( hex_visib_damage[i] )
            {
                hex_visib[i] = FALSE;
                damage_count++;
            }
    }
}

/************************************************************
 * TAG( hex_rough_cut )
 *
 * Display just the elements that are on one side of a cutting
 * plane.  Marks all elements on the normal side of the cutting
 * plane as invisible.
 *
 * ppt          | A point on the plane
 * norm         | Normal vector of plane.  Elements on the normal
 *              | side of the plane are not shown.
 * p_hex_class  | Hexahedral class element data
 * nodes        | Node data
 * hex_visib    | Hex element visibility
 */
static void
hex_rough_cut( float *ppt, float *norm, MO_class_data *p_hex_class,
               GVec3D *nodes, unsigned char *hex_visib )
{
    int test, i;
    int hex_qty;
    int (*connects)[8];

    hex_qty = p_hex_class->qty;
    connects = (int (*)[8]) p_hex_class->objects.elems->nodes;

    for ( i = 0; i < hex_qty; i++ )
    {
        /* If element is on exterior side of plane (in normal direction),
         * mark it as invisible.
         */
        test = test_plane_hex_elem( ppt, norm, i, connects, nodes );
        if ( test <= 0 )
            hex_visib[i] = FALSE;
    }
}

/************************************************************
 * TAG( particle_rough_cut )
 *
 * Display just the particles that are intersected by the
 * cutting plane.
 *
 * norm             | Normal vector of plane.  Elements on the normal
 *                  | side of the plane are not shown.
 * p_particle_class | Particle class element data
 * nodes            | Node data
 * particle_visib   | particle element visibility
 */
void
particle_rough_cut( Analysis *analy, float *plane_ppt, float *norm, MO_class_data *p_particle_class,
                    GVec3D *nodes, unsigned char *particle_visib )
{
    int i, j;
    int node, particle_qty;
    int (*connects)[8];
    float node_radius=0.0, ppt[3];
    double distance=0.0, distance_abs=0.0;
    Bool_type set_cut_width=FALSE;

    analy->show_particle_cut = TRUE;

    node_radius = calc_particle_radius( analy, 0.0 );
    if ( analy->free_nodes_cut_width>0. )
    {
        node_radius = calc_particle_radius( analy, analy->free_nodes_cut_width );
        set_cut_width = TRUE;
    }

    particle_qty = p_particle_class->qty;
    connects     = (int (*)[8]) p_particle_class->objects.elems->nodes;

    for ( i = 0; i < particle_qty; i++ )
    {
        node = connects[i][0]; /* All nodes at same point for particles so
				  * just need to consider 1 node.
				  */

        /* Get position of the node */
        get_node_vert_2d_3d( node, NULL, analy, ppt );

        ppt[0] = ppt[0] - plane_ppt[0];
        ppt[1] = ppt[1] - plane_ppt[1];
        ppt[2] = ppt[2] - plane_ppt[2];

        distance     = VEC_DOT( norm, ppt );
        distance_abs = fabs( distance );

        /* If particle intersects plane then mark it as invisible.
         */
        if ( distance<=0 )
            if ( set_cut_width )
            {
                if ( distance_abs<=node_radius )
                {
                    particle_visib[i]     = TRUE;
                }
            }
            else
            {
                particle_visib[i]     = TRUE;
            }
    }
}

/************************************************************
* TAG( set_particle_visibility )
*
* Set the visibility flag for each particle.  Takes into
* account element activity, invisible materials and roughcut
* planes.
*/
void
set_particle_visibility( MO_class_data *p_part_class, Analysis *analy )
{
    Plane_obj *plane;
    unsigned char *part_visib, *hide_mtl;
    int *materials;
    float *activity;
    Bool_type hidden_matls;
    int part_qty, mtl_qty, plane_qty=0;
    int i, j;

    Bool_type hide_obj=FALSE;

    int hide_qty = 0;
    int damage_count = 0;

    part_visib = p_part_class->p_vis_data->visib;

    part_qty = p_part_class->qty;

    /* Initialize all elements to be visible. */
    for ( i = 0; i < part_qty; i++ )
        part_visib[i] = TRUE;

    /*
     * Perform the cuts.
     */
    if ( analy->show_roughcut || analy->show_cut )
    {
        for ( plane = analy->cut_planes; plane != NULL; plane = plane->next )
        {
            if ( plane_qty==0 )
                for ( i = 0; i < part_qty; i++ )
                    part_visib[i] = FALSE;

            particle_rough_cut( analy, plane->pt, plane->norm, p_part_class,
                                analy->state_p->nodes.nodes3d, part_visib );
            plane_qty++;
        }
    }

    for(j = 0; j < MESH(analy).qty_class_selections; j++)
    {
        if(!strcmp(p_part_class->short_name, MESH(analy).by_class_select[j].p_class->short_name))
        {
            break;
        }
    }
    for(i = 0; i < part_qty; i++)
    {
        if(MESH(analy).by_class_select[j].hide_class_elem[i] == TRUE)
        {
            part_visib[i] = FALSE;
        }
    }
}



/************************************************************
* TAG( set_pyramid_visibility )
*
* Set the visibility flag for each element.  Takes into
* account element activity, invisible materials and roughcut
* planes.
*/
void
set_pyramid_visibility( MO_class_data *p_pyramid_class, Analysis *analy )
{
    Plane_obj *plane;
    unsigned char *pyramid_visib, *pyramid_visib_damage, *hide_mtl;
    int *materials;
    float *activity;
    Bool_type hidden_matls;
    int pyramid_qty, mtl_qty;
    int i;

    Bool_type hide_obj=FALSE;

    int hide_qty = 0;
    int damage_count = 0;

    pyramid_visib        = p_pyramid_class->p_vis_data->visib;
    pyramid_visib_damage = p_pyramid_class->p_vis_data->visib_damage;

    hide_qty = analy->mesh_table[p_pyramid_class->mesh_id].hide_brick_elem_qty +
               analy->mesh_table[p_pyramid_class->mesh_id].hide_shell_elem_qty +
               analy->mesh_table[p_pyramid_class->mesh_id].hide_truss_elem_qty +
               analy->mesh_table[p_pyramid_class->mesh_id].hide_beam_elem_qty;

    if ( hide_qty>0 )
        hide_obj=TRUE;

    hide_mtl = analy->mesh_table[p_pyramid_class->mesh_id].hide_material;
    materials = p_pyramid_class->objects.elems->mat;
    pyramid_qty = p_pyramid_class->qty;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[p_pyramid_class->elem_class_index]
               : NULL;

    /* Initialize all elements to be visible. */
    for ( i = 0; i < pyramid_qty; i++ )
        pyramid_visib[i] = TRUE;

    if ( analy->draw_wireframe_trans )
        return;

    /*
     * If pyramid element is inactive (0.0) then it is invisible.
     */
    if ( activity )
    {
        for ( i = 0; i < pyramid_qty; i++ )
        {
            if ( activity[i] == 0.0 )
                pyramid_visib[i] = FALSE;
            if ( activity[i] == 0.0 && (analy->show_deleted_elements || analy->show_only_deleted_elements) )
                pyramid_visib[i] = TRUE;
            if ( activity[i] != 0.0 && analy->show_only_deleted_elements )
                pyramid_visib[i] = FALSE;
        }
    }

    /*
     * If pyramid element material is hidden then it is invisible.
     */
    hidden_matls = FALSE;
    mtl_qty = analy->mesh_table[p_pyramid_class->mesh_id].material_qty;
    for ( i = 0; i < mtl_qty; i++ )
        if ( hide_mtl[i] )
            hidden_matls = TRUE;

    if ( hidden_matls || hide_obj )
    {
        for ( i = 0; i < pyramid_qty; i++ )
            if ( hide_mtl[materials[i]]
                    || hide_by_object_type( p_pyramid_class, materials[i], i, analy, NULL ) )
                pyramid_visib[i] = FALSE;
    }

    /*
     * Perform the roughcuts.
     */
    /*if ( analy->show_roughcut )
    {
        for ( plane = analy->cut_planes; plane != NULL; plane = plane->next )
            pyramid_rough_cut( plane->pt, plane->norm, p_pyramid_class,
                           analy->state_p->nodes.nodes3d, pyramid_visib );
    } */

    /*
     * Set the visibility for damaged elements.
     */

    /* Added 09/15/04: IRC - The field visib_damage is used to control the
     * display of damaged elements.
     */
    if (analy->damage_hide && analy->damage_result)
    {
        pyramid_visib_damage = p_pyramid_class->p_vis_data->visib_damage;
        for ( i = 0; i < pyramid_qty; i++ )
            if ( pyramid_visib_damage[i] )
            {
                pyramid_visib[i] = FALSE;
                damage_count++;
            }
    }
}

/************************************************************
 * TAG( tet_rough_cut )
 *
 * Display just the elements that are on one side of a cutting
 * plane.  Marks all elements on the normal side of the cutting
 * plane as invisible.
 *
 * ppt          | A point on the plane
 * norm         | Normal vector of plane.  Elements on the normal
 *              | side of the plane are not shown.
 * p_tet_class  | Tetrahedral class element data
 * nodes        | Node data
 * tet_visib    | Tet element visibility
 */
static void
tet_rough_cut( float *ppt, float *norm, MO_class_data *p_tet_class,
               GVec3D *nodes, unsigned char *tet_visib )
{
    int test, i;
    int tet_qty;
    int (*connects)[4];

    tet_qty = p_tet_class->qty;
    connects = (int (*)[4]) p_tet_class->objects.elems->nodes;

    for ( i = 0; i < tet_qty; i++ )
    {
        /* If element is on exterior side of plane (in normal direction),
         * mark it as invisible.
         */
        test = test_plane_tet_elem( ppt, norm, i, connects, nodes );
        if ( test <= 0 )
            tet_visib[i] = FALSE;
    }
}


/************************************************************
 * TAG( test_plane_hex_elem )
 *
 * Test how a hex element lies with respect to a plane.  Returns
 *     1 for interior (opposite normal),
 *     -1 for exterior,
 *     0 for intersection.
 * Plane is defined by a point on the plane and a normal vector.
 */
static int
test_plane_hex_elem( float *ppt, float *norm, int el, int connects[][8],
                     GVec3D *nodes )
{
    int i, j, nnum, result;
    float nd[3], con, test;

    result = 0;
    con = VEC_DOT( norm, ppt );

    /* Blindly perform eight node tests.  Plug the node point into
     * the plane equation and look at the sign of the result.
     */
    for ( i = 0; i < 8; i++ )
    {
        nnum = connects[el][i];
        for ( j = 0; j < 3; j++ )
            nd[j] = nodes[nnum][j];

        test = VEC_DOT( norm, nd ) - con;
        result += SIGN( test );
    }

    if ( result == -8 )
        return 1;
    else if ( result == 8 )
        return -1;
    else
        return 0;
}


/************************************************************
 * TAG( test_plane_tet_elem )
 *
 * Test how a tet element lies with respect to a plane.  Returns
 *     1 for interior (opposite normal),
 *     -1 for exterior,
 *     0 for intersection.
 * Plane is defined by a point on the plane and a normal vector.
 */
static int
test_plane_tet_elem( float *ppt, float *norm, int el, int connects[][4],
                     GVec3D *nodes )
{
    int i, j, nnum, result;
    float nd[3], con, test;

    result = 0;
    con = VEC_DOT( norm, ppt );

    /* Blindly perform four node tests.  Plug the node point into
     * the plane equation and look at the sign of the result.
     */
    for ( i = 0; i < 4; i++ )
    {
        nnum = connects[el][i];
        for ( j = 0; j < 3; j++ )
            nd[j] = nodes[nnum][j];

        test = VEC_DOT( norm, nd ) - con;
        result += SIGN( test );
    }

    if ( result == -4 )
        return 1;
    else if ( result == 4 )
        return -1;
    else
        return 0;
}


/************************************************************
 * TAG( get_hex_faces )
 *
 * Regenerate the volume element face lists from the element
 * adjacency information.  Assumes that hex adjacency has been
 * computed and hex visibility has been set.
 */
void
get_hex_faces( MO_class_data *p_hex_class, Analysis *analy )
{
    int **hex_adj;
    unsigned char *hex_visib;
    int *face_el;
    int *face_fc;
    int hcnt, fcnt, m1, m2;
    int i, j;
    Visibility_data *p_vd;
    Mesh_data *p_md;
    int *mat;

    hcnt = p_hex_class->qty;
    p_vd = p_hex_class->p_vis_data;
    p_md = analy->mesh_table + p_hex_class->mesh_id;
    mat = p_hex_class->objects.elems->mat;


    /* Free up old face lists. */
    if ( p_vd->face_el != NULL )
    {
        free( p_vd->face_el );
        free( p_vd->face_fc );
        for ( i = 0; i < 4; i++ )
            for ( j = 0; j < 3; j++ )
                free( p_vd->face_norm[i][j] );
    }

    /* Faster references. */
    hex_adj = p_vd->adjacency;
    hex_visib = p_vd->visib;

    /* Count the number of external faces. */
    for ( fcnt = 0, i = 0; i < hcnt; i++ )
        if ( hex_visib[i] )
            for ( j = 0; j < 6; j++ )
            {
                if ( hex_adj[j][i] == -1 || (hex_adj[j][i] >= 0 && !hex_visib[ hex_adj[j][i] ] ))
                    fcnt++;
                else if ( p_md->translate_material )
                {
                    /* Material translations can expose faces. */
                    m1 = mat[i];
                    m2 = mat[ hex_adj[j][i] ];
                    if ( p_md->mtl_trans[0][m1] != p_md->mtl_trans[0][m2] ||
                            p_md->mtl_trans[1][m1] != p_md->mtl_trans[1][m2] ||
                            p_md->mtl_trans[2][m1] != p_md->mtl_trans[2][m2] )
                        fcnt++;
                }
            }

 
    p_vd->face_cnt = fcnt;

    /* Allocate new face lists. */
    p_vd->face_el = NEW_N( int, fcnt, "Face element table" );
    p_vd->face_fc = NEW_N( int, fcnt, "Face side table" );
    for ( i = 0; i < 4; i++ )
        for ( j = 0; j < 3; j++ )
            p_vd->face_norm[i][j] = NEW_N( float, fcnt, "Face normals" );

    /* Faster references. */
    face_el = p_vd->face_el;
    face_fc = p_vd->face_fc;

    /* Build the tables. */
    for ( fcnt = 0, i = 0; i < hcnt; i++ )
        if ( hex_visib[i] )
            for ( j = 0; j < 6; j++ )
            {
                if ( hex_adj[j][i] == -1 || (hex_adj[j][i] >= 0 && !hex_visib[ hex_adj[j][i] ]))
                {
                    face_el[fcnt] = i;
                    face_fc[fcnt] = j;
                    fcnt++;
                }
                else if ( p_md->translate_material)
                {
                    /* Material translations can expose faces. */
                    m1 = mat[i];
                    m2 = mat[ hex_adj[j][i] ];
                    if ( p_md->mtl_trans[0][m1] != p_md->mtl_trans[0][m2] ||
                            p_md->mtl_trans[1][m1] != p_md->mtl_trans[1][m2] ||
                            p_md->mtl_trans[2][m1] != p_md->mtl_trans[2][m2] )
                    {
                        face_el[fcnt] = i;
                        face_fc[fcnt] = j;
                        fcnt++;
                    }
                }
            }

    /*
     * Keep in mind that the face normals still need to be computed.
     * This routine allocates space for the face normals but does not
     * compute them.
     */

     /* Bill Oliver added the following to clean up memory leaks and eliminating dangling pointers */
     hex_adj = NULL;
     hex_visib = NULL;
     p_vd = NULL;
     p_md = NULL;
     mat = NULL;
     face_el = NULL;
     face_fc = NULL;
}


/************************************************************
 * TAG( get_tet_faces )
 *
 * Regenerate the volume element face lists from the element
 * adjacency information.  Assumes that tet adjacency has been
 * computed and tet visibility has been set.
 */
void
get_tet_faces( MO_class_data *p_tet_class, Analysis *analy )
{
    int **tet_adj;
    unsigned char *tet_visib;
    int *face_el;
    int *face_fc;
    int tcnt, fcnt, m1, m2;
    int i, j;
    Visibility_data *p_vd;
    Mesh_data *p_md;
    int *mat;

    tcnt = p_tet_class->qty;
    p_vd = p_tet_class->p_vis_data;
    p_md = analy->mesh_table + p_tet_class->mesh_id;
    mat = p_tet_class->objects.elems->mat;

    /* Free up old face lists. */
    if ( p_vd->face_el != NULL )
    {
        free( p_vd->face_el );
        free( p_vd->face_fc );
        for ( i = 0; i < 3; i++ )
            for ( j = 0; j < 3; j++ )
                free( p_vd->face_norm[i][j] );
    }

    /* Faster references. */
    tet_adj = p_vd->adjacency;
    tet_visib = p_vd->visib;

    /* Count the number of external faces. */
    for ( fcnt = 0, i = 0; i < tcnt; i++ )
        if ( tet_visib[i] )
            for ( j = 0; j < 4; j++ )
            {
                if ( tet_adj[j][i] < 0 || !tet_visib[ tet_adj[j][i] ] )
                    fcnt++;
                else if ( p_md->translate_material )
                {
                    /* Material translations can expose faces. */
                    m1 = mat[i];
                    m2 = mat[ tet_adj[j][i] ];
                    if ( p_md->mtl_trans[0][m1] != p_md->mtl_trans[0][m2] ||
                            p_md->mtl_trans[1][m1] != p_md->mtl_trans[1][m2] ||
                            p_md->mtl_trans[2][m1] != p_md->mtl_trans[2][m2] )
                        fcnt++;
                }
            }
    p_vd->face_cnt = fcnt;

    /* Allocate new face lists. */
    p_vd->face_el = NEW_N( int, fcnt, "Face element table" );
    p_vd->face_fc = NEW_N( int, fcnt, "Face side table" );
    for ( i = 0; i < 3; i++ )
        for ( j = 0; j < 3; j++ )
            p_vd->face_norm[i][j] = NEW_N( float, fcnt, "Face normals" );

    /* Faster references. */
    face_el = p_vd->face_el;
    face_fc = p_vd->face_fc;

    /* Build the tables. */
    for ( fcnt = 0, i = 0; i < tcnt; i++ )
        if ( tet_visib[i] )
            for ( j = 0; j < 4; j++ )
            {
                if ( tet_adj[j][i] < 0 || !tet_visib[ tet_adj[j][i] ] )
                {
                    face_el[fcnt] = i;
                    face_fc[fcnt] = j;
                    fcnt++;
                }
                else if ( p_md->translate_material )
                {
                    /* Material translations can expose faces. */
                    m1 = mat[i];
                    m2 = mat[ tet_adj[j][i] ];
                    if ( p_md->mtl_trans[0][m1] != p_md->mtl_trans[0][m2] ||
                            p_md->mtl_trans[1][m1] != p_md->mtl_trans[1][m2] ||
                            p_md->mtl_trans[2][m1] != p_md->mtl_trans[2][m2] )
                    {
                        face_el[fcnt] = i;
                        face_fc[fcnt] = j;
                        fcnt++;
                    }
                }
            }

    /*
     * Keep in mind that the face normals still need to be computed.
     * This routine allocates space for the face normals but does not
     * compute them.
     */
}



/************************************************************
 * TAG( get_pyramid_faces )
 *
 * Regenerate the volume element face lists from the element
 * adjacency information.  Assumes that pyramid adjacency has been
 * computed and pyramid visibility has been set.
 */
void
get_pyramid_faces( MO_class_data *p_pyramid_class, Analysis *analy )
{
    int **pyramid_adj;
    unsigned char *pyramid_visib;
    int *face_el;
    int *face_fc;
    int pcnt, fcnt, m1, m2;
    int i, j;
    Visibility_data *p_vd;
    Mesh_data *p_md;
    int *mat;

    pcnt = p_pyramid_class->qty;
    p_vd = p_pyramid_class->p_vis_data;
    p_md = analy->mesh_table + p_pyramid_class->mesh_id;
    mat = p_pyramid_class->objects.elems->mat;

    /* Free up old face lists. */
    if ( p_vd->face_el != NULL )
    {
        free( p_vd->face_el );
        free( p_vd->face_fc );
        for ( i = 0; i < 4; i++ )
            for ( j = 0; j < 3; j++ )
                free( p_vd->face_norm[i][j] );
    }

    /* Faster references. */
    pyramid_adj = p_vd->adjacency;
    pyramid_visib = p_vd->visib;

    /* Count the number of external faces. */
    for ( fcnt = 0, i = 0; i < pcnt; i++ )
        if ( pyramid_visib[i] )
            for ( j = 0; j < 5; j++ )
            {
                if ( pyramid_adj[j][i] < 0 || !pyramid_visib[ pyramid_adj[j][i] ] )
                    fcnt++;
                else if ( p_md->translate_material )
                {
                    /* Material translations can expose faces. */
                    m1 = mat[i];
                    m2 = mat[ pyramid_adj[j][i] ];
                    if ( p_md->mtl_trans[0][m1] != p_md->mtl_trans[0][m2] ||
                            p_md->mtl_trans[1][m1] != p_md->mtl_trans[1][m2] ||
                            p_md->mtl_trans[2][m1] != p_md->mtl_trans[2][m2] )
                        fcnt++;
                }
            }
    p_vd->face_cnt = fcnt;

    /* Allocate new face lists. */
    p_vd->face_el = NEW_N( int, fcnt, "Face element table" );
    p_vd->face_fc = NEW_N( int, fcnt, "Face side table" );
    for ( i = 0; i < 4; i++ )
        for ( j = 0; j < 3; j++ )
            p_vd->face_norm[i][j] = NEW_N( float, fcnt, "Face normals" );

    /* Faster references. */
    face_el = p_vd->face_el;
    face_fc = p_vd->face_fc;

    /* Build the tables. */
    for ( fcnt = 0, i = 0; i < pcnt; i++ )
        if ( pyramid_visib[i] )
            for ( j = 0; j < 5; j++ )
            {
                if ( pyramid_adj[j][i] < 0 || !pyramid_visib[ pyramid_adj[j][i] ] )
                {
                    face_el[fcnt] = i;
                    face_fc[fcnt] = j;
                    fcnt++;
                }
                else if ( p_md->translate_material )
                {
                    /* Material translations can expose faces. */
                    m1 = mat[i];
                    m2 = mat[ pyramid_adj[j][i] ];
                    if ( p_md->mtl_trans[0][m1] != p_md->mtl_trans[0][m2] ||
                            p_md->mtl_trans[1][m1] != p_md->mtl_trans[1][m2] ||
                            p_md->mtl_trans[2][m1] != p_md->mtl_trans[2][m2] )
                    {
                        face_el[fcnt] = i;
                        face_fc[fcnt] = j;
                        fcnt++;
                    }
                }
            }

    /*
     * Keep in mind that the face normals still need to be computed.
     * This routine allocates space for the face normals but does not
     * compute them.
     */
}



/************************************************************
 * TAG( reset_face_visibility )
 *
 * Reset the visibility of hex elements and then regenerate
 * the face lists.  Note that the face normals are not computed
 * by this routine and will need to be recomputed after the
 * routine is called.
 */
void
reset_face_visibility( Analysis *analy )
{
    int srec_id;
    Mesh_data *p_mesh;
    MO_class_data **mo_classes;
    int class_qty;
    int i;
    int state;

    /* Get the current state record format and its mesh. */
    state = analy->cur_state + 1;
    analy->db_query( analy->db_ident, QRY_SREC_FMT_ID, &state, NULL,
                     &srec_id );

    p_mesh = MESH_P( analy );

#ifdef NEWMILI
    htable_get_data( p_mesh->class_table, (void ***) &mo_classes , &class_qty);
#else
    class_qty = htable_get_data( p_mesh->class_table, (void ***) &mo_classes);
#endif

    /* Manage visibility for volumetric element classes. */
    for ( i = 0; i < class_qty; i++ )
    {
        switch ( mo_classes[i]->superclass )
        {
        case G_HEX:
            if ( !is_particle_class( analy, mo_classes[i]->superclass, mo_classes[i]->short_name ) )
            {
                set_hex_visibility( mo_classes[i], analy );
                get_hex_faces( mo_classes[i], analy );
            }
            if ( (analy->particle_nodes_enabled || analy->free_nodes_enabled) &&
                    is_particle_class( analy, mo_classes[i]->superclass, mo_classes[i]->short_name ) )
            {
                set_particle_visibility( mo_classes[i], analy );
            }
            break;
        case G_PARTICLE:
            set_particle_visibility( mo_classes[i], analy );
            break;
        case G_TET:
            set_tet_visibility( mo_classes[i], analy );
            get_tet_faces( mo_classes[i], analy );
            break;
        case G_PYRAMID:
        case G_WEDGE:
            /* not implemented */
            break;
        default:
            ;/* Do nothing for non-volumetric mesh object types. */
        }
    }   /* for each class in mesh */

    free( mo_classes );
}


/************************************************************
 * TAG( get_tet_face_verts )
 *
 * Return the vertices of a tet element face.
 */
void
get_tet_face_verts( int elem, int face, MO_class_data *p_tet_class,
                    Analysis *analy, float verts[][3] )
{
    float orig;
    int nd, i, j;
    int (*connects)[4];
    GVec3D *nodes, *onodes;

    connects = (int (*)[4]) p_tet_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes3d;
    onodes = (GVec3D *) analy->cur_ref_state_data;

    for ( i = 0; i < 3; i++ )
    {
        nd = connects[elem][ tet_fc_nd_nums[face][i] ];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < 3; j++ )
            {
                orig = onodes[nd][j];
                verts[i][j] = orig + analy->displace_scale[j]*
                              (nodes[nd][j] - orig);
            }
        }
        else
        {
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes[nd][j];
        }
    }
}


/************************************************************
 * TAG( get_hex_face_verts )
 *
 * Return the vertices of a hex element face.
 */
void
get_hex_face_verts( int elem, int face, MO_class_data *p_hex_class,
                    Analysis *analy, float verts[][3] )
{
    float orig;
    int nd, i, j;
    int (*connects)[8];
    GVec3D *nodes, *onodes;

    connects = (int (*)[8]) p_hex_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes3d;
    onodes = (GVec3D *) analy->cur_ref_state_data;

    for ( i = 0; i < 4; i++ )
    {
        nd = connects[elem][ fc_nd_nums[face][i] ];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < 3; j++ )
            {
                orig = onodes[nd][j];
                verts[i][j] = orig + analy->displace_scale[j]*
                              (nodes[nd][j] - orig);
            }
        }
        else
        {
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes[nd][j];
        }
    }
}

/************************************************************
 * TAG( get_hex_face_nodes )
 *
 * Return the  of a hex element face.
 */
void
get_hex_face_nodes( int elem, int face, MO_class_data *p_hex_class,
                    Analysis *analy, int *faceNodes )
{
    float orig;
    int nd, i;
    int (*connects)[8];
    GVec3D *nodes, *onodes;

    connects = (int (*)[8]) p_hex_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes3d;
    onodes = (GVec3D *) analy->cur_ref_state_data;

    for ( i = 0;
            i < 4;
            i++ )
    {
        faceNodes[i] = connects[elem][ fc_nd_nums[face][i] ];
    }
}

/************************************************************
 * TAG( get_hex_verts )
 *
 * Return the vertices of a hex element.
 */
void
get_hex_verts( int elem, MO_class_data *p_hex_class, Analysis *analy,
               float verts[][3] )
{
    GVec3D *nodes, *onodes;
    int (*connects)[8];
    float orig;
    int nd, i, j;

    nodes = analy->state_p->nodes.nodes3d;
    onodes = (GVec3D *) analy->cur_ref_state_data;
    connects = (int (*)[8]) p_hex_class->objects.elems->nodes;

    for ( i = 0; i < 8; i++ )
    {
        nd = connects[elem][i];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < 3; j++ )
            {
                orig = onodes[nd][j];
                verts[i][j] = orig + analy->displace_scale[j]
                              * (nodes[nd][j] - orig);
            }
        }
        else
        {
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes[nd][j];
        }
    }
}


/************************************************************
 * TAG( get_pyramid_verts )
 *
 * Return the vertices of a pyramid element.
 */
void
get_pyramid_verts( int elem, MO_class_data *p_pyramid_class, Analysis *analy,
                   float verts[][3] )
{
    GVec3D *nodes, *onodes;
    int (*connects)[5];
    float orig;
    int nd, i, j;

    nodes = analy->state_p->nodes.nodes3d;
    onodes = (GVec3D *) analy->cur_ref_state_data;
    connects = (int (*)[5]) p_pyramid_class->objects.elems->nodes;

    for ( i = 0; i < 5; i++ )
    {
        nd = connects[elem][i];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < 3; j++ )
            {
                orig = onodes[nd][j];
                verts[i][j] = orig + analy->displace_scale[j]
                              * (nodes[nd][j] - orig);
            }
        }
        else
        {
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes[nd][j];
        }
    }
}

/************************************************************
 * TAG( get_pyramid_face_verts )
 *
 * Return the vertices of a pyramid element face.
 */
void
get_pyramid_face_verts( int elem, int face, MO_class_data *p_pyramid_class,
                        Analysis *analy, float verts[][3] )
{
    float orig;
    int nd, i, j;
    int (*connects)[5];
    GVec3D *nodes, *onodes;

    connects = (int (*)[5]) p_pyramid_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes3d;
    onodes = (GVec3D *) analy->cur_ref_state_data;

    for ( i = 0; i < 4; i++ )
    {
        nd = connects[elem][ pyramid_fc_nd_nums[face][i] ];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < 3; j++ )
            {
                orig = onodes[nd][j];
                verts[i][j] = orig + analy->displace_scale[j]*
                              (nodes[nd][j] - orig);
            }
        }
        else
        {
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes[nd][j];
        }
    }
}

/************************************************************
 * TAG( get_particle_verts )
 *
 * Return the vertices of a hex element.
 */
void
get_particle_verts( int elem, MO_class_data *p_class, Analysis *analy,
                    float verts[][3] )
{
    GVec3D *nodes, *onodes;
    int (*connects)[1];
    float orig;
    int nd, i, j;

    nodes    = analy->state_p->nodes.nodes3d;
    onodes   = (GVec3D *) analy->cur_ref_state_data;
    connects = (int (*)[1]) p_class->objects.elems->nodes;

    nd = connects[elem][0];
    for ( j = 0; j < 3; j++ )
        verts[0][j] = nodes[nd][j];
}

/************************************************************
 * TAG( get_tet_verts )
 *
 * Return the vertices of a tetrahedral element.
 */
void
get_tet_verts( int elem, MO_class_data *p_tet_class, Analysis *analy,
               float verts[][3] )
{
    GVec3D *nodes, *onodes;
    int (*connects)[4];
    float orig;
    int nd, i, j;

    nodes = analy->state_p->nodes.nodes3d;
    onodes = (GVec3D *) analy->cur_ref_state_data;
    connects = (int (*)[4]) p_tet_class->objects.elems->nodes;

    for ( i = 0; i < 4; i++ )
    {
        nd = connects[elem][i];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < 3; j++ )
            {
                orig = onodes[nd][j];
                verts[i][j] = orig + analy->displace_scale[j]
                              * (nodes[nd][j] - orig);
            }
        }
        else
        {
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes[nd][j];
        }
    }
}


/************************************************************
 * TAG( get_tri_verts_3d )
 *
 * Return the vertices of a tri element.
 */
void
get_tri_verts_3d( int elem, MO_class_data *p_tri_class, Analysis *analy,
                  float verts[][3] )
{
    float orig;
    int nd, i, j;
    GVec3D *nodes, *onodes;
    int (*connects)[3];

    connects = (int (*)[3]) p_tri_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes3d;
    onodes = (GVec3D *) analy->cur_ref_state_data;

    for ( i = 0; i < 3; i++ )
    {
        nd = connects[elem][i];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < 3; j++ )
            {
                orig = onodes[nd][j];
                verts[i][j] = orig + analy->displace_scale[j]*
                              (nodes[nd][j] - orig);
            }
        }
        else
        {
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes[nd][j];
        }
    }
}


/************************************************************
 * TAG( get_tri_verts_2d )
 *
 * Return the vertices of a tri element.
 */
void
get_tri_verts_2d( int elem, MO_class_data *p_tri_class, Analysis *analy,
                  float verts[][3] )
{
    float orig;
    int nd, i;
    GVec2D *nodes, *onodes;
    int (*connects)[3];

    connects = (int (*)[3]) p_tri_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes2d;
    onodes = (GVec2D *) analy->cur_ref_state_data;

    for ( i = 0; i < 3; i++ )
    {
        nd = connects[elem][i];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            orig = onodes[nd][0];
            verts[i][0] = orig + analy->displace_scale[0]
                          * (nodes[nd][0] - orig);

            orig = onodes[nd][1];
            verts[i][1] = orig + analy->displace_scale[1]
                          * (nodes[nd][1] - orig);
        }
        else
        {
            verts[i][0] = nodes[nd][0];
            verts[i][1] = nodes[nd][1];
        }
    }
}


/************************************************************
 * TAG( get_quad_verts_3d )
 *
 * Return the vertices of a quad element.
 */
void
/*** ORIGINAL:
get_quad_verts_3d( int elem, MO_class_data *p_quad_class, Analysis *analy,
                   float verts[][3] )
***/

get_quad_verts_3d( int elem, int *p_connectivity, int mesh_id, Analysis *analy,
                   float verts[][3] )
{
    float orig;
    int nd, i, j;
    GVec3D *nodes, *onodes;
    int (*connects)[4];

    /*
     * ORIGINAL:
    connects = (int (*)[4]) p_quad_class->objects.elems->nodes;
     */

    connects = (int (*)[4]) p_connectivity;
    nodes = analy->state_p->nodes.nodes3d;
    onodes = (GVec3D *) analy->cur_ref_state_data;

    /*
     * ORIGINAL:
    p_node_geom = analy->mesh_table[p_quad_class->mesh_id].node_geom;
     */

    for ( i = 0; i < 4; i++ )
    {
        nd = connects[elem][i];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < 3; j++ )
            {
                orig = onodes[nd][j];
                verts[i][j] = orig + analy->displace_scale[j]*
                              (nodes[nd][j] - orig);
            }
        }
        else
        {
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes[nd][j];
        }
    }
}


/************************************************************
 * TAG( get_surface_verts_3d )
 *
 * Return the vertices of a surface facet.
 */
void
get_surface_verts_3d( int elem, int *p_connectivity, int mesh_id,
                      Analysis *analy,
                      float verts[][3] )
{
    float orig;
    int nd, i, j;
    GVec3D *nodes, *onodes;
    int (*connects)[4];

    connects = (int (*)[4]) p_connectivity;
    nodes = analy->state_p->nodes.nodes3d;
    onodes = (GVec3D *) analy->cur_ref_state_data;

    for ( i = 0; i < 4; i++ )
    {
        nd = connects[elem][i];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < 3; j++ )
            {
                orig = onodes[nd][j];
                verts[i][j] = orig + analy->displace_scale[j]*
                              (nodes[nd][j] - orig);
            }
        }
        else
        {
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes[nd][j];
        }
    }
}


/************************************************************
 * TAG( get_quad_verts_2d )
 *
 * Return the vertices of a quad element.
 */
void
get_quad_verts_2d( int elem, MO_class_data *p_quad_class, Analysis *analy,
                   float verts[][3] )
{
    float orig;
    int nd, i;
    GVec2D *nodes, *onodes;
    int (*connects)[4];

    connects = (int (*)[4]) p_quad_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes2d;
    onodes = (GVec2D *) analy->cur_ref_state_data;

    for ( i = 0; i < 4; i++ )
    {
        nd = connects[elem][i];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            orig = onodes[nd][0];
            verts[i][0] = orig + analy->displace_scale[0]
                          * (nodes[nd][0] - orig);

            orig = onodes[nd][1];
            verts[i][1] = orig + analy->displace_scale[1]
                          * (nodes[nd][1] - orig);
        }
        else
        {
            verts[i][0] = nodes[nd][0];
            verts[i][1] = nodes[nd][1];
        }
    }
}


/************************************************************
 * TAG( get_surface_verts_2d )
 *
 * Return the vertices of a surface element.
 */
void
get_surface_verts_2d( int elem, MO_class_data *p_surface_class, Analysis *analy,
                      float verts[][3] )
{
    float orig;
    int nd, i;
    GVec2D *nodes, *onodes;
    int (*connects)[4];

    connects = (int (*)[4]) p_surface_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes2d;
    onodes = (GVec2D *) analy->cur_ref_state_data;

    for ( i = 0; i < 4; i++ )
    {
        nd = connects[elem][i];

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            orig = onodes[nd][0];
            verts[i][0] = orig + analy->displace_scale[0]
                          * (nodes[nd][0] - orig);

            orig = onodes[nd][1];
            verts[i][1] = orig + analy->displace_scale[1]
                          * (nodes[nd][1] - orig);
        }
        else
        {
            verts[i][0] = nodes[nd][0];
            verts[i][1] = nodes[nd][1];
        }
    }
}


/************************************************************
 * TAG( get_beam_verts_2d_3d )
 *
 * Return the vertices of a beam element.
 */
Bool_type
get_beam_verts_2d_3d( int elem, MO_class_data *p_beam_class, Analysis *analy,
                      float verts[][3] )
{
    float *nodes, *onodes;
    int (*connects)[3];
    MO_class_data *p_node_geom;
    float orig;
    int node_qty;
    int nd, i, j, dim;

    connects = (int (*)[3]) p_beam_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes;
    p_node_geom = analy->mesh_table[p_beam_class->mesh_id].node_geom;
    onodes = analy->cur_ref_state_data;
    node_qty = p_node_geom->qty;
    dim = analy->dimension;

    for ( i = 0; i < 2; i++ )
    {
        nd = connects[elem][i];

        if ( nd < 0 || nd > node_qty - 1 )
            return FALSE;

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < dim; j++ )
            {
                orig = onodes[nd * dim + j];
                verts[i][j] = orig + analy->displace_scale[j]*
                              (nodes[nd * dim + j] - orig);
            }
        }
        else
        {
            for ( j = 0; j < dim; j++ )
                verts[i][j] = nodes[nd * dim + j];
        }
    }
    return TRUE;
}


/************************************************************
 * TAG( get_truss_verts_2d_3d )
 *
 * Return the vertices of a discrete element.
 */
extern Bool_type
get_truss_verts_2d_3d( int elem, MO_class_data *p_truss_class, Analysis *analy,
                       float verts[][3] )
{
    float *nodes, *onodes;
    int (*connects)[2];
    MO_class_data *p_node_geom;
    float orig;
    int node_qty;
    int nd, i, j, dim;

    connects = (int (*)[2]) p_truss_class->objects.elems->nodes;
    nodes = analy->state_p->nodes.nodes;
    p_node_geom = analy->mesh_table[p_truss_class->mesh_id].node_geom;
    onodes = analy->cur_ref_state_data;
    node_qty = p_node_geom->qty;
    dim = analy->dimension;

    for ( i = 0; i < 2; i++ )
    {
        nd = connects[elem][i];

        if ( nd < 0 || nd > node_qty - 1 )
            return FALSE;

        if ( analy->displace_exag )
        {
            /* Scale the node displacements. */
            for ( j = 0; j < dim; j++ )
            {
                orig = onodes[nd * dim + j];
                verts[i][j] = orig + analy->displace_scale[j]*
                              (nodes[nd * dim + j] - orig);
            }
        }
        else
        {
            for ( j = 0; j < dim; j++ )
                verts[i][j] = nodes[nd * dim + j];
        }
    }

    return TRUE;
}

/************************************************************
 * TAG( get_node_vert_2d_3d )
 *
 * Return the (possibly displaced) position of a node.
 */
void
get_node_vert_2d_3d( int node_num, MO_class_data *p_node_class, Analysis *analy,
                     float vert[3] )
{
    float *nodes, *onodes;
    float orig;
    int i, dim;

    nodes = analy->state_p->nodes.nodes;
    onodes = analy->cur_ref_state_data;
    dim = analy->dimension;

    if ( analy->displace_exag )
    {
        /* Scale the node displacements. */
        for ( i = 0; i < dim; i++ )
        {
            orig = onodes[node_num * dim + i];
            vert[i] = orig + analy->displace_scale[i]
                      * (nodes[node_num * dim + i] - orig);
        }
    }
    else
    {
        for ( i = 0; i < dim; i++ )
            vert[i] = nodes[node_num * dim + i];
    }
}

#ifdef OLD_FACE_AVER
/************************************************************
 * TAG( face_aver_norm )
 *
 * Return the average normal of a brick face.
 */
static void
face_aver_norm( int elem, int face, Analysis *analy, float *norm )
{
    float vert[4][3];
    float v1[3], v2[3];

    get_face_verts( elem, face, analy, vert );

    /*
     * Get an average normal by taking the cross-product of the
     * diagonals.  This handles triangular faces correctly.
     */
    VEC_SUB( v1, vert[2], vert[0] );
    VEC_SUB( v2, vert[3], vert[1] );
    VEC_CROSS( norm, v1, v2 );
    vec_norm( norm );
}
#endif


/************************************************************
 * TAG( tet_face_avg_norm )
 *
 * Return the average normal of a tet face.
 */
static void
tet_face_avg_norm( int elem, int face, MO_class_data *p_tet_class,
                   Analysis *analy, float *norm )
{
    float vert[3][3];
    float v1[3], v2[3];

    get_tet_face_verts( elem, face, p_tet_class, analy, vert );

    /*
     * Get an average normal by taking the cross-product of the
     * diagonals.  This handles triangular faces correctly.
     */
    VEC_SUB( v1, vert[1], vert[0] );
    VEC_SUB( v2, vert[2], vert[0] );
    VEC_CROSS( norm, v1, v2 );
    vec_norm( norm );
}


/************************************************************
 * TAG( hex_face_avg_norm )
 *
 * Return the average normal of a hex face.
 */
static void
hex_face_avg_norm( int elem, int face, MO_class_data *p_hex_class,
                   Analysis *analy, float *norm )
{
    float vert[4][3];
    float v1[3], v2[3];

    get_hex_face_verts( elem, face, p_hex_class, analy, vert );

    /*
     * Get an average normal by taking the cross-product of the
     * diagonals.  This handles triangular faces correctly.
     */
    VEC_SUB( v1, vert[2], vert[0] );
    VEC_SUB( v2, vert[3], vert[1] );
    VEC_CROSS( norm, v1, v2 );
    vec_norm( norm );
}

/************************************************************
 * TAG( pyramid_face_avg_norm )
 *
 * Return the average normal of a pyramid face.
 */
static void
pyramid_face_avg_norm( int elem, int face, MO_class_data *p_pyramid_class,
                       Analysis *analy, float *norm )
{
    float vert[4][3];
    float v1[3], v2[3];

    get_pyramid_face_verts( elem, face, p_pyramid_class, analy, vert );

    /*
     * Get an average normal by taking the cross-product of the
     * diagonals.  This handles triangular faces correctly.
     */
    VEC_SUB( v1, vert[2], vert[0] );
    VEC_SUB( v2, vert[3], vert[1] );
    VEC_CROSS( norm, v1, v2 );
    vec_norm( norm );
}

/************************************************************
 * TAG( tri_avg_norm )
 *
 * Return the average normal of a tri element.
 */
static void
tri_avg_norm( int elem, MO_class_data *p_tri_class, Analysis *analy,
              float *norm )
{
    float vert[3][3];
    float v1[3], v2[3];

    get_tri_verts_3d( elem, p_tri_class, analy, vert );

    /*
     * Get an average normal by taking the cross-product of two edges. */
    VEC_SUB( v1, vert[1], vert[0] );
    VEC_SUB( v2, vert[2], vert[1] );
    VEC_CROSS( norm, v1, v2 );
    vec_norm( norm );
}


/************************************************************
 * TAG( quad_avg_norm )
 *
 * Return the average normal of a quad element.
 */
static void
quad_avg_norm( int elem, MO_class_data *p_quad_class, Analysis *analy,
               float *norm )
{
    float vert[4][3];
    float v1[3], v2[3];

    /*
     * NOTE new format of second parameter (formerly "p_quad_class" )
     */
    get_quad_verts_3d( elem, p_quad_class->objects.elems->nodes, p_quad_class->mesh_id, analy, vert );

    /*
     * Get an average normal by taking the cross-product of the
     * diagonals.  This handles triangular faces correctly.
     */
    VEC_SUB( v1, vert[2], vert[0] );
    VEC_SUB( v2, vert[3], vert[1] );
    VEC_CROSS( norm, v1, v2 );
    vec_norm( norm );
}


/************************************************************
 * TAG( surface_avg_norm )
 *
 * Return the average normal of a quad element.
 */
static void
surface_avg_norm( int elem, MO_class_data *p_surface_class, Analysis *analy,
                  float *norm )
{
    float vert[4][3];
    float v1[3], v2[3];

    get_surface_verts_3d( elem, p_surface_class->objects.surfaces->nodes,
                          p_surface_class->mesh_id, analy, vert );

    /*
     * Get an average normal by taking the cross-product of the
     * diagonals.  This handles triangular faces correctly.
     */
    VEC_SUB( v1, vert[2], vert[0] );
    VEC_SUB( v2, vert[3], vert[1] );
    VEC_CROSS( norm, v1, v2 );
    vec_norm( norm );
}


#ifdef OLD_AVER_NORM
/************************************************************
 * TAG( shell_aver_norm )
 *
 * Return the average normal of a shell element.
 */
static void
shell_aver_norm( int elem, Analysis *analy, float *norm )
{
    float vert[4][3];
    float v1[3], v2[3];

    get_shell_verts( elem, analy, vert );

    /*
     * Get an average normal by taking the cross-product of the
     * diagonals.  This handles triangular faces correctly.
     */
    VEC_SUB( v1, vert[2], vert[0] );
    VEC_SUB( v2, vert[3], vert[1] );
    VEC_CROSS( norm, v1, v2 );
    vec_norm( norm );
}
#endif

/************************************************************
 * TAG( check_for_triangle )
 *
 * Check for a triangular face (i.e., one of the four face
 * nodes is repeated.)  if the face is triangular, reorder
 * the nodes so the repeated node is last.  The routine
 * returns the number of nodes for the face (3 or 4).
 */
static int
check_for_triangle( int nodes[4] )
{
    int shift, tmp;
    int i, j, k;

    for ( i = 0; i < 4; i++ )
    {
        if ( nodes[i] == nodes[(i+1)%4] )
        {
            if ( i == 3 )
                shift = 3;
            else
                shift = 2 - i;

            for ( j = 0; j < shift; j++ )
            {
                tmp = nodes[3];
                for ( k = 3; k > 0; k-- )
                    nodes[k] = nodes[k-1];
                nodes[0] = tmp;
            }

            return( 3 );
        }
    }

    return( 4 );
}


#ifdef OLD_FACE_NORMALS
/************************************************************
 * TAG( face_normals )
 *
 * Recalculate the vertex normals for the hex element faces.
 */
static void
face_normals( Analysis *analy )
{
    Hex_geom *bricks;
    Nodal *nodes;
    float *node_norm[3];
    int *face_el;
    int *face_fc;
    int fcnt;
    float f_norm[3], n_norm[3];
    float dot;
    int i, j, k, el, face, nd, nds[4], num_nds;

    /*
     * Steps are as follows:
     *
     * 1.) Calculate face normals.
     * 2.) Calculate averaged node normals.
     * 3.) Plug either the average face normal or the average node normal
     *     into the face node normals.
     */

    /*
     * Need to take into account element type.
     */

    fcnt = analy->face_cnt;
    face_el = analy->face_el;
    face_fc = analy->face_fc;
    for(i=0;i<3;i++)
    {
        node_norm[i] = analy->node_norm[i];
    }
    nodes = analy->state_p->nodes;
    bricks = analy->geom_p->bricks;

    /*
     * Compute an average normal for each face.
     */
    for ( i = 0; i < fcnt; i++ )
    {
        el = face_el[i];
        face = face_fc[i];

        face_aver_norm( el, face, analy, f_norm );

        /* Stick the average normal into the first vertex normal. */
        for ( k = 0; k < 3; k++ )
            analy->face_norm[0][k][i] = f_norm[k];

        /* If flat shading, put average into all four vertex normals. */
        if ( !analy->normals_smooth )
            for ( j = 1; j < 4; j++ )
                for ( k = 0; k < 3; k++ )
                    analy->face_norm[j][k][i] = f_norm[k];
    }

    /*
     * For flat shading, we're done so return.
     */
    if ( !analy->normals_smooth )
        return;

    /*
     * We must be smooth shading.  Zero out the node normals array.
     */
    for ( i = 0; i < 3; i++ )
        memset( node_norm[i], 0, nodes->cnt*sizeof( float ) );

    /*
     * Compute average normals at each of the nodes.
     */
    for ( i = 0; i < fcnt; i++ )
    {
        el = face_el[i];
        face = face_fc[i];

        for ( j = 0; j < 4; j++ )
            nds[j] = bricks->nodes[ fc_nd_nums[face][j] ][el];

        /* Check for triangular (degenerate) face. */
        num_nds = 4;
        if ( bricks->degen_elems )
            num_nds = check_for_triangle( nds );

        for ( j = 0; j < num_nds; j++ )
        {
            for ( k = 0; k < 3; k++ )
                node_norm[k][nds[j]] += analy->face_norm[0][k][i];
        }

        /*
         * Note that the node normals still need to be normalized.
         */
    }

    /*
     * Compare average normals at nodes with average normals on faces
     * to determine which normal to store for each face vertex.
     */
    for ( i = 0; i < fcnt; i++ )
    {
        el = face_el[i];
        face = face_fc[i];

        for ( k = 0; k < 3; k++ )
            f_norm[k] = analy->face_norm[0][k][i];

        for ( j = 0; j < 4; j++ )
        {
            nd = bricks->nodes[ fc_nd_nums[face][j] ][el];

            for ( k = 0; k < 3; k++ )
                n_norm[k] = node_norm[k][nd];
            vec_norm( n_norm );

            /*
             * Dot product test.
             */
            dot = VEC_DOT( f_norm, n_norm );

            /*
             * Magical constant.  An edge is detected when the angle
             * between normals is greater than a critical angle.
             */
            if ( dot < crease_threshold )
            {
                /* Edge detected, use face normal. */
                for ( k = 0; k < 3; k++ )
                    analy->face_norm[j][k][i] = f_norm[k];
            }
            else
            {
                /* No edge, use node normal. */
                for ( k = 0; k < 3; k++ )
                    analy->face_norm[j][k][i] = n_norm[k];
            }
        }
    }
    for(i=0;i<3;i++)
    {
        node_norm[i] = NULL;
    }
}
#endif


/************************************************************
 * TAG( tet_face_normals )
 *
 * Recalculate the vertex normals for the tet element faces.
 */
static void
tet_face_normals( Mesh_data *p_mesh, Analysis *analy )
{
    float *node_norm[3];
    int *face_el;
    int *face_fc;
    int face_qty;
    float f_norm[3], n_norm[3];
    float dot;
    int i, j, k, l, el, face, nd, nds[3], num_nds;
    int (*connects)[4];
    MO_class_data **pp_mocd;
    MO_class_data *p_mocd;
    int class_qty;
    Visibility_data *p_vd;

    /*
     * Steps are as follows:
     *
     * 1.) Calculate face normals.
     * 2.) Calculate averaged node normals.
     * 3.) Plug either the average face normal or the average node normal
     *     into the face node normals.
     */

    pp_mocd = (MO_class_data **) p_mesh->classes_by_sclass[G_TET].list;
    class_qty = p_mesh->classes_by_sclass[G_TET].qty;
    for (i = 0; i<3 ;i++)
    {
        node_norm[i] = analy->node_norm[i];
    }

    /*
     * Compute an average normal for each face.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        p_vd = p_mocd->p_vis_data;
        face_el = p_vd->face_el;
        face_fc = p_vd->face_fc;
        face_qty = p_vd->face_cnt;

        for ( j = 0; j < face_qty; j++ )
        {
            el = face_el[j];
            face = face_fc[j];

            tet_face_avg_norm( el, face, p_mocd, analy, f_norm );

            /* Stick the average normal into the first vertex normal. */
            for ( k = 0; k < 3; k++ )
                p_vd->face_norm[0][k][j] = f_norm[k];

            /* If flat shading, put average into all four vertex normals. */
            if ( !analy->normals_smooth )
                for ( k = 1; k < 3; k++ )
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = f_norm[l];
        }
    }
    /*
     * For flat shading, we're done so return.
     */
    if ( !analy->normals_smooth )
        return;

    /*
     * We must be smooth shading.  Zero out the node normals array.
     */
    for ( i = 0; i < 3; i++ )
        memset( node_norm[i], 0, p_mesh->node_geom->qty * sizeof( float ) );

    /*
     * Compute average normals at each of the nodes.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        connects = (int (*)[4]) p_mocd->objects.elems->nodes;
        p_vd = p_mocd->p_vis_data;
        face_qty = p_vd->face_cnt;
        face_el = p_vd->face_el;
        face_fc = p_vd->face_fc;

        for ( j = 0; j < face_qty; j++ )
        {
            el = face_el[j];
            face = face_fc[j];

            for ( k = 0; k < 3; k++ )
                nds[k] = connects[el][ tet_fc_nd_nums[face][k] ];

            /* Check for degenerate (line) face. */
            num_nds = 3;
            if ( nds[0] == nds[1]
                    || nds[0] == nds[2]
                    || nds[1] == nds[2] )
                num_nds = 0; /* Inhibit any contribution from a "line" face. */

            for ( k = 0; k < num_nds; k++ )
            {
                for ( l = 0; l < 3; l++ )
                    node_norm[l][nds[k]] += p_vd->face_norm[0][l][j];
            }
        }

        /*
         * Note that the node normals still need to be normalized.
         */
    }

    /*
     * Compare average normals at nodes with average normals on faces
     * to determine which normal to store for each face vertex.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        connects = (int (*)[4]) p_mocd->objects.elems->nodes;
        p_vd = p_mocd->p_vis_data;
        face_qty = p_vd->face_cnt;
        face_el = p_vd->face_el;
        face_fc = p_vd->face_fc;

        for ( j = 0; j < face_qty; j++ )
        {
            el = face_el[j];
            face = face_fc[j];

            for ( k = 0; k < 3; k++ )
                f_norm[k] = p_vd->face_norm[0][k][j];

            for ( k = 0; k < 3; k++ )
            {
                nd = connects[el][ tet_fc_nd_nums[face][k] ];

                for ( l = 0; l < 3; l++ )
                    n_norm[l] = node_norm[l][nd];
                vec_norm( n_norm );

                /*
                 * Dot product test.
                 */
                dot = VEC_DOT( f_norm, n_norm );

                /*
                 * Magical constant.  An edge is detected when the angle
                 * between normals is greater than a critical angle.
                 */
                if ( dot < crease_threshold )
                {
                    /* Edge detected, use face normal. */
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = f_norm[l];
                }
                else
                {
                    /* No edge, use node normal. */
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = n_norm[l];
                }
            }
        }
    }
    for(i=0;i<3;i++)
    {
        node_norm[i] = NULL;
    }
}


/************************************************************
 * TAG( hex_face_normals )
 *
 * Recalculate the vertex normals for the hex element faces.
 */
static void
hex_face_normals( Mesh_data *p_mesh, Analysis *analy )
{
    float *node_norm[3];
    int *face_el;
    int *face_fc;
    int face_qty;
    float f_norm[3], n_norm[3];
    float dot;
    int i, j, k, l, el, face, nd, nds[4], num_nds;
    int (*connects)[8];
    MO_class_data **pp_mocd;
    MO_class_data *p_mocd;
    int class_qty;
    Visibility_data *p_vd;
    Bool_type has_degen;

    /*
     * Steps are as follows:
     *
     * 1.) Calculate face normals.
     * 2.) Calculate averaged node normals.
     * 3.) Plug either the average face normal or the average node normal
     *     into the face node normals.
     */

    pp_mocd = (MO_class_data **) p_mesh->classes_by_sclass[G_HEX].list;
    class_qty = p_mesh->classes_by_sclass[G_HEX].qty;
    for (i=0;i<3;i++)
    {
        node_norm[i] = analy->node_norm[i];
    }

    /*
     * Compute an average normal for each face.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        p_vd = p_mocd->p_vis_data;
        face_qty = p_vd->face_cnt;
        face_el = p_vd->face_el;
        face_fc = p_vd->face_fc;

        for ( j = 0; j < face_qty; j++ )
        {
            el = face_el[j];
            face = face_fc[j];

            hex_face_avg_norm( el, face, p_mocd, analy, f_norm );

            /* Stick the average normal into the first vertex normal. */
            for ( k = 0; k < 3; k++ )
                p_vd->face_norm[0][k][j] = f_norm[k];

            /* If flat shading, put average into all four vertex normals. */
            if ( !analy->normals_smooth )
                for ( k = 1; k < 4; k++ )
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = f_norm[l];
        }
    }
    /*
     * For flat shading, we're done so return.
     */
    if ( !analy->normals_smooth )
        return;

    /*
     * We must be smooth shading.  Zero out the node normals array.
     */
    for ( i = 0; i < 3; i++ )
        memset( node_norm[i], 0, p_mesh->node_geom->qty * sizeof( float ) );

    /*
     * Compute average normals at each of the nodes.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        connects = (int (*)[8]) p_mocd->objects.elems->nodes;
        has_degen = p_mocd->objects.elems->has_degen;
        p_vd = p_mocd->p_vis_data;
        face_qty = p_vd->face_cnt;
        face_el = p_vd->face_el;
        face_fc = p_vd->face_fc;

        for ( j = 0; j < face_qty; j++ )
        {
            el = face_el[j];
            face = face_fc[j];

            for ( k = 0; k < 4; k++ )
                nds[k] = connects[el][ fc_nd_nums[face][k] ];

            /* Check for triangular (degenerate) face. */
            num_nds = 4;
            if ( has_degen )
                num_nds = check_for_triangle( nds );

            for ( k = 0; k < num_nds; k++ )
            {
                for ( l = 0; l < 3; l++ )
                    node_norm[l][nds[k]] += p_vd->face_norm[0][l][j];
            }
        }

        /*
         * Note that the node normals still need to be normalized.
         */
    }

    /*
     * Compare average normals at nodes with average normals on faces
     * to determine which normal to store for each face vertex.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        connects = (int (*)[8]) p_mocd->objects.elems->nodes;
        p_vd = p_mocd->p_vis_data;
        face_qty = p_vd->face_cnt;
        face_el = p_vd->face_el;
        face_fc = p_vd->face_fc;

        for ( j = 0; j < face_qty; j++ )
        {
            el = face_el[j];
            face = face_fc[j];

            for ( k = 0; k < 3; k++ )
                f_norm[k] = p_vd->face_norm[0][k][j];

            for ( k = 0; k < 4; k++ )
            {
                nd = connects[el][ fc_nd_nums[face][k] ];

                for ( l = 0; l < 3; l++ )
                    n_norm[l] = node_norm[l][nd];
                vec_norm( n_norm );

                /*
                 * Dot product test.
                 */
                dot = VEC_DOT( f_norm, n_norm );

                /*
                 * Magical constant.  An edge is detected when the angle
                 * between normals is greater than a critical angle.
                 */
                if ( dot < crease_threshold )
                {
                    /* Edge detected, use face normal. */
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = f_norm[l];
                }
                else
                {
                    /* No edge, use node normal. */
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = n_norm[l];
                }
            }
        }
    }
    for(i=0;i<3;i++)
    {
        node_norm[i]=NULL;
    }
}


/************************************************************
 * TAG( pyramid_face_normals )
 *
 * Recalculate the vertex normals for the pyramid element faces.
 */
static void
pyramid_face_normals( Mesh_data *p_mesh, Analysis *analy )
{
    float *node_norm[3];
    int *face_el;
    int *face_fc;
    int face_qty;
    float f_norm[3], n_norm[3];
    float dot;
    int i, j, k, l, el, face, nd, nds[4], num_nds;
    int (*connects)[5];
    MO_class_data **pp_mocd;
    MO_class_data *p_mocd;
    int class_qty;
    Visibility_data *p_vd;
    Bool_type has_degen;

    /*
     * Steps are as follows:
     *
     * 1.) Calculate face normals.
     * 2.) Calculate averaged node normals.
     * 3.) Plug either the average face normal or the average node normal
     *     into the face node normals.
     */

    pp_mocd = (MO_class_data **) p_mesh->classes_by_sclass[G_PYRAMID].list;
    class_qty = p_mesh->classes_by_sclass[G_PYRAMID].qty;
    
    for (i = 0; i<3 ;i++)
    {
        node_norm[i] = analy->node_norm[i];
    }
    /*
     * Compute an average normal for each face.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        p_vd = p_mocd->p_vis_data;
        face_qty = p_vd->face_cnt;
        face_el = p_vd->face_el;
        face_fc = p_vd->face_fc;

        for ( j = 0; j < face_qty; j++ )
        {
            el = face_el[j];
            face = face_fc[j];

            pyramid_face_avg_norm( el, face, p_mocd, analy, f_norm );

            /* Stick the average normal into the first vertex normal. */
            for ( k = 0; k < 3; k++ )
                p_vd->face_norm[0][k][j] = f_norm[k];

            /* If flat shading, put average into all four vertex normals. */
            if ( !analy->normals_smooth )
                for ( k = 1; k < 4; k++ )
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = f_norm[l];
        }
    }
    /*
     * For flat shading, we're done so return.
     */
    if ( !analy->normals_smooth )
        return;

    /*
     * We must be smooth shading.  Zero out the node normals array.
     */
    for ( i = 0; i < 3; i++ )
        memset( node_norm[i], 0, p_mesh->node_geom->qty * sizeof( float ) );

    /*
     * Compute average normals at each of the nodes.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        connects = (int (*)[5]) p_mocd->objects.elems->nodes;
        has_degen = p_mocd->objects.elems->has_degen;
        p_vd = p_mocd->p_vis_data;
        face_qty = p_vd->face_cnt;
        face_el = p_vd->face_el;
        face_fc = p_vd->face_fc;

        for ( j = 0; j < face_qty; j++ )
        {
            el = face_el[j];
            face = face_fc[j];

            for ( k = 0; k < 4; k++ )
                nds[k] = connects[el][ fc_nd_nums[face][k] ];

            /* Check for triangular (degenerate) face. */
            num_nds = 4;
            if ( has_degen )
                num_nds = check_for_triangle( nds );

            for ( k = 0; k < num_nds; k++ )
            {
                for ( l = 0; l < 3; l++ )
                    node_norm[l][nds[k]] += p_vd->face_norm[0][l][j];
            }
        }

        /*
         * Note that the node normals still need to be normalized.
         */
    }

    /*
     * Compare average normals at nodes with average normals on faces
     * to determine which normal to store for each face vertex.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        connects = (int (*)[5]) p_mocd->objects.elems->nodes;
        p_vd = p_mocd->p_vis_data;
        face_qty = p_vd->face_cnt;
        face_el = p_vd->face_el;
        face_fc = p_vd->face_fc;

        for ( j = 0; j < face_qty; j++ )
        {
            el = face_el[j];
            face = face_fc[j];

            for ( k = 0; k < 3; k++ )
                f_norm[k] = p_vd->face_norm[0][k][j];

            for ( k = 0; k < 4; k++ )
            {
                nd = connects[el][ fc_nd_nums[face][k] ];

                for ( l = 0; l < 3; l++ )
                    n_norm[l] = node_norm[l][nd];
                vec_norm( n_norm );

                /*
                 * Dot product test.
                 */
                dot = VEC_DOT( f_norm, n_norm );

                /*
                 * Magical constant.  An edge is detected when the angle
                 * between normals is greater than a critical angle.
                 */
                if ( dot < crease_threshold )
                {
                    /* Edge detected, use face normal. */
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = f_norm[l];
                }
                else
                {
                    /* No edge, use node normal. */
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = n_norm[l];
                }
            }
        }
    }
    class_qty = p_mesh->classes_by_sclass[G_TET].qty;
    for (i = 0; i<3 ;i++)
    {
        node_norm[i] = NULL;
    }
}

/************************************************************
 * TAG( tri_normals )
 *
 * Recalculate the vertex normals for the tri elements.
 */
static void
tri_normals( Mesh_data *p_mesh, Analysis *analy )
{
    float *node_norm[3];
    float dot, f_norm[3], n_norm[3];
    int num_nd;
    int i, j, k, l, nd;
    int (*connects)[3];
    MO_class_data **pp_mocd;
    MO_class_data *p_mocd;
    int class_qty, tri_qty;
    Visibility_data *p_vd;
    Bool_type has_degen;

    pp_mocd = (MO_class_data **) p_mesh->classes_by_sclass[G_TRI].list;
    class_qty = p_mesh->classes_by_sclass[G_TRI].qty;
    
    for (i = 0; i<3 ;i++)
    {
        node_norm[i] = analy->node_norm[i];
    }
    /*
     * Compute an average polygon normal for each tri element.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        tri_qty = p_mocd->qty;
        p_vd = p_mocd->p_vis_data;

        for ( j = 0; j < tri_qty; j++ )
        {
            tri_avg_norm( j, p_mocd, analy, f_norm );

            /* Stick the average normal into the first vertex normal. */
            for ( k = 0; k < 3; k++ )
                p_vd->face_norm[0][k][j] = f_norm[k];

            /* If flat shading, put average into all four vertex normals. */
            if ( !analy->normals_smooth )
                for ( k = 1; k < 3; k++ )
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = f_norm[l];
        }
    }

    /*
     * For flat shading, we're done so return.
     */
    if ( !analy->normals_smooth )
        return;

    /*
     * We must be smooth shading.  Zero out the node normals array.
     */
    for ( i = 0; i < 3; i++ )
        memset( node_norm[i], 0, p_mesh->node_geom->qty * sizeof( float ) );

    /*
     * Compute average normals at each of the nodes.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        tri_qty = p_mocd->qty;
        p_vd = p_mocd->p_vis_data;
        connects = (int (*)[3]) p_mocd->objects.elems->nodes;
        has_degen = p_mocd->objects.elems->has_degen;

        for ( j = 0; j < tri_qty; j++ )
        {
            num_nd = 3;

            /* Check for triangular shell element. */
            if ( has_degen && connects[j][1] == connects[j][2] )
                num_nd = 2;

            for ( k = 0; k < num_nd; k++ )
            {
                nd = connects[j][k];
                for ( l = 0; l < 3; l++ )
                    node_norm[l][nd] += p_vd->face_norm[0][l][j];
            }
        }
    }

    /*
     * Compare average normals at nodes with average normals on tris
     * to determine which normal to store for each tri vertex.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        tri_qty = p_mocd->qty;
        p_vd = p_mocd->p_vis_data;
        connects = (int (*)[3]) p_mocd->objects.elems->nodes;

        for ( j = 0; j < tri_qty; j++ )
        {
            for ( k = 0; k < 3; k++ )
                f_norm[k] = p_vd->face_norm[0][k][j];

            for ( k = 0; k < 3; k++ )
            {
                nd = connects[j][k];

                for ( l = 0; l < 3; l++ )
                    n_norm[l] = node_norm[l][nd];
                vec_norm( n_norm );

                /*
                 * Dot product test.
                 */
                /*
                                Try this if adjacent shell elements have opposite
                                directed normals (i.e., normals are inconsistent).

                                dot = fabsf( VEC_DOT( f_norm, n_norm ) );
                */
                dot = VEC_DOT( f_norm, n_norm );

                /*
                 * Magical constant.  An edge is detected when the angle
                 * between normals is greater than a critical angle.
                 */
                if ( dot < crease_threshold )
                {
                    /* Edge detected, use face normal. */
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = f_norm[l];
                }
                else
                {
                    /* No edge, use node normal. */
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = n_norm[l];
                }
            }
        }
    }
    class_qty = p_mesh->classes_by_sclass[G_TET].qty;
    for (i = 0; i<3 ;i++)
    {
        node_norm[i] = NULL;
    }
}


/************************************************************
 * TAG( quad_normals )
 *
 * Recalculate the vertex normals for the quad elements.
 */
static void
quad_normals( Mesh_data *p_mesh, Analysis *analy )
{
    float *node_norm[3];
    float dot, f_norm[3], n_norm[3];
    int num_nd;
    int i, j, k, l, nd;
    int (*connects)[4];
    MO_class_data **pp_mocd;
    MO_class_data *p_mocd;
    int class_qty, quad_qty;
    Visibility_data *p_vd;
    Bool_type has_degen;

    pp_mocd = (MO_class_data **) p_mesh->classes_by_sclass[G_QUAD].list;
    class_qty = p_mesh->classes_by_sclass[G_QUAD].qty;
    
    
    for (i = 0; i<3 ;i++)
    {
        node_norm[i] = analy->node_norm[i];
    }
    /*
     * Compute an average polygon normal for each quad element.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        quad_qty = p_mocd->qty;
        p_vd = p_mocd->p_vis_data;

        for ( j = 0; j < quad_qty; j++ )
        {
            quad_avg_norm( j, p_mocd, analy, f_norm );

            /* Stick the average normal into the first vertex normal. */
            for ( k = 0; k < 3; k++ )
                p_vd->face_norm[0][k][j] = f_norm[k];

            /* If flat shading, put average into all four vertex normals. */
            if ( !analy->normals_smooth )
                for ( k = 1; k < 4; k++ )
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = f_norm[l];
        }
    }

    /*
     * For flat shading, we're done so return.
     */
    if ( !analy->normals_smooth )
        return;

    /*
     * We must be smooth shading.  Zero out the node normals array.
     */
    for ( i = 0; i < 3; i++ )
        memset( node_norm[i], 0, p_mesh->node_geom->qty * sizeof( float ) );

    /*
     * Compute average normals at each of the nodes.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        quad_qty = p_mocd->qty;
        p_vd = p_mocd->p_vis_data;
        connects = (int (*)[4]) p_mocd->objects.elems->nodes;
        has_degen = p_mocd->objects.elems->has_degen;

        for ( j = 0; j < quad_qty; j++ )
        {
            num_nd = 4;

            /* Check for triangular quad element. */
            if ( has_degen && connects[j][2] == connects[j][3] )
                num_nd = 3;

            for ( k = 0; k < num_nd; k++ )
            {
                nd = connects[j][k];
                for ( l = 0; l < 3; l++ )
                    node_norm[l][nd] += p_vd->face_norm[0][l][j];
            }
        }
    }

    /*
     * Compare average normals at nodes with average normals on shells
     * to determine which normal to store for each shell vertex.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        quad_qty = p_mocd->qty;
        p_vd = p_mocd->p_vis_data;
        connects = (int (*)[4]) p_mocd->objects.elems->nodes;

        for ( j = 0; j < quad_qty; j++ )
        {
            for ( k = 0; k < 3; k++ )
                f_norm[k] = p_vd->face_norm[0][k][j];

            for ( k = 0; k < 4; k++ )
            {
                nd = connects[j][k];

                for ( l = 0; l < 3; l++ )
                    n_norm[l] = node_norm[l][nd];
                vec_norm( n_norm );

                /*
                 * Dot product test.
                 */
                /*
                                Try this if adjacent shell elements have opposite
                                directed normals (i.e., normals are inconsistent).

                                dot = fabsf( VEC_DOT( f_norm, n_norm ) );
                */
                dot = VEC_DOT( f_norm, n_norm );

                /*
                 * Magical constant.  An edge is detected when the angle
                 * between normals is greater than a critical angle.
                 */
                if ( dot < crease_threshold )
                {
                    /* Edge detected, use face normal. */
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = f_norm[l];
                }
                else
                {
                    /* No edge, use node normal. */
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = n_norm[l];
                }
            }
        }
    }class_qty = p_mesh->classes_by_sclass[G_TET].qty;
    for (i = 0; i<3 ;i++)
    {
        node_norm[i] = NULL;
    }
    
}


/************************************************************
 * TAG( surfce_normals )
 *
 * Recalculate the vertex normals for surfaces.
 */
static void
surface_normals( Mesh_data *p_mesh, Analysis *analy )
{
    float *node_norm[3];
    float dot, f_norm[3], n_norm[3];
    int num_nd;
    int i, j, k, l, nd;
    int class_qty, surface_qty;
    int (*connects)[4];
    MO_class_data **pp_mocd;
    MO_class_data *p_mocd;
    Visibility_data *p_vd;
    Surface_data *p_surface;
    Bool_type has_degen;

    pp_mocd = (MO_class_data **) p_mesh->classes_by_sclass[G_SURFACE].list;
    class_qty = p_mesh->classes_by_sclass[G_SURFACE].qty;
    
    for (i = 0; i<3 ;i++)
    {
        node_norm[i] = analy->node_norm[i];
    }
    /*
     * Compute an average polygon normal for each surface facet.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        surface_qty = p_mocd->qty;

        for ( j = 0; j < surface_qty; j++ )
        {
            p_surface = p_mocd->objects.surfaces + j;
            p_vd = p_surface->p_vis_data;
            surface_avg_norm( j, p_mocd, analy, f_norm );

            /* Stick the average normal into the first vertex normal. */
            for ( k = 0; k < 3; k++ )
                p_vd->face_norm[0][k][j] = f_norm[k];

            /* If flat shading, put average into all four vertex normals. */
            if ( !analy->normals_smooth )
                for ( k = 1; k < 4; k++ )
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = f_norm[l];
        }
    }

    /*
     * For flat shading, we're done so return.
     */
    if ( !analy->normals_smooth )
        return;

    /*
     * We must be smooth shading.  Zero out the node normals array.
     */
    for ( i = 0; i < 3; i++ )
        memset( node_norm[i], 0, p_mesh->node_geom->qty * sizeof( float ) );

    /*
     * Compute average normals at each of the nodes.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        surface_qty = p_mocd->qty;

        for ( j = 0; j < surface_qty; j++ )
        {
            p_surface = p_mocd->objects.surfaces + j;
            connects = (int (*)[4]) p_surface->nodes;
            p_vd = p_surface->p_vis_data;
            num_nd = 4;

            /* Check for triangular facet. */
            if ( connects[j][2] == connects[j][3] )
                num_nd = 3;

            for ( k = 0; k < num_nd; k++ )
            {
                nd = connects[j][k];
                for ( l = 0; l < 3; l++ )
                    node_norm[l][nd] += p_vd->face_norm[0][l][j];
            }
        }
    }

    /*
     * Compare average normals at nodes with average normals on shells
     * to determine which normal to store for each shell vertex.
     */
    for ( i = 0; i < class_qty; i++ )
    {
        p_mocd = pp_mocd[i];
        surface_qty = p_mocd->qty;

        for ( j = 0; j < surface_qty; j++ )
        {
            p_surface = p_mocd->objects.surfaces + j;
            connects = (int (*)[4]) p_surface->nodes;
            p_vd = p_surface->p_vis_data;
            for ( k = 0; k < 3; k++ )
                f_norm[k] = p_vd->face_norm[0][k][j];

            for ( k = 0; k < 4; k++ )
            {
                nd = connects[j][k];

                for ( l = 0; l < 3; l++ )
                    n_norm[l] = node_norm[l][nd];
                vec_norm( n_norm );

                /*
                 * Dot product test.
                 */
                dot = VEC_DOT( f_norm, n_norm );

                /*
                 * Magical constant.  An edge is detected when the angle
                 * between normals is greater than a critical angle.
                 */
                if ( dot < crease_threshold )
                {
                    /* Edge detected, use face normal. */
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = f_norm[l];
                }
                else
                {
                    /* No edge, use node normal. */
                    for ( l = 0; l < 3; l++ )
                        p_vd->face_norm[k][l][j] = n_norm[l];
                }
            }
        }
    }
    for (i = 0; i<3 ;i++)
    {
        node_norm[i] = NULL;
    }
}


/************************************************************
 * TAG( compute_normals )
 *
 * Compute the vertex normals for the brick faces and shell
 * elements.
 */
void
compute_normals( Analysis *analy )
{
    int i, j;
    Mesh_data *p_md;
    static int eval_order[] =
    {
        G_TRI, G_QUAD, G_TET, G_PYRAMID, G_HEX, G_SURFACE
    };

    for ( i = 0; i < analy->mesh_qty; i++ )
    {
        p_md = analy->mesh_table + i;

        for ( j = 0; j < sizeof( eval_order ) / sizeof ( eval_order[0] ); j++ )
        {
            switch ( eval_order[j] )
            {
            case G_TRI:
                if ( p_md->classes_by_sclass[G_TRI].qty != 0 )
                    tri_normals( p_md, analy );
                break;
            case G_QUAD:
                if ( p_md->classes_by_sclass[G_QUAD].qty != 0 )
                    quad_normals( p_md, analy );
                break;
            case G_TET:
                if ( p_md->classes_by_sclass[G_TET].qty != 0 )
                    tet_face_normals( p_md, analy );
                break;
            case G_PYRAMID:
                if ( p_md->classes_by_sclass[G_PYRAMID].qty != 0 )
                    pyramid_face_normals( p_md, analy );
                break;
            case G_HEX:
                if ( p_md->classes_by_sclass[G_HEX].qty != 0 )
                    hex_face_normals( p_md, analy );
                break;
            case G_SURFACE:
                if ( p_md->classes_by_sclass[G_SURFACE].qty != 0 )
                    surface_normals( p_md, analy );
                break;
            }
        }
    }
}


/************************************************************
 * TAG( bbox_nodes )
 *
 * Compute the bounding box defined by the nodes referenced
 * by the mesh object classes referenced in array src_classes.
 * There should only be one class if it is from the G_NODE
 * superclass, as processing stops when such a class is
 * encountered.  Lower and upper bounding coords are returned in
 * bb_lo and bb_hi.
 */
void
bbox_nodes( Analysis *analy, MO_class_data **src_classes, Bool_type use_geom,
            float bb_lo[], float bb_hi[] )
{
    int i, j, k;
    int dim, offset, node_qty;
    Bool_type el_nodes_ok;
    Bool_type *tested;
    float *nodes;
    MO_class_data *p_node_class;
    float vert_lo[3], vert_hi[3];
    unsigned char *hide_mat;

    dim = analy->dimension;
    p_node_class = MESH( analy ).node_geom;
    node_qty = p_node_class->qty;

    nodes = ( use_geom ) ? p_node_class->objects.nodes
            : analy->state_p->nodes.nodes;

    for ( i = 0; i < dim; i++ )
    {
        vert_lo[i] = MAXFLOAT;
        vert_hi[i] = -MAXFLOAT;
    }

    el_nodes_ok = TRUE;
    tested = NULL;
    for ( i = 0; i < analy->bbox_source_qty && el_nodes_ok; i++ )
    {
        if ( analy->vis_mat_bbox )
        {
            if ( tested == NULL )
            {
                tested = NEW_N( Bool_type, node_qty, "Node visit flags" );
                hide_mat = MESH_P( analy )->hide_material;
            }

            find_extents( src_classes[i], dim, nodes, tested, hide_mat,
                          vert_lo, vert_hi );

            continue;
        }

        switch ( src_classes[i]->superclass )
        {
        case G_NODE:
        case G_PARTICLE:
            for ( j = 0; j < node_qty; j++ )
                for ( k = 0; k < dim; k++ )
                {
                    offset = j * dim + k;
                    if ( nodes[offset] < vert_lo[k] )
                        vert_lo[k] = nodes[offset];
                    else if ( nodes[offset] > vert_hi[k] )
                        vert_hi[k] = nodes[offset];
                }

            el_nodes_ok = FALSE;
            break;

        case G_TRUSS:
        case G_BEAM:
        case G_TRI:
        case G_QUAD:
        case G_TET:
        case G_PYRAMID:
        case G_WEDGE:
        case G_HEX:
            if ( tested == NULL )
                tested = NEW_N( Bool_type, node_qty, "Node visit flags" );
            find_extents( src_classes[i], dim, nodes, tested, NULL,
                          vert_lo, vert_hi );
            break;

        default:
            popup_dialog( WARNING_POPUP, "Invalid source class %s\n%s",
                          "for bounding box determination;",
                          "action aborted." );
            if ( tested != NULL )
                free( tested );
            return;
        }
    }

    if ( tested != NULL )
        free( tested );

    for ( i = 0; i < dim; i++ )
    {
        bb_lo[i] = vert_lo[i];
        bb_hi[i] = vert_hi[i];
    }
}


/************************************************************
 * TAG( find_extents )
 *
 * Find geometric extents of nodes referenced by the passed
 * elements.  If material invisibility array "mat_invis" is non-NULL,
 * only test the nodes of an element it its material is visible.
 */
static void
find_extents( MO_class_data *p_mo_class, int dim, float *coords,
              Bool_type tested[], unsigned char *mat_invis,
              float bb_lo[], float bb_hi[] )
{
    int i, j, k, nd, offset;
    int *connects;
    int qty_elems, qty_el_nodes, qty_usable_nodes;
    Bool_type mat_fail;
    int *elem_mat;

    qty_el_nodes = qty_connects[p_mo_class->superclass];
    qty_usable_nodes = ( p_mo_class->superclass == G_BEAM ) ? qty_el_nodes - 1
                       : qty_el_nodes;
    connects = p_mo_class->objects.elems->nodes;
    qty_elems = p_mo_class->qty;
    elem_mat = p_mo_class->objects.elems->mat;

    for ( j = 0; j < qty_elems; j++ )
    {
        mat_fail = ( mat_invis == NULL ) ? FALSE : mat_invis[elem_mat[j]];

        for ( i = 0; i < qty_usable_nodes; i++ )
        {
            nd = connects[j * qty_el_nodes + i];
            if ( mat_fail || nd < 0 )
                continue;

            if ( !tested[nd] )
            {
                tested[nd] = TRUE;
                for ( k = 0; k < dim; k++ )
                {
                    offset = nd * dim + k;
                    if ( coords[offset] < bb_lo[k] )
                        bb_lo[k] = coords[offset];
                    if ( coords[offset] > bb_hi[k] )
                        bb_hi[k] = coords[offset];
                }
            }
        }
    }
}


/************************************************************
 * TAG( get_extents )
 *
 * Find geometric extents of all nodes in the mesh. Only
 * visible materials are counted in finding the extents
 * if hide_mats is TRUE.
 *
 */
void
get_extents(  Analysis *analy,
              Bool_type use_geom, Bool_type hide_mats,
              float vert_lo[], float vert_hi[] )
{
    int i, j;
    int           dim=3, node_qty=0;
    float         *nodes;
    Mesh_data     *p_mesh;
    MO_class_data  **mo_classes, *p_mo_class, *p_node_class;
    List_head     *p_lh;

    Bool_type     *tested=NULL;
    unsigned char *hide_mat;

    dim          = analy->dimension;
    p_mesh       = MESH_P( analy );
    p_node_class = MESH( analy ).node_geom;
    p_node_class = MESH( analy ).node_geom;
    node_qty = p_node_class->qty;

    nodes = ( use_geom ) ? p_node_class->objects.nodes
            : analy->state_p->nodes.nodes;
    node_qty     = p_node_class->qty;
    tested       = NEW_N( Bool_type, node_qty, "Node visit flags" );

    if ( hide_mats )
        hide_mat     = MESH_P( analy )->hide_material;
    else
        hide_mat = NULL;

    for ( i = 0; i < dim; i++ )
    {
        vert_lo[i] = MAXFLOAT;
        vert_hi[i] = -MAXFLOAT;
    }

    for ( i = 0; i < QTY_SCLASS; i++ )
    {
        if ( !IS_ELEMENT_SCLASS( i ) )
            continue;

        p_lh = p_mesh->classes_by_sclass + i;

        /* If classes exist from current superclass... */
        if ( p_lh->qty > 0 )
        {
            mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[i].list;

            /* Loop over each class. */
            for ( j = 0;
                    j < p_lh->qty;
                    j++ )
            {
                p_mo_class = mo_classes[j];

                find_extents( p_mo_class, dim, nodes, tested, hide_mat,
                              vert_lo, vert_hi );

            }
        }
    }
    free ( tested );
}


#ifdef OLD_MAT_COUNT
/************************************************************
 * TAG( count_materials )
 *
 * Count the number of materials in the data.
 */
void
count_materials( Analysis *analy )
{
    Hex_geom *bricks;
    Shell_geom *shells;
    Beam_geom *beams;
    int i, mat_cnt;

    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;
    beams = analy->geom_p->beams;
    mat_cnt = -1;

    if ( bricks )
    {
        for ( i = 0; i < bricks->cnt; i++ )
        {
            if ( bricks->mat[i] > mat_cnt )
                mat_cnt = bricks->mat[i];
        }
    }
    if ( shells )
    {
        for ( i = 0; i < shells->cnt; i++ )
        {
            if ( shells->mat[i] > mat_cnt )
                mat_cnt = shells->mat[i];
        }
    }
    if ( beams )
    {
        for ( i = 0; i < beams->cnt; i++ )
        {
            if ( beams->mat[i] > mat_cnt )
                mat_cnt = beams->mat[i];
        }
    }

    analy->num_materials = mat_cnt + 1;
}
#endif


#ifdef OLD_FACE_MATCHES_SHELL
/************************************************************
 * TAG( face_matches_shell )
 *
 * For a specified face on a specified hex element, this routine
 * returns "TRUE" if there is a visible shell element which duplicates
 * the face.  Duplicates in the sense that it has the same four
 * nodes.  The routine doesn't check for vertices which match by
 * position but not by node number.
 */
Bool_type
face_matches_shell( int el, int face, Analysis *analy )
{
    Shell_geom *shells;
    Hex_geom *bricks;
    float *activity;
    int *materials;
    int fnodes[4], snodes[4], tmp;
    int i, j, k, match;

    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;
    materials = shells->mat;
    activity = analy->state_p->activity_present ?
               analy->state_p->shells->activity : NULL;

    /* Get the face nodes & order in ascending order. */
    for ( j = 0; j < 4; j++ )
        fnodes[j] = bricks->nodes[ fc_nd_nums[face][j] ][el];

    for ( j = 0; j < 3; j++ )
        for ( k = j+1; k < 4; k++ )
            if ( fnodes[j] > fnodes[k] )
                SWAP( tmp, fnodes[j], fnodes[k] );

    for ( i = 0; i < shells->cnt; i++ )
    {
        /* Skip this shell if it is hidden or inactive. */
        if ( (activity && activity[i] == 0.0) ||
                analy->hide_material[materials[i]] )
            continue;

        /* Get the shell nodes & order in ascending order. */
        for ( j = 0; j < 4; j++ )
            snodes[j] = shells->nodes[j][i];

        for ( j = 0; j < 3; j++ )
            for ( k = j+1; k < 4; k++ )
                if ( snodes[j] > snodes[k] )
                    SWAP( tmp, snodes[j], snodes[k] );

        /* Test for a match. */
        for ( match = 0, j = 0; j < 4; j++ )
            if ( fnodes[j] == snodes[j] )
                match++;
        if ( match == 4 )
            return( TRUE );
    }

    return( FALSE );
}
#endif


/************************************************************
 * TAG( face_matches_quad )
 *
 * For a specified face on a specified hex element, this routine
 * returns "TRUE" if there is a visible quad element which duplicates
 * the face.  Duplicates in the sense that it has the same four
 * nodes.  The routine doesn't check for vertices which match by
 * position but not by node number.
 */
Bool_type
face_matches_quad( int el, int face, MO_class_data *p_hex_class,
                   Mesh_data *p_mesh, Analysis *analy )
{
    float *activity;
    int *mat;
    int fnodes[4], snodes[4], tmp;
    int i, j, k, l, match;
    int (*connects8)[8];
    int (*connects4)[4];
    MO_class_data **quad_classes;
    int class_qty, quad_qty;
    int sand_index;

    /* Get the face nodes & order in ascending order. */
    connects8 = (int (*)[8]) p_hex_class->objects.elems->nodes;
    for ( j = 0; j < 4; j++ )
        fnodes[j] = connects8[el][ fc_nd_nums[face][j] ];

    for ( j = 0; j < 3; j++ )
        for ( k = j+1; k < 4; k++ )
            if ( fnodes[j] > fnodes[k] )
                SWAP( tmp, fnodes[j], fnodes[k] );

    quad_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_QUAD].list;
    class_qty = p_mesh->classes_by_sclass[G_QUAD].qty;

    for ( l = 0; l < class_qty; l++ )
    {
        quad_qty = quad_classes[l]->qty;
        connects4 = (int (*)[4]) quad_classes[l]->objects.elems->nodes;
        mat = quad_classes[l]->objects.elems->mat;
        sand_index = quad_classes[l]->elem_class_index;
        activity = analy->state_p->sand_present
                   ? analy->state_p->elem_class_sand[sand_index] : NULL;

        for ( i = 0; i < quad_qty; i++ )
        {
            /* Skip this quad if it is hidden or inactive. */
            if ( (activity && activity[i] == 0.0) ||
                    p_mesh->hide_material[mat[i]] )
                continue;

            /* Get the quad nodes & order in ascending order. */
            for ( j = 0; j < 4; j++ )
                snodes[j] = connects4[i][j];

            for ( j = 0; j < 3; j++ )
                for ( k = j+1; k < 4; k++ )
                    if ( snodes[j] > snodes[k] )
                        SWAP( tmp, snodes[j], snodes[k] );

            /* Test for a match. */
            for ( match = 0, j = 0; j < 4; j++ )
                if ( fnodes[j] == snodes[j] )
                    match++;
            if ( match == 4 )
                return( TRUE );
        }
    }

    return( FALSE );
}


/************************************************************
 * TAG( face_matches_tri )
 *
 * For a specified face on a specified tet element, this routine
 * returns "TRUE" if there is a visible tri element which duplicates
 * the face.  Duplicates in the sense that it has the same four
 * nodes.  The routine doesn't check for vertices which match by
 * position but not by node number.
 */
Bool_type
face_matches_tri( int el, int face, MO_class_data *p_tet_class,
                  Mesh_data *p_mesh, Analysis *analy )
{
    float *activity;
    int *mat;
    int fnodes[3], snodes[3], tmp;
    int i, j, k, l, match;
    int (*connects4)[4];
    int (*connects3)[3];
    MO_class_data **tri_classes;
    int class_qty, tri_qty;
    int sand_index;

    /* Get the face nodes & order in ascending order. */
    connects4 = (int (*)[4]) p_tet_class->objects.elems->nodes;
    for ( j = 0; j < 3; j++ )
        fnodes[j] = connects4[el][ tet_fc_nd_nums[face][j] ];

    for ( j = 0; j < 2; j++ )
        for ( k = j+1; k < 3; k++ )
            if ( fnodes[j] > fnodes[k] )
                SWAP( tmp, fnodes[j], fnodes[k] );

    tri_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_TRI].list;
    class_qty = p_mesh->classes_by_sclass[G_TRI].qty;

    for ( l = 0; l < class_qty; l++ )
    {
        tri_qty = tri_classes[l]->qty;
        connects3 = (int (*)[3]) tri_classes[l]->objects.elems->nodes;
        mat = tri_classes[l]->objects.elems->mat;
        sand_index = tri_classes[l]->elem_class_index;
        activity = analy->state_p->sand_present
                   ? analy->state_p->elem_class_sand[sand_index] : NULL;

        for ( i = 0; i < tri_qty; i++ )
        {
            /* Skip this tri if it is hidden or inactive. */
            if ( (activity && activity[i] == 0.0) ||
                    p_mesh->hide_material[mat[i]] )
                continue;

            /* Get the tri nodes & order in ascending order. */
            for ( j = 0; j < 3; j++ )
                snodes[j] = connects3[i][j];

            for ( j = 0; j < 2; j++ )
                for ( k = j+1; k < 3; k++ )
                    if ( snodes[j] > snodes[k] )
                        SWAP( tmp, snodes[j], snodes[k] );

            /* Test for a match. */
            for ( match = 0, j = 0; j < 3; j++ )
                if ( fnodes[j] == snodes[j] )
                    match++;
            if ( match == 3 )
                return( TRUE );
        }
    }

    return( FALSE );
}


/************************************************************
 * TAG( select_item )
 *
 * Select an object from a class from a screen pick.  The position
 * of the pick is in screen coordinates.
 */
int
select_item( MO_class_data *p_mo_class, int posx, int posy, Bool_type find_only,
             Analysis *analy )
{
    float line_pt[3], line_dir[3];
    char comstr[80];
    int near_num, node_near_num, p_near;
    int i;
    Mesh_data *p_mesh;

    /* Get a ray in the scene from the pick point. */
    if ( analy->dimension == 3 )
    {
        get_pick_ray( posx, posy, line_pt, line_dir );
        vec_norm( line_dir );
    }
    else
    {
        get_pick_pt( posx, posy, line_pt );
        line_pt[2] = 0.0;
    }

    switch ( p_mo_class->superclass )
    {
    case G_NODE:
        select_node( analy, MESH_P( analy ), FALSE, line_pt, line_dir, &near_num );
        break;
    case G_TRUSS:
        select_linear( analy, MESH_P( analy ), p_mo_class , line_pt,
                       line_dir, &near_num );
        break;
    case G_BEAM:
        select_linear( analy, MESH_P( analy ), p_mo_class, line_pt,
                       line_dir, &near_num );
        break;
    case G_TRI:
        select_planar( analy, MESH_P( analy ), p_mo_class, line_pt,
                       line_dir, &near_num );
        break;
    case G_QUAD:
        select_planar( analy, MESH_P( analy ), p_mo_class, line_pt,
                       line_dir, &near_num );
        break;
    case G_TET:
        select_tet( analy, MESH_P( analy ), p_mo_class, line_pt, line_dir,
                    &near_num );
        break;
    case G_PYRAMID:
        select_pyramid( analy, MESH_P( analy ), p_mo_class, line_pt, line_dir,
                        &near_num );
        break;
    case G_PARTICLE:
        select_node( analy, MESH_P( analy ), TRUE, line_pt, line_dir, &near_num );
        break;
    case G_HEX:
        if ( is_particle_class( analy, p_mo_class->superclass, p_mo_class->short_name ) && analy->particle_nodes_enabled )
        {
            select_node( analy, MESH_P( analy ), TRUE, line_pt, line_dir, &near_num );
            break;
        }
        else
            select_hex( analy, MESH_P( analy ), p_mo_class, line_pt, line_dir,
                        &near_num );
        break;
    case G_SURFACE:
        select_surf_planar( analy, MESH_P( analy ), p_mo_class, line_pt,
                            line_dir, &near_num );
        break;
    case G_UNIT:
        if ( strcmp( p_mo_class->short_name, "part" ) == 0 )
        {
            select_particle( analy, MESH_P( analy ), p_mo_class, line_pt,
                             line_dir, &near_num );
        } else
        {
            return 0;
        }
        break;
    default:
        popup_dialog( INFO_POPUP, "Unknown object type for pick." );
        return 0;
    }

    if ( near_num >= 0 )
    {
        /* Translate index into a label number */
        node_near_num = near_num;
        near_num = get_class_label( p_mo_class, near_num );

        if ( is_particle_class( analy, p_mo_class->superclass,  p_mo_class->short_name ) && p_mo_class->labels_found &&
                analy->particle_nodes_enabled )
        {
            p_mesh = MESH_P( analy );
            select_meshless_elem( analy, p_mesh,
                                  p_mo_class, node_near_num,
                                  &p_near );
            near_num = p_near;

            if ( p_near<=0 )
                return p_near;

            near_num = get_class_label( p_mo_class, p_near );

            analy->hilite_ml_node = node_near_num;
        }

    }


    if ( near_num<=0 )
        return near_num;
    
    if ( !find_only )
    {
        /* Hilite or select the picked element or node. */
        if ( analy->mouse_mode == MOUSE_HILITE )
        {
            sprintf( comstr, "hilite %s %d", p_mo_class->short_name, near_num );
        }
        else if ( analy->mouse_mode == MOUSE_SELECT )
        {
            sprintf( comstr, "select %s %d", p_mo_class->short_name, near_num );
        }

        parse_command( comstr, env.curr_analy );
    }

    return near_num;
}


/************************************************************
 * TAG( select_hex )
 *
 * Select a hex element from a screen pick.
 */
static void
select_hex( Analysis *analy, Mesh_data *p_mesh, MO_class_data *p_mo_class,
            float line_pt[3], float line_dir[3], int *p_near )
{
    float verts[4][3];
    GVec3D *nodes3d, *onodes3d;
    int (*connects)[8];
    int j, k, l;
    int *face_el, *face_fc;
    int face_cnt;
    int el, fc, nd, near_num;
    Visibility_data *p_vd;
    float pt[3], vec[3], v1[3], v2[3];
    float near_dist, orig, dist;

    near_dist = MAXFLOAT;

    connects = (int (*)[8]) p_mo_class->objects.elems->nodes;
    nodes3d = analy->state_p->nodes.nodes3d;
    p_vd = p_mo_class->p_vis_data;
    face_el = p_vd->face_el;
    face_fc = p_vd->face_fc;
    face_cnt = p_vd->face_cnt;
    near_num = -1;

    if ( analy->displace_exag )
    {
        onodes3d = (GVec3D *) analy->cur_ref_state_data;

        for ( j = 0; j < face_cnt; j++ )
        {
            el = face_el[j];
            fc = face_fc[j];

            /* Scale the node displacements. */
            for ( l = 0; l < 4; l++ )
            {
                nd = connects[el][ fc_nd_nums[fc][l] ];
                for ( k = 0; k < 3; k++ )
                {
                    orig = onodes3d[nd][k];
                    verts[l][k] = orig + analy->displace_scale[k]*
                                  (nodes3d[nd][k] - orig);
                }
            }

            VEC_SUB( v1, verts[2], verts[0] );
            VEC_SUB( v2, verts[3], verts[1] );
            VEC_CROSS( vec, v1, v2 );
            vec_norm( vec );

            /* Test for backfacing face. */
            if ( VEC_DOT( vec, line_dir ) > 0.0 )
                continue;

            if ( intersect_line_quad( verts, line_pt, line_dir, pt ) )
            {
                VEC_SUB( vec, pt, line_pt );
                dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                if ( dist < near_dist )
                {
                    near_num = el;
                    near_dist = dist;
                }
            }
        }
    }
    else /* No displacement scaling. */
    {
        for ( j = 0; j < face_cnt; j++ )
        {
            el = face_el[j];
            fc = face_fc[j];

            for ( l = 0; l < 4; l++ )
            {
                nd = connects[el][ fc_nd_nums[fc][l] ];
                for ( k = 0; k < 3; k++ )
                    verts[l][k] = nodes3d[nd][k];
            }

            VEC_SUB( v1, verts[2], verts[0] );
            VEC_SUB( v2, verts[3], verts[1] );
            VEC_CROSS( vec, v1, v2 );
            vec_norm( vec );

            /* Test for backfacing face. */
            if ( VEC_DOT( vec, line_dir ) > 0.0 )
                continue;

            if ( intersect_line_quad( verts, line_pt, line_dir, pt ) )
            {
                VEC_SUB( vec, pt, line_pt );
                dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                if ( dist < near_dist )
                {
                    near_num = el;
                    near_dist = dist;
                }
            }
        }
    }

    /* If we didn't hit an element. */
    if ( near_dist == MAXFLOAT )
    {
        parse_command( "clrhil", env.curr_analy );
        popup_dialog( INFO_POPUP, "No %s (%s) element hit.",
                      p_mo_class->short_name, p_mo_class->long_name );
        *p_near = 0;
    }
    else
        *p_near = near_num;
}



/************************************************************
 * TAG( select_pyramid )
 *
 * Select a pyramid element from a screen pick.
 */
static void
select_pyramid( Analysis *analy, Mesh_data *p_mesh, MO_class_data *p_mo_class,
                float line_pt[3], float line_dir[3], int *p_near )
{
    float verts[4][3];
    GVec3D *nodes3d, *onodes3d;
    int (*connects)[5];
    int j, k, l;
    int *face_el, *face_fc;
    int face_cnt;
    int el, fc, nd, near_num;
    Visibility_data *p_vd;
    float pt[3], vec[3], v1[3], v2[3];
    float near_dist, orig, dist;

    near_dist = MAXFLOAT;

    connects = (int (*)[5]) p_mo_class->objects.elems->nodes;
    nodes3d = analy->state_p->nodes.nodes3d;
    p_vd = p_mo_class->p_vis_data;
    face_el = p_vd->face_el;
    face_fc = p_vd->face_fc;
    face_cnt = p_vd->face_cnt;

    if ( analy->displace_exag )
    {
        onodes3d = (GVec3D *) analy->cur_ref_state_data;

        for ( j = 0; j < face_cnt; j++ )
        {
            el = face_el[j];
            fc = face_fc[j];

            /* Scale the node displacements. */
            for ( l = 0; l < 4; l++ )
            {
                nd = connects[el][ pyramid_fc_nd_nums[fc][l] ];
                for ( k = 0; k < 3; k++ )
                {
                    orig = onodes3d[nd][k];
                    verts[l][k] = orig + analy->displace_scale[k]*
                                  (nodes3d[nd][k] - orig);
                }
            }

            VEC_SUB( v1, verts[2], verts[0] );
            VEC_SUB( v2, verts[3], verts[1] );
            VEC_CROSS( vec, v1, v2 );
            vec_norm( vec );

            /* Test for backfacing face. */
            if ( VEC_DOT( vec, line_dir ) > 0.0 )
                continue;

            if ( intersect_line_quad( verts, line_pt, line_dir, pt ) )
            {
                VEC_SUB( vec, pt, line_pt );
                dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                if ( dist < near_dist )
                {
                    near_num = el;
                    near_dist = dist;
                }
            }
        }
    }
    else /* No displacement scaling. */
    {
        for ( j = 0; j < face_cnt; j++ )
        {
            el = face_el[j];
            fc = face_fc[j];

            for ( l = 0; l < 4; l++ )
            {
                nd = connects[el][ pyramid_fc_nd_nums[fc][l] ];
                for ( k = 0; k < 3; k++ )
                    verts[l][k] = nodes3d[nd][k];
            }

            VEC_SUB( v1, verts[2], verts[0] );
            VEC_SUB( v2, verts[3], verts[1] );
            VEC_CROSS( vec, v1, v2 );
            vec_norm( vec );

            /* Test for backfacing face. */
            if ( VEC_DOT( vec, line_dir ) > 0.0 )
                continue;

            if ( intersect_line_quad( verts, line_pt, line_dir, pt ) )
            {
                VEC_SUB( vec, pt, line_pt );
                dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                if ( dist < near_dist )
                {
                    near_num = el;
                    near_dist = dist;
                }
            }
        }
    }

    /* If we didn't hit an element. */
    if ( near_dist == MAXFLOAT )
    {
        parse_command( "clrhil", env.curr_analy );
        popup_dialog( INFO_POPUP, "No %s (%s) element hit.",
                      p_mo_class->short_name, p_mo_class->long_name );
        *p_near = 0;
    }
    else
        *p_near = near_num;
}

/************************************************************
 * TAG( select_tet )
 *
 * Select a tet element from a screen pick.
 */
static void
select_tet( Analysis *analy, Mesh_data *p_mesh, MO_class_data *p_mo_class,
            float line_pt[3], float line_dir[3], int *p_near )
{
    float verts[3][3];
    GVec3D *nodes3d, *onodes3d;
    int (*connects)[4];
    int j, k, l;
    int *face_el, *face_fc;
    int face_cnt;
    int el, fc, nd, near_num;
    Visibility_data *p_vd;
    float pt[3], vec[3], v1[3], v2[3];
    float near_dist, dist, orig;

    near_dist = MAXFLOAT;

    connects = (int (*)[4]) p_mo_class->objects.elems->nodes;
    nodes3d = analy->state_p->nodes.nodes3d;
    p_vd = p_mo_class->p_vis_data;
    face_el = p_vd->face_el;
    face_fc = p_vd->face_fc;
    face_cnt = p_vd->face_cnt;

    if ( analy->displace_exag )
    {
        onodes3d = (GVec3D *) analy->cur_ref_state_data;

        for ( j = 0; j < face_cnt; j++ )
        {
            el = face_el[j];
            fc = face_fc[j];

            /* Scale the node displacements. */
            for ( l = 0; l < 3; l++ )
            {
                nd = connects[el][ tet_fc_nd_nums[fc][l] ];
                for ( k = 0; k < 3; k++ )
                {
                    orig = onodes3d[nd][k];
                    verts[l][k] = orig + analy->displace_scale[k]*
                                  (nodes3d[nd][k] - orig);
                }
            }

            VEC_SUB( v1, verts[1], verts[0] );
            VEC_SUB( v2, verts[2], verts[0] );
            VEC_CROSS( vec, v1, v2 );
            vec_norm( vec );

            /* Test for backfacing face. */
            if ( VEC_DOT( vec, line_dir ) > 0.0 )
                continue;

            if ( intersect_line_tri( verts, line_pt, line_dir, pt ) )
            {
                VEC_SUB( vec, pt, line_pt );
                dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                if ( dist < near_dist )
                {
                    near_num = el;
                    near_dist = dist;
                }
            }
        }
    }
    else /* No displacement scaling. */
    {
        for ( j = 0; j < face_cnt; j++ )
        {
            el = face_el[j];
            fc = face_fc[j];

            for ( l = 0; l < 3; l++ )
            {
                nd = connects[el][ tet_fc_nd_nums[fc][l] ];
                for ( k = 0; k < 3; k++ )
                    verts[l][k] = nodes3d[nd][k];
            }

            VEC_SUB( v1, verts[1], verts[0] );
            VEC_SUB( v2, verts[2], verts[0] );
            VEC_CROSS( vec, v1, v2 );
            vec_norm( vec );

            /* Test for backfacing face. */
            if ( VEC_DOT( vec, line_dir ) > 0.0 )
                continue;

            if ( intersect_line_tri( verts, line_pt, line_dir, pt ) )
            {
                VEC_SUB( vec, pt, line_pt );
                dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                if ( dist < near_dist )
                {
                    near_num = el;
                    near_dist = dist;
                }
            }
        }
    }

    /* If we didn't hit an element. */
    if ( near_dist == MAXFLOAT )
    {
        parse_command( "clrhil", env.curr_analy );
        popup_dialog( INFO_POPUP, "No %s (%s) element hit.",
                      p_mo_class->short_name, p_mo_class->long_name );
        *p_near = 0;
    }
    else
        *p_near = near_num;
}


/************************************************************
 * TAG( select_planar )
 *
 * Select a tri or quad element from a screen pick.
 */
static void
select_planar( Analysis *analy, Mesh_data *p_mesh, MO_class_data *p_mo_class,
               float line_pt[3], float line_dir[3], int *p_near )
{
    int mo_qty;
    unsigned char *hide_mtl;
    float verts[4][3];
    GVec3D *nodes3d, *onodes3d;
    GVec2D *nodes2d, *onodes2d;
    int *connects;
    int j, k, l;
    int *mat;
    int conn_qty;
    static float zdir[] = { 0.0, 0.0, 1.0 };
    int (*intersect_func)();
    float pt[3], vec[3];
    float near_dist, orig, dist;
    int node_base, nd, near_num;

    if ( p_mo_class->superclass == G_QUAD )
    {
        conn_qty = 4;
        intersect_func = intersect_line_quad;
    }
    else
    {
        conn_qty = 3;
        intersect_func = intersect_line_tri;
    }

    hide_mtl = p_mesh->hide_material;
    near_dist = MAXFLOAT;

    mo_qty = p_mo_class->qty;
    connects = p_mo_class->objects.elems->nodes;
    mat = p_mo_class->objects.elems->mat;

    if ( analy->dimension == 3 )
    {
        nodes3d = analy->state_p->nodes.nodes3d;

        if ( analy->displace_exag )
        {
            onodes3d = (GVec3D *) analy->cur_ref_state_data;

            for ( j = 0; j < mo_qty; j++ )
            {
                if ( hide_mtl[mat[j]] )
                    continue;

                /* Scale the node displacements. */
                node_base = j * conn_qty;
                for ( l = 0; l < conn_qty; l++ )
                {
                    nd = connects[node_base + l];
                    for ( k = 0; k < 3; k++ )
                    {
                        orig = onodes3d[nd][k];
                        verts[l][k] = orig + analy->displace_scale[k]*
                                      (nodes3d[nd][k] - orig);
                    }
                }

                if ( intersect_func( verts, line_pt, line_dir, pt ) )
                {
                    VEC_SUB( vec, pt, line_pt );
                    dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                    if ( dist < near_dist )
                    {
                        near_num = j;
                        near_dist = dist;
                    }
                }
            }
        }
        else /* No displacement scaling. */
        {
            for ( j = 0; j < mo_qty; j++ )
            {
                if ( hide_mtl[mat[j]] )
                    continue;

                node_base = j * conn_qty;
                for ( l = 0; l < conn_qty; l++ )
                {
                    nd = connects[node_base + l];
                    for ( k = 0; k < 3; k++ )
                        verts[l][k] = nodes3d[nd][k];
                }

                if ( intersect_func( verts, line_pt, line_dir, pt ) )
                {
                    VEC_SUB( vec, pt, line_pt );
                    dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                    if ( dist < near_dist )
                    {
                        near_num = j;
                        near_dist = dist;
                    }
                }
            }
        }
    }
    else /* 2D database */
    {
        nodes2d = analy->state_p->nodes.nodes2d;
        verts[0][2] = verts[1][2] = verts[2][2] = verts[3][2] = 0.0;

        if ( analy->displace_exag )
        {
            onodes2d = (GVec2D *) analy->cur_ref_state_data;

            for ( j = 0; j < mo_qty; j++ )
            {
                if ( hide_mtl[mat[j]] )
                    continue;

                /* Scale the node displacements. */
                node_base = j * conn_qty;
                for ( l = 0; l < conn_qty; l++ )
                {
                    nd = connects[node_base + l];
                    for ( k = 0; k < 2; k++ )
                    {
                        orig = onodes2d[nd][k];
                        verts[l][k] = orig + analy->displace_scale[k]*
                                      (nodes2d[nd][k] - orig);
                    }
                }

                if ( intersect_func( verts, line_pt, zdir, pt ) )
                {
                    VEC_SUB_2D( vec, pt, line_pt );
                    dist = VEC_DOT_2D( vec, vec );     /* Skip the sqrt. */

                    if ( dist < near_dist )
                    {
                        near_num = j;
                        near_dist = dist;
                    }
                }
            }
        }
        else /* No displacement scaling. */
        {
            for ( j = 0; j < mo_qty; j++ )
            {
                if ( hide_mtl[mat[j]] )
                    continue;

                node_base = j * conn_qty;
                for ( l = 0; l < conn_qty; l++ )
                {
                    nd = connects[node_base + l];
                    for ( k = 0; k < 2; k++ )
                        verts[l][k] = nodes2d[nd][k];
                }

                if ( intersect_func( verts, line_pt, zdir, pt ) )
                {
                    VEC_SUB_2D( vec, pt, line_pt );
                    dist = VEC_DOT_2D( vec, vec );     /* Skip the sqrt. */

                    if ( dist < near_dist )
                    {
                        near_num = j;
                        near_dist = dist;
                    }
                }
            }
        }
    }

    /* If we didn't hit an element. */
    if ( near_dist == MAXFLOAT )
    {
        parse_command( "clrhil", env.curr_analy );
        popup_dialog( INFO_POPUP, "No %s (%s) element hit.",
                      p_mo_class->short_name, p_mo_class->long_name );
        *p_near = 0;
    }
    else
        *p_near = near_num ;
}


/************************************************************
 * TAG( select_surf_planar )
 *
 * Select a surface from a screen pick.
 */
static void
select_surf_planar( Analysis *analy, Mesh_data *p_mesh,
                    MO_class_data *p_mo_class,
                    float line_pt[3], float line_dir[3], int *p_near )
{
    int mo_qty;
    Surface_data *p_surf;
    unsigned char *hide_surf;
    float verts[4][3];
    GVec3D *nodes3d, *onodes3d;
    GVec2D *nodes2d, *onodes2d;
    int *connects=NULL;
    int j, k, l;
    int ii, jj, kk;
    int facet_qty;
    int conn_qty = 4;
    static float zdir[] = { 0.0, 0.0, 1.0 };
    int (*intersect_func)();
    float pt[3], vec[3];
    float near_dist, orig, dist;
    int node_base, nd, near_num;
    int near_facet;

    /* for now assume quad connects */
    connects = p_mo_class->objects.elems->nodes;

    intersect_func = intersect_line_quad;
    hide_surf = p_mesh->hide_surface;
    near_dist = MAXFLOAT;

    mo_qty = p_mo_class->qty;
    p_surf = p_mo_class->objects.surfaces;

    if ( analy->dimension == 3 )
    {
        nodes3d = analy->state_p->nodes.nodes3d;

        if ( analy->displace_exag )
        {
            onodes3d = (GVec3D *) analy->cur_ref_state_data;

            for ( j = 0; j < mo_qty; j++ )
            {
                if ( hide_surf[j] )
                    continue;

                connects = p_surf[j].nodes;
                facet_qty = p_surf[j].facet_qty;

                for ( k = 0; k < facet_qty; k++ )
                {
                    node_base = k * conn_qty;

                    /* Scale the node displacements. */
                    for ( l = 0; l < conn_qty; l++ )
                    {
                        nd = connects[node_base + l];
                        for ( ii = 0; ii < 3; ii++ )
                        {
                            orig = onodes3d[nd][ii];
                            verts[l][ii] = orig + analy->displace_scale[ii]*
                                           (nodes3d[nd][ii] - orig);
                        }
                    }

                    if ( intersect_func( verts, line_pt, line_dir, pt ) )
                    {
                        VEC_SUB( vec, pt, line_pt );
                        dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                        if ( dist < near_dist )
                        {
                            near_num = k;
                            near_dist = dist;
                        }
                    }
                }
            }
        }
        else /* No displacement scaling. */
        {
            for ( j = 0; j < mo_qty; j++ )
            {
                if ( hide_surf[j] )
                    continue;

                connects = p_surf[j].nodes;
                facet_qty = p_surf[j].facet_qty;

                for ( k = 0; k < facet_qty; k++ )
                {
                    node_base = k * conn_qty;
                    for ( l = 0; l < conn_qty; l++ )
                    {
                        nd = connects[node_base + l];
                        for ( ii = 0; ii < 3; ii++ )
                            verts[l][ii] = nodes3d[nd][ii];
                    }

                    if ( intersect_func( verts, line_pt, line_dir, pt ) )
                    {
                        VEC_SUB( vec, pt, line_pt );
                        dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                        if ( dist < near_dist )
                        {
                            near_num = k;
                            near_dist = dist;
                        }
                    }
                }
            }
        }
    }
    else /* 2D database */
    {
        nodes2d = analy->state_p->nodes.nodes2d;
        verts[0][2] = verts[1][2] = verts[2][2] = verts[3][2] = 0.0;

        if ( analy->displace_exag )
        {
            onodes2d = (GVec2D *) analy->cur_ref_state_data;

            for ( j = 0; j < mo_qty; j++ )
            {
                if ( hide_surf[j] )
                    continue;

                /* Scale the node displacements. */
                node_base = j * conn_qty;
                for ( l = 0; l < conn_qty; l++ )
                {
                    nd = connects[node_base + l];
                    for ( k = 0; k < 2; k++ )
                    {
                        orig = onodes2d[nd][k];
                        verts[l][k] = orig + analy->displace_scale[k]*
                                      (nodes2d[nd][k] - orig);
                    }
                }

                if ( intersect_func( verts, line_pt, zdir, pt ) )
                {
                    VEC_SUB_2D( vec, pt, line_pt );
                    dist = VEC_DOT_2D( vec, vec );     /* Skip the sqrt. */

                    if ( dist < near_dist )
                    {
                        near_num = k;
                        near_dist = dist;
                    }
                }
            }
        }
        else /* No displacement scaling. */
        {
            for ( j = 0; j < mo_qty; j++ )
            {
                if ( hide_surf[j] )
                    continue;

                node_base = j * conn_qty;
                for ( l = 0; l < conn_qty; l++ )
                {
                    nd = connects[node_base + l];
                    for ( k = 0; k < 2; k++ )
                        verts[l][k] = nodes2d[nd][k];
                }

                if ( intersect_func( verts, line_pt, zdir, pt ) )
                {
                    VEC_SUB_2D( vec, pt, line_pt );
                    dist = VEC_DOT_2D( vec, vec );     /* Skip the sqrt. */

                    if ( dist < near_dist )
                    {
                        near_num = k;
                        near_dist = dist;
                    }
                }
            }
        }
    }

    /* If we didn't hit an element. */
    if ( near_dist == MAXFLOAT )
    {
        parse_command( "clrhil", env.curr_analy );
        popup_dialog( INFO_POPUP, "No %s (%s) element hit.",
                      p_mo_class->short_name, p_mo_class->long_name );
        *p_near = 0;
        p_surf->sel_facet_num = -1;
    }
    else
        *p_near = near_num ;
}

/************************************************************
 * TAG( select_linear )
 *
 * Select a beam or truss element from a screen pick.
 */
static void
select_linear( Analysis *analy, Mesh_data *p_mesh, MO_class_data *p_mo_class,
               float line_pt[3], float line_dir[3], int *p_near )
{
    int mo_qty;
    unsigned char *hide_mtl;
    static float zdir[] = { 0.0, 0.0, 1.0 };
    float verts[2][3];
    GVec3D *nodes3d, *onodes3d;
    GVec2D *nodes2d, *onodes2d;
    int *connects;
    int j, k, l;
    int *mat;
    int conn_qty;
    int node_base, nd, near_num;
    float near_dist, dist, orig;

    if ( p_mo_class->superclass == G_BEAM )
        conn_qty = 3;
    else
        conn_qty = 2;

    hide_mtl = p_mesh->hide_material;
    near_dist = MAXFLOAT;

    mo_qty = p_mo_class->qty;
    connects = p_mo_class->objects.elems->nodes;
    mat = p_mo_class->objects.elems->mat;

    if ( analy->dimension == 3 )
    {
        nodes3d = analy->state_p->nodes.nodes3d;

        if ( analy->displace_exag )
        {
            onodes3d = (GVec3D *) analy->cur_ref_state_data;

            for ( j = 0; j < mo_qty; j++ )
            {
                if ( hide_mtl[mat[j]] )
                    continue;

                node_base = j * conn_qty;
                for ( l = 0; l < 2; l++ )
                {
                    nd = connects[node_base + l];
                    for ( k = 0; k < 3; k++ )
                    {
                        orig = onodes3d[nd][k];
                        verts[l][k] = orig + analy->displace_scale[k]
                                      * (nodes3d[nd][k] - orig);
                    }
                }

                dist = sqr_dist_seg_to_line( verts[0], verts[1],
                                             line_pt, line_dir );

                if ( dist < near_dist )
                {
                    near_num = j;
                    near_dist = dist;
                }
            }
        }
        else /* no displacement scaling */
        {
            for ( j = 0; j < mo_qty; j++ )
            {
                if ( hide_mtl[mat[j]] )
                    continue;

                node_base = j * conn_qty;
                for ( l = 0; l < 2; l++ )
                {
                    nd = connects[node_base + l];
                    for ( k = 0; k < 3; k++ )
                        verts[l][k] = nodes3d[nd][k];
                }

                dist = sqr_dist_seg_to_line( verts[0], verts[1],
                                             line_pt, line_dir );

                if ( dist < near_dist )
                {
                    near_num = j;
                    near_dist = dist;
                }
            }
        }
    }
    else /* 2D database */
    {
        nodes2d = analy->state_p->nodes.nodes2d;
        verts[0][2] = verts[1][2] = 0.0;

        if ( analy->displace_exag )
        {
            onodes2d = (GVec2D *) analy->cur_ref_state_data;

            for ( j = 0; j < mo_qty; j++ )
            {
                if ( hide_mtl[mat[j]] )
                    continue;

                node_base = j * conn_qty;
                for ( l = 0; l < 2; l++ )
                {
                    nd = connects[node_base + l];
                    for ( k = 0; k < 2; k++ )
                    {
                        orig = onodes2d[nd][k];
                        verts[l][k] = orig + analy->displace_scale[k]
                                      * (nodes2d[nd][k] - orig);
                    }
                }

                dist = sqr_dist_seg_to_line( verts[0], verts[1],
                                             line_pt, zdir );

                if ( dist < near_dist )
                {
                    near_num = j;
                    near_dist = dist;
                }
            }
        }
        else /* no displacement scaling */
        {
            for ( j = 0; j < mo_qty; j++ )
            {
                if ( hide_mtl[mat[j]] )
                    continue;

                node_base = j * conn_qty;
                for ( l = 0; l < 2; l++ )
                {
                    nd = connects[node_base + l];
                    for ( k = 0; k < 2; k++ )
                        verts[l][k] = nodes2d[nd][k];
                }

                dist = sqr_dist_seg_to_line( verts[0], verts[1],
                                             line_pt, zdir );

                if ( dist < near_dist )
                {
                    near_num = j;
                    near_dist = dist;
                }
            }
        }
    }

    /* If we didn't hit an element. */
    if ( near_dist == MAXFLOAT )
    {
        parse_command( "clrhil", env.curr_analy );
        popup_dialog( INFO_POPUP, "No %s (%s) element hit.",
                      p_mo_class->short_name, p_mo_class->long_name );
        *p_near = 0;
    }
    else
        *p_near = near_num ;
}


/************************************************************
 * TAG( select_node )
 *
 * Select a node from a screen pick.
 */
static void
select_node( Analysis *analy, Mesh_data *p_mesh, Bool_type ml_node,
             float line_pt[3],
             float line_dir[3], int *p_near )
{
    float pt[3], near_pt[3], vec[3];
    float near_dist, dist;
    float *onsurf;
    MO_class_data **mo_classes;
    MO_class_data *p_node_class;
    int class_qty, node_qty;
    int (*connects8)[8];
    int (*connects4)[4];
    int (*connects3)[3];
    int (*connects2)[2];
    int *face_el, *face_fc;
    int face_cnt;
    int nd, el, fc, i, j, k, near_num;
    int *mat;
    unsigned char *hide_mtl;
    GVec3D *nodes3d, *onodes3d;
    GVec2D *nodes2d, *onodes2d;
    int dims;
    float orig;

    int *free_nodes;

    Bool_type ml_found=FALSE;

    p_node_class = p_mesh->node_geom;
    node_qty = p_node_class->qty;

    /* Init to ensure update. */
    near_dist = MAXFLOAT;

    /* Mark all nodes that are on the surface of the mesh. */
    onsurf = analy->tmp_result[0];

    for ( i = 0; i < node_qty; i++ )
    {
        onsurf[i] = 0.0;
        if ( ml_node )
            onsurf[i] = 1.0;
    }

    dims = analy->dimension;

    if ( dims == 3 )
    {
        /* Mark nodes shared by visible hex faces. */
        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_HEX].list;
        class_qty = p_mesh->classes_by_sclass[G_HEX].qty;

        for ( i = 0; i < class_qty; i++ )
        {
            if(!ml_node && mo_classes[i]->p_vis_data->face_el == NULL)
            {
                if(class_qty < 2)
                {
                    return;
                }
                continue;
            }
            face_el = mo_classes[i]->p_vis_data->face_el;
            face_fc = mo_classes[i]->p_vis_data->face_fc;
            face_cnt = mo_classes[i]->p_vis_data->face_cnt;
            connects8 = (int (*)[8]) mo_classes[i]->objects.elems->nodes;

            if ( is_particle_class( analy, mo_classes[i]->superclass, mo_classes[i]->short_name ) )
                ml_found = TRUE;

            for ( j = 0; j < face_cnt; j++ )
            {
                el = face_el[j];
                fc = face_fc[j];

                hex_face_avg_norm( el, fc, mo_classes[i], analy, vec );

                /* Test for backfacing face. */
                if ( VEC_DOT( vec, line_dir ) > 0 )
                    continue;

                for ( k = 0; k < 4; k++ )
                {
                    nd = connects8[el][ fc_nd_nums[fc][k] ];
                    onsurf[nd] = 1.0;
                }
            }
        }

        if ( ml_found ) /* Consider all nodes */
            for ( i = 0; i < node_qty; i++ )
                onsurf[i] = 1.0;

        /* Mark nodes shared by visible tet faces. */

        mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_TET].list;
        class_qty = p_mesh->classes_by_sclass[G_TET].qty;

        for ( i = 0; i < class_qty; i++ )
        {
            face_el = mo_classes[i]->p_vis_data->face_el;
            face_fc = mo_classes[i]->p_vis_data->face_fc;
            face_cnt = mo_classes[i]->p_vis_data->face_cnt;
            connects4 = (int (*)[4]) mo_classes[i]->objects.elems->nodes;

            for ( j = 0; j < face_cnt; j++ )
            {
                el = face_el[j];
                fc = face_fc[j];

                tet_face_avg_norm( el, fc, mo_classes[i], analy, vec );

                /* Test for backfacing face. */
                if ( VEC_DOT( vec, line_dir ) > 0 )
                    continue;

                for ( k = 0; k < 3; k++ )
                {
                    nd = connects4[el][ tet_fc_nd_nums[fc][k] ];
                    onsurf[nd] = 1.0;
                }
            }
        }
    }

    hide_mtl = p_mesh->hide_material;

    /* Mark nodes shared by quad elements. */

    mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_QUAD].list;
    class_qty = p_mesh->classes_by_sclass[G_QUAD].qty;

    for ( i = 0; i < class_qty; i++ )
    {
        face_cnt = mo_classes[i]->qty;
        connects4 = (int (*)[4]) mo_classes[i]->objects.elems->nodes;
        mat = mo_classes[i]->objects.elems->mat;

        for ( j = 0; j < face_cnt; j++ )
        {
            if ( hide_mtl[mat[j]] )
                continue;

            for ( k = 0; k < 4; k++ )
                onsurf[connects4[j][k]] = 1.0;
        }
    }

    /* Mark nodes shared by tri elements. */

    mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_TRI].list;
    class_qty = p_mesh->classes_by_sclass[G_TRI].qty;

    for ( i = 0; i < class_qty; i++ )
    {
        face_cnt = mo_classes[i]->qty;
        connects3 = (int (*)[3]) mo_classes[i]->objects.elems->nodes;
        mat = mo_classes[i]->objects.elems->mat;

        for ( j = 0; j < face_cnt; j++ )
        {
            if ( hide_mtl[mat[i]] )
                continue;

            for ( k = 0; k < 3; k++ )
                onsurf[connects3[j][k]] = 1.0;
        }
    }

    /* Mark nodes shared by beam elements. */

    mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_BEAM].list;
    class_qty = p_mesh->classes_by_sclass[G_BEAM].qty;

    for ( i = 0; i < class_qty; i++ )
    {
        face_cnt = mo_classes[i]->qty;
        connects3 = (int (*)[3]) mo_classes[i]->objects.elems->nodes;
        mat = mo_classes[i]->objects.elems->mat;

        for ( j = 0; j < face_cnt; j++ )
        {
            if ( hide_mtl[mat[i]] )
                continue;

            onsurf[connects3[j][0]] = 1.0;
            onsurf[connects3[j][1]] = 1.0;
        }
    }

    /* Mark nodes shared by truss elements. */

    mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[G_TRUSS].list;
    class_qty = p_mesh->classes_by_sclass[G_TRUSS].qty;

    for ( i = 0; i < class_qty; i++ )
    {
        face_cnt = mo_classes[i]->qty;
        connects2 = (int (*)[2]) mo_classes[i]->objects.elems->nodes;
        mat = mo_classes[i]->objects.elems->mat;

        for ( j = 0; j < face_cnt; j++ )
        {
            if ( hide_mtl[mat[i]] )
                continue;

            onsurf[connects2[j][0]] = 1.0;
            onsurf[connects2[j][1]] = 1.0;
        }
    }


    /* If free nodes are enabled - then allow user to
     * select them.
     */
    if ( ml_node && (analy->free_nodes_enabled ||  analy->particle_nodes_enabled) )
    {
        free_nodes = get_free_nodes( analy );
        if (free_nodes!=NULL)
        {
            for ( i = 0;
                    i < node_qty;
                    i++ )
                if (free_nodes[i] > 0 || analy->particle_nodes_enabled )
                    onsurf[i] = 1.0;
            free(free_nodes);
        }
    }


    /* Choose node whose distance from line is smallest. */

    if ( dims == 3 )
    {
        nodes3d = analy->state_p->nodes.nodes3d;
        onodes3d = (GVec3D *) analy->cur_ref_state_data;

        if ( analy->displace_exag )
        {
            for ( i = 0; i < node_qty; i++ )
            {
                if ( onsurf[i] == 1.0 )
                {
                    /* Include displacement scaling. */
                    for ( j = 0; j < 3; j++ )
                    {
                        orig = onodes3d[i][j];
                        pt[j] = orig + analy->displace_scale[j]
                                * (nodes3d[i][j] - orig);
                    }

                    near_pt_on_line( pt, line_pt, line_dir, near_pt );

                    VEC_SUB( vec, pt, near_pt );
                    dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                    if ( dist < near_dist )
                    {
                        near_num = i;
                        near_dist = dist;
                    }
                }
            }
        }
        else /* no displacement scaling */
        {
            for ( i = 0; i < node_qty; i++ )
            {
                if ( onsurf[i] == 1.0 )
                {
                    for ( j = 0; j < 3; j++ )
                        pt[j] = nodes3d[i][j];

                    near_pt_on_line( pt, line_pt, line_dir, near_pt );

                    VEC_SUB( vec, pt, near_pt );
                    dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                    if ( dist < near_dist )
                    {
                        near_num = i;
                        near_dist = dist;
                    }
                }
            }
        }
    }
    else /* 2D database */
    {
        nodes2d = analy->state_p->nodes.nodes2d;
        onodes2d = (GVec2D *) analy->cur_ref_state_data;

        if ( analy->displace_exag )
        {
            for ( i = 0; i < node_qty; i++ )
            {
                if ( onsurf[i] == 1.0 )
                {
                    /* Include displacement scaling. */
                    orig = onodes2d[i][0];
                    pt[0] = orig + analy->displace_scale[0]
                            * (nodes2d[i][0] - orig);
                    orig = onodes2d[i][0];
                    pt[1] = orig + analy->displace_scale[0]
                            * (nodes2d[i][0] - orig);

                    VEC_SUB_2D( vec, pt, line_pt );
                    dist = VEC_DOT_2D( vec, vec );     /* Skip the sqrt. */

                    if ( dist < near_dist )
                    {
                        near_num = i;
                        near_dist = dist;
                    }
                }
            }
        }
        else /* no displacement scaling */
        {
            for ( i = 0; i < node_qty; i++ )
            {
                if ( onsurf[i] == 1.0 )
                {
                    pt[0] = nodes2d[i][0];
                    pt[1] = nodes2d[i][1];

                    VEC_SUB_2D( vec, pt, line_pt );
                    dist = VEC_DOT_2D( vec, vec );     /* Skip the sqrt. */

                    if ( dist < near_dist )
                    {
                        near_num = i;
                        near_dist = dist;
                    }
                }
            }
        }
    }
    *p_near = near_num ;
}


/************************************************************
 * TAG( select_particle )
 *
 * Select a particle from a screen pick.
 */
static void
select_particle( Analysis *analy, Mesh_data *p_mesh,
                 MO_class_data *p_part_class, float line_pt[3],
                 float line_dir[3], int *p_near )
{
    float pt[3], near_pt[3], vec[3];
    float near_dist, dist;
    int part_qty;
    int i, j, near_num;
    GVec3D *particles3d;
    GVec2D *particles2d;
#ifdef HAVE_PART_INIT_POS
    GVec3D *oparticles3d;
    GVec2D *oparticles2d;
    float orig;
#endif
    int dims;

    part_qty = p_part_class->qty;

    /* Init to ensure update. */
    near_dist = MAXFLOAT;

    dims = analy->dimension;

    /* Choose node whose distance from line is smallest. */

    if ( dims == 3 )
    {
        particles3d = analy->state_p->particles.particles3d;

#ifdef HAVE_PART_INIT_POS
        /* When/if we store particle initial positions we can scale displacements. */
        oparticles3d = p_part_class->objects.particles3d;

        if ( analy->displace_exag )
        {
            for ( i = 0; i < part_qty; i++ )
            {
                /* Include displacement scaling. */
                for ( j = 0; j < 3; j++ )
                {
                    orig = oparticles3d[i][j];
                    pt[j] = orig + analy->displace_scale[j]
                            * (particles3d[i][j] - orig);
                }

                near_pt_on_line( pt, line_pt, line_dir, near_pt );

                VEC_SUB( vec, pt, near_pt );
                dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                if ( dist < near_dist )
                {
                    near_num = i;
                    near_dist = dist;
                }
            }
        }
        else /* no displacement scaling */
        {
#endif
            for ( i = 0; i < part_qty; i++ )
            {
                for ( j = 0; j < 3; j++ )
                    pt[j] = particles3d[i][j];

                near_pt_on_line( pt, line_pt, line_dir, near_pt );

                VEC_SUB( vec, pt, near_pt );
                dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

                if ( dist < near_dist )
                {
                    near_num = i;
                    near_dist = dist;
                }
            }
#ifdef HAVE_PART_INIT_POS
        }
#endif
    }
    else /* 2D database */
    {
        particles2d = analy->state_p->particles.particles2d;

#ifdef HAVE_PART_INIT_POS
        /* When/if we store particle initial positions we can scale displacements. */
        oparticles2d = p_part_class->objects.particles2d;

        if ( analy->displace_exag )
        {
            for ( i = 0; i < part_qty; i++ )
            {
                /* Include displacement scaling. */
                orig = oparticles2d[i][0];
                pt[0] = orig + analy->displace_scale[0]
                        * (particles2d[i][0] - orig);
                orig = oparticles2d[i][0];
                pt[1] = orig + analy->displace_scale[0]
                        * (particles2d[i][0] - orig);

                VEC_SUB_2D( vec, pt, line_pt );
                dist = VEC_DOT_2D( vec, vec );     /* Skip the sqrt. */

                if ( dist < near_dist )
                {
                    near_num = i;
                    near_dist = dist;
                }
            }
        }
        else /* no displacement scaling */
        {
#endif
            for ( i = 0; i < part_qty; i++ )
            {
                pt[0] = particles2d[i][0];
                pt[1] = particles2d[i][1];

                VEC_SUB_2D( vec, pt, line_pt );
                dist = VEC_DOT_2D( vec, vec );     /* Skip the sqrt. */

                if ( dist < near_dist )
                {
                    near_num = i;
                    near_dist = dist;
                }
            }
#ifdef HAVE_PART_INIT_POS
        }
#endif
    }

    *p_near = near_num ;
}


/************************************************************
 * TAG( select_meshless_elem )
 *
 * Select a meshless object element from a screen pick.
 */
void
select_meshless_elem( Analysis *analy, Mesh_data *p_mesh,
                      MO_class_data *p_ml_class, int near_node,
                      int *p_near )
{
    int ml_qty;
    int i, j, near_num;
    GVec3D *nodes3d;

    int num_ml, num_nodes, ml_node;

    int (*connects_ml)[8];
    int (*connects_particle)[1];

    MO_class_data *p_nodes;

    p_nodes = p_mesh->node_geom;

    num_ml    = p_ml_class->qty;
    num_nodes = p_nodes->qty;

    if ( p_ml_class->superclass==G_PARTICLE )
        connects_particle = (int (*)[1]) p_ml_class->objects.elems->nodes;
    else
        connects_ml = (int (*)[8]) p_ml_class->objects.elems->nodes;

    nodes3d     = analy->state_p->nodes.nodes3d;

    /* Choose node whose distance from line is smallest. */

    for ( i = 0;
            i < num_ml;
            i++ )
    {
        if ( p_ml_class->superclass==G_PARTICLE )
            ml_node = connects_particle[i][0];
        else
            ml_node = connects_ml[i][0];
        if ( ml_node == near_node-1 )
        {
            *p_near = i+1;
            return;
        }
    }

    *p_near = -1;
}

/************************************************************
 * TAG( select_meshless_node )
 *
 * Select a meshless object element from a screen pick.
 */
void
select_meshless_node( Analysis *analy, Mesh_data *p_mesh,
                      MO_class_data *p_ml_class, int elem_id,
                      int *p_near )
{
    int ml_qty;
    int i, j, near_num;
    GVec3D *nodes3d;

    int num_ml, num_nodes, ml_node;

    int (*connects_ml)[8];
    int (*connects_particle)[1];

    MO_class_data *p_nodes;

    p_nodes = p_mesh->node_geom;

    num_ml    = p_ml_class->qty;
    num_nodes = p_nodes->qty;

    if ( p_ml_class->superclass==G_PARTICLE )
        connects_particle = (int (*)[1]) p_ml_class->objects.elems->nodes;
    else
        connects_ml = (int (*)[8]) p_ml_class->objects.elems->nodes;
    nodes3d     = analy->state_p->nodes.nodes3d;

    /* Choose node whose distance from line is smallest. */

    if ( elem_id>=num_ml )
        *p_near = 0;
    else if ( p_ml_class->superclass==G_PARTICLE )
        *p_near = connects_particle[elem_id][0];
    else
        *p_near = connects_ml[elem_id][0];

    return;
}

#ifdef OLD_SELECT_ML
/************************************************************
 * TAG( select_meshless_point )
 *
 * Select a meshless object from a screen pick.
 */
static void
select_meshless( Analysis *analy, Mesh_data *p_mesh,
                 MO_class_data *p_ml_class, float line_pt[3],
                 float line_dir[3], int *p_near )
{
    float pt[3], pt_near[3], near_pt[3], vec[3];
    float near_dist, dist;
    int ml_qty;
    int i, j, near_num;
    GVec3D *nodes3d;

    int num_ml, num_nodes, ml_node;

    int (*connects_brick)[8];
    int (*connects_ml)[8];
    MO_class_data *p_ml, *p_nodes;

    p_ml    = p_mesh->p_ml_class;
    p_nodes = p_mesh->node_geom;

    num_ml    = p_ml->qty;
    num_nodes = p_nodes->qty;

    connects_ml = (int (*)[8]) p_ml->objects.elems->nodes;
    nodes3d     = analy->state_p->nodes.nodes3d;

    /* Init to ensure update. */
    near_dist = MAXFLOAT;

    /* Choose node whose distance from line is smallest. */

    for ( i = 0;
            i < num_ml;
            i++ )
    {
        ml_node = connects_ml[i][0];

        for ( j = 0; j < 3; j++ )
            pt[j] = nodes3d[ml_node][j];

        near_pt_on_line( pt, line_pt, line_dir, near_pt );

        VEC_SUB( vec, pt, near_pt );
        dist = VEC_DOT( vec, vec );     /* Skip the sqrt. */

        if ( dist < near_dist )
        {
            near_num = i;
            near_dist = dist;
            pt_near[0] = pt[0];
            pt_near[1] = pt[1];
            pt_near[2] = pt[2];
        }
    }
    *p_near = near_num + 1;
}
#endif

/************************************************************
 * TAG( get_mesh_edges )
 *
 * Generate the mesh edge list for currently displayed
 * bricks and shells.  Doesn't do real edge detection.
 */
void
get_mesh_edges( int mesh_id, Analysis *analy )
{
    int i, j;
    Mesh_data *p_md;
    static int eval_order[] =
    {
        G_TRI, G_QUAD, G_TET, G_PYRAMID, G_HEX
    };
    List_head *p_lh;
    Edge_list_obj *p_elo, *p_mesh_elo;
    Edge_list_obj *(*edge_func)();

    p_md = analy->mesh_table + mesh_id;

    /* Clean-up any existing edge list. */
    if ( p_md->edge_list != NULL )
    {
        free( p_md->edge_list->list );
        if ( p_md->edge_list->overflow != NULL )
            free( p_md->edge_list->overflow );
        free( p_md->edge_list );
        p_md->edge_list = NULL;
    }

    for ( j = 0; j < sizeof( eval_order ) / sizeof ( eval_order[0] ); j++ )
    {
        switch ( eval_order[j] )
        {
        case G_TRI:
            p_lh = p_md->classes_by_sclass + G_TRI;
            edge_func = get_tri_class_edges;
            break;
        case G_QUAD:
            p_lh = p_md->classes_by_sclass + G_QUAD;
            edge_func = get_quad_class_edges;
            break;
        case G_TET:
            p_lh = p_md->classes_by_sclass + G_TET;
            edge_func = get_tet_class_edges;
            break;
        case G_PYRAMID:
            p_lh = p_md->classes_by_sclass + G_PYRAMID;
            edge_func = get_pyramid_class_edges;
            break;
        case G_HEX:
            p_lh = p_md->classes_by_sclass + G_HEX;
            edge_func = get_hex_class_edges;
            break;
        }

        for ( i = 0; i < p_lh->qty; i++ )
        {
            /* Get edge list for this element class. */
            p_elo = edge_func( p_md->mtl_trans,
                               ((MO_class_data **) p_lh->list)[i], analy );

            if ( p_elo == NULL )
                continue;

            /* If first occurrence, save as edge list for mesh, else merge. */
            if ( p_md->edge_list == NULL )
            {
                /* Put new list into mesh. */
                p_md->edge_list = p_elo;
            }
            else
            {
                /* Merge with existing list. */
                p_mesh_elo = p_md->edge_list;
                p_md->edge_list = merge_and_free_edge_lists( p_mesh_elo,
                                  p_elo );
            }
        }
    }
}


/************************************************************
 * TAG( get_hex_class_edges )
 *
 * Generate the mesh edge list for the currently displayed
 * faces.  Looks for boundaries of material regions and
 * for crease edges which exceed a threshold angle.
 */
static Edge_list_obj *
get_hex_class_edges( float *mtl_trans[3], MO_class_data *p_hex_class,
                     Analysis *analy )
{
    float norm[3], norm_next[3], dot;
    int *edge_tbl[3];
    int *m_edges[2], *m_edge_mtl;
    int *ord;
    int *mat;
    int el, fc, el_next, fc_next, node1, node2, tmp;
    int fcnt, ecnt, m_edges_cnt;
    int i, j;
    Visibility_data *p_vd;
    int (*connects)[8];
    Edge_list_obj *p_elo;

    p_vd = p_hex_class->p_vis_data;
    connects = (int (*)[8]) p_hex_class->objects.elems->nodes;
    mat = p_hex_class->objects.elems->mat;

    if ( p_vd->face_cnt == 0 )
        return NULL;

    /* Create the edge table. */
    fcnt = p_vd->face_cnt;
    ecnt = fcnt*4;
    for ( i = 0; i < 3; i++ )
        edge_tbl[i] = NEW_N( int, ecnt, "Hex edge table" );

    /*
     * The edge_tbl contains for each edge
     *
     *     node1 node2 face
     */
    for ( i = 0; i < fcnt; i++ )
    {
        el = p_vd->face_el[i];
        fc = p_vd->face_fc[i];

        for ( j = 0; j < 4; j++ )
        {
            node1 = connects[el][ fc_nd_nums[fc][j] ];
            node2 = connects[el][ fc_nd_nums[fc][(j+1)%4] ];
            
            if(node1 == node2)
            {
                continue;
            }
            /* Order the node numbers in ascending order. */
            if ( node1 > node2 )
                SWAP( tmp, node1, node2 );

            edge_tbl[0][i*4 + j] = node1;
            edge_tbl[1][i*4 + j] = node2;
            edge_tbl[2][i*4 + j] = i;
        }
    }

    /* Sort the edge table. */
    ord = NEW_N( int, ecnt, "Sort ordering table" );
    edge_heapsort( ecnt, edge_tbl, ord, p_vd->face_el, mat, mtl_trans );

    /* Put boundary edges in the edge list. */
    m_edges[0] = NEW_N( int, ecnt, "Hex edges" );
    m_edges[1] = NEW_N( int, ecnt, "Hex edges" );
    m_edge_mtl = NEW_N( int, ecnt, "Hex edge materials" );
    m_edges_cnt = 0;

    /* For hex faces, all edges should be in the sorted list twice. */
    for ( i = 0; i < ecnt - 1; i++ )
    {
        j = edge_tbl[2][ord[i]];
        el = p_vd->face_el[j];
        fc = p_vd->face_fc[j];

        if ( edge_tbl[0][ord[i]] == edge_tbl[0][ord[i+1]]
                && edge_tbl[1][ord[i]] == edge_tbl[1][ord[i+1]] )
        {
            j = edge_tbl[2][ord[i+1]];
            el_next = p_vd->face_el[j];
            fc_next = p_vd->face_fc[j];

            if ( mat[el] != mat[el_next] )
            {
                /*
                 * Edge detected for material boundary. Put edge in for
                 * each material to cover material translations.
                 */
                m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                m_edge_mtl[m_edges_cnt] = mat[el];
                m_edges_cnt++;

                m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                m_edge_mtl[m_edges_cnt] = mat[el_next];
                m_edges_cnt++;
            }
            else
            {
                /* Get face normals for both faces. */
                hex_face_avg_norm( el, fc, p_hex_class, analy, norm );
                hex_face_avg_norm( el_next, fc_next, p_hex_class, analy,
                                   norm_next );
                vec_norm( norm );
                vec_norm( norm_next );

                /*
                 * Dot product test.
                 */
                dot = VEC_DOT( norm, norm_next );

                /*
                 * Magical constant.  An edge is detected when the angle
                 * between normals is greater than a critical angle.
                 */
                if ( dot < explicit_threshold )
                {
                    m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                    m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                    m_edge_mtl[m_edges_cnt] = mat[el];
                    m_edges_cnt++;
                }
            }

            /* Skip past the redundant edge. */
            i++;
        }
        else
            popup_dialog( WARNING_POPUP,
                          "Unexpected behavior in hex edge detection." );
    }

    /* Check for unused last edge. */
    if ( i < ecnt )
        popup_dialog( WARNING_POPUP,
                      "Unexpected behavior in hex edge detection." );

    for ( i = 0; i < 3; i++ )
        free( edge_tbl[i] );
    free( ord );

    /* Create Edge_list_obj for return. */
    p_elo = create_compressed_edge_list( m_edges, m_edge_mtl, m_edges_cnt );

    free( m_edges[0] );
    free( m_edges[1] );
    free( m_edge_mtl );

    return p_elo;
}


/************************************************************
 * TAG( get_pyramid_class_edges )
 *
 * Generate the mesh edge list for the currently displayed
 * faces.  Looks for boundaries of material regions and
 * for crease edges which exceed a threshold angle.
 */
static Edge_list_obj *
get_pyramid_class_edges( float *mtl_trans[3], MO_class_data *p_pyramid_class,
                         Analysis *analy )
{
    float norm[3], norm_next[3], dot;
    int *edge_tbl[3];
    int *m_edges[2], *m_edge_mtl;
    int *ord;
    int *mat;
    int el, fc, el_next, fc_next, node1, node2, tmp;
    int fcnt, ecnt, m_edges_cnt;
    int i, j;
    Visibility_data *p_vd;
    int (*connects)[5];
    Edge_list_obj *p_elo;

    p_vd = p_pyramid_class->p_vis_data;
    connects = (int (*)[5]) p_pyramid_class->objects.elems->nodes;
    mat = p_pyramid_class->objects.elems->mat;

    if ( p_vd->face_cnt == 0 )
        return NULL;

    /* Create the edge table. */
    fcnt = p_vd->face_cnt;
    /* We need to treat the edge count as a hex with one degenerate face */
    ecnt = fcnt*4;
    for ( i = 0; i < 3; i++ )
        edge_tbl[i] = NEW_N( int, ecnt, "Pyramid edge table" );

    /*
     * The edge_tbl contains for each edge
     *
     *     node1 node2 face
     */
    for ( i = 0; i < fcnt; i++ )
    {
        el = p_vd->face_el[i];
        fc = p_vd->face_fc[i];

        for ( j = 0; j < 4; j++ )
        {
            node1 = connects[el][ pyramid_fc_nd_nums[fc][j] ];
            node2 = connects[el][ pyramid_fc_nd_nums[fc][(j+1)%4] ];

            /* Order the node numbers in ascending order. */
            if ( node1 > node2 )
                SWAP( tmp, node1, node2 );

            edge_tbl[0][i*4 + j] = node1;
            edge_tbl[1][i*4 + j] = node2;
            edge_tbl[2][i*4 + j] = i;
        }
    }

    /* Sort the edge table. */
    ord = NEW_N( int, ecnt, "Sort ordering table" );
    edge_heapsort( ecnt, edge_tbl, ord, p_vd->face_el, mat, mtl_trans );

    /* Put boundary edges in the edge list. */
    m_edges[0] = NEW_N( int, ecnt, "Pyramid edges" );
    m_edges[1] = NEW_N( int, ecnt, "Pyramid edges" );
    m_edge_mtl = NEW_N( int, ecnt, "Pyramid edge materials" );
    m_edges_cnt = 0;

    /* For pyramid faces, all edges should be in the sorted list twice. */
    for ( i = 0; i < ecnt - 1; i++ )
    {
        j = edge_tbl[2][ord[i]];
        el = p_vd->face_el[j];
        fc = p_vd->face_fc[j];

        if ( edge_tbl[0][ord[i]] == edge_tbl[0][ord[i+1]]
                && edge_tbl[1][ord[i]] == edge_tbl[1][ord[i+1]] )
        {
            j = edge_tbl[2][ord[i+1]];
            el_next = p_vd->face_el[j];
            fc_next = p_vd->face_fc[j];

            if ( mat[el] != mat[el_next] )
            {
                /*
                 * Edge detected for material boundary. Put edge in for
                 * each material to cover material translations.
                 */
                m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                m_edge_mtl[m_edges_cnt] = mat[el];
                m_edges_cnt++;

                m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                m_edge_mtl[m_edges_cnt] = mat[el_next];
                m_edges_cnt++;
            }
            else
            {
                /* Get face normals for both faces. */
                pyramid_face_avg_norm( el, fc, p_pyramid_class, analy, norm );
                pyramid_face_avg_norm( el_next, fc_next, p_pyramid_class, analy,
                                       norm_next );
                vec_norm( norm );
                vec_norm( norm_next );

                /*
                 * Dot product test.
                 */
                dot = VEC_DOT( norm, norm_next );

                /*
                 * Magical constant.  An edge is detected when the angle
                 * between normals is greater than a critical angle.
                 */
                if ( dot < explicit_threshold )
                {
                    m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                    m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                    m_edge_mtl[m_edges_cnt] = mat[el];
                    m_edges_cnt++;
                }
            }

            /* Skip past the redundant edge. */
            i++;
        }
        else
            popup_dialog( WARNING_POPUP,
                          "Unexpected behavior in pyramid edge detection." );
    }

    /* Check for unused last edge. */
    if ( i < ecnt )
        popup_dialog( WARNING_POPUP,
                      "Unexpected behavior in pyramid edge detection." );

    for ( i = 0; i < 3; i++ )
        free( edge_tbl[i] );
    free( ord );

    /* Create Edge_list_obj for return. */
    p_elo = create_compressed_edge_list( m_edges, m_edge_mtl, m_edges_cnt );

    free( m_edges[0] );
    free( m_edges[1] );
    free( m_edge_mtl );

    return p_elo;
}


/************************************************************
 * TAG( get_tet_class_edges )
 *
 * Generate the mesh edge list for the currently displayed
 * faces.  Looks for boundaries of material regions and
 * for crease edges which exceed a threshold angle.
 */
static Edge_list_obj *
get_tet_class_edges( float *mtl_trans[3], MO_class_data *p_tet_class,
                     Analysis *analy )
{
    float norm[3], norm_next[3], dot;
    int *edge_tbl[3];
    int *m_edges[2], *m_edge_mtl;
    int *ord;
    int *mat;
    int el, fc, el_next, fc_next, node1, node2, tmp;
    int fcnt, ecnt, m_edges_cnt;
    int i, j;
    Visibility_data *p_vd;
    int (*connects)[4];
    Edge_list_obj *p_elo;

    p_vd = p_tet_class->p_vis_data;
    connects = (int (*)[4]) p_tet_class->objects.elems->nodes;
    mat = p_tet_class->objects.elems->mat;

    if ( p_vd->face_cnt == 0 )
        return NULL;

    /* Create the edge table. */
    fcnt = p_vd->face_cnt;
    ecnt = fcnt*3;
    for ( i = 0; i < 3; i++ )
        edge_tbl[i] = NEW_N( int, ecnt, "Tet edge table" );

    /*
     * The edge_tbl contains for each edge
     *
     *     node1 node2 face
     */
    for ( i = 0; i < fcnt; i++ )
    {
        el = p_vd->face_el[i];
        fc = p_vd->face_fc[i];

        for ( j = 0; j < 3; j++ )
        {
            node1 = connects[el][ tet_fc_nd_nums[fc][j] ];
            node2 = connects[el][ tet_fc_nd_nums[fc][(j+1)%3] ];

            /* Order the node numbers in ascending order. */
            if ( node1 > node2 )
                SWAP( tmp, node1, node2 );

            edge_tbl[0][i*3 + j] = node1;
            edge_tbl[1][i*3 + j] = node2;
            edge_tbl[2][i*3 + j] = i;
        }
    }

    /* Sort the edge table. */
    ord = NEW_N( int, ecnt, "Sort ordering table" );
    edge_heapsort( ecnt, edge_tbl, ord, p_vd->face_el, mat, mtl_trans );

    /* Put boundary edges in the edge list. */
    m_edges[0] = NEW_N( int, ecnt, "Tet edges" );
    m_edges[1] = NEW_N( int, ecnt, "Tet edges" );
    m_edge_mtl = NEW_N( int, ecnt, "Tet edge materials" );
    m_edges_cnt = 0;

    /* For tet faces, all edges should be in the sorted list twice. */
    for ( i = 0; i < ecnt - 1; i++ )
    {
        j = edge_tbl[2][ord[i]];
        el = p_vd->face_el[j];
        fc = p_vd->face_fc[j];

        if ( edge_tbl[0][ord[i]] == edge_tbl[0][ord[i+1]]
                && edge_tbl[1][ord[i]] == edge_tbl[1][ord[i+1]] )
        {
            j = edge_tbl[2][ord[i+1]];
            el_next = p_vd->face_el[j];
            fc_next = p_vd->face_fc[j];

            if ( mat[el] != mat[el_next] )
            {
                /*
                 * Edge detected for material boundary. Put edge in for
                 * each material to cover material translations.
                 */
                m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                m_edge_mtl[m_edges_cnt] = mat[el];
                m_edges_cnt++;

                m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                m_edge_mtl[m_edges_cnt] = mat[el_next];
                m_edges_cnt++;
            }
            else
            {
                /* Get face normals for both faces. */
                tet_face_avg_norm( el, fc, p_tet_class, analy, norm );
                tet_face_avg_norm( el_next, fc_next, p_tet_class, analy,
                                   norm_next );
                vec_norm( norm );
                vec_norm( norm_next );

                /*
                 * Dot product test.
                 */
                dot = VEC_DOT( norm, norm_next );

                /*
                 * Magical constant.  An edge is detected when the angle
                 * between normals is greater than a critical angle.
                 */
                if ( dot < explicit_threshold )
                {
                    m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                    m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                    m_edge_mtl[m_edges_cnt] = mat[el];
                    m_edges_cnt++;
                }
            }

            /* Skip past the redundant edge. */
            i++;
        }
        else
            popup_dialog( WARNING_POPUP,
                          "Unexpected behavior in tet edge detection." );
    }

    /* Check for unused last edge. */
    if ( i < ecnt )
        popup_dialog( WARNING_POPUP,
                      "Unexpected behavior in tet edge detection." );

    for ( i = 0; i < 3; i++ )
        free( edge_tbl[i] );
    free( ord );

    /* Create Edge_list_obj for return. */
    p_elo = create_compressed_edge_list( m_edges, m_edge_mtl, m_edges_cnt );

    free( m_edges[0] );
    free( m_edges[1] );
    free( m_edge_mtl );

    return p_elo;
}


/************************************************************
 * TAG( create_compressed_edge_list )
 *
 * Build a non-redundant edge list object from arrayed edge data.
 */
static Edge_list_obj *
create_compressed_edge_list( int *enodes[2], int *emtls, int eqty )
{
    Edge_list_obj *p_elo;
    Edge_obj *p_eo;
    int i, j, qty, oqty;

    /* Sanity check. */
    if ( eqty == 0 )
        return NULL;

    p_elo = NEW( Edge_list_obj, "New edge list" );
    p_eo = NEW_N( Edge_obj, eqty, "List edge objects" );

    /* Initialize first entry. */
    p_eo[0].node1 = enodes[0][0];
    p_eo[0].node2 = enodes[1][0];
    p_eo[0].mtl = emtls[0];
    p_eo[0].addl_mtl = -1;

    for ( i = 1, qty = 1; i < eqty; i++ )
    {
        j = qty - 1;

        /* If current and previous edges are different... */
        if ( p_eo[j].node1 ^ enodes[0][i] | p_eo[j].node2 ^ enodes[1][i] )
        {
            /* Different - add current edge. */
            p_eo[qty].node1 = enodes[0][i];
            p_eo[qty].node2 = enodes[1][i];
            p_eo[qty].mtl = emtls[i];
            p_eo[qty].addl_mtl = -1;

            qty++;
        }
        else
        {
            /*
             * Same - queue a reference to current edge mtl in previous edge.
             *
             * Don't sweat multiple references to same material; shouldn't
             * be common, and can be dealt with during rendering.
             */

            oqty = p_elo->overflow_qty;
            p_elo->overflow = RENEWC_N( Int_2tuple, p_elo->overflow, oqty, 1,
                                        "Edge overflow entry" );
            if(p_elo->overflow == NULL)
            {
                popup_dialog(WARNING_POPUP, "out of memory in function create_compressed_edge_list.\n");
                parse_command("quit", env.curr_analy);
            }
            p_elo->overflow[oqty][0] = emtls[i];
            p_elo->overflow[oqty][1] = p_eo[j].addl_mtl;
            p_eo[j].addl_mtl = oqty;
            p_elo->overflow_qty++;
        }
    }

    /*
     * Want to let memory manager have excess back so we don't have to
     * manage it.
     */
    p_eo = RENEW_N( Edge_obj, p_eo, eqty, qty - eqty, "Free excess edge objs" );

    p_elo->list = p_eo;
    p_elo->size = qty;

    return p_elo;
}


/************************************************************
 * TAG( merge_and_free_edge_lists )
 *
 * Merge two sorted edge lists to create a sorted new list.
 *
 * Fractionally better performance if first list has largest overflow
 * queue.
 */
static Edge_list_obj *
merge_and_free_edge_lists( Edge_list_obj *p_elo1, Edge_list_obj *p_elo2 )
{
    Edge_list_obj *p_elo;
    Edge_obj *p_eo1, *p_eo2, *p_eo, *bound1, *bound2;
    int qty, oqty, offset;
    Int_2tuple *p_i2t, *src_q, *add_q, *add_bound;

    p_elo = NEW( Edge_list_obj, "Merged edge list obj" );
    p_elo->list = NEW_N( Edge_obj, p_elo1->size + p_elo2->size,
                         "Merged edge list" );

    /*
     * Let edge list 1 provide the "source" overflow list (if extant) and
     * edge list 2 provide the "add" overflow list.  Since overflow for the
     * merged list will be a superset of the overflow lists of the input
     * edge lists, start with overflow list 1, extend it, and copy overflow
     * list 2 into the extension, modifying the index fields (which aren't -1)
     * by the size of overflow list 1 to reflect their indices in the
     * newly extended list.
     */
    offset = 0;
    if ( p_elo1->overflow == NULL )
    {
        if ( p_elo2->overflow != NULL )
        {
            p_elo->overflow = p_elo2->overflow;
            p_elo->overflow_qty = p_elo2->overflow_qty;
            offset = 0;

            p_elo2->overflow = NULL;
            p_elo2->overflow_qty = 0;
        }
    }
    else
    {
        p_elo->overflow = p_elo1->overflow;
        p_elo->overflow_qty = p_elo1->overflow_qty;
        offset = 0;

        if ( p_elo2->overflow != NULL )
        {
            /* Both overflows non-NULL, so extend 1 and copy 2... */
            p_elo->overflow = RENEW_N( Int_2tuple, p_elo1->overflow,
                                       p_elo1->overflow_qty,
                                       p_elo2->overflow_qty,
                                       "Merged overflow list" );
            if(p_elo->overflow == NULL)
            {
                popup_dialog(WARNING_POPUP, "out of memory in function merge_and_free_edge_lists.");
                parse_command("quit", env.curr_analy);
                exit(0);
            }
            offset = p_elo->overflow_qty;
            src_q = p_elo->overflow + offset;
            add_bound = p_elo2->overflow + p_elo2->overflow_qty;
            for ( add_q = p_elo2->overflow; add_q < add_bound; add_q++ )
            {
                (*src_q)[0] = (*add_q)[0];
                (*src_q)[1] = ( (*add_q)[1] != -1 ) ? (*add_q)[1] + offset : -1;
                src_q++;
            }

            p_elo->overflow_qty += p_elo2->overflow_qty;
        }
    }

    /* Traverse and merge the two lists. */

    p_eo1 = p_elo1->list;
    bound1 = p_eo1 + p_elo1->size;
    p_eo2 = p_elo2->list;
    bound2 = p_eo2 + p_elo2->size;

    p_eo = p_elo->list;

    qty = 0;

    while ( p_eo1 < bound1 && p_eo2 < bound2 )
    {
        /* Merge sort first on node1, then on node2. */

        if ( p_eo1->node1 < p_eo2->node1 )
        {
            *p_eo = *p_eo1;
            p_eo1++;
        }
        else if ( p_eo2->node1 < p_eo1->node1 )
        {
            *p_eo = *p_eo2;
            if ( p_eo->addl_mtl != -1 )
                p_eo->addl_mtl += offset;
            p_eo2++;
        }
        else
        {
            /* Nodes 1 are equal, look at nodes 2. */

            if ( p_eo1->node2 < p_eo2->node2 )
            {
                *p_eo = *p_eo1;
                p_eo1++;
            }
            else if ( p_eo2->node2 < p_eo1->node2 )
            {
                *p_eo = *p_eo2;
                if ( p_eo->addl_mtl != -1 )
                    p_eo->addl_mtl += offset;
                p_eo2++;
            }
            else
            {
                /*
                 * Edge match.
                 *
                 * Deal with four overflow cases: (1) neither has overflow;
                 * (2) & (3) one or the other but not both has overflow;
                 * (4) both have overflow.
                 */

                /* Always need one new overflow entry. */
                oqty = p_elo->overflow_qty;
                p_elo->overflow = RENEW_N( Int_2tuple, p_elo->overflow, oqty, 1,
                                           "Edge overflow entry" );

                if(p_elo->overflow == NULL)
                {
                    popup_dialog(WARNING_POPUP, "out of memory in function merge_and_free_edge_lists");
                    parse_command("quit", env.curr_analy);
                    exit(0);
                }
                src_q = p_elo->overflow;

                if ( p_eo1->addl_mtl == -1 && p_eo2->addl_mtl == -1 )
                {
                    /* Add 2 as overflow to 1. */
                    *p_eo = *p_eo1;
                    p_eo->addl_mtl = oqty;
                    src_q[oqty][0] = p_eo2->mtl;
                    src_q[oqty][1] = -1;
                }
                else if ( p_eo1->addl_mtl == -1 && p_eo2->addl_mtl != -1 )
                {
                    /* Enqueue 1 as additional overflow to 2. */
                    *p_eo = *p_eo2;
                    src_q[oqty][0] = p_eo1->mtl;
                    src_q[oqty][1] = p_eo2->addl_mtl + offset;
                    p_eo->addl_mtl = oqty;
                }
                else if ( p_eo1->addl_mtl != -1 && p_eo2->addl_mtl == -1 )
                {
                    /* Enqueue 2 as additional overflow to 1. */
                    *p_eo = *p_eo1;
                    src_q[oqty][0] = p_eo2->mtl;
                    src_q[oqty][1] = p_eo1->addl_mtl;
                    p_eo->addl_mtl = oqty;
                }
                else
                {
                    /*
                     * Enqueue 2 as additional overflow to _2_, the enqueue
                     * 2's overflow as additional overflow to 1.
                     */
                    src_q[oqty][0] = p_eo2->mtl;
                    src_q[oqty][1] = p_eo2->addl_mtl + offset;

                    /* Find end of 2 queue. */
                    for ( p_i2t = src_q + src_q[oqty][1];
                            (*p_i2t)[1] != -1; p_i2t = src_q + (*p_i2t)[1] );

                    *p_eo = *p_eo1;
                    (*p_i2t)[1] = p_eo->addl_mtl;
                    p_eo->addl_mtl = oqty;
                }

                p_eo1++;
                p_eo2++;

                p_elo->overflow_qty++;
            }
        }

        qty++;
        p_eo++;
    }

    /*
     * Finish any leftover (at least one of p_eo1 or p_eo2 will be
     * maxed out, so no merge required).
     */

    for ( ; p_eo1 < bound1; *p_eo++ = *p_eo1++ )
    {
        qty++;
    }

    for ( ; p_eo2 < bound2; p_eo2++ )
    {
        *p_eo = *p_eo2;
        if ( p_eo->addl_mtl != -1 )
            p_eo->addl_mtl += offset;
        p_eo++;
        qty++;
    }

    p_elo->size = qty;

    if ( p_elo2->overflow != NULL )
        free( p_elo2->overflow );
    free( p_elo2->list );
    free( p_elo2 );

    free( p_elo1->list );
    free( p_elo1 );

    return p_elo;
}


#ifdef OLD_GET_EDGES
/************************************************************
 * TAG( old_get_brick_edges )
 *
 * Generate the mesh edge list for the currently displayed
 * faces.  This is sort of a rough way of picking out some
 * of the edges of hex elements.  It doesn't do real edge
 * detection.
 */
static void
old_get_brick_edges( Analysis *analy )
{
    Hex_geom *bricks;
    int **hex_adj;
    int *hex_visib;
    int nd, vis[6], vis_cnt, edge_cnt;
    int m1, m2;
    int i, j;

    bricks = analy->geom_p->bricks;
    hex_adj = analy->hex_adj;
    hex_visib = analy->hex_visib;
    edge_cnt = 0;

    /*
     * Loop through the brick elements.  At each brick, check whether
     * each of its faces is visible.  Then go through the edges:  if
     * the faces on both sides of an edge are visible, then the edge is
     * added to the edge list for display.  The first pass just counts
     * edges; the second pass actually stores them.
     */
    for ( i = 0; i < bricks->cnt; i++ )
    {
        if ( hex_visib[i] )
        {
            vis_cnt = 0;
            for ( j = 0; j < 6; j++ )
            {
                if ( hex_adj[j][i] == -1 || (hex_adj[j][i] >= 0 &&!hex_visib[ hex_adj[j][i] ] ))
                {
                    vis[j] = TRUE;
                    vis_cnt++;
                }
                else if ( analy->translate_material )
                {
                    /* Material translations can expose faces. */
                    m1 = bricks->mat[i];
                    m2 = bricks->mat[ hex_adj[j][i] ];
                    if ( analy->mtl_trans[0][m1] != analy->mtl_trans[0][m2] ||
                            analy->mtl_trans[1][m1] != analy->mtl_trans[1][m2] ||
                            analy->mtl_trans[2][m1] != analy->mtl_trans[2][m2] )
                    {
                        vis[j] = TRUE;
                        vis_cnt++;
                    }
                    else
                        vis[j] = FALSE;
                }
                else
                    vis[j] = FALSE;
            }

            if ( vis_cnt > 1 )
                for ( j = 0; j < 12; j++ )
                    if ( vis[ edge_face_nums[j][0] ] &&
                            vis[ edge_face_nums[j][1] ] )
                        edge_cnt++;
        }
    }

    analy->m_edges_cnt = edge_cnt;
    analy->m_edges[0] = NEW_N( int, edge_cnt, "Mesh edges" );
    analy->m_edges[1] = NEW_N( int, edge_cnt, "Mesh edges" );
    analy->m_edge_mtl = NEW_N( int, edge_cnt, "Mesh edge material" );
    edge_cnt = 0;

    for ( i = 0; i < bricks->cnt; i++ )
    {
        if ( hex_visib[i] )
        {
            vis_cnt = 0;
            for ( j = 0; j < 6; j++ )
            {
                if ( hex_adj[j][i] == -1 || (hex_adj[j][i] >= 0 && !hex_visib[ hex_adj[j][i] ] ))
                {
                    vis[j] = TRUE;
                    vis_cnt++;
                }
                else if ( analy->translate_material )
                {
                    /* Material translations can expose faces. */
                    m1 = bricks->mat[i];
                    m2 = bricks->mat[ hex_adj[j][i] ];
                    if ( analy->mtl_trans[0][m1] != analy->mtl_trans[0][m2] ||
                            analy->mtl_trans[1][m1] != analy->mtl_trans[1][m2] ||
                            analy->mtl_trans[2][m1] != analy->mtl_trans[2][m2] )
                    {
                        vis[j] = TRUE;
                        vis_cnt++;
                    }
                    else
                        vis[j] = FALSE;
                }
                else
                    vis[j] = FALSE;
            }

            if ( vis_cnt > 1 )
                for ( j = 0; j < 12; j++ )
                    if ( vis[ edge_face_nums[j][0] ] &&
                            vis[ edge_face_nums[j][1] ] )
                    {
                        analy->m_edge_mtl[edge_cnt] = bricks->mat[i];
                        nd = bricks->nodes[ edge_node_nums[j][0] ][i];
                        analy->m_edges[0][edge_cnt] = nd;
                        nd = bricks->nodes[ edge_node_nums[j][1] ][i];
                        analy->m_edges[1][edge_cnt] = nd;
                        edge_cnt++;
                    }
        }
    }
}
#endif


/************************************************************
 * TAG( get_quad_class_edges )
 *
 * Generate the edge list for quad elements.  Looks for the
 * boundaries of the quad regions, and adds those edges to
 * the list.
 */
static Edge_list_obj *
get_quad_class_edges( float *mtl_trans[3], MO_class_data *p_quad_class,
                      Analysis *analy )
{
    Bool_type vis_this, vis_next, three_d;
    float *activity;
    float norm_this[3], norm_next[3], dot;
    int *edge_tbl[3];
    int *m_edges[2], *m_edge_mtl;
    int *ord;
    int *mat;
    int node1, node2, tmp, el_this, el_next;
    int i, j, scnt, ecnt, m_edges_cnt;
    int (*connects)[4];
    int ec_idx;
    unsigned char *hide_material;
    Edge_list_obj *p_elo;

    connects = (int (*)[4]) p_quad_class->objects.elems->nodes;
    mat = p_quad_class->objects.elems->mat;
    ec_idx = p_quad_class->elem_class_index;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[ec_idx] : NULL;
    hide_material = analy->mesh_table[p_quad_class->mesh_id].hide_material;
    three_d = ( analy->dimension == 3 );

    /* Create the edge table. */
    scnt = p_quad_class->qty;
    ecnt = scnt*4;
    for ( i = 0; i < 3; i++ )
        edge_tbl[i] = NEW_N( int, ecnt, "Quad edge table" );

    /*
     * The edge_tbl contains for each edge
     *     node1 node2 element
     */
    for ( i = 0; i < scnt; i++ )
    {
        for ( j = 0; j < 4; j++ )
        {
            node1 = connects[i][j];
            node2 = connects[i][(j+1)%4];

            /* Order the node numbers in ascending order. */
            if ( node1 > node2 )
                SWAP( tmp, node1, node2 );

            edge_tbl[0][i*4 + j] = node1;
            edge_tbl[1][i*4 + j] = node2;
            edge_tbl[2][i*4 + j] = i;
        }
    }

    /* Sort the edge table. */
    ord = NEW_N( int, ecnt, "Sort ordering table" );
    edge_heapsort( ecnt, edge_tbl, ord, NULL, mat, mtl_trans );

    /* Put boundary edges in the edge list. */
    m_edges[0] = NEW_N( int, ecnt, "Quad edges" );
    m_edges[1] = NEW_N( int, ecnt, "Quad edges" );
    m_edge_mtl = NEW_N( int, ecnt, "Quad edge materials" );
    m_edges_cnt = 0;

    for ( i = 0; i < ecnt - 1; i++ )
    {
        el_this = edge_tbl[2][ord[i]];
        vis_this = TRUE;
        if ( ( activity && activity[el_this] == 0.0 )
                || hide_material[mat[el_this]] )
            vis_this = FALSE;

        if ( edge_tbl[0][ord[i]] == edge_tbl[0][ord[i+1]]
                && edge_tbl[1][ord[i]] == edge_tbl[1][ord[i+1]] )
        {
            el_next = edge_tbl[2][ord[i+1]];
            vis_next = TRUE;
            if ( ( activity && activity[el_next] == 0.0 )
                    || hide_material[mat[el_next]] )
                vis_next = FALSE;

            if ( mat[el_this] != mat[el_next] )
            {
                /* Different mtls generate an edge for each visible quad. */
                if ( vis_this )
                {
                    m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                    m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                    m_edge_mtl[m_edges_cnt] = mat[el_this];
                    m_edges_cnt++;
                }
                if ( vis_next )
                {
                    m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                    m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                    m_edge_mtl[m_edges_cnt] = mat[el_next];
                    m_edges_cnt++;
                }
            }
            else if ( vis_this && !vis_next )
            {
                /* One quad invisible generates an edge. */
                m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                m_edge_mtl[m_edges_cnt] = mat[el_this];
                m_edges_cnt++;
            }
            else if ( !vis_this && vis_next )
            {
                /* One quad invisible generates an edge. */
                m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                m_edge_mtl[m_edges_cnt] = mat[el_next];
                m_edges_cnt++;
            }
            else if ( vis_this && vis_next && three_d )
            {
                /* Get face normals for both faces. */
                quad_avg_norm( el_this, p_quad_class, analy, norm_this );
                quad_avg_norm( el_next, p_quad_class, analy, norm_next );
                vec_norm( norm_this );
                vec_norm( norm_next );

                /*
                 * Dot product test.
                 */
                dot = VEC_DOT( norm_this, norm_next );

                /*
                 * Magical constant.  An edge is detected when the angle
                 * between normals is greater than a critical angle.
                         *
                         * Need absolute value of dot product for shells because
                         * normals can be parallel but opposite.
                 */
                if ( fabs( dot ) < explicit_threshold )
                {
                    m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                    m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                    m_edge_mtl[m_edges_cnt] = mat[el_this];
                    m_edges_cnt++;
                }
            }

            /* Skip past the redundant edge. */
            i++;
        }
        else if ( vis_this )
        {
            m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
            m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
            m_edge_mtl[m_edges_cnt] = mat[el_this];
            m_edges_cnt++;
        }
    }

    /* Pick up the last edge. */
    if ( i < ecnt )
    {
        el_this = edge_tbl[2][ord[i]];

        if ( !( (activity && activity[edge_tbl[2][ord[i]]] == 0.0)
                || hide_material[mat[el_this]] ) )
        {
            m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
            m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
            m_edge_mtl[m_edges_cnt] = mat[ edge_tbl[2][ord[i]] ];
            m_edges_cnt++;
        }
    }

    for ( i = 0; i < 3; i++ )
        free( edge_tbl[i] );
    free( ord );

    /* Create Edge_list_obj for return. */
    p_elo = create_compressed_edge_list( m_edges, m_edge_mtl, m_edges_cnt );

    free( m_edges[0] );
    free( m_edges[1] );
    free( m_edge_mtl );

    return p_elo;
}


/************************************************************
 * TAG( get_tri_class_edges )
 *
 * Generate the edge list for tri elements.  Looks for the
 * boundaries of the tri regions, and adds those edges to
 * the list.
 */
static Edge_list_obj *
get_tri_class_edges( float *mtl_trans[3], MO_class_data *p_tri_class,
                     Analysis *analy )
{
    Bool_type vis_this, vis_next, three_d;
    float *activity;
    float norm_this[3], norm_next[3], dot;
    int *edge_tbl[3];
    int *m_edges[2], *m_edge_mtl;
    int *ord;
    int *mat;
    int node1, node2, tmp, el_this, el_next;
    int i, j, scnt, ecnt, m_edges_cnt;
    int (*connects)[3];
    int ec_idx;
    unsigned char *hide_material;
    Edge_list_obj *p_elo;

    connects = (int (*)[3]) p_tri_class->objects.elems->nodes;
    mat = p_tri_class->objects.elems->mat;
    ec_idx = p_tri_class->elem_class_index;
    activity = analy->state_p->sand_present
               ? analy->state_p->elem_class_sand[ec_idx] : NULL;
    hide_material = analy->mesh_table[p_tri_class->mesh_id].hide_material;
    three_d = ( analy->dimension == 3 );

    /* Create the edge table. */
    scnt = p_tri_class->qty;
    ecnt = scnt*3;
    for ( i = 0; i < 3; i++ )
        edge_tbl[i] = NEW_N( int, ecnt, "Tri edge table" );

    /*
     * The edge_tbl contains for each edge
     *     node1 node2 element
     */
    for ( i = 0; i < scnt; i++ )
    {
        for ( j = 0; j < 3; j++ )
        {
            node1 = connects[i][j];
            node2 = connects[i][(j+1)%3];

            /* Order the node numbers in ascending order. */
            if ( node1 > node2 )
                SWAP( tmp, node1, node2 );

            edge_tbl[0][i*3 + j] = node1;
            edge_tbl[1][i*3 + j] = node2;
            edge_tbl[2][i*3 + j] = i;
        }
    }

    /* Sort the edge table. */
    ord = NEW_N( int, ecnt, "Sort ordering table" );
    edge_heapsort( ecnt, edge_tbl, ord, NULL, mat, mtl_trans );

    /* Put boundary edges in the edge list. */
    m_edges[0] = NEW_N( int, ecnt, "Tri edges" );
    m_edges[1] = NEW_N( int, ecnt, "Tri edges" );
    m_edge_mtl = NEW_N( int, ecnt, "Tri edge materials" );
    m_edges_cnt = 0;

    for ( i = 0; i < ecnt - 1; i++ )
    {
        el_this = edge_tbl[2][ord[i]];
        vis_this = TRUE;
        if ( ( activity && activity[el_this] == 0.0 )
                || hide_material[mat[el_this]] )
            vis_this = FALSE;

        if ( edge_tbl[0][ord[i]] == edge_tbl[0][ord[i+1]]
                && edge_tbl[1][ord[i]] == edge_tbl[1][ord[i+1]] )
        {
            el_next = edge_tbl[2][ord[i+1]];
            vis_next = TRUE;
            if ( ( activity && activity[el_next] == 0.0 )
                    || hide_material[mat[el_next]] )
                vis_next = FALSE;

            if ( mat[el_this] != mat[el_next] )
            {
                /* Different mtls generate an edge for each visible tri. */
                if ( vis_this )
                {
                    m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                    m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                    m_edge_mtl[m_edges_cnt] = mat[el_this];
                    m_edges_cnt++;
                }
                if ( vis_next )
                {
                    m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                    m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                    m_edge_mtl[m_edges_cnt] = mat[el_next];
                    m_edges_cnt++;
                }
            }
            else if ( vis_this && !vis_next )
            {
                /* One tri invisible generates an edge. */
                m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                m_edge_mtl[m_edges_cnt] = mat[el_this];
                m_edges_cnt++;
            }
            else if ( !vis_this && vis_next )
            {
                /* One tri invisible generates an edge. */
                m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                m_edge_mtl[m_edges_cnt] = mat[el_next];
                m_edges_cnt++;
            }
            else if ( vis_this && vis_next && three_d )
            {
                /* Get face normals for both faces. */
                tri_avg_norm( el_this, p_tri_class, analy, norm_this );
                tri_avg_norm( el_next, p_tri_class, analy, norm_next );
                vec_norm( norm_this );
                vec_norm( norm_next );

                /*
                 * Dot product test.
                 */
                dot = VEC_DOT( norm_this, norm_next );

                /*
                 * Magical constant.  An edge is detected when the angle
                 * between normals is greater than a critical angle.
                         *
                         * Need absolute value of dot product for shells because
                         * normals can be parallel but opposite.
                 */
                if ( fabs( dot ) < explicit_threshold )
                {
                    m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
                    m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
                    m_edge_mtl[m_edges_cnt] = mat[el_this];
                    m_edges_cnt++;
                }
            }

            /* Skip past the redundant edge. */
            i++;
        }
        else if ( vis_this )
        {
            m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
            m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
            m_edge_mtl[m_edges_cnt] = mat[el_this];
            m_edges_cnt++;
        }
    }

    /* Pick up the last edge. */
    if ( i < ecnt )
    {
        el_this = edge_tbl[2][ord[i]];

        if ( !( (activity && activity[edge_tbl[2][ord[i]]] == 0.0)
                || hide_material[mat[el_this]] ) )
        {
            m_edges[0][m_edges_cnt] = edge_tbl[0][ord[i]];
            m_edges[1][m_edges_cnt] = edge_tbl[1][ord[i]];
            m_edge_mtl[m_edges_cnt] = mat[ edge_tbl[2][ord[i]] ];
            m_edges_cnt++;
        }
    }

    for ( i = 0; i < 3; i++ )
        free( edge_tbl[i] );
    free( ord );

    /* Create Edge_list_obj for return. */
    p_elo = create_compressed_edge_list( m_edges, m_edge_mtl, m_edges_cnt );

    free( m_edges[0] );
    free( m_edges[1] );
    free( m_edge_mtl );

    return p_elo;
}


/*************************************************************
 * TAG( edge_heapsort )
 *
 * Heap sort, from Numerical_Recipes_in_C.  The node list array
 * is arrin and the sorted ordering is returned in the indx array.
 */
static void
edge_heapsort( int n, int *arrin[3], int *indx, int *face_el, int *mat,
               float *mtl_trans[3] )
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
            if ( j < ir
                    && edge_compare( indx[j], indx[j+1], arrin, face_el, mat,
                                     mtl_trans )
                    < 0 )
                j++;
            if ( edge_compare( indxt, indx[j], arrin, face_el, mat,
                               mtl_trans )
                    < 0 )
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
 * TAG( edge_compare )
 *
 * Differentiates two edges on the basis of (1) defining nodes,
 * (2) material inequality, or (3) material translation
 * inequality.  Called by the sorting routine to sort shell
 * and element face edges.  Assumes that the node numbers are
 * stored in the first two entries of n_arr, and face or
 * element number in the third entry.
 */
static int
edge_compare( int indx1, int indx2, int *n_arr[3], int *face_el, int *mat,
              float *mtl_trans[3] )
{
    int i;
    int el1, el2;
    int mat1, mat2;
    float trans1x, trans1y, trans1z, trans2x, trans2y, trans2z;

    for ( i = 0; i < 2; i++ )
    {
        if ( n_arr[i][indx1] < n_arr[i][indx2] )
            return -1;
        else if ( n_arr[i][indx1] > n_arr[i][indx2] )
            return 1;
    }

    /*
     * Edge matches.  Sort further on material and  material translation
     * distance to prevent equating different materials which have
     * unequal translations.  Without this distinction, opposing faces
     * of different materials (exposed through a material translation)
     * which share an element edge can generate a visual edge even
     * though they're translated apart and visually have no common edge.
     */

    el1 = n_arr[2][indx1];
    el2 = n_arr[2][indx2];

    if ( face_el != NULL )
    {
        /*
         * For hexes, n_arr holds the face number; get the element id
         * from face_el.
         */
        el1 = face_el[el1];
        el2 = face_el[el2];
    }

    mat1 = mat[el1];
    mat2 = mat[el2];

    if ( mat1 != mat2 )
    {
        trans1x = mtl_trans[0][mat1];
        trans2x = mtl_trans[0][mat2];

        if ( trans1x < trans2x )
            return -1;
        else if ( trans1x > trans2x )
            return 1;

        trans1y = mtl_trans[1][mat1];
        trans2y = mtl_trans[1][mat2];

        if ( trans1y < trans2y )
            return -1;
        else if ( trans1y > trans2y )
            return 1;

        trans1z = mtl_trans[2][mat1];
        trans2z = mtl_trans[2][mat2];

        if ( trans1z < trans2z )
            return -1;
        else if ( trans1z > trans2z )
            return 1;
    }

    return 0;
}


#ifdef OLD_CREATE_BLOCKS
/************************************************************
 * TAG( create_elem_blocks )
 *
 * Group elements into smaller blocks in order to make face
 * table generation and vector drawing routines more efficient.
 */
void
create_elem_blocks( Analysis *analy )
{
    Nodal *nodes;
    Hex_geom *bricks;
    Shell_geom *shells;
    int block_sz, block_cnt;
    int lo, hi, nd;
    int i, j, k, m;

    nodes = analy->geom_p->nodes;

    if ( analy->dimension == 3 && analy->geom_p->bricks != NULL )
    {
        bricks = analy->geom_p->bricks;

        if ( bricks->cnt > 5000 )
        {
            block_sz = 2 * (int) sqrt( (double) bricks->cnt );
            block_cnt = bricks->cnt / block_sz;
            if ( bricks->cnt % block_sz > 0 )
                block_cnt++;
        }
        else
        {
            /* For small grids, just use one big block. */
            block_cnt = 1;
            block_sz = bricks->cnt;
        }

        /* Allocate block tables. */
        analy->num_blocks = block_cnt;
        analy->block_lo = NEW_N( int, block_cnt, "Blocks low elem" );
        analy->block_hi = NEW_N( int, block_cnt, "Blocks high elem" );
        for ( i = 0; i < 2; i++ )
            for ( j = 0; j < 3; j++ )
                analy->block_bbox[i][j] =
                    NEW_N( float, block_cnt, "Block bboxes" );

        /* Get low and high element of each block. */
        lo = 0;
        hi = block_sz - 1;
        for ( i = 0; i < block_cnt; i++ )
        {
            if ( hi >= bricks->cnt )
                hi = bricks->cnt - 1;

            analy->block_lo[i] = lo;
            analy->block_hi[i] = hi;

            lo = hi + 1;
            hi = lo + block_sz - 1;
        }

        /* Get a bbox for all nodes in each block. */
        for ( i = 0; i < block_cnt; i++ )
        {
            /* Initialize min and max. */
            nd = bricks->nodes[0][ analy->block_lo[i] ];
            for ( k = 0; k < 3; k++ )
            {
                analy->block_bbox[0][k][i] = nodes->xyz[k][nd];
                analy->block_bbox[1][k][i] = nodes->xyz[k][nd];
            }

            for ( j = analy->block_lo[i]; j <= analy->block_hi[i]; j++ )
            {
                for ( m = 0; m < 8; m++ )
                {
                    nd = bricks->nodes[m][j];
                    for ( k = 0; k < 3; k++ )
                    {
                        if ( nodes->xyz[k][nd] < analy->block_bbox[0][k][i] )
                            analy->block_bbox[0][k][i] = nodes->xyz[k][nd];
                        if ( nodes->xyz[k][nd] > analy->block_bbox[1][k][i] )
                            analy->block_bbox[1][k][i] = nodes->xyz[k][nd];
                    }
                }
            }
        }
    }
    else if ( analy->dimension == 2 && analy->geom_p->shells != NULL )
    {
        /*
         * Blocking for 2D shell data is currently only used to
         * speed up the 2D vector drawing routines.
         */

        shells = analy->geom_p->shells;

        if ( shells->cnt > 1000 )
        {
            block_sz = (int) sqrt( (double) shells->cnt );
            block_cnt = shells->cnt / block_sz;
            if ( shells->cnt % block_sz > 0 )
                block_cnt++;
        }
        else
        {
            /* For small grids, just use one big block. */
            block_cnt = 1;
            block_sz = shells->cnt;
        }

        /* Allocate block tables. */
        analy->num_blocks = block_cnt;
        analy->block_lo = NEW_N( int, block_cnt, "Blocks low elem" );
        analy->block_hi = NEW_N( int, block_cnt, "Blocks high elem" );
        for ( i = 0; i < 2; i++ )
            for ( j = 0; j < 3; j++ )
                analy->block_bbox[i][j] =
                    NEW_N( float, block_cnt, "Block bboxes" );

        /* Get low and high element of each block. */
        lo = 0;
        hi = block_sz - 1;
        for ( i = 0; i < block_cnt; i++ )
        {
            if ( hi >= shells->cnt )
                hi = shells->cnt - 1;

            analy->block_lo[i] = lo;
            analy->block_hi[i] = hi;

            lo = hi + 1;
            hi = lo + block_sz - 1;
        }

        /* Get a bbox for all nodes in each block. */
        for ( i = 0; i < block_cnt; i++ )
        {
            /* Initialize min and max. */
            nd = shells->nodes[0][ analy->block_lo[i] ];
            for ( k = 0; k < 2; k++ )
            {
                analy->block_bbox[0][k][i] = nodes->xyz[k][nd];
                analy->block_bbox[1][k][i] = nodes->xyz[k][nd];
            }

            for ( j = analy->block_lo[i]; j <= analy->block_hi[i]; j++ )
            {
                for ( m = 0; m < 4; m++ )
                {
                    nd = shells->nodes[m][j];
                    for ( k = 0; k < 2; k++ )
                    {
                        if ( nodes->xyz[k][nd] < analy->block_bbox[0][k][i] )
                            analy->block_bbox[0][k][i] = nodes->xyz[k][nd];
                        if ( nodes->xyz[k][nd] > analy->block_bbox[1][k][i] )
                            analy->block_bbox[1][k][i] = nodes->xyz[k][nd];
                    }
                }
            }
        }
    }
}
#endif


/************************************************************
 * TAG( create_hex_blocks )
 *
 * Group elements into smaller blocks in order to make face
 * table generation and vector drawing routines more efficient.
 */
void
create_hex_blocks( MO_class_data *p_hex_class, Analysis *analy )
{
    int block_sz, block_cnt;
    int lo, hi, nd;
    int i, j, k, m;
    GVec3D *nodes;
    int (*connects)[8];
    int hex_qty;
    Elem_block_obj *p_ebo;

    connects = (int (*)[8]) p_hex_class->objects.elems->nodes;
    nodes = analy->mesh_table[p_hex_class->mesh_id].node_geom->objects.nodes3d;
    hex_qty = p_hex_class->qty;

    if ( hex_qty > 5000 )
    {
        block_sz = 2 * (int) sqrt( (double) hex_qty );
        block_cnt = hex_qty / block_sz;
        if ( hex_qty % block_sz > 0 )
            block_cnt++;
    }
    else
    {
        /* For small grids, just use one big block. */
        block_cnt = 1;
        block_sz = hex_qty;
    }

    /* Allocate block tables. */
    p_ebo = NEW( Elem_block_obj, "Hex elem block" );
    p_ebo->num_blocks = block_cnt;
    p_ebo->block_lo = NEW_N( int, block_cnt, "Blocks low elem" );
    p_ebo->block_hi = NEW_N( int, block_cnt, "Blocks high elem" );
    for ( i = 0; i < 2; i++ )
        for ( j = 0; j < 3; j++ )
            p_ebo->block_bbox[i][j] =
                NEW_N( float, block_cnt, "Block bboxes" );

    /* Get low and high element of each block. */
    lo = 0;
    hi = block_sz - 1;
    for ( i = 0; i < block_cnt; i++ )
    {
        if ( hi >= hex_qty )
            hi = hex_qty - 1;

        p_ebo->block_lo[i] = lo;
        p_ebo->block_hi[i] = hi;

        lo = hi + 1;
        hi = lo + block_sz - 1;
    }

    /* Get a bbox for all nodes in each block. */
    for ( i = 0; i < block_cnt; i++ )
    {
        /* Initialize min and max. */
        nd = connects[ p_ebo->block_lo[i] ][0];
        for ( k = 0; k < 3; k++ )
        {
            p_ebo->block_bbox[0][k][i] = nodes[nd][k];
            p_ebo->block_bbox[1][k][i] = nodes[nd][k];
        }

        for ( j = p_ebo->block_lo[i]; j <= p_ebo->block_hi[i]; j++ )
        {
            for ( m = 0; m < 8; m++ )
            {
                nd = connects[j][m];
                for ( k = 0; k < 3; k++ )
                {
                    if ( nodes[nd][k] < p_ebo->block_bbox[0][k][i] )
                        p_ebo->block_bbox[0][k][i] = nodes[nd][k];
                    if ( nodes[nd][k] > p_ebo->block_bbox[1][k][i] )
                        p_ebo->block_bbox[1][k][i] = nodes[nd][k];
                }
            }
        }
    }

    p_hex_class->p_elem_block = p_ebo;
}


/************************************************************
 * TAG( create_tet_blocks )
 *
 * Group elements into smaller blocks in order to make face
 * table generation and vector drawing routines more efficient.
 */
void
create_tet_blocks( MO_class_data *p_tet_class, Analysis *analy )
{
    int block_sz, block_cnt;
    int lo, hi, nd;
    int i, j, k, m;
    GVec3D *nodes;
    int (*connects)[4];
    int tet_qty;
    Elem_block_obj *p_ebo;

    connects = (int (*)[4]) p_tet_class->objects.elems->nodes;
    nodes = analy->mesh_table[p_tet_class->mesh_id].node_geom->objects.nodes3d;
    tet_qty = p_tet_class->qty;

    if ( tet_qty > 5000 )
    {
        block_sz = 2 * (int) sqrt( (double) tet_qty );
        block_cnt = tet_qty / block_sz;
        if ( tet_qty % block_sz > 0 )
            block_cnt++;
    }
    else
    {
        /* For small grids, just use one big block. */
        block_cnt = 1;
        block_sz = tet_qty;
    }

    /* Allocate block tables. */
    p_ebo = NEW( Elem_block_obj, "Tet elem block" );
    p_ebo->num_blocks = block_cnt;
    p_ebo->block_lo = NEW_N( int, block_cnt, "Blocks low elem" );
    p_ebo->block_hi = NEW_N( int, block_cnt, "Blocks high elem" );
    for ( i = 0; i < 2; i++ )
        for ( j = 0; j < 3; j++ )
            p_ebo->block_bbox[i][j] =
                NEW_N( float, block_cnt, "Block bboxes" );

    /* Get low and high element of each block. */
    lo = 0;
    hi = block_sz - 1;
    for ( i = 0; i < block_cnt; i++ )
    {
        if ( hi >= tet_qty )
            hi = tet_qty - 1;

        p_ebo->block_lo[i] = lo;
        p_ebo->block_hi[i] = hi;

        lo = hi + 1;
        hi = lo + block_sz - 1;
    }

    /* Get a bbox for all nodes in each block. */
    for ( i = 0; i < block_cnt; i++ )
    {
        /* Initialize min and max. */
        nd = connects[ p_ebo->block_lo[i] ][0];
        for ( k = 0; k < 3; k++ )
        {
            p_ebo->block_bbox[0][k][i] = nodes[nd][k];
            p_ebo->block_bbox[1][k][i] = nodes[nd][k];
        }

        for ( j = p_ebo->block_lo[i]; j <= p_ebo->block_hi[i]; j++ )
        {
            for ( m = 0; m < 4; m++ )
            {
                nd = connects[j][m];
                for ( k = 0; k < 3; k++ )
                {
                    if ( nodes[nd][k] < p_ebo->block_bbox[0][k][i] )
                        p_ebo->block_bbox[0][k][i] = nodes[nd][k];
                    if ( nodes[nd][k] > p_ebo->block_bbox[1][k][i] )
                        p_ebo->block_bbox[1][k][i] = nodes[nd][k];
                }
            }
        }
    }

    p_tet_class->p_elem_block = p_ebo;
}

/************************************************************
 * TAG( create_pyramid_tblocks )
 *
 * Group elements into smaller blocks in order to make face
 * table generation and vector drawing routines more efficient.
 */
void
create_pyramid_blocks( MO_class_data *p_pyramid_class, Analysis *analy )
{
    int block_sz, block_cnt;
    int lo, hi, nd;
    int i, j, k, m;
    GVec3D *nodes;
    int (*connects)[5];
    int pyramid_qty;
    Elem_block_obj *p_ebo;

    connects = (int (*)[5]) p_pyramid_class->objects.elems->nodes;
    nodes = analy->mesh_table[p_pyramid_class->mesh_id].node_geom->objects.nodes3d;
    pyramid_qty = p_pyramid_class->qty;

    if ( pyramid_qty > 5000 )
    {
        block_sz = 2 * (int) sqrt( (double) pyramid_qty );
        block_cnt = pyramid_qty / block_sz;
        if ( pyramid_qty % block_sz > 0 )
            block_cnt++;
    }
    else
    {
        /* For small grids, just use one big block. */
        block_cnt = 1;
        block_sz = pyramid_qty;
    }

    /* Allocate block tables. */
    p_ebo = NEW( Elem_block_obj, "Pyramid elem block" );
    p_ebo->num_blocks = block_cnt;
    p_ebo->block_lo = NEW_N( int, block_cnt, "Blocks low elem" );
    p_ebo->block_hi = NEW_N( int, block_cnt, "Blocks high elem" );
    for ( i = 0; i < 2; i++ )
        for ( j = 0; j < 3; j++ )
            p_ebo->block_bbox[i][j] =
                NEW_N( float, block_cnt, "Block bboxes" );

    /* Get low and high element of each block. */
    lo = 0;
    hi = block_sz - 1;
    for ( i = 0; i < block_cnt; i++ )
    {
        if ( hi >= pyramid_qty )
            hi = pyramid_qty - 1;

        p_ebo->block_lo[i] = lo;
        p_ebo->block_hi[i] = hi;

        lo = hi + 1;
        hi = lo + block_sz - 1;
    }

    /* Get a bbox for all nodes in each block. */
    for ( i = 0; i < block_cnt; i++ )
    {
        /* Initialize min and max. */
        nd = connects[ p_ebo->block_lo[i] ][0];
        for ( k = 0; k < 3; k++ )
        {
            p_ebo->block_bbox[0][k][i] = nodes[nd][k];
            p_ebo->block_bbox[1][k][i] = nodes[nd][k];
        }

        for ( j = p_ebo->block_lo[i]; j <= p_ebo->block_hi[i]; j++ )
        {
            for ( m = 0; m < 5; m++ )
            {
                nd = connects[j][m];
                for ( k = 0; k < 3; k++ )
                {
                    if ( nodes[nd][k] < p_ebo->block_bbox[0][k][i] )
                        p_ebo->block_bbox[0][k][i] = nodes[nd][k];
                    if ( nodes[nd][k] > p_ebo->block_bbox[1][k][i] )
                        p_ebo->block_bbox[1][k][i] = nodes[nd][k];
                }
            }
        }
    }

    p_pyramid_class->p_elem_block = p_ebo;
}

/************************************************************
 * TAG( create_quad_blocks )
 *
 * Group elements into smaller blocks in order to make face
 * table generation and vector drawing routines more efficient.
 */
void
create_quad_blocks( MO_class_data *p_quad_class, Analysis *analy )
{
    int block_sz, block_cnt;
    int lo, hi, nd;
    int i, j, k, m;
    GVec2D *nodes;
    int (*connects)[4];
    int quad_qty;
    Elem_block_obj *p_ebo;

    /*
     * Blocking for 2D quad data is currently only used to
     * speed up the 2D vector drawing routines.
     */
    if ( analy->dimension != 2 )
        return;

    connects = (int (*)[4]) p_quad_class->objects.elems->nodes;
    nodes = analy->mesh_table[p_quad_class->mesh_id].node_geom->objects.nodes2d;
    quad_qty = p_quad_class->qty;

    if ( quad_qty > 1000 )
    {
        block_sz = (int) sqrt( (double) quad_qty );
        block_cnt = quad_qty / block_sz;
        if ( quad_qty % block_sz > 0 )
            block_cnt++;
    }
    else
    {
        /* For small grids, just use one big block. */
        block_cnt = 1;
        block_sz = quad_qty;
    }

    /* Allocate block tables. */
    p_ebo = NEW( Elem_block_obj, "Quad elem block" );
    p_ebo->num_blocks = block_cnt;
    p_ebo->block_lo = NEW_N( int, block_cnt, "Blocks low elem" );
    p_ebo->block_hi = NEW_N( int, block_cnt, "Blocks high elem" );
    for ( i = 0; i < 2; i++ )
        for ( j = 0; j < 2; j++ )       /* Change limit to 3 if 3D added. */
            p_ebo->block_bbox[i][j] =
                NEW_N( float, block_cnt, "Block bboxes" );

    /* Get low and high element of each block. */
    lo = 0;
    hi = block_sz - 1;
    for ( i = 0; i < block_cnt; i++ )
    {
        if ( hi >= quad_qty )
            hi = quad_qty - 1;

        p_ebo->block_lo[i] = lo;
        p_ebo->block_hi[i] = hi;

        lo = hi + 1;
        hi = lo + block_sz - 1;
    }

    /* Get a bbox for all nodes in each block. */
    for ( i = 0; i < block_cnt; i++ )
    {
        /* Initialize min and max. */
        nd = connects[ p_ebo->block_lo[i] ][0];
        for ( k = 0; k < 2; k++ )
        {
            p_ebo->block_bbox[0][k][i] = nodes[nd][k];
            p_ebo->block_bbox[1][k][i] = nodes[nd][k];
        }

        for ( j = p_ebo->block_lo[i]; j <= p_ebo->block_hi[i]; j++ )
        {
            for ( m = 0; m < 4; m++ )
            {
                nd = connects[j][m];
                for ( k = 0; k < 2; k++ )
                {
                    if ( nodes[nd][k] < p_ebo->block_bbox[0][k][i] )
                        p_ebo->block_bbox[0][k][i] = nodes[nd][k];
                    if ( nodes[nd][k] > p_ebo->block_bbox[1][k][i] )
                        p_ebo->block_bbox[1][k][i] = nodes[nd][k];
                }
            }
        }
    }

    p_quad_class->p_elem_block = p_ebo;
}


/************************************************************
 * TAG( create_tri_blocks )
 *
 * Group elements into smaller blocks in order to make face
 * table generation and vector drawing routines more efficient.
 */
void
create_tri_blocks( MO_class_data *p_tri_class, Analysis *analy )
{
    int block_sz, block_cnt;
    int lo, hi, nd;
    int i, j, k, m;
    GVec2D *nodes;
    int (*connects)[3];
    int tri_qty;
    Elem_block_obj *p_ebo;

    /*
     * Blocking for 2D tri data is currently only used to
     * speed up the 2D vector drawing routines.
     */
    if ( analy->dimension != 2 )
        return;

    connects = (int (*)[3]) p_tri_class->objects.elems->nodes;
    nodes = analy->mesh_table[p_tri_class->mesh_id].node_geom->objects.nodes2d;
    tri_qty = p_tri_class->qty;

    if ( tri_qty > 1000 )
    {
        block_sz = (int) sqrt( (double) tri_qty );
        block_cnt = tri_qty / block_sz;
        if ( tri_qty % block_sz > 0 )
            block_cnt++;
    }
    else
    {
        /* For small grids, just use one big block. */
        block_cnt = 1;
        block_sz = tri_qty;
    }

    /* Allocate block tables. */
    p_ebo = NEW( Elem_block_obj, "Tri elem block" );
    p_ebo->num_blocks = block_cnt;
    p_ebo->block_lo = NEW_N( int, block_cnt, "Blocks low elem" );
    p_ebo->block_hi = NEW_N( int, block_cnt, "Blocks high elem" );
    for ( i = 0; i < 2; i++ )
        for ( j = 0; j < 2; j++ )       /* Change limit to 3 if 3D added. */
            p_ebo->block_bbox[i][j] =
                NEW_N( float, block_cnt, "Block bboxes" );

    /* Get low and high element of each block. */
    lo = 0;
    hi = block_sz - 1;
    for ( i = 0; i < block_cnt; i++ )
    {
        if ( hi >= tri_qty )
            hi = tri_qty - 1;

        p_ebo->block_lo[i] = lo;
        p_ebo->block_hi[i] = hi;

        lo = hi + 1;
        hi = lo + block_sz - 1;
    }

    /* Get a bbox for all nodes in each block. */
    for ( i = 0; i < block_cnt; i++ )
    {
        /* Initialize min and max. */
        nd = connects[ p_ebo->block_lo[i] ][0];
        for ( k = 0; k < 2; k++ )
        {
            p_ebo->block_bbox[0][k][i] = nodes[nd][k];
            p_ebo->block_bbox[1][k][i] = nodes[nd][k];
        }

        for ( j = p_ebo->block_lo[i]; j <= p_ebo->block_hi[i]; j++ )
        {
            for ( m = 0; m < 3; m++ )
            {
                nd = connects[j][m];
                for ( k = 0; k < 2; k++ )
                {
                    if ( nodes[nd][k] < p_ebo->block_bbox[0][k][i] )
                        p_ebo->block_bbox[0][k][i] = nodes[nd][k];
                    if ( nodes[nd][k] > p_ebo->block_bbox[1][k][i] )
                        p_ebo->block_bbox[1][k][i] = nodes[nd][k];
                }
            }
        }
    }

    p_tri_class->p_elem_block = p_ebo;
}


/************************************************************
 * TAG( free_elem_block )
 *
 * Free resources allocated for an Elem_block_obj.
 */
void
free_elem_block( Elem_block_obj **pp_elem_block, int dim )
{
    int i, j;
    Elem_block_obj *p_ebo;

    p_ebo = *pp_elem_block;

    if ( p_ebo == NULL )
        return;

    for ( i = 0; i < 2; i++ )
        for ( j = 0; j < dim; j++ )
            free( p_ebo->block_bbox[i][j] );

    free( p_ebo->block_hi );
    free( p_ebo->block_lo );

    free( p_ebo );

    *pp_elem_block = NULL;
}


/************************************************************
 * TAG( write_ref_file )
 *
 * Write out the current M_HEX face tables (or a subset thereof)
 * as a reference surface file.
 */
void
write_ref_file( char tokens[MAXTOKENS][TOKENLENGTH], int token_cnt,
                Analysis *analy )
{
    FILE *ofile;
    int i, j, k, l;
    int (*connects)[8];
    int fc, el;
    float *p_f;
    float verts[4][3];
    float *xyz_cons[3];
    float con;
    float *constraints = NULL;
    int xyz_qtys[3];
    static char xyz_chars[] = { 'x', 'y', 'z' };
    int face_total, face_qty;
    size_t c_qty;
    MO_class_data **p_hex_classes;
    List_head *p_lh;
    int *face_el, *face_fc;

    if ( token_cnt == 1 )
    {
        popup_dialog( USAGE_POPUP, "outref <filename> [x|y|z <coord>]..." );
        return;
    }

    ofile = fopen( tokens[1], "w" );
    if ( ofile == NULL )
    {
        popup_dialog( WARNING_POPUP, "Unable to open file \"%s\".", tokens[1] );
        return;
    }

    /* Get hex class list. */
    p_lh = MESH_P( analy )->classes_by_sclass + G_HEX;
    p_hex_classes = (MO_class_data **) p_lh->list;
    face_total = 0;

    if ( token_cnt == 2 )
    {
        /* No constraints; dump all external faces. */

        /* Count faces. */
        for ( i = 0; i < p_lh->qty; i++ )
            face_total += p_hex_classes[i]->p_vis_data->face_cnt;
        fprintf( ofile, "%d\n", face_total );

        /* Output faces for each hex class. */
        for ( i = 0; i < p_lh->qty; i++ )
        {
            face_qty = p_hex_classes[i]->p_vis_data->face_cnt;
            face_el = p_hex_classes[i]->p_vis_data->face_el;
            face_fc = p_hex_classes[i]->p_vis_data->face_fc;
            connects = (int (*)[8]) p_hex_classes[i]->objects.elems->nodes;

            for ( j = 0; j < face_qty; j++ )
            {
                el = face_el[j];
                fc = face_fc[j];

                fprintf( ofile, "%d %d %d %d\n",
                         connects[el][ fc_nd_nums[fc][0] ] + 1,
                         connects[el][ fc_nd_nums[fc][1] ] + 1,
                         connects[el][ fc_nd_nums[fc][2] ] + 1,
                         connects[el][ fc_nd_nums[fc][3] ] + 1 );
            }
        }
    }
    else
    {
        /* Extract constraints. */
        c_qty = (token_cnt - 2) / 2;
        if ( c_qty * 2 + 2 != (size_t) token_cnt )
        {
            popup_dialog( USAGE_POPUP, "outref <filename> [x|y|z <coord>]..." );
            fclose(ofile);
            return;
        }

        constraints = NEW_N( float, c_qty, "Outref constraint list" );

        /* Loop to find x, y, and z constraints sequentially. */
        p_f = constraints;
        for ( i = 0; i < 3; i++ )
        {
            xyz_cons[i] = p_f;
            xyz_qtys[i] = 0;

            /* Scan entire constraints list for current coord constraints. */
            for ( j = 2; j < token_cnt; j += 2 )
                if ( tokens[j][0] == xyz_chars[i] )
                {
                    /* Found one.  Save constraint value. */
                    *p_f++ = (float) atof( tokens[j + 1] );
                    xyz_qtys[i]++;
                }
        }

        if ( p_f - constraints != c_qty )
        {
            free( constraints );
            popup_dialog( USAGE_POPUP, "outref <filename> [x|y|z <coord>]..." );
            fclose(ofile);
            return;
        }

        /*
         * Now write the file.
         */

        /* Write a blank first line since we don't know the count yet. */
        fwrite( "          \n", 1, 11, ofile );

        /* Output faces for each hex class. */
        for ( i = 0; i < p_lh->qty; i++ )
        {
            face_qty = p_hex_classes[i]->p_vis_data->face_cnt;
            face_el = p_hex_classes[i]->p_vis_data->face_el;
            face_fc = p_hex_classes[i]->p_vis_data->face_fc;
            connects = (int (*)[8]) p_hex_classes[i]->objects.elems->nodes;

            for ( j = 0; j < face_qty; j++ )
            {
                el = face_el[j];
                fc = face_fc[j];

                get_hex_face_verts( el, fc, p_hex_classes[i], analy, verts );

                /* For each dimension... */
                for ( l = 0; l < 3; l++ )
                {
                    /* For each constraint for this dimension... */
                    for ( k = 0; k < xyz_qtys[l]; k++ )
                    {
                        con = xyz_cons[l][k];

                        /* Compare coord for all four nodes. */
                        if ( fabs( verts[0][l] - con ) < 0.001
                                && fabs( verts[1][l] - con ) < 0.001
                                && fabs( verts[2][l] - con ) < 0.001
                                && fabs( verts[3][l] - con ) < 0.001 )
                        {
                            /* Face matches constraint; write it out. */
                            fprintf( ofile, "%d %d %d %d\n",
                                     connects[el][ fc_nd_nums[fc][0] ] + 1,
                                     connects[el][ fc_nd_nums[fc][1] ] + 1,
                                     connects[el][ fc_nd_nums[fc][2] ] + 1,
                                     connects[el][ fc_nd_nums[fc][3] ] + 1 );

                            face_total++;
                        }
                    }
                }
            }
        }

        /* Now overwrite the first line with the correct face count. */
        fseek( ofile, 0, SEEK_SET );
        fprintf( ofile, "%d", face_total );
    }
   
    if(constraints != NULL)
    {
        free( constraints );
    }

    fclose( ofile );
}

