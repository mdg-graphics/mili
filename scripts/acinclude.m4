# $Id: acinclude.m4,v 1.56 2021/08/12 20:07:14 jdurren Exp $
#	+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	+                                                               +
#	+                       Copyright (c) 2010                      +
#	+             Lawrence Livermore National Laboratory            +
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
#       14-Nov-05 EMP: Created
#
#       05-Oct-06 IRC: Added a macro for SILO and HDF for future 
#                      development
#
#       24-Aug-07 IRC: For Silo builds, add _SILO to the dir name.
#
#       02-Oct-07 IRC: Updated
#                      Add support for KlockWorks build - Re: Bill Oliver.
#		       SCR # 429
#
#	-----------------------------------------------------------------
#
# Notes:>
#   For IBM Linux xlc compiler use -q32 or -q64 for 32 or 64-bit.
#       IBM Use compilers xlc/xlf
#
#################################################################

AC_DEFUN([CONFIGURE_INIT],
  [
        #
        # Init Configure options string
        #
        CONFIG_OPTIONS="CONFIGURE=configure "
        AC_SUBST(CONFIG_OPTIONS)
  ])

AC_DEFUN([CONFIGURE_OS],
  [
        #
        # Determine the name and version of the operating system.
        #
        AC_MSG_CHECKING(for name and version of operating system)
        OS_NAME="`(uname -s) 2> /dev/null`"
        OS_NAME_VERSION="`(uname -s -r) 2> /dev/null`"
        
        PROCESSOR_TYPE="`(uname -p) 2> /dev/null`"

        AC_DEFINE_UNQUOTED(OS_NAME,        $OS_NAME,        [Operating system name])
        AC_DEFINE_UNQUOTED(PROCESSOR_TYPE, $PROCESSOR_TYPE, [Processor Type])

        AC_SUBST(OS_NAME)
        AC_SUBST(PROCESSOR_TYPE)
        AC_MSG_RESULT($OS_NAME_VERSION)
  ])


AC_DEFUN([CONFIGURE_HOST],
  [
    #
    #  Set the prfix of the build directory
    #
    if test "$SILO_ENABLE" = "True"; then
      HOST_PREFIX="MILI-SILO-"
    else
      HOST_PREFIX="MILI-"
    fi
    
    #
    # Determine the name of the HOST machine
    #
    AC_MSG_CHECKING(for name of host machine)
    HOSTNAME="`(hostname | tr -d '[0-9]' |  tr '[a-z]' '[A-Z]') 2> /dev/null`"
    HOSTDIR="`(echo $SYS_TYPE) 2> /dev/null`"
    
    # Determine the name of the host directory - this is usually where the 
    # code is installed.
    if test "$HOSTDIR" = ""; then
      TEMPLIST=`echo ":$TEMPDIR" | tr '[/]' '[ ]'`
      HOSTDIR=$OS_NAME
      
      HOSTDIR="`(type mili) 2> /dev/null`"
      HOSTDIR="`(ls -l $HOSTDIR | grep mdg) 2> /dev/null`" 
      
      for dir in $HOSTDIR; do
        TEMPDIR=$dir
      done
      
      TEMPLIST=`echo ":$TEMPDIR" | tr '[/]' '[ ]'`
      
      found_platform="False"
      
      HOSTDIR=""
      
      for dir in $TEMPLIST; do
        TEMPDIR=$dir
        if test "$found_platform" = "True"; then
          HOSTDIR=$dir
          found_platform="False"
        fi
        if test "$dir" = "mdg"; then
          found_platform="True"
        fi
      done
      
      if test "$HOSTDIR" = ""; then
        TEMPLIST=`echo ":$TEMPDIR" | tr '[/]' '[ ]'`
        HOSTDIR=$OS_NAME
      fi
      
      if test "$HOSTDIR" = "bin"; then
        HOSTDIR=$HOSTNAME
      fi
    fi
    
    HOSTDIR="$HOSTDIR-$HOSTNAME"
    
    AC_DEFINE_UNQUOTED(HOSTDIR, $HOSTDIR, [Host directory name ($SYS_TYPE)])
    AC_SUBST(HOSTDIR)
    AC_DEFINE_UNQUOTED(HOSTNAME, $HOSTNAME, [Host name])
    AC_SUBST(HOSTNAME)
    AC_MSG_RESULT(Hostname=$HOSTNAME)
  ]
)

