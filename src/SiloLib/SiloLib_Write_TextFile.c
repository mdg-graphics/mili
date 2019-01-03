/*
 * $Log: SiloLib_Write_TextFile.c,v $
 * Revision 1.6  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.5  2008/11/14 00:08:53  corey3
 * Continued development of MiliSilo
 *
 * Revision 1.4  2008/09/24 18:36:02  corey3
 * New update of Passit source files
 *
 * Revision 1.3  2007/07/23 19:21:00  corey3
 * Adding more capabilities to the SiloLibrary layer
 *
 * Revision 1.2  2007/07/10 20:57:57  corey3
 * More additions to the Silo IO Library
 *
 * Revision 1.1  2007/07/05 22:06:21  corey3
 * Adding more functionality to Silo Library Interface
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
#include <stdio.h>
#include <string.h>
#include "silo.h"
#include "SiloLib_Internal.h"


int SiloLIB_Write_TextFile(int  famid,
                           int  filetype,
                           char *filename,
                           double time, int cycle,
                           char *varname)

/****************************************************************/
/*  Routine SiloLib_Write_TextFile - This function will write a */
/*  complete file to a set of Silo char arrays.                 */
/****************************************************************/

{
   DBfile *db=NULL;
   int  len, linecnt=1;
   FILE *fp;
   char temp_varname[64];
   char linein[512], lineno[8];
   int  status=OK;

   db = SiloLIB_getDBid(famid, filetype);

   fp = fopen(filename, "r");
   if (fp==NULL)
      return(FILE_NOT_FOUND);

   status = SiloLIB_mkdir(db, varname) ;
   status = SiloLIB_cddir(db, varname) ;

   while (fgets(linein, 256, fp)!=NULL)
   {
      strcpy(temp_varname, varname);
      strcat(temp_varname, "_LINE_");
      sprintf(lineno, "%05d", linecnt);
      strcat(temp_varname, lineno);

      len = strlen(linein)+1; /* Make sure to include string terminator */

      if (db != NULL)
      {
         status = Silo_Write_Var (db, temp_varname, time, cycle, 1, len , DB_CHAR, linein);
      }
      linecnt++;
   }

   linecnt--;
   strcpy(temp_varname, varname);
   strcat(temp_varname, "_LINECNT");
   status = Silo_Write_Var (db, temp_varname, time, cycle, 1, 1 , DB_INT, &linecnt);

   status = SiloLIB_cddir(db, "..") ;

   return (status);
}


/* End SiloLib_Write_TextFile.c */
