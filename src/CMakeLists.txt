#------------------------------------------------------------------------------
# Generate mili_config.h
#------------------------------------------------------------------------------

# we reconfigure this file at build time too (to resolve compile time and generator expressions), but we need to generate it here
# so we can add it to the list of files for the diablo_lib target and pickup dependencies correctly
set( MILI_CONFIG_TEMPLATE ${CMAKE_SOURCE_DIR}/src/mili_config.h.cmake.in )
set( MILI_CONFIG_H ${CMAKE_BINARY_DIR}/include/mili_config.h )

# ====================================================================================================================================
# WARNING: this file contains references to cmake variables, so if renaming any cmake vars, be sure to update them in this file as well
# ====================================================================================================================================
configure_file( "${MILI_CONFIG_TEMPLATE}" "${MILI_CONFIG_H}" )

#------------------------------------------------------------------------------
# Define source/header files
#------------------------------------------------------------------------------

# Source files for Mili Library
set(MILI_SOURCE_FILES
    ${MILI_SOURCE_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/dep.c
    ${CMAKE_CURRENT_LIST_DIR}/direc.c
    ${CMAKE_CURRENT_LIST_DIR}/eprtf.c
    ${CMAKE_CURRENT_LIST_DIR}/gahl.c
    ${CMAKE_CURRENT_LIST_DIR}/io_mem.c
    ${CMAKE_CURRENT_LIST_DIR}/makemili.c
    ${CMAKE_CURRENT_LIST_DIR}/mesh_u.c
    ${CMAKE_CURRENT_LIST_DIR}/mili.c
    ${CMAKE_CURRENT_LIST_DIR}/mr_funcs.c
    ${CMAKE_CURRENT_LIST_DIR}/param.c
    ${CMAKE_CURRENT_LIST_DIR}/parson.c
    ${CMAKE_CURRENT_LIST_DIR}/partition.c
    ${CMAKE_CURRENT_LIST_DIR}/read_db.c
    ${CMAKE_CURRENT_LIST_DIR}/sarray.c
    ${CMAKE_CURRENT_LIST_DIR}/srec.c
    ${CMAKE_CURRENT_LIST_DIR}/svar.c
    ${CMAKE_CURRENT_LIST_DIR}/ti.c
    ${CMAKE_CURRENT_LIST_DIR}/tidirc.c
    ${CMAKE_CURRENT_LIST_DIR}/util.c
    ${CMAKE_CURRENT_LIST_DIR}/wrap_c.c
    ${CMAKE_CURRENT_LIST_DIR}/write_db.c
    ${CMAKE_CURRENT_LIST_DIR}/f_util.f
    ${CMAKE_CURRENT_LIST_DIR}/wrap_f.F
)

# Header files for Mili Library
set(MILI_HEADER_FILES
    ${MILI_HEADER_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/eprtf.h
    ${CMAKE_CURRENT_LIST_DIR}/gahl.h
    ${CMAKE_CURRENT_LIST_DIR}/list.h
    ${CMAKE_CURRENT_LIST_DIR}/mili.h
    ${CMAKE_CURRENT_LIST_DIR}/mili_endian.h
    ${CMAKE_CURRENT_LIST_DIR}/mili_enum.h
    ${CMAKE_CURRENT_LIST_DIR}/mili_fparam.h
    ${CMAKE_CURRENT_LIST_DIR}/mili_internal.h
    ${CMAKE_CURRENT_LIST_DIR}/misc.h
    ${CMAKE_CURRENT_LIST_DIR}/mr.h
    ${CMAKE_CURRENT_LIST_DIR}/parson.h
    ${CMAKE_CURRENT_LIST_DIR}/partition.h
    ${CMAKE_CURRENT_LIST_DIR}/sarray.h
    ${CMAKE_BINARY_DIR}/include/mili_config.h
)

# Taurus Library Source/Headers
set(TAURUS_SOURCE_FILES
    ${TAURUS_SOURCE_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/taurus_db.c
    ${CMAKE_CURRENT_LIST_DIR}/taurus_mesh_u.c
    ${CMAKE_CURRENT_LIST_DIR}/taurus_svars.c
    ${CMAKE_CURRENT_LIST_DIR}/taurus_srec.c
    ${CMAKE_CURRENT_LIST_DIR}/taurus_util.c
)

set(TAURUS_HEADER_FILES
    ${TAURUS_HEADER_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/taurus_db.h
)

# Extended Printf library Source/Headers
set(EPRTF_SOURCE_FILES
    ${EPRTF_SOURCE_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/eprtf.c
)

set(EPRTF_HEADER_FILES
    ${EPRTF_HEADER_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/eprtf.h
)

##------------------------------------------------------------------------------
## Create include directory with all header files
##------------------------------------------------------------------------------
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include)

if( ENABLE_MILI )
    file(COPY ${MILI_HEADER_FILES} DESTINATION ${CMAKE_BINARY_DIR}/include)
endif()

if( ENABLE_TAURUS )
    file(COPY ${TAURUS_HEADER_FILES} DESTINATION ${CMAKE_BINARY_DIR}/include)
endif()

if( ENABLE_EPRINTF )
    file(COPY ${EPRTF_HEADER_FILES} DESTINATION ${CMAKE_BINARY_DIR}/include)
endif()

##------------------------------------------------------------------------------
## Add libraries as build targets
##------------------------------------------------------------------------------

# Create Mili Library
if( ENABLE_MILI )
    blt_add_library(
        NAME mili
        SOURCES ${MILI_SOURCE_FILES}
        HEADERS ${MILI_HEADER_FILES}
    )
    target_include_directories( mili PUBLIC ${CMAKE_BINARY_DIR}/include )
endif()

# Create Taurus Library
if( ENABLE_TAURUS )
    blt_add_library(
        NAME taurus
        SOURCES ${TAURUS_SOURCE_FILES}
        HEADERS ${TAURUS_HEADER_FILES}
    )
endif()

# Create eprtf Library
if( ENABLE_EPRINTF )
    blt_add_library(
        NAME eprtf
        SOURCES ${EPRTF_SOURCE_FILES}
        HEADERS ${EPRTF_HEADER_FILES}
    )
    # Current Mili build system doesn't have this and it breaks build, will fix later.
    #add_compile_definitions( HAVE_EPRINT )
endif()