AC_DEFUN([CONFIGURE_64bit],
  [
    #
    # Set options for running with 64bit support
    #
    bits64_ENABLE="False" 
    WORD_SIZE="" 
    AC_ARG_ENABLE([bits64],
      [AC_HELP_STRING([--enable-bits64],[Compile and load with 64bit option])],
      [
        TARGET_DNS_DOMAIN="`(dnsdomainname) 2> /dev/null`"
        case $TARGET_DNS_DOMAIN in
          *llnl.gov)
            
            case "$build" in                     
              powerpc64*linux-gnu)
                case "$CC" in
                  *xlc)
                    WORD_SIZE=" -q64 "
                    bits64_ENABLE="True"   
                    AC_MSG_RESULT(---->Enabling: 64bit)
                    ;; 
                esac
                ;;
            esac  
            ;;
        esac
      ],
      [
        bits64_ENABLE="False"
      ]
    )
    # Export 64bit Options
    AC_SUBST(bits64_ENABLE)
    AC_SUBST(WORD_SIZE)
    AC_SUBST(EXE_SUFFIX)
  ]
)

AC_DEFUN([CONFIGURE_MILI_LIBS],
  [
    MILI_LIB="libmili.a"
    TAURUS_LIB="libtaurus.a"
    EPRTF_LIB="libeprtf.a"
    SILO_LIB="libmilisilo.a"
    SL_LIB="libsl.a"
    MILISILOAPI_LIB="libmilisiloapi.a"
    SILO_LIB_LD="milisilo"
    MILISILOAPI_LIB_LD="milisiloapi"
    SL_LIB_LD="sl"
    
    MILI_LIB_DEBUG="libmili.a"
    TAURUS_LIB_DEBUG="libtaurus.a"
    EPRTF_LIB_DEBUG="libeprtf.a"
    SILO_LIB_DEBUG="libsilo.a"
    SL_LIB_DEBUG="libsl.a"
    MILISILOAPI_LIB_DEBUG="libmilisiloapi.a"

    SILO_LIB_OPT="libmilisilo_opt.a"
    SL_LIB_OPT="libsl_opt.a"
    MILISILOAPI_LIB_OPT="libmilisiloapi_opt.a"
    
    MILI_LIB_OPT="libmili.a"
    TAURUS_LIB_OPT="libtaurus.a"
    EPRTF_LIB_OPT="libeprtf.a"
    SILO_LIB_OPT="libmilisilo.a"
    SL_LIB_OPT="libsl.a"
    
    AC_SUBST(MILI_LIB)
    AC_SUBST(TAURUS_LIB)
    AC_SUBST(EPRTF_LIB)
    AC_SUBST(SILO_LIB)
    AC_SUBST(SL_LIB)
    AC_SUBST(MILI_LIB_DEBUG)
    AC_SUBST(TAURUS_LIB_DEBUG)
    AC_SUBST(EPRTF_LIB_DEBUG)
    AC_SUBST(SILO_LIB_DEBUG)
    AC_SUBST(SL_LIB_DEBUG)
    AC_SUBST(MILISILOAPI_LIB)
    AC_SUBST(MILISILOAPI_LIB_DEBUG)
    
    AC_SUBST(MILI_LIB_OPT)
    AC_SUBST(TAURUS_LIB_OPT)
    AC_SUBST(EPRTF_LIB_OPT)
    AC_SUBST(SILO_LIB_OPT)
    AC_SUBST(SL_LIB_OPT)
    AC_SUBST(MILISILOAPI_LIB_OPT)
    AC_SUBST(SILO_LIB_LD)
    AC_SUBST(SL_LIB_LD)
    AC_SUBST(MILISILOAPI_LIB_LD)
    AC_SUBST(MILI_LIBS)
  ]
)

