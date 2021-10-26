/*
 * $Log: SiloLib_Close_Restart_File.c,v $
 * Revision 1.4  2011/05/26 21:43:13  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.3  2008/11/14 00:08:52  corey3
 * Continued development of MiliSilo
 *
 * Revision 1.2  2008/09/24 18:36:01  corey3
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
/*  20-Jan-2005  IRC  Initial version.                          */
/*                                                              */
/*  17-Apr-2007  IRC  Adapted for Mili                          */
/****************************************************************/

/*  Include Files  */
#include "silo.h"
#include "SiloLib_Internal.h"


int SiloLib_Close_Restart_File(int famid, int filetype)
/****************************************************************/
/*  Routine SiloLib_Open_Restart_File closes a Silo restart.    */
/*  file.                                                       */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  20-Jan-2005  IRC  Initial version.                          */
/****************************************************************/
{
   DBfile *db=NULL;

   if (famid != NULL)
   {
      db = SiloLIB_getDBid(famid, filetype);
      DBClose(db);
   }

   silo_family_qty--;

   return (OK);
}

/* End SiloLib_Close_Restart_File.c */
