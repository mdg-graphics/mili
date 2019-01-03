/* 
 * Milo Database Difference Tool
 * 
 *  This utility was developed to compare two Mili Databases for equality.
 *  The utility will compare two complete databases for an exact "logical"
 *  match, or do a comparison of an uncombined single processor database 
 *  against a complete "combined" database.
 *
 *************************************************************************
 *
 * Modifications:
 *
 *  I. R. Corey - September 12, 2011: Created.
 *
 *  I. R. Corey - October 26, 2012: Modified. Added checking for TI data.
 *
 *  I. R. Corey - February 25, 2013: Modified. Made various performance
 *                improvements.
 *
 *  I. R. Corey - March 5, 2013: Modified. Changed TI label check from 
 *                an error on failure to a warning.
 *
 *************************************************************************
 */

#define DEBUG 1

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "mili.h"
#include "mr.h"
#include "mili_enum.h"

/*****************************************************************
 * TAG( fam_list )
 *
 * Dynamically allocated array of pointers to all currently open
 * MILI families.
 */
extern Mili_family **fam_list;


#define QTY_ELEM_SCLASSES 12
static int elem_sclasses[] = 
{
        M_NODE,
        M_UNIT,
        M_TRUSS,
        M_BEAM,
        M_TRI,
        M_QUAD,
        M_TET,
        M_PYRAMID,
        M_WEDGE,
        M_HEX,
	M_PARTICLE,
	M_MAT,
	M_MESH
	};

#define X 0
#define Y 1
#define Z 2

/* Error message definitions */
#define NON_ELEM_CLASS 100
#define INVALID_LABEL -1
#define INVALID_ID    -2
#define INVALID_CLASS -10
#define MISSING_LABELS -20

#define NOT_OK -1

#define MAX_REPORTED_ERRORS 10

typedef struct _labels
{
   int id, label_num;
} Label_type;

typedef struct _db_class
{
  char *name, *long_name;
  int  sclass;
  int conn_cnt;
  int *connects;
  int  num_elems;
  int  max_label;
  int  *id_index;
  Label_type *labels;
} Class_type;

typedef struct _db_svar
{
  char *name;
  char *long_name;
  int num_type;
  int vec_size;
  int rank;
  int num_components;
  char **components;
  char **comp_titles;
} Svar_type;

typedef struct _db_toc
{
   int num_states;
   int num_nodes, num_elems;
   int *node_labels;
   Label_type *node_labels_index;
   int num_classes;
   Class_type *classes;
   int  num_svars;
   Svar_type *svars;
} TOC_type;

typedef struct _db_stats
{
   int num_states_checked;
   int num_nodes_checked, num_elems_checked;
   int num_classes_checked;
   int  num_svars_checked;
   int  num_fps_checked;
} Stats_type;

/* The following a return error types for a no match on various
 * types of Mili data.
 */
#define NOMATCH_NODE_LABEL  100
#define NOMATCH_COORD  101
#define NOMATCH_CLASS  102
#define NOMATCH_SVAR   103
#define NOMATCH_RESULT 104
#define NOMATCH_ELEM_LABEL 105
#define NOMATCH_ELEM_PROPS 106
#define NOMATCH_ELEM_CONN  107
#define NOMATCH_NUM_STATES  108
#define NOMATCH_RESULT_ID 109
#define NOMATCH_TI_FIELD_NAME 110
#define NOMATCH_TI_FIELD_LEN 111
#define NOMATCH_TI_FIELD_TYPE 112
#define NOMATCH_TI_FIELD_VALUE 113
  
static Return_value scan_args( int argc, char *argv[], int last_state );
static inline Bool_type compare_fp_digits( float fp1, float fp2, int num_sig_digits );
static void usage( void );
static int strip_path( char *root, char *path );

/* Mili DB compare functions */
static int compare_nodes( char *f1, char *f1_path, char *f2, char *f2_path,
			  char *fout, int *error_id );
static int compare_elements( char *f1, char *f1_path, char *f2, char *f2_path,
			     char *fout, int *error_class,  int *error_id );
static int compare_classes( char *f1, char *f1_path, char *f2, char *f2_path,
			    char *fout, int *error_id );
static int compare_svars( char *f1, char *f1_path, char *f2, char *f2_path,
			  char *fout, int *error_id );
static int compare_ti_data( char *f1, char *f1_path, char *f2, char *f2_path,
			    Bool_type subset, char *fout, int *error_id );

static inline void output_message( const char *string, FILE *fp, const Bool_type quiet_mode );

static inline void output_stats( Stats_type *stats );

static void failure_debug_bp( int status );
static void free_memory(TOC_type *f1TOC, TOC_type *f2TOC);

/* Label list-building searching functions */
static inline int get_class_label( TOC_type *fileTOC, char *class, int id_num, int *label );
static inline int get_label_id( Label_type *label, int qty, int label_num );
static inline int sort_compare_labels( const void *label1, const void *label2 );
static inline int bsearch_compare_labels( const int *key, const Label_type *label );

#define MAX_NAME 256

FILE *fpout=NULL;

char f1[MAX_NAME]="", f2[MAX_NAME]="", fout[MAX_NAME]="";
char f1_path[MAX_NAME]="", f2_path[MAX_NAME]="";

char class_name[64];

Bool_type f1_set=FALSE, f2_set=FALSE, output_set=FALSE;

/* Run Mode Flags */
Bool_type subset_compare=FALSE, check_results=TRUE;
Bool_type quiet_mode=FALSE;
int num_sig_digits=10; /* Make 10 the default */

int start_proc=-1, end_proc=-1;
int start_state=-1, stop_state=-1;

char search_class[MAX_NAME]="";
char search_var[MAX_NAME]="";

TOC_type f1TOC, f2TOC;

Stats_type stats;

int max_nodes=0, max_components=1;

int dims=3;

/* These counters are needed since psegs and
 * cseg classes do not have labels. Need to
 * track number of elements processed to
 * generate labels on the fly.
 *
 * This can be removed when psegs and csegs have
 * labels.
 *
 */
int pseg_label_max=1, cseg_label_max=1;

/*
 * This variable is used to save the id of the file we are
 * comparing against so we only need to open it once.
 */
Famid fid2_saved=-1;

/*********************************************************************************
 * MiliDiff Main
 ********************************************************************************/
main( int argc, char *argv[] )
{
    int num_files=0, num_file_errors=0;
    int proc=0;
    int status=OK;
    int error_id=0;
    int num_digits=3;

    char tempname[MAX_NAME];
    char output_string[1024];
 
    char *version="13.1", *date="March 04, 2013";

    status = scan_args( argc, argv, 0 );

    if ( fout )
         fpout = fopen( fout, "w+" );

    sprintf(output_string, "\n** MiliDiff Version %s, %s **\n", version, date);
    output_message( output_string, fpout, FALSE );

    if ( start_proc>=0 && end_proc>=0 ) {
         for ( proc=start_proc;
	       proc<=end_proc;
	       proc++ ) {
  	       num_files++;

	       status = gen_par_filename ( f1, tempname, proc, num_digits );  

	       status = compare_files( tempname, f2 );
	       if ( status!= OK ) {
		    num_file_errors++;
		    sprintf(output_string, "\nError - Mili Files (f1)%s and (f2)%s did not match.\n", tempname, f2);
		    output_message( output_string, fpout, FALSE );
		         
	}
      }
   }
   else {
        status = compare_files( f1, f2 );
	if ( status!= OK ) {
	     sprintf(output_string, "\nError - Mili Files (f1)%s and (f2)%s did not match.\n", f1, f2);
	     output_message( output_string, fpout, FALSE );
	     num_file_errors++;
	}
   }

    if ( num_file_errors>0 ) {
         sprintf(output_string, "\n------------------------------------------\nError! - %d of %d files did not match.\n\n",
		 num_file_errors, num_files);
         output_message( output_string, fpout, FALSE );
    }
    else {
         sprintf(output_string, "\n------------------------------------------\nSuccess! - All Mili files checked matched.\n");  
	 output_message( output_string, fpout, FALSE );
    }

    if ( fpout ) fclose( fpout );
    if ( fid2_saved>=0 )
         status = mc_close( fid2_saved );
    exit(1);
}

/************************************************************
 * TAG( compare_files ) LOCAL
 *
 * 
 */
