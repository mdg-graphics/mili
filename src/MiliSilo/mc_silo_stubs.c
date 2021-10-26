/* $Id: mc_silo_stubs.c,v 1.3 2011/05/26 21:43:13 durrenberger1 Exp $ */

/*
 * MILI C wrappers for the FORTRAN API
 *
 * These routines
 *   - implement platform-dependent FORTRAN/C interface naming conventions;
 *   - provide character reference conversion under UNICOS;
 *   - de-reference pointers to numeric scalars for parameters
 *     which are pass-by-value in the C API routine.
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

#include <string.h>

#ifdef cray

#define CHAR_CONV
#include <fortran.h>
#define CHAR_DESCR              _fcd
#define CHAR_CONV_F2C( x )    _fcdtocp( x )
#define CHAR_CONV_C2F( x, l ) _cptofcd( x, l )
#define ASSIGN_STRING_IN( d, sc, sw ) {(d) = (sc);}
#define COPY_STRING_OUT( d, sc ) \
        {strncpy( CHAR_CONV_F2C( d ), sc, strlen( sc ) );}

#else
#define CHAR_DESCR              char *
#define CHAR_CONV_F2C( x )      x
#define CHAR_CONV_C2F( x, l )   x
#define ASSIGN_STRING_IN( d, sc, sw ) {(d) = (sw);}
#define COPY_STRING_OUT( d, sc ) \
        {strncpy( CHAR_CONV_F2C( d ), sc, strlen( sc ) );}

#endif


#include "mili_internal.h"


int fortran_api = 0;
#ifdef DONE
/*****
 ***** After the subroutine is written,
 ***** we no longer need the stub
 *****/
int
mc_silo_open( Famid *fam_id )
{
   return OK;
}

#endif

int
mc_silo_close( Famid *fam_id )
{
   return OK;
}


void
mc_silo_filelock_enable( void )
{
   return;
}


void
mc_silo_filelock_disable( void )
{
   return;
}


int
mc_silo_delete_family( CHAR_DESCR param_root, CHAR_DESCR param_path )
{
   return OK;
}


int
mc_silo_partition_state_data( Famid *fam_id )
{
   return OK;
}


int
mc_silo_wrt_scalar( Famid *fam_id, int *type, CHAR_DESCR name, void *p_value )
{
   return OK;
}


int
mc_silo_read_scalar( Famid *fam_id, CHAR_DESCR name, void *p_value )
{
   return OK;
}


int
mc_silo_wrt_array( Famid *fam_id, int *type, CHAR_DESCR name, int *rank,
                   int *dimensions, void *data )
{
   return OK;
}


int
mc_silo_wrt_string( Famid *fam_id, CHAR_DESCR name, CHAR_DESCR value )
{
   return OK;
}


int
mc_silo_read_string( Famid *fam_id, CHAR_DESCR name, CHAR_DESCR value )
{
   rval = OK;

   return rval;
}


int
mc_silo_def_conn_arb( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                      int *qty, int *elem_ids, int *data )
{
   return OK;
}


int
mc_silo_def_conn_surface( Famid *fam_id, int *mesh_id, CHAR_DESCR short_name,
                          int *qty_of_facets, int *data, int *surface_id )
{
   return OK;
}


int
mc_silo_load_nodes( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                    void *coords )
{
   return OK;
}


int
mc_silo_load_conns( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                    int *conns, int *mats, int *parts )
{
   return OK;
}


int
mc_silo_load_surface( Famid *fam_id, int *mesh_id, int surf, CHAR_DESCR class_name,
                      int *conns )
{
   return OK;
}


int
mc_silo_get_mesh_id( Famid *fam_id, CHAR_DESCR mesh_name, int *p_mesh_id )
{
   return OK;
}


int
mc_silo_def_svars( Famid *fam_id, int *qty_svars, CHAR_DESCR names,
                   int *name_stride, CHAR_DESCR titles, int *title_stride,
                   int types[] )
{
   return OK;
}


int
mc_silo_def_vec_svar( Famid *fam_id, int *type, int *qty_fields, CHAR_DESCR name,
                      CHAR_DESCR title, CHAR_DESCR field_names, int *name_stride,
                      CHAR_DESCR field_titles, int *title_stride )
{
   rval = OK;

   return rval;
}


int
mc_silo_def_arr_svar( Famid *fam_id, int *rank, int dims[], CHAR_DESCR name,
                      CHAR_DESCR title, int *type )
{
   return OK;
}


int
mc_silo_def_vec_arr_svar( Famid *fam_id, int *rank, int dims[], CHAR_DESCR name,
                          CHAR_DESCR title, int *qty_fields, CHAR_DESCR field_names,
                          int *name_stride, CHAR_DESCR field_titles,
                          int *title_stride, int *type )
{
   rval = OK;

   return rval;
}


int
mc_silo_open_srec( Famid *fam_id, int *mesh_id, int *p_srec_id )
{
   return OK;
}


