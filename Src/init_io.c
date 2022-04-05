/* $Id$ */
/*
 * init_io.c - Database access initialization routines.
 *             Routines in this module hard-code knowledge of database
 *             formats in order to ascertain the type (I/O library of
 *             origin) without actually calling the originating library.
 *
 *      Douglas E. Speck
 *      Methods Development Group
 *      Lawrence Livermore National Laboratory
 *      14 Feb 1997
 *
 ************************************************************************
 * Modifications:
 *  I. R. Corey - Nov 1, 2004: Put #ifdef EXO_SUPPORT around a dlclose
 *  that was always being compiled - caused problems on SunOS.
 ************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <dlfcn.h>
#include "viewer.h"

#ifdef EXO_SUPPORT
#include "exodusII.h"
#endif

#define CTL_WORDS 40

static Bool_type is_mili_db( char *fname );
static Bool_type is_taurus_plot_db( char *fname );
static Bool_type is_byte_swapped_taurus_plot_db( char *fname );
static Bool_type is_exodus_db( char *fname );

static void *exo_handle;


/*****************************************************************
 * TAG( parse_griz_init_file )
 *
 * This routine is called after minimal startup is completed in order to
 * find and process a grizinit file of initial commands to execute.
 */
void
parse_griz_init_file( void )
{
    FILE *test_file;
    char *init_file;
    char *home, home_path[512], home_hist[512];

    char init_cmd[100];

    /* Set error handler so that rgb dump errors don't cause an exit. */
    i_seterror( do_nothing_stub );

    /*
     * Read in the start-up history file.  First check in current
     * directory, then in directory specified in environment path.
     */

    /* First read an init file from home directory */
    home = getenv( "HOME" );
    strcpy(home_hist, "h ");
    strcpy(home_path, home);
    strcat(home_path, "/grizinit");

    if ( ( test_file = fopen( home_path, "r" ) ) != NULL )
    {
        fclose( test_file );
        strcat(home_hist, home_path);
        parse_command( home_hist, env.curr_analy );
    }
    else
    {
        init_file = getenv( "GRIZINIT" );
        if ( init_file != NULL )
        {
            strcpy( init_cmd, "h " );
            strcat( init_cmd, init_file );
            if ( ( test_file = fopen( init_file, "r" ) ) != NULL )
            {
                fclose( test_file );
                parse_command( init_cmd, env.curr_analy );
            }
        }
    }

    /* Now read init file from local directory */
    if ( ( test_file = fopen( "grizinit", "r" ) ) != NULL )
    {
        fclose( test_file );
        parse_command( "h grizinit", env.curr_analy );
    }


    /* Now read from local directory - problem specific init file = grizinit.<plotfile_name> */
    strcpy(home_path, "grizinit.");
    strcat(home_path, env.plotfile_name);
    strcpy(home_hist, "h ");
    strcat(home_hist, home_path);

    if ( ( test_file = fopen( home_path, "r" ) ) != NULL )
    {
        fclose( test_file );
        parse_command( home_hist, env.curr_analy );
    }

}


/************************************************************
 * TAG( is_known_db )
 *
 * Verify that a handle represents a database of a known
 * format.
 */
Bool_type
is_known_db( char *fname, Database_type_griz *p_db_type )
{
    if ( is_mili_db( fname ) )
    {
        *p_db_type = MILI;
    }
    else if ( is_taurus_plot_db( fname ) )
    {
        *p_db_type = TAURUS;
    }
    else if ( is_byte_swapped_taurus_plot_db( fname ) )
    {
        *p_db_type = TAURUS;
    }
#ifdef EXO_SUPPORT
    else if ( is_exodus_db( fname ) )
    {
        *p_db_type = EXODUS;
    }
#endif
    else
        return FALSE;

    return TRUE;
}


/************************************************************
 * TAG( init_db_io )
 *
 * Initialize I/O functions for an analysis.
 */
