/* $Id$ */
/* 
 * interpret.c - Command line interpreter for graphics viewer.
 * 
 *  Donald J. Dovey
 *  Lawrence Livermore National Laboratory
 *  Jan 2 1992
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
 * of the Federal Government are reserved under Contract 48 subject t`o 
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
 ************************************************************************
 *
 * Modifications:
 *  I. R. Corey - Sept 15, 2004: Added new option "damage_hide" which 
 *   controls if damaged elements are displayed.
 *
 *  I. R. Corey - Nov 15, 2004: Added new option "extreme_min" and
 *   "extreme_max" to display element min and maxes over all time
 *   steps. See SRC# 292.
 *
 *  I. R. Corey - Dec  28, 2004: Added new function draw_free_nodes. This
 *                function is invoked using the on free_nodes command.
 *                See SRC#: 286
 *
 *  I. R. Corey - Jan 24, 2005: Added a new free_node option to disable
 *                mass scaling of the nodes.
 *                
 *  I. R. Corey - Feb 10, 2005: Added a new free_node option to enable
 *                volume disable scaling of the nodes.
 *
 *  I. R. Corey - Mar 29, 2005: Added range selection option to scale
 *                command like we now have for materials - 
 *                    example: select brick 100-110
 *                See SCR#: 308
 *
 *  I. R. Corey - Apr 05, 2005: Added function to check for valid
 *                result. Called before performing various operations
 *                such as plotting (timehistory). 
 *                See SCR#: 310
 *
 *  I. R. Corey - Sept 08, 2005: Alias outjpeg with outjpg.
 *                See SCR#: 348
 *
 *  I. R. Corey - Feb 02, 2006: Add a new capability to display meshless, 
 *                particle-based results.
 *                See SRC# 367.
 *
 *  I. R. Corey - Mar 30, 2006: Add the capability to put multiple commands
 *                on a single line. Fucntion parse_command will be replaced
 *                by parse_single_command, and parse_command sill now strip
 *                out single commands from the input stream.
 *                See SRC# 381.
 *
 *  I. R. Corey - Oct 26, 2006: Added a new feature for enable/disable 
 *                vis/invis that allows selection of an object type and
 *                material for that object.
 *                See SRC# 421.
 *
 *  I. R. Corey - Nov 09, 2006: Added a new feature for enable/disable 
 *                display updates during resize events.
 *                See SRC# 421.
 *
 *  I. R. Corey - Feb 12, 2007: Added wireframe viewing capability and added
 *                logic to fix degenerate polygon faces.
 *                See SRC#437.
 *
 *  I. R. Corey - Feb 20, 2007: Added refinement to objects selection for 
 *                bricks, truss, shells, and beams that allows hide/enable
 *                of indivisual elements or a range of elements.
 *                See SRC#437.
 * 
 *  I. R. Corey - March 12, 2007: Added two new zoom commands: zoom forward
 *                (zf) and zoom backward (zb). Each requires a number between
 *                0 and 1 which is multiplied by the scale factor.
 *                See SRC#438.
 *
 *  I. R. Corey - Jul 25, 2007: Added feature to plot disabled materials in
 *                greyscale.
 *                See SRC#476.
 *
 *  I. R. Corey - Aug 09, 2007:	Added date/time string to render window.
 *                See SRC#478.
 *
 *  I. R. Corey - Aug 24, 2007:	Added a new help button to display cur-
 *                rent release notes.
 *                See SRC#483.
 *
 *  I. R. Corey - Oct 05, 2007:	Added new error indicator result option.
 *                See SRC#483.
 *
 *  I. R. Corey - Nov 08, 2007:	Add Node/Element labeling.
 *                See SRC#418 (Mili) and 504 (Griz)
 *
 *  I. R. Corey - Apr 18, 2008:	Allow for longer commands (more tokens).
 *                Change from max of 20 -> 1000.
 *                See SRC#529
 *
 *  I. R. Corey - May 5, 2008: Added support for code usage tracking using
 *                the AX tracker tool.
 *                See SRC#533.
 *************************************************************************
 */

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "viewer.h"
#include "draw.h"
#include "traction_force.h"

#ifdef VIDEO_FRAMER
extern void vid_select( /* int select */ );
extern void vid_cbars();
extern int vid_vlan_init();
extern int vid_vlan_rec( /* int move_frame, int record_n_frames */ );
extern void vid_cbars();
#endif

extern void update_min_max();
extern View_win_obj *v_win;
extern float crease_threshold;
extern float explicit_threshold;
extern FILE *text_history_file;
extern Bool_type text_history_invoked;

static int mat_obj_type;

extern void define_color_properties();
extern void update_current_color_property();

extern Bool_type history_inputCB_cmd;


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
    int *materials;
    int mtl_cnt;
    int cur_idx;
} Material_property_obj;


/*****************************************************************
 * TAG( Surface_property_obj )
 *
 * Holds information parsed from a surface manager request.
 */
typedef struct _Surface_property_obj
{
    int *surfaces;
    int surf_cnt;
    int cur_idx;
} Surface_property_obj;

#ifdef JPEG_SUPPORT
/*****************************************************************
 * TAG( JPEG global variable initialization )
 *
 */
int jpeg_quality = 75;
#endif




static char last_command[200] = "\n";

/* Local routines. */
static void tokenize_line( char *buf, char tokens[MAXTOKENS][TOKENLENGTH], 
                           int *token_cnt );
static void check_visualizing( Analysis *analy );
static void create_alias( int token_cnt, char tokens[MAXTOKENS][TOKENLENGTH] );
static void alias_substitute( char tokens[MAXTOKENS][TOKENLENGTH], 
                              int *token_cnt );
static int parse_embedded_mtl_cmd( Analysis *analy, char tokens[][TOKENLENGTH],
                                   int cnt, Bool_type preview, 
                                   GLfloat *save[MTL_PROP_QTY], 
                                   Bool_type *p_renorm );
static int parse_embedded_surf_cmd( Analysis *analy, char tokens[][TOKENLENGTH],
                                    int cnt, Bool_type *p_renorm );

static void parse_single_command( char *buf, Analysis *analy );

static void parse_vcent( Analysis *analy, char tokens[MAXTOKENS][TOKENLENGTH], 
                         int token_cnt, Redraw_mode_type *p_redraw );
static void parse_mtl_cmd( Analysis *analy, char tokens[MAXTOKENS][TOKENLENGTH],
                           int token_cnt, Bool_type src_mtl_mgr, 
                           Redraw_mode_type *p_redraw, Bool_type *p_renorm );
static void parse_mtl_range( char *token, int qty, int *mat_min, int *mat_max );
static void parse_surf_cmd( Analysis *analy, char tokens[MAXTOKENS][TOKENLENGTH],
                            int token_cnt, Bool_type src_surf_mgr, 
                           Redraw_mode_type *p_redraw, Bool_type *p_renorm );
static void update_previous_prop( Material_property_type cur_property, 
                                  Bool_type update[MTL_PROP_QTY], 
                                  Material_property_obj *props );
static void copy_property( Material_property_type property, GLfloat *source, 
                           GLfloat **dest, int mqty );
static void outvec( char outvec_file_name[TOKENLENGTH], Analysis *analy );
static void write_outvec_data( FILE *outvec_file, Analysis *analy );
static void prake( int token_cnt, char tokens[MAXTOKENS][TOKENLENGTH], 
                   int *ptr_ival, float pt[], float vec[], float rgb[], 
                   Analysis *analy );
static int get_prake_data( int token_count, char prake_tokens[][TOKENLENGTH], 
                           int *ptr_ival, float pt[], float vec[], 
                           float rgb[] );
/**static **/ int tokenize( char *ptr_input_string, char token_list[][TOKENLENGTH], 
                     size_t maxtoken );
static void start_text_history( char text_history_file_name[] );
static void end_text_history( void );

extern void hidden_inline( char * );

static int  check_for_result( Analysis *analy, int display_warning );


