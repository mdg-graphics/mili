/* $Id$ */
/*
 * io_wrap.h - Declarations for I/O wrapper routines which are
 *             dependent upon Mili declarations.
 *
 *      Douglas E. Speck
 *      Methods Development Group
 *      Lawrence Livermore National Laboratory
 *      14 Feb 1997
 *
 ************************************************************************
 * Modifications:
 *
 *  I. R. Corey - Dec 28, 2009: Fixed several problems releated to drawing
 *                ML particles.
 *                See SRC#648
 ************************************************************************
 */
 
#ifndef IO_WRAP
#define IO_WRAP

#include "mili.h"

#define G_UNIT      M_UNIT
#define G_NODE      M_NODE
#define G_TRUSS     M_TRUSS
#define G_BEAM      M_BEAM
#define G_TRI       M_TRI
#define G_QUAD      M_QUAD
#define G_TET       M_TET
#define G_PYRAMID   M_PYRAMID
#define G_WEDGE     M_WEDGE
#define G_HEX       M_HEX
#define G_MAT       M_MAT
#define G_MESH      M_MESH
#define G_SURFACE   M_SURFACE
#define G_PARTICLE  M_PARTICLE

#define QTY_SCLASS  M_QTY_SUPERCLASS

#define G_STRING    M_STRING
#define G_CHAR      M_STRING*100
#define G_FLOAT     M_FLOAT
#define G_FLOAT4    M_FLOAT4
#define G_FLOAT8    M_FLOAT8
#define G_INT       M_INT
#define G_INT4      M_INT4
#define G_INT8      M_INT8

#define G_MAX_ARRAY_DIMS        M_MAX_ARRAY_DIMS
#define G_MAX_NAME_LEN          M_MAX_NAME_LEN
#define G_MAX_STRING_LEN        M_MAX_STRING_LEN

#define QRY_QTY_STATES          QTY_STATES
#define QRY_QTY_DIMENSIONS      QTY_DIMENSIONS
#define QRY_QTY_MESHES          QTY_MESHES
#define QRY_QTY_SREC_FMTS       QTY_SREC_FMTS
#define QRY_QTY_SUBRECS         QTY_SUBRECS
#define QRY_QTY_SUBREC_SVARS    QTY_SUBREC_SVARS
#define QRY_QTY_SVARS           QTY_SVARS
#define QRY_QTY_NODE_BLKS       QTY_NODE_BLKS
#define QRY_QTY_NODES_IN_BLK    QTY_NODES_IN_BLK
#define QRY_QTY_CLASS_IN_SCLASS QTY_CLASS_IN_SCLASS
#define QRY_QTY_ELEM_CONN_DEFS  QTY_ELEM_CONN_DEFS
#define QRY_QTY_ELEMS_IN_DEF    QTY_ELEMS_IN_DEF
#define QRY_SREC_FMT_ID         SREC_FMT_ID
#define QRY_SERIES_SREC_FMTS    SERIES_SREC_FMTS
#define QRY_SUBREC_CLASS        SUBREC_CLASS
#define QRY_SREC_MESH           SREC_MESH
#define QRY_CLASS_SUPERCLASS    CLASS_SUPERCLASS
#define QRY_STATE_TIME          STATE_TIME
#define QRY_SERIES_TIMES        SERIES_TIMES
#define QRY_MULTIPLE_TIMES      MULTIPLE_TIMES
#define QRY_STATE_OF_TIME       STATE_OF_TIME
#define QRY_CLASS_EXISTS        CLASS_EXISTS
#define QRY_LIB_VERSION         LIB_VERSION
#define QRY_STATE_SIZE          STATE_SIZE

#endif
