/* $Id$ */
/* 
 * time_hist.c - Routines for drawing time-history plots.
 * 
 * 	Donald J. Dovey
 * 	Lawrence Livermore National Laboratory
 * 	Oct 28 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */

#include "viewer.h"
#include "draw.h"


static GLfloat plot_colors[20][3] =
{ 
  {0.157, 0.157, 1.0},    /* Blue */
  {1.0, 0.157, 0.157},    /* Red */
  {255, 0.157, 1.0},      /* Magenta */
  {0.157, 1.0, 1.0},      /* Cyan */
  {0.157, 1.0, 0.157},    /* Green */

  {1.0, 0.647, 0.0},      /* Orange */
  {139, 0.271, 0.075},    /* Brown */
  {0.118, 0.565, 1.0},    /* Dodger blue */
  {1.0, 0.412, 0.706},    /* Hot pink */
  {0.573, 0.545, 0.341},  /* SeaGreen */

  {0.157, 0.157, 1.0},    /* Blue */
  {1.0, 0.157, 0.157},    /* Red */
  {255, 0.157, 1.0},      /* Magenta */
  {0.157, 1.0, 1.0},      /* Cyan */
  {0.157, 1.0, 0.157},    /* Green */

  {1.0, 0.647, 0.0},      /* Orange */
  {139, 0.271, 0.075},    /* Brown */
  {0.118, 0.565, 1.0},    /* Dodger blue */
  {1.0, 0.412, 0.706},    /* Hot pink */
  {0.573, 0.545, 0.341}   /* SeaGreen */
};

static void draw_th_plot();
static void smooth_th_data();


/*****************************************************************
 * TAG( time_hist_plot )
 *
 * Draw a time-history plot.  The filename is optional and is used
 * for saving time-history data to a file.
 */
void
time_hist_plot( file_name, analy )
char *file_name;
Analysis *analy;
{
    static char *el_label[4] = { "Node", "Beam", "Shell", "Brick" };
    FILE *outfile;
    Result_type result_id;
    Bool_type gather;
    float *t_result, *tmp;
    char str[90];
    int eltype, num_sel, num_states;
    int i, j;
    float scale, offset;
    int cnt;

    result_id = analy->result_id;
    if ( result_id == VAL_NONE )
    {
	popup_dialog( INFO_POPUP, "No result to plot..." );
        return;
    }

    /* Decide which selected elements to use. */
    if ( analy->num_select[3] > 0 &&
         ( is_hex_result(result_id) || is_shared_result(result_id) ) )
    {
        eltype = BRICK_T;
    }
    else if ( analy->num_select[2] > 0 &&
              ( is_shell_result(result_id) || is_shared_result(result_id) ) )
    {
        eltype = SHELL_T;
    }
    else if ( analy->num_select[1] > 0 && is_beam_result(result_id) )
    {
        eltype = BEAM_T;
    }
    else if ( analy->num_select[0] > 0 )
    {
        eltype = NODE_T;
    }
    else
    {
        wrt_text( "No elements selected to plot values on.\n" );
        return;
    }

    /* GET VALUES. */

    num_sel = analy->num_select[eltype];
    num_states = analy->num_states;

    /* See if we need to gather the result. */
    gather = TRUE;
    if ( analy->th_result_type == eltype )
    {
        for ( i = 0; i < analy->th_result_cnt; i++ )
        {
/*            if ( analy->th_result_spec[i].ident == result_id ) */
            if ( match_result( analy, result_id, &analy->th_result_spec[i] ) )
            {
                gather = FALSE;
                t_result = &analy->th_result[i*num_sel*num_states];
                break;
            }
        }
    }

    /* If needed, gather the result for display. */
    if ( gather )
    {
        clear_gather( analy );
        analy->th_result_type = eltype;
        analy->th_result_spec[0].ident = result_id;
	analy->th_result_spec[0].strain_variety = analy->strain_variety;
	analy->th_result_spec[0].ref_surf = analy->ref_surf;
	analy->th_result_spec[0].ref_frame = analy->ref_frame;
        analy->th_result_cnt = 1;
        gather_results( analy );
        t_result = analy->th_result;
    }
    
    if ( analy->perform_unit_conversion )
    {
        cnt = num_sel * num_states;
	tmp = NEW_N( float, cnt, "Converted time history" );
	scale = analy->conversion_scale;
	offset = analy->conversion_offset;
	for ( i = 0; i < cnt; i++ )
	    tmp[i] = t_result[i] * scale + offset;
	t_result = tmp;
    }

    /* SAVE TO FILE. */

    if ( file_name != NULL )
    {
        if ( ( outfile = fopen(file_name, "w") ) == NULL )
        {
            wrt_text( "Couldn't open file %s.\n", file_name );
            return;
        }

        /* Print the header. */
        fprintf( outfile, "# GRIZ Time-history Data\n#\n" );
        fprintf( outfile, "# Database: %s\n", analy->title );
        fprintf( outfile, "# Date: %s\n", env.date );
        fprintf( outfile, "# Result: %s\n", analy->result_title );
	if ( analy->perform_unit_conversion )
	    fprintf( outfile, "# Applied conversion scale/offset: %E/%E\n", 
	             scale, offset );
        fprintf( outfile, "# Data: Time" );
        for ( i = 0; i < num_sel; i++ )
            fprintf( outfile, ", %s %d", el_label[eltype],
                     analy->select_elems[eltype][i] + 1 );
        fprintf( outfile, "\n#\n" );

        /* Print the data. */
        for ( i = 0; i < num_states; i++ )
        {
            fprintf( outfile, "%E", analy->state_times[i] );
            for ( j = 0; j < num_sel; j++ )
                fprintf( outfile, " %E", t_result[j*num_states + i] );
            fprintf( outfile, "\n" );
        }

        fclose( outfile );
    }

    /* DRAW. */

    draw_th_plot( num_sel, t_result, el_label[eltype],
                  analy->select_elems[eltype], analy );
		  
    if ( analy->perform_unit_conversion )
        free( t_result );
}