void
process_mat_obj_selection ( Analysis *analy, char tokens[MAXTOKENS][TOKENLENGTH],
			    int idx, int token_cnt, int mat_qty, int elem_qty,
			    unsigned char *p_elem, int  *p_hide_qty, int *p_hide_elem_qty, 
                            unsigned char *p_uc,
			    unsigned char setval, 
			    Bool_type vis_selected, Bool_type mat_selected);



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
read_token( FILE *infile, char *token, int max_length )
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
tokenize_line( char *buf, char tokens[MAXTOKENS][TOKENLENGTH], int *token_cnt )
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
                ++chr;
                while ( isspace( buf[chr] ) && buf[chr] != '\0' )
                    ++chr;
                /* 
                 * Don't terminate token if next character is square bracket,
                 * as that would indicate we're parsing a vector or array 
                 * result specification. 
                 */
                if ( buf[chr] != '[' )
                {
                    tokens[word][wchr] = '\0';
                    ++word;
                    wchr = 0;
                }
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
        else if ( buf[chr] == '[' )
        {
            /* Indexing expression - pick it all off here to avoid partial
             * overlap with a quoted expression.
             */
            do
            {
                if ( buf[chr] != quote_char )
                {
                    tokens[word][wchr] = buf[chr];
                    ++wchr;
                }
                ++chr;
            }
            while ( buf[chr] != ']' && buf[chr] != '\0' );
            
            if ( buf[chr] == ']' )
            {
                tokens[word][wchr] = buf[chr];
                ++wchr;
                ++chr;
            }

            if ( buf[chr] == '\0' )
            {
                tokens[word][wchr] = '\0';
                ++word;
            }
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
 * Command parser for viewer.  Parse the commands in buf. This 
 * function will strip out multiple commands from a command
 * line and pass them to parse_single_command.
 */
void
parse_command( char *buf, Analysis *analy )
{
  char command_buf[2000];
  int  i, j;
  int  len_buf;
  int  num_cmds = 1;
  int  next_cmd_index[2000];

  int  first_nonspace=0, start_of_cmd=0;

  len_buf = strlen(buf);

  next_cmd_index[0]=0;

  history_inputCB_cmd = FALSE;
 
  for (i = 0; i < len_buf; i++ )
       if ( buf[i] == ';' )
       {
            buf[i] = '\0';
            next_cmd_index[num_cmds++]=i+1;        
       }

  for (i=0; i < num_cmds ; i++ )
  {   
    if (strlen(&buf[next_cmd_index[i]]) > 0)
        strcpy(command_buf, &buf[next_cmd_index[i]]);

    /* Remove file references that could be at the beginning of a command.
     * These are commands that would have come from a history file and
     * the file reference is placed in the command window.
     * If the user selects the command with the mouse and does a paste,
     * this file reference would also be copied.  
     * For example:    [grizinit - Line:1] on all
     * 
     * Note that the file reference in enclosed in '[ ]'.
     */

    for(j=0; j , strlen( command_buf ); j ++ )
      if (command_buf[j]!=' ')
      {
        first_nonspace = j;
        break;
      }

    if (command_buf[first_nonspace]=='[')
      for(j = 0; j < strlen( command_buf ); j++ )
	if (command_buf[j]==']')
	    start_of_cmd = j+1;

    parse_single_command( &command_buf[start_of_cmd], analy );
  }
}
  


/*****************************************************************
 * TAG( parse_single_command )
 * 
 * Command parser for viewer.  Parse the single command in buf.
 */
void
parse_single_command( char *buf, Analysis *analy )
{
    char tokens[MAXTOKENS][TOKENLENGTH];
    int token_cnt, token_index;
    float val, pt[3], vec[3], rgb[3], pt2[3], pt3[3], pt4[3];
    Bool_type renorm, setval, valid_command, found;
    Redraw_mode_type redraw;
    static Bool_type cutting_plane_defined = FALSE;
    int sz[3], nodes[3];
    int i_args[4];
    int idx, ival, temp_ival, i, j, dim, qty;
    int start_state, stop_state, max_state, min_state;
    char result_variable[1];
    Redraw_mode_type tellmm_redraw;
    Bool_type selection_cleared;
    static Result result_vector[3];
    int rval;
    Htable_entry *p_hte;
    MO_class_data *p_mo_class;
    MO_class_data **p_classes;
    Mesh_data *p_mesh;
    List_head *p_lh;
    Surface_data *p_surface;
    unsigned char *p_uc, *p_uc2;
    unsigned char *p_elem, *p_elem2;
    int           elem_qty;
    int superclass;
    int qty_facets;

    int *p_hide_qty, *p_hide_qty2, *p_disable_qty, p_hide_qty_temp_int=0;
    int *p_hide_elem_qty, *p_hide_elem_qty2, *p_disable_elem_qty;

    int elem_sclasses[] = 
    {
        G_TRUSS,
        G_BEAM,
        G_TRI,
        G_QUAD,
        G_TET,
        G_PYRAMID,
        G_WEDGE,
        G_HEX,
        G_SURFACE
    };
    Specified_obj *p_so, *p_del_so;
    Classed_list *p_cl;
    Util_panel_button_type btn_type;
    int old;
    float (*xfmat)[3];
    char *p_string;
    char *tellmm_usage = "tellmm [<result> [<first state> [<last state>]]]";
    char line_buf[2000];

    FILE *fp_outview_file;
    FILE *select_file;

    char tmp_token[64];

    char *griz_home, acroread_cmd[128];

    /* Material range selection variables
     */
    int mtl, mat_qty, mat_min, mat_max;

    /* Object range selection variables
     */
    int obj, obj_min, obj_max;

    Bool_type mat_selected     = TRUE;
    Bool_type result_selected  = TRUE;
    Bool_type vis_selected     = FALSE;
    Bool_type enable_selected  = FALSE;
    Bool_type include_selected = FALSE;

    int view_state; /* Used for extreme_min/max */

    float *rgb_default, *rgb_ei; 

    tokenize_line( buf, tokens, &token_cnt );

    if ( token_cnt < 1 )
        return;

    /*
     * Clean up any dialogs from the last command that the user
     * hasn't acknowledged.
     */
    clear_popup_dialogs();

    alias_substitute( tokens, &token_cnt );

    /*
     * Should call getnum in slots below to make sure input is valid,
     * and prompt user if it isn't.   (Use instead of sscanf.)
     * Also getint and getstring.
     */

    redraw = NO_VISUAL_CHANGE;
    renorm = FALSE;
    valid_command = TRUE;

    if ( strcmp( tokens[0], "rx" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        inc_mesh_rot( 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, DEG_TO_RAD(val) );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "ry" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        inc_mesh_rot( 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, DEG_TO_RAD(val) );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "rz" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        inc_mesh_rot( 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, DEG_TO_RAD(val) );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "tx" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        inc_mesh_trans( val, 0.0, 0.0 );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "ty" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        inc_mesh_trans( 0.0, val, 0.0 );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "tz" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        inc_mesh_trans( 0.0, 0.0, val );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "scale" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        set_mesh_scale( val, val, val );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "zf" ) == 0 ) /* Zoom forward */
    {
        get_mesh_scale( vec );
        sscanf( tokens[1], "%f", &val );

        vec[0] = vec[0] + (val*vec[0]);
        vec[1] = vec[1] + (val*vec[1]);
        vec[2] = vec[2] + (val*vec[2]);

        set_mesh_scale( vec[0], vec[1], vec[2] );
        redraw = BINDING_MESH_VISUAL;
    }	
    else if ( strcmp( tokens[0], "zb" ) == 0 ) /* Zoom backward */
    {
        get_mesh_scale( vec );
        sscanf( tokens[1], "%f", &val );

	if ( val<0 )
  	    val = val*(-1.);

	vec[0] = vec[0]/val;
        vec[1] = vec[1]/val;
        vec[2] = vec[2]/val;

        set_mesh_scale( vec[0], vec[1], vec[2] );
        redraw = BINDING_MESH_VISUAL;
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
        if ( token_cnt != 4 )
        {
            popup_dialog( USAGE_POPUP, "scalax <x scale> <y scale> <z scale>" );
            valid_command = FALSE;
        }
        else
        {
            sscanf( tokens[1], "%f", &vec[0] );
            sscanf( tokens[2], "%f", &vec[1] );
            sscanf( tokens[3], "%f", &vec[2] );
            set_mesh_scale( vec[0], vec[1], vec[2] );
            redraw = BINDING_MESH_VISUAL;
        }
    }
    else if ( strcmp( tokens[0], "r" ) == 0 )
    {
        /* Repeat the last command. */
        parse_command( last_command, analy );
    }
    else if ( strcmp( tokens[0], "hilite" ) == 0 )
    {
        rval = htable_search( MESH( analy ).class_table, tokens[1], FIND_ENTRY,
                              &p_hte );
        if ( rval == OK  )
             p_mo_class = (MO_class_data *) p_hte->data;

        if ( token_cnt == 3 && is_numeric_token( tokens[2] ) )
        {
            sscanf( tokens[2], "%d", &ival );
	    if ( p_mo_class->labels_found )
		 temp_ival = get_class_label_index( p_mo_class, ival );
	    else
 	         temp_ival = ival;

            if ( ival == analy->hilite_num
                 && p_mo_class == analy->hilite_class )
            {
                /* Hilited existing hilit object -> de-hilite. */
                analy->hilite_class = NULL;
                analy->hilite_num = 0;
            }
            else
            {
                analy->hilite_class = p_mo_class;
		analy->hilite_num   = temp_ival-1;
                analy->hilite_label = ival;
            }
            redraw = BINDING_MESH_VISUAL;
        }
        else
            popup_dialog( USAGE_POPUP, "hilite <class name> <ident>" );
    }
    else if ( strcmp( tokens[0], "clrhil" ) == 0 )
    {
        analy->hilite_class = NULL;
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "select" ) == 0 )
    {
        rval = htable_search( MESH( analy ).class_table, tokens[1], FIND_ENTRY,
                              &p_hte );
        if ( rval == OK && token_cnt > 2 )
        {
            p_mo_class = (MO_class_data *) p_hte->data;
            superclass = p_mo_class->superclass;

            qty = 0;
            for ( i = 2; i < token_cnt; i++ )
            {

                /* IRC: Added March 29, 2005 */
                /* Check for a range in the object list */
                if ( is_numeric_range_token( tokens[i] ) )
                {
	 	   parse_mtl_range(tokens[i], p_mo_class->qty, &obj_min, &obj_max ) ;
                   obj = obj_min ;
                   for (obj =  obj_min ; 
			obj <= obj_max; 
			obj++ )
                   {
                     if( superclass != G_SURFACE )
                     { 
                       if ( obj < 1 || obj > obj_max ||
                            get_class_label_index( p_mo_class, obj ) ==
                                M_INVALID_LABEL )
                       {
                         popup_dialog( INFO_POPUP,
                         "Invalid ident (%d) for class %s (%s); ignored.",
                         obj, p_mo_class->short_name, p_mo_class->long_name );
                         continue;
                       }
                
                        /*
                         * Traverse currently selected objects to see if current
                         * candidate already exists.
                         */

			if ( p_mo_class->labels_found )
			     temp_ival = get_class_label_index( p_mo_class, obj );
			else
			     temp_ival = obj;

                        for ( p_so = analy->selected_objects; p_so != NULL; 
                              NEXT( p_so ) )
                               if ( p_so->mo_class == p_mo_class && p_so->ident == temp_ival )
                                    break;
                        
                       /* If object exists, delete to de-select, else add. */
                       if ( p_so != NULL )
                       {
                            DELETE( p_so, analy->selected_objects );
                       }
                       else
                       {
                         p_so = NEW( Specified_obj, "New object selection" );
                         p_so->ident = temp_ival-1;
			 p_so->label = obj;
			    
                         p_so->mo_class = p_mo_class;
                         APPEND( p_so, analy->selected_objects );
                       }
                        
                        qty++; /* Additions or deletions should cause redraw. */
                     }
                     else
                     {
                       p_surface = p_mo_class->objects.surfaces;
                       qty_facets = p_surface->facet_qty;
                       if ( obj < 1 || obj > qty_facets )
                       {
                         popup_dialog( INFO_POPUP,
                         "Invalid ident (%d) for class %s (%s); ignored.",
                         obj, p_mo_class->short_name, p_mo_class->long_name );
                         continue;
                       }
                                                                                 
		       if ( p_mo_class->labels_found )
			    temp_ival = get_class_label_index( p_mo_class, obj );
		       else
			    temp_ival = obj;

                       /*
                        * Traverse currently selected objects to see
                        * if current candidate already exists.
                        */
                       for ( p_so = analy->selected_objects; p_so != NULL;
                             NEXT( p_so ) )
                         if ( p_so->mo_class == p_mo_class
                              && p_so->ident == temp_ival - 1 )
                              break;
                                                                                 
                       /* If object exists, delete to de-select, else add. */
                       if ( p_so != NULL )
                       {
                         DELETE( p_so, analy->selected_objects );
                       }
                       else
                       {
                         p_so = NEW( Specified_obj, "New object selection" );
                         p_so->ident    = temp_ival - 1;
                         p_so->mo_class = p_mo_class;
			 p_so->label = obj;
			 APPEND( p_so, analy->selected_objects );
                       }
                                                                                 
                        qty++; /*Additions or deletions should cause redraw.*/
                     }
                   }
                }
                else if ( strcmp( tokens[i], "all" ) == 0 )
                {
                   if( superclass != G_SURFACE )
                   {
                       obj_max = p_mo_class->qty;
                       if( p_mo_class->labels )
                           obj_max = p_mo_class->labels_max; 

                        for( j = 1; 
			     j <= obj_max;
			     j++ )
                        {
			    if ( p_mo_class->labels_found )
			         temp_ival = get_class_label_index( p_mo_class, j );
			    else
			         temp_ival = j;

                            /*
                             * Traverse currently selected objects to see if
                             * current candidate already exists.
                             */
                            for( p_so = analy->selected_objects; p_so != NULL;
                                 NEXT( p_so ) )
			    {
                                 if( p_so->mo_class == p_mo_class &&
                                     p_so->ident == temp_ival - 1 )
                                     break;
			    }

                            /* If object exists, delete to de-select, else add. */
                            if ( p_so != NULL )
                            {
                                DELETE( p_so, analy->selected_objects );
                            }
                            else
                            {
                                p_so = NEW( Specified_obj, "New object selection" );
                                p_so->ident = temp_ival-1;
		                p_so->label = j-1;
                                p_so->mo_class = p_mo_class;
                                APPEND( p_so, analy->selected_objects );
                            }

                            /* Additions or deletions should cause redraw. */
                            qty++;
                        }
                    }
                    else
                    {
                        p_surface = p_mo_class->objects.surfaces;
                        qty_facets = p_surface->facet_qty;
                        for( j = 1; j <= qty_facets; j++ )
                        {
                                                                                 
			     if ( p_mo_class->labels_found )
			          temp_ival = get_class_label_index( p_mo_class, j );
			     else
			          temp_ival = j;


                            /*
                             * Traverse currently selected objects to see if
                             * current candidate already exists.
                             */
                            for( p_so = analy->selected_objects; p_so != NULL;
                                 NEXT( p_so ) )
			    {
				 if( p_so->mo_class == p_mo_class && p_so->ident == temp_ival - 1 )
                                     break;
			    }
                                                    
                            /* If object exists, delete to de-select, else add. */
                            if( p_so != NULL )
                            {
                                DELETE( p_so, analy->selected_objects );
                            }
                            else
                            {
                                p_so = NEW( Specified_obj, "New object selection" );
                                p_so->ident    = temp_ival-1;
                                p_so->label    = j-1;
                                p_so->mo_class = p_mo_class;
                                APPEND( p_so, analy->selected_objects );
                            }

                            /* Additions or deletions should cause redraw. */
                            qty++;
                         }
                    }
                }
                else
                {
                  if( superclass != G_SURFACE )
                  {
                    j = atoi( tokens[i] );
                    obj_max = p_mo_class->qty;
                    if ( p_mo_class->labels )
                         obj_max = p_mo_class->labels_max; 

                    if ( j < 1 || j > obj_max ||
		         get_class_label_index( p_mo_class, j ) ==
                             M_INVALID_LABEL ) 
                    {
                      popup_dialog( INFO_POPUP,
                      "Invalid ident (%d) for class %s (%s); ignored.",
                      j, p_mo_class->short_name, p_mo_class->long_name );
                      continue;
                    }
                
		    if ( p_mo_class->labels_found )
		         temp_ival = get_class_label_index( p_mo_class, j );
		    else
		         temp_ival = j;

                    /*
                     * Traverse currently selected objects to see if current
                     * candidate already exists.
                     */
                    for ( p_so = analy->selected_objects; p_so != NULL; 
                          NEXT( p_so ) )
		    {
			  if ( p_so->mo_class == p_mo_class && 
			       (p_so->ident == temp_ival-1) )
			        break;
		    }


                    /* If object exists, delete to de-select, else add. */
                    if ( p_so != NULL )
                    {
                      DELETE( p_so, analy->selected_objects );
                    }
                    else
                    {
                      p_so = NEW( Specified_obj, "New object selection" );
                      p_so->ident = temp_ival-1;
		      p_so->label = j;

                      p_so->mo_class = p_mo_class;
                      APPEND( p_so, analy->selected_objects );
                    }

                    qty++; /* Additions or deletions should cause redraw. */
                }
                else
                {
                  p_surface = p_mo_class->objects.surfaces;
                  qty_facets = p_surface->facet_qty;
                  j = atoi( tokens[i] );
                  if ( j < 1 || j > qty_facets )
                  {
                    popup_dialog( INFO_POPUP,
                        "Invalid ident (%d) for class %s (%s); ignored.",
                         j, p_mo_class->short_name, p_mo_class->long_name );
                    continue;
                  }
                                                                                 
                  /*
                   * Traverse currently selected objects to see if current
                   * candidate already exists.
                   */
                  for ( p_so = analy->selected_objects; p_so != NULL;
                        NEXT( p_so ) )
                    if ( p_so->mo_class == p_mo_class && 
                         (p_so->ident == j-1 || p_so->label == j) )
                         break;
                                                                                 
                  /* If object exists, delete to de-select, else add. */
                  if ( p_so != NULL )
                  {
                    DELETE( p_so, analy->selected_objects );
                  }
                  else
                  {
                    p_so = NEW( Specified_obj, "New object selection" );
                    if ( p_mo_class->labels_found )
		         p_so->ident = get_class_label_index( p_mo_class, j );
		    else
		         p_so->ident = j-1;

		    p_so->label = j;

                    p_so->mo_class = p_mo_class;
                    APPEND( p_so, analy->selected_objects );
                  }
                                                                                 
                  qty++; /* Additions or deletions should cause redraw. */
                }
              }
            }

            if ( i > 2 && qty > 0 )
            {
/**/
                /* clear_gather( analy ); */
                redraw = BINDING_MESH_VISUAL;
            }
        }
        else if( token_cnt == 2 )
        {
            if ( (select_file = fopen( tokens[1], "r" )) == NULL )
            {
                popup_dialog( WARNING_POPUP,
                              "Unable to open file \"%s\".", tokens[1] );
                valid_command = FALSE;
            }
            else
            {
                qty = 0;
                while ( !feof( select_file ) )
                {
                    if( fgets( line_buf, 2000, select_file ) != NULL )
                    {
                    tokenize_line( line_buf, tokens, &token_cnt );
                    rval = htable_search( MESH( analy ).class_table, tokens[0],
                                          FIND_ENTRY,&p_hte );

                    if ( rval == OK && token_cnt > 1)
                    {
                        p_mo_class = (MO_class_data *) p_hte->data;

                        for ( i = 1; i < token_cnt; i++ )
                        {
                            ival = atoi( tokens[i] );
			    if ( p_mo_class->labels_found )
			         temp_ival = get_class_label_index( p_mo_class, ival );
			    else
			         temp_ival = ival;

			    obj_max = p_mo_class->qty;
			    if ( p_mo_class->labels )
			         obj_max = p_mo_class->labels_max; 

                            if ( ival < 1 || ival > obj_max )
                            {
                                popup_dialog( INFO_POPUP,
                               "Invalid ident (%d) for class %s (%s); ignored.",
                                ival, p_mo_class->short_name, p_mo_class->long_name );
                                continue;
                            }

                            /*
                             * Traverse currently selected objects to see if current
                             * candidate already exists.
                             */
                            for ( p_so = analy->selected_objects; p_so != NULL;
                                  NEXT( p_so ) )
			    {
			          if ( p_so->mo_class == p_mo_class && 
				       (p_so->ident == ival - 1) )
                                        break;
			    }


                            /* If object exists, delete to de-select, else add. */
                            if ( p_so != NULL )
                            {
                                DELETE( p_so, analy->selected_objects );
                            }
                            else
                            {
                                p_so = NEW( Specified_obj, "New object selection" );
                                p_so->ident = temp_ival-1;
                                p_so->label = ival;
				p_so->mo_class = p_mo_class;
				APPEND( p_so, analy->selected_objects );
                            }

                            qty++; /* Additions or deletions should cause redraw. */
                        }
                    }
                    }
               }

               if ( qty > 0 )
               {
                   /* clear_gather( analy ); */
                   redraw = BINDING_MESH_VISUAL;
               }

            }

        }
        else
        {
            popup_dialog( USAGE_POPUP, "select <class name> <ident>...\n     or\n select <filename>" );
            valid_command = FALSE;
        }
    }
    else if ( strcmp( tokens[0], "clrsel" ) == 0 )
    {
        selection_cleared = FALSE;
                
        if ( token_cnt == 1 )
        {
            if ( analy->selected_objects != NULL )
                selection_cleared = TRUE;
            DELETE_LIST( analy->selected_objects );
        }
        else
        {
            rval = htable_search( MESH( analy ).class_table, tokens[1], 
                                  FIND_ENTRY, &p_hte );
            p_mo_class = (MO_class_data *) p_hte->data;

            if ( rval == OK && token_cnt == 2 )
            {
                /* Clear all for specified class. */
                p_so = analy->selected_objects;
                while ( p_so != NULL )
                {
                    if ( p_so->mo_class == p_mo_class )
                    {
                        p_del_so = p_so;
                        NEXT( p_so );
                        DELETE( p_del_so, analy->selected_objects );
                        selection_cleared = TRUE;
                    }
                    else
                        NEXT( p_so );
                }
            }
            else if ( rval == OK && token_cnt > 2 )
            {
                /* Clear specific selections for class. */

                for ( i = 2; i < token_cnt; i++ )
                {
                    sscanf( tokens[i], "%d", &j );
                    j--; 
                    p_so = analy->selected_objects;
                    while ( p_so != NULL )
                    {
		        if ( p_so->mo_class == p_mo_class && 
			   (p_so->ident == j || p_so->label == j) )
                        {
                            DELETE( p_so, analy->selected_objects );
                            selection_cleared = TRUE;
                            break;
                        }
                        else
                            NEXT( p_so );
                    }
                }
            }
            else
            {
                popup_dialog( USAGE_POPUP, 
                              "clrsel [<class name> [<ident>...]]" );
                valid_command = FALSE;
            }
        }

        if ( selection_cleared )
        {
/**/
            /* clear_gather( analy ); */
            redraw = BINDING_MESH_VISUAL;
        }
    }
    else if ( strcmp( tokens[0], "show" ) == 0 )
    {

        if ( strcmp( tokens[1], "damage" ) == 0 )
        {
            if ( token_cnt != 6 )
            {
                popup_dialog( USAGE_POPUP, "show damage vel_dir <vx,vy,vz>, vel_cutoff, relVol_cutoff, eps_cutoff ");
                valid_command = FALSE;
            }
            else
            {
                if ( strncmp( tokens[2], "v", 1 ) != 0 )
                {
                    popup_dialog( USAGE_POPUP, "show damage vel_dir <vx,vy,vz>, vel_cutoff, relVol_cutoff, eps_cutoff ");
                    valid_command = FALSE;
                }
                else
                {
                    strcpy( analy->damage_params.vel_dir, tokens[2] );
                    sscanf( tokens[3], "%f", analy->damage_params.cut_off );
                    sscanf( tokens[4], "%f", analy->damage_params.cut_off+1 );
                    sscanf( tokens[5], "%f", analy->damage_params.cut_off+2 );
                    analy->result_mod = TRUE;
		    if (  strncmp( tokens[1], "mat", 3 ) == 0 )
		          analy->result_active = FALSE;

                    if ( parse_show_command( tokens[1], analy ) )
                        redraw = BINDING_MESH_VISUAL;
                    else
                        valid_command = FALSE;
                }
            }
        }
        else
        {
            analy->result_mod = TRUE;
	    if ( !strcmp("show", tokens[0]) && strcmp("mat", tokens[1]) )
		 strcpy( analy->ei_result_name,tokens[1] );

            if ( parse_show_command( tokens[1], analy ) )
                redraw = BINDING_MESH_VISUAL;
            else
                valid_command = FALSE;
        }
    }

    /* Multi-commands like "show" should go here for faster parsing.
     * Follow those with display-changing commands.
     * Follow those with miscellaneous commands (info, etc.).
     */
 
    else if ( strcmp( tokens[0], "quit" ) == 0   ||
              strcmp( tokens[0], "done" ) == 0   ||
	      strcmp( tokens[0], "exit" ) == 0   ||
              strcmp( tokens[0], "end" ) == 0 )
    {
        write_log_message( );

#ifdef TIME_GRIZ
        manage_timer( 8, 1 );
#endif
        quit( 0 );
    }
    else if ( strcmp( tokens[0], "help" ) == 0 || strcmp( tokens[0], "man" ) == 0 ||  strcmp( tokens[0], "grizman" ) == 0 ||
              strcmp( tokens[0], "?" ) == 0 )
    {
         griz_home = getenv( "GRIZHOME" );
	 if ( griz_home != NULL )
	 {
	      sprintf( acroread_cmd, "acroread %s/griz_manual.pdf &", griz_home );
	      system( acroread_cmd );
	 }
    }
    else if ( strcmp( tokens[0], "relnotes" ) == 0 || strcmp( tokens[0], "notes" ) == 0 ||  strcmp( tokens[0], "rn" ) == 0 ||
              strcmp( tokens[0], "rnotes" ) == 0 )
    {
         griz_home = getenv( "GRIZHOME" );
	 if ( griz_home != NULL )
	 {
	      sprintf( acroread_cmd, "acroread %s/griz_relnotes.pdf &", griz_home );
	      system( acroread_cmd );
	 }
    }
     else if ( strcmp( tokens[0], "copyrt" ) == 0 )
    {
        copyright();
    }

    /* IRC: Added May 15, 2007: New function to hide all windows except Render window */
    else if ( strcmp( tokens[0], "hidewin" ) == 0 || strcmp( tokens[0], "pushwin" ) == 0)
    {
        pushpop_window( PUSHPOP_BELOW );
    }
    else if ( strcmp( tokens[0], "unhidewin" ) == 0 || strcmp( tokens[0], "popwin" ) == 0 )
    {
        pushpop_window( PUSHPOP_ABOVE);
    }
    else if ( strcmp( tokens[0], "tell" ) == 0 )
    {
        if ( token_cnt > 1 )
            parse_tell_command( analy, tokens, token_cnt, TRUE, &redraw );
        else
            popup_dialog( USAGE_POPUP, 
                          "tell <reports>\n" 
                          "  Where <reports> can be any sequence of:\n"
                          "    info\n"
                          "    times [<first state> [<last state>]]\n"
                          "    results\n"
                          "    class\n"
                          "    pos <class> <ident>\n"
                          "    mm [<result> [<first state> [<last state>]]]\n"
                          "    em\n" 
                          "    th\n" 
                          "    iso\n" 
                          "    view\n\n"
                          "  Aliases\n"
                          "    \"lts [<args>]\" for \"tell times [<args>]\"\n"
                          "    \"info\" for \"tell info class view\"\n"
                          "    \"tellmm <args>\" for \"tell mm <args>\"\n"
                          "    \"tellpos <args>\" for \"tell pos <args>\"\n"
                          "    \"telliso\" for \"tell iso\"\n" 
                          "    \"tellem\" for \"tell em\"" );
    }
    else if ( strcmp( tokens[0], "info" ) == 0 )
    {
        char info_tokens[4][TOKENLENGTH] = 
        {
            "tell", "info", "class", "view"
        };

        parse_tell_command( analy, info_tokens, 4, TRUE, &redraw );
    }
    else if ( strcmp( tokens[0], "lts" ) == 0 )
    {
        parse_tell_command( analy, tokens, token_cnt, FALSE, &redraw );
    }
    else if ( strcmp( tokens[0], "tellpos" ) == 0)
    {
        parse_tell_command( analy, tokens, token_cnt, FALSE, &redraw );
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
            if ( !load_analysis( tokens[1], analy ) )
                return;
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "bufqty" ) == 0 )
    {

        if ( token_cnt == 3 )
        {
            p_string = tokens[1];
            j = atoi( tokens[2] );
        }
        else if ( token_cnt == 2 )
        {
            p_string = NULL;
            j = atoi( tokens[1] );
        }
        else
            valid_command = FALSE;

        if ( j < 0 )
            valid_command = FALSE;
        
        if ( valid_command )
            for ( i = 0; i < analy->mesh_qty; i++ )
                analy->db_set_buffer_qty( analy->db_ident, i, p_string, j );
        else
            popup_dialog( USAGE_POPUP, 
                          "bufqty [<class name>] <buffer qty>" );
    }
    else if ( strcmp( tokens[0], "rview" ) == 0 )
    {
      if (analy->rb_vcent_flag)
      {
         history_command(  "vcent off" );
         parse_command( "vcent off", analy);
         parse_command( "scale 1", analy);
      }
        reset_mesh_transform();

     if (analy->rb_vcent_flag)
      {
         analy->rb_vcent_flag==FALSE;
         adjust_near_far( analy );
      }
         redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "vcent" ) == 0 )
    {
        parse_vcent( analy, tokens, token_cnt, &redraw );
    }
    else if ( strcmp( tokens[0], "conv" ) == 0 )
    {
        if ( token_cnt > 3 )
            popup_dialog( USAGE_POPUP, "conv [<scale> [<offset>]]" );
        else
        {
            if ( token_cnt > 1 )
            {
                sscanf( tokens[1], "%f", &val );
                analy->conversion_scale = val;
            
                if ( token_cnt > 2 )
                {
                    sscanf( tokens[2], "%f", &val );
                    analy->conversion_offset = val;
                }
                else
                    analy->conversion_offset = 0.0;
            }
            else
            {
                analy->conversion_scale = 1.0;
                analy->conversion_offset = 0.0;
            }

            if ( analy->perform_unit_conversion )
                redraw = redraw_for_render_mode( FALSE, RENDER_ANY, 
                                                 analy );
        }
    }
    else if (strcmp( tokens[0], "clrconv" ) == 0 )
    {
        analy->conversion_scale = 1.0;
        analy->conversion_offset = 0.0;

        analy->perform_unit_conversion = FALSE;

        redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
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
            else if ( strcmp( tokens[i], "scale" ) == 0 )
                analy->show_scale = setval;
            else if ( strcmp( tokens[i], "date" ) == 0 )
                analy->show_datetime = setval;
            else if ( strcmp( tokens[i], "all" ) == 0 )
            {   
                analy->show_coord     = setval;
                analy->show_time      = setval;
                analy->show_title     = setval;
                analy->show_colormap  = setval;
                analy->show_minmax    = setval;
                analy->show_scale     = setval;
                analy->show_datetime  = setval;
		if ( !setval )
		     parse_command( "off ei", analy ); /* On all should not enable ei mode */
           }
            else if ( strcmp( tokens[i], "cscale" ) == 0 )
                analy->show_colorscale = setval;
            else if ( strcmp( tokens[i], "edges" ) == 0 )
            {
                if ( strcmp( tokens[0], "on" ) == 0
                     && analy->state_p->sand_present )
                    get_mesh_edges( analy->cur_mesh_id, analy );
                analy->show_edges = setval;
                if ( setval && MESH( analy ).edge_list == NULL )
                    popup_dialog( INFO_POPUP, 
                                  "Empty edge list; check mat visibility" );
                update_util_panel( VIEW_EDGES, NULL );
            }
            else if ( strcmp( tokens[i], "safe" ) == 0 )
                analy->show_safe = setval;
/*
            else if ( strcmp( tokens[i], "ndnum" ) == 0 )
                analy->show_node_nums = setval;
            else if ( strcmp( tokens[i], "elnum" ) == 0 )
                analy->show_elem_nums = setval;
*/
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
                if ( cutting_plane_defined )
                {
                    analy->show_cut = setval;
                    gen_cuts( analy );
#ifdef HAVE_VECTOR_CARPETS
                    gen_carpet_points( analy );
#endif
                    check_visualizing( analy );
                }
                else
                    popup_dialog( WARNING_POPUP,
                                  "Cutting plane must be defined BEFORE display/removal of the cutting plane.\n"
                                  "cutpln <pt x> <pt y> <pt z> <norm x> <norm y> <norm z>" );
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
#ifdef HAVE_VECTOR_CARPETS
                gen_carpet_points( analy );
#endif
                check_visualizing( analy );
            }
            else if ( strcmp( tokens[i], "vec" ) == 0 )
            {
                if ( setval &&
                     analy->vector_result[0] == NULL &&
                     analy->vector_result[1] == NULL &&
                     analy->vector_result[2] == NULL )
                {
                    popup_dialog( INFO_POPUP, "%s%s",
                                  "Please set a vector result, i.e.,\n",
                                  "  \"vec <vx> <vy> <vz>\"." );
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
#ifdef HAVE_VECTOR_CARPETS
            else if ( strcmp( tokens[i], "carpet" ) == 0 )
            {
                if ( setval &&
                     analy->vec_id[0] == VAL_NONE &&
                     analy->vec_id[1] == VAL_NONE &&
                     analy->vec_id[2] == VAL_NONE )
                {
                    popup_dialog( INFO_POPUP, "%s%s",
                                  "Please set a vector result, i.e.,\n",
                                  "  \"vec <vx> <vy> <vz>\"." );
                    analy->show_carpet = FALSE;
                }
                else
                    analy->show_carpet = setval;
                gen_reg_carpet_points( analy );
                gen_carpet_points( analy );
                check_visualizing( analy );
            }
#endif
            else if ( strcmp( tokens[i], "refresh" ) == 0 )
                analy->refresh = setval;
            else if ( strcmp( tokens[i], "sym" ) == 0 )
                analy->reflect = setval;
            else if ( strcmp( tokens[i], "cull" ) == 0 )
                analy->manual_backface_cull = setval;
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
            else if ( strcmp( tokens[i], "locref" ) == 0 )
                analy->loc_ref = setval;
            else if ( strcmp( tokens[i], "autorgb" ) == 0 )
            { 
                if ( setval && analy->img_root == NULL )
                    popup_dialog( INFO_POPUP, "%s\n%s\n%s", 
                                 "Please specify an RGB root name",
                                 "with \"autorgb <root name>\"",
                                 "on command ignored" );
                else                                 
                    analy->auto_img = setval;
            }
            else if ( strcmp( tokens[i], "autoimg" ) == 0 )
            { 
                if ( setval && analy->img_root == NULL )
                    popup_dialog( INFO_POPUP, "%s\n%s", 
                                 "Please specify an image root name with the "
                                 "\"autoimg\" command;",
                                 "on command ignored" );
                else                                 
                    analy->auto_img = setval;
            }
            else if ( strcmp( tokens[i], "num" ) == 0 )
            { 
                if ( analy->num_class == NULL )
                    popup_dialog( USAGE_POPUP, "%s\n%s\n%s", 
                                 "Please specify element classes",
                                 "with \"numclass <classname>...\"",
                                 "on command ignored" );
                else
                    analy->show_num = setval;
            }
                                 
            else if ( strcmp( tokens[i], "lighting" ) == 0 )
            {
                if ( analy->dimension == 2 && setval == TRUE )
                    popup_dialog( INFO_POPUP, 
                                  "Lighting is not utilized for 2D meshes." );
                else
                    v_win->lighting = setval;
            }
            else if ( strcmp( tokens[i], "bgimage" ) == 0 )
                analy->show_background_image = setval;
            else if ( strcmp( tokens[i], "coordxf" ) == 0 )
            {
#ifdef HAVE_NEW_RES_ENUMS
/* 
 * Need to re-define derived result enums to enable this specificity
 * without having to perform string comparisons.  Also need to consider
 * desirability and method to allow tensor transform on appropriate 
 * primal results.
 */
                analy->do_tensor_transform = setval;
                if ( analy->tensor_transform_matrix != NULL
                     && (    analy->result_id >= VAL_SHARE_EPSX
                             && analy->result_id <= VAL_SHARE_EPSZX
                          || 
                             analy->result_id >= VAL_SHARE_SIGX
                             && analy->result_id <= VAL_SHARE_SIGZX ) )
#endif
                if ( analy->do_tensor_transform != setval )
                {
                    analy->do_tensor_transform = setval;
                    analy->result_mod = TRUE;
                    if ( setval && analy->ref_frame == LOCAL )
                        /* These two are exclusive of each other. */
                        analy->ref_frame = GLOBAL;
                }
            }
            else if ( strcmp( tokens[i], "particles" ) == 0 )
                analy->show_particle_class = setval;
            else if ( strcmp( tokens[i], "glyphs" ) == 0 )
                analy->show_plot_glyphs = setval;
            else if ( strcmp( tokens[i], "plotcol" ) == 0 )
                analy->color_plots = setval;
            else if ( strcmp( tokens[i], "glyphcol" ) == 0 )
                analy->color_glyphs = setval;
            else if ( strcmp( tokens[i], "leglines" ) == 0 )
                analy->show_legend_lines = setval;
            else if ( strcmp( tokens[i], "plotgrid" ) == 0 )
                analy->show_plot_grid = setval;
            else if ( strcmp( tokens[i], "plotcoords" ) == 0 )
            {
                analy->show_plot_coords = setval;
                manage_plot_cursor_display( setval, analy->render_mode,
                                            analy->render_mode );
            }
            else if ( strcmp( tokens[i], "derived" ) == 0 )
            {
                switch ( analy->result_source )
                {
                    case DERIVED:
                        if ( !setval )
                        {
                            /* At least one table must be on. */
                            popup_dialog( INFO_POPUP,
                                          "Turning on primal result table." );
                            analy->result_source = PRIMAL;
                        }
                        break;
                    case PRIMAL:
                        if ( setval )
                            /* From primal to both. */
                            analy->result_source = ALL;
                        break;
                    case ALL:
                        if ( !setval )
                            /* From both to primal. */
                            analy->result_source = PRIMAL;
                        break;
                }

                analy->result_mod = refresh_shown_result( analy );
            }
            else if ( strcmp( tokens[i], "primal" ) == 0 )
            {
                switch ( analy->result_source )
                {
                    case DERIVED:
                        if ( setval )
                            /* From derived to both. */
                            analy->result_source = ALL;
                        break;
                    case PRIMAL:
                        if ( !setval )
                        {
                            /* At least one table must be on. */
                            popup_dialog( INFO_POPUP,
                                          "Turning on derived result table." );
                            analy->result_source = DERIVED;
                        }
                        break;
                    case ALL:
                        if ( !setval )
                            /* From both to derived. */
                            analy->result_source = DERIVED;
                        break;
                }

                analy->result_mod = refresh_shown_result( analy );
            }
            else if ( strcmp( tokens[i], "hex_overlap" ) == 0 )
            {
                if ( setval != analy->hex_overlap )
                {
                     analy->hex_overlap = setval;
		     update_hex_adj( analy );
		     reset_face_visibility( analy );
		     if ( analy->dimension == 3 ) 
		          renorm = TRUE;
		     redraw = NONBINDING_MESH_VISUAL;
                }
            }
            else if ( strcmp( tokens[i], "su" ) == 0 )
            {
              suppress_display_updates( setval );
            }
            else if ( strcmp( tokens[i], "ei" ) == 0 )
            {
	              if ( analy->ei_result != setval )
	              {
			   analy->ei_result = setval;
			 
			   if ( !setval ) /* Turing EI mode off */
			   {
			        redraw = BINDING_MESH_VISUAL;

				parse_command( "setcol bg 1 1 1", analy );

				strcpy(tmp_token, "show ");
				strcat(tmp_token, analy->ei_result_name);
				parse_command( tmp_token, analy );
				
				/* Delete the plot for this result if computing an EI result */
				strcpy(tmp_token, "delth ");
				strcat(tmp_token, analy->ei_result_name);
				parse_command( tmp_token, analy );
			   }
			   else /* Enabling EI */
			   {
				analy->ei_result = setval;
				parse_command( "setcol bg .7568 .8039 .80391", analy );

				strcpy(tmp_token, "show ");
				strcat(tmp_token, analy->ei_result_name);
				parse_command( tmp_token, analy );
			   }
		      }
            }
             else if ( strcmp( tokens[i], "logscale" ) == 0 )
            {
              analy->logscale = setval;
            }
            else if ( strcmp( tokens[i], "grayscale" ) == 0  ||
		      strcmp( tokens[i], "matgs" )     == 0  ||
		      strcmp( tokens[i], "gs" ) == 0 )
            {
              analy->material_greyscale = setval;
              if  ( setval )
		    update_util_panel( VIEW_GS, NULL );
	      else
		    update_util_panel( VIEW_GS_NONE, NULL );
             }
             else if ( strcmp( tokens[i], "damage_hide" ) == 0 )
            {
              analy->damage_hide       = setval;
              analy->reset_damage_hide = TRUE;
              analy->result_mod = TRUE;
              reset_face_visibility( analy );
              if ( analy->dimension == 3 ) renorm = TRUE;
              redraw = BINDING_MESH_VISUAL;
            }
           else if ( strcmp( tokens[i], "fn_output_mom" ) == 0 )
            {
              analy->fn_output_momentum = setval;
            }
             else if ( strcmp( tokens[i], "free_nodes" ) == 0 ||  
                      strcmp( tokens[i], "particle" )   == 0 ||
                      strcmp( tokens[i], "particles" )  == 0 ||
                      strcmp( tokens[i], "fn" )         == 0 ||
                      strcmp( tokens[i], "pn" ) == 0)
            {

              if ( strcmp( tokens[i], "particle" )  == 0 ||
                   strcmp( tokens[i], "particles" ) == 0 ||
                   strcmp( tokens[i], "pn" ) == 0 )
                 analy->free_particles = setval;
              else
                 analy->free_nodes     = setval;


              analy->result_mod = FALSE; /* IRC: was TRUE */

              qty = MESH( analy ).material_qty;

              token_index = 1;
              if ( analy->dimension == 3 ) renorm = TRUE;
              redraw = BINDING_MESH_VISUAL;
	     
              if (!strcmp( tokens[i+token_index], "scale"))
              {
                 analy->free_nodes_scale_factor = atof( tokens[i + token_index+1] );
                 token_index+=2;
              }

              /* User has input the sphere resolution factor */
              if (!strcmp( tokens[i+token_index], "res"))
              {
                analy->free_nodes_sphere_res_factor = atoi( tokens[i + token_index+1] );
                 if (analy->free_nodes_sphere_res_factor<2)
                    analy->free_nodes_sphere_res_factor = 2;
		 token_index+=2;
              }

              if (!strcmp( tokens[i+token_index], "scale"))
              {
                 analy->free_nodes_scale_factor = atof( tokens[i + token_index+1] );
		 token_index+=2;
              }

              if (!strcmp( tokens[i+token_index], "mass_scale") ||
                  !strcmp( tokens[i+token_index], "ms") )
              {
                 if (!strcmp( tokens[i+token_index+1], "on"))
		   {
                    analy->free_nodes_mass_scaling = 1;
		    analy->free_nodes_vol_scaling  = 0;
		   }
                 if (!strcmp( tokens[i+token_index+1], "off"))
                    analy->free_nodes_mass_scaling = 0;
		 token_index+=2;
              }

              if (!strcmp( tokens[i+token_index], "vol_scale") ||
                  !strcmp( tokens[i+token_index], "vs"))
              {
                 if (!strcmp( tokens[i+token_index+1], "on"))
		   {
                    analy->free_nodes_vol_scaling  = 1;
		    analy->free_nodes_mass_scaling = 0;
		   }
                 if (!strcmp( tokens[i+token_index+1], "off"))
		 {
                    analy->free_nodes_vol_scaling  = 0;
		 }
		 token_index+=2;
              }

              if (!strcmp( tokens[i+1], "mat"))
              {
                for ( i = token_index; 
                      i < token_cnt; 
                      i++ )
                  {
                    if ( is_numeric_range_token( tokens[i] ) )
                      {
                        parse_mtl_range( tokens[i], qty, &mat_min, &mat_max ) ;
                        mtl = mat_min ;
                        for (mtl =  mat_min ;
                             mtl <= mat_max ;
                             mtl++)
                          {
                            if (mtl > 0 && mtl <= qty)
                              {
                               analy->free_nodes_mats[mtl-1] = 1;
                              }
                          }
                      }
                    else
                      {
                        sscanf( tokens[i], "%d", &ival );
                        if ( ival > 0 && ival <= qty )
                           analy->free_nodes_mats[ival-1] = 1;
                      }
                  }
              }

              i = token_cnt;
            }
              else
                popup_dialog( INFO_POPUP,
                              "On/Off command unrecognized: %s\n", tokens[i] );
        } 

        /* Reload the result vector. */
        if ( analy->result_mod )
        {
            if ( analy->render_mode == RENDER_MESH_VIEW )
                load_result( analy, TRUE, TRUE );
            analy->display_mode_refresh( analy );
        }
        redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
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
            else if ( strcmp( tokens[i], "wf" ) == 0 || 
		      strcmp( tokens[i], "wireframe" ) == 0)
            {
                analy->mesh_view_mode = RENDER_WIREFRAME;
                update_util_panel( VIEW_WIREFRAME, NULL );
            }
            else if ( strcmp( tokens[i], "wft" ) == 0 || 
		      strcmp( tokens[i], "wireframet" ) == 0)
            {
                analy->mesh_view_mode = RENDER_WIREFRAMETRANS;
		analy->draw_wireframe_trans = TRUE;
                update_util_panel( VIEW_WIREFRAMETRANS, NULL );
            }
            else if ( strcmp( tokens[i], "hidden" ) == 0 )
            {
                analy->mesh_view_mode = RENDER_HIDDEN;
                update_util_panel( VIEW_SOLID_MESH, NULL );
            }
            else if ( strcmp( tokens[i], "solid" ) == 0 )
            {
                analy->mesh_view_mode = RENDER_FILLED;
                update_util_panel( VIEW_SOLID, NULL );
            }
            else if ( strcmp( tokens[i], "cloud" ) == 0 )
            {
                analy->mesh_view_mode = RENDER_POINT_CLOUD;
                update_util_panel( VIEW_POINT_CLOUD, NULL );
            }
            else if ( strcmp( tokens[i], "none" ) == 0 )
            {
                analy->mesh_view_mode = RENDER_NONE;
                update_util_panel( VIEW_NONE, NULL );
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
                update_util_panel( PICK_MODE_SELECT, NULL );
            }
            else if ( strcmp( tokens[i], "pichil" ) == 0 )
            {
                analy->mouse_mode = MOUSE_HILITE;
                update_util_panel( PICK_MODE_HILITE, NULL );
            }

            /* Result min/max. */
            else if( strcmp( tokens[i], "mstat" ) == 0 )
            {
                analy->use_global_mm = FALSE;
                if ( !analy->mm_result_set[0] )
                    analy->result_mm[0] = analy->state_mm[0];
                if ( !analy->mm_result_set[1] )
                    analy->result_mm[1] = analy->state_mm[1];
                analy->display_mode_refresh( analy );
            }
            else if( strcmp( tokens[i], "mglob" ) == 0 )
            {
                analy->use_global_mm = TRUE;
                if ( !analy->mm_result_set[0] )
                    analy->result_mm[0] = analy->global_mm[0];
                if ( !analy->mm_result_set[1] )
                    analy->result_mm[1] = analy->global_mm[1];
                analy->display_mode_refresh( analy );
            }

            /* Symmetry plane behavior. */
            else if ( strcmp( tokens[i], "symcu" ) == 0 )
                analy->refl_orig_only = FALSE;
            else if ( strcmp( tokens[i], "symor" ) == 0 )
                analy->refl_orig_only = TRUE;

            /* Screen aspect ratio correction. */
            else if ( strcmp( tokens[i], "vidasp" ) == 0 )
            {
                aspect_correct( TRUE );
            }
            else if ( strcmp( tokens[i], "norasp" ) == 0 )
            {
                aspect_correct( FALSE );
            }

            /* Shell reference surface. */
            else if ( strcmp( tokens[i], "middle" ) == 0 )
            {
                old = analy->ref_surf;
                analy->ref_surf = MIDDLE;
                analy->result_mod = analy->check_mod_required( analy, 
                                        REFERENCE_SURFACE, old,
                                        analy->ref_surf );
            }
            else if ( strcmp( tokens[i], "inner" ) == 0 )
            {
                old = analy->ref_surf;
                analy->ref_surf = INNER;
                analy->result_mod = analy->check_mod_required( analy, 
                                        REFERENCE_SURFACE, old,
                                        analy->ref_surf );
            }
            else if ( strcmp( tokens[i], "outer" ) == 0 )
            {
                old = analy->ref_surf;
                analy->ref_surf = OUTER;
                analy->result_mod = analy->check_mod_required( analy, 
                                        REFERENCE_SURFACE, old,
                                        analy->ref_surf );
            }

            /* Strain basis. */
            else if ( strcmp( tokens[i], "rglob" ) == 0 )
            {
                old = analy->ref_frame;
                analy->ref_frame = GLOBAL;
                analy->result_mod = analy->check_mod_required( analy, 
                                        REFERENCE_FRAME, old,
                                        analy->ref_frame );
            }
            else if ( strcmp( tokens[i], "rloc" ) == 0 )
            {
                old = analy->ref_frame;
                analy->ref_frame = LOCAL;
                analy->result_mod = analy->check_mod_required( analy, 
                                        REFERENCE_FRAME, old,
                                        analy->ref_frame );
                if ( old != LOCAL )
                    /* LOCAL and analy->do_tensor_transform are exclusive */
                    analy->do_tensor_transform = FALSE;
            }

            /* Strain variety. */
            else if ( strcmp( tokens[i], "infin" ) == 0 )
            {
	        if (is_primal_quad_strain_result( analy->cur_result->name ))  
		{
		    popup_dialog( USAGE_POPUP,
				  "Cannot set strain variety for a primal shell strain" );
		    valid_command = FALSE;
		} 
                old = analy->strain_variety;
                analy->strain_variety = INFINITESIMAL;
                analy->result_mod = analy->check_mod_required( analy, 
                                        STRAIN_TYPE, old,
                                        analy->strain_variety );
            }
            else if ( strcmp( tokens[i], "grn" ) == 0 )
            {
                old = analy->strain_variety;
                analy->strain_variety = GREEN_LAGRANGE;
                analy->result_mod = analy->check_mod_required( analy, 
                                        STRAIN_TYPE, old,
                                        analy->strain_variety );
            }
            else if ( strcmp( tokens[i], "alman" ) == 0 )
            {
                old = analy->strain_variety;
                analy->strain_variety = ALMANSI;
                analy->result_mod = analy->check_mod_required( analy, 
                                        STRAIN_TYPE, old,
                                        analy->strain_variety );
            }
            else if ( strcmp( tokens[i], "rate" ) == 0 )
            {
                old = analy->strain_variety;
                analy->strain_variety = RATE;
                analy->result_mod = analy->check_mod_required( analy, 
                                        STRAIN_TYPE, old,
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
            else if ( strcmp( tokens[i], "glyphalign" ) == 0 )
            {
                set_glyph_alignment( GLYPH_ALIGN );
            }
            else if ( strcmp( tokens[i], "glyphstag" ) == 0 )
            {
                set_glyph_alignment( GLYPH_STAGGERED );
            }
            else
                popup_dialog( INFO_POPUP, 
                              "Switch command unrecognized: %s\n", tokens[i] );
        } 

        /* Reload the result vector. */
        if ( analy->result_mod )
        {
            if ( analy->render_mode == RENDER_MESH_VIEW )
                load_result( analy, TRUE, TRUE );
            analy->display_mode_refresh( analy );
        }
        redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
    }
    else if ( strcmp( tokens[0], "setpick" ) == 0 )
    {
        if ( token_cnt == 3 )
        {
            ival = atoi( tokens[1] );
            if ( ival == 1 )
                btn_type = BTN1_PICK;
            else if ( ival == 2 )
                btn_type = BTN2_PICK;
            else if ( ival == 3 )
                btn_type = BTN3_PICK;
            else
                valid_command = FALSE;
            
            if ( valid_command )
            {
                p_mo_class = find_named_class( MESH_P( analy ), tokens[2] );
                if ( p_mo_class != NULL )
                    set_pick_class( p_mo_class, btn_type );
                else
                    valid_command = FALSE;
            }
        }
        else
            valid_command = FALSE;
        
        if ( valid_command )
            update_util_panel( btn_type, p_mo_class );
        else
            popup_dialog( USAGE_POPUP,
                          "setpick <button (ie,  1, 2, or 3)> <class name>" );
    }
    else if ( strcmp( tokens[0], "fracsz" ) == 0 )
    {
        sscanf( tokens[1], "%d", &ival );
        if ( ival >= 0 && ival <= 6 )
        {
            analy->float_frac_size = ival;
            redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
        }
    }
    else if ( strcmp( tokens[0], "hidwid" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        if ( token_cnt == 2 && val > 0.0 )
        {
            analy->hidden_line_width = val;
            redraw = redraw_for_render_mode( FALSE, RENDER_MESH_VIEW, analy );
        }
        else
            popup_dialog( USAGE_POPUP, "hidwid <line width>" );
    }


    /****************************************
     * VIS/INVIS
     ****************************************
     */
    else if ( strcmp( tokens[0], "invis" ) == 0 ||
              strcmp( tokens[0], "vis" ) == 0 )
    {
        if ( strcmp( tokens[0], "invis" ) == 0 )
        {
            setval       = TRUE;
            vis_selected = FALSE;
        }
        else
        {
            setval       = FALSE;
            vis_selected = TRUE;
        }

	idx = 0;
 
	mat_selected    = TRUE;
	result_selected = FALSE;
	qty        = MESH( analy ).material_qty;
	p_uc       = MESH( analy ).hide_material;
	mat_qty    = MESH( analy ).material_qty;
        p_hide_qty = &MESH( analy ).mat_hide_qty;
        p_uc       = MESH( analy ).hide_material;
        p_elem     = NULL;

       /* Look for element selection keyword */
       string_to_upper( tokens[1], tmp_token ); /* Make case insensitive */

       if( strcmp( tmp_token, "ELEM" ) == 0 )
       {
	   mat_selected = FALSE;
	   string_to_upper( tokens[2], tmp_token ); /* Make case insensitive */
	   idx++;
       }

       if( strcmp( tmp_token, "RESULT" ) == 0 )
       {
	   result_selected = TRUE;
	   mat_selected    = FALSE;
	   string_to_upper( tokens[2], tmp_token ); /* Make case insensitive */
	   idx++;
       }

        if( strcmp( tokens[1], "SURF" ) == 0 )
        {
            qty  = MESH( analy ).surface_qty;
            p_uc = MESH( analy ).hide_surface;
            idx++;
        }
        
        if ( strstr( "BRICK", tmp_token ) )
        {
	     string_to_upper( tokens[2], tmp_token );      /* Make case insensitive */
	     mat_selected    = FALSE; 

	     if ( result_selected )
	     {
	          MESH( analy ).hide_brick_by_result = TRUE;
	          MESH( analy ).brick_result_min = atof( tokens[3] );
	          MESH( analy ).brick_result_max = atof( tokens[4] );
	          idx+=3;
	     }

	     if (!mat_selected && !result_selected)
	     {
	         MESH( analy ).hide_brick_by_mat    = FALSE;
	         MESH( analy ).hide_brick_by_result = FALSE;

		 /* Hide bricks by elements */
		 elem_qty        =  MESH( analy ).brick_qty;
		 p_elem          =  MESH( analy ).hide_brick_elem;
		 p_hide_elem_qty =  &MESH( analy ).hide_brick_elem_qty;
	     }

             idx++;
        }


        if ( strstr( "SHELL", tmp_token ) )
        {
	     string_to_upper( tokens[2], tmp_token );      /* Make case insensitive */
  	     mat_selected = FALSE;
 
	     if ( result_selected )
	     {
	          MESH( analy ).hide_shell_by_result = TRUE;
	          MESH( analy ).shell_result_min = atof( tokens[3] );
	          MESH( analy ).shell_result_max = atof( tokens[4] );
	          idx+=3;
	     }

	     if (!mat_selected && !result_selected)
	     {
	         MESH( analy ).hide_shell_by_mat    = FALSE;
	         MESH( analy ).hide_shell_by_result = FALSE;

		 /* Hide shells by elements */
		 elem_qty        =  MESH( analy ).shell_qty;
		 p_elem          =  MESH( analy ).hide_shell_elem;
		 p_hide_elem_qty =  &MESH( analy ).hide_shell_elem_qty;
	     }

             idx++;
        }

        if ( strstr( "TRUSS", tmp_token ) )
        {
	     string_to_upper( tokens[2], tmp_token );      /* Make case insensitive */
	     mat_selected = FALSE; 
	     if ( result_selected )
	     {
	          MESH( analy ).hide_truss_by_result = TRUE;
	          MESH( analy ).truss_result_min = atof( tokens[3] );
	          MESH( analy ).truss_result_max = atof( tokens[4] );
	          idx+=3;
	     }

	     if (!mat_selected && !result_selected)
	     {
	         MESH( analy ).hide_truss_by_mat    = FALSE;
	         MESH( analy ).hide_truss_by_result = FALSE;

		 /* Hide trusss by elements */
		 elem_qty        =  MESH( analy ).truss_qty;
		 p_elem          =  MESH( analy ).hide_truss_elem;
		 p_hide_elem_qty =  &MESH( analy ).hide_truss_elem_qty;
	     }

             idx++;
        }

        if ( strstr( "BEAM", tmp_token ) )
        {
	     string_to_upper( tokens[2], tmp_token );      /* Make case insensitive */
	     mat_selected = FALSE; 
	     if ( result_selected )
	     {
	          MESH( analy ).hide_beam_by_result = TRUE;
	          MESH( analy ).beam_result_min = atof( tokens[3] );
	          MESH( analy ).beam_result_max = atof( tokens[4] );
	          idx+=3;
	     }

	     if (!mat_selected && !result_selected)
	     {
	         MESH( analy ).hide_beam_by_mat    = FALSE;
	         MESH( analy ).hide_beam_by_result = FALSE;
		 
		 /* Hide beams by elements */
		 elem_qty        =  MESH( analy ).beam_qty;
		 p_elem          =  MESH( analy ).hide_beam_elem;
		 p_hide_elem_qty =  &MESH( analy ).hide_beam_elem_qty;
	     }

             idx++;
        }

	/* Particle type objects */
        if ( strstr( tmp_token, "PART" ) )
        {
	     if ( result_selected )
	     {
	          MESH( analy ).hide_particle_by_result = TRUE;
	          MESH( analy ).particle_result_min = atof( tokens[3] );
	          MESH( analy ).particle_result_max = atof( tokens[4] );
	          idx+=3;
	     }
	     else
	          MESH( analy ).hide_particle_by_result = FALSE;

             p_uc         = MESH( analy ).hide_particle;
             p_hide_qty   = &MESH( analy ).particle_hide_qty;

	     elem_qty     = mat_qty;
             idx++;
	     mat_selected = FALSE;
        } 
  
        if ( !result_selected)
	     process_mat_obj_selection ( analy,  tokens, idx, token_cnt, mat_qty,
					 elem_qty, 
					 p_elem, p_hide_qty, p_hide_elem_qty, 
					 p_uc, 
					 setval, vis_selected, mat_selected );
	

        reset_face_visibility( analy );
        if ( analy->dimension == 3 ) renorm = TRUE;
        analy->result_mod = TRUE;
        redraw = BINDING_MESH_VISUAL;
    }


    /****************************************
     * ENABLE/DISABLE
     ****************************************
     */

    else if ( strcmp( tokens[0], "disable" ) == 0 ||
              strcmp( tokens[0], "enable" ) == 0 )
    {
        if ( strcmp( tokens[0], "disable" ) == 0 )
        {
             setval = TRUE;
             enable_selected = FALSE;
        }
        else
        {
             setval = FALSE;
             enable_selected = TRUE;
        }

	idx        = 0;
	mat_selected    = TRUE;
	result_selected = FALSE;
	qty        = MESH( analy ).material_qty;
	p_uc       = MESH( analy ).disable_material;
	mat_qty    = MESH( analy ).material_qty;
        p_hide_qty = &MESH( analy ).mat_disable_qty;
        p_uc       = MESH( analy ).disable_material;
        p_elem     = NULL;

	/* Look for element selection keyword */
	string_to_upper( tokens[1], tmp_token ); /* Make case insensitive */

	if( strcmp( tmp_token, "ELEM" ) == 0 )
        {
	  mat_selected = FALSE;
	  string_to_upper( tokens[2], tmp_token ); /* Make case insensitive */
	  idx++;
        }

	/* Result range selection */
        if( strcmp( tmp_token, "RESULT" ) == 0 )
        {
	    result_selected = TRUE;
	    string_to_upper( tokens[2], tmp_token ); /* Make case insensitive */
	    idx++;
        }

        if( strcmp( tokens[1], "SURF" ) == 0 )
        {
            mat_qty = MESH( analy ).surface_qty;
            p_uc    = MESH( analy ).disable_surface;
            idx++;
        }

        if ( strstr( "BRICK", tmp_token ) )
        {
	     string_to_upper( tokens[2], tmp_token );      /* Make case insensitive */
 
	     if ( result_selected )
	     {
	          MESH( analy ).brick_result_min = atof( tokens[3] );
	          MESH( analy ).brick_result_max = atof( tokens[4] );
	          idx+=3;
	     }

	     if (!mat_selected && !result_selected)
	     {
	         MESH( analy ).disable_brick_by_mat = FALSE;

		 /* Disable bricks by elements */
		 elem_qty           =  MESH( analy ).brick_qty;
		 p_elem             =  MESH( analy ).disable_brick_elem;
		 p_disable_elem_qty =  &MESH( analy ).disable_brick_elem_qty;
	     }

             idx++;
        }

        if ( strstr( "SHELL", tmp_token ) )
        {
	     string_to_upper( tokens[2], tmp_token );      /* Make case insensitive */
 
	     if ( result_selected )
	     {
	          MESH( analy ).shell_result_min = atof( tokens[3] );
	          MESH( analy ).shell_result_max = atof( tokens[4] );
	          idx+=3;
	     }

	     if (!mat_selected && !result_selected)
	     {
	         MESH( analy ).disable_shell_by_mat = FALSE;

		 /* Disable shells by elements */
		 elem_qty           =  MESH( analy ).shell_qty;
		 p_elem             =  MESH( analy ).disable_shell_elem;
		 p_disable_elem_qty =  &MESH( analy ).disable_shell_elem_qty;
	     }

             idx++;
        }

        if ( strstr( "TRUSS", tmp_token ) )
        {
	     string_to_upper( tokens[2], tmp_token );      /* Make case insensitive */
 
	     if ( result_selected )
	     {
	          MESH( analy ).truss_result_min = atof( tokens[3] );
	          MESH( analy ).truss_result_max = atof( tokens[4] );
	          idx+=3;
	     }


	     if (!mat_selected && !result_selected)
	     {
	         MESH( analy ).disable_truss_by_mat=FALSE;

		 /* Disable trusss by elements */
		 elem_qty           =  MESH( analy ).truss_qty;
		 p_elem             =  MESH( analy ).disable_truss_elem;
		 p_disable_elem_qty =  &MESH( analy ).disable_brick_elem_qty;
	     }

             idx++;
        }

        if ( strstr( "BEAM", tmp_token ) )
        {
	     string_to_upper( tokens[2], tmp_token );      /* Make case insensitive */
 
	     if ( result_selected )
	     {
	          MESH( analy ).beam_result_min = atof( tokens[3] );
	          MESH( analy ).beam_result_max = atof( tokens[4] );
	          idx+=3;
	     }


	     if (!mat_selected && !result_selected)
	     {
	         MESH( analy ).disable_beam_by_mat=FALSE;

		 /* Disable beams by elements */
		 elem_qty           =  MESH( analy ).beam_qty;
		 p_elem             =  MESH( analy ).disable_beam_elem;
		 p_disable_elem_qty =  &MESH( analy ).disable_beam_elem_qty;
	     }

             idx++;
        }

        if ( strstr( tmp_token, "PART" ) )
        {
	     if ( result_selected )
	     {
	          MESH( analy ).particle_result_min = atof( tokens[3] );
	          MESH( analy ).particle_result_max = atof( tokens[4] );
	          idx+=3;
	     }

             p_uc       = MESH( analy ).disable_particle;
             p_hide_qty = &MESH( analy ).particle_disable_qty;

	     elem_qty   = mat_qty;
             idx++;
	     mat_selected = TRUE;
        } 

        if ( !result_selected)
             process_mat_obj_selection ( analy,  tokens, idx, token_cnt, mat_qty,
					 elem_qty, 
					 p_elem, p_hide_qty, p_disable_elem_qty, 
					 p_uc, 
					 setval, enable_selected, mat_selected );
	
        analy->result_mod = TRUE;
        load_result( analy, TRUE, TRUE );
        redraw = BINDING_MESH_VISUAL;
    }


    /****************************************
     * INCLUDE/EXCLUDE
     ****************************************
     */

   else if ( strcmp( tokens[0], "exclude" ) == 0 ||
              strcmp( tokens[0], "include" ) == 0 )
    {
        if ( strcmp( tokens[0], "exclude" ) == 0 )
        {
            setval           = TRUE;
            include_selected = FALSE;
        }
        else
        {
            setval           = FALSE;
            include_selected = TRUE;
        }        

	mat_selected    = TRUE;
 	result_selected = FALSE;
        idx = 0;
	mat_qty    = MESH( analy ).material_qty;
	p_hide_qty = &MESH( analy ).mat_hide_qty;
        p_uc    = MESH( analy ).disable_material;
        p_uc2   = MESH( analy ).hide_material;
        p_elem  = NULL;
        p_elem2 = NULL;

       /* Look for element selection keyword */
       string_to_upper( tokens[1], tmp_token ); /* Make case insensitive */

       if( strcmp( tmp_token, "ELEM" ) == 0 )
       {
	   mat_selected = FALSE;
	   string_to_upper( tokens[2], tmp_token ); /* Make case insensitive */
	   idx++;
       }

	/* Result range selection */
        if( strcmp( tmp_token, "RESULT" ) == 0 )
        {
	    result_selected = TRUE;
	    string_to_upper( tokens[2], tmp_token ); /* Make case insensitive */
	    idx++;
        }

        if( strcmp( tokens[1], "SURF" ) == 0 )
        {
            mat_qty = MESH( analy ).surface_qty;
            p_uc    = MESH( analy ).hide_surface;
	    p_uc2   = MESH( analy ).disable_surface;
            idx++;
        }

        if ( strstr( "BRICK", tmp_token ) )
        {
	     string_to_upper( tokens[2], tmp_token );      /* Make case insensitive */
 
	     if ( result_selected )
	     {
	          MESH( analy ).hide_brick_by_result = TRUE;
	          MESH( analy ).brick_result_min = atof( tokens[3] );
	          MESH( analy ).brick_result_max = atof( tokens[4] );
	          idx+=3;
	     }

	     if (!mat_selected && !result_selected)
	     {
	         MESH( analy ).exclude_brick_by_mat  =FALSE;
	         MESH( analy ).hide_brick_by_result = FALSE;

		 /* Exclude bricks by elements */
		 elem_qty         =  MESH( analy ).brick_qty;
		 p_elem           =  MESH( analy ).hide_brick_elem;
		 p_hide_elem_qty  =  &MESH( analy ).hide_brick_elem_qty;
		 p_elem2          =  MESH( analy ).disable_brick_elem;
		 p_hide_elem_qty2 =  &MESH( analy ).disable_brick_elem_qty;
	     }

             idx++;
        }

        if ( strstr( "SHELL", tmp_token ) )
        {
	     string_to_upper( tokens[2], tmp_token );      /* Make case insensitive */
 
	     if ( result_selected )
	     {
	          MESH( analy ).hide_shell_by_result = TRUE;
	          MESH( analy ).shell_result_min = atof( tokens[3] );
	          MESH( analy ).shell_result_max = atof( tokens[4] );
	          idx+=3;
	     }

	     if (!mat_selected && !result_selected)
	     {
	         MESH( analy ).exclude_shell_by_mat = FALSE;
	         MESH( analy ).hide_shell_by_result = FALSE;

		 /* Exclude shells by elements */
		 elem_qty         =  MESH( analy ).shell_qty;
		 p_elem           =  MESH( analy ).hide_shell_elem;
		 p_hide_elem_qty  =  &MESH( analy ).hide_shell_elem_qty;
		 p_elem2          =  MESH( analy ).disable_shell_elem;
		 p_hide_elem_qty2 =  &MESH( analy ).disable_shell_elem_qty;
	     }

             idx++;
        }

        if ( strstr( "TRUSS", tmp_token ) )
        {
	     string_to_upper( tokens[2], tmp_token );      /* Make case insensitive */
 
	     if ( result_selected )
	     {
	          MESH( analy ).hide_truss_by_result = TRUE;
	          MESH( analy ).truss_result_min = atof( tokens[3] );
	          MESH( analy ).truss_result_max = atof( tokens[4] );
	          idx+=3;
	     }

	     if (!mat_selected && !result_selected)
	     {
	         MESH( analy ).exclude_truss_by_mat = FALSE;
	         MESH( analy ).hide_truss_by_result = FALSE;

		 /* Exclude trusss by elements */
		 elem_qty         =  MESH( analy ).truss_qty;
		 p_elem           =  MESH( analy ).hide_truss_elem;
		 p_hide_elem_qty  =  &MESH( analy ).hide_truss_elem_qty;
		 p_elem2          =  MESH( analy ).disable_truss_elem;
		 p_hide_elem_qty2 =  &MESH( analy ).disable_truss_elem_qty;
	     }

             idx++;
        }

        if ( strstr( "BEAM", tmp_token ) )
        {
	     string_to_upper( tokens[2], tmp_token );      /* Make case insensitive */
 
	     if ( result_selected )
	     {
	          MESH( analy ).hide_beam_by_result = TRUE;
	          MESH( analy ).beam_result_min = atof( tokens[3] );
	          MESH( analy ).beam_result_max = atof( tokens[4] );
	          idx+=3;
	     }

	     if (!mat_selected && !result_selected)
	     {
	         MESH( analy ).exclude_beam_by_mat = FALSE;
	         MESH( analy ).hide_beam_by_result = FALSE;

		 /* Exclude beams by elements */
		 elem_qty         =  MESH( analy ).beam_qty;
		 p_elem           =  MESH( analy ).hide_beam_elem;
		 p_hide_elem_qty  =  &MESH( analy ).hide_beam_elem_qty;
		 p_elem2          =  MESH( analy ).disable_beam_elem;
		 p_hide_elem_qty2 =  &MESH( analy ).disable_beam_elem_qty;
	     }

             idx++;
        }

        if ( strstr( tmp_token, "PART" ) )
        {
	      if ( result_selected )
	      {
	          MESH( analy ).hide_particle_by_result = TRUE;
	          MESH( analy ).particle_result_min = atof( tokens[3] );
	          MESH( analy ).particle_result_max = atof( tokens[4] );
	          idx+=3;
	      }
	      else MESH( analy ).hide_particle_by_result = FALSE;

              p_uc  = MESH( analy ).hide_particle;
              p_uc2 = MESH( analy ).disable_particle;
	      p_hide_qty = &MESH( analy ).particle_disable_qty;
	      elem_qty   = mat_qty;
	      idx++;
	      mat_selected = TRUE;
        } 

        if ( !result_selected)
	     process_mat_obj_selection ( analy,  tokens, idx, token_cnt, mat_qty,
					 elem_qty, 
					 p_elem, p_hide_qty, p_hide_elem_qty, 
					 p_uc, 
					 setval, include_selected, mat_selected );

        if ( !result_selected)
	     process_mat_obj_selection ( analy,  tokens, idx, token_cnt, mat_qty,
					 elem_qty, 
					 p_elem2, p_hide_qty, p_hide_elem_qty, 
					 p_uc2, 
					 setval, include_selected, mat_selected );

        reset_face_visibility( analy );
        if ( analy->dimension == 3 ) renorm = TRUE;
        analy->result_mod = TRUE;
        load_result( analy, TRUE, TRUE );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "mtl" ) == 0 ||
              strcmp( tokens[0], "matl") == 0 )
    {
        parse_mtl_cmd( analy, tokens, token_cnt, TRUE, &redraw, &renorm );
    }
    else if ( strcmp( tokens[0], "surf" ) == 0 )
    {
        parse_surf_cmd( analy, tokens, token_cnt, TRUE, &redraw, &renorm );
    }
    else if ( strcmp( tokens[0], "tmx" ) == 0 )
    {
        p_mesh = MESH_P( analy );
        p_mesh->translate_material = TRUE;
        sscanf( tokens[1], "%d", &ival );
        sscanf( tokens[2], "%f", &val );
        if ( ival > 0 && ival <= p_mesh->material_qty )
            p_mesh->mtl_trans[0][ival-1] = val;
        reset_face_visibility( analy );
        renorm = TRUE;
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "tmy" ) == 0 )
    {
        p_mesh = MESH_P( analy );
        p_mesh->translate_material = TRUE;
        sscanf( tokens[1], "%d", &ival );
        sscanf( tokens[2], "%f", &val );
        if ( ival > 0 && ival <= p_mesh->material_qty )
            p_mesh->mtl_trans[1][ival-1] = val;
        reset_face_visibility( analy );
        renorm = TRUE;
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "tmz" ) == 0 )
    {
        p_mesh = MESH_P( analy );
        p_mesh->translate_material = TRUE;
        sscanf( tokens[1], "%d", &ival );
        sscanf( tokens[2], "%f", &val );
        if ( ival > 0 && ival <= p_mesh->material_qty )
            p_mesh->mtl_trans[2][ival-1] = val;
        reset_face_visibility( analy );
        renorm = TRUE;
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "clrtm" ) == 0
          || strcmp( tokens[0], "clrem" ) == 0 )
    {
        p_mesh = MESH_P( analy );
        p_mesh->translate_material = FALSE;
        for ( i = 0; i < p_mesh->material_qty; i++ )
            for ( j = 0; j < 3; j++ )
                p_mesh->mtl_trans[j][i] = 0.0;
        reset_face_visibility( analy );
        renorm = TRUE;
        redraw = BINDING_MESH_VISUAL;
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
            MESH_P( analy )->translate_material = TRUE;
            explode_materials( token_cnt, tokens, analy, FALSE );
            reset_face_visibility( analy );
            renorm = TRUE;
            redraw = BINDING_MESH_VISUAL;
        }
    }
    else if ( strcmp( tokens[0], "emsc" ) == 0 )
    {
        if ( token_cnt < 2 )
            popup_dialog( USAGE_POPUP, "emsc [<name>...] <scale>" );
        else
        {
            MESH_P( analy )->translate_material = TRUE;
            explode_materials( token_cnt, tokens, analy, TRUE );
            reset_face_visibility( analy );
            renorm = TRUE;
            redraw = BINDING_MESH_VISUAL;
        }
    }
    else if ( strcmp( tokens[0], "tellem" ) == 0 
              || strcmp( tokens[0], "telem" ) == 0 )
    {
        char em_tokens[2][TOKENLENGTH] = 
        {
            "tell", "em"
        };

        parse_tell_command( analy, em_tokens, 2, TRUE, &redraw );
    }
    else if ( strcmp( tokens[0], "emrm" ) == 0 )
    {
        remove_exp_assoc( token_cnt, tokens );
    }
    else if ( strcmp( tokens[0], "rnf" ) == 0 )
    {
        adjust_near_far( analy );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "near" ) == 0 )
    {
        if ( token_cnt == 2 && is_numeric_token( tokens[1] ) )
        {
            sscanf( tokens[1], "%f", &val );
            set_near( val );
            set_mesh_view();
            redraw = BINDING_MESH_VISUAL;
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
            redraw = BINDING_MESH_VISUAL;
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
            redraw = BINDING_MESH_VISUAL;
        }
        else
        {
            if ( strcmp( tokens[0], "lookup" ) == 0 )
                popup_dialog( USAGE_POPUP, "lookup <vx> <vy> <vz>" );
            else
                popup_dialog( USAGE_POPUP, "%s <x> <y> <z>", tokens[0] );
        }
    }
    else if ( strcmp( tokens[0], "tfx" ) == 0 )
    {
        if ( token_cnt == 2 )
        {
            sscanf( tokens[1], "%f", &val );
            move_look_from( 0, val );
            adjust_near_far( analy );
            redraw = BINDING_MESH_VISUAL;
        }
        else
            popup_dialog( USAGE_POPUP, "tfx <value>" );
    }
    else if ( strcmp( tokens[0], "tfy" ) == 0 )
    {
        if ( token_cnt == 2 )
        {
            sscanf( tokens[1], "%f", &val );
            move_look_from( 1, val );
            adjust_near_far( analy );
            redraw = BINDING_MESH_VISUAL;
        }
        else
            popup_dialog( USAGE_POPUP, "tfy <value>" );
    }
    else if ( strcmp( tokens[0], "tfz" ) == 0 )
    {
        if ( token_cnt == 2 )
        {
            sscanf( tokens[1], "%f", &val );
            move_look_from( 2, val );
            adjust_near_far( analy );
            redraw = BINDING_MESH_VISUAL;
        }
        else
            popup_dialog( USAGE_POPUP, "tfz <value>" );
    }
    else if ( strcmp( tokens[0], "tax" ) == 0 )
    {
        if ( token_cnt == 2 )
        {
            sscanf( tokens[1], "%f", &val );
            move_look_at( 0, val );
            adjust_near_far( analy );
            redraw = BINDING_MESH_VISUAL;
        }
        else
            popup_dialog( USAGE_POPUP, "tax <value>" );
    }
    else if ( strcmp( tokens[0], "tay" ) == 0 )
    {
        if ( token_cnt == 2 )
        {
            sscanf( tokens[1], "%f", &val );
            move_look_at( 1, val );
            adjust_near_far( analy );
            redraw = BINDING_MESH_VISUAL;
        }
        else
            popup_dialog( USAGE_POPUP, "tay <value>" );
    }
    else if ( strcmp( tokens[0], "taz" ) == 0 )
    {
        if ( token_cnt == 2 )
        {
            sscanf( tokens[1], "%f", &val );
            move_look_at( 2, val );
            adjust_near_far( analy );
            redraw = BINDING_MESH_VISUAL;
        }
        else
            popup_dialog( USAGE_POPUP, "taz <value>" );
    }
    else if ( strcmp( tokens[0], "camang" ) == 0 )
    {
        if ( token_cnt == 2 )
        {
            val = (float)strtod( tokens[1], (char **)NULL );
            set_camera_angle( val, analy );
            redraw = BINDING_MESH_VISUAL;
        }
        else
            popup_dialog( USAGE_POPUP, "camang <angle>" );
    }
    else if ( strcmp( tokens[0], "bbsrc" ) == 0 )
    {
        MO_class_data **mo_classes;

        p_mesh = MESH_P( analy );

        if ( token_cnt == 1 )
            valid_command = FALSE;
        else if ( strcmp( tokens[1], "nodes" ) == 0 )
        {
            mo_classes = NEW( MO_class_data *, "Bbox source class" );
            *mo_classes = p_mesh->node_geom;
            free( analy->bbox_source );
            analy->bbox_source = mo_classes;
            analy->bbox_source_qty = 1;
            analy->vis_mat_bbox = FALSE;
        }
        else if ( strcmp( tokens[1], "elems" ) == 0 
                  || (rval = strcmp( tokens[1], "vis" )) == 0 )
        {
            mo_classes = NEW_N( MO_class_data *, p_mesh->elem_class_qty, 
                         "Bbox source classes" );
            ival = 0;

            for ( i = 0;
                  i < sizeof( elem_sclasses ) / sizeof( elem_sclasses[0] ); 
                  i++ )
            {
                p_lh = p_mesh->classes_by_sclass + elem_sclasses[i];

                if ( p_lh->qty != 0 )
                {
                    p_classes = (MO_class_data **) p_lh->list;
                    
                    for ( j = 0; j < p_lh->qty; j++ )
                    {
                        mo_classes[ival] = p_classes[j];
                        ival++;
                    }
                }
            }
            free( analy->bbox_source );
            analy->bbox_source = mo_classes;
            analy->bbox_source_qty = p_mesh->elem_class_qty;
            if ( rval == 0 )
                analy->vis_mat_bbox = TRUE;
        }
        else
        {
            mo_classes = NEW_N( MO_class_data *, token_cnt - 1, 
                                "Bbox source classes" );
            for ( i = 1; i < token_cnt; i++ )
            {
                rval = htable_search( MESH( analy ).class_table, tokens[1],
                                      FIND_ENTRY, &p_hte );
                if ( rval == OK )
                    mo_classes[i - 1] = (MO_class_data *) p_hte->data;
                else
                {
                    popup_dialog( INFO_POPUP,
                                  "Class \"%s\" not known; command aborted.",
                                  tokens[i] );
                    valid_command = FALSE;
                    free( mo_classes );
                    break;
                }
            }

            if ( valid_command && i > 1 )
            {
                free( analy->bbox_source );
                analy->bbox_source = mo_classes;
                analy->bbox_source_qty = token_cnt - 1;
                analy->vis_mat_bbox = FALSE;
            }
        }

        if ( !valid_command )
            popup_dialog( USAGE_POPUP, 
                "bbsrc { nodes | elems | <elem class name(s)> | vis }" );
        else
            parse_command( "bbox", analy );
    }
    else if ( strcmp( tokens[0], "bbox" ) == 0 )
    {
        ival = analy->dimension;
        if ( token_cnt == 1 )
        {
            bbox_nodes( analy, analy->bbox_source, FALSE, pt, vec );
            if ( analy->keep_max_bbox_extent )
            {
                for ( i = 0; i < ival; i++ )
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
            redraw = BINDING_MESH_VISUAL;
        }
        else if ( token_cnt == 2 * ival + 1 )
        {
            for ( i = 0; i < ival; i++ )
            {
                pt[i] = atof( tokens[i + 1] );
                vec[i] = atof( tokens[i + 1 + ival] );
            }
            if ( pt[0] < vec[0] && pt[1] < vec[1] 
                 && ( ival == 2 || ( pt[2] < vec[2] ) ) )
            {
                VEC_COPY( analy->bbox[0], pt );
                VEC_COPY( analy->bbox[1], vec );
                redraw = BINDING_MESH_VISUAL;
            }
        }

        if ( ival == 2 )
            analy->bbox[0][2] = analy->bbox[1][2] = 0.0;

        if ( !redraw )
            popup_dialog( USAGE_POPUP,
                       "bbox [<xmin> <ymin> [<zmin>] <xmax> <ymax> [<zmax>]]" ); 
        else
            set_view_to_bbox( analy->bbox[0], analy->bbox[1], ival );
    }
    else if ( strcmp( tokens[0], "plot" ) == 0 )
    {
      /* Delete the plot for this result if computing an EI result */
      if ( analy->ei_result && strlen(analy->ei_result_name)>0 )
      {
	   strcpy(tmp_token, "delth ");
	   strcat(tmp_token, analy->ei_result_name);
           parse_command( tmp_token, analy );
      }

      if ( check_for_result( analy, TRUE ) || analy->ei_result )
        {
           create_plot_objects( token_cnt, tokens, analy, &analy->current_plots ); 
           redraw = BINDING_PLOT_VISUAL;
        }
    }
    else if ( strcmp( tokens[0], "oplot" ) == 0 )
    {
        create_oper_plot_objects( token_cnt, tokens, analy, 
                                  &analy->current_plots ); 
        redraw = BINDING_PLOT_VISUAL;
    }
    else if ( strcmp( tokens[0], "timhis" ) == 0 )
    {
        parse_command( "plot", analy );
        valid_command = FALSE;
    }
    else if ( strcmp( tokens[0], "outth" ) == 0 
              || strcmp( tokens[0],  "thsave" ) == 0 )
    {
        if ( token_cnt > 1 )
            output_time_series( token_cnt, tokens, analy );
        else
            popup_dialog( USAGE_POPUP, "%s\n%s\n%s",
                          "outth <filename>",
                          "OR",
                          "outth <filename> [<result>...] [<mesh object>...]"
                          " [vs <result>]" );
    }
    else if ( strcmp( tokens[0], "delth" ) == 0 )
    {
        if ( token_cnt == 1 )
            popup_dialog( USAGE_POPUP, "%s\n%s\n%s",
                         "delth [<index> | <result> | <class short name>]...",
                         "OR",
                         "delth all" );
        else
        {
            delete_time_series( token_cnt, tokens, analy );
            redraw = redraw_for_render_mode( FALSE, RENDER_PLOT, analy );
        }
    }
    else if ( strcmp( tokens[0], "glyphqty" ) == 0 )
    {
        if ( token_cnt != 2 )
            popup_dialog( USAGE_POPUP, "%s\n%s\n%s", "glyphqty <qty>", 
                  "Where qty indicates desired number of glyphs per curve,",
                  "and qty < 1 turns off glyph rendering." );
        else
        {
            set_glyphs_per_plot( atoi( tokens[1] ), analy );
            redraw = redraw_for_render_mode( FALSE, RENDER_PLOT, analy );
        }
    }
    else if ( strcmp( tokens[0], "glyphscl" ) == 0 )
    {
        if ( token_cnt != 2 )
            popup_dialog( USAGE_POPUP, "%s\n%s", "glyphscl <scale>"
                  "Where <scale> (floating point) scales glyph size." );
        else
        {
            set_glyph_scale( atof( tokens[1] ) );
            redraw = redraw_for_render_mode( FALSE, RENDER_PLOT, analy );
        }
    }
    else if ( strcmp( tokens[0], "mmloc" ) == 0 )
    {
        if ( token_cnt != 2 
             || ( ( tokens[1][0] != 'u' && tokens[1][0] != 'l' )
                  || ( tokens[1][1] != 'l' && tokens[1][1] != 'r' ) ) )
            popup_dialog( USAGE_POPUP, "%s\n%s\n%s", 
                          "mmloc { ul | ll | lr | ur }",
                          "Where the first character dictates upper/lower and",
                          "the second character dictates left/right" );
        else
        {
            analy->minmax_loc[0] = tokens[1][0];
            analy->minmax_loc[1] = tokens[1][1];
            redraw = redraw_for_render_mode( FALSE, RENDER_PLOT, analy );
        }
    }
/*
    else if ( strcmp( tokens[0], "mth" ) == 0 )
    {
        parse_mth_command( token_cnt, tokens, analy );
    }
*/
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
    else if ( strcmp( tokens[0], "coordxf" ) == 0 )
    {
        setval = TRUE;

        if ( token_cnt == 10 )
        {
            for ( i = 1; i < 4; i++ )
                pt[i - 1] = atof( tokens[i] );
            for ( j = 0; j < 3; j++, i++ )
                pt2[j] = atof( tokens[i] );
            for ( j = 0; j < 3; j++, i++ )
                pt3[j] = atof( tokens[i] );
        }
        else if ( token_cnt == 4 )
        {
            for ( i = 0; i < 3; i++ )
                nodes[i] = atoi( tokens[i + 1] ) - 1;
            for ( i = 0; i < 3; i++ )
                pt[i] = analy->state_p->nodes.nodes3d[nodes[0]][i];
            for ( i = 0; i < 3; i++ )
                pt2[i] = analy->state_p->nodes.nodes3d[nodes[1]][i];
            for ( i = 0; i < 3; i++ )
                pt3[i] = analy->state_p->nodes.nodes3d[nodes[2]][i];
        }
        else
        {
            popup_dialog( USAGE_POPUP, "%s\n%s\n%s",
                          "coordxf <node 1> <node 2> <node 3>",
                          "OR",
                          "coordxf <x1> <y1> <z1> <x2> <y2> <z2> <x3> <y3> <z3>"
                        );
            setval = FALSE;
        }
        
        if ( setval )
        {
            if ( analy->tensor_transform_matrix == NULL )
                analy->tensor_transform_matrix = (float (*)[3]) NEW_N( float, 9,
                                                "Tensor xform matrix" );
            xfmat = analy->tensor_transform_matrix;

            /* "i" axis is direction from first to second point. */
            VEC_SUB( xfmat[0], pt2, pt );
            
            /* "j" axis is perpendicular to "i" and contains third point. */
            near_pt_on_line( pt3, pt, xfmat[0], pt4 );
            VEC_SUB( xfmat[1], pt3, pt4 );
            
            /* "k" axis is cross-product of "i" and "j". */
            VEC_CROSS( xfmat[2], xfmat[0], xfmat[1] );
            
            vec_norm( xfmat[0] );
            vec_norm( xfmat[1] );
            vec_norm( xfmat[2] );
            
            /* Want column vectors. */
            VEC_COPY( pt, xfmat[0] );
            VEC_COPY( pt2, xfmat[1] );
            VEC_COPY( pt3, xfmat[2] );
            for ( i = 0; i < 3; i++ )
            {
                xfmat[i][0] = pt[i];
                xfmat[i][1] = pt2[i];
                xfmat[i][2] = pt3[i];
            }

            if ( analy->do_tensor_transform )
            {
                analy->result_mod = TRUE;
                load_result( analy, TRUE, TRUE );
                redraw = redraw_for_render_mode( FALSE, RENDER_MESH_VIEW, 
                                                 analy );
            }
        }
            
    }
    else if ( strcmp( tokens[0], "dirv" ) == 0 )
    {
        if ( token_cnt != analy->dimension + 1 )
        {
            popup_dialog( USAGE_POPUP, "dirv <x> <y> <z (if 3D)>" );
            return;
        }
        
        for ( i = 0; i < analy->dimension; i++ )
            analy->dir_vec[i] = atof( tokens[i + 1] );
        
        if ( analy->dimension == 2 )
            analy->dir_vec[2] = 0.0;
        
        vec_norm( analy->dir_vec );
        
        if ( analy->cur_result != NULL
             && strcmp( analy->cur_result->name, "pvmag" ) == 0 )
        {
            analy->result_mod = TRUE;
            load_result( analy, TRUE, TRUE );
            redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
        }
    }
    else if ( strcmp( tokens[0], "dir3p" ) == 0 )
    {
        dim = analy->dimension;
        
        if ( dim == 2 )
        {
            popup_dialog( INFO_POPUP,
                          "\"dir3p\" not implemented for 2D data;\n%s",
                          "use \"dirv\" instead." );
            return;
        }

        if ( token_cnt != 10 )
        {
            popup_dialog( USAGE_POPUP, "dir3p <x1> <y1> <z1>  %s",
                          "<x2> <y2> <z2>  <x3> <y3> <z3>" );
            return;
        }
        
        for ( i = 0; i < dim; i++ )
            pt[i] = atof( tokens[i + 1] );
        for ( j = 0; j < dim; j++, i++ )
            pt2[j] = atof( tokens[i + 1] );
        for ( j = 0; j < dim; j++, i++ )
            vec[j] = atof( tokens[i + 1] );

        norm_three_pts( analy->dir_vec, pt, pt2, vec );
        
        if ( analy->cur_result != NULL
             && strcmp( analy->cur_result->name, "pvmag" ) == 0 )
        {
            analy->result_mod = TRUE;
            load_result( analy, TRUE, TRUE );
            redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
        }
    }    
    else if ( strcmp( tokens[0], "dir3n" ) == 0 )
    {
        dim = analy->dimension;
        
        if ( dim == 2 )
        {
            popup_dialog( INFO_POPUP,
                          "\"dir3n\" not implemented for 2D data;\n%s",
                          "use \"dirv\" instead." );
            return;
        }

        if ( token_cnt != 4 )
        {
            popup_dialog( USAGE_POPUP, "dir3n <node 1> <node 2> <node 3>  %s" );
            return;
        }
        
        for ( i = 0; i < 3; i++ )
            nodes[i] = atoi( tokens[i + 1] ) - 1;

        for ( i = 0; i < 3; i++ )
            pt[i] = analy->state_p->nodes.nodes3d[nodes[0]][i];
        for ( i = 0; i < 3; i++ )
            pt2[i] = analy->state_p->nodes.nodes3d[nodes[1]][i];
        for ( i = 0; i < 3; i++ )
            vec[i] = analy->state_p->nodes.nodes3d[nodes[2]][i];

        norm_three_pts( analy->dir_vec, pt, pt2, vec );
        
        if ( analy->cur_result != NULL
             && strcmp( analy->cur_result->name, "pvmag" ) == 0 )
        {
            analy->result_mod = TRUE;
            load_result( analy, TRUE, TRUE );
            redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
        }
    }
    else if ( strcmp( tokens[0], "vec" ) == 0 )
    {
        /* Get the result types from the translation table. */
        for ( i = 1, j = 0; i < token_cnt && j < analy->dimension; i++, j++ )
        {
            if ( is_numeric_token( tokens[i] ) )
            {
                if ( atof( tokens[i] ) == 0.0 )
                    analy->vector_result[j] = NULL;
            }
            else
            {
                if ( find_result( analy, ALL, TRUE, result_vector + j, 
                                  tokens[i] ) )
                {
                    /* Save reference. */
                    analy->vector_result[j] = result_vector + j;

                    /* If found result is a vector or array... */
                    if ( !result_vector[j].single_valued )
                    {
                        Bool_type did_it;
                        int idx = j;
                        Primal_result *p_pr = (Primal_result *) 
                                              result_vector[j].original_result;
                        Result *p_dest = result_vector + j;

                        /* ...use its components instead. */
                        did_it = gen_component_results( analy, p_pr, &idx, 
                                                        analy->dimension,
                                                        p_dest );
                        if ( !did_it )
                            popup_dialog( INFO_POPUP, "Unable to generate "
                                          "component results \nfrom non-scalar"
                                          " input result." );

                        /* Load extracted component result references. */
                        for ( ; j < idx; j++ )
                            analy->vector_result[j] = result_vector + j;
                    }
                }
                else
                    popup_dialog( WARNING_POPUP, 
                                  "Result \"%s\" unrecognized, set to zero.\n", 
                                  tokens[i] );
            }
        }

        /* Any unspecified are set to zero. */
        for ( ; j < analy->dimension; i++ )
            analy->vector_result[j] = NULL;
        
        found = intersect_srec_maps( &analy->vector_srec_map, 
                                     analy->vector_result, 3, analy );

        if ( found )
        {
            if ( analy->show_vectors )
                update_vec_points( analy );

/**/
            if ( analy->show_vectors /* || analy->show_carpet */ )
                redraw = redraw_for_render_mode( FALSE, RENDER_MESH_VIEW, 
                                                 analy );

            if ( analy->cur_result != NULL
                 && strcmp( analy->cur_result->name, "pvmag" ) == 0 )
            {
                analy->result_mod = TRUE;
                load_result( analy, TRUE, TRUE );
                redraw = redraw_for_render_mode( FALSE, RENDER_ANY, 
                                                 analy );
            }
        }
        else
            popup_dialog( WARNING_POPUP, "%s\n%s\n%s",
                          "This database does not support the requested",
                          "combination of vector components anywhere on",
                          "the mesh." );
    }
    else if ( strcmp( tokens[0], "veccm" ) == 0 )
    {
        analy->vec_col_set = FALSE;
        analy->vec_hd_col_set = FALSE;
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "vecscl" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &analy->vec_scale );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "vhdscl" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &analy->vec_head_scale );
        redraw = BINDING_MESH_VISUAL;
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
            analy->have_grid_points = TRUE;
        }
        redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
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
            analy->have_grid_points = TRUE;
        }
        redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
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
            analy->have_grid_points = TRUE;
        }
        redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
    }
    else if ( strcmp( tokens[0], "clrvgr" ) == 0 )
    {
        delete_vec_points( analy );
        redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
    }