static int
compare_files( char *f1, char *f2 )
{
    int status=OK;
    Bool_type file_match = TRUE;
    int error_class=0, error_id=0;
    char output_string[1024];
    Bool_type Warning=FALSE;

    sprintf(output_string, "\nComparing Mili Files: %s and %s\n------------------------------------------", f1, f2 );
    output_message( output_string, fpout, FALSE );

    /*
     * Initialize stats data
     */
     stats.num_states_checked = 0;;
     stats.num_nodes_checked = 0;
     stats.num_elems_checked = 0;
     stats.num_classes_checked = 0;
     stats. num_svars_checked = 0;
     stats. num_fps_checked = 0;

     status = compare_ti_data( f1, f1_path, f2, f2_path, subset_compare, fout, &error_id );
     if ( status!= OK ) {
         if ( status ==  NOMATCH_TI_FIELD_NAME ) {
	      sprintf(output_string, "\nWarning - TI field named [%s] not found in file F2\n", fout );
	      output_message( output_string, fpout, quiet_mode );
	      Warning = TRUE;
	 }

         if ( status ==  NOMATCH_TI_FIELD_LEN ) {
	      sprintf(output_string, "\nError - TI field named [%s] does not match in length to the field in file F2\n", fout );
	      output_message( output_string, fpout, quiet_mode );
	 }

         if ( status ==  NOMATCH_TI_FIELD_TYPE ) {
	      sprintf(output_string, "\nError - TI field named [%s] does not match in datatype to the field in file F2\n", fout );
	      output_message( output_string, fpout, quiet_mode );
	 }

         if ( status ==  NOMATCH_TI_FIELD_VALUE ) {
	      sprintf(output_string, "\nError - TI field named [%s] does not match at array position [%d] to the data in file F2\n", fout, error_id );
	      output_message( output_string, fpout, quiet_mode );
	 }
         if ( !Warning ) {
              sprintf(output_string, "\nTI Fields Error" );
	      output_message( output_string, fpout, FALSE );
	      file_match = FALSE;
	 } 
	 else {
	      sprintf(output_string, "\nTI Fields OK" );
	      output_message( output_string, fpout, FALSE );
	 }
    }
    else {
         sprintf(output_string, "\nTI Fields OK" );
	 output_message( output_string, fpout, FALSE );
    }

    status = compare_nodes( f1, f1_path, f2, f2_path, fout, &error_id );
    if ( status!= OK ) {
         sprintf(output_string, "\nError - Node coords failed to compare for node = %d\n", error_id );
	 output_message( output_string, fpout, quiet_mode );
	 sprintf(output_string, "\nNodes Error" );
	 output_message( output_string, fpout, FALSE );
	 file_match = FALSE;
   }
    else {
         sprintf(output_string, "\nNodes OK" );
	 output_message( output_string, fpout, FALSE );
    }

    status = compare_classes( f1, f1_path, f2, f2_path, fout, &error_class );
    if ( status!= OK ) {
         sprintf(output_string, "\nError - Classes failed to compare for class = %s\n", f1TOC.classes[error_class].name );
	 output_message( output_string, fpout, quiet_mode );
         sprintf(output_string, "\nClasses Error" );
	 output_message( output_string, fpout, FALSE );
	 file_match = FALSE;
    }
    else {
         sprintf(output_string, "\nClasses OK" );
	 output_message( output_string, fpout, FALSE );
    }

    status = compare_elements( f1, f1_path, f2, f2_path, fout, &error_class, &error_id );
    if ( status!= OK ) {
         if ( status==MISSING_LABELS ) {
	      sprintf(output_string, "\nError - Element labels are missing.\n");
	      output_message( output_string, fpout, quiet_mode );
	      return (status);
	 }
 	 else {
 	   sprintf(output_string, "\nError - Element connectivity failed to compare for class = %s, element = %d\n",f1TOC.classes[error_class].name, error_id );
	   output_message( output_string, fpout, quiet_mode );
	 }
	 sprintf(output_string, "\nElements and Connectivity Error" );	    
	 output_message( output_string, fpout, FALSE );
	 file_match = FALSE;
    } 
    else {
         sprintf(output_string, "\nElements and Connectivity OK" );	    
	 output_message( output_string, fpout, FALSE );
    }

    if ( check_results ) {
         sprintf(output_string, "\n\nChecking Results:\n");
	 output_message( output_string, fpout, quiet_mode );
	 status = compare_svars( f1, f1_path, f2, f2_path, fout, &error_id );
	 if ( status!= OK ) {
	      sprintf(output_string, "\nError - Svars failed to compare for variable = %s\n", f1TOC.svars[error_id].name );
	      output_message( output_string, fpout, quiet_mode );
	      sprintf(output_string, "\nResults Error\n" );
	      output_message( output_string, fpout, FALSE );
	      file_match = FALSE;
	 }
	 else {
	      sprintf(output_string, "\nResults OK\n" );
	      output_message( output_string, fpout, FALSE );
	 }
    }
    free_memory(&f1TOC, &f2TOC);
    if ( !file_match )
         return( NOT_OK );
    else
         return ( OK );
}

/************************************************************
 * TAG( compare_ti_data ) LOCAL
 *
 * 
 */
static int
compare_ti_data( char *f1, char *f1_path, char *f2, char *f2_path,
		 Bool_type subset_compare, char *fout, int *error_id )
{
    int i, j;
 
    Return_value status;

    int mesh_id=0;
    char **f1_ti, **f2_ti;
    int  f1_qty=0, f2_qty=0,
         f1_num_read=0, f2_num_read=0;

    Famid fid1=0, fid2=0;

    int f1_datalen=0, f1_datatype=0,
        f2_datalen=0, f2_datatype=0;

    void  *f1_data=NULL,   *f2_data=NULL;
    int   *f1_int=NULL,    *f2_int=NULL;
    float *f1_float=NULL,  *f2_float=NULL;
    char  *f1_string=NULL, *f2_string=NULL;
   
    Bool_type found=FALSE;

    *error_id = 0;

    status = mc_quick_open( f1, f1_path, "r", &fid1 );
    if ( status != 0 )
    {
         mc_print_error( "mc_quick_open failed for f1", status );
         exit( -1 );
    }

    status = mc_quick_open( f2, f2_path, "r", &fid2 );
    if ( status != 0 )
    {
         mc_print_error( "mc_quick_open failed for f2", status );
         exit( -1 );
    }
    f1_qty = mc_ti_htable_search_wildcard(fid1, 0, FALSE,
					  "*", "NULL", "NULL",
					  NULL );

    f1_ti = (char **) malloc( sizeof(char *)*f1_qty);
    f1_qty = mc_ti_htable_search_wildcard(fid1, 0, FALSE,
					  "*", "NULL", "NULL",
					  f1_ti );
    
    f2_qty = mc_ti_htable_search_wildcard(fid2, 0, FALSE,
					  "*", "NULL", "NULL",
					  NULL );

    f2_ti = (char **) malloc( sizeof(char *)*f2_qty);
    f2_qty = mc_ti_htable_search_wildcard(fid2, 0, FALSE,
					  "*", "NULL", "NULL",
					  f2_ti );

    /* Compare field definitions first - every TI field in file f1 
     * should be found in file f2.
     */
    for ( i=0;
	  i<f1_qty;
	  i++ ) {

          /* If we are comparing an uncombined file with a combined file then we
	   * may not have 1-1 matching names for Element Labels
	   * in the combined file - so skip checking for a name match.
	   */
          if ( subset_compare && !strncmp( f1_ti[i], "Element Labels", strlen("Element Labels")) )
	       continue;
          found = FALSE;
	  for ( j=0;
		j<f2_qty;
		j++ ) {
	        if ( !strcmp( f1_ti[i], f2_ti[j]) ) {
		     found = TRUE;
		     break;
		}
	  }
	  if ( !subset_compare && !found ) {
	       *error_id = i+1;
	       strcpy( fout, f1_ti[i] );
	       return( NOMATCH_TI_FIELD_NAME );
	  }
    }

    /* Now compare the values for all of the TI Fields.
     */
    for ( i=0;
	  i<f1_qty;
	  i++ ) {

         /* Ignore some TI fields releated to user/time/arch  - these may
	 *  differ between two files only because of the time and arch 
	 *  they were generated.
	 */
         if ( !strcmp( f1_ti[i], "username" )    ||
	      !strcmp( f1_ti[i], "arch name" )   ||
	      !strcmp( f1_ti[i], "date" )        ||
	      !strcmp( f1_ti[i], "code_name" )   ||
	      !strcmp( f1_ti[i], "host name" )   ||
	      !strcmp( f1_ti[i], "lib version" ) ||
	      !strcmp( f1_ti[i], "nproc" ) )
	      continue;
	 if ( subset_compare && !strncmp( f1_ti[i], "Element Labels", strlen("Element Labels") ) )
	      continue;
	 if ( subset_compare && !strncmp( f1_ti[i], "Node Labels", strlen("Node Labels") ) )
	      continue;

         /* Make sure data len and types are the same before 
	  *   checking values.
	  */
	  status = mc_ti_get_data_len( fid1, f1_ti[i],
				       &f1_datatype, &f1_datalen );     
          status = mc_ti_get_data_len( fid2, f1_ti[i],
				       &f2_datatype, &f2_datalen );     
	  
	  if ( f1_datatype != f2_datatype ) {
	       *error_id = i+1;
	       strcpy( fout, f1_ti[i] );
	       return( NOMATCH_TI_FIELD_TYPE );
	  }
	  if ( f1_datalen != f2_datalen ) {
	       *error_id = i+1;
	       strcpy( fout, f1_ti[i] );
	       return( NOMATCH_TI_FIELD_LEN );
	  }

	  switch ( f1_datatype ) {
	  case ( M_INT ):
	    if ( f1_datalen==1 ) {
	         f1_data = (int *) malloc(sizeof(int));
	         status = mc_ti_read_scalar( fid1, f1_ti[i], (void *) f1_data  );
	         f2_data = (int *) malloc(sizeof(int));
	         status = mc_ti_read_scalar( fid2, f1_ti[i], (void *) f2_data  );
	    }
	    else {
	         status = mc_ti_read_array(fid1, f1_ti[i],
					   (void **) &f1_data, &f1_num_read );
		 status = mc_ti_read_array(fid2, f1_ti[i],
					   (void **) &f2_data, &f2_num_read );
	    }

	    f1_int = (int *) f1_data;
	    f2_int = (int *) f2_data;

	    for ( j=0;
		  j<f1_datalen;
		  j++ ) {
	          if ( f1_int[j] != f2_int[j] ) {
		       *error_id = j; /* Return index of TI data */
		       strcpy( fout, f1_ti[i] );
		       free( f1_data );
		       free( f2_data );
		       return( NOMATCH_TI_FIELD_VALUE );
		  }
	    }

	    free( f1_data );
	    free( f2_data );
	    break;

	  case ( M_FLOAT ):
	    if ( f1_datalen==1 ) {
	         f1_data = (float *) malloc(sizeof(float));
	         status = mc_ti_read_scalar( fid1, f1_ti[i], (void *) f1_data  );
	         f2_data = (float *) malloc(sizeof(float));
	         status = mc_ti_read_scalar( fid2, f1_ti[i], (void *) f2_data  );
	    }
	    else {
	         status = mc_ti_read_array(fid1, f1_ti[i],
					   (void **) &f1_data, &f1_num_read );
		 status = mc_ti_read_array(fid2, f1_ti[i],
					   (void **) &f2_data, &f2_num_read );
	    }

	    f1_float = (float *) f1_data;
	    f2_float = (float *) f2_data;
	    for ( j=0;
		  j<f1_datalen;
		  j++ ) {
	          if ( f1_float[j] != f2_float[j] ) {
		       *error_id = j; /* Return index of TI data */
		       strcpy( fout, f1_ti[i] );
		       free( f1_data );
		       free( f2_data );
		       return( NOMATCH_TI_FIELD_VALUE );
		  }
	    }

	    free( f1_data );
	    free( f2_data );
	    break;

	  case ( M_STRING ):
	    f1_string = ( char *) malloc(sizeof(char)*f1_datalen);
	    f2_string = ( char *) malloc(sizeof(char)*f2_datalen);

	    status = mc_ti_read_string( fid1, f1_ti[i], f1_string );
	    status = mc_ti_read_string( fid2, f1_ti[i], f2_string );

	    if ( strcmp( f1_string, f2_string ) ) {
	         *error_id = 0; /* Return index of TI data */
		 strcpy( fout, f1_ti[i] );
		 free( f1_string );
		 free( f2_string );
		 return( NOMATCH_TI_FIELD_VALUE );
	    }
	    free( f1_string );
	    free( f2_string );
	    break;
	  }
    }
    return( OK );
}