/*****************************************************************
 * TAG( parse_gather_command )
 *
 * Parse the command to gather result data for time-history plots.
 */
void
parse_gather_command( token_cnt, tokens, analy )
int token_cnt;
char tokens[MAXTOKENS][TOKENLENGTH];
Analysis *analy;
{
    Result_type result_id;
    int r_type, i;

    if ( strcmp( tokens[1], "node" ) == 0
	 || strcmp( tokens[1], "n" ) == 0 )
        r_type = NODE_T;
    else if ( strcmp( tokens[1], "beam" ) == 0
	      || strcmp( tokens[1], "b" ) == 0 )
        r_type = BEAM_T;
    else if ( strcmp( tokens[1], "shell" ) == 0
	      || strcmp( tokens[1], "s" ) == 0 )
        r_type = SHELL_T;
    else if ( strcmp( tokens[1], "brick" ) == 0
	      || strcmp( tokens[1], "h" ) == 0 )
        r_type = BRICK_T;
    else
    {
        wrt_text( "Please specify what type element to gather.\n" );
        return;
    }
    analy->th_result_type = r_type;

    /* Free up any previous storage. */
    clear_gather( analy );

    for ( i = 2; i < token_cnt; i++ )
    {
        result_id = lookup_result_id( tokens[i] );
        analy->th_result_spec[i-2].ident = result_id;
	analy->th_result_spec[i-2].strain_variety = analy->strain_variety;
	analy->th_result_spec[i-2].ref_surf = analy->ref_surf;
	analy->th_result_spec[i-2].ref_frame = analy->ref_frame;

        /* Make sure the result is of the correct type. */
        if ( r_type != NODE_T &&
             ( ( is_beam_result(result_id) && r_type != BEAM_T ) ||
               ( is_shell_result(result_id) && r_type != SHELL_T ) ||
               ( is_hex_result(result_id) && r_type != BRICK_T ) ||
               ( is_shared_result(result_id) &&
                 !(r_type == SHELL_T || r_type == BRICK_T) ) ) )
        {
            wrt_text( "Illegal result type specified.\n" );
            return;
        }
    }
    analy->th_result_cnt = token_cnt - 2;

    /* Perform the gather operation. */
    gather_results( analy );
}


/*****************************************************************
 * TAG( gather_results )
 *
 * Gather result data for time-history plots.  The calling routine
 * should call clear_gather first, and then set the appropriate
 * gather flags in analy.
 */
