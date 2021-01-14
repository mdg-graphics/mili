/* $Id$ */
/*
 * tell.c - Parse and action routines for "tell" command reports.
 *
 *  Doug Speck
 *  Lawrence Livermore National Laboratory
 *  20 Sep 2000
 *
 ************************************************************************
 *
 * Modifications:
 *  I. R. Corey - Sept 12th, 2012: Added new option to tellpos to select
 *  'all' idents.
 *  See TeamForge#????
 *
 *  I. R. Corey - November 25th, 2012: Added fracsz argument to print-width
 *  specification for coordinates.
 *
 *  I. R. Corey - November 30th, 2012: Added loading of DP nodal coords
 *  for tell pos commands if DP data is available.
 *
 ************************************************************************
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "viewer.h"
#include "sarray.h"

#define SIZE1 1024

static char * build_extended_svar_name( char *svdesc, State_variable *p_sv, int * namesize );
static void tell_info( Analysis *analy );
static void parse_tell_times_command( Analysis *analy,
                                      char tokens[][TOKENLENGTH],
                                      int token_cnt, int *p_addl_tokens );
static void parse_tell_pos_command( Analysis *analy, char tokens[][TOKENLENGTH],
                                    int token_cnt, int *p_addl_tokens );
static void parse_tell_mm_command( Analysis *analy, char tokens[][TOKENLENGTH],
                                   int token_cnt, int *p_addl_tokens,
                                   Redraw_mode_type *p_redraw );
static void tell_coordinates( char class[], int id, Analysis *analy );
static void tell_mesh_classes( Analysis *analy );
static void tell_results( Analysis *analy );
static int max_id_string_width( int *connects, int node_cnt );
static int num_digits(int x);


/*****************************************************************
 * TAG( parse_tell_command )
 *
 * Parse a "tell" command and generate a text report.
 */
void
parse_tell_command( Analysis *analy, char tokens[][TOKENLENGTH],
                    int token_cnt, Bool_type ignore_tell_token,
                    Redraw_mode_type *p_redraw )
{
    int i, addl_tokens;

    for ( i = ignore_tell_token ? 1 : 0; i < token_cnt; i++ )
    {
        addl_tokens = 0;

        if ( strcmp( tokens[i], "info" ) == 0 )
        {
            tell_info( analy );
        }
        else if ( strcmp( tokens[i], "pos" ) == 0
                  || strcmp( tokens[i], "tellpos" ) == 0 )
        {
            parse_tell_pos_command( analy, tokens + i, token_cnt - i,
                                    &addl_tokens );
        }
        else if ( strcmp( tokens[i], "mm" ) == 0 
                  || strcmp( tokens[i], "tellmm") == 0)
        {
            parse_tell_mm_command( analy, tokens + i, token_cnt - i,
                                   &addl_tokens, p_redraw );
        }
        else if ( strcmp( tokens[i], "times" ) == 0
                  || strcmp( tokens[i], "lts" ) == 0 )
        {
            parse_tell_times_command( analy, tokens + i, token_cnt - i,
                                      &addl_tokens );
        }
        else if ( strcmp( tokens[i], "th" ) == 0 )
        {
            tell_time_series( analy );
        }
        else if ( strcmp( tokens[i], "iso" ) == 0 )
        {
            report_contour_vals( analy );
        }
        else if ( strcmp( tokens[i], "class" ) == 0 )
        {
            tell_mesh_classes( analy );
        }
        else if ( strcmp( tokens[i], "results" ) == 0 )
        {
            tell_results( analy );
        }
        else if ( strcmp( tokens[i], "view" ) == 0 )
        {
            print_view();
        }
        else if ( strcmp( tokens[i], "em" ) == 0 )
        {
            report_exp_assoc();
        }
        else
        {
            popup_dialog( INFO_POPUP, "Invalid \"tell\" request: %s",
                          tokens[i] );
        }

        i += addl_tokens;
    }
}


/*****************************************************************
 * TAG( tell_info )
 *
 * Report general information to the feedback window.
 */
static void
tell_info( Analysis *analy )
{
    int max_state;
    float pt[3];
    int i_args[4];

    max_state = get_max_state( analy );
    i_args[0] = 3;
    i_args[1] = 1;
    i_args[2] = analy->cur_state + 1;
    i_args[3] = max_state + 1;
    analy->db_query( analy->db_ident, QRY_MULTIPLE_TIMES, (void *) i_args,
                     NULL, (void *) pt );

    wrt_text( "Database information:\n" );
    wrt_text( "    Path/name: %s\n    States: %d\n",
              analy->root_name, max_state + 1 );
    if ( max_state >= 0 )
        wrt_text( "    Start time: %.4e\n    End time:   %.4e\n",
                  pt[0], pt[2] );

    wrt_text( "\nCurrent status:\n" );

    if ( max_state >= 0 )
        wrt_text( "    State: %d\n    Time: %.4e\n",
                  analy->cur_state + 1, pt[1] );

    if ( analy->cur_result != NULL )
    {
        wrt_text( "    Result %s, using min: %.4e  max: %.4e\n",
                  analy->cur_result->title, analy->result_mm[0],
                  analy->result_mm[1] );
        wrt_text( "    Result, global min: %.4e  max: %.4e\n",
                  analy->global_mm[0], analy->global_mm[1] );
        wrt_text( "    Result, state min: %.4e  max: %.4e\n",
                  analy->state_mm[0], analy->state_mm[1] );
        wrt_text( "    Result zero tolerance: %.4e\n", analy->zero_result );
    }

    wrt_text( "    Bounding box, low corner: %.4f, %.4f, %.4f\n",
              analy->bbox[0][0], analy->bbox[0][1], analy->bbox[0][2] );
    wrt_text( "    Bounding box, high corner: %.4f, %.4f, %.4f\n\n",
              analy->bbox[1][0], analy->bbox[1][1], analy->bbox[1][2] );
}


