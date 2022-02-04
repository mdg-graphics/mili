/* The tests the capabiliy of the Mili Library to restart at state 1
 *
 * Writes out 5 states, closes file, reopens file and restarts at state 0.
 * This truncates/deletes the state file since it contains no data.
 *
 */
#include <stdio.h>
#include <string.h>
#include "mili_internal.h"
#include "mili.h"

#define MAX_RNAME_LEN (64)
#define MAX_SVARS (40)

#define MAX_STATES    5
#define RESTART_STATE 0

/*****************************************************************
 * TAG( fam_list )
 *
 * Dynamically allocated array of pointers to all currently open
 * MILI families.
 */
extern Mili_family **fam_list;
 

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
    M_FLOAT, M_FLOAT 
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

char *vnames[] =
{
        "sx", "sy", "sz", "sxy", "syz",
        "szx", "eps"
};
char *vtitles[] =
{
        "SX", "SY", "SZ", "SXY", "SYX",
        "SZX", "EPS"
};

int shells[][6] = {                                        /* shells 0:4 */
    {1, 5, 6, 2, 1, 1}, {5, 9, 10, 6, 1, 1}, {9, 13, 14, 10, 1, 1}, 
    {13, 17, 18, 14, 1, 1}, {17, 21, 22, 18, 1, 1}
};
int shell_labels[] = {1, 2, 3, 4, 5};


int hexs[][10] = {                                         /* bricks 0:4 */
    {1, 5, 6, 2, 4, 8, 7, 3, 2, 1},
    {5, 9, 10, 6, 8, 12, 11, 7, 2, 1}, 
    {9, 13, 14, 10, 12, 16, 15, 11, 2, 1}, 
    {13, 17, 18, 14, 16, 20, 19, 15, 2, 1}, 
    {17, 21, 22, 18, 20, 24, 23, 19, 2, 1}
};
int hex_labels[] = {1,2,3,4,5};

int tets[][6] = {                                          /* tets 0:3 */
    {25, 26, 8, 7, 3, 1}, {26, 27, 12, 11, 3, 1}, {27, 28, 16, 15, 3, 1}, 
    {28, 29, 20, 19, 3, 1}
};
int tet_labels[] = {1,2,3,4};

int pyras[][7] = {                                         /* pyramids 0:4 */
    {3, 7, 6, 2, 24, 5, 1}, {7, 11, 10, 6, 25, 5, 1}, 
    {11, 15, 14, 10, 26, 5, 1}, {15, 19, 18, 14, 27, 5, 1}, 
    {19, 23, 22, 18, 28, 5, 1}
};
int pyramid_labels[] = {1,2,3,4,5};

int truss_ids[4] = {1,2,3,4};
int trusses[][4] = {                                       /* trusses 0:3 */
    {25, 26, 4, 1}, {26, 27, 4, 1}, {27, 28, 4, 1}, {28, 29, 4, 1}
};

int wedges[][8] = {                                        /* wedges 0:4 */
    {29, 30, 4, 0, 3, 7, 6, 1}, {30, 31, 8, 4, 7, 11, 6, 1}, 
    {31, 32, 12, 8, 11, 15, 6, 1}, {32, 33, 16, 12, 15, 19, 6, 1}, 
    {33, 34, 20, 16, 19, 23, 6, 1}
};
int wedge_labels[] = {1,2,3,4,5};

float st_rec[] = 
{
    /* node data */
    0, 0, 0,  0, 1, 0,  0, 1, 1,  0, 0, 1, /* 12 fields */  
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
    40, 400, 4000, 40000, 400000,
    
    /* Shell stress data */
    0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1,
    0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2,
    0.3, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3,
    0.4, 0.4, 0.4, 0.4, 0.4, 0.4, 0.4,
    0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
};

static int create_mesh( int fid, int *mid );
static void writeStates( int fid, int sid, int stop_state, 
                         float start_time, float time_increment);


