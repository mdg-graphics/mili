/*
 * $Log $
 *
*/

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*                                                              */
/*                      Copyright (c) 2007                      */
/*         The Regents of the University of California          */
/*                     All Rights Reserved                      */
/*                                                              */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  18-Jan-2005  IRC  Initial version.                          */
/*                                                              */
/*  17-Apr-2007  IRC  Adapted for Mili                          */
/****************************************************************/

#ifndef SILOLIB_INCLUDE
#define SILOLIB_INCLUDE

#include "silo.h"

/* */
/*  Parameters */
/* */
#define TYPE_INT    0
#define TYPE_FLOAT  1
#define TYPE_DOUBLE 2

#define FILE_CREATE 1
#define FILE_APPEND 2
#define FILE_READ   3

#define X       0
#define Y       1
#define Z       2

#define OK      0
#define NOT_OK  1

#define ON      1
#define OFF     0

#define TRUE    1
#define FALSE   0

/* Return Error conditions */
#define FILE_NOT_FOUND 10


/* Element types */
#define ELEM_BRICK   0
#define ELEM_SHELL   1
#define ELEM_TSHELL  2
#define ELEM_BEAM    3
#define ELEM_TET     4
#define ELEM_WEDGE   5
#define ELEM_PYRAMID 6
#define ELEM_TRUSS   7

#define ELEM_SLIDE  10

/* Silo Filetypes */
#define SILO_PLOTFILE       0
#define SILO_ROOTFILE       1
#define SILO_RESTART_FILE   2

#define ROOT 0

/* */
/*  C Prototypes  */
/* */

int SiloLIB_AddRegion_MRG_Tree(DBmrgtree *tp,
                               char *region_name,
                               int  max_children,
                               char *maps_name,
                               int  nsegs,
                               int  *seg_ids,
                               int  *seg_lens,
                               int  *seg_types);

char *SiloLib_Create_Filename(char *, int, int, int, int);

int SiloLib_Open_Restart_File (int, char *, char *, int, int, int, int, int);

int SiloLib_Close_Restart_File (int, int);

DBmrgtree *SiloLIB_Create_MRG_Tree(int mesh_type, int max_children);

DBfacelist *SiloLib_Create_External_Facelist (int *, int, int *, int *, int, int *);

void SiloLIB_Destory_MRG_Tree(DBmrgtree *tp);

void SiloLIB_Dump_MRG_Tree(int, int, char *treename, char *filename);

DBfile *SiloLIB_getDBid(int, int);

char *SiloLIB_GetCwr_MRG_Tree(DBmrgtree *tp);

int SiloLIB_GetVarInfo(int, int, char *, int *, int *, int *);

DBmrgtree *SiloLIB_Read_MRG_Tree(int, int, char *tree_name);

int SiloLIB_Read_TextFile(int, int, char *, char *);

int SiloLib_Read_Var(int, int, char *, void *);

int SiloLib_Write_UCD_Mesh (int, int, char *, int, int, int, int *, int, char **, void **, int, int, int *, int *, int, double, double, int, int);

int SiloLIB_Write_Groupelmap_MRG_Tree(int, int, char *, int, int *, int *, int **, int);

int SiloLib_Write_Material (int, int, char *, char *, int *, int, int *, int *, double, double, int, int);

int SiloLib_Write_MeshVar(int, int, char *, char *, double, int, int, int, int, int, void *);

int SiloLIB_Write_MRG_Tree(int, int, char *tree_name, char *mesh_name, DBmrgtree *tp);

int SiloLIB_Write_Multimat (int, int, char *, int, char **, int, int *, int, double);

int SiloLIB_Write_Multimesh (int, int, char *, int, char **, int *, int *, double *, int, double);

int SiloLIB_Write_MultiVar(int, int, char *, int, char **, int *, int, double);

int SiloLib_Write_Particle_Mesh (int, int, char *, double  , double,   int,   int , void *, void *, void *);

int SiloLib_Write_Particle_Var_Scalar(int, int, char *, double,   double,   int,   int,   int,   void *);

int SiloLib_Write_Particle_Var_Vec(int, int, char *, char *, double, double, int, int, int, int, void **);

int SiloLib_Write_UCD_Mesh (int, int, char *, int, int, int, int *, int, char **, void **, int, int, int *, int *, int, double, double, int, int);

int SiloLIB_SetCwr_MRG_Tree(DBmrgtree *tp, char *path) ;

int SiloLIB_Write_TextFile(int, int, char *, double, int, char *);

int SiloLib_Write_Var(int, int, char *, double, int, int, int, int, void *);

/* Directory Functions */

int SiloLib_cddir (int, int, char *) ;

int SiloLib_getdir (int, int, char *) ;

int SiloLib_mkdir (int, int, char *) ;

int SiloLib_mkcddir (int, int, char *, char *) ;

int SiloLIB_getTOC(int, int, int *, char **, int *, char **, int *, char **, int *, char **);

int SiloLIB_Enable_Errors( void ) ;
int SiloLIB_Disable_Errors( void ) ;

#endif
