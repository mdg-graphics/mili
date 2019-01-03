/* $Id: MiliGeomGen.c,v 1.3 2011/05/26 19:42:09 durrenberger1 Exp $ */

/* 
 * Mili database geometry generator
 * 
 ************************************************************************
 * NOTE: To build this utility must configure with --enable-sl - do NOT
 * --enable-silo since we do not want to use the MiliSilo API.
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - November 21, 2008: Created
 *
 */


#include <stdio.h>
#include <string.h>

#include "mili.h"
#include "mili_internal.h"
#include "mr.h"

extern Mili_family **fam_list;

static char fname_in[M_MAX_NAME_LEN], fname_out[M_MAX_NAME_LEN];

static void scan_args( int argc, char *argv[] );
static void usage( void );

 
main( int argc, char *argv[] )
{
    int i, j, k, l;
    int dbid=0;

    /* Node variables */
    int mesh_id, class_id;
    int qty_node;
    int   *node_labels;
    float *coords;

    char short_name[M_MAX_NAME_LEN], long_name[M_MAX_NAME_LEN];
    int status;

    typedef struct _mili_geom_type {
        int qty_truss,    *conn_truss,    *mat_truss,
            num_mats_truss, *mat_list_truss, *part_truss,    *labels_truss;
        int qty_beam,     *conn_beam,     *mat_beam,
            num_mats_beam,  *mat_list_beam,  *part_beam,     *labels_beam;
        int qty_tri,      *conn_tri,      *mat_tri,
            num_mats_tri,   *mat_list_tri,   *part_tri,      *labels_tri;
        int qty_quad,     *conn_quad,     *mat_quad,
            num_mats_quad,  *mat_list_quad,  *part_quad,     *labels_quad;
        int qty_tet,      *conn_tet,      *mat_tet,
            num_mats_tet,   *mat_list_tet,   *part_tet,      *labels_tet;
        int qty_hex,      *conn_hex,      *mat_hex,
            num_mats_hex,   *mat_list_hex,   *part_hex,      *labels_hex;
        int qty_particle, *conn_particle, *mat_particle,
                                             *part_particle, *labels_particle;
    } Mili_geom_type;


    Mili_geom_type mili_geom;
  
    int  fid;
    int  qty=0;

    int total_zones=0;

    int obj_qty=0, obj_index=0, qty_classes=0;
    int state, qty_states=1;
    float time=0.0;

    Mili_analysis analy;
    Mili_family *fam_in;
    Database_type db_type;

    int matid, meshid, superclass;
    Bool_type isNodal, isMeshvar;

    /* Variables used for labels */
    int block_qty=0, *block_range, block_index=0;

     /* State record variables */
    int srec_qty=0, svar_qty=0, subrec_qty=0, subrec_count=0;
    int block_id=0, elem_id=0;
    Subrecord *p_subrec;
    int  subrec_names_len=0,  subrec_vars_len=0;
    char **subrec_names=NULL, *subrec_vars[1];
    
    /* Result variables */
    float *result[1];
    int    result_len=0, result_veclen=0, result_type=0;

    /* Mili parameter variables - constants and non time dependent variables */
    int  num_params=0,  num_params_ti=0;
    char **param_list,  **param_list_ti;
    int  *param_lens,   *param_lens_ti;
    int  *param_types,  *param_types_ti;    

    /* Mesh variables */
    int mid;

    float *ncoords1, *ncoords2, *ncoords3;
    int   *hexs, *tets, *shells, *beams;

    /* Metadata variables */
    char mili_version[64], host[64], arch[64], timestamp[64];

    /* Material Variables */
    short *mat_list;
    int   *mat_zone_list;  
    int   mat, matnum, num_mats;
    int   *mat_nums;
    int   mat_count=0;
    int   *node_list, nl_length=0, zone_cnt=0;
    char  *mat_names[20], matname[64];
    
    /* State record variables */
    int   sid;

    /* Object database variables */
    int group=-1;
    ObjDef *p_obj;

    fprintf(stderr, "\n\t Running MiliGeomGen: %s\n\n", MILI_VERSION);

    /* Scan command-line arguments, other initialization. */
    scan_args( argc, argv );
 
    /* Load an OBJ file into memory */
    status = mc_OBJ_read( fname_in, group, &p_obj );

    /*
     * Create the database.
     */
    status = mc_open( fname_out, NULL, "AwPdEn", &fid );
    if ( status != OK )
    {
        mc_print_error( "mc_open", status );
        exit( -1 );
    }

    status = mc_wrt_string( fid, "title", "Mixed elem types demo 1" );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_wrt_string (title)", status );
        exit( -1 );
    }
    
     /*
     * Create an emtpy mesh.
     */
    status = mc_make_umesh( fid, "Mixed elem mesh", 3, &mid );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_make_umesh", status );
        exit( -1 );
    }

    /*
     * Define and populate a mesh class.
     */
    status = mc_def_class( fid, mid, M_MESH, "mesh", "Mesh" );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (mesh)", status );
        exit( -1 );
    }
    status = mc_def_class_idents( fid, mid, "mesh", 1, 1 );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class_idents (mesh)", status );
        exit( -1 );
    }

    /*
     * Define and populate a material class.
     */
    status = mc_def_class( fid, mid, M_MAT, "mat", "Material" );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (mat)", status );
        exit( -1 );
    }
    status = mc_def_class_idents( fid, mid, "mat", 1, 4 );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class_idents (mat)", status );
        exit( -1 );
    }

    /*
     * Define and populate a node class.
     */
    status = mc_def_class( fid, mid, M_NODE, "node", "Nodal" );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (node)", status );
        exit( -1 );
    }
    status = mc_def_nodes( fid, mid, "node", 1, 24, (float *) ncoords1 );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_nodes (1,24)", status );
        exit( -1 );
    }
    status = mc_def_nodes( fid, mid, "node", 25, 29, (float *) ncoords2 );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_nodes (25,29)", status );
        exit( -1 );
    }
    status = mc_def_nodes( fid, mid, "node", 30, 35, (float *) ncoords3 );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_nodes (30,35)", status );
        exit( -1 );
    }

    /*
     * Define and populate a shell class.
     */
    status = mc_def_class( fid, mid, M_QUAD, "s", "Shell" );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (s)", status );
        exit( -1 );
    }
    status = mc_def_conn_seq( fid, mid, "s", 1, 5, (int *) shells );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_conn_seq (s)", status );
        exit( -1 );
    }

    /*
     * Define and populate a tet class.
     */
    status = mc_def_class( fid, mid, M_TET, "t", "Tet" );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (t)", status );
        exit( -1 );
    }
    status = mc_def_conn_seq( fid, mid, "t", 1, 4, (int *) tets );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_conn_seq (t)", status );
        exit( -1 );
    }

    /*
     * Define and populate a hex class.
     */
    status = mc_def_class( fid, mid, M_HEX, "h", "Brick" );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (h)", status );
        exit( -1 );
    }
    status = mc_def_conn_seq( fid, mid, "h", 1, 5, (int *) hexs );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_conn_seq (h)", status );
        exit( -1 );
    }

    /* State rec */
    status = mc_open_srec( fid, mid, &sid );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_open_srec", status );
        exit( -1 );
    }

    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_svars", status );
        exit( -1 );
    }
 
    dbid = analy.db_ident;
    fam_in = fam_list[dbid];

    status = mc_close( dbid );
    if ( status != 0 )
    {
         mc_print_error( "mc_close", status );
         exit( -1 );
    }

    exit( OK );


   /* All the geometry data has been written out */

}


/************************************************************
 * TAG( scan_args ) LOCAL
 *
 * Parse the reader program's command line arguments.
 */
static void
scan_args( int argc, char *argv[] )
{
    int i;
    int inname_set=FALSE, outname_set=FALSE;
;
 
    for ( i = 1; i < argc; i++ )
    {
        if ( strcmp( argv[i], "-i" ) == 0 )
        {
            i++;
            strcpy( fname_in, argv[i] );
            inname_set = TRUE;
        }

        if ( strcmp( argv[i], "-o" ) == 0 )
        {
            i++;
            strcpy( fname_out, argv[i] );
            outname_set = TRUE;
        }
    }  

    if ( !inname_set || !outname_set || argc==1 )
    {
         usage();
         exit(1);
    }
}


/************************************************************
 * TAG( usage ) LOCAL
 *
 * Write out command-line syntax.
 */
static void
usage( void )
{
    printf("\n" );
    printf("Usage: milireader -i input_text_filename -o output_base_name\n" );
    printf("\n" );
}

/* End of MiliReader.c */
