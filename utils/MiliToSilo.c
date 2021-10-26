/* $Id: MiliToSilo.c,v 1.10 2011/05/26 19:42:27 durrenberger1 Exp $ */

/* 
 * Mili to Silo database converter
 * 
 *
 ************************************************************************
 * NOTE: To build this utility must configure with --enable-sl - do NOT
 * --enable-silo since we do not want to use the MiliSilo API.
 ************************************************************************
 * Modifications:
 *  I. R. Corey - October 8, 2008: Created
 *
 */

#include <stdio.h>
#include <string.h>

#include "silo.h"
#include "SiloLib.h"
#include "SiloLib_Internal.h"

#include "mili_internal.h"
#include "mili.h"
#include "mr.h"

extern Mili_family **fam_list;

static char fname_in[M_MAX_NAME_LEN], fname_out[M_MAX_NAME_LEN];
static int  start_state,  stop_state;

static void scan_args( int argc, char *argv[] );
static void usage( void );

main( int argc, char *argv[] )
{
    int i, j, k, l;
    int dbid=0;

    /* Node variables */
    int mesh_id, class_id;
    int node_qty;
    int   *node_labels;
    float *coords;

    char short_name[M_MAX_NAME_LEN], long_name[M_MAX_NAME_LEN];
    int status;

    /* For each element type: quantity, connectivity, materials, parts,
     * element ids, and element labels */
    int qty_truss=0,    *conn_truss=NULL,    *mat_truss=NULL,
        num_mats_truss, *mat_list_truss, *part_truss=NULL,
        *labels_truss=NULL;
    int qty_beam=0,     *conn_beam=NULL,     *mat_beam=NULL,
        num_mats_beam,  *mat_list_beam,  *part_beam=NULL,
        *labels_beam=NULL;
    int qty_tri=0,      *conn_tri=NULL,      *mat_tri=NULL,
        num_mats_tri,   *mat_list_tri,   *part_tri=NULL,
        *labels_tri=NULL;
    int qty_quad=0,     *conn_quad=NULL,     *mat_quad=NULL,
        num_mats_quad,  *mat_list_quad,  *part_quad=NULL,
        *labels_quad=NULL;
    int qty_tet=0,      *conn_tet=NULL,      *mat_tet=NULL,
        num_mats_tet,   *mat_list_tet,   *part_tet=NULL,
        *labels_tet=NULL;
    int qty_hex=0,      *conn_hex=NULL,      *mat_hex=NULL,
        num_mats_hex,   *mat_list_hex,   *part_hex=NULL,
        *labels_hex=NULL;
    int qty_particle=0, *conn_particle=NULL, *mat_particle=NULL,
                                         *part_particle=NULL,
        *labels_particle=NULL;

    int  qty_vars_truss=0, qty_vars_beam=0;;
    char **vars_truss, **vars_beam, **vars_tri;
 
    float  *results_flt=NULL;
    double *results_dbl=NULL;
    int    *results_int=NULL;
    void   *results_ptr;

    int obj_qty=0, obj_index=0, qty_classes=0;
    int state, qty_states=1;
    float time=0.0;

    Mili_analysis analy;
    Mili_family *fam_in;
    Database_type db_type;

    int matid, meshid, superclass;
    Bool_type isNodal, isMeshvar;

    /* Variables used for labels */
    int block_qty=0, *block_range=NULL;

    /* Variables used for TI table searches */
    int  num_entries, tsize;
    int  num_items_read;

    /* State record variables */
    int srec_qty=0, svar_qty=0, subrec_qty=0, subrec_count=0;
    int block_id=0;
    Subrecord *p_subrec;
    int  subrec_names_len=0, subrec_vars_len=0;
    char **subrec_names,     **subrec_vars;
    
    /* Result variables */
    float *result[1];
    int    result_len=0, result_veclen=0, result_type=0;

    /* Mili parameter variables - constants and non time dep variables */
    int  num_params=0, num_params_ti=0;
    char **param_list,  **param_list_ti;
    int  *param_lens,   *param_lens_ti;
    int  *param_types,  *param_types_ti;    

    /* Metadata variables */
    char mili_version[64], host[64], arch[64], timestamp[64];

    /* Silo Variables */
    DBfile *dbPlot;
    char *plotfile;
    int cycle=0, processor=0, sequence_number=0;

    /* Silo Mesh variables */
    int    total_zones;
    float  *coord_x, *coord_y, *coord_z;
    float  *coords_ptr[3];
    char   *coord_names[3] = {"X Coord", "Y Coord", "Z Coord"};

   /* Material Variables */
   short *mat_list;
   int   *mat_zone_list; 
   int   mat, matnum, num_mats;
   int   *mat_nums;
   int   mat_count=0;
   int   *node_list, nl_length=0, zone_cnt=0;
   char  *mat_names[20], matname[64];

    fprintf(stderr, "\n\t Running MiliToSilo Version: %s", MILI_VERSION);

    /* Scan command-line arguments, other initialization. */
    scan_args( argc, argv );
 
    mc_ti_enable(dbid);

    /* Determine database type - Mili or ???? */

    mili_is_known_db( fname_in, &db_type );

    if (db_type==MILI_DB_TYPE) {
        printf("\n\nOpening Mili Database: %s", fname_in);
    }
    else 
    {
        printf("Unknown Database: %s", fname_in);
        printf("Aborting");
        exit(1);
    }

    status = mili_open_input_dbase( fname_in, &analy );

    dbid = analy.db_ident;
    fam_in = fam_list[dbid];


    status = mc_query_family( dbid, QTY_STATES, NULL, NULL,
                              &qty_states );

    /* Get the TOC list for all the non-TI params */
    status = mc_get_param_list( dbid, &num_params,    FALSE, &param_list );

    param_lens  = (int *) malloc(num_params*sizeof(int));
    param_types = (int *) malloc(num_params*sizeof(int));

    /* Get the data attributes for all param variables */
    for (i=0; i<num_params; i++) {
         status = mc_reader_get_param_atributes( dbid, param_list[i], FALSE,
                                                 &param_types[i],
                                                 &param_lens[i] );
    }

    param_lens_ti  = (int *) malloc(num_params*sizeof(int));
    param_types_ti = (int *) malloc(num_params*sizeof(int));

    /* Get the TOC list for all the non-TI params */
    status = mc_get_param_list( dbid, &num_params_ti, TRUE, &param_list_ti );

    /* Get the data attributes for all param variables */
    for (i=0; i<num_params_ti; i++) {
         status = mc_get_param_atributes( dbid, param_list_ti[i], TRUE,
                                          &param_types_ti[i],
                                          &param_lens_ti[i] );
    }
    
 
    /* Read Mili File Metadata */
    mc_read_string( dbid, "lib version", mili_version );
    mc_read_string( dbid, "host name",   host );
    mc_read_string( dbid, "arch name",   arch );
    mc_read_string( dbid, "date",        timestamp );

    /* Load node data - coords x,y,z */
    mesh_id  = 0;
    class_id = 0;
    status = mc_get_class_info( dbid, mesh_id, M_NODE, class_id, 
                                short_name, long_name,
                                &node_qty );

    coords = (float *) malloc(node_qty*3*sizeof(float));
    status = mc_load_nodes( dbid, mesh_id, short_name, 
                            (void *) coords );


    node_labels = (int *) malloc(node_qty*sizeof(int));
    status = mc_load_node_labels( dbid,  mesh_id, short_name, 
                                  &block_qty, &block_range, 
                                  node_labels );


    /* Load element connectivity */

    /*********/
    /* HEXES */
    /*********/
    status = mc_get_geom( dbid, mesh_id, M_HEX,
                          &qty_hex,
                          &conn_hex, &mat_hex, &num_mats_hex, &mat_list_hex,
                          &part_hex, &labels_hex);

    /**********/
    /* SHELLS */
    /**********/
    status = mc_get_geom( dbid, mesh_id, M_QUAD,
                          &qty_quad,
                          &conn_quad, &mat_quad, &num_mats_hex, &mat_list_hex, 
                          &part_quad, &labels_quad);

    /*********/
    /* BEAMS */
    /*********/
    status = mc_get_geom( dbid, mesh_id, M_BEAM,
                          &qty_beam,
                          &conn_beam, &mat_beam, &num_mats_beam, &mat_list_beam, 
                          &part_beam, &labels_beam);


    total_zones = qty_hex + qty_quad + qty_beam;

    /* Get state record format count for this database. */
    status = mc_query_family( dbid, QTY_SREC_FMTS, NULL, NULL, 
                              (void *) &srec_qty );

    for ( i=0; i < srec_qty; i++ ) {
 
          /* Get subrecord count for this state record. */
          status = mc_query_family( dbid, QTY_SUBRECS, (void *) &i, NULL, 
                                    (void *) &subrec_qty );
          subrec_count += subrec_qty;
    }

    /* Get state variable quantity for this database. */
    status = mc_query_family( dbid, QTY_SVARS, NULL, NULL, 
                              (void *) &svar_qty );

    /* Load the Subrecord definitions into a file */
    p_subrec = (Subrecord *) malloc(subrec_count*sizeof(Subrecord));

    k=0;
    for (i=0; i < srec_qty; i++) {
         for ( j=0; j< subrec_qty; j++ ) {

               /* Get binding */
               status = mc_get_subrec_def( dbid, i, j, &p_subrec[k++] );
         }
    }

    /* Dump the Subrecord definitions to a file called Subrecs.txt. */

    status = mc_dump_subrecs(dbid, "Subrecs.txt", subrec_count, p_subrec);

    status = mc_get_subrec_list(dbid, subrec_count, p_subrec, "particle",
                                &subrec_names_len,  &subrec_names);


    status = mc_get_subrec_vars(dbid, subrec_count, p_subrec, 
                                "particle", subrec_names[0],
                                &subrec_vars_len,   &subrec_vars);

    status = mc_get_subrec_list(dbid, subrec_count, p_subrec, "shell",
                                &subrec_names_len,  &subrec_names);


    status = mc_get_subrec_vars(dbid, subrec_count, p_subrec, "shell", 
                                subrec_names[0], &subrec_vars_len, 
                                &subrec_vars);

 
    /* Open the Silo Output File */
    cycle =          -1;
    processor       = -1;
    sequence_number = -1;
    plotfile = SiloLib_Create_Filename(fname_out, SILO_PLOTFILE, 
                                       cycle, processor, sequence_number);
    status   = SiloLib_Open_Restart_File(1,plotfile,NULL,
                                         cycle,processor,sequence_number,
                                         SILO_PLOTFILE,FILE_CREATE);

    /* Get DB Identifier from Silo Library */
    dbPlot = SiloLIB_getDBid(1, SILO_PLOTFILE );

    /* Create Silo Specific Data Structures */

    coord_x   = (float *) calloc(node_qty,sizeof(double));
    coord_y   = (float *) calloc(node_qty,sizeof(double));
    coord_z   = (float *) calloc(node_qty,sizeof(double));
    
    coords_ptr[X] = coord_x;
    coords_ptr[Y] = coord_y;
    coords_ptr[Z] = coord_z;
    
    mat_list      = (short *) calloc(1000,sizeof(short));
    mat_zone_list = (int *)   calloc(total_zones,sizeof(int)); 
    mat_nums      = (int *)   calloc(1000,sizeof(int));
    node_list     = (int *)   calloc(total_zones*8,sizeof(int));

    /* Read results per state */

    status = mc_query_family( dbid, QTY_STATES, NULL, NULL, &qty_states );
    if (stop_state<0) {
        stop_state = qty_states;
    }

    analy.state_times = (float *) malloc(qty_states*sizeof(float));

    for (i=start_state-1; i<stop_state; i++)
    {
         state = i+1;
         status = mc_query_family( dbid, STATE_TIME, (void *) &state, NULL, 
                                   (void *) &time );

         analy.state_times[i] = time;

    }

    status = SiloLib_Close_Restart_File(dbPlot);

    status = mc_close( dbid );
    if ( status != 0 )
    {
         mc_print_error( "mc_close", status );
         exit( -1 );
    }

    exit( OK );


   /* All the Mili data had been loaded into memory */

}