#ifdef HAVE_VECTOR_CARPETS
    else if ( strcmp( tokens[0], "carpet" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &analy->vec_cell_size );
        if ( analy->show_carpet )
        {
            gen_carpet_points( analy );
            redraw = BINDING_MESH_VISUAL;
        }
    }
    else if ( strcmp( tokens[0], "regcar" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &analy->reg_cell_size );
        if ( analy->show_carpet )
        {
            gen_reg_carpet_points( analy );
            redraw = BINDING_MESH_VISUAL;
        }
    }
    else if ( strcmp( tokens[0], "vecjf" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &analy->vec_jitter_factor );
        if ( analy->show_carpet )
            redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "veclf" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &analy->vec_length_factor );
        if ( analy->show_carpet )
            redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "vecif" ) == 0 )
    {
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &analy->vec_import_factor );
        if ( analy->show_carpet )
            redraw = BINDING_MESH_VISUAL;
    }
#endif
    else if ( strcmp( tokens[0], "prake" ) == 0 )
    {
        if ( 3 == analy->dimension )
            prake( token_cnt, tokens, &ival, pt, vec, rgb, analy );
        else
            popup_dialog( INFO_POPUP, "Particle traces on 3D datasets only" );
    }
    else if ( strcmp( tokens[0], "clrpar" ) == 0 )
    {
        free_trace_points( analy );
        redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
    }
    else if ( strcmp( tokens[0], "ptrace" ) == 0 
              || strcmp( tokens[0], "aptrace" ) == 0 )
    {
        /* Start time. */
        if ( token_cnt > 1 )
            sscanf( tokens[1], "%f", &vec[0] );
        else
        {
            ival = 1;
            analy->db_query( analy->db_ident, QRY_STATE_TIME, &ival, NULL, 
                             &val );
            vec[0] = val;
        }

        /* End time. */
        if ( token_cnt > 2 )
            sscanf( tokens[2], "%f", &vec[1] );
        else
        {
            max_state = get_max_state( analy );
            ival = max_state + 1;
            analy->db_query( analy->db_ident, QRY_STATE_TIME, &ival, NULL, 
                             &val );
            vec[1] = val;
        }

        /* Time step size. */
        if ( token_cnt > 3 )
            sscanf( tokens[3], "%f", &val );
        else
            val = 0.0;
    
        /* "ptrace" deletes extant traces, leaving only new ones. */
        if ( tokens[0][0] == 'p' )
            free_trace_list( &analy->trace_pts );
        
        particle_trace( vec[0], vec[1], val, analy, FALSE );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "ptstat" ) == 0 )
    {
        if ( token_cnt < 4 )
            valid_command = FALSE;
        else
        {
            max_state = get_max_state( analy );

            switch( tokens[1][0] )
            {
                case 's':
                    ival = atoi( tokens[2] );
                    if ( ival < 1 || ival > max_state + 1 )
                        valid_command = FALSE;
                    else
                        analy->db_query( analy->db_ident, QRY_STATE_TIME, &ival,
                                         NULL, (void *) &vec[0] );
                    break;
                case 't':
                    val = atof( tokens[2] );
                    i_args[0] = 2;
                    i_args[1] = 1;
                    i_args[2] = max_state + 1;
                    analy->db_query( analy->db_ident, QRY_MULTIPLE_TIMES, 
                                     (void *) i_args, NULL, (void *) pt );
                    if ( val < pt[0] || val > pt[1] )
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
            redraw = BINDING_MESH_VISUAL;
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
            redraw = BINDING_MESH_VISUAL;
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
                redraw = BINDING_MESH_VISUAL;
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
            int mtl_qty = MESH( analy ).material_qty;

            /* Store material numbers for disabling particle traces. */
            analy->trace_disable = NEW_N( int, token_cnt - 1, "Trace disable" );
            for ( i = 1, j = 0; i < token_cnt; i++ )
            {
                ival = atoi( tokens[i] );
                if ( ival > 0 && ival <= mtl_qty )
                    analy->trace_disable[j++] = ival - 1;
                else
                    popup_dialog( INFO_POPUP, 
                                  "Invalid material \"%d\" ignored", ival );
            }
    
            if ( j == 0 )
            {
                free( analy->trace_disable );
                analy->trace_disable = NULL;
            }
            else
                analy->trace_disable_qty = j;
        }
    
        redraw = BINDING_MESH_VISUAL;
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
        if ( token_cnt == 2 )
        {
            if ( is_numeric_token( tokens[1] ) )
            {
                sscanf( tokens[1], "%d", &ival );
                ival--;
                analy->limit_max_state = TRUE;
                analy->db_query( analy->db_ident, QRY_QTY_STATES, NULL, NULL, 
                                 &j );
                j--;
                analy->max_state = MIN( ival, j );
                
                if ( analy->cur_state > analy->max_state )
                {
                    analy->cur_state = analy->max_state;
                    change_state( analy );
                    redraw = redraw_for_render_mode( FALSE, RENDER_ANY, 
                                                     analy );
                }
                
                if ( analy->render_mode == RENDER_PLOT )
                {
                    /* Update plots for new extreme. */
                    update_plot_objects( analy ); 
                    redraw = redraw_for_render_mode( FALSE, RENDER_ANY, 
                                                     analy );
                }
            }
            else if ( strcmp( tokens[1], "off" ) == 0 )
                analy->limit_max_state = FALSE;
            else
                valid_command = FALSE;
        }
        else
            valid_command = FALSE;
        
        if ( !valid_command )
            popup_dialog( USAGE_POPUP, "maxst 1 | 2 | 3 | ... | off" );
    }
    else if ( strcmp( tokens[0], "minst" ) == 0 )
    {
        if ( token_cnt == 2 )
        {
            if ( is_numeric_token( tokens[1] ) )
            {
                sscanf( tokens[1], "%d", &ival );
                ival--;
                analy->limit_min_state = TRUE;
                analy->min_state = MAX( ival, 0 );
                
                if ( analy->cur_state < analy->min_state )
                {
                    analy->cur_state = analy->min_state;
                    change_state( analy );
                    redraw = redraw_for_render_mode( FALSE, RENDER_ANY, 
                                                     analy );
                }
                
                if ( analy->render_mode == RENDER_PLOT )
                {
                    /* Update plots for new extreme. */
                    update_plot_objects( analy ); 
                    redraw = redraw_for_render_mode( FALSE, RENDER_ANY, 
                                                     analy );
                }
            }
            else if ( strcmp( tokens[1], "off" ) == 0 )
                analy->limit_min_state = FALSE;
            else
                valid_command = FALSE;
        }
        else
            valid_command = FALSE;
        
        if ( !valid_command )
            popup_dialog( USAGE_POPUP, "minst 1 | 2 | 3 | ... | off" );
    }
    else if ( strcmp( tokens[0], "refstate" ) == 0 )
    {
        if ( token_cnt == 1 )
        {
            popup_dialog( INFO_POPUP, "Current reference state is %d%s",
                          analy->reference_state,
                          analy->reference_state ? 
                          "." : " (initial position)." );
        }
        else
        {
            ival = atoi( tokens[1] );
            old = analy->reference_state;
            set_ref_state( analy, ival );
            redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
            analy->result_mod = analy->check_mod_required( analy, 
                                    REFERENCE_STATE, old, 
                                    analy->reference_state );
            if ( analy->result_mod )
            {
                if ( analy->render_mode == RENDER_MESH_VIEW )
                    load_result( analy, TRUE, TRUE );
                analy->display_mode_refresh( analy );
            }
        }
    }
    else if ( strcmp( tokens[0], "stride" ) == 0 )
    {
        if ( token_cnt != 2 )
            valid_command = FALSE;
        else
        {
            ival = atoi( tokens[1] );
            if ( ival >= 1 && ival <= get_max_state( analy ) )
            {
                put_step_stride( ival );
                update_util_panel( STRIDE_EDIT, NULL );
            }
            else
                valid_command = FALSE;
        }
        if ( !valid_command )
            popup_dialog( USAGE_POPUP, "stride <step size>\n  Where\n"
                          "    1 <= <step size> < maximum state" );
    }
    else if ( strcmp( tokens[0], "state" ) == 0 ||
              strcmp( tokens[0], "n" ) == 0 ||
              strcmp( tokens[0], "p" ) == 0 ||
              strcmp( tokens[0], "f" ) == 0 ||
              strcmp( tokens[0], "l" ) == 0 )
    {
        max_state = get_max_state( analy );
        min_state = GET_MIN_STATE( analy );

        if ( strcmp( tokens[0], "state" ) == 0 )
        {
            if ( token_cnt < 2 )
                popup_dialog( USAGE_POPUP, "state <state number>" );
            else
            {
                sscanf( tokens[1], "%d", &i );
                i--;
                if ( (i != analy->cur_state && i <= max_state && i >= min_state) )
                {
                    analy->cur_state = i;
                }
                else if ( i > max_state )
                    popup_dialog( INFO_POPUP, "Maximum state is %d", 
                                  max_state + 1 );
                else if ( i < min_state )
                    popup_dialog( INFO_POPUP, "Minimum state is %d", 
                                  min_state + 1 );
                else if ( i == analy->cur_state )
                    popup_dialog( INFO_POPUP, "Already at state %d", i + 1 );
            }
        }
        else if ( strcmp( tokens[0], "n" ) == 0 )
        {
            if ( analy->cur_state < max_state )
                analy->cur_state += 1;
            else
                popup_dialog( INFO_POPUP, "Already at max state..." );
        }
        else if ( strcmp( tokens[0], "p" ) == 0 )
        {
            ival = analy->cur_state + 1;
            analy->db_query( analy->db_ident, QRY_STATE_TIME, &ival, NULL, 
                             &val );
            if ( analy->state_p->time <= val )
                if ( analy->cur_state > min_state )
                    analy->cur_state -= 1;
                else
                    popup_dialog( INFO_POPUP, "Already at min state..." );
        }
        else if ( strcmp( tokens[0], "f" ) == 0 )
            analy->cur_state = min_state;
        else if ( strcmp( tokens[0], "l" ) == 0 )
            analy->cur_state = max_state;
 
        change_state( analy );
        redraw = BINDING_MESH_VISUAL;
    } 
    else if ( strcmp( tokens[0], "time" ) == 0 )
    {
        if ( token_cnt == 2 )
        {
            max_state = get_max_state( analy );
            i_args[0] = 2;
            i_args[1] = GET_MIN_STATE( analy ) + 1;
            i_args[2] = max_state + 1;
            analy->db_query( analy->db_ident, QRY_MULTIPLE_TIMES, 
                             (void *) i_args, NULL, (void *) pt );
            sscanf( tokens[1], "%f", &val );
            if ( val <= pt[0] )
            {
                analy->cur_state = i_args[1] - 1;
                change_state( analy );
            }
            else if ( val >= pt[1] )
            {
                analy->cur_state = max_state;
                change_state( analy );
            }
            else
                change_time( val, analy );

            redraw = BINDING_MESH_VISUAL;
        }
        else
            popup_dialog( USAGE_POPUP, "time <time value>" );
    }
    else if ( strcmp( tokens[0], "anim" ) == 0 )
    {
        redraw = BINDING_MESH_VISUAL;
        parse_animate_command( token_cnt, tokens, analy );
    }
    else if ( strcmp( tokens[0], "animc" ) == 0 )
    {
        redraw = BINDING_MESH_VISUAL;
        continue_animate( analy );
    }
    else if ( strcmp( tokens[0], "stopan" ) == 0 )
    {
        end_animate_workproc( TRUE );
    }
    else if ( strcmp( tokens[0], "cutpln" ) == 0 )
    {   
        if ( token_cnt < 7 )
            popup_dialog( USAGE_POPUP, 
                          "cutpln <pt x> <pt y> <pt z> %s",
                          "<norm x> <norm y> <norm z>" );
        else
        {
            Plane_obj *plane;

            plane = NEW( Plane_obj, "Cutting plane" );
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+1], "%f", &(plane->pt[i]) );
            for ( i = 0; i < 3; i++ )
                sscanf( tokens[i+4], "%f", &(plane->norm[i]) );
            vec_norm( plane->norm );
            APPEND( plane, analy->cut_planes );

            if ( analy->show_roughcut )
            {
                reset_face_visibility( analy );
                renorm = TRUE;
            }
            gen_cuts( analy );
#ifdef HAVE_VECTOR_CARPETS
            gen_carpet_points( analy );
#endif
            if ( analy->show_cut || analy->show_roughcut )
                redraw = BINDING_MESH_VISUAL;

            cutting_plane_defined = TRUE;
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

        for ( p_cl = analy->cut_poly_lists; p_cl != NULL; NEXT( p_cl ) )
        {
            Triangle_poly *tp_list = (Triangle_poly *) p_cl->list;
            DELETE_LIST( tp_list );
        }
        DELETE_LIST( analy->cut_poly_lists );
        
        if ( analy->show_cut || analy->show_roughcut )
            redraw = BINDING_MESH_VISUAL;

        cutting_plane_defined = FALSE;
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
#ifdef HAVE_VECTOR_CARPETS
        gen_carpet_points( analy );
#endif
        if ( analy->show_isosurfs || analy->show_contours )
            redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "isop" ) == 0 )
    {
        if ( token_cnt > 1 )
        {
            sscanf( tokens[1], "%f", &val );
            add_contour_val( val, analy );
            gen_contours( analy );
            gen_isosurfs( analy );
#ifdef HAVE_VECTOR_CARPETS
            gen_carpet_points( analy );
#endif
            if ( analy->show_isosurfs || analy->show_contours )
                redraw = BINDING_MESH_VISUAL;
        }
    }
    else if ( strcmp( tokens[0], "isov" ) == 0 )
    {
        if ( token_cnt > 1 )
        {
            sscanf( tokens[1], "%f", &val );
            if ( analy->cur_result != NULL )
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
#ifdef HAVE_VECTOR_CARPETS
            gen_carpet_points( analy );
#endif
            if ( analy->show_isosurfs || analy->show_contours )
                redraw = BINDING_MESH_VISUAL;
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
        if ( analy->show_isosurfs || analy->show_contours )
            redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "telliso" ) == 0 
              || strcmp( tokens[0], "teliso" ) == 0 )
    {
        char iso_tokens[2][TOKENLENGTH] = 
        {
            "tell", "iso"
        };

        parse_tell_command( analy, iso_tokens, 2, TRUE, &redraw );
    }
    else if ( strcmp( tokens[0], "conwid" ) == 0 )
    {
        val = atof( tokens[1] );
        if ( val <= 0.0 )
        {
            popup_dialog( INFO_POPUP, 
                          "Contour width must be greater than 0.0." );
            valid_command = FALSE;
        }
        else
        {
            analy->contour_width = val;
            if ( analy->show_contours )
                redraw = BINDING_MESH_VISUAL;
        }
    }
    else if ( strcmp( tokens[0], "sym" ) == 0 )
    {
        if ( token_cnt < 7 )
            popup_dialog( USAGE_POPUP, 
                "sym <pt-x> <pt-y> <pt-z> <norm-x> <norm-y> <norm-z>" );
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
            
            if ( analy->reflect )
                redraw = BINDING_MESH_VISUAL;
        }
    }
    else if ( strcmp( tokens[0], "clrsym" ) == 0 )
    {
        DELETE_LIST( analy->refl_planes );
        if ( analy->reflect )
            redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "inrgb" ) == 0 )
    {
        if ( token_cnt > 1 && token_cnt <= 3 )
        {
            setval = FALSE;

            if ( token_cnt == 3 )
                if ( strcmp( tokens[1], "bg" ) == 0 )
                {
                    setval = TRUE;
                    redraw = redraw_for_render_mode( FALSE, RENDER_ANY, 
                                                     analy );
                }

            rgb_to_screen( tokens[token_cnt - 1], setval, analy );
        }
        else
            popup_dialog( USAGE_POPUP, "inrgb [bg] <rgb filename>" );
    }
    else if( strcmp( tokens[0], "outrgb" ) == 0 )
    {
        if ( token_cnt > 1 )
	{
	    /* Hide all other Griz windows */
            pushpop_window( PUSHPOP_BELOW );
            screen_to_rgb( tokens[1], FALSE );
        } 
        else
            popup_dialog( USAGE_POPUP, "outrgb <filename>" );
    }
    else if( strcmp( tokens[0], "outrgba" ) == 0 )
    {
        if ( token_cnt > 1 )
	{
	    /* Hide all other Griz windows */
            pushpop_window( PUSHPOP_BELOW );
            screen_to_rgb( tokens[1], TRUE );
	}
        else
            popup_dialog( USAGE_POPUP, "outrgba <filename>" );
    }
    else if( strcmp( tokens[0], "outps" ) == 0 )
    {
        if ( token_cnt > 1 )
	{
	    /* Hide all other Griz windows */
            pushpop_window( PUSHPOP_BELOW );
            screen_to_ps( tokens[1] );
        }
        else
            popup_dialog( USAGE_POPUP, "outps <filename>" );
    }
     else if( strcmp( tokens[0], "ldhmap" ) == 0 )
    {
        popup_dialog( INFO_POPUP, "%s\n%s\n%s",
                      "HDF palettes are no longer supported.", 
                      "Convert HDF palettes with \"h2g\" utility,",
                      "then use \"ldmap\" instead." );
    }
    else if( strcmp( tokens[0], "ldmap" ) == 0 )
    {
        if ( token_cnt > 1 )
            read_text_colormap( tokens[1] );
        else
            popup_dialog( USAGE_POPUP, "ldmap <filename>" );
        redraw = BINDING_MESH_VISUAL;
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
            popup_dialog( USAGE_POPUP, "%s\n%s", 
                          "posmap <pos x> <pos y> <size x> <size y>",
                         "where <pos x> and <pos y> locate lower left corner" );
        redraw = BINDING_MESH_VISUAL;
    }
    else if( strcmp( tokens[0], "hotmap" ) == 0 )
    {
        hot_cold_colormap();
        redraw = BINDING_MESH_VISUAL;
    }
    else if( strcmp( tokens[0], "grmap" ) == 0 )
    {
        gray_colormap( FALSE );
        redraw = BINDING_MESH_VISUAL;
    }
    else if( strcmp( tokens[0], "igrmap" ) == 0 )
    {
        gray_colormap( TRUE );
        redraw = BINDING_MESH_VISUAL;
    }
    else if( strcmp( tokens[0], "invmap" ) == 0 )
    {
        invert_colormap();
        redraw = BINDING_MESH_VISUAL;
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

        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "setcol" ) == 0 )
    {
        if ( token_cnt == 5 || token_cnt == 3 )
        {
            float *rgb_src;
            
            if ( strcmp( tokens[2], "default" ) == 0 )
                rgb_src = NULL;
            else
            {
                sscanf( tokens[2], "%f", &rgb[0] );
                sscanf( tokens[3], "%f", &rgb[1] );
                sscanf( tokens[4], "%f", &rgb[2] );
                rgb_src = rgb;
            }

            if ( strncmp( tokens[1], "plot", 4 ) == 0 )
            {
                ival = atoi( tokens[1] + 4 );
                if ( ival > 0 )
                {
                    set_plot_color( ival, rgb_src );
                    redraw = redraw_for_render_mode( FALSE, RENDER_PLOT, 
                                                     analy );
                }
                else
                    valid_command = FALSE;
            }
            else
            {
                set_color( tokens[1], rgb_src );

                if ( strcmp( tokens[1], "vec" ) == 0 )
                    analy->vec_col_set = TRUE;
                else if ( strcmp( tokens[1], "vechd" ) == 0 )
                    analy->vec_hd_col_set = TRUE;
                else if ( strcmp( tokens[1], "rmin" ) == 0 
                      || strcmp( tokens[1], "rmax" ) == 0 )
                    set_cutoff_colors( TRUE, analy->mm_result_set[0], 
                                       analy->mm_result_set[1] );

                if ( strcmp( tokens[1], "bg" ) == 0
                     || strcmp( tokens[1], "fg" ) == 0 
                     || strcmp( tokens[1], "text" ) == 0 )
                    /* These options affect either view mode. */
                    redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
                else
                    /* The other options affect only mesh view mode. */
                    redraw = redraw_for_render_mode( FALSE, RENDER_MESH_VIEW, 
                                                     analy );
            }
        }
        else
            valid_command = FALSE;
        
        if ( !valid_command )
            popup_dialog( USAGE_POPUP, 
                          "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s", 
                          "setcol <which> <red> <green> <blue>",
                          "setcol <which> default\n", 
                          "Where",
                          "  <which> is one of the following:",
                          "      fg  bg  text  mesh  edges  con  hilite"
                          "  plot<n>  rmax  rmin  select  vec  vechd;\n",
                          "  <red> <green> and <blue> are floating point "
                          "numbers on [0, 1];\n",
                          "  \"default\" instead of a color triple returns "
                          "<which> color to its initial value;\n",
                          "  <n> (from <which> = \"plot<n>\") is an integer "
                          "indicating a time history curve", 
                          "      to set the color for (\"plot1\", \"plot2\", "
                          "etc.).  The integer is mod'ed with",
                          "      the quantity of colors stored "
                          "(currently 15) to generate the actual indexed ",
                          "      color entry to modify." );
     }
    else if ( strcmp( tokens[0], "savhis" ) == 0 )
    {
        if ( token_cnt > 1 )
            open_history_file( tokens[1] );
        else
            popup_dialog( USAGE_POPUP, "savhis <filename>" );

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
	{
	  strcpy( analy->hist_filename, tokens[1] );
	  analy->hist_line_cnt = 0;
	  read_history_file( tokens[1], 1, analy );
	}
        else
            popup_dialog( USAGE_POPUP, "%s\nOR\n%s", "rdhis <filename>",
                          "h <filename>" );
    }
    else if ( strcmp( tokens[0], "savhislog" ) == 0 )
    {
        if ( token_cnt > 1 )
	     history_log_command( tokens[1] );
        else
            popup_dialog( USAGE_POPUP, "savhislog <filename>" );

        /* Don't want to save _this_ command. */
        valid_command = FALSE;
    }
     else if ( strcmp( tokens[0], "loop" ) == 0 )
    {
#ifdef SERIAL_BATCH
        /*
         * Disable use of command "loop" to avoid "runaway processes"
         * during batch operations
         */

        popup_dialog( INFO_POPUP, "Command loop is disabled for batch execution." );
#else
        if ( token_cnt > 1 )
        {
	     while ( read_history_file( tokens[1], 1, analy ) );
        }
        else
            popup_dialog( USAGE_POPUP, "loop <filename>" );
#endif /* SERIAL_BATCH */
    }
    else if ( strcmp( tokens[0], "getedg" ) == 0 )
    {
        get_mesh_edges( analy->cur_mesh_id, analy );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "crease" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        crease_threshold = cos( (double)DEG_TO_RAD( 0.5 * val ) );
        explicit_threshold = cos( (double)DEG_TO_RAD( val ) );
        get_mesh_edges( analy->cur_mesh_id, analy );
        renorm = TRUE;
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "edgwid" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        if ( token_cnt == 2 && val > 0.0 )
        {
            analy->edge_width = val;
            redraw = redraw_for_render_mode( FALSE, RENDER_MESH_VIEW, analy );
        }
        else
            popup_dialog( USAGE_POPUP, "edgwid <line width>" );
    }
    else if ( strcmp( tokens[0], "edgbias" )  == 0 ||
	      strcmp( tokens[0], "eb" )       == 0 ||
	      strcmp( tokens[0], "edgebias" ) == 0 )
    {
        if ( token_cnt != 2 )
            popup_dialog( USAGE_POPUP, "edgbias <bias value>" );
        else
        {
            sscanf( tokens[1], "%f", &val );
            analy->edge_zbias = val;
            redraw = BINDING_MESH_VISUAL;
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
        redraw = BINDING_MESH_VISUAL;
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
        redraw = BINDING_MESH_VISUAL;
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
        redraw = BINDING_MESH_VISUAL;
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
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "globmm" ) == 0 )
    {
        get_global_minmax( analy );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "extreme_min" ) == 0 )
    {
        view_state = 0;
        if ( token_cnt > 1 )
        {
           sscanf( tokens[1], "%d", &view_state );
        }
        analy->result_mod = TRUE;
        get_extreme_minmax( analy, EXTREME_MIN, view_state);
        
        reset_face_visibility( analy );
        if ( analy->dimension == 3 ) renorm = TRUE;
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "extreme_max" ) == 0 )
    {
        view_state = 0;
        if ( token_cnt > 1 )
        {
           sscanf( tokens[1], "%d", &view_state );
        }
        analy->result_mod = TRUE;
        get_extreme_minmax( analy, EXTREME_MAX, view_state);
        
        reset_face_visibility( analy );
        if ( analy->dimension == 3 ) renorm = TRUE;
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "resetmm" ) == 0 )
    {
        redraw = reset_global_minmax( analy );
        if ( redraw != NO_VISUAL_CHANGE )
             load_result( analy, TRUE, TRUE );
    }
    else if ( strcmp( tokens[0], "tellmm" ) == 0 )
    {
        start_state        = GET_MIN_STATE( analy ) + 1;
        stop_state         = get_max_state( analy ) + 1;

        if ( token_cnt == 1 )
        {
            /*
             * Undocumented format:
             *
             * tellmm <no arguments> -- display minimums and maximums
             *                          for all states of current display result
             */
            result_variable[0] = '\0';
            tellmm( analy, result_variable, start_state, stop_state, 
                    &tellmm_redraw );
            redraw = tellmm_redraw;
        }
        else if ( token_cnt == 2 )
        {
            if ( strcmp( tokens[1], "mat" ) != 0 )
            {
                /*
                 * tellmm valid_result; NOTE:  result MAY NOT be materials
                 */
                analy->result_mod = TRUE;
                tellmm( analy, tokens[1], start_state, stop_state, 
                        &tellmm_redraw );
                redraw = tellmm_redraw;
            }
            else
            {
                /*
                 * tellmm invalid_result
                 */

                popup_dialog( USAGE_POPUP, tellmm_usage );
            }
        }
        else if ( token_cnt == 3 )
        {
            ival = (int)strtol( tokens[2], (char **)NULL, 10);

            if ( strcmp( tokens[1], "mat" ) != 0
                 && ival >= start_state )
            {
                /*
                 * tellmm valid_result state_number
                 *
                 * NOTE:  result MAY NOT be materials;
                 *        state_number != 0
                 */
                analy->result_mod = TRUE;
                tellmm( analy, tokens[1], ival, ival, &tellmm_redraw );
                redraw = tellmm_redraw;
            }
            else
            {
                /*
                 * tellmm invalid_result
                 */

                popup_dialog( USAGE_POPUP, tellmm_usage );
            }
        }
        else if ( token_cnt == 4 )
        {

            min_state = (int) strtol( tokens[2], (char **)NULL, 10 );
            max_state = (int) strtol( tokens[3], (char **)NULL, 10 );

            if ( strcmp( tokens[1], "mat" ) != 0
                 && min_state >= start_state 
                 && max_state <= stop_state )
            {
                /*
                 * tellmm result state_number state_number
                 *
                 * NOTE:  result MAY NOT be materials;
                 *        state_number != 0
                 */
                analy->result_mod = TRUE;
                tellmm( analy, tokens[1], min_state, max_state, 
                        &tellmm_redraw );
                redraw = tellmm_redraw;
            }
            else
            {
                /*
                 * tellmm invalid_result
                 */
                popup_dialog( USAGE_POPUP, tellmm_usage );
            }
        }
        else
        {
            popup_dialog( USAGE_POPUP, tellmm_usage );
        }
    }
    else if ( strcmp( tokens[0], "outmm" ) == 0 )
    {
        if ( token_cnt < 2 )
            popup_dialog( USAGE_POPUP, "outmm <file name> [<mat>...]" );
        else
            redraw = parse_outmm_command( analy, tokens, token_cnt );    
    }
    else if ( strcmp( tokens[0], "pause" ) == 0 )
    {

#ifdef SERIAL_BATCH

        /*
         * pausing is redundant in batch mode
         *
         * Pead specified "pause" duration token to properly
         * continue GRIZ processing
         */

        if ( token_cnt > 1 )
             sscanf( tokens[1], "%d", &ival );

#else
        if ( token_cnt > 1 )
	{
           sscanf( tokens[1], "%d", &ival );
           sleep( ival );
        }
        else
	{
           analy->hist_paused = TRUE;
	}
#endif
    }
    else if ( strcmp( tokens[0], "dellit" ) == 0 )
    {
        delete_lights();    
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "continue" ) == 0 || 
	      strcmp( tokens[0], "go" ) == 0       || 
	      strcmp( tokens[0], "resume" ) == 0 )
    {
      if ( analy->hist_line_cnt>1 && env.history_input_active )
	   read_history_file( analy->hist_filename, analy->hist_line_cnt, analy );
      analy->hist_paused = FALSE;    
    }
    else if ( strcmp( tokens[0], "light" ) == 0 )
    {
        if ( token_cnt >= 6 )
        {
            set_light( token_cnt, tokens );
            redraw = BINDING_MESH_VISUAL;
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
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "tly" ) == 0 )
    {
        sscanf( tokens[1], "%d", &ival );
        sscanf( tokens[2], "%f", &val );
        move_light( ival-1, 1, val );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "tlz" ) == 0 )
    {
        sscanf( tokens[1], "%d", &ival );
        sscanf( tokens[2], "%f", &val );
        move_light( ival-1, 2, val );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "mat" ) == 0 )
    {
        redraw = set_material( token_cnt, tokens, analy->max_mesh_mat_qty );
    }
    else if ( strcmp( tokens[0], "alias" ) == 0 )
    {
        create_alias( token_cnt, tokens );
    }