/************************************************************
 * TAG( compare_nodes ) LOCAL
 *
 * Check node numbering and node positions at initial state.
 * 
 */
static int
compare_nodes( char *f1, char *f1_path, char *f2, char *f2_path, 
	       char *fout, int *error_id )
{
    int i, j, k, l;
 
    Return_value status;

    int mesh_id=0;

    Famid fid1, fid2;

    /* Nodal Variables */
    int f1_node_index=0;
    int f1_qty_nodes=0, f2_qty_nodes=0;
    float *f1_coords=NULL, *f2_coords=NULL;
    int   *f1_labels=NULL, *f2_labels=NULL;
    int f1_label;
    float f1_coord[3];

    int f2_node_index=0, f2_label_id=0;
    char f1_short_name[MAX_NAME], f1_long_name[MAX_NAME];
    char f2_short_name[MAX_NAME], f2_long_name[MAX_NAME];
    int f2_label;
    float f2_coord[3];
 
    int f1_block_qty=0, *f1_block_range=NULL;
    int f2_block_qty=0, *f2_block_range=NULL;

    int dims1=0, dims2=0;

    Bool_type label_match=FALSE, coord_match=FALSE;

    struct stat f1attr, f2attr;
    char ftemp[MAX_NAME];

    stat( f1, &f1attr );
    stat( f2, &f2attr );

    status = mc_quick_open( f1, f1_path, "r", &fid1 );
    if ( status != 0 )
    {
         mc_print_error( "mc_quick_open failed for f1", status );
         exit( -1 );
    }

    status = mc_quick_open( f2, f2_path, "r", &fid2 );
    if ( status != 0 )
    {
        mc_print_error( "mc_quick_open failed for f2", status );
        exit( -1 );
    }

    status = mc_query_family( fid1, QTY_DIMENSIONS, NULL, NULL, (void *) &dims1 );
    status = mc_query_family( fid2, QTY_DIMENSIONS, NULL, NULL, (void *) &dims2 );
    if ( dims1!=dims2 ) {
         return NOT_OK;
    }
    else dims = dims1;

    status = mc_get_class_info( fid1, mesh_id, M_NODE, 0, f1_short_name, f1_long_name,
				&f1_qty_nodes );

    status = mc_get_class_info( fid2, mesh_id, M_NODE, 0, f2_short_name, f2_long_name,
				&f2_qty_nodes );

    stats.num_nodes_checked = f1_qty_nodes;

    f1_coords = (float *) malloc( sizeof(float)*f1_qty_nodes*dims);
    f2_coords = (float *) malloc( sizeof(float)*f2_qty_nodes*dims);

    status = mc_load_nodes( fid1, mesh_id, f1_short_name, f1_coords );   
    status = mc_load_nodes( fid2, mesh_id, f2_short_name, f2_coords );
  
    f1_labels = (int *) malloc( sizeof(int)*f1_qty_nodes);
    f2_labels = (int *) malloc( sizeof(int)*f2_qty_nodes);
 
    f1TOC.num_nodes = f1_qty_nodes;
    f2TOC.num_nodes = f2_qty_nodes;

    if ( f1_qty_nodes>f2_qty_nodes )
         max_nodes = f1_qty_nodes;
    else
         max_nodes = f2_qty_nodes;

    f1TOC.node_labels = (int *) malloc( sizeof(int)*f1_qty_nodes);
    f2TOC.node_labels = (int *) malloc( sizeof(int)*f2_qty_nodes);
    f1TOC.node_labels_index = (Label_type *) malloc( sizeof(Label_type)*f1_qty_nodes);
    f2TOC.node_labels_index = (Label_type *) malloc( sizeof(Label_type)*f2_qty_nodes);
  
    status = mc_load_node_labels( fid1,  mesh_id, "node", 
				  &f1_block_qty, &f1_block_range, 
				  f1_labels );
    
    
    status = mc_load_node_labels( fid2,  mesh_id, "node", 
				  &f2_block_qty, &f2_block_range,
				  f2_labels );

   for ( i=0;
	  i<f1_qty_nodes;
	  i++ ) {
          f1TOC.node_labels[i] = f1_labels[i];
	  f1TOC.node_labels_index[i].id=i+1;
	  f1TOC.node_labels_index[i].label_num=f1_labels[i];
	  if ( f1_labels[i]>max_nodes )
	       max_nodes = f1_labels[i];
    }
    for ( i=0;
	  i<f2_qty_nodes;
	  i++ ) {
          f2TOC.node_labels[i] = f2_labels[i];
	  f2TOC.node_labels_index[i].id=i+1;
	  f2TOC.node_labels_index[i].label_num=f2_labels[i];
	  if ( f2_labels[i]>max_nodes )
	       max_nodes = f2_labels[i];
    }


    /* Sort the node labels for F1 */
    qsort(f1TOC.node_labels_index, f1_qty_nodes, sizeof(Label_type), 
	  sort_compare_labels);

    /* Sort the node labels for F2 */
    qsort(f2TOC.node_labels_index, f2_qty_nodes, sizeof(Label_type), 
	  sort_compare_labels);

    /* Compare the node labels and coordinates */
    for ( i=0;
	  i<f1_qty_nodes;
	  i++ ) {
          f1_label = f1_labels[i];
          f1_node_index = i*dims;
	  f1_coord[X]=f1_coords[f1_node_index];
	  f1_coord[Y]=f1_coords[f1_node_index+1];		     
	  f1_coord[Z] = 0.0;
	  if ( dims==3 )
	       f1_coord[Z]=f1_coords[f1_node_index+2];

	  label_match=FALSE;
          f2_label_id = get_label_id( f2TOC.node_labels_index, f2_qty_nodes, f1_label );
	  if ( f2_label_id==INVALID_LABEL )
               label_match=FALSE;
	  else
	  {
	       label_match=TRUE;	
	       f2_node_index = (f2_label_id-1)*dims;
	       f2_coord[X]=f2_coords[f2_node_index];
	       f2_coord[Y]=f2_coords[f2_node_index+1];		     
	       f2_coord[Z] = 0.0;
	       if ( dims==3 )
	            f2_coord[Z]=f2_coords[f2_node_index+2];
	       
	       if ( compare_fp_digits( f1_coord[X],f2_coord[X], num_sig_digits )!=TRUE ) {
		    label_match=FALSE;
		    break;
	       }
	       if ( compare_fp_digits( f1_coord[Y],f2_coord[Y], num_sig_digits )!=TRUE ) {
		    label_match=FALSE;
		    break;
	       }
	       if ( compare_fp_digits( f1_coord[Z],f2_coord[Z], num_sig_digits )!=TRUE ) {
		    label_match=FALSE;
		    break;
	       }
	       f2_node_index+=3;
#ifdef DEBUG1
	       sprintf(output_string, "\nChecking Node=%d - F1[%15.10f/%15.10f/%15.10f] F2[%15.10f/%15.10f/%15.10f]", f1_label, f1_coord[X], f1_coord[Y], f1_coord[Z],
		       f2_coord[X], f2_coord[Y], f2_coord[Z]);	    
	       output_message( output_string, fpout, quiet_mode );
#endif		    
	  }
	  if ( !label_match ) {
	       *error_id = f1_label;
	       failure_debug_bp( NOMATCH_COORD ); 
#ifndef DEBUG
               return NOMATCH_COORD;
#endif
	  }
    }

    free( f1_block_range );
    free( f2_block_range );
    free( f1_coords );
    free( f2_coords );
    free( f1_labels );
    free( f2_labels );
    
    status = mc_close( fid1 );
    status = mc_close( fid2 );

    return OK;
}

/************************************************************
 * TAG( compare_elements ) LOCAL
 *
 * Check geometry at initial state.
 *
 */
