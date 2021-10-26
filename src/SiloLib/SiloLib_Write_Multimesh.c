/*
 * $Log: SiloLib_Write_Multimesh.c,v $
 * Revision 1.4  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.3  2008/11/14 00:08:52  corey3
 * Continued development of MiliSilo
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
/*                   Copyright (c) 2007                         */
/*         The Regents of the University of California          */
/*                     All Rights Reserved                      */
/*                                                              */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  20-Apr-2007 IRC  Initial version.                           */
/*                                                              */
/****************************************************************/

/*  Include Files  */
#include <stdlib.h>
#include "silo.h"
#include "SiloLib_Internal.h"

int SiloLIB_Write_Multimesh(int    famid,
                            int    filetype,
                            char   *name,
                            int    nmesh,
                            char   **meshnames,
                            int    *meshtypes,
                            int    *zonecounts,
                            double *extents,
                            int    cycle,
                            double time
                           )

/****************************************************************/
/*  Routine SiloLib_Write_multimesh writes data form a multi-   */
/*  mesh to a Silo file.                                        */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  20-Apr-2007  IRC  Initial version.                          */
/****************************************************************/
{
   DBfile *db=NULL;
   int extents_size = 6;
   int *external_zones;
   int i;
   char *mrgname = "mrg_tree";
   DBoptlist *optlist;
   int status=OK;

   db = SiloLIB_getDBid(famid, filetype);

   external_zones = (int *) malloc(nmesh*sizeof(int));
   for (i=0;
         i<nmesh;
         i++)
      external_zones[i] = 0;

   /* Make the mesh option list */
   optlist = DBMakeOptlist(16) ;

   DBAddOption(optlist, DBOPT_CYCLE,        (void *)&cycle) ;
   DBAddOption(optlist, DBOPT_DTIME,        (void *)&time) ;
   DBAddOption(optlist, DBOPT_MRGTREE_NAME, mrgname);

   DBAddOption(optlist, DBOPT_HAS_EXTERNAL_ZONES, (void *)external_zones) ;

   if (db != NULL)
   {
      status = DBPutMultimesh(db,
                              name,
                              nmesh,
                              meshnames,
                              meshtypes,
                              optlist
                             );
   }

   return (status);
}

/* End SiloLIB_Write_Multimesh.c */
