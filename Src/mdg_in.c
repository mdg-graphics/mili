/* $Id$ */
/*
 * mdg_in.c - File reading module for MDG plotfiles.
 *
 *      Methods Development Group
 *      Lawrence Livermore National Laboratory
 *      Jan 6 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */

#include <sys/types.h>
#include <sys/stat.h>
#include "misc.h"
#include "list.h"
#include "mdg.h"
#include "results.h"

typedef struct _Family
{
    struct _Family *next;
    struct _Family *prev;
    char root_name[50];
    char title[50];
    int num_files;
    int num_states;
    int *file_sz;

    /* Arrays with information about the individual states. */
    float *st_time;
    int *st_file_num;
    int *st_file_loc;

    int geom_sz;
    int state_sz;
    FILE *cur_file;
    int cur_file_num;

    /* File header information. */
    int ctl[49];
    int unpacked;

    /* Section offsets, section sizes, and result locations in sections. */
    int node_nvar;
    int hex_offset;
    int hex_nvar;
    int shell_offset;
    int shell_nvar;
    int beam_offset;
    int beam_nvar;
    int *result_offsets;
    
    /* Input buffers by mesh object type. */
    Buffer_pool *node_input;
    Buffer_pool *beam_input;
    Buffer_pool *shell_input;
    Buffer_pool *hex_input;

    /* Sleazy, for handling files with no position data at states. */
    Geom *geom_p;
} Family;

Family *family_list = NULL;
Family *cur_family = NULL;


/*
 * Control words in plotfile header:
 *
 *   ndim    = ctl[ 0]     Dimension of data 
 *   numnp   = ctl[ 1]     Number of nodes
 *   icode   = ctl[ 2]     Flag for old or new database
 *   nglbv   = ctl[ 3]     Number of global variables for each state
 *   it      = ctl[ 4]     Temperatures included flag
 *   iu      = ctl[ 5]     Current geometry included flag
 *   iv      = ctl[ 6]     Velocities included flag
 *   ia      = ctl[ 7]     Accelerations included flag
 *   nel8    = ctl[ 8]     Number of brick elements
 *   nummat8 = ctl[ 9]     Number of materials used by brick elements
 *   nv3d    = ctl[12]     Number of variables for each brick element
 *   nel2    = ctl[13]     Number of beam elements
 *   nummat2 = ctl[14]     Number of materials used by beam elements
 *   nv1d    = ctl[15]     Number of variables for each beam element
 *   nel4    = ctl[16]     Number of shell elements
 *   nummat4 = ctl[17]     Number of materials used by shell elements
 *   nv2d    = ctl[18]     Number of variables for each shell element
 *   activ   = ctl[20]     Element activity included flag
 *   ixd     = ctl[24]     Flag word for extra data
 */


/* Local routines. */
static void init_buffer_pool();
static void delete_buffer_pool();
static void pf_name();
static void set_fam_file();
static void mdg_read();
static void fix_title();
static void check_degen_bricks();
static void check_degen_shells();


/*  Main for testing.  */
/*
main( argc, argv )
int argc;
char *argv[];
{
    int num_states;
    Geom *geo;
    State *state_ptr;
    int i;

    num_states = open_family( argv[1] );
    if ( num_states < 0 )
    {
        fprintf( stderr, "Couldn't read valid data from file %s\n", argv[1] );
        exit( -1 );
    }
    geo = get_geom( NULL );
    state_ptr = get_state( 0, NULL );

    debug_geom( geo );
    debug_state( state_ptr );

    for ( i = 1; i < num_states; i++ )
    {
        state_ptr = get_state( i, state_ptr );
        debug_state( state_ptr );
    }

    state_ptr = get_state( 0, state_ptr );
    debug_state( state_ptr );
}
*/


/*****************************************************************
 * TAG( open_family )
 *
 * Open a family of MDG plot files.  Returns -1 if unsuccessful,
 * otherwise returns the number of states in the family.  Note
 * that if the plot file has valid geometry but no states, the
 * routine returns 0.
 */
