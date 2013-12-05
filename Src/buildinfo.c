/* $Id$ */
/*
 * versioninfo.c - Dumps configure information to file or sysout.
 *
 *      Tom Treadway
 *      Lawrence Livermore National Laboratory
 *      November 18, 2004
 *
 *
 * This work was produced at the University of California, Lawrence
 * Livermore National Laboratory (UC LLNL) under contract no.
 * W-7405-ENG-48 (Contract 48) between the U.S. Department of Energy
 * (DOE) and The Regents of the University of California (University)
 * for the operation of UC LLNL. Copyright is reserved to the University
 * for purposes of controlled dissemination, commercialization through
 * formal licensing, or other disposition under terms of Contract 48;
 * DOE policies, regulations and orders; and U.S. statutes. The rights
 * of the Federal Government are reserved under Contract 48 subject to
 * the restrictions agreed upon by the DOE and University as allowed
 * under DOE Acquisition Letter 97-1.
 *
 *
 * DISCLAIMER
 *
 * This work was prepared as an account of work sponsored by an agency
 * of the United States Government. Neither the United States Government
 * nor the University of California nor any of their employees, makes
 * any warranty, express or implied, or assumes any liability or
 * responsibility for the accuracy, completeness, or usefulness of any
 * information, apparatus, product, or process disclosed, or represents
 * that its use would not infringe privately-owned rights.  Reference
 * herein to any specific commercial products, process, or service by
 * trade name, trademark, manufacturer or otherwise does not necessarily
 * constitute or imply its endorsement, recommendation, or favoring by
 * the United States Government or the University of California. The
 * views and opinions of authors expressed herein do not necessarily
 * state or reflect those of the United States Government or the
 * University of California, and shall not be used for advertising or
 * product endorsement purposes.
 *
 ************************************************************************
 * Modifications:
 *
 *  T. Treadway - Nov  18, 2004: Created
 *  T. Treadway - Nov  24, 2004: Cleanup problems
 *
 *  IRC - Oct  26, 2007: Added bi_user function. Output was giving the
 *  current login name not the user that built the code.
 *
 ************************************************************************
 */

/* buildinfo.c */


#include "griz_config.h"
#include "misc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include <Xm/Xm.h>

#if HAVE_MILI
#include <mili.h>
#endif
#if HAVE_PNG
#include <png.h>
#endif
#if HAVE_JPEG
#include <jpeglib.h>
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
    int  recordcount;
    char *developer;

    fprintf(outfile, "#define BI_VERSION \"%s-%s\"\n",
            PACKAGE_NAME, PACKAGE_VERSION);

    /* We don't want to reproduce the complete config.h,
       but those settings which may be related to problems at runtime */

    fprintf(outfile, "#define BI_MOTIF \"yes - Version=%s\"\n", XmVERSION_STRING);

#if HAVE_MILI
    fprintf(outfile, "#define BI_MILI \"yes - Version=%s\"\n", MILI_VERSION);
#else
    fprintf(outfile, "#define BI_MILI \"no\"\n");
#endif

#if HAVE_EXODUS
    fprintf(outfile, "#define BI_EXODUS \"yes\"\n");
#else
    fprintf(outfile, "#define BI_EXODUS \"no\"\n");
#endif

#if HAVE_NETCDF
    fprintf(outfile, "#define BI_NETCDF \"yes\"\n");
#else
    fprintf(outfile, "#define BI_NETCDF \"no\"\n");
#endif

#if HAVE_PNG
    fprintf(outfile, "#define BI_PNG \"yes - Version=%s\"\n",
            PNG_LIBPNG_VER_STRING);
#else
    fprintf(outfile, "#define BI_PNG \"no\"\n");
#endif

#if HAVE_JPEG
    fprintf(outfile, "#define BI_JPEG \"yes - Version=%i\"\n",
            JPEG_LIB_VERSION);
#else
    fprintf(outfile, "#define BI_JPEG \"no\"\n");
#endif

#if HAVE_MESA
    fprintf(outfile, "#define BI_MESA \"yes\"\n");
#else
    fprintf(outfile, "#define BI_MESA \"no\"\n");
#endif

#if HAVE_OSMESA
    fprintf(outfile, "#define BI_OSMESA \"Yes - Version=%d.%d\"\n",
            0,  0);
#else
    fprintf(outfile, "#define BI_OSMESA \"no\"\n");
#endif

#if HAVE_BATCH
    fprintf(outfile, "#define BI_BATCH \"yes\"\n");
#else
    fprintf(outfile, "#define BI_BATCH \"no\"\n");
#endif

    fprintf(outfile, "#define BI_CONFIG \"%s\"\n",
            CONFIG_CMD);
    fp = popen( "module 2> /dev/null", "r");
    if (fp == NULL)
    {
        fprintf(outfile, "#define BI_MODULES \"none\"\n");
    }
    else
    {
        fprintf(outfile, "#define BI_MODULES \"");
        recordcount = 0;
        module_lst = "";
        while ( !feof(fp) )
        {
            recordcount++;
            fscanf(fp, "%s", module_lst);
            fprintf(outfile, " %s", module_lst);
        }
        if (recordcount <= 0)
            fprintf(outfile, "none\"\n");
        else
            fprintf(outfile, "\"\n");
    }
    pclose(fp);

    fprintf(outfile, "#define BI_MAKE \"%s\"\n",
            MAKE_CMD);

    fprintf(outfile, "#define BI_COMPILE \"%s\"\n",
            COMPILE_CMD);

    fprintf(outfile, "#define BI_LINK \"%s\"\n",
            LINK_CMD);

    developer =	getlogin();
    fprintf(outfile, "#define BI_DEVELOPER \"%s\"\n", developer );

    uname(&u_info);
    fprintf(outfile, "#define BI_SYSTEM \"%s %s %s %s\"\n",
            u_info.sysname, u_info.version, u_info.release, u_info.machine);

    time_info = time(NULL);
    ctime_string = ctime(&time_info);
    if (ctime_string[strlen(ctime_string) - 1] == '\n')
    {
        ctime_string[strlen(ctime_string) - 1] = '\0';
    }
    fprintf(outfile, "#define BI_DATE \"%s\"\n", ctime_string);

    fprintf(outfile, "\n");

    return;
}


int main(int argc, char *argv[])
{
    FILE *outfile;

    if (argc == 1)
    {
        outfile = stdout;
    }
    else
    {
        if (!(outfile = fopen(argv[1], "w")))
        {
            fprintf(stderr, "Failed to open %s for writing!\a\n", argv[1]);
            exit(1);
        }
    }

    BuildInfo(outfile);

    if (outfile != stdout)
    {
        fclose(outfile);
    }
    exit(0);
}
