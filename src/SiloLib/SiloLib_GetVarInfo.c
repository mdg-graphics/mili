/*
 * $Log: SiloLib_GetVarInfo.c,v $
 * Revision 1.5  2011/05/26 21:43:13  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.4  2008/11/14 00:08:52  corey3
 * Continued development of MiliSilo
 *
 * Revision 1.3  2008/09/24 18:36:01  corey3
 * New update of Passit source files
 *
 * Revision 1.2  2007/07/10 20:57:57  corey3
 * More additions to the Silo IO Library
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
/*  29-June-2007 IRC  Initial version.                          */
/*                                                              */
/****************************************************************/

/*  Include Files  */
#include <string.h>
#include "silo.h"
#include "SiloLib_Internal.h"

int SiloLIB_GetVarInfo(int  famid,    int filetype,
                       char *varname, int *dims,
                       int  *len,     int *type)
/****************************************************************/
/*  Routine SiloLib_GetVarInfo - This function will return the  */
/*  attributes for a Silo Variable.                             */
/****************************************************************/

{
   DBfile *db=NULL;
   char dimsname[32];
   int  status=OK;

   strcpy(dimsname, varname);
   strcat(dimsname, "_DIMS");

   if (famid != NULL)
   {
      db      = SiloLIB_getDBid(famid, filetype);
      dims[0] = 0;
      *len     = DBGetVarLength(db, varname);
      *type    = DBGetVarType(db, varname);
      status   = DBReadVar(db, dimsname, dims);
   }

   return (status);
}


/* End SiloLib_GetVarInfo.c */