int
open_family( root_name )
char *root_name;
{
    Family *fam;
    struct stat statbuf;
    int sum, sumsav, max_st, st_num;
    int fnum, loc;
    int ndim, numnp, icode, nglbv, it, iu, iv, ia, ixd, xnd, nvqty;
    int nel8, nummat8, nv3d, nel2, nummat2, nv1d;
    int nel4, nummat4, nv2d, activ;
    int nv3dact, nv2dact, nv1dact;
    int seg;
    char fname[50];
    int i;
    Buffer_pool *p_bp;

    /*
     * Open the family, read in title and control data.
     */
    fam = NEW( Family, "Family struct" );
    strcpy( fam->root_name, root_name );
    if ( (fam->cur_file = fopen(root_name, "r")) == NULL )
    {
        free( fam );
        return -1;
    }
    fam->cur_file_num = 0;
    INSERT( fam, family_list );
    cur_family = fam;

    /*
     * Loop to get the number of files.  Then get the length of each
     * file.
     */
    i = 0;
    pf_name( i, fname );
    while ( stat( fname, &statbuf ) != -1 )
    {
        i++;
        pf_name( i, fname );
    }
    fam->num_files = i;

    fam->file_sz = NEW_N( int, fam->num_files, "File sizes" );

    for ( i = 0; i < fam->num_files; i++ )
    {
        pf_name( i, fname );
        stat( fname, &statbuf );
        fam->file_sz[i] = statbuf.st_size;
    }

    /*
     * Read in the header information.
     */
    mdg_read( 0, 0, 48*sizeof(char), fam->title );
    fix_title( fam->title );

    mdg_read( 0, 15*sizeof(int), 49*sizeof(int), (char *)(fam->ctl) );

    if ( fam->ctl[0] == 4 )
    {
        fam->unpacked = TRUE;
        fam->ctl[0] = 3;
    }
    else
        fam->unpacked = FALSE;

    ndim    = fam->ctl[ 0];
    numnp   = fam->ctl[ 1];
    icode   = fam->ctl[ 2];
    nglbv   = fam->ctl[ 3];
    it      = fam->ctl[ 4];
    iu      = fam->ctl[ 5];
    iv      = fam->ctl[ 6];
    ia      = fam->ctl[ 7];
    nel8    = fam->ctl[ 8];
    nummat8 = fam->ctl[ 9];
    nv3d    = fam->ctl[12];
    nel2    = fam->ctl[13];
    nummat2 = fam->ctl[14];
    nv1d    = fam->ctl[15];
    nel4    = fam->ctl[16];
    nummat4 = fam->ctl[17];
    nv2d    = fam->ctl[18];
    activ   = fam->ctl[20];
    ixd     = fam->ctl[24];

    /* If activity data present, include it with the rest of the variables. */
    nv1dact = nv1d;
    nv2dact = nv2d;
    nv3dact = nv3d;
    if ( activ >= 1000 && activ <= 1005 )
    {
        if ( nel2 > 0 )
            nv1dact++;
        if ( nel4 > 0 )
            nv2dact++;
        if ( nel8 > 0 )
            nv3dact++;
    }

    /* Check extra data flags to account for additional data. */
    xnd = ( ixd & K_EPSILON_MASK ) ? 2 : 0;  /* Two add'l var's if flag set. */
    xnd += ( ixd & A2_MASK ) ? 1 : 0;          /* One add'l var if flag set. */

    /*
     * Calculate number of nodal variable fields in state record.
     * Assume A2 will never be present without k and epsilon.
     */
    nvqty = (( icode == 1 ) ? 1 : it) + xnd + ndim * (iu + iv + ia);

#ifdef DEBUG
    wrt_text( "ndim %d nnodes %d nel8 %d nel4 %d nel2 %d\n",
                  ndim, numnp, nel8, nel4, nel2 );
#endif

    fam->geom_sz = (ndim*numnp + 9*nel8 + 5*nel4 + 6*nel2) * sizeof( float );
    fam->state_sz = (nvqty*numnp + nel8*nv3dact + 
                    nel4*nv2dact + nel2*nv1dact + 1 + nglbv) * sizeof( float );

#ifdef DEBUG
    wrt_text( "geom_sz %d state_sz %d\n", fam->geom_sz, fam->state_sz );
#endif

    /*
     * Calculate the maximum possible number of states.  When(ever) we can
     * forget IRIX 4.x compatibility, type sum as "long long" (64 bit) and
     * go back to previous code for max_st calculation to avoid overflow
     * checking.
     */
    max_st = 0;
    for ( i = 0, sum = -fam->geom_sz; i < fam->num_files; i++ )
    {
	sumsav = sum;
        sum += fam->file_sz[i];
	if ( sum < sumsav ) /* oops, overflow */
	{
	    max_st += sumsav / fam->state_sz;
	    sum = sumsav % fam->state_sz + fam->file_sz[i];
	}
    }
    max_st += sum / fam->state_sz;

    /*
     * Allocate large enough state arrays to handle the maximum
     * number of states possible. 
     */
    fam->st_time = NEW_N( float, max_st, "State times" );
    fam->st_file_num = NEW_N( int, max_st, "State files" );
    fam->st_file_loc = NEW_N( int, max_st, "State locations" );

    /*
     * Loop through the states and get the location and time of each.
     */
    loc = 64*sizeof( int ) + fam->geom_sz;
    fnum = 0;
    while ( loc >= fam->file_sz[fnum] )
    {
        loc = loc - fam->file_sz[fnum];
        fnum++;
    }

    for ( st_num = 0;; st_num++ )
    {
        /*
         * if ( fam->num_files > 1 && fam->state_sz <= fam->file_sz[1] )
	 * 
	 * A Nike family broke above logic, so see how this survives...
	 */
        if ( fam->num_files > 1 && fam->state_sz <= fam->file_sz[fnum] - loc )
        {
            if ( !(loc + fam->state_sz <= fam->file_sz[fnum]) )
            {
		/* Find next file in family big enough to hold a state. */
		do
		    fnum++;
                while ( fnum < fam->num_files
			&& fam->file_sz[fnum] < fam->state_sz );
                loc = 0;
                if ( fnum >= fam->num_files )
                {
                    fam->num_states = st_num;
                    break;
                }
            }
        }
        else if ( fam->num_files == 1 )
        {
            if ( !(loc + fam->state_sz <= fam->file_sz[fnum]) )
            {
                fam->num_states = st_num;
                break;
            }
        }
        else
        {
            while ( loc > 0 )
            {
                loc = loc - fam->file_sz[fnum];
                fnum++;
            }
            loc = 0;

            if ( fnum >= fam->num_files )
            {
                fam->num_states = st_num;
                break;
            }
        }

        fam->st_file_num[st_num] = fnum;
        fam->st_file_loc[st_num] = loc;
        mdg_read( fnum, loc, sizeof( float ), &(fam->st_time[st_num]) );

        /*
         * End of states for this file is signalled by -99999.0 in the
	 * time word.  We'll accept any negative number as the end marker.
	 * Reset file and state pointers and re-read the time word for the
	 * next state.
         */
	/*
        if ( fam->st_time[st_num] < 0.0 )
        {
            fam->num_states = st_num;
            break;
        }
	*/

        if ( fam->st_time[st_num] < 0.0 )
        {
            fnum++;
            loc = 0;
            if ( fnum >= fam->num_files )
            {
                fam->num_states = st_num;
                break;
            }
	    else
	    {
                fam->st_file_num[st_num] = fnum;
                fam->st_file_loc[st_num] = loc;
                mdg_read( fnum, loc, sizeof( float ), &(fam->st_time[st_num]) );
	    }
        }

        loc = loc + fam->state_sz;
    }

    /*
     * Calculate the offsets of results and their sections.  A result
     * offset of -1 indicates that the result is not present in the
     * data.
     */
    fam->result_offsets = NEW_N( int, VAL_ALL_END, "Result locations" );
    for ( i = 0; i < VAL_ALL_END; i++ )
        fam->result_offsets[i] = -1;

    loc = (1 + nglbv)*sizeof(float);
    seg = numnp*sizeof(float);
    
    /* Nodal variable qty. */
    fam->node_nvar = nvqty;
    
    /* Nodal positions. */
    if ( iu )
    {
        fam->result_offsets[VAL_NODE_POS] = loc;
        loc += ndim*seg;
    }

    /* Nodal velocities. */
    if ( iv )
    {
        /* Use the -2 and -3 flags to indicate the Y and Z components. */
        fam->result_offsets[VAL_NODE_VELX] = loc;
        fam->result_offsets[VAL_NODE_VELY] = -2;
        if ( ndim > 2 )
            fam->result_offsets[VAL_NODE_VELZ] = -3;
        loc += ndim*seg;
    }

    /* Nodal accelerations. */
    if ( ia )
    {
        fam->result_offsets[VAL_NODE_ACCX] = loc;
        fam->result_offsets[VAL_NODE_ACCY] = -2;
        if ( ndim > 2 )
            fam->result_offsets[VAL_NODE_ACCZ] = -3;
        loc += ndim*seg;
    }

    /* Nodal temperatures. */
    if ( it || icode == 1 )
    {
        fam->result_offsets[VAL_NODE_TEMP] = loc;
        loc += seg;
    }

    /* Turbulence vector. */
    if ( ixd & K_EPSILON_MASK )
    {
	fam->result_offsets[VAL_NODE_K] = loc;
	fam->result_offsets[VAL_NODE_EPSILON] = -2;
	loc += 2 * seg;
	if ( ixd & A2_MASK )
	{
	    fam->result_offsets[VAL_NODE_A2] = -3;
	    loc += seg;
	}
    }
    
    /* Node input buffer pool. */
    p_bp = NEW( Buffer_pool, "Node buffer pool" );
    init_buffer_pool( p_bp, DFLT_BUFFER_POOL_SIZE, nvqty * numnp );
    fam->node_input = p_bp;

    /* Brick data. */
    if ( nel8 > 0 )
    {
        fam->hex_offset = loc;
        fam->hex_nvar = nv3d;
        fam->result_offsets[VAL_HEX_SIGX] = 0;
        fam->result_offsets[VAL_HEX_SIGY] = 1;
        fam->result_offsets[VAL_HEX_SIGZ] = 2;
        fam->result_offsets[VAL_HEX_SIGXY] = 3;
        fam->result_offsets[VAL_HEX_SIGYZ] = 4;
        fam->result_offsets[VAL_HEX_SIGZX] = 5;
        fam->result_offsets[VAL_HEX_EPS_EFF] = 6;
        seg = nel8*sizeof(float);
        loc += nv3d*seg;
	
	p_bp = NEW( Buffer_pool, "Hex buffer pool" );
        init_buffer_pool( p_bp, DFLT_BUFFER_POOL_SIZE, nel8 * nv3d );
        fam->hex_input = p_bp;
    }

    /* Beam data. */
    if ( nel2 > 0 )
    {
        fam->beam_offset = loc;
        fam->beam_nvar = nv1d;
        fam->result_offsets[VAL_BEAM_AX_FORCE] = 0;
        fam->result_offsets[VAL_BEAM_S_SHEAR] = 1;
        fam->result_offsets[VAL_BEAM_T_SHEAR] = 2;
        fam->result_offsets[VAL_BEAM_S_MOMENT] = 3;
        fam->result_offsets[VAL_BEAM_T_MOMENT] = 4;
        fam->result_offsets[VAL_BEAM_TOR_MOMENT] = 5;
        seg = nel2*sizeof(float);
        loc += nv1d*seg;
	
	p_bp = NEW( Buffer_pool, "Beam buffer pool" );
        init_buffer_pool( p_bp, DFLT_BUFFER_POOL_SIZE, nel2 * nv1d );
        fam->beam_input = p_bp;
    }

    /* Shell data. */
    if ( nel4 > 0 )
    {
        fam->shell_offset = loc;
        fam->shell_nvar = nv2d;
        fam->result_offsets[VAL_SHELL_SIGX_MID] = 0;
        fam->result_offsets[VAL_SHELL_SIGY_MID] = 1;
        fam->result_offsets[VAL_SHELL_SIGZ_MID] = 2;
        fam->result_offsets[VAL_SHELL_SIGXY_MID] = 3;
        fam->result_offsets[VAL_SHELL_SIGYZ_MID] = 4;
        fam->result_offsets[VAL_SHELL_SIGZX_MID] = 5;
        fam->result_offsets[VAL_SHELL_EPS_EFF_MID] = 6;
        fam->result_offsets[VAL_SHELL_SIGX_IN] = 7;
        fam->result_offsets[VAL_SHELL_SIGY_IN] = 8;
        fam->result_offsets[VAL_SHELL_SIGZ_IN] = 9;
        fam->result_offsets[VAL_SHELL_SIGXY_IN] = 10;
        fam->result_offsets[VAL_SHELL_SIGYZ_IN] = 11;
        fam->result_offsets[VAL_SHELL_SIGZX_IN] = 12;
        fam->result_offsets[VAL_SHELL_EPS_EFF_IN] = 13;
        fam->result_offsets[VAL_SHELL_SIGX_OUT] = 14;
        fam->result_offsets[VAL_SHELL_SIGY_OUT] = 15;
        fam->result_offsets[VAL_SHELL_SIGZ_OUT] = 16;
        fam->result_offsets[VAL_SHELL_SIGXY_OUT] = 17;
        fam->result_offsets[VAL_SHELL_SIGYZ_OUT] = 18;
        fam->result_offsets[VAL_SHELL_SIGZX_OUT] = 19;
        fam->result_offsets[VAL_SHELL_EPS_EFF_OUT] = 20;
        fam->result_offsets[VAL_SHELL_RES1] = 21;
        fam->result_offsets[VAL_SHELL_RES2] = 22;
        fam->result_offsets[VAL_SHELL_RES3] = 23;
        fam->result_offsets[VAL_SHELL_RES4] = 24;
        fam->result_offsets[VAL_SHELL_RES5] = 25;
        fam->result_offsets[VAL_SHELL_RES6] = 26;
        fam->result_offsets[VAL_SHELL_RES7] = 27;
        fam->result_offsets[VAL_SHELL_RES8] = 28;
        fam->result_offsets[VAL_SHELL_THICKNESS] = 29;
        fam->result_offsets[VAL_SHELL_ELDEP1] = 30;
        fam->result_offsets[VAL_SHELL_ELDEP2] = 31;
	
	if ( nv2d == 33 )
            fam->result_offsets[VAL_SHELL_INT_ENG] = 32;
	
        if ( nv2d == 45 || nv2d == 46 )
        {
            fam->result_offsets[VAL_SHELL_EPSX_IN] = 32;
            fam->result_offsets[VAL_SHELL_EPSY_IN] = 33;
            fam->result_offsets[VAL_SHELL_EPSZ_IN] = 34;
            fam->result_offsets[VAL_SHELL_EPSXY_IN] = 35;
            fam->result_offsets[VAL_SHELL_EPSYZ_IN] = 36;
            fam->result_offsets[VAL_SHELL_EPSZX_IN] = 37;
            fam->result_offsets[VAL_SHELL_EPSX_OUT] = 38;
            fam->result_offsets[VAL_SHELL_EPSY_OUT] = 39;
            fam->result_offsets[VAL_SHELL_EPSZ_OUT] = 40;
            fam->result_offsets[VAL_SHELL_EPSXY_OUT] = 41;
            fam->result_offsets[VAL_SHELL_EPSYZ_OUT] = 42;
            fam->result_offsets[VAL_SHELL_EPSZX_OUT] = 43;
            fam->result_offsets[VAL_SHELL_INT_ENG] = 44;
        }
        seg = nel4*sizeof(float);
        loc += nv2d*seg;
	
	p_bp = NEW( Buffer_pool, "Shell buffer pool" );
        init_buffer_pool( p_bp, DFLT_BUFFER_POOL_SIZE, nel4 * nv2d );
        fam->shell_input = p_bp;
    }

    /* Activity data. */
    if ( activ )
    {
        if ( nel8 > 0 )
        {
            fam->result_offsets[VAL_HEX_ACTIVITY] = loc;
            loc += nel8*sizeof(float);
        }
        if ( nel2 > 0 )
        {
            fam->result_offsets[VAL_BEAM_ACTIVITY] = loc;
            loc += nel2*sizeof(float);
        }
        if ( nel4 > 0 )
        {
            fam->result_offsets[VAL_SHELL_ACTIVITY] = loc;
            loc += nel4*sizeof(float);
        }
    }

    /* Done, return number of states. */
    return fam->num_states;
}