/*
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
            redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
        }
    }
*/
    else if ( strcmp( tokens[0], "title" ) == 0 )
    {
        strcpy( analy->title, tokens[1] );
        if ( analy->show_title )
            redraw = redraw_for_render_mode( FALSE, RENDER_ANY, analy );
    }
    else if ( strcmp( tokens[0], "echo" ) == 0 )
        wrt_text( "%s\n\n", tokens[1] );
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
        renorm = ( analy->dimension == 3 );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "dscalx" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        analy->displace_scale[0] = val;
        analy->displace_exag = TRUE;
        renorm = ( analy->dimension == 3 );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "dscaly" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        analy->displace_scale[1] = val;
        analy->displace_exag = TRUE;
        renorm = ( analy->dimension == 3 );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "dscalz" ) == 0 
              && analy->dimension == 3 )
    {
        sscanf( tokens[1], "%f", &val );
        analy->displace_scale[2] = val;
        analy->displace_exag = TRUE;
        renorm = TRUE;
        redraw = BINDING_MESH_VISUAL;
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
            popup_dialog( USAGE_POPUP, "inslp <filename>" );
    }
    else if ( strcmp( tokens[0], "inref" ) == 0 )
    {
        if ( token_cnt > 1 )
            read_ref_file( tokens[1], analy );
        else
            popup_dialog( USAGE_POPUP, "inref <filename>" );
    }
