/* $Id$ */
/*
 * show.c - Parser for 'show' command to display analysis results.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Aug 28 1992
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
 * of the Federal Government are reserved under Contract 48 subject to
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
 *
 *  I. R. Corey - Oct 25, 2007: TBD.
 *                See SRC#---.
 *
 *************************************************************************
 */

#include <stdlib.h>
#include "viewer.h"

static Result current_result, new_result;

/* Local routines. */
void lookup_global_minmax( Result *p_result, Analysis *analy );


/*****************************************************************
 * TAG( parse_show_command )
 *
 * Parses the "show" command for displaying result values.  Returns
 * 1 if a valid command was given and 0 if the command was invalid.
 */
int
parse_show_command( char *token, Analysis *analy )
{
    Result es_result;
    Result combined_es_result;
    Result * res_ptr;
    Result * ptr;
    Result_candidate *p_rc;
    Bool_type Match = FALSE;
    static char mat[] = "mat";
    int index_qty=0;
    char root[G_MAX_NAME_LEN + 1];
    char component[G_MAX_NAME_LEN + 1];
    char es_name[125];
    char name[125];
    char newcmd[64];
    int indices[G_MAX_ARRAY_DIMS];
    int i=0;
    int * subrecs;
    int j, k, ii;
    int rval;
    int qty_candidates;
    int es_qty = 0;
    int qty=0;
    int status=OK;
    int *map;
    char ipt[125];
    char c_ptr = NULL;
    es_Result_candidate *p_es_rc;
    Hash_table * p_es_components_ht;
    Hash_table * p_sv_ht;
    Htable_entry *p_hte;
    Htable_entry *p_hte2;
    State_variable * p_sv;
    int subrecqty;
    Subrec_obj * subrec;
    int qty_classes = 0;
    char *class_names[2000];
    int  super_classes[2000];

    p_es_components_ht = analy->es_components_table;
    p_sv_ht = analy->st_var_table;

    status = mili_get_class_names(analy, &qty_classes, class_names, super_classes);

    analy->showmat = FALSE;
    if(analy->int_labels != NULL)
    {
        analy->int_labels->use_combined = FALSE;
    }

    /* Cache the current global min/max. */
    if ( !analy->ei_result )
        cache_global_minmax( analy );

    /* If request is for "mat", don't bother to look it up. */
    if ( strcmp( token, mat ) == 0 )
    {
        analy->cur_result = NULL;
        analy->showmat = TRUE;
    }
    else if (FALSE && strncmp( token, "es_", 3 )== 0 )
    {
        init_result( &es_result );
        if ( !find_result( analy, analy->result_source, TRUE, &es_result,
                           token ) )
        {
            popup_dialog( INFO_POPUP, "Result \"%s\" not found.", token );
            return( 0 );
        }
        if ( !es_result.single_valued )
        {
            /* An es result of the form es_n[var] is really an invalid specification that
             * needs to be resolved when we load the result. The result will later be
             * converted to the form es_n[i,var] where I is the integration point.
             * for now use the component as the result but we  will look at the original
             * result later to build a result spec for the load.
             */

            /* Divide the specification into its component parts. */
            if ( !parse_result_spec( token, root, &index_qty, indices, component ) )
            {
                return 0;
            }
            /* now that element sets for multiple integration points are supported just redirect to the component */
            strcpy(newcmd, "show ");
            strcat(newcmd, component);
            parse_command(newcmd, analy); 
 
        }
        else
        {
            current_result = es_result;
        }
        init_result( &es_result );
        init_result( &new_result );

        analy->cur_result = &current_result;
        analy->cur_result->reference_count++; 
    }
    else if ( find_result( analy, analy->result_source, TRUE, &new_result,
                           token ) && analy->int_labels != NULL || !htable_search(p_es_components_ht, token, FIND_ENTRY, &p_hte)) 
    {
        /* since we are displaying a new result should initialize original_results array values to NULL */
        for(i = 0; i < ORIG_RESULTS; i++)
        {
            analy->original_results[i] = NULL;
        }

        res_ptr = create_result_list(token, analy);
        subrecs = (int *) calloc(analy->srec_tree->qty + 1, sizeof(int));
 
        if(res_ptr != NULL)
        {
            qty = 0;
            j = 0;
            for(ptr = res_ptr; ptr != NULL; ptr = ptr->next)
            {
                for(i = 0; i < ptr->qty; i++)
                {
                    if(subrecs[ptr->subrecs[i]] == 0)
                    {
                        qty++;
                        subrecs[ptr->subrecs[i]] = 1;
                    }
                }
                if(!strncmp(ptr->name, "es_", 3))
                {
                    strncpy(analy->int_labels->result_names[j], ptr->name, strlen(ptr->name) + 1);
                    j++;
                }
            }
        } else
        {
            popup_dialog( INFO_POPUP, "Result \"%s\" not found.", token );
            return( 0 );
        }

        combined_es_result.qty = qty;
        combined_es_result.subrecs = calloc(qty, sizeof(int));
        combined_es_result.superclasses = calloc(qty, sizeof(int));
        combined_es_result.indirect_flags = calloc(qty, sizeof(int));
        combined_es_result.result_funcs = (void **) calloc(qty, sizeof(void (**)()));
        if(combined_es_result.subrecs == NULL || combined_es_result.superclasses == NULL ||
           combined_es_result.indirect_flags == NULL || combined_es_result.result_funcs == NULL)
        {
            popup_dialog(WARNING_POPUP, "Out of memory in function parse_show_command. exiting\n");
            parse_command("quit", analy);
        }
        combined_es_result.original_names = (char **) malloc(qty*sizeof(char *));
       
        combined_es_result.primals = NEW_N(char ***, qty, "Result primals array");
        i = 0;
        for(ptr = res_ptr; ptr != NULL; ptr = ptr->next)
        {
            if(ptr->primals != NULL)
            {
                for(j = 0; j < ptr->qty; j++)
                {
                    if(subrecs[ptr->subrecs[j]] == 1)
                    {
                        combined_es_result.primals[i+j] = ptr->primals[j];
                        subrecs[ptr->subrecs[j]]++;
                    } else
                    {
                        continue;
                    }
                }
            }

            i++;
        } 
        /* now reset the subrecs array values > 0 back to 1 */
        for(i = 0; i < analy->srec_tree->qty + 1; i++)
        {
            if(subrecs[i] > 0)
            {
                subrecs[i] = 1;
            } 
        }

        i = 0;
        for(ptr = res_ptr; ptr != NULL; ptr = ptr->next)
        {
            for(k = 0; k < ptr->qty; k++)
            {
                if(subrecs[ptr->subrecs[k]] == 1)
                {
                    combined_es_result.original_names[i + k] = malloc(strlen(ptr->name) + 1);
                    strcpy(combined_es_result.original_names[i + k], ptr->original_name);
                    subrecs[ptr->subrecs[k]]++;
                } else
                {
                    continue;
                } 
            }
            i++;
        }

        for(i = 0; i < analy->srec_tree->qty + 1; i++)
        {
            if(subrecs[i] > 0)
            {
                subrecs[i] = 1;
            } 
        }
 
        i = 0;
        c_ptr = res_ptr->name[0];   
        while( c_ptr != NULL && c_ptr != ']')
        {
           c_ptr = res_ptr->name[i];
           if(c_ptr == '[')
           {
               i++;
               Match = TRUE;
               break;
           }
           i++; 
        }
        
        if(Match)
        {
            j = 0;
            do 
            {
                c_ptr = res_ptr->name[i]; 
                combined_es_result.name[j] = c_ptr;
                i++;
                j++;
            }while(res_ptr->name[i] != ']' && res_ptr->name[i] != ',');
  
            combined_es_result.name[j] = '\0';
        } else
        {
            strncpy(combined_es_result.name, res_ptr->name, strlen(res_ptr->name) + 1);
        }
        strncpy(combined_es_result.root, combined_es_result.name, strlen(combined_es_result.name)+1);
        strncpy(combined_es_result.original_name, combined_es_result.name, strlen(combined_es_result.name)+1);

        if(!strncmp(res_ptr->name, "es_", 3))
        {
            i = 0;
            c_ptr = res_ptr->title[0];
            while(c_ptr != NULL && c_ptr != ']')
            {
                c_ptr = res_ptr->title[i];
                if(c_ptr == ',')
                {
                    i += 2;
                    break;
                }
                i++; 
            } 

            j = 0;
            do
            {
                c_ptr = res_ptr->title[i];
                combined_es_result.title[j] = c_ptr;
                i++;
                j++;
            }while(res_ptr->title[i] != ']');
            
            combined_es_result.title[j] = '\0';                   
        } else if(Match)
        {
            i = 0;
            c_ptr = res_ptr->title[0];
            while(c_ptr != NULL && c_ptr != ']')
            {
                c_ptr = res_ptr->title[i];
                if(c_ptr == '[')
                {
                    i++;
                    break;
                }
                i++; 
            } 

            j = 0;
            do
            {
                c_ptr = res_ptr->title[i];
                combined_es_result.title[j] = c_ptr;
                i++;
                j++;
            }while(res_ptr->title[i] != ']');
            
            combined_es_result.title[j] = '\0';                   
        } else
        {
            strncpy(combined_es_result.root, res_ptr->root, strlen(res_ptr->root) + 1);
            strncpy(combined_es_result.name, res_ptr->name, strlen(res_ptr->name) + 1);
            strncpy(combined_es_result.title, res_ptr->title, strlen(res_ptr->title) + 1);
            strncpy(combined_es_result.original_name, combined_es_result.name, strlen(combined_es_result.name) + 1);

        }

        combined_es_result.next = NULL;
        combined_es_result.prev = NULL;
        i = 0; 
        /* now loop through the res_ptr and create the combined result */
        for(ptr = res_ptr; ptr != NULL; ptr = ptr->next)
        { 
            for(j = 0; j < ptr->qty; j++)
            {
              if(subrecs[ptr->subrecs[j]] == 1)
              {
                  combined_es_result.subrecs[i] = ptr->subrecs[j];
                  combined_es_result.superclasses[i] = ptr->superclasses[j];
                  combined_es_result.result_funcs[i] = ptr->result_funcs[j];
                  combined_es_result.indirect_flags[i] = ptr->indirect_flags[j];
                  analy->original_results[ptr->superclasses[j]] = ptr->original_result;
                  i++; 
                  subrecs[ptr->subrecs[j]]++;
              } else
              {
                  continue;
              }  
            }
            /*analy->original_results[ptr->superclasses[0]] = ptr->original_result;*/
        }

        combined_es_result.mesh_id = res_ptr->mesh_id; 
        combined_es_result.srec_id = res_ptr->srec_id; 
        combined_es_result.origin = res_ptr->origin;
        combined_es_result.modifiers = res_ptr->modifiers;
        combined_es_result.single_valued = res_ptr->single_valued;
        combined_es_result.original_result = res_ptr->original_result;
        combined_es_result.reference_count = res_ptr->reference_count;
        combined_es_result.must_recompute = res_ptr->must_recompute;
        combined_es_result.hide_in_menu = res_ptr->hide_in_menu;
        
       /* set the reference surface if needed */
       if(strstr(token, "in"))
       {
           analy->ref_surf = INNER;
       } else if (strstr(token, "out"))
       {
           analy->ref_surf = OUTER;
       }
       free(subrecs);

        if ( !combined_es_result.single_valued && analy->int_labels != NULL && !analy->int_labels->use_combined)
        {
            popup_dialog( INFO_POPUP,
                          "Result specification for \"show\" command\n"
                          "must resolve to a scalar quantity for plotting." );
            return( 0 );
        }

        if(res_ptr != NULL)
        {
            delete_result_list(&res_ptr, analy);
        }

        if(analy->int_labels != NULL && analy->int_labels->use_combined)
        {
            current_result = combined_es_result;
            init_result( &combined_es_result );
        } else 
        { 
            current_result = new_result;
            init_result( &new_result );
        } 

        current_result = combined_es_result;
        init_result( &combined_es_result);

        analy->cur_result = &current_result;
        analy->cur_result->reference_count++;
    }
    else if(analy->int_labels != NULL)
    {
        popup_dialog( INFO_POPUP, "Result \"%s\" not found.", token );
        return( 0 );
    } else if(analy->int_labels == NULL)
    {
        init_result( &new_result);
        if( find_result(analy, analy->result_source, TRUE, &new_result, token ))
        { 
            if ( !new_result.single_valued )
            {
                popup_dialog( INFO_POPUP,
                              "Result specification for \"show\" command\n"
                              "must resolve to a scalar quantity for plotting." );
                return( 0 );
            }
            current_result = new_result;
            init_result( &new_result );

            analy->cur_result = &current_result;
            analy->cur_result->reference_count++;
        } else
        {
            popup_dialog( INFO_POPUP, "Result \"%s\" not found.", token );
            return( 0 );
        }
          
    } else
    {
        popup_dialog( INFO_POPUP, "Result \"%s\" not found.", token );
        return( 0 );
    }

   
    if(analy->int_labels != NULL)
    {
        analy->int_labels->use_combined = TRUE;
    }  
    
    analy->extreme_result = FALSE; 

    load_result( analy, TRUE, TRUE, FALSE );

    /*
     * Look up the global min/max for the new variable.  This must
     * be performed after load_result() is called.
     */
    lookup_global_minmax( analy->cur_result, analy );

    /* Update cut planes, isosurfs, contours. */
    update_vis( analy );

    return( 1 );
}


