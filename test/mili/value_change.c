/* $Id: value_change.c,v 1.1 2021/07/23 16:03:03 rhathaw Exp $ */

/*
 * C test app for a diverse Mili database.
 * 

 ************************************************************************
 * Modifications:
 *  I. R. Corey - February 19, 2010: Added tests for file size and state
 *  limits.
 *  SCR #663
 *
 *  I. R. Corey - March 02, 2011: Added tests for TH write with no
 *  nodal positions in state records.
 ************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include "mili_internal.h"
#include "mili.h"

#define MAX_RNAME_LEN (64)
#define MAX_SVARS (40)

#define MAX_STATES    5
#define TH 1

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
    "temp"
};
char *titles[] = 
{
    "temperature"
};
int types[] = 
{ 
    M_FLOAT
};

char *rnames[] =
{
        "mat", "temp"
};



char *particle_short_names[] ={"node_temp"};
char *particle_long_vars[] = {"Nodal Temperature"};



//static int create_mesh( int fid, int *mid );


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
  
    /* Added test for really long filenames */
    char fname[1000];
    //strcpy(fname, "rgrido.mod.plt_c");
    strcpy(fname, "test");

    /*
    * Create the Mili database.
    */
    stat = mc_open( fname, ".", "AwPd", &fid );
    if ( stat != 0 )
    {
        mc_print_error( "mc_open", stat );
        exit( -1 );
    }
    
    

    mc_limit_filesize( fid, 10000000 );
    mc_limit_states( fid, 10000 );
    stat = mc_wrt_string( fid, "title", "Change Value test" );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_wrt_string (title)", stat );
        exit( -1 );
    }
    //stat = create_mesh( fid, &mid );
    stat=0;

    float ncoords1[][3] = {                                    /* nodes 0:23 */
        {0, 0, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 1}, 
        {1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}, 
        {2, 0, 0}, {2, 1, 0}, {2, 1, 1}, {2, 0, 1}, 
        {3, 0, 0}, {3, 1, 0}, {3, 1, 1}, {3, 0, 1}, 
        {4, 0, 0}, {4, 1, 0}, {4, 1, 1}, {4, 0, 1}, 
        {5, 0, 0}, {5, 1, 0}, {5, 1, 1}, {5, 0, 1}
    };

    /*
     * Create an emtpy mesh.
     */
    stat = mc_make_umesh( fid, "Mixed elem mesh", 3, &mid );
    
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_make_umesh", stat );
        exit( -1 ); 
    }

    /*
     * Define and populate a mesh class.
     */
    stat = mc_def_class( fid, mid, M_MESH, "mesh", "Mesh" );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (mesh)", stat );
        exit( -1 );
    }
    stat = mc_def_class_idents( fid, mid, "mesh", 1, 1 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class_idents (mesh)", stat );
        exit( -1 );
    }

    /*
     * Define and populate a material class.
     */
    stat = mc_def_class( fid, mid, M_MAT, "mat", "Material" );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (mat)", stat );
        exit( -1 );
    }
    stat = mc_def_class_idents( fid, mid, "mat", 1, 1 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class_idents (mat)", stat );
        exit( -1 );
    }
    /*
     * Define and populate a node class.
     */
    stat = mc_def_class( fid, mid, M_NODE, "node", "Nodal" );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (node)", stat );
        exit( -1 );
    }
    stat = mc_def_nodes( fid, mid, "node", 1, 24, (float *) ncoords1 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_nodes (1,24)", stat );
        exit( -1 );
    }
    
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

    stat = mc_def_svars( fid, qty, names[0], MAX_RNAME_LEN, 
                         titles[0], MAX_RNAME_LEN, types );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_svars", stat );
        exit( -1 );
    }
    
    stat = mc_def_class( fid, mid, M_PARTICLE, "node_contact", "Contact Nodes" );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_class (s)", stat );
        exit( -1 );
    }
    int contact_nodes[] = { 2,1,1,  3,1,1,  5,1,1,
                            6,1,1,  8,1,1,  9,1,1,
                            15,1,1, 16,1,1, 17,1,1,
                            18,1,1, 20,1,1, 21,1,1,
                            22,1,1, 23,1,1, 24,1,1};
    stat = mc_def_conn_seq( fid, mid, "node_contact", 1, 15, 
                            (int *) contact_nodes );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_conn_seq (contact_nodes)", stat );
        exit( -1 );
	}
       
    
    stat = mc_open_srec( fid, mid, &sid );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_open_srec", stat );
        exit( -1 );
    }
    
    strcpy( sv_names[0], nnames[0] );
    mo_ids[0] = 1;
    mo_ids[1] = 24;
    stat = mc_def_subrec( fid, sid, "NodeSubrec", OBJECT_ORDERED, 1,
                          sv_names[0], MAX_RNAME_LEN, "node", 
                          M_BLOCK_OBJ_FMT, 1, mo_ids, 0 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_subrec (NodeSubrec)", stat );
        exit( -1 );
    }
    mo_ids[0] = 1;
    mo_ids[1] = 7;
    stat = mc_def_subrec( fid, sid, "Boundary1", OBJECT_ORDERED, 1,
                          names[0], MAX_RNAME_LEN, "node_contact", 
                          M_BLOCK_OBJ_FMT, 1, mo_ids, 0 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_subrec (Boundary1)", stat );
        exit( -1 );
    }
    mo_ids[0] = 8;
    mo_ids[1] = 15;
    stat = mc_def_subrec( fid, sid, "Boundary2", OBJECT_ORDERED, 1,
                          names[0], MAX_RNAME_LEN, "node_contact", 
                          M_BLOCK_OBJ_FMT, 1, mo_ids, 0 );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_def_subrec (Boundary2)", stat );
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
    float init_temps[] = { 0.0,0.0,0.0,0.0,0.0,0.0,0.0};
                           
    float init_temps1[] = { 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
    time = 0.0;                       
    stat = mc_new_state( fid, sid, time, &file_suffix, &state_index );
    if ( stat != 0 )
    {
        mc_close( fid );
        mc_print_error( "mc_new_state", stat );
        exit( -1 );
    }
    mc_wrt_subrec(fid, "NodeSubrec", 1,24, ncoords1);
    mc_wrt_subrec(fid, "Boundary1", 1,7, init_temps);
    mc_wrt_subrec(fid, "Boundary2", 1,8, init_temps1);
    mc_end_state(fid,sid);
    
    for ( i = 1, time = 0.1; 
	  i < MAX_STATES; 
	  time += 0.1, i++ )
    {
        stat = mc_new_state( fid, sid, time, &file_suffix, &state_index );
        if ( stat != 0 )
        {
            mc_close( fid );
            mc_print_error( "mc_new_state", stat );
            exit( -1 );
        }
        
        for(j=0; j<24;j++)
        {
            ncoords1[j][0] += 0.01;
            ncoords1[j][1] += 0.01;
            ncoords1[j][2] += 0.02;
            
        }
        
        for( j=0; j < 7; j++)
        {
            init_temps[i] +=.01;
        }
        for(j = 0; j < 8; j++)
        {
            init_temps1[i] +=.02;
        }
        mc_wrt_subrec(fid, "NodeSubrec", 1,24, ncoords1);
        
        mc_wrt_subrec(fid, "Boundary1", 1,7, init_temps);
        mc_wrt_subrec(fid, "Boundary2", 1,8, init_temps1);
        
        
        mc_end_state(fid,sid);
        
        

    } 
    
    mc_close(fid);
    /* Open */
    stat = mc_open( fname, ".", "AaPdEn", &fid );
    if ( stat != 0 )
    {
        mc_print_error( "mc_open", stat );
        exit( -1 );
    }

     
    /* Test multiple value changes - Read/write multiple values for state variable "temp" */

    int start = 1;
    int stop = 7;
     
    float *results4 = malloc(sizeof(float) * (stop-start+1));
    if ( results4 == NULL )
      {
	return ALLOC_FAILED;
      }
     
    char *result_chars3[] = {names[0]};
     
    stat = mc_read_results(fid, 1, 1, 1, result_chars3, (void*)results4); // fid, state, subrec_id, qty, **results, *data
    if ( stat != 0 )
      {
	mc_print_error( "mc_read_results", stat );
	exit( -1 );
      }

    float *sr;
    sr = malloc(sizeof(float) * (stop-start+1));
    if ( sr == NULL )
      {
	return ALLOC_FAILED;
      }
     
    sr[0] = 120000;
    sr[1] = 130000;
    sr[2] = 140000;
    sr[3] = 150000;
    sr[4] = 160000;
    sr[5] = 170000;
    sr[6] = 180000;

    stat = mc_rewrite_subrec( fid, "Boundary1", start, stop, sr, 1 );
    if ( stat != 0 )
      {
	mc_close( fid );
	mc_print_error( "rewrite_subrec", stat );
	exit( -1 );
      }

    stat = mc_flush( fid, STATE_DATA );
    if ( stat != 0 )
      {
        mc_close( fid );
        mc_print_error( "mc_flush", stat );
        exit( -1 );
      }

    float *results5 = malloc(sizeof(float) * (stop-start+1));
    if ( results5 == NULL )
      {
	return ALLOC_FAILED;
      }
    
    stat = mc_read_results(fid, 1, 1, 1, result_chars3, (void*)results5);
    if ( stat != 0 )
      {
	mc_print_error( "mc_read_results", stat );
	exit( -1 );
      }

    int passed = 1;
    for ( i = 0; i < stop; i++ )
      {
	      if ( results5[i] != sr[i] )
	  {
	      passed = 0;
	  }
      }
    
    printf("Change Value Test - mc_rewrite_subrecord(): %s\n", passed==1 ? "Passed" : "Failed");

    free(results4);
    free(sr);
    free(results5);
    
    stat = mc_close( fid );
   
    exit( 0 );
}

