/* $Id$ */
/*
 * viewer.c - MDG mesh viewer.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Dec 27 1991
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
 ************************************************************************
 * Modifications:
 *  I. R. Corey - Dec 22, 2004: Add new function to display free nodes.
 *                See SRC# 292.
 *
 *  I. R. Corey - Jan  5, 2005: Added new option to allow for selecting
 *                a beta version of the code. This option (-beta) is 
 *                also parsed by the griz4s shell script.
 *                 See SRC# 292.
 *
 *  I. R. Corey - Jan 24, 2005: Added a new free_node option to disable
 *                mass scaling of the nodes.
 *                
 *  I. R. Corey - Feb 10, 2005: Added a new free_node option to enable
 *                volume disable scaling of the nodes.
 *
 *  I. R. Corey - May 19, 2005: Added new option to allow for selecting
 *                a alpha version of the code. This option (-alpha) is 
 *                also parsed by the griz4s shell script.
 *                See SCR #328.
 *
 *  J. K. Durrenberger
 *              - June 24, 2005 commented line 367 to allow the running 
 *                of the code in batch mode with no xterm available. When
 *                running as a cronjob or running with ssh with X11 
 *                tunneling disabled the software would try to instantiate
 *                a window and exit with an error at this point. 
 *
 *  I. R. Corey - October 24, 2006: Add a class selector to all vis/invis
 *                and enable/disable commands.
 *                See SRC#421.
 *
 *  I. R. Corey - May 15, 2007: Added more functionality to the session
 *                read-write function.
 *                See SRC#421.
 *
 *  I. R. Corey - May 15, 2007:	Save/Restore window attributes.
 *                See SRC#458.
 *
 *  I. R. Corey - Aug 09, 2007:	Fixed initialization bug with object
 *                selection arrays (enable/disable).
 *                See SRC#479.
 *
 *  I. R. Corey - Aug 15, 2007:	Added menus for displaying TI results.
 *                See SRC#480
 *
 *  I. R. Corey - Nov 08, 2007:	Add Node/Element labeling.
 *                See SRC#418 (Mili) and 504 (Griz)
 *
 *  I. R. Corey - Apr 18, 2008:	Allow for longer commands when reading
 *                from history file.
 *                See SRC#529
 *
 *  I. R. Corey - June 07, 2008: Force Griz to run in foreground when 
 *                running in batch mode so that all I/O is completed
 *                before Griz job returns to script.
 *                See SRC#539
 *
 *	J.K.Durrenberger - June 13, 2008 Added a check of the batch option 
 *								if no -b option was passed to griz we exit with an an 
 *								error.
 ************************************************************************
 */

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "viewer.h"
#include "mesh.h"

#ifdef SERIAL_BATCH
#include <GL/osmesa.h>
OSMesaContext OSMesa_ctx;

static void *offscreen;

Bool_type serial_batch_mode;

static void process_serial_batch_mode( char *batch_input_file_name, 
                                       Analysis *analy );
extern void init_app_context_serial_batch( int argc, char *argv[] );
extern int get_window_width();
extern int get_window_height();
extern void init_griz_session( Session * );


#else
/*
int     batch = 0;
int     bufsize;
void    *Buf;
void    *offscreen;

static  unsigned short Width  = DEFAULT_WIDTH;
static  unsigned short Height = DEFAULT_HEIGHT;

extern  void init_mesh_window( Analysis *analy );
extern  void init_btn_pick_classes();
*/
#endif /* SERIAL_BATCH */


Environ env;
    
Database_type db_type;

Session  *session;

char      *history_log[MAXHIST];
int        history_log_index;
Bool_type  history_inputCB_cmd;

/* The current version of the code. */
char *griz_version = GRIZ_VERSION;

/* Required short name for a particle G_UNIT class. */
char *particle_cname = "part";

static void scan_args( int argc, char *argv[], Analysis *analy );
static Bool_type open_analysis( char *fname, Analysis *analy );
static void usage( void );

extern void log_variable_scale( float, float, int, float *, float *, 
                                float *, float *, int * );

extern void
write_history_text( char *, Bool_type );


/************************************************************
 * TAG( main )
 *
 * Griz main routine.
 */
