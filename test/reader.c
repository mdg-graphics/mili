/* $Id: reader.c,v 1.3 2009/03/20 21:38:29 corey3 Exp $ */

/* 
 * C test application for reading any Mili database.
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

char fname_in[64], fname_out[64];

static void scan_args( int argc, char *argv[] );

static void usage( void );

extern Bool_type
mr_is_known_db( char *fname_in, Database_type *p_db_type );

main( int argc, char *argv[] )
{
    int i, j;
    int fid=0;
    int state, qty_states=1;
    float time=0.0;

    int status;

    Mili_analysis analy, analy_out;
    Mili_family *fam_in, *fam_out;
     
    int datatype, datalength;
    int matid, meshid, superclass;
    Bool_type isNodal, isMeshvar;

    /* Variables used for TI table searches */
    int  num_entries, tsize;
    int  num_items_read;

    int temp_mesh_id=0;


    Database_type db_type;

    /* Scan command-line arguments, other initialization. */
    scan_args( argc, argv );
 
    mc_ti_enable(fid);


    /* Determine database type - Mili or */

    mr_is_known_db( fname_in, &db_type );

    if (db_type==MILI_DB_TYPE)
        printf("Opening Mili Database: %s", fname_in);
    else
        printf("Opening Taurus Database: %s", fname_in);

    status = mr_open_input_dbase( fname_in, &analy );

    fid = 0;
    fam_in = fam_list[fid];
	
    analy.mesh_table=NULL;
	
    status = mc_read_non_state_data( &analy ); 
	
    status = mc_query_family( fid, QTY_STATES, NULL, NULL,
			      &qty_states );
	
    analy.state_times = (int *) malloc(qty_states*sizeof(int));
    
    for (i=0;
	 i<qty_states;
	 i++)
    {
         state = i+1;
	 status = mc_query_family( fid, STATE_TIME, (void *) &state, NULL, 
				   (void *) &time );
	 
	 analy.state_times[i] = time;
	 
	 status = mc_read_state_data( i, &analy );
    }
    
    status = mc_close( fid );
    if ( status != 0 )
    {
	 mc_print_error( "mc_close", status );
	 exit( -1 );
    }


    /* Now write out what we just read in */

    status = mc_open_output_dbase( fname_out, &analy_out );

    fid = 1;
    fam_in = fam_list[fid];
	
    status = mili_write_non_state_data( &analy ); 

    status = mc_close( fid );
    if ( status != 0 )
    {
	 mc_print_error( "mc_close", status );
	 exit( -1 );
    }
    exit( OK );
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
 
    for ( i = 1; i < argc; i++ )
    {
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
    printf("Mili Reader Usage: reader -i <input_base_name> -o ,output_base_name>\n" );

}
