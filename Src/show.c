/* $Id$ */
/* 
 * show.c - Parser for 'show' command to display analysis results.
 * 
 * 	Donald J. Dovey
 * 	Lawrence Livermore National Laboratory
 * 	Aug 28 1992
 *
 * Copyright (c) 1992 Regents of the University of California
 */

#include "viewer.h"

/* Local routines. */
static void lookup_global_minmax();


/*****************************************************************
 * TAG( parse_show_command )
 * 
 * Parses the "show" command for displaying result values.  Returns
 * 1 if a valid command was given and 0 if the command was invalid.
 */
int
parse_show_command( token, analy )
char *token;
Analysis *analy;
{
    char str[90];
    int idx;

    /* Cache the current global min/max. */
    cache_global_minmax( analy );

    /* Get the result type. */
    analy->result_id = lookup_result_id( token );

    /* Error if no match in table. */
    if ( analy->result_id < 0 )
    {
        wrt_text( "Show command unrecognized: %s\n", token );
        return( 0 );
    }

    /* Update the result title. */
    idx = resultid_to_index[analy->result_id];
    strcpy( analy->result_title, trans_result[idx][1] );

    /* Check for valid result request and set display flags. */
    analy->show_hex_result = FALSE;
    analy->show_shell_result = FALSE;
    if ( is_hex_result( analy->result_id ) )
    {
        if ( analy->geom_p->bricks == NULL )
        {
            wrt_text( "No brick data present.\n" );
            analy->result_id = VAL_NONE;
            return( 0 );
        }
        analy->show_hex_result = TRUE;
    }
    else if ( is_shell_result( analy->result_id ) )
    {
        if ( analy->geom_p->shells == NULL )
        {
            wrt_text( "No shell data present.\n" );
            analy->result_id = VAL_NONE;
            return( 0 );
        }
        analy->show_shell_result = TRUE;
    }
    else if ( is_shared_result( analy->result_id ) )
    {
        if ( (analy->geom_p->bricks == NULL) &&
             (analy->geom_p->shells == NULL) )
        {
            wrt_text( "No shell or brick data present.\n" );
            analy->result_id = VAL_NONE;
            return( 0 );
        }
        if ( analy->geom_p->bricks != NULL )
           analy->show_hex_result = TRUE;
        if ( analy->geom_p->shells  != NULL )
           analy->show_shell_result = TRUE;
    }
    else if ( is_beam_result( analy->result_id ) )
    {
        if ( analy->geom_p->beams == NULL )
        {
            wrt_text( "No beam data present.\n" );
            analy->result_id = VAL_NONE;
            return( 0 );
        }
    }
    else if ( is_nodal_result( analy->result_id ) )
    {
	if ( ( analy->result_id == VAL_NODE_HELICITY 
	       || analy->result_id == VAL_NODE_ENSTROPHY )
	     && analy->dimension != 3 )
	{
	    wrt_text( "%s valid only for 3D data.\n", 
	              ( analy->result_id == VAL_NODE_HELICITY ) ? "Helicity"
		                                                : "Enstrophy" );
	    analy->result_id = VAL_NONE;
	    return( 0 );
	}
        analy->show_hex_result = TRUE;
        analy->show_shell_result = TRUE;
    }

    load_result( analy, TRUE );

    /* 
     * Look up the global min/max for the new variable.  This must
     * be performed after load_result() is called.
     */
    lookup_global_minmax( analy->result_id, analy );

    /* Update cut planes, isosurfs, contours. */
    update_vis( analy ); 

    return( 1 );
}


/*****************************************************************
 * TAG( cache_global_minmax )
 * 
 * Takes the global min/max for the specified result type and
 * stashes it in a list for later retrieval.
 */
