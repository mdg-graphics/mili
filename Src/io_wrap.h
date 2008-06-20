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

/* #define G_PARTICLE  M_PARTICLE */

#define QTY_SCLASS  M_QTY_SUPERCLASS

#define G_STRING    M_STRING
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