Bool_type
init_db_io( Database_type_griz db_type, Analysis *analy )
{
    Bool_type rval;
#ifdef EXO_SUPPORT
    int (*exo_db_open)( char *, int * );
    int (*exo_db_get_geom)( int, Mesh_data **, int * );
    int (*exo_db_get_st_descriptors)( Analysis *, int );
    int (*exo_db_get_state)( Analysis *, int, State2 *, State2 **, int * );
    int (*exo_db_get_subrec_def)( int, int , int, Subrecord * );
    int (*exo_db_cleanse_subrec)( Subrecord * );
    int (*exo_db_cleanse_state_var)( State_variable * );
    int (*exo_db_get_results)( int, int, int, int, char **, void * );
    int (*exo_db_query)( int, int, void *, char *, void * );
    int (*exo_db_get_title)( int, char * );
    int (*exo_db_get_dimension)( int, int * );
    int (*exo_db_set_buffer_qty)( int, int, char *, int );
    int (*exo_db_close)( Analysis * );
#endif

    rval = TRUE;

    switch ( db_type )
    {
    case MILI:
        analy->db_open = mili_db_open;
        analy->db_get_geom = mili_db_get_geom;
        analy->db_close = mili_db_close;
        analy->db_get_st_descriptors = mili_db_get_st_descriptors;
        analy->db_set_results = mili_db_set_results;
        analy->db_get_state = mili_db_get_state;
        analy->db_get_subrec_def = mili_db_get_subrec_def;
        analy->db_cleanse_subrec = mili_db_cleanse_subrec;
        analy->db_cleanse_state_var = mili_db_cleanse_state_var;
        analy->db_get_results = mili_db_get_results;
        analy->db_get_title = mili_db_get_title;
        analy->db_get_dimension = mili_db_get_dimension;
        analy->db_query = mili_db_query;
        analy->db_set_buffer_qty = mili_db_set_buffer_qty;
        break;

    case TAURUS:
        /* Taurus-specific functions. */
        analy->db_open = taurus_db_open;
        analy->db_get_geom = taurus_db_get_geom;
        analy->db_close = taurus_db_close;
        analy->db_get_subrec_def = taurus_db_get_subrec_def;
        analy->db_get_title = taurus_db_get_title;

        /* Just use Mili functions for these... */
        analy->db_get_st_descriptors = mili_db_get_st_descriptors;
        analy->db_cleanse_subrec = mili_db_cleanse_subrec;
        analy->db_cleanse_state_var = mili_db_cleanse_state_var;
        analy->db_set_results = mili_db_set_results;
        analy->db_get_state = mili_db_get_state;
        analy->db_get_results = mili_db_get_results;
        analy->db_get_dimension = mili_db_get_dimension;
        analy->db_query = mili_db_query;
        analy->db_set_buffer_qty = mili_db_set_buffer_qty;
        break;

#ifdef EXO_SUPPORT
    case EXODUS:
        exo_db_open = (int (*)( char *, int * ))
                      dlsym( exo_handle, "exodus_db_open" );
        if ( exo_db_open == NULL )
        {
            rval = FALSE;
            break;
        }
        analy->db_open = exo_db_open;

        exo_db_get_geom = (int (*)( int, Mesh_data **, int * ))
                          dlsym( exo_handle, "exodus_db_get_geom" );
        if ( exo_db_get_geom == NULL )
        {
            rval = FALSE;
            break;
        }
        analy->db_get_geom = exo_db_get_geom;

        exo_db_get_st_descriptors = (int (*)( Analysis *, int ))
                                    dlsym( exo_handle,
                                           "exodus_db_get_st_descriptors" );
        if ( exo_db_get_st_descriptors == NULL )
        {
            rval = FALSE;
            break;
        }
        analy->db_get_st_descriptors = exo_db_get_st_descriptors;

        analy->db_set_results = mili_db_set_results;

        exo_db_get_state = (int (*)( Analysis *, int, State2 *, State2 **,
                                     int * ) )
                           dlsym( exo_handle, "exodus_db_get_state" );
        if ( exo_db_get_state == NULL )
        {
            rval = FALSE;
            break;
        }
        analy->db_get_state = exo_db_get_state;

        exo_db_get_subrec_def = (int (*)
                                 ( int, int, int, Subrecord * ))
                                dlsym( exo_handle,
                                       "exodus_db_get_subrec_def" );
        if ( exo_db_get_subrec_def == NULL )
        {
            rval = FALSE;
            break;
        }
        analy->db_get_subrec_def = exo_db_get_subrec_def;

        exo_db_cleanse_subrec = (int (*)
                                 ( Subrecord * ))
                                dlsym( exo_handle,
                                       "exodus_db_cleanse_subrec" );
        if ( exo_db_cleanse_subrec == NULL )
        {
            rval = FALSE;
            break;
        }
        analy->db_cleanse_subrec = exo_db_cleanse_subrec;

        exo_db_cleanse_state_var = (int (*)
                                    ( State_variable * ))
                                   dlsym( exo_handle,
                                          "exodus_db_cleanse_state_var" );
        if ( exo_db_cleanse_state_var == NULL )
        {
            rval = FALSE;
            break;
        }
        analy->db_cleanse_state_var = exo_db_cleanse_state_var;

        exo_db_get_results = (int (*)
                              ( int, int, int, int, char **, void * ))
                             dlsym( exo_handle, "exodus_db_get_results" );
        if ( exo_db_get_results == NULL )
        {
            rval = FALSE;
            break;
        }
        analy->db_get_results = exo_db_get_results;

        exo_db_close = (int (*)( Analysis * ))
                       dlsym( exo_handle, "exodus_db_close" );
        if ( exo_db_close == NULL )
        {
            rval = FALSE;
            break;
        }
        analy->db_close = exo_db_close;

        exo_db_get_title = (int (*)( int, char * ))
                           dlsym( exo_handle, "exodus_db_get_title" );
        if ( exo_db_get_title == NULL )
        {
            rval = FALSE;
            break;
        }
        analy->db_get_title = exo_db_get_title;

        exo_db_get_dimension = (int (*)( int, int * ))
                               dlsym( exo_handle,
                                      "exodus_db_get_dimension" );
        if ( exo_db_get_dimension == NULL )
        {
            rval = FALSE;
            break;
        }
        analy->db_get_dimension = exo_db_get_dimension;

        exo_db_query = (int (*)( int, int, void *, char *, void * ))
                       dlsym( exo_handle, "exodus_db_query" );
        if ( exo_db_query == NULL )
        {
            rval = FALSE;
            break;
        }
        analy->db_query = exo_db_query;

        exo_db_set_buffer_qty = (int (*)( int, int, char *, int ))
                                dlsym( exo_handle,
                                       "exodus_db_set_buffer_qty" );
        if ( exo_db_set_buffer_qty == NULL )
        {
            rval = FALSE;
            break;
        }
        analy->db_set_buffer_qty = exo_db_set_buffer_qty;

        rval = TRUE;
        break;
#endif
    }

#ifdef EXO_SUPPORT
    if ( db_type == EXODUS && rval == FALSE )
    {
        dlerror();
        dlclose( exo_handle );
    }
#endif

    return rval;
}


