/* $Id: mc_silo_wrap.c,v 1.3 2011/05/26 21:43:13 durrenberger1 Exp $ */

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
#ifdef ENABLE_SILO

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

#define mc_open_                    MC_OPEN
#define mc_close_                   MC_CLOSE
#define mc_filelock_enable_	    MC_FILE_LOCK_ENABLE
#define mc_filelock_disable_	    MC_FILE_LOCK_DISABLE
#define mc_delete_family_           MC_DELETE_FAMILY
#define mc_partition_state_data_    MC_PARTITION_STATE_DATA
#define mc_wrt_scalar_              MC_WRT_SCALAR
#define mc_read_scalar_             MC_READ_SCALAR
#define mc_wrt_array_               MC_WRT_ARRAY
#define mc_wrt_string_              MC_WRT_STRING
#define mc_read_string_             MC_READ_STRING
#define mc_make_umesh_              MC_MAKE_UMESH
#define mc_def_class_               MC_DEF_CLASS
#define mc_def_class_idents_        MC_DEF_CLASS_IDENTS
#define mc_def_nodes_               MC_DEF_NODES
#define mc_def_conn_seq_            MC_DEF_CONN_SEQ
#define mc_def_conn_arb_            MC_DEF_CONN_ARB
#define mc_def_conn_surface_        MC_DEF_CONN_SURFACE
#define mc_load_nodes_              MC_LOAD_NODES
#define mc_load_conns_              MC_LOAD_CONNS
#define mc_load_surface_            MC_LOAD_SURFACE
#define mc_get_mesh_id_             MC_GET_MESH_ID
#define mc_def_svars_               MC_DEF_SVARS
#define mc_def_vec_svar_            MC_DEF_VEC_SVAR
#define mc_def_arr_svar_            MC_DEF_ARR_SVAR
#define mc_def_vec_arr_svar_        MC_DEF_VEC_ARR_SVAR
#define mc_open_srec_               MC_OPEN_SREC
#define mc_close_srec_              MC_CLOSE_SREC
#define mc_def_subrec_              MC_DEF_SUBREC
#define mc_flush_                   MC_FLUSH
#define mc_new_state_               MC_NEW_STATE
#define mc_restart_at_state_        MC_RESTART_AT_STATE
#define mc_restart_at_file_         MC_RESTART_AT_FILE
#define mc_wrt_stream_              MC_WRT_STREAM
#define mc_wrt_subrec_              MC_WRT_SUBREC
#define mc_read_results_            MC_READ_RESULTS
#define mc_limit_states_            MC_LIMIT_STATES
#define mc_limit_filesize_          MC_LIMIT_FILESIZE
#define mc_suffix_width_            MC_SUFFIX_WIDTH
#define mc_print_error_             MC_PRINT_ERROR
#define mc_query_family_            MC_QUERY_FAMILY
#define mc_get_class_info_          MC_GET_CLASS_INFO
#define mc_get_simple_class_info_   MC_GET_SIMPLE_CLASS_INFO
#define mc_set_buffer_qty_          MC_SET_BUFFER_QTY
#define mc_read_param_array_        MC_READ_PARAM_ARRAY

/************************************************************************
 *
 *   New TI functions - added October 2006 EMP
 *
 ************************************************************************
 */
#define mc_undef_ti_class_       MC_UNDEF_TI_CLASS
#define mc_def_ti_class_         MC_DEF_TI_CLASS
#define mc_set_ti_class_         MC_SET_TI_CLASS
#define mc_enable_ti_            MC_ENABLE_TI
#define mc_disable_ti_           MC_DISABLE_TI
#define mc_make_ti_var_name_     MC_MAKE_TI_VAR_NAME
#define mc_wrt_ti_scalar_        MC_WRT_TI_SCALER
#define mc_read_ti_scalar_       MC_READ_TI_SCALER
#define mc_wrt_ti_string_        MC_WRT_TI_STRING
#define mc_read_ti_string_       MC_READ_TI_STRING
#define mc_wrt_ti_array_         MC_WRT_TI_ARRAY
#define mc_read_ti_array_        MC_READ_TI_ARRAY
#define mc_ti_get_metadata_      MC_TI_GET_METADATA
#define mc_get_metadata_         MC_GET_METADATA
#define mc_ti_check_arch_        MC_TI_CHECK_ARCH


