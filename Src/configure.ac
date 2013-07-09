# $Id$
#	+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	+                                                               +
#	+                       Copyright (c) 2004                      +
#	+          The Regents of the University of California          +
#	+                      All Rights Reserved                      +
#	+                                                               +
#	+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#
#	*****************************************************************
#	*								*
#	*	     Griz4 configure - File "configure.ac"    		*
#	*								*
#	*****************************************************************
#
#	Process this file with "autoconf" to produce "configure".
#
#	-----------------------------------------------------------------
#	Revision History:
#
#	20-Feb-04 IRC: Created
#
#	05-Nov-04 trt: Renamed to configure.ac as per GNU convensions
#	24-Nov-04 trt: Added Fortran compiler/library support
#	30-Nov-04 trt: Added useful variables.
#	02-Oct-07 IRC: Added Configure for KlocWorks
#			SCR #492.
#	-----------------------------------------------------------------
#
#
# The Regents of the University of California.  
# All rights reserved.
# 
# This work was produced at the University of California, 
# Lawrence Livermore National Laboratory (UC LLNL) under 
# contract no. W-7405-ENG-48 between the U.S. Department of 
# Energy (DOE) and The Regents of the University of California
# (University) for   the operation of UC LLNL. Copyright is 
# reserved to the University for purposes of controlled dis-
# semination, commercialization through formal licensing, or 
# other disposition under terms of Contract 48; DOE policies, 
# regulations and orders; and U.S. statutes.
# 
# DISCLAIMER OF LIABILITY
# 
# This document was prepared as an account of work sponsored 
# by an agency of the United States Government.  Neither the 
# United States Government nor the University of California 
# nor any of their employees, makes any warranty, express
# or implied, including the warranties of merchantability
# and fitness for a particular purpose, or assumes any legal
# liability or responsibility for the accuracy, completeness,
# or usefulness of any information, apparatus, product, or 
# process disclosed, or represents that its use would not 
# infringe privately-owned rights.
# 
# DISCLAIMER OF ENDORSEMENT
# 
# Reference herein to any specific commercial products,
# process, or service by trade name, trademark, manufacturer,
# or otherwise, does not necessarily constitute or imply its
# endorsement, recommendation, or favoring by the United States
# Government or the University of California.  The views and 
# opinions of authors expressed herein do not necessarily state
# or reflect those of the United States Government or the 
# University of California, and shall not be used for advertising
# or product endorsement purposes.
#
#############################################################################
#
#	Initialize AutoConf.
#
#############################################################################

m4_define(GRIZ_NAME_M4, Griz4)
m4_define(GRIZ_VERSION_M4, V13_01)
m4_define(MILI_VERSION_M4, V13_01)
m4_define(GRIZ_DATE_M4, 2013/07/09)
m4_define(GRIZ_TIME_M4, 07:39:00)
m4_define(GRIZ_DATETIME_M4, [GRIZ_DATE_M4 GRIZ_TIME_M4])
m4_define(GRIZ_BUGS_M4,oliver4@llnl.gov)
m4_define(GRIZ_TOP_SRC_DIR_M4, `pwd`)

AC_INIT(GRIZ_NAME_M4, GRIZ_VERSION_M4, GRIZ_BUGS_M4, GRIZ_NAME_M4)
#AC_PROG_CC([icc xlc cc gcc])
GRIZ_VERSION=GRIZ_VERSION_M4
MILI_VERSION=MILI_VERSION_M4
AC_SUBST([GRIZ_VERSION])
AC_SUBST([MILI_VERSION])

AC_REVISION($Revision$)

AC_PREREQ(2.59)
AC_DEFINE_UNQUOTED(CONFIG_CMD, "$0 $@", [Configure command])
CONFIG_CMD="configure $@"
AC_SUBST([CONFIG_CMD])

AC_COPYRIGHT([This work was produced at the University of California, 
Lawrence Livermore National Laboratory (UC LLNL) under 
contract no. W-7405-ENG-48 between the U.S. Department of 
Energy (DOE) and The Regents of the University of California
(University) for   the operation of UC LLNL. Copyright is 
reserved to the University for purposes of controlled dis-
semination, commercialization through formal licensing, or 
other disposition under terms of Contract 48; DOE policies, 
regulations and orders; and U.S. statutes.])

CONFIGURE_INIT

CONFIGURE_VERSION

AC_CONFIG_AUX_DIR(scripts)
dnl Determine the host type
AC_CANONICAL_HOST
dnl Turn off shared libraries by default, since build process takes longer.
#AC_DISABLE_SHARED
#AM_CONDITIONAL([STATICONLY],[test "$enable_shared" = no])
dnl Disable the automatic (smart) rebuild rules
#AM_MAINTAINER_MODE

#AM_INIT_AUTOMAKE([1.9 foreign no-installinfo no-installman no-texinfo.tex])

# Variable Initialization
AC_DEFINE([PACKAGE_NAME], ["GRIZ_NAME_M4"], [Name of release])
AC_DEFINE([PACKAGE_DATE], ["GRIZ_DATE_M4"], [Date of release])
AC_DEFINE([PACKAGE_TIME], ["GRIZ_TIME_M4"], [Time of release])
AC_DEFINE([PACKAGE_DATETIME], ["GRIZ_DATETIME_M4"], [Date and time of release])
PACKAGE_DATETIME="GRIZ_DATE_M4 GRIZ_TIME_M4"
AC_SUBST([PACKAGE_DATETIME])