int
main( int argc, char *argv[] )
{
    Analysis *analy;

    Bool_type status;

    double result[500];
    int i;


#ifdef SERIAL_BATCH
    register int
                 rc;

    int osmesa_window_width;
    int osmesa_window_height;
#endif

    /* Clear out the env struct just in case. */
    memset( &env, 0, sizeof( Environ ) );

    /* Open the plotfiles and initialize analysis data structure. */
    analy   = NEW( Analysis, "Analysis struct" );
    session = NEW( Session,  "Session struct"  );

    env.win32 = FALSE;

    /* Scan command-line arguments, other initialization. */
    scan_args( argc, argv, analy );

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

    /* Initialize the Session Data structure */
    init_griz_session( session ) ;

#ifdef TIME_GRIZ
    manage_timer( 8, 0 );
#endif

#ifdef TIME_OPEN_ANALYSIS
    manage_timer( 1, 0 );
#endif

    env.ti_enable = TRUE;
    status = open_analysis( env.plotfile_name, analy );

#ifdef TIME_OPEN_ANALYSIS
    printf( "Open_analysis timing...\n" );
    manage_timer( 1, 1 );
#endif

    if ( !status )
        exit( 1 );

    env.curr_analy = analy;
    
    /* Assign these outside of open_analysis() so they have session lifetime. */
    init_plot_colors();

#ifdef SERIAL_BATCH

    if ( serial_batch_mode )
    {
        /*
         * perform required GRIZ and serial batch initializations
         * MUST do non-graphic initializations found in "init_mesh_window" 
         * to set up v_win, etc.
         */

        write_start_text();
	/**
	 * Commented this out to all run in batch mode when xterm is 
	 * unavailable.
         *  init_app_context_serial_batch( argc, argv );
	 */

        /*
         * establish offscreen rendering context needed by OSMesa
         *
         * NOTE:  window width and height may be set on GRIZ command line
         *        using parameter sequence:  -w <width> <height>
         */

        osmesa_window_width  = get_window_width();
        osmesa_window_height = get_window_height();

        if ( (rc = OffscreenContext( offscreen, osmesa_window_width, 
                                     osmesa_window_height, 0 )) < 0 )
        {
            /*
             * GUI's will NOT be available in batch mode, but errors and 
             * messages may be written to stdout/err.
             */

            popup_dialog( INFO_POPUP, 
                "Unable to create offscreen rendering context -- rc:  %d\n", 
                rc );
            quit( -1 );
        }


        process_serial_batch_mode( env.input_file_name, env.curr_analy );


        /*
         * Only get to this point via batch input file command(s):  quit, exit,
         * end, or if EOF condition encountered
         */

        free( offscreen );
        OSMesaDestroyContext( OSMesa_ctx );
        quit( 0 );
    }

#endif


    /* 
     * Initialize variables dealing with vis/invis and enable/disable.
     */
    MESH( analy ).mat_hide_qty            = 0;

    MESH( analy ).brick_hide_qty          = 0;
    MESH( analy ).brick_disable_qty       = 0;
    MESH( analy ).brick_exclude_qty       = 0;
    MESH( analy ).hide_brick_elem_qty     = 0;
    MESH( analy ).disable_brick_elem_qty  = 0;
    MESH( analy ).brick_result_min        = 0.;
    MESH( analy ).brick_result_max        = 0.;
    MESH( analy ).hide_brick_by_mat       = FALSE;
    MESH( analy ).hide_brick_by_result    = FALSE;

    MESH( analy ).beam_hide_qty          = 0;
    MESH( analy ).beam_disable_qty       = 0;
    MESH( analy ).beam_exclude_qty       = 0;
    MESH( analy ).hide_beam_elem_qty     = 0;
    MESH( analy ).disable_beam_elem_qty  = 0;
    MESH( analy ).beam_result_min        = 0.;
    MESH( analy ).beam_result_max        = 0.;
    MESH( analy ).hide_beam_by_mat       = FALSE;
    MESH( analy ).hide_beam_by_result    = FALSE;

    MESH( analy ).shell_hide_qty         = 0;
    MESH( analy ).shell_disable_qty      = 0;
    MESH( analy ).shell_exclude_qty      = 0;
    MESH( analy ).hide_shell_elem_qty    = 0;
    MESH( analy ).disable_shell_elem_qty = 0;
    MESH( analy ).shell_result_min       = 0.;
    MESH( analy ).shell_result_max       = 0.;
    MESH( analy ).hide_shell_by_mat       = FALSE;
    MESH( analy ).hide_shell_by_result    = FALSE;

    MESH( analy ).truss_hide_qty         = 0;
    MESH( analy ).truss_disable_qty      = 0;
    MESH( analy ).truss_exclude_qty      = 0;
    MESH( analy ).hide_truss_elem_qty    = 0;
    MESH( analy ).disable_truss_elem_qty = 0;
    MESH( analy ).truss_result_min       = 0.;
    MESH( analy ).truss_result_max       = 0.;
    MESH( analy ).hide_truss_by_mat       = FALSE;
    MESH( analy ).hide_truss_by_result    = FALSE;

    MESH( analy ).beam_hide_qty          = 0;
    MESH( analy ).beam_disable_qty       = 0;
    MESH( analy ).beam_exclude_qty       = 0;
    MESH( analy ).hide_beam_elem_qty     = 0;
    MESH( analy ).disable_beam_elem_qty  = 0;
    MESH( analy ).beam_result_min        = 0.;
    MESH( analy ).beam_result_max        = 0.;
    MESH( analy ).hide_beam_by_mat       = FALSE;
    MESH( analy ).hide_beam_by_result    = FALSE;

    MESH( analy ).particle_hide_qty      = 0;
    MESH( analy ).particle_disable_qty   = 0;
    MESH( analy ).hide_particle_by_result= FALSE;


    analy->ei_result_name[0] = '\0';
    analy->hist_paused       = FALSE;

    /* Initialize the history log */
    for ( i=0;
	  i<MAXHIST;
	  i++ )
          history_log[i] = (char *) NULL;

    /*
     * All other modes (at least for time being -- 03-10-00)
     */

    gui_start( argc, argv );

#ifdef TIME_GRIZ
    manage_timer( 8, 1 );
#endif
}


/************************************************************
 * TAG( scan_args )
 *
 * Parse the program's command line arguments.
 */
static void
scan_args( int argc, char *argv[], Analysis *analy )
{
  extern Bool_type include_util_panel, include_mtl_panel, new_window_behavior; /* From gui.c */
    int i;
    int inname_set;
    char *fontlib;
    time_t time_int;
    struct tm *tm_struct;

    char *name;

    inname_set = FALSE;

    /**** No Parallel TAURUS databases for now ****/
    env.parallel = FALSE;

    env.ti_enable = TRUE;

    if ( argc < 2 )
    {
        usage();
        exit( 0 );
    }

    for ( i = 1; i < argc; i++ )
    {
        /* Save a complete copy of the command line */
        strcat(env.command_line, " ");     
        strcat(env.command_line, argv[i]);        
       

        if ( strcmp( argv[i], "-i" ) == 0 )
        {
            /* Olga's very first bug fix */
            /* If i >= argc, then Griz just dumps core */
            /* User's don't like this and so now we check */

            i++;
            if ( i >= argc )
            {
                usage();
                exit( 0 );
            }

	    strcat(env.command_line, " ");     
	    strcat(env.command_line, argv[i]);        

            strcpy( env.plotfile_name, argv[i] );
            inname_set = TRUE;
        }
        else if ( strcmp( argv[i], "-s" ) == 0 )
            env.single_buffer = TRUE;
        else if ( strcmp( argv[i], "-v" ) == 0 )
            set_window_size( VIDEO_WIDTH, VIDEO_HEIGHT );
        else if ( strcmp( argv[i], "-V" ) == 0 )
        {
            VersionInfo();
            exit( 0 );
        }
        else if ( strcmp( argv[i], "-alpha" ) == 0 )
        {
            env.run_alpha_version = 1.0;
        }
        else if ( strcmp( argv[i], "-beta" ) == 0 )
        {
            env.run_beta_version = 1.0;
        }
        else if ( strcmp( argv[i], "-man" ) == 0 )
        {
	  parse_command( "help", analy );
	  exit( 0 );
        }
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

        else if ( strcmp( argv[i], "-win32" ) == 0 )
             env.win32 = TRUE;

#ifdef SERIAL_BATCH
       /*
        * do NOT establish utility panel for batch operations
        */
#else
        else if ( strcmp( argv[i], "-u" ) == 0 )
            include_util_panel = TRUE;
        else if ( strcmp( argv[i], "-m" ) == 0 )
            include_mtl_panel = TRUE;
        else if ( strcmp( argv[i], "-nw" ) == 0 )
            new_window_behavior = TRUE;
#endif

#ifdef SERIAL_BATCH
        else if ( strcmp( argv[i], "-b" ) == 0 ||
		  strcmp( argv[i], "-batch" ) == 0)
        {
	        i++;

            if ( i >= argc )
            {
                usage();
                exit( 0 );
	        }

            if ( NULL != argv[i] )
            {
	        strcat(env.command_line, " ");     
	        strcat(env.command_line, argv[i]);        

                strcpy( env.input_file_name, argv[i] );
                serial_batch_mode = TRUE;


		/* If in batch mode - force to run in
		 * foreground so all I/O is completed
		 * before Griz returns.
		 */
		env.foreground = TRUE;
            }
            else
            {
                usage();
                exit( 0 );
            }
        }
#endif
    }
#ifdef SERIAL_BATCH
		if(serial_batch_mode != TRUE)
		{
			fprintf( stderr, "Batch compiled code requires a -b option.\n" );
			usage();
			exit(-1);
		}
#endif
    if ( !inname_set )
    {
        usage();
        exit( 0 );
    }

    /* Make sure font path is set. */
    fontlib      = getenv( "HFONTLIB" );

    if ( fontlib == NULL )
        popup_fatal( "Set HFONTLIB env variable to font file location" );

    /* Get today's date. */
    time_int = time( NULL );
    env.time_start = time_int; /* Store Griz startup time */
    tm_struct = gmtime( &time_int );
    strftime( env.date, 19, "%D", tm_struct );

    /* Get user's name. */
    env.user_name[0]='\0';
    name = getenv( "LOGNAME" );
    if ( name!=NULL )
         strcpy( env.user_name, name );

    /* Get hostname */
    env.host[0]='\0';
    name = getenv( "HOST" );
    if ( name!=NULL )
         strcpy( env.host, name );

    /* Get arch */
    env.arch[0]='\0';
    name = getenv( "MACHTYPE" );
    if ( name!=NULL )
	 strcpy( env.arch, name );

    /* Get systype */
    env.systype[0]='\0';
    name = getenv( "SYS_TYPE" );
    if ( name!=NULL )
         strcpy( env.systype, name );

    /* Get logging file path */
    env.grizlogdir[0]='\0';
    name = getenv( "GRIZLOGDIR" );
    if ( name!=NULL )
         strcpy( env.grizlogdir, name );

    /* Get logging file name */
    env.grizlogfile[0]='\0';
    name = getenv( "GRIZLOGFILE" );
    if ( name!=NULL )
         strcpy( env.grizlogfile, name );

    /* Get logging file enable flag */
    name = getenv( "LOGFILE" );
    env.enable_logging = FALSE;
    if ( name!=NULL )
         if ( strcmp(name, "yes") == 0 )
	      env.enable_logging = TRUE;
}


