/*
 * $Log: SiloLib_Write_Quad_Mesh.c,v $
 * Revision 1.3  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.2  2008/09/24 18:36:02  corey3
 * New update of Passit source files
 *
 * Revision 1.1  2007/06/25 17:58:32  corey3
 * New Mili/Silo interface
 *
 *
*/


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*                                                              */
/*                   Copyright (c) 2005                         */
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

/*  Include Files  */
#include "string.h"
#include "silo.h"
#include "stdlib.h"
#include "SiloLib_Internal.h"

int SiloLib_Write_Quad_Mesh(DBfile *db,
                            double time,
                            double time_step,
                            int    cycle,
                            int    *num_zones, /* x,y,z */
                            double zone_size,
                            int   *quad_data)

/****************************************************************/
/*  This function will write the mesh and material data to a    */
/*  Silo file.                                                  */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  20-Jan-2005  IRC  Initial version.                          */
/****************************************************************/
{
   int     i ;
   int     cell_index ;
   int     num_cells;
   int     num_points[3] ;
   int     dims_point[3] ;
   double  coord_init;
   float  *coord_arrays[3] ;
   char   *coord_names[3] ;
   char   *mesh_name = "LB_ThreeD_Structured" ;

   /* Mixed materials */
   int     all_clean = TRUE ;
   int     max_index ;
   int    *mix_mat ;
   int    *mix_matlist ;
   int    *mix_next ;
   int    *mix_zone ;
   float  *mix_vf ;
   int     mat_nos[2] = {1,2} ;
   int     next_mat_index = 1 ;
   int     xindex=0, yindex=0, zindex=0 ;
   double  vf ;

   int     quad_index ;
   float  *coord_x ;
   float  *coord_y ;
   float  *coord_z ;

   int     temp ;
   int     status=OK;

   /* optlist variables */
   int        origin = 0 ;
   int        majororder = 0 ;
   int        hi[3]  = {0,0,0} ;
   int        low[3] = {0,0,0} ;
   int        coordsys;
   int        planar;
   DBoptlist *optlist;

   for (i=0;
         i<3;
         i++)
      if (num_zones[i]<=0)
         num_zones[i] = 1 ;

   num_points[X] = num_zones[X]+1 ;
   num_points[Y] = num_zones[Y]+1 ;
   num_points[Z] = num_zones[Z]+1 ;

   /* Initialize the coordinates for each axis */
   coord_x = (float *) malloc(num_points[X]*sizeof(float));
   coord_y = (float *) malloc(num_points[Y]*sizeof(float));
   coord_z = (float *) malloc(num_points[Z]*sizeof(float));

   coord_init = 0.;
   for (i = 0;
         i < num_points[X];
         i++)
   {
      coord_x[i]  = coord_init ;
      coord_init += zone_size ;
   }
   coord_init = 0.;
   for (i = 0;
         i < num_points[Y];
         i++)
   {
      coord_y[i]  = coord_init ;
      coord_init += zone_size ;
   }
   coord_init = 0.;
   for (i = 0;
         i < num_points[Z];
         i++)
   {
      coord_z[i]  = coord_init ;
      coord_init += zone_size ;
   }

   coord_names[X]  = (char *) strdup("X-Coord") ;
   coord_names[Y]  = (char *) strdup("Y-Coord") ;
   coord_names[Z]  = (char *) strdup("Z-Coord") ;

   coord_arrays[X] = coord_x;
   coord_arrays[Y] = coord_y;
   coord_arrays[Z] = coord_z;

   /* Define nodal dimensions of mesh */
   dims_point[X] = num_points[X];
   dims_point[Y] = num_points[Y];
   dims_point[Z] = num_points[Z];

   /* Make quadvar option list */
   optlist = DBMakeOptlist(16) ;
   DBAddOption(optlist, DBOPT_CYCLE, (void *)&cycle) ;
   DBAddOption(optlist, DBOPT_DTIME, (void *)&time) ;
   DBAddOption(optlist, DBOPT_MAJORORDER, (void *)&majororder) ;
   DBAddOption(optlist, DBOPT_ORIGIN, (void *)&origin) ;
   DBAddOption(optlist, DBOPT_HI_OFFSET, (void *)&hi) ;
   DBAddOption(optlist, DBOPT_LO_OFFSET, (void *)&low) ;
   DBAddOption(optlist, DBOPT_XUNITS, "cm");
   DBAddOption(optlist, DBOPT_YUNITS, "cm");
   DBAddOption(optlist, DBOPT_ZUNITS, "cm");

   planar   = DB_AREA ;
   coordsys = DB_CARTESIAN ;
   DBAddOption(optlist, DBOPT_PLANAR, (void *)&planar) ;
   DBAddOption(optlist, DBOPT_COORDSYS, (void *)&coordsys) ;
   DBAddOption(optlist, DBOPT_XLABEL, "X Axis") ;
   DBAddOption(optlist, DBOPT_YLABEL, "Y Axis") ;
   DBAddOption(optlist, DBOPT_ZLABEL, "Z Axis") ;

   /* Write the Nodal Mesh */
   status = DBPutQuadmesh(db,
                          mesh_name,
                          coord_names,
                          coord_arrays,
                          dims_point,
                          3,
                          DB_FLOAT,
                          DB_COLLINEAR,
                          optlist) ;

   /* Allocate space for the mixed-material data structures */
   num_cells = num_zones[X]*num_zones[Y]*num_zones[Z] ;
   max_index = num_cells*2 ;

   mix_matlist = (int *)    malloc(num_cells*sizeof(int)) ;
   mix_mat     = (int *)    malloc(max_index*sizeof(int)) ;
   mix_next    = (int *)    malloc(max_index*sizeof(int)) ;
   mix_zone    = (int *)    malloc(max_index*sizeof(int)) ;
   mix_vf      = (float *)  malloc(max_index*sizeof(float)) ;

   for (i=0;
         i<num_cells;
         i++)
   {
      mix_matlist[i] = 1;
      mix_mat[i]     = 1;
      if (quad_data[i] > 1)
         all_clean = FALSE ;
   }

   /* Set to all clean for now */
   all_clean = TRUE ;

   for (i=0;
         i<max_index;
         i++)
   {
      mix_next[i] = 0;
      mix_zone[i] = 0;
      mix_vf[i]   = 1.0;
   }

   cell_index = 0;
   for (zindex = 0;
         zindex < num_zones[Z];
         zindex++)

      for (yindex = 0;
            yindex < num_zones[Y];
            yindex++)

         for (xindex = 0;
               xindex < num_zones[X];
               xindex++)
         {
            quad_index = (zindex*num_zones[X]*num_zones[Y]) +
                         (yindex*num_zones[X])+
                         xindex;

            if (all_clean)
            {

               /* Clean Zones */

               if (quad_data[quad_index] <100)
                  mix_matlist[cell_index] = 1 ;
               else
                  mix_matlist[cell_index] = 2 ;
            }
            else
            {
               /* Mixed Zones */

               mix_matlist[cell_index]    = -next_mat_index ;

               mix_zone[next_mat_index-1] = quad_index ;

               vf  =  (double) quad_data[quad_index]/255.0 ;
               mix_mat[next_mat_index-1]  = 1 ;
               mix_vf[next_mat_index-1]   = (float) vf ;
               mix_next[next_mat_index-1] = next_mat_index+1 ;

               next_mat_index++ ;

               mix_zone[next_mat_index-1] = quad_index ;
               vf  =  1.0 - ((double) quad_data[quad_index]/255.0) ;
               mix_vf[next_mat_index-1]   = (float) vf ;
               mix_mat[next_mat_index-1]  = 2 ;
               mix_next[next_mat_index-1] = 0 ;

               next_mat_index++ ;
            }
            cell_index++ ;
         }

   status = DBPutMaterial(db,
                          "Material",
                          mesh_name,
                          2,
                          mat_nos,
                          mix_matlist,
                          num_zones,
                          3,
                          mix_next,
                          mix_mat,
                          mix_zone,
                          mix_vf,
                          next_mat_index-1,
                          DB_FLOAT,
                          optlist) ;

   free(mix_matlist) ;
   free(mix_mat) ;
   free(mix_next) ;
   free(mix_zone) ;
   free(mix_vf) ;

   DBFreeOptlist(optlist);
   return(status) ;

}

/* End SiloLib_Write_Mesh.c */
