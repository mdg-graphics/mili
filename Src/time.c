/* $Id$ */
/* 
 * time.c - Routines which execute time-related commands (like animation).
 * 
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Jan 2 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */

#include <values.h>
#include "viewer.h"


/* Local routines. */
static void interp_activity();
static void write_nodal_mm_report();
static void write_elem_mm_report();

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
    static char *el_label[] = { "Node", "Beam", "Shell", "Brick" };

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
 * TAG( outmm )
 *
 * Prepare summary of minimums and maximums for specified time
 * states of currently (or specified) result.
 *
 * NOTE:  Similar to get_global_minmax
 *
 */
void
outmm( analy, tokens, token_cnt )
Analysis *analy;
char tokens[MAXTOKENS][TOKENLENGTH];
int token_cnt;
{
    FILE *outfile;
    int *beam_mat, *shell_mat, *hex_mat;
    int req_mat_qty;
    int *req_mats;
    Bool_type is_nodal, is_beam, is_shell, is_hex;
    Bool_type sorting;
    float *beam_res, *shell_res, *hex_res;
    int beam_qty, shell_qty, hex_qty;
    int mat, qty_states, idx, size, rm_idx;
    int max_rows;
    int *mat_flags;
    float *mins, *maxs;
    int *min_ids, *max_ids;
    float val;
    int cur_state;
    int i, j;
    
    if ( analy->result_id == VAL_NONE )
    {
        popup_dialog( INFO_POPUP, "%s\n%s",
                      "Please specify a current result with \"show\",",
                      "then retry \"outmm\"." );
        return;
    }

    /*
     * Filter input data
     */

    if ( (outfile = fopen( tokens[1], "w")) == 0 )
    {
        popup_dialog( WARNING_POPUP, "Unable to open output file %s.\n", 
                      tokens[1] );
        return;
    }
    
    if ( is_numeric_token( tokens[1] ) )
        popup_dialog( INFO_POPUP, "Writing outmm output to file \"%s\".",
                      tokens[1] );
    
    if ( token_cnt > 2 )
    {
        req_mat_qty = token_cnt - 2;
        req_mats = NEW_N( int, req_mat_qty, "Requested outmm materials" );
        
        rm_idx = 0;
        for ( i = 2; i < token_cnt; i++ )
        {
            req_mats[rm_idx] = atoi( tokens[i] ) - 1;
            
            if ( req_mats[rm_idx] >= 0 
                 && req_mats[rm_idx] < analy->num_materials )
                rm_idx++;
            else
                popup_dialog( WARNING_POPUP,
                              "Material %s out of range; ignored.", tokens[i] );
        }
     
        if ( rm_idx == 0 )
        {
            popup_dialog( WARNING_POPUP, "%s\n%s",
                          "Unable to successfully parse any materials.",
                          "Aborting \"outmm\" command." );
            free( req_mats );
            fclose( outfile );
            return;
        }

        req_mat_qty = rm_idx;
        
        /* Bubble sort to ensure material numbers are in ascending order. */
        sorting = TRUE;
        while ( sorting )
        {
            sorting = FALSE;
            for ( i = 1; i < req_mat_qty; i++ )
            {
                if ( req_mats[i - 1] > req_mats[i] )
                {
                    SWAP( val, req_mats[i - 1], req_mats[i] );
                    sorting = TRUE;
                }
            }
        }
    }
    else
    {
        /* Initialize requested materials array to all materials. */
        req_mat_qty = analy->num_materials;
        req_mats = NEW_N( int, req_mat_qty, "Requested outmm all materials" );
        for ( i = 0; i < req_mat_qty; i++ )
            req_mats[i] = i;
    }
    
    qty_states = analy->num_states;

    /* Set mesh object source flags. */
    is_nodal = is_nodal_result( analy->result_id );
    is_beam = is_beam_result( analy->result_id )
              && analy->geom_p->beams != NULL;
    is_shell = ( is_shell_result( analy->result_id )
                 || is_shared_result( analy->result_id ) )
               && analy->geom_p->shells != NULL;
    is_hex = ( is_hex_result( analy->result_id )
               || is_shared_result( analy->result_id ) )
             && analy->geom_p->bricks != NULL;
    
    /* Init local references for improved performance. */
    if ( is_beam )
    {
        beam_mat = analy->geom_p->beams->mat;
        beam_res = analy->beam_result;
        beam_qty = analy->geom_p->beams->cnt;
    }

    if ( is_shell )
    {
        shell_mat = analy->geom_p->shells->mat;
        shell_res = analy->shell_result;
        shell_qty = analy->geom_p->shells->cnt;
    }

    if ( is_hex )
    {
        hex_mat = analy->geom_p->bricks->mat;
        hex_res = analy->hex_result;
        hex_qty = analy->geom_p->bricks->cnt;
    }

    /*
     * Allocate tables.  For nodal results, we don't discriminate by
     * material so only one per-state array is required.  For element
     * results, need one per-state array per material.
     */
    if ( is_nodal )
    {
        max_rows = 1;
        size = analy->num_states;
        mins = NEW_N( float, size, "Nodal min table" );
        maxs = NEW_N( float, size, "Nodal max table" );
        min_ids = NEW_N( int, size, "Nodal min id table" );
        max_ids = NEW_N( int, size, "Nodal max id table" );
    }
    else
    {
        max_rows = analy->num_materials;
        size = max_rows * analy->num_states;
        mins = NEW_N( float, size, "Nodal min table" );
        maxs = NEW_N( float, size, "Nodal max table" );
        min_ids = NEW_N( int, size, "Nodal min id table" );
        max_ids = NEW_N( int, size, "Nodal max id table" );
        mat_flags = NEW_N( Bool_type, max_rows, "Mat use flags" );
    }
     
    /* Initialize min and max value arrays. */
    for ( i = 0; i < size; i++ )
    {
        mins[i] = MAXFLOAT;
        maxs[i] = -MAXFLOAT;
    }
     
    /* Initialize material use flags. */
    if ( !is_nodal )
    {   
        if ( is_beam )
            for ( i = 0; i < beam_qty; i++ )
                mat_flags[beam_mat[i]] = BEAM_T;
        
        if ( is_shell )
            for ( i = 0; i < shell_qty; i++ )
                mat_flags[shell_mat[i]] = SHELL_T;
        
        if ( is_hex )
            for ( i = 0; i < hex_qty; i++ )
                mat_flags[hex_mat[i]] = BRICK_T;
    }

    /* retain current state data */
    cur_state = analy->cur_state;

    /* Loop over states to fill tables. */
    for ( i = 0; i < analy->num_states; i++ )
    {
        analy->cur_state = i;
        analy->state_p   = get_state( i, analy->state_p );

        /* Update result */
        load_result( analy, TRUE );

        if ( is_nodal )
        {
            mins[i] = analy->state_mm[0];
            maxs[i] = analy->state_mm[1];
            min_ids[i] = analy->state_mm_nodes[0];
            max_ids[i] = analy->state_mm_nodes[1];
        }
        else
        {
            if ( is_beam )
            {
                for ( j = 0; j < beam_qty; j++ )
                {
                    mat = beam_mat[j];
                    idx = mat * qty_states + i;
                    val = beam_res[j];

                    if ( val < mins[idx] )
                    {
                        mins[idx] = val;
                        min_ids[idx] = j + 1;
                    }
                    if ( val > maxs[idx] )
                    {
                        maxs[idx] = val;
                        max_ids[idx] = j + 1;
                    }
                }
            }

            if ( is_shell )
            {
                for ( j = 0; j < shell_qty; j++ )
                {
                    mat = shell_mat[j];
                    idx = mat * qty_states + i;
                    val = shell_res[j];

                    if ( val < mins[idx] )
                    {
                        mins[idx] = val;
                        min_ids[idx] = j + 1;
                    }
                    if ( val > maxs[idx] )
                    {
                        maxs[idx] = val;
                        max_ids[idx] = j + 1;
                    }
                }
            }

            if ( is_hex )
            {
                for ( j = 0; j < hex_qty; j++ )
                {
                    mat = hex_mat[j];
                    idx = mat * qty_states + i;
                    val = hex_res[j];

                    if ( val < mins[idx] )
                    {
                        mins[idx] = val;
                        min_ids[idx] = j + 1;
                    }
                    if ( val > maxs[idx] )
                    {
                        maxs[idx] = val;
                        max_ids[idx] = j + 1;
                    }
                }
            }
        }
    }
     
    /* Convert data units if requested. */   
    if ( analy->perform_unit_conversion )
    {
        float scale, offset;
        
        scale = analy->conversion_scale;
        offset = analy->conversion_offset;

        if ( is_nodal )
        {
            for ( i = 0; i < qty_states; i++ )
            {
                mins[i] = (scale * mins[i]) + offset;
                maxs[i] = (scale * maxs[i]) + offset;
            }
        }
        else
        {
            float *mat_min_row, *mat_max_row;
            
            mat_min_row = mins;
            mat_max_row = maxs;
            rm_idx = 0;

            for ( i = 0; i < max_rows && rm_idx < req_mat_qty; i++ )
            {
                /* Increment material index to next requested material. */
                while ( i < req_mats[rm_idx] )
                {
                    i++;
                    mat_min_row += qty_states;
                    mat_max_row += qty_states;
                }

                if ( mat_flags[i] )
                {
                    for ( j = 0; j < qty_states; j++ )
                    {
                        mat_min_row[j] = (scale * mat_min_row[j]) + offset;
                        mat_max_row[j] = (scale * mat_max_row[j]) + offset;
                    }
                }
                
                mat_min_row += qty_states;
                mat_max_row += qty_states;
                rm_idx++;
            }
        }
    }
    
    /*
     * Output.
     */

    if ( is_nodal )
        write_nodal_mm_report( analy, outfile, mins, min_ids, maxs, max_ids );
    else
        write_elem_mm_report( analy, outfile, mins, min_ids, maxs, max_ids,
                              is_beam, is_shell, is_hex, mat_flags, 
                              req_mat_qty, req_mats );
    
    fclose( outfile );
    
    free( req_mats );
    free( mins );
    free( maxs );
    free( min_ids );
    free( max_ids );

    /*
     * Return to original status
     */
    analy->cur_state = cur_state;
    analy->state_p = get_state( analy->cur_state, analy->state_p );
    load_result( analy, TRUE );
}


