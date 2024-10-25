cmake_minimum_required( VERSION 3.18 )

set( MILI_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE PATH "" )
set( MILI_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "" )
set( MILI_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "" )
set( MILI_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" CACHE PATH "" )

project( MILI LANGUAGES Fortran C )

set( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH} )

#------------------------------------------------------------------------------
# Setup BLT
#------------------------------------------------------------------------------

# set blt options prior to inclusion
set( ENABLE_ALL_WARNINGS OFF CACHE BOOL "Disable -Wall -Wextra" )
set( ENABLE_WARNINGS_AS_ERRORS OFF CACHE BOOL "Disable -Werror" )
set( ENABLE_FRUIT OFF CACHE BOOL "" )
set( ENABLE_GTEST OFF CACHE BOOL "" )
set( ENABLE_DOXYGEN OFF CACHE BOOL "" )
set( ENABLE_SPHINX OFF CACHE BOOL "" )
set( ENABLE_ASTYLE OFF CACHE BOOL "" )
# set( ENABLE_CLANGFORMAT OFF CACHE BOOL "" )
set( ENABLE_CLANGQUERY OFF CACHE BOOL "" )
set( ENABLE_CLANGTIDY OFF CACHE BOOL "" )
set( ENABLE_CPPCHECK OFF CACHE BOOL "" )
set( ENABLE_UNCRUSTIFY OFF CACHE BOOL "" )
set( ENABLE_YAPF OFF CACHE BOOL "" )
set( ENABLE_CMAKEFORMAT OFF CACHE BOOL "" )

include( cmake/blt/SetupBLT.cmake )

#------------------------------------------------------------------------------
# Check build type and set compiler debug/optimization flags
#------------------------------------------------------------------------------

set( CMAKE_POSITION_INDEPENDENT_CODE )

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set( CMAKE_C_FLAGS_DEBUG " -g" )
    set( CMAKE_Fortran_FLAGS_DEBUG " -g" )
elseif (CMAKE_BUILD_TYPE STREQUAL Release)
    set( CMAKE_C_FLAGS_RELEASE " -O3" )
    set( CMAKE_Fortran_FLAGS_RELEASE " -O3" )