#endif

#ifdef __hpux

#define mc_open_                    MC_OPEN
#define mc_close_                   MC_CLOSE
#define mc_filelock_enable_	    MC_FILE_LOCK_ENABLE
#define mc_filelock_disable_	    MC_FILE_LOCK_DISABLE
#define mc_delete_family_           MC_DELETE_FAMILY
#define mc_partition_state_data_    MC_PARTITION_STATE_DATA
#define mc_wrt_scalar_              MC_WRT_SCALAR
#define mc_read_scalar_             MC_READ_SCALAR
#define mc_wrt_array_               MC_WRT_ARRAY
#define mc_wrt_string_              MC_WRT_STRING
#define mc_read_string_             MC_READ_STRING
#define mc_make_umesh_              MC_MAKE_UMESH
#define mc_def_class_               MC_DEF_CLASS
#define mc_def_class_idents_        MC_DEF_CLASS_IDENTS
#define mc_def_nodes_               MC_DEF_NODES
#define mc_def_conn_seq_            MC_DEF_CONN_SEQ
#define mc_def_conn_arb_            MC_DEF_CONN_ARB
#define mc_def_conn_surface_        MC_DEF_CONN_SURFACE
#define mc_load_nodes_              MC_LOAD_NODES
#define mc_load_conns_              MC_LOAD_CONNS
#define mc_load_surface_            MC_LOAD_SURFACE
#define mc_get_mesh_id_             MC_GET_MESH_ID
#define mc_def_svars_               MC_DEF_SVARS
#define mc_def_vec_svar_            MC_DEF_VEC_SVAR
#define mc_def_arr_svar_            MC_DEF_ARR_SVAR
#define mc_def_vec_arr_svar_        MC_DEF_VEC_ARR_SVAR
#define mc_open_srec_               MC_OPEN_SREC
#define mc_close_srec_              MC_CLOSE_SREC
#define mc_def_subrec_              MC_DEF_SUBREC
#define mc_flush_                   MC_FLUSH
#define mc_new_state_               MC_NEW_STATE
#define mc_restart_at_state_        MC_RESTART_AT_STATE
#define mc_restart_at_file_         MC_RESTART_AT_FILE
#define mc_wrt_stream_              MC_WRT_STREAM
#define mc_wrt_subrec_              MC_WRT_SUBREC
#define mc_read_results_            MC_READ_RESULTS
#define mc_limit_states_            MC_LIMIT_STATES
#define mc_limit_filesize_          MC_LIMIT_FILESIZE
#define mc_suffix_width_            MC_SUFFIX_WIDTH
#define mc_print_error_             MC_PRINT_ERROR
#define mc_query_family_            MC_QUERY_FAMILY
#define mc_get_class_info_          MC_GET_CLASS_INFO
#define mc_get_simple_class_info_   MC_GET_SIMPLE_CLASS_INFO
#define mc_set_buffer_qty_          MC_SET_BUFFER_QTY
#define mc_read_param_array_        MC_READ_PARAM_ARRAY

/************************************************************************
 *
 *   New TI functions - added October 2006 EMP
 *
 ************************************************************************
 */
#define mc_undef_ti_class_       MC_UNDEF_TI_CLASS
#define mc_def_ti_class_         MC_DEF_TI_CLASS
#define mc_set_ti_class_         MC_SET_TI_CLASS
#define mc_enable_ti_            RMC_ENABLE_TI
#define mc_disable_ti_           MC_DISABLE_TI
#define mc_make_ti_var_name_     MC_MAKE_TI_VAR_NAME
#define mc_wrt_ti_scalar_        MC_WRT_TI_SCALER
#define mc_read_ti_scalar_       MC_READ_TI_SCALER
#define mc_wrt_ti_string_        MC_WRT_TI_STRING
#define mc_read_ti_string_       MC_READ_TI_STRING
#define mc_wrt_ti_array_         MC_WRT_TI_ARRAY
#define mc_read_ti_array_        MC_READ_TI_ARRAY
#define mc_ti_get_metadata_      MC_TI_GET_METADATA
#define mc_get_metadata_         MC_GET_METADATA
#define mc_ti_check_arch_        MC_TI_CHECK_ARCH

#endif


#ifndef CHAR_CONV