int
mc_silo_close_srec( Famid *fam_id, int *p_srec_id )
{
   return OK;
}


int
mc_silo_def_subrec( Famid *fam_id, int *srec_id, CHAR_DESCR subrec_name,
                    int *data_org, int *qty_svars, CHAR_DESCR svar_names,
                    int *name_stride, CHAR_DESCR subclass, int *format,
                    int *qty, int *mo_ids, int *flag )
{
   int rval;

   rval =  OK;

   fortran_api = 0;

   return rval;
}


int
mc_silo_flush( Famid *fam_id, int *data_type )
{
   return OK;
}


int
mc_silo_new_state( Famid *fam_id, int *srec_id, float *time, int *p_file_suffix,
                   int *p_file_state_index )
{
   return OK;
}


int
mc_silo_restart_at_state( Famid *fam_id, int *p_file_suffix, int *p_state_index )
{
   return OK;
}


int
mc_silo_restart_at_file( Famid *fam_id, int *file_name_index )
{
   return OK;
}


int
mc_silo_wrt_stream( Famid *fam_id, int *type, int *qty, void *data )
{
   return OK;
}


int
mc_silo_wrt_subrec( Famid *fam_id, CHAR_DESCR subrec_name, int *p_start,
                    int *p_stop, void *data )
{
   return OK;
}


int
mc_silo_read_results( Famid *fam_id, int *p_state, int *p_subrec_id, int *p_qty,
                      CHAR_DESCR res_names, int *name_stride, void *p_data )
{
   int rval;

   rval = OK;

   return rval;
}


int
mc_silo_limit_states( Famid *fam_id, int *states_per_file )
{
   return OK;
}


int
mc_silo_limit_filesize( Famid *fam_id, int *filesize, int *srec_id )
{
   return OK;
}


int
mc_silo_suffix_width( Famid *fam_id, int *suffix_width )
{
   return OK;
}


void
mc_silo_print_error( CHAR_DESCR preamble, int *return_value )
{
   return;
}


int
mc_silo_query_family( Famid *fam_id, int *req_type, void *num_args,
                      CHAR_DESCR char_args, int *p_info )
{
   int rval;

   rval = OK;

   return rval;
}


int
mc_silo_set_buffer_qty( Famid *famid, int *mesh_id, CHAR_DESCR class_name,
                        int *buf_qty )
{
   int rval;

   rval = OK;

   return rval;
}


int
mc_silo_read_param_array( Famid *famid, char *name, void *value )
{
   return OK;
}

/*************************************************************************
 *
 *  New TI functions - added October 2006 EMP
 *
 ************************************************************************/
int
mc_undef_ti_class( Famid *famid )
{
   return OK;
}


int
mc_def_ti_class( Famid *famid, int *state, int  *matid,
                 CHAR_DESCR superclass,
                 CHAR_DESCR short_name, CHAR_DESCR long_name )
{
   return OK;
}


int
mc_set_ti_class( Famid *famid, int *state, int *matid, CHAR_DESCR superclass,
                 CHAR_DESCR short_name, CHAR_DESCR long_name )
{
   return OK;
}


void
mc_ti_enable()
{
   return;
}


void
mc_ti_disable()
{
   return;
}


void
mc_silo_make_ti_var_name( Famid *famid, CHAR_DESCR name, CHAR_DESCR new_name )
{
   return;
}


int
mc_silo_wrt_ti_scalar( Famid *fam_id, int *type, CHAR_DESCR name, void *value )
{
   return OK;
}


int
mc_silo_read_ti_scalar( Famid *famid, CHAR_DESCR name, void *value )
{
   int rval;

   rval = OK;

   return rval;
}


int
mc_silo_wrt_ti_string( Famid *fam_id, CHAR_DESCR name, CHAR_DESCR value )
{
   return OK;
}


int
mc_silo_read_ti_string( Famid *famid, CHAR_DESCR name, CHAR_DESCR value )
{
   int rval;

   rval = OK;

   return rval;
}


int
mc_silo_wrt_ti_array( Famid *famid, int *type, CHAR_DESCR name,
                      int *order, int *dims, void *data )
{
   int rval;

   rval = OK;

   return rval;
}


int
mc_silo_read_ti_array( Famid *famid, CHAR_DESCR name, void *value )
{
   int rval;

   rval = OK;

   return rval;
}

void
mc_silo_ti_get_metadata( Famid *fam_id, CHAR_DESCR mili_version, CHAR_DESCR host,
                         CHAR_DESCR arch,         CHAR_DESCR timestamp)
{
   return;
}

void
mc_silo_get_metadata( Famid *fam_id, CHAR_DESCR mili_version, CHAR_DESCR host,
                      CHAR_DESCR arch,         CHAR_DESCR timestamp)
{
   return;
}

Return_value
mc_silo_ti_check_arch( Famid *fam_id )
{
   return OK;
}
