#------------------------------------------------------------------------------
# Host-config file for XYZ
#------------------------------------------------------------------------------
#
# This file provides CMake with paths / details for:
#  C/Fortran:   XYZ
#
#------------------------------------------------------------------------------

#---------------------------------------
# Compilers
#---------------------------------------

# These specify the individual compilers for C and Fortran.
set( CMAKE_C_COMPILER "<PLACEHOLDER>" CACHE PATH "Path to C compiler" )
set( CMAKE_Fortran_COMPILER "<PLACEHOLDER>" CACHE PATH "Path to Fortran compiler" )

# Below are Flags to specify what libraries and executables are built
set( ENABLE_MILI TRUE CACHE BOOL "Turn on/off building of Mili Library" )
set( ENABLE_TAURUS TRUE CACHE BOOL "Turn on/off building of Taurus Library" )
set( ENABLE_EPRINTF TRUE CACHE BOOL "Turn on/off building of Extended printf Library" )
set( ENABLE_XMILICS TRUE CACHE BOOL "Turn on/off building of Xmilics" )
set( ENABLE_UTILITIES TRUE CACHE BOOL "Turn on/off building of Mili Utilities (md, tipart, MiliReader, ti_strings, makemili_driver)" )