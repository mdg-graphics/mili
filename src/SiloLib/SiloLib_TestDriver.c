/*
 * $Log: SiloLib_TestDriver.c,v $
 * Revision 1.11  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.10  2008/11/14 01:03:45  corey3
 * Continued development of MiliSilo
 *
 * Revision 1.9  2008/11/14 00:08:52  corey3
 * Continued development of MiliSilo
 *
 * Revision 1.8  2008/09/24 18:36:02  corey3
 * New update of Passit source files
 *
 * Revision 1.7  2007/09/14 22:56:03  corey3
 * Added more functionality to Mili-Silo API
 *
 * Revision 1.6  2007/08/24 19:54:07  corey3
 * Added more functions for support writing Silo files
 *
 * Revision 1.5  2007/07/27 22:26:36  corey3
 * Changed placement of meshvars (results)
 *
 * Revision 1.4  2007/07/23 19:21:00  corey3
 * Adding more capabilities to the SiloLibrary layer
 *
 * Revision 1.3  2007/07/10 20:57:57  corey3
 * More additions to the Silo IO Library
 *
 * Revision 1.2  2007/07/05 22:03:50  corey3
 * Adding more functionality to Silo Library Interface
 *
 * Revision 1.1  2007/06/25 17:58:31  corey3
 * New Mili/Silo interface
 *
 *
*/

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*                                                              */
/*                   Copyright (c) 2007                         */
/*         The Regents of the University of California          */
/*                     All Rights Reserved                      */
/*                                                              */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*  Include Files  */

#define ENABLE_SILO

#include "partition.h"

#include "silo.h"
#include "SiloLib.h"
#include "SiloLib_Internal.h"

#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

char partfile_in[32];        /* Optional partition file */
int  partfile_found = FALSE;

char filein[64], fileout[64];
int  debug_flag;

/* Output Silo filenames */
char plot_filename[64], *plotfile, root_filename[64], *rootfile;