#define CHAR_DESCR              char *
#define CHAR_CONV_F2C( x )      x
#define CHAR_CONV_C2F( x, l )   x
#define ASSIGN_STRING_IN( d, sc, sw ) {(d) = (sw);}
#define COPY_STRING_OUT( d, sc ) \
        {strncpy( CHAR_CONV_F2C( d ), sc, strlen( sc ) );}

#endif


#include "mili_internal.h"


int fortran_api = 0;




int
mc_open_( CHAR_DESCR param_root, CHAR_DESCR param_path,
          CHAR_DESCR param_mode, Famid *fam_id )
{
   char *c_param_root, *c_param_path, *c_param_mode;

   c_param_root = CHAR_CONV_F2C( param_root );
   c_param_path = CHAR_CONV_F2C( param_path );
   c_param_mode = CHAR_CONV_F2C( param_mode );

   return mc_silo_open( c_param_root, c_param_path, c_param_mode, fam_id );
}


int
mc_close_( Famid *fam_id )
{
   return mc_silo_close( *fam_id );
}


void
mc_filelock_enable_( void )
{
   mc_silo_filelock_enable( );
   return;
}


void
mc_filelock_disable_( void )
{
   mc_silo_filelock_disable( );
   return;
}



int
mc_delete_family_( CHAR_DESCR param_root, CHAR_DESCR param_path )
{
   char *c_param_root, *c_param_path;

   c_param_root = CHAR_CONV_F2C( param_root );
   c_param_path = CHAR_CONV_F2C( param_path );

   return mc_silo_delete_family( c_param_root, c_param_path );
}


int
mc_partition_state_data_( Famid *fam_id )
{
   return mc_silo_partition_state_data( *fam_id );
}


int
mc_wrt_scalar_( Famid *fam_id, int *type, CHAR_DESCR name, void *p_value )
{
   char *c_name;

   c_name = CHAR_CONV_F2C( name );

   return mc_silo_wrt_scalar( *fam_id, *type, c_name, p_value );
}


int
mc_read_scalar_( Famid *fam_id, CHAR_DESCR name, void *p_value )
{
   char *c_name;

   c_name = CHAR_CONV_F2C( name );

   return mc_silo_read_scalar( *fam_id, c_name, p_value );
}


int
mc_wrt_array_( Famid *fam_id, int *type, CHAR_DESCR name, int *rank,
               int *dimensions, void *data )
{
   char *c_name;

   c_name = CHAR_CONV_F2C( name );

   return mc_silo_wrt_array( *fam_id, *type, c_name, *rank, dimensions, data );
}


int
mc_wrt_string_( Famid *fam_id, CHAR_DESCR name, CHAR_DESCR value )
{
   char *c_name, *c_value;

   c_name = CHAR_CONV_F2C( name );
   c_value = CHAR_CONV_F2C( value );

   return mc_silo_wrt_string( *fam_id, c_name, c_value );
}


int
mc_read_string_( Famid *fam_id, CHAR_DESCR name, CHAR_DESCR value )
{
   char *c_name;
   static char c_value[M_MAX_STRING_LEN + 1];
   int rval;
   char *p_value;

   c_name = CHAR_CONV_F2C( name );

   ASSIGN_STRING_IN( p_value, c_value, value );

   rval = mc_silo_read_string( *fam_id, c_name, p_value );

   COPY_STRING_OUT( value, c_value );

   return rval;
}


int
mc_make_umesh_( Famid *fam_id, CHAR_DESCR mesh_name,  int *p_dim,
                int *p_mesh_id )
{
   char *c_mesh_name;

   c_mesh_name = CHAR_CONV_F2C( mesh_name );

   return mc_silo_make_umesh( *fam_id, c_mesh_name, *p_dim, p_mesh_id );
}


int
mc_def_class_( Famid *fam_id, int *mesh_id, int *superclass,
               CHAR_DESCR short_name, CHAR_DESCR long_name )
{
   char *c_short_name, *c_long_name;

   c_short_name = CHAR_CONV_F2C( short_name );
   c_long_name = CHAR_CONV_F2C( long_name );

   return mc_silo_def_class( *fam_id, *mesh_id, *superclass, c_short_name,
                             c_long_name );
}


int
mc_def_class_idents_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                      int *start, int *stop )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_silo_def_class_idents( *fam_id, *mesh_id, c_class_name, *start,
                                    *stop );
}