/*****************************************************************
 * TAG( get_input_buffer_qty )
 *
 * Query the quantity of buffers used for data input.
 */
int
get_input_buffer_qty( object_type )
int object_type;
{
    switch ( object_type )
    {
	case NODE_T:
	    return cur_family->node_input->buffer_count;
	    break;
	case BRICK_T:
	    if ( cur_family->hex_input != NULL )
	        return cur_family->hex_input->buffer_count;
            else
	        return -1;
	    break;
	case SHELL_T:
	    if ( cur_family->shell_input != NULL )
	        return cur_family->shell_input->buffer_count;
            else
	        return -1;
	    break;
	case BEAM_T:
	    if ( cur_family->beam_input != NULL )
	        return cur_family->beam_input->buffer_count;
            else
	        return -1;
	    break;
	default:
	    return -1;
    }
}


/*****************************************************************
 * TAG( set_input_buffer_qty )
 *
 * Reset the quantity of buffers used for data input.
 */
void
set_input_buffer_qty( object_type, qty )
int object_type;
int qty;
{
    int nv1d, nv2d, nv3d, nvqty;
    int nel2, nel4, nel8, numnp;
    
    if ( object_type == NODE_T || object_type == ALL_OBJECT_T )
    {
        numnp   = cur_family->ctl[ 1];
        nvqty   = cur_family->node_nvar;
	init_buffer_pool( cur_family->node_input, qty, nvqty * numnp );
    }
    
    if ( object_type == BRICK_T || object_type == ALL_OBJECT_T )
    {
        nel8    = cur_family->ctl[ 8];
        nv3d    = cur_family->ctl[12];
	
        if ( nel8 > 0 )
            init_buffer_pool( cur_family->hex_input, qty, nv3d * nel8 );
    }
    
    if ( object_type == SHELL_T || object_type == ALL_OBJECT_T )
    {
        nel4    = cur_family->ctl[16];
        nv2d    = cur_family->ctl[18];
	
        if ( nel4 > 0 )
            init_buffer_pool( cur_family->shell_input, qty, nv2d * nel4 );
    }
    
    if ( object_type == BEAM_T || object_type == ALL_OBJECT_T )
    {
        nel2    = cur_family->ctl[13];
        nv1d    = cur_family->ctl[15];
	
        if ( nel2 > 0 )
            init_buffer_pool( cur_family->beam_input, qty, nv1d * nel2 );
    }
}


