/* $Id$ */
/* 
 * time.c - Routines which execute time-related commands (like animation).
 * 
 * 	Donald J. Dovey
 * 	Lawrence Livermore National Laboratory
 * 	Jan 2 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */

#include <values.h>
#include "viewer.h"


/* Local routines. */
static void interp_activity();


/*****************************************************************
 * TAG( anim_info )
 *
 * Holds the animation state for the animation in progress.
 */
static struct
{
    Bool_type interpolate;

    /* Use if not interpolating. */
    int cur_state;
    int last_state;

    /* Use if interpolating. */
    float end_time;
    float start_time;
    int num_frames;
    int cur_frame;
    int st_num_a;
    int st_num_b;
    State *state_a;
    State *state_b;
    float *result_a;
    float *result_b;
} anim_info;

 
/*****************************************************************
 * TAG( change_state )
 * 
 * The things that need to be done when we move to a new state.
 */
void
change_state( analy )
Analysis *analy;
{
    char str[90];
    Bool_type recompute_norms;

    if ( env.timing )
    {
        wrt_text( "\nTiming for change state...\n" );
        check_timing( 0 ); 
    }

/*
wrt_text( "\nTiming for get_state...\n" );
check_timing( 0 );
*/
    analy->state_p = get_state( analy->cur_state,
                                analy->state_p );
/*
check_timing( 1 );
*/
    wrt_text( "State %d out of %d states.\n", analy->cur_state+1,
              analy->num_states );

/*
wrt_text( "\nTiming for reset_face_visibility...\n" );
check_timing( 0 );
*/
    /* If activity data then activity may have changed. */
    recompute_norms = FALSE;
    if ( analy->geom_p->bricks && analy->state_p->activity_present )
    {
        reset_face_visibility( analy );
        recompute_norms = TRUE;
    }
/*
check_timing( 1 );
*/

/*
wrt_text( "\nTiming for compute_normals...\n" );
check_timing( 0 );
*/
    /* Gotta recompute normals, unless node positions don't change. */
    if ( !analy->normals_constant || recompute_norms )
        compute_normals( analy );
/*
check_timing( 1 );
*/

/*
wrt_text( "\nTiming for load_result...\n" );
check_timing( 0 );
*/
    /* Update displayed result. */
    load_result( analy, TRUE );
/*
check_timing( 1 );
*/

/*
wrt_text( "\nTiming for rest of stuff...\n" );
check_timing( 0 );
*/
    /* Update cut planes, isosurfs, contours. */
    update_vis( analy );
/*
check_timing( 1 );
*/

    if ( env.timing )
        check_timing( 1 ); 
}

 
/*****************************************************************
 * TAG( change_time )
 * 
 * Moves the display to a specified time, interpolating only the
 * position and the currently displayed variable.
 */
void
change_time( time, analy )
float time;
Analysis *analy;
{
    State *state_a, *state_b;
    float *result_a, *result_b;
    float t_a, t_b, interp, ninterp;
    Bool_type recompute_norms;
    char str[20];
    int st_num_a, st_num_b;
    int i, j;

    /*
     * This routine should really go through and interpolate everything
     * in the database.
     */
    st_num_b = 1;
    while ( time > analy->state_times[st_num_b] )
        st_num_b++;
    st_num_a = st_num_b - 1;
    state_a = get_state( st_num_a, analy->state_p );
    state_b = get_state( st_num_b, NULL );
    analy->cur_state = st_num_a;

    if ( analy->result_id != VAL_NONE )
    {
        result_b = NEW_N( float, state_b->nodes->cnt, "Interpolate result" );
        result_a = analy->result;
        load_result( analy, TRUE );
        analy->state_p = state_b;
        analy->result = result_b;
        analy->cur_state = st_num_b;
        load_result( analy, TRUE );
        analy->state_p = state_a;
        analy->result = result_a;
        analy->cur_state = st_num_a;
    }

    t_a = state_a->time;
    t_b = state_b->time;
    interp = (time-t_a) / (t_b-t_a);
    ninterp = 1.0 - interp;

    /*
     * Stash the interpolated nodal values in state_a and
     * the interpolated result values in result_a.
     */
    for ( i = 0; i < state_a->nodes->cnt; i++ )
    {
        for ( j = 0; j < 3; j++ )
            state_a->nodes->xyz[j][i]=ninterp*state_a->nodes->xyz[j][i]+
                                      interp*state_b->nodes->xyz[j][i];
        if ( analy->result_id != VAL_NONE )
            result_a[i] = ninterp*result_a[i] + interp*result_b[i];
    }
    analy->state_p->time = time;

    /* If activity data then activity may have changed. */
    recompute_norms = FALSE;
    if ( state_a->activity_present )
    {
        interp_activity( state_a, state_b );
        if ( state_a->bricks )
        {
            reset_face_visibility( analy );
            recompute_norms = TRUE;
        }
    }

    /* Gotta recompute normals, unless node positions don't change. */
    if ( !analy->normals_constant || recompute_norms )
        compute_normals( analy );

    /* Update cut planes, isosurfs, contours. */
    update_vis( analy );

    /* Clean up. */
    fr_state( state_b );
    if ( analy->result_id != VAL_NONE )
        free( result_b );

    /*
     * Print the time and return so the caller can redisplay the mesh.
     */ 
    wrt_text( "t = %f\n", time );
}


