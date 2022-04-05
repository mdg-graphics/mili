/* $Id$ */
/*
 * minmax.c - Routines which do min/max searching and reporting for
 * "tellmm" and "outmm" commands.  These routines are not for normal
 * state-change min/max collection and caching.
 *
 *      Doug Speck
 *      Lawrence Livermore National Laboratory
 *      Jan 4 2001
 *
 ************************************************************************
 * Modifications:
 *  I. R. Corey - Dec 14, 2004: Add new function to compute extreme min
 *  and max - See SRC# 292.
 *
 *  I. R. Corey - March 18, 2010: Added capability to outmm for material
 *  result - See SRC# 671.
 *
 *  I. R. Corey - Jan 30, 2013: Moved test for nodal result in
 *  parse_outmm_command().
 *
 ************************************************************************
 */

#include <stdlib.h>
#include "viewer.h"

/* Local routines. */
static void add_node_ref_counts( Mesh_data *, Material_data *, int * );
static void minmax_search_by_mat( Analysis *, int, int, int, int *, float **,
                                  int *, Int_2tuple **, float *, int *, float *,
                                  int *, Bool_type );
static void minmax_search_by_mat_sand_elem( Analysis *, int, int, int, int *,
        float **, int *, Int_2tuple **,
        float *, int *, float *, int * );
static void minmax_search_by_mat_sand_node( Analysis *, int, int, int, int *,
        float **, int *, Int_2tuple **,
        int *, float *, int *, float *,
        int * );
static void write_mm_report( Analysis *, FILE *, int, int, float *, int *,
                             float *, int *, int, int *, int *, Int_2tuple **,
                             Bool_type );

/*****************************************************************
 * TAG( get_global_minmax )
 *
 * Get the global min/maxes for the currently displayed result.
 * Just walk through all the states without redisplaying the mesh.
 */