/*****************************************************************
 * TAG( init_buffer_pool )
 *
 * (Re-)Initialize a buffer pool.
 */
static void
init_buffer_pool( p_bp, bufqty, bufsize )
Buffer_pool *p_bp;
int bufqty;
int bufsize;
{
    float **p_tmp_dat, **p_swap_dat;
    int *p_tmp_st;
    int cnt;
    int i, j;
    Bool_type new_memory;
    int save, gone;

    /* 
     * Attempt to allocate memory first if necessary to capture any 
     * allocation failures before modifying existing structures.
     * 
     * Note that if changing a buffer size, this will result in both
     * the old buffers and the new ones being allocated 
     * simultaneously, and so could conceivably cause an alloc failure
     * unnecessarily.
     */
    new_memory = FALSE;
    if ( bufqty > p_bp->buffer_count || p_bp->buffer_size != bufsize )
    {
	cnt = bufqty - p_bp->buffer_count;
	p_tmp_dat = NEW_N( float *, cnt, "Temp input buffer array" );
	
	for ( i = 0; i < cnt; i++ )
	{
	    p_tmp_dat[i] = NEW_N( float, bufsize, "Input buffer" );
	    
	    if ( p_tmp_dat[i] == NULL )
	    {
	        /* Failure - clean-up and return. */
		
		popup_dialog( WARNING_POPUP, 
		              "Input buffer allocation failure;\n%s", 
			      "buffer queue not modified." );
		for ( j = 0; j < i; j++ )
		    free( p_tmp_dat[j] );
		free( p_tmp_dat );
		return;
	    }
	}
	
	new_memory = TRUE;
    }
    
    /* If buffer already used and buffer size changes or no buffering... */
    if ( p_bp->buffer_count != 0 
         && ( p_bp->buffer_size != bufsize || bufqty == 0 ) )
    {
	/* Free everything to start from scratch. */
	
	for ( i = 0; i < p_bp->buffer_count; i++ )
	    free( p_bp->data_buffers[i] );
	free( p_bp->data_buffers );
	free( p_bp->state_numbers );
	p_bp->buffer_count = 0;
    }
    
    if ( p_bp->buffer_count == 0 )
    {
	/* A clean Buffer_pool - create the queue. */
	
	p_bp->data_buffers = NEW_N( float *, bufqty, "Input buffer queue" );
	p_bp->state_numbers = NEW_N( int, bufqty, "Input queue states" );
	for ( i = 0; i < bufqty; i++ )
	{
            p_bp->data_buffers[i] = p_tmp_dat[i];
	    p_bp->state_numbers[i] = -1;
	}
	p_bp->new_count = bufqty;
	p_bp->buffer_count = bufqty;
	p_bp->buffer_size = bufsize;
	p_bp->recent = -1;
    }
    else if ( p_bp->buffer_count > bufqty )
    {
	/* Shrink the queue */
	
	/* Free the excess data buffers. */
	for ( i = 0, gone = p_bp->recent; i < p_bp->buffer_count - bufqty; i++ )
	{
	    gone = (gone + 1) % p_bp->buffer_count;
	    free( p_bp->data_buffers[gone] );
	    
	    if ( p_bp->state_numbers[gone] == -1 )
	        p_bp->new_count--;
	}
	
	/* Migrate remaining buffers to low end of queue. */
	
	p_swap_dat = NEW_N( float *, bufqty, "Temp input buffer array" );
	p_tmp_st = NEW_N( int, bufqty, "Temp buffer state array" );
	
	/* Move the keepers to temporary storage. */
	for ( i = 0, save = gone; i < bufqty; i++ )
	{
	    save = (save + 1) % p_bp->buffer_count;
	    
	    p_swap_dat[i] = p_bp->data_buffers[save];
	    p_tmp_st[i] = p_bp->state_numbers[save];
	}
	
	/* Now move them back to the Buffer_pool. */
	for ( i = 0; i < bufqty; i++ )
	{
	    p_bp->data_buffers[i] = p_swap_dat[i];
	    p_bp->state_numbers[i] = p_tmp_st[i];
	}
	
	free( p_swap_dat );
	free( p_tmp_st );
	
	/* Free up unused ends of queue arrays. */
	p_bp->data_buffers = RENEW_N( float *, p_bp->data_buffers, 
	                              p_bp->buffer_count, 
				      bufqty - p_bp->buffer_count, 
				      "Reduced input buffer queue" );
	p_bp->state_numbers = RENEW_N( int, p_bp->state_numbers, 
	                               p_bp->buffer_count, 
				       bufqty - p_bp->buffer_count, 
				       "Reduced input queue states" );
    }
    else if ( p_bp->buffer_count < bufqty )
    {
	/* Extend the queue. */
	
	cnt = bufqty - p_bp->buffer_count;
	
	p_bp->data_buffers = RENEW_N( float *, p_bp->data_buffers, 
	                              p_bp->buffer_count, cnt, 
				      "Extended input buffer queue" );
	p_bp->state_numbers = RENEW_N( int, p_bp->state_numbers, 
	                               p_bp->buffer_count, cnt, 
				       "Extended input queue states" );
	
	/* Move the previously allocated new buffers to the Buffer_pool. */
	for ( i = p_bp->buffer_count; i < bufqty; i++ )
	{
	    p_bp->data_buffers[i] = p_tmp_dat[i - p_bp->buffer_count];
	    p_bp->state_numbers[i] = -1;
	}
	
	p_bp->buffer_count = bufqty;
	p_bp->new_count = cnt;
    }
    else
    {
	popup_dialog( WARNING_POPUP, "Re-initializing with same buffer\n%s", 
	              "size and quantity is not implemented." );
    }
    
    if ( new_memory )
        free( p_tmp_dat );
}


