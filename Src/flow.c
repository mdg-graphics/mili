/* $Id$ */
/*
 * flow.c - Fluid flow visualization techniques.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Jan 11 1993
 *
 * Copyright (c) 1993 Regents of the University of California
 */

#include "viewer.h"
#include "draw.h"

#define MAX_TRACER_STEPS 5000


/* Local routines. */
static Bool_type pt_in_hex_bbox();
static Bool_type pt_in_quad_bbox();
static void init_trace_points();
static void follow_particles();
static void vel_at_time();
static void save_trace();
static void init_vec_points_3d();
static void init_vec_points_2d();
static float ramp_random();
static void rand_bary_pt();
static void gen_iso_carpet_points();
static void gen_grid_carpet_points();
static void real_heapsort();

/* Local data. */
static char *trace_tag = "Particle trace";
static char *points_tag = "Points: ";
static char *color_tag = "Color: ";
static char *mtls_trav_tag = "Materials traversed: ";
static char *pos_his_hdr_tag = 
            "Time         X              Y              Z";


/************************************************************
 * TAG( pt_in_hex_bbox )
 *
 * Test whether a point is within the bounding box of a brick
 * element.  Returns TRUE for within and FALSE for not in.
 */
static Bool_type
pt_in_hex_bbox( verts, pt )
float verts[8][3];
float pt[3];
{
    int i, j;
    float min[3];
    float max[3];

    for ( i = 0; i < 3; i++ )
    {
        min[i] = verts[0][i];
        max[i] = verts[0][i];
    }

    for ( i = 1; i < 8; i++ )
        for ( j = 0; j < 3; j++ )
        {
            if ( verts[i][j] < min[j] )
                min[j] = verts[i][j];
            else if ( verts[i][j] > max[j] )
                max[j] = verts[i][j];
        }

    for ( i = 0; i < 3; i++ )
        if ( DEF_LT( pt[i], min[i] ) || DEF_GT( pt[i], max[i] ) )
            return FALSE;
    return TRUE;
}


/************************************************************
 * TAG( pt_in_quad_bbox )
 *
 * Test whether a point is within the bounding box of a 2D
 * quadrilateral element.  Returns TRUE for within and FALSE
 * for not in.
 */
static Bool_type
pt_in_quad_bbox( verts, pt )
float verts[4][2];
float pt[2];
{
    int i, j;
    float min[2];
    float max[2];

    min[0] = verts[0][0];
    max[0] = verts[0][0];
    min[1] = verts[0][1];
    max[1] = verts[0][1];

    for ( i = 1; i < 4; i++ )
        for ( j = 0; j < 2; j++ )
        {
            if ( verts[i][j] < min[j] )
                min[j] = verts[i][j];
            else if ( verts[i][j] > max[j] )
                max[j] = verts[i][j];
        }

    for ( i = 0; i < 2; i++ )
        if ( DEF_LT( pt[i], min[i] ) || DEF_GT( pt[i], max[i] ) )
            return FALSE;

    return TRUE;
}


/*
 * SECTION( Particle tracing routines )
 */


/************************************************************
 * TAG( gen_trace_points )
 *
 * Generate a rake of point locations for particle tracing.
 */
void
gen_trace_points( num, start_pt, end_pt, color, analy )
int num;
float start_pt[3];
float end_pt[3];
float color[3];
Analysis *analy;
{
    Trace_pt_def_obj *pt, *ptlist;
    float t;
    int i, j;

    ptlist = NULL;

    for ( i = 0; i < num; i++ )
    {
        pt = NEW( Trace_pt_def_obj, "Trace pt definition object" );
        if ( num == 1 )
            t = 0.0;
        else
            t = i / (float)( num - 1 );
        for ( j = 0; j < 3; j++ )
            pt->pt[j] = (1.0-t)*start_pt[j] + t*end_pt[j];
        VEC_COPY( pt->color, color );
        INSERT( pt, ptlist );
    }

    /* Locate the element that each point initially lies in. */
    init_trace_points( &ptlist, analy );
    
    if ( ptlist != NULL )
        APPEND( ptlist, analy->trace_pt_defs );
}


/************************************************************
 * TAG( init_trace_points )
 *
 * For each point in the list, find the element that the point
 * initially resides in.  Points that don't lie in any element
 * are deleted from the list.
 */
static void
init_trace_points( ptlist, analy )
Trace_pt_def_obj **ptlist;
Analysis *analy;
{
    Hex_geom *bricks;
    Nodal *nodes;
    Trace_pt_def_obj *pt, *nextpt;
    Bool_type found;
    float verts[8][3];
    float xi[3];
    int i, j, k, el, nd;

    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;

    for ( pt = *ptlist; pt != NULL; )
    {
        found = FALSE;

        for ( i = 0; i < analy->num_blocks; i++ )
        {
            /* See if the point lies in the block's bounding box first. */
            if ( pt->pt[0] < analy->block_bbox[0][0][i] ||
                 pt->pt[1] < analy->block_bbox[0][1][i] ||
                 pt->pt[2] < analy->block_bbox[0][2][i] ||
                 pt->pt[0] > analy->block_bbox[1][0][i] ||
                 pt->pt[1] > analy->block_bbox[1][1][i] ||
                 pt->pt[2] > analy->block_bbox[1][2][i] )
                continue;

            /* Test point against elements in block. */
            for ( el = analy->block_lo[i]; el <= analy->block_hi[i]; el++ )
            {
                for ( j = 0; j < 8; j++ )
                {
                    nd = bricks->nodes[j][el];
                    for ( k = 0; k < 3; k++ )
                        verts[j][k] = nodes->xyz[k][nd];
                }

                /* Perform approximate inside test first. */
                if ( pt_in_hex_bbox( verts, pt->pt ) &&
                     pt_in_hex( verts, pt->pt, xi ) )
                {
                    pt->elem = el;
                    VEC_COPY( pt->xi, xi );
		    pt->material = bricks->mat[el];
                    found = TRUE;
                    pt = pt->next;
                    break;
                }
            }

            if ( found )
                break;
        }

        if ( !found )
        {
            /* The point wasn't in any bricks, so delete it. */
            wrt_text( "Point not in any bricks: %f %f %f.\n",
                     pt->pt[0], pt->pt[1], pt->pt[2] );
            nextpt = pt->next;
            DELETE( pt, *ptlist );
            pt = nextpt;
        }
    }
}


/************************************************************
 * TAG( free_trace_points )
 *
 * Delete the particle trace points.
 */
void
free_trace_points( analy )
Analysis *analy;
{
    Trace_pt_obj *pt;

    /* Free up the memory allocated for the particle paths. */
    for ( pt = analy->trace_pts; pt != NULL; pt = pt->next )
    {
        free( pt->pts );
        free( pt->time );
	DELETE_LIST( pt->mtl_segments );
    }

    DELETE_LIST( analy->trace_pts );
    
    DELETE_LIST( analy->trace_pt_defs );
}


/************************************************************
 * TAG( init_new_traces )
 *
 * Duplicate the "trace_pt_defs" list as a list of Trace_pt_obj
 * structures to prepare for a new trace integration.
 */
static Bool_type
init_new_traces( analy )
Analysis *analy;
{
    Trace_pt_def_obj *p_tpd;
    Trace_pt_obj *p_tp;
    
    if ( analy->new_trace_pts != NULL )
    {
	popup_dialog( WARNING_POPUP, "New traces already active!" );
	return FALSE;
    }
    
    for ( p_tpd = analy->trace_pt_defs; p_tpd != NULL; p_tpd = p_tpd->next )
    {
	p_tp = NEW( Trace_pt_obj, "Trace point object" );
	
	p_tp->elnum = p_tpd->elem;
	p_tp->last_elem = p_tpd->elem;
	p_tp->mtl_num = p_tpd->material;
	VEC_COPY( p_tp->pt, p_tpd->pt );
	VEC_COPY( p_tp->color, p_tpd->color );
	VEC_COPY( p_tp->xi, p_tpd->xi );
        p_tp->pts = NEW_N( float, 3*MAX_TRACER_STEPS, "Trace points" );
        p_tp->time = NEW_N( float, MAX_TRACER_STEPS, "Trace point times" );
	
        APPEND( p_tp, analy->new_trace_pts );
    }

    return ( analy->new_trace_pts != NULL ) ? TRUE : FALSE;
}