#############################################################################
#
#	Check for system parameters such as HOST and OS type.
#
#############################################################################
CONFIGURE_OS

CONFIGURE_HOST

dnl select compiler
CONFIGURE_64bit
CONFIGURE_COMPILER

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h sys/time.h unistd.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(gethostname regcomp strstr)

#############################################################################
#
#	Create the AutoConf environment variables for export.
#
#############################################################################
AC_SUBST(ARFLAGS)

# CC Options
#AC_SUBST(CC)
AC_SUBST(CC_INCLUDE_PATHS)
AC_SUBST(CC_FLAGS_DEBUG)
AC_SUBST(CC_FLAGS_OPT)
AC_SUBST(CC_DEFINES)
AC_SUBST(CC_DEPEND)

AC_SUBST(LOADER)
AC_SUBST(LD_FLAGS_DEBUG)
AC_SUBST(LD_FLAGS_OPT)

AC_SUBST(EXE_SUFFIX)
AC_SUBST(LIB_SUFFIX)

AC_SUBST(LIBRARY_PATHS)
AC_SUBST(LIBS)
AC_SUBST(OTHER_LIBS)

AC_SUBST(GRIZ_VERBOSE)

# AR variables
#AC_SUBST(AR)
#AC_SUBST(AR_FLAGS)


###################################
# GRIZ specific variables
###################################

# Mili Library Options
AC_SUBST(MILI_HOME)
AC_SUBST(MILI_INCLUDE_PATHS)
AC_SUBST(MILI_LIBRARY_PATHS)
AC_SUBST(MILI_DEFINES)
AC_SUBST(MILI_LIBRARY)

#############################################################################
#
# Determine if 'gmake' is available for building the code.	
#
#############################################################################
CONFIGURE_GMAKE
CONFIGURE_TASKS

#############################################################################
# 
# Configure library options
#
#############################################################################

CONFIGURE_MILI
if test "$OS_NAME" = "IRIX64" -o "$OS_NAME" = "IRIX"; then
CONFIGURE_SGI_GRAPHICS
else
CONFIGURE_X11

CONFIGURE_MESA

CONFIGURE_OSMESA
fi
CONFIGURE_BITMAPS

CONFIGURE_IMAGE

CONFIGURE_HERSHEY

CONFIGURE_EXODUS

CONFIGURE_NETCDF

CONFIGURE_JPEG

CONFIGURE_PNG

CONFIGURE_KLOCWORK

#############################################################################
#
#	Set the directory names and paths.
#
#############################################################################
CONFIGURE_DIRS
CONFIGURE_BUILDDIRS
EXTERNAL_CONFIGURATIONS
CONFIGURE_INSTALL

#############################################################################
#
#	Configure the Compiler Options
#
#############################################################################
CONFIGURE_COMPILER_FLAGS
CONFIGURE_COMPILER_GRIZ
CONFIGURE_GRIZ_LIBRARIES
CONFIGURE_COMPILER_HERSHEYLIB
CONFIGURE_COMPILER_IMAGELIB

#############################################################################
# 
#	Fortran compiler and libraries support
#       use the more modern FC macro versions
# 
#############################################################################
# AC_PROG_FC
# AC_FC_LIBRARY_LDFLAGS
# AC_FC_WRAPPERS
#############################################################################
# 
#	Check for other functions nad libraries
# 
#############################################################################
#AC_CHECK_LIB(m, isnan, AC_DEFINE(HAVE_ISNAN, 1, [Math Lib has isNaN funtion]))
AC_SEARCH_LIBS([isnan],[m],[AC_DEFINE(HAVE_ISNAN, 1, [Math Lib has isNaN funtion])])
# General procedure for checking Fortran Libraries, but in the case of
# libm.a this doesn't appear to work as expected.
#AC_FC_FUNC(isnan)
#AC_CHECK_LIB(m, $isnan, AC_DEFINE(HAVE_F77_ISNAN, 1, [isNaN F77 binding]))
#############################################################################
# 
#	Create the exported environment variables.
# 
#############################################################################

CFLAGS_DEBUG="$DEBUG_CFLAG_OPTIONS $GPROF_FLAGS"
CFLAGS_OPT="$OPT_CFLAG_OPTIONS $GPROF_FLAGS"

AC_CONFIG_HEADERS([griz_config.h:scripts/config.h.in])
# Look for scripts in ./scripts

#############################################################################
#
#	Set the names of the 'Makefile's to be created.  Don't forget to
#       include the Makefile.Common file on all but the Driver.
#
#############################################################################
MAKEFILES="$PATH_NAME/Makefile $PATH_NAME/src/Makefile $PATH_NAME/ImageLib/Makefile $PATH_NAME/HersheyLib/src/Makefile $MAKEFILES"

#############################################################################
#
#	Specify the commands to run at the end of 'configure'.
#
#############################################################################

AC_OUTPUT_COMMANDS([rm -f config.cache config.log config.status])
#############################################################################
#
#	Create the 'Makefile's from the template 'Makefile.in'
#	by exporting the AutoConf environment variables.
#
#############################################################################
AC_CONFIG_FILES($MAKEFILES)
AC_OUTPUT

# End of configure.ac

