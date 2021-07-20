/* $Id$ */
/*
 * time.c - Routines which execute time-related commands (like animation).
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Jan 2 1992
 *
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - Sept 15, 2004: Fixed problem with anim command when
 *   running from a batch script. The animation was stopping immediately
 *   because the next command was the stop command.
 *
 *  I. R. Corey - Dec 28, 2009: Fixed several problems releated to drawing
 *                ML particles.
 *                See SRC#648
 *
 *  I. R. Corey - Jan 17, 2013: Added test in change_state() and change_
 *                time() for zero states.
 *                See TeamForge#19786
 *
 *  I. R. Corey - Feb 08, 2013: Removed calculation for hex_vol_sums -
 *                no need with changes in hex_to_nodal().
 *
 ************************************************************************
 */

#include <stdlib.h>
#include <unistd.h>
#include "viewer.h"

/* #define DEBUG_TIME 1 */

/* Local routines. */
static void interp_activity( State2 *state_a, State2 *state_b );


/*****************************************************************
 * TAG( anim_info )
 *
 * Holds the animation state for the animation in progress.
 * W. B. Oliver - Apr 29, 2013: Added a delay member to anim_info -
 *                to allow the user to slow down the animation -
 *                default is normal time.
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
    State2 *state_a;
    State2 *state_b;
    float *result_a;
    float *result_b;
    unsigned int delay;
} anim_info;


/*****************************************************************
 * TAG( change_state )
 *
 * The things that need to be done when we move to a new state.
 */