main( int argc, char *argv[] )
{
    int i, j, k;
    int mo_ids[2];
    Famid fid, thid;
    char sv_names[MAX_SVARS][MAX_RNAME_LEN];
    char sv_titles[MAX_SVARS][MAX_RNAME_LEN];
    int stat;
    int sid, mid;
    float time;
    int file_suffix, state_index, qty;
    int num_bytes = 0, num_domains=0;
  
    char fname[1000];
    strcpy(fname, "restart_zero.plt");

   /*
    * Create the Mili database.
    */
    stat = mc_open( fname, ".", "AwPd", &fid );
    if ( stat != 0 )
    {
        mc_print_error( "mc_open()", stat );
        exit( -1 );
    }
    
    
    mc_limit_filesize( fid, 10000000 );
    mc_limit_states( fid, 10000 );
    mc_activate_visit_file(fid,0);

    // Write out database title
    stat = mc_wrt_string(fid, "title", "Restart from Zero Test");
    if(stat != 0) {
        mc_print_error("mc_wrt_string (title)", stat);
        exit(-1);
    }
    
    /*
     * Create a mesh.
     */
    stat = create_mesh( fid, &mid );
    
    /*
     * Define state variables.
     */
    
    stat = mc_def_vec_svar( fid, M_FLOAT, 3, nnames[0], ntitles[0], 
                            nnames + 1, ntitles + 1 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_vec_svar (nodpos)", stat );
        exit( -1 );
    }
    
    qty = sizeof( names ) / sizeof( names[0] );

    for ( i = 0; i < qty; i++ )
    {
        strcpy( sv_names[i], names[i] );
        strcpy( sv_titles[i], titles[i] );
    }

    stat = mc_def_svars( fid, qty, (char *) sv_names, MAX_RNAME_LEN, 
                         (char *) sv_titles, MAX_RNAME_LEN, types );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_svars", stat );
        exit( -1 );
    }
    stat = mc_def_vec_svar( fid, M_FLOAT, 7, "stress", "Stress", 
                            vnames, vtitles );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_vec_svar (stress)", stat );
        exit( -1 );
    }

    /*
     * Define and populate a truss class.
     */
    stat = mc_def_class( fid, mid, M_TRUSS, "truss", "Truss" );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (b)", stat );
        exit( -1 );
    }
    stat = mc_def_conn_seq_labels( fid, mid, "truss", 1, 4, truss_ids, (int *) trusses );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_conn_seq (truss)", stat );
        exit( -1 );
	}
   stat = mc_def_seq_global_ids( fid, mid, "truss", 1, 1, (int *) truss_ids );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_conn_seq (truss)", stat );
        exit( -1 );
	}
   stat = mc_def_seq_global_ids( fid, mid, "truss", 2, 4, (int *) truss_ids+1 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_conn_seq (truss)", stat );
        exit( -1 );
	}
   int global_class_count=0;
   mc_set_global_class_count(fid, "truss", 4);
   mc_get_global_class_count(fid,  "truss", &global_class_count);
   global_class_count=0;
   mc_get_local_class_count(fid, "truss", &global_class_count);
   
    /*
     * Define and populate a shell class.
     */
    stat = mc_def_class( fid, mid, M_QUAD, "shell", "Shell" );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (s)", stat );
        exit( -1 );
    }
    stat = mc_def_conn_seq_labels( fid, mid, "shell", 1, 5, shell_labels, (int *) shells );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_conn_seq (s)", stat );
        exit( -1 );
	}

    /*
     * Define and populate a tet class.
     */
    stat = mc_def_class( fid, mid, M_TET, "t", "Tet" );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (t)", stat );
        exit( -1 );
    }
        
    stat = mc_def_conn_seq_labels( fid, mid, "t", 1, 4, tet_labels, (int *) tets );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_conn_seq (t)", stat );
        exit( -1 );
	}

    /*
     * Define and populate a hex class.
     */
    stat = mc_def_class( fid, mid, M_HEX, "h", "Brick" );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (h)", stat );
        exit( -1 );
    }
    stat = mc_def_conn_seq_labels( fid, mid, "h", 1, 5, hex_labels, (int *) hexs );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_conn_seq (h)", stat );
        exit( -1 );
	}

    /* State rec */
    stat = mc_open_srec( fid, mid, &sid );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_open_srec", stat );
        exit( -1 );
    }
    strcpy( sv_names[0], nnames[0] );
    mo_ids[0] = 1;
    mo_ids[1] = 35;
    stat = mc_def_subrec( fid, sid, "NodeSubrec", RESULT_ORDERED, 1,
                          sv_names[0], MAX_RNAME_LEN, "node", 
                          M_BLOCK_OBJ_FMT, 1, mo_ids, 0 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_subrec (NodeSubrec)", stat );
        exit( -1 );
    }
    
    mo_ids[0] = 1;
    mo_ids[1] = 4;
    stat = mc_def_subrec( fid, sid, "TrussSubrec", OBJECT_ORDERED, 2,
                          sv_names[5], MAX_RNAME_LEN, "truss", 
                          M_BLOCK_OBJ_FMT, 1, mo_ids, 0 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_subrec (TetSubrec)", stat );
        exit( -1 );
    }
    
    mo_ids[0] = 1;
    mo_ids[1] = 4;
    stat = mc_def_subrec( fid, sid, "TetSubrec", OBJECT_ORDERED, 5,
                          sv_names[1], MAX_RNAME_LEN, "t", 
                          M_BLOCK_OBJ_FMT, 1, mo_ids, 0 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_subrec (TetSubrec)", stat );
        exit( -1 );
    }

    mo_ids[0] = 1;
    mo_ids[1] = 5;
    stat = mc_def_subrec( fid, sid, "ShellStressSubrec", OBJECT_ORDERED, 1,
                          "stress", MAX_RNAME_LEN, "shell", 
                          M_BLOCK_OBJ_FMT, 1, mo_ids, 0 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_subrec (ShellStressSubrec)", stat );
        exit( -1 );
    }

    mc_close_srec( fid, sid );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_close_srec", stat );
        exit( -1 );
	}

    stat = mc_flush( fid, NON_STATE_DATA );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_flush", stat );
        exit( -1 );
    }
    
    writeStates( fid, sid, MAX_STATES, 0.0, 0.1);
    
    stat = mc_close( fid );
    
    if ( stat != 0 )
    {
        mc_print_error( "mc_close", stat );
        exit( -1 );
    }
    
    /* Restart Test */
    stat = mc_open( fname, ".", "AaPdEn", &fid );
    if ( stat != 0 )
    {
        mc_print_error( "mc_open", stat );
        exit( -1 );
    }

    stat = mc_restart_at_state( fid, 0, RESTART_STATE );
    
    if ( stat != 0 )
    { 
        mc_print_error( "mc_restart_at_state", stat );
        exit( -1 );
    }

    
    stat = mc_close( fid );
    if ( stat != 0 )
    {
        mc_print_error( "mc_close", stat );
        exit( -1 );
    }
    
    
    exit( 0 );
}
void writeStates( int fid, int sid, int stop_state, float start_time, float time_increment)
{  
    float time = start_time;
    int i, j, k;
    int state_index; 
    int file_suffix; 
    int stat; 
    
    for ( i = 1, time = start_time; 
	  i <= stop_state; 
	  time += time_increment, i++ )
    {
        stat = mc_new_state( fid, sid, time, &file_suffix, &state_index );
        if ( stat != 0 )
        {
            mc_close( fid );
            mc_print_error( "mc_new_state", stat );
            exit( -1 );
        }

        stat = mc_wrt_stream( fid, M_FLOAT, 105, st_rec );
        if ( stat != 0 )
        {
            mc_close( fid );
            mc_print_error( "mc_wrt_stream", stat );
            exit( -1 );
        }

        /* Truss data */
        stat = mc_wrt_stream( fid, M_FLOAT, 8, st_rec + 105 );
        if ( stat != 0 )
        {
            mc_close( fid );
            mc_print_error( "mc_wrt_stream", stat );
            exit( -1 );
        }

        /* Tet data */
	    stat = mc_wrt_stream( fid, M_FLOAT, 20, st_rec + 113 );
        if ( stat != 0 )
        {
            mc_close( fid );
            mc_print_error( "mc_wrt_stream", stat );
            exit( -1 );
        }
        /* Shell stress values */
        stat = mc_wrt_stream( fid, M_FLOAT, 35, st_rec + 133 );
        if ( stat != 0 )
        {
            mc_close( fid );
            mc_print_error( "mc_wrt_stream", stat );
            exit( -1 );
        }
    
        mc_end_state(fid,sid);

	    /* Update State Data so not constant over all time steps */
        // Update node positions
	    for ( j=0; j<105; j+=3 ) 
        {
	        st_rec[j] += .01;
	    }

        // Update Truss data 
        for ( j=105; j<113; j++ ) 
        {
            st_rec[j] += 10000;
        }

        // Update Tet data   
        for ( j=113; j<133; j++ ) 
        {
	        st_rec[j] += 10;
        }
 
        // Update Shell stresses
        for ( j=133; j<168; j++ ) 
        {
	        st_rec[j] += .1;
        }
    }
}
/*****************************************************************
 * TAG( create_mesh )
 *
 * Create a mesh and with definitions of materials and nodes.
 */