/************************************************************
 * TAG( follow_particles )
 *
 * Using the current particle positions, find the element that
 * each particle lies in and the natural coordinates for the
 * particle location.  This routine "walks" from element to
 * element until it finds the enclosing element.  Then the 
 * velocity of the particle is updated by interpolating from
 * the nodes of the enclosing element.
 */
static void
follow_particles( plist, vel, analy, follow_mtl, point_cnt )
Trace_pt_obj *plist;
float *vel[3];
Analysis *analy;
Bool_type follow_mtl;
int point_cnt;
{
    Hex_geom *bricks;
    Nodal *nodes;
    int **hex_adj;
    Trace_pt_obj *pc;
    Bool_type found;
    float verts[8][3];
    float xi[3];
    float h[8];
    int nd, el, face;
    int i, j;
    Trace_segment_obj *p_ts;

    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;
    hex_adj = analy->hex_adj;

    for ( pc = plist; pc != NULL; pc = pc->next )
    {
        /* Check to see we've bounced out of the grid already. */
        if ( !pc->in_grid )
            continue;

        /* Get the vertices of the element. */
        el = pc->elnum;
        for ( i = 0; i < 8; i++ )
        {
            nd = bricks->nodes[i][el];
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes->xyz[j][nd];
        }

        /* Test whether new point is inside this element. */
        if ( pt_in_hex( verts, pc->pt, xi ) )
        {
            VEC_COPY( pc->xi, xi );
            found = TRUE;
        }
        else
            found = FALSE;

        /* 
	 * Move till we find an element that the particle is inside of or
         * we bounce out of the grid.
         */
        while ( !found )
        {
            face = -1;
            if ( xi[0] > 1.0 && hex_adj[0][el] >= 0 )
                face = 0;
            else if ( xi[1] > 1.0 && hex_adj[1][el] >= 0 )
                face = 1;
            else if ( xi[0] < -1.0 && hex_adj[2][el] >= 0 )
                face = 2;
            else if ( xi[1] < -1.0 && hex_adj[3][el] >= 0 )
                face = 3;
            else if ( xi[2] > 1.0 && hex_adj[4][el] >= 0 )
                face = 4;
            else if ( xi[2] < -1.0 && hex_adj[5][el] >= 0 )
                face = 5;

            if ( face >= 0 )
            {
                /* Move to next element. */

                el = hex_adj[face][el];

                for ( i = 0; i < 8; i++ )
                {
                    nd = bricks->nodes[i][el];
                    for ( j = 0; j < 3; j++ )
                        verts[i][j] = nodes->xyz[j][nd];
                }

                if ( pt_in_hex( verts, pc->pt, xi ) )
                {
                    /* Found the desired element. */

                    pc->elnum = el;
                    VEC_COPY( pc->xi, xi );
                    found = TRUE;
                }
            }
            else
            {
                /* We have bounced out of the grid. */

                pc->in_grid = FALSE;
                found = TRUE;
		
		/* Always add a material trace segment for the last segment. */
		p_ts = NEW( Trace_segment_obj, "Trace segment" );
		p_ts->material = pc->mtl_num;
		p_ts->last_pt_idx = point_cnt - 1;
		APPEND( p_ts, pc->mtl_segments );
            }
        }
		    
        /*
        * If monitoring material transitions, check new material
        * against previous material and add a list entry for
        * last material's trace segment if there's a change.
        */
        if ( follow_mtl && ( el != pc->last_elem ) && pc->in_grid )
        {
	    /*
	     * "pc->elnum" is not suitable for this comparison because
	     * it may be updated during either call to follow_particles
	     * which occurs during a single position integration 
	     * calculation, and only the element validated during the
	     * first call is guaranteed to be the correct element
	     * for the current position.  However, this element may
	     * actually have been identified during either of the calls
	     * that occurred while the current position was being
	     * integrated, so this test must happen outside of the
	     * logic which identifies/validates the element number.  As
	     * long as "follow_mtl" is TRUE only during the first
	     * follow_particles call and pc->last_elem is only updated
	     * when "follow_mtl" is TRUE, this logic will correctly 
	     * capture material transitions.
	     */
	    pc->last_elem = el;
			
	    if ( bricks->mat[el] != pc->mtl_num )
	    {
	        p_ts = NEW( Trace_segment_obj, "Trace segment" );
	        p_ts->material = pc->mtl_num;
		
		/*
		 * If point_cnt is "i", we're validating the element for
		 * point "i-1" and the last point for the PREVIOUS 
		 * material is "i-2".
		 */
	        p_ts->last_pt_idx = point_cnt - 2;
	        APPEND( p_ts, pc->mtl_segments );
	        pc->mtl_num = bricks->mat[el];
	    }
        }

        /* Update the velocities at the particle. */
        if ( pc->in_grid )
        {
            shape_fns_3d( pc->xi[0], pc->xi[1], pc->xi[2], h );

            VEC_SET( pc->vec, 0.0, 0.0, 0.0 );
            for ( i = 0; i < 8; i++ )
            {
                nd = bricks->nodes[i][pc->elnum];
                for ( j = 0; j < 3; j++ )
                    pc->vec[j] += h[i]*vel[j][nd];
            }
        }
    }
}


/************************************************************
 * TAG( vel_at_time )
 *
 * Retrieve the nodal velocities for the specified time.
 */
static void
vel_at_time( time, vel, analy )
float time;
float *vel[3];
Analysis *analy;
{
    Result_type tmp_id;
    float *vec1, *vec2;
    float t1, t2, interp;
    int node_cnt, tmp_state, state1, state2;
    int i, j;

    vec1 = analy->tmp_result[0];
    vec2 = analy->tmp_result[1];
    node_cnt = analy->geom_p->nodes->cnt;
    tmp_id = analy->result_id;
    tmp_state = analy->cur_state;

    /* Find the states which sandwich the desired time. */
    state1 = 1;
    while ( time > analy->state_times[state1] )
        state1++;
    state2 = state1 - 1;

    t1 = analy->state_times[state1];
    t2 = analy->state_times[state2];
    interp = (time-t1) / (t2-t1);

    for ( i = 0; i < 3; i++ )
    {
        analy->result_id = VAL_NODE_VELX + i;
        analy->cur_state = state1;
        compute_node_velocity( analy, vec1 );
        analy->cur_state = state2;
        compute_node_velocity( analy, vec2 );

        for ( j = 0; j < node_cnt; j++ )
            vel[i][j] = (1.0-interp)*vec1[j] + interp*vec2[j];
    }

    analy->result_id = tmp_id;
    analy->cur_state = tmp_state;
}


/************************************************************
 * TAG( particle_trace )
 *
 * Trace out particle paths.
 */
