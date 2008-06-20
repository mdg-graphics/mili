/* $Id$ */

/*
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
 */

 /*
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - October 24, 2007: Added element and nodal labels.
 *                See SCR #418.
 *
 ************************************************************************
 */

#ifndef MESH_H
#define MESH_H

#include "gahl.h"
#include "results.h"
#include "io_wrap.h"

#define MESH( a ) (a)->mesh_table[(a)->cur_mesh_id]
#define MESH_P( a ) ((a)->mesh_table + (a)->cur_mesh_id)

#define IS_ELEMENT_SCLASS( s ) \
    ( (s) == G_TRUSS || (s) == G_BEAM || (s) == G_TRI || (s) == G_QUAD || \
      (s) == G_TET || (s) == G_PYRAMID || (s) == G_WEDGE || (s) == G_HEX || \
      (s) == G_SURFACE )

/* Used for debugging data buffers */
/* #define NODAL_RESULT_BUFFER( a )					\
  (a)->mesh_table[(a)->cur_mesh_id + check_nodal_result((a))].node_geom->data_buffer
*/

#define NODAL_RESULT_BUFFER( a ) \
  (a)->mesh_table[(a)->cur_mesh_id].node_geom->data_buffer

/*****************************************************************
 * TAG( GVec2D )
 *
 * Two-element float array.
 */
typedef float GVec2D[2];


/*****************************************************************
 * TAG( GVec2D2P )
 *
 * Two-element double precision floating point array.
 */
typedef double GVec2D2P[2];


/*****************************************************************
 * TAG( GVec3D )
 *
 * Three-element float array.
 */
typedef float GVec3D[3];


/*****************************************************************
 * TAG( GVec3D2P )
 *
 * Three-element double precision floating point array.
 */
typedef double GVec3D2P[3];


/*****************************************************************
 * TAG( Int_2tuple )
 *
 * Two-element integer array.
 */
typedef int Int_2tuple[2];

typedef struct _visibility_data
{
    int **adjacency;
    unsigned char *visib;
    unsigned char *visib_damage;
    int *face_el;
    int *face_fc;
    float *face_norm[4][3];
    int face_cnt;
} Visibility_data;


typedef struct _elem_data
{
    int *nodes;
    int *mat;
    int *part;
    Bool_type has_degen;
} Elem_data;


typedef struct _material_data
{
    int elem_qty;
    int elem_block_qty;
    Int_2tuple *elem_blocks;
    struct _mo_class_data *elem_class;
    int node_block_qty;
    Int_2tuple *node_blocks;
} Material_data;


typedef struct _surface_data
{
    int *nodes;
    int facet_qty;
    int surface_id;
    int sel_facet_num;
    Bool_type visible;
    Visibility_data *p_vis_data;
} Surface_data;


typedef union _object_data
{
    float *nodes;
    GVec2D *nodes2d;
    GVec3D *nodes3d;
    GVec2D *particles2d;
    GVec3D *particles3d;
    Elem_data *elems;
    Material_data *materials;
    Surface_data *surfaces;
} Object_data;



/*****************************************************************
 * TAG( Elem_block_obj )
 *
 * Element blocks for improved access.
 */
typedef struct _elem_block_obj
{
    int num_blocks;
    int *block_lo;
    int *block_hi;
    float *block_bbox[2][3];
} Elem_block_obj;


/*****************************************************************
 * TAG( Vector_pt_obj )
 *
 * Point location for a displayed vector.
 */
typedef struct _vector_pt_obj
{
    struct _vector_pt_obj *next;
    struct _vector_pt_obj *prev;
    float pt[3];
    int elnum;
    float xi[4]; /* (r,s,t) for hex's; (r,s,t,u) for tet's */
    float vec[3];
} Vector_pt_obj;


typedef struct _mo_class_labels
{
    int local_id;
    int label_num;
} MO_class_labels; 


/*****************************************************************
 * TAG( Label_blocks )
 *
 *   Struct which contains label blocks info for each superclass.
 */
typedef struct _Label_Block_Data
{
  int       label_mat;
  int       label_start;
  int       label_stop;
} Label_block_data;

typedef struct _Label_Blocks
{
  int              block_qty;
  int              block_total_objects;
  int              block_min, block_max;
  Label_block_data *block_objects;
} Label_blocks;


typedef struct _mo_class_data
{
    int mesh_id;
    char *short_name;
    char *long_name;
    int superclass;
    int elem_class_index;
    int qty;
    int simple_start;
    int simple_stop;
    Object_data objects;
    Visibility_data *p_vis_data;
    Elem_block_obj *p_elem_block;
    Vector_pt_obj *vector_pts;
    int vector_pt_qty;
    int *referenced_nodes;
    int referenced_node_qty;
    int surface_sizes;
    float *data_buffer;

    /* Data associated with Labels */
    Bool_type labels_found;
    MO_class_labels *labels;
    int             *labels_index;
    int labels_min;
    int labels_max;

    Label_blocks label_blocking;
} MO_class_data;


/*****************************************************************
 * TAG( Edge_obj )
 *
 * Face/element edge data.
 */
typedef struct _edge_obj
{
    int node1;
    int node2;
    int mtl;
    int addl_mtl;
} Edge_obj;


/*****************************************************************
 * TAG( Edge_list_obj )
 *
 * Face/element edge list data.
 */
typedef struct _edge_list_obj
{
    int size;
    Edge_obj *list;
    int overflow_qty;
    Int_2tuple *overflow;
} Edge_list_obj;