static int
compare_elements( char *f1, char *f1_path, char *f2, char *f2_path,
		  char *fout, int *error_class, int *error_id )
{
    int i, j, k;
 
    Return_value status;

    int mesh_id=0;

    Famid fid1, fid2;

    int query_args[2];

    /* Element Variables */
    int conn_count=1;
    int block_qty=0, *block_range=NULL;

    int dims1=0, dims2=0;

    int *f1_mats, *f1_parts, *f1_elems, *f1_labels;
    int *f2_mats, *f2_parts, *f2_elems, *f2_labels;

    int f1_nodes[32], f2_nodes[32];
    int f1_index=0, f2_index=0;			
    int f1_class_index=0, f2_class_index=0;
    int f1_label=0, f2_label=0;
    int f1_label_id, f2_label_id=0;
    int index = 0, f1_node_label=0, f2_node_label=0;

    char output_string[1024];

    Bool_type label_match=FALSE;
    int label_index=0;

    *error_class = 0;
    *error_id    = 0;

    status = mc_quick_open( f1, f1_path, "r", &fid1 );
    if ( status != 0 )
    {
         mc_print_error( "mc_quick_open failed for f1", status );
         exit( -1 );
    }

    status = mc_quick_open( f2, f2_path, "r", &fid2 );
    if ( status != 0 )
    {
         mc_print_error( "mc_quick_open failed for f2", status );
         exit( -1 );
    }

    stats.num_classes_checked = f1TOC.num_classes;

    for ( i=0;
	  i<f1TOC.num_classes;
	  i++ ) { 
          f1TOC.classes[i].connects = NULL; 
          if ( f1TOC.classes[i].conn_cnt>0 ) {
	       f1TOC.classes[i].connects = (int *) malloc( sizeof(int)*f1TOC.classes[i].num_elems*f1TOC.classes[i].conn_cnt);
	       f1_mats     = (int *) malloc( sizeof(int)*f1TOC.classes[i].num_elems);
	       f1_parts    = (int *) malloc( sizeof(int)*f1TOC.classes[i].num_elems);
	       f1_elems    = (int *) malloc( sizeof(int)*f1TOC.classes[i].num_elems);
	       f1_labels   = (int *) malloc( sizeof(int)*f1TOC.classes[i].num_elems);

	       status = mc_load_conns( fid1, mesh_id, f1TOC.classes[i].name, f1TOC.classes[i].connects,
				       f1_mats, f1_parts );

	       status = mc_load_conn_labels( fid1, mesh_id, f1TOC.classes[i].name, 
					     f1TOC.classes[i].num_elems,
					     &block_qty, &block_range,
					     f1_elems, f1_labels );

	       stats.num_elems_checked += f1TOC.classes[i].num_elems;

	       if ( block_qty==0 ) {
		    /* This logic will be removed when labels are added for psegs and csegs.
		     *
		     */
		    if ( !strcmp( f1TOC.classes[i].name, "pseg") || !strcmp( f1TOC.classes[i].name, "cseg") ) {
		         for ( label_index=0;
			       label_index<f1TOC.classes[i].num_elems;
			       label_index++ ) {
			       f1_elems[label_index] = label_index;
			       if ( !strcmp( f1TOC.classes[i].name, "pseg") ) {
				    f1_labels[label_index] = pseg_label_max++;
			       }
			       else
			            f1_labels[label_index] = cseg_label_max++;
			 }
			 
		    }
		    else {
		              sprintf(output_string, "\n\tError - No labels found for file = / %s class = %s. Skipping connectivity checking.", 
				      f1, f2TOC.classes[f2_class_index].name );
			      free( f1_mats ); 
			      free( f1_parts );
			      free( f1_elems );
			      free( f1_labels );
			      status = mc_close( fid1 );
			      status = mc_close( fid2 );
			      return MISSING_LABELS;
			 }
	       }
	       
	       /* Build the label indexes for fast searching */
	       f1TOC.classes[i].max_label = 0;
	       for ( j=0;
		     j<f1TOC.classes[i].num_elems;
		     j++ ) {
		     f1TOC.classes[i].labels[j].id        = j+1;
		     f1TOC.classes[i].labels[j].label_num = f1_labels[j];
		     if ( f1TOC.classes[i].labels[j].label_num > f1TOC.classes[i].max_label )
		          f1TOC.classes[i].max_label = f1TOC.classes[i].labels[j].label_num;
	       }

	       /* Sort the element labels for F1 */
	       qsort(f1TOC.classes[i].labels, f1TOC.classes[i].num_elems, sizeof(Label_type), 
		     sort_compare_labels);
	       for ( j=0;
		     j<f1TOC.classes[i].num_elems;
		     j++ ) {
		     f1TOC.classes[i].id_index[f1TOC.classes[i].labels[j].id-1] = j;
	       }

	       free( f1_mats ); 
	       free( f1_parts );
	       free( f1_elems );
	       free( f1_labels );
	  }
    }

    for ( i=0;
	  i<f2TOC.num_classes;
	  i++ ) {
          f2TOC.classes[i].connects = NULL; 
          if ( f2TOC.classes[i].conn_cnt>0 ) {
	       f2TOC.classes[i].connects = (int *) malloc( sizeof(int)*f2TOC.classes[i].num_elems*f2TOC.classes[i].conn_cnt);
	       f2_mats     = (int *) malloc( sizeof(int)*f2TOC.classes[i].num_elems);
	       f2_parts    = (int *) malloc( sizeof(int)*f2TOC.classes[i].num_elems);
	       f2_elems    = (int *) malloc( sizeof(int)*f2TOC.classes[i].num_elems);
	       f2_labels   = (int *) malloc( sizeof(int)*f2TOC.classes[i].num_elems); 

	       status = mc_load_conns( fid2, mesh_id, f2TOC.classes[i].name, f2TOC.classes[i].connects,
				       f2_mats, f2_parts );

	       status = mc_load_conn_labels( fid2, mesh_id, f2TOC.classes[i].name, 
					     f2TOC.classes[i].num_elems,
					     &block_qty, &block_range, 
					     f2_elems, f2_labels );

	       /* If no labels were found then use 1-n for labels */

	       if ( block_qty==0 ) {
		    sprintf(output_string, "\n\tError - No labels found for file = %s / class = %s. Skipping connectivity checking.", f2, f2TOC.classes[i].name );
		    output_message( output_string, fpout, quiet_mode );

		    free( f2_mats ); 
		    free( f2_parts );
		    free( f2_elems );
		    free( f2_labels ); 
		    status = mc_close( fid1 );
		    status = mc_close( fid2 );
		    return MISSING_LABELS;
	       }

	       f2TOC.classes[i].max_label = 0;
	       for ( j=0;
		     j<f2TOC.classes[i].num_elems;
		     j++ ) {
		     f2TOC.classes[i].labels[j].id        = j+1;
		     f2TOC.classes[i].labels[j].label_num = f2_labels[j];
		     if ( f2TOC.classes[i].labels[j].label_num > f2TOC.classes[i].max_label )
		          f2TOC.classes[i].max_label = f2TOC.classes[i].labels[j].label_num; 
	       }
	       /* Sort the element labels for F2 */
	       qsort(f2TOC.classes[i].labels, f2TOC.classes[i].num_elems, sizeof(Label_type), 
		     sort_compare_labels);

	       for ( j=0;
		     j<f2TOC.classes[i].num_elems;
		     j++ ) {
		     f2TOC.classes[i].id_index[f2TOC.classes[i].labels[j].id-1] = j;
	       }

	       free( f2_mats ); 
	       free( f2_parts );
	       free( f2_elems );
	       free( f2_labels );
	  }
    }

    for ( i=0;
	  i<f1TOC.num_classes;
	  i++ ) {
          f1_class_index = i;
          if ( f1TOC.classes[f1_class_index].conn_cnt>0 ) {
	       for ( j=0;
		     j<f2TOC.num_classes;
		     j++ ) {
		     f2_class_index = -1;
		     if ( !strcmp( f1TOC.classes[f1_class_index].name, f2TOC.classes[j].name ) ) {
		          f2_class_index = j;
			  break;
		     }
	       }
	       
	       /* Check connectivity of f1->f2 for equality */
	       if ( f2_class_index>=0 ) {
		       for ( j=0;
			     j<f1TOC.classes[f1_class_index].num_elems;
			     j++ ) {
			     f1_label = f1TOC.classes[f1_class_index].labels[j].label_num;
			     f1_index = j;
			     label_match = TRUE;
			     
			 f2_label_id = get_label_id( f2TOC.classes[f2_class_index].labels, f2TOC.classes[f2_class_index].num_elems, f1_label );
			 if ( f2_label_id==INVALID_LABEL )
			      label_match=FALSE;
			 
			 f2_index = f2_label_id-1;
			 
			 if ( label_match ) {
			      conn_count = f1TOC.classes[f1_class_index].conn_cnt;
			      if ( conn_count != f2TOC.classes[f2_class_index].conn_cnt ) {
				   *error_class = i;
		 		   *error_id    = f1_label;
				   failure_debug_bp( NOMATCH_ELEM_PROPS ); 
#ifndef DEBUG 
			     return NOMATCH_ELEM_PROPS;
#endif
			   }

			   f1_label_id = get_label_id( f1TOC.classes[f1_class_index].labels, f1TOC.classes[f1_class_index].num_elems, f1_label );
			   f1_index = f1_label_id-1;

			   /* Check connection points using node labels */
			   for ( k=0;
				 k<f1TOC.classes[f1_class_index].conn_cnt;
				 k++ ) {
			         index = (f1_index*conn_count) + k; 
				 f1_nodes[k] = f1TOC.classes[f1_class_index].connects[index];
			     
				 /* Convert the nodes to labels */
				 f1_node_label  = f1TOC.node_labels[f1_nodes[k]];
				 f1_nodes[k]    = f1_node_label;	
				 
			   }

			   for ( k=0;
				 k<f1TOC.classes[f1_class_index].conn_cnt;
				 k++ ) {
			         index = (f2_index*conn_count) + k;
				 f2_nodes[k] = f2TOC.classes[f2_class_index].connects[index];
				 
				 /* Convert the nodes to labels */
				 f2_node_label  = f2TOC.node_labels[f2_nodes[k]];
				 f2_nodes[k]    = f2_node_label;	
			   }
			   
			   /* Compare the connectivity using the node labels */
			   for ( k=0;
				 k<conn_count;
				 k++ ) {
			        if ( f1_nodes[k] != f2_nodes[k] ) {
				     *error_class = i;
				     *error_id    = f1_label;
				     failure_debug_bp( NOMATCH_ELEM_CONN ); 
#ifndef DEBUG
				     return NOMATCH_ELEM_CONN;
#endif
				}
			   }
			   
			 }
			 else {
			      *error_class = i;
			      *error_id = f1_label;
			      failure_debug_bp( NOMATCH_ELEM_LABEL ); 
#ifndef DEBUG
			      return NOMATCH_ELEM_LABEL;
#endif
			 }
		       }
		     }
	  } /* if ( f2_class_index>=0 ) */
    } /* for on i */

    status = mc_close( fid1 );
    status = mc_close( fid2 );

    if ( *error_class !=0 )
         return NOT_OK;

    return OK;
}


/************************************************************
 * TAG( compare_classes ) LOCAL
 *
 * Check all class definitions.
 *
 */