void
particle_trace( t_zero, t_stop, delta_t, analy, static_field )
float t_zero;
float t_stop;
float delta_t;
Analysis *analy;
Bool_type static_field;
{
    Trace_pt_obj *pc, *p_tp;
    Trace_segment_obj *p_ts;
    float *vel[3];
    float t, beg_t, end_t;
    int node_cnt, cnt, state;
    int i;

    analy->show_traces = TRUE;
/*
    analy->render_mode = RENDER_NONE;
*/
    analy->show_edges = TRUE;

    /* Bounds check on start time. */
    beg_t = analy->state_times[0];
    end_t = analy->state_times[analy->num_states-1];
    t_zero = BOUND( beg_t, t_zero, end_t );

    /* Initialize particles. */
    if ( !init_new_traces( analy ) )
        return;
    for ( pc = analy->new_trace_pts; pc != NULL; NEXT( pc ) )
    {
        for ( i = 0; i < 3; i++ )
            pc->pts[i] = pc->pt[i];
        pc->time[0] = t_zero;
        pc->cnt = 1;
        pc->in_grid = TRUE;
    }

    /* Move to the start state; this is to take care of other display stuff.
     * Ie vectors, isosurfs, & etc should get updated during the animation.
     */
    state = 0;
    while ( state < analy->num_states-1 &&
            t_zero >= analy->state_times[state+1] )
        state++;
    analy->cur_state = state;
    change_state( analy );

    /* When necessary to calculate delta_t, do so differently for static and
     * dynamic flow fields.
     */
    if ( static_field )
    {
	if ( delta_t <= 0.0 || delta_t > t_stop )
	    delta_t = t_stop * 0.01;
	wrt_text( "Static field particle trace, using delta t of %f.\n",
		  delta_t );
    }
    else
    {
        t_stop = BOUND( beg_t, t_stop, end_t );

    	/* Guess delta_t if a value has not been provided. */
        if ( delta_t <= 0.0 )
            delta_t = 0.3 * (end_t - beg_t) / analy->num_states;
        wrt_text( "Particle trace, using delta t of %f.\n", delta_t );
    }

    /* Setup. */
    node_cnt = analy->geom_p->nodes->cnt;
    for ( i = 0; i < 3; i++ )
        vel[i] = NEW_N( float, node_cnt, "Velocities" );

    /* Initialize nodal velocities. */
    t = ( static_field ) ? 0.0 : t_zero;
    vel_at_time( t_zero, vel, analy );

    /* Integration loop. */
    cnt = 1;
    while ( cnt < MAX_TRACER_STEPS && analy->new_trace_pts != NULL )
    {
        /*
         * We use second-order Runge-Kutta integration to integrate
         * the particle velocities over time, which generates the
         * the particle paths.
         */

        if ( t + delta_t > t_stop )
            break;

        follow_particles( analy->new_trace_pts, vel, analy, TRUE, cnt );

        /* Get P1. */
	pc = analy->new_trace_pts;
	while ( pc != NULL )
        {
            if ( pc->in_grid )
	    {
                for ( i = 0; i < 3; i++ )
                    pc->pt[i] += delta_t*pc->vec[i];
		NEXT( pc );
	    }
	    else
	    {
	        /* Remove from "under construction" list. */
		p_tp = pc;
		NEXT( pc );
		UNLINK( p_tp, analy->new_trace_pts );
		APPEND( p_tp, analy->trace_pts );
	    }
        }

        t += delta_t;
        if ( !static_field ) vel_at_time( t, vel, analy );

        follow_particles( analy->new_trace_pts, vel, analy, FALSE, cnt );

        /* Get P2 and Pn+1. */
	pc = analy->new_trace_pts;
	while ( pc != NULL )
        {
            if ( pc->in_grid )
            {
                for ( i = 0; i < 3; i++ )
                {
                    pc->pt[i] = 0.5*( pc->pt[i] + delta_t*pc->vec[i] +
                                      pc->pts[(cnt-1)*3+i] );
                    pc->pts[cnt*3+i] = pc->pt[i];
                }
                pc->time[cnt] = ( static_field ) ? t_zero : t;
                pc->cnt++;
		NEXT( pc );
            }
	    else
	    {
	        /* Remove from "under construction" list. */
		p_tp = pc;
		NEXT( pc );
		UNLINK( p_tp, analy->new_trace_pts );
		APPEND( p_tp, analy->trace_pts );
	    }
        }

        cnt++;

        /* Redraw the display. */
        analy->state_p->time = ( static_field ) ? t_zero : t;
        update_display( analy );
    }

    if ( cnt >= MAX_TRACER_STEPS )
    {
        popup_dialog( WARNING_POPUP, 
	              "Exhausted room for ptrace; limit to %d steps.", 
	              MAX_TRACER_STEPS );
    }

    /* Terminate material segment lists; free unused trace memory. */
    for ( pc = analy->new_trace_pts; pc != NULL; NEXT( pc ) )
    {
	/* 
	 * Add last material segment for particles which
	 * terminated within the grid.
	 */
	p_ts = NEW( Trace_segment_obj, "Last trace segment" );
	p_ts->material = pc->mtl_num;
	p_ts->last_pt_idx = pc->cnt - 1;
	APPEND( p_ts, pc->mtl_segments );
/**/
/* Can't tell if this has any effect by watching gr_osview... */	
	/* Free unused space. */
/*
	pc->pts = (float *) realloc( pc->pts, 3 * sizeof( float ) * pc->cnt );
	pc->time = (float *) realloc( pc->time, sizeof( float ) * pc->cnt );
*/
    }
    
    /* Add remaining new traces to existing traces. */
    if ( analy->new_trace_pts != NULL )
    {
	APPEND( analy->new_trace_pts, analy->trace_pts );
	analy->new_trace_pts = NULL;
    }

    /* Free up memory. */
    for ( i = 0; i < 3; i++ )
        free( vel[i] );

    if ( static_field )
        /* Leave time at static field trace time. */
        change_time( t_zero, analy );
    else
    {
        /* Move to the last state. */
        while ( state < analy->num_states-1 &&
                t >= analy->state_times[state+1] )
            state++;
	    
        if ( state != analy->cur_state )
        {
            analy->cur_state = state;
            change_state( analy );
        }
        else
            analy->state_p->time = analy->state_times[analy->cur_state];
    }
}


/************************************************************
 * TAG( find_next_subtrace )
 *
 * Find the first contiguous sequence of steps of a particle 
 * trace subset for which all steps are in materials that
 * aren't disabled for particle traces.  When "init" is true, 
 * initialize the trace subset to those trace steps bounded 
 * by "start" and "start + qty".  Thereafter, consider only 
 * steps in the initial subset which have not belonged to a 
 * previous subtrace.
 *
 * The net effect of a sequence of calls to find_next_subtrace
 * is to partition a trace subset into a disconnected sequence 
 * of trace subsets separated by materials for which traces 
 * are disabled.
 */
Bool_type
find_next_subtrace( init, skip, qty, pt, analy, new_skip, new_qty )
Bool_type init;
int skip;
int qty;
Trace_pt_obj *pt;
Analysis *analy;
int *new_skip;
int *new_qty;
{
    static int start, end;
    static int *vis_map;
    int i, stop;
    Bool_type disabled;
    Trace_segment_obj *p_seg;
    
    /*
     * On initialization, build an array of flags, one per trace step
     * point, which are FALSE when the point lies in a material which
     * is trace-disabled.
     */
    if ( init )
    {
        end = skip + qty;
	
	vis_map = NEW_N( Bool_type, end, "Trace vis map" );
	
	i = 0;
	for ( p_seg = pt->mtl_segments; p_seg != NULL; NEXT( p_seg ) )
	{
	    disabled = is_in_iarray( p_seg->material, analy->trace_disable_qty, 
	                             analy->trace_disable );
	    for ( ; i <= p_seg->last_pt_idx && i < end; i++ )
	        vis_map[i] = ( disabled ) ? FALSE : TRUE;
	}
	
	/*
	 * If the trace is under construction, the length will be longer
	 * than the steps accounted for in the material segments list.
	 * Pick up the "tail" steps here.
	 */
	if ( end > i )
	{
	    disabled = is_in_iarray( pt->mtl_num, analy->trace_disable_qty, 
	                             analy->trace_disable );
	    for ( ; i < end; i++ )
	        vis_map[i] = ( disabled ) ? FALSE : TRUE;
	}
	
	start = skip;
    }
    
    /* Find the first step after (or equal to) "start" which is visible. */
    while ( start < end && !vis_map[start] )
        start++;

    if ( start == end )
    {
        /* No visible subtrace left. */
        free( vis_map );
	vis_map = NULL;
        return FALSE;
    }

    /* 
     * Traverse only _CONTIGUOUS_ visible steps after "start" and 
     * find the last one. 
     */
    stop = start + 1;
    while ( stop < end && vis_map[stop] )
        stop++;

    if ( stop == (start + 1) )
    {
        /* No visible subtrace left. */
        free( vis_map );
	vis_map = NULL;
        return FALSE;
    }

    /* Return the end points of the visible subtrace. */
    *new_skip = start;
    *new_qty = stop - start;
    
    /* Prepare for next iteration. */
    start = stop;
    
    return TRUE;
}


/*****************************************************************
 * TAG( save_particle_traces )
 * 
 * Output ASCII file of particle traces.
 */
