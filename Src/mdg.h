/* $Id$ */
/*
 * mdg.h - Common FEM data structures.
 *
 *      Methods Development Group
 *      Lawrence Livermore National Laboratory
 *      Jan 6 1992
 *
 * Copyright (c) 1992 Regents of the University of California
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
 * TAG( NODE_T BEAM_T SHELL_T BRICK_T )
 *
 * Some useful defines for selecting element type.
 */
#define NODE_T 0
#define BEAM_T 1
#define SHELL_T 2
#define BRICK_T 3


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
extern Geom *get_geom( /* Geom * */ );
extern State *get_state( /* state_number, State * */ );
extern int is_in_database( /* result_id */ );
extern void get_result( /* result_id, state_num, result_arr */ );
extern void get_vec_result( /* result_id, state_num, arr_x, arr_y, arr_z */ );
extern void get_state_times( /* state_times */ );
extern void get_family_title( /* title_string */ );
extern void close_family();


#endif MDG_H
