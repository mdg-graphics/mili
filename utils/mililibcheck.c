/* $Id: mililibcheck.c,v 1.20 2021/09/13 17:16:04 jdurren Exp $ */
/*
 * mililibcheck.c - Dumps mili version information to file or sysout.
 *
 *      Ivan R. Corey
 *      Lawrence Livermore National Laboratory
 *      November 30, 2004
 *
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - Nov  30, 2004: Created
 ************************************************************************
 */

/* mililibcheck.c */


#include "mili_config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#ifndef XMILICS_VERSION
#define XMILICS_VERSION "21_01(12-07-2021)"
#endif

#ifndef MILI_VERSION
#define MILI_VERSION "V20_01_01"
#endif

void get_mili_version( void *p_info );

int main(int argc, char *argv[])
{
   char  mili_version[64], xmilics_mili_version[16];
   double mili_version_float, xmilics_mili_version_float;
   int len,i;
   get_mili_version(mili_version);
   len = strlen(mili_version);
   
   printf("\nMili Version = %s", mili_version);
   for(i=0;i<len;i++)
   {
      if(!isdigit(mili_version[i]))
      {
         mili_version[i]=' ';
      }else
      {
         break;
      }
   }
   mili_version_float = atof(mili_version);

   strcpy(xmilics_mili_version, MILI_VERSION);
   xmilics_mili_version_float = atof(xmilics_mili_version);

   if (mili_version_float >= xmilics_mili_version_float) {
      printf("\nMILI_LIB_OK -- Version=%4.2f\n", (float) mili_version_float);
   } else {
      printf("\nMILI_LIB_OUTOFDATE -- Version found=%f / Expected=%f,",
             (float) mili_version_float, (float) xmilics_mili_version_float);
   }
   exit(0);
}