/*****************************************************************
 * TAG( delete_buffer_pool )
 *
 * Free memory resources allocated to a Buffer_pool.
 */
static void
delete_buffer_pool( p_bp )
Buffer_pool *p_bp;
{
    int i;
    
    /* Sanity check. */
    if ( p_bp == NULL )
        return;

    for ( i = 0; i < p_bp->buffer_count; i++ )
	free( p_bp->data_buffers[i] );

    free( p_bp->data_buffers );
    free( p_bp->state_numbers );
    
    free( p_bp );
}


/*****************************************************************
 * TAG( close_family )
 *
 * Close the current family of MDG plot files.
 */
void
close_family()
{
    if ( cur_family != NULL )
    {
        fclose( cur_family->cur_file );

        free( cur_family->file_sz );
        free( cur_family->st_time );
        free( cur_family->st_file_num );
        free( cur_family->st_file_loc );
        free( cur_family->result_offsets );
	
        delete_buffer_pool( cur_family->node_input );
        delete_buffer_pool( cur_family->hex_input );
        delete_buffer_pool( cur_family->shell_input );
        delete_buffer_pool( cur_family->beam_input );

        DELETE( cur_family, family_list );

        cur_family = family_list;
    }
}


/*****************************************************************
 * TAG( pf_name )
 *
 * Get the plot file name for the given file number.
 */
static void
pf_name( fnum, fname )
int fnum;
char *fname;
{
    if ( fnum == 0 )
        strcpy( fname, cur_family->root_name );
    else if ( fnum < 100 )
        sprintf( fname, "%s%02d", cur_family->root_name, fnum );
    else
        sprintf( fname, "%s%03d", cur_family->root_name, fnum );
}


/*****************************************************************
 * TAG( set_family )
 *
 * Select the current family of MDG plot files to be read from.
 */
void
set_family( root_name )
char *root_name;
{
    Family *fam;

    for ( fam = family_list; fam != NULL; fam = fam->next )
        if ( strcmp( fam->root_name, root_name ) == 0 )
            cur_family = fam;
}


/*****************************************************************
 * TAG( set_fam_file )
 *
 * Select the current file number within the current family of files.
 */
static void
set_fam_file( file_num )
int file_num;
{
    char fname[50];

    if ( cur_family->cur_file_num != file_num )
    {
        fclose( cur_family->cur_file );
        pf_name( file_num, fname );

        if ( (cur_family->cur_file = fopen(fname, "r")) == NULL )
        {
            wrt_text( "Can't open file %s\n", fname );
            return;
        }

        cur_family->cur_file_num = file_num;
    }
}


/*****************************************************************
 * TAG( get_geom )
 *
 * Get the geometry data from a family of MDG plot files.
 */
Geom *
get_geom( geo )
Geom *geo;
{
    int numnp, ndim, nel8, nel2, nel4;
    int loc, fnum;
    float *tmp1;
    int *tmp2;
    int mem_amt;
    int i;

    ndim  = cur_family->ctl[0];
    numnp = cur_family->ctl[1];
    nel8  = cur_family->ctl[8];
    nel2  = cur_family->ctl[13];
    nel4  = cur_family->ctl[16];

    if ( geo == NULL )
        geo = mk_geom( numnp, nel8, nel4, nel2 );

    geo->dimension = ndim;

    fnum = 0;

    tmp1 = NEW_N( float, numnp*ndim, "Temp geom data" );
    loc = 64*sizeof( int );

    mdg_read( fnum, loc, numnp*ndim*sizeof(float), tmp1 );
    loc += numnp*ndim*sizeof(float);
    geo->nodes->cnt = numnp;
    for ( i = 0; i < numnp; i++ )
    {
        geo->nodes->xyz[0][i] = tmp1[ndim*i]; 
        geo->nodes->xyz[1][i] = tmp1[ndim*i+1];
        if ( ndim == 3 )
            geo->nodes->xyz[2][i] = tmp1[ndim*i+2];
    }

    free( tmp1 );

    mem_amt = MAX( MAX(9*nel8, 5*nel4), 6*nel2 );
    tmp2 = NEW_N( int, mem_amt, "Temp geom data" );

    if ( nel8 > 0 )
    {
        mdg_read( fnum, loc, 9*nel8*sizeof(int), tmp2 );
        loc += 9*nel8*sizeof(int);
        geo->bricks->cnt = nel8;
        for ( i = 0; i < nel8; i++ )
        {
            geo->bricks->nodes[0][i] = tmp2[9*i] - 1; 
            geo->bricks->nodes[1][i] = tmp2[9*i+1] - 1;
            geo->bricks->nodes[2][i] = tmp2[9*i+2] - 1;
            geo->bricks->nodes[3][i] = tmp2[9*i+3] - 1;
            geo->bricks->nodes[4][i] = tmp2[9*i+4] - 1;
            geo->bricks->nodes[5][i] = tmp2[9*i+5] - 1;
            geo->bricks->nodes[6][i] = tmp2[9*i+6] - 1;
            geo->bricks->nodes[7][i] = tmp2[9*i+7] - 1;
            geo->bricks->mat[i]      = tmp2[9*i+8] - 1;

            /* Stupidity check.  Needed by Doug Faux for ALE3D databases. */
            if ( geo->bricks->mat[i] < 0 || geo->bricks->mat[i] > 1000000 )
                geo->bricks->mat[i] = 0;
        }

        check_degen_bricks( geo->bricks );
    }

    if ( nel2 > 0 )
    {
        mdg_read( fnum, loc, 6*nel2*sizeof(int), tmp2 );
        loc += 6*nel2*sizeof(int);
        geo->beams->cnt = nel2;
        for ( i = 0; i < nel2; i++ )
        {
            geo->beams->nodes[0][i] = tmp2[6*i] - 1;
            geo->beams->nodes[1][i] = tmp2[6*i+1] - 1;
            geo->beams->nodes[2][i] = tmp2[6*i+2] - 1;
            geo->beams->nodes[3][i] = tmp2[6*i+3] - 1;
            geo->beams->nodes[4][i] = tmp2[6*i+4] - 1;
            geo->beams->mat[i]      = tmp2[6*i+5] - 1;

            /* Stupidity check. */
            if ( geo->beams->mat[i] < 0 || geo->beams->mat[i] > 1000000 )
                geo->beams->mat[i] = 0;
        }
    }

    if ( nel4 > 0 )
    {
        mdg_read( fnum, loc, 5*nel4*sizeof(int), tmp2 );
        loc += 5*nel4*sizeof(int);
        geo->shells->cnt = nel4;
        for ( i = 0; i < nel4; i++)
        {
            geo->shells->nodes[0][i] = tmp2[5*i] - 1; 
            geo->shells->nodes[1][i] = tmp2[5*i+1] - 1;
            geo->shells->nodes[2][i] = tmp2[5*i+2] - 1;
            geo->shells->nodes[3][i] = tmp2[5*i+3] - 1;
            geo->shells->mat[i]      = tmp2[5*i+4] - 1;

            /* Stupidity check. */
            if ( geo->shells->mat[i] < 0 || geo->shells->mat[i] > 1000000 )
                geo->shells->mat[i] = 0;
        }

        check_degen_shells( geo->shells );
    }

    free( tmp2 );

    /* Sleazy. */
    cur_family->geom_p = geo;

    return geo;
}