/*****************************************************************
 * TAG( parse_tell_times_command )
 *
 * Parse a "tell times" command and generate a text report.
 */
static void
parse_tell_times_command( Analysis *analy, char tokens[][TOKENLENGTH],
                          int token_cnt, int *p_addl_tokens )
{
    int min_state, max_state, qty;
    int times_token_cnt;
    int i;
    float val;
    float *f_args;
    int *i_data;

    /* Determine if optional additional times tokens are present. */
    times_token_cnt = 0;
    if ( token_cnt > 1 )
    {
        /* First additional would be the first state. */
        if ( is_numeric_token( tokens[1] ) )
        {
            times_token_cnt++;

            /* Second additional token would be last state. */
            if ( token_cnt > 2 && is_numeric_token( tokens[2] ) )
                times_token_cnt++;
        }
    }

    /* Get constrained/specified first and last states for report. */
    min_state = GET_MIN_STATE( analy );
    if ( times_token_cnt > 0 )
    {
        min_state = MAX( min_state, atoi( tokens[1] ) - 1 );

        if ( times_token_cnt > 1 )
            max_state = MIN( get_max_state( analy ), atoi( tokens[2] ) - 1 );
        else
            max_state = min_state;
    }
    else
        max_state = get_max_state( analy );

    if ( min_state > max_state )
    {
        popup_dialog( USAGE_POPUP, "%s\n%s\n%s",
                      "tell times [<first state> [<last state>]]",
                      "  Where <first state> <= <last state>",
                      "  Alias: lts" );
        return;
    }

    qty = max_state - min_state + 1;
    f_args = NEW_N( float, qty, "Times query data array" );
    i_data = NEW_N( int, qty + 1, "Times query states array" );
    i_data[0] = qty;
    for ( i = 1; i <= qty; i++ )
        i_data[i] = i + min_state;
    analy->db_query( analy->db_ident, QRY_MULTIPLE_TIMES, (void *) i_data,
                     NULL, (void *) f_args );
    wrt_text( "Database state times:\n" );
    wrt_text( "    State Num \t State Time \t Delta Time\n" );
    wrt_text( "    --------- \t ---------- \t ----------\n" );
    wrt_text( "       %3d \t %.4e\n", i_data[1], f_args[0] );
    for (  i = 1; i < qty; i++ )
    {
        val = f_args[i] - f_args[i-1];
        wrt_text( "       %3d \t %.4e \t %.4e\n", i_data[i + 1], f_args[i],
                  val );
    }
    wrt_text( "\n" );
    free( f_args );
    free( i_data );

    *p_addl_tokens = times_token_cnt;
}


/*****************************************************************
 * TAG( parse_tell_pos_command )
 *
 * Parse a "tell pos" command and generate a text report.
 */
static void
parse_tell_pos_command( Analysis *analy, char tokens[][TOKENLENGTH],
                        int token_cnt, int *p_addl_tokens )
{
    int object_id, pos_addl_tokens;
    Bool_type parse_failure;
    int i=0;

    pos_addl_tokens = 0;

    parse_failure = FALSE;

    if ( token_cnt > 1 )
    {
        MO_class_data *p_mo_class;

        p_mo_class = find_named_class( MESH_P( analy ), tokens[1] );

        if ( p_mo_class != NULL )
        {
            pos_addl_tokens++;

            if ( is_numeric_token( tokens[2] ) )
            {
                pos_addl_tokens++;
                object_id = atoi( tokens[2] );
                tell_coordinates( tokens[1], object_id, analy );
            }
            else if ( !strcmp( "all", tokens[2] ) )
            {
                pos_addl_tokens++;
                for ( i=0;
                        i<p_mo_class->qty;
                        i++ )
                    tell_coordinates( tokens[1], i+1, analy );
            }
            else
                parse_failure = TRUE;
        }
        else
            parse_failure = TRUE;
    }
    else
        parse_failure = TRUE;

    if ( parse_failure )
        popup_dialog( USAGE_POPUP,
                      "tell pos <class name> <ident>" );

    *p_addl_tokens = pos_addl_tokens;
}


/*****************************************************************
 * TAG( tell_results )
 *
 * Write a summary of available primal and derived results.
 */
