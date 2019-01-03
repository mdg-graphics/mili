/* $Id: titest.c,v 1.19 2011/06/24 20:27:43 arrighi2 Exp $ */

/* 
 * C test app for a diverse Mili database.
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
 */


#include <stdio.h>
#include <string.h>
#include "mili.h"
#include "mili_internal.h"

extern Mili_family **fam_list;

#define MAX_RNAME_LEN (32)
#define MAX_SVARS (40)

/* #define FLUSH */

#define LABELS 1
/* #define TEST_IO_STORE 1 */
#define WRITE 

char *nnames[] = 
{
    "nodpos", "ux", "uy", "uz"
};
char *ntitles[] = 
{
    "Node Position", "X Position", "Y Position", "Z Position"
};

char *names[] = 
{
    "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9"
};
char *titles[] = 
{
    "Var 0", "Var 1", "Var 2", "Var 3", "Var 4", "Var 5", 
    "Var 6", "Var 7", "Var 8", "Var 9"
};
int types[] = 
{ 
    M_FLOAT, M_FLOAT, M_FLOAT, M_FLOAT, M_FLOAT, M_FLOAT, M_FLOAT, M_FLOAT, 
    M_FLOAT, M_FLOAT, M_INT 
};

char *rnames[] =
{
        "mat", "dispx", "dispy", "dispz", "dispmag",
        "velx", "vely", "velz", "velmag", "accx",
        "accy", "accz", "accmag", "temp", "pint",
        "hel", "pdev1", "pdev2", "pdev3", "maxshr",
        "prin1", "prin2", "prin3", "pdstrn1", "pdstrn2",
        "pdstrn3", "pshrstr", "pstrn1", "pstrn2", "pstrn3",
        "relvol", "res1", "res2", "res3", "res4",
        "res5", "res6", "res7", "res8", "thick",
        "inteng", "surf1", "surf2", "surf3", "surf4",
        "surf5", "surf6", "eff1", "eff2", "effmax",
        "sx", "sy", "sz", "sxy", "syz",
        "szx", "seff", "ex", "ey", "ez",
        "exy", "eyz", "ezx", "eeff", "press",
        "axfor", "sshear", "tshear", "smom", "tmom",
        "tor", "saep", "saem", "taep", "taem"
};

int n_labels[35];

float ncoords1[][3] = {                                    /* nodes 0:23 */
    {0, 0, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 1}, 
    {1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}, 
    {2, 0, 0}, {2, 1, 0}, {2, 1, 1}, {2, 0, 1}, 
    {3, 0, 0}, {3, 1, 0}, {3, 1, 1}, {3, 0, 1}, 
    {4, 0, 0}, {4, 1, 0}, {4, 1, 1}, {4, 0, 1}, 
    {5, 0, 0}, {5, 1, 0}, {5, 1, 1}, {5, 0, 1}
};
float ncoords2[][3] = {                                    /* nodes 24:28 */
    {.5, .5, 2}, {1.5, .5, 2}, {2.5, .5, 2}, {3.5, .5, 2}, {4.5, .5, 2}
};
float ncoords3[][3] = {                                    /* nodes 29:34 */
    {0, -1, 0}, {1, -1, 0}, {2, -1, 0}, {3, -1, 0}, {4, -1, 0}, {5, -1, 0}
};

#ifdef OLDSHELL
int shells[][6] = {                                        /* shells 0:4 */
    {0, 4, 5, 1, 1, 1}, {4, 8, 9, 5, 1, 1}, {8, 12, 13, 9, 1, 1}, 
    {12, 16, 17, 13, 1, 1}, {16, 20, 21, 17, 1, 1}
};
#endif
int shells[][6] = {                                        /* shells 0:4 */
    {1, 5, 6, 2, 1, 1}, {5, 9, 10, 6, 1, 1}, {9, 13, 14, 10, 1, 1}, 
    {13, 17, 18, 14, 1, 1}, {17, 21, 22, 18, 1, 1}
};