/*****************************************************************
 * TAG( get_state )
 *
 * Get state data from a family of MDG plot files.
 */
State *
get_state( st_num, st_ptr )
int st_num;
State *st_ptr;
{
    int ndim, numnp, nel8, nel2, nel4, nglbv, iu, activ;
    float *tmp, *x, *y, *z, *tmp_ptr;
    int fnum, loc, i; 

    /* If no states, just return node positions from geometry. */
    if ( cur_family->num_states <= 0 )
    {
        /* Allocate space for state if needed. */
        if ( st_ptr == NULL )
            st_ptr = mk_state( 0, 0, 0, 0, FALSE ); 

        /* Sleazy. */
        st_ptr->nodes = cur_family->geom_p->nodes;
        st_ptr->position_constant = TRUE;

        return( st_ptr );
    }

    /* Check for state number out-of-bounds. */
    if ( st_num < 0 || st_num >= cur_family->num_states )
    {
        popup_dialog( WARNING_POPUP, 
	              "Request to get_state, state number out of bounds!" );
        return( st_ptr );
    }

    ndim  = cur_family->ctl[0];
    numnp = cur_family->ctl[1];
    nel8  = cur_family->ctl[8];
    nel2  = cur_family->ctl[13];
    nel4  = cur_family->ctl[16];
    nglbv = cur_family->ctl[3];
    iu    = cur_family->ctl[5];

    if ( cur_family->ctl[20] >= 1000 && cur_family->ctl[20] <= 1005 )
        activ = TRUE;
    else
        activ = FALSE;

    /* Allocate space for state if needed. */
    if ( st_ptr == NULL )
    {
        /* Don't allocate space for node positions if they aren't present. */
        if ( iu )
            st_ptr = mk_state( numnp, nel8, nel4, nel2, activ ); 
        else
            st_ptr = mk_state( 0, nel8, nel4, nel2, activ ); 
    }

    st_ptr->state_id = st_num;
    st_ptr->time = cur_family->st_time[st_num];

    fnum = cur_family->st_file_num[st_num];

    /* Node positions. */
    if ( iu )
    {
        tmp = NEW_N( float, numnp*ndim, "Tmp state pos" );

        loc = cur_family->st_file_loc[st_num] +
              cur_family->result_offsets[VAL_NODE_POS];
        mdg_read( fnum, loc, numnp*ndim*sizeof(float), tmp );

        x = st_ptr->nodes->xyz[0];
        y = st_ptr->nodes->xyz[1];
        z = st_ptr->nodes->xyz[2];
        tmp_ptr = tmp;

        for ( i = 0; i < numnp; i++ )
        {
            x[i] = *tmp_ptr;
            tmp_ptr++;
            y[i] = *tmp_ptr;
            tmp_ptr++;
            if ( ndim == 3 )
            {
                z[i] = *tmp_ptr;
                tmp_ptr++;
            }
        }

        free( tmp );
    }
    else
    {
        /* Sleazy. */
        st_ptr->nodes = cur_family->geom_p->nodes;
        st_ptr->position_constant = TRUE;
    }

#ifdef DEBUG
/*
    wrt_text( "\nNodal data:\n" );
    for ( i = 0; i <= 10; i++ )
        wrt_text( "%d %f %f %f\n\n",i, st_ptr->nodes->xyz[0][i],
                  st_ptr->nodes->xyz[1][i], st_ptr->nodes->xyz[2][i] );
*/
#endif

    /* Hex data. */
    if ( activ && nel8 > 0 )
    {
        loc = cur_family->st_file_loc[st_num] +
              cur_family->result_offsets[VAL_HEX_ACTIVITY];
        mdg_read( fnum, loc, nel8*sizeof(float), st_ptr->bricks->activity );
    }

    /* Beam data. */
    if ( activ && nel2 > 0 )
    {
        loc = cur_family->st_file_loc[st_num] +
              cur_family->result_offsets[VAL_BEAM_ACTIVITY];
        mdg_read( fnum, loc, nel2*sizeof(float), st_ptr->beams->activity );
    }

    /* Shell data. */
    if ( activ && nel4 > 0 )
    {
        loc = cur_family->st_file_loc[st_num] +
              cur_family->result_offsets[VAL_SHELL_ACTIVITY];
        mdg_read( fnum, loc, nel4*sizeof(float), st_ptr->shells->activity );
    }

    return( st_ptr );
}


/*****************************************************************
 * TAG( get_result )
 *
 * Get result data for the specified state from a family of MDG
 * plot files.
 */