int
mc_def_nodes_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
               int *start_node, int *stop_node, float *coords )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_silo_def_nodes( *fam_id, *mesh_id, c_class_name, *start_node,
                             *stop_node, coords );
}


int
mc_def_conn_seq_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                  int *start_el, int *stop_el, int *data )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_silo_def_conn_seq( *fam_id, *mesh_id, c_class_name, *start_el,
                                *stop_el, data );
}


int
mc_def_conn_arb_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                  int *qty, int *elem_ids, int *data )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_silo_def_conn_arb( *fam_id, *mesh_id, c_class_name, *qty, elem_ids,
                                data );
}


int
mc_def_conn_surface_( Famid *fam_id, int *mesh_id, CHAR_DESCR short_name,
                      int *qty_of_facets, int *data, int *surface_id )
{
   char *c_short_name;

   c_short_name = CHAR_CONV_F2C( short_name );

   return mc_silo_def_conn_surface( *fam_id, *mesh_id, short_name, *qty_of_facets,
                                    data, surface_id );
}


int
mc_load_nodes_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                void *coords )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_silo_load_nodes( *fam_id, *mesh_id, c_class_name, coords );
}


int
mc_load_conns_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                int *conns, int *mats, int *parts )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_silo_load_conns( *fam_id, *mesh_id, c_class_name, conns, mats, parts );
}


int
mc_load_surface_( Famid *fam_id, int *mesh_id, int surf, CHAR_DESCR class_name,
                  int *conns )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_silo_load_surface( *fam_id, *mesh_id, surf, c_class_name, conns );
}


int
mc_get_mesh_id_( Famid *fam_id, CHAR_DESCR mesh_name, int *p_mesh_id )
{
   char *c_mesh_name;

   c_mesh_name = CHAR_CONV_F2C( mesh_name );

   return mc_silo_get_mesh_id( *fam_id, c_mesh_name, p_mesh_id );
}


int
mc_def_svars_( Famid *fam_id, int *qty_svars, CHAR_DESCR names,
               int *name_stride, CHAR_DESCR titles, int *title_stride,
               int types[] )
{
   char *c_names, *c_titles;

   c_names = CHAR_CONV_F2C( names );
   c_titles = CHAR_CONV_F2C( titles );

   return mc_silo_def_svars( *fam_id, *qty_svars, c_names, *name_stride,
                             c_titles, *title_stride, types );
}


int
mc_def_vec_svar_( Famid *fam_id, int *type, int *qty_fields, CHAR_DESCR name,
                  CHAR_DESCR title, CHAR_DESCR field_names, int *name_stride,
                  CHAR_DESCR field_titles, int *title_stride )
{
   char *c_name, *c_title, *c_field_names, *c_field_titles;
   char **field_names_array, **field_titles_array;
   int i;
   int qty, dtype;
   int n_stride, t_stride;
   int rval;

   c_name = CHAR_CONV_F2C( name );
   c_title = CHAR_CONV_F2C( title );
   c_field_names = CHAR_CONV_F2C( field_names );
   c_field_titles = CHAR_CONV_F2C( field_titles );
   dtype = *type;
   qty = *qty_fields;
   n_stride = *name_stride;
   t_stride = *title_stride;

   field_names_array = NEW_N( char *, qty, "Def vec svar name array" );
   field_titles_array = NEW_N( char *, qty, "Def vec svar title array" );
   for ( i = 0; i < qty; i++ )
   {
      str_dup_f2c( &field_names_array[i], c_field_names + i * n_stride,
                   n_stride );
      str_dup_f2c( &field_titles_array[i], c_field_titles + i * t_stride,
                   t_stride );
   }

   rval = mc_silo_def_vec_svar( *fam_id, dtype, qty, c_name, c_title,
                                field_names_array, field_titles_array );

   for ( i = 0; i < qty; i++ )
   {
      free( field_names_array[i] );
      free( field_titles_array[i] );
   }
   free( field_names_array );
   free( field_titles_array );

   return rval;
}


int
mc_def_arr_svar_( Famid *fam_id, int *rank, int dims[], CHAR_DESCR name,
                  CHAR_DESCR title, int *type )
{
   char *c_name, *c_title;

   c_name = CHAR_CONV_F2C( name );
   c_title = CHAR_CONV_F2C( title );

   return mc_silo_def_arr_svar( *fam_id, *rank, dims, c_name, c_title, *type );
}