/*
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
*/
    else if ( strcmp( tokens[0], "clrref" ) == 0 )
    {
        DELETE_LIST( analy->ref_polys );
        if ( analy->show_ref_polys )
            redraw = BINDING_MESH_VISUAL;
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
        {
	    /* Hide all other Griz windows */
            pushpop_window( PUSHPOP_BELOW );

            write_hid_file( tokens[1], analy );

        /* Added: IRC - 10/12/04: Call the inline hidden line
         * function to generate the output PS file.
         * See SCR#:289
         */
        popup_dialog( INFO_POPUP,
                      "Generating hidden line output in file %s.ps",
                      tokens[1] );
        hidden_inline( tokens[1] );
        }
        else
            popup_dialog( USAGE_POPUP, "outhid <filename>" );
    }
#ifdef PNG_SUPPORT
    else if ( strcmp( tokens[0], "outpng" ) == 0 )
    {
        /* Hide all other Griz windows */
        pushpop_window( PUSHPOP_BELOW );

        if ( token_cnt > 1 )
            write_PNG_file( tokens[1], FALSE );
        else
            popup_dialog( USAGE_POPUP, "outpng <filename>" );
    }
    else if( strcmp( tokens[0], "outpnga" ) == 0 )
    {
        popup_dialog( WARNING_POPUP, " Alpha channel output not supported " );
        if ( token_cnt > 1 )
            /*
             *  Change the FALSE to TRUE when problem with Alpha Channel in PNG
             * files is fixed
             */
            write_PNG_file( tokens[1], FALSE );
        else
            popup_dialog( USAGE_POPUP, "outpnga <filename>" );
    }