void
change_state( Analysis *analy )
{
    Bool_type recompute_norms;
    int st_qty;
    int srec_id;
    int status=OK;
    static Bool_type warn_state = TRUE;

    if ( env.timing )
    {
        wrt_text( "Timing for change state...\n" );
        check_timing( 0 );
    }

    analy->db_get_state( analy, analy->cur_state, analy->state_p,
                         &analy->state_p, &st_qty );

    if ( st_qty==0 )
    {
        if ( warn_state )
        {
            popup_dialog( WARNING_POPUP, "%s\n%s\n%s",
                          "Cannot change state when",
                          "number of states is zero.",
                          "(This warning will not be repeated.)" );
            warn_state = FALSE;
        }
        return;
    }

    /* Update current mesh ident. */
    srec_id = analy->state_p->srec_id;
    analy->cur_mesh_id =
        analy->srec_tree[srec_id].subrecs[0].p_object_class->mesh_id;

    wrt_text( "State %d out of %d states.\n\n", analy->cur_state + 1, st_qty );

    /* If activity data then activity may have changed. */
    recompute_norms = FALSE;
    if ( analy->state_p->sand_present )
    {
        reset_face_visibility( analy );
        recompute_norms = TRUE;
    }

    /* Gotta recompute normals, unless node positions don't change. */
    if ( !analy->normals_constant || recompute_norms )
        compute_normals( analy );

    analy->show_interp = FALSE;

    /* If new mesh, may need to compute edge list. */
    if ( MESH( analy ).edge_list == NULL )
        get_mesh_edges( analy->cur_mesh_id, analy );

    /* Update displayed result. */
    load_result( analy, TRUE, TRUE, FALSE );

    /*
     * Update cut planes, isosurfs, contours.
     *
     * OK to call update_vis() directly, since change_state() doesn't apply
     * to plots.
     */
    update_vis( analy );

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
change_time( float time, Analysis *analy )
{
    State2 *state_a, *state_b;
    float *result_a, *result_b;
    float t_a, t_b, interp, ninterp;
    Bool_type recompute_norms;
    int st_num_a, st_num_b, st_num;
    int st_nums[2];
    int i;
    int st_qty;
    int node_qty;
    GVec2D *nodes2d_a, *nodes2d_b;
    GVec3D *nodes3d_a, *nodes3d_b;
    int interp_result, can_interp;
    int srec_id_a, srec_id_b;
    static Bool_type warn_once = TRUE, warn_time=TRUE;
    int mesh_id;
    /**
     *  Added to hold enough information to reset the values for the
     *  selected elements. 
     */
    struct temp_subrecord
    {
      int subrecord_id; 
      int obj_qty;
      int *object_ids;
      int data_size;
      float *data;
    };
    int subrec_qty= 0;
    Subrec_obj *p_subrec;
    struct temp_subrecord *subrecords_b =NULL;
    float *temp_results;
    int s1time, s2time = 0;

    /*
     * This routine should really go through and interpolate everything
     * in the database.
     */

    st_nums[0] = st_nums[1] = -1;
    analy->db_query( analy->db_ident, QRY_STATE_OF_TIME, (void *) &time,
                     NULL, (void *) st_nums );

    /*
     * Note output of QRY_STATE_OF_TIME query above (in st_nums[]) is on
     * [1, qty_states], but request to analy->db_get_state is on
     * [0, qty_states - 1].
     */

    st_num_b = ( st_nums[1] > 1 ) ? st_nums[1] - 1 : 1;
    st_num_a = st_num_b - 1;
    analy->db_get_state( analy, st_num_a, analy->state_p, &state_a, &st_qty );

    if ( st_qty==0 )
    {
        if ( warn_time )
        {
            popup_dialog( WARNING_POPUP, "%s\n%s\n%s",
                          "Cannot change time when",
                          "number of states is zero.",
                          "(This warning will not be repeated.)" );
            warn_time = FALSE;
        }
        return;
    }

    analy->db_get_state( analy, st_num_b, NULL, &state_b, NULL );
    analy->cur_state      = st_num_a;

    //SNAP code
    if(analy->use_snap){
    	if(abs(time - state_a->time) < abs(time - state_b->time)){
    		analy->cur_state = st_num_a;
    	}
    	else{
    		analy->cur_state = st_num_b;
    	}
    	change_state(analy);
    	return;
    }


    /* OK to interpolate results if state rec formats are the same. */
    st_num = st_num_a + 1;
    analy->db_query( analy->db_ident, QRY_SREC_FMT_ID, (void *) &st_num, NULL, &srec_id_a );

    st_num = st_num_b + 1;
    analy->db_query( analy->db_ident, QRY_SREC_FMT_ID, (void *) &st_num, NULL, &srec_id_b );

    can_interp = ( srec_id_a == srec_id_b );
    interp_result = ( analy->cur_result != NULL && can_interp );

    if ( srec_id_a != srec_id_b && warn_once )
    {
        popup_dialog( WARNING_POPUP, "%s\n%s\n%s",
                      "Griz does not interpolate results across",
                      "differing state record formats.",
                      "(This warning will not be repeated.)" );
        warn_once = FALSE;
    }

    analy->db_query( analy->db_ident, QRY_SREC_MESH, (void *) &srec_id_a, NULL, (void *) &mesh_id );

    node_qty = analy->mesh_table[mesh_id].node_geom->qty;
    
    
    /**
     * JKD reworked this as we were originally were setting the 
     * incorrect values expecting them to show up in the graphics.
     */
    if ( interp_result )
    {
    	analy->show_interp = TRUE;
        result_b = NEW_N( float, node_qty, "Interpolate result" );
        result_a = NODAL_RESULT_BUFFER( analy );
        analy->state_p = state_b;
        NODAL_RESULT_BUFFER( analy ) = result_b;
        analy->cur_state = st_num_b;
        load_result( analy, TRUE, TRUE, FALSE );
        subrec_qty = analy->cur_result->qty;
        subrecords_b = malloc(sizeof(struct temp_subrecord)* analy->cur_result->qty);
        for(i=0 ; i< analy->cur_result->qty;i++)
        {
            subrecords_b[i].subrecord_id = analy->cur_result->subrecs[i];
            p_subrec = analy->srec_tree[srec_id_b].subrecs + subrecords_b[i].subrecord_id;
            subrecords_b[i].obj_qty = p_subrec->subrec.qty_objects;
            subrecords_b[i].object_ids = malloc(sizeof(int)*p_subrec->subrec.qty_objects);
            if(p_subrec->object_ids)
            {
                subrecords_b[i].object_ids = memcpy((void*)subrecords_b[i].object_ids, 
                                              (void*)p_subrec->object_ids, 
                                              p_subrec->subrec.qty_objects*sizeof(int));
            }else
            {
                subrecords_b[i].object_ids = p_subrec->object_ids;
            }
            subrecords_b[i].data_size = p_subrec->p_object_class->qty;
            subrecords_b[i].data = malloc(sizeof(float)*subrecords_b[i].data_size);
            subrecords_b[i].data = memcpy((void*)subrecords_b[i].data, 
                                          (void*)p_subrec->p_object_class->data_buffer, 
                                          subrecords_b[i].data_size*sizeof(float));
        }
        analy->state_p = state_a;
        NODAL_RESULT_BUFFER( analy ) = result_a;
        analy->cur_state = st_num_a;
        load_result( analy, TRUE, TRUE, FALSE );
    }
    
    t_a = state_a->time;
    t_b = state_b->time;
    interp = (time-t_a) / (t_b-t_a);
    ninterp = 1.0 - interp;

    /*
     * Stash the interpolated nodal values in state_a and
     * the interpolated result values in result_a.
     */
    int max_obj,j;
    int *obj_ids;
    int obj_count;
    float *class_data_buffer;
    if ( analy->dimension == 3 )
    {
        nodes3d_a = state_a->nodes.nodes3d;
        nodes3d_b = state_b->nodes.nodes3d;

        for ( i = 0; i < node_qty; i++ )
        {
            nodes3d_a[i][0] = ninterp * nodes3d_a[i][0] + interp * nodes3d_b[i][0];
            nodes3d_a[i][1] = ninterp * nodes3d_a[i][1] + interp * nodes3d_b[i][1];
            nodes3d_a[i][2] = ninterp * nodes3d_a[i][2] + interp * nodes3d_b[i][2];

            if ( interp_result )
                result_a[i] = ninterp * result_a[i] + interp * result_b[i];

        }
        
        
    }
    else
    {
        nodes2d_a = state_a->nodes.nodes2d;
        nodes2d_b = state_b->nodes.nodes2d;

        for ( i = 0; i < node_qty; i++ )
        {
            nodes2d_a[i][0] = ninterp * nodes2d_a[i][0] + interp * nodes2d_b[i][0];
            nodes2d_a[i][1] = ninterp * nodes2d_a[i][1] + interp * nodes2d_b[i][1];

        }
    }
    
    
    if ( interp_result )
    {
       // The following commented linewas the original calculation extracted  
       // from the above if else statements.
       //result_a[i] = ninterp * result_a[i] + interp * result_b[i];
       for(i=0 ; i< analy->cur_result->qty;i++)
       {
           p_subrec = analy->srec_tree[srec_id_b].subrecs + analy->cur_result->subrecs[i];
           max_obj = p_subrec->subrec.qty_objects;
               
           class_data_buffer = p_subrec->p_object_class->data_buffer;
           for(j=0; j<max_obj; j++)
           {
               if(p_subrec->object_ids)
               {
                   class_data_buffer[p_subrec->object_ids[j]] = 
                       ninterp * class_data_buffer[p_subrec->object_ids[j]] + 
                       interp * subrecords_b[i].data[subrecords_b[i].object_ids[j]];
               }else
               {
                   class_data_buffer[j] =  ninterp * class_data_buffer[j] + 
                                           interp * subrecords_b[i].data[j];
               }       
           }
               
        }
    }
    
    
    for (i = 0; i< subrec_qty; i++)
    {
        free(subrecords_b[i].data);
        free(subrecords_b[i].object_ids);
    }
    
    free(subrecords_b);

#ifdef DEBUG_TIME
    int count=0;
    for (i=0;
            i<node_qty;
            i++)
    {
        if ( result_a[i]!=0.0 )
        {
            printf("\nTIME[%d] = %e/%e", i, result_b[i], result_a[i] );
            count++;
            if ( count>10 )
                break;
        }
    }
#endif

    analy->state_p->time = time;

    /* If activity data then activity may have changed. */
    recompute_norms = FALSE;
    if ( can_interp )
    {
        if ( state_a->sand_present )
        {
            interp_activity( state_a, state_b );
            reset_face_visibility( analy );
            recompute_norms = TRUE;
        }
    }

    /* Gotta recompute normals, unless node positions don't change. */
    if ( !analy->normals_constant || recompute_norms )
        compute_normals( analy );


    //NODAL_RESULT_BUFFER( analy ) = result_a;
    //load_result( analy, TRUE, TRUE, FALSE );

    if(!can_interp){
    	analy->show_interp = FALSE;
    }

    /*
     * Update cut planes, isosurfs, contours.
     *
     * OK to call update_vis() directly, since change_time() doesn't apply
     * to plots.
     */
    update_vis( analy );

    /* Clean up. */
    fr_state2( state_b, analy );
    if ( analy->cur_result != NULL )
        free( result_b );

    /*
     * Print the time and return so the caller can redisplay the mesh.
     */
    wrt_text( "t = %f\n\n", time );
}


/*****************************************************************
 * TAG( parse_animate_command )
 *
 * Parse the animation command and initialize an animation.  A
 * workproc is used to step the animation along.
 */
void
parse_animate_command( int token_cnt, char tokens[MAXTOKENS][TOKENLENGTH],
                       Analysis *analy )
{
    int max_state, min_state;
    int i_args[3];
    float time_bounds[2];

    max_state = get_max_state( analy );
    min_state = GET_MIN_STATE( analy );

    /* Sanity check - can't animate on zero or one states in db. */
    if ( max_state <= 0 )
        return;

    if ( token_cnt == 1 )
    {
        /* Just display all of the states in succesion. */
        anim_info.interpolate = FALSE;
        anim_info.cur_state = min_state;
        anim_info.last_state = max_state;

        if(env.direct_rendering) {
            /* When using direct rendering, add 80 milisecond delay between animation steps
             * so that each state in rendered.
             */
            anim_info.delay = 80 * 1000;
        }
        else {
            anim_info.delay = 0;
        }
    }
    else if ( token_cnt == 2 || token_cnt == 4 )
    {
        anim_info.interpolate = TRUE;

        i_args[0] = 2;
        i_args[1] = min_state + 1;
        i_args[2] = max_state + 1;
        analy->db_query( analy->db_ident, QRY_MULTIPLE_TIMES, (void *) i_args,
                         NULL, (void *) time_bounds );

        /* sscanf( tokens[1], "%d", &anim_info.num_frames ); */

        if ( token_cnt == 2 && strcmp(tokens[0], "animd") != 0)
        {
            /* The user input just the number of frames. */
            sscanf( tokens[1], "%d", &anim_info.num_frames);
            anim_info.start_time = time_bounds[0];
            anim_info.end_time = time_bounds[1];
        }
        else if (token_cnt == 2 && strcmp(tokens[0], "animd") == 0)
        {
            /* The user spedified a time delayed animation of all frames */
            sscanf(tokens[1], "%u", &anim_info.delay);
            anim_info.delay *= 1000;
            anim_info.interpolate = FALSE;
            anim_info.cur_state = min_state;
            anim_info.last_state = max_state;
        }
        else
        {
            /* The user input number of frames and start & end times. */
            sscanf( tokens[2], "%f", &anim_info.start_time );
            sscanf( tokens[3], "%f", &anim_info.end_time );

            /* Sanity checks. */
            if ( anim_info.start_time < time_bounds[0] )
                anim_info.start_time = time_bounds[0];
            if ( anim_info.end_time > time_bounds[1])
                anim_info.end_time = time_bounds[1];
        }
        anim_info.cur_frame = 0;

        /* Initialize the animation. */
        anim_info.st_num_a = min_state;
        anim_info.st_num_b = min_state + 1;

        analy->db_get_state( analy, anim_info.st_num_a, analy->state_p,
                             &anim_info.state_a, NULL );
        analy->db_get_state( analy, anim_info.st_num_b, NULL,
                             &anim_info.state_b, NULL );

        if ( analy->cur_result != NULL )
        {
            anim_info.result_a = NODAL_RESULT_BUFFER( analy );
            anim_info.result_b = NEW_N( float, analy->max_result_size,
                                        "Animate result" );
            analy->cur_state = anim_info.st_num_a;
            load_result( analy, TRUE, TRUE, FALSE );
            analy->state_p = anim_info.state_b;
            NODAL_RESULT_BUFFER( analy ) = anim_info.result_b;
            analy->cur_state = anim_info.st_num_b;
            load_result( analy, TRUE, TRUE, FALSE );
            analy->state_p = anim_info.state_a;
            NODAL_RESULT_BUFFER( analy ) = anim_info.result_a;
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
#ifdef SERIAL_BATCH
    while( !step_animate( analy ) );
#else

    /* Added Sept 15, 2004: IRC */
    if (env.history_input_active) /* If we are getting commands from
                                   * a file and we see an animate command,
                                   * then let it complete.
                                   */
        while( !step_animate( analy ) );
    else
        init_animate_workproc();

#endif /* SERIAL_BATCH */
}


/*****************************************************************
 * TAG( continue_animate )
 *
 * Start an animation from the current state to the last state.
 */
void
continue_animate( Analysis *analy )
{
    int max_state;

    max_state = get_max_state( analy );
    if ( max_state == -1 )
        return;

    /* Just display all of the states in succesion. */
    anim_info.last_state = max_state;
    anim_info.interpolate = FALSE;
    anim_info.cur_state = analy->cur_state;

    /* Quit any previous animation. */
    end_animate_workproc( TRUE );

    /* Fire up a work proc to step the animation forward. */
#ifdef SERIAL_BATCH
    while( !step_animate( analy ) );
#else
    init_animate_workproc();
#endif /* SERIAL_BATCH */
}



/*****************************************************************
 * TAG( step_animate )
 *
 * Move the animation forward one frame.
 */
Bool_type
step_animate( Analysis *analy )
{
    GVec2D *nodes_a2d, *nodes_b2d;
    GVec3D *nodes_a3d, *nodes_b3d;
    State2 *tmp_state;
    float *tmp_result;
    float start_t, end_t, t, t_a, t_b, interp, ninterp, next_t;
    Bool_type recompute_norms;
    int num_frames, node_qty;
    int i, j, k;
    int st_bounds[2];
    int rval;
    usleep(anim_info.delay);
    if ( !anim_info.interpolate )
    {
        /* Display the next state. */
        analy->cur_state = anim_info.cur_state;
        change_state( analy );
        if ( analy->center_view )
            center_view( analy );
        analy->update_display( analy );

        if ( analy->auto_img )
            output_image( analy );
        /*
        {
            sprintf( rgbfile, "%s%04d.rgb", analy->rgb_root,
                     anim_info.cur_state + 1 );
            screen_to_rgb( rgbfile, FALSE );
        }
        */

        anim_info.cur_state += get_step_stride();
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
            j = anim_info.st_num_b + 2;
            analy->db_query( analy->db_ident, QRY_STATE_TIME, (void *) &j, NULL,
                             (void *) &next_t );

            if ( t <= next_t )
            {
                anim_info.st_num_a = anim_info.st_num_b;
                anim_info.st_num_b++;
                tmp_state = anim_info.state_a;
                anim_info.state_a = anim_info.state_b;
                anim_info.state_b = tmp_state;
                analy->db_get_state( analy, anim_info.st_num_b,
                                     anim_info.state_b, &anim_info.state_b,
                                     NULL );

                if ( analy->cur_result != NULL )
                {
                    tmp_result = anim_info.result_a;
                    anim_info.result_a = anim_info.result_b;
                    anim_info.result_b = tmp_result;
                    analy->state_p = anim_info.state_b;
                    NODAL_RESULT_BUFFER( analy ) = anim_info.result_b;
                    analy->cur_state = anim_info.st_num_b;
                    load_result( analy, TRUE, TRUE, FALSE );
                    analy->state_p = anim_info.state_a;
                    NODAL_RESULT_BUFFER( analy ) = anim_info.result_a;
                    analy->cur_state = anim_info.st_num_a;
                }
            }
            else
            {
                rval = analy->db_query( analy->db_ident, QRY_STATE_OF_TIME,
                                        (void *) &t, NULL, (void *) st_bounds );
                if ( rval != 0
                        || st_bounds[1] - 1 > get_max_state( analy ) )
                {
                    popup_dialog( WARNING_POPUP,
                                  "Animation failure with bad time." );
                    end_animate_workproc( FALSE );
                    return( TRUE );
                }
                anim_info.st_num_a = st_bounds[0] - 1;
                anim_info.st_num_b = st_bounds[1] - 1;
                analy->db_get_state( analy, anim_info.st_num_a,
                                     anim_info.state_a, &anim_info.state_a,
                                     NULL );
                analy->db_get_state( analy, anim_info.st_num_b,
                                     anim_info.state_b, &anim_info.state_b,
                                     NULL );

                if ( analy->cur_result != NULL )
                {
                    analy->cur_state = anim_info.st_num_a;
                    load_result( analy, TRUE, TRUE, FALSE );
                    analy->state_p = anim_info.state_b;
                    NODAL_RESULT_BUFFER( analy ) = anim_info.result_b;
                    analy->cur_state = anim_info.st_num_b;
                    load_result( analy, TRUE, TRUE, FALSE );
                    analy->state_p = anim_info.state_a;
                    NODAL_RESULT_BUFFER( analy ) = anim_info.result_a;
                    analy->cur_state = anim_info.st_num_a;
                }
            }
        }
        else
        {
            /* State_a has been messed up with the interpolated
             * values so we must reget it.
             */
            analy->db_get_state( analy, anim_info.st_num_a,
                                 anim_info.state_a, &anim_info.state_a, NULL );

            if ( analy->cur_result != NULL )
                load_result( analy, TRUE, TRUE, FALSE );
        }

        /* If a-b swapping occurred, need to fix up. */
        analy->cur_state = anim_info.st_num_a;
        analy->state_p = anim_info.state_a;
        if ( analy->cur_result != NULL )
            NODAL_RESULT_BUFFER( analy ) = anim_info.result_a;

        t_a = anim_info.state_a->time;
        t_b = anim_info.state_b->time;
        interp = (t-t_a) / (t_b-t_a);
        ninterp = 1.0 - interp;

        /*
         * Stash the interpolated nodal values in state_a and
         * the interpolated result values in result_a.
         */
        node_qty = MESH( analy ).node_geom->qty;
        if ( analy->dimension == 3 )
        {
            nodes_a3d = anim_info.state_a->nodes.nodes3d;
            nodes_b3d = anim_info.state_b->nodes.nodes3d;

            for ( j = 0; j < node_qty; j++ )
            {
                for ( k = 0; k < 3; k++ )
                    nodes_a3d[j][k] = ninterp * nodes_a3d[j][k]
                                      + interp * nodes_b3d[j][k];
                if ( analy->cur_result != NULL )
                    anim_info.result_a[j] = ninterp * anim_info.result_a[j]
                                            + interp * anim_info.result_b[j];
            }
        }
        else
        {
            nodes_a2d = anim_info.state_a->nodes.nodes2d;
            nodes_b2d = anim_info.state_b->nodes.nodes2d;

            for ( j = 0; j < node_qty; j++ )
            {
                for ( k = 0; k < 2; k++ )
                    nodes_a2d[j][k] = ninterp * nodes_a2d[j][k]
                                      + interp * nodes_b2d[j][k];
                if ( analy->cur_result != NULL )
                    anim_info.result_a[j] = ninterp * anim_info.result_a[j]
                                            + interp * anim_info.result_b[j];
            }
        }

        anim_info.state_a->time = t;

        /* If activity data then activity may have changed. */
        recompute_norms = FALSE;
        if ( anim_info.state_a->sand_present )
        {
            interp_activity( anim_info.state_a, anim_info.state_b );
            reset_face_visibility( analy );
            recompute_norms = TRUE;
        }

        /* Gotta recompute normals, unless node positions don't change. */
        if ( !analy->normals_constant || recompute_norms )
            compute_normals( analy );

        /*
         * Update cut planes, isosurfs, contours.
         *
         * OK to call update_vis() directly, since step_animate() doesn't apply
         * to plots.
         */
        update_vis( analy );

        if ( analy->center_view )
            center_view( analy );

        /*
         * Redraw the mesh.
         */
        analy->update_display( analy );

        if ( analy->auto_img )
            output_image( analy );
        /*if ( analy->auto_rgb )
        {
            sprintf( rgbfile, "%s%04d.rgb", analy->rgb_root,
                     anim_info.cur_frame + 1 );
            screen_to_rgb( rgbfile, FALSE );
        }
        */

        /* Increment frame number. */
        anim_info.cur_frame++;
        if ( anim_info.cur_frame >= anim_info.num_frames )
        {
            /* Finish on the next state. */
            if ( analy->cur_state < get_max_state( analy ) )
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
end_animate( Analysis *analy )
{
    if ( anim_info.interpolate )
    {
        fr_state2( anim_info.state_b, analy );
        if ( anim_info.result_b != NULL )
            free( anim_info.result_b );

        change_state( analy );
        analy->update_display( analy );

        if ( analy->auto_img )
            output_image( analy );
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
interp_activity( State2 *state_a, State2 *state_b )
{
    float *sand_a, *sand_b;
    int obj_qty, qty, i, j;
    Subrec_obj *p_so;
    int elem_class_index;
    MO_class_data *p_mocd;

    /*
     * Ahem.  We really want to just loop over the elem_class_sand
     * array in the State2 struct to interpolate each sand array.  But,
     * we only know the size of those arrays from the mesh object class data.
     * So, we'll traverse the Subrec_obj's for the current state record
     * format to get at the mesh object class data, and from that get
     * the array sizes where sand flags exist.
     */

    p_so = env.curr_analy->srec_tree[state_a->srec_id].subrecs;
    qty = env.curr_analy->srec_tree[state_a->srec_id].qty;

    for ( i = 0; i < qty; i++ )
    {
        if ( !p_so[i].sand )
            continue;

        p_mocd = p_so[i].p_object_class;
        obj_qty = p_mocd->qty;
        elem_class_index = p_mocd->elem_class_index;

        sand_a = state_a->elem_class_sand[elem_class_index];
        sand_b = state_b->elem_class_sand[elem_class_index];

        for ( j = 0; j < obj_qty; j++ )
            if ( sand_b[i] == 0.0 )
                sand_a[i] = 0.0;
    }
}


