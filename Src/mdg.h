/* $Id$ */
/*
 * mdg.h - Common FEM data structures.
 *
 *      Methods Development Group
 *      Lawrence Livermore National Laboratory
 *      Jan 6 1992
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

#ifndef MDG_H
#define MDG_H


/*****************************************************************
 * TAG( Nodal )
 *
 * Contains all nodal data.
 */
typedef struct
{
    int   cnt;
    float *xyz[3];
} Nodal;


/*****************************************************************
 * TAG( Hex_geom, Shell_geom, Beam_geom )
 *
 * The hex, shell and beam geom structures contain the connectivity
 * and material data.
 */
typedef struct
{
    int cnt;
    int *nodes[8];
    int *mat;
    Bool_type degen_elems;
} Hex_geom;

typedef struct
{
    int cnt;
    int *nodes[4];
    int *mat;
    Bool_type degen_elems;
} Shell_geom;

typedef struct
{
    int cnt;
    int *nodes[5]; /* Connectivities in 0-1; orientation in 2; rest unused. */
    int *mat;
} Beam_geom;


/*****************************************************************
 * TAG( Geom )
 *
 * Groups all the geometric information for a dataset.
 */
typedef struct
{
    Nodal      *nodes;
    Hex_geom   *bricks;
    Shell_geom *shells;
    Beam_geom  *beams;
    int dimension;
} Geom;


/*****************************************************************
 * TAG( Hex_state, Shell_state, Beam_state )
 *
 * The state structures contain all time-dependent element data.
 */
typedef struct
{
    int cnt;
    float *activity;
} Hex_state;

typedef struct
{
    int cnt;
    float *activity;
} Shell_state;

typedef struct
{
    int cnt;
    float *activity;
} Beam_state;


/*****************************************************************
 * TAG( State )
 *
 * Groups all of the time-dependent state data for a given instant.
 */
typedef struct
{
    int position_constant;
    int activity_present;

    /* These two aren't used, but keep them for debugging. */
    int state_id;
    float time;

    Nodal       *nodes;
    Hex_state   *bricks;
    Shell_state *shells;
    Beam_state  *beams;
} State;


/*****************************************************************
 * TAG( Buffer_pool )
 *
 * Structure to maintain a circular queue of data input buffers.
 */
typedef struct _buffer_pool
{
    int buffer_size;
    int buffer_count;
    int new_count;
    float **data_buffers;
    int *state_numbers;
    int recent;
} Buffer_pool;


/*****************************************************************
 * TAG( DFLT_BUFFER_POOL_SIZE )
 *
 * Default size of Buffer_pool circular queue.
 */
#define DFLT_BUFFER_POOL_SIZE (1)


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
extern int fc_nd_nums[6][4];


/*****************************************************************
 * TAG( edge_face_nums )
 *
 * Table which gives the numbers of the faces which are adjacent
 * to each edge on a hexahedral element.
 */
extern int edge_face_nums[12][2];


/*****************************************************************
 * TAG( edge_node_nums )
 *
 * Table which gives the numbers of the nodes at the ends of
 * each edge on a hexahedral element.
 */
extern int edge_node_nums[12][2];

/*****************************************************************
 * TAG( K_EPSILON_MASK A2_MASK )
 *
 * Bit-flag masks for additional state data.
 */
#define K_EPSILON_MASK 0x1
#define A2_MASK 0x2


/*
 * Routines in mdg_mem.c
 */
extern Geom *mk_geom( /* nnodes, nbricks, nshells, nbeams */ );
extern State *mk_state( /* nnodes, nbricks, nshells, nbeams */ );
extern void fr_geom( /* Geom * */ );
extern void fr_state( /* State * */ );
extern void debug_state( /* State * */ );
extern void debug_geom( /* Geom * */ );

/*
 * Routines in mdg_in.c
 */
extern int open_family( /* root_name */ );
extern void set_family( /* root_name */ );
extern void set_input_buffer_qty( /* object_type, qty */ );
extern Geom *get_geom( /* Geom * */ );
extern State *get_state( /* state_number, State * */ );
extern int is_in_database( /* result_id */ );
extern void get_result( /* result_id, state_num, result_arr */ );
extern void get_vec_result( /* result_id, state_num, arr_x, arr_y, arr_z */ );
extern Bool_type get_tensor_result( /* st_num, which, tensor_array */ );
extern void get_state_times( /* state_times */ );
extern void get_family_title( /* title_string */ );
extern void close_family();
extern int get_input_buffer_qty( /* object_type */ );


#endif