/************************************************************
 * TAG( reset_db_io )
 *
 * Clean up loose ends for some db types when a db isn't
 * needed anymore.
 */
extern void
reset_db_io( Database_type_griz db_type )
{
    switch ( db_type )
    {
    case EXODUS:
#ifdef EXO_SUPPORT
        dlclose( exo_handle );
#endif
        break;

    default:
        /* Do nothing. */
        break;
    }
}


/************************************************************
 * TAG( is_mili_db )
 *
 * Verify a Mili database family by looking for an "A" file
 * and reading the Mili "magic" byte sequence at the beginning
 * of the file.
 */
static Bool_type
is_mili_db( char *fname )
{
    char mili_fname[512];
    char *cbuf;
    size_t mb_len;
    FILE *p_f;
    Bool_type rval;

    /* MILI LIBRARY-DEPENDENT! */
    char *magic_bytes = "mili";

    /* Build the "A" file name. */
    sprintf( mili_fname, "%sA", fname );

    mb_len = strlen( magic_bytes );
    cbuf = NEW_N( char, mb_len + 1, "Mili magic bytes" );

    rval = FALSE;

    p_f = fopen( mili_fname, "r" );
    if ( p_f != NULL )
    {
        fread( cbuf, 1, mb_len, p_f );
        if ( strcmp( magic_bytes, cbuf ) == 0 )
            rval = TRUE;

        fclose( p_f );
    }

    free( cbuf );

    return rval;
}


