/*
 * $Log: SiloLib_errors.c,v $
 * Revision 1.4  2011/05/26 21:43:14  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.3  2007/08/31 22:38:25  corey3
 * Added new functionality to Mili-Silo
 *
 * Revision 1.2  2007/08/24 19:54:07  corey3
 * Added more functions for support writing Silo files
 *
 * Revision 1.1  2007/08/01 01:17:02  corey3
 * New Silo error level setting function
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
/*  31-Jul-2007  IRC  Initial version.                          */
/*                                                              */
/****************************************************************/

/*  Include Files  */
#include "silo.h"
#include "SiloLib_Internal.h"


int SiloLIB_Enable_Errors( void )
/****************************************************************/
/*  Routine SiloLib_Enable_Errors - This function will turn on  */
/*  full Silo Error Checking.                                   */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  31-Jul-2007  IRC  Initial version.                          */
/****************************************************************/
{

   DBShowErrors( DB_ALL, NULL );

   return (0);
}


int SiloLIB_Disable_Errors( void )
/****************************************************************/
/*  Routine SiloLib_Enable_Errors - This function will turn off */
/*  ALL Silo Error Checking.                                    */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  31-Jul-2007  IRC  Initial version.                          */
/****************************************************************/
{

   DBShowErrors( DB_NONE, NULL );

   return (0);
}

/* End SiloLib_errors.c */