void
save_particle_traces( analy, filename )
Analysis *analy;
char *filename;
{
    Trace_pt_obj *p_tp;
    FILE *ofile;
    int tcnt;
    
    if ( analy->trace_pts == NULL )
        return;

    if ( (ofile = fopen( filename, "w" )) == NULL )
    {
	popup_dialog( WARNING_POPUP, 
	              "Unable to open particle trace output file." );
	return;
    }
    
    write_preamble( ofile );

    /* Count and output the number of traces. */    
    tcnt = 0;
    for ( p_tp = analy->trace_pts; p_tp != NULL; NEXT( p_tp ) )
        tcnt++;
    fprintf( ofile, "# Quantity particle traces: %d\n", tcnt );
    
    for ( p_tp = analy->trace_pts; p_tp != NULL; NEXT( p_tp ) )
        save_trace( p_tp, ofile );

    fclose( ofile );
    
    return;
}


/*****************************************************************
 * TAG( save_trace )
 * 
 * Output ASCII file of particle traces.
 * 
 * Modifications to the format of this output require changes to
 * read_particle_traces() as well!!!
 */
static void
save_trace( p_trace, ofile )
Trace_pt_obj *p_trace;
FILE *ofile;
{
    Trace_segment_obj *p_ts;
    float *p_xyz, *p_time;
    int i, mcnt;
    
    fprintf( ofile, "#\n#                  %s\n#\n", trace_tag );
    fprintf( ofile, "# Points: %d\n", p_trace->cnt );
    fprintf( ofile, "# %s%5.3f  %5.3f  %5.3f\n", color_tag, 
             p_trace->color[0], p_trace->color[1], p_trace->color[2] );
    
    mcnt = 0;
    for ( p_ts = p_trace->mtl_segments; p_ts != NULL; NEXT( p_ts ) )
        mcnt++;
    
    fprintf( ofile, "# %s%d\n", mtls_trav_tag, mcnt );
    fprintf( ofile, "#      Material   Last point\n" );
    for ( p_ts = p_trace->mtl_segments; p_ts != NULL; NEXT( p_ts ) )
        fprintf( ofile, "#       %3d        %d\n", p_ts->material + 1, 
	         p_ts->last_pt_idx );

    fprintf( ofile, "# Position history:\n" );
    fprintf( ofile, "#        %s\n", pos_his_hdr_tag );
    
    p_xyz = p_trace->pts;
    p_time = p_trace->time;
    for ( i = 0; i < p_trace->cnt; i++ )
    {
	fprintf( ofile, "    %8.6e %9.6e  %9.6e  %9.6e\n", *p_time, p_xyz[0], 
	         p_xyz[1], p_xyz[2] );
        p_time++;
	p_xyz += 3;
    }
}


/*****************************************************************
 * TAG( read_particle_traces )
 * 
 * Read the particle trace file.
 * 
 * The format read MUST be kept current with output from
 * save_particle_traces() and save_trace().
 */
void
read_particle_traces( analy, filename )
Analysis *analy;
char *filename;
{
    FILE *ifile;
    char *p_c;
    Trace_pt_obj *p_tp;
    Trace_segment_obj *p_ts;
    int i, j, mcnt;
    float *p_time, *p_xyz;
    char txtline[80];
    
    if ( (ifile = fopen( filename, "r" )) == NULL )
    {
	popup_dialog( WARNING_POPUP, "Unable to open file \"%s\".", filename );
	return;
    }
    
    /* Locate each particle trace dataset and load a Trace_pt_obj. */
    while ( find_text( ifile, trace_tag ) != NULL )
    {
        p_tp = NEW( Trace_pt_obj, "Trace obj from file" );
	
	/* Quantity of points in trace. */
	p_tp->cnt = atoi( find_text( ifile, points_tag ) + strlen( points_tag) );
	
	/* RGB values for trace color. */
	p_c = find_text( ifile, color_tag ) + strlen( color_tag );
	for ( i = 0; i < 3; i++ )
	    p_tp->color[i] = (float) strtod( p_c, &p_c );
	
	/* Traversed materials list. */
	mcnt = atoi( find_text( ifile, mtls_trav_tag ) 
	             + strlen( mtls_trav_tag ) );
	/* Swallow the column titles. */
	fgets( txtline, 80, ifile );
	
	/* Now read each material and its last point. */
	for ( i = 0; i < mcnt; i++ )
	{
	    p_ts = NEW( Trace_segment_obj, "Trace seg from file" );
	    fscanf( ifile, "# %d %d\n", &p_ts->material, 
	            &p_ts->last_pt_idx );
	    p_ts->material--;
	    APPEND( p_ts, p_tp->mtl_segments );
	}
	
	/* Find the position history. */
	p_c = find_text( ifile, pos_his_hdr_tag );
	
	/* Allocate arrays to hold the position history. */
        p_tp->pts = NEW_N( float, 3*p_tp->cnt, "Trace points from file" );
        p_tp->time = NEW_N( float, p_tp->cnt, "Trace point times from file" );
	
	/* Read the position history. */
        p_xyz = p_tp->pts;
        p_time = p_tp->time;
	for ( i = 0; i < p_tp->cnt; i++ )
	{
	    fscanf( ifile, " %f %f %f %f\n", p_time, p_xyz, 
	            p_xyz + 1, p_xyz + 2 );
	    p_time++;
	    p_xyz += 3;
	}
	
	/* Store the Trace_pt_obj. */
	APPEND( p_tp, analy->trace_pts );
    }
    
    fclose( ifile );
    
    /* Enable trace rendering. */
    analy->show_traces = TRUE;
}


/*
 * SECTION( Vector grid routines )
 */


/************************************************************
 * TAG( gen_vec_points )
 *
 * Generate a grid of point locations at which to display
 * vectors.
 *
 * dim              - (1, 2 or 3) Generate a linear, planar
 *                    or volume grid.
 * size             - Number of values in each direction.
 * start_pt, end_pt - Boundaries of region.
 */
void
gen_vec_points( dim, size, start_pt, end_pt, analy )
int dim;
int size[3];
float start_pt[3];
float end_pt[3];
Analysis *analy;
{
    Vector_pt_obj *pt, *ptlist;
    float t;
    int ind1, ind2, inds;
    int i, j, k;

    ptlist = NULL;
    switch ( dim )
    {
        case 1:
            for ( i = 0; i < size[0]; i++ )
            {
                pt = NEW( Vector_pt_obj, "Vector point object" );
                if ( size[0] == 1 )
                    t = 0.0;
                else
                    t = i / (float)( size[0] - 1 );
                for ( j = 0; j < 3; j++ )
                    pt->pt[j] = (1.0-t)*start_pt[j] + t*end_pt[j];
                INSERT( pt, ptlist );
            }
            break;
        case 2:
            ind1 = 0;
            ind2 = 2;
            inds = 1;
            if ( start_pt[0] == end_pt[0] )
            {
                ind1 = 1;
                inds = 0;
            }
            else if ( start_pt[2] == end_pt[2] )
            {
                ind2 = 1;
                inds = 2;
            }
            for ( i = 0; i < size[0]; i++ )
                for ( j = 0; j < size[1]; j++ )
                {
                    pt = NEW( Vector_pt_obj, "Vector point object" );

                    t = i / (float)( size[0] - 1 );
                    pt->pt[ind1] = (1.0-t)*start_pt[ind1] + t*end_pt[ind1];

                    t = j / (float)( size[1] - 1 );
                    pt->pt[ind2] = (1.0-t)*start_pt[ind2] + t*end_pt[ind2];

                    pt->pt[inds] = start_pt[inds];

                    INSERT( pt, ptlist );
                }
            break;
        case 3:
            for ( i = 0; i < size[0]; i++ )
                for ( j = 0; j < size[1]; j++ )
                    for ( k = 0; k < size[2]; k++ )
                    {
                        pt = NEW( Vector_pt_obj, "Vector point object" );

                        t = i / (float)( size[0] - 1 );
                        pt->pt[0] = (1.0-t)*start_pt[0] + t*end_pt[0];

                        t = j / (float)( size[1] - 1 );
                        pt->pt[1] = (1.0-t)*start_pt[1] + t*end_pt[1];

                        t = k / (float)( size[2] - 1 );
                        pt->pt[2] = (1.0-t)*start_pt[2] + t*end_pt[2];

                        INSERT( pt, ptlist );
                    }
            break;
        default:
            popup_dialog( WARNING_POPUP, 
	                  "gen_vec_points(): bad switch statement value." );
    }

    /* Locate the element that each point lies in. */
    if ( analy->dimension == 3 )
        init_vec_points_3d( &ptlist, analy );
    else
        init_vec_points_2d( &ptlist, analy );

    if ( ptlist != NULL )
        APPEND( ptlist, analy->vec_pts );

    /* Update the display vectors if needed. */
    update_vec_points( analy );
}