/*****************************************************************
 * TAG( write_nodal_mm_report )
 *
 * Output to text file min/max data for nodal result.
 */
static void
write_nodal_mm_report( analy, outfile, mins, min_ids, maxs, max_ids )
Analysis *analy;
FILE *outfile;
float *mins;
int *min_ids;
float *maxs;
int *max_ids;
{
    int qty_states;
    int i, min_st, max_st, limit_st; 
    int ident_width, node_label_width, node_pre;
    float minval, maxval;
    char *node_label = "Node";
    char *min_label = " <min", *max_label = " <<<max";
    char *std_out_format = " %5d  %12.6e %10.3e %*d  %10.3e %*d\n";
    char *mm_out_format = " %5d  %12.6e %10.3e %*d  %10.3e %*d%s";
    char *blanks = "                    ";
    Bool_type min_first;
    
    qty_states = analy->num_states;
    
    /* Determine states with min and max values. */
    minval = mins[0];
    maxval = maxs[0];
    min_st = max_st = 0;
    for ( i = 1; i < qty_states; i++ )
    {
        if ( mins[i] < minval )
        {
            minval = mins[i];
            min_st = i;
        }
        
        if ( maxs[i] > maxval )
        {
            maxval = maxs[i];
            max_st = i;
        }
    }

    min_first = ( min_st <= max_st ) ? TRUE : FALSE;
    
    /* Dump preamble. */
    fprintf( outfile, "# Report of nodal %s min/max values.\n",
             trans_result[resultid_to_index[analy->result_id]][1] );
    write_preamble( outfile );
     
    /* Calculate field width parameters. */   
    ident_width = (int) ((double) 1.0 
                         + log10( (double) analy->geom_p->nodes->cnt ));
    node_label_width = strlen( node_label );
    if ( ident_width < node_label_width )
        ident_width = node_label_width;
    
    node_pre = (ident_width - node_label_width + 1) / 2;
    
    /* Dump column headers. */
    fprintf( outfile, "# State     Time       Maximum%.*s%s%.*sMinimum%.*s%s\n",
             2 + node_pre, blanks, node_label, 
             ident_width - node_label_width - node_pre + 4, blanks,
             2 + node_pre, blanks, node_label );

    /* Dump states before the first min or max extreme. */
    limit_st = min_first ? min_st : max_st;
    for ( i = 0; i < limit_st; i++ )
        fprintf( outfile, std_out_format,
                 i + 1, analy->state_times[i], maxs[i], ident_width, max_ids[i],
                 mins[i], ident_width, min_ids[i] );
    
    /* Dump the min or max extreme which occurs first. */
    fprintf( outfile, mm_out_format,
             i + 1, analy->state_times[i], maxs[i], ident_width, max_ids[i],
             mins[i], ident_width, min_ids[i], 
             (min_first ? min_label : max_label) );
    if ( min_st == max_st )
        fprintf( outfile, " %s\n", max_label );
    else
        fprintf( outfile, "\n" );
    i++;
    
    /* Dump the states after first extreme but before the second extreme. */
    limit_st = min_first ? max_st : min_st;
    for ( ; i < limit_st; i++ )
        fprintf( outfile, std_out_format,
                 i + 1, analy->state_times[i], maxs[i], ident_width, max_ids[i],
                 mins[i], ident_width, min_ids[i] );
    
    /* Dump the second min or max extreme. */
    if ( min_st != max_st )
    {
        fprintf( outfile, mm_out_format,
                 i + 1, analy->state_times[i], maxs[i], ident_width, max_ids[i],
                 mins[i], ident_width, min_ids[i], 
                 (min_first ? max_label : min_label) );
        fprintf( outfile, "\n" );
        i++;
    }
    
    /* Dump the states after the second extreme. */
    for ( ; i < qty_states; i++ )
        fprintf( outfile, std_out_format,
                 i + 1, analy->state_times[i], maxs[i], ident_width, max_ids[i],
                 mins[i], ident_width, min_ids[i] );
}