static void
tell_results( Analysis *analy )
{
    int i, j, k, idx, arrsize, namesize = SIZE1;
    int superclass;
    int res_qty, srec_qty, subrec_qty, c_qty;
    int rval;
    Mesh_data *p_md;
    MO_class_data **p_mo_classes;
    Derived_result **pp_dr;
    Primal_result **pp_pr;
    State_variable *p_sv;
    StringArray sa_arr[QTY_SCLASS] = { NULL };
    Subrecord_result *sr_array;
    Subrec_obj *p_subrec, *p_subrecs;
    Htable_entry *p_hte;
    Result_candidate *p_cand;
    char * names = NULL;
    char * tmp_names = NULL;
    char * current = NULL;
    char * more_current = NULL;
    char *sclass_names[QTY_SCLASS] =
    {
        "G_UNIT", "G_NODE", "G_TRUSS", "G_BEAM", "G_TRI", "G_QUAD", "G_TET",
        "G_PRISM", "G_WEDGE", "G_HEX", "G_SURFACE", "G_MAT", "G_MESH", "G_PARTICLE"
    };
    char *p_short, *p_long;
    Bool_type have_result;
    int max_len = 0, tmp_len;
    int *i_array;
    Hash_table *p_ht;
    typedef struct _class_primals
    {
        MO_class_data *p_class;
        StringArray sa;
    } Class_primals;
    Class_primals *p_cp;
    StringArray sa;

    srec_qty = analy->qty_srec_fmts;
    names = (char *) malloc(SIZE1 * sizeof(char));
    if(names == NULL)
    {

        printf("Out of memory in file %s, line %d, exiting\n", __FILE__, __LINE__);
        exit(1);
    }

    /*
     * Derived results.
     */

#ifdef NEWMILI
    htable_get_data( analy->derived_results, (void ***) &pp_dr ,&res_qty);
#else
    res_qty = htable_get_data( analy->derived_results, (void ***) &pp_dr);
#endif

    /* Sort result names into StringArray's by superclass. */
    for ( i = 0; i < res_qty; i++ )
    {
        for ( j = 0; j < srec_qty; j++ )
        {
            subrec_qty = pp_dr[i]->srec_map[j].qty;
            sr_array = (Subrecord_result *) pp_dr[i]->srec_map[j].list;

            for ( k = 0; k < subrec_qty; k++ )
            {
                p_cand = sr_array[k].candidate;
                superclass = p_cand->superclass;
                idx = sr_array[k].index;

                /* Init the StringArray if first insertion. */
                if ( sa_arr[superclass] == NULL )
                    SANEWS( sa_arr[superclass], 4096 );


                if(strlen(p_cand->short_names[idx]) + strlen(p_cand->long_names[idx]) + 3  > SIZE1)
                {

                    namesize = strlen(p_cand->short_names[idx]) + strlen(p_cand->long_names[idx]) + 3;
                    /* Need to increase the memory allocated for names */
                    tmp_names = (char *) realloc(names, namesize*sizeof(char));
                    if(tmp_names == NULL)
                    {
                        printf("memory allocation error in file %s line %d\n", __FILE__, __LINE__);
                        exit(1);
                    }
                    names = tmp_names;

                }
                /* Build a name that combines short and long names. */
                sprintf( names, "%s|%s", p_cand->short_names[idx],
                         p_cand->long_names[idx] );

                /* Save the longest short name length. */
                if ( (tmp_len = strlen( p_cand->short_names[idx] )) > max_len )
                    max_len = tmp_len;

                /* Insert the string into the StringArray. */
#ifdef NEWMILI
                rval = sa_add( &sa_arr[superclass], names , &arrsize);
#else
                rval = SAADD( sa_arr[superclass], names);
#endif

                if ( rval < 0 )
                    /* String insertion failed - warn but continue. */
                    popup_dialog( WARNING_POPUP,
                                  "StringArray insertion failure\n(\"%s\")",
                                  names );
            }
        }
    }

    /* max_len on line 388 should contain the largest string so now allocate memory for current */
    current = (char *) malloc(max_len);
    if( current == NULL)
    {
        popup_dialog( WARNING_POPUP, "Out of memory in file tell.c, exiting.");
        exit(1);
    }

    /* Sort StringArrays to enable duplicate detection. */
    have_result = FALSE;
    for ( i = 0; i < QTY_SCLASS; i++ )
    {
        if ( sa_arr[i] != NULL )
        {
            SASORT( sa_arr[i], 1 );
            have_result = TRUE;
        }
    }

    if ( have_result )
    {
        wrt_text( "Derived results:\n" );

        /* Write the names, ignoring duplicated short names. */
        for ( i = 0; i < QTY_SCLASS; i++ )
        {
            if ( sa_arr[i] == NULL )
                continue;

            res_qty = SAQTY( sa_arr[i] );

            wrt_text( "    For %s classes:\n", sclass_names[i] );

            /* Process the first result, saving reference to the short name. */
            strcpy( names, SASTRING( sa_arr[i], 0 ) );
            p_short = strtok( names, "|" );
            p_long = strtok( NULL, "|" );
            wrt_text( "        %-*s - %s\n", max_len, p_short, p_long );
            strcpy( current, p_short );

            for(j = 1; j < res_qty; j++)
            {
                if(strlen(SASTRING(sa_arr[i], j)) > namesize)
                {
                    namesize = strlen(SASTRING(sa_arr[i], j));

                }
            }

            if(namesize > SIZE1)
            {
                tmp_names = (char *) realloc(names, namesize*sizeof(char));
                if(tmp_names == NULL)
                {
                    printf("Memory allocation error in file %s line %d\n", __FILE__, __LINE__);
                    exit(1);
                }
            }

            for ( j = 1; j < res_qty; j++ )
            {
                strcpy( names, SASTRING( sa_arr[i], j ) );
                p_short = strtok( names, "|" );
                p_long = strtok( NULL, "|" );

                /* Skip over duplicate names. */
                if ( strcmp( current, p_short ) == 0 )
                    continue;

                wrt_text( "        %-*s - %s\n", max_len, p_short, p_long );
                strcpy( current, p_short );
            }
        }
    }

    /* Clean-up for derived results reporting. */
    for ( i = 0; i < QTY_SCLASS; i++ )
        if ( sa_arr[i] != NULL )
            free( sa_arr[i] );
    free( pp_dr );

    /*
     * Primal results.
     */
#ifdef NEWMILI
    htable_get_data( analy->primal_results, (void ***) &pp_pr, &res_qty );
#else
    res_qty = htable_get_data( analy->primal_results, (void ***) &pp_pr);
#endif

    /* go ahead and realloc names to SIZE1 before proceeding
    tmp_names = realloc(names, SIZE1*sizeof(char));
    if(tmp_names == NULL)
    {
      printf("memory allocation error in file %s lind $d\n", __FILE__, __LINE__);
      exit(1);
    }

    names = tmp_names; */

    /* at this point the amount of memory allocated for names is either SIZE1 or namesize if namesize > SIZE1 */


    if ( have_result )
        wrt_text( "\n" );

    if ( res_qty > 0 )
        wrt_text( "Primal results:\n" );

    /* Hash table for StringArray's by class. */
    p_ht = htable_create( 151 );

    /* Sort result names into StringArray's by class. */
    max_len = 0;
    for ( i = 0; i < res_qty; i++ )
    {
        /* Quick access to current primal result's State_variable struct. */
        p_sv = pp_pr[i]->var;

        /* Loop over state record formats... */
        for ( j = 0; j < srec_qty; j++ )
        {
            /* Set up to access Subrecord_obj's where current primal exists. */
            p_subrecs = analy->srec_tree[j].subrecs;
            subrec_qty = pp_pr[i]->srec_map[j].qty;
            i_array = (int *) pp_pr[i]->srec_map[j].list;

            /* Loop over subrecords where result exists... */
            for ( k = 0; k < subrec_qty; k++ )
            {
                p_subrec = p_subrecs + i_array[k];

                /* Get/create hash table entry for class of current primal. */
                htable_search( p_ht, p_subrec->p_object_class->short_name,
                               ENTER_MERGE, &p_hte );

                /* Init the StringArray if first insertion. */
                if ( p_hte->data == NULL )
                {
                    p_cp = NEW( Class_primals, "New class primals struct" );
                    p_cp->p_class = p_subrec->p_object_class;
                    SANEWS( p_cp->sa, 4096 );
                    p_hte->data = (void *) p_cp;
                }

                sa = ((Class_primals *) p_hte->data)->sa;

                /* Append state variable aggregation info to short name. */
                names =  build_extended_svar_name( names, p_sv, & namesize );

                /* Save the longest extended short name length. */
                if ( (tmp_len = strlen( names )) > max_len )
                    max_len = tmp_len;

                /* Append long name for storage in StringArray. */
                sprintf( names + strlen( names ), "|%s", p_sv->long_name );

                /* Insert the string into the StringArray. */
#ifdef NEWMILI
                rval = sa_add( &sa, names, &arrsize );
#else
                rval = SAADD( sa, names );
#endif

                if ( rval < 0 )
                    /* String insertion failed - warn but continue. */
                    popup_dialog( WARNING_POPUP,
                                  "StringArray insertion failure\n(\"%s\")",
                                  names );
            }
        }
    }

    /* there is a new value for max_len so we need to check it against the size of current and
       realloc if necessary */
    if(max_len >= strlen(current))
    {
        more_current =  (char *) realloc(current, (max_len + 10)*sizeof(char));
        if(more_current == NULL  )
        {
            popup_dialog( WARNING_POPUP, "Out of memory in file tell.c, exiting\n");
            exit(1);
        }

        current = more_current;
    }
    /* Pointer to current mesh. */
    p_md = MESH_P( analy );

    /* Loop over superclasses to access classes in that order... */
    for ( i = 0; i < QTY_SCLASS; i++ )
    {
        if ( (c_qty = p_md->classes_by_sclass[i].qty) == 0 )
            continue;

        p_mo_classes = (MO_class_data **) p_md->classes_by_sclass[i].list;

        /* Loop over classes of current superclass... */
        for ( j = 0; j < c_qty; j++ )
        {
            rval = htable_search( p_ht, p_mo_classes[j]->short_name, FIND_ENTRY,
                                  &p_hte );
            if ( rval != OK )
                continue;

            p_cp = (Class_primals *) p_hte->data;

            /* Sort result names alphabetically. */
            SASORT( p_cp->sa, 1 );

            wrt_text( "    Class %s (%s):\n",
                      p_cp->p_class->short_name, p_cp->p_class->long_name );

            res_qty = SAQTY( p_cp->sa );

            /* Process the first result, saving reference to the short name. */
            strcpy( names, SASTRING( p_cp->sa, 0 ) );
            p_short = strtok( names, "|" );
            p_long = strtok( NULL, "|" );

            wrt_text( "        %-*s - %s\n", max_len, p_short, p_long );
            strcpy( current, p_short );

            for ( k = 1; k < res_qty; k++ )
            {
                strcpy( names, SASTRING( p_cp->sa, k ) );
                p_short = strtok( names, "|" );
                p_long = strtok( NULL, "|" );

                /* Skip over duplicate names. */
                if ( strcmp( current, p_short ) == 0 )
                    continue;

                wrt_text( "        %-*s - %s\n", max_len, p_short, p_long );

                strcpy( current, p_short );
            }

            /* Don't need the StringArray anymore. */
            free( p_cp->sa );
        }
    }

    if ( res_qty > 0 )
        wrt_text( "\n" );

    /* Don't need the hash table anymore. */
    htable_delete( p_ht, NULL, TRUE );
    if(current != NULL)
    {
        free(current);
    }

    if(names != NULL)
    {

        free(names);

    }
}