int main(int argc, char **argv)
/****************************************************************/
/* This routine will test the Silo I/O Library by setting up    */
/* a mesh read in from a TrueGrid output file.                  */
/*                                                              */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*  20-Jan-2005  IRC  Initial version.                          */
/*                                                              */
/*  25-May-2007  IRC  Adapted for Mili.                         */
/****************************************************************/
{
   int status=OK;

   FILE *fp;
   char linein[210];

   int  i, j, k ;
   int  num_nodes=0, tot_zones=0, num_zones[3] = {0,0,0};
   int  nodenum;
   int  num_bricks, num_shells, num_tshells, num_beams;
   int  group_num;

   int  num_nodes_brick=0, num_nodes_beam=0, num_nodes_shell=0, num_nodes_tshell=0;

   double time=1.0, time_step ;
   double zone_size ;

   float  *coord_x, *coord_y, *coord_z;
   float  *coords[3];
   char   *coord_names[3] = {"X Coord", "Y Coord", "Z Coord"};

   double xval, yval, zval;

   int temp;
   int processor, cycle=0, sequence_number=1;

   Partition_mili *pmap;
   TGMESH_type    *tgmesh;

   /* Material Variables */
   short *mat_list;
   int   *mat_zone_list;
   int   mat, matnum, num_mats;
   int   *mat_nums;
   int   mat_count=0;
   int   *node_list, nl_length=0, zone_cnt=0;
   char  *mat_names[20], matname[64];

   /* Multi-mesh variables */
   int    mm_count=0;
   int    num_meshes = 0, meshtypes[200], zonecounts[400];
   double extents[120];
   char   *meshnames[400];
   char   temp_string[400];

   /* Slide interface variables */
   int slide_index, zl_number=1;
   int num_slides = 0;
   char meshname[64], dirname[64];

   /* Results variables for testing */
   float *result1, *result2;

   /* Simple variables */
   int    dims[3], varlen, vartype;
   double var1, var2[20];

   /* Silo TOC variables */
   int num_dirs, num_vars, num_meshvars, num_ptvars;
   char **dir_names, **var_names, **mesh_vars, **pt_vars;

   DBfile *dbPlot, *dbRoot;
   int    famid=1;

   /* Grouping variables */
   DBmrgtree *tp;

   /**********************************************************************************/

   /* Get the command line arguments */
   status = parse_command_line(argc, argv) ;

   /* Read in the TrueGrid mesh definition */
   /*  status = SiloLIB_ReadTGMesh( &tgmesh,filein ); */
   if ( status==FILE_NOT_FOUND )
   {
      printf("\n** Error - TrueGrid input file not found: %s", filein) ;
      exit(1);
   }


   /* Read in the partition file - if available */

   if ( partfile_found )
   {
      pmap = (Partition_mili *) malloc(1*sizeof(Partition_mili)) ;
      status = read_partition_mili( partfile_in, &pmap );
      if ( status==FILE_NOT_FOUND )
      {
         printf("\n** Error - Partition input file not found: %s", filein) ;
         exit(1);
      }
   }

   cycle           = -1; /* Do not include in file name = -1 */
   sequence_number = -1; /* Do not include in file name = -1 */
   processor       = -1; /* Do not include in file name = -1 */

   /*
   /* Open the Root File */
   rootfile = SiloLib_Create_Filename(root_filename, SILO_ROOTFILE,
                                      cycle, processor, sequence_number);

   status = SiloLib_Open_Restart_File(famid,rootfile,NULL,
                                      cycle,processor,sequence_number,
                                      SILO_ROOTFILE,FILE_CREATE);

   /* Open the Plot File */
   processor = 1;
   plotfile = SiloLib_Create_Filename(root_filename, SILO_PLOTFILE,
                                      cycle, processor, sequence_number);
   status   = SiloLib_Open_Restart_File(famid,plotfile,NULL,
                                        cycle,processor,sequence_number,
                                        SILO_PLOTFILE,FILE_CREATE);

   /* Get DB Identifiers from Silo Library */
   dbPlot = SiloLIB_getDBid(famid, SILO_PLOTFILE );
   dbRoot = SiloLIB_getDBid(famid, SILO_ROOTFILE );

   /* Open and read the TG geometry description file */

   num_nodes = tgmesh->num_np;

   coord_x   = (float *) calloc(num_nodes,sizeof(double));
   coord_y   = (float *) calloc(num_nodes,sizeof(double));
   coord_z   = (float *) calloc(num_nodes,sizeof(double));

   coords[X] = coord_x;
   coords[Y] = coord_y;
   coords[Z] = coord_z;

   /* Create a mesh for the bricks */
   tot_zones = tgmesh->num_elem_hex +  tgmesh->num_elem_beam + tgmesh->num_elem_shell + tgmesh->num_elem_beam;
   num_mats = tgmesh->num_mats;

   mat_list      = (short *) calloc(1000,sizeof(short));
   mat_zone_list = (int *)   calloc(tot_zones,sizeof(int));
   mat_nums      = (int *)   calloc(1000,sizeof(int));
   node_list     = (int *)   calloc(tot_zones*8,sizeof(int));

   /* Construct the coordinate list */
   for (i=0;
         i<num_nodes;
         i++)
   {
      coord_x[i] = tgmesh->node_def[i].pos[0];
      coord_y[i] = tgmesh->node_def[i].pos[1];
      coord_z[i] = tgmesh->node_def[i].pos[2];
   }

   /* Construct a matlist for all elements in the problem */
   for (i=0;
         i<num_mats;
         i++)
   {
      mat_nums[i] = tgmesh->mat_def[i].matnum;
   }

   group_num = 1;

   /*********************/
   /* Create a MRG Tree */
   /*********************/
   tp = SiloLIB_Create_MRG_Tree(DB_UCDMESH, 100 );

   /*********************/
   /* BRICK ELEMENTS -1 */
   /*********************/

   /* Jump back to Root dir */
   status = SiloLIB_cddir(famid,  SILO_PLOTFILE, "/") ;
   status = SiloLIB_getdir(famid, SILO_PLOTFILE, temp_string) ;

   num_bricks = tgmesh->num_elem_hex/2;
   nl_length  = 0;

   if(num_bricks>0)
   {
      num_zones[0] = tgmesh->num_elem_hex;

      status = SiloLIB_getdir(famid, SILO_PLOTFILE, temp_string) ;
      status = SiloLIB_mkdir(famid, SILO_PLOTFILE,  "Bricks_1") ;
      status = SiloLIB_cddir(famid, SILO_PLOTFILE,  "Bricks_1") ;

      /* Identify the set of nodes that are for brick elements */
      for (i=0;
            i<num_bricks;
            i++)
      {
         matnum = tgmesh->elem_hex_def[i].matnum;
         if ( mat_list[matnum] == OFF )
         {
            mat_list[matnum]     = ON;
         }

         mat_zone_list[i] = matnum;

         for (j=0;
               j<8;
               j++)
         {
            nodenum = tgmesh->elem_hex_def[i].nodes[j] - 1;
            if ( nodenum<0 )
            {
               printf("\n** Fatal error - node number of 0 found");
               exit(1);
            }

            node_list[nl_length++] = nodenum;
         }
      }

      /* Reverse order of node list to put in right-hand format */
      for (i=0;
            i<nl_length;
            i+=4)
      {
         for (j=0;
               j<2;
               j++)
         {
            temp             = node_list[i+3-j];
            node_list[i+3-j] = node_list[i+j];
            node_list[i+j]   = temp;
         }
      }

      /* Make an assembly region */
      status = SiloLIB_AddRegion_MRG_Tree(tp,
                                          "Assembly",
                                          100,
                                          "objects_to_assembly",
                                          0,
                                          NULL,
                                          NULL,
                                          NULL);


      /* Add the region to the MRG Tree */
      status = SiloLIB_AddRegion_MRG_Tree(tp,
                                          "Bricks_1",
                                          100,
                                          "Bricks_1",
                                          0,
                                          NULL,
                                          NULL,
                                          NULL);

      /* Define a Mesh for the Brick Elements */

      zl_number  = 1;

      status = SiloLib_Write_UCD_Mesh(famid, SILO_PLOTFILE,
                                      "Mesh_Bricks_1",
                                      ELEM_BRICK,
                                      group_num,
                                      3,
                                      node_list,
                                      nl_length,
                                      coord_names,
                                      (void *) coords,
                                      num_nodes,
                                      num_bricks,
                                      NULL,
                                      NULL,
                                      zl_number,
                                      0.0,
                                      time,
                                      0,
                                      DB_FLOAT) ;

      /* Define Materials for all Element Types */

      status = SiloLib_Write_Material(famid, SILO_PLOTFILE,
                                      "Mesh_Bricks_1",
                                      "Mat_Bricks_1",
                                      &num_bricks,
                                      num_mats,
                                      mat_nums,
                                      mat_zone_list,
                                      0.0,
                                      time,
                                      0,
                                      DB_DOUBLE) ;


      /* Define a Result variable for the bricks */
      status = SiloLIB_mkdir(famid, SILO_PLOTFILE, "Bricks_Vars1") ;
      status = SiloLIB_cddir(famid, SILO_PLOTFILE, "Bricks_Vars1") ;

      result1 = (float *) malloc(num_bricks*sizeof(float));
      for (i=0;
            i<num_bricks;
            i++)
         result1[i] = 1.0;

      status = SiloLib_Write_MeshVar (famid, SILO_PLOTFILE,
                                      "Mesh_Bricks_1",
                                      "processor_id",
                                      time,
                                      1,
                                      1,
                                      num_bricks,
                                      DB_ZONECENT,
                                      DB_FLOAT,
                                      (void *)result1);

      free(result1);
   }


   /*********************/
   /* BRICK ELEMENTS -2 */
   /*********************/

   /* Jump back to Root dir */
   status = SiloLIB_cddir(famid, SILO_PLOTFILE, "/") ;
   status = SiloLIB_getdir(famid, SILO_PLOTFILE, temp_string) ;

   num_bricks = tgmesh->num_elem_hex/2;
   nl_length  = 0;

   if(num_bricks>0)
   {
      num_zones[0] = tgmesh->num_elem_hex;

      status = SiloLIB_getdir(famid, SILO_PLOTFILE, temp_string) ;
      status = SiloLIB_mkdir(famid, SILO_PLOTFILE, "Bricks_2") ;
      status = SiloLIB_cddir(famid, SILO_PLOTFILE, "Bricks_2") ;

      /* Identify the set of nodes that are for brick elements */
      for (i=num_bricks;
            i<tgmesh->num_elem_hex;
            i++)
      {
         matnum = tgmesh->elem_hex_def[i].matnum;
         if ( mat_list[matnum] == OFF )
         {
            mat_list[matnum]     = ON;
         }

         mat_zone_list[i] = matnum;

         for (j=0;
               j<8;
               j++)
         {
            nodenum = tgmesh->elem_hex_def[i].nodes[j] - 1;
            if ( nodenum<0 )
            {
               printf("\n** Fatal error - node number of 0 found");
               exit(1);
            }

            node_list[nl_length++] = nodenum;
         }
      }

      /* Reverse order of node list to put in right-hand format */
      for (i=0;
            i<nl_length;
            i+=4)
      {
         for (j=0;
               j<2;
               j++)
         {
            temp             = node_list[i+3-j];
            node_list[i+3-j] = node_list[i+j];
            node_list[i+j]   = temp;
         }
      }

      /* Define a Mesh for the Brick Elements */

      zl_number  = 1;

      status = SiloLib_Write_UCD_Mesh(famid, SILO_PLOTFILE,
                                      "Mesh_Bricks_2",
                                      ELEM_BRICK,
                                      group_num,
                                      3,
                                      node_list,
                                      nl_length,
                                      coord_names,
                                      (void *) coords,
                                      num_nodes,
                                      num_bricks,
                                      NULL,
                                      NULL,
                                      zl_number,
                                      0.0,
                                      time,
                                      0,
                                      DB_FLOAT) ;

      /* Define Materials for all Element Types */

      status = SiloLib_Write_Material(famid, SILO_PLOTFILE,
                                      "Mesh_Bricks_2",
                                      "Mat_Bricks_2",
                                      &num_bricks,
                                      num_mats,
                                      mat_nums,
                                      mat_zone_list,
                                      0.0,
                                      time,
                                      0,
                                      DB_DOUBLE) ;

      /* Define a Result variable for the bricks */
      status = SiloLIB_mkdir(famid, SILO_PLOTFILE, "Bricks_Vars2") ;
      status = SiloLIB_cddir(famid, SILO_PLOTFILE, "Bricks_Vars2") ;

      result1 = (float *) malloc(num_bricks*sizeof(float));
      for (i=0;
            i<num_bricks;
            i++)
         result1[i] = 2.0;

      status = SiloLib_Write_MeshVar (famid, SILO_PLOTFILE,
                                      "Mesh_Bricks_2",
                                      "processor_id",
                                      time,
                                      1,
                                      1,
                                      num_bricks,
                                      DB_ZONECENT,
                                      DB_FLOAT,
                                      (void *)result1);

      status = SiloLib_Write_MeshVar (famid, SILO_PLOTFILE,
                                      "Mesh_Bricks_2",
                                      "processor_id2",
                                      time,
                                      1,
                                      1,
                                      num_bricks,
                                      DB_ZONECENT,
                                      DB_FLOAT,
                                      (void *)result1);

      free(result1);
   }


   /********************/
   /* SHELL ELEMENTS-1 */
   /********************/

   /* Jump back to Root dir */
   status = SiloLIB_cddir(famid, SILO_PLOTFILE, "/") ;
   status = SiloLIB_getdir(famid, SILO_PLOTFILE, temp_string) ;

   num_shells = tgmesh->num_elem_shell/2;

   if (num_shells >0)
   {
      num_zones[0] = tgmesh->num_elem_shell;
      nl_length    = 0;
      status = SiloLIB_mkdir(famid, SILO_PLOTFILE, "Shells_1") ;
      status = SiloLIB_cddir(famid, SILO_PLOTFILE, "Shells_1") ;

      /* Identify the set of nodes that are for shell elements */
      for (i=0;
            i<num_shells;
            i++)
      {
         matnum = tgmesh->elem_shell_def[i].matnum;
         if ( mat_list[matnum] == OFF )
         {
            mat_list[matnum]     = ON;
         }

         mat_zone_list[i] = matnum;

         for (j=0;
               j<4;
               j++)
         {
            nodenum = tgmesh->elem_shell_def[i].nodes[j] - 1;
            if ( nodenum<0 )
            {
               printf("\n** Fatal error - node number of 0 found");
               exit(1);
            }
            node_list[nl_length++] = nodenum;
         }
      }

      /* Reverse order of node list to put in right-hand format */
      for (i=0;
            i<nl_length;
            i+=4)
      {
         for (j=0;
               j<2;
               j++)
         {
            temp             = node_list[i+3-j];
            node_list[i+3-j] = node_list[i+j];
            node_list[i+j]   = temp;
         }
      }

      /* Define a Mesh for the Shell Elements */

      zl_number  = 1;
      status = SiloLib_Write_UCD_Mesh(famid, SILO_PLOTFILE,
                                      "Mesh_Shells_1",
                                      ELEM_SHELL,
                                      group_num,
                                      3,
                                      node_list,
                                      nl_length,
                                      coord_names,
                                      (void *) coords,
                                      num_nodes,
                                      num_shells,
                                      NULL,
                                      NULL,
                                      zl_number,
                                      0.0,
                                      time,
                                      0,
                                      DB_FLOAT) ;


      /* Define Materials for the Shell Elements */

      status = SiloLib_Write_Material(famid, SILO_PLOTFILE,
                                      "Mesh_Shells_1",
                                      "Mat_Shells_1",
                                      &num_shells,
                                      num_mats,
                                      mat_nums,
                                      mat_zone_list,
                                      0.0,
                                      time,
                                      0,
                                      DB_DOUBLE) ;


      /* Define a Result variable for the shells */
      status = SiloLIB_mkdir(famid, SILO_PLOTFILE, "Shells_Vars1") ;
      status = SiloLIB_cddir(famid, SILO_PLOTFILE, "Shells_Vars1") ;

      result1 = (float *) malloc(num_shells*sizeof(float));
      for (i=0;
            i<num_shells;
            i++)
         result1[i] = 1.0;

      for (i=0;
            i<num_shells;
            i++)
         result1[i] = -1.0;
      status = SiloLib_Write_MeshVar (famid, SILO_PLOTFILE,
                                      "Mesh_Shells_1",
                                      "processor_id2",
                                      time,
                                      1,
                                      1,
                                      num_shells,
                                      DB_ZONECENT,
                                      DB_FLOAT,
                                      result1);
      free(result1);
      status = SiloLIB_cddir(famid, SILO_PLOTFILE, "..") ;
   }


   /********************/
   /* SHELL ELEMENTS-2 */
   /********************/

   /* Jump back to Root dir */
   status = SiloLIB_cddir(famid, SILO_PLOTFILE, "/") ;
   status = SiloLIB_getdir(famid, SILO_PLOTFILE, temp_string) ;

   num_shells = tgmesh->num_elem_shell/2;

   if (num_shells >0)
   {
      num_zones[0] = tgmesh->num_elem_shell;
      nl_length    = 0;
      status = SiloLIB_mkdir(famid, SILO_PLOTFILE, "Shells_2") ;
      status = SiloLIB_cddir(famid, SILO_PLOTFILE, "Shells_2") ;

      /* Identify the set of nodes that are for shell elements */
      for (i=num_shells;
            i<tgmesh->num_elem_shell;
            i++)
      {
         matnum = tgmesh->elem_shell_def[i].matnum;
         if ( mat_list[matnum] == OFF )
         {
            mat_list[matnum]     = ON;
         }

         mat_zone_list[i] = matnum;

         for (j=0;
               j<4;
               j++)
         {
            nodenum = tgmesh->elem_shell_def[i].nodes[j] - 1;
            if ( nodenum<0 )
            {
               printf("\n** Fatal error - node number of 0 found");
               exit(1);
            }
            node_list[nl_length++] = nodenum;
         }
      }

      /* Reverse order of node list to put in right-hand format */
      for (i=0;
            i<nl_length;
            i+=4)
      {
         for (j=0;
               j<2;
               j++)
         {
            temp             = node_list[i+3-j];
            node_list[i+3-j] = node_list[i+j];
            node_list[i+j]   = temp;
         }
      }


      /* Define a Mesh for the Shell Elements */

      zl_number  = 1;
      status = SiloLib_Write_UCD_Mesh(famid, SILO_PLOTFILE,
                                      "Mesh_Shells_2",
                                      ELEM_SHELL,
                                      group_num,
                                      3,
                                      node_list,
                                      nl_length,
                                      coord_names,
                                      (void *) coords,
                                      num_nodes,
                                      num_shells,
                                      NULL,
                                      NULL,
                                      zl_number,
                                      0.0,
                                      time,
                                      0,
                                      DB_FLOAT) ;


      /* Define Materials for the Shell Elements */

      status = SiloLib_Write_Material(famid, SILO_PLOTFILE,
                                      "Mesh_Shells_2",
                                      "Mat_Shells_2",
                                      &num_shells,
                                      num_mats,
                                      mat_nums,
                                      mat_zone_list,
                                      0.0,
                                      time,
                                      0,
                                      DB_DOUBLE) ;


      /* Define a Result variable for the shells */
      status = SiloLIB_mkdir(famid, SILO_PLOTFILE, "Shells_Vars2") ;
      status = SiloLIB_cddir(famid, SILO_PLOTFILE, "Shells_Vars2") ;

      result1 = (float *) malloc(num_shells*sizeof(float));
      for (i=0;
            i<num_shells;
            i++)
         result1[i] = 1.0;

      status = SiloLib_Write_MeshVar (famid, SILO_PLOTFILE,
                                      "Mesh_Shells_2",
                                      "processor_id",
                                      time,
                                      1,
                                      1,
                                      num_shells,
                                      DB_ZONECENT,
                                      DB_FLOAT,
                                      result1);

      status = SiloLib_Write_MeshVar (famid, SILO_PLOTFILE,
                                      "Mesh_Shells_2",
                                      "processor_id2",
                                      time,
                                      1,
                                      1,
                                      num_shells,
                                      DB_ZONECENT,
                                      DB_FLOAT,
                                      result1);

      free(result1);
      status = SiloLIB_cddir(famid, SILO_PLOTFILE, "..") ;
   }


   /*****************/
   /* BEAM ELEMENTS */
   /*****************/

   /* Jump back to Root dir */
   status = SiloLIB_cddir(famid, SILO_PLOTFILE, "/") ;
   status = SiloLIB_getdir(famid, SILO_PLOTFILE, temp_string) ;

   num_beams = tgmesh->num_elem_beam;

   if (num_beams >0)
   {
      num_zones[0] = tgmesh->num_elem_beam;
      nl_length    = 0;
      status = SiloLIB_mkdir(famid, SILO_PLOTFILE, "Beams") ;
      status = SiloLIB_cddir(famid, SILO_PLOTFILE, "Beams") ;

      /* Identify the set of nodes that are for beam elements */
      for (i=0;
            i<num_beams;
            i++)
      {
         matnum = tgmesh->elem_beam_def[i].matnum;
         if ( mat_list[matnum] == OFF )
         {
            mat_list[matnum]     = ON;
         }

         mat_zone_list[i] = matnum;

         for (j=0;
               j<2;
               j++)
         {
            nodenum = tgmesh->elem_beam_def[i].nodes[j] - 1;
            if ( nodenum<0 )
            {
               printf("\n** Fatal error - node number of 0 found");
               exit(1);
            }
            node_list[nl_length++] = nodenum;
         }
      }

      /* Define a Mesh for the Beam Elements */

      zl_number  = 1;
      status = SiloLib_Write_UCD_Mesh(famid, SILO_PLOTFILE,
                                      "Mesh_Beams",
                                      ELEM_BEAM,
                                      group_num,
                                      3,
                                      node_list,
                                      nl_length,
                                      coord_names,
                                      (void *) coords,
                                      num_nodes,
                                      num_beams,
                                      NULL,
                                      NULL,
                                      zl_number,
                                      0.0,
                                      time,
                                      0,
                                      DB_FLOAT) ;


      /* Define Materials for the Beam Elements */

      status = SiloLib_Write_Material(famid, SILO_PLOTFILE,
                                      "Mesh_Beams",
                                      "Mat_Beams_1",
                                      &num_zones[0],
                                      num_mats,
                                      mat_nums,
                                      mat_zone_list,
                                      0.0,
                                      time,
                                      0,
                                      DB_DOUBLE) ;


      /* Define a Result variable for the bricks */
      status = SiloLIB_mkdir(famid, SILO_PLOTFILE, "Beams_Vars1") ;
      status = SiloLIB_cddir(famid, SILO_PLOTFILE, "Beams_Vars1") ;

      result1 = (float *) malloc(num_beams*sizeof(float));
      for (i=0;
            i<num_beams;
            i++)
         result1[i] = 3.0;

      status = SiloLib_Write_MeshVar (famid, SILO_PLOTFILE,
                                      "Mesh_Beams",
                                      "processor_id",
                                      time,
                                      1,
                                      1,
                                      num_beams,
                                      DB_ZONECENT,
                                      DB_FLOAT,
                                      result1);

      for (i=0;
            i<num_beams;
            i++)
         result1[i] = -10.0;
      status = SiloLib_Write_MeshVar (famid, SILO_PLOTFILE,
                                      "Mesh_Beams",
                                      "processor_id2",
                                      time,
                                      1,
                                      1,
                                      num_beams,
                                      DB_ZONECENT,
                                      DB_FLOAT,
                                      result1);

      free(result1);
      status = SiloLIB_cddir(famid, SILO_PLOTFILE, "..") ;
   }


   /**********************************/
   /* Create the multi-mesh - Bricks */
   /**********************************/
   mm_count  = 0;
   mat_count = 0;
   if (num_bricks>0)
   {
      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Bricks_1/Mesh_Bricks_1");
      meshnames[mm_count]  = strdup(meshname);
      meshtypes[mm_count]  = DB_UCDMESH;
      zonecounts[mm_count] = tgmesh->num_elem_hex/2;

      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Bricks_1/Mat_Bricks_1");
      mat_names[mat_count++] = strdup(meshname);

      mm_count++;

      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Bricks_2/Mesh_Bricks_2");
      meshnames[mm_count]  = strdup(meshname);
      meshtypes[mm_count]  = DB_UCDMESH;
      zonecounts[mm_count] = tgmesh->num_elem_hex/2;

      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Bricks_2/Mat_Bricks_2");
      mat_names[mat_count++] = strdup(meshname);

      mm_count++;

      /* Create the MultiMesh with Material and variables */
      status = SiloLIB_Write_Multimesh(famid, SILO_ROOTFILE,
                                       "Mili_Mesh_Bricks",
                                       mm_count,
                                       meshnames,
                                       meshtypes,
                                       zonecounts,
                                       extents,
                                       0,
                                       time);

      status = SiloLIB_Write_Multimat(famid, SILO_ROOTFILE,
                                      "Mili_Mats_Bricks",
                                      mat_count,
                                      &mat_names[0],
                                      num_mats,
                                      mat_nums,
                                      0,
                                      time);
   }



   /*********************************/
   /* Create the multi-mesh- Shells */
   /*********************************/
   mm_count  = 0;
   mat_count = 0;
   if (num_shells>0)
   {
      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Shells_1/Mesh_Shells_1");
      meshnames[mm_count]  = strdup(meshname);
      meshtypes[mm_count]  = DB_UCDMESH;
      zonecounts[mm_count] = tgmesh->num_elem_shell/2;

      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Shells_1/Mat_Shells_1");
      mat_names[mat_count++] = strdup(meshname);

      mm_count++;

      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Shells_2/Mesh_Shells_2");
      meshnames[mm_count]  = strdup(meshname);
      meshtypes[mm_count]  = DB_UCDMESH;
      zonecounts[mm_count] = tgmesh->num_elem_shell/2;
      strcpy(meshname, plotfile) ;

      strcat(meshname, ":");
      strcat(meshname, "Shells_2/Mat_Shells_2");
      mat_names[mat_count++] = strdup(meshname);

      mm_count++;
   }

   if (num_beams>0)
   {
      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Beams/Mesh_Beams");
      meshnames[mm_count]  = strdup(meshname);
      meshtypes[mm_count]  = DB_UCDMESH;
      zonecounts[mm_count] = tgmesh->num_elem_beam;
      mm_count++;
   }

   /* Create the MultiMesh with Material and variables */
   if (num_shells>0)
   {
      status = SiloLIB_Write_Multimesh(famid, SILO_ROOTFILE,
                                       "Mili_Mesh_Shells",
                                       mm_count,
                                       meshnames,
                                       meshtypes,
                                       zonecounts,
                                       extents,
                                       0,
                                       time);

      status = SiloLIB_Write_Multimat(famid, SILO_ROOTFILE,
                                      "Mili_Mats_Shells",
                                      mat_count,
                                      &mat_names[0],
                                      num_mats,
                                      mat_nums,
                                      0,
                                      time);
   }


   /********************************************/
   /* Create the multi-mesh- Bricks and Shells */
   /********************************************/
   mm_count = 0;
   strcpy(meshname, rootfile) ;
   strcat(meshname, ":");
   strcat(meshname, "Mili_Mesh_Bricks");
   meshnames[mm_count]  = strdup(meshname);
   meshtypes[mm_count]  = DB_UCDMESH;
   zonecounts[mm_count] = tgmesh->num_elem_hex;

   mm_count++;

   strcpy(meshname, rootfile) ;
   strcat(meshname, ":");
   strcat(meshname, "Mili_Mesh_Shells");
   meshnames[mm_count]  = strdup(meshname);
   meshtypes[mm_count]  = DB_UCDMESH;
   zonecounts[mm_count] = tgmesh->num_elem_shell;

   mm_count++;
   status = SiloLIB_Write_Multimesh(famid, SILO_ROOTFILE,
                                    "Mili_Mesh",
                                    mm_count,
                                    meshnames,
                                    meshtypes,
                                    zonecounts,
                                    extents,
                                    0,
                                    time);


   /* Write Mesh Vars */

   mm_count=0;
   if (num_bricks>0)
   {
      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Bricks_Vars1/processor_id");
      meshnames[mm_count]  = strdup(meshname);
      meshtypes[mm_count]  = DB_UCDVAR;
      mm_count++;

      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Bricks_Vars2/processor_id");
      meshnames[mm_count]  = strdup(meshname);
      meshtypes[mm_count]  = DB_UCDVAR;
      mm_count++;
   }

   if (num_shells>0)
   {
      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Shells_Vars2/processor_id");
      meshnames[mm_count]  = strdup(meshname);
      meshtypes[mm_count]  = DB_UCDVAR;
      mm_count++;
   }

   if (num_beams>0)
   {
      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Beams_Var1/processor_id");
      meshnames[mm_count]  = strdup(meshname);
      meshtypes[mm_count]  = DB_UCDVAR;
      mm_count++;
   }

   status = SiloLIB_Write_MultiVar(famid, SILO_ROOTFILE,
                                   "processor_id",
                                   mm_count,
                                   meshnames,
                                   meshtypes,
                                   cycle,
                                   time);

   mm_count=0;
   if (num_bricks>0)
   {
      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Bricks_Vars2/processor_id2");
      meshnames[mm_count]  = strdup(meshname);
      meshtypes[mm_count]  = DB_UCDVAR;
      mm_count++;
   }

   if (num_shells>0)
   {

      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Shells_Vars1/processor_id2");
      meshnames[mm_count]  = strdup(meshname);
      meshtypes[mm_count]  = DB_UCDVAR;
      mm_count++;

      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Shells_Vars2/processor_id2");
      meshnames[mm_count]  = strdup(meshname);
      meshtypes[mm_count]  = DB_UCDVAR;
      mm_count++;
   }

   if (num_beams>0)
   {
      strcpy(meshname, plotfile) ;
      strcat(meshname, ":");
      strcat(meshname, "Beams_Var1/processor_id");
      meshnames[mm_count]  = strdup(meshname);
      meshtypes[mm_count]  = DB_UCDVAR;
      mm_count++;
   }

   status = SiloLIB_Write_MultiVar(famid, SILO_ROOTFILE,
                                   "processor_id2",
                                   mm_count,
                                   meshnames,
                                   meshtypes,
                                   cycle,
                                   time);

   /*********************/
   /* SLIDE INTERFACES  */
   /*********************/

   if (num_slides>0)
   {
      status = SiloLIB_mkdir(famid, SILO_PLOTFILE, "Slides") ;
      status = SiloLIB_cddir(famid, SILO_PLOTFILE, "Slides") ;

      zl_number  = 1;
      num_slides = tgmesh->num_slides;
      mm_count   = 0;
      nl_length  = 0;
   }


   for (slide_index=0;
         slide_index<num_slides;
         slide_index++)
   {

      /* Ignore Slide types 8, */
      if ( tgmesh->slide_def[slide_index].type==7 ||  tgmesh->slide_def[slide_index].type==8 )
         continue;


      /**********/
      /* Slaves */
      /**********/

      num_zones[0] = tgmesh->slide_def[slide_index].numslaves;
      nl_length    = 0;

      matnum = tgmesh->slide_def[slide_index].type;

      /* Identify the set of nodes that are for this slides Slave */
      for (i=0;
            i<num_zones[0];
            i++)
      {
         for (j=0;
               j<4;
               j++)
         {
            nodenum = tgmesh->slide_def[slide_index].slide_nodes_slaves[i].nodes[j] - 1;
            if ( nodenum<0 )
            {
               printf("\n** Fatal error - node number of 0 found (Slide Slave)");
               exit(1);
            }
            node_list[nl_length++] = nodenum;
         }
      }

      /* Reverse order of node list to put in right-hand format */
      for (k=0;
            k<nl_length;
            k+=4)
      {
         for (j=0;
               j<2;
               j++)
         {
            temp             = node_list[k+3-j];
            node_list[k+3-j] = node_list[k+j];
            node_list[k+j]   = temp;
         }
      }

      /* Define a Mesh for the Slide Elements - Slaves */
      if (nl_length>0)
      {
         sprintf(meshname, "ELEM_SLIDE_%04d_SLAVE", slide_index);
         status = SiloLib_Write_UCD_Mesh(famid, SILO_PLOTFILE,
                                         meshname,
                                         ELEM_SLIDE,
                                         group_num,
                                         3,
                                         node_list,
                                         nl_length,
                                         coord_names,
                                         (void *) coords,
                                         num_nodes,
                                         num_zones[0],
                                         NULL,
                                         NULL,
                                         zl_number++,
                                         0.0,
                                         time,
                                         0,
                                         DB_FLOAT) ;

         /* Add this mesh to the Multi-mesh list */
         strcpy(temp_string, plotfile) ;
         strcat(temp_string, ":Slides/");
         strcat(temp_string, meshname);

         meshnames[mm_count]  = strdup(temp_string);
         meshtypes[mm_count]  = DB_UCDMESH;
         zonecounts[mm_count] = num_zones[0];
         mm_count++;
      }


      /***********/
      /* Masters */
      /***********/

      num_zones[0] = tgmesh->slide_def[slide_index].nummasters;
      nl_length    = 0;

      matnum = tgmesh->slide_def[slide_index].type;

      /* Identify the set of nodes that are for this slides Master */
      for (i=0;
            i<num_zones[0];
            i++)
      {
         for (j=0;
               j<4;
               j++)
         {
            nodenum = tgmesh->slide_def[slide_index].slide_nodes_masters[i].nodes[j] - 1;
            if ( nodenum<0 )
            {
               printf("\n** Fatal error - node number of 0 found (Slide Master)");
               exit(1);
            }
            node_list[nl_length++] = nodenum;
         }
      }

      /* Reverse order of node list to put in right-hand format */
      for (k=0;
            k<nl_length;
            k+=4)
      {
         for (j=0;
               j<2;
               j++)
         {
            temp             = node_list[k+3-j];
            node_list[k+3-j] = node_list[k+j];
            node_list[k+j]   = temp;
         }
      }

      /* Define a Mesh for the Slide Elements - Masters */
      if (nl_length>0)
      {
         sprintf(meshname, "ELEM_SLIDE_%04d_MASTER", slide_index);
         status = SiloLib_Write_UCD_Mesh(famid, SILO_PLOTFILE,
                                         meshname,
                                         ELEM_SLIDE,
                                         group_num,
                                         3,
                                         node_list,
                                         nl_length,
                                         coord_names,
                                         (void *) coords,
                                         num_nodes,
                                         num_zones[0],
                                         NULL,
                                         NULL,
                                         zl_number++,
                                         0.0,
                                         time,
                                         0,
                                         DB_FLOAT) ;

         /* Add this mesh to the Multi-mesh list */
         strcpy(temp_string, plotfile) ;
         strcat(temp_string, ":Slides/");
         strcat(temp_string, meshname);

         meshnames[mm_count]  = strdup(temp_string);
         meshtypes[mm_count]  = DB_UCDMESH;
         zonecounts[mm_count] = num_zones[0];
         mm_count++;
      }
   }


   if (num_slides>0)
   {
      status = SiloLIB_cddir(ROOT_FILE, "..") ;
      status = SiloLIB_cddir(famid, SILO_PLOTFILE, "..") ;

      /* Create the multi-mesh */
      status = SiloLIB_Write_Multimesh(famid, SILO_ROOTFILE,
                                       "Mili_Mesh_Slides",
                                       mm_count,
                                       meshnames,
                                       meshtypes,
                                       zonecounts,
                                       extents,
                                       0,
                                       time);
   }


   /* Write and read some simple variables */
   var1 = 1234567890.0;
   status = SiloLib_Write_Var (famid, SILO_PLOTFILE, "var1", 0., 1, 1, 1, DB_DOUBLE, &var1);

   for(i=0;
         i<20;
         i++)
      var2[i] = i*10.;
   status = SiloLib_Write_Var (famid, SILO_PLOTFILE, "var2", 0., 1, 1, 20, DB_DOUBLE, &var2);

   status = SiloLIB_GetVarInfo(famid, SILO_PLOTFILE, "var1", dims, &varlen, &vartype);

   status = SiloLIB_GetVarInfo(famid, SILO_PLOTFILE, "var2", dims, &varlen, &vartype);

   status = Silo_Read_Var (famid, SILO_PLOTFILE, "var1", &var1);

   /* Write and read a text file */
   /* status = SiloLIB_Write_TextFile(famid, SILO_PLOTFILE, "trugrdo.64", 0., 1, "file1");

   status = SiloLIB_Read_TextFile(famid, SILO_PLOTFILE, "trugrdo-2.64", "file1");*/


   /* First get the counts for the difference variables types */

   status = SiloLIB_getTOC(famid,
                           SILO_PLOTFILE,
                           &num_dirs,     NULL,
                           &num_vars,     NULL,
                           &num_meshvars, NULL,
                           &num_ptvars,   NULL);

   dir_names = (char **) malloc(num_dirs*sizeof(char *));
   var_names = (char **) malloc(num_vars*sizeof(char *));
   mesh_vars = (char **) malloc(num_meshvars*sizeof(char *));

   status = SiloLIB_getTOC(famid,
                           SILO_PLOTFILE,
                           &num_dirs,     &dir_names[0],
                           &num_vars,     &var_names[0],
                           &num_meshvars, &mesh_vars[0],
                           &num_ptvars,   &pt_vars[0]);

   /* All done! */
   /* Close the Silo files */

   status = SiloLib_Close_Restart_File(famid, SILO_PLOTFILE);
   status = SiloLib_Close_Restart_File(famid, SILO_PLOTFILE);
   status = SiloLib_Close_Restart_File(famid, SILO_PLOTFILE);

   /* Release memory and exit */

   free(mat_list);
   free(mat_zone_list);
   free(mat_nums);
   free(node_list);

   exit(1);
}