#endif
#ifdef JPEG_SUPPORT
    else if ( strcmp( tokens[0], "outjpeg" ) == 0 || 
              strcmp( tokens[0], "outjpg"  ) == 0)
    {
        /* Hide all other Griz windows */
        pushpop_window( PUSHPOP_BELOW );
        if ( token_cnt > 1 )
            write_JPEG_file( tokens[1], FALSE, jpeg_quality );
        else
            popup_dialog( USAGE_POPUP, "outjpeg <filename>" );
    }
    else if ( strcmp( tokens[0], "jpegqual" ) == 0 ||
              strcmp( tokens[0], "jpgqual" ) == 0 )
    {
	  /* Hide all other Griz windows */
	  pushpop_window( PUSHPOP_BELOW );
	  if ( token_cnt > 1 )
               write_JPEG_file( tokens[1], FALSE, jpeg_quality );
	  else
               popup_dialog( USAGE_POPUP, "outjpeg <filename>" );

        if ( token_cnt > 1 )
	  jpeg_quality = strtod( tokens[1], (char **)NULL );
        else
	  popup_dialog( USAGE_POPUP, "jpegqual <value>" );
    }
#endif
/*
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
*/
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
    else if ( strcmp( tokens[0], "partrad" ) == 0 )
    {
        sscanf( tokens[1], "%f", &val );
        set_particle_radius( val );
        redraw = BINDING_MESH_VISUAL;
    }
    else if ( strcmp( tokens[0], "outvec" ) == 0 )
    {
        if ( token_cnt == 2 )
            outvec( tokens[1], analy );
        else
            popup_dialog( USAGE_POPUP, "outvec <filename>" );
    }
    else if ( strcmp( tokens[0], "autorgb" ) == 0 )
    {
        if ( token_cnt == 2 )
        {
            str_dup( &analy->img_root, tokens[1] );
            analy->autoimg_format = IMAGE_FORMAT_RGB;
            reset_autoimg_count();
        }
        else
            popup_dialog( USAGE_POPUP, 
                          "autorgb <root file name>" );
    }
    else if ( strcmp( tokens[0], "autoimg" ) == 0 )
    {
        if ( token_cnt == 2 )
        {
            str_dup( &analy->img_root, tokens[1] );
            analy->autoimg_format = IMAGE_FORMAT_RGB;
            reset_autoimg_count();
        }
        else if ( token_cnt == 3 && strcmp( tokens[2], "jpeg" ) == 0 )
        {
#ifdef JPEG_SUPPORT
            Bool_type no_jpeg = FALSE;
#else
            Bool_type no_jpeg = TRUE;
#endif

            if ( no_jpeg )
                popup_dialog( INFO_POPUP, "%s\n%s", 
                              "JPEG support is not compiled into this "
                              "executable;",
                              "using RGB format." );

            str_dup( &analy->img_root, tokens[1] );
            analy->autoimg_format = no_jpeg 
                                    ? IMAGE_FORMAT_RGB : IMAGE_FORMAT_JPEG;
            reset_autoimg_count();
        }
        else if ( token_cnt == 3 && strcmp( tokens[2], "png" ) == 0 )
        {
#ifdef PNG_SUPPORT
            Bool_type no_png = FALSE;
#else
            Bool_type no_png = TRUE;
#endif

            if ( no_png )
                popup_dialog( INFO_POPUP, "%s\n%s", 
                              "PNG support is not compiled into this "
                              "executable;",
                              "using RGB format." );

            str_dup( &analy->img_root, tokens[1] );
            analy->autoimg_format = no_png 
                                    ? IMAGE_FORMAT_RGB : IMAGE_FORMAT_PNG;
            reset_autoimg_count();
        }
        else
            popup_dialog( USAGE_POPUP, 
                          "autoimg <root file name> [jpeg | png]" );
    }
    else if ( strcmp( tokens[0], "resetimg" ) == 0 )
    {
        reset_autoimg_count();
    }
    else if ( strcmp( tokens[0], "numclass" ) == 0 )
    {
        MO_class_data **classarray = NULL;
        int classqty = 0;

        if ( token_cnt > 1 )
            str_dup( &analy->num_class, tokens[0] );   
        else
            popup_dialog( USAGE_POPUP, 
                          "numclass <nodal or elem class name>..." ); 

        for ( i = 1; i < token_cnt; i++ )
        {
            rval = htable_search( MESH( analy ).class_table, tokens[i],
                                  FIND_ENTRY, &p_hte );
            if ( rval == OK )
            {   
                classarray = RENEW_N( MO_class_data *, classarray, classqty, 1,
                                      "numbering class pointers" );
                classarray[classqty] = (MO_class_data *) p_hte->data;
                classqty++;
            }
            else if ( strcmp( tokens[i], "all" ) == 0 )
            {
                create_class_list( &classqty, &classarray, MESH_P( analy ), 7, 
                                   G_NODE, G_TRUSS, G_BEAM, G_TRI, G_QUAD, 
                                   G_TET, G_HEX );
            }
            else
                popup_dialog( INFO_POPUP,
                              "Invalid class name; command ignored.");
        }

        if ( classarray != NULL )
        {
            if ( analy->classarray != NULL )
                free( analy->classarray );
            analy->classarray = classarray;
            analy->classqty = classqty;
            redraw = BINDING_MESH_VISUAL;
        } 
    }
    else if ( strcmp( tokens[0], "outview" ) == 0 )
    {
        /* Hide all other Griz windows */
        pushpop_window( PUSHPOP_BELOW );

        if ( token_cnt == 2 )
        {
            if ( (fp_outview_file = fopen( tokens[1], "w" )) != NULL )
            {
                fprintf( fp_outview_file, "off refresh\n" );
                outview( fp_outview_file, analy );
                fprintf( fp_outview_file, "on refresh\n" );
                fclose( fp_outview_file );
            }
            else
                popup_dialog( WARNING_POPUP,
                             "Unable to open outview file:  %s\n", tokens[1] );
        }
        else
            popup_dialog( INFO_POPUP, "Filename needed for outview command." );
    }
    else if ( strcmp( tokens[0], "initval" ) == 0 )
    {
        if ( token_cnt == 2 )
        {
            val = atof( tokens[1] );
            analy->buffer_init_val = val;
            if ( analy->cur_result != NULL )
            {
                /* redraw = NONBINDING_MESH_VISUAL; */
                analy->result_mod = TRUE;
                redraw = reset_global_minmax( analy );
                if ( redraw != NO_VISUAL_CHANGE )
                    load_result( analy, TRUE, TRUE );
            }
        }
        else
            popup_dialog( USAGE_POPUP, "initval <value> \n" 
                          "(current value = %.2g)", analy->buffer_init_val );
    }
    else if ( strcmp( tokens[0], "surface" ) == 0 )
    {
        if ( TRUE == traction_data_table_initialized )
        {
            /*
             * "clean up" surface command
             */

            free( traction_data_table );
        }


        initialize_surface_parameter_table();

        if ( valid_state == parse_surface_command( tokens, token_cnt ) )
        {
            if ( auto_computing_state == surface_parameter_table.n )
            {
                /*
                 * perform autocomputing to establish "n"
                 */

                if ( invalid_state == auto_compute_n( analy ) )
                    return;
            }
            else
            {
                if ( NULL != (surface_target = surface_function_lookup( surface_parameter_table.type )) )
                {
                    surface_target();

                    wrt_text( "\n\n" );
                    wrt_text( "Number of points:  %d\n", qty_tdt );
                    wrt_text( "surface area:  %f\n", surface_area );
                    wrt_text( "p  ==>  %f, %f, %f\n",  surface_parameter_table.p.x,  surface_parameter_table.p.y,  surface_parameter_table.p.z );
                    wrt_text( "u0 ==>  %f, %f, %f\n", u0.x, u0.y, u0.z );
                    wrt_text( "u1 ==>  %f, %f, %f\n", u1.x, u1.y, u1.z );
                    wrt_text( "u2 ==>  %f, %f, %f\n", u2.x, u2.y, u2.z );
                }
                else
                    popup_dialog( USAGE_POPUP, "surface:  Invalid surface type" );
            }


            free( surface_parameter_table.type );
        }
        else
            popup_dialog( USAGE_POPUP, "surface {spot | ring | rect | tube | poly }" );
    }
    else if ( strcmp( tokens[0], "traction" ) == 0 )
    {
        if ( TRUE == surface_command_processed )
        {
            if ( valid_state == parse_traction_command( analy, tokens, token_cnt ) )
            {
                traction( analy, traction_material_list );

                free( traction_material_list );
            }
        }
        else
        {
            popup_dialog( WARNING_POPUP, "traction:  A surface command MUST be processed before using traction." );
            traction_data_table_initialized = FALSE;
        }
    }
    /* Null strings don't become tokens. */
    else
    {
        popup_dialog( INFO_POPUP, "Command \"%s\" not valid", tokens[0] );
        valid_command = FALSE;
    }

    if ( renorm )
        compute_normals( analy );

    switch ( redraw )
    {
        case BINDING_MESH_VISUAL:
            if ( analy->render_mode != RENDER_MESH_VIEW )
                manage_render_mode( RENDER_MESH_VIEW, analy, TRUE );
            /* Note no break, want to fall into next case. */
            
        case NONBINDING_MESH_VISUAL:
            if ( analy->render_mode == RENDER_MESH_VIEW )
            {
                if ( analy->center_view )
                    center_view( analy );
                if ( analy->refresh )
                        analy->update_display( analy );
            }
            break;
            
        case BINDING_PLOT_VISUAL:
            if ( analy->render_mode != RENDER_PLOT )
                manage_render_mode( RENDER_PLOT, analy, TRUE );
            /* Note no break, want to fall into next case. */
            
        case NONBINDING_PLOT_VISUAL:
            if ( analy->refresh && analy->render_mode == RENDER_PLOT )
            {
                    analy->update_display( analy );
            }
            break;
            
        default:
            /* case NO_VISUAL_CHANGE - do nothing. */
            ;
    }

    analy->result_mod = FALSE;
    
    if ( valid_command )
         history_command( buf );

    if ( strcmp( tokens[0], "r" ) != 0 )
         strcpy( last_command, buf );
}

 
/*****************************************************************
 * TAG( redraw_for_render_mode )
 * 
 * Return a redraw mode consistent with current render mode and
 * binding flag.  If parameter "exclusive" is not RENDER_ANY,
 * the current render mode must equal the value of "exclusive" or
 * NO_VISUAL_CHANGE is returned.
 */