#ifdef OLDHEX
int hexs[][10] = {                                         /* bricks 0:4 */
    {0, 4, 5, 1, 3, 7, 6, 2, 2, 1}, {4, 8, 9, 5, 7, 11, 10, 6, 2, 1}, 
    {8, 12, 13, 9, 11, 15, 14, 10, 2, 1}, 
    {12, 16, 17, 13, 15, 19, 18, 14, 2, 1}, 
    {16, 20, 21, 17, 19, 23, 22, 18, 2, 1}
};
#endif
int hexs[][10] = {                                         /* bricks 0:4 */
    {1, 5, 6, 2, 4, 8, 7, 3, 2, 1}, {5, 9, 10, 6, 8, 12, 11, 7, 2, 1}, 
    {9, 13, 14, 10, 12, 16, 15, 11, 2, 1}, 
    {13, 17, 18, 14, 16, 20, 19, 15, 2, 1}, 
    {17, 21, 22, 18, 20, 24, 23, 19, 2, 1}
};

#ifdef OLDTETS
int tets[][6] = {                                          /* tets 0:3 */
    {24, 25, 7, 6, 3, 1}, {25, 26, 11, 10, 3, 1}, {26, 27, 15, 14, 3, 1}, 
    {27, 28, 19, 18, 3, 1}
};
#endif
int tets[][6] = {                                          /* tets 0:3 */
    {25, 26, 8, 7, 3, 1}, {26, 27, 12, 11, 3, 1}, {27, 28, 16, 15, 3, 1}, 
    {28, 29, 20, 19, 3, 1}
};

int pyras[][7] = {                                         /* pyramids 0:4 */
    {3, 7, 6, 2, 24, 5, 1}, {7, 11, 10, 6, 25, 5, 1}, 
    {11, 15, 14, 10, 26, 5, 1}, {15, 19, 18, 14, 27, 5, 1}, 
    {19, 23, 22, 18, 28, 5, 1}
};
int trusses[][4] = {                                       /* trusses 0:3 */
    {25, 26, 4, 1}, {26, 27, 4, 1}, {27, 28, 4, 1}, {28, 29, 4, 1}
};
#ifdef HAVE_BEAMS
/* These beams need a third node to be beams. */
int beams[][4] = {                                         /* beams 0:3 */
    {24, 25, 4, 1}, {25, 26, 4, 1}, {26, 27, 4, 1}, {27, 28, 4, 1}
};
#endif
int wedges[][8] = {                                        /* wedges 0:4 */
    {29, 30, 4, 0, 3, 7, 6, 1}, {30, 31, 8, 4, 7, 11, 6, 1}, 
    {31, 32, 12, 8, 11, 15, 6, 1}, {32, 33, 16, 12, 15, 19, 6, 1}, 
    {33, 34, 20, 16, 19, 23, 6, 1}
};


float st_rec[] = 
{
    /* node data */
    0, 0, 0,  0, 1, 0,  0, 1, 1,  0, 0, 1,  
    1, 0, 0,  1, 1, 0,  1, 1, 1,  1, 0, 1,  
    2, 0, 0,  2, 1, 0,  2, 1, 1,  2, 0, 1,  
    3, 0, 0,  3, 1, 0,  3, 1, 1,  3, 0, 1,  
    4, 0, 0,  4, 1, 0,  4, 1, 1,  4, 0, 1,  
    5, 0, 0,  5, 1, 0,  5, 1, 1,  5, 0, 1, 
    .5, .5, 2,  1.5, .5, 2,  2.5, .5, 2,  3.5, .5, 2,  
    4.5, .5, 2,  0, -1, 0,  1, -1, 0,  2, -1, 0,  
    3, -1, 0,  4, -1, 0,  5, -1, 0,

    /* truss data */
    150000, 500000,
    175000, 550000,
    200000, 600000,
    225000, 650000,

    /* tet data */
    10, 100, 1000, 10000, 100000,
    20, 200, 2000, 20000, 200000,
    30, 300, 3000, 30000, 300000,
    40, 400, 4000, 40000, 400000
};