/*****************************************************************
 * TAG( parse_animate_command )
 *
 * Parse the animation command and initialize an animation.  A
 * workproc is used to step the animation along.
 */
void
parse_animate_command( token_cnt, tokens, analy )
int token_cnt;
char tokens[MAXTOKENS][TOKENLENGTH];
Analysis *analy;
{
    if ( token_cnt == 1 )
    {
        /* Just display all of the states in succesion. */
        anim_info.interpolate = FALSE;
        anim_info.cur_state = 0;
        anim_info.last_state = analy->num_states - 1;
    }
    else if ( token_cnt == 2 || token_cnt == 4 )
    {
        anim_info.interpolate = TRUE;

        sscanf( tokens[1], "%d", &anim_info.num_frames );

        if ( token_cnt == 2 )
        {
            /* The user input just the number of frames. */
            anim_info.start_time = analy->state_times[0];
            anim_info.end_time = analy->state_times[analy->num_states-1];
        }
        else
        {
            /* The user input number of frames and start & end times. */
            sscanf( tokens[2], "%f", &anim_info.start_time );
            sscanf( tokens[3], "%f", &anim_info.end_time );

            /* Sanity checks. */
            if ( anim_info.start_time < analy->state_times[0] )
                anim_info.start_time = analy->state_times[0];
            if ( anim_info.end_time > analy->state_times[analy->num_states-1])
                anim_info.end_time = analy->state_times[analy->num_states-1];
        }
        anim_info.cur_frame = 0;

        /* Initialize the animation. */
        anim_info.st_num_a = 0;
        anim_info.st_num_b = 1;
        anim_info.state_a = get_state( anim_info.st_num_a, analy->state_p );
        anim_info.state_b = get_state( anim_info.st_num_b, NULL );
        if ( analy->result_id != VAL_NONE )
        {
            anim_info.result_a = analy->result;
            anim_info.result_b = NEW_N( float, anim_info.state_b->nodes->cnt,
                                        "Animate result" );
            analy->cur_state = anim_info.st_num_a;
            load_result( analy, TRUE );
            analy->state_p = anim_info.state_b;
            analy->result = anim_info.result_b;
            analy->cur_state = anim_info.st_num_b;
            load_result( analy, TRUE );
            analy->state_p = anim_info.state_a;
            analy->result = anim_info.result_a;
            analy->cur_state = anim_info.st_num_a;
        }
        else
        {
            anim_info.result_a = NULL;
            anim_info.result_b = NULL;
        }
    }

    /* Quit any previous animation. */
    end_animate_workproc( TRUE );

    /* Fire up a work proc to step the animation forward. */
    init_animate_workproc();
}


/*****************************************************************
 * TAG( continue_animate )
 *
 * Start an animation from the current state to the last state.
 */
void
continue_animate( analy )
Analysis *analy;
{
    /* Just display all of the states in succesion. */
    anim_info.interpolate = FALSE;
    anim_info.cur_state = analy->cur_state;
    anim_info.last_state = analy->num_states - 1;

    /* Quit any previous animation. */
    end_animate_workproc( TRUE );

    /* Fire up a work proc to step the animation forward. */
    init_animate_workproc();
}



/*****************************************************************
 * TAG( step_animate )
 *
 * Move the animation forward one frame.
 */