/************************************************************
 * TAG( init_vec_points_3d )
 *
 * For each point in the list, find the element that the point
 * initially resides in.  Points that don't lie in any element
 * are deleted from the list.  This routine is for brick 
 * elements.
 */
static void
init_vec_points_3d( ptlist, analy )
Vector_pt_obj **ptlist;
Analysis *analy;
{
    Hex_geom *bricks;
    Nodal *nodes;
    Vector_pt_obj *pt, *nextpt;
    Bool_type found;
    float verts[8][3];
    float *lo[3], *hi[3];
    float xi[3];
    float val;
    int i, j, k, el, nd;

    if ( analy->geom_p->bricks == NULL )
        return;

    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;

    /* Compute a bounding box for each element in advance. */
    for ( i = 0; i < 3; i++ )
    {
        lo[i] = analy->tmp_result[i];
        hi[i] = analy->tmp_result[i+3];
    }

    for ( i = 0; i < bricks->cnt; i++ )
    {
        nd = bricks->nodes[0][i];
        for ( k = 0; k < 3; k++ )
        {
            lo[k][i] = nodes->xyz[k][nd];
            hi[k][i] = nodes->xyz[k][nd];
        }

        for ( j = 1; j < 8; j++ )
        {
            nd = bricks->nodes[j][i];
            for ( k = 0; k < 3; k++ )
            {
                val = nodes->xyz[k][nd];
                if ( val < lo[k][i] )
                    lo[k][i] = val;
                else if ( val > hi[k][i] )
                    hi[k][i] = val;
            }
        }
    }

    /* Now do the point tests. */
    for ( pt = *ptlist; pt != NULL; )
    {
        found = FALSE;

        for ( i = 0; i < analy->num_blocks; i++ )
        {
            /* See if the point lies in the block's bounding box first. */
            if ( pt->pt[0] < analy->block_bbox[0][0][i] ||
                 pt->pt[1] < analy->block_bbox[0][1][i] ||
                 pt->pt[2] < analy->block_bbox[0][2][i] ||
                 pt->pt[0] > analy->block_bbox[1][0][i] ||
                 pt->pt[1] > analy->block_bbox[1][1][i] ||
                 pt->pt[2] > analy->block_bbox[1][2][i] )
                continue;

            /* Test point against elements in block. */
            for ( el = analy->block_lo[i]; el <= analy->block_hi[i]; el++ )
            {
                /* Perform approximate inside (bbox) test first. */
                if ( pt->pt[0] >= lo[0][el] && pt->pt[0] <= hi[0][el] &&
                     pt->pt[1] >= lo[1][el] && pt->pt[1] <= hi[1][el] &&
                     pt->pt[2] >= lo[2][el] && pt->pt[2] <= hi[2][el] )
                {
                    for ( j = 0; j < 8; j++ )
                    {
                        nd = bricks->nodes[j][el];
                        for ( k = 0; k < 3; k++ )
                            verts[j][k] = nodes->xyz[k][nd];
                    }

                    if ( pt_in_hex( verts, pt->pt, xi ) )
                    {
                        pt->elnum = el;
                        VEC_COPY( pt->xi, xi );
                        found = TRUE;
                        pt = pt->next;
                        break;
                    }
                }
            }

            if ( found )
                break;
        }

        if ( !found )
        {
            /* The point wasn't in any bricks, so delete it. */
#ifdef DEBUG
            fprintf( stderr, "Point not in any bricks: %f %f %f.\n",
                     pt->pt[0], pt->pt[1], pt->pt[2] );
#endif
            nextpt = pt->next;
            DELETE( pt, *ptlist );
            pt = nextpt;
        }
    }
}


/************************************************************
 * TAG( init_vec_points_2d )
 *
 * For each point in the list, find the element that the point
 * initially resides in.  Points that don't lie in any element
 * are deleted from the list.  This routine is for shell
 * elements.
 */
static void
init_vec_points_2d( ptlist, analy )
Vector_pt_obj **ptlist;
Analysis *analy;
{
    Shell_geom *shells;
    Nodal *nodes;
    Vector_pt_obj *pt, *nextpt;
    Bool_type found;
    float verts[4][2];
    float xi[2];
    int i, j, k, el, nd;

    if ( analy->geom_p->shells == NULL )
        return;

    shells = analy->geom_p->shells;
    nodes = analy->state_p->nodes;

    for ( pt = *ptlist; pt != NULL; )
    {
        found = FALSE;

        for ( i = 0; i < analy->num_blocks; i++ )
        {
            /* See if the point lies in the block's bounding box first. */
            if ( pt->pt[0] < analy->block_bbox[0][0][i] ||
                 pt->pt[1] < analy->block_bbox[0][1][i] ||
                 pt->pt[0] > analy->block_bbox[1][0][i] ||
                 pt->pt[1] > analy->block_bbox[1][1][i] )
                continue;

            /* Test point against elements in block. */
            for ( el = analy->block_lo[i]; el <= analy->block_hi[i]; el++ )
            {
                for ( j = 0; j < 4; j++ )
                {
                    nd = shells->nodes[j][el];
                    for ( k = 0; k < 3; k++ )
                        verts[j][k] = nodes->xyz[k][nd];
                }

                /* Perform approximate inside test first. */
                if ( pt_in_quad_bbox( verts, pt->pt ) &&
                     pt_in_quad( verts, pt->pt, xi ) )
                {
                    pt->elnum = el;
                    pt->xi[0] = xi[0];
                    pt->xi[1] = xi[1];
                    found = TRUE;
                    pt = pt->next;
                    break;
                }
            }

            if ( found )
                break;
        }

        if ( !found )
        {
            /* The point wasn't in any shells, so delete it. */
#ifdef DEBUG
            fprintf( stderr, "Point not in any shells: %f %f.\n",
                     pt->pt[0], pt->pt[1] );
#endif
            nextpt = pt->next;
            DELETE( pt, *ptlist );
            pt = nextpt;
        }
    }
}


/************************************************************
 * TAG( update_vec_points )
 *
 * For each point in the vector point list, update the displayed
 * vector at that point.
 */
