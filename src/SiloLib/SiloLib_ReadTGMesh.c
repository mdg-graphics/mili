/*
 * $Log: SiloLib_ReadTGMesh.c,v $
 * Revision 1.4  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.3  2008/09/24 18:36:01  corey3
 * New update of Passit source files
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
/*                                                              */
/* SiloLib_ReadTGMesh: This function will read a TG mesh        */
/* into an internal data structure.                             */
/*                                                              */
/****************************************************************/
/*                                                              */
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  25-May-2007  IRC  Initial version.                          */
/*                                                              */
/****************************************************************/

/*  Include Files  */

#define ENABLE_SILO

#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "silo.h"
#include "SiloLib_Internal.h"

int SiloLIB_ReadTGMesh(TGMESH_type **mesh, char *filename)
{
   FILE *fp;
   int  i, j, index, slide_index;
   int  mat_count=0;
   int  block_len=7;
   int  node0;

   char dummy[32], dummy_int;
   char linein[256], last_linein[256];

   int num_slides = 0;

   TGMESH_type *meshout;
   meshout = (TGMESH_type *) malloc(1*sizeof(TGMESH_type)) ;

   fp = fopen(filename, "r");
   if (fp==NULL)
      return(FILE_NOT_FOUND);

   /* Read in the totals for all object types
    * so we can allocate memory
    */
   while (fgets(linein, 256, fp)!=NULL)
   {
      if ( strstr(linein, "88 large") )
         continue;

      if ( linein[0] != '*' && linein[0] != '$' )
      {
         sscanf(linein, "%d %d %d %d %d %d", &meshout->num_mats,      &meshout->num_np,
                &meshout->num_elem_hex,      &meshout->num_elem_beam, &meshout->num_elem_shell,
                &meshout->num_elem_tshell );
         break;
      }
   }


   /* Jump to Control Card #5 */
   index = 0;
   while (fgets(linein, 256, fp)!=NULL)
   {
      if ( strstr(linein, "CONTROL CARD #5") )
         break;
   }

   while (fgets(linein, 256, fp)!=NULL)
   {
      if ( linein[0] != '*' && linein[0] != '$' )
      {
         sscanf(linein, "%d %d %d %d %d %d %d %d", &dummy_int, &dummy_int, &dummy_int, &dummy_int, &dummy_int,
                &meshout->num_nodal_constraints, &dummy_int, &meshout->num_slides);
         break;
      }
   }



   /* Allocate memory for all arrays */

   meshout->mat_def         = (mat_def_type *)  malloc( meshout->num_mats*sizeof(mat_def_type) ) ;
   meshout->node_def        = (node_def_type *) malloc( meshout->num_np*sizeof(node_def_type) ) ;
   meshout->elem_hex_def    = (elem_hex_def_type *) malloc( meshout->num_elem_hex*sizeof(elem_hex_def_type) ) ;
   meshout->elem_beam_def   = (elem_beam_def_type *) malloc( meshout->num_elem_beam*sizeof(elem_beam_def_type) ) ;
   meshout->elem_shell_def  = (elem_shell_def_type *) malloc( meshout->num_elem_shell*sizeof(elem_shell_def_type) ) ;
   meshout->elem_tshell_def = (elem_tshell_def_type *) malloc( meshout->num_elem_tshell*sizeof(elem_tshell_def_type) ) ;
   meshout->slide_def       = (slide_def_type *) malloc( meshout->num_slides*sizeof(slide_def_type) ) ;


   /* Jump to the Material Cards */
   index = 0;
   while (fgets(linein, 256, fp)!=NULL)
   {
      if ( strstr(linein, "MATERIAL CARDS") )
         break;
   }


   /* Read in the material definitions */
   index     = 0;
   block_len = 7;
   while (fgets(linein, 256, fp)!=NULL)
   {
      if ( strstr(linein, "NODE DEFINITIONS") )
         break;

      /* Skip section props if they exit */

      i=0;
      if ( strstr(linein, "section properties") )
         while (fgets(linein, 256, fp)!=NULL && i<2)
         {
            /* Skip comment lines */
            if ( linein[0] == '*' || linein[0] == '$' )
               continue ;
            i++;
         }

      /* Skip comment lines */
      if ( linein[0] == '*' || linein[0] == '$' )
         continue ;

      if (index==0)
         sscanf(linein, "%d", &meshout->mat_def[mat_count].matnum);

      if (index==1)
         sscanf(linein, "%s %s %s %d", dummy, dummy, dummy, &meshout->mat_def[mat_count++].mattype);

      /* 10 lines per material block */
      index++;
      if (index>block_len)
         index=0;
   }


   /* Read in the Node definitions */
   index = 0;
   while (fgets(linein, 256, fp)!=NULL)
   {
      if ( strstr(linein, "ELEMENT CARDS FOR"))
         break;

      if ( linein[0] != '*' && linein[0] != '$' )
      {
         sscanf(linein, "%d %lf %lf %lf %lf %lf",
                &meshout->node_def[index].nodenum,
                &meshout->node_def[index].displacement,
                &meshout->node_def[index].pos[0],
                &meshout->node_def[index].pos[1],
                &meshout->node_def[index].pos[2],
                &meshout->node_def[index].rotation);
         index++;
      }
   }

   /* Read in the Solid Element definitions */
   index = 0;
   if ( strstr(linein, "ELEMENT CARDS FOR SOLID ELEMENTS") )
      while (fgets(linein, 256, fp)!=NULL)
      {
         if ( strstr(linein, "ELEMENT CARDS FOR") ||
               strstr(linein, "*-----") ) /* Next element set */
            break;

         if ( linein[0] != '*' && linein[0] != '$' )
         {
            sscanf(linein, "%d %d %d %d %d %d %d %d %d %d",
                   &meshout->elem_hex_def[index].elemnum,
                   &meshout->elem_hex_def[index].matnum,
                   &meshout->elem_hex_def[index].nodes[0],
                   &meshout->elem_hex_def[index].nodes[1],
                   &meshout->elem_hex_def[index].nodes[2],
                   &meshout->elem_hex_def[index].nodes[3],
                   &meshout->elem_hex_def[index].nodes[4],
                   &meshout->elem_hex_def[index].nodes[5],
                   &meshout->elem_hex_def[index].nodes[6],
                   &meshout->elem_hex_def[index].nodes[7],
                   &meshout->elem_hex_def[index].nodes[8]);
            index++;
         }
      }

   /* Read in the Beam definitions */
   index = 0;
   if ( strstr(linein, "ELEMENT CARDS FOR BEAM ELEMENTS") )
      while (fgets(linein, 256, fp)!=NULL)
      {
         if ( strstr(linein, "ELEMENT CARDS FOR") ||
               strstr(linein, "*-----") ) /* Next element set */
            break;

         if ( linein[0] != '*' && linein[0] != '$' )
         {
            sscanf(linein, "%d %d %d %d %d %d",
                   &meshout->elem_beam_def[index].elemnum,
                   &meshout->elem_beam_def[index].matnum,
                   &meshout->elem_beam_def[index].nodes[0],
                   &meshout->elem_beam_def[index].nodes[1],
                   &meshout->elem_beam_def[index].nodes[2],
                   &meshout->elem_beam_def[index].nodes[3]);
            index++;
         }
      }

   /* Read in the Shell definitions */
   index = 0;
   if ( strstr(linein, "ELEMENT CARDS FOR SHELL ELEMENTS") )
      while (fgets(linein, 256, fp)!=NULL)
      {
         if ( strstr(linein, "ELEMENT CARDS FOR") ||
               strstr(linein, "*-----") ) /* Next element set */
            break;

         if ( linein[0] != '*' && linein[0] != '$' )
         {
            sscanf(linein, "%d %d %d %d %d %d",
                   &meshout->elem_shell_def[index].elemnum,
                   &meshout->elem_shell_def[index].matnum,
                   &meshout->elem_shell_def[index].nodes[0],
                   &meshout->elem_shell_def[index].nodes[1],
                   &meshout->elem_shell_def[index].nodes[2],
                   &meshout->elem_shell_def[index].nodes[3]);

            fgets(linein, 256, fp);
            sscanf(linein, "%lf %lf %lf %lf",
                   &meshout->elem_shell_def[index].thickness[0],
                   &meshout->elem_shell_def[index].thickness[1],
                   &meshout->elem_shell_def[index].thickness[2],
                   &meshout->elem_shell_def[index].thickness[3]);
            index++;
         }
      }

   /* Skip to the Slide Surfaces */
   index = 0;
   while (fgets(linein, 256, fp)!=NULL)
   {
      if ( strstr(linein, "SLIDING INTERFACE DEFINITIONS") )
         break;
   }

   index = 0;

   /* Read the Slide Surface definitions */
   num_slides = meshout->num_slides;
   if ( strstr(linein, "SLIDING INTERFACE DEFINITIONS") )
      while (fgets(linein, 256, fp)!=NULL)
      {
         if ( linein[0] != '*' && linein[0] != '$' )
         {
            sscanf(linein, "%d %d %d", &meshout->slide_def[index].numslaves, &meshout->slide_def[index].nummasters,
                   &meshout->slide_def[index].type);

            meshout->slide_def[index].slide_nodes_slaves =
               (slide_node_type *) malloc(meshout->slide_def[index].numslaves*sizeof(slide_node_type) ) ;

            meshout->slide_def[index].slide_nodes_masters =
               (slide_node_type *) malloc(meshout->slide_def[index].nummasters*sizeof(slide_node_type) ) ;

            index++;
         }

         if (index==num_slides)
            break;
      }

   /* Read the Slide Surface nodes */

   for (slide_index=0;
         slide_index<num_slides;
         slide_index++)
   {

      /* Read in Slave nodes */

      index = 0;
      for (j=0;
            j<meshout->slide_def[slide_index].numslaves;
            j++)
      {
         fgets(linein, 256, fp);
         if (j==0)
         {
            j=0;
         }

         if ( linein[0] == '*' || linein[0] == '$' )
         {
            j--;
            continue;
         }

         if ( strstr(linein, "RIGID ") )
            break;

         if (strlen(linein)>20)
         {
            sscanf(linein, "%d %d %d %d %d",
                   &meshout->slide_def[slide_index].slide_nodes_slaves[index].num,
                   &meshout->slide_def[slide_index].slide_nodes_slaves[index].nodes[0],
                   &meshout->slide_def[slide_index].slide_nodes_slaves[index].nodes[1],
                   &meshout->slide_def[slide_index].slide_nodes_slaves[index].nodes[2],
                   &meshout->slide_def[slide_index].slide_nodes_slaves[index].nodes[3]);
            index++;
         }
         else
         {
            sscanf(linein, "%d %d",
                   &meshout->slide_def[slide_index].slide_nodes_slaves[index].num,
                   &meshout->slide_def[slide_index].slide_nodes_slaves[index].nodes[0]);

            node0 = meshout->slide_def[slide_index].slide_nodes_slaves[index].nodes[0];

            meshout->slide_def[slide_index].slide_nodes_slaves[index].nodes[1] = node0;
            meshout->slide_def[slide_index].slide_nodes_slaves[index].nodes[2] = node0;
            meshout->slide_def[slide_index].slide_nodes_slaves[index].nodes[3] = node0;
            index++;
         }
      } /* Slaves */


      /* Read in Master nodes */

      index = 0;
      for (j=0;
            j<meshout->slide_def[slide_index].nummasters;
            j++)
      {
         if (j==0)
         {
            j=0;
         }

         fgets(linein, 256, fp);

         if ( linein[0] == '*' || linein[0] == '$' )
         {
            j--;
            continue;
         }

         if ( strstr(linein, "RIGID ") )
            break;

         if (strlen(linein)>20)
         {
            sscanf(linein, "%d %d %d %d %d",
                   &meshout->slide_def[slide_index].slide_nodes_masters[index].num,
                   &meshout->slide_def[slide_index].slide_nodes_masters[index].nodes[0],
                   &meshout->slide_def[slide_index].slide_nodes_masters[index].nodes[1],
                   &meshout->slide_def[slide_index].slide_nodes_masters[index].nodes[2],
                   &meshout->slide_def[slide_index].slide_nodes_masters[index].nodes[3]);
            index++;
         }
         else
         {
            sscanf(linein, "%d %d",
                   &meshout->slide_def[slide_index].slide_nodes_masters[index].num,
                   &meshout->slide_def[slide_index].slide_nodes_masters[index].nodes[0]);

            node0 = meshout->slide_def[slide_index].slide_nodes_masters[index].nodes[0];

            meshout->slide_def[slide_index].slide_nodes_masters[index].nodes[1] = node0;
            meshout->slide_def[slide_index].slide_nodes_masters[index].nodes[2] = node0;
            meshout->slide_def[slide_index].slide_nodes_masters[index].nodes[3] = node0;
            index++;
         }
      } /* Masters */

      if ( strstr(linein, "RIGID ") )
         break;
   }


   if ( !strstr(linein, "DISCRETE SPRINGS,") )
      while (fgets(linein, 256, fp)!=NULL)
      {
         if ( strstr(linein, "DISCRETE SPRINGS,") )
            break;
      }


   /* Read Springs, dampers, and mass-nodes */

   if ( strstr(linein, "DISCRETE SPRINGS,") )
      while (fgets(linein, 256, fp)!=NULL)
      {
         if ( linein[0] != '*' && linein[0] != '$' )
         {
            sscanf(linein, "%d %d %d", &meshout->num_springs, &meshout->num_dampers, &meshout->num_node_mass);
            break;
         }
      }


   /* Do not read in this section for now

   for (i=0;
   i<meshout->num_springs;
   i++)
   {
        fgets(linein, 256, fp);
   }

   for (i=0;
   i<meshout->num_dampers;
   i++)
   {
        fgets(linein, 256, fp);
   }

   if( meshout->num_node_mass >0 )
       meshout->node_mass_def = (node_mass_def_type *) malloc( meshout->num_node_mass*sizeof(node_mass_def_type) ) ;

   for (i=0;
   i<meshout->num_node_mass;
   i++)
   {
        fgets(linein, 256, fp);
   sscanf(linein, "%d %lf", &meshout->node_mass_def[index].nodenum, &meshout->node_mass_def[index].mass);;
   } */


   fclose(fp);

   *mesh = meshout;

   return (1);
}

/* End SiloLib_ReadTGMesh.c */

