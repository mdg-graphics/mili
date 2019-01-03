/*
 * $Log: SiloLib_Write_Var.c,v $
 * Revision 1.7  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.6  2008/11/14 01:03:45  corey3
 * Continued development of MiliSilo
 *
 * Revision 1.5  2008/09/24 18:36:02  corey3
 * New update of Passit source files
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
 * Revision 1.1  2007/06/25 17:58:33  corey3
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
/*  This function will write out a single non-mesh variable of  */
/*  any length to a Silo file.                                  */
/****************************************************************/

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  07-Jun-2007  IRC  Initial version.                          */
/*                                                              */
/****************************************************************/


/*  Include Files  */

#include <string.h>
#include "silo.h"
#include "SiloLib_Internal.h"

int SiloLib_Write_Var (int famid,
                       int filetype,
                       char   *name,
                       double time,
                       int    cycle,
                       int    dims,
                       int    len,
                       int    data_type,
                       void   *var_data)
{
   DBfile *db=NULL;
   int  tempdims[3];
   int  i ;
   int status=OK;

   char dimsname[32];

   /* optlist variables */

   DBoptlist *optlist;

   db = SiloLIB_getDBid(famid, filetype);

   /* Make variable option list */
   optlist = DBMakeOptlist(16) ;
   DBAddOption(optlist, DBOPT_CYCLE, (void *)&cycle) ;
   DBAddOption(optlist, DBOPT_TIME, (void *)&time) ;

   tempdims[0] = len;
   tempdims[1] = 0;
   tempdims[2] = 0;

   status = DBWrite (db,
                     name,
                     var_data,
                     tempdims,
                     1,
                     data_type);


   /* Create a dims variable */
   if (data_type != DB_CHAR)
   {
      strcpy(dimsname, name);
      strcat(dimsname, "_DIMS");
      tempdims[0] = 1;
      status = DBWrite (db,
                        dimsname,
                        tempdims,
                        tempdims,
                        1,
                        DB_INT);
   }

   DBFreeOptlist(optlist);

   return (status);

}

/* End SiloLib_Write_Var.c */