static int
compare_classes( char *f1, char *f1_path, char *f2, char *f2_path,
		 char *fout, int *error_id )
{
  int i, j, k;
 
    Return_value status;

    int mesh_id=0;

    Famid fid1, fid2;
    char short_name[64], long_name[128];
    int total_classes=0, qty_classes=0, obj_qty=0;
    int class_index=0;
    int query_args[2];

    Bool_type class_match = FALSE;

    *error_id = 0;

    status = mc_quick_open( f1, f1_path, "r", &fid1 );
    if ( status != 0 )
    {
         mc_print_error( "mc_quick_open failed for f1", status );
         exit( -1 );
    }

    status = mc_quick_open( f2, f2_path, "r", &fid2 );
    if ( status != 0 )
    {
         mc_print_error( "mc_quick_open failed for f2", status );
         exit( -1 );
    }

    /* Get class info for file-1 */
    f1TOC.num_classes = 0;
    f1TOC.num_svars   = 0;

    query_args[0] = mesh_id;
    for ( i=0;
	  i<QTY_ELEM_SCLASSES;
	  i++ ) {
          query_args[1] = elem_sclasses[i];
	  status = mc_query_family( fid1, QTY_CLASS_IN_SCLASS, (void *) query_args, 
    				    NULL, (void *) &qty_classes );
	  status = mc_get_class_info( fid1, mesh_id,  elem_sclasses[i], 0,
				      short_name, long_name,
				      &obj_qty );
	  total_classes += qty_classes;
	  f1TOC.num_classes = total_classes;
    }
    
    f1TOC.classes = (Class_type *) malloc(sizeof(Class_type)*total_classes); 

    for ( i=0;
	  i<QTY_ELEM_SCLASSES;
	  i++ ) {
          query_args[1] = elem_sclasses[i];
	  status = mc_query_family( fid1, QTY_CLASS_IN_SCLASS, (void *) query_args, 
				    NULL, (void *) &qty_classes );

          for ( j=0; 
		j<qty_classes;
		j++ ) {
	        status = mc_get_class_info( fid1, mesh_id,  elem_sclasses[i], j,
					    short_name, long_name,
					    &obj_qty );

		f1TOC.classes[class_index].name      = strdup(short_name);
		f1TOC.classes[class_index].long_name = strdup(long_name);
		f1TOC.classes[class_index].num_elems = obj_qty;
		f1TOC.classes[class_index].sclass    = elem_sclasses[i];
		f1TOC.classes[class_index].conn_cnt  = mc_get_conn_count( f1TOC.classes[class_index].sclass );
		f1TOC.classes[class_index].max_label = 1;
		f1TOC.classes[class_index].labels    = (Label_type *) malloc(sizeof(Label_type)*obj_qty);
		f1TOC.classes[class_index].id_index  = (int *) malloc(sizeof(Label_type)*obj_qty);

		/* Initialize Labels */
		for ( k=0;
		      k<obj_qty;
		      k++ ) {
		      f1TOC.classes[class_index].labels[k].id        = -1;
		      f1TOC.classes[class_index].labels[k].label_num = -1;
		}
		class_index++;
	  }
    }

    /* Get class info for file-2 */
    f2TOC.num_classes = 0;
    f2TOC.num_svars   = 0;
    total_classes     = 0;

    query_args[0] = mesh_id;
    for ( i=0;
	  i<QTY_ELEM_SCLASSES;
	  i++ ) {
          query_args[1] = elem_sclasses[i];
	  status = mc_query_family( fid2, QTY_CLASS_IN_SCLASS, (void *) query_args, 
    				    NULL, (void *) &qty_classes );
	  total_classes += qty_classes;
	  f2TOC.num_classes = total_classes;
    }

    f2TOC.classes = (Class_type *) malloc(sizeof(Class_type)*total_classes); 
    class_index=0;

    for ( i=0;
	  i<QTY_ELEM_SCLASSES;
	  i++ ) {
          query_args[1] = elem_sclasses[i];
	  status = mc_query_family( fid2, QTY_CLASS_IN_SCLASS, (void *) query_args, 
				    NULL, (void *) &qty_classes );

           for ( j=0; 
		 j<qty_classes;
		 j++ ) {
	         status = mc_get_class_info( fid2, mesh_id,  elem_sclasses[i], j,
					     short_name, long_name,
					     &obj_qty );
		 f2TOC.classes[class_index].name      = strdup(short_name);
		 f2TOC.classes[class_index].long_name = strdup(long_name);
		 f2TOC.classes[class_index].num_elems = obj_qty;
		 f2TOC.classes[class_index].sclass    = elem_sclasses[i];
		 f2TOC.classes[class_index].conn_cnt  = mc_get_conn_count( f2TOC.classes[class_index].sclass );
		 f2TOC.classes[class_index].labels    = (Label_type *) malloc(sizeof(Label_type)*obj_qty);
	  	 f2TOC.classes[class_index].id_index  = (int *) malloc(sizeof(Label_type)*obj_qty);

		 /* Initialize Labels */
		 for ( k=0;
		       k<obj_qty;
		       k++ ) {
		      f2TOC.classes[class_index].labels[k].id        = -1;
		      f2TOC.classes[class_index].labels[k].label_num = -1;
		 }
		 class_index++;
	   }
    }

    /* Now compare f1 to f2. Look in f2 for matchs from f1. 
     * In the case of an uncombined DB, f1 should be a 
     * subset of f2.
     */
    for ( i=0;
	  i<f1TOC.num_classes;
	  i++ ) {
          class_match = FALSE;
          for ( j=0;
	        j<f2TOC.num_classes;
	        j++ ) {
	        if ( !strcmp(f1TOC.classes[i].name, f2TOC.classes[j].name) &&
		     !strcmp(f1TOC.classes[i].long_name, f2TOC.classes[j].long_name) &&
		     f1TOC.classes[i].sclass==f2TOC.classes[j].sclass &&
		     f1TOC.classes[i].num_elems<=f2TOC.classes[j].num_elems ) 
		     class_match = TRUE;
		
		if ( class_match == TRUE ) {
		     if ( !subset_compare &&  f1TOC.classes[i].num_elems!=f2TOC.classes[j].num_elems )
		          class_match = FALSE;
		}
		if ( class_match )
		     break;
	  }
	  if ( !class_match ) {
	       *error_id = i;  
	       failure_debug_bp( NOMATCH_CLASS ); return NOMATCH_CLASS;
	  }
    }

    status = mc_close( fid1 );
    status = mc_close( fid2 );

    return OK;
}

/************************************************************
 * TAG( compare_svars ) LOCAL
 *
 * Check state variables for equality.
 * 
 */
