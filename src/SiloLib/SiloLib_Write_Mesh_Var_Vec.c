/*
 * $Log: SiloLib_Write_Mesh_Var_Vec.c,v $
 * Revision 1.2  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.1  2007/06/25 17:58:32  corey3
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

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  18-Jan-2005  IRC  Initial version.                          */
/*                                                              */
/*  17-Apr-2007  IRC  Adapted for Mili.                         */
/****************************************************************/

/*  Include Files  */
#include "silo.h"
#include "SiloLib_Internal.h"


int SILOLIB_Write_Mesh_Var_Vec (int    db_number,
                                char   *name,
                                double time,
                                int    cycle,
                                int    *num_zones, /* x,y,z */
                                int    data_type,
                                int    num_vars,
                                char   *var_names_all,
                                void   **var_data)

/****************************************************************/
/*  This function will write out a single mesh variable.        */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  20-Jan-2005  IRC  Initial version.                          */
/****************************************************************/
{

   int     i ;
   int     num_cells ;

   char   *mesh_name = "LB_ThreeD_Structured" ;
   int     db_type ;

   int     temp ;

   int     *var_int[4] ;
   float   *var_float[4] ;
   float    time_float ;
   double  *var_dbl[4] ;

   /* optlist variables */
   int        origin = 0 ;
   int        majororder = 0 ;
   int        coordsys;
   int        planar;
   DBoptlist *optlist;

   char *var_names[4] ;
   void *var_ptr ;


   time_float = (float) time ;

   /* Initialize the variable names */

   for (i=0;
         i<4;
         i++)
   {
      var_names[i] = (char *) malloc(20*sizeof(char)) ;
   }

   sscanf(var_names_all, "%s %s %s %s", var_names[0],
          var_names[1], var_names[2], var_names[3]) ;

   num_cells = num_zones[X]*num_zones[Y]*num_zones[Z] ;

   /* Switch Z/X Axis for Ladd Input */
   /* temp         = num_zones[Z] ;
   num_zones[Z]    = num_zones[X] ;
   num_zones[X]    = temp ; */

   /* Write QuadVar data: Zonal Data */
   if (data_type==TYPE_INT)
   {
      var_ptr = var_int;
      for (i=0;
            i<num_vars;
            i++)
         var_int[i] = (int *) var_data[i] ;
      db_type=DB_INT;
   }

   if (data_type==TYPE_FLOAT)
   {
      var_ptr = var_float;
      for (i=0;
            i<num_vars;
            i++)
         var_float[i] = (float *) var_data[i] ;
      db_type=DB_FLOAT;
   }

   if (data_type==TYPE_DOUBLE)
   {
      var_ptr = var_dbl;
      for (i=0;
            i<num_vars;
            i++)
         var_dbl[i] = (double *) var_data[i] ;
      db_type=DB_DOUBLE;
   }

   /* Make quadvar option list */
   optlist = DBMakeOptlist(16) ;

   DBAddOption(optlist, DBOPT_CYCLE, (void *)&cycle) ;
   DBAddOption(optlist, DBOPT_TIME, (void *)&time_float) ;
   DBAddOption(optlist, DBOPT_MAJORORDER, (void *)&majororder) ;
   DBAddOption(optlist, DBOPT_ORIGIN, (void *)&origin) ;

   coordsys = DB_CARTESIAN ;
   DBAddOption(optlist, DBOPT_COORDSYS, (void *)&coordsys) ;

   DBPutQuadvar(restart_file_pointer[db_number],
                name,
                mesh_name,
                num_vars,
                var_names,
                var_ptr,
                num_zones,
                3,
                NULL,
                0,
                db_type,
                DB_ZONECENT,
                optlist) ;

   DBFreeOptlist(optlist);

   for (i=0;
         i<4;
         i++)
   {
      free(var_names[i]) ;
   }

   return (1);
}

/* End SiloLib_Write_Mesh_Var_Vec.c */