/************************************************************
 * TAG( open_analysis )
 *
 * Open a plotfile and initialize an analysis.
 */
static Bool_type
open_analysis( char *fname, Analysis *analy )
{
    int num_states;
    int i, j, k, l;
    int stat;
    MO_class_data **class_array;
    MO_class_data *p_mocd;
    Mesh_data *p_md;
    Visibility_data *p_vd;
    Surface_data *p_surface;
    int class_qty;
    int mat_sz, max_sz, surface_sz, surf_max_sz;
    int surf, facets;
    int max_face_qty, face_qty;
    int cur_mesh_mat_qty;
    int cur_mesh_surf_qty;
    int srec_id;

    int brick_qty=0, shell_qty=0, truss_qty=0, beam_qty=0;

    /* TI Variables */
    int  num_entries = 0;
    int  superclass, datatype, datalength, meshid;
    int  state, matid;
    char *wildcard_list[1500], temp_name[128], class[32];
    Bool_type isMeshvar=FALSE;
    Bool_type isNodal=FALSE;
    int status; 
        /*
         * Don't use popup_fatal() on failure before db is actually open.
         */

        /* Initialize I/O functions by database type. */
        if ( !is_known_db( fname, &db_type ) )
        {
            popup_dialog( WARNING_POPUP, "Unable to identify database type." );
            return FALSE;
        }
        else if ( !init_db_io( db_type, analy ) )
        {
            popup_dialog( WARNING_POPUP, 
                          "Unable to initialize Griz I/O interface." );
            return FALSE;
        }

       /* Enable reading of TI data if requested */
        if ( env.ti_enable )
	     mc_ti_enable( analy->db_ident ) ;

        /* Open the database. */
        stat = analy->db_open( fname, &analy->db_ident );
        if ( stat != OK )
        {
            reset_db_io( db_type );
            popup_dialog( WARNING_POPUP, 
                          "Unable to initialize database during open." );
            return FALSE;
        }

        strcpy( analy->root_name, fname );

        stat = analy->db_get_title( analy->db_ident, analy->title );
        if ( stat != OK )
            popup_dialog( INFO_POPUP, "open_analysis() - unable to load "
                          "title." );

        stat = analy->db_get_dimension( analy->db_ident, &analy->dimension );
        if ( stat != OK )
        {
            popup_fatal( "open_analysis() - unable to get db dimension." );
            return FALSE;
        }

        if ( analy->dimension == 2 )
            analy->limit_rotations = TRUE;

        stat = analy->db_get_geom( analy->db_ident, &analy->mesh_table,
                                   &analy->mesh_qty );
        if ( stat != OK )
        {
            /*
             * When Griz doesn't have to be started with a db identified,
             * do something friendly to get back to a wait state.  
             * Until then...
             */
            return FALSE;
        }

        stat = analy->db_get_st_descriptors( analy, analy->db_ident );
        if ( stat != OK )
            return FALSE;
       
        stat = analy->db_set_results( analy );
        if ( stat != OK )
            return FALSE;

#ifdef TIME_OPEN_ANALYSIS
        printf( "Timing initial state get...\n" );
        manage_timer( 0, 0 );
#endif

        stat = analy->db_get_state( analy, 0, analy->state_p, &analy->state_p, 
                                    &num_states );
        if ( stat != 0 )
            return FALSE;

#ifdef TIME_OPEN_ANALYSIS
        manage_timer( 0, 1 );
        putc( (int) '\n', stdout );
#endif

        if ( num_states > 0 )
        {
            srec_id = analy->state_p->srec_id;
            analy->cur_mesh_id = 
                analy->srec_tree[srec_id].subrecs[0].p_object_class->mesh_id;
        }
        else
            analy->cur_mesh_id = 0;

        if ( analy->dimension == 2 )
            analy->normals_constant = TRUE;
        else
            analy->normals_constant = analy->state_p->position_constant &&
                                      !analy->state_p->sand_present;

    analy->result_source = ALL;
    analy->cur_state = 0;
    analy->refresh = TRUE;
    analy->normals_smooth = TRUE;
    analy->render_mode = RENDER_MESH_VIEW;
    analy->mesh_view_mode = RENDER_HIDDEN;
    analy->interp_mode = REG_INTERP;
    analy->manual_backface_cull = TRUE;
    analy->float_frac_size = DEFAULT_FLOAT_FRACTION_SIZE;
    analy->auto_frac_size = TRUE;
    analy->hidden_line_width = 1.0;
    analy->show_colorscale = TRUE;
    analy->use_global_mm = TRUE;
    analy->strain_variety = INFINITESIMAL;
    analy->ref_surf = MIDDLE;
    analy->ref_frame = GLOBAL;
    analy->show_plot_grid = TRUE;
    analy->show_plot_coords = TRUE;
    analy->color_plots = TRUE;
    analy->minmax_loc[0] = 'u';
    analy->minmax_loc[1] = 'l';
    analy->th_filter_width = 1;
    analy->th_filter_type = BOX_FILTER;
    if ( num_states > 0 )
        analy->show_particle_class = FALSE;
        create_class_list( &analy->bbox_source_qty, &analy->bbox_source,
                           MESH_P( analy ), 5, G_TRUSS, G_BEAM, G_TRI, G_QUAD,
                           G_HEX );
        if ( analy->bbox_source == NULL )
        {
            popup_fatal( "Failed to find object classes for initial bbox." );
            return FALSE;
        }
    analy->vec_scale = 1.0;
    analy->vec_head_scale = 1.0;
    analy->mouse_mode = MOUSE_HILITE;
    analy->result_on_refs = TRUE;
/*    analy->vec_cell_size = 1.0; */
    analy->contour_width = 1.0;
    analy->trace_width = 1.0;
    analy->vectors_at_nodes = TRUE;
    analy->conversion_scale = 1.0;
    analy->zbias_beams = TRUE;
    analy->beam_zbias = DFLT_ZBIAS;
    analy->edge_zbias = DFLT_ZBIAS;
    analy->edge_width = 1.0;
    analy->z_buffer_lines = TRUE;
/*    
    for ( i = 0; i < MAXSELECT - 1; i++ )
        analy->select_objects[i] = i + 1;
    analy->select_objects[MAXSELECT - 1].ident = -1;
*/
    analy->hist_line_cnt = 0;
                
    set_contour_vals( 6, analy );

    VEC_SET( analy->displace_scale, 1.0, 1.0, 1.0 );
    
    /*
     * Loop over meshes to complete mesh-specific initializations.
     */

    mat_sz = 0;
    max_sz = 0;
    surf_max_sz = 0;

        max_face_qty = 0;

        for ( i = 0; i < analy->mesh_qty; i++ )
        {
            p_md = analy->mesh_table + i;
        
            cur_mesh_mat_qty = 0;
            cur_mesh_surf_qty = 0;
        
            class_qty = htable_get_data( p_md->class_table, 
                                        (void ***) &class_array );
        
            /*
             * Make an initial pass to process G_MAT classes.  This generates
             * data necessary for element classes.
             */
            surface_sz         = 0;
            p_md->brick_qty    = 0;      
            p_md->shell_qty    = 0;      
            p_md->beam_qty     = 0;      
            p_md->truss_qty    = 0;      
            p_md->particle_qty = 1;  /* Force particle allocation for now since we
                                      * do not have a superclass for particles
                                      */

            for ( j = 0; j < class_qty; j++ )
            {
		p_mocd = class_array[j];
                
		/* Skip particle sub-classes */
		if ( !strcmp(p_mocd->short_name, "particle") )
		     continue;

                if ( p_mocd->superclass == G_MAT )
                {
                    if ( p_mocd->qty > mat_sz )
                        mat_sz = p_mocd->qty;
                    if ( mat_sz > max_sz )
                        max_sz = mat_sz;

                    /*
                     * Should only be one mat class, but be defensive...
                     * If there's more than one class, we'll have problems
                     * because references to materials for elements assume
                     * a single material class, (i.e., the material numbers
                     * only identify a material, not a class).
                     */
                    cur_mesh_mat_qty += p_mocd->qty;
                }
            
                if ( p_mocd->superclass == G_SURFACE )
                {
                    if ( p_mocd->qty > surface_sz )
                        surface_sz = p_mocd->qty;
                    if ( surface_sz > surf_max_sz )
                        surf_max_sz = surface_sz;

                    cur_mesh_surf_qty += p_mocd->qty;
                }

                if ( p_mocd->superclass == G_QUAD )
		{
                     p_md->shell_qty = p_mocd->qty;
		     shell_qty      +=  p_md->shell_qty;
		}             
                if ( p_mocd->superclass == G_HEX )
		{
                     p_md->brick_qty = p_mocd->qty;  
		     brick_qty      +=  p_md->brick_qty;
		}             
                if ( p_mocd->superclass == G_BEAM )
		{
                     p_md->beam_qty = p_mocd->qty;  
		     beam_qty      += p_md->beam_qty;
		}            
                if ( p_mocd->superclass == G_TRUSS )
		{
                     p_md->truss_qty = p_mocd->qty;   
		     truss_qty      += p_md->truss_qty;
		}           
            }
       
            /* Allocate material quantity-driven arrays for mesh. */ 
            p_md->hide_material = NEW_N( unsigned char, cur_mesh_mat_qty,
                                         "Material visibility" );
            p_md->disable_material = NEW_N( unsigned char, cur_mesh_mat_qty,
                                            "Material-based result disabling" );

            p_md->hide_brick    = NULL;
            p_md->disable_brick = NULL;
            if ( p_md->brick_qty >0 )
            {
                 p_md->hide_brick = NEW_N( unsigned char, cur_mesh_mat_qty,
                                           "Brick visibility" );
                 p_md->disable_brick = NEW_N( unsigned char, cur_mesh_mat_qty,
                                              "Brick-based result disabling" );
                 p_md->exclude_brick = NEW_N( unsigned char, cur_mesh_mat_qty,
                                              "Brick-based result exclusion" );

		 /* Allocate space for the per element qualtities */
                 p_md->hide_brick_elem = NEW_N( unsigned char, brick_qty,
                                                "Brick element visibility" );
                 p_md->disable_brick_elem = NEW_N( unsigned char, brick_qty,
                                                   "Brick-based element result disabling" );
                 p_md->exclude_brick_elem = NEW_N( unsigned char, brick_qty,
                                                   "Brick-based element result exclution" );

		 /* Initialize brick arrays */

		 for (i=0;
		      i<brick_qty;
		      i++)
		 {
		      p_md->hide_brick_elem[i]    = '\0';
		      p_md->disable_brick_elem[i] = '\0';
		      p_md->exclude_brick_elem[i] = '\0';
                 }
           }   

            p_md->hide_shell    = NULL;
            p_md->disable_shell = NULL;
            if ( p_md->shell_qty >0 )
            {
                 p_md->hide_shell = NEW_N( unsigned char, cur_mesh_mat_qty,
                                           "Shell visibility" );
                 p_md->disable_shell = NEW_N( unsigned char, cur_mesh_mat_qty,
                                              "Shell-based result disabling" );
                 p_md->exclude_shell = NEW_N( unsigned char, cur_mesh_mat_qty,
                                              "Shell-based result exclusion" );

		 /* Allocate space for the per element qualtities */
                 p_md->hide_shell_elem = NEW_N( unsigned char, shell_qty,
                                                "Shell element visibility" );
                 p_md->disable_shell_elem = NEW_N( unsigned char, shell_qty,
                                                   "Shell-based element result disabling" );
                 p_md->exclude_shell_elem = NEW_N( unsigned char, shell_qty,
                                                   "Shell-based element result exclution" );
		 /* Initialize shell arrays */

		 for (i=0;
		      i<shell_qty;
		      i++)
		 {
		      p_md->hide_shell_elem[i]     = '\0';
		      p_md->disable_shell_elem[i]  = '\0';
		      p_md->exclude_shell_elem[i]  = '\0';
                 }
             }   

            p_md->hide_beam    = NULL;
            p_md->disable_beam = NULL; 
            if ( p_md->beam_qty >0 )
            {
                 p_md->hide_beam = NEW_N( unsigned char, cur_mesh_mat_qty,
                                           "Beam visibility" );
                 p_md->disable_beam = NEW_N( unsigned char, cur_mesh_mat_qty,
                                              "Beam-based result disabling" );
                 p_md->exclude_beam = NEW_N( unsigned char, cur_mesh_mat_qty,
                                             "Beam-based result exclusion" );

		 /* Allocate space for the per element qualtities */
                 p_md->hide_beam_elem = NEW_N( unsigned char, beam_qty,
                                                "Beam element visibility" );
                 p_md->disable_beam_elem = NEW_N( unsigned char, beam_qty,
                                                   "Beam-based element result disabling" );
                 p_md->exclude_beam_elem = NEW_N( unsigned char, beam_qty,
                                                   "Beam-based element result exclution" );

		 /* Initialize beam arrays */

		 for (i=0;
		      i<beam_qty;
		      i++)
		 {
		      p_md->hide_beam_elem[i]     = '\0';
		      p_md->disable_beam_elem[i]  = '\0';
		      p_md->exclude_beam_elem[i]  = '\0';
                 }
             }   


            p_md->hide_truss    = NULL;
            p_md->disable_truss = NULL; 
            if ( p_md->truss_qty >0 )
            {
                 p_md->hide_truss = NEW_N( unsigned char, cur_mesh_mat_qty,
                                           "Truss visibility" );
                 p_md->disable_truss = NEW_N( unsigned char, cur_mesh_mat_qty,
                                              "Truss-based result disabling" );
                 p_md->exclude_truss = NEW_N( unsigned char, cur_mesh_mat_qty,
                                              "Truss-based result exclusion" );

		 /* Allocate space for the per element qualtities */
                 p_md->hide_truss_elem = NEW_N( unsigned char, truss_qty,
                                                "Truss element visibility" );
                 p_md->disable_truss_elem = NEW_N( unsigned char, truss_qty,
                                                   "Truss-based element result disabling" );
                 p_md->exclude_truss_elem = NEW_N( unsigned char, truss_qty,
                                                   "Truss-based element result exclution" );

		 /* Initialize truss arrays */

		 for (i=0;
		      i<truss_qty;
		      i++)
		 {
		      p_md->hide_truss_elem[i]     = '\0';
		      p_md->disable_truss_elem[i]  = '\0';
		      p_md->exclude_truss_elem[i]  = '\0';
                 }
            }   

            p_md->hide_particle    = NULL;
            p_md->disable_particle = NULL; 
            if ( p_md->particle_qty >0 )
            {
                 p_md->hide_particle = NEW_N( unsigned char, cur_mesh_mat_qty,
                                              "Particle visibility" );
                 p_md->disable_particle = NEW_N( unsigned char, cur_mesh_mat_qty,
                                                 "Particle-based result disabling" );
            }   


	    /* Initialize per-material arrays */

	    for (i=0;
		 i<cur_mesh_mat_qty;
		 i++)
	    {
	        if ( p_md->brick_qty>0)
		{
	  	     p_md->hide_brick[i]     = '\0';
		     p_md->disable_brick[i]  = '\0';
		     p_md->exclude_brick[i]  = '\0';
		}

	        if ( p_md->shell_qty>0)
		{
		     p_md->hide_shell[i]     = '\0';
		     p_md->disable_shell[i]  = '\0';
		     p_md->exclude_shell[i]  = '\0';
		}

	        if ( p_md->beam_qty>0)
		{
	       	     p_md->hide_beam[i]     = '\0';
		     p_md->disable_beam[i]  = '\0';
		     p_md->exclude_beam[i]  = '\0';
		}

	        if ( p_md->truss_qty>0)
		{
		     p_md->hide_truss[i]     = '\0';
		     p_md->disable_truss[i]  = '\0';
		     p_md->exclude_truss[i]  = '\0';
		}

	        if ( p_md->particle_qty>0)
		{
		     p_md->hide_particle[i]     = '\0';
		     p_md->disable_particle[i]  = '\0';
		}
	    }
	    
            for ( i = 0; i < 3; i++ )
                  p_md->mtl_trans[i] = NEW_N( float, cur_mesh_mat_qty,
                                              "Material translations" );
            p_md->material_qty = cur_mesh_mat_qty;


            analy->free_nodes_mats = NEW_N( int, cur_mesh_mat_qty,
                                            "Free Node material list" );
            memset( analy->free_nodes_mats, 0, cur_mesh_mat_qty );
            analy->free_nodes_scale_factor      = 1.0;
            analy->free_nodes_sphere_res_factor = 2;
            analy->free_nodes_mass_scaling      = 0; /* Default=Disabled */
            analy->free_nodes_vol_scaling       = 0; /* Default=Disabled */

#ifdef TIME_OPEN_ANALYSIS
            printf( "Timing face table creation...\n" );
            manage_timer( 0, 0 );
#endif

            face_qty = 0;
        
            /* Now handle the other classes. */
            for ( j = 0; j < class_qty; j++ )
            {
                p_mocd = class_array[j];
            
                /* Class-specific initializations.
                 *
                 * Face tables are created for each volumetric element 
                 * class.  Since this won't cull faces shared between elements
                 * of the same superclass but different classes, it may be 
                 * preferable to create a table only for each volumetric 
                 * element superclass, merging classes derived from the same 
                 * superclass.  That would require additional structures to 
                 * map elements unique within a superclass to their class-
                 * specific identities, and may not be worth the overhead 
                 * if meshes typically have only one class per superclass.
                 */
                switch ( p_mocd->superclass )
                {
                    case G_UNIT:
                    case G_NODE:
                    case G_TRUSS:
                    case G_BEAM:
                        if ( p_mocd->qty > max_sz )
                            max_sz = p_mocd->qty;
                        break;

                    case G_TRI:
                        if ( p_mocd->qty > max_sz )
                            max_sz = p_mocd->qty;

                        create_tri_blocks( p_mocd, analy );

                        p_vd = NEW( Visibility_data, 
                                    "Elem class Visibility_data" );
                        p_mocd->p_vis_data = p_vd;
                       
                        face_qty += p_mocd->qty;
                    
                        /* Allocate space for normals. */
                        if ( analy->dimension == 3 )
                            for ( k = 0; k < 3; k++ )
                                for ( l = 0; l < 3; l++ )
                                    p_vd->face_norm[k][l] =
                                        NEW_N( float, p_mocd->qty, 
                                               "Tri normals" );

                        break;

                    case G_QUAD:
                        if ( p_mocd->qty > max_sz )
                            max_sz = p_mocd->qty;

                        create_quad_blocks( p_mocd, analy );

                        p_vd = NEW( Visibility_data, 
                                    "Elem class Visibility_data" );
                        p_mocd->p_vis_data = p_vd;
                       
                        face_qty += p_mocd->qty;
                    
                        /* Allocate space for normals. */
                        if ( analy->dimension == 3 )
                            for ( k = 0; k < 4; k++ )
                                for ( l = 0; l < 3; l++ )
                                    p_vd->face_norm[k][l] =
                                        NEW_N( float, p_mocd->qty, 
                                               "Quad normals" );

                        break;

                    case G_TET:
                        if ( p_mocd->qty > max_sz )
                            max_sz = p_mocd->qty;

                        create_tet_blocks( p_mocd, analy );

                        p_vd = NEW( Visibility_data, 
                                    "Elem class Visibility_data" );
                        p_mocd->p_vis_data = p_vd;

                        p_vd->adjacency = NEW_N( int *, 4, 
                                                 "Tet adjacency array" );
                        for ( k = 0; k < 4; k++ )
                            p_vd->adjacency[k] = NEW_N( int, p_mocd->qty,
                                                        "Tet adjacency" );
                        p_vd->visib = NEW_N( unsigned char, p_mocd->qty,
                                             "Tet visibility" );

                        create_tet_adj( p_mocd, analy );
                        set_tet_visibility( p_mocd, analy );
                        get_tet_faces( p_mocd, analy );
                       
                        face_qty += p_mocd->p_vis_data->face_cnt;

                        break;

                    case G_HEX:
                        if ( p_mocd->qty > max_sz )
                            max_sz = p_mocd->qty;

                        create_hex_blocks( p_mocd, analy );

                        p_vd = NEW( Visibility_data, 
                                    "Elem class Visibility_data" );
                        p_mocd->p_vis_data = p_vd;

                        p_vd->adjacency = NEW_N( int *, 6, 
                                                 "Hex adjacency array" );
                        for ( k = 0; k < 6; k++ )
                            p_vd->adjacency[k] = NEW_N( int, p_mocd->qty,
                                                        "Hex adjacency" );
                        p_vd->visib = NEW_N( unsigned char, p_mocd->qty,
                                             "Hex visibility" );

                        p_vd->visib_damage = NEW_N( unsigned char, p_mocd->qty,
                                             "Hex Damage visibility" );
                        init_hex_visibility( p_mocd, analy );

                        create_hex_adj( p_mocd, analy );
                        set_hex_visibility( p_mocd, analy );
                        get_hex_faces( p_mocd, analy );
                       
                        face_qty += p_mocd->p_vis_data->face_cnt;

                        break;


                    case G_SURFACE:

                        for ( k = 0; k < p_mocd->qty; k++ )
                            max_sz += p_mocd->qty;

                        face_qty += p_mocd->qty;

                        for( surf = 0; surf < p_mocd->qty; surf++ )
                        {
                            p_surface = p_mocd->objects.surfaces + surf;
                            facets = p_surface->facet_qty;

                            p_vd = NEW( Visibility_data,
                                        "Surface Visibility_data" );
                            p_surface->p_vis_data = p_vd;

                            /* Allocate space for normals. */
                            for ( k = 0; k < 4; k++ )
                                for ( l = 0; l < 3; l++ )
                                    p_vd->face_norm[k][l] =
                                        NEW_N( float, facets, "Surface normals" );
                        }

                        /* Allocate surface quantity-driven arrays for mesh. */ 
                        p_md->hide_surface = NEW_N( unsigned char,
                                                    cur_mesh_surf_qty,
                                                    "Surface visibility" );
                        p_md->disable_surface = NEW_N( unsigned char,
                                                       cur_mesh_surf_qty,
                                                       "Surface-based result disabling" );
                        p_md->surface_qty = cur_mesh_surf_qty;

                        break;

                    default:
                        ;
                }
            }

#ifdef TIME_OPEN_ANALYSIS
            manage_timer( 0, 1 );
            putc( (int) '\n', stdout );
#endif
        
            free( class_array );
        }
       
        if ( face_qty > max_face_qty )
            max_face_qty = face_qty;
    
    /* Save max quantity of materials. */
    analy->max_mesh_mat_qty = mat_sz;
    
    /* Save max quantity of surfaces. */
    analy->max_mesh_surf_qty = surface_sz;
    
    /* 
     * Allocate space for temporary results. Allocate as a contiguous
     * array so the space can do double duty as storage for aggregate
     * primals of up to six components.
     */
#ifdef NEW_TMP_RESULT_USE
    init_data_buffers( max_sz );
#endif
#ifndef NEW_TMP_RESULT_USE
    analy->tmp_result[0] = NEW_N( float, TEMP_RESULT_ARRAY_QTY * max_sz,
                                  "Tmp result cache" );
    for ( i = 1; i < TEMP_RESULT_ARRAY_QTY ; i++ )
        analy->tmp_result[i] = analy->tmp_result[i - 1] + max_sz;
#endif
    analy->max_result_size = max_sz;

    /* Get the normals. */
    if ( analy->dimension == 3 )
    {
        /* Allocate the node normals working array. */
        for ( i = 0; i < 3; i++ )
            analy->node_norm[i] = NEW_N( float, max_sz, "Node normals" );

#ifdef TIME_OPEN_ANALYSIS
       printf( "Timing normals computation...\n" );
       manage_timer( 0, 0 );
#endif

        compute_normals( analy );

#ifdef TIME_OPEN_ANALYSIS
       manage_timer( 0, 1 );
       putc( (int) '\n', stdout );
#endif
    }

    /* Compute an edge list for fast object display. */

#ifdef TIME_OPEN_ANALYSIS
        printf( "Timing edge set determination...\n" );
        manage_timer( 0, 0 );
#endif

        get_mesh_edges( 0, analy );

#ifdef TIME_OPEN_ANALYSIS
        manage_timer( 0, 1 );
        putc( (int) '\n', stdout );
#endif
    
    /* Set initial display drawing function. */
    analy->update_display = draw_mesh_view;
    /*analy->update_display = quick_draw_mesh_view; */
    analy->display_mode_refresh = update_vis;
    analy->check_mod_required = mod_required_mesh_mode;
    
    /* Set render mode based on the number of potentially visible faces. */
    if ( max_face_qty > 10000 )
        analy->mesh_view_mode = RENDER_FILLED;

    /* Enable one-time warning if zero-volume-elements encountered. */
    reset_zero_vol_warning();

    /* Set initial reference state for nodes to mesh definition data. */
    set_ref_state( analy, 0 );


    /* Load in TI TOC if TI data is found */

    if ( env.ti_enable )
         if ( mc_ti_check_if_data_found( analy->db_ident ) ) /* Check if we found a TI
							      * data file.
							      */
         {
	      num_entries = mc_ti_htable_search_wildcard(analy->db_ident, 0, FALSE,
						         "M_", "NULL", "NULL",
						         wildcard_list );
 
	      analy->ti_vars      = NULL;
              analy->ti_var_count = num_entries;
	      analy->ti_vars      = (TI_Var *) malloc( num_entries*sizeof(TI_Var) );
 
              for ( i=0;
		    i<num_entries;
		    i++ )
      	      {
		    /* Extract the long name (with class info in name) 
		     * and short name of the TI variable. 
		     */
		    status = mc_ti_get_metadata_from_name( wildcard_list[i],
							   class, &meshid, &state,
							   &matid, &superclass,
							   &isMeshvar, &isNodal);
		    
		    status = mc_ti_get_data_len( analy->db_ident, wildcard_list[i],
						 &datatype, &datalength );

		    analy->ti_vars[i].long_name  = (char *) strdup(wildcard_list[i]);
		    analy->ti_vars[i].short_name = (char *) strdup( class );
		    analy->ti_vars[i].superclass = superclass;		  
		    analy->ti_vars[i].type       = datatype;		  
		    analy->ti_vars[i].length     = datalength;
		    analy->ti_vars[i].nodal      = isNodal;
	      }

              htable_delete_wildcard_list( num_entries, wildcard_list ) ;
              analy->ti_data_found = TRUE;
         }
         else    
              analy->ti_data_found = FALSE;

    /* Get the metadata from the A file */
    mc_get_metadata( analy->db_ident,  analy->mili_version,   analy->mili_host, 
                     analy->mili_arch, analy->mili_timestamp, analy->xmilics_version );

   return TRUE;
}