Bool_type
step_animate( analy )
Analysis *analy;
{
    Nodal *nodes_a, *nodes_b;
    State *tmp_state;
    float *tmp_result;
    float start_t, end_t, t, t_a, t_b, interp, ninterp;
    Bool_type recompute_norms;
    int num_frames;
    int i, j, k;

    if ( !anim_info.interpolate )
    {
        /* Display the next state. */
        analy->cur_state = anim_info.cur_state;
        change_state( analy );
	if ( analy->center_view )
            center_view( analy );
        update_display( analy );

        anim_info.cur_state++;
        if ( anim_info.cur_state > anim_info.last_state )
        {
            end_animate_workproc( FALSE );
            return( TRUE );
        }
        else
            return( FALSE );
    }
    else
    {
        num_frames = anim_info.num_frames;
        start_t = anim_info.start_time;
        end_t = anim_info.end_time;
        i = anim_info.cur_frame;

        t = ( i / (float)(num_frames - 1) )*(end_t - start_t) + start_t;

        /* Get the two states that bracket the time value. */
        if ( t > anim_info.state_b->time )
        {
            if ( t <= analy->state_times[anim_info.st_num_b+1] )
            {
                anim_info.st_num_a = anim_info.st_num_b;
                anim_info.st_num_b++;
                tmp_state = anim_info.state_a;
                anim_info.state_a = anim_info.state_b;
                anim_info.state_b = tmp_state;
                anim_info.state_b = get_state( anim_info.st_num_b,
                                               anim_info.state_b );

                if ( analy->result_id != VAL_NONE )
                {
                    tmp_result = anim_info.result_a;
                    anim_info.result_a = anim_info.result_b;
                    anim_info.result_b = tmp_result;
                    analy->state_p = anim_info.state_b;
                    analy->result = anim_info.result_b;
                    analy->cur_state = anim_info.st_num_b;
                    load_result( analy, TRUE );
                    analy->state_p = anim_info.state_a;
                    analy->result = anim_info.result_a;
                    analy->cur_state = anim_info.st_num_a;
                }
            }
            else
            {
                while( t > analy->state_times[anim_info.st_num_b] )
                    anim_info.st_num_b++;
                anim_info.st_num_a = anim_info.st_num_b - 1;
                anim_info.state_a = get_state( anim_info.st_num_a,
                                               anim_info.state_a );
                anim_info.state_b = get_state( anim_info.st_num_b,
                                               anim_info.state_b );

                if ( analy->result_id != VAL_NONE )
                {
                    analy->cur_state = anim_info.st_num_a;
                    load_result( analy, TRUE );
                    analy->state_p = anim_info.state_b;
                    analy->result = anim_info.result_b;
                    analy->cur_state = anim_info.st_num_b;
                    load_result( analy, TRUE );
                    analy->state_p = anim_info.state_a;
                    analy->result = anim_info.result_a;
                    analy->cur_state = anim_info.st_num_a;
                }
            }
        }
        else
        {
            /* State_a has been messed up with the interpolated
             * values so we must reget it.
             */
            anim_info.state_a = get_state( anim_info.st_num_a,
                                           anim_info.state_a );
            if ( analy->result_id != VAL_NONE )
                load_result( analy, TRUE );
        }

        /* If a-b swapping occurred, need to fix up. */
        analy->cur_state = anim_info.st_num_a;
        analy->state_p = anim_info.state_a;
        if ( analy->result_id != VAL_NONE )
            analy->result = anim_info.result_a;

        t_a = anim_info.state_a->time;
        t_b = anim_info.state_b->time;
        interp = (t-t_a) / (t_b-t_a);
        ninterp = 1.0 - interp;

        /*
         * Stash the interpolated nodal values in state_a and
         * the interpolated result values in result_a.
         */
        nodes_a = anim_info.state_a->nodes;
        nodes_b = anim_info.state_b->nodes;
        for ( j = 0; j < nodes_a->cnt; j++ )
        {
            for ( k = 0; k < 3; k++ )
                nodes_a->xyz[k][j] = ninterp * nodes_a->xyz[k][j] +
                                     interp * nodes_b->xyz[k][j];
            if ( analy->result_id != VAL_NONE )
                anim_info.result_a[j] = ninterp*anim_info.result_a[j] +
                                        interp*anim_info.result_b[j];
        }
        anim_info.state_a->time = t;

        /* If activity data then activity may have changed. */
        recompute_norms = FALSE;
        if ( anim_info.state_a->activity_present )
        {
            interp_activity( anim_info.state_a, anim_info.state_b );
            if ( anim_info.state_a->bricks )
            {
                reset_face_visibility( analy );
                recompute_norms = TRUE;
            }
        }

        /* Gotta recompute normals, unless node positions don't change. */
        if ( !analy->normals_constant || recompute_norms )
            compute_normals( analy );

        /* Update cut planes, isosurfs, contours. */
        update_vis( analy );
	
	if ( analy->center_view )
            center_view( analy );

        /*
         * Redraw the mesh.
         */
        update_display( analy );

        /* Increment frame number. */
        anim_info.cur_frame++;
        if ( anim_info.cur_frame >= anim_info.num_frames )
        {
            /* Finish on the next state. */
            if ( analy->cur_state < analy->num_states-1 )
                analy->cur_state++;

            end_animate_workproc( FALSE );
            return( TRUE );
        }
        else
            return( FALSE );
    }
}


