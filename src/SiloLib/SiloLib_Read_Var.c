/*
 * $Log: SiloLib_Read_Var.c,v $
 * Revision 1.5  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.4  2008/11/14 00:08:52  corey3
 * Continued development of MiliSilo
 *
 * Revision 1.3  2008/09/24 18:36:02  corey3
 * New update of Passit source files
 *
 * Revision 1.2  2007/07/10 20:57:57  corey3
 * More additions to the Silo IO Library
 *
 * Revision 1.1  2007/07/05 22:06:21  corey3
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
/*  This function will read a single non-mesh variable of any   */
/*  length to a Silo file.                                      */
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

int Silo_Read_Var (int    famid,
                   int    filetype,
                   char   *name,
                   void   *var_data)
{
   int  i ;
   int status;
   DBfile *db;

   if (famid != NULL)
   {
      db = SiloLIB_getDBid(famid, filetype);
      status = DBReadVar (db,
                          name,
                          var_data);
   }
   return (status);

}

/* End SiloLib_Read_Var.c */

