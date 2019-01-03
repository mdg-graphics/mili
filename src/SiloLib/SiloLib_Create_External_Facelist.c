/*
 * $Log: SiloLib_Create_External_Facelist.c,v $
 * Revision 1.2  2011/05/26 21:43:13  durrenberger1
 * fixed a few errant return values which were still declared as int and standardized the indentation of the code to 3 spaces
 *
 * Revision 1.1  2007/06/25 17:58:31  corey3
 * New Mili/Silo interface
 *
 *
*/


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*                                                              */
/*                   Copyright (c) 2005                         */
/*         The Regents of the University of California          */
/*                     All Rights Reserved                      */
/*                                                              */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  18-Jan-2005  IRC  Initial version.                          */
/*                                                              */
/*  30-May-2007  IRC  Initial version                           */
/****************************************************************/

/*  Include Files  */

#include "silo.h"
#include "string.h"
#include "SiloLib_Internal.h"

DBfacelist *SiloLib_Create_External_Facelist(int *nodelist,
      int  nnodes,
      int *shapesize,
      int *shapecnt,
      int nshapes,
      int *matlist)

/****************************************************************/
/*  This function will create an external facelist from a list  */
/*  of nodes..                                                  */
/*                                                              */
/****************************************************************/
/****************************************************************/
/*  Revision History                                            */
/*  ----------------                                            */
/*                                                              */
/*  04-Jun-2007  IRC  Initial version.                          */
/****************************************************************/
{
   DBfacelist *fl;

   int bnd_method = 1;

   if ( matlist==NULL )
      bnd_method = 0;

   /* Create a facelist and return a pointer to it */

   fl = DBCalcExternalFacelist(nodelist,
                               nnodes,
                               0,
                               shapesize,
                               shapecnt,
                               nshapes,
                               matlist,
                               bnd_method);

   return(fl);
}

/* End SiloLib_Create_External_Facelist.c */
