/* $Id$ */
/*
 * viewer.c - MDG mesh viewer.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Dec 27 1991
 *
 * Copyright (c) 1991 Regents of the University of California
 */

/*                                                                      
 *                        L E G A L   N O T I C E                       
 *                                                                      
 *                 (C) Copyright 1991 the Regents of the                
 *          University of California. All Rights Reserved.              
 *                                                                      
 *   This work was produced at the University of California,  Lawrence  
 *   Livermore  National  Laboratory  (UC  LLNL) under contract no. W-  
 *   7405-ENG-48 (Contract 48) between the U.S. Department  of  Energy  
 *   (DOE)  and the Regents of the University of California (Universi-  
 *   ty) for the operation of UC LLNL.  Copyright is reserved  to  the  
 *   University  for the purposes of controlled dissemination, commer-  
 *   cialization through formal licensing, or other disposition  under  
 *   terms  of  Contract 48; DOE policies, regulations and orders; and  
 *   U.S. statutes.  The rights of the Federal Government are reserved  
 *   under  Contract 48 subject to the restrictions agreed upon by the  
 *   DOE and University as allowed under DOE Acquisition Letter 88-1.   
 *                                                                     
 *                          D I S C L A I M E R                         
 *                                                                      
 *   This computer code was prepared as an account of  work  sponsored  
 *   by an agency of the United States Government.  Neither the United  
 *   States Government nor the University of  California  nor  any  of  
 *   their  employees  makes  any warranty, express or implied, or as-  
 *   sumes any liability or responsibility for the accuracy, complete-  
 *   ness,  or  usefulness  of  any information, apparatus, product or  
 *   process disclosed or represents that its specific commercial pro-  
 *   ducts, process or service by trade name, trademark, manufacturer,  
 *   or otherwise does not necessarily constitute  or  imply  its  en-  
 *   dorsement,  recommendation,  or  favoring  by  the  United  State  
 *   Government or the University of California.  The views and  opin-  
 *   ions  of authors expressed herein do not necessarily state or re-  
 *   flect those of the United States Government or the University  of  
 *   California,  and shall not be used for advertising or product en-  
 *   dorsement purposes.                                                
 *                                                                    
 */

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "viewer.h"


extern char *getenv();

Environ env;

/* The current version of the code. */
char *griz_version = "2.0";

static void scan_args();
static void open_analysis();
static void usage();


main( argc, argv )
int argc;
char *argv[];
{
    Analysis *analy;

    /* Clear out the env struct just in case. */
    memset( &env, 0, sizeof( Environ ) );

    /* Scan command-line arguments, other initialization. */
    scan_args( argc, argv );

    /* run in the background by default to free shell window */
    if ( !env.foreground )
    {
        fprintf( stderr, "Main: putting GRIZ in background.\n" );
        switch( fork() )
        {
	    case 0:
	        break;
	    case -1:
	        fprintf( stderr, "Fork failed.  Running in foreground.\n" );
	        break;
	    default:
	        exit( 0 );
	}
    }

    /* Open the plotfile and initialize analysis data structure. */
    analy = NEW( Analysis, "Analysis struct" );
    open_analysis( env.plotfile_name, analy );

    /* Start up the command loop. */
    env.curr_analy = analy;
    gui_start( argc, argv );
}


/************************************************************
 * TAG( scan_args )
 *
 * Parse the program's command line arguments.
 */