/*****************************************************************
 * TAG( build_extended_svar_name )
 *
 * Compose a descriptive string of a State_variable aggregation type.
 */
static char *
build_extended_svar_name( char *svdesc, State_variable *p_sv, int * namesize )
{
    int k, size = 0;
    char * tmp_names = svdesc;
    svdesc[0] = '\0';
    /* namesize is the amount of memory allocated for svdesc */
    switch ( p_sv->agg_type )
    {
    case SCALAR:
        if(strlen(p_sv->short_name) > *namesize)
        {
            *namesize = strlen(p_sv->short_name);
            tmp_names = (char *)  realloc(svdesc, *namesize*sizeof(char));
            if(tmp_names == NULL)
            {
                printf("Memory allocation error in file %s line %d\n", __FILE__, __LINE__);
                exit(1);
            }

            svdesc = tmp_names;
        }
        sprintf( svdesc, p_sv->short_name );
        break;
    case VECTOR:
        size = 2 + strlen(p_sv->short_name) + strlen(p_sv->components[0]);
        for( k = 1; k < p_sv->vec_size; k++)
        {
            size += strlen(p_sv->components[k]);
        }
        size += 2; /* add room for the null terminator */
        if(size > *namesize)
        {
            *namesize = size*2;
            tmp_names =  (char *) realloc(svdesc, *namesize*sizeof(char));
            if(tmp_names == NULL)
            {
                printf("Memory allocation error in file %s line %d\n", __FILE__, __LINE__);
                exit(1);
            }

            svdesc = tmp_names;
        }

        sprintf( svdesc, "%s[%s", p_sv->short_name, p_sv->components[0] );
        for ( k = 1; k < p_sv->vec_size; k++ )
            strcat( svdesc, p_sv->components[k]);
        strcat( svdesc, "]" );
        break;
    case ARRAY:

        size = strlen(p_sv->short_name) + 1; /* add 1 to account for the null terminating character that
                                                    will be required at the end. */
        for( k = p_sv->rank - 1; k >= 0; k--)
        {
            size += num_digits(p_sv->dims[k]) + 2;
        }

        if(size > *namesize)
        {
            *namesize = size*2;
            tmp_names = (char *) realloc(svdesc, *namesize*sizeof(char));
            if(tmp_names == NULL)
            {
                printf("Memory allocation error in file %s line %d\n", __FILE__, __LINE__);
                exit(1);
            }
            svdesc = tmp_names;
        }

        sprintf( svdesc, "%s", p_sv->short_name );
        /*
         * Write indices in row-major order to match menu & command-line
         * order & result expression in rendering window.
         */
        for ( k = p_sv->rank - 1; k >= 0; k-- )
            sprintf( svdesc + strlen( svdesc ), "[%d]", p_sv->dims[k] );
        break;
    case VEC_ARRAY:
        size = strlen(p_sv->short_name) + 1;
        for( k = p_sv->rank - 1; k >= 0; k--)
        {
            size += num_digits(p_sv->dims[k]) + 2;
        }


        for ( k = 1; k < p_sv->vec_size; k++ )
        {
            size =+  num_digits(strlen(p_sv->components[k])) ;

        }

        if(size > *namesize)
        {
            *namesize = size*2;
            tmp_names = (char *) realloc(svdesc, *namesize*sizeof(char));
            if(tmp_names == NULL)
            {
                printf("Memory allocation error in file %s line %d\n", __FILE__, __LINE__);
                exit(1);
            }

            svdesc = tmp_names;
        }

        sprintf( svdesc, "%s", p_sv->short_name );
        /*
         * Write indices in row-major order to match menu & command-line
         * order & result expression in rendering window.
         */
        for ( k = p_sv->rank - 1; k >= 0; k-- )
            sprintf( svdesc + strlen( svdesc ), "[%d]", p_sv->dims[k] );
        sprintf( svdesc + strlen( svdesc ), "[%s", p_sv->components[0] );
        for ( k = 1; k < p_sv->vec_size; k++ )
            /*sprintf( svdesc + strlen( svdesc ), " %s",
                     p_sv->components[k] ); */
            strcat(svdesc, p_sv->components[k]);

        strcat( svdesc, "]" );
        break;
    }

    return tmp_names;
}


