#------------------------------------------------------------------------------
# Host-config file for the blueos 3 machines at LLNL
#------------------------------------------------------------------------------
#
# This file provides CMake with paths / details for:
#  C/Fortran:   NVHPC 23.11
#
#------------------------------------------------------------------------------

#---------------------------------------
# Compilers
#---------------------------------------

set( MILI_SYS_TYPE blueos_3_ppc64le_ib_p9 CACHE STRING "" )
set( MILI_COMPILER_FAMILY nvhpc  CACHE STRING "" )

set( MILI_COMPILER_VERSION 23.11 )

set( MILI_COMPILER_NAME ${MILI_COMPILER_FAMILY}-${MILI_COMPILER_VERSION} CACHE STRING "" )

if( DEFINED ENV{SYS_TYPE} )
  set( ENV_SYS_TYPE $ENV{SYS_TYPE} )
  if( NOT "${ENV_SYS_TYPE}" STREQUAL "${MILI_SYS_TYPE}" )
    message( WARNING "SYS_TYPE environment variable set as '${ENV_SYS_TYPE}' for host-config file desgigned for '${MILI_SYS_TYPE}', attempting to config with '${ENV_SYS_TYPE}'!")
  endif( )
  set( MILI_SYS_TYPE ${ENV_SYS_TYPE} )
endif( )

set( MILI_COMPILER_PREFIX "/usr/tce/packages/${MILI_COMPILER_FAMILY}/${MILI_COMPILER_NAME}/bin" CACHE PATH "" )

set( CMAKE_C_COMPILER "${MILI_COMPILER_PREFIX}/nvc" CACHE PATH "" )
set( CMAKE_Fortran_COMPILER "${MILI_COMPILER_PREFIX}/nvfortran" CACHE PATH "" )

set( ENABLE_MILI TRUE CACHE BOOL "Turn on/off building of Mili Library" )
set( ENABLE_TAURUS TRUE CACHE BOOL "Turn on/off building of Taurus Library" )
set( ENABLE_EPRINTF TRUE CACHE BOOL "Turn on/off building of Extended printf Library" )
set( ENABLE_XMILICS TRUE CACHE BOOL "Turn on/off building of Xmilics" )
set( ENABLE_UTILITIES TRUE CACHE BOOL "Turn on/off building of Mili Utilities (md, tipart, MiliReader, ti_strings, makemili_driver)" )