int create_mesh( int fid, int *mid )
{
int stat=0;

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

int node_labels[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35};


    /*
     * Create an emtpy mesh.
     */
    stat = mc_make_umesh( fid, "Mixed elem mesh", 3, mid );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_make_umesh", stat );
        exit( -1 ); 
    }

    /*
     * Define and populate a mesh class.
     */
    stat = mc_def_class( fid, *mid, M_MESH, "mesh", "Mesh" );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (mesh)", stat );
        exit( -1 );
    }
    stat = mc_def_class_idents( fid, *mid, "mesh", 1, 1 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class_idents (mesh)", stat );
        exit( -1 );
    }

    /*
     * Define and populate a material class.
     */
    stat = mc_def_class( fid, *mid, M_MAT, "mat", "Material" );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (mat)", stat );
        exit( -1 );
    }
    stat = mc_def_class_idents( fid, *mid, "mat", 1, 4 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class_idents (mat)", stat );
        exit( -1 );
    }
    /*
     * Define and populate a node class.
     */
    stat = mc_def_class( fid, *mid, M_NODE, "node", "Nodal" );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (node)", stat );
        exit( -1 );
    }
    stat = mc_def_nodes( fid, *mid, "node", 1, 24, (float *) ncoords1 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_nodes (1,24)", stat );
        exit( -1 );
    }
    stat = mc_def_nodes( fid, *mid, "node", 25, 29, (float *) ncoords2 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_nodes (25,29)", stat );
        exit( -1 );
    }
    stat = mc_def_nodes( fid, *mid, "node", 30, 35, (float *) ncoords3 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_nodes (30,35)", stat );
        exit( -1 );
    }

    stat = mc_def_node_labels(fid, *mid, "node", 35, (int*) node_labels);
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_nodes (30,35)", stat );
        exit( -1 );
    }

    return stat;
}
