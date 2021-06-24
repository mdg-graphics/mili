/* $Id$ */
/*
 * time_hist.c - Routines for drawing time-history plots.
 *
 *      Donald J. Dovey
 *      Lawrence Livermore National Laboratory
 *      Oct 28 1992
 *
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - Apr 04, 2005: Fixed ifdef for O2000 - should be ifndef.
 *
 *  I. R. Corey - May 23, 2005: Added logic in build_result_list to
 *                recompute result if flag 'must_recompute' is set.
 *                See SRC#: 316
 *
 *  I. R. Corey - Oct 25, 2007: For outth make cur result the default.
 *                See SRC#499.
 *
 *  I. R. Corey - Nov 08, 2007:	Add Node/Element labeling.
 *                See SRC#418 (Mili) and 504 (Griz)
 *
 *  I. R. Corey - Mar 31, 2008:	Add Node/Element labeling.
 *                See SRC#418 (Mili) and 504 (Griz)
 *
 *  I. R. Corey - Jan 13, 2009:	Fixed a problem with testing for labels
 *                for a node class where structure (mo_class) does not
 *                exist.
 *                 See SRC#562
 *
 *  I. R. Corey - Nov 02, 2009:	Fixed a problem with multiple Griz sessions
 *                hanging on Windows with Hummingbird Exceed X11 software
 *                due to a glBegin( GL_LINE_LOOP ) that does not have a
 *                glEnd();
 *                See SRC#636
 ************************************************************************
 *
 */

#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include "viewer.h"
#include "draw.h"

#ifndef O2000x
#include "hershey.h"
#endif


#define NON_IDENT -1

#define GET_SUBREC_ID( r, m, k ) \
    ( r->origin.is_primal ) \
        ? ((int *) (m)->list)[k] \
        : ((Subrecord_result *) (m)->list)[k].subrec_id

#define ROUND_UP_INT( n, r ) ( ((n)%(r)) ? (n) + ((r) - (n)%(r)) : (n) )

#define GLYPH_SCALE_FUDGE 0.35

#define INVALID_STATE -1


static Bool_type refresh = FALSE;
static char current_tokens[MAXTOKENS][TOKENLENGTH];
static int current_token_qty;

typedef char Title_string[256];
int vslogic;
static GLfloat default_plot_colors[20][3] =
{
    {0.157, 0.157, 1.0},    /* Blue */
    {1.0, 0.157, 0.157},    /* Red */
    {1.0, 0.157, 1.0},      /* Magenta */
    {0.2, 0.2, 0.2},
    {0.157, 1.0, 0.157},    /* Green */

    {1.0, 0.647, 0.0},      /* Orange */
    {0.45, 0.22, 0.06},     /* Brown */
    {0.118, 0.565, 1.0},    /* Dodger blue */
    {1.0, 0.412, 0.706},    /* Hot pink */
    {0.573, 0.545, 0.341},  /* SeaGreen */

    {0.0, 0.9, 0.0},
    {0.0, 0.3, 0.7},
    {0.7, 0.0, 0.6},
    {0.0, 0.6, 0.0},
    {0.294, 0.0, 0.51},

    {0.7, 0.5, 0.0},
    {0.06, 0.06, 0.4},
    {0.8, 0.05, 0.05},
    {0.4, 0.8, 0.5},
    {0.157, 1.0, 1.0}
};

static GLfloat plot_colors[20][3];


typedef struct _series_evt_obj
{
    struct _series_evt_obj *prev;
    struct _series_evt_obj *next;
    Time_series_obj *p_series;
    Bool_type start_gather;
    int state;
} Series_evt_obj;


typedef struct _MOR_entry
{
    Time_series_obj *p_series;
} MOR_entry;


typedef struct _gather_segment
{
    struct _gather_segment *prev;
    struct _gather_segment *next;
    int start_state;
    int end_state;
    int entries_used;
    int qty_entries;
    MOR_entry *entries;
} Gather_segment;


/* Plot glyph identifiers. */
/* IF THIS ORDER IS CHANGED, likewise modify array glyph_geom, below. */
typedef enum
{
    GLYPH_SQUARE = 0,
    GLYPH_TRIANGLE,
    GLYPH_PENTAGON,
    GLYPH_DIAMOND,
    GLYPH_CROSS,
    GLYPH_HEXAGON,
    GLYPH_SPLAT,
    GLYPH_STAR,
    GLYPH_X,
    GLYPH_PLUS,
    GLYPH_CIRCLE,
    GLYPH_MOON,
    GLYPH_BULLET,
    GLYPH_THING,
    GLYPH_BOW,
    QTY_GLYPH_TYPES /* Always last */
} Plot_glyph_type;

static int glyphs_per_plot = 5;
static float glyph_scale = 1.0;

/* Storage for world-coordinate translated time history glyph vertices. */
static GVec3D circle_verts[16];
static GVec3D triangle_verts[3];
static GVec3D square_verts[4];
static GVec3D pentagon_verts[5];
static GVec3D diamond_verts[4];
static GVec3D cross_verts[12];
static GVec3D hexagon_verts[6];
static GVec3D splat_verts[8];
static GVec3D star_verts[10];
static GVec3D x_verts[4];
static GVec3D moon_verts[30];
static GVec3D plus_verts[4];
static GVec3D bullet_verts[21];
static GVec3D thing_verts[16];
static GVec3D bow_verts[4];

static struct
{
    GVec3D *verts;
    int qty_verts;
} glyph_geom[] =
{
    { square_verts, sizeof( square_verts ) / sizeof( square_verts[0] ) },
    { triangle_verts, sizeof( triangle_verts ) / sizeof( triangle_verts[0] ) },
    { pentagon_verts, sizeof( pentagon_verts ) / sizeof( pentagon_verts[0] ) },
    { diamond_verts, sizeof( diamond_verts ) / sizeof( diamond_verts[0] ) },
    { cross_verts, sizeof( cross_verts ) / sizeof( cross_verts[0] ) },
    { hexagon_verts, sizeof( hexagon_verts ) / sizeof( hexagon_verts[0] ) },
    { splat_verts, sizeof( splat_verts ) / sizeof( splat_verts[0] ) },
    { star_verts, sizeof( star_verts ) / sizeof( star_verts[0] ) },
    { x_verts, sizeof( x_verts ) / sizeof( x_verts[0] ) },
    { plus_verts, sizeof( plus_verts ) / sizeof( plus_verts[0] ) },
    { circle_verts, sizeof( circle_verts ) / sizeof( circle_verts[0] ) },
    { moon_verts, sizeof( moon_verts ) / sizeof( moon_verts[0] ) },
    { bullet_verts, sizeof( bullet_verts ) / sizeof( bullet_verts[0] ) },
    { thing_verts, sizeof( thing_verts ) / sizeof( thing_verts[0] ) },
    { bow_verts, sizeof( bow_verts ) / sizeof( bow_verts[0] ) }
};


typedef struct _plot_glyph_data
{
    Plot_glyph_type glyph_type;
    float next_threshold;
    float interval;
} Plot_glyph_data;


typedef Bool_type (*Glyph_eval_func)( float, Plot_glyph_data * );

typedef void (*Init_glyph_eval_func)( int, int, float, Analysis *,
                                      Plot_glyph_data * );


#ifdef OLDSMOOTH
static void smooth_th_data();
#endif
static void output_interleaved_series( FILE *, Plot_obj *, Analysis * );
static void output_one_series( int, Plot_obj *, FILE *, Analysis * );
static Time_series_obj *new_time_tso( Analysis * );
static Time_series_obj *new_time_series_obj( Result *, int, MO_class_data *,
        Time_series_obj *,
        Time_series_obj *, int );
static void update_eval_states( Time_series_obj *, Analysis * );
static void check_for_global( Result *, Specified_obj **, Analysis * );
static void build_result_list( int, char [][TOKENLENGTH], Analysis *, int *,
                               Result ** );
static void build_object_list( int, char [][TOKENLENGTH], Analysis *, int *,
                               Specified_obj ** );
static void build_oper_result_list( int, char [][TOKENLENGTH], Analysis *,
                                    int *, Bool_type, Result ** );
static void build_oper_object_list( int, char [][TOKENLENGTH], Analysis *,
                                    int *, Bool_type, Specified_obj ** );
static void parse_abscissa_spec( int, char [][TOKENLENGTH], Analysis *, int *,
                                 Result ** );
static void clear_gather_resources( Gather_segment **, Analysis * );
static void gather_time_series( Gather_segment *, Analysis * );
static int get_result_superclass( int, Result * );
static void remove_unused_time_series( Time_series_obj **, Time_series_obj *,
                                       Analysis * );
static void create_oper_time_series( Analysis *, Result *, Specified_obj *,
                                     Result *, Specified_obj *,
                                     Plot_operation_type, Time_series_obj ** );
static void prepare_plot_objects( Result *, Specified_obj *, Analysis *,
                                  Plot_obj ** );
static Time_series_obj * new_oper_time_series( Time_series_obj *,
        Time_series_obj *,
        Plot_operation_type );
static void set_oper_plot_objects( Time_series_obj *, Analysis *, Plot_obj ** );
static void compute_operation_time_series( Plot_obj * );
static Bool_type extract_numeric_str( char *, char * );
static void gen_srec_fmt_blocks( int, int *, int, int, List_head * );
static void add_valid_states( Time_series_obj *, List_head *, int, int, int );
static void add_series_map_entry( int, int, Time_series_obj * );
static void update_srec_fmt_blocks( int, int, Analysis * );
static int gen_gather( Result *, Specified_obj *, Analysis *,
                       Time_series_obj ** );
static void gen_control_list( Time_series_obj *, Analysis *,
                              Gather_segment ** );
static void copy_gather_segment( Gather_segment *, Gather_segment * );
static void pop_gather_lazy( Series_evt_obj *, Gather_segment * );
static void push_gather( Series_evt_obj *, Gather_segment * );
static void add_gather_tree_entry( Subrec_obj *, Time_series_obj * );
static void add_event( Bool_type, int, Time_series_obj *, Series_evt_obj ** );
static void prep_gather_tree( Gather_segment *, Analysis * );
static Bool_type find_global_time_series( Result *, Analysis *,
        Time_series_obj ** );
static Bool_type find_time_series( Result *, int, MO_class_data *, Analysis *,
                                   Time_series_obj *, Time_series_obj ** );
static Bool_type find_result_in_list( Result *, Result *, Result ** );
static Bool_type find_named_result_in_list( Analysis *, char *, Result *,
        Result ** );
static Bool_type match_series_results( Result *, Analysis *,
                                       Time_series_obj * );
static Bool_type match_result_source_with_analy( Analysis *, Result * );
static Bool_type match_spec_with_analy( Analysis *, Result_spec * );
static void clear_plot_list( Plot_obj ** );
static Specified_obj *copy_obj_list( Specified_obj * );
static void add_mo_nodes( Specified_obj **, MO_class_data *, int, int );
static MO_list_token_type get_token_type( char *, Analysis *,
        MO_class_data ** );
static void init_glyph_draw( float, float, float );
static void draw_glyph( float[], Plot_glyph_data * );
static void get_interval_min_max( float *, int, int, int, float *, float * );
static void get_bounded_series_min_max( Time_series_obj *, int, int, float *,
                                        float * );
static void gen_blocks_intersection( int, Int_2tuple *, int, Int_2tuple *,
                                     int *, Int_2tuple ** );
static void init_aligned_glyph_func( int, int, float, Analysis *,
                                     Plot_glyph_data * );
static void init_staggered_glyph_func( int, int, float, Analysis *,
                                       Plot_glyph_data * );
static Bool_type thresholded_glyph_func( float, Plot_glyph_data * );
static void add_legend_text( Time_series_obj *, char **, int, int *, int *,
                             Bool_type [] );


/* Defaults for glyph distribution function. */
static Init_glyph_eval_func init_glyph_eval = init_staggered_glyph_func;
static Glyph_eval_func need_glyph_now;


#ifdef THTEST
static void
dump_gather_segment( Gather_segment * );
#endif

static Bool_type th_series_print_header = TRUE;

/*****************************************************************
 * TAG( tell_time_series )
 *
 * Write summary of currently gathered time series to feedback
 * window.
 */
void
tell_time_series( Analysis *analy )
{
    int i;
    Time_series_obj *p_tso;
    char label[256];

    i = 0;

    wrt_text( "Gathered time series':\n" );
    if ( analy->time_series_list == NULL
            && analy->oper_time_series_list == NULL )
        wrt_text( "    (none)\n" );
    else
    {
        for ( p_tso = analy->time_series_list; p_tso != NULL; NEXT( p_tso ) )
        {
            i++;
            build_result_label( p_tso->result,
                                ( p_tso->mo_class->superclass == G_QUAD ||
                                  p_tso->mo_class->superclass == G_HEX  ),
                                label );
            wrt_text( "    %d. %s %d %s\n", i, p_tso->mo_class->long_name,
                      p_tso->ident + 1, label );
        }

        for ( p_tso = analy->oper_time_series_list; p_tso != NULL;
                NEXT( p_tso ) )
        {
            i++;
            build_oper_series_label( p_tso, FALSE, label );
            wrt_text( "    %d. %s\n", i, label );
        }
    }

    wrt_text( "\n" );
}


/*****************************************************************
 * TAG( delete_time_series )
 *
 * Delete specified time series', freeing their resources.
 *
 * Time series' can be specified for deletion through any
 * combination of :
 *   - Numeric indices into the current list of Time_series_obj's
 *     as indicated in the output of tell_time_series();
 *   - Result names, for which all time series of that result
 *     will be deleted (regardless of mesh object);
 *   - Class short names, for which all time series for objects
 *     in that class will be deleted (regardless of result).
 */
void
delete_time_series( int token_qty, char tokens[][TOKENLENGTH], Analysis *analy )
{
    int i, j, qty, oper_qty, idx;
    Time_series_obj *p_tso, *p_del_tso;
    Plot_obj *p_po, *p_del_po;
    Bool_type *del_flags, *oper_del_flags;

    /* Sanity check. */
    if ( analy->time_series_list == NULL )
        return;

    /* Count the entries and allocate an array of delete flags. */
    qty = 0;
    for ( p_tso = analy->time_series_list; p_tso != NULL; NEXT( p_tso ) )
        qty++;

    del_flags = NEW_N( Bool_type, qty, "Time series delete flags array" );

    /* Count the operation plots and allocate an array of delete flags. */
    oper_qty = 0;
    for ( p_tso = analy->oper_time_series_list; p_tso != NULL; NEXT( p_tso ) )
        oper_qty++;

    oper_del_flags = NEW_N( Bool_type, oper_qty,
                            "Operation time series delete flags array" );

    /* Parse command line to schedule time series for deletion. */
    for ( i = 1; i < token_qty; i++ )
    {
        if ( strcmp( tokens[i], "all" ) == 0 )
        {
            for ( j = 0; j < qty; j++ )
                del_flags[j] = TRUE;
            for ( j = 0; j < oper_qty; j++ )
                oper_del_flags[j] = TRUE;
            break;
        }
        else if ( is_numeric_token( tokens[i] ) )
        {
            idx = atoi( tokens[i] ) - 1;
            if ( idx < qty && idx >= 0 )
                del_flags[idx] = TRUE;
            else if ( idx < oper_qty + qty && idx >= qty )
                oper_del_flags[idx - qty] = TRUE;
            else
                popup_dialog( INFO_POPUP,
                              "Out-of-bounds time series index \"%s\" ignored.",
                              tokens[i] );
        }
        else
        {
            /*
             * Search series to match token as result name or mesh object
             * class short name.
             */
            j = 0;
            for ( p_tso = analy->time_series_list; p_tso != NULL;
                    NEXT( p_tso ) )
            {
                if ( strcmp( tokens[i], p_tso->result->name ) == 0
                        || strcmp( tokens[i], p_tso->mo_class->short_name ) == 0 )
                {
                    del_flags[j] = TRUE;
                }
                j++;
            }

            j = 0;
            for ( p_tso = analy->oper_time_series_list; p_tso != NULL;
                    NEXT( p_tso ) )
            {
                if ( strcmp( tokens[i], p_tso->operand1->result->name ) == 0
                        || strcmp( tokens[i],
                                   p_tso->operand1->mo_class->short_name ) == 0
                        || strcmp( tokens[i], p_tso->operand2->result->name ) == 0
                        || strcmp( tokens[i],
                                   p_tso->operand2->mo_class->short_name ) == 0 )
                {
                    oper_del_flags[j] = TRUE;
                }
                j++;
            }
        }
    }

    /*
     * Delete operation Time_series_obj's _first_ so we can avoid
     * decrementing the reference count for any regular time series that
     * have already been deleted.  Also delete the referencing Plot_obj's.
     */
    p_tso = analy->oper_time_series_list;
    for ( i = 0; i < oper_qty; i++ )
    {
        p_del_tso = p_tso;
        NEXT( p_tso );

        if ( oper_del_flags[i] )
        {
            UNLINK( p_del_tso, analy->oper_time_series_list );

            if ( p_del_tso->reference_count > 0 )
            {
                /* Find/delete referencing Plot_obj's. */
                p_po = analy->current_plots;
                while ( p_po != NULL )
                {
                    p_del_po = p_po;
                    NEXT( p_po );

                    if ( p_del_po->ordinate == p_del_tso
                            || p_del_po->abscissa == p_del_tso )
                    {
                        p_del_po->ordinate->reference_count--;
                        p_del_po->abscissa->reference_count--;

                        DELETE( p_del_po, analy->current_plots );
                    }
                }
            }

            destroy_time_series( &p_del_tso );
        }
    }

    free( oper_del_flags );

    /* Delete specified Time_series_obj's and any referencing Plot_obj's. */
    p_tso = analy->time_series_list;
    for ( i = 0; i < qty; i++ )
    {
        p_del_tso = p_tso;
        NEXT( p_tso );

        if ( del_flags[i] )
        {
            UNLINK( p_del_tso, analy->time_series_list );

            if ( p_del_tso->reference_count > 0 )
            {
                /* Find/delete referencing Plot_obj's. */
                p_po = analy->current_plots;
                while ( p_po != NULL )
                {
                    p_del_po = p_po;
                    NEXT( p_po );

                    if ( p_del_po->ordinate == p_del_tso
                            || p_del_po->abscissa == p_del_tso )
                    {
                        p_del_po->ordinate->reference_count--;
                        p_del_po->abscissa->reference_count--;

                        DELETE( p_del_po, analy->current_plots );
                    }
                }
            }

            destroy_time_series( &p_del_tso );
        }
    }

    free( del_flags );

    /* Now delete any un-referenced Result's. */
    remove_unused_results( &analy->series_results );
    remove_unused_results( &analy->abscissa );
}


/*****************************************************************
 * TAG( parse_gather_command )
 *
 * Gather time series' into memory.
 */
void
parse_gather_command( int token_qty, char tokens[MAXTOKENS][TOKENLENGTH],
                      Analysis *analy )
{
    Plot_obj *dummy_plots;
    Plot_operation_type oper_type;

    dummy_plots = NULL;

    /* Preclude saving the plot specification. */
    refresh = TRUE;

    if ( is_operation_token( tokens[1], &oper_type ) )
        create_oper_plot_objects( token_qty, tokens, analy, &dummy_plots );
    else
        create_plot_objects( token_qty, tokens, analy, &dummy_plots );

    refresh = FALSE;

    /* Don't need the plot objects. */
    if ( dummy_plots != NULL )
        clear_plot_list( &dummy_plots );

    return;

}


/*****************************************************************
 * TAG( output_time_series )
 *
 * Write time series data to an ASCII file.
 *
 * If curves are plotted versus time, then data is written out in
 * N + 1 columns, where N is the number of curves and time is in
 * the first column.  Otherwise the curves are cross-plots and the
 * data for each curve is put out sequentially, i.e., all states for
 * the first curve (one state per line), all states for the second
 * curve, etc.  Each curve's data consists of one column for the
 * abscissa and one column for the ordinate.
 */
void
output_time_series( int token_qty, char tokens[MAXTOKENS][TOKENLENGTH],
                    Analysis *analy )
{
    FILE *outfile;
    int cnt;
    Plot_obj *p_po, *output_plots;
    Bool_type created_for_output;
    char *version_info_ptr;

    created_for_output = FALSE;

    if ( token_qty == 3 && strcmp( tokens[2], "cur" ) == 0 )
    {
        /* Output the current set of plots. */
        if ( analy->current_plots == NULL )
        {
            popup_dialog( INFO_POPUP, "No current plots to output." );
            return;
        }
        else
            output_plots = analy->current_plots;
    }
    else if ( token_qty >= 2 )
    {
        /*
         * Apparently the command line specifies the histories to output,
         * so create the objects, offsetting by one into the tokens array
         * since create_plot_objects() expects the pertinent parameters
         * to start with the second token, not the third.
         */
        if ( token_qty==2 && analy->current_plots != NULL )
            output_plots = analy->current_plots;
        else
        {
            output_plots = NULL;
            refresh = TRUE; /* Preclude saving the plot specification. */
            create_plot_objects( token_qty - 1, tokens + 1, analy, &output_plots );
            refresh = FALSE;
            if ( output_plots == NULL )
            {
                popup_dialog( INFO_POPUP,
                              "No plots from specification to output." );
                return;
            }
            created_for_output = TRUE;
        }
    }

    if ( analy->th_append_output )
    {
        struct stat s;
        if ( stat( tokens[1], &s ) == 0 )
            th_series_print_header = FALSE;
        else
            th_series_print_header = TRUE;
        if ( ( outfile = fopen( tokens[1], "a") ) == NULL )
        {
            popup_dialog( INFO_POPUP, "Couldn't open file \"%s\".\n", tokens[1] );
            return;
        }
    }
    else
    {
        if ( ( outfile = fopen( tokens[1], "w") ) == NULL )
        {
            popup_dialog( INFO_POPUP, "Couldn't open file \"%s\".\n", tokens[1] );
            return;
        }
    }

    /* Print the header. */
    if ( analy->th_append_output && th_series_print_header )
    {
        fprintf( outfile, "# Time series data\n#\n" );
        fprintf( outfile, "#---------------------------------------------------------------------\n" );
        fprintf( outfile, "# Griz Version Info:\n" );
        /* Print out Griz version info header */
        version_info_ptr = get_VersionInfo( analy );

        fprintf( outfile, "%s#\n", version_info_ptr );
        if ( version_info_ptr )
            free( version_info_ptr );
        fprintf( outfile, "#---------------------------------------------------------------------\n" );

        fprintf( outfile, "# Database path/name: %s\n", analy->root_name );
        fprintf( outfile, "# Description: %s\n", analy->title );
        fprintf( outfile, "# Output date: %s\n", env.date );
        if ( analy->perform_unit_conversion )
            fprintf( outfile, "# Applied conversion scale/offset: %E/%E\n",
                     analy->conversion_scale, analy->conversion_offset );
        fprintf( outfile, "#\n" );
    }

    /* Output format depends on source of abscissa. */
    if ( output_plots->abscissa->mo_class == NULL && ! analy->th_single_col )
    {
        /* Time is common abscissa; dump curves' data adjacently. */
        output_interleaved_series( outfile, output_plots, analy );
    }
    else
    {
        /* Dump curves sequentially. */

        for ( p_po = output_plots, cnt = 1; p_po != NULL;
                NEXT( p_po ), cnt++ )
            output_one_series( cnt, p_po, outfile, analy );
    }

    fclose( outfile );

    if ( created_for_output )
    {
        /* Destroy Plot_obj's if they were only created for output. */
        clear_plot_list( &output_plots );

        /*
         * Could also destroy unused Time_series_obj's and Result's,
         * but for now leave them around for later use.
         */
    }
}


/*****************************************************************
 * TAG( update_plots )
 *
 * Update plot objects for current plot request.
 */
extern void
update_plots( Analysis *analy )
{
    refresh = TRUE;

    /* On refresh, use existing command. */
    if ( strcmp( current_tokens[0], "plot" ) == 0 )
        create_plot_objects( current_token_qty, current_tokens, analy,
                             &analy->current_plots );
    else if ( strcmp( current_tokens[0], "oplot" ) == 0 )
        create_oper_plot_objects( current_token_qty, current_tokens, analy,
                                  &analy->current_plots );
    else
        popup_dialog( WARNING_POPUP,
                      "Unable to parse saved plotting command; aborting." );

    refresh = FALSE;
}


/*****************************************************************
 * TAG( create_plot_objects )
 *
 * Parse the command line and prepare plot objects for plotting
 * or output.
 *
 * Format:
 *   plot [<result>...] [<mesh object>...] [vs <abscissa result>]
 *
 * Where <result> defaults to the current result, <mesh object>
 * defaults to the list of currently selected objects, and
 * <abscissa result> defaults to "time".
 */
extern void
create_plot_objects( int token_qty, char tokens[][TOKENLENGTH],
                     Analysis *analy, Plot_obj **p_plot_list )
{
    int i;
    Result *res_list;
    Specified_obj *p_so, *so_list;
    int idx;
    Time_series_obj *old_tsos, *gather_list, *abscissa_gather_list;
    Gather_segment *control_list;
    int good_time_series;

    /* If not refreshing, save current command for subsequent refreshes. */
    if ( !refresh )
    {
        /* Save new command. */
        for ( i = 0; i < token_qty; i++ )
            strcpy( current_tokens[i], tokens[i] );
        current_token_qty = token_qty;
    }

    res_list = NULL;

    /* Results are first among tokens - get results to be plotted. */
    build_result_list( token_qty, tokens, analy, &idx, &res_list );
    if ( res_list == NULL )
    {
        popup_dialog( INFO_POPUP, "No results are specified." );
        return;
    }

    /* Mesh objects are next - get mesh objects to provide plot results. */
    build_object_list( token_qty, tokens, analy, &idx, &so_list );


    /* Check for abscissa specification to finish parsing the command line. */
    parse_abscissa_spec( token_qty, tokens, analy, &idx, &res_list );

    /* Ensure there's a global object if the results need one. */
    check_for_global( res_list, &so_list, analy );

    if ( so_list == NULL )
    {
        remove_unused_results( &res_list );
        popup_dialog( INFO_POPUP,
                      "No objects are identified to plot results for." );
        return;
    }

    /* Prepare for new plot set. */
    clear_plot_list( p_plot_list );

    /* Generate necessary time series objects and subrecord gather trees. */
    good_time_series = 0;
    good_time_series = gen_gather( res_list, so_list, analy, &gather_list );

    if ( good_time_series == 0 )
    {
        remove_unused_results( &res_list );
        DELETE_LIST( so_list );
        popup_dialog( INFO_POPUP, "No valid result/mesh object combinations "
                      "found; aborting." );
        return;
    }
    
    if(analy->abscissa)
    {
        good_time_series = gen_gather( analy->abscissa, 
                                   so_list, analy, 
                                   &abscissa_gather_list );

        if ( good_time_series == 0 )
        {
            remove_unused_results( &res_list );
            DELETE_LIST( so_list );
            popup_dialog( INFO_POPUP, "No valid result/mesh object combinations "
                      "found; aborting." );
            return;
        }
        //for(old_tsos = abscissa_gather_list; old_tsos !=NULL;NEXT(old_tsos))
        //{
            APPEND(abscissa_gather_list, gather_list);
        //}
        old_tsos = NULL;
    }
    
    /* Generate state "segments" with constant gather lists. */
    gen_control_list( gather_list, analy, &control_list );

    /* Update "evaluated at" limits for each time series. */
    update_eval_states( gather_list, analy );

    /* Keep a reference to first of any extant time series'. */
    old_tsos = analy->time_series_list;

    /* Attach the old series list to the tail of the new list. */
    if ( old_tsos != NULL )
        APPEND( old_tsos, gather_list );

    /* Update time series list pointer. */
    analy->time_series_list = gather_list;

    if ( analy->abscissa == NULL)
    {
        /* Retrieve time array from db if necessary. */
        if ( analy->times == NULL )
            analy->times = new_time_tso( analy );
        else if ( analy->times->state_blocks[0][1] != analy->last_state )
        {
            destroy_time_series( &analy->times );
            analy->times = new_time_tso( analy );
        }
    }

    /* Create plot objects. */
    prepare_plot_objects( res_list, so_list, analy, p_plot_list );

    if(p_plot_list != NULL && *p_plot_list == NULL)
    {
       popup_dialog(USAGE_POPUP, "specifiying the same element class on both sides of \"vs\" is not supported.");
       return;
    }

    /* Don't need Specified_obj list anymore. */
    DELETE_LIST( so_list );

    /*
     * Clean-up any unreferenced time series' among the new ones.
     * This could happen if one of an abscissa/ordinate pair were
     * unavailable for a plot, in which case no Plot_obj would have been
     * created and the existing half of the pair would have a zero
     * reference count.
     */
    remove_unused_time_series( &analy->time_series_list, old_tsos, analy );

    /* Remove unused results then hang the remainder for long-term storage. */
    remove_unused_results( &res_list );
    if ( res_list != NULL )
        APPEND( res_list, analy->series_results );

    /* Gather the time series data. */
    gather_time_series( control_list, analy );

    /* Clean up. */
    clear_gather_resources( &control_list, analy );
}