/************************************************************
 * TAG( close_analysis )
 *
 * Free up all the memory absorbed by an analysis.
 */
void
close_analysis( Analysis *analy )
{
    int i;
    Classed_list *p_cl;
    Triangle_poly *p_tp;
    char delth_tokens[2][TOKENLENGTH] = 
    {
        "delth", "all"
    };

    fr_state2( analy->state_p, analy );
    analy->state_p = NULL;

    if ( analy->ref_state_data != NULL )
        free( analy->ref_state_data );
    
    free( analy->bbox_source );

    DELETE_LIST( analy->refl_planes );
    free_matl_exp();
    if ( analy->classarray != NULL )
        free( analy->classarray );
    free( analy->tmp_result[0] );
    if ( analy->dimension == 3 )
    {
        /* Free the node normals working array. */
        for ( i = 2; i >= 0; i-- )
            free( analy->node_norm[i] );
    }
    DELETE_LIST( analy->result_mm_list );
    if ( analy->contour_cnt > 0 )
        free( analy->contour_vals );
    free_contours( analy );
    free_trace_points( analy );
    if ( analy->trace_disable != NULL )
        free( analy->trace_disable );
    analy->trace_disable_qty = 0;
    DELETE_LIST( analy->isosurf_poly );
    DELETE_LIST( analy->cut_planes );
    for ( p_cl = analy->cut_poly_lists; p_cl != NULL; NEXT( p_cl ) )
    {
        p_tp = (Triangle_poly *) p_cl->list;
        DELETE_LIST( p_tp );
    }
    DELETE_LIST( analy->cut_poly_lists );
    delete_vec_points( analy );
    if ( analy->vector_srec_map != NULL )
    {
        for ( i = 0; i < analy->qty_srec_fmts; i++ )
            if ( analy->vector_srec_map[i].qty != 0 )
                free( analy->vector_srec_map[i].list );

        free( analy->vector_srec_map );
    }

    DELETE_LIST( analy->selected_objects );

    /* Clean up time history resources. */
    if ( analy->srec_formats != NULL )
    {
        free( analy->srec_formats );
        analy->srec_formats = NULL;
    }
    if ( analy->format_blocks != NULL )
    {
        for ( i = 0; i < analy->qty_srec_fmts; i++ )
            free( analy->format_blocks[i].list );
        free( analy->format_blocks );
        analy->format_blocks = NULL;
    }

    /* 
     * Call this directly rather than via parse_command() since 
     * parse_command() will attempt to redraw after the plots have 
     * been deleted.
     */
    delete_time_series( sizeof( delth_tokens ) / sizeof( delth_tokens[0] ),
                        delth_tokens, analy );
    if ( analy->current_plots != NULL )
        popup_dialog( WARNING_POPUP, 
                      "Current plot list non-NULL after clear." );
    remove_unused_results( &analy->series_results );
    if ( analy->series_results != NULL )
        popup_dialog( WARNING_POPUP, 
                      "Time series result list non-NULL after clear." );

    if ( analy->abscissa != NULL )
    {
        cleanse_result( analy->abscissa );
        analy->abscissa = NULL;
    }
    destroy_time_series( &analy->times );

    free_static_strain_data();

    /* GUI resources...*/
    if ( analy->component_menu_specs != NULL )
        free( analy->component_menu_specs );
    destroy_mtl_mgr();
    destroy_surf_mgr();
}