void
update_vec_points( analy )
Analysis *analy;
{
    Hex_geom *bricks;
    Shell_geom *shells;
    Nodal *nodes;
    Vector_pt_obj *pt;
    Result_type tmp_id;
    float h[8], vec[3];
    float *tmp_res;
    float *vec_result[3];
    float vmag, vmin, vmax;
    Bool_type init;
    int i, j, nd, node_cnt;

    if ( analy->show_vectors == FALSE || analy->vec_pts == NULL )
        return;

    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;

    if ( ( analy->dimension == 3 && bricks == NULL ) ||
         ( analy->dimension == 2 && shells == NULL ) )
        return;

    node_cnt = analy->state_p->nodes->cnt;

    /* Load the three results into vector result array. */
    for ( i = 0; i < 3; i++ )
        vec_result[i] = NEW_N( float, node_cnt, "Vec result" );

    tmp_id = analy->result_id;
    tmp_res = analy->result;
    for ( i = 0; i < 3; i++ )
    {
        analy->result_id = analy->vec_id[i];
        analy->result = vec_result[i];
        load_result( analy, FALSE );
    }
    analy->result_id = tmp_id;
    analy->result = tmp_res;

    /* Get the min and max magnitude for the vector result. */
/*
 * Can do this up here or down below.  If done up here, the min/max is
 * based on _all_ nodal points rather than just the vector grid points.
 * There are advantages and disadvantages to either method.
 */
    init = FALSE;
/*
    for ( i = 0; i < node_cnt; i++ )
    {
        vec[0] = vec_result[0][i];
        vec[1] = vec_result[1][i];
        vec[2] = vec_result[2][i];
        vmag = VEC_DOT( vec, vec );

        if ( init )
        {
            if ( vmag > vmax )
                vmax = vmag;
            else if ( vmag < vmin )
                vmin = vmag;
        }
        else
        {
            vmin = vmag;
            vmax = vmag;
            init = TRUE;
        }
    }
    analy->vec_min_mag = sqrt( (double)vmin );
    analy->vec_max_mag = sqrt( (double)vmax );
*/

    /* Interpolate the nodal vectors to get the vectors at the
     * grid points.
     */
    for ( pt = analy->vec_pts; pt != NULL; pt = pt->next )
    {
        VEC_SET( pt->vec, 0.0, 0.0, 0.0 );

        if ( analy->dimension == 3 )
        {
            /* 3D data. */
            shape_fns_3d( pt->xi[0], pt->xi[1], pt->xi[2], h );

            for ( i = 0; i < 8; i++ )
            {
                nd = bricks->nodes[i][pt->elnum];
                for ( j = 0; j < 3; j++ )
                    pt->vec[j] += h[i]*vec_result[j][nd];
            }
        }
        else
        {
            /* 2D data. */
            shape_fns_2d( pt->xi[0], pt->xi[1], h );

            for ( i = 0; i < 4; i++ )
            {
                nd = shells->nodes[i][pt->elnum];
                for ( j = 0; j < 3; j++ )
                    pt->vec[j] += h[i]*vec_result[j][nd];
            }
        }

        vmag = VEC_DOT( pt->vec, pt->vec );
        if ( init )
        {
            if ( vmag > vmax )
                vmax = vmag;
            else if ( vmag < vmin )
                vmin = vmag;
        }
        else
        {
            vmin = vmag;
            vmax = vmag;
            init = TRUE;
        }
    }

    analy->vec_min_mag = sqrt( (double)vmin );
    analy->vec_max_mag = sqrt( (double)vmax );

    for ( i = 0; i < 3; i++ )
        free( vec_result[i] );
}


/*
 * SECTION( Vector carpet routines )
 */


/************************************************************
 * TAG( ramp_random )
 *
 * Generate a random coordinate with a linear ramped
 * probability distribution.
 */
static float
ramp_random()
{
    float g, h;

    /* Probability distribution is a linear ramp with probability
     * of one for val == 0 and probability of zero for val == 1.
     * That is, Probability(V) = 1 - V where V is in (0,1).
     */
    while ( TRUE )
    {
        g = drand48();
        h = drand48();

        if ( h > g )
            return g;
    }
}


/************************************************************
 * TAG( rand_bary_pt )
 *
 * Given a triangle embedded in an element, generate a random
 * point on the triangle and return the natural coordinates of
 * the point with respect to the element.
 */
static void
rand_bary_pt( tri_vert, hex_vert, xi )
float tri_vert[3][3];
float hex_vert[8][3];
float xi[3];
{
    float r, s, t, pt[3];
    int i;

    /* Get barycentric coordinates of point on triangle. */
    r = ramp_random();
    s = (1.0 - r)*drand48();
    t = 1.0 - r - s;

    /* Get physical coordinates of point. */
    for ( i = 0; i < 3; i++ )
        pt[i] = r*tri_vert[0][i] + s*tri_vert[1][i] + t*tri_vert[2][i];

    /* Get natural coordinates of point on element. */
    if ( !pt_in_hex( hex_vert, pt, xi ) )
    {
        xi[0] = BOUND( -1.0, xi[0], 1.0 );
        xi[1] = BOUND( -1.0, xi[1], 1.0 );
        xi[2] = BOUND( -1.0, xi[2], 1.0 );
    }
}


/************************************************************
 * TAG( gen_carpet_points )
 *
 * Generate carpet points for all triangles on the cut planes
 * and on the isosurfaces if displayed, otherwise generate
 * carpet on the grid itself.
 */
void
gen_carpet_points( analy )
Analysis *analy;
{
    int i;

    /* Free any existing carpet points. */
    if ( analy->vol_carpet_cnt > 0 )
    {
        for ( i = 0; i < 3; i++ )
            free( analy->vol_carpet_coords[i] );
        free( analy->vol_carpet_elem );
        analy->vol_carpet_cnt = 0;
    }
    if ( analy->shell_carpet_cnt > 0 )
    {
        for ( i = 0; i < 2; i++ )
            free( analy->shell_carpet_coords[i] );
        free( analy->shell_carpet_elem );
        analy->shell_carpet_cnt = 0;
    }

    if ( !analy->show_carpet )
        return;

    if ( analy->isosurf_poly != NULL || analy->cut_poly != NULL ||
         analy->ref_polys != NULL || analy->reg_dim[0] > 0 )
        gen_iso_carpet_points( analy );
    else
        gen_grid_carpet_points( analy );
}


/************************************************************
 * TAG( gen_iso_carpet_points )
 *
 * Generate carpet points for all triangles on the cut planes
 * and on the isosurfaces if displayed.
 */
static void
gen_iso_carpet_points( analy )
Analysis *analy;
{
    Hex_geom *bricks;
    Nodal *nodes;
    Triangle_poly *tri;
    float area;
    float verts[8][3], xi[3];
    float *carpet_coords[3], *ftmp;
    float vecs_per_area, vec_num, remain;
    int *carpet_elem, *itmp;
    int cnt, vec_cnt, max_cnt;
    int nd, i, j;

    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;
    vecs_per_area = 1.0 / ( analy->vec_cell_size * analy->vec_cell_size );

    /* Estimate the number of points we're going to generate. */
    area = 0.0;
    for ( tri = analy->isosurf_poly; tri != NULL; tri = tri->next )
        area += area_of_triangle( tri->vtx );
    for ( tri = analy->cut_poly; tri != NULL; tri = tri->next )
        area += area_of_triangle( tri->vtx );

    if ( area == 0.0 )
        return;

    max_cnt = (int)(1.3 * area * vecs_per_area);

    for ( i = 0; i < 3; i++ )
        carpet_coords[i] = NEW_N( float, max_cnt, "Iso carpet coords" );
    carpet_elem = NEW_N( int, max_cnt, "Iso carpet elem" );

    /* Loop through triangles to generate the points. */
    cnt = 0;
    for ( tri = analy->isosurf_poly; tri != NULL; tri = tri->next )
    {
        /* Get point count. */
        vec_num = vecs_per_area * area_of_triangle( tri->vtx );
        vec_cnt = (int) vec_num;
        remain = vec_num - vec_cnt;
        if ( drand48() < remain )
            ++vec_cnt;

        if ( cnt + vec_cnt > max_cnt )
        {   
            popup_dialog( WARNING_POPUP, 
	                  "Not enough room in iso carpet array." );
            break;
        }

        /* Get element vertices. */
        for ( i = 0; i < 8; i++ )
        {
            nd = bricks->nodes[i][tri->elem];
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes->xyz[j][nd];
        }

        /* Generate the points and throw them on the list. */
        for ( i = 0; i < vec_cnt; i++ )
        { 
            rand_bary_pt( tri->vtx, verts, xi );

            for ( j = 0; j < 3; j++ )
                carpet_coords[j][cnt] = xi[j];
            carpet_elem[cnt] = tri->elem;
            cnt++;
        }
    }
    for ( tri = analy->cut_poly; tri != NULL; tri = tri->next )
    {
        /* Get point count. */
        vec_num = vecs_per_area * area_of_triangle( tri->vtx );
        vec_cnt = (int) vec_num;
        remain = vec_num - vec_cnt;
        if ( drand48() < remain )
            ++vec_cnt;

        if ( cnt + vec_cnt > max_cnt )
        {
            popup_dialog( WARNING_POPUP, 
	                  "Not enough room in iso carpet array." );
            break;
        }

        /* Get element vertices. */
        for ( i = 0; i < 8; i++ )
        {
            nd = bricks->nodes[i][tri->elem];
            for ( j = 0; j < 3; j++ )
                verts[i][j] = nodes->xyz[j][nd];
        }

        /* Generate the points and throw them on the list. */
        for ( i = 0; i < vec_cnt; i++ )
        {
            rand_bary_pt( tri->vtx, verts, xi );

            for ( j = 0; j < 3; j++ )
                carpet_coords[j][cnt] = xi[j];
            carpet_elem[cnt] = tri->elem;
            cnt++;
        }
    }

    /* Resize the point arrays. */
    analy->vol_carpet_cnt = cnt;

    for ( i = 0; i < 3; i++ )
    {
        ftmp = NEW_N( float, cnt, "Iso carpet coords" );
        memcpy( ftmp, carpet_coords[i], cnt*sizeof(float) );
        free( carpet_coords[i] );
        analy->vol_carpet_coords[i] = ftmp;
    }

    itmp = NEW_N( int, max_cnt, "Iso carpet elem" );
    memcpy( itmp, carpet_elem, cnt*sizeof(int) );
    free( carpet_elem );
    analy->vol_carpet_elem = itmp;
}