extern Redraw_mode_type
redraw_for_render_mode( Bool_type binding, Render_mode_type exclusive,
                        Analysis *analy )
{
    switch ( analy->render_mode )
    {
        case RENDER_MESH_VIEW:
            if ( exclusive == RENDER_ANY
                 || exclusive == RENDER_MESH_VIEW ) 
            {
                if ( binding )
                    return BINDING_MESH_VISUAL;
                else
                    return NONBINDING_MESH_VISUAL;
            }
            else
                return NO_VISUAL_CHANGE;
                
        case RENDER_PLOT:
            if ( exclusive == RENDER_ANY
                 || exclusive == RENDER_PLOT ) 
            {
                if ( binding )
                    return BINDING_PLOT_VISUAL;
                else
                    return NONBINDING_PLOT_VISUAL;
            }
            else
                return NO_VISUAL_CHANGE;
            
        default:
            popup_dialog( WARNING_POPUP, 
                          "Un-implemented render mode in set redraw." );
    }
    
    return NO_VISUAL_CHANGE;
}
            
 
/*****************************************************************
 * TAG( manage_render_mode )
 * 
 * Take care of transitions between different render modes.
 */
void
manage_render_mode( Render_mode_type new_rmode, Analysis *analy,
                    Bool_type suppress )
{
    /* Yes, a hack, but we can remove it when we add multiple windows. */
    if ( suppress )
         suppress_display_updates( TRUE );

    switch ( new_rmode )
    {
        case RENDER_MESH_VIEW:
            if ( analy->render_mode != RENDER_MESH_VIEW )
            {
                analy->update_display = draw_mesh_view;
                analy->display_mode_refresh = update_vis;
                analy->check_mod_required = mod_required_mesh_mode;
#ifndef SERIAL_BATCH
                update_gui( analy, new_rmode, analy->render_mode );
#endif
                analy->render_mode = RENDER_MESH_VIEW;
            }
            break;
            
        case RENDER_PLOT:
            if ( analy->render_mode != RENDER_PLOT )
            {
                analy->update_display = draw_plots;
                analy->display_mode_refresh = update_plots;
                analy->check_mod_required = mod_required_plot_mode;
#ifndef SERIAL_BATCH
                update_gui( analy, new_rmode, analy->render_mode );
#endif
                analy->render_mode = RENDER_PLOT;
            }
            break;
            
        default:
            popup_dialog( WARNING_POPUP,
                          "Un-implemented render mode in render mode update." );
    }
    
    if ( suppress )
         suppress_display_updates( FALSE );
}

 
/*****************************************************************
 * TAG( parse_mtl_cmd )
 * 
 * Parse a material manager command.
 */