/*****************************************************************
 * TAG( tell_mesh_classes )
 *
 * Write a summary of classes in the current mesh.
 */
static void
tell_mesh_classes( Analysis *analy )
{
    MO_class_data **mo_classes;
    int i, j;
    int qty_classes;
    char *sclass_types[] =
    {
        "object(s)", "node(s)", "element(s)", "element(s)", "element(s)",
        "element(s)", "element(s)", "element(s)", "element(s)", "element(s)",
        "material(s)", "mesh(es)", "surface(s)", "particle(s)"

    };

    wrt_text( "Classes in current mesh:\n" );
    for ( i = 0; i < QTY_SCLASS; i++ )
    {
        qty_classes = MESH_P( analy )->classes_by_sclass[i].qty;
        mo_classes = (MO_class_data **)
                     MESH_P( analy )->classes_by_sclass[i].list;

        for ( j = 0; j < qty_classes; j++ )
        {
            wrt_text( "    %s (%s) - %d %s\n", mo_classes[j]->long_name,
                      mo_classes[j]->short_name, mo_classes[j]->qty,
                      sclass_types[ mo_classes[j]->superclass ] );
        }
    }
    wrt_text( "\n" );
}


/*****************************************************************
 * TAG( parse_tell_mm_command )
 *
 * Parse a "tell mm" command and generate a text report.
 */
