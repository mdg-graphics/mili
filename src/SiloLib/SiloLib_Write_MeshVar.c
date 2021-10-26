/*
 * $Log: SiloLib_Write_MeshVar.c,v $
 * Revision 1.5  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.4  2008/11/14 01:03:45  corey3
 * Continued development of MiliSilo
 *
 * Revision 1.3  2008/09/24 18:36:02  corey3
 * New update of Passit source files
 *
 * Revision 1.2  2007/07/05 22:03:50  corey3
 * Adding more functionality to Silo Library Interface
 *
 * Revision 1.1  2007/06/25 17:58:31  corey3
 * New Mili/Silo interface
 *
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
/*  This function will write out a single non-mesh variable.     */
/****************************************************************/

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  07-Jun-2007  IRC  Initial version.                          */
/*                                                              */
/****************************************************************/


/*  Include Files  */
#include "silo.h"
#include "SiloLib_Internal.h"

int SiloLib_Write_MeshVar (int    famid,
                           int    filetype,
                           char   *meshname,
                           char   *varname,
                           double time,
                           int    cycle,
                           int    dim,
                           int    nels,
                           int    centering,
                           int    data_type,
                           void   *var_data)
{
   DBfile *db=NULL ;
   int  i ;

   /* optlist variables */
   int        origin = 0 ;
   int        majororder = 0 ;

   DBoptlist *optlist;
   int  status=OK ;

   db = SiloLIB_getDBid(famid, filetype);

   /* Make UCD var option list */
   optlist = DBMakeOptlist(16) ;
   DBAddOption(optlist, DBOPT_CYCLE, (void *)&cycle) ;
   DBAddOption(optlist, DBOPT_TIME, (void *)&time) ;
   DBAddOption(optlist, DBOPT_MAJORORDER, (void *)&majororder) ;
   DBAddOption(optlist, DBOPT_ORIGIN, (void *)&origin) ;

   if (dim==1)
      /* Scalar */
      status = DBPutUcdvar1(db,
                            varname,
                            meshname,
                            (float *) var_data,
                            nels,
                            0,
                            0,
                            data_type,
                            centering,
                            optlist) ;
   else
   {
      /* Vector */
      status = DBPutUcdvar1(db,
                            varname,
                            meshname,
                            (float *) var_data,
                            nels,
                            0,
                            0,
                            data_type,
                            centering,
                            optlist) ;
   }

   DBFreeOptlist(optlist);

   return (status);

}

/* End SiloLib_Write_MeshVar.c */

