/*
 * $Log: MiliSilo_Internal.h,v $
 * Revision 1.8  2011/05/26 21:43:13  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.7  2008/09/24 18:36:01  corey3
 * New update of Passit source files
 *
 * Revision 1.6  2008/02/13 01:59:48  corey3
 * Added new capability to MiliSilo
 *
 * Revision 1.5  2007/09/14 22:50:33  corey3
 * Added more functionality to Mili-Silo API
 *
 * Revision 1.4  2007/08/24 19:59:20  corey3
 * Added more functions for support writing Silo files
 *
 * Revision 1.2  2007/08/01 01:18:41  corey3
 * More new functionality for writing Mili-Silo files
 *
 * Revision 1.1  2007/07/27 19:27:49  corey3
 * Added new Mili-Silo functions indcluding a test driver
 *
 * Revision 1.1  2007/06/25 17:58:31  corey3
 * New Mili/Silo interface
 *
 *
 *
 */

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*                                                              */
/*                      Copyright (c) 2007                      */
/*         The Regents of the University of California          */
/*                     All Rights Reserved                      */
/*                                                              */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* File: MiliSilo_Internal.h                                    */
/*                                                              */
/* Description: This file contains the definitions for all      */
/*              internal valiables and parameters.              */
/*                                                              */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  23-Jul-2007  IRC  Initial version.                          */
/****************************************************************/

#ifndef MILISILO_INTERNAL_INCLUDE
#define MILISILO_INTERNAL_INCLUDE

#include "SiloLib.h"
#include "mili_enum.h"

/* */
/*  Parameters */
/* */

#define SILOLIB_MAX_INPUT_STRING_LENGTH  256
#define MAX_CLASSES                      100
#define MAX_FILES                        1000
#define MAX_MESHES                       100
#define MAX_SRECS                        100

int num_families;

/* Prototypes */

int mc_silo_get_class_id( Famid, int, char *, int ) ;


typedef struct
{
   int    num_nodes;
   int    start_node, stop_node;
   int    *labels;
   float  *coords;
} MiliSilo_nodelist;


typedef struct
{
   int    num_el;
   Bool_type sequential;
   int    class;
   int    start_el, stop_el;
   int    *el_ids;
   int    *mats;
   int    *parts;
   int    num_el_points;
   int    *elements;
   int    *labels;
} MiliSilo_elementlist;


/* Class Data Structures */
typedef struct
{
   int  class_id;
   int  num_parents;
   int  parent_ids[20];
   int  num_children;
   int  child_ids[20];
   char *class_name;
   char *long_name;
   char *short_name;
   int  qty_objects;

   Block_list *p_bl;
   int obj_ids_start[100];
   int obj_ids_stop[100];
} MiliSilo_class;


/* Mesh Data Structures */
typedef struct
{
   int  mesh_id;
   int  num_parents;
   int  parent_ids[20];
   int  num_children;
   int  child_ids[20];

   char *name;
   int  type;
   int  superclass;

   int            qty_classes;
   MiliSilo_class class[MAX_CLASSES];
   char           *classList[100];

   Bool_type nodes_defined, connects_defined; /* When both are true we can write out the
						* Silo mesh object
   						*/

   Block_list *p_nl;
   int num_nl;
   int total_nodes;
   MiliSilo_nodelist nodes[20];

   Block_list *p_el;
   int num_el;
   int total_elements;
   MiliSilo_elementlist elements[20];
   int dim;
} MiliSilo_mesh;


/* File Data Structure */
typedef struct
{
   char    *filename;
   int      processor_id;
   int      num_states;
   double   first_time, last_time;
} MiliSilo_file;


/* State Record Data Structure */
typedef struct
{
   char     name;
   int      srec_id;
   float    time;
   int      size;
} MiliSilo_state_rec;


/* State Data Structure */
typedef struct
{
   char    *filename;
   int      srec_id;
   float    time;
} MiliSilo_state;


/* Family Data Structure */
typedef struct
{
   int            fid;
   char           access_mode;
   int            dimensions;
   int            precision_limit;

   int            qty_files;
   int            current_state_file_id;
   char           *path;
   char           *root;
   int            root_len;

   MiliSilo_file  files[MAX_FILES];
   char           *fileList[100];

   int            qty_meshes;
   MiliSilo_mesh  meshes[MAX_MESHES];

   /* File size fields */
   int            st_suffix_width;
   int            bytes_per_file_limit;
   int            states_per_file;
   int            qty_states;

   int            cur_st_index;
   MiliSilo_state_rec srecs[MAX_SRECS];
} MiliSilo_family;


MiliSilo_family family[20];
int             family_qty;


static int conn_words[M_QTY_SUPERCLASS] =  /* Keep current with mili.h! */
{
   0, 0, 4, 5, 5, 6, 6, 7, 8, 10, 0, 0, 4
};

#endif