/*****************************************************************
 * TAG( end_animate )
 *
 * Clean up the current animation.
 */
void
end_animate( analy )
Analysis *analy;
{
    if ( anim_info.interpolate )
    {
        fr_state( anim_info.state_b );
        if ( anim_info.result_b != NULL )
            free( anim_info.result_b );

        change_state( analy );
        update_display( analy );
    }
}


/*****************************************************************
 * TAG( get_global_minmax )
 *
 * Get the global min/maxes for the currently displayed result.
 * Just walk through all the states without redisplaying the mesh.
 */
void
get_global_minmax( analy )
Analysis *analy;
{
    float *global_mm, *result;
    int cur_state;
    int cnt, i, j;
    Minmax_obj mm_save;

    if ( analy->result_id == VAL_NONE )
        return;

    cur_state = analy->cur_state;
    mm_save = analy->elem_state_mm;

/**/
/* This speed-up avoids nodal interpolation of element results and so
 * generates a global minmax based on element results.  The normal
 * minmax update logic always generates the global minmax from nodal
 * results (interpolated element results) and saves the element minmax
 * into analy->global_elem_mm.  So...this speed-up is wrong, but keep
 * it around temporarily for reference.
 */
#ifdef BAD_SPEEDUP
    if ( is_in_database( analy->result_id ) )
    {
        /* Result is in database (not derived), loop to grab it.
         * This is a speed-up for results in the database.
         */
        global_mm = analy->global_mm;
        result = analy->result;

        if ( is_nodal_result( analy->result_id ) )
            cnt = analy->geom_p->nodes->cnt;
        else if ( is_hex_result( analy->result_id ) )
            cnt = analy->geom_p->bricks->cnt;
        else if ( is_shell_result( analy->result_id ) )
            cnt = analy->geom_p->shells->cnt;
        else if ( is_beam_result( analy->result_id ) )
            cnt = analy->geom_p->beams->cnt;

        for ( i = 0; i < analy->num_states; i++ )
        {
	    init_mm_obj( &analy->tmp_elem_mm );
	    
            get_result( analy->result_id, i, result );
            for ( j = 0; j < cnt; j++ )
            {
                if ( result[j] < global_mm[0] )
                    global_mm[0] = result[j];
                else if ( result[j] > global_mm[1] )
                    global_mm[1] = result[j];
            }
        }
    }
    else
    {
        for ( i = 0; i < analy->num_states; i++ )
        {
            analy->cur_state = i;
            analy->state_p = get_state( i, analy->state_p );

            /* Update displayed result. */
            load_result( analy, TRUE );
        }
    }
#else
    for ( i = 0; i < analy->num_states; i++ )
    {
        analy->cur_state = i;
        analy->state_p = get_state( i, analy->state_p );

        /* Update displayed result. */
        load_result( analy, TRUE );
    }
#endif

    /* Go back to where we were before. */
    analy->elem_state_mm = mm_save;
    analy->cur_state = cur_state;
    change_state( analy );
}


/*****************************************************************
 * TAG( tellmm )
 *
 * Prepare summary of minimums and maximums for specified time
 * states of currently (or specified) result.
 *
 * NOTE:  Similar to get_global_minmax
 *
 */
