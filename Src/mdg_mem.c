/* $Id$ */
/*
 * mdg_mem.c - Memory routines for common FEM data structures.
 *
 *      Methods Development Group
 *      Lawrence Livermore National Laboratory
 *      Jan 6 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */

#include <stdio.h>
#include <math.h>
#include "misc.h"
#include "mdg.h"
#include "results.h"


/* Forward declarations for local routines. */
static Nodal       *mk_nodal( /* cnt, all_states_flag */ );
static Hex_geom    *mk_hex_geom( /* cnt */ );
static Shell_geom  *mk_shell_geom( /* cnt */ );
static Beam_geom   *mk_beam_geom( /* cnt */ );
static Hex_state   *mk_hex_state( /* cnt */ );
static Shell_state *mk_shell_state( /* cnt */ );
static Beam_state  *mk_beam_state( /* cnt */ );
static void fr_nodal( /* Nodal * */ );
static void fr_hex_geom( /* Hex_geom * */ );
static void fr_shell_geom( /* Shell_geom * */ );
static void fr_beam_geom( /* Beam_geom * */ );
static void fr_hex_state( /* Hex_state * */ );
static void fr_shell_state( /* Shell_state * */ );
static void fr_beam_state( /* Beam_state * */ );


/*****************************************************************
 * TAG( fc_nd_nums )
 *
 * Table which gives the nodes corresponding to each face of a
 * brick element.
 */
int fc_nd_nums[6][4] = { {1, 2, 6, 5}, 
                         {2, 3, 7, 6},
                         {0, 4, 7, 3},
                         {1, 5, 4, 0},
                         {4, 5, 6, 7},
                         {0, 3, 2, 1} };


/*****************************************************************
 * TAG( edge_face_nums )
 *
 * Table which gives the numbers of the faces which are adjacent
 * to each edge on a hexahedral element.
 */
int edge_face_nums[12][2] = { {2,3}, {0,3}, {0,1}, {1,2}, {3,5}, {1,5},
                              {1,4}, {3,4}, {2,5}, {2,4}, {4,0}, {0,5} };


/*****************************************************************
 * TAG( edge_node_nums )
 *
 * Table which gives the numbers of the nodes at the ends of
 * each edge on a hexahedral element.
 */
int edge_node_nums[12][2] = { {0,4}, {1,5}, {2,6}, {3,7}, {0,1}, {3,2},
                              {7,6}, {4,5}, {0,3}, {4,7}, {5,6}, {1,2} };


/*****************************************************************
 * TAG( mk_nodal )
 *
 * Geometry allocation.
 */
static Nodal *mk_nodal( cnt )
int cnt;
{
    Nodal *nodes;
    float *array;
    int i;

    nodes = NEW( Nodal, "Nodal struct" );
    array = NEW_N( float, 3*cnt, "Nodal data" );

    if ( nodes == NULL || array == NULL )
        popup_fatal( "Can't allocate space for Nodal.\n" );

    nodes->cnt = cnt;
    for ( i = 0; i < 3; i++ )
        nodes->xyz[i] = array + i*cnt;

    return nodes;
}


/*****************************************************************
 * TAG( mk_hex_geom )
 *
 * Geometry allocation.
 */
static Hex_geom *mk_hex_geom( cnt )
int cnt;
{
    Hex_geom *bricks;
    int *array;
    int i;

    bricks = NEW( Hex_geom, "Hex struct" );
    array = NEW_N( int, 9*cnt, "Hex data" );
    if ( bricks == NULL || array == NULL )
        popup_fatal( "Can't allocate space for Hex.\n" );

    bricks->cnt = cnt;
    for ( i = 0; i < 8; i++ )
        bricks->nodes[i] = array + i*cnt;
    bricks->mat = array + 8*cnt;
    bricks->degen_elems = FALSE;

    return bricks;
}


/*****************************************************************
 * TAG( mk_shell_geom )
 *
 * Geometry allocation.
 */
static Shell_geom *mk_shell_geom( cnt )
int cnt;
{
    Shell_geom *shells;
    int *array;
    int i;

    shells = NEW( Shell_geom, "Shell struct" );
    array = NEW_N( int, 5*cnt, "Shell data" );
    if ( shells == NULL || array == NULL )
        popup_fatal( "Can't allocate space for Shell.\n" );

    shells->cnt = cnt;
    for ( i = 0; i < 4; i++ )
        shells->nodes[i] = array + i*cnt;
    shells->mat = array + 4*cnt;
    shells->degen_elems = FALSE;

    return shells;
}


/*****************************************************************
 * TAG( mk_beam_geom )
 *
 * Geometry allocation.
 */
