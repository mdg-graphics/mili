/* $Id$ */
/* 
 * interpret.c - Command line interpreter for graphics viewer.
 * 
 * 	Donald J. Dovey
 * 	Lawrence Livermore National Laboratory
 * 	Jan 2 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */
#include "viewer.h"
#include "draw.h"

#ifdef VIDEO_FRAMER
extern void vid_select( /* int select */ );
extern void vid_cbars();
extern int vid_vlan_init();
extern int vid_vlan_rec( /* int move_frame, int record_n_frames */ );
extern void vid_cbars();
#endif VIDEO_FRAMER

extern void update_min_max();
extern View_win_obj *v_win;
extern float crease_threshold;
extern float explicit_threshold;
extern FILE *text_history_file;
extern Bool_type text_history_invoked;

/*****************************************************************
 * TAG( Alias_obj )
 *
 * Holds a command alias.
 */
typedef struct _Alias_obj
{ 
    struct _Alias_obj *next;
    struct _Alias_obj *prev;
    char alias_str[TOKENLENGTH];
    int token_cnt;
    char tokens[MAXTOKENS][TOKENLENGTH];
} Alias_obj;

static Alias_obj *alias_list = NULL;

/*****************************************************************
 * TAG( Material_property_obj )
 *
 * Holds information parsed from a material manager request.
 */
typedef struct _Material_property_obj
{
    GLfloat color_props[3];
    int materials[MAX_MATERIALS];
    int mtl_cnt;
    int cur_idx;
} Material_property_obj;


static char last_command[200] = "\n";
static Bool_type user_cmap_loaded = FALSE;
static char *el_label[] = { "Node", "Beam", "Shell", "Hex" };

/* Local routines. */
static void check_visualizing();
static void create_alias();
static void alias_substitute();
static int parse_embedded_mtl_cmd();
static void parse_vcent();
static Bool_type is_numeric_token();
static void parse_mtl_cmd();
static int parse_embedded_mtl_cmd();
static void update_previous_prop();
static void copy_property();
static void outvec();
static void write_outvec_data();
static void prake();
static int get_prake_data();
static int tokenize();
static void start_text_history();
static void end_text_history();
static void tell_coordinates();
static void tell_element_coords();
static int max_id_string_width();

/* Used by other files, but not by the interpreter. */
/*****************************************************************
 * TAG( read_token )
 * 
 * General purpose routine to read a token (delimited by white space)
 * from a file.  Comments begin with a '#' and extend to the end of
 * the line or are enclosed in braces.  Strings in double quotes are
 * returned as a single token.
 */
void
read_token( infile, token, max_length )
FILE *infile;
char *token;
int max_length;
{
    int i;
    char c;
    char lc_char;           /* Line comment character. */
    char encl_char[2];      /* Enclosing comment characters. */
    char quote_char;        /* Quote character for quoted strings. */

    lc_char = '#';
    encl_char[0] = '{';
    encl_char[1] = '}';
    quote_char = '"';

    /*
     * Eat up any preliminary white space.
     */
    c = getc( infile );

    while ( !feof( infile ) &&
            (isspace( c ) || c == encl_char[0] || c == lc_char) )
    {
        if ( c == lc_char )
            while ( !feof( infile ) && c != '\n' )
                c = getc( infile );
        else if ( c == encl_char[0] )
            while ( !feof( infile ) && c != encl_char[1] )
                c = getc( infile );

        c = getc( infile );
    }

    i = 0;
    if ( c == quote_char )
    {
        /* Get a quoted string. */
        c = getc( infile );
        while ( !feof( infile ) && c != quote_char && i < max_length-1 )
        {
            token[i] = c;
            i++;
            c = getc( infile );
        }
    }
    else
    {
        /* Get a normal token. */
        while ( !feof( infile ) && !isspace( c ) &&
                c != encl_char[0] && c != lc_char && i < max_length-1 )
        {
            token[i] = c;
            i++;
            c = getc( infile );
        }
    }

    /*
     * Eat up comment if we ended up at one.
     */
    if ( c == lc_char )
        while ( !feof( infile ) && c != '\n' )
            c = getc( infile );
    else if ( c == encl_char[0] )
        while ( !feof( infile ) && c != encl_char[1] )
            c = getc( infile );

    token[i] = '\0';
}

/* UNUSED - a simpler version of the above routine. */
/*****************************************************************
 * TAG( read_token )
 *
 * General purpose routine to read a token (delimited by white space)
 * from a file.
 */
/*
void
read_token( infile, token, max_length )
FILE *infile;
char *token;
int max_length;
{
    int i;
    char c;

    * Eat up any preliminary white space. *
    c = getc( infile );
    while ( !feof( infile ) && isspace( c ) )
        c = getc( infile );

    * Get the token. *
    i = 0;
    while ( !feof( infile ) && !isspace( c ) && i < max_length-1 )
    {
        token[i] = c;
        i++;
        c = getc( infile );
    }

    token[i] = '\0';
}
*/

/* UNUSED. */
/*****************************************************************
 * TAG( get_line )
 * 
 * Get a line of text from a file.
 */
/*
void
get_line( buf, max_length, infile )
char *buf;
int max_length;
FILE *infile;
{
    while ( fgets( buf, max_length, infile ) == NULL );
}
*/


/*****************************************************************
 * TAG( tokenize_line )
 * 
 * Tokenize a command line.  Breaks space-separated words into
 * separate tokens.
 */
static void
tokenize_line( buf, tokens, token_cnt )
char *buf;
char tokens[MAXTOKENS][TOKENLENGTH];
int *token_cnt;
{
    int chr, word, wchr;
    char lc_char;           /* Line comment character. */
    char quote_char;        /* Quote character for quoted strings. */
    Bool_type in_quote;

    lc_char = '#';
    quote_char = '"';

    chr = 0;
    word = 0;
    wchr = 0;
    in_quote = FALSE;

    while ( isspace( buf[chr] ) )
        ++chr;

    while( buf[chr] != '\0' )
    {
        if ( in_quote )
        {
            if ( buf[chr] == quote_char )
            {
                in_quote = FALSE;
                tokens[word][wchr] = '\0';
                ++word;
                wchr = 0;
                ++chr;
                while ( isspace( buf[chr] ) && buf[chr] != '\0' )
                    ++chr;
            }
            else
            {
                tokens[word][wchr] = buf[chr];
                ++chr;
                ++wchr;
            }
        }
        else if ( buf[chr] == quote_char )
        {
            tokens[word][wchr] = '\0';
            if ( wchr > 0 )
                ++word;
            wchr = 0;
            ++chr;
            in_quote = TRUE;
        }
        else if ( buf[chr] == lc_char )
        {
            /* Comment runs to end of line. */
            break;
        }
        else if ( isspace( buf[chr] ) )
        {
            tokens[word][wchr] = '\0';
	    ++word;
            wchr = 0;
            while ( isspace( buf[chr] ) && buf[chr] != '\0' )
                ++chr;
        }
        else
        {
            tokens[word][wchr] = buf[chr];
            ++chr;
            ++wchr;
	    if ( buf[chr] == '\0' )
	    {
		tokens[word][wchr] = '\0';
	        ++word;
	    }
        }
    }

    *token_cnt = word; 
}


/*****************************************************************
 * TAG( parse_command )
 * 
 * Command parser for viewer.  Parse the command in buf.
 */
