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