static void
scan_args( argc, argv )
int argc;
char *argv[];
{
    extern Bool_type include_util_panel; /* From gui.c */
    int i;
    int inname_set;
    char *fontlib;
    time_t time_int;
    struct tm *tm_struct;
    char *name;

    inname_set = FALSE;

    if ( argc < 2 )
    {
        usage();
        exit( 0 );
    }

    for ( i = 1; i < argc; i++ )
    {
        if ( strcmp( argv[i], "-i" ) == 0 )
        {
            strcpy( env.plotfile_name, argv[i+1] );
            inname_set = TRUE;
            i++;
        }
        else if ( strcmp( argv[i], "-s" ) == 0 )
            env.single_buffer = TRUE;
	else if ( strcmp( argv[i], "-v" ) == 0 )
	    set_window_size( VIDEO_WIDTH, VIDEO_HEIGHT );
	else if ( strcmp( argv[i], "-w" ) == 0 )
	{
	    if ( i + 2 <= argc )
	    {
	        set_window_size( atoi( argv[i + 1] ), atoi( argv[i + 2] ) );
	        i += 2;
	    }
	    else
	    {
		usage();
		exit( 0 );
	    }
	}
	else if ( strcmp( argv[i], "-f" ) == 0 )
	    env.foreground = TRUE;
	else if ( strcmp( argv[i], "-u" ) == 0 )
	    include_util_panel = TRUE;
    }

    if ( !inname_set )
    {
        usage();
        exit( 0 );
    }

    /* Make sure font path is set. */
    fontlib = getenv( "HFONTLIB" );
    if ( fontlib == NULL )
    {
        wrt_text( "\nGRIZ error - path to font library not set.\n" );
        wrt_text( "Need to set environment variable HFONTLIB.\n\n" );
        wrt_text( "Example:\n" );
        wrt_text( "\tsetenv HFONTLIB /public/Griz/hfonts\n\n" );
        wrt_text( "This should go in your .cshrc file.\n\n" );
        exit( 1 );
    }

    /* Get today's date. */
    time_int = time( NULL );
    tm_struct = gmtime( &time_int );
    strftime( env.date, 19, "%D", tm_struct );

    /* Get user's name. */
    name = getenv( "LOGNAME" );
    strcpy( env.user_name, name );
}


/************************************************************
 * TAG( open_analysis )
 *
 * Open a plotfile and initialize an analysis.
 */
static void
open_analysis( fname, analy )
char *fname;
Analysis *analy;
{
    Shell_geom *shells;
    float bb_lo[3], bb_hi[3];
    int num_states;
    int i, j, k, sz;

    /* Open the familied file and read in the geom and first state. */
    num_states = open_family( fname );

    if ( num_states < 0 )
    {
        wrt_text( "Couldn't read data from file %s\n", fname );
        exit( -1 );
    }

    strcpy( analy->root_name, fname );
    analy->num_states = num_states;
    analy->state_times = NEW_N( float, num_states, "State times" );
    get_state_times( analy->state_times );
    get_family_title( analy->title );
    analy->geom_p = get_geom( NULL );
    analy->state_p = get_state( 0, NULL );
    analy->dimension = analy->geom_p->dimension;

    if ( analy->dimension == 2 )
        analy->normals_constant = TRUE;
    else
        analy->normals_constant = analy->state_p->position_constant &&
                                  !analy->state_p->activity_present;
    analy->cur_state = 0;
    analy->normals_smooth = TRUE;
    analy->render_mode = RENDER_HIDDEN;
    analy->interp_mode = REG_INTERP;
    analy->manual_backface_cull = TRUE;
    analy->float_frac_size = DEFAULT_FLOAT_FRACTION_SIZE;
    analy->auto_frac_size = TRUE;
    analy->show_colorscale = TRUE;
    analy->result_id = VAL_NONE;
    analy->use_global_mm = TRUE;
    analy->strain_variety = INFINITESIMAL;
    analy->ref_surf = MIDDLE;
    analy->ref_frame = GLOBAL;
    analy->th_filter_width = 1;
    analy->th_filter_type = BOX_FILTER;
    for ( i = 0; i < 3; i++ )
        analy->vec_id[i] = VAL_NONE;
    str_dup( &analy->bbox_source, "hsb" );
    analy->vec_scale = 1.0;
    analy->vec_head_scale = 1.0;
    analy->mouse_mode = MOUSE_HILITE;
    analy->result_on_refs = TRUE;
    analy->vec_cell_size = 1.0;
    analy->trace_width = 1.0;
    analy->vectors_at_nodes = TRUE;
    analy->conversion_scale = 1.0;
    analy->zbias_beams = TRUE;
    analy->beam_zbias = DFLT_ZBIAS;
    analy->edge_zbias = DFLT_ZBIAS;

    analy->result = NEW_N( float, analy->geom_p->nodes->cnt, "Result" );
    if ( analy->geom_p->beams != NULL )
        analy->beam_result = NEW_N( float, analy->geom_p->beams->cnt,
                                    "Beam result" );
    if ( analy->geom_p->shells != NULL )
        analy->shell_result = NEW_N( float, analy->geom_p->shells->cnt,
                                     "Shell result" );
    if ( analy->geom_p->bricks != NULL )
        analy->hex_result = NEW_N( float, analy->geom_p->bricks->cnt,
                                   "Brick result" );
    set_contour_vals( 6, analy );
    sz = analy->geom_p->nodes->cnt;
    if ( analy->geom_p->bricks != NULL && analy->geom_p->bricks->cnt > sz )
        sz = analy->geom_p->bricks->cnt;
    if ( analy->geom_p->shells != NULL && analy->geom_p->shells->cnt > sz )
        sz = analy->geom_p->shells->cnt;
    if ( analy->geom_p->beams != NULL && analy->geom_p->beams->cnt > sz )
        sz = analy->geom_p->beams->cnt;
    for ( i = 0; i < 6; i++ )
        analy->tmp_result[i] = NEW_N( float, sz, "Tmp result cache" );
    VEC_SET( analy->displace_scale, 1.0, 1.0, 1.0 );

/*
    debug_geom( analy->geom_p );
    debug_state( analy->state_p );
*/

    /* Count the number of materials for material display. */
    count_materials( analy );
    analy->hide_material = NEW_N( Bool_type, analy->num_materials,
                                 "Material visibility" );
    analy->disable_material = NEW_N( Bool_type, analy->num_materials,
                                     "Material-based result disabling" );
    for ( i = 0; i < 3; i++ )
        analy->mtl_trans[i] = NEW_N( float, analy->num_materials,
                                     "Material translations" );
    
    /* Use element blocks to speed up the adjacency table creation. */
    create_elem_blocks( analy );

    /* Create adjacency information and external face list. */
    if ( analy->geom_p->bricks != NULL )
    {
        /* Allocate hex_adj table and hex_visib table. */
        sz = analy->geom_p->bricks->cnt;
        for ( i = 0; i < 6; i++ )
            analy->hex_adj[i] = NEW_N( int, sz, "Hex adjacency table" );
        analy->hex_visib = NEW_N( Bool_type, sz, "Hex visibility table" );

        create_hex_adj( analy );
        set_hex_visibility( analy );
        get_hex_faces( analy );
        
        /* Still need to get face normals. */
    }

    /* Get the normals. */
    if ( analy->dimension == 3 )
    {
        /*
         * Space for face normals gets allocated by get_hex_faces().
         * Allocate the shell normals array.
         */
        shells = analy->geom_p->shells;
        if ( shells )
        {
            for ( i = 0; i < 4; i++ )
                for ( j = 0; j < 3; j++ )
                    analy->shell_norm[i][j] =
                            NEW_N( float, shells->cnt, "Shell normals" );
        }

        /*
         * Allocate the node normals working array.
         */
        sz = analy->geom_p->nodes->cnt;
        for ( i = 0; i < 3; i++ )
            analy->node_norm[i] = NEW_N( float, sz, "Node normals" );

        compute_normals( analy );
    }

    /* Compute an edge list for fast object display. */
    get_mesh_edges( analy );

    /* Initialize the translation table for derived variable ID's. */
    init_resultid_table();

    /* Estimate the number of polygons to be drawn. */
    sz = analy->face_cnt;
    if ( analy->geom_p->shells != NULL )
        sz += analy->geom_p->shells->cnt;

    if ( sz > 10000 )
        analy->render_mode = RENDER_FILLED;
}