/************************************************************
 * TAG( gen_grid_carpet_points )
 *
 * Generate carpet points for all volume and shell elements
 * in the grid.
 */
static void
gen_grid_carpet_points( analy )
Analysis *analy;
{
    Hex_geom *bricks;
    Shell_geom *shells;
    Nodal *nodes;
    float area, vol, csize;
    float xx[8], yy[8], zz[8];
    float qverts[4][3];
    float *carpet_coords[3], *ftmp;
    float vec_num, vec_cnt, remain;
    int *carpet_elem, *itmp;
    int cnt, max_cnt;
    int nd, i, j, k;

    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;
    nodes = analy->state_p->nodes;
    csize = analy->vec_cell_size;

    /* Loop through elements to generate the points. */
    if ( bricks != NULL )
    {
        /* Estimate the number of points we're going to generate. */
        vol = 0.0;
        for ( i = 0; i < bricks->cnt; i++ )
        {
            for ( j = 0; j < 8; j++ )
            {
                nd = bricks->nodes[j][i];
                xx[j] = nodes->xyz[0][nd];
                yy[j] = nodes->xyz[1][nd];
                zz[j] = nodes->xyz[2][nd];
            }
            vol += hex_vol( xx, yy, zz );
        }
        max_cnt = (int)( 1.5 * vol / (csize*csize*csize) ) + 1;
        for ( i = 0; i < 3; i++ )
            carpet_coords[i] = NEW_N( float, max_cnt, "Carpet coords" );
        carpet_elem = NEW_N( int, max_cnt, "Carpet elem" );

        cnt = 0;
        for ( i = 0; i < bricks->cnt; i++ )
        {
            for ( j = 0; j < 8; j++ )
            {
                nd = bricks->nodes[j][i];
                xx[j] = nodes->xyz[0][nd];
                yy[j] = nodes->xyz[1][nd];
                zz[j] = nodes->xyz[2][nd];
            }
            vol = hex_vol( xx, yy, zz );

            /* Get point count. */
            vec_num = vol / (csize*csize*csize);
            vec_cnt = (int) vec_num;
            remain = vec_num - vec_cnt;
            if ( drand48() < remain )
                ++vec_cnt;

            if ( cnt + vec_cnt > max_cnt )
            {
                popup_dialog( WARNING_POPUP, 
		              "Not enough room in carpet array." );
                break;
            }

            /* Generate the points and throw them on the list. */
            for ( j = 0; j < vec_cnt; j++ )
            {
                for ( k = 0; k < 3; k++ )
                    carpet_coords[k][cnt] = 2.0 * drand48() - 1.0;
                carpet_elem[cnt] = i;
                cnt++;
            }
        }

        /* Resize the point arrays. */
        analy->vol_carpet_cnt = cnt;

        for ( i = 0; i < 3; i++ )
        {
            ftmp = NEW_N( float, cnt, "Carpet coords" );
            memcpy( ftmp, carpet_coords[i], cnt*sizeof(float) );
            free( carpet_coords[i] );
            analy->vol_carpet_coords[i] = ftmp;
        }
        itmp = NEW_N( int, max_cnt, "Carpet elem" );
        memcpy( itmp, carpet_elem, cnt*sizeof(int) );
        free( carpet_elem );
        analy->vol_carpet_elem = itmp;
    }

    if ( shells != NULL )
    {
        /* Estimate the number of points we're going to generate. */
        area = 0.0;
        for ( i = 0; i < shells->cnt; i++ )
        {
            for ( j = 0; j < 4; j++ )
            {
                nd = shells->nodes[j][i];
                for ( k = 0; k < 3; k++ )
                    qverts[j][k] = nodes->xyz[k][nd];
            }
            area += area_of_quad( qverts );
        }
        max_cnt = (int)( 1.3 * area / (csize*csize) ) + 1;
        for ( i = 0; i < 2; i++ )
            carpet_coords[i] = NEW_N( float, max_cnt, "Carpet coords" );
        carpet_elem = NEW_N( int, max_cnt, "Carpet elem" );

        cnt = 0;
        for ( i = 0; i < shells->cnt; i++ )
        {
            for ( j = 0; j < 4; j++ )
            {
                nd = shells->nodes[j][i];
                for ( k = 0; k < 3; k++ )
                    qverts[j][k] = nodes->xyz[k][nd];
            }
            area = area_of_quad( qverts );

            /* Get point count. */
            vec_num = area / (csize*csize);
            vec_cnt = (int) vec_num;
            remain = vec_num - vec_cnt;
            if ( drand48() < remain )
                ++vec_cnt;

            if ( cnt + vec_cnt > max_cnt )
            {
                popup_dialog( WARNING_POPUP, 
		              "Not enough room in carpet array." );
                break;
            }

            /* Generate the points and throw them on the list. */
            for ( j = 0; j < vec_cnt; j++ )
            {
                for ( k = 0; k < 2; k++ )
                    carpet_coords[k][cnt] = 2.0 * drand48() - 1.0;
                carpet_elem[cnt] = i;
                cnt++;
            }
        }

        /* Resize the point arrays. */
        analy->shell_carpet_cnt = cnt;

        for ( i = 0; i < 2; i++ )
        {
            ftmp = NEW_N( float, cnt, "Carpet coords" );
            memcpy( ftmp, carpet_coords[i], cnt*sizeof(float) );
            free( carpet_coords[i] );
            analy->shell_carpet_coords[i] = ftmp;
        }
        itmp = NEW_N( int, max_cnt, "Carpet elem" );
        memcpy( itmp, carpet_elem, cnt*sizeof(int) );
        free( carpet_elem );
        analy->shell_carpet_elem = itmp;
    }
}


/************************************************************
 * TAG( sort_carpet_points )
 *
 * Sort the carpet points in order from nearest to the viewer
 * to farthest from the viewer.
 */
