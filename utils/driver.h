/*
 * driver.h
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <sys/types.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/resource.h>
#include <errno.h>
#include <ctype.h>
#include <dlfcn.h>

#include "mili.h"
#include "misc.h"
#include "list.h"
#include "gahl.h"
#include "mili_internal.h"
#include "mili_config.h"
#ifdef TIMER
#undef TIMER
#endif
//#define TIMER 1
#ifdef TIMER
#include <sys/timeb.h>
#endif
/*#define TIMER 1*/
#ifndef MININT
#define MININT -1000000
#endif
#define NO_LABELS 100

/* A nominal return value. */
#ifndef OK
#define OK 0
#endif

#define PROC 1000
#define MAT  2000

#define MAX_MAT  4000
#define MAX_PROC 128000

/* A failure return value. */
#ifndef XMILICS_FAIL
#define XMILICS_FAIL 1
#endif

//#define XMILICS_VERSION "16_1(12-09-2016)"

Bool_type is_known_db( char *fname, Database_type *p_db_type );


/*****************************************************************
 * TAG( Environ )
 *
 * The environment struct contains global program variables which
 * aren't associated with a particular database.
 */
typedef struct {
  
   char input_file_name[1024];
   char output_file_name[1024];
   char partfile_name_c[100];
   char partfile_name_s[100];
   char plotfile_name[512];
   char user_name[30];
   char date[20];
   int pad;
   int nprocs;
   int nmats;
   int flsize;
   int n_states;
   int start_state, stop_state;

   int num_selected_procs;
   int start_proc, stop_proc;
   short *selected_proc_list;

   int num_selected_mats;
   int start_mat, stop_mat;
   short *selected_mat_list;
   int wait;
   int wait_time;
   int restart;
   int current_state_max;
   char hsp_file_name[256];
   Bool_type hsp_done;
   int current_state_array_size;
   
   Bool_type matproc_selected;
   short **matproc_table;
   int   *num_mats_proc;

   int proc_seqnum; /* Processor seq num to append to output file */

   Bool_type states_per_file;
   Bool_type timing;
   Bool_type result_timing;
   Bool_type combine;
   Bool_type split;
   Bool_type ti_enabled;
   Bool_type append;
   Bool_type batch_overwrite;
   Bool_type batch;
   Bool_type force_partition;
   Bool_type newfile;
   Bool_type write_tfile;

   /* TI Input Variables */
   char ti_mili_version[64], ti_host[64], ti_arch[64],
        ti_timestamp[64],    ti_user[64], ti_xmilics_version[64],
        ti_code_name[64];
   
} Environ;

extern Environ env;




/***
   struct Labels 
   Container for labels to build them correctly from labels.
*/
typedef struct _Labels
{
  struct _Labels* next;
  char sname[36];
  int sclass;
  int mesh_id;
  int material;
  Bool_type isMeshVar;
  Bool_type isNodal;
  int datatype;
  long *num_per_processors;
  LONGLONG *offset_per_processor;
  int highest_count;
  short combiner_created;
  int task;
  long size;
  int *labels,
      *map;
} Label ;

struct sorter{
   int label_id;
   int proc;
   int proc_position;
};

typedef struct _SubrecContainer{

  struct _SubrecContainer *next;
  char subrec_name[128];
  int srec_id;
  int stype;
  short * in_subrec_per_proc;
  int **  subrecord_map_for_each_proc;  
  int *   subrec_entries_from_each_proc;
  int subrec_offset_per_proc;  
  int total_external_contributions;
  int * subrecord_map; 
  int * subrecord_offsets_per_proc;
  Subrecord subrecord;
     
}SubrecContainer;

/*****************************************************************
 * TAG( TILabels )
 *
 * This structure holds all nodal and element nodes and labels that
 * are read in from the TI data files. Data fields to support multiple
 * element sets that are not named in the normal fashion (ie not named
 * brick for hexes or shell for quads) was added June 8, 2008: IRC.
 *
 * The array hex_label_elem_set_id or shell_label_elem_set_id holds
 * the id of the element set for each element. It is used to generate
 * labels for a individual element set before writing out the TI
 * label data.
 *
 */
typedef struct {
   int num_procs;
   Label* labels;
   short *subrec_contributions;
   int dimensions;
} TILabels;



#define MAXTOKENS 25
#define TOKENLENGTH 80
#define MAX_CLASS_NAMES 1500

extern Database_type db_type;


/* Mili wrappers. */
extern int mili_db_open( char *, int * );
extern int mili_db_cleanse_state_var( State_variable * );

/* Taurus plotfile wrappers. */
extern int taurus_db_open( char *, int * );
extern int taurus_db_close( Mili_analysis * );

/* result_data.c */
extern void load_result( Mili_analysis *, Bool_type, Bool_type );
extern Bool_type load_subrecord_result( Mili_analysis *, int, Bool_type, Bool_type );

/* gui.c */
extern void wrt_text( char *, ... );

/* Class Summary Functions */
void      free_class_summary( void );
Bool_type search_class_summary( int superclass, char *class_name );

int
get_mat_list_for_proc( Mili_analysis **in_db, int proc,
                       short **mat_list );
void
find_pad_count(char* input_file_name);
static void 
scan_args( int argc, char *argv[] );
static void 
pf_name( char *root_name, int fnum, char *fname );
static void usage( void );
static int 
wait_for_restart(Mili_analysis ** anal, int current_state);
/**
* From read_db.c
*/
Return_value
read_non_state_data( Mili_analysis *in_dbase );

Return_value
read_state_data( int state_num, Mili_analysis *in_dbase );

/**
* From combine_db.c
*/
Return_value
combine_definitions(Mili_analysis **in_db, Mili_analysis *out_db,TILabels *labels);
/**
* From io_func.c
*/
Bool_type 
open_input_dbase( char *fname, Mili_analysis *analy );
Bool_type
open_output_dbase( char *fname, Mili_analysis *analy );
void 
close_dbase(Mili_analysis *dbase, int full );

/**
* From write_db.c
*/
Return_value write_non_state_data( Mili_analysis *out_dbase );
Return_value write_state_data( int state_num, Mili_analysis *in_dbase );
Return_value write_ti_data( Mili_analysis *out_db );

/**
* From misc.c
*/
void manage_timer( int end_flag );


#endif