void
gather_results( analy )
Analysis *analy;
{
    Result_type result_id;
    float *result, *th_result;
    int cur_state, r_type, num_select, num_states, num_result;
    int i, j, k;

    /*
     * In analy:
     *      th_result_type:  (-1 ... 3) None or node, beam, shell, brick.
     *      th_result_cnt
     *      th_result_spec[ num_results ].ident
     *      th_result_spec[ num_results ].strain_variety
     *      th_result_spec[ num_results ].ref_frame
     *      th_result_spec[ num_results ].ref_surf
     *      th_result[ num_results * num_selected * num_states ]
     */
    r_type = analy->th_result_type;
    num_select = analy->num_select[r_type];
    num_states = analy->num_states;
    num_result = analy->th_result_cnt;

    /* Allocate space for gathered variables. */
    th_result = NEW_N( float, num_result*num_select*num_states*sizeof(float),                          "Time-history data" );
    analy->th_result = th_result;

    switch ( r_type )
    {
        case 0:
            result = analy->result;
            break;
        case 1:
            result = analy->beam_result;
            break;
        case 2:
            result = analy->shell_result;
            break;
        case 3:
            result = analy->hex_result;
            break;
    }

    cur_state = analy->cur_state;
    wrt_text( "Gathering timehist values...\n" );
    set_alt_cursor( CURSOR_WATCH );

    for ( i = 0; i < num_states; i++ )
    {
        analy->cur_state = i;
        analy->state_p = get_state( i, analy->state_p );

        for ( j = 0; j < num_result; j++ )
        {
            result_id = analy->result_id;
            analy->result_id = analy->th_result_spec[j].ident;
            load_result( analy, FALSE );

            for ( k = 0; k < num_select; k++ )
            {
                th_result[j*num_select*num_states+k*num_states+i] =
                        result[analy->select_elems[r_type][k]];
            }

            analy->result_id = result_id;
        }
    }

    analy->cur_state = cur_state;
    analy->state_p = get_state( cur_state, analy->state_p );
    load_result( analy, FALSE );
    unset_alt_cursor();
    wrt_text( "Done gathering timehist values.\n" );
}


/*****************************************************************
 * TAG( clear_gather )
 *
 * Delete any gathered result data.
 */
void
clear_gather( analy )
Analysis *analy;
{
    if ( analy->th_result != NULL )
        free( analy->th_result );
    analy->th_result = NULL;
    analy->th_result_cnt = 0;
}


/*****************************************************************
 * TAG( parse_mth_command )
 *
 * Parse the material time-history command.
 */
void
parse_mth_command( token_cnt, tokens, analy )
int token_cnt;
char tokens[MAXTOKENS][TOKENLENGTH];
Analysis *analy;
{
    Result_type result_id;
    int mat_cnt, mat_nums[50];
    int ival, i;

    if ( token_cnt < 2 )
    {
        wrt_text( "Material number needed for material time-history.\n" );
        return;
    }

    if ( token_cnt > 7 )
        wrt_text( "No more than six materials at a time, please.\n" );

    mat_cnt = token_cnt - 1;
    for ( i = 1; i < token_cnt && i < 7; i++ )
    {
        sscanf( tokens[i], "%d", &ival );
        mat_nums[i-1] = ival - 1;
    }

    mat_time_hist( mat_cnt, mat_nums, analy );
}


/*****************************************************************
 * TAG( mat_time_hist )
 *
 * Draw a material time-history plot for the specified materials.
 * The material count and then the number of each material is
 * passed into the routine.  Unlike the regular time-histories,
 * we don't keep the gathered data around.  Maximum number of
 * materials is set at 6 for convenience.
 */
