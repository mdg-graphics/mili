/*
 * $Log: SiloLib_Write_Material.c,v $
 * Revision 1.4  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.3  2008/11/14 00:08:52  corey3
 * Continued development of MiliSilo
 *
 * Revision 1.2  2008/09/24 18:36:02  corey3
 * New update of Passit source files
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

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  17-Apr-2007  IRC  Adapted for Mili                          */
/****************************************************************/

/*  Include Files  */
#include "silo.h"
#include "SiloLib_Internal.h"

int SiloLib_Write_Material(int    famid,
                           int    filetype,
                           char   *mesh_name,
                           char   *mat_name,
                           int    *num_zones,
                           int    num_mats,
                           int    *mat_nums,
                           int    *mat_list,
                           double time,
                           double time_step,
                           int    cycle,
                           int    datatype)

/****************************************************************/
/*  This function will write the material data for clean zones  */
/*  to a Silo file.                                             */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  17-Apr-2007  IRC  Initial version.                          */
/****************************************************************/
{
   DBfile *db=NULL;
   int  i ;
   int  status=OK ;

   /* Mixed materials */

   int dims[3];

   /* optlist variables */
   int        origin = 0 ;
   int        majororder = 0 ;
   int        hi[3]  = {0,0,0} ;
   int        low[3] = {0,0,0} ;
   int        coordsys;
   int        planar;
   DBoptlist *optlist;

   db = SiloLIB_getDBid(famid, filetype);

   /* Define nodal dimensions of mesh */
   dims[X] = num_zones[0];
   dims[Y] = 0;
   dims[Z] = 0;

   /* Make quadvar option list */
   optlist = DBMakeOptlist(20) ;
   DBAddOption(optlist, DBOPT_CYCLE, (void *)&cycle) ;
   DBAddOption(optlist, DBOPT_TIME, (void *)&time) ;
   DBAddOption(optlist, DBOPT_DTIME, (void *)&time_step) ;
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

   status = DBPutMaterial(db,
                          mat_name,
                          mesh_name,
                          num_mats,
                          mat_nums,
                          mat_list,
                          num_zones,
                          1,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          0,
                          datatype,
                          optlist) ;

   DBFreeOptlist(optlist);
   return(status) ;
}

/* End SiloLib_Write_Material.c */