void
tellmm( analy, desired_result_variable, start_state, stop_state, tellmm_redraw )
Analysis *analy;
char *desired_result_variable;
int start_state, stop_state;
Bool_type *tellmm_redraw;
{
    Bool_type recompute_norms;
    Minmax_obj mm_save;
    Result_type desired_result ,result_id_save;
    float high, low, maximum_of_states, minimum_of_states;
    float *el_mm;
    int cur_state, i, state_id;
    int start_idx, stop_idx;
    int obj_fwid, st_fwid, nam_fwid;
    int cnt1, cnt2, cnt3;
    int maximum_element_id, maximum_element_type, maximum_state_id, 
        minimum_element_id, minimum_element_type, minimum_state_id, 
        mm_node_types[2];
    int *el_type, *el_id;
    static char *el_label[] = { "node", "beam", "shell", "brick" };

    /*
     * Filter input data
     */

    desired_result = (Result_type) lookup_result_id( desired_result_variable );

    if ( (analy->result_id != VAL_NONE) &&
         (strcmp( desired_result_variable, "" ) == 0 ) )
    {
        /*
         * current result_id != materials and
         * no overriding result_id is specified -->
         * process current result_id
         */

        result_id_save = analy->result_id;

        if ( analy->result_id == desired_result )
            *tellmm_redraw = TRUE;
        else
            *tellmm_redraw = FALSE;
    }
    else if ( (desired_result != VAL_NONE) &&
              (desired_result != -1) &&
              (strcmp( desired_result_variable, "" ) != 0 ) )
    {
        /*
         * current result_id MAY or MAY NOT be materials, but
         * valid overriding result_id is specified that is != materials -->
         * process specified result_id
         */

        result_id_save   = analy->result_id;
        analy->result_id = desired_result;

        if ( analy->result_id == desired_result )
            *tellmm_redraw = TRUE;
        else
            *tellmm_redraw = FALSE;
    }
    else
    {
        /*
         * Invalid data present:
         *
         * current result_id and desired result_id == materials;
         * desired result_id == materials;
         * invalid overriding desired_result is specified
         */

        popup_dialog( USAGE_POPUP
                     ,"tellmm [<result> [<first state> [<last state>]]]" );

        tellmm_redraw = FALSE;
        return;
    }

    /* retain current state data */

    cur_state = analy->cur_state;
    mm_save   = analy->elem_state_mm;

    maximum_of_states = -MAXFLOAT;
    minimum_of_states =  MAXFLOAT;

    state_id = start_state;

    start_idx = MAX( 0, start_state - 1 );
    stop_idx  = MIN( stop_state, analy->num_states );

    if ( start_state > stop_state )
    {
        popup_dialog( WARNING_POPUP
                     ,"Start state MUST be less than stop state." );
        return;
    }
    
    /* 
     * Calculate field widths for formatting output. 
     */
    
    /* Node/element label widths */
    if ( is_nodal_result( analy->result_id ) )
    {
	obj_fwid = (int) ((double) 1.0 
	                  + log10( (double) analy->geom_p->nodes->cnt ));
	nam_fwid = 4; /* length of string "node" */
    }
    else
    {
        cnt1 = cnt2 = cnt3 = 0;

	if ( analy->geom_p->bricks != NULL )
	    cnt1 = analy->geom_p->bricks->cnt;
	if ( analy->geom_p->shells != NULL )
	    cnt2 = analy->geom_p->shells->cnt;
	if ( analy->geom_p->beams != NULL )
	    cnt3 = analy->geom_p->beams->cnt;

	cnt1 = MAX( cnt1, MAX( cnt2, cnt3 ) );
    
        obj_fwid = (int) ((double) 1.0 + log10( (double) cnt1 ));
	
	nam_fwid = 5; /* length of longest of "beam", "shell", and "brick" */
    }
    
    /* State number width */
    st_fwid = (int) ((double) 1.0 + log10( (double) stop_state ));

    /*
     * Write the report.
     */
    
    wrt_text( "\n\nReport of %s max/min values, states %d to %d:\n\n%s\n\n", 
              trans_result[resultid_to_index[analy->result_id]][1], 
	      start_state, stop_state, 
              "STATE          MAX                     MIN" );

    for ( i = start_idx; i < stop_idx; i++ )
    {
        analy->cur_state = i;
        analy->state_p   = get_state( i, analy->state_p );

        /* update displayed result */

        load_result( analy, TRUE );

        if ( is_nodal_result( analy->result_id ) )
        {
            mm_node_types[0] = mm_node_types[1] = NODE_T;
            el_mm   = analy->state_mm;
            el_type = mm_node_types;
            el_id   = analy->state_mm_nodes;
        }
        else
        {
            el_mm   = analy->elem_state_mm.el_minmax;
            el_type = analy->elem_state_mm.el_type;
            el_id   = analy->elem_state_mm.mesh_object;
        }
	
        if ( analy->perform_unit_conversion )
        {
            low  = (analy->conversion_scale * el_mm[0]) 
                 + analy->conversion_offset;
            high = (analy->conversion_scale * el_mm[1]) 
                 + analy->conversion_offset;
	}
        else
        {
            low  = el_mm[0];
            high = el_mm[1];
        }

        wrt_text( " %*d    %9.2e  %*s %-*d    %9.2e  %*s %d\n", 
	          st_fwid, state_id, 
		  high, nam_fwid, el_label[el_type[1]], obj_fwid, el_id[1], 
		  low, nam_fwid, el_label[el_type[0]], el_id[0] );

        if ( high > maximum_of_states )
        {
            maximum_of_states    = high;
            maximum_element_type = el_type[1];
            maximum_element_id   = el_id[1];
            maximum_state_id     = state_id;
        }
                
        if ( low < minimum_of_states )
        {
            minimum_of_states    = low;
            minimum_element_type = el_type[0];
            minimum_element_id   = el_id[0];
            minimum_state_id     = state_id;
        }

        state_id++;
	
	cache_global_minmax( analy );
    }

    wrt_text( " \n" );

    wrt_text( "Maximum of states:  %9.2e at %s %d, state %2d\n"
             ,maximum_of_states
             ,el_label[maximum_element_type]
             ,maximum_element_id
             ,maximum_state_id );

    wrt_text( "Minimum of states:  %9.2e at %s %d, state %2d\n\n"
             ,minimum_of_states
             ,el_label[minimum_element_type]
             ,minimum_element_id
             ,minimum_state_id );

    /*
     * Return to original status
     *
     * NOTE:  This is an abbreviated form of "change_state"
     *        which avoids display of messages superfluous
     *        to "tellmm"
     */

    analy->elem_state_mm = mm_save;
    analy->cur_state     = cur_state;
    analy->result_id     = result_id_save;

    analy->state_p = get_state( analy->cur_state,
                                analy->state_p );

    /* if activity data then activity may have changed */

    recompute_norms = FALSE;

    if ( analy->geom_p->bricks && analy->state_p->activity_present )
    {
        reset_face_visibility( analy );
        recompute_norms = TRUE;
    }

    /* recompute normals if node positions have changed */

    if ( !analy->normals_constant || recompute_norms )
        compute_normals( analy );

    /* update displayed result */

    load_result( analy, TRUE );

    /* update cut planes, isosurfaces, contours */
    if ( desired_result == analy->result_id )
        update_vis( analy );
}