AC_DEFUN([CONFIGURE_INSTALL],
  [
    #
    # Set the directory names and paths
    #
    case "$TARGET_DNS_DOMAIN" in
      *llnl.gov)
        INSTALL_HOME="/usr/gapps/mdg"
        ;;
      *)
        
        ;;
    esac
    TOP_DIR="`pwd`"
    INSTALL_DIR="src"
    AC_SUBST(INSTALL_HOME)
    AC_SUBST(INSTALL_DIR)
  ]
)


AC_DEFUN([CONFIGURE_DIRS],
  [
    #
    # Set the directory names and paths
    #
    MILI_HOME="."
    TOP_DIR="`pwd`"
    SRC_DIRS="src"
    if test "$SILO_ENABLE" = "True"; then
      SRC_DIRS="$SRC_DIRS src/SiloLib src/MiliSilo" 
    fi
    if test "$SL_ENABLE" = "True"; then
      SRC_DIRS="$SRC_DIRS src/StressLink"
    fi
    
    #
    # These are source directories with Makefiles - some source directories are
    # just place holders for source files and have no Makefile - they are referenced
    # by an upper level Makefile.
    #
    
    SRC_DIRS_MAKE="src"
    SRC_TEST_DIR="src_test"
    SRC_UTLIS_DIR="utils"
    
    OBJS_DIRS="objs_debug objs_opt objs_test"

    LIB_DIRS="lib_debug lib_opt lib lib64_debug lib64_opt lib64"
    
    if test "$bits64_ENABLE" = "True"; then
      # Set paths to 64-bit name
      LIBDEBUG_PATH="lib64_debug"
      LIBOPT_PATH="lib64_opt"
      LIB_PATH="lib64"
    else
      LIBDEBUG_PATH="lib_debug"
      LIBOPT_PATH="lib_opt"
      LIB_PATH="lib"
    fi
    INC_DIRS="include"
    
    BIN_DIR="bin_utils_debug bin_utils_opt bin_test bin_utils64_debug bin_utils64_opt bin_test64"
    
    if test "$bits64_ENABLE" = "True"; then
      BINUTILSDEBUG_PATH="bin_utils64_debug"
      BINUTILSOPT_PATH="bin_utils64_opt"
      BINTEST_PATH="bin_test64"
    else
      BINUTILSDEBUG_PATH="bin_utils_debug"
      BINUTILSOPT_PATH="bin_utils_opt"
      BINTEST_PATH="bin_test"
    fi
    AC_ARG_WITH([install],
      [AC_HELP_STRING([--install-path=[PATH]],[Use given base PATH installing Mili])],
      [
        INSTALL_DIRS="-I${withval}/include" 
        AC_MSG_RESULT(---->Installing Mili st: $withval)
      ]
    )
    AC_ARG_WITH([usrbuild],
      [AC_HELP_STRING([--with-usrbuild=[PATH]],[Use given PATH for BUILD DIRECTORY])],
      [
        BASE_DIR="${withval}"
        AC_MSG_RESULT("Setting Build directory to : $withval")
      ]
    )
    
    AC_SUBST(MILI_HOME)
    AC_SUBST(TOP_DIR)
    AC_SUBST(SRC_DIRS)
    AC_SUBST(SRC_DIRS_MAKE)
    AC_SUBST(SRC_TEST_DIR)
    AC_SUBST(BIN_DIR)
    AC_SUBST(LIB_DIRS)
    AC_SUBST(INC_DIRS)
    AC_SUBST(INSTALL_DIRS)
    AC_SUBST(BASE_DIR)
    AC_SUBST(LIBDEBUG_PATH)
    AC_SUBST(LIBOPT_PATH)
    AC_SUBST(LIB_PATH)
    AC_SUBST(BINUTILSDEBUG_PATH)
    AC_SUBST(BINUTILSOPT_PATH)
    AC_SUBST(BINTEST_PATH)
  ]
)