/*****************************************************************
 * TAG( refresh_shown_result )
 *
 * Re-generate the current result if the result table has changed.
 */
extern Bool_type
refresh_shown_result( Analysis *analy )
{
    if ( analy->cur_result == NULL )
        return FALSE;

    if ( ( analy->cur_result->origin.is_derived
            && analy->result_source == PRIMAL )
            || ( analy->cur_result->origin.is_primal
                 && ( analy->result_source == DERIVED
                      || analy->result_source == ALL ) ) )
    {
        /* Potential change in result... */
        if ( find_result( analy, analy->result_source, TRUE, &new_result,
                          analy->cur_result->original_name ) )
        {
            if ( new_result.original_result
                    == analy->cur_result->original_result )
            {
                /* No actual change in result. */
                return FALSE;
            }
            else
            {
                if ( !new_result.single_valued )
                {
                    /* Actual result change but can't use new one. */
                    popup_dialog( INFO_POPUP, "New result is not scalar." );
                    return FALSE;
                }

                current_result = new_result;
                init_result( &new_result );
                analy->cur_result = &current_result;
                analy->cur_result->reference_count++;

                return TRUE;
            }
        }
        else
        {
            popup_dialog( INFO_POPUP, "Result \"%s\" not found.",
                          analy->cur_result->original_name );
            cleanse_result( analy->cur_result );
            analy->cur_result = NULL;
            return TRUE;
        }
    }

    return FALSE;
}