static int
compare_svars( char *f1, char *f1_path, char *f2, char *f2_path,
	       char *fout, int *error_id )
{
    Famid fid1, fid2;

    State_variable p_sv1, p_sv2;
    Subrecord subrec;

    int staterec_id=0;
    int state=0, state_cnt=0;
    int subrec_qty=0;
    int sid=0;

    char **f1_svars=NULL, **f2_svars=NULL;
  
    int *f1_sids, *f2_sids;
    int f1_num_sids=0, f2_num_sids=0;
    int f1_num_svars=0, f2_num_svars=0;

    /* Result variables */
    int elem_qty=0;
    char class_name[MAX_NAME]="", svar_name[MAX_NAME]="";
    int class_index=0;

    int result_len=0, result_index=0, result_label=0; 
    int result_buff_index=0;

    float *f1_result=NULL, *f2_result=NULL;
    int    *result_buff_int=NULL;
    long   *result_buff_long=NULL;
    float  *result_buff_float=NULL;
    double *result_buff_dbl=NULL;
    void *p_result_buff;

    char *results[2]={NULL, NULL};

    int f1_list_count=0, f2_list_count=0;
    int *f1_list, *f2_list; 
    Bool_type *f1_ids=NULL, *f2_ids=NULL;
    
    int i=0, j=0, k=0, l=0, m=0;
    int node_index=0;
    int num_components=1;
    int max_label=0;

    Bool_type svar_match=FALSE, svar_errors=FALSE, svar_found=FALSE,
              search_class_found=FALSE;
    Bool_type elem_error=FALSE;

    int status=OK;
    int error_count_id=1, error_count_val=0;
    
    char id_string[32];
    char output_string[1024];

    status = mc_open( f1, f1_path, "r", &fid1 );
    if ( status != 0 )
    {
         mc_print_error( "mc_open failed for f1", status );
         exit( -1 );
    }

    if ( fid2_saved<0 ) {
         status = mc_open( f2, f2_path, "r", &fid2 );
	 if ( status != 0 )
	 {
	     mc_print_error( "mc_open failed for f2", status );
	     exit( -1 );
	 }
	 fid2_saved = fid2;
    }
    else fid2 = fid2_saved; /* Only open F2 one time to avoid performance issue
			     * with loading in state table.
			     */

    status = mc_query_family( fid1, QTY_STATES, NULL, NULL,
                              &f1TOC.num_states );

    status = mc_query_family( fid2, QTY_STATES, NULL, NULL,
                              &f2TOC.num_states );
    
    if ( start_state==-1 ) {
         start_state = 0;
         stop_state  = f1TOC.num_states;
    }
    if ( f1TOC.num_states>f2TOC.num_states )
         stop_state = f2TOC.num_states;

    if ( f1TOC.num_states!=f2TOC.num_states ) {
         *error_id = 0;
         failure_debug_bp( NOMATCH_NUM_STATES ); 
	 sprintf(output_string, "\nWarning - number of states in F1[%d] does not match number of states in F2[%d]", 
		f1TOC.num_states, f2TOC.num_states);
	 output_message( output_string, fpout, FALSE );
    }

    staterec_id=0;
    status = mc_get_svars( fid1, staterec_id,
			   &f1_num_svars, &f1_svars );

    status = mc_get_svars( fid2, staterec_id,
			   &f2_num_svars, &f2_svars );

    f1TOC.num_svars = f1_num_svars;
    f1TOC.svars = (Svar_type *) malloc( sizeof(Svar_type)*f1_num_svars);

    f2TOC.num_svars = f2_num_svars;
    f2TOC.svars = (Svar_type *) malloc( sizeof(Svar_type)*f2_num_svars);

    /* Check for matching svars. For a match we must find every svar
     * from file f1 in file f2.
     */

    /* Populate Svar fields for file f1. */
    for ( i=0;
	  i<f1_num_svars;
	  i++ ) {
          status = mc_get_svar_def( fid1, f1_svars[i], &p_sv1 );

	  f1TOC.svars[i].name             = strdup(f1_svars[i]);
	  f1TOC.svars[i].long_name        = strdup(p_sv1.long_name);    
	  f1TOC.svars[i].num_type         = p_sv1.num_type;
	  f1TOC.svars[i].vec_size         = p_sv1.vec_size;
	  f1TOC.svars[i].num_components   = p_sv1.vec_size;
	  f1TOC.svars[i].rank             = p_sv1.rank;
	  f1TOC.svars[i].components       = NULL;
	  f1TOC.svars[i].comp_titles      = NULL;	  
	  if ( f1TOC.svars[i].num_components > max_components )
  	       max_components = f1TOC.svars[i].num_components+1;

	  if ( f1TOC.svars[i].num_components>1 ) {
	       f1TOC.svars[i].components  = (char **) malloc( sizeof(char *)*f1TOC.svars[i].num_components );
	       f1TOC.svars[i].comp_titles = (char **) malloc( sizeof(char *)*f1TOC.svars[i].num_components );
               for ( j=0;
		     j<f1TOC.svars[i].num_components;
		     j++ ) {
		     f1TOC.svars[i].components[j]  = strdup(p_sv1.components[j]);
		     f1TOC.svars[i].comp_titles[j] = strdup(p_sv1.component_titles[j]);
	       } 
	  }
    }

    /* Populate Svar fields for file f2. */
    for ( i=0;
	  i<f2_num_svars;
	  i++ ) {
          status = mc_get_svar_def( fid1, f2_svars[i], &p_sv1 );

	  f2TOC.svars[i].name             = strdup(f2_svars[i]);
	  f2TOC.svars[i].long_name        = strdup(p_sv1.long_name);    
	  f2TOC.svars[i].num_type         = p_sv1.num_type;
	  f2TOC.svars[i].vec_size         = p_sv1.vec_size;
	  f2TOC.svars[i].rank             = p_sv1.rank;
	  f2TOC.svars[i].num_components   = p_sv1.vec_size;
	  f2TOC.svars[i].components       = NULL;
	  f2TOC.svars[i].comp_titles      = NULL;

	  if ( f2TOC.svars[i].num_components > max_components )
  	       max_components = f2TOC.svars[i].num_components+1;

	  if ( f2TOC.svars[i].num_components>1 ) {
	       f2TOC.svars[i].components  = (char **) malloc( sizeof(char *)*f2TOC.svars[i].num_components );
	       f2TOC.svars[i].comp_titles = (char **) malloc( sizeof(char *)*f2TOC.svars[i].num_components );
               for ( j=0;
		     j<f2TOC.svars[i].num_components;
		     j++ ) {
		     f2TOC.svars[i].components[j]  = strdup(p_sv1.components[j]);
		     f2TOC.svars[i].comp_titles[j] = strdup(p_sv1.component_titles[j]);
	       } 
	  }
    }

    /* Check for matching svars. For a match we must find every svar
     * from file f1 in file f2. For those found then check that all
     * of the fields match.
     *
     */
    for ( i=0;
	  i<f1_num_svars;
	  i++ ) {
          svar_match = TRUE;

          for ( j=0;
	        j<f2_num_svars;
		j++ ) {
	        if ( !strcmp( f1TOC.svars[i].name, f2TOC.svars[i].name) ) {
		     if ( !strcmp( f1TOC.svars[i].long_name, f2TOC.svars[i].long_name) &&
			  f1TOC.svars[i].num_type       == f2TOC.svars[i].num_type &&
			  f1TOC.svars[i].vec_size       == f2TOC.svars[i].vec_size &&
			  f1TOC.svars[i].num_components == f2TOC.svars[i].num_components )
		          svar_match = TRUE;
		     else
		          svar_match = FALSE;

		     if ( svar_match && f1TOC.svars[i].num_components>1 ) {
		          svar_match = FALSE;
		          for ( k=0;
				k<f1TOC.svars[i].num_components;
				k++ ) {
			        if ( !strcmp( f1TOC.svars[i].components[k],  f2TOC.svars[i].components[k] ) &&
				     !strcmp( f1TOC.svars[i].comp_titles[k], f2TOC.svars[i].comp_titles[k] ) )
				     svar_match = TRUE;
			  }
		          
		     }
		     continue;
		}
	  }
	  if ( !svar_match ) {
	       *error_id = i;
	       failure_debug_bp( NOMATCH_SVAR ); return NOMATCH_SVAR;
	  }
    }

    stats.num_svars_checked = f1_num_svars;

    /* Read and check results for each SVAR */

    for ( i=0;
	  i<f1_num_svars;
	  i++ ) {
          svar_match = TRUE;
	  svar_found = FALSE;

	  if ( strlen(search_var)>0 )
	       if ( strcmp( search_var, f1TOC.svars[i].name ) )
	            continue;

          if ( f1TOC.svars[i].num_type==M_FLOAT ) {
	       result_buff_float = (float *) malloc(sizeof(float)*max_nodes*max_components);
	       p_result_buff = result_buff_float;
	  }
	       else if ( f1TOC.svars[i].num_type==M_INT ) {
		         result_buff_int = (int *) malloc(sizeof(int)*max_nodes*max_components);
			 p_result_buff = result_buff_int;
	       }
		         else if ( f1TOC.svars[i].num_type==M_FLOAT8 ) {
			           result_buff_dbl = (double *) malloc(sizeof(double)*max_nodes*max_components);
				   p_result_buff = result_buff_dbl;
			 }
                                   else if ( f1TOC.svars[i].num_type==M_INT8 ) {
				             result_buff_long = (long *) malloc(sizeof(long)*max_nodes*max_components);
					     p_result_buff = result_buff_long;
			 }
    
          status = mc_get_subrec_ids( fid1, staterec_id, f1TOC.svars[i].name,
				      &f1_num_sids, &f1_sids );

          status = mc_get_subrec_ids( fid2, staterec_id, f1TOC.svars[i].name,
				      &f2_num_sids, &f2_sids );

	  if ( f2_num_sids==0 ) {
	       *error_id = i;
	       return (NOMATCH_SVAR);
	  }
	  num_components = f1TOC.svars[i].num_components;
	  if ( num_components==0 )
	       num_components=1;

	  for ( class_index=0;
		class_index<f1TOC.num_classes;
		class_index++ )
	  {
 		strcpy ( class_name, f1TOC.classes[class_index].name ); /* Save the class name */

	        svar_found         = FALSE;
		search_class_found = FALSE;
	        for ( k=0;
		      k<f1_num_sids;
		      k++ ) {
	              sid = f1_sids[k];
	              status = mc_get_subrec_def( fid1, staterec_id, sid, &subrec );
		      if ( strlen(search_class)>0 )
			   if ( !strcmp( search_class, subrec.class_name ) ) {
			        search_class_found = TRUE;
			}
		        if ( !strcmp( class_name, subrec.class_name ) ) {
			     svar_found = TRUE;
		      }
		      if ( svar_found && search_class_found )
			   break;
		}
                if ( !svar_found )
		     continue;
		if ( strlen(search_class)>0 && !search_class_found )
		     continue;

	        max_label = f1TOC.classes[class_index].max_label;
		if ( max_nodes>max_label )
		     max_label = max_nodes;

		elem_qty = f1TOC.classes[class_index].num_elems;

	        f1_result = (float *) malloc(sizeof(float)*max_label*(max_components));
		f2_result = (float *) malloc(sizeof(float)*max_label*(max_components));
		
		f1_list   = (int *) malloc(sizeof(int)*max_label);
		f2_list   = (int *) malloc(sizeof(int)*max_label);
		
		f1_ids   = (Bool_type *) malloc(sizeof(int)*(max_label+1));
		f2_ids   = (Bool_type *) malloc(sizeof(int)*(max_label+1));

		state_cnt=0;
		error_count_id  = 1;
 		for ( state=start_state;
		      state<stop_state; 
		      state++ ) {

		      if ( error_count_id>MAX_REPORTED_ERRORS )
		           break;

		      if ( !svar_errors && state_cnt>100 ) {
		 	   if ( state_cnt==0 )
			        printf("\n");
			   printf("\tWorking on: SVAR=%s/Class=%s @ State: %d\r", f1TOC.svars[i].name, class_name, state+1 );
			   if ( state==(stop_state-1) )
			        printf("\n");
		      }
		      state_cnt++;
		      stats.num_states_checked++;

		      error_count_val = 0;

		      for ( node_index=0;
			    node_index<max_label;
			    node_index++ ) {
			    f1_ids[node_index] = FALSE;
			    f2_ids[node_index] = FALSE;
		      }
		      
		      for ( k=0;
		            k<f1_num_sids;
			    k++ ) { 
		            sid = f1_sids[k];
			    status = mc_get_subrec_def( fid1, staterec_id, sid, &subrec );

			    if ( strlen(search_class)>0 )
			         if ( strcmp( search_class, subrec.class_name ) )
				      continue;
			    if ( strcmp( class_name, subrec.class_name ) )
				 continue;

			    if ( !strcmp("mat", subrec.class_name) )
			         strcpy( id_string, "Mat " );
			    else
			         strcpy( id_string, "Id " );

			    result_len = subrec.qty_objects*num_components;

                            mc_blocks_to_list( subrec.qty_blocks, subrec.mo_blocks,
					       &f1_list_count, f1_list, FALSE );

  		           /* Read in F1 result */

			   results[0] = strdup(f1TOC.svars[i].name);

    			   status = mc_read_results( fid1, state+1, sid, 1, results,
						     (void *) p_result_buff );

			   for ( l=0; 
				 l<f1_list_count;
				 l++ ) {
			         for ( m=0;
			               m<num_components;
				       m++ ) {
     
				       /* Convert element id to a label */
				       status = get_class_label( &f1TOC, subrec.class_name, f1_list[l], &result_label );
				       if ( status!=OK )
					    if ( status!=NON_ELEM_CLASS ) {
					         failure_debug_bp( NOMATCH_ELEM_LABEL );
					         svar_match = FALSE;
					         /* return NOMATCH_ELEM_LABEL */;
				       }

				       if ( result_label < 1 || result_label > max_label ) {
					    failure_debug_bp( INVALID_LABEL );
					    svar_match = FALSE;
					    status = get_class_label( &f1TOC, subrec.class_name, f1_list[l], &result_label );
					    continue;
					    /* return INVALID_LABEL */;
				       }

				       result_index           = ((result_label-1)*num_components)+m;
				       result_buff_index      = (l*num_components)+m;
				       f1_ids[result_label-1] = TRUE; /* Set to TRUE if this result at result_label is
								       * present in the file f1.
								       */

				       if ( result_index      < 0 || result_index      > max_label*max_components ||
					    result_buff_index < 0 || result_buff_index > max_label*max_components) {
					    failure_debug_bp( INVALID_LABEL );
					    svar_match = FALSE;
				       } 

				       if ( f1TOC.svars[i].num_type==M_INT )
					    f1_result[result_index] = (float) result_buff_int[result_buff_index];
				       else if ( f1TOC.svars[i].num_type==M_FLOAT )
					    f1_result[result_index] = (float) result_buff_float[result_buff_index];
				       else if ( f1TOC.svars[i].num_type==M_INT8 )
					    f1_result[result_index] = (float) result_buff_long[result_buff_index];
				       else if ( f1TOC.svars[i].num_type==M_FLOAT8 )
				            f1_result[result_index] = (float) result_buff_dbl[result_buff_index];
				 }
			   }  
			   free( results[0] );

#ifndef DEBUG
			   free( subrec.name );
			   free( subrec.class_name );
#endif
		      } /* f1_num_sids */

		      if ( !svar_found ) {
		           state=stop_state;
		           break; 
		      }

	             for ( k=0;
		           k<f2_num_sids;
			   k++ ) {
		           sid = f2_sids[k];

			   status = mc_get_subrec_def( fid2, staterec_id, sid, &subrec ); 

			   if ( strlen(search_class)>0 )
			        if ( strcmp( search_class, subrec.class_name ) )
			             continue;
			   if ( strcmp( class_name, subrec.class_name ) )
			        continue;
			   
			   result_len = subrec.qty_objects*num_components;

			   mc_blocks_to_list( subrec.qty_blocks, subrec.mo_blocks, 
					      &f2_list_count, f2_list, FALSE );

			   /* Read in F2 result */
			   results[0] = strdup(f1TOC.svars[i].name);

    			   status = mc_read_results( fid2, state+1, sid, 1, results,
                                   (void *) p_result_buff );

			   for ( l=0; 
				 l<f2_list_count;
				 l++ ) {
			         for ( m=0;
			               m<num_components;
				       m++ ) {

				       /* Convert element id to a label */
				       status = get_class_label( &f2TOC, subrec.class_name, f2_list[l], &result_label );
				       if ( status!=OK )
					    if ( status!=NON_ELEM_CLASS ) {
					         failure_debug_bp( NOMATCH_ELEM_LABEL );
					         svar_match = FALSE;
					         /* return NOMATCH_ELEM_LABEL */;
				       }

				       if ( result_label < 1 || result_label > max_label ) {
					    failure_debug_bp( INVALID_LABEL );
					    svar_match = FALSE;
					    status = get_class_label( &f2TOC, subrec.class_name, f2_list[l], &result_label );
					    continue;
					    /* return INVALID_LABEL */;
				       }

				       result_index      = ((result_label-1)*num_components)+m;
				       result_buff_index = (l*num_components)+m;
				       f2_ids[result_label-1] = TRUE; /* Set to TRUE if this result at result_label is
								       * present in the file f2.
								       */

				       if ( result_index      < 0 || result_index      > max_label*max_components ||
					    result_buff_index < 0 || result_buff_index > max_label*max_components) {
					    failure_debug_bp( INVALID_LABEL );
					    svar_match = FALSE;
				       }

				       if ( f1TOC.svars[i].num_type==M_FLOAT )
					    f2_result[result_index] = (float) result_buff_float[result_buff_index];
				       else if ( f1TOC.svars[i].num_type==M_INT )
					    f2_result[result_index] = (float) result_buff_int[result_buff_index];
				       else if ( f1TOC.svars[i].num_type==M_INT8 )
					    f2_result[result_index] = (float) result_buff_long[result_buff_index];
				       else if ( f1TOC.svars[i].num_type==M_FLOAT8 )
				            f2_result[result_index] = (float) result_buff_dbl[result_buff_index];
				 }
			   } 
			   free( results[0] );
#ifndef DEBUG 
			   free( subrec.name );
			   free( subrec.class_name );
#endif
		     } /* f2_num_sids */

		     /* Compare element ids - look for all f1 ids in f2 since f1 may be a
		      * subset of f2.
		      */
		     svar_match = TRUE;
		     strcpy( svar_name, f1TOC.svars[i].name); 

		     /* Compare the results for this SVAR */
		     for ( node_index=0;
			   node_index<max_label;
			   node_index++ ) {

		          /* Compare element ids  - look for all f1 ids in f2 since f1 may be a
			   * subset of f2.
			   */
		           if ( f1_ids[node_index]==TRUE && f2_ids[node_index]!=TRUE ) {
			        *error_id = i;
			        failure_debug_bp( NOMATCH_ELEM_LABEL );
				if ( error_count_id<=MAX_REPORTED_ERRORS ){
				     sprintf(output_string, "Error - No match on result ids for Class = %s, \tSvar = %s, %s= %d at \tState = %d\n", 
					     class_name, f1TOC.svars[i].name, id_string, node_index, state+1 );	 
				     output_message( output_string, fpout, FALSE );
				     error_count_id++;
				}
				svar_match       = FALSE;
				svar_errors      = TRUE;
		           }
			   elem_error = FALSE;

			   for ( m=0;
				 m<num_components;
				 m++ ) {
			         strcpy( svar_name, f1TOC.svars[i].name);

			         if ( num_components> 1) {
				      strcat( svar_name, "[" );
				      strcat( svar_name, f1TOC.svars[i].components[m] );
				      strcat( svar_name, "]" );
				 }
				 result_index = (node_index*num_components)+m;
				 if ( !f1_ids[node_index] ) /* In the case of comparing an uncombined file to a
							     * combined file not every result in the combined file
							     * will be found in the f1 file.
							     */
				      continue;

				 stats.num_fps_checked++;

				 if ( compare_fp_digits( f1_result[result_index], f2_result[result_index], num_sig_digits )!=TRUE ) {
				      *error_id = i;
				      failure_debug_bp( NOMATCH_RESULT ); 
				      if ( error_count_id<=MAX_REPORTED_ERRORS ){
					   sprintf(output_string, "Error - No match on result: Class = %s, \tSvar = %s, Id = %d at \t\tState = %d. Values F1/F2=%15.12f/%15.12f\n", 
						   class_name, svar_name, 
						   node_index+1, state+1,
						   f1_result[result_index], f2_result[result_index] );
					   output_message( output_string, fpout, FALSE );  
					   error_count_id++;
				      }
				      svar_match       = FALSE;
				      svar_errors      = TRUE;
				 }
			   }
		     }

		     if ( elem_error )
		          error_count_val++;

		     if ( error_count_val>0 ) {
		          sprintf(output_string, "\nError Summary for result: Class = %s, \tSvar = %s, \t\tState = %d. %d elements have errors out of %d elements.\n", 
				  class_name, svar_name, 
				  state+1,
				  error_count_val, elem_qty);
			  output_message( output_string, fpout, FALSE );
		     }
		} /* End loop on state */

		 if ( !svar_errors && !quiet_mode && svar_found && elem_qty>0 ) {
		   if ( start_state+1==stop_state ) {
		         sprintf(output_string, "\tMatch on result: Class = %s, Svar = %s, for %d elements at \tState = %d\n", class_name, f1TOC.svars[i].name,
				 elem_qty, start_state+1 );
		         output_message( output_string, fpout, FALSE );
		   }
		   else {
			 sprintf(output_string, "\tMatch on result: Class = %s, Svar = %s, for %d elements at \tStates = [%d-%d]\n", class_name, f1TOC.svars[i].name,
				 elem_qty, start_state+1, stop_state);
			 output_message( output_string, fpout, FALSE );
		   }
		 }
		 free( f1_result );
		 free( f2_result );
		 free( f1_list );
		 free( f2_list );
		 free( f1_ids );
		 free( f2_ids );
		} /* Loop on classes */
	  
    free ( p_result_buff );

    } /* Loop on svars */

    status = mc_close( fid1 );
    if ( fid2_saved<0 )
         status = mc_close( fid2_saved );

    if ( svar_errors )
         return (-1);
    else
         return OK;
}