void
mat_time_hist( mat_cnt, mat_nums, analy )
int mat_cnt;
int *mat_nums;
Analysis *analy;
{
    Hex_geom *bricks; 
    Shell_geom *shells;
    Beam_geom *beams;
    float *mark_nodes[6], *t_result, *result;
    int cur_state, num_nodes, num_states;
    int i, j, k;
    float scale, offset;
    int cnt;

    if ( analy->result_id == VAL_NONE )
        return;

    /* Free up any regular gather storage so we have enough space. */
    clear_gather( analy );

    /* GET VALUES. */

    num_states = analy->num_states;
    num_nodes = analy->geom_p->nodes->cnt;
    result = analy->result;
    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;
    beams = analy->geom_p->beams;

    /* Mark all nodes that are associated with each material. */
    for ( i = 0; i < mat_cnt; i++ )
    {
        mark_nodes[i] = analy->tmp_result[i];
        memset( mark_nodes[i], 0, num_nodes*sizeof( float ) );

        if ( bricks != NULL ) 
        {
            for ( j = 0; j < bricks->cnt; j++ )
                if ( bricks->mat[j] == mat_nums[i] )
                    for ( k = 0; k < 8; k++ )
                        mark_nodes[i][ bricks->nodes[k][j] ] = 1.0;
        }
        if ( shells != NULL )
        {
            for ( j = 0; j < shells->cnt; j++ )
                if ( shells->mat[j] == mat_nums[i] )
                    for ( k = 0; k < 4; k++ )
                        mark_nodes[i][ shells->nodes[k][j] ] = 1.0;
        }
        if ( beams != NULL )
        {
            for ( j = 0; j < beams->cnt; j++ )
                if ( beams->mat[j] == mat_nums[i] )
                    for ( k = 0; k < 2; k++ )
                        mark_nodes[i][ beams->nodes[k][j] ] = 1.0;
        }
    }

    /* Sum the values at the nodes for each state. */

    t_result = NEW_N( float, mat_cnt*num_states, "Material time-histories" );

    wrt_text( "Gathering timehist values...\n" );

    cur_state = analy->cur_state;
    for ( i = 0; i < num_states; i++ )
    {
        analy->cur_state = i;
        analy->state_p = get_state( i, analy->state_p );
        load_result( analy, FALSE );

        for ( j = 0; j < num_nodes; j++ )
            for ( k = 0; k < mat_cnt; k++ )
                if ( mark_nodes[k][j] == 1.0 )
                    t_result[k*num_states+i] += result[j];
    }
    analy->cur_state = cur_state;
    analy->state_p = get_state( cur_state, analy->state_p );
    load_result( analy, FALSE );

    wrt_text( "Done gathering timehist values.\n" );
    
    if ( analy->perform_unit_conversion )
    {
	scale = analy->conversion_scale;
	offset = analy->conversion_offset;
	cnt = num_states * mat_cnt;
	for ( i = 0; i < cnt; i++ )
	    t_result[i] = t_result[i] * scale + offset;
    }

    /* DRAW. */

    draw_th_plot( mat_cnt, t_result, "Material", mat_nums, analy );

    free( t_result );
}


/*****************************************************************
 * TAG( draw_th_plot )
 *
 * Draw a time-history plot.
 */
