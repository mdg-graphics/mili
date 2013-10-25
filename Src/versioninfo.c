/* $Id$ */
/*
 * versioninfo.c - Dumps version info to sysout.
 *
 *      Tom Treadway
 *      Lawrence Livermore National Laboratory
 *      November 18, 2004
 *
 *
 ************************************************************************
 * Modifications:
 *
 *  T. Treadway - Nov  18, 2004: Created
 *  T. Treadway - Nov  24, 2004: Reduced #ifdef processing
 *
 *  IRC - Oct  26, 2007: Added bi_user function. Output was giving the
 *  current login name not the user that built the code.
 ************************************************************************
 */

#include <griz_config.h>
#include "buildinfo.h"
#include "misc.h"
#include "viewer.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *get_VersionInfo( Analysis * );

/*
 * Build info stuff
 */
char *bi_developer(void)
{
    return BI_DEVELOPER;
}

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

char *bi_motif(void)
{
    return BI_MOTIF;
}

char *bi_mili(void)
{
    return BI_MILI;
}

char *bi_exodus(void)
{
    return BI_EXODUS;
}

char *bi_netcdf(void)
{
    return BI_NETCDF;
}

char *bi_png(void)
{
    return BI_PNG;
}

char *bi_jpeg(void)
{
    return BI_JPEG;
}

char *bi_mesa(void)
{
    return BI_MESA;
}

char *bi_osmesa(void)
{
    return BI_OSMESA;
}

char *bi_batch(void)
{
    return BI_BATCH;
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

void VersionInfo(Bool_type quiet_mode)
{
    fprintf(stdout, "\n%s\n\n", bi_version_string());
    fprintf(stdout, "\nGriz Version %d.%d.%d\n\n", GRIZ_MAJOR, GRIZ_MINOR, GRIZ_BUG);

    if ( quiet_mode )
    {
        fprintf(stdout, "Built: \t%s on %s by %s\n\n",
                bi_date(), bi_system(), bi_developer());
        fprintf(stdout, "Configure: %s\n\n", bi_config());
        return;
    }

    /* We don't want to reproduce the complete config.h,
     * but those settings which may be related to problems
     * on runtime
     */

    fprintf(stdout, "Motif toolkit: %s\n", bi_motif());
    fprintf(stdout, "Mili support: %s\n", bi_mili());
    fprintf(stdout, "Exodus support: %s\n", bi_exodus());
    fprintf(stdout, "NetCDF support: %s\n", bi_netcdf());
    fprintf(stdout, "PNG support: %s\n", bi_png());
    fprintf(stdout, "JPEG support: %s\n", bi_jpeg());
    fprintf(stdout, "Mesa support: %s\n", bi_mesa());
    fprintf(stdout, "OSMesa support: %s\n", bi_osmesa());
    fprintf(stdout, "Batch support: %s\n\n", bi_batch());

    fprintf(stdout, "Built: \t%s on %s by %s\n\n",
            bi_date(), bi_system(), bi_developer());
    fprintf(stdout, "Configure: %s\n\n", bi_config());
    fprintf(stdout, "Modules: %s\n\n", bi_modules());
    fprintf(stdout, "Make Options: gmake %s\n\n", bi_make());
    fprintf(stdout, "Compile Options: %s\n\n", bi_compile());
    fprintf(stdout, "Link Options: %s\n", bi_link());

    fprintf(stdout, "\n");

    fprintf(stdout, "(C) Copyright 1992, 2004, 2009, 2010, 2011, 2012\n");
    fprintf(stdout, "    Lawrence Livermore National Laboratory\n");

    return;
}


char *get_VersionInfo( Analysis *analy )
{
    char *version_string, temp_string[500], mili_version[100];

    version_string = NEW_N( char, 2000, "Version Info String" );

    sprintf(temp_string, "#\n#\t%s\n# ", bi_version_string());
    strcpy( version_string, temp_string );

    sprintf(temp_string, "\tBuilt: \t%s on %s by %s\n# ",
            bi_date(), bi_system(), bi_developer());
    strcat( version_string, temp_string );

    sprintf(temp_string, "\tConfigure: %s\n# ", bi_config());
    strcat( version_string, temp_string );

    sprintf(temp_string, "\tCompile Options: %s\n# ", bi_compile());
    strcat( version_string, temp_string );

    get_mili_version( mili_version );
    sprintf(temp_string, "\t\tMili Library Version (Griz Linked): \t%s\n#", mili_version);
    strcat( version_string, temp_string );

    sprintf(temp_string, "\t\tMili Database-Library Version: \t\t%s\n#", analy->mili_version);
    strcat( version_string, temp_string );

    sprintf(temp_string, "\t\tMili Database-Host: \t\t\t%s\n#", analy->mili_host);
    strcat( version_string, temp_string );

    sprintf(temp_string, "\t\tMili Database-Arch: \t\t\t%s\n#", analy->mili_arch);
    strcat( version_string, temp_string );

    sprintf(temp_string, "\t\tMili Database-Timestamp: \t\t%s\n", analy->mili_timestamp);
    strcat( version_string, temp_string );

    if ( strlen(analy->xmilics_version)>0 )
    {
        sprintf(temp_string, "#\t\tXmilics Version: \t\t\t%s\n", analy->xmilics_version);
        strcat( version_string, temp_string );
    }

    return( version_string );
}