extern void
cache_global_minmax( analy )
Analysis *analy;
{
    int result_id;
    float *test_mm;
    float *el_mm_vals;
    Minmax_obj *mm_list;
    int *mesh_object;
    Minmax_obj *mm;
    
    result_id = analy->result_id;
    test_mm = analy->global_mm;
    mm_list = analy->result_mm_list;
    mesh_object = analy->global_mm_nodes;

    if ( result_id == VAL_NONE )
        return;

    /* See if it's already in the list.
     */
    for ( mm = mm_list; mm != NULL; mm = mm->next )
        if ( match_result( analy, analy->result_id, &mm->result ) )
        {
/* Why update unequivocally?
            mm->minmax[0] = test_mm[0];
            mm->minmax[1] = test_mm[1];
*/
            if ( test_mm[0] < mm->minmax[0] )
	    {
	        mm->minmax[0] = test_mm[0];
		mm->mesh_object[0] = mesh_object[0];
	    }
            if ( test_mm[1] > mm->minmax[1] )
	    {
	        mm->minmax[1] = test_mm[1];
		mm->mesh_object[1] = mesh_object[1];
	    }
	    
	    /* 
	     * If element result, update element minmax for use when
	     * interpolation mode is "noterp".
	     */
	    if ( !is_nodal_result( result_id ) )
	    {
		el_mm_vals = analy->global_elem_mm;
		
		if ( el_mm_vals[0] < mm->el_minmax[0] )
	            mm->el_minmax[0] = el_mm_vals[0];
		if ( el_mm_vals[1] > mm->el_minmax[1] )
	            mm->el_minmax[1] = el_mm_vals[1];
	    }
            return;
        }

    /* Isn't in the list yet, add it.
     */
    mm = NEW( Minmax_obj, "Minmax obj" );
    mm->result.ident = result_id;
    mm->minmax[0] = test_mm[0];
    mm->minmax[1] = test_mm[1];
    mm->mesh_object[0] = mesh_object[0];
    mm->mesh_object[1] = mesh_object[1];
    mm->el_minmax[0] = analy->elem_state_mm.el_minmax[0];
    mm->el_minmax[1] = analy->elem_state_mm.el_minmax[1];
    /* Save these regardless of result (cheaper than all the tests). */
    mm->result.strain_variety = analy->strain_variety;
    mm->result.ref_frame = analy->ref_frame;
    mm->result.ref_surf = analy->ref_surf;
    
    INSERT( mm, analy->result_mm_list );
}


/*****************************************************************
 * TAG( lookup_global_minmax )
 * 
 * Look in the min/max list to see if we saved a global min/max
 * for this result type.  If found, the min/max is copied in to
 * the minmax parameter.  
 *
 * NOTE:
 *     Assumes that the state min/max has already been computed.
 */
static void
lookup_global_minmax( result_id, analy )
Result_type result_id;
Analysis *analy;
{
    Minmax_obj *mm;
    Bool_type found;
    int i;
    float *el_state_mm;

    el_state_mm = analy->elem_state_mm.el_minmax;
    
    /*
     * Lookup the global min/max for a result from the library.
     * If the global min/max is not found in the library, we must
     * use the state min/max instead.  This means that the state
     * min/max must have already been computed!
     */

    if ( result_id == VAL_NONE )
        return;

    found = FALSE;
    for ( mm = analy->result_mm_list; mm != NULL; mm = mm->next )
    {
        if ( match_result( analy, result_id, &mm->result ) )
        {
            analy->global_mm[0] = mm->minmax[0];
	    analy->global_mm_nodes[0] = mm->mesh_object[0];
            analy->global_mm[1] = mm->minmax[1];
	    analy->global_mm_nodes[1] = mm->mesh_object[1];

	    analy->global_elem_mm[0] = mm->el_minmax[0];
	    analy->global_elem_mm[1] = mm->el_minmax[1];
	    
            found = TRUE;
            break;
        }
    }

    if ( found )
    {
        if ( analy->state_mm[0] < analy->global_mm[0] )
	{
            analy->global_mm[0] = analy->state_mm[0];
	    analy->global_mm_nodes[0] = analy->state_mm_nodes[0];
	}
        if ( analy->state_mm[1] > analy->global_mm[1] )
	{
            analy->global_mm[1] = analy->state_mm[1];
	    analy->global_mm_nodes[1] = analy->state_mm_nodes[1];
	}
    
	/* Update the global minmax for element (pre-interpolated) results. */
	if ( el_state_mm[0] < analy->global_elem_mm[0] )
	    analy->global_elem_mm[0] = el_state_mm[0];
	if ( el_state_mm[1] > analy->global_elem_mm[1] )
	    analy->global_elem_mm[1] = el_state_mm[1];
    }
    else
    {
        /* Not found, initialize global min/max to state min/max. */
        analy->global_mm[0] = analy->state_mm[0];
	analy->global_mm_nodes[0] = analy->state_mm_nodes[0];
        analy->global_mm[1] = analy->state_mm[1];
	analy->global_mm_nodes[1] = analy->state_mm_nodes[1];
        analy->global_elem_mm[0] = el_state_mm[0];
        analy->global_elem_mm[1] = el_state_mm[1];
    }

    /* Update the current min/max. */
    for ( i = 0; i < 2; i++ )
        if ( !analy->mm_result_set[i] )
        {
            if ( analy->use_global_mm )
                analy->result_mm[i] = analy->global_mm[i];
            else
                analy->result_mm[i] = analy->state_mm[i];
        }
}