static void
draw_th_plot( num_plots, t_result, el_label, el_numbers, analy )
int num_plots;
float *t_result;
char *el_label;
int *el_numbers;
Analysis *analy;
{
    float cx, cy, vp_to_world_y;
    float text_height, win_ll[2], win_ur[2], gr_ll[2], gr_ur[2];
    float min_res, max_res, scale_width, min_incr_sz;
    float min_ax[2], max_ax[2], incr_ax[2];
    float pos[3];
    float val, val2;
    float char_width;
    float y_scale_width, x_scale_width;
    float *st_result;
    char str[90];
    int num_states;
    int max_incrs, incr_cnt[2];
    int i, j, k;
    int fracsz, y_fracsz, x_fracsz;

    num_states = analy->num_states;

    /* Get the min and max result values. */
    min_res = t_result[0];
    max_res = t_result[0];
    for ( i = 1; i < num_plots*num_states; i++ )
    {
        if ( t_result[i] < min_res )
            min_res = t_result[i];
        else if ( t_result[i] > max_res )
            max_res = t_result[i];
    }

    /* Apply smoothing if requested. */
    if ( analy->th_smooth )
    {
        /* Need to copy the data before modifying it. */
        st_result = NEW_N( float, num_plots*num_states, "Filter timhis plot" );
        for ( i = 0; i < num_plots*num_states; i++ )
            st_result[i] = t_result[i];
        
        smooth_th_data( num_plots, num_states, st_result, analy );
        t_result = st_result;
    }

    /* Get viewport position and size. */
    get_foreground_window( &pos[2], &cx, &cy );

    /* Text size. */
    vp_to_world_y = 2.0*cy / v_win->vp_height;
    text_height = 14.0 * vp_to_world_y;
    hfont( "futura.l" );
    htextsize( text_height, text_height );

    /* Width of scale values on the sides of the graph. */
    /* hgetcharsize( '0', &val, &val2 ); */
    /* scale_width = 8.0*val; */
    /* fracsz = analy->float_frac_size; */
    /* scale_width = ((float) fracsz + 6.0) * val; */
    
    /* 
     * Assign a scale width and graph corners based upon the 
     * current fraction size.  If auto fraction size has been
     * requested, use the maximum allowable digit quantity but
     * recalculate actual size and graph corners later.
     * 
     * Note that there is a cart-before-the-horse problem.
     * To calculate the fraction size, we need the tic interval.
     * To calculate the tic interval, we need the fraction size.
     * We could iterate, but is it really worth it?
     */
    hgetcharsize( '0', &char_width, &val2 );
    fracsz = analy->float_frac_size;
    scale_width = ((( analy->auto_frac_size ) ? 6 : fracsz) + 6.0) * char_width;

    /* Corners of the window. */
    win_ll[0] = -cx;
    win_ll[1] = -cy;
    win_ur[0] = cx;
    win_ur[1] = cy;

    /* Corners of the graph. */
    gr_ll[0] = win_ll[0] + 3*text_height + scale_width;
    gr_ll[1] = win_ll[1] + 6*text_height + scale_width;
    gr_ur[0] = win_ur[0] - text_height;
    gr_ur[1] = win_ur[1] - text_height;

    /* Get the tic increment and start and end tic values for each axis. */
    for ( i = 0; i < 2; i++ )
    {
        if ( i == 0 )
        {
            min_ax[i] = analy->state_times[0];
            max_ax[i] = analy->state_times[num_states-1];
        }
        else  /* i == 1 */
        {
            min_ax[i] = min_res;
            max_ax[i] = max_res;
        }

        /* 
	 * Take care of the special case where min and max are 
	 * approximately equal. APX_EQ macro was too tight.
	 */
	if ( fabs( (double) 1.0 - min_ax[i] / max_ax[i] ) < EPS )
        {
            if ( min_ax[i] == 0.0 )
            {
                incr_ax[i] = 1.0;
                min_ax[i] = -1.0;
                max_ax[i] = 1.0;
            }
            else
            {
                incr_ax[i] = min_ax[i];
                min_ax[i] = min_ax[i] - fabs( (double)min_ax[i] );
                max_ax[i] = max_ax[i] + fabs( (double)max_ax[i] );
            }
            incr_cnt[i] = 2;
            continue;
        }

        max_incrs = (int)( (gr_ur[i] - gr_ll[i]) / (2.0*text_height) );
        min_incr_sz = (max_ax[i] - min_ax[i]) / max_incrs;

        /* Get val into range [1.0,9.9999...]. */
        j = 0;
        if ( min_incr_sz < 1.0 )
            for ( val = min_incr_sz; (int)val < 1.0; val = val*10.0, j++ );
        else
            for ( val = min_incr_sz; (int)val >= 10.0; val = val*0.1, j++ );

        /* Increments of either 1, 2 or 5 are allowed. */
        if ( val == 1.0 )
            incr_ax[i] = 1.0;
        else if ( val <= 2.0 )
            incr_ax[i] = 2.0;
        else if ( val <= 5.0 )
            incr_ax[i] = 5.0;
        else
            incr_ax[i] = 10.0;

        /* Shift back to original range. */
        if ( min_incr_sz < 1.0 )
            for ( k = 0; k < j; incr_ax[i] *= 0.1, k++ );
        else
            for ( k = 0; k < j; incr_ax[i] *= 10.0, k++ );

        min_ax[i] = floor( (double)min_ax[i]/incr_ax[i] ) * incr_ax[i];
        max_ax[i] = ceil( (double)max_ax[i]/incr_ax[i] ) * incr_ax[i];

        /* Number of increments along the axes. */
        incr_cnt[i] = (int) ((max_ax[i] - min_ax[i])/incr_ax[i] + 0.5);
    }
    
    if ( analy->auto_frac_size )
    {
	y_fracsz = calc_fracsz( max_ax[1], min_ax[1], incr_cnt[1] );
	y_scale_width = (y_fracsz + 6.0) * char_width;
	
	x_fracsz = calc_fracsz( max_ax[0], min_ax[0], incr_cnt[0] );
	x_scale_width = (x_fracsz + 6.0) * char_width;

	/* Corners of the graph. */
	gr_ll[0] = win_ll[0] + 3*text_height + y_scale_width;
	gr_ll[1] = win_ll[1] + 6*text_height + x_scale_width;
	gr_ur[0] = win_ur[0] - text_height;
	gr_ur[1] = win_ur[1] - text_height;
    }
    else
    {
	y_fracsz = fracsz;
	y_scale_width = scale_width;
	x_fracsz = fracsz;
	x_scale_width = scale_width;
    }

    /* No lighting for foreground stuff. */
    if ( v_win->lighting )
        glDisable( GL_LIGHTING );

    /* Clear screen. */
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    /* Antialias the lines. */
    glEnable( GL_LINE_SMOOTH );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glLineWidth( 1.2 );

    /* Draw the curves. */
    for ( i = 0; i < num_plots; i++ )
    {
        glColor3fv( plot_colors[i] );
        glBegin( GL_LINE_STRIP );
        for ( j = 0; j < num_states; j++ )
        {
            val = analy->state_times[j];
            pos[0] = (gr_ur[0]-gr_ll[0]) * (val-min_ax[0]) /
                     (max_ax[0]-min_ax[0]) + gr_ll[0];
            val = t_result[i*num_states + j];
            pos[1] = (gr_ur[1]-gr_ll[1]) * (val-min_ax[1]) /
                     (max_ax[1]-min_ax[1]) + gr_ll[1];
            glVertex3fv( pos );
        }
        glEnd();
    }

    /* Draw the border. */
    glColor3fv( v_win->foregrnd_color );
    glBegin( GL_LINE_LOOP );
    pos[0] = gr_ll[0];
    pos[1] = gr_ll[1];
    glVertex3fv( pos );
    pos[0] = gr_ur[0];
    glVertex3fv( pos );
    pos[1] = gr_ur[1];
    glVertex3fv( pos );
    pos[0] = gr_ll[0];
    glVertex3fv( pos );
    glEnd();

    /* Draw the tic marks along each axis and write the tic values. */
    htextang( 90.0 );
    hrightjustify( TRUE );
    for ( i = 0; i <= incr_cnt[0]; i++ )
    {
        glBegin( GL_LINES );
        pos[0] = (gr_ur[0] - gr_ll[0]) * i/(float)incr_cnt[0] + gr_ll[0];
        pos[1] = gr_ll[1];
        glVertex3fv( pos );
        pos[1] = gr_ll[1] + 0.5*text_height;
        glVertex3fv( pos );
        glEnd();

        pos[0] += 0.5*text_height;
        pos[1] = gr_ll[1] - 0.5*text_height;
        hmove( pos[0], pos[1], pos[2] );
        /* sprintf( str, "%.2e", i*incr_ax[0] + min_ax[0] ); */
        sprintf( str, "%.*e", x_fracsz, i*incr_ax[0] + min_ax[0] );
        hcharstr( str );
    }

    htextang( 0.0 );
    hrightjustify( TRUE );
    for ( i = 0; i <= incr_cnt[1]; i++ )
    {
        glBegin( GL_LINES );
        pos[0] = gr_ll[0];
        pos[1] = (gr_ur[1] - gr_ll[1])* i/(float)incr_cnt[1] + gr_ll[1];
        glVertex3fv( pos );
        pos[0] = gr_ll[0] + 0.5*text_height;
        glVertex3fv( pos );
        glEnd();

        pos[0] = gr_ll[0] - 0.5*text_height;
        pos[1] -= 0.5*text_height;
        hmove( pos[0], pos[1], pos[2] );
        /* sprintf( str, "%.2e", i*incr_ax[1] + min_ax[1] ); */
        sprintf( str, "%.*e", y_fracsz, i*incr_ax[1] + min_ax[1] );
        hcharstr( str );
    }

    /* Label each axis. */
    hcentertext( TRUE );
    pos[0] = 0.5*(gr_ur[0]-gr_ll[0]) + gr_ll[0];
    pos[1] = gr_ll[1] - x_scale_width - 2.0*text_height;
    hmove( pos[0], pos[1], pos[2] );
    hcharstr( "Time" );

    htextang( 90.0 );
    pos[0] = gr_ll[0] - y_scale_width - 2.0*text_height;
    pos[1] = 0.5*(gr_ur[1]-gr_ll[1]) + gr_ll[1];
    hmove( pos[0], pos[1], pos[2] );
    hcharstr( trans_result[resultid_to_index[analy->result_id]][1] );
    htextang( 0.0 );

    /* Color key. */
    hleftjustify( TRUE );
    pos[0] = gr_ll[0];
    pos[1] = win_ll[1] 
             + (( analy->perform_unit_conversion ) ? 2.0 : 1.0 )
	       * text_height;
    hmove( pos[0], pos[1], pos[2] );
    hcharstr( el_label );
    /* hgetcharsize( '0', &val, &val2 ); */
    pos[0] += 8.0 * char_width; 

    for ( i = 0; i < num_plots; i++ )
    {
        hmove( pos[0], pos[1], pos[2] );
        glColor3fv( plot_colors[i] );
        sprintf( str, "%d", el_numbers[i] + 1 );
        hcharstr( str );
        pos[0] += 6.0 * char_width; 
    }
    
    /* Notify if data conversion active. */
    if ( analy->perform_unit_conversion )
    {
        pos[0] = gr_ll[0];
        pos[1] = win_ll[1] + 0.5 * text_height;
	hmove( pos[0], pos[1], pos[2] );
        glColor3fv( v_win->foregrnd_color );
	sprintf( str, "Data conversion scale/offset: %.3e/%.3e", 
	         analy->conversion_scale, analy->conversion_offset );
        hcharstr( str );
    }

    /* Message if smoothing. */
    if ( analy->th_smooth )
    {
        sprintf( str, "Smooth: %d", analy->th_filter_width );
        hrightjustify( TRUE );
        glColor3fv( v_win->foregrnd_color );
        pos[0] = gr_ur[0];
        pos[1] = win_ll[1] 
                 + (( analy->perform_unit_conversion ) ? 2.0 : 1.0 )
	           * text_height;
        hmove( pos[0], pos[1], pos[2] );
        hcharstr( str );
    }

    /* Antialiasing off. */
    glDisable( GL_LINE_SMOOTH );
    glDisable( GL_BLEND );
    glLineWidth( 1.0 );

    /* Free smoothing array. */
    if ( analy->th_smooth )
        free( st_result );

    gui_swap_buffers();
}