extern void
get_global_minmax( Analysis *analy )
{
    int cur_state, max_state, min_state;
    int i;
    int rval;
    Minmax_obj mm_save;
//    Htable_entry *p_hte;
    if ( analy->cur_result == NULL )
        return;

//    //if we have not found this global min max yet, find it, add it, and return to original location.
//    rval = htable_search(analy->prev_mm_globals,analy->curr_result,FIND_ENTRY,&p_hte);
//    //if we found a null table
//    if (rval == NULL_POINTER)
//    {
//    	printf("old mm table empty\n");
//    	printf(analy->curr_result);
//    	printf("\n");
//    	analy->prev_mm_globals = htable_create(128);
//    	rval = htable_search(analy->prev_mm_globals,analy->curr_result,FIND_ENTRY,&p_hte);
//    }
//    if( rval == ENTRY_NOT_FOUND)
//    {
//    	printf("calculating mm\n");
		cur_state = analy->cur_state;
		mm_save = analy->elem_state_mm;
		min_state = GET_MIN_STATE( analy );
		max_state = get_max_state( analy );
//		if(p_hte == NULL){
//			p_hte = NEW(Htable_entry,"Hashtable Entry");
//			if(p_hte == NULL){
//				return;
//			}
//		}

		for ( i = min_state; i <= max_state; i++ )
		{
			analy->cur_state = i;
			analy->db_get_state( analy, i, analy->state_p, &analy->state_p, NULL );

			/* Update displayed result. */
			load_result( analy, TRUE, TRUE, FALSE );
		}

		/* Go back to where we were before. */
		analy->elem_state_mm = mm_save;
		analy->cur_state     = cur_state;
		change_state( analy );
		//add mm to previous mm list
//		Minmax_obj *temp_mm_loc;
//		Minmax_obj temp_mm;
//		temp_mm_loc = &temp_mm;
//		temp_mm = (Minmax_obj*)malloc(sizeof(struct Minmax_obj));
//		Minmax_obj * temp_mm;
//		temp_mm = (Minmax_obj*)calloc(1,sizeof(Minmax_obj));
//		init_mm_obj(temp_mm);
//		temp_mm = &analy->elem_global_mm;
//		mmHistEnt *temp_mm;
//		//temp_mm = (mmHistEnt*)calloc(1,sizeof(mmHistEnt));
//		temp_mm = NEW(mmHistEnt,"MinMax HistEntry");
//		if(temp_mm == NULL){
//			return;
//		}
//		temp_mm->elem_global_mm = analy->elem_global_mm;
//		temp_mm->global_mm[0] = analy->global_mm[0];
//		temp_mm->global_mm[1] = analy->global_mm[1];
//		temp_mm->global_mm_class_long_name[0] = analy->global_mm_class_long_name[0];
//		temp_mm->global_mm_class_long_name[1] = analy->global_mm_class_long_name[1];
//		temp_mm->global_mm_nodes[0] = analy->global_mm_nodes[0];
//		temp_mm->global_mm_nodes[1] = analy->global_mm_nodes[1];
//		temp_mm->global_mm_sclass[0] = analy->global_mm_sclass[0];
//		temp_mm->global_mm_sclass[1] = analy->global_mm_sclass[1];
////		void *tempVoid = NULL;
////		tempVoid = temp_mm;
////		p_hte->data = tempVoid;
//		p_hte->data = (void *) temp_mm;
//		htable_add_entry_data(analy->prev_mm_globals,analy->curr_result,ENTER_UNIQUE,&p_hte);
//    }
//	else // we have this min max already, find and
//	{
//    	printf("found old mm\n");
//		Minmax_obj *temp_mm = ((Minmax_obj *) p_hte->data);
//		analy->elem_state_mm =  *temp_mm;
//	}
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
extern Bool_type
tellmm( Analysis *analy, char *desired_result_variable, int start_state,
        int stop_state, Redraw_mode_type *tellmm_redraw )
{
    Bool_type found_req;
    Minmax_obj mm_save;
    Result *result_save, *p_res;
    Result tell_result;
    float high, low, maximum_of_states, minimum_of_states;
    float *el_mm;
    int cur_state, i, state_id, max_state;
    int start_idx, stop_idx;
    int obj_fwid, st_fwid, nam_fwid;
    int mo_qty;
    int maximum_element_id, maximum_state_id, minimum_element_id,
        minimum_state_id;
    char *maximum_element_type, *minimum_element_type;
    int *el_id;
    char **class;
    Subrec_obj *p_so;
    MO_class_data *p_mo_class;
    MO_class_data **p_mo_classes;
    int cnam_len;
    Mesh_data *p_mesh;
    int mesh, superclass, j, class_qty;
    Derived_result *p_dr;
    Subrecord_result *p_sr;

    /* retain current state data */

    cur_state = analy->cur_state;
    mm_save   = analy->elem_state_mm;

    maximum_of_states = -MAXFLOAT;
    minimum_of_states =  MAXFLOAT;

    state_id  = start_state;

    start_idx = MAX( 0, start_state - 1 );
    max_state = get_max_state( analy );
    stop_idx  = MIN( stop_state, max_state + 1 );

    /*
     * Filter input data
     */

    if ( analy->cur_result != NULL
            && ( desired_result_variable[0] == '\0'
                 || strcmp( analy->cur_result->name,
                            desired_result_variable ) == 0 ) )
    {
        if ( !analy->cur_result->single_valued )
        {
            popup_dialog( INFO_POPUP,
                          "Result specification for \"tellmm\" command\n%s",
                          "must resolve to a scalar quantity." );
            *tellmm_redraw = NO_VISUAL_CHANGE;
            return FALSE;
        }

        /*
         * current result_id != materials and
         * no overriding result_id is specified or
         * specified overriding result is same as current result -->
         * process current result_id
         */
        result_save = analy->cur_result;
        tell_result.qty = 0;
        *tellmm_redraw = redraw_for_render_mode( FALSE, RENDER_MESH_VIEW,
                         analy );
    }
    else
    {
        /* desired_result_id = lookup_result_id( desired_result_variable ); */
        analy->cur_state = start_idx;
        memset( (void *) &tell_result, (int) '\0', sizeof( Result ) );
        found_req = find_result( analy, ALL, TRUE, &tell_result,
                                 desired_result_variable );

        if ( found_req )
        {
            if ( !tell_result.single_valued )
            {
                popup_dialog( INFO_POPUP,
                              "Result specification for \"tellmm\" command\n%s",
                              "must resolve to a scalar quantity." );
                *tellmm_redraw = NO_VISUAL_CHANGE;
                return FALSE;
            }

            /*
             * current result_id MAY or MAY NOT be materials, but
             * valid overriding result_id is specified that is != materials -->
             * process specified result_id
             */
            result_save = analy->cur_result;
            analy->cur_result = &tell_result;
            *tellmm_redraw = NO_VISUAL_CHANGE;
        }
        else
        {
            /*
             * Invalid data present:
             *
             * current result_id and desired result_id == materials;
             * desired result_id == materials;
             * invalid overriding desired_result_id is specified
             */
            tellmm_redraw = NO_VISUAL_CHANGE;
            return FALSE;
        }
    }

    if ( start_state > stop_state )
    {
        popup_dialog( WARNING_POPUP,
                      "Start state MUST be less than stop state." );
        analy->cur_result = result_save;
        if ( tell_result.qty != 0 )
            cleanse_result( &tell_result );
        return FALSE;
    }

    /*
     * Calculate field widths for formatting output.
     */

    /* Traverse subrecords for first state to collect width data. */
    p_res = analy->cur_result;
    p_so = analy->srec_tree[p_res->srec_id].subrecs;
    nam_fwid = 0;
    mo_qty = 0;
    p_mesh = NULL;
    p_dr = NULL;
    for ( i = 0; i < p_res->qty; i++ )
    {
        if ( p_res->indirect_flags[i] )
        {
            /* Oh hell.  Let's do a lot of work for little gain. */

            /*
             * For an indirect result, we need to check the name lengths
             * of all the classes having the superclass of the candidate.
             */
            if ( p_mesh == NULL )
            {
                analy->db_query( analy->db_ident, QRY_SREC_MESH,
                                 (void *) &p_res->srec_id, NULL,
                                 (void *) &mesh );
                p_mesh = analy->mesh_table + mesh;
            }

            if ( p_dr == NULL )
                p_dr = (Derived_result *) p_res->original_result;
            p_sr = (Subrecord_result *)  p_dr->srec_map[i].list;
            superclass = p_sr[i].superclass;
            p_mo_classes = (MO_class_data **) p_mesh->classes_by_sclass[superclass].list;
            class_qty = p_mesh->classes_by_sclass[superclass].qty;

            for ( j = 0; j < class_qty; j++ )
            {
                cnam_len = strlen( p_mo_classes[j]->long_name );
                if ( cnam_len > nam_fwid )
                    nam_fwid = cnam_len;
                if ( p_mo_classes[j]->qty > mo_qty )
                    mo_qty = p_mo_classes[j]->qty;
            }
        }
        else
        {
            p_mo_class = p_so[p_res->subrecs[i]].p_object_class;
            cnam_len = strlen( p_mo_class->long_name );
            if ( cnam_len > nam_fwid )
                nam_fwid = cnam_len;
            if ( p_mo_class->qty > mo_qty )
                mo_qty = p_mo_class->qty;
        }
    }
    obj_fwid = (int) ((double) 1.0 + log10( (double) mo_qty ));

    /* State number width */
    st_fwid = (int) ((double) 1.0 + log10( (double) stop_state ));

    /*
     * Write the report.
     */

    wrt_text( "%s min/max values, state(s) %d to %d:\n%s\n",
              p_res->title, start_state, stop_state,
              "    STATE          MAX                     MIN" );

    for ( i = start_idx; i < stop_idx; i++ )
    {
        analy->cur_state = i;
        analy->db_get_state( analy, i, analy->state_p, &analy->state_p, NULL );

        /* update displayed result */

        load_result( analy, TRUE, TRUE, TRUE );

        if ( p_res->origin.is_node_result )
        {
            el_mm = analy->state_mm;
            class = analy->state_mm_class_long_name;
            el_id = analy->state_mm_nodes;
        }
        else
        {
            el_mm = analy->elem_state_mm.object_minmax;
            class = analy->elem_state_mm.class_long_name;
            el_id = analy->elem_state_mm.object_id;
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
        int min_label;
        int max_label;
        if(analy->elem_state_mm.sclass[0] == G_MAT)
        {
            min_label = el_id[0] +1;
        }else
        {
            min_label = p_mo_class->labels[p_mo_class->labels_index[el_id[0]]].label_num;
        }
        if(analy->elem_state_mm.sclass[1] == G_MAT)
        {
            max_label = el_id[1] +1;
        }else
        {
            max_label = p_mo_class->labels[p_mo_class->labels_index[el_id[1]]].label_num;
        }
        wrt_text( "     %*d    %9.2e  %*s %-*d    %9.2e  %*s %d\n",
                  st_fwid, state_id,
                  high, nam_fwid, class[1], obj_fwid, max_label,
                  low, nam_fwid, class[0], min_label );

        if ( high > maximum_of_states )
        {
            maximum_of_states    = high;
            maximum_element_type = class[1];
            maximum_element_id   = max_label;
            maximum_state_id     = state_id;
        }

        if ( low < minimum_of_states )
        {
            minimum_of_states    = low;
            minimum_element_type = class[0];
            minimum_element_id   = min_label;
            minimum_state_id     = state_id;
        }

        state_id++;

        cache_global_minmax( analy );
    }

    wrt_text( "\n" );

    wrt_text( "    Maximum of states:  %9.2e at %s %d, state %2d\n",
              maximum_of_states, maximum_element_type,
              maximum_element_id, maximum_state_id );

    wrt_text( "    Minimum of states:  %9.2e at %s %d, state %2d\n\n",
              minimum_of_states, minimum_element_type,
              minimum_element_id, minimum_state_id );

    /* Return to original status. */
    analy->elem_state_mm = mm_save;
    analy->cur_state = cur_state;
    analy->cur_result = result_save;
    analy->db_get_state( analy, analy->cur_state, analy->state_p,
                         &analy->state_p, NULL );

    load_result( analy, FALSE, TRUE, FALSE );

    if ( tell_result.qty > 0 )
        cleanse_result( &tell_result );

    return TRUE;
}


/*****************************************************************
 * TAG( parse_outmm_command )
 *
 * Prepare report of minimums and maximums for the current result
 * across states and sorted by material.
 *
 */
extern Redraw_mode_type
parse_outmm_command( Analysis *analy, char tokens[MAXTOKENS][TOKENLENGTH],
                     int token_cnt )
{
    FILE *outfile;
    int req_mat_qty, qty_mats;
    int *req_mats;
    Bool_type sorting;
    int qty_states, idx, size, rm_idx;
    float *mins, *maxs;
    int *min_ids, *max_ids;
    float val;
    int cur_state, min_st, max_st;
    int i, j;
    MO_class_data *p_mat_class;
    Material_data *p_md;
    float **data_src_arrays;
    int *node_ref_counts;
    int *data_src_qty_blocks;
    Int_2tuple **data_src_obj_blocks;
    int collapse;
    Bool_type *mats_processed;
    Bool_type is_sand_db, is_elem_result, show_mat_result=FALSE;;
    Mesh_data *p_mesh;

    Bool_type nonElem_result_flag=FALSE;

    if ( analy->cur_result == NULL )
    {
        popup_dialog( INFO_POPUP, "%s\n%s",
                      "Please specify a current result with \"show\",",
                      "then retry \"outmm\"." );
        return NO_VISUAL_CHANGE;
    }

    /* No results if no states in db... */
    analy->db_query( analy->db_ident, QRY_QTY_STATES, NULL, NULL,
                     (void *) &qty_states );
    if ( qty_states == 0 )
    {
        popup_dialog( INFO_POPUP, "No state data present." );
        return NO_VISUAL_CHANGE;
    }
    else
    {
        min_st = GET_MIN_STATE( analy );
        max_st = get_max_state( analy );
        qty_states = max_st - min_st + 1;
    }

    if ( !analy->cur_result->origin.is_node_result
            && !analy->cur_result->origin.is_elem_result )
        nonElem_result_flag = TRUE;

    /*
     * Filter input data
     */

    if ( (outfile = fopen( tokens[1], "w")) == 0 )
    {
        popup_dialog( WARNING_POPUP, "Unable to open output file %s.\n",
                      tokens[1] );
        return NO_VISUAL_CHANGE;
    }

    /* Let user know if he/she has requested a numeric file name. */
    if ( is_numeric_token( tokens[1] ) )
        popup_dialog( INFO_POPUP, "Writing outmm output to file \"%s\".",
                      tokens[1] );

    p_mesh          = MESH_P( analy );
    show_mat_result = result_has_superclass( analy->cur_result, G_MAT, analy );
    qty_mats        = p_mesh->material_qty;

    /* Generate array of requested & sorted material numbers. */
    if ( token_cnt > 2 && !show_mat_result )
    {
        req_mat_qty = token_cnt - 2;
        req_mats = NEW_N( int, req_mat_qty, "Requested outmm materials" );

        rm_idx = 0;
        for ( i = 2; i < token_cnt; i++ )
        {
            req_mats[rm_idx] = atoi( tokens[i] ) - 1;

            if ( req_mats[rm_idx] >= 0
                    && req_mats[rm_idx] < qty_mats )
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
            return NO_VISUAL_CHANGE;
        }

        req_mat_qty = rm_idx;

        //WHY-FIX
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
        /* Initialize requested materials to all non-disabled materials. */

        /* Count them. */
        req_mat_qty = 0;
        for ( i = 0; i < qty_mats; i++ )
            if ( !p_mesh->disable_material[i] )
                req_mat_qty++;

        req_mats = NEW_N( int, req_mat_qty, "Requested outmm all materials" );

        /* Load them into the array. */
        j = 0;
        for ( i = 0; i < qty_mats; i++ )
            if ( !p_mesh->disable_material[i] )
                req_mats[j++] = i;
    }

    /* Get reference to the material class. */
    p_mat_class = ((MO_class_data **)
                   MESH_P( analy )->classes_by_sclass[M_MAT].list)[0];

    /*
     * Collapse out materials which don't support the requested result
     * or don't actually have any elements in the mesh.
     */

    is_elem_result = analy->cur_result->origin.is_elem_result;

    collapse = 0;
    for ( i = 0; i < req_mat_qty; i++ )
    {
        /* Get pointer to this material's object data. */
        p_md = p_mat_class->objects.materials + req_mats[i];

        if ( p_md->elem_class == NULL
                || ( is_elem_result
                     && !result_has_class( analy->cur_result, p_md->elem_class,
                                           analy ) ) )
            collapse++;
        else if ( collapse > 0 )
            req_mats[i - collapse] = req_mats[i];
    }

    if ( collapse > 0 && !show_mat_result )
    {
        popup_dialog( INFO_POPUP, "%d material%s support result \"%s\" \n"
                      "or %s no elements; culled from request.",
                      collapse,
                      ( collapse == 1 ) ? " doesn't" : "s don't",
                      analy->cur_result->title,
                      ( collapse == 1 ) ? "has" : "have" );
        req_mat_qty -= collapse;
    }

    if ( req_mat_qty == 0 )
    {
        popup_dialog( INFO_POPUP, "No materials remaining to report "
                      "min/max on; aborting." );
        free( req_mats );
        fclose( outfile );
        return NO_VISUAL_CHANGE;
    }

    /*
     * Allocate tables for per material min's/max's.
     */
    size = req_mat_qty * qty_states;
    mins = NEW_N( float, size, "Object min table" );
    maxs = NEW_N( float, size, "Object max table" );
    min_ids = NEW_N( int, size, "Object min id table" );
    max_ids = NEW_N( int, size, "Object max id table" );

    /* Is it a SAND db? */
    is_sand_db = analy->state_p->sand_present;

    /* Per-material node lists required for nodal results... */
    node_ref_counts = NULL;
    mats_processed = NULL;
    if ( analy->cur_result->origin.is_node_result )
    {
        /* Additional data needed for SAND databases. */
        if ( is_sand_db )
        {
            node_ref_counts = NEW_N( int, MESH_P( analy )->node_geom->qty,
                                     "Node reference counts array" );
            mats_processed = NEW_N( Bool_type, p_mat_class->qty, "mat flags" );
        }

        /* Loop over materials and ensure lists exist. */
        for ( i = 0; i < req_mat_qty; i++ )
        {
            /* Get pointer to this material's object data. */
            p_md = p_mat_class->objects.materials + req_mats[i];

            if ( p_md->node_block_qty == 0 )
            {
                /* List not present; create it. */
                wrt_text( "Generating node list for material %d.\n",
                          req_mats[i] + 1 );
                gen_material_node_list( MESH_P( analy ), req_mats[i], p_md );

                /* See note below. */
                if ( is_sand_db )
                    add_node_ref_counts( MESH_P( analy ), p_md,
                                         node_ref_counts );

#ifdef DUMP_MAT_LISTS
                /* Dump the node list. */
                wrt_text( "Material %d node blocks:\n", req_mats[i] + 1 );
                for ( j = 0; j < p_md->node_block_qty; j++ )
                    wrt_text( "%d. %d - %d\n", j + 1,
                              p_md->node_blocks[j][0] + 1,
                              p_md->node_blocks[j][1] + 1 );
#endif
            }
            else if ( is_sand_db )
            {
                /*
                 * Note that we accumulate references from all specified
                 * materials but in some meshes not all have element failure
                 * flags available.  If a node is shared by both a material
                 * that has failure flags and a material that does not have
                 * failure flags, it will never be deleted from consideration
                 * during the min/max search as its reference count can never
                 * go to zero.  The user can, through judicious specification
                 * of materials to "outmm" or judicious disabling of materials,
                 * ensure that only materials with failure are processed (or
                 * only materials without failure) to preclude ambiguities
                 * about when a node on a material boundary should be
                 * considered or discarded from consideration in the min/max
                 * search.
                 */
                add_node_ref_counts( MESH_P( analy ), p_md, node_ref_counts );
            }

            if ( mats_processed )
                mats_processed[req_mats[i]] = TRUE;
        }

#ifdef ANALYSTS_WANT_THIS
        /*
         * Based on discussion with R. Chun, this add'l processing is not
         * desirable. (Speck, 7/01)
         */

        /* Need additional data for SAND databases. */
        if ( is_sand_db )
        {
            /*
             * At this point, "node_ref_counts" reflects all of the requested
             * materials.  If we stop now, we have info necessary to filter
             * nodes from min/max searching on the basis of element deletion,
             * permitting _requested_ materials' activity solely to determine
             * whether or not a node is considered for min/max.  With the
             * additional processing below, we permit a reference by non-
             * deleted elements of _any_ material to force min/max
             * consideration of a node for every requested material that
             * references it (whether that material's referencing elements
             * have all failed or not).  I.e., with the additional processing,
             * a node can appear in a material's min/max listing even if all
             * of that material's elements that reference the node have failed.
             * This supports, in effect, an analyst wanting to have a node
             * considered whenever it's still valid in the mesh, even if it's
             * not currently "valid" for a requested material.
             */

            /* Add in node reference counts for not-requested materials. */
            for ( i = 0; i < p_mat_class->qty; i++ )
            {
                if ( mats_processed[i] )
                    continue;

                add_node_ref_counts( MESH_P( analy ),
                                     p_mat_class->objects.materials
                                     + req_mats[i], node_ref_counts );
            }
        }
#endif

        if ( mats_processed )
            free( mats_processed );
    }

    /*
     * Allocate/init arrays to speed up references to each material's data
     * and abstract node/element source distinction.
     */

    data_src_arrays = NEW_N( float *, req_mat_qty, "Data array pointers" );
    data_src_qty_blocks = NEW_N( int, req_mat_qty, "Data blk list sizes" );
    data_src_obj_blocks = NEW_N( Int_2tuple *, req_mat_qty, "Data blk lists" );

    for ( i = 0; i < req_mat_qty; i++ )
    {
        /* Get pointer to this material's object data. */
        p_md = p_mat_class->objects.materials + req_mats[i];

        if ( analy->cur_result->origin.is_node_result )
        {
            data_src_arrays[i] = NODAL_RESULT_BUFFER( analy );
            data_src_qty_blocks[i] = p_md->node_block_qty;
            data_src_obj_blocks[i] = p_md->node_blocks;
        }
        else
        {
            data_src_arrays[i] = p_md->elem_class->data_buffer;
            data_src_qty_blocks[i] = p_md->elem_block_qty;
            data_src_obj_blocks[i] = p_md->elem_blocks;
        }
    }


    /* Save current state. */
    cur_state = analy->cur_state;

    /* Perform search depending on SAND/not and elem result vs. node result. */
    if ( !is_sand_db || nonElem_result_flag )
        minmax_search_by_mat( analy, min_st, qty_states, req_mat_qty, req_mats,
                              data_src_arrays, data_src_qty_blocks,
                              data_src_obj_blocks,
                              mins, min_ids, maxs, max_ids, nonElem_result_flag );
    else if ( analy->cur_result->origin.is_elem_result )
        minmax_search_by_mat_sand_elem( analy, min_st, qty_states, req_mat_qty,
                                        req_mats, data_src_arrays,
                                        data_src_qty_blocks,
                                        data_src_obj_blocks,
                                        mins, min_ids, maxs, max_ids );
    else
        minmax_search_by_mat_sand_node( analy, min_st, qty_states, req_mat_qty,
                                        req_mats, data_src_arrays,
                                        data_src_qty_blocks,
                                        data_src_obj_blocks, node_ref_counts,
                                        mins, min_ids, maxs, max_ids );

    /* Convert data units if requested. */
    if ( analy->perform_unit_conversion )
    {
        float scale, offset;

        scale = analy->conversion_scale;
        offset = analy->conversion_offset;

        for ( i = 0; i < qty_states; i++ )
        {
            for ( j = 0; j < req_mat_qty; j++ )
            {
                idx = j * qty_states + i;

                mins[idx] = (scale * mins[idx]) + offset;
                maxs[idx] = (scale * maxs[idx]) + offset;
            }
        }
    }

    /* Output. */
    write_mm_report( analy, outfile, min_st, max_st, mins, min_ids,
                     maxs, max_ids, req_mat_qty, req_mats, data_src_qty_blocks,
                     data_src_obj_blocks, show_mat_result );

    fclose( outfile );

    free( req_mats );
    free( mins );
    free( maxs );
    free( min_ids );
    free( max_ids );
    free( data_src_arrays );
    free( data_src_qty_blocks );
    free( data_src_obj_blocks );
    if ( node_ref_counts )
        free( node_ref_counts );

    /*
     * Return to original status
     */
    analy->cur_state = cur_state;
    analy->db_get_state( analy, cur_state, analy->state_p, &analy->state_p,
                         NULL );
    /**/
    /* Note "interpolate" flag = TRUE wrt SCR #65. */
    load_result( analy, TRUE, TRUE, FALSE );

    return NONBINDING_MESH_VISUAL;
}


/************************************************************
 * TAG( add_node_ref_counts )
 *
 * Traverse material element list to generate node list.
 */
static void
add_node_ref_counts( Mesh_data *p_mesh, Material_data *p_md,
                     int *node_ref_counts )
{
    int i, j, k;
    int last;
    int block_qty, conn_qty;
    int *connects, *elem_conns;

    connects = p_md->elem_class->objects.elems->nodes;
    conn_qty = qty_connects[p_md->elem_class->superclass];

    /* Traverse material's element list to mark all connected nodes. */
    block_qty = p_md->elem_block_qty;
    for ( i = 0; i < block_qty; i++ )
    {
        last = p_md->elem_blocks[i][1];
        j = p_md->elem_blocks[i][0];
        elem_conns = connects + j * conn_qty;

        for ( ; j <= last; j++ )
        {
            for ( k = 0; k < conn_qty; k++ )
                node_ref_counts[elem_conns[k]]++;

            elem_conns += conn_qty;
        }
    }
}


/************************************************************
 * TAG( minmax_search_by_mat )
 *
 * Search a data array for min and max values, examining data
 * for objects listed among a specified set of material
 * object lists.
 */
static void
minmax_search_by_mat( Analysis *analy, int min_state, int qty_states,
                      int req_mat_qty, int *req_mats,
                      float **data_src_arrays, int *data_src_qty_blocks,
                      Int_2tuple **data_src_obj_blocks,
                      float *p_mins, int *p_min_ids, float *p_maxs,
                      int *p_max_ids, Bool_type nonElem_result_flag )
{
    int i, j, k, l;
    float minval, maxval;
    int min_id, max_id;
    int qty_mats;
    int qty_blocks;
    Int_2tuple *obj_blocks;
    float *data;
    int first, second, next, last, idx;
    int srec_id;
    static Bool_type warn_once=TRUE, show_mat_result=FALSE;
    Mesh_data *p_mesh;

    p_mesh          = MESH_P( analy );
    srec_id         = analy->cur_result->srec_id;
    qty_mats        = p_mesh->material_qty;
    show_mat_result = result_has_superclass( analy->cur_result, G_MAT, analy );

    /* Loop over states to fill min/max table. */
    for ( i = 0; i < qty_states; i++ )
    {
        analy->cur_state = min_state + i;
        analy->db_get_state( analy, min_state + i, analy->state_p,
                             &analy->state_p, NULL );

        /* Update result */
        /**/
        /* Node "interpolation" flag is TRUE.  This forces unnecessary
         * calculations on element results but is the only way to get
         * state min/max values extracted.  See SCR #65.
         */
        load_result( analy, TRUE, TRUE, FALSE );

        if ( analy->cur_result->srec_id != srec_id
                && warn_once )
        {
            popup_dialog( WARNING_POPUP,
                          "Multiple state record formats encountered.  No \n"
                          "checking is performed to ensure that min's/max's \n"
                          "are drawn from identical domains in each format." );
            warn_once = FALSE;
        }

        if ( show_mat_result )
        {
            p_mesh = MESH_P( analy );
            data = ((MO_class_data **)
                    p_mesh->classes_by_sclass[G_MAT].list)[0]->data_buffer;

            minval = data[0];
            min_id = 1;
            maxval = data[0];
            max_id = 1;
            for ( j = 0; j < req_mat_qty; j++ )
            {
                if ( data[j]<minval )
                {
                    minval = data[j];
                    min_id = j+1;
                }
                if ( data[j]>maxval )
                {
                    maxval = data[j];
                    max_id = j+1;
                }
            }
            for ( j = 0; j < req_mat_qty; j++ )
            {
                idx = j * qty_states + i;
                p_mins[idx]    = minval;
                p_min_ids[idx] = min_id;

                p_maxs[idx] = maxval;
                p_max_ids[idx] = max_id;
            }
            continue; /* Next state */
        }

        /*
         * For SAND db and a nodal result, process "node_ref_counts" wrt
         * current SAND flags to create a nodal SAND flag array.
         */

        /* Loop over requested materials. */
        for ( j = 0; j < req_mat_qty; j++ )
        {
            idx = j * qty_states + i;
            qty_blocks = data_src_qty_blocks[j];
            obj_blocks = data_src_obj_blocks[j];
            data = data_src_arrays[j];

            /* Init min's/max's for current state. */

            first = obj_blocks[0][0];
            if ( obj_blocks[0][1] > first )
                second = first + 1;
            else if ( qty_blocks > 1 )
                second = obj_blocks[1][0];
            else
            {
                /* Only one object in material - copy and continue.*/
                maxval = data[first];
                minval = maxval;
                max_id = first + 1;
                min_id = first + 1;
                continue;
            }

            if ( data[first] > data[second] )
            {
                maxval = data[first];
                max_id = first + 1;

                minval = data[second];
                min_id = second + 1;
            }
            else
            {
                minval = data[first];
                min_id = first + 1;

                maxval = data[second];
                max_id = second + 1;
            }

            /* Loop over blocks... */
            for ( k = 0; k < qty_blocks; k++ )
            {
                if ( obj_blocks[k][0] < obj_blocks[k][1] )
                {
                    /* The block has at least two objects in it... */

                    first = obj_blocks[k][0];
                    last = obj_blocks[k][1] + 1;

                    /* Loop over block by pairs. */
                    for ( l = first; l + 2 <= last; l += 2 )
                    {
                        next = l + 1;

                        if ( data[l] > data[next] )
                        {
                            /*
                             * If current greater than next, current can't be
                             * a min candidate and next can't be a max
                             * candidate.
                             */

                            if ( data[l] > maxval )
                            {
                                maxval = data[l];
                                max_id = l + 1;
                            }

                            if ( data[next] < minval )
                            {
                                minval = data[next];
                                min_id = next + 1;
                            }
                        }
                        else
                        {
                            /*
                             * Current less than next, so current can't be a
                             * max candidate and next can't be a min candidate.
                             */

                            if ( data[next] > maxval )
                            {
                                maxval = data[next];
                                max_id = next + 1;
                            }

                            if ( data[l] < minval )
                            {
                                minval = data[l];
                                min_id = l + 1;
                            }
                        }
                    }

                    /* Consider the odd extra if extant. */
                    if ( l < last )
                    {
                        if ( data[l] > maxval )
                        {
                            maxval = data[l];
                            max_id = l + 1;
                        }

                        if ( data[l] < minval )
                        {
                            minval = data[l];
                            min_id = l + 1;
                        }
                    }
                }
                else
                {
                    /* Only one object in the block, could be a min or max. */

                    first = obj_blocks[k][0];

                    if ( data[first] > maxval )
                    {
                        maxval = data[first];
                        max_id = first + 1;
                    }

                    if ( data[first] < minval )
                    {
                        minval = data[first];
                        min_id = first + 1;
                    }
                }
            }

            p_mins[idx] = minval;
            p_min_ids[idx] = min_id;

            p_maxs[idx] = maxval;
            p_max_ids[idx] = max_id;
        }
    }
}


/************************************************************
 * TAG( minmax_search_by_mat_sand_elem )
 *
 * Search a data array for min and max values, examining data
 * for objects listed among material object lists.
 */
static void
minmax_search_by_mat_sand_elem( Analysis *analy, int min_state, int qty_states,
                                int req_mat_qty, int *req_mats,
                                float **data_src_arrays,
                                int *data_src_qty_blocks,
                                Int_2tuple **data_src_obj_blocks,
                                float *p_mins, int *p_min_ids, float *p_maxs,
                                int *p_max_ids )
{
    int i, j, k, l;
    float minval, maxval;
    int qty_mats;
    int min_id, max_id;
    int qty_blocks;
    Int_2tuple *obj_blocks;
    float *data;
    int first, last, idx;
    int srec_id;
    MO_class_data *p_mat_class;
    MO_class_data *p_mo_class;
    Material_data *p_md;
    float *p_sand_flags;
    static Bool_type warn_once = TRUE;
    float val;

    srec_id = analy->cur_result->srec_id;

    /* Get reference to the material class. */
    p_mat_class = ((MO_class_data **)
                   MESH_P( analy )->classes_by_sclass[M_MAT].list)[0];

    /* Loop over states to fill min/max table. */
    for ( i = 0; i < qty_states; i++ )
    {
        analy->cur_state = min_state + i;
        analy->db_get_state( analy, min_state + i, analy->state_p,
                             &analy->state_p, NULL );

        /* Update result */
        /**/
        /* Node "interpolation" flag is TRUE.  This forces unnecessary
         * calculations on element results but is the only way to get
         * state min/max values extracted.  See SCR #65.
         */
        load_result( analy, TRUE, TRUE, FALSE );

        if ( analy->cur_result->srec_id != srec_id
                && warn_once )
        {
            popup_dialog( WARNING_POPUP,
                          "Multiple state record formats encountered.  No \n"
                          "checking is performed to ensure that min's/max's \n"
                          "are drawn from identical domains in each format." );
            warn_once = FALSE;
        }

        /* Loop over requested materials. */
        for ( j = 0; j < req_mat_qty; j++ )
        {
            /* Get the SAND flags for the elem class of the current material. */
            p_md = p_mat_class->objects.materials + req_mats[j];
            p_sand_flags = analy->state_p->elem_class_sand[
                               p_md->elem_class->elem_class_index];
            /* Get MO class object */
            p_mo_class = p_md->elem_class;

            idx = j * qty_states + i;
            qty_blocks = data_src_qty_blocks[j];
            obj_blocks = data_src_obj_blocks[j];
            data = data_src_arrays[j];

            /* Init min's/max's for current state. */
            first = obj_blocks[0][0];
            maxval = minval = data[first];
            max_id = min_id = p_mo_class->labels[p_mo_class->labels_index[first]].label_num;

            /* Loop over blocks... */
            for ( k = 0; k < qty_blocks; k++ )
            {
                first = obj_blocks[k][0];
                last = obj_blocks[k][1] + 1;

                /* Loop over current block. */
                if ( p_sand_flags != NULL )
                {
                    for ( l = first; l < last; l++ )
                    {
                        /* Ignore deleted elements. */
                        if ( p_sand_flags[l] == 0.0 )
                            continue;

                        val = data[l];

                        if ( val > maxval )
                        {
                            maxval = val;
                            max_id = p_mo_class->labels[p_mo_class->labels_index[l]].label_num;
                        }

                        if ( val < minval )
                        {
                            minval = val;
                            min_id = p_mo_class->labels[p_mo_class->labels_index[l]].label_num;
                        }
                    }
                }
                else
                {
                    for ( l = first; l < last; l++ )
                    {
                        val = data[l];

                        if ( val > maxval )
                        {
                            maxval = val;
                            max_id = p_mo_class->labels[p_mo_class->labels_index[l]].label_num;
                        }

                        if ( val < minval )
                        {
                            minval = val;
                            min_id = p_mo_class->labels[p_mo_class->labels_index[l]].label_num;
                        }
                    }
                }
            }

            p_mins[idx] = minval;
            p_min_ids[idx] = min_id;

            p_maxs[idx] = maxval;
            p_max_ids[idx] = max_id;
        }
    }
}


/************************************************************
 * TAG( minmax_search_by_mat_sand_node )
 *
 * Search a data array for min and max values, examining data
 * for objects listed among material object lists.
 */
static void
minmax_search_by_mat_sand_node( Analysis *analy, int min_state, int qty_states,
                                int req_mat_qty, int *req_mats,
                                float **data_src_arrays,
                                int *data_src_qty_blocks,
                                Int_2tuple **data_src_obj_blocks,
                                int *node_ref_counts,
                                float *p_mins, int *p_min_ids, float *p_maxs,
                                int *p_max_ids )
{
    int i, j, k, l, m;
    float minval, maxval;
    int min_id, max_id;
    int qty_blocks;
    Int_2tuple *obj_blocks;
    float *data;
    int first, last, idx;
    int srec_id;
    MO_class_data *p_mat_class;
    Material_data *p_md;
    float *p_sand_flags;
    int node_qty;
    int *connects, *elem_conns;
    static Bool_type warn_once = TRUE;
    float val;
    int *node_work_array;
    int conn_qty;

    srec_id = analy->cur_result->srec_id;

    /* Get reference to the material class. */
    p_mat_class = ((MO_class_data **)
                   MESH_P( analy )->classes_by_sclass[M_MAT].list)[0];

    /* Get working array for node reference counts. */
    node_qty = MESH_P( analy )->node_geom->qty;
    node_work_array = NEW_N( int, node_qty,
                             "Node reference counts working array" );

    /* Loop over states to fill min/max table. */
    for ( i = 0; i < qty_states; i++ )
    {
        analy->cur_state = min_state + i;
        analy->db_get_state( analy, min_state + i, analy->state_p,
                             &analy->state_p, NULL );

        /* Update result */
        load_result( analy, TRUE, FALSE, FALSE );

        if ( analy->cur_result->srec_id != srec_id
                && warn_once )
        {
            popup_dialog( WARNING_POPUP,
                          "Multiple state record formats encountered.  No \n"
                          "checking is performed to ensure that min's/max's \n"
                          "are drawn from identical domains in each format." );
            warn_once = FALSE;
        }

        /* Copy the node reference counts. */
        for ( j = 0; j < node_qty; j++ )
            node_work_array[j] = node_ref_counts[j];

        /*
         * Decrement node reference counts for deleted elements of the
         * requested materials at the current state.
         */
        for ( j = 0; j < req_mat_qty; j++ )
        {
            /* Get the SAND flags for the current material's element class. */
            p_md = p_mat_class->objects.materials + req_mats[j];
            p_sand_flags = analy->state_p->elem_class_sand[
                               p_md->elem_class->elem_class_index];

            /* If no failure data available, don't try to decrement. */
            if ( p_sand_flags == NULL )
                continue;

            /* Get connectivity information for the element class. */
            connects = p_md->elem_class->objects.elems->nodes;
            conn_qty = qty_connects[p_md->elem_class->superclass];

            qty_blocks = p_md->elem_block_qty;
            obj_blocks = p_md->elem_blocks;

            /* Loop over element blocks to get the affecting elem id's... */
            for ( k = 0; k < qty_blocks; k++ )
            {
                l = obj_blocks[k][0];
                last = obj_blocks[k][1] + 1;
                elem_conns = connects + l * conn_qty;

                /* Loop over current block. */
                for ( ; l < last; l++ )
                {
                    /* If element deleted, decrement references to its nodes. */
                    if ( p_sand_flags[l] == 0.0 )
                        for ( m = 0; m < conn_qty; m++ )
                            node_work_array[elem_conns[m]]--;

                    elem_conns += conn_qty;
                }
            }
        }

        /* Loop again over requested materials to search for min's/max's. */
        for ( j = 0; j < req_mat_qty; j++ )
        {
            idx = j * qty_states + i;
            qty_blocks = data_src_qty_blocks[j];
            obj_blocks = data_src_obj_blocks[j];
            data = data_src_arrays[j];

            /* Init min's/max's for current state. */
            first = obj_blocks[0][0];
            maxval = minval = data[first];
            max_id = min_id = first + 1;

            /* Loop over blocks... */
            for ( k = 0; k < qty_blocks; k++ )
            {
                first = obj_blocks[k][0];
                last = obj_blocks[k][1] + 1;

                /* Loop over current block. */
                for ( l = first; l < last; l++ )
                {
                    /* Ignore node if all referencing elements have failed. */
                    if ( node_work_array[l] == 0 )
                        continue;

                    val = data[l];

                    if ( val > maxval )
                    {
                        maxval = val;
                        max_id = l + 1;
                    }

                    if ( val < minval )
                    {
                        minval = val;
                        min_id = l + 1;
                    }
                }
            }

            p_mins[idx] = minval;
            p_min_ids[idx] = min_id;

            p_maxs[idx] = maxval;
            p_max_ids[idx] = max_id;
        }
    }
}


/*****************************************************************
 * TAG( write_elem_mm_report )
 *
 * Output to text file min/max data for element result.
 */
static void
write_mm_report( Analysis *analy, FILE *outfile, int min_state, int max_state,
                 float *mins, int *min_ids, float *maxs, int *max_ids,
                 int req_mat_qty, int *req_mats, int *block_list_sizes,
                 Int_2tuple **block_lists, Bool_type show_mat_result )
{
    int i, j, min_st, max_st, limit_st, qty_states;
    int ident_width, object_label_width, node_pre;
    int mat;
    char *mat_name;
    int query_ints[2];
    float minval, maxval;
    /*char *header =
        "# Material  State     Time       Maximum%.*s%s%.*sMinimum%.*s%s\n"; */
    char *header =
        "# Material  State     Time       Maximum    %.*s%s%.*sMinimum    %.*s%s\n";
    char *header_mat =
        "State    Time          Maximum    Material     Minimum    Material\n";
    char *object_label;
    char *min_label = " <min", *max_label = " <<<max";
    /*char *std_out_format = "   %4d    %5d  %12.6e %10.3e %*d  %10.3e %*d\n";
    char *std_out_format_mat = " %5d   %12.6e %10.3e     %5d    %10.3e     %5d\n";
    char *mm_out_format =
        "   %4d    %5d  %12.6e %10.3e %*d  %10.3e %*d\t%s";
    char *mm_out_format_mat =
        " %5d   %12.6e %10.3e     %5d    %10.3e     %5d\t%s "; */
    char *std_out_format = "   %4d    %5d  %12.6e  %13.6e %*d  %13.6e %*d\n";
    char *std_out_format_mat = " %5d   %12.6e  %13.6e     %5d    %13.6e     %5d\n";
    char *mm_out_format =
        "   %4d    %5d  %12.6e  %13.6e %*d  %13.6e %*d\t%s";
    char *mm_out_format_mat =
        " %5d   %12.6e %13.6e     %5d    %13.6e     %5d\t%s ";

    char *header_new = "#                           Material  State     Time       Maximum    %.*s%s%.*sMinimum    %.*s%s\n";
    char *std_out_format_new = "   %32s    %5d  %12.6e  %13.6e %*d  %13.6e %*d\n";
    char *mm_out_format_new =  "   %32s    %5d  %12.6e  %13.6e %*d  %13.6e %*d\t%s";

    char *blanks = "                    ";
    Bool_type min_first;
    float *mat_min_row, *mat_max_row;
    float *state_times;
    int *mat_min_id, *mat_max_id;
    int object_max, mat_object_max;

    /* Dump preamble stuff. */
    qty_states = max_state - min_state + 1;
    fprintf( outfile, "# Report of %s min/max values by material.\n",
             analy->cur_result->title );
    fprintf( outfile, "# Quantity of materials reported - %d\n"
             "# States reported (first/last/qty) - %d/%d/%d\n", req_mat_qty,
             min_state + 1, max_state + 1, qty_states );

    write_preamble( outfile );

    /* Calculate field width parameters. */

    object_max = 0;
    for ( i = 0; i < req_mat_qty; i++ )
    {
        mat_object_max = block_lists[i][block_list_sizes[i] - 1][1] + 1;
        if ( mat_object_max > object_max )
            object_max = mat_object_max;
    }

    ident_width = (int) ((double) 1.0 + log10( (double) object_max ));

    if ( !show_mat_result )
        object_label = analy->cur_result->origin.is_node_result
                       ? "Node" : "Elem";
    else
    {
        object_label = " Material";
        req_mat_qty = 1;
    }

    object_label_width = strlen( object_label );

    if ( ident_width < object_label_width )
        ident_width = object_label_width;

    node_pre = (ident_width - object_label_width + 1) / 2;

    /* Inits. */
    mat_min_row = mins;
    mat_min_id = min_ids;
    mat_max_row = maxs;
    mat_max_id = max_ids;

    query_ints[0] = min_state + 1;
    query_ints[1] = max_state + 1;
    state_times = NEW_N( float, qty_states, "state times array" );
    analy->db_query( analy->db_ident, QRY_SERIES_TIMES, query_ints, NULL,
                     (void *) state_times );

    /* Loop over materials, dumping all states per material. */
    for ( i = 0; i < req_mat_qty; i++ )
    {

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

        mat = req_mats[i] + 1;

        if(mat > 0){
        	mat_name = malloc(33 * sizeof(char));
        	Htable_entry *tempEnt;
			char tempname[32];
			sprintf(tempname,"%d",mat);
			htable_search(analy->mat_labels_reversed,tempname,FIND_ENTRY,&tempEnt);
			if(tempEnt != NULL){
				sprintf( mat_name, tempEnt->data);
			}
        }

        /* Dump column headers. */
        fprintf( outfile, "#\n#\n" );

        if ( !show_mat_result )
        	if(mat_name){
				fprintf( outfile, header_new,
						 2 + node_pre, blanks, object_label,
						 ident_width - object_label_width - node_pre + 4, blanks,
						 2 + node_pre, blanks, object_label );
        	}
        	else{
				fprintf( outfile, header,
						 2 + node_pre, blanks, object_label,
						 ident_width - object_label_width - node_pre + 4, blanks,
						 2 + node_pre, blanks, object_label );
        	}
        else
            fprintf( outfile, header_mat );

        /* Dump states before the first min or max extreme. */

        limit_st = min_first ? min_st : max_st;
        for ( j = 0; j < limit_st; j++ )
            if ( !show_mat_result )
            {
            	if(mat_name){
            		fprintf( outfile, std_out_format_new,
							 mat_name, min_state + j + 1, state_times[j],
							 mat_max_row[j], ident_width, mat_max_id[j],
							 mat_min_row[j], ident_width, mat_min_id[j] );
            	}
            	else{
            		fprintf( outfile, std_out_format,
							 mat, min_state + j + 1, state_times[j],
							 mat_max_row[j], ident_width, mat_max_id[j],
							 mat_min_row[j], ident_width, mat_min_id[j] );
            	}
            }
            else
            {
                fprintf( outfile, std_out_format_mat,
                         min_state + j + 1, state_times[j],
                         mat_max_row[j], ident_width, mat_max_id[j],
                         mat_min_row[j], ident_width, mat_min_id[j] );
            }

        /* Dump the min or max extreme which occurs first. */

        if ( !show_mat_result ){
        	if(mat_name){
				fprintf( outfile, mm_out_format_new,
						 mat_name, min_state + j + 1, state_times[j], mat_max_row[j],
						 ident_width, mat_max_id[j], mat_min_row[j], ident_width,
						 mat_min_id[j], (min_first ? min_label : max_label) );
        	}
        	else{
        		fprintf( outfile, mm_out_format,
						 mat, min_state + j + 1, state_times[j], mat_max_row[j],
						 ident_width, mat_max_id[j], mat_min_row[j], ident_width,
						 mat_min_id[j], (min_first ? min_label : max_label) );
        	}
        }
        else
        {
            fprintf( outfile, mm_out_format_mat,
                     min_state + j + 1, state_times[j], mat_max_row[j],
                     mat_max_id[j], mat_min_row[j],
                     mat_min_id[j], (min_first ? min_label : max_label) );
        }

        if ( min_st == max_st )
            fprintf( outfile, " %s\n", max_label );
        else
            fprintf( outfile, "\n" );
        j++;

        /* Dump states after first extreme but before second extreme. */
        limit_st = min_first ? max_st : min_st;
        for ( ; j < limit_st; j++ )
            if ( !show_mat_result ){
            	if(mat_name){
            		fprintf( outfile, std_out_format_new,
							 mat_name, min_state + j + 1, state_times[j], mat_max_row[j],
							 ident_width, mat_max_id[j], mat_min_row[j],
							 ident_width, mat_min_id[j] );
            	}
            	else{
					fprintf( outfile, std_out_format,
							 mat, min_state + j + 1, state_times[j], mat_max_row[j],
							 ident_width, mat_max_id[j], mat_min_row[j],
							 ident_width, mat_min_id[j] );
            	}
            }
            else{
                fprintf( outfile, std_out_format_mat,
                         min_state + j + 1, state_times[j], mat_max_row[j],
                         mat_max_id[j], mat_min_row[j],
                         mat_min_id[j] );
            }

        /* Dump the second min or max extreme. */
        if ( min_st != max_st )
        {
            if ( !show_mat_result )
            	if(mat_name){
					fprintf( outfile, mm_out_format_new,
							 mat_name, min_state + j + 1, state_times[j], mat_max_row[j],
							 ident_width, mat_max_id[j], mat_min_row[j],
							 ident_width, mat_min_id[j],
							 (min_first ? max_label : min_label) );
            	}
            	else{
            		fprintf( outfile, mm_out_format,
							 mat, min_state + j + 1, state_times[j], mat_max_row[j],
							 ident_width, mat_max_id[j], mat_min_row[j],
							 ident_width, mat_min_id[j],
							 (min_first ? max_label : min_label) );
            	}
            else
                fprintf( outfile, mm_out_format_mat,
                         min_state + j + 1, state_times[j], mat_max_row[j],
                         mat_max_id[j], mat_min_row[j],
                         mat_min_id[j],
                         (min_first ? max_label : min_label) );

            fprintf( outfile, "\n" );
            j++;
        }

        /* Dump the states after the second extreme. */
        for ( ; j < qty_states; j++ )
            if ( !show_mat_result ){
            	if(mat_name){
					fprintf( outfile, std_out_format_new,
							 mat_name, min_state + j + 1, state_times[j], mat_max_row[j],
							 ident_width, mat_max_id[j], mat_min_row[j],
							 ident_width, mat_min_id[j] );
            	}
            	else{
            		fprintf( outfile, std_out_format,
							 mat, min_state + j + 1, state_times[j], mat_max_row[j],
							 ident_width, mat_max_id[j], mat_min_row[j],
							 ident_width, mat_min_id[j] );
            	}
            }
            else{
                fprintf( outfile, std_out_format_mat,
                         min_state + j + 1, state_times[j], mat_max_row[j],
                         mat_max_id[j], mat_min_row[j],
                         mat_min_id[j] );
            }
        mat_min_row += qty_states;
        mat_min_id += qty_states;
        mat_max_row += qty_states;
        mat_max_id += qty_states;
        free(mat_name);
        //free(tempEnt);
    }
    free( state_times );
}

/*****************************************************************
 * TAG( get_extreme_minmax )
 *
 * Get the extreme element min/maxes for the currently displayed
 * result.
 *
 * Just walk through all the states without redisplaying the mesh.
 */
extern void
get_extreme_minmax( Analysis *analy, int minmax,
                    int view_state)
{
    int id_qty, class_qty, elem_qty, node_qty, obj_index, obj_qty;
    int cur_state, max_state, min_state;
    int index, class_index, elem_index, node_index;
    int i, j, k, l;
    int mat_qty;
    int sclass;
    Bool sclass_ok=FALSE;

    unsigned char *disable_mat;

    float max_val=-1.0;

    float *data_buffer;
    float  temp_mm[2];

    int    result_qty;

    MO_class_data *p_class,
                  *p_node_class,
                  **mo_classes;

    Result     *p_result;
    int         subrec, subrec_qty, srec=0, obj_id, *object_ids, result_index=0;
    Subrec_obj *p_subrec;

    Bool_type interp=TRUE;

    /* Pointer to current mesh. */

    analy->extreme_result = TRUE;
    p_result = analy->cur_result;
    if ( p_result == NULL)
        return;

    p_node_class = MESH_P( analy )->node_geom;
    node_qty     = p_node_class->qty;

    subrec_qty = p_result->qty;
    index  = analy->result_index;
    subrec = p_result->subrecs[index];
    srec   = p_result->srec_id;
    p_subrec = analy->srec_tree[srec].subrecs + subrec;
    sclass      = p_result->superclasses[index];
    obj_qty     = p_subrec->subrec.qty_objects;
    p_class     = p_subrec->p_object_class;

    /* Initialize Class data buffers */
    for ( i=0;
            i<subrec_qty;
            i++ )
    {
        subrec = p_result->subrecs[i];
        srec   = p_result->srec_id;
        p_subrec = analy->srec_tree[srec].subrecs + subrec;
        p_class = p_subrec->p_object_class;
        if ( p_class->data_buffer_mm==NULL )
        {
            p_class->data_buffer_mm = NEW_N( float, p_class->qty, "Class data buffer for min/max data" );
            for ( j=0;
                    j<p_class->qty;
                    j++ )
            {
                if (minmax==EXTREME_MIN)
                    p_class->data_buffer_mm[j] = MAXFLOAT;
                else
                    p_class->data_buffer_mm[j] = -MAXFLOAT;

            }
        }
    }

    /* Determine the range of states */
    min_state = GET_MIN_STATE( analy );
    max_state = get_max_state( analy );

    /* Make sure that the optional view state is in range */
    if ( view_state < min_state )
        view_state = min_state;
    if ( view_state > max_state )
        view_state = max_state;

    if ( analy->extreme_node_mm==NULL )
        analy->extreme_node_mm = NEW_N( float , node_qty,  "analy->extreme_node_mm" );

    /* Loop over all states  - reading the result value for each element or node */
    for ( i  = min_state;
            i <= max_state;
            i++ )
    {
        analy->cur_state = i;
        wrt_text( "\n\n");
        analy->db_get_state( analy, i, analy->state_p, &analy->state_p, NULL );
        wrt_text( "Computing Extreme Max for State %d out of %d states.", analy->cur_state + 1, max_state+1 );

        load_result( analy, TRUE, TRUE, TRUE );

        data_buffer = NODAL_RESULT_BUFFER( analy );

        /* Update min/max for nodal data */
        for (l=0;
                l<node_qty;
                l++)
        {
            obj_id = l;
            if (minmax==EXTREME_MIN)
            {
                if (data_buffer[obj_id] < analy->extreme_node_mm[obj_id])
                    analy->extreme_node_mm[obj_id] = data_buffer[obj_id];
            }
            else
            {
                if (data_buffer[obj_id] > analy->extreme_node_mm[obj_id])
                    analy->extreme_node_mm[obj_id] = data_buffer[obj_id];
            }
        }

        for ( j=0;
                j<subrec_qty;
                j++ )
        {
            subrec = p_result->subrecs[j];
            srec   = p_result->srec_id;
            p_subrec = analy->srec_tree[srec].subrecs + subrec;
            p_class = p_subrec->p_object_class;
            for ( k=0;
                    k<p_class->qty;
                    k++ )
            {

                if (minmax==EXTREME_MIN)
                {
                    if ( p_class->data_buffer[k] < p_class->data_buffer_mm[k] )
                        p_class->data_buffer_mm[k] = p_class->data_buffer[k];
                }
                else
                {
                    if ( p_class->data_buffer[k] > p_class->data_buffer_mm[k] )
                    {
                        p_class->data_buffer_mm[k] = p_class->data_buffer[k];
                    }
                }
            }
        }
    } /* Loop over states */


    /* Done collecting all of the data for all states -
     *      * Nodal data is stored in array analy->extreme_node_mm
     *      * Element data (by sclass) is stored in array minmax_elem
     */
    for (node_index=0;
            node_index<node_qty;
            node_index++)
    {
        data_buffer[node_index] = analy->extreme_node_mm[node_index];
        if (data_buffer[node_index]>max_val)
            max_val=data_buffer[node_index];
    }

    /* update_extreme_mm( analy ); */

    for ( j=0;
            j<subrec_qty;
            j++ )
    {
        analy->result_index = j;
        subrec = p_result->subrecs[j];
        srec   = p_result->srec_id;
        p_subrec = analy->srec_tree[srec].subrecs + subrec;
        p_class = p_subrec->p_object_class;

        if ( p_class->data_buffer_mm )
        {
            for ( k=0;
                    k<p_class->qty;
                    k++ )
                p_class->data_buffer[k] = p_class->data_buffer_mm[k];

            if ( p_result->origin.is_elem_result )
                elem_get_minmax(p_class->data_buffer, p_class->qty, analy );
        }
        update_nodal_min_max( analy );
    }

    free_extreme_mm( analy );

    update_min_max( analy );

    /* Free up temporary storage */

    analy->cur_result     = p_result;
    analy->extreme_result = TRUE;

    return;
}

/*****************************************************************
 * TAG( free_exteme_minmax )
 *
 * Free up all temp extreme max data.
 */
extern void
free_extreme_mm( Analysis *analy )
{
    int j,k;
    int index, obj_qty, sclass;
    Result     *p_result;
    int         subrec, subrec_qty, srec=0, obj_id, *object_ids, result_index=0;

    MO_class_data *p_class;
    Subrec_obj *p_subrec;

    if ( !analy->extreme_result )
        return;

    analy->extreme_result = FALSE;

    p_result   = analy->cur_result;
    subrec_qty = p_result->qty;
    index      = analy->result_index;
    subrec     = p_result->subrecs[index];
    srec       = p_result->srec_id;
    p_subrec   = analy->srec_tree[srec].subrecs + subrec;
    sclass     = p_result->superclasses[index];
    obj_qty    = p_subrec->subrec.qty_objects;
    p_class    = p_subrec->p_object_class;

    if ( analy->extreme_node_mm!=NULL )
    {
        free( analy->extreme_node_mm );
        analy->extreme_node_mm=NULL;
    }

    for ( j=0;
            j<subrec_qty;
            j++ )
    {
        subrec   = p_result->subrecs[j];
        srec     = p_result->srec_id;
        p_subrec = analy->srec_tree[srec].subrecs + subrec;
        p_class  = p_subrec->p_object_class;
        if ( p_class->data_buffer_mm )
        {
            free (p_class->data_buffer_mm);
            p_class->data_buffer_mm = NULL;
        }
    }
}

/*****************************************************************
 * TAG( update_extreme_minmax )
 *
 * Update the extreme element min/maxes for the currently displayed
 * result.
 */
extern void
update_extreme_mm( Analysis *analy )
{
    int j,k;
    int index, obj_qty, node_index=0, node_qty=0, sclass;
    Result     *p_result;
    int         subrec, subrec_qty, srec=0, obj_id, *object_ids, result_index=0;

    MO_class_data *p_class, *p_node_class;;
    Subrec_obj *p_subrec;

    float *data_buffer;

    if ( !analy->extreme_result )
        return;

    data_buffer = NODAL_RESULT_BUFFER( analy );

    p_node_class = MESH_P( analy )->node_geom;
    node_qty     = p_node_class->qty;

    p_result   = analy->cur_result;
    subrec_qty = p_result->qty;
    index      = analy->result_index;
    srec       = p_result->srec_id;
    subrec = p_result->subrecs[index];
    p_subrec   = analy->srec_tree[srec].subrecs + subrec;
    sclass     = p_result->superclasses[index];
    obj_qty    = p_subrec->subrec.qty_objects;
    p_class    = p_subrec->p_object_class;

    for (node_index=0;
            node_index<node_qty;
            node_index++)
        data_buffer[node_index] = analy->extreme_node_mm[node_index];
    update_nodal_min_max( analy );

    if ( p_result->origin.is_elem_result )
        for ( j=0;
                j<subrec_qty;
                j++ )
        {
            subrec = p_result->subrecs[j];
            srec   = p_result->srec_id;
            p_subrec = analy->srec_tree[srec].subrecs + subrec;
            p_class = p_subrec->p_object_class;
            if ( p_class->data_buffer_mm )
            {
                for ( k=0;
                        k<p_class->qty;
                        k++ )
                    p_class->data_buffer[k] = p_class->data_buffer_mm[k];

                elem_get_minmax(p_class->data_buffer, p_class->qty, analy );
            }
        }

    update_min_max( analy );

    analy->cur_result = p_result;
}

/*******************/
/* End of minmax.c */
/*******************/
