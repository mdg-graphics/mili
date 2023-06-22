#------------------------------------------------------------------------------
# Define source/header files
#------------------------------------------------------------------------------

# Source files for xmilics
set(XMILICS_SOURCE_FILES
    ${XMILICS_SOURCE_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/combine_db.c
    ${CMAKE_CURRENT_LIST_DIR}/driver.c
    ${CMAKE_CURRENT_LIST_DIR}/init_io.c
    ${CMAKE_CURRENT_LIST_DIR}/io_funcs.c
    ${CMAKE_CURRENT_LIST_DIR}/misc.c
    ${CMAKE_CURRENT_LIST_DIR}/process_ti.c
    ${CMAKE_CURRENT_LIST_DIR}/versioninfo.c
)

# Header files for xmilics
set(XMILICS_HEADER_FILES
    ${XMILICS_HEADER_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/driver.h
)

# Source/Header files for md
set( MD_SOURCE_FILES ${MD_SOURCE_FILES} ${CMAKE_CURRENT_LIST_DIR}/md.c )

# Source/Header files for tipart
set( TIPART_SOURCE_FILES ${TIPART_SOURCE_FILES} ${CMAKE_CURRENT_LIST_DIR}/tipart.c )

# Source/Header files for MiliReader
set( MILIREADER_SOURCE_FILES ${MILIREADER_SOURCE_FILES} ${CMAKE_CURRENT_LIST_DIR}/MiliReader.c )

# Source/Header files for ti_strings
set( TI_STRINGS_SOURCE_FILES ${TI_STRINGS_SOURCE_FILES} ${CMAKE_CURRENT_LIST_DIR}/ti_strings.c )

# Source/Header files for makemili_driver
set( MAKEMILI_DRIVER_SOURCE_FILES ${MAKEMILI_DRIVER_SOURCE_FILES} ${CMAKE_CURRENT_LIST_DIR}/makemili_driver.c )

##------------------------------------------------------------------------------
## Add Utilities as build targets
##------------------------------------------------------------------------------

# Xmilics
if( ENABLE_XMILICS )
    blt_add_executable(
        NAME xmilics
        SOURCES ${XMILICS_SOURCE_FILES}
        HEADERS ${XMILICS_HEADER_FILES}
        DEPENDS_ON mili taurus
    )
    target_include_directories( xmilics PUBLIC ${CMAKE_BINARY_DIR}/include )
endif()

# Utilities
if( ENABLE_UTILITIES )
    # md
    blt_add_executable(
        NAME md
        SOURCES ${MD_SOURCE_FILES}
        DEPENDS_ON mili
    )

    # tipart
    blt_add_executable(
        NAME tipart
        SOURCES ${TIPART_SOURCE_FILES}
        DEPENDS_ON mili
    )

    # MiliReader
    blt_add_executable(
        NAME MiliReader
        SOURCES ${MILIREADER_SOURCE_FILES}
        DEPENDS_ON mili
    )

    # ti_strings
    blt_add_executable(
        NAME ti_strings
        SOURCES ${TI_STRINGS_SOURCE_FILES}
        DEPENDS_ON mili
    )

    # makemili_driver
    blt_add_executable(
        NAME makemili_driver
        SOURCES ${MAKEMILI_DRIVER_SOURCE_FILES}
        DEPENDS_ON mili
    )

endif()

##------------------------------------------------------------------------------
## Set up Code Style checking using clang-format
##------------------------------------------------------------------------------

if( CLANGFORMAT_FOUND )

    blt_add_code_checks(
        PREFIX util_format_checks
        SOURCES ${XMILICS_SOURCE_FILES} ${XMILICS_HEADER_FILES} ${MD_SOURCE_FILES} ${TIPART_SOURCE_FILES}
                ${MILIREADER_SOURCE_FILES} ${TI_STRINGS_SOURCE_FILES} ${MAKEMILI_DRIVER_SOURCE_FILES}
        CLANGFORMAT_CFG_FILE ${PROJECT_SOURCE_DIR}/scripts/style/clang-format.yaml
    )

endif()