AC_DEFUN([CONFIGURE_BUILDDIRS],
  [
    #
    # Create the operating-system dependent build directories.
    #
    if test "$BASE_DIR" != ""; then
      if test "$SILO_ENABLE" = "True"; then
	      HOSTDIR="MILI-SILO-$BASE_DIR-$HOSTNAME"
      else
        HOSTDIR="MILI-$BASE_DIR-$HOSTNAME"
      fi
    else
      HOSTDIR="$HOST_PREFIX""$HOSTDIR""$EXE_SUFFIX"
    fi
#    if test -d "$HOSTDIR"; then
#      mv -f "$HOSTDIR" "$HOSTDIR-old"
#    fi
    if test "$bits64_ENABLE" = "True"; then
      HOSTDIR="$HOSTDIR""_64bit"
    fi
    if test ! -d "$HOSTDIR"; then
      AC_MSG_RESULT([Creating directory:  $PATH_NAME])
      mkdir "$HOSTDIR"
      if test ! -d "$HOSTDIR"; then
        AC_MSG_WARN([****  Error creating directory:  $HOSTDIR])
      fi
    fi
    for dir in $SRC_DIRS $SRC_UTLIS_DIR $LIB_DIRS $INC_DIRS $SRC_TEST_DIR $BIN_DIR $OBJS_DIRS $MISC_DIRS; do
      PATH_NAME=$HOSTDIR
      set PATH_NAME_TEMP `echo ":$dir" | sed -ne 's/^:\//#/;s/^://;s/\// /g;s/^#/\//;p'`
      shift
      for dir_name do
        PATH_NAME="$PATH_NAME/$dir_name"
          case "$PATH_NAME" in
            -* ) PATH_NAME=./$PATH_NAME ;;
          esac
          if test ! -d "$PATH_NAME"; then
            AC_MSG_RESULT("Creating directory:  $PATH_NAME")
            mkdir "$PATH_NAME"
            if test ! -d "$PATH_NAME"; then
              AC_MSG_WARN("****  Error creating directory:  $PATH_NAME")
            fi
          fi
      done
    done

    ROOT_DIR=$TOP_DIR/$HOSTDIR
    cd $ROOT_DIR
    
    if test ! -e "Doc"; then
      ln -sf ../doc Doc
    fi
    
    ln -sf ../Makefile.Library Makefile.in
    
    # Set up links for source files
    cd src
#    ln -sf ../../Makefile.Library Makefile.in
#    ln -sf ../../Makefile.Library-Debug MakefileDebug.in
#    ln -sf ../../Makefile.Library-Opt MakefileOpt.in
#    ln -sf ../../Makefile.Library-Install MakefileInstall.in
    
    ln -sf ../../src/*.c .     > /dev/null 2>&1
    ln -sf ../../src/*.f .     > /dev/null 2>&1
    ln -sf ../../src/*.F .     > /dev/null 2>&1
    ln -sf ../../src/*.h .     > /dev/null 2>&1
    cp -f ../../mili_config.h . > /dev/null 2>&1
    
    # Set up links for test case files
    cd ../src_test
    ln -sf ../../Makefile.Tests Makefile.in
    ln -sf ../../test/mili/*.c .     > /dev/null 2>&1
    ln -sf ../../test/mili/*.f .     > /dev/null 2>&1
    
    
    # Set up links for test case files
    cd ../utils
    ln -sf ../../utils/*.c .     > /dev/null 2>&1
    ln -sf ../../utils/*.h .     > /dev/null 2>&1
    
    cd ..
    cd include
    ln -sf ../../src/*.h .     > /dev/null 2>&1
    
    # Set up links for include files
#    cd $ROOT_DIR
#    cd include
#    ln -sf ../../src/*.h . > /dev/null 2>&1
    
#    cd $ROOT_DIR
#    for dir in $INC_DIRS; do
#      dir_tmp="$TOP_DIR/$dir"
#      cd "$dir_tmp"
#      for file in *.h; do
#        if test ! -e "$TOP_DIR/$HOSTDIR/$dir/$file" && test -e $dir_tmp/$file; then
#          ln -s $dir_tmp/$file $TOP_DIR/$HOSTDIR/$dir/$file > /dev/null 2>&1
#        fi
#      done
#      for file in *.h.in; do
#        if test ! -e "$TOP_DIR/$HOSTDIR/$dir/$file" && test -e $dir_tmp/$file; then
#          ln -s $dir_tmp/$file $TOP_DIR/$HOSTDIR/$dir/$file > /dev/null 2>&1
#        fi
#      done
#      cd $TOP_DIR
#    done
    
    # Misc directories do not consist of code
    cd $TOP_DIR
    PATH_NAME="$HOSTDIR"
    for dir in $MISC_DIRS; do
      cd $PATH_NAME/$dir
      # Link to all files in these directories
      dir_tmp="$TOP_DIR/$dir"
      cd "$dir_tmp"
      for file in *; do
        if test ! -e "$TOP_DIR/$HOSTDIR/$dir/$file" && test -e $dir_tmp/$file; then
          ln -s $dir_tmp/$file $TOP_DIR/$HOSTDIR/$dir/$file > /dev/null 2>&1
        fi
      done
      cd $TOP_DIR
    done

    AC_SUBST(ROOT_DIR)
  ]
)


AC_DEFUN([CONFIGURE_TASKS],
  [
    #
    # Define the number of tasks based on the system type
    #
    TASKS=8;
    AC_SUBST(TASKS)
  ])


AC_DEFUN([CONFIGURE_GMAKE],
  [
    #
    # Determine if 'gmake' is available for building the code.
    #
    AC_CHECK_PROG(FOUND_GMAKE,gmake,"yes","no")
    if test "$FOUND_GMAKE" = "no"; then
      AC_MSG_ERROR("**** 'gmake' is REQUIRED to build the code!")
    fi
  ]
)

AC_DEFUN([CONFIGURE_COMPILER_FLAGS],
  [
    #
    # Define the compile search order
    #
    CC_DEFINES="$CC_DEFINES"

    if test "$PROCESSOR_TYPE" != "ppc64"; then
      CC_INCLUDE_PATHS=""
    fi
    if test "$PROCESSOR_TYPE" != "x86_64"; then 
      CC_DEFINES="-DNOOPTERON $CC_DEFINES"
    fi
    if test $ac_cv_sizeof_long = 8;then
        CC_DEFINES="-DHAVEINT8 $CC_DEFINES"
    fi
#    CC_INCLUDE_PATHS="`echo $CC_INCLUDE_PATHS | sed -e 's|-O2||'`"
    FF_DEFINES=""
    AC_MSG_RESULT([**************************************])
    AC_MSG_RESULT([---->Include Paths: $CC_INCLUDE_PATHS ])
    AC_MSG_RESULT([**************************************])
    AC_SUBST(CC_DEFINES)
    AC_SUBST(CC_INCLUDE_PATHS)
    AC_SUBST(FF_DEFINES)
  ])


AC_DEFUN([CONFIGURE_COMPILER],
  [
    case $CC in
      *icc)
        CC_FLAGS_DEBUG="-g $WORD_SIZE "
        CC_FLAGS_OPT="-O3 $WORD_SIZE "
        CC_FLAGS_LD_DEBUG="-g $WORD_SIZE"
        CC_FLAGS_LD_OPT="-O3 $WORD_SIZE"
        ;;
      *xlc)
        CC_FLAGS_DEBUG="-g $WORD_SIZE "
        CC_FLAGS_OPT="-O4 $WORD_SIZE "
        CC_FLAGS_LD_DEBUG="-g $WORD_SIZE"
        CC_FLAGS_LD_OPT="-O4"
        ;;
      *gcc)
        CC_FLAGS_DEBUG="-g $WORD_SIZE "
        CC_FLAGS_OPT="-O4 $WORD_SIZE "
        CC_FLAGS_LD_DEBUG="-g $WORD_SIZE"
        CC_FLAGS_LD_OPT="-O4 $WORD_SIZE"
        ;;
      *cc)
        ;;
    esac
    case $F77 in
      *ifort)
        FC_FLAGS_DEBUG="-g $WORD_SIZE "
        FC_FLAGS_OPT="-O3 $WORD_SIZE "
        FC_FLAGS_LD_DEBUG="-g $WORD_SIZE"
        FC_FLAGS_LD_OPT="-O3 $WORD_SIZE"
        ;;
      *xlf)
        FC_FLAGS_DEBUG="-g  $WORD_SIZE -WF,-DAIX"
        FC_FLAGS_OPT="-O3  $WORD_SIZE -WF,-DAIX"
        FC_FLAGS_LD_DEBUG="-g $WORD_SIZE -WF,-DAIX"
        FC_FLAGS_LD_OPT="-O3 $WORD_SIZE -WF,-DAIX"
        ;;
      *gfortran)
        CC_FLAGS_DEBUG="-g $WORD_SIZE "
        CC_FLAGS_OPT="-O3 $WORD_SIZE "
        CC_FLAGS_LD_DEBUG="-g $WORD_SIZE"
        CC_FLAGS_LD_OPT="-O3 $WORD_SIZE"
        ;;
      *cc)
        ;;
    esac
    SHELL="/bin/sh"
    SHELL_ARGS="-ec"
    
    AC_PROG_INSTALL
    AC_SUBST(AR)
    AC_SUBST(AR_FLAGS)
    AC_SUBST(RANLIB)
    AC_SUBST(SHELL)
    AC_SUBST(SHELL_ARGS)
    AC_SUBST(CC_DEPEND)
    AC_SUBST(CC_FLAGS_DEBUG)
    AC_SUBST(CC_FLAGS_OPT)
    AC_SUBST(CC_FLAGS_LD_DEBUG)
    AC_SUBST(CC_FLAGS_LD_OPT)
    AC_SUBST(FC_FLAGS_DEBUG)
    AC_SUBST(FC_FLAGS_OPT)
    AC_SUBST(FC_FLAGS_LD_DEBUG)
    AC_SUBST(FC_FLAGS_LD_OPT)
    AC_SUBST(LDFLAGS_EXTRA)
  ]
)

AC_DEFUN([CONFIGURE_EPRINTF],
  [
    AC_ARG_ENABLE([eprintf],
      [AC_HELP_STRING([--enable-eprintf],[Build with eprintf option])],
      [
        CC_DEFINES="$CC_DEFINES -DHAVE_EPRINT "
      ]
    )
  ]
)

AC_DEFUN([CONFIGURE_COMPILER_MILI],
  [
    #
    # Define the compile search order
    #
    
    CC_DEFINES_MILI="$CC_DEFINES"
#    CC_DEFINES_MILI="`echo $CC_DEFINES_MILI | sed -e 's|-O2||'`"
    
#    if test "$PROCESSOR_TYPE" != "ppc64"; then
#      CC_INCLUDE_PATHS_MILI=" -I/usr/local/include -I/usr/include "
#    fi
    
    CC_INCLUDE_PATHS_MILI="$CC_INCLUDE_PATHS "
#    CC_INCLUDE_PATHS_MILI="`echo $CC_INCLUDE_PATHS_MILI | sed -e 's|-O2||'`"
    AC_SUBST(CC_DEFINES_MILI)
    AC_SUBST(CC_INCLUDE_PATHS_MILI)
  ]
)
 
AC_DEFUN([CONFIGURE_SILO],
  [
    #
    # Set options for the Silo Library
    #
    CC_DEFINES_SILO=""
    SILO_ENABLE_DEBUG=""
    SILO_HOME=""
    SILO_VERSION=""
    SILO_LIB=""
    SILO_LIB_DIR=""
    SILO_INC_DIR=""
    SILO_INC=""
    SZIP_HOME=""
    SZIP_VERSION=""
    SZIP_LIB=""
    SZIP_LIB_DIR=""
    SZIP_INC_DIR=""
    SZIP_INC=""
    AC_ARG_ENABLE([silo],
      [AC_HELP_STRING([--enable-silo],[Compile and load with SILO])],
      [
        SILO_ENABLE="True"  
        AC_MSG_RESULT(---->Enabling: Silo)
        SILO_VERSION="4.6.1"
        SILO_HOME="/usr/gapps/silo/$SILO_VERSION/$SYS_TYPE"
        SZIP_HOME="/usr/gapps/silo/szip/2.1/$SYS_TYPE"
        AC_ARG_WITH([silo],
          [AC_HELP_STRING([--with-silo=PATH],[Use given base PATH for SILO libraries and header files])],
          [
            SILO_HOME="${withval}"
            AC_MSG_RESULT("Using SILO Path : $withval")
          ]
        )
        AC_ARG_ENABLE([silo-debug],
          [AC_HELP_STRING([--enable-silo-debug],[Compile and load with debug version of SILO])],
          [
            SILO_DEBUG_ENABLE="True"  
            AC_MSG_RESULT([---->Enabling: silo-debug])
          ],
          [
            SILO_DEBUG_ENABLE="False"
          ]
        )
    
   
    
        AC_ARG_ENABLE([siloh5],
          [AC_HELP_STRING([--enable-siloh5],[Compile and load with Silo HDF5])],
          [
            SILOH5="True"  
            AC_MSG_RESULT([---->Enabling: Silo HDF5])
          ],
          [
            SILOH5="False"
          ]
        )
    
        if test "$SILO_DEBUG_ENABLE" = "True"; then
          SILO_ENABLE="True"
        fi
        CC_DEFINES_SILO="  "
        if test "$SILO_ENABLE" = "True"; then
          CC_DEFINES_SILO="-DSILOENABLED"
        fi
    
        /* Do not enable Silo compile if just SL enabled */
        if test "$SL_ENABLE" = "True"; then
          SILOH5="True"
          SILO_ENABLE="True"
          AC_MSG_RESULT(---->Enabling: Silo HDF5)
        fi
    
        SZIP_INC_DIR="$SZIP_HOME/include"
        SZIP_LIB_DIR="$SZIP_HOME-gcc-3.4.4/lib  "
        SZIP_LIB=" -lz  "
        if test  "$SZIP_HOME" = ""; then
          SZIP_INC_DIR=""
          SZIP_LIB_DIR=""
          SZIP_LIB=""
        fi
    
        SILO_LIB=" -lsilo "
        if test  "$SILOH5" = "True"; then
          SILO_LIB=" -lsiloh5 "
        fi
    
        CONFIG_OPTIONS="$CONFIG_OPTIONS --with-silo=$SILO_HOME $SZIP_HOME"
    
        SILO_INC_DIR="$SILO_HOME/include"
        SILO_LIB_DIR="$SILO_HOME/lib"
              
      ],
      [
        SILO_ENABLE="False"
      ]
    )
    
    #  SILO Library Options
    AC_SUBST(SILO_ENABLE)
    AC_SUBST(CC_DEFINES_SILO)
    AC_SUBST(SILO_ENABLE_DEBUG)
    AC_SUBST(SILO_HOME)
    AC_SUBST(SILO_VERSION)
    AC_SUBST(SILO_LIB)
    AC_SUBST(SILO_LIB_DIR)
    AC_SUBST(SILO_INC_DIR)
    AC_SUBST(SILO_INC)
    AC_SUBST(SZIP_HOME)
    AC_SUBST(SZIP_VERSION)
    AC_SUBST(SZIP_LIB)
    AC_SUBST(SZIP_LIB_DIR)
    AC_SUBST(SZIP_INC_DIR)
    AC_SUBST(SZIP_INC)
  ]
)


