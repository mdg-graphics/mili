/*
 * $Log: SiloLib_Read_TextFile.c,v $
 * Revision 1.6  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.5  2008/11/14 00:08:52  corey3
 * Continued development of MiliSilo
 *
 * Revision 1.4  2008/09/24 18:36:01  corey3
 * New update of Passit source files
 *
 * Revision 1.3  2007/07/23 19:20:59  corey3
 * Adding more capabilities to the SiloLibrary layer
 *
 * Revision 1.2  2007/07/10 20:57:57  corey3
 * More additions to the Silo IO Library
 *
 * Revision 1.1  2007/07/05 22:05:15  corey3
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
#include <string.h>
#include "silo.h"
#include "SiloLib_Internal.h"


int SiloLIB_Read_TextFile(int  famid,
                          int  filetype,
                          char *filename,
                          char *varname)

/****************************************************************/
/*  Routine SiloLib_Write_TextFile - This function will read a  */
/*  set of Silo char arrays and write to an ASCII file.         */
/****************************************************************/

{
   DBfile  *db=NULL;
   int i;
   int  len, linecnt=0;
   FILE *fp;
   char temp_varname[64];
   char linein[512], lineno[8];
   int  status=OK;

   db = SiloLIB_getDBid(famid, filetype);

   fp = fopen(filename, "w");
   if (fp==NULL)
      return(FILE_NOT_FOUND);

   status = SiloLIB_cddir(db, varname) ;

   /* Read the number of lines in the Silo file */
   strcpy(temp_varname, varname);
   strcat(temp_varname, "_LINECNT");
   status = Silo_Read_Var (db, temp_varname, &linecnt);

   /* Write out the text lines from the Silo file to a text file */
   for (i=0;
         i<linecnt;
         i++)
   {
      strcpy(temp_varname, varname);
      strcat(temp_varname, "_LINE_");
      sprintf(lineno, "%05d", i+1);
      strcat(temp_varname, lineno);

      status = Silo_Read_Var(db, temp_varname, linein);
      fputs(linein, fp);
   }

   status = SiloLIB_cddir(db, "..") ;

   return (status);
}

/* End SiloLib_Read_TextFile.c */
