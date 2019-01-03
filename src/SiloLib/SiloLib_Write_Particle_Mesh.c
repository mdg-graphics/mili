/*
 * $Log: SiloLib_Write_Particle_Mesh.c,v $
 * Revision 1.4  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.3  2008/09/24 18:36:02  corey3
 * New update of Passit source files
 *
 * Revision 1.2  2007/07/31 19:18:38  corey3
 * Add new options to particle mesh type
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

/*  Include Files  */
#include <string.h>
#include "silo.h"
#include "SiloLib_Internal.h"


int SILOLIB_Write_Particle_Mesh(db,
                                time,
                                time_step,
                                cycle,
                                num_particles,
                                coords_x,
                                coords_y,
                                coords_z)

/****************************************************************/
/*  This function will write the particle field to a point mesh */
/*  object.                                                     */
/****************************************************************/

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  18-Jan-2005  IRC  Initial version.                          */
/*                                                              */
/*  17-Apr-2007  IRC  Adapted for Mili                          */
/****************************************************************/

DBfile *db ;

double time ;
double time_step ;
int    cycle ;

int    num_particles ;
float  *coords_x, *coords_y, *coords_z ;

{
   int     i ;
   int     cell_index ;
   int     group=1;
   int     num_cells;
   double  coord_init;
   char   *coord_names[3] ;
   char   *mesh_name = "LB_Particles" ;
   float  *coords[3] ;
   int    status=OK;

   DBoptlist *optlist;

   coord_names[X]  = (char *) strdup("X-Coord") ;
   coord_names[Y]  = (char *) strdup("Y-Coord") ;
   coord_names[Z]  = (char *) strdup("Z-Coord") ;

   coords[X] = coords_x ;
   coords[Y] = coords_y ;
   coords[Z] = coords_z ;

   /* Make quadvar option list */

   optlist = DBMakeOptlist(10) ;
   DBAddOption(optlist, DBOPT_GROUPNUM, (void *)&group) ;
   DBAddOption(optlist, DBOPT_CYCLE, (void *)&cycle) ;
   DBAddOption(optlist, DBOPT_TIME, (void *)&time) ;
   DBAddOption(optlist, DBOPT_DTIME, (void *)&time_step) ;

   DBAddOption(optlist, DBOPT_XUNITS, "cm");
   DBAddOption(optlist, DBOPT_YUNITS, "cm");
   DBAddOption(optlist, DBOPT_ZUNITS, "cm");

   DBAddOption(optlist, DBOPT_XLABEL, "X Axis") ;
   DBAddOption(optlist, DBOPT_YLABEL, "Y Axis") ;
   DBAddOption(optlist, DBOPT_ZLABEL, "Z Axis") ;

   /* Write the Point Mesh */
   status = DBPutPointmesh(db,
                           mesh_name,
                           3,
                           coords,
                           num_particles,
                           DB_FLOAT,
                           optlist) ;

   DBFreeOptlist(optlist);

   return(status) ;
}

/* End SiloLib_Write_Particle_Mesh.c */