AC_DEFUN([CONFIGURE_HDF],
  [
    HDF_HOME=""
    HDF_INC_DIR=""
    HDF_INC=""
    HDF_LIB_DIR=""
    HDF_LIB=""
    HDF_HOME=""
    
    AC_ARG_ENABLE([hdf],
      [AC_HELP_STRING([--enable-hdf],[Compile and load with HDF])],
      [
        HDF_ENABLE="True"
        AC_MSG_RESULT([---->Enabling: HDF])
        AC_ARG_WITH([hdf],
          [AC_HELP_STRING([--with-hdf=[PATH]],[Use given base PATH for HDF libraries and header files])],
          [
            HDF_HOME="${withval}"
            AC_MSG_RESULT([Using HDF Path : $withval])
            HDF_LIB_DIR="-L$HDF_HOME/lib"
            HDF_INC_DIR="$HDF_HOME/lib"
            HDF_INC_CC_DIR=" -I$HDF_HOME/include "
            HDF_INC="$HDF_LIB_DIR/../include $HDF_LIB_DIR"
            CONFIG_OPTIONS="$CONFIG_OPTIONS --with-hdf=$HDF_HOME "
            AC_DEFINE(HAVE_HDF, 1, [HDF library])
          ]
	      )
      ],
      [
        HDF_ENABLE="False"
      ]
    )
    
    #  HDF Library Options
    AC_SUBST(HDF_HOME)
    AC_SUBST(HDF_LIB_DIR)
    AC_SUBST(HDF_LIB)
    AC_SUBST(HDF_INC_DIR)
    AC_SUBST(HDF_INC_CC_DIR) 
    AC_SUBST(HDF_INC)
    AC_SUBST(HDF_ENABLE)
    AC_SUBST(HDF_DEBUG_ENABLE)
  ]
)