int tet_labels[5]  = {10, 20, 30, 40, 50}, tet_elems[5];

int beam_labels[4]  = {10, 20, 30, 40};
int truss_labels[4] = {10, 20, 30, 40};
int shell_labels[5] = {10, 30, 40, 50};
int pyra_labels[5]  = {10, 30, 40, 50};

int hex_labels1[5]  = {10, 20, 30, 40, 50};
int hex_labels2[5]  = {60, 70, 80, 90, 100};

main( int argc, char *argv[] )
{
    int i, j;
    int mo_ids[2];
    Famid fid=0;
    char sv_names[MAX_SVARS][MAX_RNAME_LEN];
    char sv_titles[MAX_SVARS][MAX_RNAME_LEN];
    int status;
    int sid, mid;
    float time;
    int file_suffix, state_index, qty;

    int rval, val;

    int   ti_int   = 16, ti_int_array[32], *ti_int_ptr=NULL;

    float ti_float = 32,  ti_float_array[32];
    char  ti_string[128], ti_var_name[128];

    int   dims[3];

    Mili_family *fam;
     
    int write_ti=TRUE, read_ti=TRUE;

    int datatype, datalength;
    int state, matid, meshid, superclass;
    Bool_type isNodal, isMeshvar;

    /* Variables used for TI table searches */
    int  num_entries, tsize;
    int  num_items_read;
    char **wildcard_list = NULL;

    /* Labels variables */
    int num_blocks, *block_range;

    int temp_mesh_id=0;

    if ( write_ti )
    {
    printf("\n\n\n\n*********************************************  Testing TI write functions:\n");

    /* Enable TI functions */
    mc_ti_enable( fid ) ;
  
    /* Disable all file locking */
    mc_filelock_disable( ) ;

    /*
     * Create the database.
     */
    status = mc_open( "mixo", "./", "AwPdEn", &fid );
    if ( status != 0 )
    {
        mc_print_error( "mc_open", status );
        exit( -1 );
    }

    fam = fam_list[fid];


#ifdef TEST_IO_STORE
#ifdef WRITE
    for (i=0;
	 i<10;
	 i++)
    { 
        sprintf(  ti_var_name, "title %d -------------------------------------------------------------------", i );
        status = mc_wrt_string( fid, ti_var_name, "Mixed elem types demo 1" );
    }
 
    mc_close (fid);
    /*
     * Open the database.
     */
#endif

    status = mc_open( "mixo", "./", "Ar", &fid ); 
    if ( status != 0 )
    {
         mc_print_error( "mc_open(Read)", status );
         exit( -1 );
    }

    exit(1);
#endif

    status = mc_wrt_string( fid, "title", "Mixed elem types demo 1" );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_wrt_string (title)", status );
        exit( -1 );
    }
    
    for (i=0;
	 i<100;
	 i++)
    { 
        sprintf(  ti_var_name, "title %d -------------------------------------------------------------------", i );
        status = mc_wrt_string( fid, ti_var_name, "Mixed elem types demo 1" );
    }


    /*
     * Define state variables.
     */
    
    status = mc_def_vec_svar( fid, M_FLOAT, 3, nnames[0], ntitles[0], 
                            nnames + 1, ntitles + 1 );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_vec_svar (nodpos)", status );
        exit( -1 );
    }

    qty = sizeof( names ) / sizeof( names[0] );

    for ( i = 0; i < qty; i++ )
    {
        strcpy( sv_names[i], names[i] );
        strcpy( sv_titles[i], titles[i] );
    }

    status = mc_def_svars( fid, qty, (char *) sv_names, MAX_RNAME_LEN, 
                         (char *) sv_titles, MAX_RNAME_LEN, types );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_svars", status );
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
     * Test the new Time Invarant (TI) capability
     */
    status = mc_ti_undef_class( fid );
    
    isMeshvar = TRUE;
    isNodal   = TRUE;
    state     = 0;
    matid     = 0;
    status = mc_ti_def_class( fid, mid, state, matid, "M_HEX", isMeshvar, isNodal, "node", "node");
    if ( status != 0 )
    {
	 mc_close( fid );
	 mc_print_error( "mc_def_ti_class", status );
	 exit( -1 );
    } 