/*****************************************************************
 * TAG( smooth_th_data )
 *
 * Smooth the data for time-history curves by filtering it.
 */
static void
smooth_th_data( num_plots, num_pts, t_result, analy )
int num_plots;
int num_pts;
float *t_result;
Analysis *analy;
{
    float *kernel;
    float sum;
    int fwidth, fsize;
    int i, j, k, idx;

    /* Build the filtering kernel. */
    fwidth = analy->th_filter_width;
    fsize = fwidth*2 + 1;
    kernel = NEW_N( float, fsize, "Filtering kernel\n" );

    switch ( analy->th_filter_type )
    {
        case BOX_FILTER:
            for ( i = 0; i < fsize; i++ )
                kernel[i] = 1.0 / fsize;
            break;
        default:
            popup_dialog( WARNING_POPUP, 
	                  "Smoothing filter type unrecognized." );
    }

    /* Perform the convolution on each curve. */
    for ( i = 0; i < num_plots; i++ )
    {
        for ( j = 0; j < num_pts; j++ )
        {
            sum = 0.0;
            for ( k = 0; k < fsize; k++ )
            {
                idx = j - fwidth + k;

                if ( idx < 0 )
                    sum += kernel[k] * t_result[i*num_pts];
                else if ( idx >= num_pts )
                    sum += kernel[k] * t_result[i*num_pts + num_pts - 1];
                else
                    sum += kernel[k] * t_result[i*num_pts + idx];
            }
            t_result[i*num_pts + j] = sum;
        }
    }

    free( kernel );
}


