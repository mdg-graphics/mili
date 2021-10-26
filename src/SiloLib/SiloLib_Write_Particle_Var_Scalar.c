/*
 * $Log: SiloLib_Write_Particle_Var_Scalar.c,v $
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
#include "silo.h"
#include "SiloLib_Internal.h"

int SiloLIB_Write_Particle_Var_Scalar (DBfile *db,
                                       char   *mesh_name,
                                       char   *var_name,
                                       double time,
                                       double time_step,
                                       int    cycle,
                                       int    num_particles,
                                       int    data_type,
                                       void   *var)

/****************************************************************/
/*  This function will write the particle result to a point     */
/*  mesh object.                                                */
/****************************************************************/

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  20-Jan-2005  IRC  Initial version.                          */
/*                                                              */
/*  17-Apr-2007  IRC  Adapted for Mili                          */
/****************************************************************/

{
   int     i ;
   int     group=1 ;
   int     num_cells ;
   int     ndims = 3 ;
   int     db_type ;

   void   *var_ptr ;
   float   time_float  ;
   double  time_step_dbl ;
   int     origin ;
   int     status=OK;

   DBoptlist *optlist;

   time_float = (float)  time ;

   /* Write QuadVar data: Zonal Data */
   if (data_type==TYPE_INT)
   {
      var_ptr = (int *) var ;
      db_type=DB_INT;
   }

   if (data_type==TYPE_FLOAT)
   {
      var_ptr = (float *) var ;
      db_type=DB_FLOAT;
   }

   if (data_type==TYPE_DOUBLE)
   {
      var_ptr = (double *) var ;
      db_type=DB_DOUBLE;
   }

   /* Make pointvar option list */

   optlist = DBMakeOptlist(5) ;
   DBAddOption(optlist, DBOPT_NSPACE,  (void *)&ndims) ;
   DBAddOption(optlist, DBOPT_CYCLE,   (void *)&cycle) ;
   DBAddOption(optlist, DBOPT_TIME,    (void *)&time_float) ;
   DBAddOption(optlist, DBOPT_DTIME,   (void *)&time_step) ;
   DBAddOption(optlist, DBOPT_ORIGIN,  (void *)&origin);

   /* Write the Point Mesh */
   status = DBPutPointvar1(db,
                           var_name,
                           mesh_name,
                           var_ptr,
                           num_particles,
                           db_type,
                           optlist) ;

   DBFreeOptlist(optlist);

   return(status) ;

}

/* End SiloLib_Write_Particle_Var_Scalar.c */