elseif (CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
    set( CMAKE_C_FLAGS_RELWITHDEBINFO " -g -O3" )
    set( CMAKE_Fortran_FLAGS_RELWITHDEBINFO " -g -O3" )
elseif (CMAKE_BUILD_TYPE STREQUAL RelCleanup)
    set( CMAKE_C_FLAGS_RELCLEANUP " -g -O3 -Wunused-variable -Wunused-function" )
    set( CMAKE_Fortran_FLAGS_RELCLEANUP " -g -O3" )
endif()

#------------------------------------------------------------------------------
# Check for size of long
#------------------------------------------------------------------------------
include(CheckTypeSize)
check_type_size( "long" LONG_SIZE )
if( ${LONG_SIZE} EQUAL 8 )
    add_compile_definitions( HAVEINT8 )
endif()

#------------------------------------------------------------------------------
# Output build compile flags
#------------------------------------------------------------------------------
string( TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_UPPER_BUILD_TYPE )
set( CMAKE_UPPER_BUILD_TYPE ${CMAKE_UPPER_BUILD_TYPE} CACHE STRING "" )
message( STATUS "CMAKE_C_FLAGS_${CMAKE_UPPER_BUILD_TYPE}: \"${CMAKE_C_FLAGS_${CMAKE_UPPER_BUILD_TYPE}}\"" )
message( STATUS "CMAKE_CXX_FLAGS_${CMAKE_UPPER_BUILD_TYPE}: \"${CMAKE_CXX_FLAGS_${CMAKE_UPPER_BUILD_TYPE}}\"" )
message( STATUS "CMAKE_Fortran_FLAGS_${CMAKE_UPPER_BUILD_TYPE}: \"${CMAKE_Fortran_FLAGS_${CMAKE_UPPER_BUILD_TYPE}}\"" )


#------------------------------------------------------------------------------
# Setup Various Project Variables needed for mili_config.h and xmilics_config.h
#------------------------------------------------------------------------------

# Get user
set( BUILD_DEVELOPER $ENV{USER} )
message( STATUS "Build developer: ${BUILD_DEVELOPER}")

# Get timestamp for build
string( TIMESTAMP BUILD_TIMESTAMP )
message( STATUS "Build timestamp: ${BUILD_TIMESTAMP}")

# Get System information
cmake_host_system_information( RESULT BI_SYSTEM QUERY OS_NAME )
cmake_host_system_information( RESULT BI_RELEASE QUERY OS_RELEASE )
cmake_host_system_information( RESULT BI_VERSION QUERY OS_VERSION )
cmake_host_system_information( RESULT BI_PLATFORM QUERY OS_PLATFORM )
set( SYSTEM_INFO_STRING "${BI_SYSTEM} ${BI_RELEASE} ${BI_VERSION} ${BI_PLATFORM}" )
message( STATUS "System info: ${SYSTEM_INFO_STRING}")

# Build version
set( major_version 24 )
set( minor_version 01 )
set( patch_version 00 )
string( CONCAT PACKAGE_VERSION "${major_version}_${minor_version}_${patch_version}" )

# Package name and developer
set( PACKAGE_NAME "Mili")
set( PACKAGE_BUGREPORT "durrenberger1@llnl.gov" )
string( CONCAT PACKAGE_STRING "${PACKAGE_NAME} V${PACKAGE_VERSION}")

# Get git commit hash
blt_git_hashcode( HASHCODE MILI_GIT_COMMIT_SHA1 RETURN_CODE GIT_CALL_SUCCESS )
if( NOT ${GIT_CALL_SUCCESS} EQUAL 0 )
    set( MILI_GIT_COMMIT_SHA1 "NULL")
endif()
message( STATUS "Commit sha1(${GIT_CALL_SUCCESS}): ${MILI_GIT_COMMIT_SHA1}")

# Get Datetime from git commit hash
set( PACKAGE_DATETIME "NULL" )
blt_git(
    SOURCE_DIR ${CMAKE_SOURCE_DIR}
    GIT_COMMAND show -s --format=%ci ${MILI_GIT_COMMIT_SHA1}
    OUTPUT_VARIABLE PACKAGE_DATETIME
    RETURN_CODE GIT_CALL_SUCCESS
)
if( NOT ${GIT_CALL_SUCCESS} EQUAL 0 )
    set( PACKAGE_DATETIME "NULL" )
endif()
message( STATUS "Commit date(${GIT_CALL_SUCCES}): ${PACKAGE_DATETIME}")

# Split date and time into components
set( PACKAGE_DATE "NULL" )
set( PACKAGE_TIME "NULL" )
if( NOT ${PACKAGE_DATETIME} STREQUAL "NULL" )
    set( datetime_components ${PACKAGE_DATETIME})
    separate_arguments( datetime_components )
    list(GET datetime_components 0 PACKAGE_DATE )
    list(GET datetime_components 1 PACKAGE_TIME )
endif()

##------------------------------------------------------------------------------
## Set up Variables defining config, make, compile, and link commands
##------------------------------------------------------------------------------
set( CONFIG_CMD ${MILI_CONFIG_CMD} )
set( MAKE_CMD "make -j" )
get_property( mili_compile_definitions DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY COMPILE_DEFINITIONS )
set( COMPILE_CMD "${CMAKE_C_COMPILER}" )
foreach( def ${mili_compile_definitions} )
    set( COMPILE_CMD "${COMPILE_CMD} -D${def}")
endforeach()
set( COMPILE_CMD "${COMPILE_CMD} ${CMAKE_C_FLAGS_${CMAKE_UPPER_BUILD_TYPE}} ${ADDITIONAL_C_COMPILE_FLAGS}" )
set( LINK_CMD " " )

##------------------------------------------------------------------------------
## Set up target and build commands for Libraries
##------------------------------------------------------------------------------
add_subdirectory(src)

##------------------------------------------------------------------------------
## Set up target and build commands for Utilities
##------------------------------------------------------------------------------
add_subdirectory(utils)