/************************************************************
 * TAG( load_analysis )
 *
 * Load in a new analysis from the specified plotfile.
 */
Bool_type
load_analysis( char *fname, Analysis *analy )
{
    Database_type db_type;
    Bool_type status;

    /* 
     * Check that filename is valid before doing anything. 
     * 
     * (If OK, this check will be performed redundantly again 
     * in open_analysis.)
     */
    if ( !is_known_db( fname, &db_type ) )
    {
        popup_dialog( WARNING_POPUP, "%s\n%s", 
                      "Unable to open database or",
                      "unable to identify database type." );
        return FALSE;
    }

#ifdef SERIAL_BATCH
#else
    set_alt_cursor( CURSOR_EXCHANGE );
#endif /* SERIAL_BATCH */

    /* Reset render mode before closing old db so GUI state will be correct. */
    manage_render_mode( RENDER_MESH_VIEW, analy, TRUE );

    /* Call close_analysis() before db_close() to permit db queries. */
    close_analysis( analy );
    analy->db_close( analy ); 
    memset( analy, 0, sizeof( Analysis ) );
    strcpy( env.plotfile_name, fname );
    
    status = open_analysis( fname, analy );
    if ( !status )
        return FALSE;

#ifdef SERIAL_BATCH
#else
    regenerate_result_menus();
#endif

    reset_mesh_window( analy );

#ifdef SERIAL_BATCH
#else
    reset_window_titles();
#endif /* SERIAL_BATCH */

    wrt_standard_db_text( analy, TRUE );

#ifdef SERIAL_BATCH
#else
    init_btn_pick_classes();
    regenerate_pick_menus();
    unset_alt_cursor();
#endif /* SERIAL_BATCH */
    
    return TRUE;
}


