/*
 * $Log: SiloLib_Internal.h,v $
 * Revision 1.4  2011/05/26 21:43:13  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.3  2009/05/28 23:10:56  corey3
 * Changed max len of Silo filename to 256
 *
 * Revision 1.2  2008/09/24 18:36:01  corey3
 * New update of Passit source files
 *
 * Revision 1.1  2007/06/25 17:58:31  corey3
 * New Mili/Silo interface
 *
 *
 *
*/


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*                                                              */
/*                      Copyright (c) 2005                      */
/*         The Regents of the University of California          */
/*                     All Rights Reserved                      */
/*                                                              */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* File: Silolib_Internal.h                                     */
/*                                                              */
/* Description: This file contains the definitions for all      */
/*              internal valiables and parameters.              */
/*                                                              */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  18-Jan-2005  IRC  Initial version.                          */
/****************************************************************/

#ifndef SILOLIB_PARAMETERS_INCLUDE
#define SILOLIB_PARAMETERS_INCLUDE

#include "silo.h"
#include "SiloLib.h"

#define MAX_OPEN_FILES 100

/* */
/*  Parameters */
/* */

#define SILOLIB_MAX_INPUT_STRING_LENGTH           256
#define SILOLIB_MAX_OUTPUT_STRING_LENGTH          256
#define SILOLIB_MAX_FILE_NAME_LENGTH              256

#define SILOLIB_TINY_DOUBLE                       1.0e-13
#define SILOLIB_SMALL_DOUBLE                      1.0e-10
#define SILOLIB_LARGE_DOUBLE                      1.0e+10
#define SILOLIB_HUGE_DOUBLE                       1.0e+75

#define SILOLIB_LARGE_INT                         1000000000

#define SILOLIB_ONE_BYTE                          8

#define SILOLIB_REALLOC_CYCLE                     100

#define ROOT_FILE                                 0

#define SILO_FAMILY_MAX                           100


/*********************************
 * METADATA STRUCTURE
 ********************************/

typedef struct _silo_metadata
{
   char filename[128];
   char filepath[128];
   int  vartype;
   int  varsize;
   char varname[64];

   int       multimesh_object;
   char      meshname[64];
   int       meshtype;

} Silo_metadata;

typedef struct silo_family
{
   int    famid;
   char   *root;

   int    cycle, processor, seqnum;
   DBfile *restart_file_pointer;
   DBfile *root_file_pointer ;

   int    metadata_count;
   Silo_metadata *mdata;
} Silo_Family;

Silo_Family silo_family[SILO_FAMILY_MAX];
int silo_family_qty;

/* Geometry Definition Structures */
typedef struct _matdef
{
   int   matnum;
   int   mattype;
   char *matname;
} mat_def_type;

typedef struct _nodedef
{
   double displacement;
   int    nodenum;
   double pos[3];
   double rotation;
} node_def_type;

typedef struct _elem_hex_def
{
   int    elemnum;
   int    matnum;
   int    nodes[8];
} elem_hex_def_type;


typedef struct _elem_beam_def
{
   int    elemnum;
   int    matnum;
   int    nodes[3];
} elem_beam_def_type;

typedef struct _elem_shell_def
{
   int    elemnum;
   int    matnum;
   int    nodes[4];
   double thickness[4];
} elem_shell_def_type;

typedef struct _elem_tshell_def
{
   int    elemnum;
   int    matnum;
   int    nodes[8];
} elem_tshell_def_type;


typedef struct _node_mass_def
{
   int     nodenum;
   double  mass;
} node_mass_def_type;


typedef struct _slide_node
{
   int  num;
   int  nodes[4];
} slide_node_type;

typedef struct _slide_def
{
   int  numslaves;
   int  nummasters;
   int  type;
   slide_node_type *slide_nodes_slaves;
   slide_node_type *slide_nodes_masters;
} slide_def_type;

typedef struct _tgmesh
{
   int num_mats, num_np, num_elem_hex, num_elem_beam, num_elem_shell, num_elem_tshell;
   int num_nodal_constraints, num_slides;
   int num_springs, num_dampers, num_node_mass;

   mat_def_type          *mat_def;
   node_def_type         *node_def;

   elem_hex_def_type    *elem_hex_def;
   elem_beam_def_type   *elem_beam_def;
   elem_shell_def_type  *elem_shell_def;
   elem_tshell_def_type *elem_tshell_def;

   slide_def_type       *slide_def;

   node_mass_def_type   *node_mass_def;
} TGMESH_type;

#endif