/************************************************************
 * TAG( scan_args ) LOCAL
 *
 * Parse the MiliDiff Program's command line arguments.
 *
 */
static Return_value
scan_args( int argc, char *argv[], int last_state )
{
    int i, j;
    int status=OK;
    check_results = TRUE;
    quiet_mode    = FALSE;

    for ( i = 1; 
	  i < argc;
	  i++ )
    {
          for ( j=0;
	        j<strlen( argv[i] );
		j++ )
	        argv[i][j] =  tolower( argv[i][j] ); /* Convert all input options to lowercase */
	      
          if ( argc==1 || strcmp( argv[i], "-h" ) == 0 ||
	       strcmp( argv[i], "-help" ) == 0 )
	  {
	       usage();
	       exit(1);
	 }

         if ( strcmp( argv[i], "-f1" ) == 0 )
         {
	      i++;
	      strcpy( f1, argv[i] );
	      status = strip_path( f1, f1_path );
	      f1_set = TRUE;
	 }
	 
	 if ( strcmp( argv[i], "-f2" ) == 0 && i+1<argc )
	 {
	      i++;
	      strcpy( f2, argv[i] );
	      status = strip_path( f2, f2_path );
	      f2_set = TRUE;
	 }

	 if ( strcmp( argv[i], "-uncombined" ) == 0 ||
	      strcmp( argv[i], "-uc" ) == 0 ||
	      strcmp( argv[i], "-subset" ) == 0 )
	 {
	      subset_compare = TRUE;
	 }
	 
	 if ( strcmp( argv[i], "-d" ) == 0  && i+1<argc )
	 {
	      i++;
	      num_sig_digits = atoi(argv[i]);
         }
	 if ( strcmp( argv[i], "-startp" ) == 0  && i+1<argc )
	 {
	      i++;
	      start_proc = atoi(argv[i]);
 	      subset_compare = TRUE;
         }
	 if ( strcmp( argv[i], "-endp" ) == 0 ||
	      strcmp( argv[i], "-stopp" ) == 0  && i+1<argc )
	 {
	      i++;
	      end_proc = atoi(argv[i]);
 	      subset_compare = TRUE;
         }
	 if ( strcmp( argv[i], "-state" ) == 0 )
	 {
	      i++;
	      start_state = atoi(argv[i])-1;
	      stop_state = start_state+1;
         }
	 if ( strcmp( argv[i], "-starts" ) == 0  && i+1<argc )
	 {
	      i++;
	      start_state = atoi(argv[i])-1;
         }
	 if ( strcmp( argv[i], "-ends" ) == 0 ||
	      strcmp( argv[i], "-stops" ) == 0  && i+1<argc )	 {
	      i++;
	      stop_state = atoi(argv[i]);
	      
         }
	 if ( strcmp( argv[i], "-procs" ) == 0  && i+1<argc ||
 	      strcmp( argv[i], "-proc" )  == 0  )
	 {
	      i++;
	      start_proc = 0;
	      end_proc = atoi(argv[i]);
	      subset_compare = TRUE;
         }
	 if ( strcmp( argv[i], "-noresult" ) == 0 ||
	      strcmp( argv[i], "-noresults" ) == 0 )
	 {
	      check_results = FALSE;
         }
	 if ( strcmp( argv[i], "-class" ) == 0 && i+1<argc )
	 {
   	      i++;
	      strcpy( search_class, argv[i] );
         }
	 if ( strcmp( argv[i], "-var" ) == 0 && i+1<argc )
	 {
   	      i++;
	      strcpy( search_var, argv[i] );
         }
	 if ( strcmp( argv[i], "-q" ) == 0 ||
	      strcmp( argv[i], "-quiet" ) == 0 )
	 {
	      quiet_mode = TRUE;
         }
	 if ( strcmp( argv[i], "-o" ) == 0 )

	 {
	      i++;
	      strcpy( fout, argv[i] );
         }
    }  

    if ( !f1_set || !f2_set || argc==1 )
    {
         usage();
         exit(1);
    }
    return OK;    
}