static Beam_geom *mk_beam_geom( cnt )
int cnt;
{
    Beam_geom *beams;
    int *array;
    int i;

    beams = NEW( Beam_geom, "Beam struct" );
    array = NEW_N( int, 6*cnt, "Beam data" );
    if ( beams == NULL || array == NULL )
        popup_fatal( "Can't allocate space for Beam.\n" );

    beams->cnt = cnt;
    for ( i = 0; i < 5; i++ )
        beams->nodes[i] = array + i*cnt;
    beams->mat = array + 5*cnt;

    return beams;
}


/*****************************************************************
 * TAG( mk_geom )
 *
 * Geometry allocation.
 */
Geom *mk_geom( nnodes, nbricks, nshells, nbeams )
int nnodes;
int nbricks;
int nshells;
int nbeams;
{
    Geom *geom_str;

    if ( (geom_str = NEW( Geom, "Geom struct" )) == NULL )
        popup_fatal( "Can't allocate space for Geom.\n" );

    geom_str->nodes = mk_nodal( nnodes );
    geom_str->bricks = nbricks > 0 ? mk_hex_geom( nbricks ) : NULL;
    geom_str->shells = nshells > 0 ? mk_shell_geom( nshells ) : NULL;
    geom_str->beams = nbeams > 0 ? mk_beam_geom( nbeams ) : NULL;

    return geom_str;
}


/*****************************************************************
 * TAG( mk_hex_state )
 *
 * State allocation.
 */
static Hex_state *mk_hex_state( cnt )
int cnt;
{
    Hex_state *bricks;
    float *array;

    bricks = NEW( Hex_state, "Hex_state struct" );
    array = NEW_N( float, cnt, "Hex_state data" );
    if ( bricks == NULL || array == NULL )
        popup_fatal( "Can't allocate space for Hex_state.\n" );

    bricks->cnt = cnt;
    bricks->activity = array;

    return bricks;
}


/*****************************************************************
 * TAG( mk_shell_state )
 *
 * State allocation.
 */
static Shell_state *mk_shell_state( cnt )
int cnt;
{
    Shell_state *shells;
    float *array;

    shells = NEW( Shell_state, "Shell_state struct" );
    array = NEW_N( float, cnt, "Shell_state data" );
    if ( shells == NULL || array == NULL )
        popup_fatal( "Can't allocate space for Shell_state.\n" );

    shells->cnt = cnt;
    shells->activity = array;

    return shells;
}


/*****************************************************************
 * TAG( mk_beam_state )
 *
 * State allocation.
 */
static Beam_state *mk_beam_state( cnt )
int cnt;
{
    Beam_state *beams;
    float *array;

    beams = NEW( Beam_state, "Beam_state struct" );
    array = NEW_N( float, cnt, "Beam_state data" );
    if ( beams == NULL || array == NULL )
        popup_fatal( "Can't allocate space for Beam_state.\n" );

    beams->cnt = cnt;
    beams->activity = array;

    return beams;
}


/*****************************************************************
 * TAG( mk_state )
 *
 * State allocation.
 */
State *mk_state( nnodes, nbricks, nshells, nbeams, activity )
int nnodes;
int nbricks;
int nshells;
int nbeams;
int activity;
{
    State *state_str;

    if ( (state_str = NEW( State, "State struct" )) == NULL )
        popup_fatal( "Can't allocate space for State.\n" );

    state_str->nodes = nnodes > 0 ? mk_nodal( nnodes ) : NULL;
    state_str->activity_present = activity;

    if ( activity )
    {
        state_str->bricks = nbricks > 0 ? mk_hex_state( nbricks ) : NULL;
        state_str->shells = nshells > 0 ? mk_shell_state( nshells ) : NULL;
        state_str->beams = nbeams > 0 ? mk_beam_state( nbeams ) : NULL;
    }

    return state_str;
}


/*****************************************************************
 * TAG( fr_nodal )
 *
 * Geometry de-allocation.
 */
static void fr_nodal( nodes )
Nodal *nodes;
{
    if ( nodes != NULL )
    {
        free( nodes->xyz[0] );
        free( nodes );
    }
}


/*****************************************************************
 * TAG( fr_hex_geom )
 *
 * Geometry de-allocation.
 */
static void fr_hex_geom( bricks )
Hex_geom *bricks;
{
    if ( bricks != NULL )
    {
        free( bricks->nodes[0] );
        free( bricks );
    }
}


/*****************************************************************
 * TAG( fr_shell_geom )
 *
 * Geometry de-allocation.
 */