void
sort_carpet_points( analy, cnt, carpet_coords, carpet_elem, index, is_hex )
Analysis *analy;
int cnt;
float **carpet_coords;
int *carpet_elem;
int *index;
Bool_type is_hex;
{
    Hex_geom *bricks;
    Shell_geom *shells;
    Nodal *nodes;
    Transf_mat view_mat;
    float *dist;
    float eyept[3], vecpt[3], diff[3];
    float r, s, t, h[8];
    int el, i, j, k;

    bricks = analy->geom_p->bricks;
    shells = analy->geom_p->shells;
    nodes = analy->state_p->nodes;

    /* Get the inverse viewing transformation matrix. */
    inv_view_transf_mat( &view_mat );

    /* Inverse transform the eyepoint into world coordinates. */
    point_transform( eyept, v_win->look_from, &view_mat );

    /* Calculate the square of the distance from the eye for all vec pts. */
    dist = NEW_N( float, cnt, "Carpet sort tmp" );

    if ( is_hex )
    {
        /* Points were generated in hex elements. */
        for ( i = 0; i < cnt; i++ )
        {
            /* Get the natural coordinates of the vector point. */
            r = carpet_coords[0][i];
            s = carpet_coords[1][i];
            t = carpet_coords[2][i];
            shape_fns_3d( r, s, t, h );

            /* Get the physical coordinates of the vector point. */
            el = carpet_elem[i];
            VEC_SET( vecpt, 0.0, 0.0, 0.0 );
            for ( j = 0; j < 8; j++ )
                for ( k = 0; k < 3; k++ )
                    vecpt[k] += h[j]*nodes->xyz[k][ bricks->nodes[j][el] ];

            /* Calculate square of distance. */
            VEC_SUB( diff, vecpt, eyept );
            dist[i] = VEC_DOT( diff, diff );
        }
    }
    else
    {
        /* Points were generated in shell elements. */
        for ( i = 0; i < cnt; i++ )
        {
            /* Get the natural coordinates of the vector point. */
            r = carpet_coords[0][i];
            s = carpet_coords[1][i];
            shape_fns_2d( r, s, h );

            /* Get the physical coordinates of the vector point. */
            el = carpet_elem[i];
            VEC_SET( vecpt, 0.0, 0.0, 0.0 );
            for ( j = 0; j < 4; j++ )
                for ( k = 0; k < 3; k++ )
                    vecpt[k] += h[j]*nodes->xyz[k][ shells->nodes[j][el] ];

            /* Calculate square of distance. */
            VEC_SUB( diff, vecpt, eyept );
            dist[i] = VEC_DOT( diff, diff );
        }
    }

    /* Sort distances from smallest to largest. */
    real_heapsort( cnt, dist, index );
}


/*************************************************************
 * TAG( real_heapsort )
 *
 * Heap sort, from Numerical_Recipes_in_C.  The array of floats
 * in arrin is sorted in ascending order and the sorted ordering
 * is returned in the indx array.
 */
static void
real_heapsort( n, arrin, indx )
int n;
float *arrin;
int *indx;
{
    int l, j, ir, indxt, i;

    for ( j = 0; j < n; j++ )
        indx[j] = j;

    l = n/2;
    ir = n - 1;

    for (;;)
    {
        if ( l > 0 )
        {
            --l;
            indxt = indx[l];
        }
        else
        {
            indxt = indx[ir];
            indx[ir] = indx[0];
            --ir;
            if ( ir == 0 )
            {
                indx[0] = indxt;
                return;
            }
        }

        i = l;
        j = 2*l + 1;
        while ( j <= ir )
        {
            if ( j < ir && arrin[indx[j]] < arrin[indx[j+1]] )
                j++;
            if ( arrin[indxt] < arrin[indx[j]] )
            {
                indx[i] = indx[j];
                i = j;
                j = 2*j + 1;
            }
            else
                j = ir + 1;
        }
        indx[i] = indxt;
    }
}


/************************************************************
 * TAG( gen_reg_carpet_points )
 *
 * Generate carpet points for jittered regular grid.
 */
void
gen_reg_carpet_points( analy )
Analysis *analy;
{
    Hex_geom *bricks;
    Nodal *nodes;
    Bool_type found;
    float verts[8][3], pt[3], xi[3];
    float *lo[3], *hi[3];
    float cell_size, val;
    float *reg_carpet_coords[3];
    int *reg_carpet_elem;
    int *reg_dim;
    int ii, jj, kk, el, nd, idx, cnt;
    int i, j, k;

    /* Handle 3D grids only for now. */
    if ( analy->geom_p->bricks == NULL )
        return;

    /* Free any existing regular carpet points. */
    if ( analy->reg_dim[0] > 0 )
    {
        for ( i = 0; i < 3; i++ )
            free( analy->reg_carpet_coords[i] );
        free( analy->reg_carpet_elem );
        analy->reg_dim[0] = 0;
    }

    if ( !analy->show_carpet || analy->reg_cell_size == 0.0 )
        return;

    /* Get grid dimensions and allocate space for grid. */
    cell_size = analy->reg_cell_size;
    reg_dim = analy->reg_dim;
    for ( i = 0; i < 3; i++ )
        reg_dim[i] = (int)( (analy->bbox[1][i] - analy->bbox[0][i]) /
                            cell_size );
    cnt = reg_dim[0] * reg_dim[1] * reg_dim[2];

    for ( i = 0; i < 3; i++ )
        reg_carpet_coords[i] = NEW_N( float, cnt, "Reg carpet coords" );
    reg_carpet_elem = NEW_N( int, cnt, "Reg carpet elem" );

    bricks = analy->geom_p->bricks;
    nodes = analy->state_p->nodes;

    /* Compute a bounding box for each element in advance. */
    for ( i = 0; i < 3; i++ )
    {
        lo[i] = analy->tmp_result[i];
        hi[i] = analy->tmp_result[i+3];
    }

    for ( i = 0; i < bricks->cnt; i++ )
    {
        nd = bricks->nodes[0][i];
        for ( k = 0; k < 3; k++ )
        {
            lo[k][i] = nodes->xyz[k][nd];
            hi[k][i] = nodes->xyz[k][nd];
        }

        for ( j = 1; j < 8; j++ )
        {
            nd = bricks->nodes[j][i];
            for ( k = 0; k < 3; k++ )
            {
                val = nodes->xyz[k][nd];
                if ( val < lo[k][i] )
                    lo[k][i] = val;
                else if ( val > hi[k][i] )
                    hi[k][i] = val;
            }
        }
    }

    /* Now do the point inclusion tests. */
    for ( ii = 0; ii < reg_dim[0]; ii++ )
        for ( jj = 0; jj < reg_dim[1]; jj++ )
            for ( kk = 0; kk < reg_dim[2]; kk++ )
            {
                idx = ii*reg_dim[1]*reg_dim[2] + jj*reg_dim[2] + kk;

                /* Calculate the point position. */
                pt[0] = analy->bbox[0][0] + cell_size*( ii + drand48() );
                pt[1] = analy->bbox[0][1] + cell_size*( jj + drand48() );
                pt[2] = analy->bbox[0][2] + cell_size*( kk + drand48() );

                found = FALSE;
                for ( i = 0; i < analy->num_blocks; i++ )
                {
                    /* See if point lies in block's bound box. */
                    if ( pt[0] < analy->block_bbox[0][0][i] ||
                         pt[1] < analy->block_bbox[0][1][i] ||
                         pt[2] < analy->block_bbox[0][2][i] ||
                         pt[0] > analy->block_bbox[1][0][i] ||
                         pt[1] > analy->block_bbox[1][1][i] ||
                         pt[2] > analy->block_bbox[1][2][i] )
                        continue;

                    /* Test point against elements in block. */
                    for ( el = analy->block_lo[i];
                          el <= analy->block_hi[i];
                          el++ )
                    {
                        /* Perform approximate inside (bbox) test first. */
                        if ( pt[0] >= lo[0][el] && pt[0] <= hi[0][el] &&
                             pt[1] >= lo[1][el] && pt[1] <= hi[1][el] &&
                             pt[2] >= lo[2][el] && pt[2] <= hi[2][el] )
                        {
                            for ( j = 0; j < 8; j++ )
                            {
                                nd = bricks->nodes[j][el];
                                for ( k = 0; k < 3; k++ )
                                    verts[j][k] = nodes->xyz[k][nd];
                            }

                            if ( pt_in_hex( verts, pt, xi ) )
                            {
                                reg_carpet_elem[idx] = el;
                                for ( k = 0; k < 3; k++ )
                                    reg_carpet_coords[k][idx] = xi[k];
                                found = TRUE;
                                break;
                            }
                        }
                    }

                    if ( found )
                        break;
                }

                /* If the point wasn't in any bricks, mark it as out. */
                if ( !found )
                {
                    reg_carpet_elem[idx] = -1;
#ifdef DEBUG
fprintf( stderr, "Point not in any bricks: %f %f %f.\n",
    pt[0], pt[1], pt[2] );
#endif
                }
            }

    analy->reg_carpet_elem = reg_carpet_elem;
    for ( i = 0; i < 3; i++ )
        analy->reg_carpet_coords[i] = reg_carpet_coords[i];
}


