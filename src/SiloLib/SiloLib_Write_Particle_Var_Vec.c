/*
 * $Log: SiloLib_Write_Particle_Var_Vec.c,v $
 * Revision 1.5  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.4  2008/11/14 00:08:52  corey3
 * Continued development of MiliSilo
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

int SiloLib_Write_Particle_Var_Vec(int    famid,
                                   int    filetype,
                                   char   *mesh_name,
                                   char   *var_name,
                                   double time,
                                   double time_step,
                                   int    cycle,
                                   int    num_particles,
                                   int    data_type,
                                   int    num_vars,
                                   void   **var)

/****************************************************************/
/*  This function will write the particle field to a point mesh */
/*  object.                                                     */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  20-Jan-2005  IRC  Initial version.                          */
/****************************************************************/
{
   DBfile  *db=NULL;
   int     i ;
   int     ndims = 3 ;
   int     db_type ;

   void   *var_ptr[4];
   float   time_float ;
   int     origin = 0 ;
   int     status=OK;

   DBoptlist *optlist;

   /* Write PointVar data */

   time_float = (float) time ;

   db = SiloLIB_getDBid(famid, filetype);

   if (data_type==TYPE_INT)
   {
      for (i=0;
            i<4;
            i++)
         var_ptr[i] = (int *) var[i] ;
      db_type=DB_INT;
   }

   if (data_type==TYPE_FLOAT)
   {
      for (i=0;
            i<4;
            i++)
         var_ptr[i] = (float *) var[i] ;
      db_type=DB_FLOAT;
   }

   if (data_type==TYPE_DOUBLE)
   {
      for (i=0;
            i<4;
            i++)
         var_ptr[i] = (double *) var[i] ;
      db_type=DB_DOUBLE;
   }

   /* Make quadvar option list */

   optlist = DBMakeOptlist(5) ;
   DBAddOption(optlist, DBOPT_NSPACE,  (void *)&ndims) ;
   DBAddOption(optlist, DBOPT_CYCLE,   (void *)&cycle) ;
   DBAddOption(optlist, DBOPT_TIME,    (void *)&time_float) ;
   DBAddOption(optlist, DBOPT_DTIME,   (void *)&time_step) ;
   DBAddOption(optlist, DBOPT_ORIGIN,  (void *)&origin);

   /* Write the Point Mesh */
   status = DBPutPointvar(db,
                          var_name,
                          mesh_name,
                          num_vars,
                          (void *) var_ptr,
                          num_particles,
                          db_type,
                          optlist) ;

   DBFreeOptlist(optlist);

   return(status) ;
}

/* End SiloLib_Write_Particle_Var_Vec.c */