static void fr_shell_geom( shells )
Shell_geom *shells;
{
    if ( shells != NULL )
    {
        free( shells->nodes[0] );
        free( shells );
    }
}


/*****************************************************************
 * TAG( fr_beam_geom )
 *
 * Geometry de-allocation.
 */
static void fr_beam_geom( beams )
Beam_geom *beams;
{
    if ( beams != NULL )
    {
        free( beams->nodes[0] );
        free( beams );
    }
}


/*****************************************************************
 * TAG( fr_geom )
 *
 * Geometry de-allocation.
 */
void fr_geom( geom_str )
Geom *geom_str;
{
    if ( geom_str != NULL )
    {
        fr_nodal( geom_str->nodes );
        fr_hex_geom( geom_str->bricks );
        fr_shell_geom( geom_str->shells );
        fr_beam_geom( geom_str->beams );
        free( geom_str );
    }
}


/*****************************************************************
 * TAG( fr_hex_state )
 *
 * State de-allocation.
 */
static void fr_hex_state( bricks )
Hex_state *bricks;
{
    if ( bricks != NULL )
    {
        free ( bricks->activity );
        free ( bricks );
    }
}


/*****************************************************************
 * TAG( fr_shell_state )
 *
 * State de-allocation.
 */
static void fr_shell_state( shells )
Shell_state *shells;
{
    if ( shells != NULL )
    {
        free ( shells->activity );
        free ( shells );
    }
}


/*****************************************************************
 * TAG( fr_beam_state )
 *
 * State de-allocation.
 */
static void fr_beam_state( beams )
Beam_state *beams;
{
    if ( beams != NULL )
    {
        free ( beams->activity );
        free ( beams );
    }
}


/*****************************************************************
 * TAG( fr_state )
 *
 * State de-allocation.
 */
void fr_state( state_str )
State *state_str;
{
    if ( state_str != NULL )
    {
        if ( !state_str->position_constant )
            fr_nodal( state_str->nodes );
        fr_hex_state( state_str->bricks );
        fr_shell_state( state_str->shells );
        fr_beam_state( state_str->beams );
        free( state_str );
    }
}


/*****************************************************************
 * TAG( debug_geom )
 *
 * Debug routine for printing out geometry structure information.
 */
void debug_geom( geom_data )
Geom *geom_data;
{
    wrt_text( "\nGeom structure:" );
    wrt_text( "\n\tNode count: %d", geom_data->nodes->cnt );
    wrt_text( "\n\tFirst node: %f %f %f",
              geom_data->nodes->xyz[0][0],
              geom_data->nodes->xyz[1][0],
              geom_data->nodes->xyz[2][0] );

    if ( geom_data->bricks )
        wrt_text( "\n\tBrick count: %d", geom_data->bricks->cnt );
    else
        wrt_text( "\n\tNo bricks present" );
    if ( geom_data->shells )
        wrt_text( "\n\tShell count: %d", geom_data->shells->cnt );
    else
        wrt_text( "\n\tNo shells present" );
    if ( geom_data->beams )
        wrt_text( "\n\tBeam count: %d", geom_data->beams->cnt );
    else
        wrt_text( "\n\tNo beams present" );

    wrt_text( "\n\n" );
}


/*****************************************************************
 * TAG( debug_state )
 *
 * Debug routine for printing out state structure information.
 */
void debug_state( state_data )
State *state_data;
{
    int l;

    wrt_text( "\nState structure:" );

    wrt_text( "\n\tState id: %d", state_data->state_id );
    wrt_text( "\n\tTime: %f", state_data->time );

    wrt_text( "\n\tNode count: %d", state_data->nodes->cnt );
    wrt_text( "\n\tFirst node: %f %f %f", state_data->nodes->xyz[0][0],
              state_data->nodes->xyz[1][0], state_data->nodes->xyz[2][0] );
    l = state_data->nodes->cnt - 1;
    wrt_text( "\n\tLast node: %f %f %f", state_data->nodes->xyz[0][l],
              state_data->nodes->xyz[1][l], state_data->nodes->xyz[2][l] );

    if ( state_data->bricks )
        wrt_text( "\n\tBrick count: %d", state_data->bricks->cnt );
    else
        wrt_text( "\n\tNo bricks present" );
    if ( state_data->shells )
        wrt_text( "\n\tShell count: %d", state_data->shells->cnt );
    else
        wrt_text( "\n\tNo shells present" );
    if ( state_data->beams )
        wrt_text( "\n\tBeam count: %d", state_data->beams->cnt );
    else
        wrt_text( "\n\tNo beams present" );

    wrt_text( "\n\n" );
}