/*****************************************************************
 * TAG( interp_activity )
 *
 * Interpolate activity data between two states.  If an element is
 * inactive at either state, we mark it as inactive for the whole
 * interval.
 */
static void
interp_activity( state_a, state_b )
State *state_a;
State *state_b;
{
    float *activity_a, *activity_b;
    int cnt, i;

    /*
     * If element is inactive at either end of the interval, mark
     * it as inactive through the whole interval.
     */
    if ( state_a->bricks )
    {
        activity_a = state_a->bricks->activity;
        activity_b = state_b->bricks->activity;
        cnt = state_a->bricks->cnt;
        for ( i = 0; i < cnt; i++ )
            if ( activity_b[i] == 0.0 )
                activity_a[i] = 0.0;
    }
    if ( state_a->shells )
    {
        activity_a = state_a->shells->activity;
        activity_b = state_b->shells->activity;
        cnt = state_a->shells->cnt;
        for ( i = 0; i < cnt; i++ )
            if ( activity_b[i] == 0.0 )
                activity_a[i] = 0.0;
    }
    if ( state_a->beams )
    {
        activity_a = state_a->beams->activity;
        activity_b = state_b->beams->activity;
        cnt = state_a->beams->cnt;
        for ( i = 0; i < cnt; i++ )
            if ( activity_b[i] == 0.0 )
                activity_a[i] = 0.0;
    }
}