#ifdef LABELS
    /* Assign labels to the nodes */
    for ( i=0;
	  i<35;
	  i++)
          n_labels[i] = i*10;

    status = mc_def_node_labels( fid, mid, "node", 35, n_labels );
#endif
 
    status = mc_wrt_string( fid, "S1", "S1 Test" );

   /*
     * Define and populate a truss class.
     */
    status = mc_def_class( fid, mid, M_TRUSS, "truss", "Truss" );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (b)", status );
        exit( -1 );
    }
    status = mc_def_conn_seq_labels( fid, mid, "truss", 1, 4, truss_labels, (int *) trusses );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_conn_seq (truss)", status );
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
    status = mc_def_conn_seq_labels( fid, mid, "s", 1, 5, shell_labels, (int *) shells );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_conn_seq (s)", status );
        exit( -1 );
    }

    /*
     * Define and populate a tet class.
     */
    status = mc_def_class( fid, mid, M_TET, "Tet", "Tet" );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (Tet)", status );
        exit( -1 );
    }



#ifdef LABELS
    status = mc_def_conn_seq_labels( fid, mid, "Tet", 1, 4, 
				     tet_labels, (int *) tets );
#else
    status = mc_def_conn_seq( fid, mid, "Tet", 1, 4, (int *) tets );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_conn_seq (t)", status );
        exit( -1 );
    }
#endif

    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_conn_seq (Tet)", status );
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
    status = mc_def_conn_seq_labels( fid, mid, "h", 1, 5, hex_labels1, (int *) hexs );
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

    strcpy( sv_names[0], nnames[0] );
    mo_ids[0] = 1;
    mo_ids[1] = 35;
    status = mc_def_subrec( fid, sid, "NodeSubrec", RESULT_ORDERED, 1,
                          sv_names[0], MAX_RNAME_LEN, "node", 
                          M_BLOCK_OBJ_FMT, 1, mo_ids, 0 );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_subrec (NodeSubrec)", status );
        exit( -1 );
    }
    
    mo_ids[0] = 1;
    mo_ids[1] = 4;
    status = mc_def_subrec( fid, sid, "TrussSubrec", OBJECT_ORDERED, 2,
                          sv_names[5], MAX_RNAME_LEN, "truss", 
                          M_BLOCK_OBJ_FMT, 1, mo_ids, 0 );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_subrec (TetSubrec)", status );
        exit( -1 );
    }
    
#ifdef WANT_TETS
    mo_ids[0] = 1;
    mo_ids[1] = 4;
    status = mc_def_subrec( fid, sid, "TetSubrec", OBJECT_ORDERED, 5,
                          sv_names[1], MAX_RNAME_LEN, "Tet", 
                          M_BLOCK_OBJ_FMT, 1, mo_ids, 0 );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_subrec (TetSubrec)", status );
        exit( -1 );
    }