/************************************************************
 * TAG( is_taurus_plot_db )
 *
 * Attempt to verify a Taurus plotfile database family by
 * empirical checking of selected control header values.
 */
static Bool_type
is_taurus_plot_db( char *fname )
{
    FILE *p_f;
    Bool_type rval;
    int ctl[CTL_WORDS];
    size_t count;
    int title_bytes;
    int prtchar_cnt;
#ifdef KEEP_TITLE_CHECK
    int i;
    char *p_title;
#endif

    rval = TRUE;

    p_f = fopen( fname, "r" );
    if ( p_f != NULL )
    {
        /* Read in the control section. */
        count = fread( ctl, sizeof( int ), CTL_WORDS, p_f );
        if ( count != CTL_WORDS )
            return FALSE;

        /* First 10 words should contain only printable characters. */
        title_bytes = 10 * sizeof( int );
#ifndef KEEP_TITLE_CHECK
        prtchar_cnt = title_bytes;
#else
        prtchar_cnt = 0;
        for ( i = 0, p_title = (char *) ctl; i < title_bytes; p_title++, i++ )
            if ( isprint( (int) *p_title ) )
                prtchar_cnt++;
#endif

        /* Perform checks. */
        if ( (prtchar_cnt < title_bytes -1 ) || (prtchar_cnt > title_bytes ) )
            rval = FALSE;
        else if ( ctl[15] != 2        /* 2D database */
                  && ctl[15] != 3     /* 3D packed database */
                  && ctl[15] != 4 )   /* 3D unpacked database */
            rval = FALSE;
        else if ( ctl[17] != 1        /* Topaz database */
                  && ctl[17] != 2     /* old Dyna/Nike database */
                  && ctl[17] != 6     /* new Dyna/Nike database */
                  && ctl[17] != 200   /* Hydra database */
                  && ctl[17] != 300 ) /* Ping database */
            rval = FALSE;
        else if ( ctl[19] != 0 && ctl[19] != 1 ) /* node temperature flag */
            rval = FALSE;
        else if ( ctl[20] != 0 && ctl[20] != 1 ) /* node displacements flag */
            rval = FALSE;
        else if ( ctl[21] != 0 && ctl[21] != 1 ) /* node velocities flag */
            rval = FALSE;
        else if ( ctl[22] != 0 && ctl[22] != 1 ) /* node accelerations flag */
            rval = FALSE;

        fclose( p_f );
    }
    else
        rval = FALSE;

    return rval;
}


/************************************************************
 * TAG( is_byte_swapped_taurus_plot_db )
 *
 * Attempt to verify a Taurus plotfile database family by
 * empirical checking of selected control header values,
 * with values appropriately modified to match a db written
 * on a platform with opposite endianness of current host.
 */