int
mc_def_vec_arr_svar_( Famid *fam_id, int *rank, int dims[], CHAR_DESCR name,
                      CHAR_DESCR title, int *qty_fields, CHAR_DESCR field_names,
                      int *name_stride, CHAR_DESCR field_titles,
                      int *title_stride, int *type )
{
   char *c_name, *c_title, *c_field_names, *c_field_titles;
   char **field_names_array, **field_titles_array;
   int qty, i;
   int n_stride, t_stride;
   int rval;

   c_name = CHAR_CONV_F2C( name );
   c_title = CHAR_CONV_F2C( title );
   c_field_names = CHAR_CONV_F2C( field_names );
   c_field_titles = CHAR_CONV_F2C( field_titles );
   qty = *qty_fields;
   n_stride = *name_stride;
   t_stride = *title_stride;

   field_names_array = NEW_N( char *, qty, "Def vec arr svar name array" );
   field_titles_array = NEW_N( char *, qty, "Def vec arr svar title array" );
   for ( i = 0; i < qty; i++ )
   {
      str_dup_f2c( &field_names_array[i], c_field_names + i * n_stride,
                   n_stride );
      str_dup_f2c( &field_titles_array[i], c_field_titles + i * t_stride,
                   t_stride );
   }

   rval = mc_silo_def_vec_arr_svar( *fam_id, *rank, dims, c_name, c_title, qty,
                                    field_names_array, field_titles_array, *type );

   for ( i = 0; i < qty; i++ )
   {
      free( field_names_array[i] );
      free( field_titles_array[i] );
   }
   free( field_names_array );
   free( field_titles_array );

   return rval;
}


int
mc_open_srec_( Famid *fam_id, int *mesh_id, int *p_srec_id )
{
   return mc_silo_open_srec( *fam_id, *mesh_id, p_srec_id );
}


int
mc_close_srec_( Famid *fam_id, int *p_srec_id )
{
   return mc_silo_close_srec( *fam_id, *p_srec_id );
}


int
mc_def_subrec_( Famid *fam_id, int *srec_id, CHAR_DESCR subrec_name,
                int *data_org, int *qty_svars, CHAR_DESCR svar_names,
                int *name_stride, CHAR_DESCR subclass, int *format,
                int *qty, int *mo_ids, int *flag )
{
   int rval;
   char *c_subrec_name, *c_svar_names, *c_subclass;

   c_subrec_name = CHAR_CONV_F2C( subrec_name );
   c_svar_names = CHAR_CONV_F2C( svar_names );
   c_subclass = CHAR_CONV_F2C( subclass );

   fortran_api = 1;

   rval =  mc_silo_def_subrec( *fam_id, *srec_id, c_subrec_name, *data_org,
                               *qty_svars, c_svar_names, *name_stride,
                               c_subclass, *format, *qty, mo_ids, flag );

   fortran_api = 0;

   return rval;
}


int
mc_flush_( Famid *fam_id, int *data_type )
{
   return mc_silo_flush( *fam_id, *data_type );
}


int
mc_new_state_( Famid *fam_id, int *srec_id, float *time, int *p_file_suffix,
               int *p_file_state_index )
{
   return mc_silo_new_state( *fam_id, *srec_id, *time, p_file_suffix,
                             p_file_state_index );
}


int
mc_restart_at_state_( Famid *fam_id, int *p_file_suffix, int *p_state_index )
{
   return mc_silo_restart_at_state( *fam_id, *p_file_suffix, *p_state_index );
}


int
mc_restart_at_file_( Famid *fam_id, int *file_name_index )
{
   return mc_silo_restart_at_file( *fam_id, *file_name_index );
}


int
mc_wrt_stream_( Famid *fam_id, int *type, int *qty, void *data )
{
   return mc_silo_wrt_stream( *fam_id, *type, *qty, data );
}


int
mc_wrt_subrec_( Famid *fam_id, CHAR_DESCR subrec_name, int *p_start,
                int *p_stop, void *data )
{
   char *c_subrec_name;

   c_subrec_name = CHAR_CONV_F2C( subrec_name );

   return mc_silo_wrt_subrec( *fam_id, c_subrec_name, *p_start, *p_stop, data );
}