/************************************************************
 * TAG( close_analysis )
 *
 * Free up all the memory absorbed by an analysis.
 */
static void
close_analysis( analy )
Analysis *analy;
{
    int i, j;

    free( analy->state_times );
    fr_geom( analy->geom_p );
    fr_state( analy->state_p );
    
    free( analy->bbox_source );

    if ( analy->geom_p->bricks != NULL )
    {
        for ( i = 0; i < 6; i++ )
            free( analy->hex_adj[i] );
        free( analy->hex_visib );
        free( analy->face_el );
        free( analy->face_fc );
    }
    if ( analy->dimension == 3 )
    {
        for ( i = 0; i < 3; i++ )
            free( analy->node_norm[i] );
        if ( analy->face_norm[0][0] != NULL )
            for ( i = 0; i < 4; i++ )
                for ( j = 0; j < 3; j++ )
                    free( analy->face_norm[i][j] );
        if ( analy->shell_norm[0][0] != NULL )
            for ( i = 0; i < 4; i++ )
                for ( j = 0; j < 3; j++ )
                    free( analy->shell_norm[i][j] );
    }
    if ( analy->block_lo != NULL )
    {
        free( analy->block_lo );
        free( analy->block_hi );
        for ( i = 0; i < 2; i++ )
            for ( j = 0; j < 3; j++ )
                free( analy->block_bbox[i][j] );
    }
    DELETE_LIST( analy->refl_planes );
    if ( analy->hide_material != NULL )
        free( analy->hide_material );
    if ( analy->disable_material != NULL )
        free( analy->disable_material );
    if ( analy->trace_disable != NULL )
        free( analy->trace_disable );
    analy->trace_disable_qty = 0;
    free_matl_exp();
    for ( i = 0; i < 3; i++ )
        if ( analy->mtl_trans[i] != NULL )
            free( analy->mtl_trans[i] );
    if ( analy->result != NULL )
        free( analy->result );
    if ( analy->beam_result != NULL )
        free( analy->beam_result );
    if ( analy->shell_result != NULL )
        free( analy->shell_result );
    if ( analy->hex_result != NULL )
        free( analy->hex_result );
    for ( i = 0; i < 6; i++ )
        free( analy->tmp_result[i] );
    DELETE_LIST( analy->result_mm_list );
    if ( analy->m_edges_cnt > 0 )
    {
        for ( i = 0; i < 2; i++ )
            free( analy->m_edges[i] );
        free( analy->m_edge_mtl );
    }
    if ( analy->contour_cnt > 0 )
        free( analy->contour_vals );
    free_contours( analy );
    free_trace_points( analy );
    DELETE_LIST( analy->isosurf_poly );
    DELETE_LIST( analy->cut_planes );
    DELETE_LIST( analy->cut_poly );
    
    destroy_mtl_mgr();
}