int parse_command_line(int argc, char **argv)
/****************************************************************/
/* This routine will test the Silo I/O Library by setting up    */
/* a mesh read in from a TrueGrid output file.                  */
/*                                                              */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*  12-Jun-2007  IRC  Initial version.                          */
/****************************************************************/
{
   /* Parse the input arguments and display the
    * help text if the -h option is input.
    *
    */
   int  arg_cnt;
   char arg_next[128];       /* The next argument from the
			     * command line
			     */
   for (arg_cnt=0;
         arg_cnt<argc;
         arg_cnt++)
   {
      strcpy(arg_next,argv[arg_cnt]); /* arg_next - used for debugging */

      if (!strncmp(argv[arg_cnt],"-debug",6) ||
            !strncmp(argv[arg_cnt],"-DEBUG",6))
      {
         debug_flag = ON;
      }


      if (!strncmp(argv[arg_cnt],"-i",2) ||
            !strncmp(argv[arg_cnt],"-input",6))
      {
         strcpy(filein, argv[arg_cnt+1]);
         arg_cnt++;
      }

      if (!strncmp(argv[arg_cnt],"-o",2) ||
            !strncmp(argv[arg_cnt],"-output",7))
      {
         strcpy(root_filename, argv[arg_cnt+1]);
         arg_cnt++;
      }

      if (!strncmp(argv[arg_cnt],"-p",2) ||
            !strncmp(argv[arg_cnt],"-part",5))
      {
         strcpy(partfile_in, argv[arg_cnt+1]);
         partfile_found = TRUE;
         arg_cnt++;
      }

      if (!strncmp(argv[arg_cnt],"-help",5) ||
            !strncmp(argv[arg_cnt],"-HELP",5))
      {
         printf("\nSilo I/O Library Driver (SiloLib_TestDriver)");
         printf("\n\tOptions:");
         printf("\n-i filename\t\tInput  filename");
         printf("\n-o filename\t\tOutput filename");
         printf("\n-p filename\t\tInpit Partition filename");
         exit(1);
      }
   }

   return (OK);

}

/* End SiloLib_TestDriver.c */