static Bool_type
is_byte_swapped_taurus_plot_db( char *fname )
{
    FILE *p_f;
    Bool_type rval;
    unsigned int ctl[CTL_WORDS];
    size_t count;
    int title_bytes;
    int prtchar_cnt;
#ifdef KEEP_TITLE_CHECK
    char *p_title;
    int i;
#endif

    rval = TRUE;

    p_f = fopen( fname, "r" );
    if ( p_f != NULL )
    {
        /* Read in the control section. */
        count = fread( ctl, sizeof( int ), CTL_WORDS, p_f );
        if ( count != CTL_WORDS )
            return FALSE;

        /* First 10 words should contain only printable characters. */
        title_bytes = 10 * sizeof( int );
#ifndef KEEP_TITLE_CHECK
        prtchar_cnt = title_bytes;
#else
        prtchar_cnt = 0;
        for ( i = 0, p_title = (char *) ctl; i < title_bytes; p_title++, i++ )
            if ( isprint( (int) *p_title ) )
                prtchar_cnt++;
#endif

        /* Perform checks. */
        if ( prtchar_cnt != title_bytes )
            rval = FALSE;
        else if ( ctl[15] != 33554432        /* 2D database */
                  && ctl[15] != 50331648     /* 3D packed database */
                  && ctl[15] != 67108864 )   /* 3D unpacked database */
            rval = FALSE;
        else if ( ctl[17] != 16777216        /* Topaz database */
                  && ctl[17] != 33554432     /* old Dyna/Nike database */
                  && ctl[17] != 100663296    /* new Dyna/Nike database */
                  && ctl[17] != 3355443200   /* Hydra database */
                  && ctl[17] != 738263040 )  /* Ping database */
            rval = FALSE;
        else if ( ctl[19] != 0 && ctl[19] != 16777216 ) /* node temperature flag */
            rval = FALSE;
        else if ( ctl[20] != 0 && ctl[20] != 16777216 ) /* node displacements flag */
            rval = FALSE;
        else if ( ctl[21] != 0 && ctl[21] != 16777216 ) /* node velocities flag */
            rval = FALSE;
        else if ( ctl[22] != 0 && ctl[22] != 16777216 ) /* node accelerations flag */
            rval = FALSE;

        fclose( p_f );
    }
    else
        rval = FALSE;

    return rval;
}


#ifdef EXO_SUPPORT
/************************************************************
 * TAG( is_exodus_db )
 *
 * Verify a Exodus database family by first confirming it is
 * netCDF, then verifying Exodus based on the presence of
 * certain attributes.  Note that we can do the netCDF test
 * without loading any libraries, but for Exodus we must have
 * netCDF (unless/until someone at NCAR can identify a way of
 * doing this without the library).  However, if we pass the
 * netCDF test, we have a high probability of success for
 * Exodus, so loading netCDF will likely be required anyway
 * (even if we do violate the identify-library logic goal of
 * independence from actual I/O libraries).
 */
static Bool_type
is_exodus_db( char *fname )
{
    int c1, c2, c3;
    FILE *fid;
    int exo_id;
    int CPU_word_size, IO_word_size;
    float exo_version;
    int (*exo_open)( char *, int, int *, int *, float * );
    int (*exo_close)( int );
    char *err_txt;

    CPU_word_size = 0;
    IO_word_size  = 0;

    fid = fopen( fname, "r" );

    if ( fid == NULL )
    {
        popup_dialog( WARNING_POPUP,
                      "Unable to stdio open file \"%s\".", fname );
        return FALSE;
    }

    c1 = getc( fid );
    c2 = getc( fid );
    c3 = getc( fid );
    if ( c1 == 'C' && c2 == 'D' && c3 == 'F' )
        fclose( fid );

    exo_handle = dlopen( "libgex.so", RTLD_LAZY );
    if ( exo_handle == NULL )
    {
        err_txt = dlerror();
        fprintf( stderr, "%s", err_txt );
        return FALSE;
    }

    exo_open = (int (*)( char *, int, int *, int *, float * ))
               dlsym( exo_handle, "ex_open" );
    if ( exo_open == NULL )
    {
        err_txt = dlerror();
        fprintf( stderr, "%s", err_txt );
        dlclose( exo_handle );
        return FALSE;
    }

    exo_id = exo_open( fname, EX_READ, &CPU_word_size, &IO_word_size,
                       &exo_version );

    if ( exo_id < 0 )
    {
        dlclose( exo_handle );
        return FALSE;
    }

    exo_close = (int (*)( int )) dlsym( exo_handle, "ex_close" );
    if ( exo_close == NULL )
    {
        dlerror();
        dlclose( exo_handle );
        return FALSE;
    }

    if ( exo_close( exo_id ) == -1 )
    {
        dlclose( exo_handle );
        return FALSE;
    }

    return TRUE;
}
#endif

