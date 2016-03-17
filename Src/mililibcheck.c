/* $Id$ */
/*
 * mililibcheck.c - Dumps mili version information to file or sysout.
 *
 *      Ivan R. Corey
 *      Lawrence Livermore National Laboratory
 *      November 30, 2004
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
 *  I. R. Corey - Nov  30, 2004: Created
 ************************************************************************
 */

/* mililibcheck.c */


#include "griz_config.h" 
#include "misc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "io_wrap.h"
int main(int argc, char *argv[])
{
    int   status;
    char  mili_version[64], griz_mili_version[16];
    double mili_version_float, griz_mili_version_float;

    get_mili_version( mili_version);

    printf("\nMili Version = %s", mili_version);

    mili_version_float = atof(mili_version);

    strcpy(griz_mili_version, MILI_VERSION);
    griz_mili_version_float = atof(griz_mili_version);

    if (mili_version_float >= griz_mili_version_float)
    {
        printf("\nMILI_LIB_OK -- Version=%4.2f\n", (float) mili_version_float);
    }
    else
    {
        printf("\nMILI_LIB_OUTOFDATE -- Version found=%f / Expected=%f,",
               (float) mili_version_float, (float) griz_mili_version_float);
    }
    exit(0);
}