void
parse_command( buf, analy )
char *buf;
Analysis *analy;
{
    char tokens[MAXTOKENS][TOKENLENGTH];
    int token_cnt;
    int object_id;
    char str[90];
    float val, pt[3], vec[3], rgb[3];
    Bool_type redraw, redrawview, renorm, setval, valid_command;
    int sz[3];
    int ival, i, j, k, m;
    int start_state, stop_state;
    char result_variable[1];
    Bool_type tellmm_redraw;
    Bool_type selection_cleared;
    int sel_qty;
    int *p_sel_id;
    int old;
    char *mo_usage_spec = "{{node|n}|{beam|b}|{shell|s}|{brick|h}}";

    tokenize_line( buf, tokens, &token_cnt );

    if ( token_cnt < 1 )
        return;

    /*
     * Clean up any dialogs from the last command that the user
     * hasn't acknowledged.
     */
    clear_popup_dialogs();

    alias_substitute( tokens, &token_cnt, analy );

    /*
     * Should call getnum in slots below to make sure input is valid,
     * and prompt user if it isn't.   (Use instead of sscanf.)
     * Also getint and getstring.
     */

    redraw = FALSE;
    redrawview = FALSE;
    renorm = FALSE;
    valid_command = TRUE;


    if ( strcmp( tokens[0], "rx" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        inc_mesh_rot( 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, DEG_TO_RAD(val) );
        redrawview = TRUE;
    }
/**/
    else if ( strcmp( tokens[0], "clppln" ) == 0 )
    {
        Plane_obj *plane;

        plane = NEW( Plane_obj, "Cutting plane" );
        for ( i = 0; i < 3; i++ )
            sscanf( tokens[i+1], "%f", &(plane->pt[i]) );
        for ( i = 0; i < 3; i++ )
            sscanf( tokens[i+4], "%f", &(plane->norm[i]) );
	val = -VEC_DOT( plane->pt, plane->norm );
	add_clip_plane( plane->norm, val );
        redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "ry" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        inc_mesh_rot( 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, DEG_TO_RAD(val) );
        redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "rz" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        inc_mesh_rot( 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, DEG_TO_RAD(val) );
        redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "tx" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        inc_mesh_trans( val, 0.0, 0.0 );
        redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "ty" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        inc_mesh_trans( 0.0, val, 0.0 );
        redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "tz" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        inc_mesh_trans( 0.0, 0.0, val );
        redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "scale" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        set_mesh_scale( val, val, val );
        redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "minmov" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
	if ( val < 0.0 )
	    popup_dialog( USAGE_POPUP, "minmov <pixel distance>" );
	else
	    set_motion_threshold( val );
    }
    else if ( strcmp( tokens[0], "scalax" ) == 0 )
    {
        sscanf( tokens[1], "%f", &vec[0] );
        sscanf( tokens[2], "%f", &vec[1] );
        sscanf( tokens[3], "%f", &vec[2] );
        set_mesh_scale( vec[0], vec[1], vec[2] );
        redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "r" ) == 0 )
    {
        /* Repeat the last command. */
        parse_command( last_command, analy );
    }
    else if ( strcmp( tokens[0], "hilite" ) == 0 )
    {
        if ( strcmp( tokens[1], "node" ) == 0 
	     || strcmp( tokens[1], "n" ) == 0 )
            analy->hilite_type = 1;
        else if ( strcmp( tokens[1], "beam" ) == 0
	          || strcmp( tokens[1], "b" ) == 0 )
            analy->hilite_type = 2;
        else if ( strcmp( tokens[1], "shell" ) == 0
	          || strcmp( tokens[1], "s" ) == 0 )
            analy->hilite_type = 3;
        else if ( strcmp( tokens[1], "brick" ) == 0
	          || strcmp( tokens[1], "h" ) == 0 )
            analy->hilite_type = 4;
        else
	    popup_dialog( USAGE_POPUP,  "hilite %s <ident>", 
			  mo_usage_spec );

        if ( token_cnt > 2 )
        {
            sscanf( tokens[2], "%d", &analy->hilite_num );
            analy->hilite_num--;
        }
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "clrhil" ) == 0 )
    {
        analy->hilite_type = 0;
	analy->hilite_num = -1;
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "select" ) == 0 )
    {
        ival = -1;
        if ( strcmp( tokens[1], "node" ) == 0
	     || strcmp( tokens[1], "n" ) == 0 )
            ival = 0;
        else if ( strcmp( tokens[1], "beam" ) == 0
	          || strcmp( tokens[1], "b" ) == 0 )
            ival = 1;
        else if ( strcmp( tokens[1], "shell" ) == 0
	          || strcmp( tokens[1], "s" ) == 0 )
            ival = 2;
        else if ( strcmp( tokens[1], "brick" ) == 0
	          || strcmp( tokens[1], "h" ) == 0 )
            ival = 3;
        else
	    popup_dialog( USAGE_POPUP,  "select %s <ident>...", 
			  mo_usage_spec );

        if ( ival >= 0 )
        {
            for ( i = 2; i < token_cnt; i++ )
            {
                sscanf( tokens[i], "%d", &j );
                analy->select_elems[ival][analy->num_select[ival]] = j-1;
                analy->num_select[ival]++;
            }
            clear_gather( analy );
        }
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "clrsel" ) == 0 )
    {
        selection_cleared = FALSE;
	
        if ( token_cnt == 1 )
	{
	    for ( i = 0; i < 4; i++ )
		analy->num_select[i] = 0;
		
	    selection_cleared = TRUE;
	}
	else
	{
	    ival = -1;
	    if ( strcmp( tokens[1], "node" ) == 0
		 || strcmp( tokens[1], "n" ) == 0 )
		ival = 0;
	    else if ( strcmp( tokens[1], "beam" ) == 0
		      || strcmp( tokens[1], "b" ) == 0 )
		ival = 1;
	    else if ( strcmp( tokens[1], "shell" ) == 0
		      || strcmp( tokens[1], "s" ) == 0 )
		ival = 2;
	    else if ( strcmp( tokens[1], "brick" ) == 0
		      || strcmp( tokens[1], "h" ) == 0 )
		ival = 3;
	    else
		popup_dialog( USAGE_POPUP,  "clrsel [%s <ident>...]", 
		              mo_usage_spec );
    
	    if ( ival >= 0 )
	    {
	        sel_qty = analy->num_select[ival];
		p_sel_id = analy->select_elems[ival];
		
		for ( i = 2; i < token_cnt; i++ )
		{
		    sscanf( tokens[i], "%d", &j );
		    
		    /* Locate the object id among current selected objects. */
		    if ( is_in_iarray( j - 1, sel_qty, p_sel_id, &k ) )
		    {
		        /* Move remaining selections down. */
			for ( m = k + 1; m < sel_qty; m++ )
			    p_sel_id[m - 1] = p_sel_id[m];
			
			analy->num_select[ival]--;
			sel_qty = analy->num_select[ival];
			
			selection_cleared = TRUE;
		    }
                    else
		        popup_dialog( INFO_POPUP,
			              "%s %d not selected; request ignored.", 
				      el_label[ival], j );
		}
	    }
	}
	
	if ( selection_cleared )
	{
	    clear_gather( analy );
	    redraw = TRUE;
	}
    }
    else if ( strcmp( tokens[0], "show" ) == 0 )
    {
        analy->result_mod = TRUE;
        if ( parse_show_command( tokens[1], analy ) )
            redraw = TRUE;
        else
            valid_command = FALSE;
    }

    /* Multi-commands like "show" should go here for faster parsing.
     * Follow those with display-changing commands.
     * Follow those with miscellaneous commands (info, etc.).
     */
 
    else if ( strcmp( tokens[0], "quit" ) == 0 ||
              strcmp( tokens[0], "exit" ) == 0 ||
              strcmp( tokens[0], "end" ) == 0 )
    {
        quit( 0 );
    }
    else if ( strcmp( tokens[0], "help" ) == 0 ||
              strcmp( tokens[0], "?" ) == 0 )
    {
        wrt_text( "Help function not installed yet.\n" );
    }
    else if ( strcmp( tokens[0], "copyrt" ) == 0 )
    {
        copyright();
    }
    else if ( strcmp( tokens[0], "info" ) == 0 )
    {
        wrt_text( "\n\t*************** GRIZ INFO ***************\n" );
        wrt_text( "Data file: %s\nStates: %d\nCurrent state: %d\n\n",
                  analy->root_name, analy->num_states, analy->cur_state+1 );
        wrt_text( "Start time: %.4e\nEnd time:   %.4e\nCurrent time: %.4e\n\n",
                  analy->state_times[0],
                  analy->state_times[analy->num_states-1],
                  analy->state_times[analy->cur_state] );
        wrt_text( "Nodes: %d\n", analy->geom_p->nodes->cnt );
        if ( analy->geom_p->bricks )
            wrt_text( "Hex elements: %d\n", analy->geom_p->bricks->cnt );
        if ( analy->geom_p->shells )
            wrt_text( "Shell elements: %d\n", analy->geom_p->shells->cnt );
        if ( analy->geom_p->beams )
            wrt_text( "Beam elements: %d\n", analy->geom_p->beams->cnt );
        if ( analy->hilite_type == 1 )
        {
            i = analy->hilite_num;
            wrt_text( "Hilited node %d: %.4f %.4f %.4f\n", i+1,
                      analy->state_p->nodes->xyz[0][i],
                      analy->state_p->nodes->xyz[1][i],
                      analy->state_p->nodes->xyz[2][i] );
        }
        wrt_text( "\n" );
        if ( analy->result_id != VAL_NONE )
        {
            wrt_text( "Result, using min: %.4e  max: %.4e\n",
                      analy->result_mm[0], analy->result_mm[1] );
            wrt_text( "Result, global min: %.4e  max: %.4e\n",
                      analy->global_mm[0], analy->global_mm[1] );
            wrt_text( "Result, state min: %.4e  max: %.4e\n",
                      analy->state_mm[0], analy->state_mm[1] );
            wrt_text( "Result zero tolerance: %.4e\n\n", analy->zero_result );
        }
        print_view();
        wrt_text( "\nCurrent bounding box, low corner: %.4f, %.4f, %.4f\n",
                  analy->bbox[0][0], analy->bbox[0][1], analy->bbox[0][2] );
        wrt_text( "Current bounding box, high corner: %.4f, %.4f, %.4f\n\n",
                  analy->bbox[1][0], analy->bbox[1][1], analy->bbox[1][2] );
    }
    else if ( strcmp( tokens[0], "lts" ) == 0 )
    {
        wrt_text( "\n\t*************** STATE TIMES ***************\n\n" );
        wrt_text( "\tState Num \t State Time \t Delta Time\n" );
        wrt_text( "\t--------- \t ---------- \t ----------\n" );
        wrt_text( "\t    %d \t\t %.4e\n", 1, analy->state_times[0] );
        for (  i = 1; i < analy->num_states; i++ )
        {
            val = analy->state_times[i] - analy->state_times[i-1];
            wrt_text( "\t    %d \t\t %.4e \t %.4e\n", i+1,
                      analy->state_times[i], val );
        }
    }
    else if ( strcmp( tokens[0], "tellpos" ) == 0)
    {
        if ( token_cnt == 3 )
        {
            object_id = atoi( tokens[2] );
            tell_coordinates( tokens[1], object_id, analy );
        }
        else
            popup_dialog( USAGE_POPUP, 
	                  "tellpos { n | b | s | h } <ident>" );
    }
    else if ( strcmp( tokens[0], "savtxt" ) == 0)
    {
        if ( token_cnt == 2 )
            start_text_history( tokens[1] );
        else
            popup_dialog( USAGE_POPUP, "savtxt <filename>" );
    }
    else if ( strcmp( tokens[0], "endtxt" ) == 0)
    {
        end_text_history();
    }
    else if ( strcmp( tokens[0], "load" ) == 0 )
    {
        if ( token_cnt > 1 )
            load_analysis( tokens[1], analy );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "bufqty" ) == 0 )
    {
        if ( token_cnt == 3 )
	{
	    if ( strcmp( tokens[1], "all" ) == 0 )
	        i = ALL_OBJECT_T;
	    else if ( strcmp( tokens[1], "n" ) == 0 )
	        i = NODE_T;
	    else if ( strcmp( tokens[1], "h" ) == 0 )
	        i = BRICK_T;
	    else if ( strcmp( tokens[1], "s" ) == 0 )
	        i = SHELL_T;
	    else if ( strcmp( tokens[1], "b" ) == 0 )
	        i = BEAM_T;
	    else
	        valid_command = FALSE;
	
	    j = atoi( tokens[2] );
	    if ( j < 0 )
	        valid_command = FALSE;
	}
	else
	    valid_command = FALSE;
	
	if ( valid_command )
	    set_input_buffer_qty( i, j );
	else
	    popup_dialog( USAGE_POPUP, 
	                  "bufqty { all | n | b | s | h } <buffer qty>" );
    }
    else if ( strcmp( tokens[0], "rview" ) == 0 )
    {
            reset_mesh_transform();
            redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "vcent" ) == 0 )
    {
	parse_vcent( analy, tokens, token_cnt, &redraw );
    }
    else if ( strcmp( tokens[0], "conv" ) == 0 )
    {
        if ( token_cnt == 3 )
        {
            sscanf( tokens[1], "%f", &val );
            analy->conversion_scale = val;

            sscanf( tokens[2], "%f", &val );
            analy->conversion_offset = val;

            redraw = TRUE;
        }
        else
            popup_dialog( USAGE_POPUP, "conv <scale> <offset>" );
    }
    else if (strcmp( tokens[0], "clrconv" ) == 0 )
    {
        analy->conversion_scale = 1.0;
        analy->conversion_offset = 0.0;

        analy->perform_unit_conversion = FALSE;

        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "on" ) == 0 ||
              strcmp( tokens[0], "off" ) == 0 )
    {
        if ( strcmp( tokens[0], "on" ) == 0 )
            setval = TRUE;
        else
            setval = FALSE;

        for ( i = 1; i < token_cnt; i++ )
        {
            if ( strcmp( tokens[i], "box" ) == 0 )
                analy->show_bbox = setval; 
            else if ( strcmp( tokens[i], "coord" ) == 0 )
                analy->show_coord = setval; 
            else if ( strcmp( tokens[i], "time" ) == 0 )
                analy->show_time = setval;
            else if ( strcmp( tokens[i], "title" ) == 0 )
                analy->show_title = setval;
            else if ( strcmp( tokens[i], "cmap" ) == 0 )
                analy->show_colormap = setval;
	    else if ( strcmp( tokens[i], "minmax" ) == 0 )
	        analy->show_minmax = setval;
            else if ( strcmp( tokens[i], "all" ) == 0 )
            {   
                analy->show_coord = setval;
                analy->show_time = setval;
                analy->show_title = setval;
                analy->show_colormap = setval;
                analy->show_minmax = setval;
            }
            else if ( strcmp( tokens[i], "cscale" ) == 0 )
                analy->show_colorscale = setval;
            else if ( strcmp( tokens[i], "edges" ) == 0 )
            {
                if ( strcmp( tokens[0], "on" ) == 0 &&
                     analy->state_p->activity_present &&
                     ( analy->geom_p->bricks || analy->geom_p->shells ) )
                    get_mesh_edges( analy );
                analy->show_edges = setval;
		if ( setval && analy->m_edges_cnt == 0 )
		    wrt_text( "Empty edge list; check material visibility.\n" );
		update_util_panel( VIEW_EDGES );
            }
            else if ( strcmp( tokens[i], "safe" ) == 0 )
                analy->show_safe = setval;
            else if ( strcmp( tokens[i], "ndnum" ) == 0 )
                analy->show_node_nums = setval;
            else if ( strcmp( tokens[i], "elnum" ) == 0 )
                analy->show_elem_nums = setval;
            else if ( strcmp( tokens[i], "shrfac" ) == 0 )
                analy->shared_faces = setval;
            else if ( strcmp( tokens[i], "rough" ) == 0 )
            {
                analy->show_roughcut = setval;
                reset_face_visibility( analy );
                renorm = TRUE;
            }
            else if ( strcmp( tokens[i], "cut" ) == 0 )
            {    
                analy->show_cut = setval;
                gen_cuts( analy );
                gen_carpet_points( analy );
                check_visualizing( analy );
            }   
            else if ( strcmp( tokens[i], "con" ) == 0 )
            {
                analy->show_contours = setval;
                gen_contours( analy );
            }
            else if ( strcmp( tokens[i], "iso" ) == 0 )
            {
                analy->show_isosurfs = setval;
                gen_isosurfs( analy );
                gen_carpet_points( analy );
                check_visualizing( analy );
            }
            else if ( strcmp( tokens[i], "vec" ) == 0 )
            {
                if ( setval &&
                     analy->vec_id[0] == VAL_NONE &&
                     analy->vec_id[1] == VAL_NONE &&
                     analy->vec_id[2] == VAL_NONE )
                {
                    wrt_text("No vec result set, use `vec <vx> <vy> <vz>'.\n");
                    analy->show_vectors = FALSE;
                }
                else
                    analy->show_vectors = setval;

                update_vec_points( analy );
                check_visualizing( analy );
            }
	    else if ( strcmp( tokens[i], "sphere" ) == 0 )
	        analy->show_vector_spheres = setval;
	    else if ( strcmp( tokens[i], "sclbyres" ) == 0 )
	        analy->scale_vec_by_result = setval;
	    else if ( strcmp( tokens[i], "zlines" ) == 0 )
	        analy->z_buffer_lines = setval;
            else if ( strcmp( tokens[i], "carpet" ) == 0 )
            {
                if ( setval &&
                     analy->vec_id[0] == VAL_NONE &&
                     analy->vec_id[1] == VAL_NONE &&
                     analy->vec_id[2] == VAL_NONE )
                {
                    wrt_text("No vec result set, use `vec <vx> <vy> <vz>'.\n");
                    analy->show_carpet = FALSE;
                }
                else
                    analy->show_carpet = setval;
                gen_reg_carpet_points( analy );
                gen_carpet_points( analy );
                check_visualizing( analy );
            }
            else if ( strcmp( tokens[i], "refresh" ) == 0 )
                analy->refresh = setval;
            else if ( strcmp( tokens[i], "sym" ) == 0 )
                analy->reflect = setval;
            else if ( strcmp( tokens[i], "cull" ) == 0 )
                analy->manual_backface_cull = setval;
            else if ( strcmp( tokens[i], "cent" ) == 0 )
	    {
                /* This command has been changed. */
                if ( setval )
                    wrt_text( "Use \"vcent hi\" to center view on %s",
                              "hilited object.\n" );
                else
                    wrt_text( "Use \"vcent off\" to turn off view %s",
                              "centering.\n" );
            }
            else if ( strcmp( tokens[i], "refsrf" ) == 0 )
                analy->show_ref_polys = setval;
            else if ( strcmp( tokens[i], "refres" ) == 0 )
                analy->result_on_refs = setval;
            else if ( strcmp( tokens[i], "extsrf" ) == 0 )
                analy->show_extern_polys = setval;
            else if ( strcmp( tokens[i], "timing" ) == 0 )
                env.timing = setval;
            else if ( strcmp( tokens[i], "derivtime" ) == 0 )
                env.result_timing = setval;
            else if ( strcmp( tokens[i], "thsm" ) == 0 )
                analy->th_smooth = setval;
            else if ( strcmp( tokens[i], "conv" ) == 0 )
                analy->perform_unit_conversion = setval;
            else if ( strcmp( tokens[i], "bmbias" ) == 0 )
                analy->zbias_beams = setval;
            else if ( strcmp( tokens[i], "bbmax" ) == 0 )
                analy->keep_max_bbox_extent = setval;
            else if ( strcmp( tokens[i], "autosz" ) == 0 )
                analy->auto_frac_size = setval;
            else if ( strcmp( tokens[i], "lighting" ) == 0 )
	    {
		if ( analy->dimension == 2 && setval == TRUE )
		    popup_dialog( INFO_POPUP, 
		                  "Lighting is not utilized for 2D meshes." );
		else
		    v_win->lighting = setval;
	    }
            else
                wrt_text( "On/Off command unrecognized: %s\n", tokens[i] );
        } 
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "switch" ) == 0 ||
              strcmp( tokens[0], "sw" ) == 0 )
    {
        for ( i = 1; i < token_cnt; i++ )
        {
            /* View type. */
            if ( strcmp( tokens[i], "persp" ) == 0 )
            {
                set_orthographic( FALSE );
                set_mesh_view();
            }
            else if ( strcmp( tokens[i], "ortho" ) == 0 )
            {
                set_orthographic( TRUE );
                set_mesh_view();
            }

            /* Normals. */
            else if ( strcmp( tokens[i], "flat" ) == 0 )
            {
                analy->normals_smooth = FALSE;
                renorm = TRUE;
            }
            else if ( strcmp( tokens[i], "smooth" ) == 0 )
            {
                analy->normals_smooth = TRUE;
                renorm = TRUE;
            }

            /* Rendering style. */
            else if ( strcmp( tokens[i], "hidden" ) == 0 )
	    {
                analy->render_mode = RENDER_HIDDEN;
		update_util_panel( VIEW_SOLID_MESH );
	    }
            else if ( strcmp( tokens[i], "solid" ) == 0 )
	    {
                analy->render_mode = RENDER_FILLED;
		update_util_panel( VIEW_SOLID );
	    }
            else if ( strcmp( tokens[i], "cloud" ) == 0 )
	    {
                analy->render_mode = RENDER_POINT_CLOUD;
		update_util_panel( VIEW_POINT_CLOUD );
	    }
            else if ( strcmp( tokens[i], "none" ) == 0 )
	    {
                analy->render_mode = RENDER_NONE;
		update_util_panel( VIEW_NONE );
	    }

            /* Interpolation mode. */
            else if ( strcmp( tokens[i], "noterp" ) == 0 )
                analy->interp_mode = NO_INTERP;
            else if ( strcmp( tokens[i], "interp" ) == 0 )
                analy->interp_mode = REG_INTERP;
            else if ( strcmp( tokens[i], "gterp" ) == 0 )
                analy->interp_mode = GOOD_INTERP;

            /* Mouse picking mode. */
            else if ( strcmp( tokens[i], "picsel" ) == 0 )
	    {
                analy->mouse_mode = MOUSE_SELECT;
		update_util_panel( PICK_MODE_SELECT );
	    }
            else if ( strcmp( tokens[i], "pichil" ) == 0 )
	    {
                analy->mouse_mode = MOUSE_HILITE;
		update_util_panel( PICK_MODE_HILITE );
	    }

            /* Pick beams or shells with middle mouse button. */
            else if ( strcmp( tokens[i], "picbm" ) == 0 )
	    {
                analy->pick_beams = TRUE;
		update_util_panel( BTN2_BEAM );
	    }
            else if ( strcmp( tokens[i], "picsh" ) == 0 )
	    {
                analy->pick_beams = FALSE;
		update_util_panel( BTN2_SHELL );
	    }

            /* Result min/max. */
            else if( strcmp( tokens[i], "mstat" ) == 0 )
            {
                analy->use_global_mm = FALSE;
		if ( !analy->mm_result_set[0] )
                    analy->result_mm[0] = analy->state_mm[0];
		if ( !analy->mm_result_set[1] )
                    analy->result_mm[1] = analy->state_mm[1];
                update_vis( analy );
            }
            else if( strcmp( tokens[i], "mglob" ) == 0 )
            {
                analy->use_global_mm = TRUE;
		if ( !analy->mm_result_set[0] )
                    analy->result_mm[0] = analy->global_mm[0];
		if ( !analy->mm_result_set[1] )
                    analy->result_mm[1] = analy->global_mm[1];
                update_vis( analy );
            }

            /* Symmetry plane behavior. */
            else if ( strcmp( tokens[i], "symcu" ) == 0 )
                analy->refl_orig_only = FALSE;
            else if ( strcmp( tokens[i], "symor" ) == 0 )
                analy->refl_orig_only = TRUE;

            /* Screen aspect ratio correction. */
            else if ( strcmp( tokens[i], "vidasp" ) == 0 )
            {
                aspect_correct( TRUE, analy );
            }
            else if ( strcmp( tokens[i], "norasp" ) == 0 )
            {
                aspect_correct( FALSE, analy );
            }

            /* Shell reference surface. */
            else if ( strcmp( tokens[i], "middle" ) == 0 )
            {
                analy->ref_surf = MIDDLE;
		analy->result_mod = TRUE;
            }
            else if ( strcmp( tokens[i], "inner" ) == 0 )
            {
                analy->ref_surf = INNER;
		analy->result_mod = TRUE;
            }
            else if ( strcmp( tokens[i], "outer" ) == 0 )
            {
                analy->ref_surf = OUTER;
		analy->result_mod = TRUE;
            }

            /* Strain basis. */
            else if ( strcmp( tokens[i], "rglob" ) == 0 )
            {
                analy->ref_frame = GLOBAL;
		analy->result_mod = TRUE;
            }
            else if ( strcmp( tokens[i], "rloc" ) == 0 )
            {
                analy->ref_frame = LOCAL;
		analy->result_mod = TRUE;
            }

            /* Strain variety. */
            else if ( strcmp( tokens[i], "infin" ) == 0 )
            {
	        old = analy->strain_variety;
                analy->strain_variety = INFINITESIMAL;
		analy->result_mod = mod_required( analy, STRAIN_TYPE, old, 
		                                  analy->strain_variety );
            }
            else if ( strcmp( tokens[i], "grn" ) == 0 )
            {
	        old = analy->strain_variety;
                analy->strain_variety = GREEN_LAGRANGE;
		analy->result_mod = mod_required( analy, STRAIN_TYPE, old, 
		                                  analy->strain_variety );
            }
            else if ( strcmp( tokens[i], "alman" ) == 0 )
            {
	        old = analy->strain_variety;
                analy->strain_variety = ALMANSI;
		analy->result_mod = mod_required( analy, STRAIN_TYPE, old, 
		                                  analy->strain_variety );
            }
            else if ( strcmp( tokens[i], "rate" ) == 0 )
            {
	        old = analy->strain_variety;
                analy->strain_variety = RATE;
		analy->result_mod = mod_required( analy, STRAIN_TYPE, old, 
		                                  analy->strain_variety );
            }
            else if ( strcmp( tokens[i], "grdvec" ) == 0 )
            {
                analy->vectors_at_nodes = FALSE;
            }
            else if ( strcmp( tokens[i], "nodvec" ) == 0 )
            {
                analy->vectors_at_nodes = TRUE;
            }
            else
                wrt_text( "Switch command unrecognized: %s\n", tokens[i] );
        } 

        /* Reload the result vector. */
        if ( analy->result_mod )
        {
            load_result( analy, TRUE );
            update_vis( analy );
        }
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "fracsz" ) == 0 )
    {
	sscanf( tokens[1], "%d", &ival );
	if ( ival >= 0 && ival <= 6 )
	{
	    analy->float_frac_size = ival;
	    redraw = TRUE;
	}
    }
    else if ( strcmp( tokens[0], "invis" ) == 0 ||
              strcmp( tokens[0], "vis" ) == 0 )
    {
        if ( strcmp( tokens[0], "invis" ) == 0 )
            setval = TRUE;
        else
            setval = FALSE;

        if ( strcmp( tokens[1], "all" ) == 0 )
        {
            for ( i = 0; i < analy->num_materials; i++ )
                analy->hide_material[i] = setval;
        }
        else
        {
            for ( i = 1; i < token_cnt; i++ )
            {
                sscanf( tokens[i], "%d", &ival );
/**/
/*
                if ( ival > 0 && ival <= analy->num_materials )
*/
                if ( ival > 0 && ival <= MAX_MATERIALS )
                    analy->hide_material[ival-1] = setval;
            }
        }
        reset_face_visibility( analy );

        if ( analy->dimension == 3 ) renorm = TRUE;
	analy->result_mod = TRUE;
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "disable" ) == 0 ||
              strcmp( tokens[0], "enable" ) == 0 )
    {
        if ( strcmp( tokens[0], "disable" ) == 0 )
            setval = TRUE;
        else
            setval = FALSE;

        if ( strcmp( tokens[1], "all" ) == 0 )
        {
            for ( i = 0; i < analy->num_materials; i++ )
                analy->disable_material[i] = setval;
        }
        else
        {
            for ( i = 1; i < token_cnt; i++ )
            {
                sscanf( tokens[i], "%d", &ival );
/**/
/*
                if ( ival > 0 && ival <= analy->num_materials )
*/
                if ( ival > 0 && ival <= MAX_MATERIALS )
                    analy->disable_material[ival-1] = setval;
            }
        }
	analy->result_mod = TRUE;
	load_result( analy, TRUE );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "mtl" ) == 0 )
    {
	parse_mtl_cmd( analy, tokens, token_cnt, TRUE, &redraw, &renorm );
    }
    else if ( strcmp( tokens[0], "tmx" ) == 0 )
    {
        analy->translate_material = TRUE;
        sscanf( tokens[1], "%d", &ival );
        sscanf( tokens[2], "%f", &val );
        if ( ival > 0 && ival <= analy->num_materials )
            analy->mtl_trans[0][ival-1] = val;
        reset_face_visibility( analy );
        renorm = TRUE;
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "tmy" ) == 0 )
    {
        analy->translate_material = TRUE;
        sscanf( tokens[1], "%d", &ival );
        sscanf( tokens[2], "%f", &val );
        if ( ival > 0 && ival <= analy->num_materials )
            analy->mtl_trans[1][ival-1] = val;
        reset_face_visibility( analy );
        renorm = TRUE;
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "tmz" ) == 0 )
    {
        analy->translate_material = TRUE;
        sscanf( tokens[1], "%d", &ival );
        sscanf( tokens[2], "%f", &val );
        if ( ival > 0 && ival <= analy->num_materials )
            analy->mtl_trans[2][ival-1] = val;
        reset_face_visibility( analy );
        renorm = TRUE;
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "clrtm" ) == 0
	      || strcmp( tokens[0], "clrem" ) == 0 )
    {
        analy->translate_material = FALSE;
        for ( i = 0; i < analy->num_materials; i++ )
            for ( j = 0; j < 3; j++ )
                analy->mtl_trans[j][i] = 0.0;
        reset_face_visibility( analy );
        renorm = TRUE;
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "emsph" ) == 0 )
    {
	if ( associate_matl_exp( token_cnt, tokens, analy, SPHERICAL ) != 0 )
            popup_dialog( USAGE_POPUP, 
	                  "emsph <x> <y> <z> <materials> [<name>]" );
    }
    else if ( strcmp( tokens[0], "emcyl" ) == 0 )
    {
	if ( associate_matl_exp( token_cnt, tokens, analy, CYLINDRICAL ) != 0 )
            popup_dialog( USAGE_POPUP, 
	                  "emcyl <x1> <y1> <z1> <x2> <y2> <z2> %s",
		          "<materials> [<name>]" );
    }
    else if ( strcmp( tokens[0], "emax" ) == 0 )
    {
	if ( associate_matl_exp( token_cnt, tokens, analy, AXIAL ) != 0 )
            popup_dialog( USAGE_POPUP, 
	                  "emax <x1> <y1> <z1> <x2> <y2> <z2> %s",
		          "<materials> [<name>]" );
    }
    else if ( strcmp( tokens[0], "em" ) == 0 )
    {
	if ( token_cnt < 2 )
	    popup_dialog( USAGE_POPUP, "em [<name>...] <distance>" );
	else
	{
	    analy->translate_material = TRUE;
	    explode_materials( token_cnt, tokens, analy, FALSE );
	    reset_face_visibility( analy );
	    renorm = TRUE;
	    redraw = TRUE;
	}
    }
    else if ( strcmp( tokens[0], "emsc" ) == 0 )
    {
	if ( token_cnt < 2 )
	    popup_dialog( USAGE_POPUP, "emsc [<name>...] <scale>" );
	else
	{
	    analy->translate_material = TRUE;
	    explode_materials( token_cnt, tokens, analy, TRUE );
	    reset_face_visibility( analy );
	    renorm = TRUE;
	    redraw = TRUE;
	}
    }
    else if ( strcmp( tokens[0], "tellem" ) == 0 
              || strcmp( tokens[0], "telem" ) == 0 )
    {
	report_exp_assoc();
    }
    else if ( strcmp( tokens[0], "emrm" ) == 0 )
    {
	remove_exp_assoc( token_cnt, tokens );
    }
    else if ( strcmp( tokens[0], "rnf" ) == 0 )
    {
        adjust_near_far( analy );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "near" ) == 0 )
    {
        if ( token_cnt == 2 && is_numeric_token( tokens[1] ) )
	{
            sscanf( tokens[1], "%f", &val );
            set_near( val );
            set_mesh_view();
            redraw = TRUE;
	}
	else
	    popup_dialog( USAGE_POPUP, "near <value>" );
    }
    else if ( strcmp( tokens[0], "far" ) == 0 )
    {
        if ( token_cnt == 2 && is_numeric_token( tokens[1] ) )
	{
            sscanf( tokens[1], "%f", &val );
            set_far( val );
            set_mesh_view();
            redraw = TRUE;
	}
	else
	    popup_dialog( USAGE_POPUP, "far <value>" );
    }
    else if ( strcmp( tokens[0], "lookfr" ) == 0 ||
              strcmp( tokens[0], "lookat" ) == 0 ||
              strcmp( tokens[0], "lookup" ) == 0 )
    {
        if ( token_cnt > 3 )
        {
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[1+i], "%f", &pt[i] );

            if ( strcmp( tokens[0], "lookfr" ) == 0 )
                look_from( pt );
            else if ( strcmp( tokens[0], "lookat" ) == 0 )
                look_at( pt );
            else
                look_up( pt );

            adjust_near_far( analy );
            redrawview = TRUE;
        }
        else
            wrt_text( "Look needs 3 values.\n" );
    }
    else if ( strcmp( tokens[0], "tfx" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        move_look_from( 0, val );
        adjust_near_far( analy );
        redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "tfy" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        move_look_from( 1, val );
        adjust_near_far( analy );
        redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "tfz" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        move_look_from( 2, val );
        adjust_near_far( analy );
        redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "tax" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        move_look_at( 0, val );
        adjust_near_far( analy );
        redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "tay" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        move_look_at( 1, val );
        adjust_near_far( analy );
        redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "taz" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        move_look_at( 2, val );
        adjust_near_far( analy );
        redrawview = TRUE;
    }
    else if ( strcmp( tokens[0], "camang" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        set_camera_angle( val, analy );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "bbsrc" ) == 0 )
    {
	if ( token_cnt == 2 )
	{
	    free( analy->bbox_source );
	    str_dup( &analy->bbox_source, tokens[1] );
	    parse_command( "bbox", analy );
	}
	else
	    popup_dialog( USAGE_POPUP, "bbsrc n|h|s|b|hs|hb|sb|hsb" );
    }
    else if ( strcmp( tokens[0], "bbox" ) == 0 )
    {
        ival = analy->dimension;
	if ( token_cnt <= 2 )
	{
	    redrawview = TRUE;
	    
	    if ( token_cnt == 1 )
                bbox_nodes( analy, analy->bbox_source, FALSE, pt, vec );
	    else if ( strcmp( tokens[1], "vis" ) == 0 )
                bbox_nodes( analy, "v", FALSE, pt, vec );
	    else
	        redrawview = FALSE;
	    
	    if ( redrawview )
	    {
	        if ( analy->keep_max_bbox_extent )
	        {
		    for ( i = 0; i < 3; i++ )
		    {
		        if ( pt[i] < analy->bbox[0][i] )
		            analy->bbox[0][i] = pt[i];
		        if ( vec[i] > analy->bbox[1][i] )
		            analy->bbox[1][i] = vec[i];
		    }
	        }
	        else
	        {
		    VEC_COPY( analy->bbox[0], pt );
		    VEC_COPY( analy->bbox[1], vec );
	        }
	    }
	}
	else if ( ival == 3 && token_cnt == 7 )
	{
	    for ( i = 0; i < 3; i++ )
	    {
	        pt[i] = atof( tokens[i + 1] );
	        vec[i] = atof( tokens[i + 4] );
	    }
	    if ( pt[0] < vec[0] && pt[1] < vec[1] && pt[2] < vec[2] )
            {
		VEC_COPY( analy->bbox[0], pt );
		VEC_COPY( analy->bbox[1], vec );
		redrawview = TRUE;
	    }
	}
	if ( !redrawview )
	    popup_dialog( USAGE_POPUP, 
	                  "bbox [%s | %s]"
			  "vis", "<xmin> <ymin> <zmin> <xmax> <ymax> <zmax>" );
	else
            set_view_to_bbox( analy->bbox[0], analy->bbox[1], ival );
    }
    else if ( strcmp( tokens[0], "timhis" ) == 0 )
    {
        time_hist_plot( NULL, analy );
    }
    else if ( strcmp( tokens[0], "outth" ) == 0 
              || strcmp( tokens[0],  "thsave" ) == 0 )
    {
        if ( token_cnt > 1 )
            time_hist_plot( tokens[1], analy );
        else
            wrt_text( "Filename needed for outth command.\n" );
    }
    else if ( strcmp( tokens[0], "mth" ) == 0 )
    {
        parse_mth_command( token_cnt, tokens, analy );
    }
    else if ( strcmp( tokens[0], "thsm" ) == 0 )
    {
        if ( token_cnt == 3 )
        {
            sscanf( tokens[1], "%d", &ival );
            analy->th_filter_width = ival;

            /* Filter type. */
	    if ( strcmp( tokens[2], "box" ) == 0 )
	        analy->th_filter_type = BOX_FILTER;
/* Not implemented.
	    else if ( strcmp( tokens[2], "tri" ) == 0 )
	        analy->th_filter_type = TRIANGLE_FILTER;
	    else if ( strcmp( tokens[2], "sync" ) == 0 )
	        analy->th_filter_type = SYNC_FILTER;
*/
	    else
	    {
		popup_dialog( WARNING_POPUP, 
		              "Unknown filter type \"%s\"; using \"box\".\n", 
		              tokens[2] );
		analy->th_filter_type = BOX_FILTER;
	    }
        }
        else
            popup_dialog( USAGE_POPUP, 
	                  "thsm <filter_width> <filter_type>" );
    }
    else if ( strcmp( tokens[0], "gather" ) == 0 )
    {
        parse_gather_command( token_cnt, tokens, analy );
    }
    else if ( strcmp( tokens[0], "vec" ) == 0 )
    {
        /* Get the result types from the translation table. */
        for ( i = 0; i < token_cnt - 1 && i < 3; i++ )
        {
	    if ( is_numeric_token( tokens[i+1] ) && atof( tokens[i+1] ) == 0.0 )
                analy->vec_id[i] = VAL_NONE;
	    else
                analy->vec_id[i] = lookup_result_id( tokens[i+1] );

            /* Error if we didn't get a match. */
            if ( analy->vec_id[i] < 0 )
            {
                wrt_text( "Result \"%s\" unrecognized, set to zero.\n", tokens[i+1] );
                analy->vec_id[i] = VAL_NONE;
            }
        }

        /* Any unspecified are set to zero. */
        for ( ; i < 3; i++ )
            analy->vec_id[i] = VAL_NONE;

        if ( analy->show_vectors )
            update_vec_points( analy );

        if ( analy->show_vectors || analy->show_carpet )
            redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "veccm" ) == 0 )
    {
        analy->vec_col_set = FALSE;
        analy->vec_hd_col_set = FALSE;
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "vecscl" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &analy->vec_scale );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "vhdscl" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &analy->vec_head_scale );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "vgrid1" ) == 0 )
    {
        if ( token_cnt > 6 )
        {
            sscanf( tokens[1], "%d", &sz[0] );
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+2], "%f", &pt[i] );
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+5], "%f", &vec[i] );
            gen_vec_points( 1, sz, pt, vec, analy );
        }
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "vgrid2" ) == 0 )
    {
        if ( token_cnt > 6 )
        {
            for ( i = 0; i < 2; i++ )
                sscanf( tokens[i+1], "%d", &sz[i] );
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+3], "%f", &pt[i] );
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+6], "%f", &vec[i] );
            gen_vec_points( 2, sz, pt, vec, analy );
        }
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "vgrid3" ) == 0 )
    {
        if ( token_cnt > 6 )
        {
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+1], "%d", &sz[i] );
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+4], "%f", &pt[i] );
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+7], "%f", &vec[i] );
            gen_vec_points( 3, sz, pt, vec, analy );
        }
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "clrvgr" ) == 0 )
    {
        DELETE_LIST( analy->vec_pts );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "carpet" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &analy->vec_cell_size );
        if ( analy->show_carpet )
        {
            gen_carpet_points( analy );
            redraw = TRUE;
        }
    }
    else if ( strcmp( tokens[0], "regcar" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &analy->reg_cell_size );
        if ( analy->show_carpet )
        {
            gen_reg_carpet_points( analy );
            redraw = TRUE;
        }
    }
    else if ( strcmp( tokens[0], "vecjf" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &analy->vec_jitter_factor );
        if ( analy->show_carpet )
            redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "veclf" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &analy->vec_length_factor );
        if ( analy->show_carpet )
            redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "vecif" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &analy->vec_import_factor );
        if ( analy->show_carpet )
            redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "prake" ) == 0 )
    {
        /*
         *
         * ORIGINAL
         *
        if ( token_cnt > 7 )
        {
            sscanf( tokens[1], "%d", &ival );
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+2], "%f", &pt[i] );
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+5], "%f", &vec[i] );
            if ( token_cnt > 8 )
            {
                for ( i = 0; i < 3; i++ )
                    sscanf( tokens[i+8], "%f", &rgb[i] );
            }
            else
                rgb[0] = -1;
            gen_trace_points( ival, pt, vec, rgb, analy );
        }
         *
         * REPLACEMENT
         */

        if ( 3 == analy->dimension )
            prake( token_cnt, tokens, &ival, pt, vec, rgb, analy );
	else
	    wrt_text( "Particle traces on 3D datasets only.\n" );
    }
    else if ( strcmp( tokens[0], "clrpar" ) == 0 )
    {
        free_trace_points( analy );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "ptrace" ) == 0 
              || strcmp( tokens[0], "aptrace" ) == 0 )
    {
        /* Start time. */
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &vec[0] );
        else
            vec[0] = analy->state_times[0];

        /* End time. */
        if ( token_cnt > 2 )
            sscanf( tokens[2], "%f", &vec[1] );
        else
            vec[1] = analy->state_times[analy->num_states-1];

        /* Time step size. */
        if ( token_cnt > 3 )
            sscanf( tokens[3], "%f", &val );
        else
            val = 0.0;
	
        /* "ptrace" deletes extant traces, leaving only new ones. */
        if ( tokens[0][0] == 'p' )
            DELETE_LIST( analy->trace_pts );
        
        particle_trace( vec[0], vec[1], val, analy, FALSE );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "ptstat" ) == 0 )
    {
	if ( token_cnt < 4 )
	    valid_command = FALSE;
	else
	{
	    switch( tokens[1][0] )
	    {
		case 's':
		    ival = atoi( tokens[2] );
		    if ( ival < 1 || ival > analy->num_states )
		        valid_command = FALSE;
		    else
		        vec[0] = analy->state_times[ival - 1];
		    break;
		case 't':
		    val = atof( tokens[2] );
		    if ( val < analy->state_times[0]
		         || val > analy->state_times[analy->num_states - 1] )
		        valid_command = FALSE;
		    else
		        vec[0] = val;
		    break;
		default:
		    valid_command = FALSE;	    
	    }
	}
	
	if ( valid_command )
	{
	    vec[1] = atof( tokens[3] );
	    val = ( token_cnt == 5 ) ? atof( tokens[4] ) : 0.0;
	    particle_trace( vec[0], vec[1], val, analy, TRUE );
            redraw = TRUE;
	}
	else
	    popup_dialog( USAGE_POPUP, "%s\n%s\n%s", 
	                  "ptstat t <time> <duration> [<delta t>]", 
	                  " OR ", 
			  "ptstat s <state> <duration> [<delta t>]" );
    }
    else if ( strcmp( tokens[0], "ptlim" ) == 0 )
    {
	ival = atoi( tokens[1] );
	analy->ptrace_limit = ( ival > 0 ) ? ival : 0;
	if ( analy->trace_pts != NULL )
	    redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "ptwid" ) == 0 )
    {
	val = atof( tokens[1] );
	if ( val <= 0.0 )
	{
	    popup_dialog( INFO_POPUP, "Trace width must be greater than 0.0." );
	    valid_command = FALSE;
	}
	else
	{
	    analy->trace_width = val;
	    if ( analy->show_traces )
	        redraw = TRUE;
	}
    }
    else if ( strcmp( tokens[0], "ptdis" ) == 0 )
    {
	if ( analy->trace_disable != NULL )
	{
	    free( analy->trace_disable );
	    analy->trace_disable = NULL;
	}
	analy->trace_disable_qty = 0;
	    
        if ( token_cnt > 1 )
	{
            /* Store material numbers for disabling particle traces. */
	    analy->trace_disable = NEW_N( int, token_cnt - 1, "Trace disable" );
	    for ( i = 1, j = 0; i < token_cnt; i++ )
	    {
	        ival = atoi( tokens[i] );
	        if ( ival > 0 && ival <= analy->num_materials )
	            analy->trace_disable[j++] = ival - 1;
	        else
	            wrt_text( "Invalid material \"%d\" ignored.\n", ival );
	    }
	
	    if ( j == 0 )
	    {
	        free( analy->trace_disable );
		analy->trace_disable = NULL;
	    }
	    else
	        analy->trace_disable_qty = j;
        }
	
	redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "outpt" ) == 0 )
    {
	if ( token_cnt == 2 )
            save_particle_traces( analy, tokens[1] );
	else
	    popup_dialog( USAGE_POPUP, "outpt <filename>" );
    }
    else if ( strcmp( tokens[0], "inpt" ) == 0 )
    {
	if ( token_cnt == 2 )
            read_particle_traces( analy, tokens[1] );
	else
	    popup_dialog( USAGE_POPUP, "inpt <filename>" );
    }
    else if ( strcmp( tokens[0], "maxst" ) == 0 )
    {
        sscanf( tokens[1], "%d", &ival );
        analy->num_states = ival;
    }
    else if ( strcmp( tokens[0], "state" ) == 0 ||
              strcmp( tokens[0], "n" ) == 0 ||
              strcmp( tokens[0], "p" ) == 0 ||
              strcmp( tokens[0], "f" ) == 0 ||
              strcmp( tokens[0], "l" ) == 0 )
    {
        if ( strcmp( tokens[0], "state" ) == 0 )
        {
            if ( token_cnt < 2 )
                wrt_text( "State needs a state number.\n" );
            else
            {
                sscanf( tokens[1], "%d", &i );
                i--;
                if ( i != analy->cur_state && i < analy->num_states &&
                     i >= 0 )
                {
                    analy->cur_state = i;
                }
                else if ( i >= analy->num_states )
                    wrt_text( "Only %d states.\n", analy->num_states );
                else if ( i == analy->cur_state )
                    wrt_text( "Already at state %d.\n", i+1 );
            }
        }
        else if ( strcmp( tokens[0], "n" ) == 0 )
        {
            if ( analy->cur_state < analy->num_states - 1 )
                analy->cur_state += 1;
            else
	        popup_dialog( INFO_POPUP, "Already at last state..." );
        }
        else if ( strcmp( tokens[0], "p" ) == 0 )
        {
            if ( analy->state_p->time <= analy->state_times[analy->cur_state] )
	        if ( analy->cur_state > 0 )
		    analy->cur_state -= 1;
	        else
	            popup_dialog( INFO_POPUP, "Already at first state..." );
        }
        else if ( strcmp( tokens[0], "f" ) == 0 )
            analy->cur_state = 0;
        else if ( strcmp( tokens[0], "l" ) == 0 )
            analy->cur_state = analy->num_states-1;
 
        change_state( analy );
        redraw = TRUE;
    } 
    else if ( strcmp( tokens[0], "time" ) == 0 )
    {
        if ( token_cnt == 2 )
        {
            sscanf( tokens[1], "%f", &val );
            if ( val <= analy->state_times[0] )
            {
                analy->cur_state = 0;
                change_state( analy );
            }
            else if ( val >= analy->state_times[analy->num_states-1] )
            {
                analy->cur_state = analy->num_states-1;
                change_state( analy );
            }
            else
                change_time( val, analy );

            redraw = TRUE;
        }
        else
            wrt_text( "Gototime needs a time value.\n" );
    }
    else if ( strcmp( tokens[0], "anim" ) == 0 )
    {
        parse_animate_command( token_cnt, tokens, analy );
    }
    else if ( strcmp( tokens[0], "animc" ) == 0 )
    {
        continue_animate( analy );
    }
    else if ( strcmp( tokens[0], "stopan" ) == 0 )
    {
        end_animate_workproc( TRUE );
    }
    else if ( strcmp( tokens[0], "cutpln" ) == 0 )
    {   
        if ( token_cnt < 7 )
            wrt_text( "Cutplane needs 6 arguments.\n" );
        else
        {
            Plane_obj *plane;

            plane = NEW( Plane_obj, "Cutting plane" );
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+1], "%f", &(plane->pt[i]) );
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+4], "%f", &(plane->norm[i]) );
            APPEND( plane, analy->cut_planes );

            if ( analy->show_roughcut )
            {
                reset_face_visibility( analy );
                renorm = TRUE;
            }
            gen_cuts( analy );
            gen_carpet_points( analy );
            redraw = TRUE;
        }
    }
    else if ( strcmp( tokens[0], "clrcut" ) == 0 )
    {
        DELETE_LIST( analy->cut_planes );
        if ( analy->show_roughcut )
        {
            reset_face_visibility( analy );
            renorm = TRUE;
        }
        DELETE_LIST( analy->cut_poly );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "ison" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%d", &ival );
        else
            ival = 10;
        set_contour_vals( ival, analy );
        gen_contours( analy );
        gen_isosurfs( analy );
        gen_carpet_points( analy );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "isop" ) == 0 )
    {
        if ( token_cnt > 1 )
        {
            sscanf( tokens[1], "%f", &val );
            add_contour_val( val, analy );
            gen_contours( analy );
            gen_isosurfs( analy );
            gen_carpet_points( analy );
            redraw = TRUE;
        }
    }
    else if ( strcmp( tokens[0], "isov" ) == 0 )
    {
        if ( token_cnt > 1 )
        {
            sscanf( tokens[1], "%f", &val );
            if ( analy->result_id != VAL_NONE )
            {
	        /*
		 * If units conversion is turned on, assume the user-
		 * specified value is in converted units and apply the
		 * inverse conversion so that the value is stored
		 * internally in "primal" units.
		 */
	        if ( analy->perform_unit_conversion )
		    val = (val - analy->conversion_offset) 
		          / analy->conversion_scale;

                add_contour_val( (val - analy->result_mm[0]) /
                                 (analy->result_mm[1] - analy->result_mm[0]),
                                 analy ); 
            }
            gen_contours( analy );
            gen_isosurfs( analy );
            gen_carpet_points( analy );
            redraw = TRUE;
        }
    }
    else if ( strcmp( tokens[0], "clriso" ) == 0 )
    {
        if ( analy->contour_cnt > 0 )
        {
            free( analy->contour_vals );
            analy->contour_vals = NULL;
            analy->contour_cnt = 0;
        }
        free_contours( analy );
        DELETE_LIST( analy->isosurf_poly );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "telliso" ) == 0 
              || strcmp( tokens[0], "teliso" ) == 0 )
    {
	report_contour_vals( analy );
    }
    else if ( strcmp( tokens[0], "conwid" ) == 0 )
    {
	val = atof( tokens[1] );
	if ( val <= 0.0 )
	{
	    popup_dialog( INFO_POPUP, "Contour width must be greater than 0.0." );
	    valid_command = FALSE;
	}
	else
	{
	    analy->contour_width = val;
	    if ( analy->show_contours )
	        redraw = TRUE;
	}
    }
    else if ( strcmp( tokens[0], "sym" ) == 0 )
    {
        if ( token_cnt < 7 )
            wrt_text( "Reflectplane needs 6 arguments.\n" );
        else
        {
            Refl_plane_obj *rplane;

            rplane = NEW( Refl_plane_obj, "Reflection plane" );
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+1], "%f", &(rplane->pt[i]) );
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+4], "%f", &(rplane->norm[i]) );

            /* Generate reflection transform mats. */
            create_reflect_mats( rplane );

            APPEND( rplane, analy->refl_planes );
            redraw = TRUE;
        }
    }
    else if ( strcmp( tokens[0], "clrsym" ) == 0 )
    {
        DELETE_LIST( analy->refl_planes );
        redraw = TRUE;
    }
    else if( strcmp( tokens[0], "outhdf" ) == 0 ||
             strcmp( tokens[0], "snap" ) == 0 )
    {
        popup_dialog( INFO_POPUP, 
	              "HDF is no longer supported.  Try outrgb/outrgba." );
    }
    else if( strcmp( tokens[0], "outrgb" ) == 0 )
    {
        if ( token_cnt > 1 )
            screen_to_rgb( tokens[1], FALSE );
        else
            wrt_text( "Filename needed for outrgb command.\n" );
    }
    else if( strcmp( tokens[0], "outrgba" ) == 0 )
    {
        if ( token_cnt > 1 )
            screen_to_rgb( tokens[1], TRUE );
        else
            wrt_text( "Filename needed for outrgba command.\n" );
    }
    else if( strcmp( tokens[0], "outps" ) == 0 )
    {
        if ( token_cnt > 1 )
            screen_to_ps( tokens[1] );
        else
            wrt_text( "Filename needed for outps command.\n" );
    }
     else if( strcmp( tokens[0], "ldhmap" ) == 0 )
    {
        wrt_text( "HDF palettes are no longer supported.\n" );
	wrt_text( "Convert HDF palettes with \"h2g\" utility," );
	wrt_text( "  then use \"ldmap\" instead." );
    }
    else if( strcmp( tokens[0], "ldmap" ) == 0 )
    {
        if ( token_cnt > 1 )
            read_text_colormap( tokens[1] );
        else
            wrt_text( "Filename needed for loadcolormap command.\n" );
        user_cmap_loaded = TRUE;
        redraw = TRUE;
    }
    else if( strcmp( tokens[0], "posmap" ) == 0 )
    {
        if ( token_cnt > 4 )
        {
            sscanf( tokens[1], "%f", &analy->colormap_pos[0] );
            sscanf( tokens[2], "%f", &analy->colormap_pos[1] );
            sscanf( tokens[3], "%f", &analy->colormap_pos[2] );
            sscanf( tokens[4], "%f", &analy->colormap_pos[3] );
            analy->use_colormap_pos = TRUE;
        }
        else
            wrt_text( "Four arguments needed for colormap position.\n" );
        redraw = TRUE;
    }
    else if( strcmp( tokens[0], "hotmap" ) == 0 )
    {
        hot_cold_colormap();
        redraw = TRUE;
    }
    else if( strcmp( tokens[0], "grmap" ) == 0 )
    {
        gray_colormap( FALSE );
        redraw = TRUE;
    }
    else if( strcmp( tokens[0], "igrmap" ) == 0 )
    {
        gray_colormap( TRUE );
        redraw = TRUE;
    }
    else if( strcmp( tokens[0], "invmap" ) == 0 )
    {
        invert_colormap();
        redraw = TRUE;
    }
    else if( strcmp( tokens[0], "conmap" ) == 0 ||
             strcmp( tokens[0], "chmap" ) == 0 ||
             strcmp( tokens[0], "cgmap" ) == 0 )
    {
        /* Load the smooth colormap first. */
        if ( strcmp( tokens[0], "chmap" ) == 0 )
            hot_cold_colormap();
        else if ( strcmp( tokens[0], "cgmap" ) == 0 )
            gray_colormap( FALSE );

        /* Now contour it. */
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%d", &ival );
        else
            ival = 6;
        contour_colormap( ival );

        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "setcol" ) == 0 )
    {
        if ( token_cnt > 4 )
        {
            sscanf( tokens[2], "%f", &rgb[0] );
            sscanf( tokens[3], "%f", &rgb[1] );
            sscanf( tokens[4], "%f", &rgb[2] );
            set_color( tokens[1], rgb[0], rgb[1], rgb[2] );
            if ( strcmp( tokens[1], "vec" ) == 0 )
                analy->vec_col_set = TRUE;
            else if ( strcmp( tokens[1], "vechd" ) == 0 )
                analy->vec_hd_col_set = TRUE;
            else if ( strcmp( tokens[1], "rmin" ) == 0 
	              || strcmp( tokens[1], "rmax" ) == 0 )
	        set_cutoff_colors( TRUE, analy->mm_result_set[0], 
		                   analy->mm_result_set[1] );
        }
        else
            wrt_text( "Four arguments needed for setcol.\n" );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "savhis" ) == 0 )
    {
        if ( token_cnt > 1 )
            open_history_file( tokens[1] );
        else
            wrt_text( "Filename needed for savehistory command.\n" );

        /* Don't want to save _this_ command. */
        valid_command = FALSE;
    }
    else if ( strcmp( tokens[0], "endhis" ) == 0 )
    {
        close_history_file();
    }
    else if ( strcmp( tokens[0], "rdhis" ) == 0 ||
              strcmp( tokens[0], "h" ) == 0 )
    {
        if ( token_cnt > 1 )
            read_history_file( tokens[1], analy );
        else
            wrt_text( "Filename needed for readhistory command.\n" );
    }
    else if ( strcmp( tokens[0], "loop" ) == 0 )
    {
        if ( token_cnt > 1 )
        {
            while ( TRUE )
                read_history_file( tokens[1], analy );
        }
        else
            wrt_text( "Filename needed for loop command.\n" );
    }
    else if ( strcmp( tokens[0], "getedg" ) == 0 )
    {
        get_mesh_edges( analy );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "crease" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        crease_threshold = cos( (double)DEG_TO_RAD( 0.5 * val ) );
        explicit_threshold = cos( (double)DEG_TO_RAD( val ) );
        get_mesh_edges( analy );
        renorm = TRUE;
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "edgbias" ) == 0 )
    {
        if ( token_cnt != 2 )
	    popup_dialog( USAGE_POPUP, "edgbias <bias value>" );
	else
	{
            sscanf( tokens[1], "%f", &val );
            analy->edge_zbias = val;
            redraw = TRUE;
	}
    }
    else if ( strcmp( tokens[0], "rzero" ) == 0 )
    {
        sscanf( tokens[1], "%f", &analy->zero_result );
        /*
         * If units conversion is turned on, assume the user-specified 
	 * value is in converted units and apply the inverse conversion 
	 * so that the value is stored internally in "primal" units.
         */
        if ( analy->perform_unit_conversion )
	    analy->zero_result = (analy->zero_result 
	                           - analy->conversion_offset) 
				  / analy->conversion_scale;
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "rmax" ) == 0 )
    {
        sscanf( tokens[1], "%f", &analy->result_mm[1] );
        /*
         * If units conversion is turned on, assume the user-specified 
	 * value is in converted units and apply the inverse conversion 
	 * so that the value is stored internally in "primal" units.
         */
        if ( analy->perform_unit_conversion )
	    analy->result_mm[1] = (analy->result_mm[1] 
	                           - analy->conversion_offset) 
				  / analy->conversion_scale;
        analy->mm_result_set[1] = TRUE;
	set_cutoff_colors( TRUE, 
	                   analy->mm_result_set[0], analy->mm_result_set[1] );
/**/
/*
        if ( !user_cmap_loaded )
            cutoff_colormap( analy->mm_result_set[0], analy->mm_result_set[1] );
*/
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "rmin" ) == 0 )
    {
        sscanf( tokens[1], "%f", &analy->result_mm[0] );
        /*
         * If units conversion is turned on, assume the user-specified 
	 * value is in converted units and apply the inverse conversion 
	 * so that the value is stored internally in "primal" units.
         */
        if ( analy->perform_unit_conversion )
	    analy->result_mm[0] = (analy->result_mm[0] 
	                           - analy->conversion_offset) 
				  / analy->conversion_scale;
        analy->mm_result_set[0] = TRUE;
	set_cutoff_colors( TRUE, 
	                   analy->mm_result_set[0], analy->mm_result_set[1] );
/**/
/*
        if ( !user_cmap_loaded )
            cutoff_colormap( analy->mm_result_set[0], analy->mm_result_set[1] );
*/
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "clrthr" ) == 0 )
    {
	set_cutoff_colors( FALSE, 
	                   analy->mm_result_set[0], analy->mm_result_set[1] );
        analy->zero_result = 0.0;
        analy->mm_result_set[0] = FALSE;
        analy->mm_result_set[1] = FALSE;
        if ( analy->use_global_mm )
        {
            analy->result_mm[0] = analy->global_mm[0];
            analy->result_mm[1] = analy->global_mm[1];
        }
        else
        {
            analy->result_mm[0] = analy->state_mm[0];
            analy->result_mm[1] = analy->state_mm[1];
        }
/**/
/*
        if ( !user_cmap_loaded )
            hot_cold_colormap();
*/
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "globmm" ) == 0 )
    {
        get_global_minmax( analy );
	redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "tellmm" ) == 0 )
    {
        if ( token_cnt == 1 )
        {
            /*
             * Undocumented format:
             *
             * tellmm <no arguments>    --     display minimums and maximums
             *                                 for all states of current display result
             */

            result_variable[0] = '\0';
            start_state        = 1;
            stop_state         = analy->num_states;

            tellmm( analy, result_variable, start_state, stop_state, &tellmm_redraw );

            redraw = tellmm_redraw;
        }
        else if ( token_cnt == 2 )
        {
            if ( ((lookup_result_id( tokens[1] )) != -1) &&
                 ((lookup_result_id( tokens[1] )) != VAL_NONE) )
            {
                /*
                 * tellmm valid_result; NOTE:  result MAY NOT be materials
                 */

                start_state = 1;
                stop_state  = analy->num_states;

                tellmm( analy, tokens[1], start_state, stop_state, &tellmm_redraw );

                redraw = tellmm_redraw;
            }
            else
            {
                /*
                 * tellmm invalid_result
                 */

                popup_dialog( USAGE_POPUP
                             ,"tellmm [<result> [<first state> [<last state>]]]" );
            }
        }
        else if ( token_cnt == 3 )
        {
            if ( ((lookup_result_id( tokens[1] )) != -1)       &&
                 ((lookup_result_id( tokens[1] )) != VAL_NONE) &&
                 (((int)strtol( tokens[2], (char **)NULL, 10 )) != 0) )
            {
                /*
                 * tellmm valid_result state_number
                 *
                 * NOTE:  result MAY NOT be materials;
                 *        state_number != 0
                 */

                start_state        = (int)strtol( tokens[2], (char **)NULL, 10);
                stop_state         = start_state;

                tellmm( analy, tokens[1], start_state, stop_state, &tellmm_redraw );

                redraw = tellmm_redraw;
            }
            else
            {
                /*
                 * tellmm invalid_result
                 */

                popup_dialog( USAGE_POPUP
                             ,"tellmm [<result> [<first state> [<last state>]]]" );
            }
        }
        else if ( token_cnt == 4 )
        {

            if ( ((lookup_result_id( tokens[1] )) != -1)              &&
                 ((lookup_result_id( tokens[1] )) != VAL_NONE)        &&
                 (((int)strtol( tokens[2], (char **)NULL, 10 )) != 0) &&
                 (((int)strtol( tokens[3], (char **)NULL, 10 )) != 0) )
            {
                /*
                 * tellmm result state_number state_number
                 *
                 * NOTE:  result MAY NOT be materials;
                 *        state_number != 0
                 */

                start_state = (int)strtol( tokens[2], (char **)NULL, 10);
                stop_state  = (int)strtol( tokens[3], (char **)NULL, 10);

                tellmm( analy, tokens[1], start_state, stop_state, &tellmm_redraw );

                redraw = tellmm_redraw;
            }
            else
            {
                /*
                 * tellmm invalid_result
                 */

                popup_dialog( USAGE_POPUP
                             ,"tellmm [<result> [<first state> [<last state>]]]" );
            }
        }
        else
        {
            popup_dialog( USAGE_POPUP
                         ,"tellmm [<result> [<first state> [<last state>]]]" );
        }
    }
    else if ( strcmp( tokens[0], "pause" ) == 0 )
    {
        if ( token_cnt > 1 )
           sscanf( tokens[1], "%d", &ival );
        else
           ival = 10;
        sleep( ival );
    }
    else if ( strcmp( tokens[0], "dellit" ) == 0 )
    {
        delete_lights( analy );    
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "light" ) == 0 )
    {
        if ( token_cnt >= 6 )
	{
            set_light( token_cnt, tokens );
            redraw = TRUE;
	}
	else
	    popup_dialog( USAGE_POPUP, 
	                  "light n x y z w [amb r g b] [diff r g b]\n%s%s", 
	                  "      [spec r g b] [spotdir x y z] ", 
			  "[spot exp ang]" );
    }
    else if ( strcmp( tokens[0], "tlx" ) == 0 )
    {
        sscanf( tokens[1], "%d", &ival );
        sscanf( tokens[2], "%f", &val );
        move_light( ival-1, 0, val );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "tly" ) == 0 )
    {
        sscanf( tokens[1], "%d", &ival );
        sscanf( tokens[2], "%f", &val );
        move_light( ival-1, 1, val );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "tlz" ) == 0 )
    {
        sscanf( tokens[1], "%d", &ival );
        sscanf( tokens[2], "%f", &val );
        move_light( ival-1, 2, val );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "mat" ) == 0 )
    {
        set_material( token_cnt, tokens );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "alias" ) == 0 )
    {
        create_alias( token_cnt, tokens );
    }
    else if ( strcmp( tokens[0], "resttl" ) == 0 )
    {
        ival = lookup_result_id( tokens[1] );
        if ( ival < 0 )
            wrt_text( "Result unrecognized: %s\n", tokens[1] );
        else
        {
            i = resultid_to_index[ival];
            trans_result[i][1] =
                    NEW_N( char, strlen(tokens[2])+1, "Result title" );
            strcpy( trans_result[i][1], tokens[2] );
            if ( analy->result_id == ival )
                strcpy( analy->result_title, tokens[2] );
            redraw = TRUE;
        }
    }
    else if ( strcmp( tokens[0], "title" ) == 0 )
    {
        strcpy( analy->title, tokens[1] );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "echo" ) == 0 )
        wrt_text( "%s\n", tokens[1] );
    else if ( strcmp( tokens[0], "vidti" ) == 0 )
    {
        sscanf( tokens[1], "%d", &ival );
        set_vid_title( ival-1, tokens[2] );
    }
    else if ( strcmp( tokens[0], "vidttl" ) == 0 )
    {
        draw_vid_title();
    }
    else if ( strcmp( tokens[0], "dscal" ) == 0 )
    {
        analy->displace_exag = TRUE;
        sscanf( tokens[1], "%f", &val );
        VEC_SET( analy->displace_scale, val, val, val );
        if ( val == 1.0 )
            analy->displace_exag = FALSE;
        else
            analy->displace_exag = TRUE;
        renorm = TRUE;
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "dscalx" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        analy->displace_scale[0] = val;
        analy->displace_exag = TRUE;
        renorm = TRUE;
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "dscaly" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        analy->displace_scale[1] = val;
        analy->displace_exag = TRUE;
        renorm = TRUE;
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "dscalz" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        analy->displace_scale[2] = val;
        analy->displace_exag = TRUE;
        renorm = TRUE;
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "exec" ) == 0 )
    {
        if ( token_cnt > 1 )
            system( tokens[1] );
    }
    else if ( strcmp( tokens[0], "inslp" ) == 0 )
    {
        if ( token_cnt > 1 )
            read_slp_file( tokens[1], analy );
        else
            wrt_text( "Filename needed for inslp command.\n" );
    }
    else if ( strcmp( tokens[0], "inref" ) == 0 )
    {
        if ( token_cnt > 1 )
            read_ref_file( tokens[1], analy );
        else
            wrt_text( "Filename needed for inref command.\n" );
    }
    else if ( strcmp( tokens[0], "genref" ) == 0 )
    {
        if ( tokens[1][0] == 'x' || tokens[1][0] == 'X' )
            ival = 0;
        if ( tokens[1][0] == 'y' || tokens[1][0] == 'Y' )
            ival = 1;
        else
            ival = 2;

        sscanf( tokens[2], "%f", &val );

        gen_ref_from_coord( analy, ival, val );
    }
    else if ( strcmp( tokens[0], "refave" ) == 0 )
    {
        val = get_ref_average_area( analy );
        if ( val == 0.0 )
            wrt_text( "No reference faces present!\n" );
        else
        {
            wrt_text( "Average reference face area: %f\n", val );
            wrt_text( "1 / average reference face area: %f\n", 1.0/val );
        }
    }
    else if ( strcmp( tokens[0], "triave" ) == 0 )
    {
        val = get_tri_average_area( analy );
        if ( val == 0.0 )
            wrt_text( "No triangles present!\n" );
        else
        {
            wrt_text( "Average triangle area: %f\n", val );
            wrt_text( "1 / average triangle area: %f\n", 1.0/val );
        }
    }
    else if ( strcmp( tokens[0], "clrref" ) == 0 )
    {
        DELETE_LIST( analy->ref_polys );
        redraw = TRUE;
    }
    else if ( strcmp( tokens[0], "outref" ) == 0 )
    {
	write_ref_file( tokens, token_cnt, analy );
    }
    else if ( strcmp( tokens[0], "outobj" ) == 0 )
    {
        if ( token_cnt > 1 )
            write_obj_file( tokens[1], analy );
        else
            popup_dialog( USAGE_POPUP, "outobj <filename>" );
    }
    else if ( strcmp( tokens[0], "outhid" ) == 0 )
    {
        if ( token_cnt > 1 )
            write_hid_file( tokens[1], analy );
        else
            popup_dialog( USAGE_POPUP, "outhid <filename>" );
    }
    else if ( strcmp( tokens[0], "outvc" ) == 0 )
    {
        if ( token_cnt > 1 )
            write_vv_connectivity_file( tokens[1], analy );
        else
            popup_dialog( USAGE_POPUP, "outvc <filename>" );
    }
    else if ( strcmp( tokens[0], "outvv" ) == 0 )
    {
        if ( token_cnt > 1 )
            write_vv_state_file( tokens[1], analy );
        else
            popup_dialog( USAGE_POPUP, "outvv <filename>" );
    }
    else if ( strcmp( tokens[0], "pref" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        set_pr_ref( val );
    }
    else if ( strcmp( tokens[0], "dia" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        set_diameter( val );
    }
    else if ( strcmp( tokens[0], "ym" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        set_youngs_mod( val );
    }
    else if ( strcmp( tokens[0], "outvec" ) == 0 )
    {
        if ( token_cnt == 2 )
            outvec( tokens[1], analy );
        else
            wrt_text( "Filename needed for outvec command.\n" );
    }
#ifdef VIDEO_FRAMER
    else if ( strcmp( tokens[0], "vidsel" ) == 0 )
    {
        /* Initialize the video framer and select output type.
         * Default to Beta.
         */
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%d", &ival );
        else
            ival = 2;
        vid_select( ival );
    }
    else if ( strcmp( tokens[0], "vidini" ) == 0 )
    {
        /* Initialize the V-LAN. */
        if ( !vid_vlan_init() )
            wrt_text( "Routine vid_vlan_init returned with error.\n" );
    }
    else if ( strcmp( tokens[0], "vidrec" ) == 0 )
    {
        int tries;
        tries = 0;

        /* Default to recording 1 frame. */
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%d", &ival );
        else
            ival = 1;

        while ( !vid_vlan_rec( 1, ival ) )
        {
            tries++;
            wrt_text( 
              "Routine vid_vlan_rec returned with error on try %d.\n", tries );
            sleep( 3 );

            if ( tries > 10 )
            {
                wrt_text( "Giving up on record.\n" );
                break;
            }
        }
    }
    else if ( strcmp( tokens[0], "vidrnm" ) == 0 )
    {
        int tries;
        tries = 0;

        /* Default to recording 1 frame. */
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%d", &ival );
        else
            ival = 1;

        while ( !vid_vlan_rec( 0, ival ) )
        {
            tries++;
            wrt_text( 
              "Routine vid_vlan_rec returned with error on try %d.\n", tries );
            sleep( 3 );
        }
    }
    else if ( strcmp( tokens[0], "vidmov" ) == 0 )
    {
        int tries;
        tries = 0;

        while ( !vid_vlan_rec( 1, 0 ) )
        {
            tries++;
            wrt_text( 
              "Routine vid_vlan_rec returned with error on try %d.\n", tries );
            sleep( 3 );
        }
    }
    else if ( strcmp( tokens[0], "vidcb" ) == 0 )
        vid_cbars();
#endif VIDEO_FRAMER
    /* Null strings don't become tokens. */
    else
    {
        wrt_text( "Command \"%s\" not valid.\n", tokens[0] );
        valid_command = FALSE;
    }

    if ( renorm )
        compute_normals( analy );

    if ( redraw || redrawview )
    {
	    if ( analy->center_view )
            center_view( analy );
	    if ( analy->refresh )
            update_display( analy );
    }
    analy->result_mod = FALSE;
    
    if ( valid_command )
        history_command( buf );

    if ( strcmp( tokens[0], "r" ) != 0 )
        strcpy( last_command, buf );
} 

 
/*****************************************************************
 * TAG( parse_mtl_cmd )
 * 
 * Parse a material manager command.
 */
static void
parse_mtl_cmd( analy, tokens, token_cnt, src_mtl_mgr, p_redraw, p_renorm )
Analysis *analy;
char tokens[MAXTOKENS][TOKENLENGTH];
int token_cnt;
Bool_type src_mtl_mgr;
Bool_type *p_redraw;
Bool_type *p_renorm;
{
    int i, first_token, cnt, read, mtl_qty, qty_vals;
    static Bool_type preview_set = FALSE;
    static Bool_type incomplete_cmd = FALSE;
    Bool_type parsing, redraw;
    static Material_property_obj mtl_props;
    static GLfloat *save_props[MTL_PROP_QTY];
    GLfloat *p_src, *p_dest, *bound;
    Material_property_type j;
    
    if ( token_cnt == 1 )
	return;
    
    mtl_qty = ( analy->num_materials < MAX_MATERIALS )
              ? analy->num_materials : MAX_MATERIALS;
    
    if ( src_mtl_mgr )
        first_token = 1;
    else
        first_token = 0;

    i = first_token;
    cnt = token_cnt - first_token;
    parsing = TRUE;
    while( parsing )
    {
        if ( incomplete_cmd == TRUE )
	{
            read = parse_embedded_mtl_cmd( analy, tokens + i, cnt, 
	                                   preview_set, save_props, 
					   p_renorm );
	    incomplete_cmd = FALSE;
	    if ( read + first_token >= token_cnt )
	    {
		parsing = FALSE;
	        redraw = TRUE;
	    }
	    else
	        i += read;
	}
	else if ( strcmp( tokens[i], "preview" ) == 0 )
	{
	    i++;
	    preview_set = TRUE;
	}
	else if ( strcmp( tokens[i], "apply" ) == 0 )
	{
	    /* Free saved copies of material properties. */
	    for ( j = AMBIENT; j < MTL_PROP_QTY; j++ )
	    {
		if ( save_props[j] != NULL )
		{
		    free( save_props[j] );
		    save_props[j] = NULL;
		}
	    }
	    preview_set = FALSE;
	    parsing = FALSE;
	    redraw = FALSE;
	}
	else if ( strcmp( tokens[i], "cancel" ) == 0 )
	{
	    /* Replace current structures with saved copies. */
	    for ( j = AMBIENT; j < MTL_PROP_QTY; j++ )
	    {
		if ( save_props[j] != NULL )
		{
		    if ( j == AMBIENT )
			p_dest = &v_win->matl_ambient[0][0];
		    else if ( j == DIFFUSE )
			p_dest = &v_win->matl_diffuse[0][0];
		    else if ( j == SPECULAR )
			p_dest = &v_win->matl_specular[0][0];
		    else if ( j == EMISSIVE )
			p_dest = &v_win->matl_emission[0][0];
		    else if ( j == SHININESS )
			p_dest = &v_win->matl_shininess[0];
		    
		    qty_vals = ( j == SHININESS ) ? mtl_qty : 4 * mtl_qty;
		    bound = &save_props[j][0] + qty_vals;
		    p_src = &save_props[j][0];
		    
		    for ( ; p_src < bound; *p_dest++ = *p_src++ );
		    free( save_props[j] );
		    save_props[j] = NULL;
		}
	    }
	    preview_set = FALSE;
	    parsing = FALSE;
	    redraw = TRUE;
	}
	else if ( strcmp( tokens[i],  "continue" ) == 0 )
	{
	    parsing = FALSE;
	    incomplete_cmd = TRUE;
	    redraw = FALSE;
	}
	else
	{
            read = parse_embedded_mtl_cmd( analy, tokens + i, cnt, 
	                                   preview_set, save_props, 
					   p_renorm );
	    if ( read + first_token >= token_cnt )
	    {
		parsing = FALSE;
	        redraw = TRUE;
	    }
	    else
	        i += read;
	}
    }
    
    *p_redraw = redraw;
}
 
 
/*****************************************************************
 * TAG( parse_embedded_mtl_cmd )
 * 
 * Parse a subdivision of a material manager command.  This may
 * be a typical "mat", "invis", etc. command, or a (new)
 * combination command ("invis disable...") or one with multiple
 * materials as operands.
 * 
 * This command is stateful so that multiple calls may be used
 * to complete one command.
 */
static int
parse_embedded_mtl_cmd( analy, tokens, cnt, preview, save, p_renorm )
Analysis *analy;
char tokens[][TOKENLENGTH];
int cnt;
Bool_type preview;
GLfloat *save[MTL_PROP_QTY];
Bool_type *p_renorm;
{
    static Bool_type visible = FALSE;
    static Bool_type invisible = FALSE;
    static Bool_type enable = FALSE;
    static Bool_type disable = FALSE;
    static Bool_type mat = FALSE;
    static Bool_type modify_all = FALSE;
    static Bool_type apply_defaults = FALSE;
    static Bool_type reading_materials = FALSE;
    static Bool_type reading_cprop[MTL_PROP_QTY];
    static Material_property_obj props;
    int i, limit, mtl, mqty, k;
    Material_property_type j;
    GLfloat *dest;
    
    i = 0;
    
    /* 
     * Finish up any partial segment.
     */
    if ( reading_materials )
    {
        /* Read material numbers until end of list or a word is encountered. */
	while ( is_numeric_token( tokens[i] ) && i < cnt )
	{
	    mtl = (atoi ( tokens[i] ) - 1) % MAX_MATERIALS;
	    props.materials[props.mtl_cnt++] = mtl;
	    i++;
	}
	
	/*
	 * If we ended reading materials for any reason other than
	 * encountering the "continue" token, we're done reading materials.
	 * We could be done even if "continue" occurs, but that can't be
	 * known until the next call.
	 */
	if ( i == cnt || strcmp( tokens[i], "continue" ) != 0 )
	    reading_materials = FALSE;
	else
	    return i;
    }
    else if ( mat && props.cur_idx != 0 )
    {
        /*
	 * Must be parsing a color property.  This assumes that when the
	 * "mat" string is parsed there will always be at least one material
	 * number to parse IN THE SAME CALL.  This is a safe assumption as
	 * "mat" will be at most the third token and MAX_TOKENS should never
	 * need to be set small (i.e., less than four), therefore we will
	 * have at least started reading material numbers before encountering
	 * the end of the tokens.  To have dropped into this block, reading
	 * materials will have been finished, so if anything remains to be
	 * read it must be color properties.
	 */
	for ( j = AMBIENT; j < MTL_PROP_QTY; j++ )
	    if ( reading_cprop[j] )
		break;
	
	if ( j == SHININESS )
	    props.color_props[0] = atof( tokens[0] );
	else if ( j != MTL_PROP_QTY )
	{
	    limit = 3 - props.cur_idx;
	    for ( ; i < limit; i++ )
	        props.color_props[props.cur_idx++] = 
		    (GLfloat) atof( tokens[i] );
	    props.cur_idx = 0;
	}
    }
    
    /* Parse (rest of) tokens. */
    for ( ; i < cnt; i++ )
    {
        if ( is_numeric_token( tokens[i] ) )
	{
	    /* Material numbers precede any other numeric tokens. */
	    if ( props.mtl_cnt == 0 )
	        reading_materials = TRUE;
	    
	    if ( reading_materials )
	    {
	        mtl = (atoi ( tokens[i] ) - 1) % MAX_MATERIALS;
	        props.materials[props.mtl_cnt++] = mtl;
	    }
	    else if ( mat )
	    {
		props.color_props[props.cur_idx++] = 
		    (GLfloat) atof( tokens[i] );
	    }
	    else
	        popup_dialog( WARNING_POPUP, "Bad Mtl Mgr numeric token parse." );
	}
	else if ( strcmp( tokens[i], "continue" ) == 0 )
	    /* Kick out before incrementing i again. */
	    break;
	else if ( mat )
	{
	    /* Must be the start of a color property specification. */
	    reading_materials = FALSE;
	    mqty = ( analy->num_materials < MAX_MATERIALS )
	           ? analy->num_materials : MAX_MATERIALS;
	    if ( strcmp( tokens[i], "amb" ) == 0 )
	    {
		if ( preview )
		    copy_property( AMBIENT, &v_win->matl_ambient[0][0], 
		                   &save[AMBIENT], mqty );
		update_previous_prop( AMBIENT, reading_cprop, &props );
		reading_cprop[AMBIENT] = TRUE;
	    }
	    else if ( strcmp( tokens[i], "diff" ) == 0 )
	    {
		if ( preview )
		    copy_property( DIFFUSE, &v_win->matl_diffuse[0][0], 
		                   &save[DIFFUSE], mqty );
		update_previous_prop( DIFFUSE, reading_cprop, &props );
		reading_cprop[DIFFUSE] = TRUE;
	    }
	    else if ( strcmp( tokens[i], "spec" ) == 0 )
	    {
		if ( preview )
		    copy_property( SPECULAR, &v_win->matl_specular[0][0], 
		                   &save[SPECULAR], mqty );
		update_previous_prop( SPECULAR, reading_cprop, &props );
		reading_cprop[SPECULAR] = TRUE;
	    }
	    else if ( strcmp( tokens[i], "emis" ) == 0 )
	    {
		if ( preview )
		    copy_property( EMISSIVE, &v_win->matl_emission[0][0], 
		                   &save[EMISSIVE], mqty );
		update_previous_prop( EMISSIVE, reading_cprop, &props );
		reading_cprop[EMISSIVE] = TRUE;
	    }
	    else if ( strcmp( tokens[i], "shine" ) == 0 )
	    {
		if ( preview )
		    copy_property( SHININESS, &v_win->matl_shininess[0], 
		                   &save[SHININESS], mqty );
		update_previous_prop( SHININESS, reading_cprop, &props );
		reading_cprop[SHININESS] = TRUE;
	    }
	}
	else if ( strcmp( tokens[i], "vis" ) == 0 )
	    visible = TRUE;
	else if ( strcmp( tokens[i], "invis" ) == 0 )
	    invisible = TRUE;
	else if ( strcmp( tokens[i], "enable" ) == 0 )
	    enable = TRUE;
	else if ( strcmp( tokens[i], "disable" ) == 0 )
	    disable = TRUE;
	else if ( strcmp( tokens[i], "mat" ) == 0 )
	    mat = TRUE;
	else if ( strcmp( tokens[i], "default" ) == 0 )
	    apply_defaults = TRUE;
	else if ( strcmp( tokens[i], "all" ) == 0 )
	{
	    if ( invisible || visible || enable || disable )
	        modify_all = TRUE;
	}
	else
	    popup_dialog( WARNING_POPUP, "Bad Mtl Mgr parse." );
    }
    
    if ( i >= cnt )
    {
	/* 
	 * Didn't encounter "continue", so clean-up and reset
	 * static variables for next new command.
	 */
	if ( mat )
	{
	    update_previous_prop( MTL_PROP_QTY, reading_cprop, &props );
	    mat = FALSE;
	}
	else if ( apply_defaults )
	{
	    define_materials( props.materials, props.mtl_cnt );
	    apply_defaults = FALSE;
	}
	else
	{
	    if ( visible || invisible )
	    {
	        if ( modify_all )
		{
		    for ( k = 0; k < analy->num_materials; k++ )
		        analy->hide_material[k] = invisible;
		    modify_all = FALSE;
		}
		else
	            for ( k = 0; k < props.mtl_cnt; k++ )
		        analy->hide_material[props.materials[k]] = invisible;

                reset_face_visibility( analy );
                if ( analy->dimension == 3 ) *p_renorm = TRUE;
                analy->result_mod = TRUE;
                visible = FALSE;
                invisible = FALSE;
	    }
	    
	    if ( enable || disable )
	    {
	        if ( modify_all )
		{
		    for ( k = 0; k < analy->num_materials; k++ )
		        analy->disable_material[k] = disable;
		    modify_all = FALSE;
		}
		else
	            for ( k = 0; k < props.mtl_cnt; k++ )
		        analy->disable_material[props.materials[k]] = disable;

                analy->result_mod = TRUE;
                load_result( analy, TRUE );
                enable = FALSE;
                disable = FALSE;
	    }
	}
	reading_materials = FALSE;
	props.mtl_cnt = 0;
    }
    
    return i;
}

 
/*****************************************************************
 * TAG( update_previous_prop )
 * 
 * Check for a property specified for update and do so if found.
 */
static void
update_previous_prop( cur_property, update, props )
Material_property_type cur_property;
Bool_type update[MTL_PROP_QTY];
Material_property_obj *props;
{
    Material_property_type j;
    int mtl, k;
    GLfloat (*p_dest)[4];
    
    for ( j = AMBIENT; j < MTL_PROP_QTY; j++ )
        if ( j != cur_property && update[j] )
            break;
    if ( j == MTL_PROP_QTY )
        return;

    /* Get pointer to appropriate array for update. */
    if ( j == AMBIENT )
        p_dest = v_win->matl_ambient;
    else if ( j == DIFFUSE )
        p_dest = v_win->matl_diffuse;
    else if ( j == SPECULAR )
        p_dest = v_win->matl_specular;
    else if ( j == EMISSIVE )
        p_dest = v_win->matl_emission;

    /* Update material property array. */
    for ( k = 0; k < props->mtl_cnt; k++ )
    {
        mtl = props->materials[k];
	
	if ( j == SHININESS )
	    v_win->matl_shininess[mtl] = props->color_props[0];
	else
	{
            p_dest[mtl][0] = props->color_props[0];
            p_dest[mtl][1] = props->color_props[1];
            p_dest[mtl][2] = props->color_props[2];
            p_dest[mtl][3] = 1.0;
	}
	
	if ( mtl == v_win->current_material )
	    update_current_material( mtl, j );
    }
	    
    /* Clean-up for another specification. */
    update[j] = FALSE;
    props->cur_idx = 0;
    props->color_props[0] = 0.0;
    props->color_props[1] = 0.0;
    props->color_props[2] = 0.0;
}

 
/*****************************************************************
 * TAG( copy_property )
 * 
 * Copy a material property array.
 */
static void
copy_property( property, source, dest, mqty )
Material_property_type property;
GLfloat *source;
GLfloat **dest;
int mqty;
{
    GLfloat *p_dest, *p_src, *bound;
    int qty_vals;
    
    if ( property == SHININESS )
        qty_vals = mqty;
    else
        qty_vals = mqty * 4;
    
    *dest = NEW_N( GLfloat, qty_vals, "Mtl prop copy" );
    p_dest = *dest;
    bound = source + qty_vals;
    for ( p_src = source; p_src < bound; *p_dest++ = *p_src++ );
}

 
/*****************************************************************
 * TAG( parse_vcent )
 * 
 * Parse "vcent" (view centering) command.
 */
static void
parse_vcent( analy, tokens, token_cnt, p_redraw )
Analysis *analy;
char tokens[MAXTOKENS][TOKENLENGTH];
int token_cnt;
Bool_type *p_redraw;
{
    int ival;

    if ( strcmp( tokens[1], "off" ) == 0 )
    {
	analy->center_view = NO_CENTER;
	*p_redraw = TRUE;
    }
    else if ( strcmp( tokens[1], "hi" ) == 0 )
    {
        analy->center_view = HILITE;
        center_view( analy );
	*p_redraw = TRUE;
    }
    else if ( strcmp( tokens[1], "n" ) == 0 
              || strcmp( tokens[1], "node" ) == 0 )
    {
        ival = atoi( tokens[2] );
        if ( ival < 1 || ival > analy->state_p->nodes->cnt )
    	    wrt_text( "Invalid node specified for view center.\n" );
        else
        {
            analy->center_view = NODE;
    	    analy->center_node = ival - 1;
    	    center_view( analy );
	    *p_redraw = TRUE;
        }
    }
    else if ( token_cnt == 4 )
    {
        analy->view_center[0] = atof( tokens[1] );
        analy->view_center[1] = atof( tokens[2] );
        analy->view_center[2] = atof( tokens[3] );
        analy->center_view = POINT;
        center_view( analy );
	*p_redraw = TRUE;
    }
    else
        popup_dialog( USAGE_POPUP, "vcent off\n%s\n%s\n%s",
		                   "vcent hi",
		                   "vcent n|node <node_number>",
    	                           "vcent <x> <y> <z>" );
}

 
/*****************************************************************
 * TAG( check_visualizing )
 * 
 * Test whether cutplanes, isosurfaces, or result vectors are
 * being displayed.  If all of these options are off, reset the
 * display to show the mesh surface.  Otherwise, set up to display
 * just the mesh edges.
 */
static void
check_visualizing( analy )
Analysis *analy;
{
    /*
     * This logic needs to save the render_mode when transitioning
     * to a "show_cut", "show_isosurfs",... mode so that the
     * correct render_mode can be reinstated when transitioning
     * back.  Someday...
     */
    if ( analy->show_cut || analy->show_isosurfs ||
         analy->show_vectors || analy->show_carpet )
    {
        analy->show_edges = TRUE;
        analy->render_mode = RENDER_NONE;
    }
    else
    {
        analy->show_edges = FALSE;
        analy->render_mode = RENDER_FILLED;
        update_util_panel( VIEW_SOLID );
    }
    
    update_util_panel( VIEW_EDGES );
} 

 
/*****************************************************************
 * TAG( update_vis )
 * 
 * Update the fine cutting plane, contours, and isosurfaces.
 * This routine is called when the state changes and when
 * the displayed result changes.
 */
void
update_vis( analy )
Analysis *analy;
{
    set_alt_cursor( CURSOR_WATCH );
    
    if ( analy->geom_p->bricks != NULL )
    {
        gen_cuts( analy );
        gen_isosurfs( analy );
    }
    update_vec_points( analy );
    gen_contours( analy );
    
    unset_alt_cursor();
}


/*****************************************************************
 * TAG( create_alias )
 *
 * Create a command alias.
 */
static void
create_alias( token_cnt, tokens )
int token_cnt;
char tokens[MAXTOKENS][TOKENLENGTH];
{
    Alias_obj *alias;
    int i;

    /* See if this alias is already being used. */
    for ( alias = alias_list; alias != NULL; alias = alias->next )
    {
        if ( strcmp( tokens[1], alias->alias_str ) == 0 )
        {
            for ( i = 2; i < token_cnt; i++ )
                strcpy( alias->tokens[i-2], tokens[i] );
            alias->token_cnt = token_cnt - 2;
            return;
        }
    }

    alias = NEW( Alias_obj, "Command alias" );

    strcpy( alias->alias_str, tokens[1] );
    for ( i = 2; i < token_cnt; i++ )
        strcpy( alias->tokens[i-2], tokens[i] );
    alias->token_cnt = token_cnt - 2;

    INSERT( alias, alias_list );
}


/*****************************************************************
 * TAG( alias_substitute )
 * 
 * Find and replace aliases in a command line.
 */
static void
alias_substitute( tokens, token_cnt, analy )
char tokens[MAXTOKENS][TOKENLENGTH];
int *token_cnt;
Analysis *analy;
{
    Alias_obj *alias;
    int i, j;

    /* If the command is an "alias" command, don't do any alias
     * substitutions!
     */
    if ( strcmp( tokens[0], "alias" ) == 0 )
        return;

    for ( i = 0; i < *token_cnt; i++ )
    {
        alias = alias_list;
        while ( alias != NULL )
        {
            if ( strcmp( tokens[i], alias->alias_str ) == 0 )
            {
                if ( alias->token_cnt == 1 )
                    strcpy( tokens[i], alias->tokens[0] );
                else
                {
                    for ( j = *token_cnt - 1; j >= i; j-- )
                        strcpy( tokens[*token_cnt+j-i], tokens[j] );
                    for ( j = i; j < i + alias->token_cnt; j++ )
                        strcpy( tokens[j], alias->tokens[j-i] );
                    *token_cnt += alias->token_cnt - 1;
                }

                break;
            }

            alias = alias->next;
        }
    }
}


/*****************************************************************
 * TAG( create_reflect_mats )
 *
 * Create transform matrices for the specified reflection plane.
 * The transforms are for reflecting a point and reflecting a
 * normal vector.
 */
void
create_reflect_mats( plane )
Refl_plane_obj *plane;
{
    Transf_mat rot_transf, invrot_transf;
    Transf_mat *transf;
    float vec[3], vecy[3], vecz[3];
    int i;

    /*
     * The reflection plane is specified by a point on the plane and
     * the normal to the plane.
     *
     * For transforming a point, we do the following:
     *
     *     get local set of axes, with the plane normal as the X axis
     *         and the Y and Z axes lying in the reflection plane
     *     translate plane pt to origin
     *     rotate local axes to global axes
     *     scale in X by -1
     *     rotate back
     *     translate back
     *
     *     (Remember that poly node ordering is reversed by reflection.)
     *
     * For transforming a normal, the transform is:
     *
     *     rotate local axes to global axes
     *     scale in X by -1
     *     rotate back
     */

    vec_norm( plane->norm );

    /* Get the local axes. */
    mat_copy( &rot_transf, &ident_matrix );
    mat_rotate_axis_vec( &rot_transf, 0, plane->norm, TRUE );
    VEC_SET( vec, 0.0, 1.0, 0.0 );
    point_transform( vecy, vec, &rot_transf );
    VEC_SET( vec, 0.0, 0.0, 1.0 );
    point_transform( vecz, vec, &rot_transf );

    /* Get rotation transforms for local-to-global and global-to-local. */
    mat_copy( &rot_transf, &ident_matrix );
    mat_copy( &invrot_transf, &ident_matrix );

    for ( i = 0; i < 3; i++ )
    {
        rot_transf.mat[i][0] = plane->norm[i];
        rot_transf.mat[i][1] = vecy[i];
        rot_transf.mat[i][2] = vecz[i];
        invrot_transf.mat[0][i] = plane->norm[i];
        invrot_transf.mat[1][i] = vecy[i];
        invrot_transf.mat[2][i] = vecz[i];
    }

    /* Get the point transformation matrix. */
    transf = &plane->pt_transf;
    mat_copy( transf, &ident_matrix );
    mat_translate( transf, -plane->pt[0], -plane->pt[1], -plane->pt[2] );
    mat_mul( transf, transf, &rot_transf );
    mat_scale( transf, -1.0, 1.0, 1.0 );
    mat_mul( transf, transf, &invrot_transf );
    mat_translate( transf, plane->pt[0], plane->pt[1], plane->pt[2] );

    /* Get the normal transformation matrix. */
    transf = &plane->norm_transf;
    mat_copy( transf, &ident_matrix );
    mat_mul( transf, transf, &rot_transf );
    mat_scale( transf, -1.0, 1.0, 1.0 );
    mat_mul( transf, transf, &invrot_transf );
}


/*****************************************************************
 * TAG( is_numeric_token )
 * 
 * Evaluate a string and return TRUE if string is an ASCII
 * representation of an integer or floating point value
 * (does NOT recognize scientific notation).
 */
static Bool_type
is_numeric_token( num_string )
char *num_string;
{
    char *p_c;
    int pt_cnt;

    pt_cnt = 0;
    
    /*
     * Loop through the characters in the string, returning
     * FALSE if any character is not a digit or is not the first
     * decimal point (period) in the string or if the string
     * is empty.
     */
    for ( p_c = num_string; *p_c != '\0'; p_c++ )
    {
	if ( *p_c > '9' )
	    return FALSE;
	else if ( *p_c < '0' )
	    if ( *p_c != '.' )
	        return FALSE;
	    else
	    {
		pt_cnt++;
		if ( pt_cnt > 1 )
		    return FALSE;
	    }
    }
    
    return (p_c == num_string) ? FALSE : TRUE;
}


/*****************************************************************
 * TAG( get_prake_data )
 * 
 * Assign prake data items from input tokens
 *
 */
static int
get_prake_data ( token_count, prake_tokens, ptr_ival, pt, vec, rgb )
int   token_count;
char  prake_tokens[][TOKENLENGTH];
int  *ptr_ival;
float pt[];
float vec[];
float rgb[];
{
    int count, i;

    /* */

    if ( ((count = sscanf( prake_tokens[1], "%d", ptr_ival )) == EOF ) ||
         (1 != count) )
        wrt_text( "get_prake_data:  Invalid data:  n:  %d\n", ptr_ival );

    for ( i = 0; i < 3; i++ )
    {
        if ( ((count = sscanf( prake_tokens[i + 2], "%f", &pt[i] )) == EOF ) ||
              (1 != count) )
            wrt_text( "get_prake_data:  Invalid data:  pt:  %f\n", pt[i] );
    }

    for ( i = 0; i < 3; i++ )
    {
        if ( ((count = sscanf( prake_tokens[i + 5], "%f", &vec[i] )) == EOF ) ||
              (1 != count) )
            wrt_text( "get_prake_data:  Invalid data:  vec:  %f\n", vec[i] );
    }

    if ( token_count >= 11 )
    {   
        for ( i = 0; i < 3; i++ )
        {
            if ( ((count = sscanf( prake_tokens[i+8], "%f", &rgb[i] )) == EOF )
               || (1 != count) )
                wrt_text( "get_prake_data:  Invalid data:  rgb:  %f\n", rgb[i] );
        }
    }
    else
        rgb[0] = -1;

    return ( count );
}


/*****************************************************************
 * TAG( tokenize )
 * 
 * Parse string containing prake input data into individual tokens.
 *
 */
static int
tokenize( ptr_input_string, token_list, maxtoken )
char   *ptr_input_string;
char   token_list[][TOKENLENGTH];
size_t maxtoken;
{
    int qty_tokens;
    char *ptr_current_token, *ptr_NULL;
    static char token_separators[] = "\t\n ,;";

    /* */

    ptr_NULL = "";

    if ( (ptr_input_string == NULL) || !maxtoken )
        return ( 0 );

    ptr_current_token = strtok( ptr_input_string, token_separators );

    for ( qty_tokens = 0; qty_tokens < maxtoken && ptr_current_token != NULL; )
    {
        strcpy( token_list[qty_tokens++], ptr_current_token );

        ptr_current_token = strtok ( NULL, token_separators );
    }

    strcpy( token_list[qty_tokens], ptr_NULL );

    return ( qty_tokens );
}


/*****************************************************************
 * TAG( prake )
 * 
 * Evaluate and process prake data.  Prake data may be entered
 * directly via terminal or from specified file.
 *
 */
static void
prake( token_cnt, tokens, ptr_ival, pt, vec, rgb, analy )
int       token_cnt;
char      tokens[MAXTOKENS][TOKENLENGTH];
int      *ptr_ival;
float     pt[];
float     vec[];
float     rgb[];
Analysis *analy;
{
    FILE *prake_file;
    char  buffer [TOKENLENGTH];
    char prake_tokens [MAXTOKENS][TOKENLENGTH];
    int count
        ,i
        ,prake_status
        ,prake_token_cnt;
    char *comment;
    char *usage1 = "prake n x1 y1 z1 x2 y2 z2 [r g b]";
    char *usage2 = "prake <filename>";

    /* Duplicate token count and tokens subject to local modification. */

    prake_token_cnt = token_cnt;

    for ( i = 0; i < prake_token_cnt; i++ )
        strcpy( prake_tokens[i], tokens[i] );

    if ( prake_token_cnt > 1 )
    {
        if ( 2 == prake_token_cnt )
        {
            /*
             * Obtain prake data from specified input file.
             */

            if ( (prake_file = fopen( prake_tokens[1], "r" )) != NULL )
            {
                while ( (fgets( buffer, sizeof( buffer ), prake_file )) != NULL )
                {
                    /*
                     * NOTE:  Obtain list of prake tokens from input file --
                     *        NOT directly from GRIZ token processing.
                     *
                     *        Omit blank lines or comment lines [denoted
                     *        by "#" in first column of prake data file.
                     *
                     *        Offset tokens for compatability with GRIZ token
                     *        processing in which "tokens[0] = prake".  This
                     *        also requires an additional increment to
                     *        "prake_token_cnt" for consistency with
                     *        GRIZ token processing.
                     */

                    prake_token_cnt = tokenize( buffer, prake_tokens, 
		                                (MAXTOKENS - 1) );

                    if ( (0 != prake_token_cnt) &&
                         (comment = strstr( prake_tokens[0], "#" )) == NULL )
                    { 
                        if ( prake_token_cnt >= 7 )
                        {
                            for ( i = prake_token_cnt; i > -1; i-- )
                                strcpy( prake_tokens[i + 1], prake_tokens[i] );
                            strcpy( prake_tokens[0], "prake" );
                            prake_token_cnt++;
                            if ( (count = get_prake_data( prake_token_cnt, 
			                                  prake_tokens, ptr_ival, 
                                                          pt, vec, rgb )) > 0 )
                                gen_trace_points( *ptr_ival, pt, vec, rgb, 
                                                  analy );
                            else
                                popup_dialog( USAGE_POPUP, 
				              "%s\n%s", usage1, usage2 );
                        }
                        else
                            popup_dialog( USAGE_POPUP, 
				          "%s\n%s", usage1, usage2 );
                    }
                }
                fclose( prake_file );
            }
            else
                wrt_text( "prake:  Unable to open file:  %s\n", tokens[1] );
        }
        else
        {
            /*
             * No prake data file specified.
             *
             * Obtain prake data from GRIZ input tokenizer.
             */

            if ( prake_token_cnt > 7 )
            {
                if ( (count = get_prake_data( prake_token_cnt, prake_tokens, 
                                              ptr_ival, pt, vec, rgb )) > 0 )
                    gen_trace_points( *ptr_ival, pt, vec, rgb, analy );
                else
                    popup_dialog( USAGE_POPUP, "%s\n%s", 
		                  usage1, usage2 );
            }
            else
                popup_dialog( USAGE_POPUP, "%s\n%s", usage1, usage2 );
        }
    }
    else
        popup_dialog( USAGE_POPUP, "%s\n%s", usage1, usage2 );

    return;
}


/*****************************************************************
 * TAG( outvec )
 * 
 * Prepare ASCII dump of vector grid values
 *
 */
static void
outvec( outvec_file_name, analy )

char      outvec_file_name[1][TOKENLENGTH];
Analysis *analy;

{
    FILE *outvec_file;
    Vector_pt_obj *point;
    char *title;
    int qty_of_nodes;
    int i ,j;

    /*
     * Test to determine if vgrid"N" and vector qualifiers have been established.
     */
    if ( (point = analy->vec_pts) == NULL )
    {
        popup_dialog( INFO_POPUP, "outvec: No vector points exist." );
        return;
    }

    if ( (analy->vec_id[0] == VAL_NONE)  &&
         (analy->vec_id[1] == VAL_NONE)  &&
         (analy->vec_id[2] == VAL_NONE))
    {
        wrt_text( "outvec:  No vector result has been set.\n" );
        wrt_text( "         Use  `vec <vx> <vy> <vz>'.\n" );

        return;
    }

    if ( (outvec_file = fopen( outvec_file_name, "w" )) != NULL )
    {
        write_preamble( outvec_file );
        write_outvec_data( outvec_file, analy );
        fclose( outvec_file );
    }
    else
        wrt_text( "outvec:  Unable to open file:  %s.\n", outvec_file_name );

    return;
}


/*****************************************************************
 * TAG( write_preamble )
 * 
 * Write information preamble to output data file.
 */
void
write_preamble( ofile )
FILE *ofile;
{
    char
         *date_label   = "Date: ", 
         *file_creator = "Output file created by: ", 
	 *infile_label = "Input file: "; 

    fprintf( ofile, "#\n# %s%s\n# %s%s\n#\n# %s%s\n#\n",
             date_label, env.date, 
	     file_creator, env.user_name, 
	     infile_label, env.plotfile_name ); 

    return;
}


/*****************************************************************
 * TAG( write_outvec_data )
 * 
 * Output ASCII file of nodal/vector coordinates and vector values
 */

#define PRECISION 6

static void
write_outvec_data( outvec_file, analy )
FILE     *outvec_file;
Analysis *analy;
{
    Vector_pt_obj *point;
    char
         *comment_label  = "#  "
        ,*quantity_label = "Quantity of samples: "
        ,*space          = "   "
        ,*x_label        = "X"
        ,*y_label        = "Y"
        ,*z_label        = "Z";
    int
        length_comment
       ,length_space
       ,length_vector_x_label
       ,length_vector_y_label
       ,length_vector_z_label
       ,length_x_label
       ,length_y_label
       ,length_z_label
       ,number_width
       ,subscript_0
       ,subscript_1
       ,subscript_2
       ,qty_of_samples
       ,width_column_1
       ,width_column_2
       ,width_column_3
       ,width_column_4
       ,width_column_5
       ,width_column_6;
    int
        qty_of_nodes;
    int
        i;

    number_width = PRECISION + 7;

    length_comment = strlen( comment_label );
    length_space   = strlen( space );
    length_x_label = strlen ( x_label );
    width_column_1 = length_space +
                     ( ( number_width > length_x_label ) ?
                         number_width : length_x_label );
    length_y_label = strlen ( y_label );
    width_column_2 = length_space +
                     ( ( number_width > length_y_label ) ?
                         number_width : length_y_label );

    if ( 3 == analy->dimension )
    {
        length_z_label = strlen ( z_label );
        width_column_3 = length_space +
                         ( ( number_width > length_z_label ) ?
                             number_width : length_z_label );
			     
        subscript_0 = resultid_to_index[ analy->vec_id[0] ];
        length_vector_x_label = strlen ( trans_result[subscript_0][1] );
        width_column_4 = length_space +
                         ( ( number_width > length_vector_x_label ) ?
                             number_width : length_vector_x_label );
			     
        subscript_1 = resultid_to_index[ analy->vec_id[1] ];
        length_vector_y_label = strlen ( trans_result[subscript_1][1] );
        width_column_5 = length_space +
                         ( ( number_width > length_vector_y_label ) ?
                             number_width : length_vector_y_label );
			     
        subscript_2 = resultid_to_index[ analy->vec_id[2] ];
        length_vector_z_label = strlen ( trans_result[subscript_2][1] );
        width_column_6 = length_space +
                         ( ( number_width > length_vector_z_label ) ?
                             number_width : length_vector_z_label );

        if ( analy->vectors_at_nodes )
        {
            /* switch nodvec is set; Complete nodal data set:  3D */
	    
            qty_of_nodes = analy->state_p->nodes->cnt;

            /* Print column headings */
            fprintf (
                     outvec_file
                    ,"#\n#\n# %s%d\n#\n#%s%*s%s%*s%s%*s%s%*s%s%*s%s%*s\n#\n"
                    ,quantity_label
                    ,qty_of_nodes
                    ,comment_label
                    ,width_column_1 - length_comment
                    ,x_label
                    ,space
                    ,width_column_2 - length_space
                    ,y_label
                    ,space
                    ,width_column_3 - length_space
                    ,z_label
                    ,space
                    ,width_column_4 - length_space
                    ,trans_result[ subscript_0 ][1]
                    ,space
                    ,width_column_5 - length_space
                    ,trans_result[ subscript_1 ][1]
                    ,space
                    ,width_column_6 - length_space
                    ,trans_result[ subscript_2 ][1]
                    );

            /* Print data */
            for (i = 0; i < 3; i++ )
                get_result( analy->vec_id[i], analy->cur_state, 
		            analy->tmp_result[i] );

            for ( i = 0; i < qty_of_nodes; i++ )
                fprintf (
                         outvec_file
                        ,"% *.*e% *.*e% *.*e% *.*e % *.*e% *.*e\n"
                        ,width_column_1
                        ,PRECISION
                        ,analy->state_p->nodes->xyz[0][i]
                        ,width_column_2
                        ,PRECISION
                        ,analy->state_p->nodes->xyz[1][i]
                        ,width_column_3
                        ,PRECISION
                        ,analy->state_p->nodes->xyz[2][i]
                        ,width_column_4
                        ,PRECISION
                        ,analy->tmp_result[0][i]
                        ,width_column_5
                        ,PRECISION
                        ,analy->tmp_result[1][i]
                        ,width_column_6
                        ,PRECISION
                        ,analy->tmp_result[2][i]
                        );
        }
        else
        {
            /* switch grdvec is set; Specified nodal data subset:  3D */

            qty_of_nodes = 0;

            for ( point = analy->vec_pts; point != NULL; point = point->next )
                qty_of_nodes++;

            /*
             * NOTE:  "update_vec_points" will only update vector components
             *        if the "show_vectors" flag is set == TRUE
             */

            if ( analy->show_vectors )
                update_vec_points( analy );
            else
            {
                analy->show_vectors = TRUE;
                update_vec_points( analy );
                analy->show_vectors = FALSE;
            }

            /* Print column headings */
            fprintf (
                     outvec_file
                    ,"#\n#\n# %s%d\n#\n%s%*s%s%*s%s%*s%s%*s%s%*s%s%*s\n#\n"
                    ,quantity_label
                    ,qty_of_nodes
                    ,comment_label
                    ,width_column_1 - length_comment
                    ,x_label
                    ,space
                    ,width_column_2 - length_space
                    ,y_label
                    ,space
                    ,width_column_3 - length_space
                    ,z_label
                    ,space
                    ,width_column_4 - length_space
                    ,trans_result[ subscript_0 ][1]
                    ,space
                    ,width_column_5 - length_space
                    ,trans_result[ subscript_1 ][1]
                    ,space
                    ,width_column_6 - length_space
                    ,trans_result[ subscript_2 ][1]
                    );

            /* Print data */
            for ( point = analy->vec_pts; point != NULL; point = point->next )
                fprintf(
                        outvec_file
                       ,"% *.*e% *.*e% *.*e% *.*e % *.*e% *.*e\n"
                       ,width_column_1
                       ,PRECISION
                       ,point->pt[0]
                       ,width_column_2
                       ,PRECISION
                       ,point->pt[1]
                       ,width_column_3
                       ,PRECISION
                       ,point->pt[2]
                       ,width_column_4
                       ,PRECISION
                       ,point->vec[0]
                       ,width_column_5
                       ,PRECISION
                       ,point->vec[1]
                       ,width_column_6
                       ,PRECISION
                       ,point->vec[2]
                       );
        }
    }

    if ( 2 == analy->dimension )
    {
        subscript_0 = resultid_to_index[ analy->vec_id[0] ];
        length_vector_x_label = strlen ( trans_result[subscript_0][1] );
        width_column_3 = length_space +
                         ( ( number_width > length_vector_x_label ) ?
                             number_width : length_vector_x_label );

        subscript_1 = resultid_to_index[ analy->vec_id[1] ];
        length_vector_y_label = strlen ( trans_result[subscript_1][1] );
        width_column_4 = length_space +
                         ( ( number_width > length_vector_y_label ) ?
                             number_width : length_vector_y_label );

        if ( analy->vectors_at_nodes )
        {
            /* switch nodvec is set; Complete nodal data set:  2D */

            qty_of_nodes = analy->state_p->nodes->cnt;

            /* Print column headings */
            fprintf (
                     outvec_file
                    ,"#\n#\n# %s%d\n#\n%s%*s%s%*s%s%*s%s%*s\n#\n"
                    ,quantity_label
                    ,qty_of_nodes
                    ,comment_label
                    ,width_column_1 - length_comment
                    ,x_label
                    ,space
                    ,width_column_2 - length_space
                    ,y_label
                    ,space
                    ,width_column_3 - length_space
                    ,trans_result[ subscript_0 ][1]
                    ,space
                    ,width_column_4 - length_space
                    ,trans_result[ subscript_1 ][1]
                    );

            /* Print data */
            for (i = 0; i < 2; i++ )
                get_result( analy->vec_id[i], analy->cur_state, 
                            analy->tmp_result[i] );

            for ( i = 0; i < qty_of_nodes; i++ )
                fprintf (
                         outvec_file
                        ,"% *.*e% *.*e% *.*e% *.*e\n"
                        ,width_column_1
                        ,PRECISION
                        ,analy->state_p->nodes->xyz[0][i]
                        ,width_column_2
                        ,PRECISION
                        ,analy->state_p->nodes->xyz[1][i]
                        ,width_column_3
                        ,PRECISION
                        ,analy->tmp_result[0][i]
                        ,width_column_4
                        ,PRECISION
                        ,analy->tmp_result[1][i]
                        );
        }
        else
        {
            /* switch grdvec is set; Specified nodal data subset:  2D */

            qty_of_nodes = 0;

            for ( point = analy->vec_pts; point != NULL; point = point->next )
                qty_of_nodes++;

            /*
             * NOTE:  "update_vec_points" will only update vector components
             *        if the "show_vectors" flag is set == TRUE
             */

            if ( analy->show_vectors )
                update_vec_points( analy );
            else
            {
                analy->show_vectors = TRUE;
                update_vec_points( analy );
                analy->show_vectors = FALSE;
            }

            /* Print column headings */
            fprintf (
                     outvec_file
                    ,"#\n#\n# %s%d\n#\n%s%*s%s%*s%s%*s%s%*s\n#\n"
                    ,quantity_label
                    ,qty_of_nodes
                    ,comment_label
                    ,width_column_1 - length_comment
                    ,x_label
                    ,space
                    ,width_column_2 - length_space
                    ,y_label
                    ,space
                    ,width_column_3 - length_space
                    ,trans_result[ subscript_0 ][1]
                    ,space
                    ,width_column_4 - length_space
                    ,trans_result[ subscript_1 ][1]
                    );

            update_vec_points( analy );

            for ( point = analy->vec_pts; point != NULL; point = point->next )
                fprintf(
                        outvec_file
                       ,"% *.*e% *.*e% *.*e% *.*e\n"
                       ,width_column_1
                       ,PRECISION
                       ,point->pt[0]
                       ,width_column_2
                       ,PRECISION
                       ,point->pt[1]
                       ,width_column_3
                       ,PRECISION
                       ,point->vec[0]
                       ,width_column_4
                       ,PRECISION
                       ,point->vec[1]
                       );
        }

        fclose( outvec_file );
    }
    else
    {
        wrt_text( "write_outvec_data:\n" );
        wrt_text( "Unable to open file:  %s.\n", outvec_file );
    }

    return;
}


/*****************************************************************
 * TAG( start_text_history )
 * 
 * Prepare ASCII file of text history
 */
static void
start_text_history( text_history_file_name )
char text_history_file_name[];
{
    if ( (text_history_file = fopen( text_history_file_name, "w" )) != NULL )
        text_history_invoked = TRUE;
    else
    {
        wrt_text( "text_history:  Unable to open file:  %s.\n", 
                  text_history_file_name );
        text_history_invoked = FALSE;
    }

    return;
}


/*****************************************************************
 * TAG( end_text_history )
 * 
 * Prepare ASCII file of text history
 */
static void
end_text_history()
{
    fclose( text_history_file );
    text_history_invoked = FALSE;

    return;
}


/************************************************************
 * TAG( tell_coordinates )
 *
 * Display nodal coordinates of:  a.) specified node id, or
 *                                b.) nodal components of 
 *                                    specified element type id
 */
static void
tell_coordinates( mode, id, analy )
char mode[];
int id;
Analysis *analy;
{
    Nodal *geom_nodes;
    float x_coord, y_coord, z_coord;
    Bool_type have_activity;
    
    have_activity = analy->state_p->activity_present;
    
    if ( strcmp( mode, "node" ) == 0 || strcmp( mode, "n" ) == 0 )
    {
        if ( analy->state_p->nodes->cnt > 0 )
        {
            geom_nodes = analy->state_p->nodes;

            if ( id > 0 && id <= geom_nodes->cnt )
            {
                x_coord = geom_nodes->xyz[0][id - 1];
                y_coord = geom_nodes->xyz[1][id - 1];
                z_coord = geom_nodes->xyz[2][id - 1];

                if ( 2 == analy->dimension )
                    wrt_text( "%s %d  x: % 9.6e  y: % 9.6e\n", 
                              el_label[NODE_T], id, x_coord, y_coord );
                else
                    wrt_text( "%s %d  x: % 9.6e  y: % 9.6e  z: % 9.6e\n", 
                              el_label[NODE_T], id, x_coord, y_coord, z_coord );
            }
            else
                 wrt_text( "Invalid node number: %d.\n", id );
        }
        else
            wrt_text( "No nodal present.\n" );
    }
    else if ( strcmp( mode, "beam" ) == 0 
              || strcmp( mode, "b" )  == 0 )
    {
        if ( analy->geom_p->beams == NULL )
	    wrt_text( "\nNo beams present.\n" );
	else
            tell_element_coords( id, 
	                         el_label[BEAM_T], 
			         2, 
			         analy->geom_p->beams->cnt, 
	                         analy->geom_p->beams->nodes, 
	                         ( have_activity 
			           ? analy->state_p->beams->activity : NULL ), 
	                         analy->state_p->nodes, 
			         analy->dimension );
    }
    else if ( strcmp( mode, "hex" ) == 0 
              || strcmp( mode, "brick" ) == 0 
	      || strcmp( mode, "h" ) == 0 )
    {
        if ( analy->geom_p->bricks == NULL )
	    wrt_text( "\nNo bricks present.\n" );
	else
            tell_element_coords( id, 
	                         el_label[BRICK_T], 
			         8, 
	                         analy->geom_p->bricks->cnt, 
	                         analy->geom_p->bricks->nodes, 
	                         ( have_activity 
	                           ? analy->state_p->bricks->activity : NULL ), 
	                         analy->state_p->nodes, 
	                         analy->dimension );
    }
    else if ( strcmp( mode, "shell" ) == 0 
              || strcmp( mode, "s" ) == 0 )
    {
        if ( analy->geom_p->shells == NULL )
	    wrt_text( "\nNo shells present.\n" );
	else
            tell_element_coords( id, 
	                         el_label[SHELL_T], 
			         4, 
	                         analy->geom_p->shells->cnt, 
	                         analy->geom_p->shells->nodes, 
	                         ( have_activity 
	                           ? analy->state_p->shells->activity : NULL ), 
	                         analy->state_p->nodes, 
	                         analy->dimension );
    }

    return;
}


/*****************************************************************
 * TAG( tell_element_coords )
 * 
 * Write the node idents and their coordinates for the nodes
 * referenced by an element.
 */
static void
tell_element_coords( el_ident, obj_label, node_qty, el_qty, 
                     connects, activity, nodes, dimensions )
int el_ident;
char *obj_label;
int node_qty;
int el_qty;
int *connects[];
float *activity;
Nodal *nodes;
int dimensions;
{
    int i;
    int node_number, node_id;
    float x_coord, y_coord, z_coord;
    char *blanks = "                ";
    char el_buf[16];
    int width, el_buf_width;
    
    sprintf( el_buf, "%s %d", obj_label, el_ident );
	
    if ( el_ident >  0 && ( el_ident <= el_qty ) )
    {
        /* Set up to align output cleanly. */
        width = max_id_string_width( connects, node_qty, el_ident );
        el_buf_width = strlen( el_buf );

        /* For each node referenced by element... */
        for ( i = 0; i < node_qty; i++ )
        {
	    /* Get node ident and its coordinates. */
            node_number = connects[i][el_ident - 1];
            node_id = node_number + 1;

            x_coord = nodes->xyz[0][node_number];
            y_coord = nodes->xyz[1][node_number];
            z_coord = nodes->xyz[2][node_number];

            /* Write to feedback window. */
            if ( 2 == dimensions )
            {
                if ( 0 == i )
                {
                    if ( activity && ( 0.0 == activity[el_ident - 1] ) )
                        wrt_text( "\n\n%s is inactive.\n", el_buf );

                    wrt_text( "%s  node %*d  x: % 9.6e  y: % 9.6e\n", 
                              el_buf, width, node_id, x_coord, y_coord );
                }
                else
                    wrt_text( "%.*s  node %*d  x: % 9.6e  y: % 9.6e\n", 
                              el_buf_width, blanks, width, node_id, 
			      x_coord, y_coord );
            }
            else
            {
                if ( 0 == i )
                {
                    if ( activity && ( 0.0 == activity[el_ident - 1] ) )
                        wrt_text( "\n\n%s is inactive.\n", el_buf );

                    wrt_text( "%s  node %*d  x: % 9.6e  y: % 9.6e  z: % 9.6e\n", 
                              el_buf, width, node_id, 
			      x_coord, y_coord, z_coord );
                }
                else
                    wrt_text( "%.*s  node %*d  x: % 9.6e  y: % 9.6e  z: % 9.6e\n", 
                              el_buf_width, blanks, width, node_id, 
			      x_coord, y_coord, z_coord );
            }
        }
    }
    else
        wrt_text( "Invalid element: %s\n", el_buf );

    return;
}


/*****************************************************************
 * TAG( max_id_string_width )
 * 
 * Get the maximum width among string representations of node
 * identifiers referenced by an element.
 */
static int
max_id_string_width( connects, node_cnt, ident )
int *connects[];
int node_cnt;
int ident;
{
    int i, width, temp;
    
    width = 0;
    for ( i = 0; i < node_cnt; i++ )
    {
        temp = (int) (log10( (double) connects[i][ident - 1] ) + 1.0);
	
        if ( temp > width ) 
            width = temp;
    }
    
    return width;
}