AC_DEFUN([CONFIGURE_STRESSLINK],
  [
    #
    # Set options for the SL Library
    #
    Sl_VERSION=""
    
    CC_DEFINES_SL=""
    
    SL_ENABLE="False"
    AC_ARG_ENABLE([sl],
      [AC_HELP_STRING([--enable-sl],[Compile and load with Stresslink])],
      [
        SL_ENABLE="True" 
        CC_DEFINES_SL="-DSLENABLED"
        Sl_VERSION="1.0"
        AC_MSG_RESULT([---->Enabling: StressLink])
      ]
    )
    #  SL Library Options
    AC_SUBST(SL_ENABLE)
    AC_SUBST(CC_DEFINES_SL)
    AC_SUBST(SL_VERSION)
  ]
)

AC_DEFUN([CONFIGURE_KLOCWORK],
  [
    #
    # Set options for KW
    #
    KW_ENABLE="False" 
    KW_HOME="/var/tmp/klocwork/project_sources/griz/"
    AC_ARG_ENABLE([kw],
      [AC_HELP_STRING([--enable-kw],[Compile and load with KlocWorks])],
      [
        KW_ENABLE="True"
        AC_MSG_RESULT([---->Enabling: KlocWorks])
      ],
      [
        KW_ENABLE="False"
      ]
    )
    AC_ARG_WITH([kw],
      [AC_HELP_STRING([--with-kw=[PATH]],[Use given base PATH for KW trace file])],
      [
        KW_HOME="${withval}"
        AC_MSG_RESULT([Using KlocWorks Path : $withval])
      ]
    )
    if test "$KW_ENABLE" = "True"; then
      EXE_SUFFIX="$EXE_SUFFIX""_KlocWorks"
    fi
    
    # Bitmaps Options
    AC_SUBST(KW_HOME)
    AC_SUBST(KW_ENABLE)
  ]
)

AC_DEFUN([CONFIGURE_SHARED],
  [
    #
    # Set options for building a Shared Mili Libraries
    #
    CC_DEFINES_SHARED=" "
    
    AC_ARG_ENABLE([shared],
      [AC_HELP_STRING([--enable-shared],[Compile as a shared library])],
      [
        CC_DEFINES_SHARED=" -shared -fPIC "
        SHARED_ENABLE="True"
        AC_MSG_RESULT([---->Enabling: Shared])
      ],
      [
        SHARED_ENABLE="False"
      ]
    )
    
    #  SL Library Options
    AC_SUBST(CC_DEFINES_SHARED)
  ]
)

# End of acinclude.m4