/************************************************************
 * TAG( open_history_file )
 *
 * Open a file for saving the command history.
 */
void
open_history_file( char *fname )
{
    /* Close file if we had one open already. */
    close_history_file();

    if ( ( env.history_file = fopen(fname, "w") ) == NULL )
    {
        popup_dialog( INFO_POPUP, "Unable to open file %s", fname );
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
close_history_file( void )
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
history_command( char *command_str )
{
    /* Keep a running history commands - but only commands enter via the CLI */

    if ( !history_inputCB_cmd )
    {    
         if ( history_log_index==MAXHIST )
	      history_log_index=0;
	 if ( history_log[history_log_index]!=NULL )
	      free(history_log[history_log_index]);
	 history_log[history_log_index++] = (char *) strdup(command_str);
    }
  
    if ( env.save_history )
    {
        fprintf( env.history_file, "%s\n", command_str );
        fflush( env.history_file );
    }
}


/************************************************************
 * TAG( history_log_command )
 *
 * Save the running history log to a file.
 */
void
history_log_command( char *fname )
{
   int i;
    FILE *fp;

    /* Keep a running history commands */

    if ( ( fp = fopen(fname, "w") ) == NULL )
    {
         popup_dialog( INFO_POPUP, "Unable to open file %s", fname );
         return;
    }

    /* Since the log array is periodic (wraps around) start with the
     * oldest commands first.
     */
    for ( i=history_log_index+1;
	  i<MAXHIST;
	  i++)
          if ( history_log[i]!=NULL ) 
	       fprintf( fp, "%s\n", history_log[i] );

    for ( i=0;
	  i<history_log_index;
	  i++)
          if ( history_log[i]!=NULL ) 
               fprintf( fp, "%s\n", history_log[i] );

    fclose ( fp );
}


/************************************************************
 * TAG( history_log_clear )
 *
 * Clear the running history log to a file.
 */
void
history_log_clear( char *fname )
{
   int i;
 
   for ( i=0;
	 i<MAXHIST;
	 i++ )
   {
	 if ( history_log[history_log_index]!=NULL )
	      free(history_log[history_log_index]);
	 history_log[history_log_index] = (char *) NULL;
   }
}


/************************************************************
 * TAG( read_history_file )
 *
 * Read in a command history file and execute the commands in it.
 */
Bool_type
read_history_file( char *fname, int line_num, Analysis *analy )
{
    FILE *infile;
    char c, str[2000], tmp_str[2000];
    int i;
    int current_line=0;

    if ( (infile = fopen(fname, "r") ) == NULL )
    {
        popup_dialog( INFO_POPUP, "Unable to open file %s", fname );
        return FALSE;
    }

    analy->hist_line_cnt = line_num;

    env.history_input_active = TRUE;
 
    while ( fgets( str, 2000, infile ) != NULL &&
	    !feof(infile))
    {
        /* Skip to specified line number */

        current_line++;
        if ( line_num>1 && (current_line < line_num) )
	     continue;

        if ( str[0] != '#' && str[0] != '\0' )
	{
	     analy->hist_paused = FALSE;
             parse_command( str, analy );

#ifndef SERIAL_BATCH
  	    /* If we are running interactive display this command in the
	     * history window as hilited text.
	     */

	     sprintf(tmp_str, "  [%s- Line:%d] %s", fname, analy->hist_line_cnt, str);
	     write_history_text( tmp_str, TRUE );

	     /* Check if user has a 'pause' in the history input */

	     if ( analy->hist_paused )
	     {
	          analy->hist_line_cnt++;
	          fclose( infile );
		  return TRUE;
	     }
#endif
        }
	analy->hist_line_cnt++;
	
    }

    if ( feof(infile) )
         analy->hist_line_cnt = 0;

    fclose( infile );
    
    env.history_input_active = FALSE;

    return TRUE;
}


#ifdef SERIAL_BATCH

/************************************************************
 * TAG( process_serial_batch_mode )
 *
 * Process GRIZ in serial, batch mode:
 *
 *     perform selected GRIZ initialization(s), as required;
 *     establish OSMesa offscreen rendering context;
 *     read GRIZ command input file for batch operations
 */
#define MAX_STRING_LENGTH 200

static void
process_serial_batch_mode( char *batch_input_file_name, Analysis *analy )
{
    FILE
         *fp_batch_input_file;

    char
         c,
         command_string[MAX_STRING_LENGTH];

    int i;


    /* */

    if ( NULL == (fp_batch_input_file = fopen( batch_input_file_name, "r" )) )
        popup_fatal( "Unable to open batch command file" );

    init_mesh_window( analy );


    /*
     * display initial state
     */

    analy->update_display( analy );


    /*
     * process serial batch command file
     *
     * NOTE:  "quit" ends GRIZ processing
     */

    while ( !feof( fp_batch_input_file ) )
    {
        i = 0;

        c = getc( fp_batch_input_file );

        while ( c != '\n' && !feof( fp_batch_input_file ) && i < MAX_STRING_LENGTH - 1 )
        {
            command_string[i++] = c;
            c = getc( fp_batch_input_file );
        }

        command_string[i] = '\0';


        if ( command_string[0] != '#' && command_string[0] != '\0' )
        {
            if ( ( 0 != strcmp( command_string, "quit" ) ) &&
                 ( 0 != strcmp( command_string, "exit" ) ) &&
                 ( 0 != strcmp( command_string, "end"  ) ) )
                parse_command( command_string, analy );
            else
            {
                fclose( fp_batch_input_file );

                return;
            }
        }
    }

    /*
     * EOF condition
     */

    fclose( fp_batch_input_file );
}

#endif /* SERIAL_BATCH */

/************************************************************
 * TAG( usage )
 *
 * Write out GRIZ command-line syntax.
 */
static void
usage( void )
{
    static char *usage_text[] = {
"\nUsage:  griz4s -i input_base_name\n",
"            [-s]                          # single-buffer mode         #\n",
"            [-f]                          # run in foreground          #\n",
"            [-v]                          # size image for video       #\n",
"            [-V]                          # show program version       #\n",
"            [-w <width_pix> <height_pix>] # set exact image size       #\n", 
"            [-u]                          # util panel in ctrl win     #\n\n",
"            [-beta | -alpha]              # run a alpha or beta version of Griz #\n",
"            -version aaa                  # run a specific version of Griz #\n",
"                                          #   aaa to grizx (-version grizex) runs the latest pre-release of Griz #\n",
"            [-tv ]                        # run griz under Totalview debugger #\n",
"            [-nw]                         # new window behavior        #\n\n",
"            [-win32]                      # displaying Griz on Win32 platform #\n\n",
"            [-man]                        # display Griz online manual #\n\n"
#ifdef SERIAL_BATCH
            ,
"            [-b | -batch] input_file_name # batch mode               #\n\n" 
#endif
    };
    int i, line_cnt;

    line_cnt = sizeof( usage_text ) / sizeof( usage_text[0] );

    for ( i = 0; i < line_cnt; i++ )
          wrt_text( usage_text[i] );

    return;
}

