/* $Id: buildinfo.c,v 1.6 2011/07/26 14:09:23 durrenberger1 Exp $ */
/*
 * versioninfo.c - Dumps configure information to file or sysout.
 *
 *      Tom Treadway
 *      Lawrence Livermore National Laboratory
 *      November 18, 2004
 *
 ************************************************************************
 * Modifications:
 *
 *  T. Treadway - Nov  18, 2004: Created
 *  T. Treadway - Nov  24, 2004: Cleanup problems
 ************************************************************************
 */

/* buildinfo.c */


#include "xmilics_config.h"
#include "misc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>

#if HAVE_MILI
#include <mili.h>
#endif

#ifdef IRIX
extern FILE *popen(const char *, const char *);
#endif

static void BuildInfo(FILE *outfile)
{

   struct utsname u_info;
   time_t time_info;
   char *ctime_string;
   FILE *fp;
   char *module_lst;
   int recordcount;

   fprintf(outfile, "#define BI_VERSION \"%s-%s\"\n",
           PACKAGE_NAME, PACKAGE_VERSION);

   /* We don't want to reproduce the complete config.h,
      but those settings which may be related to problems at runtime */

#if HAVE_MILI
   fprintf(outfile, "#define BI_MILI \"yes - Version=%s\"\n", MILI_VERSION);
#else
   fprintf(outfile, "#define BI_MILI \"no\"\n");
#endif

   fprintf(outfile, "#define BI_CONFIG \"%s\"\n",
           CONFIG_CMD);
   fp = popen( "module 2> /dev/null", "r");
   if (fp == NULL) {
      fprintf(outfile, "#define BI_MODULES \"none\"\n");
   } else {
      fprintf(outfile, "#define BI_MODULES \"");
      recordcount = 0;
      module_lst = "";
      while ( !feof(fp) ) {
         recordcount++;
         fscanf(fp, "%s", module_lst);
         fprintf(outfile, " %s", module_lst);
      }
      if (recordcount <= 0) {
         fprintf(outfile, "none\"\n");
      } else {
         fprintf(outfile, "\"\n");
      }
   }
   pclose(fp);

   fprintf(outfile, "#define BI_MAKE \"%s\"\n",
           MAKE_CMD);

   fprintf(outfile, "#define BI_COMPILE \"%s\"\n",
           COMPILE_CMD);

   fprintf(outfile, "#define BI_LINK \"%s\"\n",
           LINK_CMD);

   uname(&u_info);
   fprintf(outfile, "#define BI_SYSTEM \"%s %s %s %s\"\n",
           u_info.sysname, u_info.version, u_info.release, u_info.machine);

   time_info = time(NULL);
   ctime_string = ctime(&time_info);
   if (ctime_string[strlen(ctime_string) - 1] == '\n') {
      ctime_string[strlen(ctime_string) - 1] = '\0';
   }
   fprintf(outfile, "#define BI_DATE \"%s\"\n", ctime_string);

   fprintf(outfile, "\n");

   return;
}


int main(int argc, char *argv[])
{
   FILE *outfile;

   if (argc == 1) {
      outfile = stdout;
   } else {
      if (!(outfile = fopen(argv[1], "w"))) {
         fprintf(stderr, "Failed to open %s for writing!\a\n", argv[1]);
         exit(1);
      }
   }

   BuildInfo(outfile);

   fclose(outfile);
   exit(0);
}