/************************************************************
 * TAG( load_analysis )
 *
 * Load in a new analysis from the specified plotfile.
 */
void
load_analysis( fname, analy )
char *fname;
Analysis *analy;
{
    FILE *infile;

    /* Check that filename is valid before doing anything. */
    if ( ( infile = fopen( fname, "r") ) == NULL )
    {
        wrt_text( "Couldn't open file %s.\n", fname );
        return;
    }
    else
        fclose( infile );

    set_alt_cursor( CURSOR_EXCHANGE );
    
    close_family();
    close_analysis( analy );
    memset( analy, 0, sizeof( Analysis ) );
    strcpy( env.plotfile_name, fname );
    open_analysis( fname, analy );
    reset_mesh_window( analy );
    reset_window_titles();
    
    unset_alt_cursor();
}


/************************************************************
 * TAG( open_history_file )
 *
 * Open a file for saving the command history.
 */
void
open_history_file( fname )
char *fname;
{
    char str[80];

    /* Close file if we had one open already. */
    close_history_file();

    if ( ( env.history_file = fopen(fname, "w") ) == NULL )
    {
        wrt_text( "Couldn't open file %s.\n", fname );
        return;
    }

    env.save_history = TRUE;
}

 
/************************************************************
 * TAG( close_history_file )
 *
 * Close the file for saving the command history.
 */
void
close_history_file()
{
    if ( env.save_history )
        fclose( env.history_file );

    env.save_history = FALSE;
}
 

/************************************************************
 * TAG( history_command )
 *
 * Save a command to the command history file.
 */
void
history_command( command_str )
char *command_str;
{
    if ( env.save_history )
        fprintf( env.history_file, "%s\n", command_str );
}


/************************************************************
 * TAG( read_history_file )
 *
 * Read in a command history file and execute the commands in it.
 */
void
read_history_file( fname, analy )
char *fname;
Analysis *analy;
{
    FILE *infile;
    char c, str[200];
    int i;

    if ( ( infile = fopen(fname, "r") ) == NULL )
    {
        wrt_text( "Couldn't open file %s.\n", fname );
        return;
    }

    while ( !feof( infile ) )
    {
        i = 0;
        c = getc( infile );
        while ( c != '\n' && !feof( infile ) && i < 199 )
        {
            str[i] = c;
            i++;
            c = getc( infile );
        }
        str[i] = '\0';

        if ( str[0] != '#' && str[0] != '\0' )
            parse_command( str, analy );
    }

    fclose( infile );
}


/************************************************************
 * TAG( usage )
 *
 * Write out GRIZ command-line syntax.
 */
static void
usage()
{
    static char *usage_text[] = {
"\nUsage:  griz -i input_base_name\n",
"            [-s]                          # single-buffer mode       #\n",
"            [-f]                          # run in foreground        #\n",
"            [-v]                          # size image for video     #\n",
"            [-w <width_pix> <height_pix>] # set exact image size     #\n", 
"            [-u]                          # util panel in ctrl win   #\n\n" };
    int i, line_cnt;

    line_cnt = sizeof( usage_text ) / sizeof( usage_text[0] );

    for ( i = 0; i < line_cnt; i++ )
        wrt_text( usage_text[i] );

    return;
}