int
mc_read_results_( Famid *fam_id, int *p_state, int *p_subrec_id, int *p_qty,
                  CHAR_DESCR res_names, int *name_stride, void *p_data )
{
   char *c_res_names;
   char **specs, **c_specs;
   char *spec_buf, *p_c;
   int i, j;
   int qty;
   int stride;
   int rval;
   char *p_name;
   char *p_c_idx;
   char component[M_MAX_NAME_LEN + 1];
   int indices[M_MAX_ARRAY_DIMS];
   int index_qty;
   int spec_len;
   char *delimiters = ", ]";
   Bool_type subset;
   char *p_src;

   c_res_names = CHAR_CONV_F2C( res_names );
   qty = *p_qty;
   stride = *name_stride;

   /* Create C strings from FORTRAN strings. */
   specs = NEW_N( char *, qty, "Read results name array" );
   for ( i = 0, spec_len = 0; i < qty; i++ )
   {
      spec_len += str_dup_f2c( &specs[i], c_res_names + i * stride, stride );
   }

   /* Allocate space for results in C-API format. */
   c_specs = NEW_N( char *, qty, "Read results C-API name array" );
   spec_buf = NEW_N( char, spec_len + qty, "C-API result str buf" );
   p_c = spec_buf;

   /* Copy result names with subset specifications in C-API format. */
   for ( i = 0; i < qty; i++ )
   {
      /* Get the result short name. */
      p_name = strtok( specs[i], "[ " );

      index_qty = 0;
      component[0] = '\0';

      /* Parse to extract subset specification of aggregate svar. */
      p_c_idx = strtok( NULL, delimiters );
      while ( p_c_idx != NULL )
      {
         if ( is_numeric( p_c_idx ) )
         {
            indices[index_qty] = atoi( p_c_idx );
            index_qty++;
         }
         else /* Must be a vector component short name. */
         {
            strcpy( component, p_c_idx );

            /* Shouldn't be anything left after a component name. */
            if ( strtok( NULL, delimiters ) != NULL )
               return MALFORMED_SUBSET;
         }

         p_c_idx = strtok( NULL, delimiters );
      }

      /* Now copy the result to C-API buffer. */

      c_specs[i] = p_c;
      subset = index_qty > 0 || *component != '\0';

      /* Copy result short name. */
      for ( p_src = p_name; *p_src != '\0'; *p_c++ = *p_src++ );

      if ( subset )
      {
         /* Start subset text. */
         *p_c++ = '[';

         if ( index_qty > 0 )
         {
            /* Loop through indices in reverse order. */
            for ( j = index_qty - 1; j > -1; j-- )
            {
               /* Encode the index decremented by one for zero-base. */
               sprintf( p_c, "%d", indices[j] - 1 );

               /* Advance the destination pointer past the index text. */
               for ( ; *(++p_c); )

                  /* Append comma if more to come. */
                  if ( j > 0 || *component != '\0' )
                     *p_c++ = ',';
            }
         }

         if ( *component != '\0' )
         {
            /* Copy the component name. */
            for ( p_src = component; *p_src != '\0'; *p_c++ = *p_src++ );
         }

         /* Finish subset text. */
         *p_c++ = ']';
      }

      /* Terminate the current result specification string. */
      *p_c++ = '\0';
   }

   for ( i = 0; i < qty; i++ )
      free( specs[i] );
   free( specs );

   rval = mc_silo_read_results( *fam_id, *p_state, *p_subrec_id, qty, c_specs,
                                p_data );

   free( spec_buf );
   free( c_specs );

   return rval;
}


int
mc_limit_states_( Famid *fam_id, int *states_per_file )
{
   return mc_silo_limit_states( *fam_id, *states_per_file );
}


int
mc_limit_filesize_( Famid *fam_id, int *filesize, int *srec_id )
{
   return mc_silo_limit_filesize( *fam_id, *filesize, *srec_id );
}


int
mc_suffix_width_( Famid *fam_id, int *suffix_width )
{
   return mc_silo_suffix_width( *fam_id, *suffix_width );
}


void
mc_print_error_( CHAR_DESCR preamble, int *return_value )
{
   char *c_preamble;

   c_preamble = CHAR_CONV_F2C( preamble );

   mc_silo_print_error( c_preamble, *return_value );

   return;
}