static void
parse_tell_mm_command( Analysis *analy, char tokens[][TOKENLENGTH],
                       int token_cnt, int *p_addl_tokens,
                       Redraw_mode_type *p_redraw )
{
    char *tellmm_usage =
        "tell mm [<result> [<first state> [<last state>]]]";
    int start_state, stop_state, min_state, max_state;
    Bool_type rval;
    int mm_token_cnt;
    char result_variable[1];
    int ival;

    /* Determine if optional additional mm tokens are present. */
    mm_token_cnt = 1;
    if ( token_cnt > 1 )
    {
        /* First additional would be a result. */
        if ( search_result_tables( analy, ALL, tokens[1],
                                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   NULL, NULL ) )
        {
            mm_token_cnt++;

            /* If we have a result, we could have first/last states also. */
            if ( token_cnt > 2 && is_numeric_token( tokens[2] ) )
            {
                mm_token_cnt++;

                if ( token_cnt > 3 && is_numeric_token( tokens[3] ) )
                    mm_token_cnt++;
            }
        }
    }

    start_state        = GET_MIN_STATE( analy ) + 1;
    stop_state         = get_max_state( analy ) + 1;

    if ( mm_token_cnt == 1 )
    {
        /* tellmm - no optional parameters */
        result_variable[0] = '\0';
        rval = tellmm( analy, result_variable, start_state, stop_state,
                       p_redraw );
    }
    else if ( mm_token_cnt == 2 )
    {
        /* tellmm <valid_result> */
        analy->result_mod = TRUE;
        rval = tellmm( analy, tokens[1], start_state, stop_state,
                       p_redraw );
    }
    else if ( mm_token_cnt == 3 )
    {
        ival = (int) strtol( tokens[2], (char **)NULL, 10);

        if ( ival >= start_state )
        {
            /* tellmm <valid_result> <state_number> */
            analy->result_mod = TRUE;
            rval = tellmm( analy, tokens[1], ival, ival, p_redraw );
        }
        else
        {
            /* tellmm <valid_result> <invalid state number> */
            popup_dialog( USAGE_POPUP, tellmm_usage );
        }
    }
    else /* mm_token_cnt must be 4 */
    {

        min_state = (int) strtol( tokens[2], (char **)NULL, 10 );
        max_state = (int) strtol( tokens[3], (char **)NULL, 10 );

        if ( min_state >= start_state
                && max_state <= stop_state )
        {
            /* tellmm result state_number state_number */
            analy->result_mod = TRUE;
            rval = tellmm( analy, tokens[1], min_state, max_state,
                           p_redraw );
        }
        else
        {
            /* tellmm <valid_result> <invalid start or stop state> */
            popup_dialog( USAGE_POPUP, tellmm_usage );
        }
    }

    if ( !rval )
        popup_dialog( USAGE_POPUP, tellmm_usage );

    *p_addl_tokens = mm_token_cnt - 1;
}

