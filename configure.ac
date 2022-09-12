
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
#	*	     Mili configure - File "configure.ac"    		*
#	*								*
#	*****************************************************************
#
#	Process this file with "autoconf" to produce "configure".
#
#	-----------------------------------------------------------------
#	Revision History:
#
#	20-Feb-05 EMP: Created
#
#       05-Oct-06 IRC: Added a macro for SILO and HDF for future 
#                      development
#
#       02-Oct-07 IRC: Updated
#                      Add support for KlockWorks build - Re: Bill Oliver.
#		       SCR # 429
#
#	-----------------------------------------------------------------
#
#############################################################################
#
#	Initialize AutoConf.
#
#############################################################################

m4_define(MILI_NAME_M4, Mili)
m4_define(MILI_VERSION_M4, V22_01)
m4_define(MILI_DATE_M4, 9/1/2022)
m4_define(MILI_TIME_M4, 08:00:00)
m4_define(MILI_DATETIME_M4, [MILI_DATE_M4 MILI_TIME_M4])
m4_define(MILI_BUGS_M4,durrenberger1@llnl.gov)
m4_define(MILI_TOP_SRC_DIR_M4, `pwd`)

AC_INIT([MILI_NAME_M4],[MILI_VERSION_M4],[MILI_BUGS_M4],[MILI_NAME_M4])
    
MILI_VERSION=MILI_VERSION_M4
AC_SUBST([MILI_VERSION])

AC_REVISION($Revision: 1.50 $)


AC_PREREQ(2.63)
AC_DEFINE_UNQUOTED(CONFIG_CMD, "$0 $@", [Configure command])
CONFIG_CMD="echo $1 $@"
AC_SUBST(CONFIG_CMD)
CONFIGURE_INIT



# Look for scripts in ./scripts
AC_CONFIG_AUX_DIR(scripts)

# Determine the host type
AC_CANONICAL_HOST

# Turn off shared libraries by default, since build process takes longer.
#AC_DISABLE_SHARED
#AM_CONDITIONAL([STATICONLY],[test "$enable_shared" = no])
# Disable the automatic (smart) rebuild rules
#AM_MAINTAINER_MODE
#AM_INIT_AUTOMAKE([1.9 foreign no-installinfo no-installman no-texinfo.tex])

# Variable Initialization
AC_DEFINE([PACKAGE_DATE], ["MILI_DATE_M4"], [Date of release])
AC_DEFINE([PACKAGE_TIME], ["MILI_TIME_M4"], [Time of release])
AC_DEFINE([PACKAGE_DATETIME], ["MILI_DATETIME_M4"], [Date and time of release])
PACKAGE_DATETIME="MILI_DATE_M4 MILI_TIME_M4"
AC_SUBST([PACKAGE_DATETIME])

#############################################################################
#
#	Check for system parameters such as HOST and OS type.
#
#############################################################################


CONFIGURE_OS

CONFIGURE_STRESSLINK
CONFIGURE_SILO
CONFIGURE_HDF

CONFIGURE_HOST
# select compiler

#AC_PROG_FC
AC_PROG_F77([ifort xlf gfortran])
AC_PROG_CC([icc xlc gcc])
AC_CHECK_SIZEOF(long)

CONFIGURE_64bit
CONFIGURE_COMPILER

#AC_MSG_ERROR([])
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

# CC Options
AC_SUBST(CC)
AC_SUBST(CC_INCLUDE_PATHS)
AC_SUBST(CC_FLAGS_DEBUG)
AC_SUBST(CC_FLAGS_OPT)
AC_SUBST(CC_DEFINES)
AC_SUBST(CC_DEPEND)

# FC Options
AC_SUBST(FC)
AC_SUBST(FC_FLAGS_DEBUG)
AC_SUBST(FC_FLAGS_OPT)

AC_SUBST(LIB_SUFFIX)

AC_SUBST(LIBRARY_PATHS)
AC_SUBST(LIBS)
AC_SUBST(OTHER_LIBS)

AC_SUBST(MILI_VERBOSE)

# AR variables
AC_SUBST(AR)
AC_SUBST(AR_FLAGS)


###################################
# MILI specific variables
###################################

# Mili Library Options
AC_SUBST(MILI_HOME)
AC_SUBST(MILI_INCLUDE_PATHS)
AC_SUBST(MILI_LIBRARY_PATHS)
AC_SUBST(MILI_DEFINES)
AC_SUBST(MILI_LIBRARY)

# Mili Library NAMES
AC_SUBST(MILI_LIB)
AC_SUBST(TAURUS_LIB)
AC_SUBST(EPRTF_LIB)
AC_SUBST(MILI_LIB_DEBUG)
AC_SUBST(TAURUS_LIB_DEBUG)
AC_SUBST(EPRTF_LIB_DEBUG)
AC_SUBST(MILI_LIB_OPT)
AC_SUBST(TAURUS_LIB_OPT)
AC_SUBST(EPRTF_LIB_OPT)

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


#############################################################################
#
#	Set the directory names and paths.
#
#############################################################################
CONFIGURE_KLOCWORK

CONFIGURE_DIRS

CONFIGURE_BUILDDIRS

CONFIGURE_INSTALL

#############################################################################
#
#	Configure the Compiler Options
#
#############################################################################

CONFIGURE_SHARED

CONFIGURE_COMPILER_FLAGS

CONFIGURE_COMPILER_MILI

CONFIGURE_MILI_LIBS

#############################################################################
# 
#	Fortran compiler and libraries support
#       use the more modern FC macro versions
# 
#############################################################################
#AC_PROG_FC
#AC_FC_WRAPPERS

#############################################################################
# 
#	Check for other functions nad libraries
# 
#############################################################################
AC_CHECK_LIB(m, isnan, AC_DEFINE(HAVE_ISNAN, 1, [Math Lib has isNaN funtion]))
# General procedure for checking Fortran Libraries, but in the case of
# libm.a this doesn't appear to work as expected.
#AC_FC_FUNC(isnan)
#AC_CHECK_LIB(m, $isnan, AC_DEFINE(HAVE_F77_ISNAN, 1, [isNaN F77 binding]))

#############################################################################
# 
#	Create the exported environment variables.
# 
#############################################################################
CFLAGS_DEBUG="$DEBUG_CFLAG_OPTIONS"
CFLAGS_OPT="$OPT_CFLAG_OPTIONS"
AC_CONFIG_HEADERS([$HOSTDIR/src/mili_config.h:scripts/config.h.in])

#############################################################################
#
#	Set the names of the 'Makefile's to be created.  Don't forget to
#       include the Makefile.Common file on all but the Driver.
#
#############################################################################
MAKEFILES="$PATH_NAME/Makefile $PATH_NAME/src_test/Makefile"

#############################################################################
#
#	Specify the commands to run at the end of 'configure'.
#
#############################################################################
AC_CONFIG_COMMANDS([default-1],[[rm -f config.cache config.log config.status]],[[]])

#############################################################################
#
#	Create the 'Makefile's from the template 'Makefile.in'
#	by exporting the AutoConf environment variables.
#
#############################################################################
AC_CONFIG_FILES($MAKEFILES)
AC_OUTPUT

# End of configure.ac