int
mc_query_family_( Famid *fam_id, int *req_type, void *num_args,
                  CHAR_DESCR char_args, int *p_info )
{
   int rval;
   char *c_char_args;
   void *p_void;
   int rtype;
   static char char_info[M_MAX_STRING_LEN + 1];
   Bool_type string_output;

   c_char_args = CHAR_CONV_F2C( char_args );
   rtype = *req_type;

   string_output = FALSE;

   if ( rtype == SUBREC_CLASS
         || rtype == LIB_VERSION )
   {
      p_void = (void *) char_info;
      string_output = TRUE;
   }
   else
      p_void = (void *) p_info;

   rval = mc_silo_query_family( *fam_id, rtype, num_args, c_char_args, p_void );

   if ( string_output )
      COPY_STRING_OUT( (CHAR_DESCR) p_info, char_info );

   return rval;
}


int
mc_get_class_info_( Famid *famid, int *mesh_id, int *superclass,
                    int *class_index, CHAR_DESCR short_name,
                    CHAR_DESCR long_name, int *object_qty )
{
   static char c_short_name[M_MAX_NAME_LEN + 1];
   static char c_long_name[M_MAX_NAME_LEN + 1];
   int rval;
   char *p_short_name, *p_long_name;

   ASSIGN_STRING_IN( p_short_name, c_short_name, short_name )
   ASSIGN_STRING_IN( p_long_name, c_long_name, long_name )

   rval = mc_silo_get_class_info( *famid, *mesh_id, *superclass, *class_index,
                                  p_short_name, p_long_name, object_qty );

   COPY_STRING_OUT( short_name, c_short_name );
   COPY_STRING_OUT( long_name, c_long_name );

   return rval;
}


int
mc_get_simple_class_info_( Famid *famid, int *mesh_id, int *superclass,
                           int *class_index, CHAR_DESCR short_name,
                           CHAR_DESCR long_name, int *start_ident,
                           int *stop_ident )
{
   static char c_short_name[M_MAX_NAME_LEN + 1];
   static char c_long_name[M_MAX_NAME_LEN + 1];
   int rval;
   char *p_short_name, *p_long_name;

   ASSIGN_STRING_IN( p_short_name, c_short_name, short_name );
   ASSIGN_STRING_IN( p_long_name, c_long_name, long_name );

   rval = mc_silo_get_simple_class_info( *famid, *mesh_id, *superclass,
                                         *class_index, p_short_name, p_long_name,
                                         start_ident, stop_ident );

   COPY_STRING_OUT( short_name, c_short_name );
   COPY_STRING_OUT( long_name, c_long_name );

   return rval;
}


int
mc_set_buffer_qty_( Famid *famid, int *mesh_id, CHAR_DESCR class_name,
                    int *buf_qty )
{
   char *c_class_name;
   int rval;

   c_class_name = CHAR_CONV_F2C( class_name );

   rval = mc_silo_set_buffer_qty( *famid, *mesh_id, c_class_name, *buf_qty );

   return rval;
}


int
mc_read_param_array_( Famid *famid, char *name, void *value )
{
   return OK;
}
/*************************************************************************
 *
 *  New TI functions - added October 2006 EMP
 *
 ************************************************************************/
int
mc_undef_ti_class_( Famid *famid )
{
   return mc_silo_undef_ti_class( *famid );
}


int
mc_def_ti_class_( Famid *famid, int *state, int  *matid,
                  CHAR_DESCR superclass,
                  CHAR_DESCR short_name, CHAR_DESCR long_name )
{
   char *c_superclass, *c_short_name, *c_long_name;

   c_superclass = CHAR_CONV_F2C( superclass );
   c_short_name = CHAR_CONV_F2C( short_name );
   c_long_name = CHAR_CONV_F2C( long_name );


   return mc_silo_set_ti_class( *famid, *state, *matid, c_superclass,
                                c_short_name, c_long_name );
}


int
mc_set_ti_class_( Famid *famid, int *state, int *matid, CHAR_DESCR superclass,
                  CHAR_DESCR short_name, CHAR_DESCR long_name )
{
   char *c_superclass, *c_short_name, *c_long_name;

   c_superclass = CHAR_CONV_F2C( superclass );
   c_short_name = CHAR_CONV_F2C( short_name );
   c_long_name = CHAR_CONV_F2C( long_name );


   return mc_silo_set_ti_class( *famid, *state, *matid, c_superclass,
                                c_short_name, c_long_name );
}