static void
parse_mtl_cmd( Analysis *analy, char tokens[][TOKENLENGTH], 
               int token_cnt, Bool_type src_mtl_mgr, Redraw_mode_type *p_redraw,
               Bool_type *p_renorm )
{
    int i, first_token, cnt, read, mtl_qty, qty_vals;
    static Bool_type preview_set = FALSE;
    static Bool_type incomplete_cmd = FALSE;
    Bool_type parsing;
    Redraw_mode_type redraw;
    static GLfloat *save_props[MTL_PROP_QTY];
    GLfloat *p_src, *p_dest, *bound;
    Material_property_type j;
    
    if ( token_cnt == 1 )
        return;
    
    mtl_qty = analy->max_mesh_mat_qty;
    
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
                redraw = BINDING_MESH_VISUAL;
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
            redraw = NO_VISUAL_CHANGE;
        }
        else if ( strcmp( tokens[i], "cancel" ) == 0 )
        {
            /* Replace current structures with saved copies. */
            for ( j = AMBIENT; j < MTL_PROP_QTY; j++ )
            {
                if ( save_props[j] != NULL )
                {
                    if ( j == AMBIENT )
                        p_dest = &v_win->mesh_materials.ambient[0][0];
                    else if ( j == DIFFUSE )
                        p_dest = &v_win->mesh_materials.diffuse[0][0];
                    else if ( j == SPECULAR )
                        p_dest = &v_win->mesh_materials.specular[0][0];
                    else if ( j == EMISSIVE )
                        p_dest = &v_win->mesh_materials.emission[0][0];
                    else if ( j == SHININESS )
                        p_dest = &v_win->mesh_materials.shininess[0];
            
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
            redraw = BINDING_MESH_VISUAL;
        }
        else if ( strcmp( tokens[i],  "continue" ) == 0 )
        {
            parsing = FALSE;
            incomplete_cmd = TRUE;
            redraw = NO_VISUAL_CHANGE;
        }
        else
        {
            read = parse_embedded_mtl_cmd( analy, tokens + i, cnt, 
                                           preview_set, save_props, 
                                           p_renorm );
            if ( read + first_token >= token_cnt )
            {
                parsing = FALSE;
                redraw = BINDING_MESH_VISUAL;
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
parse_embedded_mtl_cmd( Analysis *analy, char tokens[][TOKENLENGTH], int cnt, 
                        Bool_type preview, GLfloat *save[MTL_PROP_QTY], 
                        Bool_type *p_renorm )
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
    static int current_mtl_qty;
    int i, limit, mtl, mqty, k;
    Material_property_type j;
    unsigned char *disable_mtl, *hide_mtl;

    int mat_min, mat_max;
    
    i = 0;
    
    /* 
     * Finish up any partial segment.
     */
    if ( reading_materials )
    {
        /* Read material numbers until end of list or a word is encountered. */
        while ( i < cnt && (is_numeric_token( tokens[i] ) ||
                is_numeric_range_token( tokens[i] )) )
        {
          /* IRC - June 04, 2004: Added material range parsing */

          if ( is_numeric_range_token( tokens[i] ) )
          {
            parse_mtl_range(tokens[i], MAX_MATERIALS, &mat_min, &mat_max) ;

            mtl = mat_min ;
            for (mtl =  mat_min ;
                 mtl <= mat_max ;
                 mtl++)
              {
                props.materials[props.mtl_cnt++] = mtl ;
              }
          }
          else
            {
              mtl = (atoi ( tokens[i] ) - 1);
              props.materials[props.mtl_cnt++] = mtl;
            }
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
        if ( is_numeric_token( tokens[i] ) || 
             is_numeric_range_token( tokens[i] ))
        {
            /* Material numbers precede any other numeric tokens. */
            if ( props.mtl_cnt == 0 )
            {
                reading_materials = TRUE;
                
                if ( current_mtl_qty != analy->max_mesh_mat_qty 
                     && current_mtl_qty != 0 )
                {
                    free( props.materials );
                    props.materials = NULL;
                    current_mtl_qty = 0;
                }

                if ( props.materials == NULL )
                {
                    props.materials = NEW_N( int, analy->max_mesh_mat_qty,
                                             "Properties material array" );
                    current_mtl_qty = analy->max_mesh_mat_qty;
                }
            }

        
            if ( reading_materials )
            {
                /* mtl = (atoi ( tokens[i] ) - 1) % MAX_MATERIALS; */

              if ( is_numeric_range_token( tokens[i] ) )
              {
	  	  parse_mtl_range(tokens[i], MAX_MATERIALS, &mat_min, &mat_max ) ;
                  
                  mtl = mat_min ;
                  for (mtl =  mat_min ;
                       mtl <= mat_max ;
                       mtl++)
                    {
                      props.materials[props.mtl_cnt++] = mtl ;
                    }
              }
                  else
                    {
                      mtl = (atoi ( tokens[i] ) - 1);
                      props.materials[props.mtl_cnt++] = mtl;
                    }
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
            /* mqty = ( analy->num_materials < MAX_MATERIALS )
                   ? analy->num_materials : MAX_MATERIALS; */
            mqty = analy->max_mesh_mat_qty;
            if ( strcmp( tokens[i], "amb" ) == 0 )
            {
                if ( preview )
                copy_property( AMBIENT, &v_win->mesh_materials.ambient[0][0], 
                               &save[AMBIENT], mqty );
                update_previous_prop( AMBIENT, reading_cprop, &props );
                reading_cprop[AMBIENT] = TRUE;
            }
            else if ( strcmp( tokens[i], "diff" ) == 0 )
            {
                if ( preview )
                    copy_property( DIFFUSE, &v_win->mesh_materials.diffuse[0][0], 
                                   &save[DIFFUSE], mqty );
                update_previous_prop( DIFFUSE, reading_cprop, &props );
                reading_cprop[DIFFUSE] = TRUE;
            }
            else if ( strcmp( tokens[i], "spec" ) == 0 )
            {
                if ( preview )
                    copy_property( SPECULAR, &v_win->mesh_materials.specular[0][0], 
                                   &save[SPECULAR], mqty );
                update_previous_prop( SPECULAR, reading_cprop, &props );
                reading_cprop[SPECULAR] = TRUE;
            }
            else if ( strcmp( tokens[i], "emis" ) == 0 )
            {
                if ( preview )
                    copy_property( EMISSIVE, &v_win->mesh_materials.emission[0][0], 
                                   &save[EMISSIVE], mqty );
                update_previous_prop( EMISSIVE, reading_cprop, &props );
                reading_cprop[EMISSIVE] = TRUE;
            }
            else if ( strcmp( tokens[i], "shine" ) == 0 )
            {
                if ( preview )
                    copy_property( SHININESS, &v_win->mesh_materials.shininess[0], 
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
            define_color_properties( &v_win->mesh_materials,
                                     props.materials, props.mtl_cnt,
                                     material_colors, MATERIAL_COLOR_CNT );

            apply_defaults = FALSE;
        }
        else
        {
            mqty = MESH( analy ).material_qty;

            if ( visible || invisible )
            {
                hide_mtl = MESH( analy ).hide_material;

                if ( modify_all )
                {
                    for ( k = 0; k < mqty; k++ )
                    hide_mtl[k] = (unsigned char) invisible;
                    modify_all = FALSE;
                }
                else
                    for ( k = 0; k < props.mtl_cnt; k++ )
                        hide_mtl[props.materials[k]] = (unsigned char) invisible;

                reset_face_visibility( analy );
                if ( analy->dimension == 3 ) *p_renorm = TRUE;
                analy->result_mod = TRUE;
                visible = FALSE;
                invisible = FALSE;
            }
        
            if ( enable || disable )
            {
                disable_mtl = MESH( analy ).disable_material;

                if ( modify_all )
                {
                    for ( k = 0; k < mqty; k++ )
                        disable_mtl[k] = (unsigned char) disable;
                    modify_all = FALSE;
                }
                else
                    for ( k = 0; k < props.mtl_cnt; k++ )
                        disable_mtl[props.materials[k]] = (unsigned char) disable;

                analy->result_mod = TRUE;
                load_result( analy, TRUE, TRUE );
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
 * TAG( parse_mtl_range)
 * 
 * This function will parse a a material range tolken and return
 * the min material and max material. This function will also
 * parse a single vaue token.
 */

void
parse_mtl_range( char *token, int qty, int *mat_min, int *mat_max )
{
    int i=0;
    char temp_token[64]; 

    Bool_type range_found=FALSE;

    
    *mat_min = 0;
    *mat_max = 0;

    string_to_upper( token, temp_token ); /* Make case insensitive */

    /* First check for all materials */

    if ( strcmp(temp_token , "ALL" ) == 0 )
    {
         *mat_min = 0;
	 *mat_max = qty;
	 return;	
    }


    for ( i = 1;
          i < strlen(temp_token); 
          i++ )
    {
        if ( temp_token[i] == '-' || temp_token[i] == ':' )
        {
             range_found=TRUE;
             temp_token[i] = ' ';
        }
    }

    if ( range_found )
         sscanf(temp_token, "%d %d", mat_min, mat_max);
    else
    {
         /* Parse a single token value */
         sscanf(temp_token, "%d", mat_min);
         *mat_max = *mat_min;
    }        

}


/*****************************************************************
 * TAG( parse_embedded_surf_cmd )
 * 
 * Parse a subdivision of a surface manager command.  This may
 * be a typical "vis", "invis", etc. command, or a (new)
 * combination command ("invis disable...") or one with multiple
 * surfaces as operands.
 * 
 * This command is stateful so that multiple calls may be used
 * to complete one command.
 */
static int
parse_embedded_surf_cmd( Analysis *analy, char tokens[][TOKENLENGTH], int cnt, 
                         Bool_type *p_renorm )
{
    static Bool_type visible = FALSE;
    static Bool_type invisible = FALSE;
    static Bool_type enable = FALSE;
    static Bool_type disable = FALSE;
    static Bool_type modify_all = FALSE;
    static Bool_type apply_defaults = FALSE;
    static Bool_type reading_surfaces = FALSE;
    static Surface_property_obj props;
    static int current_surf_qty;
    int i, limit, surf, sqty, k;
    unsigned char *disable_surf, *hide_surf;
    
    i = 0;
    
    /* 
     * Finish up any partial segment.
     */
    if ( reading_surfaces )
    {
        /* Read surface numbers until end of list or a word is encountered. */
        while ( i < cnt && is_numeric_token( tokens[i] ) )
        {
            surf = (atoi ( tokens[i] ) - 1);
            props.surfaces[props.surf_cnt++] = surf;
            i++;
        }
    
        /*
         * If we ended reading surfaces for any reason other than
         * encountering the "continue" token, we're done reading surfaces.
         * We could be done even if "continue" occurs, but that can't be
         * known until the next call.
         */
        if ( i == cnt || strcmp( tokens[i], "continue" ) != 0 )
            reading_surfaces = FALSE;
        else
            return i;
    }
    
    /* Parse (rest of) tokens. */
    for ( ; i < cnt; i++ )
    {
        if ( is_numeric_token( tokens[i] ) )
        {
            /* Material numbers precede any other numeric tokens. */
            if ( props.surf_cnt == 0 )
            {
                reading_surfaces = TRUE;

                if ( current_surf_qty != analy->max_mesh_surf_qty
                     && current_surf_qty != 0 )
                {
                    free( props.surfaces );
                    props.surfaces = NULL;
                    current_surf_qty = 0;
                }

                if ( props.surfaces == NULL )
                {
                    props.surfaces = NEW_N( int, analy->max_mesh_surf_qty,
                                             "Properties surface array" );
                    current_surf_qty = analy->max_mesh_surf_qty;
                }
            }

            /* Surface numbers precede any other numeric tokens. */
            if ( reading_surfaces )
            {
                surf = (atoi ( tokens[i] ) - 1);
                props.surfaces[props.surf_cnt++] = surf;
            }
            else
                popup_dialog( WARNING_POPUP, "Bad Surface Mgr numeric token parse." );
        }
        else if ( strcmp( tokens[i], "continue" ) == 0 )
            /* Kick out before incrementing i again. */
            break;
        else if ( strcmp( tokens[i], "vis" ) == 0 )
            visible = TRUE;
        else if ( strcmp( tokens[i], "invis" ) == 0 )
            invisible = TRUE;
        else if ( strcmp( tokens[i], "enable" ) == 0 )
            enable = TRUE;
        else if ( strcmp( tokens[i], "disable" ) == 0 )
            disable = TRUE;
        else if ( strcmp( tokens[i], "default" ) == 0 )
            apply_defaults = TRUE;
        else if ( strcmp( tokens[i], "all" ) == 0 )
        {
            if ( invisible || visible || enable || disable )
                modify_all = TRUE;
        }
        else
            popup_dialog( WARNING_POPUP, "Bad Surface Mgr parse." );
    }
    
    if ( i >= cnt )
    {
        /* 
         * Didn't encounter "continue", so clean-up and reset
         * static variables for next new command.
         */
        sqty = MESH( analy ).surface_qty;

        if ( visible || invisible )
        {
            hide_surf = MESH( analy ).hide_surface;

            if ( modify_all )
            {
                for ( k = 0; k < sqty; k++ )
                hide_surf[k] = (unsigned char) invisible;
                modify_all = FALSE;
            }
            else
                for ( k = 0; k < props.surf_cnt; k++ )
                    hide_surf[props.surfaces[k]] = (unsigned char) invisible;

            reset_face_visibility( analy );
            if ( analy->dimension == 3 ) *p_renorm = TRUE;
            analy->result_mod = TRUE;
            visible = FALSE;
            invisible = FALSE;
        }
    
        if ( enable || disable )
        {
            disable_surf = MESH( analy ).disable_surface;

            if ( modify_all )
            {
                for ( k = 0; k < sqty; k++ )
                    disable_surf[k] = (unsigned char) disable;
                modify_all = FALSE;
            }
            else
                for ( k = 0; k < props.surf_cnt; k++ )
                    disable_surf[props.surfaces[k]] = (unsigned char) disable;

            analy->result_mod = TRUE;
            load_result( analy, TRUE, TRUE );
            enable = FALSE;
            disable = FALSE;
        }
        reading_surfaces = FALSE;
        props.surf_cnt = 0;
    }
    
    return i;
}
 
/*****************************************************************
 * TAG( update_previous_prop )
 * 
 * Check for a property specified for update and do so if found.
 */
static void
update_previous_prop( Material_property_type cur_property, 
                      Bool_type update[MTL_PROP_QTY], 
                      Material_property_obj *props )
{

    /*** Verify use of v_win->mesh_materials.XXX  LAS ***/


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
        p_dest = v_win->mesh_materials.ambient;
    else if ( j == DIFFUSE )
        p_dest = v_win->mesh_materials.diffuse;
    else if ( j == SPECULAR )
        p_dest = v_win->mesh_materials.specular;
    else if ( j == EMISSIVE )
        p_dest = v_win->mesh_materials.emission;

    /* Update material property array. */
    for ( k = 0; k < props->mtl_cnt; k++ )
    {
        mtl = props->materials[k];
    
        if ( j == SHININESS )
            v_win->mesh_materials.shininess[mtl] = props->color_props[0];
        else
        {
            p_dest[mtl][0] = props->color_props[0];
            p_dest[mtl][1] = props->color_props[1];
            p_dest[mtl][2] = props->color_props[2];
            p_dest[mtl][3] = 1.0;
        }
    
        /*** ORIGINAL:

        if ( mtl == v_win->current_material )
            update_current_material( j );

        REPLACEMENT:  LAS ***/

        if ( mtl == v_win->mesh_materials.current_index )
            update_current_color_property( &v_win->mesh_materials, j );
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
copy_property( Material_property_type property, GLfloat *source, 
               GLfloat **dest, int mqty )
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
 * TAG( parse_surf_cmd )
 * 
 * Parse a surface manager command.
 */
static void
parse_surf_cmd( Analysis *analy, char tokens[][TOKENLENGTH], 
               int token_cnt, Bool_type src_surf_mgr, Redraw_mode_type *p_redraw,
               Bool_type *p_renorm )
{
    int i, first_token, cnt, read, surf_qty, qty_vals;
    static Bool_type incomplete_cmd = FALSE;
    Bool_type parsing;
    Redraw_mode_type redraw;
    GLfloat *p_src, *p_dest, *bound;
    
    if ( token_cnt == 1 )
        return;
    
    surf_qty = analy->max_mesh_surf_qty;
    
    if ( src_surf_mgr )
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
            read = parse_embedded_surf_cmd( analy, tokens + i, cnt, 
                                            p_renorm );
            incomplete_cmd = FALSE;
            if ( read + first_token >= token_cnt )
            {
                parsing = FALSE;
                redraw = BINDING_MESH_VISUAL;
            }
            else
                i += read;
        }
        else if ( strcmp( tokens[i], "apply" ) == 0 )
        {
            parsing = FALSE;
            redraw = NO_VISUAL_CHANGE;
        }
        else if ( strcmp( tokens[i], "cancel" ) == 0 )
        {
            parsing = FALSE;
            redraw = BINDING_MESH_VISUAL;
        }
        else if ( strcmp( tokens[i],  "continue" ) == 0 )
        {
            parsing = FALSE;
            incomplete_cmd = TRUE;
            redraw = NO_VISUAL_CHANGE;
        }
        else
        {
            read = parse_embedded_surf_cmd( analy, tokens + i, cnt, 
                                            p_renorm );
            if ( read + first_token >= token_cnt )
            {
                parsing = FALSE;
                redraw = BINDING_MESH_VISUAL;
            }
            else
                i += read;
        }
    }
    
    *p_redraw = redraw;
}
 
 
/*****************************************************************
 * TAG( parse_vcent )
 * 
 * Parse "vcent" (view centering) command.
 *
 * Note: assumes a node belongs to the required node class.
 */
static void
parse_vcent( Analysis *analy, char tokens[MAXTOKENS][TOKENLENGTH], 
             int token_cnt, Redraw_mode_type *p_redraw )
{
    int ival;

    if ( token_cnt > 1 && strcmp( tokens[1], "off" ) == 0 )
    {
        analy->center_view = NO_CENTER;
        *p_redraw = NO_VISUAL_CHANGE;
    }
    else if ( token_cnt > 1 && strcmp( tokens[1], "hi" ) == 0 )
    {
        analy->center_view = HILITE;
        center_view( analy );
        *p_redraw = BINDING_MESH_VISUAL;
    }
    else if ( token_cnt > 1 
              && ( strcmp( tokens[1], "n" ) == 0 
                   || strcmp( tokens[1], "node" ) == 0 ) )
    {
        ival = atoi( tokens[2] );
        if ( ival < 1 || ival > MESH( analy ).node_geom->qty )
            popup_dialog( INFO_POPUP, 
                          "Invalid node specified for view center" );
        else
        {
            analy->center_view = NODE;
            analy->center_node = ival - 1;
            center_view( analy );
            *p_redraw = BINDING_MESH_VISUAL;
        }
    }
    else if ( token_cnt == 4 )
    {
        analy->view_center[0] = atof( tokens[1] );
        analy->view_center[1] = atof( tokens[2] );
        analy->view_center[2] = atof( tokens[3] );
        analy->center_view = POINT;
        center_view( analy );
        *p_redraw = BINDING_MESH_VISUAL;
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
check_visualizing( Analysis *analy )
{
    /*
     * This logic needs to save the render_mode when transitioning
     * to a "show_cut", "show_isosurfs",... mode so that the
     * correct render_mode can be reinstated when transitioning
     * back.  Someday...
     */
    if ( analy->show_cut || analy->show_isosurfs ||
         analy->show_vectors )
/*         analy->show_vectors || analy->show_carpet ) */
    {
        analy->show_edges = TRUE;
        analy->mesh_view_mode = RENDER_NONE;
    }
    else
    {
        analy->show_edges = FALSE;
        analy->mesh_view_mode = RENDER_FILLED;
        update_util_panel( VIEW_SOLID, NULL );
    }
    
    update_util_panel( VIEW_EDGES, NULL );
} 

 
/*****************************************************************
 * TAG( update_vis )
 * 
 * Update the fine cutting plane, contours, and isosurfaces.
 * This routine is called when the state changes and when
 * the displayed result changes.
 */
void
update_vis( Analysis *analy )
{
#ifdef SERIAL_BATCH
#else
    set_alt_cursor( CURSOR_WATCH );
#endif
    
    gen_cuts( analy );
    gen_isosurfs( analy );
    update_vec_points( analy );
    gen_contours( analy );

#ifdef SERIAL_BATCH
#else
    unset_alt_cursor();
#endif
}


/*****************************************************************
 * TAG( create_alias )
 *
 * Create a command alias.
 */
static void
create_alias( int token_cnt, char tokens[MAXTOKENS][TOKENLENGTH] )
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
alias_substitute( char tokens[MAXTOKENS][TOKENLENGTH], int *token_cnt )
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
create_reflect_mats( Refl_plane_obj *plane )
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
Bool_type
is_numeric_token( char *num_string )
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
 * TAG( is_numeric_range_token )
 * 
 * Evaluate a string and return TRUE if string is an ASCII
 * representation of an integer range of the form 99-99.
 */
Bool_type
is_numeric_range_token( char *num_string )
{
    char one_char;
    char num_one[10], num_two[10];
    char temp_string[32];

    int  i;
    int  num_min,     num_max;
    int  pt_cnt;
    Bool_type         dash_found = FALSE;

    pt_cnt = 0;
    strcpy(temp_string, num_string);
    
    /*
     * Loop through the characters in the string, looking for the '-'
     * that seperates the two numbers, then parse the min mat and
     * max mat.
     */
    for ( i = 1;
          i < strlen(temp_string); 
          i++ )
    {
        if ( temp_string[i] == '-' || temp_string[i] == ':' )
        { 
          dash_found    = TRUE;
          temp_string[i] = ' ';
        }
    }
    
    if (dash_found)
    {
      sscanf(temp_string, "%s %s", num_one, num_two);
      if (is_numeric_token(num_one) && is_numeric_token(num_two))
        {
          sscanf(temp_string, "%d %d", &num_min, &num_max);
          if (num_min < num_max)
             return TRUE;
        } 
    }
    return FALSE;
}


/*****************************************************************
 * TAG( get_prake_data )
 * 
 * Assign prake data items from input tokens
 *
 */
static int
get_prake_data ( int token_count, char prake_tokens[][TOKENLENGTH], 
                 int *ptr_ival, float pt[], float vec[], float rgb[] )
{
    int count, i;

    /* */

    if ( ((count = sscanf( prake_tokens[1], "%d", ptr_ival )) == EOF ) ||
         (1 != count) )
        popup_dialog( INFO_POPUP, 
                      "get_prake_data:  Invalid data:  n:  %d", ptr_ival );

    for ( i = 0; i < 3; i++ )
    {
        if ( ((count = sscanf( prake_tokens[i + 2], "%f", &pt[i] )) == EOF ) ||
              (1 != count) )
            popup_dialog( INFO_POPUP, 
                          "get_prake_data:  Invalid data:  pt:  %f", pt[i] );
    }

    for ( i = 0; i < 3; i++ )
    {
        if ( ((count = sscanf( prake_tokens[i + 5], "%f", &vec[i] )) == EOF ) ||
              (1 != count) )
            popup_dialog( INFO_POPUP, 
                          "get_prake_data:  Invalid data:  vec:  %f", vec[i] );
    }

    if ( token_count >= 11 )
    {   
        for ( i = 0; i < 3; i++ )
        {
            if ( ((count = sscanf( prake_tokens[i+8], "%f", &rgb[i] )) == EOF )
               || (1 != count) )
                popup_dialog( INFO_POPUP, 
                              "get_prake_data:  Invalid data:  rgb:  %f", 
                              rgb[i] );
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
/**static**/ int
tokenize( char *ptr_input_string, char token_list[][TOKENLENGTH], 
          size_t maxtoken )
{
    int qty_tokens;
    char *ptr_current_token, *ptr_NULL;
    static char token_separators[] = "\t\n ,;";

    /* */

    ptr_NULL = "";

    if ( (ptr_input_string == NULL) || !maxtoken )
        return ( 0 );

    ptr_current_token = strtok( ptr_input_string, token_separators );

    for ( qty_tokens = 0; (size_t) qty_tokens < maxtoken 
                          && ptr_current_token != NULL; )
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
prake( int token_cnt, char tokens[MAXTOKENS][TOKENLENGTH], int *ptr_ival, 
       float pt[], float vec[], float rgb[], Analysis *analy )
{
    FILE *prake_file;
    char  buffer [TOKENLENGTH];
    char prake_tokens [MAXTOKENS][TOKENLENGTH];
    int i, prake_token_cnt;
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
                         strstr( prake_tokens[0], "#" ) == NULL )
                    { 
                        if ( prake_token_cnt >= 7 )
                        {
                            for ( i = prake_token_cnt; i > -1; i-- )
                                strcpy( prake_tokens[i + 1], prake_tokens[i] );
                            strcpy( prake_tokens[0], "prake" );
                            prake_token_cnt++;
                            if ( get_prake_data( prake_token_cnt, prake_tokens,
                                                 ptr_ival, pt, vec, rgb ) > 0 )
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
                popup_dialog( INFO_POPUP, 
                              "prake:  Unable to open file:  %s", tokens[1] );
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
                if ( get_prake_data( prake_token_cnt, prake_tokens, ptr_ival, 
                                     pt, vec, rgb ) > 0 )
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
outvec( char outvec_file_name[TOKENLENGTH], Analysis *analy )
{
    FILE *outvec_file;

    /*
     * Test to determine if vgrid"N" and vector qualifiers have 
     * been established.
     */
    if ( !analy->vectors_at_nodes && !analy->have_grid_points )
    {
        popup_dialog( INFO_POPUP, "outvec: No vector points exist." );
        return;
    }

    if ( (analy->vector_result[0] == NULL)  &&
         (analy->vector_result[1] == NULL)  &&
         (analy->vector_result[2] == NULL))
    {
        popup_dialog( INFO_POPUP, "No vector result has been set.\n%s"
                      "Use  \"vec <vx> <vy> <vz>\"." );

        return;
    }

    if ( (outvec_file = fopen( (char *) outvec_file_name, "w" )) != NULL )
    {
        write_preamble( outvec_file );
        write_outvec_data( outvec_file, analy );
        fclose( outvec_file );
    }
    else
        popup_dialog( INFO_POPUP, "outvec:  Unable to open file:  %s", 
                      outvec_file_name );

    return;
}


/*****************************************************************
 * TAG( write_preamble )
 * 
 * Write information preamble to output data file.
 */
void
write_preamble( FILE *ofile )
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
write_outvec_data( FILE *outvec_file, Analysis *analy )
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
       ,width_column_1
       ,width_column_2
       ,width_column_3
       ,width_column_4
       ,width_column_5
       ,width_column_6;
    int qty_of_nodes;
    int i, j;
    float *vec_data[3];
    Result *tmp_result;
    float *tmp_data;
    GVec3D *nodes3d;
    GVec2D *nodes2d;
    char *titles[3];
    int class_qty;
    MO_class_data **p_mo_classes;
    MO_class_data *p_node_class;
    
    for ( i = 0; i < 3; i++ )
    {
        if ( analy->vector_result[i] != NULL )
            griz_str_dup( titles + i, analy->vector_result[i]->title );
        else
            griz_str_dup( titles + i, "Zero" );
    }

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
     
    p_node_class = MESH_P( analy )->node_geom;   
    qty_of_nodes = p_node_class->qty;

    if ( 3 == analy->dimension )
    {
        length_z_label = strlen ( z_label );
        width_column_3 = length_space +
                         ( ( number_width > length_z_label ) ?
                             number_width : length_z_label );
                 
        length_vector_x_label = strlen ( titles[0] );
        width_column_4 = length_space +
                         ( ( number_width > length_vector_x_label ) ?
                             number_width : length_vector_x_label );
                 
        length_vector_y_label = strlen ( titles[1] );
        width_column_5 = length_space +
                         ( ( number_width > length_vector_y_label ) ?
                             number_width : length_vector_y_label );
                 
        length_vector_z_label = strlen ( titles[2] );
        width_column_6 = length_space +
                         ( ( number_width > length_vector_z_label ) ?
                             number_width : length_vector_z_label );

        if ( analy->vectors_at_nodes )
        {
            /* switch nodvec is set; Complete nodal data set:  3D */
            
            for ( i = 0; i < 3; i++ )
            {
                vec_data[i] = NEW_N( float, qty_of_nodes, "Vec comp result" );
                
                if ( vec_data[i] == NULL )
                {
                    for ( j = i - 1; j >= 0; j-- )
                        free( vec_data[j] );
                    popup_dialog( WARNING_POPUP, "%s\n%s",
                                  "Memory allocation for vector result failed.",
                                  "\"outvec\" processing aborted." );
                    return;
                }
            }

            /* Print column headings */
            fprintf ( outvec_file,
                      "#\n#\n# %s%d\n#\n#%s%*s%s%*s%s%*s%s%*s%s%*s%s%*s\n#\n",
                      quantity_label, qty_of_nodes, comment_label,
                      width_column_1 - length_comment, x_label, space,
                      width_column_2 - length_space, y_label, space,
                      width_column_3 - length_space, z_label, space,
                      width_column_4 - length_space, titles[0], space,
                      width_column_5 - length_space, titles[1], space, 
                      width_column_6 - length_space, titles[2] );

            /* Load data */

            for ( i = 0; i < 3; i++ )
            {
                if ( analy->vector_result[i] == NULL )
                    continue;
                update_result( analy, analy->vector_result[i] );
            }

            tmp_result = analy->cur_result;
            tmp_data = p_node_class->data_buffer;

            for ( i = 0; i < 3; i++ )
            {
                if ( analy->vector_result[i] == NULL )
                    continue;
                analy->cur_result = analy->vector_result[i];
                p_node_class->data_buffer = vec_data[i];
                load_result( analy, TRUE, TRUE );
            }

            analy->cur_result = tmp_result;
            p_node_class->data_buffer = tmp_data;
            
            nodes3d = analy->state_p->nodes.nodes3d;

            /* Print data */
            for ( i = 0; i < qty_of_nodes; i++ )
                fprintf ( outvec_file,
                          "% *.*e% *.*e% *.*e% *.*e % *.*e% *.*e\n",
                          width_column_1, PRECISION, nodes3d[i][0],
                          width_column_2, PRECISION, nodes3d[i][1],
                          width_column_3, PRECISION, nodes3d[i][2],
                          width_column_4, PRECISION, vec_data[0][i],
                          width_column_5, PRECISION, vec_data[1][i],
                          width_column_6, PRECISION, vec_data[2][i] );
            
            for ( i = 0; i < 3; i++ )
                free( vec_data[i] );
        }
        else
        {
            /* switch grdvec is set; Specified nodal data subset:  3D */

            /* Count the grid points. */
            qty_of_nodes = 0;
            
            class_qty = htable_get_data( MESH( analy ).class_table, 
                                         (void ***) &p_mo_classes );
            for ( i = 0; i < class_qty; i++ )
                qty_of_nodes += p_mo_classes[i]->vector_pt_qty;

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
            fprintf ( outvec_file,
                      "#\n#\n# %s%d\n#\n%s%*s%s%*s%s%*s%s%*s%s%*s%s%*s\n#\n",
                      quantity_label, qty_of_nodes, comment_label,
                      width_column_1 - length_comment, x_label, space,
                      width_column_2 - length_space, y_label, space,
                      width_column_3 - length_space, z_label, space,
                      width_column_4 - length_space, titles[0], space,
                      width_column_5 - length_space, titles[1], space,
                      width_column_6 - length_space, titles[2] );

            /* Print data */
            for ( i = 0; i < class_qty; i++ )
            {
                for ( point = p_mo_classes[i]->vector_pts; point != NULL; 
                      NEXT( point ) )
                    fprintf( outvec_file,
                             "% *.*e% *.*e% *.*e% *.*e % *.*e% *.*e\n",
                             width_column_1, PRECISION, point->pt[0],
                             width_column_2, PRECISION, point->pt[1],
                             width_column_3, PRECISION, point->pt[2],
                             width_column_4, PRECISION, point->vec[0],
                             width_column_5, PRECISION, point->vec[1],
                             width_column_6, PRECISION, point->vec[2] );
            }
            
            free( p_mo_classes );
        }
    }
    else if ( 2 == analy->dimension )
    {
        length_vector_x_label = strlen ( titles[0] );
        width_column_3 = length_space +
                         ( ( number_width > length_vector_x_label ) ?
                             number_width : length_vector_x_label );

        length_vector_y_label = strlen ( titles[1] );
        width_column_4 = length_space +
                         ( ( number_width > length_vector_y_label ) ?
                             number_width : length_vector_y_label );

        if ( analy->vectors_at_nodes )
        {
            /* switch nodvec is set; Complete nodal data set:  2D */
            
            for ( i = 0; i < 2; i++ )
            {
                vec_data[i] = NEW_N( float, qty_of_nodes, "Vec comp result" );
                
                if ( vec_data[i] == NULL )
                {
                    for ( j = i - 1; j >= 0; j-- )
                        free( vec_data[j] );
                    popup_dialog( WARNING_POPUP, "%s\n%s",
                                  "Memory allocation for vector result failed.",
                                  "\"outvec\" processing aborted." );
                    return;
                }
            }

            /* Print column headings */
            fprintf ( outvec_file,
                      "#\n#\n# %s%d\n#\n%s%*s%s%*s%s%*s%s%*s\n#\n",
                      quantity_label, qty_of_nodes, comment_label,
                      width_column_1 - length_comment, x_label, space,
                      width_column_2 - length_space, y_label, space,
                      width_column_3 - length_space, titles[0], space,
                      width_column_4 - length_space, titles[1] );

            /* Load data */

            tmp_result = analy->cur_result;
            tmp_data = p_node_class->data_buffer;

            for ( i = 0; i < 2; i++ )
            {
                if ( analy->vector_result[i] == NULL )
                    continue;
                analy->cur_result = analy->vector_result[i];
                p_node_class->data_buffer = vec_data[i];
                load_result( analy, TRUE, TRUE );
            }

            analy->cur_result = tmp_result;
            p_node_class->data_buffer = tmp_data;
            
            nodes2d = analy->state_p->nodes.nodes2d;

            /* Print data */
            for ( i = 0; i < qty_of_nodes; i++ )
                fprintf ( outvec_file,
                          "% *.*e% *.*e% *.*e% *.*e\n",
                          width_column_1, PRECISION, nodes2d[i][0],
                          width_column_2, PRECISION, nodes2d[i][1],
                          width_column_3, PRECISION, vec_data[0][i],
                          width_column_4, PRECISION, vec_data[1][i] );
        }
        else
        {
            /* switch grdvec is set; Specified nodal data subset:  2D */


            /* Count the grid points. */
            qty_of_nodes = 0;
            
            class_qty = htable_get_data( MESH( analy ).class_table, 
                                         (void ***) &p_mo_classes );
            for ( i = 0; i < class_qty; i++ )
                qty_of_nodes += p_mo_classes[i]->vector_pt_qty;

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
            fprintf ( outvec_file,
                      "#\n#\n# %s%d\n#\n%s%*s%s%*s%s%*s%s%*s\n#\n",
                      quantity_label, qty_of_nodes, comment_label,
                      width_column_1 - length_comment, x_label, space,
                      width_column_2 - length_space, y_label, space,
                      width_column_3 - length_space, titles[0], space, 
                      width_column_4 - length_space, titles[1] );

            for ( i = 0; i < class_qty; i++ )
            {
                for ( point = p_mo_classes[i]->vector_pts; point != NULL; 
                      NEXT( point ) )
                    fprintf( outvec_file,
                             "% *.*e% *.*e% *.*e% *.*e\n",
                             width_column_1, PRECISION, point->pt[0],
                             width_column_2, PRECISION, point->pt[1],
                             width_column_3, PRECISION, point->vec[0],
                             width_column_4, PRECISION, point->vec[1] );
            }
            
            free( p_mo_classes );
        }
    }
    
    for ( i = 0; i < 3; i++ )
        free( titles[i] );

    return;
}


/*****************************************************************
 * TAG( start_text_history )
 * 
 * Prepare ASCII file of text history
 */
static void
start_text_history( char text_history_file_name[] )
{
    if ( (text_history_file = fopen( text_history_file_name, "w" )) != NULL )
        text_history_invoked = TRUE;
    else
    {
        popup_dialog( INFO_POPUP, "text_history:  Unable to open file:  %s", 
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
end_text_history( void )
{
    fclose( text_history_file );
    text_history_invoked = FALSE;

    return;
}

/*****************************************************************
 * TAG( check_for_result )
 * 
 * Prepare ASCII file of text history
 */
static int
check_for_result( Analysis *analy, int display_warning )
{
int valid_result = TRUE;

   if ( analy->cur_result == NULL)
   {
    if ( display_warning )
       popup_dialog( WARNING_POPUP, "No result selected.");

    valid_result = FALSE;
   }
   return( valid_result );
}


/*****************************************************************
 * TAG( process_mat_obj_selection )
 * 
 * This function will process the material an object selection 
 * requests for vis/invis, enable/disable, and include/exclude
 * commands.
 */
void
process_mat_obj_selection ( Analysis *analy, char tokens[MAXTOKENS][TOKENLENGTH],
			    int idx, int token_cnt, int mat_qty, int elem_qty, 
			    unsigned char *p_elem, int  *p_hide_qty, int *p_hide_elem_qty, 
                            unsigned char *p_mat,
			    unsigned char setval, 
			    Bool_type vis_selected, Bool_type mat_selected )

{
   int i, ival;
   int mtl;
   int mat_min, mat_max;
   int elem_min, elem_max;

   char tmp_token[1000];

   string_to_upper( tokens[idx+1], tmp_token ); /* Make case insensitive */

   /* Material based selection */
   if ( p_mat != NULL && mat_selected )
   {
        if ( strcmp( tmp_token, "ALL" ) == 0  )
        {
	     for ( i = 0; i < mat_qty; i++ )
	     {
		   p_mat[i]  = (unsigned char) setval; /* Hide the selected material */
		  
		   if (setval) 
		      (*p_hide_qty)++;
		   else
		      (*p_hide_qty)--;
	     }
	}
	else
	     for ( i = 1; i < token_cnt - idx; i++ )
             {
	             parse_mtl_range(tokens[i+idx], mat_qty, &mat_min, &mat_max ) ;
                     mtl = mat_min ;

		     for (mtl  = mat_min ;
			  mtl <= mat_max ;
			  mtl++)
		     {
			  if (mtl > 0 && mtl <= mat_qty)
			  {
			      p_mat[mtl-1]  = (unsigned char) setval; /* Hide the selected material */
                             
			      if ( setval )
			           (*p_hide_qty)++;
			     else
			           (*p_hide_qty)--;
			  }
		     }
	      }
   }
   else /* Object selected */
        {
	     for ( i = 1; i < token_cnt - idx; i++ )
             {
	           parse_mtl_range(tokens[i+idx], elem_qty, &elem_min, &elem_max) ;

		   if (elem_max > elem_qty)
		       elem_max = elem_qty;
		   
		   for (i=elem_min;
			i<=elem_max;
			i++)
		   {
		        if ( !vis_selected )
			{
			     p_elem[i-1] = TRUE; 
			     (*p_hide_elem_qty)++;
			}
		        else
			{
			     p_elem[i-1] = FALSE;  
			     (*p_hide_elem_qty)--;
			}
		   } 
	     } 
	}
}