/*****************************************************************
 * TAG( write_elem_mm_report )
 *
 * Output to text file min/max data for element result.
 */
static void
write_elem_mm_report( analy, outfile, mins, min_ids, maxs, max_ids, is_beam, 
                      is_shell, is_hex, mat_flags, req_mat_qty, req_mats )
Analysis *analy;
FILE *outfile;
float *mins;
int *min_ids;
float *maxs;
int *max_ids;
Bool_type is_beam;
Bool_type is_shell;
Bool_type is_hex;
int *mat_flags;
int req_mat_qty;
int *req_mats;
{
    int qty_states;
    int i, j, min_st, max_st, rm_idx, limit_st, max_rows; 
    int ident_width, elem_label_width, node_pre;
    float minval, maxval;
    char *header = 
        "# Material  State     Time       Maximum%.*s%s%.*sMinimum%.*s%s\n";
    char *elem_label = "Elem";
    char *min_label = " <min", *max_label = " <<<max";
    char *std_out_format = "   %4d    %5d  %12.6e %10.3e %*d  %10.3e %*d\n";
    char *mm_out_format = 
        "   %4d    %5d  %12.6e %10.3e %*d  %10.3e %*d%s";
    char *blanks = "                    ";
    Bool_type min_first;
    float *mat_min_row, *mat_max_row;
    int *mat_min_id, *mat_max_id;
    int beam_qty, shell_qty, hex_qty, max_qty;
    
    qty_states = analy->num_states;
    max_rows = analy->num_materials;

    /* Dump preamble. */
    fprintf( outfile, 
             "# Report of %s min/max values %s.\n",
             trans_result[resultid_to_index[analy->result_id]][1],
             (( max_rows == req_mat_qty ) 
              ? "by material" : "on selected materials") );
    write_preamble( outfile );
     
    /* Calculate field width parameters. */
    beam_qty = is_beam ? analy->geom_p->beams->cnt : 0;
    shell_qty = is_shell ? analy->geom_p->shells->cnt : 0;
    hex_qty = is_hex ? analy->geom_p->bricks->cnt : 0;
    max_qty = MAX( beam_qty, MAX( shell_qty, hex_qty ) );
    
    ident_width = (int) ((double) 1.0 + log10( (double) max_qty ));

    elem_label_width = strlen( elem_label );
    if ( ident_width < elem_label_width )
        ident_width = elem_label_width;
    
    node_pre = (ident_width - elem_label_width + 1) / 2;
    
    /* Loop over materials, dumping all states per requested material. */
    mat_min_row = mins;
    mat_max_row = maxs;
    mat_min_id = min_ids;
    mat_max_id = max_ids;
    rm_idx = 0;

    for ( i = 0; i < max_rows && rm_idx < req_mat_qty; i++ )
    {
        /* Increment material index to next requested material. */
        while ( i < req_mats[rm_idx] )
        {
            i++;
            mat_min_row += qty_states;
            mat_min_id += qty_states;
            mat_max_row += qty_states;
            mat_max_id += qty_states;
        }

        if ( mat_flags[i] )
        {
            /* Dump column headers. */
            fprintf( outfile, "#\n#\n" );
            fprintf( outfile, header,
                     2 + node_pre, blanks, elem_label, 
                     ident_width - elem_label_width - node_pre + 4, blanks,
                     2 + node_pre, blanks, elem_label );
    
            /* Determine states with min and max values. */
            minval = mat_min_row[0];
            maxval = mat_max_row[0];
            min_st = max_st = 0;
            for ( j = 1; j < qty_states; j++ )
            {
                if ( mat_min_row[j] < minval )
                {
                    minval = mat_min_row[j];
                    min_st = j;
                }
                
                if ( mat_max_row[j] > maxval )
                {
                    maxval = mat_max_row[j];
                    max_st = j;
                }
            }

            min_first = ( min_st <= max_st ) ? TRUE : FALSE;

            /* Dump states before the first min or max extreme. */
            limit_st = min_first ? min_st : max_st;
            for ( j = 0; j < limit_st; j++ )
                fprintf( outfile, std_out_format,
                         i + 1, j + 1, analy->state_times[j], mat_max_row[j], 
                         ident_width, mat_max_id[j], mat_min_row[j], 
                         ident_width, mat_min_id[j] );
            
            /* Dump the min or max extreme which occurs first. */
            fprintf( outfile, mm_out_format,
                     i + 1, j + 1, analy->state_times[j], mat_max_row[j], 
                     ident_width, mat_max_id[j], mat_min_row[j], ident_width, 
                     mat_min_id[j], (min_first ? min_label : max_label) );
            if ( min_st == max_st )
                fprintf( outfile, " %s\n", max_label );
            else
                fprintf( outfile, "\n" );
            j++;
            
            /* Dump states after first extreme but before second extreme. */
            limit_st = min_first ? max_st : min_st;
            for ( ; j < limit_st; j++ )
                fprintf( outfile, std_out_format,
                         i + 1, j + 1, analy->state_times[j], mat_max_row[j], 
                         ident_width, mat_max_id[j], mat_min_row[j], 
                         ident_width, mat_min_id[j] );
            
            /* Dump the second min or max extreme. */
            if ( min_st != max_st )
            {
                fprintf( outfile, mm_out_format,
                         i + 1, j + 1, analy->state_times[j], mat_max_row[j], 
                         ident_width, mat_max_id[j], mat_min_row[j], 
                         ident_width, mat_min_id[j], 
                         (min_first ? max_label : min_label) );
                fprintf( outfile, "\n" );
                j++;
            }
            
            /* Dump the states after the second extreme. */
            for ( ; j < qty_states; j++ )
                fprintf( outfile, std_out_format,
                         i + 1, j + 1, analy->state_times[j], mat_max_row[j], 
                         ident_width, mat_max_id[j], mat_min_row[j], 
                         ident_width, mat_min_id[j] );
        }
        
        mat_min_row += qty_states;
        mat_min_id += qty_states;
        mat_max_row += qty_states;
        mat_max_id += qty_states;
        rm_idx++;
    }
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