#endif

    mc_close_srec( fid, sid );
    if ( status != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_close_srec", status );
        exit( -1 );
    }

    /* Limit size of state files */
    status = mc_limit_filesize( fid, 1000 );

    /* Write a few records */
    for ( i = 0, time = 0.0; i < 5; time += 0.2, i++ )
    {
        status = mc_new_state( fid, sid, time, &file_suffix, &state_index );
        if ( status != 0 )
        {
            mc_close( fid );
            mc_print_error( "mc_new_state", status );
            exit( -1 );
        }
        
        status = mc_wrt_stream( fid, M_FLOAT, 105, st_rec );
        if ( status != 0 )
        {
            mc_close( fid ); 
            mc_print_error( "mc_wrt_stream", status );
            exit( -1 );
        }

        /* Truss data */
        status = mc_wrt_stream( fid, M_FLOAT, 8, st_rec + 105 );
        if ( status != 0 )
        {
            mc_close( fid );
            mc_print_error( "mc_wrt_stream", status );
            exit( -1 );
        }
    }
    
    status = mc_flush( fid, NON_STATE_DATA );

    strcpy( ti_var_name, "TI_int" );
    status = mc_ti_wrt_scalar( fid, M_INT, ti_var_name, &ti_int );
    if ( status != 0 )
    {
 	 mc_print_error( "mc_wrt_ti_scalar", status );
    } 
    
    strcpy( ti_var_name, "TI_float" );
    status = mc_ti_wrt_scalar( fid, M_FLOAT, ti_var_name, &ti_float );
    if ( status != 0 )
    {
 	 mc_print_error( "mc_wrt_ti_scalar", status );
    } 
    
    for (i=0;
	 i<100;
	 i++)
    { 
        sprintf(  ti_var_name, "TI_int-%d", i );
        status = mc_ti_wrt_scalar( fid, M_INT, ti_var_name, &ti_int );
        if ( status != 0 )
	{
	     mc_print_error( "mc_wrt_ti_scalar", status );
        } 
    }
 
    /* status = mc_flush( fid, NON_STATE_DATA ); */

    strcpy( ti_var_name, "TI_string" );
    status = mc_ti_wrt_string( fid,  (char *) ti_var_name, (char *) "Test of TI string");
    if ( status != 0 )
    {
         mc_print_error( "mc_wrt_ti_scalar", status );
    }
 
    
    dims[0] = 32;
    dims[1] = 0;
    dims[2] = 0;
    
    /* Fill array with some data */
    for (j=0;
	 j<32; 
	 j++)
         ti_int_array[j]=(j+1)*10;
    

    strcpy( ti_var_name, "TI_int_array" );
    status = mc_ti_wrt_array( fid,  M_INT, (char *) ti_var_name, 1, dims, (void *) ti_int_array);
    if ( status != 0 )
    {
	 mc_print_error( "mc_wrt_ti_array[TI_int_array]", status );
    }

    status = mc_wrt_array( fid,  M_INT, (char *) "TI_int_array[/Mesh-0/Sname-node/++/IsMeshvar-TRUE/IsNodal-TRUE/Sclass-M_HEX/Mat-0/State-0]", 1, dims, (void *) ti_int_array);

    /*
     * Close the TI Data file
     */
    status = mc_close( fid );
    if ( status != 0 )
    {
        mc_print_error( "mc_close", status );
        exit( -1 );
    }
    }

    if ( read_ti )
    {  
    printf("\nTesting TI read functions:\n");

    /* Enable TI functions */
#ifdef TI
    mc_ti_enable( fid ) ;
#endif

    /*
     * Open the database.
     */

    status = mc_open( "mixo", "./", "Ar", &fid ); 
    if ( status != 0 )
    {
         mc_print_error( "mc_open(Read)", status );
         exit( -1 );
    }

    fam = fam_list[fid];

    /* Define TI class search criteria */
    status = mc_ti_undef_class( fid );
    
    isMeshvar = TRUE;
    isNodal   = TRUE;
    state     = 0;
    matid     = 0;
    status = mc_ti_def_class( fid, mid, state, matid, "M_HEX", isMeshvar, isNodal, "node", "node");
 
    if ( status != 0 )
    {
         mc_close( fid ); 
         mc_print_error( "mc_def_ti_class", status );
         exit( -1 );
    } 

    status = mc_read_string( fid, "S1", ti_string );
    
    /* Read an integer array */

    strcpy( ti_var_name, "TI_int_array" );
    status = mc_ti_read_array( fid, (char *) ti_var_name, (void **) &ti_int_ptr, &num_items_read );
    if ( status != 0 )
    {
        mc_print_error( "mc_ti_read_array[TI_int_array]", status );
    } 
    else
    {
        if ( ti_int_ptr!=NULL )
        {
            printf("\n\t\tTI int array = ");
            for (j=0;
                 j<32;
                 j++)
                 printf("\n\t\t[%d]=%d", j, ti_int_ptr[j]);
        }
    }

    status = mc_read_param_array( fid, (char *) "TI_int_array[/Mesh-0/Sname-node/++/IsMeshvar-TRUE/IsNodal-TRUE/Sclass-M_HEX/Mat-0/State-0]", (void **) &ti_int_ptr );

    /* Read the nodal labels if they exist */

#ifdef LABELS
    /* Reset initial labels to zero */
    for ( i=0;
	  i<35;
	  i++)
          n_labels[i] = -1;

    status = mc_load_node_labels( fid, mid, "node", &num_blocks, &block_range,
				  n_labels );
    free( block_range );
 
    /* Read the element labels if they exist */

    /* Reset initial labels to zero */
    for ( i=0;
	  i<4;
	  i++)
          tet_labels[i] = -1;

    status = mc_load_conn_labels( fid, mid, "Tet", 10,
				  &num_blocks,
				  &block_range, 
				  tet_elems, tet_labels );

    free( block_range );

#endif

    /* Read an integer scalar */
    strcpy( ti_var_name, "TI_int" );
    ti_int = 0;
    status = mc_ti_read_scalar( fid, ti_var_name, &ti_int );
    if ( status != 0 )
    {
        mc_print_error( "mc_ti_read_scalar[TI_int]", status );
    } 
    else  
        printf("\n\t\tint=%d\n", ti_int);

    /* Read an float scalar */
    strcpy( ti_var_name, "TI_float" );
    ti_float = 0.;
    status = mc_ti_read_scalar( fid, ti_var_name, &ti_float );
    if ( status != 0 )
    {
        mc_print_error( "mc_ti_read_scalar[TI_float]", status );
    } 
    else
        printf("\n\t\tti float=%e\n", ti_float);


    /* Read a string */
    strcpy( ti_var_name, "TI_string" );
    ti_string[0] = (char) NULL;
    status = mc_ti_read_string( fid,  (char *) ti_var_name,  (char *) ti_string );
    if ( status != 0 )
    {
        mc_print_error( "mc_ti_read_scalar[TI_string]", status );
    } 
    else
        printf("\n\t\tti string=/%s/\n", ti_string);


    /* Check for variables in the TI hash table 
     *
     */
    
    tsize = fam->ti_param_table->size;
    
    wildcard_list = (char **) malloc(tsize*sizeof(char *));
    num_entries = htable_search_wildcard(fam->ti_param_table, 0, FALSE, 
					 "*", "NULL", "NULL",
					 wildcard_list );
    
    for ( i=0;
	  i<num_entries;
	  i++)
          printf("\nTI var = %s:", wildcard_list[i]);

    htable_delete_wildcard_list( num_entries, wildcard_list ) ;

    printf("\n\n");

    meshid = 0;

    /* Define the Superclass that will be used for the search */
    status = mc_ti_def_class( fid, mid, (int) -1, (int) -1, "M_HEX", TRUE, TRUE, 
			      "node", "node");

    status = mc_ti_get_data_attributes( fid, mid, "TI_int_array", "node", &state, &matid, &superclass, &isMeshvar,
			                &isNodal, &datatype, &datalength);

    printf("\n\nTesting get attributes function: %s: Type=%d, Len=%d", ti_var_name, 
	    datatype, datalength);


    /* Define the Superclass that will be used for the search */
    status = mc_ti_def_class( fid, mid, (int) -1, (int) -1, "M_TET", TRUE, TRUE, 
			      "Tet", "Tet");

    status = mc_ti_get_data_attributes( fid, mid, "Element Labels", "Tet", &state, &matid, &superclass, &isMeshvar,
			                &isNodal, &datatype, &datalength);

    printf("\n\nTesting get attributes function: %s: Type=%d, Len=%d", "Element Labels", 
	    datatype, datalength);

    status = mc_close( fid );
    if ( status != 0 )
    {
        mc_print_error( "mc_close", status );
        exit( -1 );
    }
    }
    exit( 0 );
}