typedef struct _mesh_data
{
    Hash_table *class_table;                /* Hash table to store classes */
    MO_class_data *node_geom;               /* Direct access to "Nodal" class */
    List_head classes_by_sclass[QTY_SCLASS]; /* Direct access by superclass */
    int elem_class_qty;
    int material_qty;
    int surface_qty;
    Bool_type double_precision_nodpos;
    Bool_type double_precision_sand;
    Bool_type double_precision_partpos;
    unsigned char *hide_material;
    unsigned char *disable_material;
    Bool_type translate_material;
    unsigned char *hide_surface;
    unsigned char *disable_surface;

    /* October 24, 2006: IRC - New arrays for hiding/enabling by class
     * type.
     */
    int mat_hide_qty;
    int mat_disable_qty;
 
    /* BRICKS */
    int brick_qty;      /* Total number of bricks */
    int brick_hide_qty; /* Number of current hidden 
                         * materials for bricks 
                         */
    int brick_disable_qty; /* Number of current disabled 
                            * materials for bricks 
			    */
    int brick_exclude_qty; /* Number of current excluded 
                            * materials for bricks 
                            */

    unsigned char *hide_brick;    /* List of hidden materials for bricks   */
    unsigned char *disable_brick; /* List of disabled materials for bricks */
    unsigned char *exclude_brick; /* List of disabled materials for bricks */
    
    /* February 20, 2007: IRC Added array to track individual bricks that are
     * hidden or enabled.
     */
    Bool_type hide_brick_by_mat;
    Bool_type hide_brick_by_result;
    Bool_type disable_brick_by_mat;
    Bool_type exclude_brick_by_mat;

    float     brick_result_min;
    float     brick_result_max;

    int hide_brick_elem_qty;
    int disable_brick_elem_qty;
    int exclude_brick_elem_qty;

    unsigned char *hide_brick_elem;    /* A flag for each brick - TRUE if hidden   */
    unsigned char *disable_brick_elem; /* A flag for each brick - TRUE if disabled */ 
    unsigned char *exclude_brick_elem; /* A flag for each brick - TRUE if excluded */ 

    /* SHELLS */
    int shell_qty;
    int shell_hide_qty;
    int shell_disable_qty;
    int shell_exclude_qty; 
 
    unsigned char *hide_shell;
    unsigned char *disable_shell;
    unsigned char *exclude_shell;

    float     shell_result_min;
    float     shell_result_max;

    /* February 20, 2007: IRC Added array to track individual shells that are
     * hidden, enabled, or excluded.
     */
    Bool_type hide_shell_by_mat;
    Bool_type hide_shell_by_result;
    Bool_type disable_shell_by_mat;
    Bool_type exclude_shell_by_mat;

    int hide_shell_elem_qty;
    int disable_shell_elem_qty;
    int exclude_shell_elem_qty;

    unsigned char *hide_shell_elem;    /* A flag for each shell - TRUE if hidden   */
    unsigned char *disable_shell_elem; /* A flag for each shell - TRUE if disabled */ 
    unsigned char *exclude_shell_elem; /* A flag for each shell - TRUE if excluded */ 


    /* BEAMS */
    int beam_qty;
    int beam_hide_qty;
    int beam_disable_qty;
    int beam_exclude_qty;

    unsigned char *hide_beam;
    unsigned char *disable_beam;
    unsigned char *exclude_beam;

    float     beam_result_min;
    float     beam_result_max;

    /* February 20, 2007: IRC Added array to track individual beams that are
     * hidden or enabled.
     */
    Bool_type hide_beam_by_mat;
    Bool_type hide_beam_by_result;
    Bool_type disable_beam_by_mat;
    Bool_type exclude_beam_by_mat;

    int hide_beam_elem_qty;
    int disable_beam_elem_qty;
    int exclude_beam_elem_qty;

    unsigned char *hide_beam_elem;    /* A flag for each beam - TRUE if hidden   */
    unsigned char *disable_beam_elem; /* A flag for each beam - TRUE if disabled */ 
    unsigned char *exclude_beam_elem; /* A flag for each beam - TRUE if excluded */ 


    /* TRUSS */
    int truss_qty;
    int truss_hide_qty; 
    int truss_disable_qty; 
    int truss_exclude_qty; 

    unsigned char *hide_truss;
    unsigned char *disable_truss;
    unsigned char *exclude_truss;

    float     truss_result_min;
    float     truss_result_max;

    /* February 20, 2007: IRC Added array to track individual trusss that are
     * hidden or enabled.
     */
    Bool_type hide_truss_by_mat;
    Bool_type hide_truss_by_result;
    Bool_type disable_truss_by_mat;
    Bool_type exclude_truss_by_mat;

    int hide_truss_elem_qty;
    int disable_truss_elem_qty;
    int exclude_truss_elem_qty;

    unsigned char *hide_truss_elem;    /* A flag for each truss - TRUE if hidden   */
    unsigned char *disable_truss_elem; /* A flag for each truss - TRUE if disabled */ 
    unsigned char *exclude_truss_elem; /* A flag for each truss - TRUE if excluded */ 


    /* PARTICLES */
    int particle_qty;    
    int particle_hide_qty;    
    int particle_disable_qty;    

    unsigned char *hide_particle;
    unsigned char *disable_particle;
        
    float     particle_result_min;
    float     particle_result_max;

    Bool_type hide_particle_by_result;

    float *mtl_trans[3];
    Edge_list_obj *edge_list;
} Mesh_data;



/*****************************************************************
 * TAG( State2 )
 *
 * Groups all of the time-dependent state data for a given instant.
 */
typedef struct
{
    int state_no;
    int srec_id;
    
    float time;
    
    int position_constant;
    int sand_present;

    Object_data nodes;
    float **elem_class_sand;
    
    Object_data particles;
} State2;

#endif