/************************************************************
 * TAG( compare_fp_digits ) LOCAL
 *
 * Compare if two floats are within a tolerance.
 */
static inline Bool_type
compare_fp_digits( float fp1, float fp2, int num_sig_digits )
{
   float diff=1.0/pow(10, num_sig_digits);
   if ( fabs(fp1-fp2)<=diff )
        return TRUE;
   else
        return FALSE;  
}


/************************************************************
 * TAG( gen_par_filename ) LOCAL
 *
 * Generate a parallel Mili filename using the processor
 * number as input.
 */
static Bool_type
gen_par_filename ( char *root, char *fname, int proc, int num_digits )
{
   char proc_string[32]=""; 
   int proc_len=0, num_zeros=0;
   int i=0;
   strcpy( fname, root );
   sprintf( proc_string, "%d", proc );

   proc_len = strlen(proc_string);
   if ( proc_len==num_digits ) {
        strcat( fname, proc_string );
        return OK;
   }
   if ( proc_len>num_digits )
        return NOT_OK;

   num_zeros = num_digits-proc_len;
   if ( num_zeros<0 )
        return NOT_OK;

   for ( i=0;
	 i<num_zeros;	
	 i++ )
         strcat( fname, "0" );

   strcat( fname, proc_string );   

   return OK;
}

/*
/************************************************************
 * TAG( failure_debug_bp ) LOCAL
 *
 * All failures make a call to this function to intercept 
 * location of failure for debugging.
 *
 */
void failure_debug_bp( int status ) 
{
  int i=0;
  if ( status == INVALID_LABEL )
       i=1;

}


/*
/************************************************************
 * TAG( strip_path ) LOCAL
 *
 * If a Mili file speciofication has a path name, strip off
 * the path into a var path and change root to be just the
 * root filename.
 *
 */
static int
strip_path( char *root, char *path ) 
{
  int i=0;
  int last_level=-1;
  char temp_root[MAX_NAME];
  
  strcpy( path, "" );
  for ( i=0;
	i<strlen( root );
	i++ )
	if ( root[i]=='/' )
	     last_level=i+1;
	if ( last_level<0 ) /* No path in root name */
	     return OK;

	strcpy( temp_root, root );
	strcpy ( root,  &temp_root[last_level] );
	temp_root[last_level-1] = '\0';
	strcpy( path, temp_root );
	return OK;
}

/************************************************************
 * TAG( free_memory ) LOCAL
 *
 * Deallocate all memory in F1 & F2 data stgructures.
 *
 */
static void
free_memory( TOC_type *f1TOC, TOC_type *f2TOC )
{
  int i=0, j=0;

  /* Free memory for file F1 */
  free( f1TOC->node_labels );
  free( f1TOC->node_labels_index );

  for ( i=0;
	i<f1TOC->num_classes;
	i++ ) {
        free( f1TOC->classes[i].name );
        free( f1TOC->classes[i].long_name );
        if ( f1TOC->classes[i].connects )
	     free( f1TOC->classes[i].connects );
        free( f1TOC->classes[i].labels );
  }
  free( f1TOC->classes );	  

  for ( i=0;
	i<f1TOC->num_svars;
	i++ ) {
        free( f1TOC->svars[i].name );
        free( f1TOC->svars[i].long_name );
	for ( j=0;
	      j<f1TOC->svars[i].num_components;
	      j++ ) {  
	      if ( f1TOC->svars[i].components ) 
		   free( f1TOC->svars[i].components[j] );
	      if ( f1TOC->svars[i].comp_titles )
		   free( f1TOC->svars[i].comp_titles[j] );
	}
  }
  free( f1TOC->svars );	  

  /* Free memory for file F2 */
  free( f2TOC->node_labels );
  free( f2TOC->node_labels_index );

  for ( i=0;
	i<f2TOC->num_classes;
	i++ ) {
        free( f2TOC->classes[i].name );
        free( f2TOC->classes[i].long_name );
        if ( f2TOC->classes[i].connects )
	     free( f2TOC->classes[i].connects );
        free( f2TOC->classes[i].labels );
  }
  free( f2TOC->classes );	  

  for ( i=0;
	i<f2TOC->num_svars;
	i++ ) {
        free( f2TOC->svars[i].name );
        free( f2TOC->svars[i].long_name );
	for ( j=0;
	      j<f2TOC->svars[i].num_components;
	      j++ ) {  
	      if ( f2TOC->svars[i].components ) 
		   free( f2TOC->svars[i].components[j] );
	      if ( f2TOC->svars[i].comp_titles )
		   free( f2TOC->svars[i].comp_titles[j] );
	}
  }
  free( f2TOC->svars );	  
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
    printf("Usage: milidiff -f1 filename -f2 filename \n" );
    printf("\n" );
    printf("OPTIONS:\n");
    printf("                [-uncombined | -uc] \t<f1 is a subset of f2 (used to check uncombined databases> \n");
    printf("                [-d]                \t<number significant digits to compare> \n");
    printf("\n" );
    printf("                [-procs p]          \t<p-processors for multi-file processing> \n");
    printf("                [-startp p]         \t<start processor number-p for multi-file processing> \n");
    printf("                [-stopp p]          \t<ending processor number-p for multi-file processing> \n");
    printf("\n" );
    printf("                [-state s]          \t<process a single state-s> \n");
    printf("                [-starts s]         \t<start state number-s> \n");
    printf("                [-stops s]          \t<ending state number-s> \n");
    printf("\n" );    
    printf("                [-noresults]        \t<skip checking results - check geometry data only> \n");
    printf("\n" );    
    printf("                [-class name]       \t<compare results for only a single class name> \n");
    printf("                [-var name]         \t<compare results for only a single var name> \n");
    printf("\n" );
    printf("                [-q ]               \t<run in quiet mode - only errors are reported>\n");
    printf("\n" );
    printf("                [-o fname]           \t<optional output text filename>\n");
    printf("\n");
}


/************************************************************
 * TAG( get_class_label ) LOCAL
 *
 * Given a class and id for a file TOC, return the label.
 *
 */
static inline int
get_class_label( TOC_type *fileTOC, char *class, int id_num, int *label )
{
  int i=0;
  int label_index=0;

  *label = -1;

  if ( !strcmp("node", class) ) {
       if ( id_num<1 || id_num>fileTOC->num_nodes )
            return (INVALID_ID);
       
       *label = fileTOC->node_labels[id_num-1];
       return (OK);
  }

  /* Search for requested class by name */

  for ( i=0;
	i<fileTOC->num_classes;
	i++ ) {
        if ( !strcmp(fileTOC->classes[i].name, class) ) {
	     /* Global variables do not have labels so always assign 1 as a label */
	     label_index = fileTOC->classes[i].id_index[id_num-1];
	     if ( fileTOC->classes[i].sclass==M_UNIT || fileTOC->classes[i].sclass==M_MESH || fileTOC->classes[i].sclass==M_MAT ) {
	          *label = 1;
		  return (NON_ELEM_CLASS);
	     }
	     if ( label_index<0 || label_index>=fileTOC->classes[i].num_elems )
	          return (INVALID_ID);
	     *label = fileTOC->classes[i].labels[label_index].label_num;
	     return (OK);
	}
  }
  return (INVALID_CLASS);
}

/************************************************************
 * TAG( get_label_id ) LOCAL
 *
 * Get the id number for a label.
 */
static inline int
get_label_id( Label_type *label, int qty, int label_num )
{
  Label_type *id_ptr;

  /* Perform a binary search on the label array to detrermine
   * the id number for this label.
   */
  id_ptr = bsearch( &label_num, &label[0], qty,
		    sizeof(label[0]), bsearch_compare_labels );
  if ( id_ptr )
       return( id_ptr->id );
  else
       return (INVALID_LABEL);

}

/************************************************************
 * TAG( sort_compare_labels ) LOCAL
 *
 * Used to sort labels by index for quicker searching.
 */
static inline int
sort_compare_labels( const void *label1, const void *label2 )
{
  Label_type *l1=NULL, *l2=NULL;
  l1 = (Label_type *) label1;
  l2 = (Label_type *) label2;

  if ( l1->label_num < l2->label_num )
       return -1;

  if ( l1->label_num > l2->label_num )
       return 1;

  return ( 0 );
}

/************************************************************
 * TAG( bsearch_compare_labels ) LOCAL
 *
 * Used to do a quick search for a label.
 *
 */
static inline int
bsearch_compare_labels( const int *key, const Label_type *label )
{
  return( *key - label->label_num);
}

/************************************************************
 * TAG( output_message ) LOCAL
 *
 * Writes a single line of output text.
 *
 */
static void
output_message( const char *string, FILE *fp, const Bool_type quiet_mode )
{
  if ( !quiet_mode ) {
       printf("%s", string );
       fflush(stdout);
  }
  if ( fp!=NULL ) {
        fprintf(fp, "%s", string );
        fflush(fp);
  }
}

/************************************************************
 * TAG( output_stats   ) LOCAL
 *
 * Writes out run-time stats              .
 *
 */
static void
output_stats( Stats_type *stats )
{
}

/* End of MiliDiff.c */