/*****************************************************************
 * TAG( tell_element_coords )
 *
 * Write the node idents and their coordinates for the nodes
 * referenced by an element.
 */
static void
tell_element_coords( int el_ident, int el_idx, MO_class_data *p_mo_class, 
                     State2 *state_p, int dimension, Analysis *analy )
{
    int i;
    int node_idx, node_id;
    char *blanks = "                ";
    char el_buf[32];
    int width, el_buf_width, conn_qty;
    int *el_conns;
    GVec3D *nodes3d;
    GVec2D *nodes2d;
    GVec3D2P *nodes3d2p;
    GVec2D2P *nodes2d2p;
    int frac_size = 6;
    float *activity;
    State_rec_obj *p_sro;

    if ( (int)analy->float_frac_size>frac_size )
        frac_size = (int) analy->float_frac_size;

    conn_qty = qty_connects[p_mo_class->superclass];
    el_conns = p_mo_class->objects.elems->nodes + el_idx * conn_qty;
    activity = ( state_p->sand_present )
               ? state_p->elem_class_sand[p_mo_class->elem_class_index]
               : NULL;

    sprintf( el_buf, "%s %d", p_mo_class->long_name, el_ident );

    /* Set up to align output cleanly. */
    width = max_id_string_width( el_conns, conn_qty );
    el_buf_width = strlen( el_buf );

    /* Assign appropriate pointer to nodal coordinates. */
    if (MESH_P( analy )->double_precision_nodpos)
    {
        p_sro = analy->srec_tree + analy->state_p->srec_id;
        load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                     analy->cur_state + 1, FALSE,
                     (void *)  analy->tmp_result[0] );
        if ( dimension == 3 )
            nodes3d2p = ( GVec3D2P *) analy->tmp_result[0];
        else
            nodes2d2p = ( GVec2D2P *) analy->tmp_result[0];
    }
    else
    {
        if ( dimension == 3 )
            nodes3d = state_p->nodes.nodes3d;
        else
            nodes2d = state_p->nodes.nodes2d;
    }

    /* For each node referenced by element... */
    for ( i = 0; i < conn_qty; i++ )
    {
        /* Get node ident and its coordinates. */
        node_idx = el_conns[i];
        node_id = analy->mesh_table->node_geom->labels[node_idx].label_num;

        if ( dimension == 3 )
        {
            if ( i == 0 )
            {
                if ( activity && activity[el_idx] == 0.0 )
                    wrt_text( "\n%s is inactive.\n", el_buf );
                if (MESH_P( analy )->double_precision_nodpos)
                    wrt_text( "%*.s  node %*d  x: %.*e  y: %.*e  z: %.*e\n", /* Format changed from e9.6 */
                              el_buf_width, el_buf, width, node_id,
                              frac_size, nodes3d2p[node_idx][0],
                              frac_size, nodes3d2p[node_idx][1],
                              frac_size, nodes3d2p[node_idx][2] );
                else
                    wrt_text( "%*.s  node %*d  x: %.*e  y: %.*e  z: %.*e\n", /* Format changed from e9.6 */
                              el_buf_width, el_buf, width, node_id,
                              frac_size, nodes3d[node_idx][0],
                              frac_size, nodes3d[node_idx][1],
                              frac_size, nodes3d[node_idx][2] );
            }
            else if (MESH_P( analy )->double_precision_nodpos)
                wrt_text( "%.*s  node %*d  x: %.*e  y: %.*e  z: %.*e\n",
                          el_buf_width, blanks, width, node_id,
                          frac_size, nodes3d2p[node_idx][0],
                          frac_size, nodes3d2p[node_idx][1],
                          frac_size, nodes3d2p[node_idx][2] );
            else
                wrt_text( "%.*s  node %*d  x: %.*e  y: %.*e  z: %.*e\n",
                          el_buf_width, blanks, width, node_id,
                          frac_size, nodes3d[node_idx][0],
                          frac_size, nodes3d[node_idx][1],
                          frac_size, nodes3d[node_idx][2] );
        }
        else
        {
            if ( i == 0 )
            {
                if ( activity && activity[node_idx] == 0.0 )
                    wrt_text( "\n%s is inactive.\n", el_buf );

                if (MESH_P( analy )->double_precision_nodpos)
                    wrt_text( "%s  node %*d  x: %.*e  y: %.*e\n",
                              el_buf, width, node_id,
                              frac_size, nodes2d2p[node_idx][0],
                              frac_size, nodes2d2p[node_idx][1] );
                else
                    wrt_text( "%s  node %*d  x: %.*e  y: %.*e\n",
                              el_buf, width, node_id,
                              frac_size, nodes2d[node_idx][0],
                              frac_size, nodes2d[node_idx][1] );
            }
            else if (MESH_P( analy )->double_precision_nodpos)
                wrt_text( "%.*s  node %*d  x: %.*e  y: %.*e\n",
                          el_buf_width, blanks, width, node_id,
                          frac_size, nodes2d2p[node_idx][0],
                          frac_size, nodes2d2p[node_idx][1] );
            else
                wrt_text( "%.*s  node %*d  x: %.*e  y: %.*e\n",
                          el_buf_width, blanks, width, node_id,
                          frac_size, nodes2d[node_idx][0],
                          frac_size, nodes2d[node_idx][1] );
        }
    }
    return;
}