void
get_result( result_id, st_num, result_arr )
Result_type result_id;
int st_num;
float *result_arr;
{
    float *tmp_arr;
    int ndim, fnum, loc, jmp, i, stride, ixd;
    Buffer_pool *p_bp;
    int idx, next;
    Bool_type buffered;
    int offset, obj_qty, nvar;

    if ( cur_family->result_offsets[result_id] == -1 )
    {
        wrt_text( "Result (ID = %d) not in database.\n", result_id );
        return;
    }
    
    if ( is_nodal_result( result_id ) )
    {
	p_bp = cur_family->node_input;
	offset = (1 + cur_family->ctl[3]) * sizeof( float );
	obj_qty = cur_family->ctl[1];
    }
    else if ( is_hex_result( result_id ) )
    {
	p_bp = cur_family->hex_input;
	offset = cur_family->hex_offset;
	obj_qty = cur_family->ctl[8];
	nvar = cur_family->hex_nvar;
    }
    else if ( is_shell_result( result_id ) )
    {
	p_bp = cur_family->shell_input;
	offset = cur_family->shell_offset;
	obj_qty = cur_family->ctl[16];
	nvar = cur_family->shell_nvar;
    }
    else if ( is_beam_result( result_id ) )
    {
	p_bp = cur_family->beam_input;
	offset = cur_family->beam_offset;
	obj_qty = cur_family->ctl[13];
	nvar = cur_family->beam_nvar;
    }
    
    buffered = FALSE;

    if ( p_bp->buffer_count != 0 )
    {
        buffered = TRUE;
	
        /* Attempt to match requested state among buffered states. */
	for ( i = 0; i < p_bp->buffer_count; i++ )
            if ( p_bp->state_numbers[i] == st_num )
		break;
	    
	if ( i == p_bp->buffer_count )
	{
            /* State not in memory - read it into next buffer in queue. */
	    
	    /* Check for unused buffers first. */
            if ( p_bp->new_count > 0 )
            {
		/* Find an unused buffer. */
		for ( i = 0; i < p_bp->buffer_count; i++ )
		    if ( p_bp->state_numbers[i] == -1 )
		        break;
		    
		next = i;
		p_bp->new_count--;
            }
            else
		/* No unused buffers, so overwrite the one after "recent". */
		next = (p_bp->recent + 1) % p_bp->buffer_count;
	
	    /*
	     * Calculate location of data for requested object type and
	     * read into a buffer.
	     */
            fnum = cur_family->st_file_num[st_num];
            loc = cur_family->st_file_loc[st_num] + offset;
            mdg_read( fnum, loc, p_bp->buffer_size * sizeof( float ), 
		      p_bp->data_buffers[next] );
	    
	    /* Update pool. */
            p_bp->state_numbers[next] = st_num;
            p_bp->recent = next;
	    
	    /* Save buffer index. */
            idx = next;
	}
	else
	    /* Found requested state in memory. */
	    idx = i;
    }

    if ( result_id == VAL_NODE_TEMP )
    {
        /*
         * Temperature is a special case since it is currently
         * treated as a scalar, not a vector.  What a kluge.
         */
	
	if ( buffered )
	{
	    /* Copy data from buffer into result array. */
	    tmp_arr = p_bp->data_buffers[idx]
	              + (cur_family->result_offsets[VAL_NODE_TEMP] - offset)
		        / sizeof( float );
	    
	    memcpy( result_arr, tmp_arr, obj_qty * sizeof( float ) );
	}
	else
	{
	    /* No buffering - do the old thing */
            fnum = cur_family->st_file_num[st_num];

            /* Get the proper offset from the X value identifier. */
            loc = cur_family->st_file_loc[st_num] + 
                  cur_family->result_offsets[result_id];

            mdg_read( fnum, loc, obj_qty * sizeof(float), result_arr );
	}
    }
    else if ( is_nodal_result( result_id ) )
    {
        /* Nodal results are interleaved by vectors. */
        ndim = cur_family->ctl[0];
        fnum = cur_family->st_file_num[st_num];

        jmp = 0;
        if ( cur_family->result_offsets[result_id] == -2 )
            jmp = 1;
        else if ( cur_family->result_offsets[result_id] == -3 )
            jmp = 2;
	
	/* For turbulence results, stride is not a function of dimensionality */
	if ( result_id == VAL_NODE_K
	     || result_id == VAL_NODE_EPSILON
	     || result_id == VAL_NODE_A2 )
	{
	    ixd = cur_family->ctl[24];
	    stride = ( ixd & K_EPSILON_MASK ) ? 2 : 0;
	    stride += ( ixd & A2_MASK ) ? 1 : 0;
	}
	else
	    stride = ndim;
        
	if ( buffered )
	{
	    tmp_arr = p_bp->data_buffers[idx]
	              + (cur_family->result_offsets[result_id - jmp] - offset)
		        / sizeof( float );
	}
	else
	{
            /* Get the proper offset from the X value identifier. */
            loc = cur_family->st_file_loc[st_num] + 
                  cur_family->result_offsets[result_id - jmp];

            tmp_arr = NEW_N( float, stride * obj_qty, "Tmp read result arr" );
            mdg_read( fnum, loc, stride * obj_qty * sizeof(float), tmp_arr );
	}

        /* Copy data into result array. */
	for ( i = 0, loc = jmp; i < obj_qty; i++, loc += stride )
            result_arr[i] = tmp_arr[loc];
        if ( !buffered )
	    free( tmp_arr );
    }
    else
    {
        /* Element results are stored in element order, have to
         * to jump when reading.
         */
        
	if ( buffered )
	    tmp_arr = p_bp->data_buffers[idx];
	else
	{
            fnum = cur_family->st_file_num[st_num];
            loc = cur_family->st_file_loc[st_num] + offset;

            /* Read in whole data block, then extract the desired variable. */
            tmp_arr = NEW_N( float, nvar * obj_qty, "Tmp read result arr" );
            mdg_read( fnum, loc, nvar * obj_qty * sizeof(float), tmp_arr );
	}

        loc = cur_family->result_offsets[result_id];
        for ( i = 0; i < obj_qty; i++, loc += nvar )
            result_arr[i] = tmp_arr[loc];

        if ( !buffered )
            free( tmp_arr );
    }
}


/*****************************************************************
 * TAG( get_vec_result )
 *
 * Get result data for the specified state from a family of MDG
 * plot files.
 */
void
get_vec_result( result_id, st_num, arr_x, arr_y, arr_z )
Result_type result_id;
int st_num;
float *arr_x;
float *arr_y;
float *arr_z;
{
    char *err_msg = "Can't get vector result.\n";
    float *tmp_arr, *tmp_ptr;
    int ndim, sz, fnum, loc, jmp, i;

    if ( cur_family->result_offsets[result_id] == -1 )
    {
        wrt_text( "Result (ID = %d) not in database.\n", result_id );
        return;
    }

    if ( is_nodal_result( result_id ) )
    {
        /* Nodal results are interleaved by vectors. */
        ndim = cur_family->ctl[0];
        sz = cur_family->ctl[1];
        fnum = cur_family->st_file_num[st_num];

        if ( cur_family->result_offsets[result_id] < -1 )
            popup_fatal( err_msg );

        loc = cur_family->st_file_loc[st_num] + 
              cur_family->result_offsets[result_id];

        tmp_arr = NEW_N( float, ndim*sz, "Tmp read result arr" );
        mdg_read( fnum, loc, ndim*sz*sizeof(float), tmp_arr );

        for ( i = 0, tmp_ptr = tmp_arr; i < sz; i++ )
        {
            arr_x[i] = *tmp_ptr;
            tmp_ptr++;
            arr_y[i] = *tmp_ptr;
            tmp_ptr++;
            if ( ndim == 3 )
            {
                arr_z[i] = *tmp_ptr;
                tmp_ptr++;
            }
        }
        free( tmp_arr );
    }
    else
        popup_fatal( err_msg );
}


