/* $Id: versioninfo.c,v 1.7 2021/09/13 17:16:04 jdurren Exp $ */
/*
 * versioninfo.c - Dumps version info to sysout.
 *
 *      Tom Treadway
 *      Lawrence Livermore National Laboratory
 *      November 18, 2004
 *
 ************************************************************************
 * Modifications:
 *
 *  T. Treadway - Nov  18, 2004: Created
 *  T. Treadway - Nov  24, 2004: Reduced #ifdef processing
 *
 *  I. R. Corey - Dec  28, 2006: Modified for use with Xmilics
 *
 ************************************************************************
 */

#include "buildinfo.h"
#include <stdio.h>
#include <unistd.h>

void VersionInfo(void);

/*
 * Build info stuff
 */
char *bi_version_string(void)
{
   return BI_VERSION;
}

char *bi_system(void)
{
   return BI_SYSTEM;
}

char *bi_date(void)
{
   return BI_DATE;
}

char *bi_mili(void)
{
   return BI_MILI;
}

char *bi_config(void)
{
   return BI_CONFIG;
}

char *bi_modules(void)
{
   return BI_MODULES;
}

char *bi_make(void)
{
   return BI_MAKE;
}

char *bi_compile(void)
{
   return BI_COMPILE;
}

char *bi_link(void)
{
   return BI_LINK;
}

void VersionInfo(void)
{
   fprintf(stdout, "\n%s\n\n", bi_version_string());

   /* We don't want to reproduce the complete config.h,
      but those settings which may be related to problems on runtime */

   fprintf(stdout, "Built: \t%s on %s by %s\n\n",
           bi_date(), bi_system(), getlogin());
   fprintf(stdout, "Configure: %s\n\n", bi_config());
   fprintf(stdout, "Modules: %s\n\n", bi_modules());
   fprintf(stdout, "Make Options: gmake %s\n\n", bi_make());
   fprintf(stdout, "Compile Options: %s\n\n", bi_compile());
   fprintf(stdout, "Link Options: %s\n", bi_link());

   fprintf(stdout, "\n");

   fprintf(stdout, "(C) Copyright 1992, 2004, 2006, 2009 \n");
   fprintf(stdout, "Lawrence Livermore National Laboratory\n");

   return;
}
