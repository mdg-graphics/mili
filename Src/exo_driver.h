/* $Id$ */

#ifndef EXO_DRIVER_H
#define EXO_DRIVER_H


/************************************************************
 * TAG( Elem_block_data )
 *
 * Information useful to help map ExodusII element blocks
 * into the Mili/Griz4 data model of mesh object classes and
 * state record formats.
 */
typedef struct _Elem_block_data
{
    int block_id;
    int block_size;
    int class_id_base;
    MO_class_data *p_class;
} Elem_block_data;


/*
 * Some return values defined for Mili that we need for duplicate
 * functionality in the Exodus driver.
 */
#define NO_MESH 125
#define INVALID_INDEX 166
#define INVALID_SREC_INDEX 167
#define INVALID_SUBREC_INDEX 168
#define INVALID_STATE 170
#define INVALID_TIME 172
#define UNKNOWN_QUERY_TYPE 176

#endif