/*****************************************************************
 * TAG( element_set_check )
 *
 * For plotting results we must have selected objects and when
 * the token count is 1 we also have results showing.  This function
 * is used by the plot command to return TRUE if any of the selected
 * objects are actually element sets containing multiple integration
 * points. 
 * 
 */
extern Bool_type element_set_check(Analysis * analy)
{
    int i, superclass;
    Result * p_result;
    Specified_obj *objpts;

    if((p_result = analy->cur_result) == NULL)
    {
        return FALSE;
    }

    if(analy->selected_objects == NULL)
    {
        return FALSE;
    }

    for(objpts = analy->selected_objects; objpts != NULL; objpts = objpts->next)
    {
        superclass = objpts->mo_class->superclass;

        /* for each object seleted see if it is an element set and if so return TRUE */
        for(i = 0; i < p_result->qty; i++)
        {
            /* we have found the selected superclassin the current result.  Now see if it is an element set */
            if(p_result->original_names != NULL && strstr(p_result->original_names[i], "es_") != NULL)
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}


/*****************************************************************
 * TAG( create_oper_plot_objects )
 *
 * Parse the command line and prepare plot objects for plotting
 * or output.
 *
 * Format:
 *   oplot { {-|diff} | {+|add} | {*|mult} | {/|div} }
 *         [
 *           [<result>] <class> <object ident>...
 *           [ [<operand 2 result>] <operand 2 class> <object ident>... ]
 *         ]
 *
 * Where <result> defaults to the current result, <mesh object>
 * defaults to the list of currently selected objects, and
 * <abscissa result> defaults to "time".
 */
extern void
create_oper_plot_objects( int token_qty, char tokens[][TOKENLENGTH],
                          Analysis *analy, Plot_obj **p_plot_list )
{
    Result *opnd1_res, *opnd2_res;
    Specified_obj *opnd1_so_list, *opnd2_so_list, *p_so, *p_so2;
    int idx, i;
    Time_series_obj *old_tsos, *opnd1_gather_list, *opnd2_gather_list;
    Time_series_obj *oper_time_series;
    Gather_segment *control_list;
    Plot_operation_type oper_type;
    int valid_opnd1_tsos, valid_opnd2_tsos;

    /* Prepare for new plot set. */
    clear_plot_list( p_plot_list );

    /* If not refreshing, save current command for subsequent refreshes. */
    if ( !refresh )
    {
        /* Save new command. */
        for ( i = 0; i < token_qty; i++ )
            strcpy( current_tokens[i], tokens[i] );
        current_token_qty = token_qty;
    }

    if ( !is_operation_token( tokens[1], &oper_type ) )
    {
        popup_dialog( INFO_POPUP, "Unknown oplot operation \"%s\"; aborting.",
                      tokens[1] );
        return;
    }

    opnd1_res = opnd2_res = NULL;

    /* First get the operand 1 result. */
    idx = 2; /* Get past "oplot <operation>" tokens. */
    build_oper_result_list( token_qty, tokens, analy, &idx, TRUE, &opnd1_res );
    if ( opnd1_res == NULL )
    {
        popup_dialog( INFO_POPUP, "No operand 1 result is available." );
        return;
    }

    /* Operand 1 mesh objects are next. */
    build_oper_object_list( token_qty, tokens, analy, &idx, TRUE,
                            &opnd1_so_list );

    /* Operand 2 result can be NULL. */
    build_oper_result_list( token_qty, tokens, analy, &idx, FALSE,
                            &opnd2_res );

    /* Operand 2 mesh objects are next. */
    build_oper_object_list( token_qty, tokens, analy, &idx, FALSE,
                            &opnd2_so_list );

    /* Need at least two objects to continue. */
    if ( opnd1_so_list == NULL
            || (opnd1_so_list->next == NULL && opnd2_so_list == NULL ))
    {
        if ( opnd1_so_list != NULL )
            free( opnd1_so_list );
        if ( opnd2_so_list != NULL )
            free( opnd2_so_list );

        popup_dialog( INFO_POPUP, "Please specify at least one first operand"
                      " object\nand at least two objects overall." );
        return;
    }

    /* If operand 2 list is NULL, sort the operand 1 list into two. */
    if ( opnd2_so_list == NULL )
    {
        for ( p_so = opnd1_so_list; p_so != NULL; NEXT( p_so ) )
        {
            p_so2 = p_so->next;
            if ( p_so2 != NULL )
            {
                UNLINK( p_so2, opnd1_so_list );
                INIT_PTRS( p_so2 );
                APPEND( p_so2, opnd2_so_list );
            }
        }
    }

#ifdef OPER_HAS_ABSCISSA
    /* Check for abscissa specification to finish parsing the command line. */
    parse_abscissa_spec( token_qty, tokens, analy, &idx, &res_list );
#endif

    /* Ensure there's a global object if the results need one. */
    check_for_global( opnd1_res, &opnd1_so_list, analy );
    if ( opnd2_res != NULL )
        check_for_global( opnd2_res, &opnd2_so_list, analy );

    if ( opnd1_so_list == NULL || opnd2_so_list == NULL )
    {
        remove_unused_results( &opnd1_res );
        if ( opnd2_res != NULL )
            remove_unused_results( &opnd2_res );
        if ( opnd1_so_list != NULL )
            DELETE_LIST( opnd1_so_list );
        if ( opnd2_so_list != NULL )
            DELETE_LIST( opnd2_so_list );
        popup_dialog( INFO_POPUP,
                      "Either or both operand object lists are empty; "
                      "aborting." );
        return;
    }

    /* Generate necessary time series objects and subrecord gather trees. */
    valid_opnd1_tsos = valid_opnd2_tsos = 0;
    valid_opnd1_tsos = gen_gather( opnd1_res, opnd1_so_list, analy,
                                   &opnd1_gather_list );
    if ( opnd2_res != NULL )
        valid_opnd2_tsos = gen_gather( opnd2_res, opnd2_so_list, analy,
                                       &opnd2_gather_list );
    else
        valid_opnd2_tsos = gen_gather( opnd1_res, opnd2_so_list, analy,
                                       &opnd2_gather_list );

    if ( valid_opnd1_tsos == 0 || valid_opnd2_tsos == 0 )
    {
        /* Had results and objects, but no valid combinations.  Abort. */
        if ( opnd1_gather_list == NULL )
            remove_unused_time_series( &opnd1_gather_list, NULL, analy );
        if ( opnd2_gather_list == NULL )
            remove_unused_time_series( &opnd2_gather_list, NULL, analy );
        remove_unused_results( &opnd1_res );
        if ( opnd2_res != NULL )
            remove_unused_results( &opnd2_res );
        if ( opnd1_so_list != NULL )
            DELETE_LIST( opnd1_so_list );
        if ( opnd2_so_list != NULL )
            DELETE_LIST( opnd2_so_list );
        popup_dialog( INFO_POPUP,
                      "Either or both operand time series lists are empty.\n"
                      "(results not available on objects); aborting." );
        return;
    }

    /* Combine the opnd1 and opnd2 operand time series lists. */
    APPEND( opnd2_gather_list, opnd1_gather_list );

    /* Generate state "segments" with constant gather lists. */
    gen_control_list( opnd1_gather_list, analy, &control_list );

    /* Update "evaluated at" limits for each time series. */
    update_eval_states( opnd1_gather_list, analy );

    /* Keep a reference to first of any extant time series'. */
    old_tsos = analy->time_series_list;

    /* Attach the old series list to the tail of the new list. */
    if ( old_tsos != NULL )
        APPEND( old_tsos, opnd1_gather_list );

    /* Update time series list pointer. */
    analy->time_series_list = opnd1_gather_list;

    /* Create operation resultant time series'. */
    create_oper_time_series( analy, opnd1_res, opnd1_so_list,
                             opnd2_res, opnd2_so_list,
                             oper_type, &oper_time_series );

    /* Update "evaluated at" limits for each time series. */
    update_eval_states( oper_time_series, analy );

#ifdef OPER_HAS_ABSCISSA
    if ( analy->abscissa != NULL )
    {
        /* Remove non-time abscissa from results to leave only ordinates. */
        UNLINK( analy->abscissa, res_list );
    }
    else
    {
#endif
        /* Retrieve time array from db if necessary. */
        if ( analy->times == NULL )
            analy->times = new_time_tso( analy );
        else if ( analy->times->state_blocks[0][1] != analy->last_state )
        {
            destroy_time_series( &analy->times );
            analy->times = new_time_tso( analy );
        }
#ifdef OPER_HAS_ABSCISSA
    }
#endif

    /* Create plot objects. */
    set_oper_plot_objects( oper_time_series, analy, p_plot_list );

    /* Hang the operation resultant time series for storage. */
    old_tsos = analy->oper_time_series_list;

    /* Attach the old series list to the tail of the new list. */
    if ( old_tsos != NULL )
        APPEND( old_tsos, oper_time_series );

    /* Update time series list pointer. */
    analy->oper_time_series_list = oper_time_series;

    /* Don't need Specified_obj list anymore. */
    DELETE_LIST( opnd1_so_list );
    DELETE_LIST( opnd2_so_list );

    /* Hang operand results for long-term storage. */
    APPEND( opnd1_res, analy->series_results );
    if ( opnd2_res != NULL )
        APPEND( opnd2_res, analy->series_results );

    /* Gather the time series data. */
    gather_time_series( control_list, analy );

    /* Clean up. */
    clear_gather_resources( &control_list, analy );

    /* Now we can perform "The Operation" */
    compute_operation_time_series( *p_plot_list );
}


/*****************************************************************
 * TAG( update_plot_objects )
 *
 * Update extant Plot_obj's for new min or max states.
 */
extern void
update_plot_objects( Analysis *analy )
{
    Plot_obj *p_po;
    Time_series_obj *p_tso, *update_list, *old_tsos;
    Time_series_obj *oper_update_list;
    int curmin, curmax;
    int i, j;
    int *p_i;
    Bool_type ab_is_time;
    List_head *p_lh, *fmt_blks;
    State_rec_obj *p_srec;
    Subrec_obj *p_subrec;
    Gather_segment *control_list;

    if ( analy->current_plots == NULL )
        return;

    curmin = GET_MIN_STATE( analy );
    curmax = get_max_state( analy );

    update_srec_fmt_blocks( curmin, curmax, analy );
    fmt_blks = analy->format_blocks;

    /* Traverse extant plot objects and extract time series' that are stale. */
    update_list = NULL;
    oper_update_list = NULL;
    ab_is_time = FALSE;
    for ( p_po = analy->current_plots; p_po != NULL; NEXT( p_po ) )
    {
        if ( ab_is_time )
        {
            p_po->abscissa = analy->times;
            p_po->abscissa->reference_count++;
        }
        else
        {
            p_tso = p_po->abscissa;

            if ( !ab_is_time && p_tso->mo_class == NULL )
                /* Abscissa is time. */
                ab_is_time = TRUE;

            if ( p_tso->min_eval_st > curmin || p_tso->max_eval_st < curmax )
            {
                /* Abscissa time series needs updating. */

                if ( ab_is_time )
                {
                    destroy_time_series( &analy->times );
                    analy->times = new_time_tso( analy );
                    p_po->abscissa = analy->times;
                    p_po->abscissa->reference_count++;
                }
                else
                {
                    /*
                     * Abscissa is a result.
                     *
                     * Non-time abscissas currently cannot be operation series.
                     */
                    UNLINK( p_tso, analy->time_series_list );
                    INIT_PTRS( p_tso );
                    INSERT( p_tso, update_list );
                }
            }
        }

        p_tso = p_po->ordinate;

        if ( p_tso->min_eval_st > curmin || p_tso->max_eval_st < curmax )
        {
            /*
             * Ordinate time series needs updating.
             *
             * An ordinate can be an operation time series, so check for this.
             * If so, put its operand time series' on the update list instead
             * and save the resultant time series aside for later re-
             * calculation.
             */
            if ( p_tso->operand1 != NULL )
            {
                if ( p_tso->update1 == FALSE )
                {
                    UNLINK( p_tso->operand1, analy->time_series_list );
                    INIT_PTRS( p_tso->operand1 );
                    INSERT( p_tso->operand1, update_list );
                    p_tso->update1 = TRUE;
                }

                if ( p_tso->update2 == FALSE )
                {
                    UNLINK( p_tso->operand2, analy->time_series_list );
                    INIT_PTRS( p_tso->operand2 );
                    INSERT( p_tso->operand2, update_list );
                    p_tso->update2 = TRUE;
                }

                UNLINK( p_tso, analy->oper_time_series_list );
                INIT_PTRS( p_tso );
                INSERT( p_tso, oper_update_list );
            }
            else
            {
                UNLINK( p_tso, analy->time_series_list );
                INIT_PTRS( p_tso );
                INSERT( p_tso, update_list );
            }
        }
    }

    /* Get out if no updates are required. */
    if ( update_list == NULL && oper_update_list == NULL )
        return;

    /* Use the series' srec maps to generate gather trees. */
    for ( p_tso = update_list; p_tso != NULL; NEXT( p_tso ) )
    {
        for ( i = 0; i < analy->qty_srec_fmts; i++ )
        {
            add_valid_states( p_tso, fmt_blks + i, curmin, curmax, i );

            p_lh = ((List_head *) p_tso->series_map.list) + i;

            if ( p_lh->qty == 0 )
                continue;

            p_srec = analy->srec_tree + i;
            p_i = (int *) p_lh->list;

            for ( j = 0; j < p_lh->qty; j++ )
            {
                p_subrec = p_srec->subrecs + p_i[j];

                add_gather_tree_entry( p_subrec, p_tso );

                p_srec->series_qty++;
            }
        }
    }

    /* Update operation plot resultants' state blocks. */
    for ( p_tso = oper_update_list; p_tso != NULL; NEXT( p_tso ) )
        for ( i = 0; i < analy->qty_srec_fmts; i++ )
            add_valid_states( p_tso, fmt_blks + i, curmin, curmax, i );

    /* Generate state "segments" with constant gather lists. */
    gen_control_list( update_list, analy, &control_list );

    /* Update "evaluated at" limits for each time series. */
    update_eval_states( update_list, analy );
    update_eval_states( oper_update_list, analy );

    /* Keep a reference to first of any extant time series'. */
    old_tsos = analy->time_series_list;

    if ( old_tsos != NULL )
        APPEND( old_tsos, update_list );

    /* Attach the old series list to the tail of the new list. */
    analy->time_series_list = update_list;

    /* Same save logic for operation time series. */
    old_tsos = analy->oper_time_series_list;

    if ( old_tsos != NULL )
        APPEND( old_tsos, oper_update_list );

    /* Attach the old series list to the tail of the new list. */
    analy->oper_time_series_list = oper_update_list;

    /* Gather the time series data. */
    gather_time_series( control_list, analy );

    /* Clean up. */
    clear_gather_resources( &control_list, analy );

    if ( oper_update_list != NULL )
        compute_operation_time_series( analy->current_plots );
}


/*****************************************************************
 * TAG( compute_operation_time_series )
 *
 * Calculate the diff/sum/product/quotient time series'.
 */
static void
compute_operation_time_series( Plot_obj *p_plot_list )
{
    Plot_obj *p_po;
    Plot_operation_type otype;
    int i, j;
    int start, stop;
    float *op_data, *opnd1_data, *opnd2_data;
    float minval, maxval, opval;
    int minval_st, maxval_st;
    Time_series_obj *p_op_tso;
    int divides_by_zero;

    divides_by_zero = 0;

    /* Peek to get operation plot type. */
    otype = p_plot_list->ordinate->op_type;

    for ( p_po = p_plot_list; p_po != NULL; NEXT( p_po ) )
    {
        p_op_tso = p_po->ordinate;
        op_data = p_op_tso->data;
        opnd1_data = p_op_tso->operand1->data;
        opnd2_data = p_op_tso->operand2->data;

        /* Init min/max from the data. */
        j = p_op_tso->state_blocks[0][0];
        switch( otype )
        {
        case OP_DIFFERENCE:
            opval = (double) opnd1_data[j] - opnd2_data[j];
            break;
        case OP_SUM:
            opval = (double) opnd1_data[j] + opnd2_data[j];
            break;
        case OP_PRODUCT:
            opval = (double) opnd1_data[j] * opnd2_data[j];
            break;
        case OP_QUOTIENT:
            if ( opnd2_data[j] == 0.0 )
                opval = 0.0;
            else
                opval = (double) opnd1_data[j] / opnd2_data[j];
            break;
        }
        minval = maxval = opval;
        minval_st = maxval_st = 0;

        for ( i = 0; i < p_op_tso->qty_blocks; i++ )
        {
            /* Get start and stop for current valid data block. */
            start = p_op_tso->state_blocks[i][0];
            stop = p_op_tso->state_blocks[i][1];

            /* Compute the operence and save. */
            for ( j = start; j <= stop; j++ )
            {
                switch( otype )
                {
                case OP_DIFFERENCE:
                    opval = (double) opnd1_data[j] - opnd2_data[j];
                    break;
                case OP_SUM:
                    opval = (double) opnd1_data[j] + opnd2_data[j];
                    break;
                case OP_PRODUCT:
                    opval = (double) opnd1_data[j] * opnd2_data[j];
                    break;
                case OP_QUOTIENT:
                    if ( opnd2_data[j] == 0.0 )
                    {
                        opval = 0.0;
                        divides_by_zero++;
                    }
                    else
                        opval = (double) opnd1_data[j] / opnd2_data[j];
                    break;
                }

                if ( opval < minval )
                {
                    minval = opval;
                    minval_st = j;
                }
                else if ( opval > maxval )
                {
                    maxval = opval;
                    maxval_st = j;
                }

                op_data[j] = opval;
            }
        }

        p_op_tso->min_val = minval;
        p_op_tso->min_val_state = minval_st;
        p_op_tso->max_val = maxval;
        p_op_tso->max_val_state = maxval_st;

        if ( divides_by_zero > 0 )
        {
            popup_dialog( INFO_POPUP, "Divide(s) by zero intercepted in "
                          "quotient time series\ncalculation (%d times, from "
                          "%s %d %s); set to zero.", divides_by_zero,
                          p_op_tso->operand2->mo_class->long_name,
                          p_op_tso->operand2->ident + 1,
                          p_op_tso->operand2->result->name );
            divides_by_zero = 0;
        }
    }
}


/*****************************************************************
 * TAG( output_interleaved_series )
 *
 * Write current plots to a file in parallel columns.
 */
static void
output_interleaved_series( FILE *ofile, Plot_obj *output_list, Analysis *analy )
{
    Plot_obj *p_po;
    float *times;
    float scale, offset;
    int qty, i, j, idx;
    int label;
    int tlen, time_width;
    int *column_widths;
    Result *ord_res;
    Title_string *ord_titles;
    Time_series_obj *ord_tso;
    Bool_type addl_output, no_convert, oper_series;
    char *op_symbols[] =
    {
        " ", "-", "+", "*", "/"
    };

    times = output_list->abscissa->data;

    if ( analy->perform_unit_conversion )
    {
        scale = analy->conversion_scale;
        offset = analy->conversion_offset;
        no_convert = FALSE;
    }
    else
        no_convert = TRUE;

    oper_series = ( output_list->ordinate->op_type != OP_NULL );

    /* Count the plots. */
    for ( p_po = output_list, qty = 0; p_po != NULL;
            NEXT( p_po ), qty++ );

    ord_titles = NEW_N( Title_string, qty * 2, "Plot output ordinate titles" );
    column_widths = NEW_N( int, qty, "Plot output column widths" );

    /* Traverse the plots and write out descriptive info. */
    addl_output = FALSE;
    for ( p_po = output_list, i = 0; p_po != NULL; NEXT( p_po ), i++ )
    {
        ord_tso = p_po->ordinate;
        ord_res = ord_tso->result;

        /* Initialize current block in preparation for writing data out. */
        ord_tso->cur_block = 0;

        /* Set initial column width for 13.6e numeric format. */
        column_widths[i] = 13;

        /* Titles index (2 entries per time series). */
        idx = i * 2;

        /* use labels if they exist */
        if(p_po->ordinate->mo_class)
        {
            if(p_po->ordinate->mo_class->labels_found)
            {
                label = get_class_label(p_po->ordinate->mo_class, p_po->ordinate->ident);
            } else
            {
                label = p_po->ordinate->ident + 1;
            }
        }

        if ( !oper_series )
        {
            /* Record the object. */
            sprintf( ord_titles[idx], "%s %d", ord_tso->mo_class->long_name,
                     label);
            tlen = strlen( ord_titles[idx] );
            if ( tlen > column_widths[i] )
                column_widths[i] = tlen;

            /* Record the result title.*/
            sprintf( ord_titles[idx + 1], ord_tso->result->title );
            tlen = strlen( ord_titles[idx + 1] );
            if ( tlen > column_widths[i] )
                column_widths[i] = tlen;
        }
        else
        {
            /* Record the first operand object result.*/
            sprintf( ord_titles[idx], "%s %d %s",
                     ord_tso->operand1->mo_class->long_name,
                     ord_tso->operand1->ident + 1,
                     ord_tso->operand1->result->title );
            tlen = strlen( ord_titles[idx] );
            if ( tlen > column_widths[i] )
                column_widths[i] = tlen;

            /* Record the operation and the second operand object result.*/
            sprintf( ord_titles[idx + 1], "%s %s %d %s",
                     op_symbols[ord_tso->op_type],
                     ord_tso->operand2->mo_class->long_name,
                     ord_tso->operand2->ident + 1,
                     ord_tso->operand2->result->title );
            tlen = strlen( ord_titles[idx + 1] );
            if ( tlen > column_widths[i] )
                column_widths[i] = tlen;

            /* Don't need the stuff below for operation plot data. */
            continue;
        }

        if ( ord_res->modifiers.use_flags.use_strain_variety )
        {
            addl_output = TRUE;
            fprintf( ofile, "# %s %s strain variety: %s\n", ord_titles[idx],
                     ord_titles[idx + 1],
                     strain_label[ord_res->modifiers.strain_variety] );
        }

        if ( ord_res->modifiers.use_flags.use_ref_surface
                && ord_tso->mo_class->superclass == G_QUAD )
        {
            addl_output = TRUE;
            fprintf( ofile, "# %s %s reference surface: %s\n", ord_titles[idx],
                     ord_titles[idx + 1],
                     ref_surf_label[ord_res->modifiers.ref_surf] );
        }

        if ( ord_res->modifiers.use_flags.use_ref_frame  &&
                (     ord_tso->mo_class->superclass == G_QUAD
                      ||  ord_tso->mo_class->superclass == G_HEX  ) )
        {
            addl_output = TRUE;
            fprintf( ofile, "# %s %s reference frame: %s\n", ord_titles[idx],
                     ord_titles[idx + 1],
                     ref_frame_label[ord_res->modifiers.ref_frame] );
        }

        if ( ord_res->modifiers.use_flags.time_derivative )
        {
            addl_output = TRUE;
            fprintf( ofile, "# %s %s strain type: rate\n", ord_titles[idx],
                     ord_titles[idx + 1] );
        }

        if ( ord_res->modifiers.use_flags.coord_transform )
        {
            addl_output = TRUE;
            fprintf( ofile, "# %s %s global coord transform: yes\n",
                     ord_titles[idx], ord_titles[idx + 1] );
        }

        if ( ord_res->modifiers.use_flags.use_ref_state )
        {
            addl_output = TRUE;
            fprintf( ofile, "# %s %s reference state: %d\n",
                     ord_titles[idx], ord_titles[idx + 1],
                     ord_res->modifiers.ref_state );
        }
    }

    if ( addl_output )
        fprintf( ofile, "#\n" );

    /* Increment column spacing to allow for minimum of 2 blanks' separation. */
    for ( i = 0; i < qty; i++ )
        column_widths[i] += 2;

    /* Write column headers. */
    time_width = 13;
    fprintf( ofile, "#%*s", time_width - 1, " " );
    for ( i = 0; i < qty; i++ )
        fprintf( ofile, "%*s", column_widths[i], ord_titles[i * 2] );
    fprintf( ofile, "\n" );
    fprintf( ofile, "#%*s", time_width - 1, "Time" );
    for ( i = 0; i < qty; i++ )
        fprintf( ofile, "%*s", column_widths[i], ord_titles[i * 2 + 1] );
    fprintf( ofile, "\n" );

    if ( analy->th_single_col )
    {
        fprintf( ofile, "# %s - %d: %s", ord_tso->mo_class->long_name, ord_tso->ident + 1, ord_titles[0] );
    }

    /* Write the data. */
    for ( i = analy->first_state;
            i <= analy->last_state;
            i++ )
    {
        /* Abscissa is time. */
        fprintf( ofile, "%*.6e", time_width, times[i] );

        /* Traverse the plot list to get ordinate values at current state. */
        for ( p_po = output_list, j = 0; p_po != NULL; NEXT( p_po ), j++ )
        {
            ord_tso = p_po->ordinate;
            idx = ord_tso->cur_block;

            if ( idx < ord_tso->qty_blocks
                    && i >= ord_tso->state_blocks[idx][0] )
            {
                if ( i <= ord_tso->state_blocks[idx][1] )
                    /* State falls within current valid block of data. */
                    fprintf( ofile, "%*.6e", column_widths[j],
                             ( no_convert
                               ? ord_tso->data[i]
                               : ord_tso->data[i] * scale + offset ) );
                else
                {
                    /* State falls beyond current valid block. */

                    /* Increment current block until it contains state. */
                    idx++;
                    while ( idx < ord_tso->qty_blocks )
                    {
                        if ( i <= ord_tso->state_blocks[idx][1] )
                            break;
                        else
                            idx++;
                    }

                    if ( idx < ord_tso->qty_blocks )
                        /* Found block containing current state. */
                        fprintf( ofile, "%*.6e", column_widths[j],
                                 ( no_convert
                                   ? ord_tso->data[i]
                                   : ord_tso->data[i] * scale + offset ) );
                    else
                        /* State is beyond all in valid blocks. */
                        fprintf( ofile, "%*s", column_widths[j], " " );

                    ord_tso->cur_block = idx;
                }
            }
            else
                fprintf( ofile, "%*s", column_widths[j], " " );
        }

        /* Terminate the line of data for the current state. */
        fputc( (int) '\n', ofile );
    } /* Loop on Time */

    fputc( (int) '\n', ofile );

    free( ord_titles );
    free( column_widths );
}


/*****************************************************************
 * TAG( output_one_series )
 *
 * Write one plot curve to a file.
 */
static void
output_one_series( int index, Plot_obj *p_plot, FILE *ofile, Analysis *analy )
{
    float *ab_data, *ord_data;
    int i, j;
    int col_1_width, col_2_width, title_1_width, title_2_width, cwidth=0;
    int qty_blocks, limit_state;
    Int_2tuple *blocks;
    float scale, offset;
    Result *ab_res, *ord_res;
    Bool_type ab_is_time;
    Time_series_obj *ab_tso, *ord_tso;
    int fracsz = 6;
    char ab_title[64], col_str[64];

    if ( analy->float_frac_size>fracsz )
        fracsz = analy->float_frac_size;

    if ( analy->perform_unit_conversion )
    {
        scale = analy->conversion_scale;
        offset = analy->conversion_offset;
    }

    /* Get abscissa and ordinate data. */
    ab_tso = p_plot->abscissa;
    ab_res = ab_tso->result;
    ab_data = ab_tso->data;
    ord_tso = p_plot->ordinate;
    ord_res = ord_tso->result;
    ord_data = ord_tso->data;

    if ( ab_res==NULL && analy->th_single_col )
    {
        ab_res = ord_res;
        strcpy(ab_title, "Time" );
    }
    else strcpy(ab_title, ab_res->title );

    /* Assign blocks list to drive curve segmentation. */
    if ( p_plot->abscissa->mo_class != NULL )
    {
        /* Non-time abscissa; find intersection of valid states. */
        gen_blocks_intersection( ab_tso->qty_blocks, ab_tso->state_blocks,
                                 ord_tso->qty_blocks, ord_tso->state_blocks,
                                 &qty_blocks, &blocks );
        ab_is_time = FALSE;
    }
    else
    {
        /* Abscissa is time; let ordinate drive segmentation. */
        qty_blocks = ord_tso->qty_blocks;
        blocks = ord_tso->state_blocks;
        ab_is_time = TRUE;
    }

    /* Output individual curve header. */
    if ( ab_tso->mo_class==NULL )
        fprintf( ofile, "\n#\n# Curve %d - %s %d\n", index,
                 ord_tso->mo_class->long_name, ord_tso->ident + 1 );
    else
        fprintf( ofile, "\n\n# Curve %d - %s %d\n", index,
                 ab_tso->mo_class->long_name, ab_tso->ident + 1 );

    if ( ab_res->modifiers.use_flags.use_strain_variety )
        fprintf( ofile, "# %s strain variety: %s\n", ab_title,
                 strain_label[ab_res->modifiers.strain_variety] );

    if ( ab_res->modifiers.use_flags.use_ref_surface )
        fprintf( ofile, "# %s reference surface: %s\n", ab_title,
                 ref_surf_label[ab_res->modifiers.ref_surf] );

    if ( ab_res->modifiers.use_flags.use_ref_frame )
        fprintf( ofile, "# %s reference frame: %s\n", ab_title,
                 ref_frame_label[ab_res->modifiers.ref_frame] );

    if ( ab_res->modifiers.use_flags.use_ref_state )
        fprintf( ofile, "# %s reference state: %d\n", ab_title,
                 ab_res->modifiers.ref_state );

    if ( ord_res->modifiers.use_flags.use_strain_variety )
        fprintf( ofile, "# %s strain variety: %s\n", ord_res->title,
                 strain_label[ord_res->modifiers.strain_variety] );

    if ( ord_res->modifiers.use_flags.use_ref_surface
            && ord_tso->mo_class->superclass == G_QUAD )
        fprintf( ofile, "# %s reference surface: %s\n",
                 ord_res->title,
                 ref_surf_label[ord_res->modifiers.ref_surf] );

    if ( ord_res->modifiers.use_flags.use_ref_frame  &&
            (     ord_tso->mo_class->superclass == G_QUAD
                  ||  ord_tso->mo_class->superclass == G_HEX  ) )
        fprintf( ofile, "# %s reference frame: %s\n", ord_res->title,
                 ref_frame_label[ord_res->modifiers.ref_frame] );

    if ( ord_res->modifiers.use_flags.use_ref_state )
        fprintf( ofile, "# %s reference state: %d\n", ord_res->title,
                 ord_res->modifiers.ref_state );

    /* Set initial column widths for 13.6e numeric format. */
    col_1_width = col_2_width = fracsz;

    title_1_width = strlen( ab_title );
    sprintf(col_str, "%.*e",col_1_width, 1.0);
    cwidth = strlen( col_str );
    if  ( (col_1_width*2)>title_1_width )
        title_1_width = cwidth;

    title_2_width = strlen( ord_res->title );
    sprintf(col_str, "%.*e",col_2_width, 1.0);
    cwidth = strlen( col_str );
    if  ( (col_2_width*2)>title_2_width )
        title_2_width = cwidth;

    fprintf( ofile, "\n# %*s  %*s\n", title_1_width, ab_title,
             title_2_width, ord_res->title );

    if ( analy->th_single_col )
    {
        fprintf( ofile, "# %s - %d: %s\n", ord_tso->mo_class->long_name,
                 ord_tso->ident + 1, ord_res->title );
    }

    for ( i = 0; i < qty_blocks; i++ )
    {
        limit_state = blocks[i][1] + 1;

        if ( !analy->perform_unit_conversion )
            for ( j = blocks[i][0]; j < limit_state; j++ )
                fprintf( ofile, "  %.*e  %.*e\n", col_1_width, ab_data[j],
                         col_2_width, ord_data[j] );
        else
            for ( j = blocks[i][0]; j < limit_state; j++ )
                fprintf( ofile, "%.*e  %.*e\n",
                         col_1_width, ab_data[j] * scale + offset,
                         col_2_width, ord_data[j] * scale + offset );
    }

    if ( !ab_is_time )
        free( blocks );
}


/*****************************************************************
 * TAG( destroy_time_series )
 *
 * Free resources associated with a time series object, and the
 * object.
 */
extern void
destroy_time_series( Time_series_obj **pp_tso )
{
    Time_series_obj *p_tso;
    int qty, i;
    List_head *p_lh;

    p_tso = *pp_tso;
    if ( p_tso == NULL )
        return;

    if ( p_tso->state_blocks != NULL )
        free( p_tso->state_blocks );

    if ( p_tso->data != NULL )
        free( p_tso->data );

    if ( p_tso->merged_formats.list != NULL )
        free( p_tso->merged_formats.list );

    if ( p_tso->result != NULL )
        p_tso->result->reference_count--;

    if ( p_tso->series_map.qty > 0 )
    {
        qty = p_tso->series_map.qty;
        p_lh = (List_head *) p_tso->series_map.list;
        for ( i = 0; i < qty; i++ )
            if ( p_lh[i].qty > 0 )
                free( p_lh[i].list );
        free( p_tso->series_map.list );
    }

    if ( p_tso->operand1 != NULL )
        p_tso->operand1->reference_count--;

    if ( p_tso->operand2 != NULL )
        p_tso->operand2->reference_count--;

    free( p_tso );

    *pp_tso = NULL;
}


/*****************************************************************
 * TAG( new_time_tso )
 *
 * Allocate and initialize a time series object to hold a time
 * array for a database.
 */
static Time_series_obj *
new_time_tso( Analysis *analy )
{
    Time_series_obj *p_tso;
    Int_2tuple st_nums;
    int i, qty_states;

    st_nums[0] = 1;
    st_nums[1] = analy->last_state + 1;

    qty_states = st_nums[1] - st_nums[0] + 1;

    p_tso = NEW( Time_series_obj, "New Time_series_obj" );
    p_tso->data = NEW_N( float, qty_states, "Time series time array" );
    p_tso->state_blocks = NEW( Int_2tuple, "Time state block" );
    p_tso->state_blocks[0][0] = st_nums[0] - 1;
    p_tso->state_blocks[0][1] = st_nums[1] - 1;
    p_tso->qty_blocks = 1;
    p_tso->qty_states = qty_states;

    analy->db_query( analy->db_ident, QRY_SERIES_TIMES, (void *) st_nums, NULL,
                     (void *) p_tso->data );

    p_tso->min_val = p_tso->data[0];
    p_tso->min_val_state = 0;
    p_tso->max_val = p_tso->data[qty_states - 1];
    p_tso->max_val_state = analy->last_state;

    p_tso->min_eval_st = 0;
    p_tso->max_eval_st = analy->last_state;

    /* All state record formats have time. */
    p_tso->merged_formats.qty = analy->qty_srec_fmts;
    p_tso->merged_formats.list = NEW_N( int, analy->qty_srec_fmts,
                                        "Merged srec fmts for time array" );
    for ( i = 0; i < analy->qty_srec_fmts; i++ )
        ((int *) p_tso->merged_formats.list)[i] = i;

    return p_tso;
}


/*****************************************************************
 * TAG( new_time_series_obj )
 *
 * Allocate and partially initialize a time series object.
 */
static Time_series_obj *
new_time_series_obj( Result *p_r, int ident, MO_class_data *p_mo_class,
                     Time_series_obj *p_operand1, Time_series_obj *p_operand2,
                     int qty_formats )
{
    Time_series_obj *p_tso;

    p_tso = NEW( Time_series_obj, "New Time_series_obj" );

    p_tso->ident = ident;
    p_tso->mo_class = p_mo_class;

    p_tso->result = p_r;
    if ( p_r != NULL )
        p_r->reference_count++;

    p_tso->operand1 = p_operand1;
    p_tso->operand2 = p_operand2;

    p_tso->series_map.qty = qty_formats;
    p_tso->series_map.list = NEW_N( List_head, qty_formats,
                                    "Series srec map heads" );

    p_tso->min_val = MAXFLOAT;
    p_tso->max_val = -MAXFLOAT;

    p_tso->min_eval_st = INVALID_STATE;
    p_tso->max_eval_st = INVALID_STATE;

    return p_tso;
}


/*****************************************************************
 * TAG( update_eval_states )
 *
 * Update the min/max evaluated states for a list of Time_series_obj's
 * to reflect current limits at which they have been evaluated.
 */
static void
update_eval_states( Time_series_obj *tso_list, Analysis *analy )
{
    Time_series_obj *p_tso;
    int first_st, last_st;

    first_st = analy->first_state;
    last_st = analy->last_state;

    for ( p_tso = tso_list; p_tso != NULL; NEXT( p_tso ) )
    {
        if ( p_tso->min_eval_st > first_st
                || p_tso->min_eval_st == INVALID_STATE )
            p_tso->min_eval_st = first_st;
        if ( p_tso->max_eval_st < last_st
                || p_tso->max_eval_st == INVALID_STATE)
            p_tso->max_eval_st = last_st;
    }
}


/*****************************************************************
 * TAG( check_for_global )
 *
 * If a G_MESH result was specified, ensure there's an object present.
 * Since there's usually only one object in the class in a G_MESH
 * class that the result could be for, we let the user slide and leave
 * out the object specification.  However, the object really needs to
 * be present to generate the plot so here we check for it and create
 * it if the user didn't supply it.
 */
static void
check_for_global( Result *res_list, Specified_obj **p_so_list, Analysis *analy )
{
    Bool_type global;
    int i, j;
    int *p_i;
    Result *p_r;
    Primal_result *p_pr;
    Derived_result *p_dr;
    MO_class_data *p_class;
    Specified_obj *p_so;
    Subrec_obj *p_subrecs;
    Subrecord_result *p_sr;
    int mesh_id;
    List_head *p_lh;

    /* First search results for one with a G_MESH superclass. */
    global = FALSE;
    for ( p_r = res_list; p_r != NULL && !global; NEXT( p_r ) )
    {
        if ( p_r->qty != 0 )
        {
            for ( i = 0; i < p_r->qty; i++ )
                if ( p_r->superclasses[i] == G_MESH )
                {
                    /*
                     * Assume there's only one G_MESH class in the mesh at
                     * the current state and get it from the mesh table
                     * rather than burrowing throught the result to uncover
                     * a reference.
                     */
                    p_lh = MESH_P( analy )->classes_by_sclass + G_MESH;
                    p_class = ((MO_class_data **) p_lh->list)[0];
                    global = TRUE;
                    break;
                }
        }
        else
        {
            /* Ugh. Result unavailable at current state so we have to burrow. */

            if ( p_r->origin.is_primal )
            {
                p_pr = (Primal_result *) p_r->original_result;

                /*
                 * For primal results, just grab the object (class) from the
                 * first format/subrecord binding a G_MESH class.
                 */

                for ( i = 0; i < analy->qty_srec_fmts; i++ )
                {
                    p_subrecs = analy->srec_tree[i].subrecs;
                    p_i = (int *) p_pr->srec_map[i].list;

                    for ( j = 0; j < p_pr->srec_map[i].qty; j++ )
                    {
                        p_class = p_subrecs[p_i[j]].p_object_class;

                        if ( p_class->superclass == G_MESH )
                        {
                            global = TRUE;
                            break;
                        }
                    }

                    if ( global )
                        break;
                }
            }
            else
            {
                p_dr = (Derived_result *) p_r->original_result;

                /*
                 * For derived results, we do the same as for primals unless
                 * the result is indirect, in which case we grab the first
                 * G_MESH class from the mesh on which the state record format
                 * is defined that provides the result on some subrecord
                 * (just keep reading that over and over, it will make sense
                 * eventually).
                 */

                for ( i = 0; i < analy->qty_srec_fmts; i++ )
                {
                    p_subrecs = analy->srec_tree[i].subrecs;
                    p_sr = (Subrecord_result *) p_pr->srec_map[i].list;

                    for ( j = 0; j < p_dr->srec_map[i].qty; j++ )
                    {
                        if ( p_sr[j].candidate->superclass == G_MESH )
                        {
                            if ( p_sr[j].indirect )
                            {
                                /*
                                 * In case there's more than one mesh, make sure
                                 * we have the mesh for this state rec format.
                                 */
                                analy->db_query( analy->db_ident, QRY_SREC_MESH,
                                                 &i, NULL, &mesh_id );
                                p_lh = analy->mesh_table[
                                           mesh_id].classes_by_sclass + G_MESH;
                                p_class = ((MO_class_data **) p_lh->list)[0];
                                global = TRUE;
                            }
                            else
                            {
                                p_class =
                                    p_subrecs[p_sr[j].subrec_id].p_object_class;
                                global = TRUE;
                            }

                            if ( global )
                                break;
                        }
                    }

                    if ( global )
                        break;
                }
            }
        }
    }

    if ( global )
    {
        /* Found G_MESH result, so search objects for "the mesh". */
        for ( p_so = *p_so_list; p_so != NULL; NEXT( p_so ) )
            if ( p_so->mo_class->superclass == G_MESH )
                break;

        /* If entire object list was traversed, there was no G_MESH object. */
        if ( p_so == NULL )
        {
            p_so = NEW( Specified_obj, "Mesh specified obj" );
            p_so->mo_class = p_class;
            p_so->ident = 0;
            INSERT( p_so, *p_so_list );
        }
    }
}


/*****************************************************************
 * TAG( build_result_list )
 *
 * Parse the plot command line and generate plot result list.
 *
 * Note - the use of analy->result_mod below could result in
 * duplicating results unintentionally if the result modified was
 * not one in the plot set.  This may not be a problem, but if
 * ever we search a result list with the expectation that the
 * same result won't appear twice it could be.
 */
static void
build_result_list( int token_qty, char tokens[][TOKENLENGTH],
                   Analysis *analy, int *p_index, Result **p_list )
{
    int idx;
    Result *res_list, *p_r;
    Bool_type found;

    idx = 1;
    res_list = NULL;

    if ( token_qty == 1 )
    {
        /*
         * No plot arguments - use current.
         *
         * If analy->result_mod is TRUE, meaning this is occurring as a result
         * of some change in a currently used result (such as strain type
         * or surface), we don't want to re-use the result already plotted.
         *
         * Was using find_result_in_list( analy->cur_result,...) here.  Using
         * find_named...() doesn't guarantee the same physical Result struct
         * but it does a correct set of checks including modifiers and result
         * table.  If it finds a match we will still be re-using the matched
         * result, so no waste.
         */

        if(analy->cur_result == NULL)
        {
            return;
        }
        if ( !analy->result_mod
                && find_named_result_in_list( analy, analy->cur_result->name,
                                              analy->series_results, &res_list ) )
        {
            UNLINK( res_list, analy->series_results );
            INIT_PTRS( res_list );
        }
        else
        {
            if ( analy->cur_result != NULL && !env.quiet_mode )
                wrt_text( "Using current result for plot.\n\n" );
            res_list = duplicate_result( analy, analy->cur_result, TRUE );
        }
    }
    else
    {
        p_r = NULL;
        while ( idx < token_qty )
        {
            /* First search among extant time history results. */
            found = analy->result_mod
                    ? FALSE
                    : find_named_result_in_list( analy, tokens[idx],
                                                 analy->series_results, &p_r );

            if ( found )
            {
                /* Remove this result from the list */

                UNLINK( p_r, analy->series_results );
                INIT_PTRS( p_r );

                /* IRC: Added May 23, 2005
                 *      SCR#: 316
                 */
                if (p_r->must_recompute)
                {
                    cleanse_result( p_r );
                    p_r = NEW( Result, "History result" );
                    found = find_result( analy, ALL, FALSE, p_r, tokens[idx] );
                }
            }
            else
            {
                /* If necessary, search among possible results for db. */
                p_r = NEW( Result, "History result" );
                found = find_result( analy, ALL, FALSE, p_r, tokens[idx] );
            }

            if ( !found )
            {
                /* Token was not a result, assume there are no results left. */
                if( !strcmp(tokens[idx], "vs"))
                {   if(p_r != NULL)
                    free( p_r );
                    p_r = NULL;
                }

                if ( res_list == NULL )
                {
                    if ( analy->cur_result != NULL && !env.quiet_mode )
                        wrt_text( "Using current result for plot.\n\n" );
                    res_list = duplicate_result( analy, analy->cur_result,
                                                 TRUE );
                }
                break;
            }
            else if ( !p_r->single_valued )
            {
                /* Can only plot scalars. */
                cleanse_result( p_r );
                free( p_r );
                p_r = NULL;
                popup_dialog( INFO_POPUP, "Ignoring non-scalar result \"%s\".",
                              tokens[idx] );
                idx++;
            }
            else
            {
                /* Good result; put on list. */
                APPEND( p_r, res_list );
                idx++;
                p_r = NULL;
            }
        }
    }

    *p_index = idx;
    *p_list = res_list;
}


/*****************************************************************
 * TAG( build_oper_result_list )
 *
 * Parse the diff plot command line from the current indexed
 * position and develop a result list.  This routine will
 * be called twice, first for the operand 1 object result
 * and second for the operand2 object result.  If in either call
 * parsing from the current position finds no result, the
 * current result will be used as the default.  If there is
 * no result among the command tokens and no current result,
 * the returned result is NULL to indicate an error condition.
 *
 * Existing note from build_result_list():
 *
 * Note - the use of analy->result_mod below could result in
 * duplicating results unintentionally if the result modified was
 * not one in the plot set.  This may not be a problem, but if
 * ever we search a result list with the expectation that the
 * same result won't appear twice it could be.
 */
static void
build_oper_result_list( int token_qty, char tokens[][TOKENLENGTH],
                        Analysis *analy, int *p_index, Bool_type operand1,
                        Result **p_list )
{
    int idx;
    Result *p_r;
    Bool_type found;

    idx = *p_index;
    p_r = NULL;

    if ( token_qty == 2 || idx == token_qty )
    {
        /*
         * No plot arguments - use current.
         *
         * If analy->result_mod is TRUE, meaning this is occurring as a result
         * of some change in a currently used result (such as strain type
         * or surface), we don't want to re-use the result already plotted.
         *
         * Was using find_result_in_list( analy->cur_result,...) here.  Using
         * find_named...() doesn't guarantee the same physical Result struct
         * but it does a correct set of checks including modifiers and result
         * table.  If it finds a match we will still be re-using the matched
         * result, so no waste.
         */
        if ( !analy->result_mod
                && operand1
                && find_named_result_in_list( analy, analy->cur_result->name,
                                              analy->series_results, &p_r ) )
        {
            UNLINK( p_r, analy->series_results );
            INIT_PTRS( p_r );
        }
        else if ( operand1 )
        {
            p_r = duplicate_result( analy, analy->cur_result, TRUE );
        }
    }
    else
    {
        /* First search among extant time history results. */
        found = analy->result_mod
                ? FALSE
                : find_named_result_in_list( analy, tokens[idx],
                                             analy->series_results, &p_r );

        if ( found )
        {
            UNLINK( p_r, analy->series_results );
            INIT_PTRS( p_r );
        }
        else
        {
            /* If necessary, search among possible results for db. */
            p_r = NEW( Result, "History result" );
            found = find_result( analy, ALL, FALSE, p_r, tokens[idx] );
        }

        if ( !found )
        {
            /* Token was not a result, assume there are no results left. */
            free( p_r );
            p_r = NULL;

            /* Only default to current result for operand 1. */
            if ( operand1 && analy->cur_result != NULL && !env.quiet_mode )
            {
                wrt_text( "Using current result for operand 1 or 2.\n\n" );
            }

            if ( operand1 && analy->cur_result != NULL )
            {
                p_r = duplicate_result( analy, analy->cur_result, TRUE );
            }
        }
        else if ( !p_r->single_valued )
        {
            /* Can only plot scalars. */
            cleanse_result( p_r );
            free( p_r );
            p_r = NULL;
            popup_dialog( INFO_POPUP, "Ignoring non-scalar result \"%s\".",
                          tokens[idx] );
            idx++;
        }
        else
        {
            /* Good result; use. */
            idx++;
        }
    }

    *p_index = idx;
    *p_list = p_r;
}


/*****************************************************************
 * TAG( build_object_list )
 *
 * Parse the token list and create a list of mesh objects.
 * Parsing terminates at the end of list or if an unclassifiable
 * token is encountered.
 */
static void
build_object_list( int token_qty, char tokens[][TOKENLENGTH],
                   Analysis *analy, int *p_idx, Specified_obj **p_so_list )
{
    char nstr[32];
    char *p_h;
    Specified_obj *p_so, *so_list;
    int i, j, o_ident, range_start, range_stop, len, label_index;
    Bool_type parsing_range, unclassed_token;
    MO_class_data *p_class;

    i = *p_idx;
    so_list = NULL;
    vslogic = FALSE;
    /* Pick off cases where we just use the selected objects. */
    if ( token_qty == 1 || i == token_qty  )
    {
        if ( analy->selected_objects != NULL && !env.quiet_mode )
        {
            wrt_text(  "Using 'select'ed object(s) for plot(s).\n\n" );
        }

        if ( analy->selected_objects != NULL )
        {
            so_list = copy_obj_list( analy->selected_objects );
        }

        *p_so_list = so_list;

        return;
    }

    parsing_range = FALSE;
    unclassed_token = FALSE;
    p_so = NULL;

    while ( i < token_qty && !unclassed_token )
    {
        switch ( get_token_type( tokens[i], analy, &p_class ) )
        {
        case MESH_OBJ_CLASS:
            if ( p_class->superclass == G_MESH )
            {
                /*
                 * A G_MESH class will have no idents following so just
                 * add it to the list, reset the current type, and continue.
                 * By setting p_class to NULL, even if an ident does
                 * follow it will be ignored (so it better have been "1").
                 */
                p_so = NEW( Specified_obj, "Parsed G_MESH MO" );
                p_so->ident = 0;
                p_so->mo_class = p_class;
                INSERT( p_so, so_list );
                p_class = NULL;
            }
            o_ident = NON_IDENT;
            break;

        case NUMERIC:
            if ( parsing_range )
            {
                /*
                 * Must already have processed the range start ident so
                 * process the range.
                 */
                range_stop = atoi( tokens[i] ) ;
                /*
                 * We have already processed range_start, so start this range
                 * at label_index+1
                 */
                range_start = label_index+1;
                add_mo_nodes( &so_list, p_class, range_start,
                              range_stop );
                o_ident = NON_IDENT;
                parsing_range = FALSE;
            }
            else if ( p_class != NULL )
            {
                /*
                 * Just a lone ident; add it to the list.
                 */
                p_so = NEW( Specified_obj, "Parsed MO" );
                p_so->mo_class = p_class;
                if(!p_class->labels_found)
                {
                    p_so->label = atoi( tokens[i] );
                    p_so->ident = p_so->label - 1;
                } else
                {
                    label_index = atoi(tokens[i]);
                    p_so->label = label_index;
                    p_so->ident = get_class_label_index(p_class, label_index);
                    if(p_so->ident == M_INVALID_LABEL)
                    {
                        free(p_so);
                        break;
                    }
                 
                }
              
                APPEND( p_so, so_list );
                o_ident = p_so->ident;
            }
            break;

        case RANGE_SEPARATOR:
            if ( p_class != NULL && o_ident != NON_IDENT )
            {
                parsing_range = TRUE;
            }
            break;

        case COMPOUND_TOKEN:
            /* There should be at least one numeral in the string... */
            if ( extract_numeric_str( tokens[i], nstr ) )
            {
                if ( tokens[i][0] == '-' )
                {
                    /*
                     * Must be processing a range and this compound token
                     * contains the range stop ident.  Process the rest
                     * of the range here.
                     */
                    range_stop = atoi( nstr );
                    /* 
                     * Use label_index+1 because we have already processed
                     * the range_start in a previous token and we do not want
                     * process it a second time.
                     */
                    if ( o_ident != NON_IDENT )
                        add_mo_nodes( &so_list, p_class, label_index+1,
                                      range_stop );
                    else
                        /*
                         * This condition is here to handle case when just
                         * -range_stop is entered so that Griz doesn't crash
                         */
                        add_mo_nodes( &so_list, p_class, o_ident,
                                      range_stop );

                    o_ident = NON_IDENT;
                }
                else if ( p_class != NULL )
                {
                    o_ident = atoi( nstr ) ;
                    range_start = o_ident;
                    p_h = strchr( tokens[i], (int) '-' );

                    /* If the last char is a hyphen... */
                    len = strlen( tokens[i] );
                    if ( tokens[i][len - 1] == '-' )
                    {
                        /*
                         * This compound token has just the range start
                         * ident so process it here and continue.
                         */
                        p_so = NEW( Specified_obj, "Parsed MO" );
                        p_so->mo_class = p_class;
                        if(!p_class->labels_found)
                        {
                            p_so->label = atoi( tokens[i] );
                            p_so->ident = p_so->label - 1;
                        } else
                        {
                            label_index = atoi(tokens[i]);
                            p_so->label = label_index;
                            p_so->ident = get_class_label_index(p_class, label_index);
                        }
                        INSERT( p_so, so_list );
                        o_ident = p_so->ident;
                        range_start = o_ident;

                        parsing_range = TRUE;
                    }
                    else if ( extract_numeric_str( p_h , nstr ) )
                    {
                        /*
                         * This compound token has both start and stop
                         * idents so process the whole range here.
                         */
                        range_stop = atoi( nstr ) ;
                        add_mo_nodes( &so_list, p_class,
                                      range_start, range_stop );
                        o_ident = NON_IDENT;
                    }
                    else
                    {
                        popup_dialog( WARNING_POPUP,
                                      "Failed to parse compound token!\n" );
                    }
                }
            }
            break;

        default:
            /*unclassed_token = TRUE;*/
            break;

        }

        i++;
    }

    if ( parsing_range )
        popup_dialog( WARNING_POPUP,
                      "Object list parsing terminated prematurely." );
    
    j = i;
    /* Added by Bill Oliver on Nov 5 2013 */
    i = 0;
    while(i < token_qty)
    {
        if(!strcmp(tokens[i], "vs") && (i + 1 < token_qty))
        {
            vslogic = TRUE;
            break;
        }
        i++;
    }

    if(vslogic == FALSE)
    {
        i = j; 
    }
    /* Return the current token index of list. */
    *p_idx = unclassed_token ? i - 1 : i;
    if(so_list == NULL && analy->selected_objects != NULL)
    {
        so_list = copy_obj_list( analy->selected_objects);
    }
    *p_so_list = so_list;
}


/*****************************************************************
 * TAG( build_oper_object_list )
 *
 * Parse the token list and create a list of mesh objects.
 * Parsing terminates at the end of list or if an unclassifiable
 * token is encountered or at the end of the sequence of objects
 * for the first class encountered - whichever comes first.
 * Terminating after the first class's objects constrains the output
 * so that the object list is completely represented by a single
 * class.  If the token list does not contain any mesh objects, the
 * logic defaults to the current "select" list.  In this case, the
 * entire select list is duplicated as the operation object list
 * regardless of how many classes are represented in it.
 */
static void
build_oper_object_list( int token_qty, char tokens[][TOKENLENGTH],
                        Analysis *analy, int *p_idx, Bool_type operand1,
                        Specified_obj **p_so_list )
{
    char nstr[32];
    char *p_h;
    Specified_obj *p_so, *so_list;
    int i, o_ident, range_start, range_stop, len;
    Bool_type parsing_range, unclassed_token;
    MO_class_data *p_class;
    int class_cnt;

    i = *p_idx;
    so_list = NULL;

    /* Pick off cases where we just use the selected objects. */
    if ( token_qty == 2 || i == token_qty || strcmp( tokens[i], "vs" ) == 0 )
    {
        /*
         * Only want to duplicate selected objects for the operand 1 object
         * list.  Return a NULL operand 2 list to flag the condition where
         * there's no separate operand 2 object list defined in the tokens.
         */
        if ( analy->selected_objects != NULL && operand1 && !env.quiet_mode )
        {
            wrt_text(  "Using 'select'ed object(s) for operands.\n\n" );
        }

        if ( analy->selected_objects != NULL && operand1 )
        {
            so_list = copy_obj_list( analy->selected_objects );
        }

        *p_so_list = so_list;

        return;
    }

    parsing_range = FALSE;
    unclassed_token = FALSE;
    p_so = NULL;
    class_cnt = 0;

    while ( i < token_qty && !unclassed_token && class_cnt < 2 )
    {
        switch ( get_token_type( tokens[i], analy, &p_class ) )
        {
        case MESH_OBJ_CLASS:
            if ( p_class->superclass == G_MESH )
            {
                /*
                 * A G_MESH class will have no idents following so just
                 * add it to the list, reset the current type, and continue.
                 * By setting p_class to NULL, even if an ident does
                 * follow it will be ignored (so it better have been "1").
                 */
                p_so = NEW( Specified_obj, "Parsed G_MESH MO" );
                p_so->ident = 0;
                p_so->mo_class = p_class;
                APPEND( p_so, so_list );
                p_class = NULL;
            }
            o_ident = NON_IDENT;
            class_cnt++;
            break;

        case NUMERIC:
            if ( parsing_range )
            {
                /*
                 * Must already have processed the range start ident so
                 * process the range.
                 */
                range_stop = atoi( tokens[i] ) - 1;
                add_mo_nodes( &so_list, p_class, range_start + 1,
                              range_stop );
                o_ident = NON_IDENT;
                parsing_range = FALSE;
            }
            else if ( p_class != NULL )
            {
                /*
                 * Just a lone ident; add it to the list.
                 */
                p_so = NEW( Specified_obj, "Parsed MO" );
                p_so->mo_class = p_class;
                p_so->ident = atoi( tokens[i] ) - 1;
                APPEND( p_so, so_list );
                o_ident = p_so->ident;
            }
            break;

        case RANGE_SEPARATOR:
            if ( p_class != NULL && o_ident != NON_IDENT )
            {
                parsing_range = TRUE;
                range_start = o_ident;
            }
            break;

        case COMPOUND_TOKEN:
            /* There should be at least one numeral in the string... */
            if ( extract_numeric_str( tokens[i], nstr ) )
            {
                if ( tokens[i][0] == '-' )
                {
                    /*
                     * Must be processing a range and this compound token
                     * contains the range stop ident.  Process the rest
                     * of the range here.
                     */
                    range_stop = atoi( nstr ) - 1;
                    add_mo_nodes( &so_list, p_class, o_ident + 1,
                                  range_stop );
                    o_ident = NON_IDENT;
                }
                else if ( p_class != NULL )
                {
                    o_ident = atoi( nstr ) - 1;
                    range_start = o_ident;
                    p_h = strchr( tokens[i], (int) '-' );

                    /* If the last char is a hyphen... */
                    len = strlen( tokens[i] );
                    if ( tokens[i][len - 1] == '-' )
                    {
                        /*
                         * This compound token has just the range start
                         * ident so process it here and continue.
                         */
                        p_so = NEW( Specified_obj, "Parsed MO" );
                        p_so->mo_class = p_class;
                        p_so->ident = atoi( tokens[i] ) - 1;
                        APPEND( p_so, so_list );
                        o_ident = p_so->ident;
                        range_start = o_ident;

                        parsing_range = TRUE;
                    }
                    else if ( extract_numeric_str( p_h + 1, nstr ) )
                    {
                        /*
                         * This compound token has both start and stop
                         * idents so process the whole range here.
                         */
                        range_stop = atoi( nstr ) - 1;
                        add_mo_nodes( &so_list, p_class,
                                      range_start, range_stop );
                        o_ident = NON_IDENT;
                    }
                    else
                    {
                        popup_dialog( WARNING_POPUP,
                                      "Failed to parse compound token!\n" );
                    }
                }
            }
            break;

        default:
            unclassed_token = TRUE;
        }

        i++;
    }

    if ( parsing_range )
        popup_dialog( WARNING_POPUP,
                      "Object list parsing terminated prematurely." );

    /* Return the current token index of list. */
    *p_idx = ( unclassed_token || class_cnt > 1 ) ? i - 1 : i;
    *p_so_list = so_list;
}


/*****************************************************************
 * TAG( parse_abscissa_spec )
 *
 * Parse an abscissa specification if found on the command line,
 * adding it if necessary to the result list to force it to be
 * gathered over all appropriate mesh objects.
 *
 * Parameter "res_list" is assumed not to be NULL.
 */
static void
parse_abscissa_spec( int token_qty, char tokens[][TOKENLENGTH],
                     Analysis *analy, int *p_index, Result **p_res_list )
{
    int idx,loop_idx;
    Bool_type found;
    Result *abscissa_result;
    idx = *p_index;

    /* Get abscissa if specified. */
    if ( idx < token_qty && strcmp( tokens[idx], "vs" ) == 0 )
    {
        idx++;

        if ( analy->abscissa != NULL )
        {
            if ( analy->abscissa->reference_count > 0 )
            {
                INIT_PTRS( analy->abscissa );
                INSERT( analy->abscissa, analy->series_results );
                analy->abscissa = NULL;
            }
            else
            {
                cleanse_result( analy->abscissa );
                free( analy->abscissa );
                analy->abscissa = NULL;
            }
        }
        
        for( loop_idx = idx; loop_idx< token_qty; loop_idx++)
        {
            abscissa_result = NEW( Result, "Abscissa result" );
            
            

            found = find_result( analy, ALL, FALSE, abscissa_result,
                                 tokens[loop_idx] );
            if ( !found )
            {
                popup_dialog( WARNING_POPUP,
                                  "Abscissa result \"%s\" not found, %s",
                                  tokens[loop_idx], "using time." );
                /* ERROR: Need to chain down deleting this */
                free( abscissa_result );
                abscissa_result = NULL;
            }
            else if ( !abscissa_result->single_valued )
            {
                cleanse_result( abscissa_result );
                free( abscissa_result );
                abscissa_result = NULL;
                popup_dialog( INFO_POPUP,
                              "Ignoring non-scalar abscissa \"%s\".",
                              tokens[loop_idx] );
            }
            else if ( !find_result_in_list( abscissa_result, *p_res_list,
                                            NULL ) )
            {
                /* Add abscissa to result list so it will be gathered. */
                if(analy->abscissa == NULL)
                {
                    analy->abscissa = abscissa_result;
                }else
                {
                    INSERT( abscissa_result, analy->abscissa );
                }
                //INSERT( abscissa_result, *p_res_list );
            }
        }
    }
    else
    {
        /* Plotting vs time, so deal with any extant abscissa reference. */
        if ( analy->abscissa != NULL )
        {
            if ( analy->abscissa->reference_count > 0 )
            {
                /* If in use, move to long-term storage. */
                INIT_PTRS( analy->abscissa );
                INSERT( analy->abscissa, analy->series_results );
            }
            else
            {
                cleanse_result( analy->abscissa );
                free( analy->abscissa );
            }

            analy->abscissa = NULL;
        }
    }

    *p_index = idx;
}


/*****************************************************************
 * TAG( remove_unused_results )
 *
 * Remove unused results from a list then hang the remainder for
 * long-term storage.
 */
extern void
remove_unused_results( Result **res_list )
{
    Result *p_r, *p_del_r, *list;

    list = p_r = *res_list;

    while ( p_r != NULL )
    {
        if ( p_r->reference_count == 0 )
        {
            p_del_r = p_r;
            NEXT( p_r );
            UNLINK( p_del_r, list );
            delete_result( &p_del_r );
        }
        else
            NEXT( p_r );
    }

    *res_list = list;
}


/*****************************************************************
 * TAG( clear_gather_resources )
 *
 * Remove and free all gather resources.
 */
static void
clear_gather_resources( Gather_segment **p_ctl_list, Analysis *analy )
{
    int i, j;
    State_rec_obj *p_state_rec;
    Subrec_obj *p_subrec;
    Result_mo_list_obj *p_rmlo;

    /* First delete the gather segment list. */
    DELETE_LIST( *p_ctl_list );

    /* Delete the gather trees. */

    /* For each state record format... */
    for ( i = 0; i < analy->qty_srec_fmts; i++ )
    {
        p_state_rec = analy->srec_tree + i;

        /* For each subrecord in current state record format... */
        for ( j = 0; j < p_state_rec->qty; j++ )
        {
            p_subrec = p_state_rec->subrecs + j;

            /* Delete each per-result branch of the gather tree. */
            for ( p_rmlo = p_subrec->gather_tree; p_rmlo != NULL;
                    NEXT( p_rmlo ) )
                DELETE_LIST( p_rmlo->mo_list );

            /* Delete the trunk (i.e., the result list). */
            DELETE_LIST( p_subrec->gather_tree );
        }

        p_state_rec->series_qty = 0;
    }
}


/*****************************************************************
 * TAG( prep_gather_tree )
 *
 * Activate those results in the gather trees required by the
 * Gather_segment; de-activate all others.
 */
static void
prep_gather_tree( Gather_segment *p_gs, Analysis *analy )
{
    int i, j, k;
    MOR_entry *p_more;
    Series_ref_obj *p_sro;
    Result_mo_list_obj *p_rmlo;
    Subrec_obj *subrec_array;
    int qty_subrecs, qty_entries;

    p_more = p_gs->entries;
    qty_entries = p_gs->entries_used;

    /* For each srec format... */
    for ( i = 0; i < analy->qty_srec_fmts; i++ )
    {
        qty_subrecs = analy->srec_tree[i].qty;
        subrec_array = analy->srec_tree[i].subrecs;

        /* De-activate all results/objects. */
        for ( j = 0; j < qty_subrecs; j++ )
        {
            for ( p_rmlo = subrec_array[j].gather_tree; p_rmlo != NULL;
                    NEXT( p_rmlo ) )
            {
                p_rmlo->active = FALSE;
                for ( p_sro = p_rmlo->mo_list; p_sro != NULL; NEXT( p_sro ) )
                    p_sro->active = FALSE;
            }
        }

        /*
         * Activate those Series_ref_obj's whose Time_series_obj is matched
         * within the Gather_segment.
         */
        for ( j = 0; j < qty_subrecs; j++ )
        {
            for ( p_rmlo = subrec_array[j].gather_tree; p_rmlo != NULL;
                    NEXT( p_rmlo ) )
            {
                for ( p_sro = p_rmlo->mo_list; p_sro != NULL; NEXT( p_sro ) )
                {
                    for ( k = 0; k < qty_entries; k++ )
                        if ( p_more[k].p_series == p_sro->series )
                        {
                            p_sro->active = TRUE;
                            p_rmlo->active = TRUE;
                            break;
                        }
                }
            }
        }
    }
}


/*****************************************************************
 * TAG( gather_time_series )
 *
 * Gather time series referenced in the gather trees.
 */
static void
gather_time_series( Gather_segment *ctl_list, Analysis *analy )
{
    int i, j;
    int obj_cnt=0;
    int first_st, last_st;
    int *srec_fmts;
    State_rec_obj *p_state_rec;
    Subrec_obj *p_subrec;
    Series_ref_obj *p_sro;
    Time_series_obj *p_tso;
    Result_mo_list_obj *p_rmlo;
    Result *p_tmp_result;
    Gather_segment *p_gs;
    int tmp_state, st_qty;
    float value, rot_value;

    p_tmp_result = analy->cur_result;
    tmp_state = analy->cur_state;

    srec_fmts = analy->srec_formats;

    /*
     * Loop over the gather segments.  During each segment, the set of
     * mesh object/result combinations being gathered is constant.
     */
    for ( p_gs = ctl_list; p_gs != NULL; NEXT( p_gs ) )
    {
        /* Activate/deactivate entries in the gather trees for the segment. */
        prep_gather_tree( p_gs, analy );

        /* Loop over the states in the segment. */
        first_st = p_gs->start_state;
        last_st = p_gs->end_state;

        for ( i = first_st; i <= last_st; i++ )
        {
            p_state_rec = analy->srec_tree + srec_fmts[i];

            if ( p_state_rec->series_qty == 0 )
                continue;

            analy->cur_state = i;

            analy->db_get_state( analy, i, analy->state_p, &analy->state_p,
                                 &st_qty );

            /* Loop over subrecords in the format of the current state. */
            for ( j = 0; j < p_state_rec->qty; j++ )
            {
                p_subrec = p_state_rec->subrecs + j;

                /* Traverse the gather tree, generating and saving results. */
                for ( p_rmlo = p_subrec->gather_tree; p_rmlo != NULL;
                        NEXT( p_rmlo ) )
                {
                    if ( !p_rmlo->active )
                        continue;
                    /******
                                        primal_name = p_rmlo->result->primals;
                                        if( strcmp( primal_name, "nodpos") == 0 )
                                            break;
                    *****/
                    /*
                     * Gather p_rmlo result on this subrecord.
                     *
                     * Shouldn't need to zero result array prior to each load.
                     */
                    p_rmlo->result->modifiers.use_flags.use_ref_surface = 1;
                    p_rmlo->result->modifiers.use_flags.use_ref_frame = 1;
                    analy->cur_result = p_rmlo->result;

                    load_subrecord_result( analy, j, TRUE, FALSE );

                    /* Store result value for each object in list. */
                    obj_cnt=0;
                    for ( p_sro = p_rmlo->mo_list;
                            p_sro != NULL; NEXT( p_sro ) )
                    {
                        if ( !p_sro->active )
                            continue;

                        p_tso = p_sro->series;

                        value = p_tso->mo_class->data_buffer[p_tso->ident];

                        obj_cnt++;
                        if (is_primal_quad_strain_result( p_rmlo->result->name ))
                        {
                            rot_value = value;
                            rotate_quad_result( analy, p_rmlo->result->name,
                                                p_tso->ident, &rot_value );
                            value=rot_value;
                        }

                        if ( value < p_tso->min_val )
                        {
                            p_tso->min_val = value;
                            p_tso->min_val_state = i;
                        }
                        if ( value > p_tso->max_val )
                        {
                            p_tso->max_val = value;
                            p_tso->max_val_state = i;
                        }
                        p_tso->data[i] = value;
                    }
                }
            }
        }
    }

    analy->cur_result = p_tmp_result;
    analy->cur_state = tmp_state;

    analy->db_get_state( analy, analy->cur_state, analy->state_p,
                         &analy->state_p, &st_qty );
}


/*****************************************************************
 * TAG( get_result_superclass )
 *
 * Determine the correct superclass of a result calculated on
 * the indicated subrecord.  We don't just use the superclass of
 * the data bound to the subrecord because that is not guaranteed
 * to be the same as the superclass of the final result.
 */
static int
get_result_superclass( int subrec_id, Result *p_result )
{
    int i;

    for ( i = 0; i < p_result->qty; i++ )
        if ( p_result->subrecs[i] == subrec_id )
            return p_result->superclasses[i];

    /* Shouldn't ever get here. */
    return -1;
}


/*****************************************************************
 * TAG( remove_unused_time_series )
 *
 * Traverse time series list until previously existing ones are
 * reached and remove any that are unreferenced.  Prune gather
 * trees for removed time series'.
 */
static void
remove_unused_time_series( Time_series_obj **p_tso_list,
                           Time_series_obj *old_list_start, Analysis *analy )
{
    Subrec_obj *p_subrec;
    State_rec_obj *p_state_rec;
    Result_mo_list_obj *p_rmlo;
    Time_series_obj *p_tso, *delete_tso, *tso_list;
    Series_ref_obj *p_sro;
    int i, j;

    if ( *p_tso_list == NULL )
        return;
    else
        tso_list = *p_tso_list;

    p_tso = tso_list;
    while ( p_tso != old_list_start )
    {
        if ( p_tso->reference_count > 0 )
        {
            NEXT( p_tso );
            continue;
        }

        /*
         * Remove gather references to this result/object combination
         * to avoid gathering unnecessary data.
         */

        /* For each state record format... */
        for ( i = 0; i < analy->qty_srec_fmts; i++ )
        {
            p_state_rec = analy->srec_tree + i;

            /* For each subrecord in current state record format... */
            for ( j = 0; j < p_state_rec->qty; j++ )
            {
                p_subrec = p_state_rec->subrecs + j;

                /* Find the subrec's gather branch for current result. */
                for ( p_rmlo = p_subrec->gather_tree; p_rmlo != NULL;
                        NEXT( p_rmlo ) )
                {
                    if ( p_rmlo->result == p_tso->result )
                    {
                        /*
                         * Search for a reference to the current time
                         * series on this branch.
                         */
                        for ( p_sro = p_rmlo->mo_list; p_sro != NULL;
                                NEXT( p_sro ) )
                            if ( p_sro->series == p_tso )
                            {
                                DELETE( p_sro, p_rmlo->mo_list );
                                break;
                            }

                        break;
                    }
                }
            }
        }

        delete_tso = p_tso;
        NEXT( p_tso );
        UNLINK( delete_tso, tso_list );
        destroy_time_series( &delete_tso );
    }

    *p_tso_list = tso_list;
}


/*****************************************************************
 * TAG( create_oper_time_series )
 *
 * Generate a list of operation resultant time series.
 *
 * NOTE - Assumes that the operand time series have already been created.
 *
 * Use the format and availability data in the first operand time series as
 * the template for that of the resultant time series.  Technically, it would
 * be more accurate to generate the merger of this info for the two operands,
 * but we'll keep it simple in hopes that there's never a difference.
 */
static void
create_oper_time_series( Analysis *analy,
                         Result *p_opnd1_res, Specified_obj *p_opnd1_so_list,
                         Result *p_opnd2_res, Specified_obj *p_opnd2_so_list,
                         Plot_operation_type otype,
                         Time_series_obj **p_oper_time_series )
{
    Time_series_obj *p_tso1, *p_tso2, *p_oper_list, *p_op_tso;
    Specified_obj *p_so1, *p_so2;
    Bool_type processing;

    p_oper_list = NULL;

    if ( p_opnd2_res == NULL )
        p_opnd2_res = p_opnd1_res;

    p_so1 = p_opnd1_so_list;
    p_so2 = p_opnd2_so_list;
    processing = TRUE;
    while ( processing )
    {
        if ( find_time_series( p_opnd1_res, p_so1->ident, p_so1->mo_class,
                               analy, analy->time_series_list, &p_tso1 )
                && find_time_series( p_opnd2_res, p_so2->ident, p_so2->mo_class,
                                     analy, analy->time_series_list, &p_tso2 ) )
        {
            for ( p_op_tso = analy->oper_time_series_list; p_op_tso != NULL;
                    NEXT( p_op_tso ) )
            {
                if ( p_op_tso->operand1 == p_tso1
                        && p_op_tso->operand2 == p_tso2
                        && p_op_tso->op_type == otype )
                {
                    UNLINK( p_op_tso, analy->oper_time_series_list );
                    INIT_PTRS( p_op_tso );
                    break;
                }
            }

            if ( p_op_tso == NULL )
            {
                /* Got to the end of the list with no match, so create new. */
                p_op_tso = new_oper_time_series( p_tso1, p_tso2, otype );
            }

            /* Add it to the list. */
            APPEND( p_op_tso, p_oper_list );
        }

        /*
         * Manage the Specified_obj list traversals.  If either is shorter,
         * repeat its last object until the end of the longer list is
         * reached.
         */
        if ( p_so1->next == NULL && p_so2->next == NULL )
            processing = FALSE;
        else if ( p_so1->next == NULL )
            NEXT( p_so2 );
        else if ( p_so2->next == NULL )
            NEXT( p_so1 );
        else
        {
            NEXT( p_so1 );
            NEXT( p_so2 );
        }
    }

    *p_oper_time_series = p_oper_list;
}


/*****************************************************************
 * TAG( new_oper_time_series )
 *
 * Allocate and initialize an operation time series.
 */
static Time_series_obj *
new_oper_time_series( Time_series_obj *p_tso1, Time_series_obj *p_tso2,
                      Plot_operation_type otype )
{
    Time_series_obj *p_op_tso;
    int i, j;
    int stqty;
    List_head *p_lh_src, *p_lh_dest;

    p_op_tso = new_time_series_obj( NULL, 0, NULL, p_tso1, p_tso2,
                                    p_tso1->series_map.qty );

    stqty = p_tso1->state_blocks[p_tso1->qty_blocks - 1][1] + 1;
    p_op_tso->data = NEW_N( float, stqty, "Oper series array" );
    p_op_tso->qty_states = stqty;

    if ( p_tso1->qty_blocks > 0 )
    {
        p_op_tso->qty_blocks = p_tso1->qty_blocks;
        p_op_tso->state_blocks = NEW_N( Int_2tuple,
                                        p_op_tso->qty_blocks,
                                        "State blocks for oper th" );
        for ( i = 0; i < p_op_tso->qty_blocks; i++ )
        {
            p_op_tso->state_blocks[i][0] = p_tso1->state_blocks[i][0];
            p_op_tso->state_blocks[i][1] = p_tso1->state_blocks[i][1];
        }
    }

    if ( p_tso1->merged_formats.qty > 0 )
    {
        p_op_tso->merged_formats.qty = p_tso1->merged_formats.qty;
        p_op_tso->merged_formats.list = NEW_N( int,
                                               p_tso1->merged_formats.qty,
                                               "Fmts for oper resultant" );
        for ( i = 0; i < p_op_tso->merged_formats.qty; i++ )
            ((int *) p_op_tso->merged_formats.list)[i] =
                ((int *) p_tso1->merged_formats.list)[i];
    }

    if ( p_tso1->series_map.qty > 0 )
    {
        p_op_tso->series_map.qty = p_tso1->series_map.qty;
        p_op_tso->series_map.list = NEW_N( List_head,
                                           p_tso1->series_map.qty,
                                           "Map for oper resultant" );

        p_lh_src = (List_head *) p_tso1->series_map.list;
        p_lh_dest = (List_head *) p_op_tso->series_map.list;

        for ( i = 0; i < p_op_tso->series_map.qty; i++ )
        {
            p_lh_dest[i].qty = p_lh_src[i].qty;
            if ( p_lh_src[i].qty > 0 )
            {
                p_lh_dest[i].list = NEW_N( int, p_lh_src[i].qty,
                                           "Map entry, oper ts" );
                for ( j = 0; j < p_lh_src[i].qty; j++ )
                    ((int *) p_lh_dest[i].list)[j] =
                        ((int *) p_lh_src[i].list)[j];
            }
        }
    }

    p_op_tso->op_type = otype;

    return p_op_tso;
}

/*****************************************************************
 * TAG( prepare_plot_objects )
 *
 * Cross a result list and an object list together to create
 * a list of plot objects where time series' exist for the
 * result/object combination and the associated abscissa_result/object
 * combination.
 */
static void
prepare_plot_objects( Result *res_list, Specified_obj *so_list,
                      Analysis *analy, Plot_obj **p_plot_list )
{
    Result *p_r;
    Result *abscissa_result;
    Time_series_obj *p_tso, *p_tso2;
    Specified_obj *p_so, *p_so2;
    Plot_obj *p_po, *plot_list;

    plot_list = NULL;

    for ( p_r = res_list; p_r != NULL; NEXT( p_r ) )
    {
        /*
         * Loop over specified objects and check for result/object
         * combinations in time series list.
         */
        if(!vslogic)
        {
            for ( p_so = so_list; p_so != NULL; NEXT( p_so ) )
            {
                if ( find_time_series( p_r, p_so->ident, p_so->mo_class, analy,
                                       analy->time_series_list, &p_tso )
                        && ( analy->abscissa == NULL
                             || find_time_series( analy->abscissa, p_so->ident,
                                                  p_so->mo_class, analy,
                                                  analy->time_series_list,
                                                  &p_tso2 ) ) )
                {
                    p_po = NEW( Plot_obj, "New plot" );

                    p_po->ordinate = p_tso;
                    
                    p_po->ordinate->reference_count++;

                    p_po->abscissa = ( analy->abscissa == NULL )
                                     ? analy->times : p_tso2;
                    p_po->abscissa->reference_count++;

                    APPEND( p_po, plot_list );
                }
            }
        }
        else
        {
            for ( p_so = so_list; p_so != NULL; NEXT( p_so ) )
            {

                if ( find_time_series( p_r, p_so->ident, p_so->mo_class, analy,
                                       analy->time_series_list, &p_tso ))
                {
                    if(analy->abscissa == NULL)
                    {
                        p_po = NEW( Plot_obj, "New plot" );

                        p_po->ordinate = p_tso;
                        
                        p_po->ordinate->reference_count++;

                        p_po->abscissa = analy->times;
                        p_po->abscissa->reference_count++;

                        APPEND( p_po, plot_list );
                    }else
                    {
                        for(abscissa_result = analy->abscissa;
                            abscissa_result != NULL; 
                            NEXT(abscissa_result))
                        {
                            for(p_so2 = so_list; p_so2 != NULL; NEXT(p_so2))
                            {
                                if(find_time_series(abscissa_result, p_so2->ident,
                                        p_so2->mo_class, analy,
                                        analy->time_series_list,
                                        &p_tso2))
                                {
                                    if((p_tso->mo_class == p_tso2->mo_class &&
                                        p_tso->ident == p_tso2->ident) || 
                                        p_tso->mo_class != p_tso2->mo_class)
                                    { 
                                    p_po = NEW( Plot_obj, "New plot" );

                                    p_po->ordinate = p_tso;
                                    p_po->ordinate->reference_count++;

                                    p_po->abscissa = p_tso2;
                                    p_po->abscissa->reference_count++;

                                    APPEND( p_po, plot_list );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    *p_plot_list = plot_list;
}


/*****************************************************************
 * TAG( set_oper_plot_objects )
 *
 * Generate a list of plot objects from a list of operation time
 * series' vs time.
 */
static void
set_oper_plot_objects( Time_series_obj *p_oper_series_list,
                       Analysis *analy, Plot_obj **p_plot_list )
{
    Time_series_obj *p_tso;
    Plot_obj *p_po, *plot_list;

    plot_list = NULL;

    for ( p_tso = p_oper_series_list; p_tso != NULL; NEXT( p_tso ) )
    {
        p_po = NEW( Plot_obj, "New plot" );

        p_po->ordinate = p_tso;
        p_po->ordinate->reference_count++;

        p_po->abscissa = analy->times;
        p_po->abscissa->reference_count++;

        APPEND( p_po, plot_list );
    }

    *p_plot_list = plot_list;
}


/*****************************************************************
 * TAG( add_event )
 *
 * Init and add (insertion sort on state number) a Series_evt_obj
 * to a linked list.
 */
static void
add_event( Bool_type start_gather_flag, int state, Time_series_obj *p_tso,
           Series_evt_obj **se_list )
{
    Series_evt_obj *p_new_se, *p_se, *p_last_se;

    p_new_se = NEW( Series_evt_obj, "Request series start event" );
    p_new_se->p_series = p_tso;
    p_new_se->start_gather = start_gather_flag;
    p_new_se->state = state;

    p_last_se = NULL;

    for ( p_se = *se_list; p_se != NULL; NEXT( p_se ) )
    {
        if ( p_new_se->state <= p_se->state )
        {
            INSERT_BEFORE( p_new_se, p_se );

            /* Check for new list head. */
            if ( p_new_se->prev == NULL )
                *se_list = p_new_se;

            break;
        }
        else
            p_last_se = p_se;
    }

    /* If we traversed the entire list, insertion not yet performed. */
    if ( p_se == NULL )
    {
        if ( p_last_se == NULL )
            /* Empty list. */
            *se_list = p_new_se;
        else
            /* End of list. */
            INSERT_AFTER( p_new_se, p_last_se );
    }
}


#ifdef THTEST
/*****************************************************************
 * TAG( dump_gather_segment )
 *
 * Write contents of a Gather_segment to stdout.
 */
static void
dump_gather_segment( Gather_segment *p_gs )
{
    int i;

    printf( "Start/stop states - %d/%d\n", p_gs->start_state,
            p_gs->stop_state );
    printf( "Entries used/total - %d/%d\n", p_gs->entries_used,
            p_gs->qty_entries );
    for ( i = 0; i < p_gs->entries_used; i++ )
    {
        p_tso = p_gs->entries[i].p_series;
        printf( "%2d. %s %d - %s\n", i + 1, p_tso->mo_class->short_name,
                p_tso->ident, p_tso->result->name );
    }
}
#endif


/*****************************************************************
 * TAG( push_gather )
 *
 * Add a start-gather reference in a Gather_segment.
 */
static void
push_gather( Series_evt_obj *p_se, Gather_segment *p_gs )
{
#ifdef THTEST
    int i, max;
    MOR_entry *me_array;
#endif

    if ( p_gs->qty_entries == p_gs->entries_used )
    {
        popup_dialog( WARNING_POPUP, "Gather_segment too small for push." );
        return;
    }

    p_gs->entries[p_gs->entries_used].p_series = p_se->p_series;
    p_gs->entries_used++;

#ifdef THTEST
    me_array = p_gs->entries;
    for ( i = 0; i < p_gs->entries_used - 1; i++ )
    {
        if ( me_array[i].p_series == p_se->p_series )
            fprintf( stderr, "Redundant series ref in Gather_segment.\n" );
    }
#endif
}


/*****************************************************************
 * TAG( pop_gather_lazy )
 *
 * Remove a start_gather reference from a Gather_segment if it
 * exists.
 */
static void
pop_gather_lazy( Series_evt_obj *p_se, Gather_segment *p_gs )
{
    int i;
    MOR_entry *me_array;

    me_array = p_gs->entries;
    for ( i = 0; i < p_gs->entries_used; i++ )
    {
        if ( me_array[i].p_series == p_se->p_series )
            break;
    }

    if ( i < p_gs->entries_used )
    {
        i++;
        for ( ; i < p_gs->entries_used; i++ )
            me_array[i - 1].p_series = me_array[i].p_series;

        p_gs->entries_used--;
    }
}


/*****************************************************************
 * TAG( copy_gather_segment )
 *
 * Initialize a Gather_segment and its entries array from another.
 */
static void
copy_gather_segment( Gather_segment *p_gs, Gather_segment *p_old_gs )
{
    int i;
    MOR_entry *e_array;

    /* Save the entries reference before copying the struct. */
    e_array = p_gs->entries;

    /* Copy the struct. */
    *p_gs = *p_old_gs;

    /* That was easy, now handle the entries array. */
    p_gs->entries = e_array;
    for ( i = 0; i < p_old_gs->entries_used; i++ )
        e_array[i] = p_old_gs->entries[i];
}


/*****************************************************************
 * TAG( gen_control_list )
 *
 * Evaluate requested time series limits plus limits of extant
 * time series to generate segments of states over which the list
 * of mesh object results to gather is fixed.
 */
static void
gen_control_list( Time_series_obj *gather_list, Analysis *analy,
                  Gather_segment **ctl_list )
{
    int cur_min, cur_max, start_qty, offset, cur_st;
    Time_series_obj *p_tso;
    Series_evt_obj *evt_list, *p_se, *p_del_se;
    Gather_segment *gs_list, *p_gs, *p_old_gs;

    cur_max = get_max_state( analy );
    cur_min = GET_MIN_STATE( analy );

    evt_list = NULL;

    start_qty = 0;

    /*
     * Generate "events" when gathering begins or ends on a mesh object result.
     * Events are insertion-sorted into a linked list based on the simulation
     * state at which they occur.
     */
    p_tso = gather_list;
    while ( p_tso != NULL )
    {
        if ( p_tso->min_eval_st == INVALID_STATE )
        {
            /* New Time_series_obj; generate events only for current request. */
            add_event( TRUE, cur_min, p_tso, &evt_list );
            start_qty++;
            add_event( FALSE, cur_max + 1, p_tso, &evt_list );

            NEXT( p_tso );
            continue;
        }

        /* Generate events considering request min st and extant min st. */
        if ( cur_min < p_tso->min_eval_st )
        {
            if ( cur_max >= p_tso->min_eval_st )
            {
                /* Generate events for request min st and extant min st. */
                add_event( TRUE, cur_min, p_tso, &evt_list );
                start_qty++;
                add_event( FALSE, p_tso->min_eval_st, p_tso, &evt_list );
            }
            else
            {
                /* Current request disjoint from extant; replace extant. */
                cleanse_tso_data( p_tso );

                continue;
            }
        }

        /* Generate events considering request max st and extant max st. */
        if ( cur_max > p_tso->max_eval_st )
        {
            if ( cur_min <= p_tso->max_eval_st )
            {
                /* Generate events for extant max st and request max st. */
                add_event( TRUE, p_tso->max_eval_st + 1, p_tso, &evt_list );
                start_qty++;
                add_event( FALSE, cur_max + 1, p_tso, &evt_list );
            }
            else
            {
                /* Current request disjoint from extant; replace extant. */
                cleanse_tso_data( p_tso );

                continue;
            }
        }

        NEXT( p_tso );
    }

    /* Now process the event list to generate segments. */

    p_gs = p_old_gs = gs_list = NULL;
    p_se = evt_list;
    while ( p_se != NULL )
    {
        cur_st = p_se->state;

        /*
         * Process all events at the current state.
         * A "start gather" event pushes a mesh object/result combination
         * onto the gather list for the segment, and a "stop gather" (i.e.,
         * start_gather = FALSE) event pops a mesh object/result off the list.
         */
        while ( p_se != NULL && p_se->state == cur_st )
        {
            if ( p_se->start_gather )
            {
                if ( p_gs == NULL )
                {
                    /*
                     * Allocate the Gather_segment plus its entries array in
                     * one allocation, but avoid potential alignment issues
                     * by placing the array at a whole 8-byte boundary from
                     * the start of the allocated memory.
                     */
                    offset = ROUND_UP_INT( sizeof( Gather_segment ), 8 );
                    p_gs = (Gather_segment *) calloc( 1, offset + start_qty
                                                      * sizeof( MOR_entry ) );
                    p_gs->entries = (MOR_entry *) ((char *) p_gs + offset);
                    p_gs->qty_entries = start_qty;
                    p_gs->start_state = cur_st;

                    /* Start with the list from the previous segment. */
                    if ( p_old_gs != NULL )
                        copy_gather_segment( p_gs, p_old_gs );
                }

                push_gather( p_se, p_gs );
            }
            else if ( p_gs != NULL )
                pop_gather_lazy( p_se, p_gs );

            p_del_se = p_se;
            NEXT( p_se );
            DELETE( p_del_se, evt_list );
        }

        if ( p_old_gs != NULL )
            p_old_gs->end_state = cur_st - 1;

#ifdef THTEST
        dump_gather_segment( p_gs );
#endif

        if ( p_gs != NULL )
            APPEND( p_gs, gs_list );

        p_old_gs = p_gs;
        p_gs = NULL;
    }

    *ctl_list = gs_list;
}


/*****************************************************************
 * TAG( gen_srec_fmt_blocks )
 *
 * Instantiate arrays of tuples showing which states have which
 * state record formats within the overall bounds of a specified
 * first and last state.
 */
static void
gen_srec_fmt_blocks( int qty_srec_fmts, int *state_srec_fmts, int first_state,
                     int last_state, List_head *fmt_blks )
{
    int i, j, range_first, cur_blk, st_limit;
    Int_2tuple *p_i2t;

    st_limit = last_state + 1;

    /* Loop i over srec formats (i.e., id's) for the database. */
    for ( i = 0; i < qty_srec_fmts; i++ )
    {
        j = first_state;
        while ( j < st_limit )
        {
            /* Skip states whose format doesn't match current srec id. */
            for ( ; j < st_limit && state_srec_fmts[j] != i; j++ );
            range_first = j;

            /* Find contiguous range of states whose format matches current. */
            for ( ; j < st_limit && state_srec_fmts[j] == i; j++ );

            /* If block has non-zero length, allocate a tuple and record it. */
            if ( range_first < st_limit )
            {
                cur_blk = fmt_blks[i].qty++;
                fmt_blks[i].list = RENEW_N( Int_2tuple, fmt_blks[i].list,
                                            cur_blk, 1, "Addl srec fmt blk" );
                p_i2t = (Int_2tuple *) fmt_blks[i].list;
                p_i2t[cur_blk][0] = range_first;
                p_i2t[cur_blk][1] = j - 1; /* "range_last" */
            }
        }
    }
}


/*****************************************************************
 * TAG( have_merged )
 *
 * Merge a list of states into a time series' valid states list.
 */
static Bool_type
have_merged( int srec_id, List_head *p_lh )
{
    int i;
    int *p_i;

    p_i = (int *) p_lh->list;

    for ( i = 0; i < p_lh->qty; i++ )
        if ( p_i[i] == srec_id )
            return TRUE;

    return FALSE;
}


/*****************************************************************
 * TAG( add_valid_states )
 *
 * Merge a list of states into a time series' valid states list.
 *
 * This really sucks - maybe a library API to deal with the basic
 * operations used below would simplify.
 */
static void
add_valid_states( Time_series_obj *p_tso, List_head *p_lh, int first_st,
                  int last_st, int srec_id )
{
    int i, j, overlap, new_idx;
    int new_st_qty, new_blk_qty, more_new, old_last;
    int mergers, last;
    int *p_i;
    Int_2tuple *new_blks, *old_blks, *p_i2t;

    /*
     * Allocate/extend data array space for states [0, last_st] to avoid
     * having to insert states later if first_st is not state 0 now
     * (but is later).
     */
    if ( last_st + 1 > p_tso->qty_states )
    {
        new_st_qty = last_st + 1 - p_tso->qty_states;
        p_tso->data = RENEWC_N( float, p_tso->data, p_tso->qty_states,
                                new_st_qty, "Extend time series data" );
        p_tso->qty_states += new_st_qty;
    }

    if ( p_tso->state_blocks == NULL )
    {
        /* First merger - just duplicate valid states list. */
        p_i2t = NEW_N( Int_2tuple, p_lh->qty, "Valid states list" );
        new_blks = (Int_2tuple *) p_lh->list;
        for ( i = 0; i < p_lh->qty; i++ )
        {
            p_i2t[i][0] = new_blks[i][0];
            p_i2t[i][1] = new_blks[i][1];
        }
        p_tso->state_blocks = p_i2t;
        p_tso->qty_blocks = p_lh->qty;

        p_i = NEW( int, "Series merged formats list" );
        p_i[0] = srec_id;
        p_tso->merged_formats.list = p_i;
        p_tso->merged_formats.qty = 1;
    }
    else if ( have_merged( srec_id, &p_tso->merged_formats ) )
    {
        /*
         * This format already accounted for in series; only need to
         * check for possible new states at beginning or end of series.
         */

        /* Check for new states at beginning of series. */
        if ( first_st < p_tso->min_eval_st
                && ((Int_2tuple *) p_lh->list)[0][0] < p_tso->state_blocks[0][0] )
        {
            new_blks = (Int_2tuple *) p_lh->list;
            old_blks = p_tso->state_blocks;

            /*
             * Loop through new blocks to count ones disjoint from old
             * blocks and find which new block overlaps first old block.
             */
            new_blk_qty = 0;
            for ( i = 0; i < p_lh->qty; i++ )
            {
                if ( new_blks[i][1] >= old_blks[0][0] )
                    break;
                else
                    new_blk_qty++;
            }
            overlap = i;

            /* Any new states in overlapping block? */
            more_new = old_blks[0][0] - new_blks[i][0];

            /* Adjust overlapping block def. */
            if ( more_new )
                old_blks[0][0] = new_blks[overlap][0];

            /* Extend blocks array, move old blocks, add new ones. */
            if ( new_blk_qty )
            {
                old_blks = RENEW_N( Int_2tuple, old_blks, p_tso->qty_blocks,
                                    new_blk_qty, "Added series st blocks" );
                p_tso->state_blocks = old_blks;

                /* Move old blocks. */
                for ( i = p_tso->qty_blocks - 1; i >= 0; i-- )
                {
                    old_blks[i + new_blk_qty][0] = old_blks[i][0];
                    old_blks[i + new_blk_qty][1] = old_blks[i][1];
                }

                /* Copy in new blocks. */
                for ( i = 0; i < overlap; i++ )
                {
                    old_blks[i][0] = new_blks[i][0];
                    old_blks[i][1] = new_blks[i][1];
                }
            }

            p_tso->qty_blocks += new_blk_qty;
        }

        /* Check for new states at end of series. */
        if ( last_st > p_tso->max_eval_st
                && ((Int_2tuple *) p_lh->list)[p_lh->qty - 1][1]
                > p_tso->state_blocks[p_tso->qty_blocks - 1][1] )
        {
            new_blks = (Int_2tuple *) p_lh->list;
            old_blks = p_tso->state_blocks;

            /*
             * Loop through new blocks to count ones disjoint from old
             * blocks and find which new block overlaps last old block.
             * Count new states so data array can be extended.
             */
            new_blk_qty = 0;
            old_last = p_tso->qty_blocks - 1;
            for ( i = p_lh->qty - 1; i >= 0; i-- )
            {
                if ( new_blks[i][0] <= old_blks[old_last][1] )
                    break;
                else
                    new_blk_qty++;
            }
            overlap = i;

            /* Any new states in overlapping block? */
            more_new = new_blks[overlap][1] - old_blks[old_last][1];

            /* Adjust overlapping block def. */
            if ( more_new )
                old_blks[old_last][1] = new_blks[overlap][1];

            /* Extend blocks array, copy new ones. */
            if ( new_blk_qty )
            {
                old_blks = RENEW_N( Int_2tuple, old_blks, p_tso->qty_blocks,
                                    new_blk_qty, "Added series st blocks" );
                p_tso->state_blocks = old_blks;

                /* Copy in new blocks. */
                for ( i = overlap + 1, j = old_last + 1; i < p_lh->qty;
                        i++, j++ )
                {
                    old_blks[j][0] = new_blks[i][0];
                    old_blks[j][1] = new_blks[i][1];
                }
            }

            p_tso->qty_blocks += new_blk_qty;
        }
    }
    else
    {
        /*
         * This is a new srec format for this time series.  States in
         * new format's blocks will be disjoint with states in existing
         * blocks in series, and the new blocks will be non-overlapping
         * but possibly adjacent to existing blocks (and so possibly
         * can be merged into existing blocks).
         */

        new_blks = (Int_2tuple *) p_lh->list;
        old_blks = p_tso->state_blocks;

        new_idx = 0; /* Index into new blocks. */
        for ( i = 0; i < p_tso->qty_blocks; i++ )
        {
            /*
             * Process new format blocks until we find one that is
             * non-adjacent and later than the current old block.
             */
            while ( new_idx < p_lh->qty
                    && new_blks[new_idx][0] <= old_blks[i][1] + 1 )
            {
                if ( new_blks[new_idx][1] < old_blks[i][0] - 1 )
                {
                    /* Preceding, non-adjacent -> add block */

                    old_blks = RENEW_N( Int_2tuple, old_blks, p_tso->qty_blocks,
                                        1, "New addl series block" );

                    /* Move extant blocks up. */
                    for ( j = p_tso->qty_blocks; j > 0; j-- )
                    {
                        old_blks[j][0] = old_blks[j - 1][0];
                        old_blks[j][1] = old_blks[j - 1][1];
                    }
                    /* Add new block. */
                    old_blks[0][0] = new_blks[new_idx][0];
                    old_blks[0][1] = new_blks[new_idx][1];

                    p_tso->state_blocks = old_blks;
                    p_tso->qty_blocks++;
                }
                else if ( new_blks[new_idx][1] == old_blks[i][0] - 1 )
                {
                    /* Preceding, adjacent -> merge block */

                    old_blks[i][0] = new_blks[new_idx][0];
                }
                else if ( new_blks[new_idx][0] == old_blks[i][1] + 1 )
                {
                    /* Following, adjacent -> merge block */

                    old_blks[i][1] = new_blks[new_idx][1];
                }

                new_idx++;
            }
        }

        /* If there are any new blocks left, they all get added. */
        for ( ; new_idx < p_lh->qty; new_idx++ )
        {
            /* Extend the blocks array and add the new block. */
            old_blks = RENEW_N( Int_2tuple, old_blks, p_tso->qty_blocks, 1,
                                "New appended series block" );
            old_blks[i][0] = new_blks[new_idx][0];
            old_blks[i][1] = new_blks[new_idx][1];
            p_tso->state_blocks = old_blks;
            p_tso->qty_blocks++;
            i++;
        }

        /*
         * All new blocks have been added or merged.  Make a last pass
         * through the "old" blocks as merges may be possible between
         * blocks that are now adjacent.
         */
        mergers = 0;
        i = 0;
        last = p_tso->qty_blocks - 1;
        while ( i < last )
        {
            if ( old_blks[i][1] == old_blks[i + 1][0] - 1 )
            {
                /* Merge next block into current. */
                old_blks[i][1] = old_blks[i + 1][1];

                /* Move subsequent blocks down. */
                for ( j = i + 1; j < p_tso->qty_blocks - 1; j++ )
                {
                    old_blks[j][0] = old_blks[j + 1][0];
                    old_blks[j][1] = old_blks[j + 1][1];
                }

                /* Count unused block entries. */
                mergers++;

                last--;
            }
            else
                i++;
        }

        if ( mergers > 0 )
        {
            /* Give up unused space. */
            old_blks = RENEW_N( Int_2tuple, old_blks, p_tso->qty_blocks,
                                -mergers, "Free unused series blocks" );
            p_tso->state_blocks = old_blks;
            p_tso->qty_blocks -= mergers;
        }

        /* Register the new format as merged into the time series. */
        p_i = (int *) p_tso->merged_formats.list;
        p_i = RENEW_N( int, p_i, p_tso->merged_formats.qty, 1,
                       "New registered srec format in time series" );
        p_i[p_tso->merged_formats.qty] = srec_id;
        p_tso->merged_formats.list = p_i;
        p_tso->merged_formats.qty++;
    }
}


/*****************************************************************
 * TAG( add_series_map_entry )
 *
 * Add a subrec to a time series' srec map.
 */
static void
add_series_map_entry( int subrec_id, int srec_id, Time_series_obj *p_tso )
{
    List_head *p_lh;
    int i;
    int *p_i;

    p_lh = ((List_head *) p_tso->series_map.list) + srec_id;

    p_i = (int *) p_lh->list;
    for ( i = 0; i < p_lh->qty; i++ )
        if ( p_i[i] == subrec_id )
            break;

    if ( i == p_lh->qty )
    {
        /* Subrec_id not matched; add it. */
        p_i = RENEW_N( int, p_i, p_lh->qty, 1, "New series map subrec entry" );
        p_i[p_lh->qty] = subrec_id;
        p_lh->qty++;
        p_lh->list = p_i;
    }
}


/*****************************************************************
 * TAG( update_srec_fmt_blocks )
 *
 * Update the saved state rec format blocks if necessary.
 */
static void
update_srec_fmt_blocks( int min_state, int max_state, Analysis *analy )
{
    int *srec_fmts;
    List_head *fmt_blks;
    Int_2tuple iarray;
    int i;

    if ( analy->first_state != min_state || analy->last_state != max_state
            || analy->srec_formats == NULL )
    {
        if ( analy->srec_formats != NULL )
            free( analy->srec_formats );

        srec_fmts = NEW_N( int, max_state + 1, "Srec formats array" );
        analy->srec_formats = srec_fmts;
        iarray[0] = 1; /* Get state formats from beginning of database. */
        iarray[1] = max_state + 1;
        analy->db_query( analy->db_ident, QRY_SERIES_SREC_FMTS, (void *) iarray,
                         NULL, (void *) srec_fmts );

        analy->first_state = min_state;
        analy->last_state = max_state;

        /* Build block lists indicating which states utilize each format. */
        if ( analy->format_blocks != NULL )
        {
            for ( i = 0; i < analy->qty_srec_fmts; i++ )
                free( analy->format_blocks[i].list );
            free( analy->format_blocks );
        }
        fmt_blks = NEW_N( List_head, analy->qty_srec_fmts, "Fmt blocks array" );

        gen_srec_fmt_blocks( analy->qty_srec_fmts, srec_fmts, min_state,
                             max_state, fmt_blks );

        analy->format_blocks = fmt_blks;
    }
}


/*****************************************************************
 * TAG( gen_gather )
 *
 * Create a list of Time_series_obj's in preparation for a
 * data gather by evaluating all combinations of results and mesh
 * objects and selecting those which are valid and supported on
 * any subrecord.  If an existing Time_series_obj for which data
 * has already been gathered matches the current request, unlink
 * it from the storage list and put it in the gather list so it
 * can be updated if the current min or max state varies from
 * what is was gathered with.
 *
 * Returns the quantity of valid object/result combinations
 * identified (whether already gathered or not).
 */
static int
gen_gather( Result *res_list, Specified_obj *so_list, Analysis *analy,
            Time_series_obj **p_gather_list )
{
    Result *p_r;
    Specified_obj *p_so;
    List_head *srec_map;
    Subrec_obj *p_subrec;
    State_rec_obj *p_state_rec;
    Time_series_obj *p_tso, *tso_list;
    int i, j;
    int maxst, minst;

    Bool_type derived;
    int subrec_id;
    Subrecord_result *sr_array;
    List_head *fmt_blks;
    int valid_combinations;

    maxst = get_max_state( analy );
    minst = GET_MIN_STATE( analy );

    tso_list = NULL;
    valid_combinations = 0;

    update_srec_fmt_blocks( minst, maxst, analy );
    fmt_blks = analy->format_blocks;

    /* Loop over requested results. */
    for ( p_r = res_list; p_r != NULL; NEXT( p_r ) )
    {
        derived = p_r->origin.is_derived;

        /* Get the srec map for the current result. */
        srec_map = ( derived )
                   ? ((Derived_result *) p_r->original_result)->srec_map
                   : ((Primal_result *) p_r->original_result)->srec_map;

        /* Loop over state record formats. */
        sr_array = NULL;
        for ( i = 0; i < analy->qty_srec_fmts; i++ )
        {
            /* If result is not available on any subrecords, skip. */
            if ( srec_map[i].qty == 0 )
                continue;

            /* The current result is supported somewhere in this format. */

            p_state_rec = analy->srec_tree + i;

            if ( derived )
                sr_array = (Subrecord_result *) srec_map[i].list;

            /* Loop to check validity of result/object combinations. */
            for ( p_so = so_list; p_so != NULL; NEXT( p_so ) )
            {
                /* Loop over the subrecords where the result exists. */
                for ( j = 0; j < srec_map[i].qty; j++ )
                {
                    subrec_id = GET_SUBREC_ID( p_r, srec_map + i, j );
                    p_subrec = p_state_rec->subrecs + subrec_id;

                    /*
                     * IF
                     *  result is indirect on the subrecord and
                     *  the candidate superclass matches the object's
                     *  superclass
                     * OR
                     *  the result is primal and
                     *  the object is bound to the subrecord
                     * OR
                     *  the result is direct on the subrecord and
                     *  the object is bound to the subrecord
                     * THEN
                     *  the result/mesh object combination can generate
                     *  a time history on this subrecord.
                     */
                    if ( ( derived && sr_array[j].indirect
                            && sr_array[j].candidate->superclass
                            == p_so->mo_class->superclass )
                            || ( ( !derived || !sr_array[j].indirect )
                                 && object_is_bound( p_so->ident + 1,
                                                     p_so->mo_class, p_subrec ) ) )
                    {
                        /*
                         * A time series can be obtained for this combination
                         * of result and mesh object.
                         */

                        if ( find_time_series( p_r, p_so->ident, p_so->mo_class,
                                               analy, analy->time_series_list,
                                               &p_tso ) )
                        {
                            /*
                             * Time series already exists, so check to see
                             * if it has been evaluated at all states of
                             * current request.
                             */

                            valid_combinations++;

                            if ( p_tso->min_eval_st > minst
                                    || p_tso->max_eval_st < maxst )
                            {
                                /* Requested bounds exceed extant bounds. */
                                UNLINK( p_tso, analy->time_series_list );
                                INSERT( p_tso, tso_list );
                            }
                            else
                                /* Extant series will do. */
                                continue;
                        }
                        /* Ensure this combination has not already occurred. */
                        else if ( !find_time_series( p_r, p_so->ident,
                                                     p_so->mo_class, analy,
                                                     tso_list, &p_tso ) )
                        {
                            /* Create new time series object. */

                            valid_combinations++;

                            p_tso = new_time_series_obj( p_r, p_so->ident,
                                                         p_so->mo_class,
                                                         NULL, NULL,
                                                         analy->qty_srec_fmts );
                            INSERT( p_tso, tso_list );
                        }

                        p_state_rec->series_qty++;

                        /* Merge current srec's states into time series. */
                        add_valid_states( p_tso, fmt_blks + i, minst, maxst,
                                          i );

                        /*
                         * Build up the time series' srec map (which may be a
                         * subset of the result's srec map).  Later, the series
                         * can be updated without recreating the above logic.
                         */
                        add_series_map_entry( subrec_id, i, p_tso );

                        /* Prep to gather the data. */
                        add_gather_tree_entry( p_subrec, p_tso );
                    } /* if ( time series possible ) */
                } /* for ( valid subrecords ) */
            } /* for ( specified objects ) */
        } /* for ( all state record formats ) */
    } /* for ( specified results ) */

    *p_gather_list = tso_list;

    return valid_combinations;
}


/*****************************************************************
 * TAG( add_gather_tree_entry )
 *
 * Insert an entry for into a subrecords gather tree for a
 * result/mesh object combination.
 */
static void
add_gather_tree_entry( Subrec_obj *p_subrec, Time_series_obj *p_tso )
{
    Result_mo_list_obj *p_rmlo;
    Series_ref_obj *p_sro;

    for ( p_rmlo = p_subrec->gather_tree; p_rmlo != NULL; NEXT( p_rmlo ) )
        if ( p_rmlo->result == p_tso->result )
            break;

    if ( p_rmlo == NULL )
    {
        /* First occurrence of this result. */
        p_rmlo = NEW( Result_mo_list_obj, "New result/mo list" );
        INSERT( p_rmlo, p_subrec->gather_tree );
        p_rmlo->result = p_tso->result;
    }

    p_sro = NEW( Series_ref_obj, "Series object reference" );
    p_sro->series = p_tso;
    INSERT( p_sro,  p_rmlo->mo_list );
}


/*****************************************************************
 * TAG( find_global_time_series )
 *
 * Search list of time series objects and attempt to match a
 * candidate. If "pp_old_tso" is not NULL, use it to pass
 * back a pointer to a found time series object.
 */
static Bool_type
find_global_time_series( Result *p_result, Analysis *analy,
                         Time_series_obj **pp_old_tso )
{
    Time_series_obj *p_tso;

    for ( p_tso = analy->time_series_list; p_tso != NULL; NEXT( p_tso ) )
    {
        if ( match_series_results( p_result, analy, p_tso )
                && p_tso->mo_class->superclass == G_MESH )
        {
            if ( pp_old_tso != NULL )
                *pp_old_tso = p_tso;

            return TRUE;
        }
    }

    return FALSE;
}


/*****************************************************************
 * TAG( find_time_series )
 *
 * Search list of time series objects and attempt to match a
 * candidate. If "pp_old_tso" is not NULL, use it to pass
 * back a pointer to a found time series object.
 */
static Bool_type
find_time_series( Result *p_result, int ident, MO_class_data *p_mo_class,
                  Analysis *analy, Time_series_obj *series_list,
                  Time_series_obj **pp_old_tso )
{
    Time_series_obj *p_tso;

    for ( p_tso = series_list; p_tso != NULL; NEXT( p_tso ) )
    {
        if ( match_series_results( p_result, analy, p_tso )
                && p_tso->ident == ident
                && p_tso->mo_class == p_mo_class )
        {
            if ( pp_old_tso != NULL )
                *pp_old_tso = p_tso;

            return TRUE;
        }
    }

    return FALSE;
}


/*****************************************************************
 * TAG( find_result_in_list )
 *
 * Search list of Results and attempt to match a candidate.
 */
static Bool_type
find_result_in_list( Result *p_candidate, Result *result_list,
                     Result **pp_found_result )
{
    Result *p_r;

    if ( p_candidate == NULL )
        return FALSE;

    for ( p_r = result_list; p_r != NULL; NEXT( p_r ) )
    {
        /*
         * Name test differentiates sub-component specifications of
         * array or vector results and original result test
         * differentiates a primal from a derived if both exist.
         */
        if ( p_r->original_result == p_candidate->original_result
                && strcmp( p_r->name, p_candidate->name ) == 0 )
        {
            if ( pp_found_result != NULL )
                *pp_found_result = p_r;
            return TRUE;
        }
    }

    return FALSE;
}


/*****************************************************************
 * TAG( find_named_result_in_list )
 *
 * Search list of Results and attempt to match a candidate by
 * name only.
 *
 * NOTE - This is no longer matching just the name but also the
 * current analy settings which reflect what is expected of the
 * Result.
 */
static Bool_type
find_named_result_in_list( Analysis *analy, char *p_candidate,
                           Result *result_list, Result **pp_found_result )
{
    Result *p_r;

    if ( p_candidate == NULL )
        return FALSE;

    for ( p_r = result_list; p_r != NULL; NEXT( p_r ) )
    {
        if ( strcmp( p_r->name, p_candidate ) == 0
                && match_result_source_with_analy( analy, p_r )
                && match_spec_with_analy( analy, &p_r->modifiers ) )
        {
            if ( pp_found_result != NULL )
                *pp_found_result = p_r;
            return TRUE;
        }
    }

    return FALSE;
}


/*****************************************************************
 * TAG( match_result_source_with_analy )
 *
 * Compare the result source for a Result with the current
 * settings in an Analysis struct.
 */
static Bool_type
match_result_source_with_analy( Analysis *analy, Result *p_res )
{
    if ( analy->result_source == ALL )
        return TRUE;
    else if ( analy->result_source == DERIVED
              && p_res->origin.is_derived )
        return TRUE;
    else if ( analy->result_source == PRIMAL
              && p_res->origin.is_primal )
        return TRUE;
    else
        return FALSE;
}


/*****************************************************************
 * TAG( match_series_results )
 *
 * Determine if a result and current result modifiers state,
 * if applicable, match that for a stored time series.
 *
 * Assumes that the given result does/will have modifiers
 * equal to the current Analysis settings, but does not check
 * the given result to obtain these values to allow for this
 * test to happen before the result has been derived.
 */
static Bool_type
match_series_results( Result *p_result, Analysis *analy,
                      Time_series_obj *p_test )
{
    if ( p_result->original_result == p_test->result->original_result
            && strcmp( p_result->name, p_test->result->name ) == 0 )
    {
        return match_spec_with_analy( analy, &p_test->result->modifiers );
    }

    return FALSE;
}


/*****************************************************************
 * TAG( match_spec_with_analy )
 *
 * Compare the current values in a Result_spec with the current
 * settings in an Analysis struct.
 */
static Bool_type
match_spec_with_analy( Analysis *analy, Result_spec *p_spec )
{
    /* Check dependence on reference surface. */
    if ( p_spec->use_flags.use_ref_surface )
        if ( analy->ref_surf != p_spec->ref_surf )
            return FALSE;

    /* Check dependence on reference frame. */
    if ( p_spec->use_flags.use_ref_frame )
        if ( analy->ref_frame != p_spec->ref_frame )
            return FALSE;

    /* Check dependence on strain variety. */
    if ( p_spec->use_flags.use_strain_variety )
        if ( analy->strain_variety
                != p_spec->strain_variety )
            return FALSE;

    /* Check dependence on time derivative. */
    if ( p_spec->use_flags.time_derivative )
        if ( analy->strain_variety != RATE )
            return FALSE;

    /*
     * Check dependence on global coordinate transformation.
     *
     * This is different than the other Result_spec fields in that its
     * use is determined dynamically by the user and not statically by the
     * derivation (don't confuse "use" with "value").  We require it
     * to be set as soon as the Result is created and cannot wait until
     * the derivation has occurred, therefore it is set from the current
     * value of analy->do_tensor_transform at creation (see find_result()
     * and duplicate_result()).  This should be OK as long as the logic
     * sequence from creation to utilization is atomic with respect to
     * anything the user can do.  Another aspect that is different is
     * that we don't store both whether or not the transform is used
     * _plus_ the transform's current value (the transformation matrix
     * can have an essentially infinite number of values whereas the other
     * Result_spec fields have a small, finite number of possible values).
     * Perhaps that would be a better way to go and just live with the
     * unwieldy-ness of having to store and compare matrices per Result,
     * but for now we're making it work without doing that.
     */
    if ( analy->do_tensor_transform && analy->ref_frame == GLOBAL )
    {
        if ( !p_spec->use_flags.coord_transform )
            return FALSE;
    }
    else if ( p_spec->use_flags.coord_transform )
        return FALSE;

    /* Check dependence on reference state. */
    if ( p_spec->use_flags.use_ref_state )
        if ( analy->reference_state != p_spec->ref_state )
            return FALSE;

    return TRUE;
}


/*****************************************************************
 * TAG( clear_plot_list )
 *
 * Free current plot objects and their references.
 */
static void
clear_plot_list( Plot_obj **p_plot_list )
{
    Plot_obj *p_po;

    p_po = *p_plot_list;

    if ( p_po != NULL )
    {
        while ( p_po->next != NULL )
        {
            NEXT( p_po );
            p_po->prev->ordinate->reference_count--;
            p_po->prev->abscissa->reference_count--;
            free( p_po->prev );
        }
        p_po->ordinate->reference_count--;
        p_po->abscissa->reference_count--;
        free( p_po );

        *p_plot_list = NULL;
    }
}


/*****************************************************************
 * TAG( copy_obj_list )
 *
 * Duplicate a list of Specified_obj structs.
 */
static Specified_obj *
copy_obj_list( Specified_obj *src_list )
{
    Specified_obj *p_tmp, *tmp_list, *p_so, *p_dio;

    tmp_list = NULL;
    p_dio = NULL;

    for ( p_so = src_list; p_so != NULL; p_so = p_so->next )
    {
        p_dio = NEW( Specified_obj, "Copy MO" );
        p_dio->ident = p_so->ident;
        p_dio->label = p_so->label;
        p_dio->mo_class = p_so->mo_class;
        INSERT( p_dio, tmp_list );
    }

    p_so = tmp_list;
    while ( p_so != NULL )
    {
        p_dio = p_so;
        p_so = p_so->next;
        p_tmp = p_dio->next;
        p_dio->next = p_dio->prev;
        p_dio->prev = p_tmp;
    }

    return p_dio;
}


/*****************************************************************
 * TAG( add_mo_nodes )
 *
 * Allocate and list-insert a set of continuously numbered Mesh_obj's.
 */
static void
add_mo_nodes( Specified_obj **list, MO_class_data *p_class, int start_ident,
              int stop_ident )
{
    int i;
    int ident; 
    Specified_obj *p_so;

    for ( i = start_ident; i <= stop_ident; i++ )
    {
        
        if(!p_class->labels_found)
        {
            ident = i-1;
        } else
        {
            ident = get_class_label_index(p_class, i);
        }
        if(ident == M_INVALID_LABEL)
        {
            continue;
        }
        p_so = NEW( Specified_obj , "Parsed MO" );
        p_so->ident = ident;
        p_so->label = i;
        p_so->mo_class = p_class;
        APPEND( p_so, *list );
    }
}


/*****************************************************************
 * TAG( extract_numeric_str )
 *
 * Extract the first contiguous list of numeric characters from a string
 * and copy it into an array. Return FALSE if no numeric characters found.
 */
static Bool_type
extract_numeric_str( char *src_string, char *dest_string )
{
    char *p_csrc, *p_cdest;

    p_csrc = strpbrk( src_string, "0123456789" );
    if ( p_csrc )
    {
        for ( p_cdest = dest_string; isdigit( (int) *p_csrc );
                *p_cdest++ = *p_csrc++ );

        *p_cdest = '\0';
    }

    return ( p_csrc != NULL ) ? TRUE : FALSE;
}


/*****************************************************************
 * TAG( get_token_type )
 *
 * Identify command-line tokens belonging to mesh object lists.
 */
static MO_list_token_type
get_token_type( char *token, Analysis *analy, MO_class_data **class )
{
    MO_list_token_type rval;
    Htable_entry *p_hte;

    if ( strspn( token, "0123456789" ) == strlen( token ) )
        rval = NUMERIC;
    else if (  htable_search( MESH( analy ).class_table, token, FIND_ENTRY,
                              &p_hte )
               == OK )
    {
        rval = MESH_OBJ_CLASS;
        *class = (MO_class_data *) p_hte->data;
    }
    else if ( strcmp( token, "-" ) == 0 )
        rval = RANGE_SEPARATOR;
    else if ( strchr( token, (int) '-' ) )
        rval = COMPOUND_TOKEN;
    else
        rval = OTHER_TOKEN;

    return rval;
}


#ifdef OLDMATTH
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

    num_states = get_max_state( analy ) + 1;
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
        load_result( analy, FALSE, FALSE, FALSE );

        for ( j = 0; j < num_nodes; j++ )
            for ( k = 0; k < mat_cnt; k++ )
                if ( mark_nodes[k][j] == 1.0 )
                    t_result[k*num_states+i] += result[j];
    }
    analy->cur_state = cur_state;
    analy->state_p = get_state( cur_state, analy->state_p );
    load_result( analy, FALSE, FALSE, FALSE );

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
#endif


/*****************************************************************
 * TAG( get_interval_min_max )
 *
 * Find the min or max values of a time series over a continuous
 * interval.
 */
static void
get_interval_min_max( float *data, int first_st, int last_st, int which,
                      float *p_min, float *p_max )
{
    int i;
    float minv, maxv;

    switch ( which )
    {
    case 1:
        /* Test for min. */
        minv = *p_min;
        for ( i = first_st; i <= last_st; i++ )
            if ( data[i] < minv )
                minv = data[i];
        *p_min = minv;
        break;
    case 2:
        /* Test for max. */
        maxv = *p_max;
        for ( i = first_st; i <= last_st; i++ )
            if ( data[i] > maxv )
                maxv = data[i];
        *p_max = maxv;
        break;
    case 3:
        /* Test for both. */
        minv = *p_min;
        maxv = *p_max;
        for ( i = first_st; i <= last_st; i++ )
        {
            if ( data[i] < minv )
                minv = data[i];
            if ( data[i] > maxv )
                maxv = data[i];
        }
        *p_min = minv;
        *p_max = maxv;
        break;
    }
}


/*****************************************************************
 * TAG( get_bounded_series_min_max )
 *
 * Find the min/max values of a time series over an interval.
 */
static void
get_bounded_series_min_max( Time_series_obj *p_tso, int first_st, int last_st,
                            float *p_bounded_min, float *p_bounded_max )
{
    int get_extrema;
    int i, j, k;
    int first_valid_st, last_valid_st;
    float *data;
    float minv, maxv;
    Int_2tuple *blocks;

    /* Start by assigning from overall series extrema. */
    *p_bounded_min = p_tso->min_val;
    *p_bounded_max = p_tso->max_val;

    get_extrema = 0;

    if ( p_tso->min_val_state < first_st || last_st < p_tso->min_val_state )
        get_extrema += 1;

    if ( p_tso->max_val_state < first_st || last_st < p_tso->max_val_state )
        get_extrema += 2;

    /* If both series extrema fall on the interval, we're done. */
    if ( get_extrema == 0 )
        return;

    /* Must search interval for one or both extrema; get starting block. */
    blocks = p_tso->state_blocks;
    for ( i = 0; i < p_tso->qty_blocks; i++ )
    {
        if ( first_st >= blocks[i][0] )
        {
            if ( first_st <= blocks[i][1] )
            {
                /* Interval start falls within a block. */
                first_valid_st = first_st;
                break;
            }
            else if ( i + 1 < p_tso->qty_blocks && first_st < blocks[i + 1][0] )
            {
                /* Interval start falls between blocks. */
                i++;
                first_valid_st = blocks[i][0];
                break;
            }
        }
    }

    /* Get ending block. */
    for ( j = i; j < p_tso->qty_blocks; j++ )
    {
        if ( last_st <= blocks[j][1] )
        {
            if ( last_st >= blocks[j][0] )
            {
                /* Interval end falls within a block. */
                last_valid_st = last_st;
                break;
            }
            else
            {
                /* Interval end falls between blocks. */
                j--;
                last_valid_st = blocks[j][1];
                break;
            }
        }
    }

    if ( j == p_tso->qty_blocks )
    {
        /* This should never happen. */
        popup_dialog( WARNING_POPUP,
                      "Time series does not contain plot interval." );
        return;
    }

    /* Init's for search. */
    data = p_tso->data;
    minv = maxv = data[first_valid_st];

    switch ( j - i )
    {
    case 0:
        /* Entire interval in same block. */
        get_interval_min_max( data, first_valid_st, last_valid_st,
                              get_extrema, &minv, &maxv );

        break;

    case 1:
        /* Interval spans two adjacent blocks. */
        get_interval_min_max( data, first_valid_st, blocks[i][1],
                              get_extrema, &minv, &maxv );

        get_interval_min_max( data, blocks[j][0], last_valid_st,
                              get_extrema, &minv, &maxv );

        break;

    default:
        /* Interval spans three or more blocks. */
        get_interval_min_max( data, first_valid_st, blocks[i][1],
                              get_extrema, &minv, &maxv );

        for ( k = i + 1; k < j; k++ )
            get_interval_min_max( data, blocks[k][0], blocks[k][1],
                                  get_extrema, &minv, &maxv );

        get_interval_min_max( data, blocks[j][0], last_valid_st,
                              get_extrema, &minv, &maxv );

        break;
    }

    switch ( get_extrema )
    {
    case 1:
        *p_bounded_min = minv;
        break;
    case 2:
        *p_bounded_max = maxv;
        break;
    case 3:
        *p_bounded_min = minv;
        *p_bounded_max = maxv;
        break;
    }
}


/*****************************************************************
 * TAG( set_glyphs_per_plot )
 *
 * Set the desired quantity of glyphs per plot.
 */
void
set_glyphs_per_plot( int glyph_qty, Analysis *analy )
{
    if ( glyph_qty < 1 )
        analy->show_plot_glyphs = FALSE;
    else
    {
        analy->show_plot_glyphs = TRUE;
        glyphs_per_plot = glyph_qty;
    }
}


/*****************************************************************
 * TAG( set_glyph_scale )
 *
 * Set the scale factor applied to glyph geometry for rendering.
 */
void
set_glyph_scale( float new_scale )
{
    glyph_scale = new_scale;
}


/*****************************************************************
 * TAG( set_glyph_alignment )
 *
 * Set the alignment mode for glyphs on (multiple) plots.
 */
void
set_glyph_alignment( Glyph_alignment_type ga_type )
{
    switch ( ga_type )
    {
    case GLYPH_ALIGN:
        init_glyph_eval = init_aligned_glyph_func;
        break;
    case GLYPH_STAGGERED:
        init_glyph_eval = init_staggered_glyph_func;
        break;
    }
}


/*****************************************************************
 * TAG( init_aligned_glyph_func )
 *
 * Initialize the Plot_glyph_data structs to generate vertically
 * aligned glyphs using the thresholded_glyph_func().
 */
static void
init_aligned_glyph_func( int desired_glyph_qty, int qty_plots,
                         float plot_x_span, Analysis *analy,
                         Plot_glyph_data *gd_array )
{
    int i;
    float time_span, interval, init_thresh;
    float *times;

    times = analy->times->data;
    init_thresh = times[analy->first_state];
    time_span = times[analy->last_state] - init_thresh;

    switch ( desired_glyph_qty )
    {
    case 1:
        interval = 1.001 * time_span;
        break;
    case 2:
        interval = 0.999 * time_span;
        break;
    default:
        interval = time_span / (float) (desired_glyph_qty - 1);
    }

    for ( i = 0; i < qty_plots; i++ )
    {
        gd_array[i].glyph_type = (Plot_glyph_type) (i % QTY_GLYPH_TYPES);
        gd_array[i].interval = interval;
        gd_array[i].next_threshold = init_thresh;
    }

    /* Init pointer to the actual evaluation function. */
    need_glyph_now = thresholded_glyph_func;
}


/*****************************************************************
 * TAG( thresholded_glyph_func  )
 *
 * Evaluate glyph-rendering need by comparing a data value to a
 * stored threshold value.
 */
static Bool_type
thresholded_glyph_func( float val, Plot_glyph_data *p_gd )
{
    if ( val < p_gd->next_threshold )
        return FALSE;

    /* Update the threshold. */
    p_gd->next_threshold += p_gd->interval;

    return TRUE;
}


/*****************************************************************
 * TAG( init_staggered_glyph_func )
 *
 * Initialize the Plot_glyph_data structs to create glyphs which
 * are staggered in x using the thresholded_glyph_func().
 *
 * Assumes init_glyph_draw() has been called to scale glyph geometry
 * for current window and glyph scale factor.
 */
static void
init_staggered_glyph_func( int desired_glyph_qty, int qty_plots,
                           float plot_x_span, Analysis *analy,
                           Plot_glyph_data *gd_array )
{
    float phase, dt, pd_ratio, time_span;
    float desired_interval, init_thresh;
    double ip;
    float *times;
    int i;

    times = analy->times->data;
    time_span = times[analy->last_state] - times[analy->first_state];

    /*
     * Scale the maximum width of a glyph (represented as the x distance
     * between the second and fourth vertices of the pentagon glyph) as
     * a time value to get the minimum phase.
     */
    phase = time_span
            * (pentagon_verts[1][0] - pentagon_verts[3][0]) / plot_x_span;

    /* Get representative delta time between states. */
    dt = MAX( times[1] - times[0], times[2] - times[1] );

    if ( desired_glyph_qty == 1 )
        desired_interval = 1.001 * time_span;
    else
        desired_interval = (time_span - qty_plots * phase)
                           / (float) (desired_glyph_qty - 1);

    /*
     * We prefer desired_interval >> phase >> dt, and if not we need to
     * do something reasonable.
     *
     * Make the phase a whole multiple of dt.  It may wind up much larger
     * than necessary to avoid overlapping glyphs, but we'll be assured (?)
     * of separation.  In the desired case of phase >> dt, rounding up will
     * have little effect.
     */
    pd_ratio = phase / dt;
    if ( modf( (double) pd_ratio, &ip ) > 0.0 )
        phase = (ip + 1.0) * dt;

    /* Initialize the glyph data. */
    init_thresh = times[analy->first_state];
    for ( i = 0; i < qty_plots; i++ )
    {
        gd_array[i].glyph_type = (Plot_glyph_type) (i % QTY_GLYPH_TYPES);
        gd_array[i].interval = desired_interval;
        gd_array[i].next_threshold = init_thresh + i * phase;
    }

    /* Init pointer to the actual evaluation function. */
    need_glyph_now = thresholded_glyph_func;
}


/*****************************************************************
 * TAG( init_glyph_draw )
 *
 * Initialize glyph geometry for later rendering.
 */
static void
init_glyph_draw( float vp_to_world_x, float vp_to_world_y, float cur_scale )
{
    int i, j;
    static float v2w[2] = { 0.0, 0.0 };
    static float scale = 0.0;

    /* Vertices specified for GL_LINE_LOOP rendering. */
    static GVec3D circle_geom[] =
    {
        { 15.0, 0.0, 0.0 }, { 13.858, 5.740, 0.0 }, { 10.607, 10.607, 0.0 },
        { 5.740, 13.858, 0.0 } , { 0.0, 15.0, 0.0}, { -5.740, 13.858, 0.0 },
        { -10.607, 10.607, 0.0 }, { -13.858, 5.740, 0.0 }, { -15.0, 0.0, 0.0 },
        { -13.858, -5.74, 0.0 }, { -10.607, -10.607, 0.0 },
        { -5.74, -13.858, 0.0 }, { 0.0, -15.0, 0.0 }, { 5.74, -13.858, 0.0 },
        { 10.607, -10.607, 0.0 }, { 13.858, -5.74, 0.0 }
    };

    /* Vertices specified for GL_LINE_LOOP rendering. */
    static GVec3D triangle_geom[] =
    {
        { 12.99, -7.5, 0.0 }, { 0.0, 15.0, 0.0 }, { -12.99, -7.5, 0.0 }
    };

    /* Vertices specified for GL_LINE_LOOP rendering. */
    static GVec3D square_geom[] =
    {
        { 10.607, -10.607, 0.0 }, { 10.607, 10.607, 0.0 },
        { -10.607, 10.607, 0.0 }, { -10.607, -10.607, 0.0 }
    };

    /* Vertices specified for GL_LINE_LOOP rendering. */
    static GVec3D pentagon_geom[] =
    {
        { 8.817, -12.135, 0.0 }, { 14.266, 4.635, 0.0 }, { 0.0, 15.0, 0.0 },
        { -14.266, 4.635, 0.0 }, { -8.817, -12.135, 0.0 }
    };

    /* Vertices specified for GL_LINE_LOOP rendering. */
    static GVec3D diamond_geom[] =
    {
        { 10.0, 0.0, 0.0 }, { 0.0, 15.0, 0.0 }, { -10.0, 0.0, 0.0 },
        { 0.0, -15.0, 0.0 }
    };

    /* Vertices specified for GL_LINE_LOOP rendering. */
    static GVec3D cross_geom[] =
    {
        { 15.0, -5.0, 0.0 }, { 15.0, 5.0, 0.0 }, { 5.0, 5.0, 0.0 },
        { 5.0, 15.0, 0.0 }, { -5.0, 15.0, 0.0 }, { -5.0, 5.0, 0.0 },
        { -15.0, 5.0, 0.0 }, { -15.0, -5.0, 0.0 }, { -5.0, -5.0, 0.0 },
        { -5.0, -15.0, 0.0 }, { 5.0, -15.0, 0.0 }, { 5.0, -5.0, 0.0 }
    };

    /* Vertices specified for GL_LINE_LOOP rendering. */
    static GVec3D hexagon_geom[] =
    {
        { 12.99, -7.5, 0.0 }, { 12.99, 7.5, 0.0 }, { 0.0, 15.0, 0.0 },
        { -12.99, 7.5, 0.0 }, { -12.99, -7.5, 0.0 }, { 0.0, -15.0, 0.0 }
    };

    /* Vertices specified for GL_LINES rendering. */
    static GVec3D splat_geom[] =
    {
        { 15.0, 0.0, 0.0 }, { -15.0, 0.0, 0.0 },
        { 10.607, 10.607, 0.0 }, { -10.607, -10.607, 0.0 },
        { 0.0, 15.0, 0.0 }, { 0.0, -15.0, 0.0 },
        { -10.607, 10.607, 0.0 }, { 10.607, -10.607, 0.0 }
    };

    /* Vertices specified for GL_LINE_LOOP rendering. */
    static GVec3D star_geom[] =
    {
        { 14.266, 4.635, 0.0 }, { 3.368, 4.635, 0.0 }, { 0.0, 15.0, 0.0 },
        { -3.368, 4.635, 0.0 }, { -14.266, 4.635, 0.0 }, { -5.449, -1.77, 0.0 },
        { -8.817, -12.135, 0.0 }, { 0.0, -5.73, 0.0 }, { 8.817, -12.135, 0.0 },
        { 5.449, -1.77, 0.0 }
    };

    /* Vertices specified for GL_LINES rendering. */
    static GVec3D x_geom[] =
    {
        { 10.0, -15.0, 0.0 }, { -10.0, 15.0, 0.0 },
        { 10.0, 15.0, 0.0 }, { -10.0, -15.0, 0.0 }
    };

    /* Vertices specified for GL_LINE_LOOP rendering. */
    static GVec3D moon_geom[] =
    {
        { 7.5, -15.0, 0.0 }, { 5.7, -13.0, 0.0 }, { 5.1, -11.0, 0.0 },
        { 4.6, -9.0, 0.0 }, { 4.4, -7.0, 0.0 }, { 4.25, -5.0, 0.0 },
        { 4.1, -3.0, 0.0 }, { 4.0, 0.0, 0.0 }, { 4.1, 3.0, 0.0 },
        { 4.25, 5.0, 0.0 }, { 4.4, 7.0, 0.0 }, { 4.6, 9.0, 0.0 },
        { 5.1, 11.0, 0.0 }, { 5.7, 13.0, 0.0 }, { 7.5, 15.0, 0.0 },
        { 4.5, 14.75, 0.0 }, { 1.5, 13.9, 0.0 }, { -0.5, 12.9, 0.0 },
        { -3.1, 11.0, 0.0 }, { -4.8, 9.0, 0.0 }, { -6.5, 6.0, 0.0 },
        { -7.4, 3.0, 0.0 }, { -7.5, 0.0, 0.0 }, { -7.4, -3.0, 0.0 },
        { -6.5, -6.0, 0.0 }, { -4.8, -9.0, 0.0 }, { -3.1, -11.0, 0.0 },
        { -0.5, -12.9, 0.0 }, { 1.5, -13.9, 0.0 }, { 4.5, -14.75, 0.0 },
    };

    /* Vertices specified for GL_LINES rendering. */
    static GVec3D plus_geom[] =
    {
        { 15.0, 0.0, 0.0 }, { -15.0, 0.0, 0.0 },
        { 0.0, 15.0, 0.0 }, { 0.0, -15.0, 0.0 }
    };

    /* Vertices specified for GL_LINE_LOOP rendering. */
    static GVec3D bullet_geom[] =
    {
        { 15.0, -15.0, 0.0 }, { 14.85, -11.0, 0.0 }, { 14.35, -7.0, 0.0 },
        { 13.7, -3.0, 0.0 }, { 12.8, 1.0, 0.0 }, { 11.3, 5.0, 0.0 },
        { 9.35, 9.0, 0.0 }, { 7.6, 11.0, 0.0 }, { 5.5, 13.0, 0.0 },
        { 3.0, 14.5, 0.0 }, { 0.0, 15.0, 0.0 }, { -3.0, 14.5, 0.0 },
        { -5.5, 13.0, 0.0 }, { -7.6, 11.0, 0.0 }, { -9.35, 9.0, 0.0 },
        { -11.3, 5.0, 0.0 }, { -12.8, 1.0, 0.0 }, { -13.7, -3.0, 0.0 },
        { -14.35, -7.0, 0.0 }, { -14.85, -11.0, 0.0 }, { -15.0, -15.0, 0.0 }
    };

    /* Vertices specified for GL_LINE_LOOP rendering. */
    static GVec3D thing_geom[] =
    {
        { 15.0, 0.0, 0.0 }, { 9.26, 1.142, 0.0 }, { 4.393, 4.393, 0.0 },
        { 1.142, 9.26, 0.0 }, { 0.0, 15.0, 0.0 }, { -1.142, 9.26, 0.0 },
        { -4.393, 4.393, 0.0 }, { -9.26, 1.142, 0.0 }, { -15.0, 0.0, 0.0 },
        { -9.26, -1.142, 0.0 }, { -4.393, -4.393, 0.0 }, { -1.142, -9.26, 0.0 },
        { 0.0, -15.0, 0.0 }, { 1.142, -9.26, 0.0, }, { 4.393, -4.393, 0.0 },
        { 9.26, -1.142, 0.0 }
    };

    /* Vertices specified for GL_LINE_LOOP rendering. */
    static GVec3D bow_geom[] =
    {
        { 7.5, -15.0, 0.0 }, { -7.5, 15.0, 0.0 }, { 7.5, 15.0, 0.0 },
        { -7.5, -15.0, 0.0 }
    };

    /*
     * Internal fudge factor because the 30-pixel spans of the default
     * geometries are too big.
     */
    cur_scale *= GLYPH_SCALE_FUDGE;

    if ( v2w[0] != vp_to_world_x
            || v2w[1] != vp_to_world_y
            || scale != cur_scale )
    {
        v2w[0] = vp_to_world_x;
        v2w[1] = vp_to_world_y;
        scale = cur_scale;
    }
    else
        return;

    /* Scale the default geometries into file-scope arrays for rendering. */
    for ( i = 0; i < sizeof( circle_geom ) / sizeof( circle_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            circle_verts[i][j] = circle_geom[i][j] * v2w[j] * scale;

    for ( i = 0; i < sizeof( triangle_geom ) / sizeof( triangle_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            triangle_verts[i][j] = triangle_geom[i][j] * v2w[j] * scale;

    for ( i = 0; i < sizeof( square_geom ) / sizeof( square_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            square_verts[i][j] = square_geom[i][j] * v2w[j] * scale;

    for ( i = 0; i < sizeof( pentagon_geom ) / sizeof( pentagon_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            pentagon_verts[i][j] = pentagon_geom[i][j] * v2w[j] * scale;

    for ( i = 0; i < sizeof( diamond_geom ) / sizeof( diamond_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            diamond_verts[i][j] = diamond_geom[i][j] * v2w[j] * scale;

    for ( i = 0; i < sizeof( cross_geom ) / sizeof( cross_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            cross_verts[i][j] = cross_geom[i][j] * v2w[j] * scale;

    for ( i = 0; i < sizeof( hexagon_geom ) / sizeof( hexagon_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            hexagon_verts[i][j] = hexagon_geom[i][j] * v2w[j] * scale;

    for ( i = 0; i < sizeof( splat_geom ) / sizeof( splat_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            splat_verts[i][j] = splat_geom[i][j] * v2w[j] * scale;

    for ( i = 0; i < sizeof( star_geom ) / sizeof( star_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            star_verts[i][j] = star_geom[i][j] * v2w[j] * scale;

    for ( i = 0; i < sizeof( x_geom ) / sizeof( x_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            x_verts[i][j] = x_geom[i][j] * v2w[j] * scale;

    for ( i = 0; i < sizeof( moon_geom ) / sizeof( moon_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            moon_verts[i][j] = moon_geom[i][j] * v2w[j] * scale;

    for ( i = 0; i < sizeof( plus_geom ) / sizeof( plus_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            plus_verts[i][j] = plus_geom[i][j] * v2w[j] * scale;

    for ( i = 0; i < sizeof( bullet_geom ) / sizeof( bullet_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            bullet_verts[i][j] = bullet_geom[i][j] * v2w[j] * scale;

    for ( i = 0; i < sizeof( thing_geom ) / sizeof( thing_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            thing_verts[i][j] = thing_geom[i][j] * v2w[j] * scale;

    for ( i = 0; i < sizeof( bow_geom ) / sizeof( bow_geom[0] ); i++ )
        for ( j = 0; j < 2; j++ )
            bow_verts[i][j] = bow_geom[i][j] * v2w[j] * scale;
}


/*****************************************************************
 * TAG( draw_glyph )
 *
 * Render a plot curve glyph centered at the input location.
 */
static void
draw_glyph( float pos[], Plot_glyph_data *p_gd )
{
    int i;
    GVec3D vert;
    GVec3D *verts;
    int qty_verts;
    Plot_glyph_type gtype;

    gtype = p_gd->glyph_type;

    switch( gtype )
    {
    case GLYPH_TRIANGLE:
    case GLYPH_SQUARE:
    case GLYPH_PENTAGON:
    case GLYPH_DIAMOND:
    case GLYPH_HEXAGON:
    case GLYPH_STAR:
    case GLYPH_MOON:
    case GLYPH_BULLET:
    case GLYPH_THING:
    case GLYPH_CROSS:
    case GLYPH_BOW:
    case GLYPH_CIRCLE:
        glBegin( GL_LINE_LOOP );
        break;

    case GLYPH_SPLAT:
    case GLYPH_X:
    case GLYPH_PLUS:
        glBegin( GL_LINES );
        break;
    }

    qty_verts = glyph_geom[gtype].qty_verts;
    verts = glyph_geom[gtype].verts;

    for ( i = 0; i < qty_verts; i++ )
    {
        VEC_ADD( vert, pos, verts[i] );
        glVertex3fv( vert );
    }

    glEnd();
}


/*****************************************************************
 * TAG( draw_plots )
 *
 * Draw plots in the current plot list.
 */
extern void
draw_plots( Analysis *analy )
{
    GLfloat gridcolor[3];
    float cx, cy, vp_to_world_x, vp_to_world_y;
    float text_height, win_ll[2], win_ur[2], gr_ll[2], gr_ur[2];
    float min_ab, max_ab, min_ord, max_ord, scale_width, min_incr_sz;
    float mins, maxs, lastmax;
    float min_ax[2], max_ax[2], incr_ax[2];
    float win_x_span, win_y_span, win_x_min, win_y_min;
    float ax_x_span, ax_y_span, ax_x_min, ax_y_min; 
    float pos[3], vert[2];
    float val, val2;
    float char_width;
    float y_scale_width, x_scale_width;
    double scale, offset;
    float *times;
    char str[M_MAX_NAME_LEN], result_label[M_MAX_NAME_LEN];
    char *xlabel, *op_name_str;
    char **legend_labels;
    int qty_plots;
    int max_incrs, incr_cnt[2];
    int max_classes, qty_classes;
    int i, j, k;
    int fracsz, y_fracsz, x_fracsz;
    int lablen, maxlablen;
    int first_st, last_st;
    int color_qty = sizeof( plot_colors ) / sizeof( plot_colors[0] );
    int mqty;
    float leg_line_dx, leg_offset, leg_side_width, legend_width;
    Plot_obj *p_po;
    List_head *plot_classes;
    float lo_text_lines, hi_text_lines;
    float *ab_data, *ord_data;
    Int_2tuple *blocks;
    int qty_blocks, cnt;
    int blk_start, blk_stop, start_state, limit_state;
    Bool_type cross_plot, ordinates_same, assume_have_shell, oper_plot;
    MO_class_data *ord_class;
    Plot_glyph_data *glyph_data;
    Plot_operation_type op_type;
    static GVec2D leg_line_geom[11] =
    {
        { -25.0, -5.5 }, { -20.0, -5.5 }, { -15.0, -5.0 }, { -10.0, -4.2 },
        { -5.0, -2.5 }, { 0.0, 0.0 }, { 5.0, 2.5 }, { 10.0, 4.2 },
        { 15.0, 5.0 }, { 20.0, 5.5 }, { 25.0, 5.5 }
    };
    GVec2D leg_line_verts[11];
    Bool_type mods_used[QTY_RESULT_MODIFIER_TYPES];

    /* Error Indicator Plot variables */
    Bool_type ei_labels = FALSE;
    float ei_bg_rgb[3] = {.7568, .8039, .8039};
    float ch, cw;

    int label;
    Bool_type end_curve=False;

    if ( analy->ei_result && analy->result_active )
        ei_labels = TRUE;

    if ( analy->current_plots == NULL )
    {
        popup_dialog( INFO_POPUP, "No plots are currently defined." );

#ifndef SERIAL_BATCH
        update_cursor_vals();
#endif

        /* Clear the window to reinforce the lack of plots. */
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_STENCIL_BUFFER_BIT );
        gui_swap_buffers();

        return;
    }

    first_st = analy->first_state;
    last_st = analy->last_state;

    /*
     * Traverse the plot list to accumulate mins/maxs and class counts.
     */

    /* Inits. */
    max_classes = 10;
    qty_classes = 0;
    plot_classes = NEW_N( List_head, max_classes, "Init plot classes" );
    p_po = analy->current_plots;
    cross_plot = p_po->abscissa->mo_class != NULL;
    get_bounded_series_min_max( p_po->abscissa, analy->first_state,
                                analy->last_state, &min_ab, &max_ab );
    get_bounded_series_min_max( p_po->ordinate, analy->first_state,
                                analy->last_state, &min_ord, &max_ord );

    /* Scale to rmin and rmax if they are set */
    if ( analy->mm_result_set[0] ) /* rmin set */
        min_ord = analy->result_mm[0];
    if ( analy->mm_result_set[1] ) /* rmax set */
    {
        max_ord = analy->result_mm[1];
    }

    /* Scale to tmin and tmax if they are set */
    if ( analy->mm_time_set[0] ) /* tmin set */
        min_ab = analy->time_mm[0];
    if ( analy->mm_time_set[1] ) /* tmax set */
        max_ab = analy->time_mm[1];

    /* Is this an operation plot? */
    if ( p_po->ordinate->op_type != OP_NULL )
    {
        oper_plot = TRUE;
        op_type = p_po->ordinate->op_type;
    }
    else
        oper_plot = FALSE;

    /* Count plots; determine if ordinate results are all the same. */
    qty_plots = 0;
    ordinates_same = TRUE;
    for ( ; p_po != NULL; NEXT( p_po ) )
    {
        qty_plots++;

        /* Check for varying ordinate results.*/
        if ( !oper_plot )
        {
            if ( p_po->next != NULL
                    && ( strcmp( p_po->ordinate->result->title,
                                 p_po->next->ordinate->result->title ) != 0 ) )
                ordinates_same = FALSE;
        }
    }

    if ( oper_plot || !ordinates_same )
        for ( i = 0; i < QTY_RESULT_MODIFIER_TYPES; i++ )
            mods_used[i] = FALSE;

    legend_labels = NEW_N( char *, qty_plots + QTY_RESULT_MODIFIER_TYPES,
                           "Legend label ptr array" );

    /* Loop over plots and accumulate info for managing/annotating the plots. */
    j = 0;
    maxlablen = 0;
    mqty = 0;
    assume_have_shell = FALSE;

    p_po = analy->current_plots;
    get_bounded_series_min_max( p_po->ordinate, analy->first_state,
                              analy->last_state, &mins, &lastmax );

    for ( p_po = analy->current_plots ; p_po != NULL; NEXT( p_po ) )
    {
        /*
         * Note that cross plots generate a blocks-list intersection
         * later during plotting and that, technically, that block list
         * should be used here to generate a valid series min/max.
         * This could become a factor only if two results for the same
         * object had different srec maps (availability patterns), which
         * is not likely to happen.
         */

        /* Update min's and max's. */
        get_bounded_series_min_max( p_po->abscissa, analy->first_state,
                                    analy->last_state, &mins, &maxs );
        if ( mins < min_ab )
            min_ab = mins;

        if ( maxs > max_ab )
            max_ab = maxs;

        get_bounded_series_min_max( p_po->ordinate, analy->first_state,
                                    analy->last_state, &mins, &maxs );

        
        if ( mins < min_ord )
            min_ord = mins;
        
        /* rescaling attempt by Bill Oliver 
        if ( !analy->mm_result_set[1] && lastmax < maxs) 
        {
            lastmax = maxs; 
            max_ord = maxs + (maxs - mins)*0.02;
        } */  
        
        if ( maxs > max_ord )
            max_ord = maxs;

        /* Scale to rmin and rmax if they are set */
        if ( analy->mm_result_set[0] ) /* rmin set */
            min_ord = analy->result_mm[0];
        if ( analy->mm_result_set[1] ) /* rmax set */
            max_ord = analy->result_mm[1];

        /* Scale to tmin and tmax if they are set */
        if ( analy->mm_time_set[0] ) /* tmin set */
            min_ab = analy->time_mm[0];
        if ( analy->mm_time_set[1] ) /* tmax set */
            max_ab = analy->time_mm[1];

        if ( !oper_plot )
            ord_class = p_po->ordinate->mo_class;

#ifdef DONT_KNOW_WHY_THIS_EXISTS 
        /* Accumulate class references and counts. */
        for ( i = 0; i < qty_classes; i++ )
        {
            if ( (MO_class_data *) plot_classes[i].list == ord_class )
            {
                plot_classes[i].qty++;
                break;
            }
        }

        if ( i == qty_classes )
        {
            /* Didn't find class already listed - add it. */
            if ( qty_classes == max_classes )
            {
                plot_classes = RENEW_N( List_head, plot_classes, max_classes,
                                        5, "Addl plot class" );
                max_classes += 5;
            }

            plot_classes[qty_classes].qty = 1;
            plot_classes[qty_classes].list = (void *) ord_class;
            qty_classes++;
        }
#endif

        /* Build legend label(s). */

        if ( p_po->ordinate->mo_class )
            if ( p_po->ordinate->mo_class->labels_found )
                label = get_class_label(  p_po->ordinate->mo_class, p_po->ordinate->ident );
            else
                label = p_po->ordinate->ident+1;
        else
            label = p_po->ordinate->ident+1;

        if ( oper_plot )
            build_oper_series_label( p_po->ordinate, TRUE, str );
        else if ( ordinates_same ){
        	if((strcmp(p_po->ordinate->mo_class->short_name,"mat") == 0)){
        		Htable_entry *tempEnt;
				char tempname[32];
				sprintf(tempname,"%d",label);
				htable_search(analy->mat_labels_reversed,tempname,FIND_ENTRY,&tempEnt);
				if(tempEnt != NULL){
					sprintf( str, "%s", (char*)tempEnt->data);
				}
				else{
					sprintf( str, "%s", "#ERROR#");
				}
        	}
        	else{
        		sprintf( str, "%s %d", p_po->ordinate->mo_class->long_name, label );
        	}
        }
        else
        {

        	if((strcmp(p_po->ordinate->mo_class->short_name,"mat") == 0)){
        		Htable_entry *tempEnt;
				char tempname[32];
				sprintf(tempname,"%d",label);
				htable_search(analy->mat_labels_reversed,tempname,FIND_ENTRY,&tempEnt);
				if(tempEnt != NULL){
					sprintf( str, "%s %s", (char*)tempEnt->data, p_po->ordinate->result->title);
				}
				else{
					sprintf( str, "#ERROR# %s", p_po->ordinate->result->title);
				}
        	}
        	else{
        		sprintf( str, "%s %d %s", p_po->ordinate->mo_class->long_name, label, p_po->ordinate->result->title );
        	}
        }

        if ( oper_plot )
        {
            add_legend_text( p_po->ordinate->operand1, legend_labels,
                             qty_plots, &mqty, &maxlablen, mods_used );
            add_legend_text( p_po->ordinate->operand2, legend_labels,
                             qty_plots, &mqty, &maxlablen, mods_used );
        }
        else if ( !ordinates_same )
            add_legend_text( p_po->ordinate, legend_labels, qty_plots, &mqty,
                             &maxlablen, mods_used );

        lablen = griz_str_dup( legend_labels + j, str );
        if ( lablen > maxlablen )
            maxlablen = lablen;
        j++;

        /* Note if shell-type elements for annotations. */
        if ( !oper_plot
                && ( ord_class->superclass == G_QUAD
                     || ord_class->superclass == G_HEX ) )
            assume_have_shell = TRUE;
    }

    if ( analy->perform_unit_conversion )
    {
        scale = analy->conversion_scale;
        offset = analy->conversion_offset;

        /*
         * By scaling the axis extremes now, we get "nice" axis limits for
         * the converted data (but we have to also convert all the data
         * values) and the coordinates display data is correct.  If we just
         * applied the scaling to the tic label values as they're output,
         * the axis limits might come out funky, although we wouldn't then
         * have to convert the data values when plotting.
         */
        min_ord = scale * min_ord + offset;
        max_ord = scale * max_ord + offset;

        if ( cross_plot )
        {
            min_ab = scale * min_ab + offset;
            max_ab = scale * max_ab + offset;
        }
    }

#ifdef OLDSMOOTH
    /* Apply smoothing if requested. */
    if ( analy->th_smooth )
    {
        /* Need to copy the data before modifying it. */
        st_result = NEW_N( float, num_plots*qty_states, "Filter timhis plot" );
        for ( i = 0; i < num_plots*qty_states; i++ )
            st_result[i] = t_result[i];

        smooth_th_data( num_plots, qty_states, st_result, analy );
        t_result = st_result;
    }
#endif

    /* Get viewport position and size. */
    get_foreground_window( &pos[2], &cx, &cy );

    /* Text size. */
    vp_to_world_y = 2.0 * cy / v_win->vp_height;
    vp_to_world_x = 2.0 * cx / v_win->vp_width;
    text_height = 14.0 * vp_to_world_y;
    hfont( "futura.l" );
    htextsize( text_height, text_height );

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
    /* "Y" has a good representative character width. */
    hgetcharsize( 'Y', &char_width, &val2 );
    fracsz = analy->float_frac_size;
    scale_width = ((( analy->auto_frac_size ) ? 6 : fracsz) + 7.0) * char_width;

    /* Legend size(s). */
    leg_line_dx = leg_line_geom[10][0] - leg_line_geom[0][0];
    leg_offset = 15.0;
    leg_side_width = vp_to_world_x * (leg_offset
                                      + ( analy->show_legend_lines
                                          ? leg_line_dx + leg_offset : 0 ) );
    legend_width = maxlablen * char_width + leg_side_width;

    /* Corners of the window. */
    win_ll[0] = -cx;
    win_ll[1] = -cy;
    win_ur[0] = cx;
    win_ur[1] = cy;

    /* Calculate lines of text required. */
    lo_text_lines = 2.5;
    if ( analy->perform_unit_conversion )
        lo_text_lines += 1.5;
    if ( analy->th_smooth )
        lo_text_lines += 1.5;
    hi_text_lines = ( analy->show_title ) ? 2.0 : 1.0;

    /* Corners of the graph. */
    gr_ll[0] = win_ll[0] + scale_width
               + ( ordinates_same ? 3 : 1 ) * text_height;
    gr_ll[1] = win_ll[1] + lo_text_lines * text_height + scale_width;
    gr_ur[0] = win_ur[0] - text_height - legend_width;
    gr_ur[1] = win_ur[1] - hi_text_lines * text_height;

    /* Get the tic increment and start and end tic values for each axis. */
    for ( i = 0; i < 2; i++ )
    {
        if ( i == 0 )
        {
            min_ax[i] = min_ab;
            max_ax[i] = max_ab;
        }
        else  /* i == 1 */
        {
            min_ax[i] = min_ord;
            max_ax[i] = max_ord;
        }

        /*
         * Take care of the special case where min and max are
         * approximately equal. APX_EQ macro was too tight.
         */
        if ( max_ax[i] == EPS )
        { 
            max_ax[i] = EPS;
        }

        if ( min_ax[i] == max_ax[i]
                || fabs( (double) 1.0 - min_ax[i] / max_ax[i] ) < EPS )
        {
            if ( min_ax[i] == 0.0 )
            {
                incr_ax[i] = 1.0;
                min_ax[i] = -1.0;
                max_ax[i] = 1.0;
            }
            else
            {
                incr_ax[i] = (float) fabs( (double) min_ax[i] );
                min_ax[i] = min_ax[i] - fabsf( max_ax[i] - min_ax[i] );
                max_ax[i] = max_ax[i] + fabsf( max_ax[i] - min_ax[i]); 
            }
            incr_cnt[i] = 2;
            /*continue; */
        } 

        max_incrs = (int)( (gr_ur[i] - gr_ll[i]) / (2.0 * text_height) );
        if ( max_incrs == 0 )
            max_incrs = 1;
        min_incr_sz = (max_ax[i] - min_ax[i]) / max_incrs;

        /* Get val into range [1.0,9.9999...]. */
        j = 0;
        val = 0;
        if ( min_incr_sz < 1.0  && min_incr_sz > 0.0)
            for ( val = min_incr_sz; (int)val < 1.0; val = val*10.0, j++ );
        else
            for ( val = min_incr_sz; (int)val >= 10.0; val = val*0.1, j++ );

        /* Increments of either 1, 2 or 5 are allowed. */ 
        if ( val <= 1.0 )
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

    /* Sanity checks. */
    if ( incr_cnt[0] == 0 )
    {
        incr_cnt[0] = 1;
        incr_ax[0] =  max_ax[0] - min_ax[0];
        popup_dialog( WARNING_POPUP, "%s\n%s",
                      "Standard X-axis generation failed;",
                      "data values may be incorrect." );
    }

    if ( incr_cnt[1] == 0 )
    {
        incr_cnt[1] = 1;
        incr_ax[1] =  max_ax[1] - min_ax[1];
        popup_dialog( WARNING_POPUP, "%s\n%s",
                      "Standard Y-axis generation failed;",
                      "data values may be incorrect." );
    }


    if ( analy->auto_frac_size )
    {
        y_fracsz = calc_fracsz( min_ax[1], max_ax[1], incr_cnt[1] );
        y_scale_width = (y_fracsz + 7.0) * char_width;

        x_fracsz = calc_fracsz( min_ax[0], max_ax[0], incr_cnt[0] );
        x_scale_width = (x_fracsz + 7.0) * char_width;

        /* Corners of the graph. */
        gr_ll[0] = win_ll[0] + y_scale_width
                   + ( ordinates_same ? 3 : 1 ) * text_height;
        gr_ll[1] = win_ll[1] + lo_text_lines * text_height + x_scale_width;
    }
    else
    {
        y_fracsz = fracsz;
        y_scale_width = scale_width;
        x_fracsz = fracsz;
        x_scale_width = scale_width;
    }

#ifndef SERIAL_BATCH
    /*
     * Record pertinent data for cursor coordinate display (in X Window System
     * coords - note the ymin and ymax are reversed as the X origin is in the
     * upper left corner of a window).
     */
    set_plot_win_params(
        gr_ll[0] / vp_to_world_x + ((float) v_win->vp_width / 2.0),  /* xmin */
        ((float) v_win->vp_height / 2.0) - gr_ur[1] / vp_to_world_y, /* ymin */
        gr_ur[0] / vp_to_world_x + ((float) v_win->vp_width / 2.0),  /* xmax */
        ((float) v_win->vp_height / 2.0) - gr_ll[1] / vp_to_world_y,  /* ymax */
        min_ax, max_ax );
#endif

    if ( analy->show_plot_glyphs )
    {
        /* Scale glyph geometry for current window and glyph scale. */
        init_glyph_draw( vp_to_world_x, vp_to_world_y, glyph_scale );

        /* Always want a time array for glyph thresholding. */
        if ( analy->times == NULL )
            analy->times = new_time_tso( analy );
        times = analy->times->data;

        /* Initialize the glyph data for rendering (AFTER init_glyph_draw()). */
        glyph_data = NEW_N( Plot_glyph_data, qty_plots, "Glyph data array" );
        init_glyph_eval( glyphs_per_plot, qty_plots, gr_ur[0] - gr_ll[0],
                         analy, glyph_data );
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
    glLineWidth( 1.25 );

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

    /*
     * Write axis sub-interval values and draw the tic marks or grid lines
     * at each sub-intervals.
     */

    /* If rendering grid, generate color as less-saturated foreground color. */
    if ( analy->show_plot_grid )
        for ( i = 0; i < 3; i++ )
            gridcolor[i] = v_win->foregrnd_color[i]
                           + 0.75 * ( 1.0 - v_win->foregrnd_color[i]);

    /* X axis values & grid/tics. */
    glColor3fv( v_win->text_color );

    /* glBegin( GL_LINE_LOOP ); SCR #636 */

    htextang( 90.0 );
    hrightjustify( TRUE );
    for ( i = 0; i <= incr_cnt[0]; i++ )
    {
        pos[0] = (gr_ur[0] - gr_ll[0]) * i/(float)incr_cnt[0] + gr_ll[0]
                 + 0.5 * text_height;
        pos[1] = gr_ll[1] - 0.5 * text_height;
        hmove( pos[0], pos[1], pos[2] );
        sprintf( str, "%.*e", x_fracsz, i * incr_ax[0] + min_ax[0] );
        hcharstr( str );

        if ( i != 0 && i != incr_cnt[0] )
        {
            if ( analy->show_plot_grid )
                glColor3fv( gridcolor );
            else
                glColor3fv( v_win->foregrnd_color );

            glBegin( GL_LINES );
            pos[0] -= 0.5 * text_height;
            pos[1] = gr_ll[1];
            glVertex3fv( pos );
            pos[1] = analy->show_plot_grid
                     ? gr_ur[1]
                     : gr_ll[1] + 0.5 * text_height;
            glVertex3fv( pos );
            glEnd();

            glColor3fv( v_win->text_color );
        }
    }

    /* Y axis values & grid/tics. */
    htextang( 0.0 );
    hrightjustify( TRUE );
    for ( i = 0; i <= incr_cnt[1]; i++ )
    {
        pos[0] = gr_ll[0] - 0.5*text_height;
        pos[1] = (gr_ur[1] - gr_ll[1]) * i/(float)incr_cnt[1] + gr_ll[1]
                 - 0.5 * text_height;
        hmove( pos[0], pos[1], pos[2] );
        sprintf( str, "%.*e", y_fracsz, i*incr_ax[1] + min_ax[1] );
        hcharstr( str );

        if ( i != 0 && i != incr_cnt[1] )
        {
            if ( analy->show_plot_grid )
                glColor3fv( gridcolor );
            else
                glColor3fv( v_win->foregrnd_color );

            glBegin( GL_LINES );
            pos[0] = gr_ll[0];
            pos[1] += 0.5*text_height;
            glVertex3fv( pos );
            pos[0] = analy->show_plot_grid
                     ? gr_ur[0]
                     : gr_ll[0] + 0.5 * text_height;
            glVertex3fv( pos );
            glEnd();

            glColor3fv( v_win->text_color );
        }
    }

    /* Init invariants for curve-drawing. */
    win_x_span = gr_ur[0] - gr_ll[0];
    win_y_span = gr_ur[1] - gr_ll[1];
    win_x_min = gr_ll[0];
    win_y_min = gr_ll[1];
    ax_x_span = max_ax[0] - min_ax[0];
    ax_y_span = max_ax[1] - min_ax[1];
    ax_x_min = min_ax[0];
    ax_y_min = min_ax[1];

    /* Draw the curves. */
    for ( p_po = analy->current_plots, cnt = 0; p_po != NULL;
            NEXT( p_po ), cnt++ )
    {
        /* Get abscissa and ordinate data arrays. */
        ab_data = p_po->abscissa->data;
        ord_data = p_po->ordinate->data;

        /* Assign blocks list to drive curve segmentation. */
        if ( p_po->abscissa->mo_class != NULL )
        {
            /* Non-time abscissa; find intersection of valid states. */
            gen_blocks_intersection( p_po->abscissa->qty_blocks,
                                     p_po->abscissa->state_blocks,
                                     p_po->ordinate->qty_blocks,
                                     p_po->ordinate->state_blocks,
                                     &qty_blocks, &blocks );
            cross_plot = TRUE;
        }
        else
        {
            /* Abscissa is time; let ordinate drive segmentation. */
            qty_blocks = p_po->ordinate->qty_blocks;
            blocks = p_po->ordinate->state_blocks;
            cross_plot = FALSE;
        }

        glColor3fv( analy->color_plots
                    ? plot_colors[cnt % color_qty] : v_win->foregrnd_color );

        /* Find data blocks for first and last plot states. */
        for ( blk_start = 0; blk_start < qty_blocks; blk_start++ )
        {
            if ( blocks[blk_start][0] <= first_st
                    && first_st <= blocks[blk_start][1] )
                break;
        }

        for ( blk_stop = blk_start; blk_stop < qty_blocks; blk_stop++ )
        {
            if ( blocks[blk_stop][0] <= last_st
                    && last_st <= blocks[blk_stop][1] )
                break;
        }

        for ( i = blk_start; i <= blk_stop; i++ )
        {
            start_state = ( i == blk_start ) ? first_st : blocks[i][0];
            limit_state = ( i == blk_stop ) ? last_st + 1 : blocks[i][1] + 1;

            glBegin( GL_LINE_STRIP );

            if ( !analy->perform_unit_conversion )
                for ( j = start_state; j < limit_state; j++ )
                {
                    end_curve = FALSE;

                    if ( analy->mm_time_set[0] && ab_data[j] < analy->time_mm[0] )
                        end_curve = TRUE;
                    if ( analy->mm_time_set[1] && ab_data[j] > analy->time_mm[1] )
                        end_curve = TRUE;
                    if ( end_curve)
                    {
                        glEnd();
                        glBegin( GL_LINE_STRIP );
                        continue;
                    }

                    if ( analy->mm_result_set[0] && ord_data[j]<analy->result_mm[0] )
                    {
                        /*
                         * If rmin is set and the current pos is below rmin.
                         * Check if previous point was above rmin. If true
                         * plot line down to rmin
                         */
                        if ( j > start_state )
                        {
                            // If previous point was above rmin, need to plot line down to rmin
                            if ( ord_data[j-1] > analy->result_mm[0] && !(analy->mm_time_set[0] && ab_data[j-1] < analy->time_mm[0]) )
                            {
                                //convert current point and previous point to screent coordinates
                                float prev_x, prev_y, curr_x, curr_y, rmin_val_y;

                                prev_x = win_x_min + win_x_span * (ab_data[j-1] - ax_x_min) / ax_x_span;
                                prev_y = win_y_min + win_y_span * (ord_data[j-1] - ax_y_min) / ax_y_span;
                                curr_x = win_x_min + win_x_span * (ab_data[j] - ax_x_min) / ax_x_span;
                                curr_y = win_y_min + win_y_span * (ord_data[j] - ax_y_min) / ax_y_span;
                                rmin_val_y = win_y_min + win_y_span * (analy->result_mm[0] - ax_y_min) / ax_y_span;

                                //Find x coordinate where line intersects with y=rmin 
                                float adjacent = fabs(prev_y - curr_y);
                                float opposite  = fabs(prev_x - curr_x);

                                //tan(angle) = opposite / adjacent
                                float angle = atan( opposite / adjacent );

                                float new_height = prev_y - rmin_val_y;
                                float x_shift = new_height * tan(angle);

                                pos[0] = prev_x + x_shift;
                                pos[1] = rmin_val_y;

                                glVertex3fv( pos );
                            }
                        }
                        end_curve = TRUE;
                    }
                    else if ( analy->mm_result_set[0] && j > start_state && ord_data[j-1] < analy->result_mm[0] )
                    {
                        /*
                         * If rmin is set and the previous point was below rmin,
                         * then plot from rmin to current point
                         *
                         * Also make sure previous state was not below tmin
                         */
                        if ( !(analy->mm_time_set[0] && ab_data[j-1] < analy->time_mm[0]) )
                        {
                            //convert current point and previous point to screent coordinates
                            float prev_x, prev_y, curr_x, curr_y, rmin_val_y;

                            prev_x = win_x_min + win_x_span * (ab_data[j-1] - ax_x_min) / ax_x_span;
                            prev_y = win_y_min + win_y_span * (ord_data[j-1] - ax_y_min) / ax_y_span;
                            curr_x = win_x_min + win_x_span * (ab_data[j] - ax_x_min) / ax_x_span;
                            curr_y = win_y_min + win_y_span * (ord_data[j] - ax_y_min) / ax_y_span;
                            rmin_val_y = win_y_min + win_y_span * (analy->result_mm[0] - ax_y_min) / ax_y_span;

                            //Find x coordinate where line intersects with y=rmin 
                            float adjacent = fabs(prev_y - curr_y);
                            float opposite  = fabs(prev_x - curr_x);

                            //tan(angle) = opposite / adjacent
                            float angle = atan( opposite / adjacent );

                            float new_height = curr_y - rmin_val_y;
                            float x_shift = new_height * tan(angle);

                            pos[0] = curr_x - x_shift;
                            pos[1] = rmin_val_y;

                            glVertex3fv( pos );
                        }
                    }

                    if ( analy->mm_result_set[1] && ord_data[j]>analy->result_mm[1] )
                    {
                        /*
                         * If rmax is set and the current pos is above rmax.
                         * Check if previous point was below rmax. If true
                         * plot line up to rmax
                         */
                        if ( j > start_state )
                        {
                            if ( ord_data[j-1] < analy->result_mm[1] && !(analy->mm_time_set[0] && ab_data[j-1] < analy->time_mm[0]) )
                            {
                                //convert current point and previous point to screent coordinates
                                float prev_x, prev_y, curr_x, curr_y, rmax_val_y;

                                prev_x = win_x_min + win_x_span * (ab_data[j-1] - ax_x_min) / ax_x_span;
                                prev_y = win_y_min + win_y_span * (ord_data[j-1] - ax_y_min) / ax_y_span;
                                curr_x = win_x_min + win_x_span * (ab_data[j] - ax_x_min) / ax_x_span;
                                curr_y = win_y_min + win_y_span * (ord_data[j] - ax_y_min) / ax_y_span;
                                rmax_val_y = win_y_min + win_y_span * (analy->result_mm[1] - ax_y_min) / ax_y_span;

                                //Find x coordinate where line intersects with y=rmin 
                                float adjacent = fabs(prev_y - curr_y);
                                float opposite  = fabs(prev_x - curr_x);

                                //tan(angle) = opposite / adjacent
                                float angle = atan( opposite / adjacent );
                                
                                float new_height = curr_y - rmax_val_y;
                                float x_shift = new_height * tan(angle);

                                pos[0] = curr_x - x_shift;
                                pos[1] = rmax_val_y;

                                glVertex3fv( pos );
                                
                            }
                        }
                        end_curve = TRUE;
                    }
                    else if ( analy->mm_result_set[1] && j > start_state && ord_data[j-1] > analy->result_mm[1] )
                    {
                        /*
                         * If rmax is set and the previous point was above rmax,
                         * then plot from rmax down to current point
                         *
                         * Also check that previous point is not below tmin
                         */
                        if ( !(analy->mm_time_set[0] && ab_data[j-1] < analy->time_mm[0]) )
                        {
                            //convert current point and previous point to screent coordinates
                            float prev_x, prev_y, curr_x, curr_y, rmax_val_y;

                            prev_x = win_x_min + win_x_span * (ab_data[j-1] - ax_x_min) / ax_x_span;
                            prev_y = win_y_min + win_y_span * (ord_data[j-1] - ax_y_min) / ax_y_span;
                            curr_x = win_x_min + win_x_span * (ab_data[j] - ax_x_min) / ax_x_span;
                            curr_y = win_y_min + win_y_span * (ord_data[j] - ax_y_min) / ax_y_span;
                            rmax_val_y = win_y_min + win_y_span * (analy->result_mm[1] - ax_y_min) / ax_y_span;

                            //Find x coordinate where line intersects with y=rmin 
                            float adjacent = fabs(prev_y - curr_y);
                            float opposite  = fabs(prev_x - curr_x);

                            //tan(angle) = opposite / adjacent
                            float angle = atan( opposite / adjacent );
                            
                            float new_height = prev_y - rmax_val_y;
                            float x_shift = new_height * tan(angle);

                            pos[0] = prev_x + x_shift;
                            pos[1] = rmax_val_y;

                            glVertex3fv( pos );
                        }
                    }

                    if ( end_curve)
                    {
                        glEnd();
                        glBegin( GL_LINE_STRIP );
                        continue;
                    }

                    pos[0] =  win_x_min + win_x_span * (ab_data[j] - ax_x_min)
                             / ax_x_span;
      
                    pos[1] =  win_y_min + win_y_span * (ord_data[j] - ax_y_min)
                                 / ax_y_span;

                    glVertex3fv( pos );
                }
            else
            {
                for ( j = start_state; j < limit_state; j++ )
                {
                    if ( analy->mm_result_set[0] && ord_data[j]<analy->result_mm[0] )
                        continue;
                    if ( analy->mm_result_set[1] && ord_data[j]>analy->result_mm[1] )
                        continue;
                    if ( analy->mm_time_set[0] && ab_data[j]<analy->time_mm[0] )
                        continue;
                    if ( analy->mm_time_set[1] && ab_data[j]>analy->time_mm[1] )
                        continue;

                    val = cross_plot ? ab_data[j] * scale + offset : ab_data[j];
                    pos[0] = win_x_min + win_x_span * (val - ax_x_min)
                             / ax_x_span;
                    val = ord_data[j] * scale + offset;
                    pos[1] = win_y_min + win_y_span * (val - ax_y_min)
                             / ax_y_span;
                    glVertex3fv( pos );
                }
            }
            glEnd();

            /* Loop over block again to draw glyphs. */
            if ( analy->show_plot_glyphs )
            {
                if ( !analy->color_glyphs && analy->color_plots )
                    glColor3fv( v_win->foregrnd_color );

                if ( !analy->perform_unit_conversion )
                    for ( j = start_state; j < limit_state; j++ )
                    {
                        if ( need_glyph_now( times[j], glyph_data + cnt ) )
                        {
                            pos[0] = win_x_min
                                     + win_x_span * (ab_data[j] - ax_x_min)
                                     / ax_x_span;
                            pos[1] = win_y_min
                                     + win_y_span * (ord_data[j] - ax_y_min)
                                     / ax_y_span;
                            draw_glyph( pos, glyph_data + cnt );
                        }
                    }
                else
                    for ( j = start_state; j < limit_state; j++ )
                    {
                        if ( need_glyph_now( times[j], glyph_data + cnt ) )
                        {
                            val = cross_plot
                                  ? ab_data[j] * scale + offset
                                  : ab_data[j];
                            pos[0] = win_x_min + win_x_span * (val - ax_x_min)
                                     / ax_x_span;
                            val = ord_data[j] * scale + offset;
                            pos[1] = win_y_min + win_y_span * (val - ax_y_min)
                                     / ax_y_span;
                            draw_glyph( pos, glyph_data + cnt );
                        }
                    }
            }
        }

        /* Save the plot's order to access correct plot color in legend. */
        p_po->index = cnt;

        if ( p_po->abscissa->mo_class != NULL )
            free( blocks );
    }

    /* Label each axis using titles from first plot. */

    p_po = analy->current_plots;

    glColor3fv( v_win->text_color );
    hcentertext( TRUE );
    pos[0] = 0.5*(gr_ur[0]-gr_ll[0]) + gr_ll[0];
    pos[1] = gr_ll[1] - x_scale_width - 1.5 * text_height;
    hmove( pos[0], pos[1], pos[2] );
    if ( p_po->abscissa->mo_class != NULL )
    {
        build_result_label( p_po->abscissa->result,
                            ( p_po->abscissa->mo_class->superclass == G_QUAD ||
                              p_po->abscissa->mo_class->superclass == G_HEX  ),
                            str );
        xlabel = str;
    }
    else
        xlabel = "Time";
    hcharstr( xlabel );

    if ( oper_plot )
    {
        switch ( op_type )
        {
        case OP_DIFFERENCE:
            op_name_str = "Difference";
            break;
        case OP_SUM:
            op_name_str = "Sum";
            break;
        case OP_PRODUCT:
            op_name_str = "Product";
            break;
        case OP_QUOTIENT:
            op_name_str = "Quotient";
            break;
        }

        if ( strcmp( p_po->ordinate->operand1->result->name,
                     p_po->ordinate->operand2->result->name ) == 0 )
        {
            sprintf( str, "%s(%s %s)", op_name_str,
                     p_po->ordinate->operand1->mo_class->long_name,
                     p_po->ordinate->operand1->result->title );
        }
        else
        {
            sprintf( str, "%s(%s %s, %s %s)", op_name_str,
                     p_po->ordinate->operand1->mo_class->long_name,
                     p_po->ordinate->operand1->result->title,
                     p_po->ordinate->operand2->mo_class->long_name,
                     p_po->ordinate->operand2->result->title );
        }

        htextang( 90.0 );
        pos[0] = gr_ll[0] - y_scale_width - 2.0*text_height;
        pos[1] = 0.5*(gr_ur[1]-gr_ll[1]) + gr_ll[1];
        hmove( pos[0], pos[1], pos[2] );
        hcharstr( str );
        htextang( 0.0 );
    }
    else if ( ordinates_same )
    {
        htextang( 90.0 );
        pos[0] = gr_ll[0] - y_scale_width - 2.0*text_height;
        pos[1] = 0.5*(gr_ur[1]-gr_ll[1]) + gr_ll[1];
        hmove( pos[0], pos[1], pos[2] );
        build_result_label( p_po->ordinate->result, assume_have_shell, str );
        strcpy( result_label, str );
        hcharstr( str );
        htextang( 0.0 );
    }

    /* File title. */
    if ( analy->show_title )
    {
        glColor3fv( v_win->text_color );
        hcentertext( TRUE );
        pos[0] = 0.0;
        pos[1] = win_ur[1] - text_height;
        hmove( pos[0], pos[1], pos[2] );
        hcharstr( analy->title );
        hcentertext( FALSE );
    }

    /* Draw legend. */
    hrightjustify( TRUE );
    /* pos[0] = win_ur[0] - (leg_line_dx + leg_offset * 2) * vp_to_world_x; */
    pos[0] = win_ur[0] - leg_side_width;
    pos[1] = gr_ur[1] - text_height;
    glColor3fv( v_win->text_color );
    for ( p_po = analy->current_plots; p_po != NULL; NEXT( p_po ) )
    {
        hmove( pos[0], pos[1], pos[2] );
        i = p_po->index;
        if ( !analy->show_legend_lines && analy->color_plots )
            glColor3fv( plot_colors[i % color_qty] );
        hcharstr( legend_labels[i] );

        pos[1] -= 1.5 * text_height;
    }

    if ( mqty > 0 )
    {
        glColor3fv( v_win->text_color );
        pos[1] -= 1.0 * text_height;

        for ( i = 0; i < mqty; i++ )
        {
            hmove( pos[0], pos[1], pos[2] );
            hcharstr( legend_labels[qty_plots + i] );
            pos[1] -= 1.5 * text_height;
        }
    }

    /* Legend label clean-up. */
    for ( i = 0; i < qty_plots + mqty; i++ )
        free( legend_labels[i] );
    free( legend_labels );

    if ( analy->show_legend_lines )
    {
        /* Scale legend line geometry for current window. */
        for ( i = 0; i < sizeof( leg_line_geom ) / sizeof( leg_line_geom[0] );
                i++ )
        {
            leg_line_verts[i][0] = leg_line_geom[i][0] * vp_to_world_x;
            leg_line_verts[i][1] = leg_line_geom[i][1] * vp_to_world_y;
        }

        vert[0] = win_ur[0] - (leg_line_dx / 2.0 + leg_offset) * vp_to_world_x;
        vert[1] = gr_ur[1] - text_height / 2.0;
        for ( p_po = analy->current_plots; p_po != NULL; NEXT( p_po ) )
        {
            i = p_po->index;
            if ( analy->color_plots )
                glColor3fv( plot_colors[i % color_qty] );

            glBegin( GL_LINE_STRIP );
            for ( j = 0;
                    j < sizeof( leg_line_geom ) / sizeof( leg_line_geom[0] );
                    j++ )
            {
                /* Note "pos" is 3D and pos[2] is already initialized. */
                VEC_ADD_2D( pos, leg_line_verts[j], vert );
                glVertex3fv( pos );
            }
            glEnd();

            if ( analy->show_plot_glyphs )
            {
                if ( analy->color_glyphs && analy->color_plots )
                    glColor3fv( plot_colors[i % color_qty] );
                else
                    glColor3fv( v_win->foregrnd_color );

                /* Position glyph in center of line. */
                VEC_SUB_2D( pos, pos, leg_line_verts[10] );
                draw_glyph( pos, glyph_data + i );
            }

            vert[1] -= 1.5 * text_height;
        }
    }

    /* Ordinate min/max display. */
    if ( analy->show_minmax )
    {
        /* Set y position. */
        switch ( analy->minmax_loc[0] )
        {
        case 'u':
            pos[1] = gr_ur[1] - 1.5 * text_height;
            break;
        case 'l':
            pos[1] = gr_ll[1] + 2.0 * text_height;
            break;
        }

        /* Set x position. */
        switch ( analy->minmax_loc[1] )
        {
        case 'l':
            pos[0] = gr_ll[0] + text_height;
            hleftjustify( TRUE );
            break;
        case 'r':
            pos[0] = gr_ur[0] - text_height;
            hrightjustify( TRUE );
            break;
        }

        glColor3fv( material_colors[15] ); /* Red */
        hmove( pos[0], pos[1], pos[2] );
        glColor3fv( v_win->text_color );
        if ( ei_labels )
            glColor3fv( material_colors[15] ); /* Red */

        /* The conditional was put in by Bill Oliver on 5/9/2014 to help with scaling.  See
 *         comment above that includes "Bill Oliver" 
        if(analy->perform_unit_conversion || analy->mm_result_set[1])
        {
            sprintf( str, "ymax %13.6e", max_ord );
        } else
        {
            sprintf( str, "ymax %13.6e", maxs);
        } */
        sprintf( str, "ymax %13.6e", max_ord );
        hcharstr( str );

        pos[1] -= 1.5 * text_height;
        hmove( pos[0], pos[1], pos[2] );
        sprintf( str, "ymin %13.6e", min_ord );
        hcharstr( str );

        if ( ei_labels )
            glColor3fv( v_win->text_color );
    }


    /* Print an Error Indicator (EI) message if error indicator
     * result is enabled.
     */
    if ( ei_labels )
    {
        set_color ("bg", ei_bg_rgb);

        if ( ! analy->show_minmax )
            hmove( pos[0], pos[1], pos[2] );

        pos[1] -= .9 * text_height*2;
        hmove( pos[0], pos[1], pos[2] );

        glColor3fv( material_colors[15] ); /* Red */
        hgetfontsize(&cw, &ch);
        htextsize(cw*1.1, ch*1.1);
        sprintf( str, "Error Indicator for Result:"  );
        hcharstr( str );

        pos[1] -= .5 * text_height*2;
        hmove( pos[0], pos[1], pos[2] );
        htextsize(cw*.85, ch*.85);
        sprintf( str, "    %s", result_label );
        hcharstr( str );

        glColor3fv( v_win->text_color );
    }


    /* Initialize Y-position to build up remaining lines of text from bottom. */
    pos[1] = win_ll[1] + 0.5 * text_height;

    /* Notify if data conversion active. */
    if ( analy->perform_unit_conversion )
    {
        hleftjustify( TRUE );
        pos[0] = win_ll[0] + text_height;
        hmove( pos[0], pos[1], pos[2] );
        glColor3fv( v_win->text_color );
        sprintf( str, "Data conversion scale/offset: %.3e/%.3e",
                 analy->conversion_scale, analy->conversion_offset );
        hcharstr( str );
    }

#ifdef OLDSMOOTH
    /* Notify if smoothing. */
    if ( analy->th_smooth )
    {
        pos[1] += 1.5 * text_height;

        hrightjustify( TRUE );
        pos[0] = gr_ur[0];
        hmove( pos[0], pos[1], pos[2] );
        glColor3fv( v_win->text_color );
        sprintf( str, "Smooth: %d", analy->th_filter_width );
        hcharstr( str );
    }
#endif

    /* Init the cursor coordinates display for this plot. */
#ifndef SERIAL_BATCH
    update_cursor_vals();
#endif

    /* Antialiasing off. */
    glDisable( GL_LINE_SMOOTH );
    glDisable( GL_BLEND );
    glLineWidth( 1.0 );

#ifdef OLDSMOOTH
    /* Free smoothing array. */
    if ( analy->th_smooth )
        free( st_result );
#endif

    glColor3fv( v_win->foregrnd_color );

    free( plot_classes );

    gui_swap_buffers();
}


/*****************************************************************
 * TAG( add_legend_text )
 *
 * Add a label string to an array of legend strings as required
 * by the modifier state of time series data.
 */
static void
add_legend_text( Time_series_obj *p_tso, char **labels, int offset,
                 int *mod_qty, int *maxlen, Bool_type mods_used[] )
{
    char *str_surf[] =
    {
        "Shell surf: middle", "Shell surf: inner",
        "Shell surf: outer"
    };
    char *str_frame[] =
    { "Ref Frame: global", "Ref Frame: local" };
    char *str_stype[] =
    {
        "Strain type: infin", "Strain type: grn-lagrange",
        "Strain type: almansi", "Strain type: rate"
    };
    char *str_time_deriv = "Time Deriv: yes";
    char *str_xform = "Coord transform: yes";
    char str_refstate[32];
    Result *p_r;
    int mqty, maxl, lablen;

    mqty = *mod_qty;
    maxl = *maxlen;
    p_r = p_tso->result;

    /* Add modifier labels at the bottom. */
    if ( !mods_used[STRAIN_TYPE]
            && p_r->modifiers.use_flags.use_strain_variety )
    {
        lablen = griz_str_dup( labels + offset + mqty,
                               str_stype[p_r->modifiers.strain_variety] );
        if ( lablen > maxl )
            maxl = lablen;
        mods_used[STRAIN_TYPE] = TRUE;
        mqty++;
    }

    if ( !mods_used[REFERENCE_FRAME]
            && p_r->modifiers.use_flags.use_ref_frame )
    {
        lablen = griz_str_dup( labels + offset + mqty,
                               str_frame[p_r->modifiers.ref_frame] );
        if ( lablen > maxl )
            maxl = lablen;
        mods_used[REFERENCE_FRAME] = TRUE;
        mqty++;
    }

    if ( !mods_used[REFERENCE_SURFACE]
            && p_r->modifiers.use_flags.use_ref_surface )
    {
        lablen = griz_str_dup( labels + offset + mqty,
                               str_surf[p_r->modifiers.ref_surf] );
        if ( lablen > maxl )
            maxl = lablen;
        mods_used[REFERENCE_SURFACE] = TRUE;
        mqty++;
    }

    if ( !mods_used[REFERENCE_STATE]
            && p_r->modifiers.use_flags.use_ref_state )
    {
        if ( p_r->modifiers.ref_state == 0 )
            sprintf( str_refstate, "Ref state: initial_geom" );
        else
            sprintf( str_refstate, "Ref state: %d", p_r->modifiers.ref_state );

        lablen = griz_str_dup( labels + offset + mqty,
                               str_refstate );
        if ( lablen > maxl )
            maxl = lablen;
        mods_used[REFERENCE_STATE] = TRUE;
        mqty++;
    }

    if ( !mods_used[TIME_DERIVATIVE]
            && p_r->modifiers.use_flags.time_derivative )
    {
        lablen = griz_str_dup( labels + offset + mqty,
                               str_time_deriv );
        if ( lablen > maxl )
            maxl = lablen;
        mods_used[TIME_DERIVATIVE] = TRUE;
        mqty++;
    }

    if ( !mods_used[COORD_TRANSFORM]
            && p_r->modifiers.use_flags.coord_transform )
    {
        lablen = griz_str_dup( labels + offset + mqty,
                               str_xform );
        if ( lablen > maxl )
            maxl = lablen;
        mods_used[COORD_TRANSFORM] = TRUE;
        mqty++;
    }

    *mod_qty = mqty;
    *maxlen = maxl;
}


/*****************************************************************
 * TAG( gen_blocks_intersection )
 *
 * Generate a block list that is the intersection of two other
 * block lists.
 *
 * Assumptions:
 * - consecutive blocks in a block list are non-contiguous
 * - the tuple defining each block has constant or increasing members
 *
 * Caller must free memory allocated for intersection block list.
 */
static void
gen_blocks_intersection( int qty_blocks1, Int_2tuple *blocks1, int qty_blocks2,
                         Int_2tuple *blocks2, int *new_qty_blocks,
                         Int_2tuple **new_blocks )
{
    int cur_idx, cur_blk1, cur_blk2, qty_blocks, block_end;
    Int_2tuple *blocks;

    cur_blk1 = 0;
    cur_blk2 = 0;
    cur_idx = blocks1[0][0];
    blocks = NULL;
    qty_blocks = 0;

    while ( cur_blk1 < qty_blocks1
            && cur_blk2 < qty_blocks2 )
    {
        /* If current index is within current blocks1 block... */
        if ( cur_idx >= blocks1[cur_blk1][0]
                && cur_idx <= blocks1[cur_blk1][1] )
        {
            /* If current index is within current blocks2 block... */
            if ( cur_idx >= blocks2[cur_blk2][0]
                    && cur_idx <= blocks2[cur_blk2][1] )
            {
                /* Have start of an intersection block. */
                blocks = RENEW_N( Int_2tuple, blocks, qty_blocks, 1,
                                  "Addl intersection block" );
                blocks[qty_blocks][0] = cur_idx;

                /*
                 * End of intersection is nearest current block end -
                 * blocks1 block or blocks2 block.
                 */
                if ( blocks2[cur_blk2][1] < blocks1[cur_blk1][1] )
                {
                    /* Intersection ends with end of current blocks2 block. */
                    block_end = blocks2[cur_blk2][1];
                    cur_blk2++;

                    /* Set current index to beginning of next blocks2 block. */
                    if ( cur_blk2 < qty_blocks2 )
                        cur_idx = blocks2[cur_blk2][0];
                }
                else
                {
                    /* Intersection ends with end of current blocks1 block. */
                    block_end = blocks1[cur_blk1][1];
                    cur_blk1++;

                    /* Set current index to beginning of next blocks1 block. */
                    if ( cur_blk1 < qty_blocks1 )
                        cur_idx = blocks1[cur_blk1][0];
                }

                blocks[qty_blocks][1] = block_end;
                qty_blocks++;
            }
            else if ( cur_idx < blocks2[cur_blk2][0] )
                /* Blocks1 in, blocks2 over; increment current index. */
                cur_idx = blocks2[cur_blk2][0];
            else
                /* Blocks1 in, blocks2 under; increment blocks2 block. */
                cur_blk2++;
        }
        else if ( cur_idx < blocks1[cur_blk1][0] )
            /* Blocks1 over; increment current index. */
            cur_idx = blocks1[cur_blk1][0];
        else
            /* Blocks1 under, increment blocks1 block. */
            cur_blk1++;
    }

    *new_qty_blocks = qty_blocks;
    *new_blocks = blocks;
}


/*****************************************************************
 * TAG( init_plot_colors )
 *
 * Initialize the plot_color array from the defaults.
 */
void
init_plot_colors( void )
{
    int i;

    for ( i = 0; i < (sizeof( plot_colors ) / sizeof( plot_colors[0] )); i++ )
        VEC_COPY( plot_colors[i], default_plot_colors[i] );
}


/*****************************************************************
 * TAG( set_plot_color )
 *
 * Assign a new RGB triple to an entry in the plot_colors array.
 */
extern void
set_plot_color( int plot, float *rgb )
{
    int idx;
    float *rgb_source;

    idx = (plot - 1) % (sizeof( plot_colors ) / sizeof( plot_colors[0] ));

    rgb_source = ( rgb != NULL ) ? rgb : default_plot_colors[idx];

    VEC_COPY( plot_colors[idx], rgb_source );
}


#ifdef OLDSMOOTH
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
#endif