/************************************************************
 * TAG( tell_coordinates )
 *
 * Display nodal coordinates of:  a.) specified node id, or
 *                                b.) nodal components of
 *                                    specified element type id
 */
static void
tell_coordinates( char class[], int id, Analysis *analy )
{
    Htable_entry *p_hte;
    int rval;
    MO_class_data *p_mo_class;
    GVec3D *nodes3d;
    GVec2D *nodes2d;
    GVec3D2P *nodes3d2p;
    GVec2D2P *nodes2d2p;
    int frac_size = 6;
    State_rec_obj *p_sro;
    int label_index = -1;

    if (MESH_P( analy )->double_precision_nodpos)
    {
        p_sro = analy->srec_tree + analy->state_p->srec_id;
        load_nodpos( analy, p_sro, MESH_P( analy ), analy->dimension,
                     analy->cur_state + 1, FALSE,
                     (void *)  analy->tmp_result[0] );
        if ( analy->dimension == 3 )
            nodes3d2p = ( GVec3D2P *) analy->tmp_result[0];
        else
            nodes2d2p = ( GVec2D2P *) analy->tmp_result[0];
    }
    else
    {
        if ( analy->dimension == 3 )
            nodes3d = analy->state_p->nodes.nodes3d;
        else
            nodes2d = analy->state_p->nodes.nodes2d;
    }

    if ( (int) analy->float_frac_size>frac_size )
        frac_size = (int) analy->float_frac_size;

    rval = htable_search( MESH( analy ).class_table, class, FIND_ENTRY,
                          &p_hte );
    if ( rval != OK )
    {
        popup_dialog( USAGE_POPUP, "tell pos <class name> <ident>" );
        return;
    }

    p_mo_class = (MO_class_data *) p_hte->data;
    label_index = get_class_label_index(p_mo_class,id);
    
    if ( label_index < 0 || label_index > p_mo_class->qty )
    {
        popup_dialog( INFO_POPUP, "Invalid %s number: %d\n", class, id );
        return;
    }

    if ( p_mo_class->superclass == G_NODE )
    {
        if ( analy->dimension == 3 )
        {
            if (MESH_P( analy )->double_precision_nodpos)
                wrt_text( "%s %d  x: %.*e  y: %.*e  z: %.*e\n",
                          p_mo_class->long_name, id,
                          frac_size, nodes3d2p[label_index][0],
                          frac_size, nodes3d2p[label_index][1],
                          frac_size, nodes3d2p[label_index][2] );
            else
                wrt_text( "%s %d  x: %.*e  y: %.*e  z: %.*e\n",
                          p_mo_class->long_name, id,
                          frac_size, nodes3d[label_index][0],
                          frac_size, nodes3d[label_index][1],
                          frac_size, nodes3d[label_index][2] );

        }
        else
        {
            if (MESH_P( analy )->double_precision_nodpos)
                wrt_text( "%s %d  x: %.*e  y: %.*e\n",
                          p_mo_class->long_name, id,
                          frac_size, nodes2d2p[label_index][0],
                          frac_size, nodes2d2p[label_index][1] );
            else
                wrt_text( "%s %d  x: %.*e  y: %.*e\n",
                          p_mo_class->long_name, id,
                          frac_size, nodes2d[label_index][0],
                          frac_size, nodes2d[label_index][1] );
        }

        wrt_text( "\n" );
    }
    else if ( IS_ELEMENT_SCLASS( p_mo_class->superclass ) )
    {
        tell_element_coords( id, label_index, p_mo_class, analy->state_p, 
                             analy->dimension, analy );

        wrt_text( "\n" );
    }
    else
        popup_dialog( INFO_POPUP, "%s\n%s%s%s",
                      "Griz cannot perform a position query on",
                      "class \"", p_mo_class->short_name, "\" objects." );

    return;
}



/*****************************************************************
 * TAG( max_id_string_width )
 *
 * Get the maximum width among string representations of node
 * identifiers referenced by an element.
 */
static int
max_id_string_width( int *connects, int node_cnt )
{
    int i, width, temp, val;

    width = 0;
    for ( i = 0; i < node_cnt; i++ )
    {
        val = ( connects[i] > 0 ) ? connects[i] : 1;
        temp = (int) (log10( (double) val ) + 1.0);

        if ( temp > width )
            width = temp;
    }

    return width;
}


/*************************************************************
 * TAG(num_digits)
 * return the number of digits in an integer including the
 * minus sign if negative.
 *
 * Added by Bill Oliver
 * July 12, 2013
 *
 */
static int num_digits(int x)
{
    int y;

    if(x >= 0)
    {
        return floor(log10(abs(x))) + 1;
    }
    else
    {
        return floor(log10(abs(x))) + 2;
    }
}