/*****************************************************************
 * TAG( is_in_database )
 *
 * Returns TRUE if the specified result is in the plotfile database,
 * FALSE otherwise.
 */
int
is_in_database( result_id )
Result_type result_id;
{
    if ( cur_family->result_offsets[result_id] == -1 )
        return( FALSE );
    else
        return( TRUE );
}


/*****************************************************************
 * TAG( mdg_read )
 *
 * Read data from a familied plot file.
 */
static void
mdg_read( file_num, file_pos, length, arr )
int file_num;
int file_pos;
int length;
char *arr;
{
    char *err_msg1 = "Can't seek in file";
    char *err_msg2 = "Can't read from file";
    char msgbuf[96];
    char nambuf[50];
    int buf_pos, offset, test;

    if ( length <= 0 )
        return;

    while ( file_pos >= cur_family->file_sz[file_num] )
    {
        file_pos -= cur_family->file_sz[file_num];
        file_num++;
    }

    set_fam_file( file_num );

    if ( fseek( cur_family->cur_file, file_pos, 0 ) != 0 )
    {
	pf_name( file_num, nambuf );
	sprintf( msgbuf, "%s %s.\n", err_msg1, nambuf );
        popup_fatal( msgbuf );
    }

    buf_pos = 0;
    while ( length > 0 )
    {
        if ( file_pos + length <= cur_family->file_sz[file_num] )
        {
            test = fread( &(arr[buf_pos]), sizeof(char), length,
                          cur_family->cur_file );
            if ( test < length )
	    {
		pf_name( file_num, nambuf );
		sprintf( msgbuf, "%s %s.\n", err_msg2, nambuf );
                popup_fatal( msgbuf );
	    }
	    length = 0;
        }
        else
        {
            offset = cur_family->file_sz[file_num] - file_pos;
            test = fread( &(arr[buf_pos]), sizeof(char), offset,
                          cur_family->cur_file );
            if ( test < offset )
	    {
		pf_name( file_num, nambuf );
		sprintf( msgbuf, "%s %s.\n", err_msg2, nambuf );
                popup_fatal( msgbuf );
	    }
            file_pos = 0;
            file_num++;
            set_fam_file( file_num );
            length -= offset;
            buf_pos += offset;
        }
    }
}


/*****************************************************************
 * TAG( fix_title )
 *
 * Fixes the screwy title from absolute files.
 */
static void
fix_title( title )
char *title;
{
    int j, loc;
 
    for ( loc = 0, j = 0; j <= 5; j++, loc++ )
        title[loc] = title[j];
    for ( j = 8; j <= 13; j++, loc++ )
        title[loc] = title[j];
    for ( j = 16; j <= 21; j++, loc++ )
        title[loc] = title[j];
    for ( j = 24; j <= 29; j++, loc++ )
        title[loc] = title[j];
    for ( j = 32; j <= 37; j++, loc++ )
        title[loc] = title[j];
    for ( j = 40; j <= 45; j++, loc++ )
        title[loc] = title[j];

    while ( isspace( title[loc-1] ) || !isprint( title[loc-1] ) )
        loc--;
    title[loc] = '\0';
}


/*****************************************************************
 * TAG( get_state_times )
 *
 * Returns all the state times in an array provided by the caller.
 */
void
get_state_times( state_times )
float *state_times;
{
    int i;
 
    for ( i = 0; i < cur_family->num_states; i++ )
        state_times[i] = cur_family->st_time[i];
}


/*****************************************************************
 * TAG( get_family_title )
 *
 * Copies the title card from the familied plotfile into the
 * argument string.
 */
void
get_family_title( title_str )
char *title_str;
{
    strcpy( title_str, cur_family->title );
}


/************************************************************
 * TAG( check_degen_bricks )
 *
 * Check for degenerate bricks (6-node wedges, 5-node pyramids
 * or 4-node tetrahedra) in the list of volume elements.  DYNA
 * specifies a special node ordering for these degenerate elements
 * which screws up the face drawing.  So this routine reorders the
 * nodes to a better ordering for face drawing.
 */
static void
check_degen_bricks( bricks )
Hex_geom *bricks;
{
    int i;

    if ( bricks == NULL )
        return;

    for ( i = 0; i < bricks->cnt; i++ )
    {
        if ( bricks->nodes[4][i] == bricks->nodes[5][i] &&
             bricks->nodes[5][i] == bricks->nodes[6][i] &&
             bricks->nodes[6][i] == bricks->nodes[7][i] )
        {
            if ( bricks->nodes[3][i] == bricks->nodes[4][i] )
            {
                /* Tetrahedral element. */
                bricks->nodes[3][i] = bricks->nodes[2][i];
                bricks->degen_elems = TRUE;
            }
            else
            {
                /* Pyramid element. */
                bricks->degen_elems = TRUE;
            }
        }
        else if ( bricks->nodes[4][i] == bricks->nodes[5][i] &&
                  bricks->nodes[6][i] == bricks->nodes[7][i] )
        {
            /* Prism-shaped (triangular wedge) element. */
            bricks->nodes[5][i] = bricks->nodes[6][i];
            bricks->nodes[7][i] = bricks->nodes[4][i];
            bricks->degen_elems = TRUE;
        }
    }
}


/************************************************************
 * TAG( check_degen_shells )
 *
 * Check for degenerate shell elements (3-node triangles instead
 * of 4-node quadrilaterals).  Reorder the nodes of triangular
 * shell elements so that the repeated node is last.
 */
static void
check_degen_shells( shells )
Shell_geom *shells;
{
    int tmp, shift, i, j, k;

    if ( shells == NULL )
        return;

    for ( i = 0; i < shells->cnt; i++ )
    {
        if ( shells->nodes[0][i] == shells->nodes[1][i] )
        {
            shift = 2;
            shells->degen_elems = TRUE; 
        }
        else if ( shells->nodes[1][i] == shells->nodes[2][i] )
        {
            shift = 1;
            shells->degen_elems = TRUE; 
        }
        else if ( shells->nodes[2][i] == shells->nodes[3][i] )
        {
            shift = 0;
            shells->degen_elems = TRUE; 
        }
        else if ( shells->nodes[0][i] == shells->nodes[3][i] )
        {
            shift = 3;
            shells->degen_elems = TRUE; 
        }
	else
	    continue;

        for ( j = 0; j < shift; j++ )
        {
            tmp = shells->nodes[3][i];
            for ( k = 3; k > 0; k-- )
                shells->nodes[k][i] = shells->nodes[k-1][i];
            shells->nodes[0][i] = tmp;
        }
    }
}