/*****************************************************************
 * TAG( cache_global_minmax )
 *
 * Takes the global min/max for the specified result type and
 * stashes it in a list for later retrieval.
 */
extern void
cache_global_minmax( Analysis *analy )
{
    float *test_mm;
    float *el_mm_vals;
    int *el_mm_id;
    char **el_mm_class;
    int  *el_mm_sclass;
    int i=0;

    Minmax_obj *mm_list;
    int *mesh_object;
    Minmax_obj *mm;
    Result *result;

    result = analy->cur_result;
    test_mm = analy->global_mm;
    mm_list = analy->result_mm_list;
    mesh_object = analy->global_mm_nodes;

    if ( result == NULL )
        return;
    if ( !strcmp( result->name, "damage" ) )
        return;

    /* See if it's already in the list. */
    for ( mm = mm_list; mm != NULL; mm = mm->next )
        if ( match_result( analy, result, &mm->result ) )
        {
            if ( test_mm[0] < mm->interpolated_minmax[0] )
            {
                mm->interpolated_minmax[0] = test_mm[0];
                mm->object_id[0] = mesh_object[0];
            }
            if ( test_mm[1] > mm->interpolated_minmax[1] )
            {
                mm->interpolated_minmax[1] = test_mm[1];
                mm->object_id[1] = mesh_object[1];
            }


            /*
             * If element result, update element minmax for use when
             * interpolation mode is "noterp".
             */
            if ( result->origin.is_elem_result )
            {
                el_mm_vals  = analy->elem_global_mm.object_minmax;
                el_mm_id    = analy->elem_global_mm.object_id;
                el_mm_class = analy->elem_global_mm.class_long_name;
                el_mm_sclass = analy->elem_global_mm.sclass;

                if ( el_mm_vals[0] < mm->object_minmax[0] )
                {
                    mm->object_minmax[0]  = el_mm_vals[0];
                    mm->object_id[0]      = el_mm_id[0];
                    mm->class_long_name[0] = el_mm_class[0];
                    mm->sclass[0]         = el_mm_sclass[0];
                }
                if ( el_mm_vals[1] > mm->object_minmax[1] )
                {
                    mm->object_minmax[1]  = el_mm_vals[1];
                    mm->object_id[1]      = el_mm_id[1];
                    mm->class_long_name[1] = el_mm_class[1];
                    mm->sclass[1]         = el_mm_sclass[1];
                }
            }
            return;
        }

    /* Isn't in the list yet, add it. */

    mm = NEW( Minmax_obj, "Minmax obj" );
    strcpy( mm->result.name, result->name );

    mm->next = NULL;

    mm->interpolated_minmax[0] = test_mm[0];
    mm->interpolated_minmax[1] = test_mm[1];
    mm->object_id[0] = mesh_object[0];
    mm->object_id[1] = mesh_object[1];
    mm->object_minmax[0] = analy->elem_state_mm.object_minmax[0];
    mm->object_minmax[1] = analy->elem_state_mm.object_minmax[1];


    /* Save these regardless of result (cheaper than all the tests). */
    mm->result.use_flags = result->modifiers.use_flags;
    mm->result.strain_variety = analy->strain_variety;
    mm->result.ref_frame = analy->ref_frame;
    mm->result.ref_surf = analy->ref_surf;
    mm->result.ref_state = analy->reference_state;

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
void
lookup_global_minmax( Result *p_result, Analysis *analy )
{
    Minmax_obj *mm;
    Bool_type found;
    int i;
    float *el_state_mm;
    int *el_state_id;
    char **el_state_class;
    int *el_state_sclass;

    el_state_mm     = analy->elem_state_mm.object_minmax;
    el_state_id     = analy->elem_state_mm.object_id;
    el_state_class  = analy->elem_state_mm.class_long_name;
    el_state_sclass = analy->elem_state_mm.sclass;


    /*
     * Lookup the global min/max for a result from the library.
     * If the global min/max is not found in the library, we must
     * use the state min/max instead.  This means that the state
     * min/max must have already been computed!
     */

    if ( p_result == NULL )
        return;

    found = FALSE;
    for ( mm = analy->result_mm_list; mm != NULL; mm = mm->next )
    {
        if ( match_result( analy, p_result, &mm->result ) )
        {
            analy->global_mm[0] = mm->interpolated_minmax[0];
            analy->global_mm_nodes[0] = mm->object_id[0];
            analy->global_mm[1] = mm->interpolated_minmax[1];
            analy->global_mm_nodes[1] = mm->object_id[1];

            analy->elem_global_mm.object_minmax[0] = mm->object_minmax[0];
            analy->elem_global_mm.object_minmax[1] = mm->object_minmax[1];


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
        if ( el_state_mm[0] < analy->elem_global_mm.object_minmax[0] )
        {
            analy->elem_global_mm.object_minmax[0]   = el_state_mm[0];
            analy->elem_global_mm.object_id[0]       = el_state_id[0];
            analy->elem_global_mm.class_long_name[0] = el_state_class[0];
            analy->elem_global_mm.sclass[0]          = el_state_sclass[0];
        }
        if ( el_state_mm[1] > analy->elem_global_mm.object_minmax[1] )
        {
            analy->elem_global_mm.object_minmax[1]   = el_state_mm[1];
            analy->elem_global_mm.object_id[1]       = el_state_id[1];
            analy->elem_global_mm.class_long_name[1] = el_state_class[1];
            analy->elem_global_mm.sclass[1]          = el_state_sclass[1];
        }
    }
    else
    {
        /* Not found, initialize global min/max to state min/max. */
        analy->global_mm[0] = analy->state_mm[0];
        analy->global_mm_nodes[0] = analy->state_mm_nodes[0];
        analy->global_mm[1] = analy->state_mm[1];
        analy->global_mm_nodes[1] = analy->state_mm_nodes[1];

        analy->elem_global_mm.object_minmax[0] = el_state_mm[0];
        analy->elem_global_mm.object_minmax[1] = el_state_mm[1];

        analy->elem_global_mm.object_id[0] = el_state_id[0];
        analy->elem_global_mm.object_id[1] = el_state_id[1];

        analy->elem_global_mm.class_long_name[0] = el_state_class[0];
        analy->elem_global_mm.class_long_name[1] = el_state_class[1];
        analy->elem_global_mm.sclass[0]          = el_state_sclass[0];
        analy->elem_global_mm.sclass[1]          = el_state_sclass[1];
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
 * TAG( reset_global_minmax )
 *
 * Remove any cached global min/max value for a result.
 */
extern Redraw_mode_type
reset_global_minmax( Analysis *analy )
{
    Minmax_obj *mm;

    if ( analy->cur_result == NULL )
        return NO_VISUAL_CHANGE;

    /* If a cached min/max for the current result exists, delete it.  */
    if ( analy->result_mm_list ) /* Added October 10, 2007: IRC - Check for a valid address */
        for ( mm = analy->result_mm_list; mm != NULL; NEXT( mm ) )
            if (  analy->result_mm_list && mm )
                if ( match_result( analy, analy->cur_result, &mm->result ) )
                {
                    DELETE( mm, analy->result_mm_list );
                    if ( !analy->result_mm_list )
                    {
                        mm=NULL;
                        break;
                    }
                }

    /* Always reset the current global min/max from the current state. */
    analy->global_mm[0] = analy->state_mm[0];
    analy->global_mm_nodes[0] = analy->state_mm_nodes[0];
    analy->global_mm[1] = analy->state_mm[1];
    analy->global_mm_nodes[1] = analy->state_mm_nodes[1];
    /*
     * NOTE:  Update entire structure
     */
    analy->elem_global_mm = analy->elem_state_mm;

    return redraw_for_render_mode( FALSE, RENDER_MESH_VIEW, analy );
}


/*****************************************************************
 * TAG( match_result )
 *
 * Match a result_id plus associated modifiers against a stored
 * result with modifiers.
 */
Bool_type
match_result( Analysis *analy, Result *p_result, Result_spec *test )
{
    /* First check result name. */
    if ( strcmp( p_result->name, test->name ) == 0 )
    {
        /* Check dependence on reference surface. */
        if ( p_result->modifiers.use_flags.use_ref_surface )
        {
            if ( test->use_flags.use_ref_surface )
            {
                if ( analy->ref_surf != test->ref_surf )
                    return FALSE;
            }
            else
                return FALSE;
        }
        else if ( test->use_flags.use_ref_surface )
            return FALSE;

        /* Check dependence on reference frame. */
        if ( p_result->modifiers.use_flags.use_ref_frame )
        {
            if ( test->use_flags.use_ref_frame )
            {
                if ( analy->ref_frame != test->ref_frame )
                    return FALSE;
            }
            else
                return FALSE;
        }
        else if ( test->use_flags.use_ref_frame )
            return FALSE;

        /* Check dependence on strain variety. */
        if ( p_result->modifiers.use_flags.use_strain_variety )
        {
            if ( test->use_flags.use_strain_variety )
            {
                if ( analy->strain_variety != test->strain_variety )
                    return FALSE;
            }
            else
                return FALSE;
        }
        else if ( test->use_flags.use_strain_variety )
            return FALSE;

        /* Check for time derivative. */
        if ( p_result->modifiers.use_flags.time_derivative )
        {
            if ( !test->use_flags.time_derivative )
                return FALSE;
        }
        else if ( test->use_flags.time_derivative )
            return FALSE;

        /* Check for global coordinate transformation. */
        if ( p_result->modifiers.use_flags.coord_transform )
        {
            if ( !test->use_flags.coord_transform )
                return FALSE;
        }
        else if ( test->use_flags.coord_transform )
            return FALSE;

        /* Check dependence on reference state. */
        if ( p_result->modifiers.use_flags.use_ref_state )
        {
            if ( test->use_flags.use_ref_state )
            {
                if ( analy->reference_state != test->ref_state )
                    return FALSE;
            }
            else
                return FALSE;
        }
        else if ( test->use_flags.use_ref_state )
            return FALSE;

        return TRUE;
    }
    else
        return FALSE;
}