/************************************************************
 * TAG( scan_args )
 *
 * Parse the reader program's command line arguments.
 */
static void
scan_args( int argc, char *argv[] )
{
    int i;
    int inname_set;
 
    start_state = 1;
    stop_state  = -1;

    for ( i = 1; i < argc; i++ )
    {
        if ( strcmp( argv[i], "-start" ) == 0 )
        {
            i++;
            start_state = atoi(argv[i]);
        }

        if ( strcmp( argv[i], "-stop" ) == 0 )
        {
            i++;
            stop_state = atoi(argv[i]);
        }

        if ( strcmp( argv[i], "-i" ) == 0 )
        {
            i++;
            if(i >= argc){
               usage();
               exit(0);
            }
            strcpy( fname_in, argv[i] );
            inname_set = TRUE;
        }

        if ( strcmp( argv[i], "-o" ) == 0 )
        {
            i++;
            if(i >= argc){
               usage();
               exit(0);
            }
            strcpy( fname_out, argv[i] );
            inname_set = TRUE;
        }
    }  

    if ( !inname_set )
    {
         usage();
         exit(1);
    }
}


/************************************************************
 * TAG( usage )
 *
 * Write out command-line syntax.
 */
static void
usage( void )
{
    printf("\n" );
    printf("Mili To Silo Conveter: militosilo\n" );
    printf("\n" );
    printf("Usage: militosilo -i <input_base_name>\n" );
    printf("                  -o <output_base_name>\n");
    printf("\n");
    printf("OPTIONS:\n");
    printf("                [-start]  <start state >=1 >                       \n");
    printf("                [-stop]   <stop state <= max states >              \n");
    printf("\n");
}
