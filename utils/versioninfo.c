#include "mili_config.h"
#include <stdio.h>
#include <unistd.h>

void VersionInfo(void)
{
    fprintf(stdout, "\n%s-%s\n\n", PACKAGE_NAME, PACKAGE_VERSION);
    fprintf(stdout, "Commit hash: %s\n", MILI_GIT_COMMIT_SHA1);
    fprintf(stdout, "Built: \t%s on %s by %s\n\n", BUILD_TIMESTAMP, SYSTEM_INFO_STRING, BUILD_DEVELOPER);
    fprintf(stdout, "Configure: %s\n\n", CONFIG_CMD);
    fprintf(stdout, "Make Options: %s\n\n", MAKE_CMD);
    fprintf(stdout, "Compile Options: %s\n\n", COMPILE_CMD);
    fprintf(stdout, "Link Options: %s\n", LINK_CMD);

    fprintf(stdout, "\n");

    fprintf(stdout, "(C) Copyright 1992, 2004, 2006, 2009 \n");
    fprintf(stdout, "Lawrence Livermore National Laboratory\n");

    return;
}