void
mc_ti_enable_()
{
   mc_silo_ti_enable();
}


void
mc_ti_disable_()
{
   mc_silo_ti_disable();
}


void
mc_make_ti_var_name_( Famid *famid, CHAR_DESCR name, CHAR_DESCR new_name )
{
   char *c_name;
   char *p_name;
   static char c_value[M_MAX_STRING_LEN + 1];
   int rval;

   c_name = CHAR_CONV_F2C( name );

   ASSIGN_STRING_IN( p_name, c_name, new_name );

   mc_silo_make_ti_var_name( *famid, c_name, p_name );

   COPY_STRING_OUT( new_name, c_name );

   return;
}


int
mc_wrt_ti_scalar_( Famid *fam_id, int *type, CHAR_DESCR name, void *value )
{
   char *c_name;

   c_name = CHAR_CONV_F2C( name );

   return mc_silo_wrt_ti_scalar( *fam_id, *type, c_name, value );
}


int
mc_read_ti_scalar_( Famid *famid, CHAR_DESCR name, void *value )
{
   char *c_name;
   int rval;

   c_name = CHAR_CONV_F2C( name );

   rval = mc_silo_read_ti_scalar( *famid, c_name, value );

   return rval;
}


int
mc_wrt_ti_string_( Famid *fam_id, CHAR_DESCR name, CHAR_DESCR value )
{
   char *c_name, *c_value;

   c_name = CHAR_CONV_F2C( name );
   c_value = CHAR_CONV_F2C( value );

   return mc_silo_wrt_ti_string( *fam_id, c_name, c_value );
}


int
mc_read_ti_string_( Famid *famid, CHAR_DESCR name, CHAR_DESCR value )
{
   char *c_name;
   char *p_value;
   static char c_value[M_MAX_STRING_LEN + 1];
   int rval;

   c_name = CHAR_CONV_F2C( name );

   ASSIGN_STRING_IN( p_value, c_value, value );

   rval = mc_silo_read_ti_string( *famid, c_name, p_value );

   COPY_STRING_OUT( value, c_value );

   return rval;
}


int
mc_wrt_ti_array_( Famid *famid, int *type, CHAR_DESCR name,
                  int *order, int *dims, void *data )
{
   char *c_name;
   int rval;

   c_name = CHAR_CONV_F2C( name );

   rval = mc_silo_wrt_ti_array( *famid, *type, c_name, *order, dims, data );

   return rval;
}


int
mc_read_ti_array_( Famid *famid, CHAR_DESCR name, void *value )
{
   char *c_name;
   int rval;

   c_name = CHAR_CONV_F2C( name );

   return mc_silo_read_ti_array( *famid, c_name, value );

}

void
mc_ti_get_metadata_( Famid *fam_id, CHAR_DESCR mili_version, CHAR_DESCR host,
                     CHAR_DESCR arch,         CHAR_DESCR timestamp)
{
   char *c_mili_version, *c_host, *c_arch, *c_timestamp;

   c_mili_version = CHAR_CONV_F2C( mili_version );
   c_host         = CHAR_CONV_F2C( host );
   c_arch         = CHAR_CONV_F2C( arch );
   c_timestamp    = CHAR_CONV_F2C( timestamp );

   mc_silo_ti_get_metadata_( fam_id, c_mili_version, c_host, c_arch, c_timestamp);
}

void
mc_get_metadata_( Famid *fam_id, CHAR_DESCR mili_version, CHAR_DESCR host,
                  CHAR_DESCR arch,         CHAR_DESCR timestamp)
{
   char *c_mili_version, *c_host, *c_arch, *c_timestamp;

   c_mili_version = CHAR_CONV_F2C( mili_version );
   c_host         = CHAR_CONV_F2C( host );
   c_arch         = CHAR_CONV_F2C( arch );
   c_timestamp    = CHAR_CONV_F2C( timestamp );

   mc_silo_get_metadata_( fam_id, c_mili_version, c_host, c_arch, c_timestamp);
}

Return_value
mc_ti_check_arch_( Famid *fam_id )
{
   return mc_silo_ti_check_arch( fam_id );
}

#endif