/*****************************************************************
 * TAG( match_result )
 *
 * Match a result_id plus associated modifiers against a stored
 * result with modifiers.
 */
Bool_type
match_result( analy, result_id, test )
Analysis *analy;
Result_type result_id;
Result_spec *test;
{
    if ( result_id == test->ident )
    {
        /* Special cases when shells present. */
	if ( is_shared_result( result_id ) 
	     && analy->geom_p->shells != NULL )
	{
	    /* Reference surface needed for all shared shell results. */
	    if ( analy->ref_surf != test->ref_surf )
	        return FALSE;
	    /* 
	     * Reference frame needed for all but effective stress/strain
	     * and pressure.
	     */
	    if ( ( result_id >= VAL_SHARE_SIGX
	           && result_id <= VAL_SHARE_SIGZX )
		 || ( result_id >= VAL_SHARE_EPSX
		      && result_id <= VAL_SHARE_EPSZX ) )
	        if ( analy->ref_frame != test->ref_frame )
		    return FALSE;
	}
	
	/* Special cases when bricks present. */
	if ( analy->geom_p->bricks != NULL )
	{
	    /*
	     * Strain variety needed for shared strains and
	     * principal strains.
	     */
	    if ( ( result_id >= VAL_SHARE_EPSX
	           && result_id <= VAL_SHARE_EPSZX )
		 || ( result_id >= VAL_HEX_EPS_PD1
		      && result_id <= VAL_HEX_EPS_P3 ) )
                if ( analy->strain_variety != test->strain_variety )
		    return FALSE;
	}
	
	return TRUE;
    }
    else
        return FALSE;
}


/*****************************************************************
 * TAG( compute_shell_in_data )
 *
 * Compute shell results which are already stored in the database.
 * (I.e., they don't need to be computed, just read in.)
 */
void
compute_shell_in_data( analy, result_arr )
Analysis *analy;
float *result_arr;
{
    if ( is_in_database( analy->result_id ) )
    {
        get_result( analy->result_id, analy->cur_state, analy->shell_result );
        shell_to_nodal( analy->shell_result, result_arr, analy, FALSE );
    }
    else
    {
        wrt_text( "Shell result not found in database.\n" );
        analy->result_id = VAL_NONE;
        analy->show_shell_result = FALSE;
        return;
    }
}


/*****************************************************************
 * TAG( compute_beam_in_data )
 *
 * Compute beam results which are already stored in the database.
 * (I.e., they don't need to be computed, just read in.)
 */
void
compute_beam_in_data( analy, result_arr )
Analysis *analy;
float *result_arr;
{
    if ( is_in_database( analy->result_id ) )
    {
        get_result( analy->result_id, analy->cur_state, analy->beam_result );
        beam_to_nodal( analy->beam_result, result_arr, analy );
    }
    else
    {
        wrt_text( "Beam result not found in database.\n" );
        analy->result_id = VAL_NONE;
        return;
    }
}


/*****************************************************************
 * TAG( compute_node_in_data )
 *
 * Compute nodal results which are already stored in the database.
 * (I.e., they don't need to be computed, just read in.)
 */
void
compute_node_in_data( analy, result_arr )
Analysis *analy;
float *result_arr;
{
    if ( is_in_database( analy->result_id ) )
        get_result( analy->result_id, analy->cur_state, result_arr );
    else
    {
        wrt_text( "Nodal result not found in database.\n" );
        analy->result_id = VAL_NONE;
        analy->show_hex_result = FALSE;
        analy->show_shell_result = FALSE;
        return;
    }
}


