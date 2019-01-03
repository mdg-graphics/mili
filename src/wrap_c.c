/*
 Copyright (c) 2016, Lawrence Livermore National Security, LLC. 
 Produced at the Lawrence Livermore National Laboratory. Written 
 by Kevin Durrenberger: durrenberger1@llnl.gov. CODE-OCEC-16-056. 
 All rights reserved.

 This file is part of Mili. For details, see <URL describing code 
 and how to download source>.

 Please also read this link-- Our Notice and GNU Lesser General 
 Public License.

 This program is free software; you can redistribute it and/or modify 
 it under the terms of the GNU General Public License (as published by 
 the Free Software Foundation) version 2.1 dated February 1999.

 This program is distributed in the hope that it will be useful, but 
 WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF 
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms 
 and conditions of the GNU General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License 
 along with this program; if not, write to the Free Software Foundation, 
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 
 * MILI C wrappers for the FORTRAN API
 *
 * These routines
 *   - implement platform-dependent FORTRAN/C interface naming conventions;
 *   - provide character reference conversion under UNICOS;
 *   - de-reference pointers to numeric scalars for parameters
 *     which are pass-by-value in the C API routine.
 *
 ************************************************************************
 * Modifications:
 *
 *
 *  I. R. Corey - August 16, 2007: Added field for TI vars to note if
 *                nodal or element result.
 *
 *  I. R. Corey - April 30, 2010: Incorporate changes from Sebastian
 *                at IABG to support building under Windows.
 *                SCR #678
 *
 ************************************************************************
 */
#ifndef SILO_ENABLED

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
#define mc_end_state_               MC_END_STATE
#define mc_open_                    MC_OPEN
#define mc_close_                   MC_CLOSE
#define mc_filelock_enable_         MC_FILE_LOCK_ENABLE
#define mc_filelock_disable_        MC_FILE_LOCK_DISABLE
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
#define mc_def_node_labels_         MC_DEF_NODE_LABELS
#define mc_def_conn_seq_            MC_DEF_CONN_SEQ
#define mc_def_seq_labels_          MC_DEF_SEQ_LABELS
#define mc_def_conn_labels_         MC_DEF_CONN_LABELS
#define mc_def_conn_arb_            MC_DEF_CONN_ARB
#define mc_def_conn_arb_labels_     MC_DEF_CONN_ARB_LABELS
#define mc_def_conn_surface_        MC_DEF_CONN_SURFACE
#define mc_load_nodes_              MC_LOAD_NODES
#define mc_load_conns_              MC_LOAD_CONNS
#define mc_load_surface_            MC_LOAD_SURFACE
#define mc_load_conn_labels_        MC_LOAD_CONN_LABELS
#define mc_load_node_labels_        MC_LOAD_NODE_LABELS
#define mc_get_node_label_info_     MC_GET_NODE_LABEL_INFO
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
#define mc_get_svar_size_           MC_GET_SVAR_SIZE
#define mc_get_svar_mo_ids_on_class_ MC_GET_SVAR_MO_IDS_ON_CLASS
#define mc_get_svar_on_class_       MC_GET_SVAR_ON_CLASS
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
#define mc_ti_undefclass_          MC_TI_UNDEF_CLASS
#define mc_ti_def_class_           MC_TI_DEF_CLASS
#define mc_ti_set_class_           MC_TI_SET_CLASS
#define mc_ti_get_class_           MC_TI_GET_CLASS
#define mc_ti_get_data_attribites_ MC_TI_GET_DATA_ATTRIBUTES
#define mc_ti_enable_              MC_TI_ENABLE
#define mc_ti_disable              MC_TI_DISABLE
#define mc_ti_make_var_name_       MC_TI_MAKE_VAR_NAME
#define mc_ti_wrt_scalar_          MC_TI_WRT_SCALAR
#define mc_ti_read_scalar_         MC_TI_READ_SCALAR
#define mc_ti_wrt_string_          MC_TI_WRT_STRING
#define mc_ti_read_string_         MC_TI_READ_STRING
#define mc_ti_wrt_array_           MC_TI_WRT_ARRAY
#define mc_ti_read_array_          MC_TI_READ_ARRAY
#define mc_ti_get_metadata_        MC_TI_GET_METADATA
#define mc_ti_check_arch_          MC_TI_CHECK_ARCH
#endif


#ifdef __hpux

#define mc_end_state_               MC_END_STATE
#define mc_open_                    MC_OPEN
#define mc_close_                   MC_CLOSE
#define mc_filelock_enable_         MC_FILE_LOCK_ENABLE
#define mc_filelock_disable_        MC_FILE_LOCK_DISABLE
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
#define mc_def_conn_seq_labels_     MC_DEF_CONN_SEQ_LABELS
#define mc_def_conn_labels_         MC_DEF_CONN_LABELS
#define mc_def_conn_arb_            MC_DEF_CONN_ARB
#define mc_def_conn_arb_labels_     MC_DEF_CONN_ARB_LABELS
#define mc_def_conn_surface_        MC_DEF_CONN_SURFACE
#define mc_load_nodes_              MC_LOAD_NODES
#define mc_load_conns_              MC_LOAD_CONNS
#define mc_load_surface_            MC_LOAD_SURFACE
#define mc_load_conn_labels_        MC_LOAD_CONN_LABELS
#define mc_load_node_labels_        MC_LOAD_NODE_LABELS
#define mc_get_node_label_info_     MC_GET_NODE_LABEL_INFO
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
#define mc_get_svar_size_           MC_GET_SVAR_SIZE
#define mc_get_svar_mo_ids_on_class_ MC_GET_SVAR_MO_IDS_ON_CLASS
#define mc_get_svar_on_class_       MC_GET_SVAR_ON_CLASS
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
#define mc_ti_undefclass_          MC_TI_UNDEF_CLASS
#define mc_ti_def_class_           MC_TI_DEF_CLASS
#define mc_ti_set_class_           MC_TI_SET_CLASS
#define mc_ti_get_class_           MC_TI_GET_CLASS
#define mc_ti_get_data_attribites_ MC_TI_GET_DATA_ATTRIBUTES
#define mc_ti_enable_              MC_TI_ENABLE
#define mc_ti_disable              MC_TI_DISABLE
#define mc_ti_make_var_name_       MC_TI_MAKE_VAR_NAME
#define mc_ti_wrt_scalar_          MC_TI_WRT_SCALAR
#define mc_ti_read_scalar_         MC_TI_READ_SCALAR
#define mc_ti_wrt_string_          MC_TI_WRT_STRING
#define mc_ti_read_string_         MC_TI_READ_STRING
#define mc_ti_wrt_array_           MC_TI_WRT_ARRAY
#define mc_ti_read_array_          MC_TI_READ_ARRAY
#define mc_ti_get_metadata_        MC_TI_GET_METADATA
#define mc_ti_check_arch_          MC_TI_CHECK_ARCH

#endif


#ifdef Linux

#define mc_end_state_               MC_END_STATE
#define mc_open_                    MC_OPEN
#define mc_close_                   MC_CLOSE
#define mc_filelock_enable_         MC_FILE_LOCK_ENABLE
#define mc_filelock_disable_        MC_FILE_LOCK_DISABLE
#define mc_delete_family_           MC_DELETE_FAMILY
#define mc_partition_state_data_    MC_PARTITION_STATE_DATA
#define mc_wrt_scalar_              MC_WRT_SCALAR
#define mc_read_scalar_             MC_READ_SCALAR
#define mc_wrt_array_               MC_WRT_ARRAY
#define mc_wrt_string_              MC_WRT_STRING
#define mc_read_string_             MC_READ_STRING
#define mc_make_umesh_              MC_MAKE_UMESH
#define mc_set_global_class_count_  MC_SET_GLOBAL_CLASS_COUNT
#define mc_get_global_class_count_  MC_GET_GLOBAL_CLASS_COUNT
#define mc_def_class_               MC_DEF_CLASS
#define mc_def_class_idents_        MC_DEF_CLASS_IDENTS
#define mc_def_nodes_               MC_DEF_NODES
#define mc_def_node_labels_         MC_DEF_NODE_LABELS
#define mc_def_conn_seq_            MC_DEF_CONN_SEQ
#define mc_def_conn_seq_labels_     MC_DEF_CONN_ARB_LABELS
#define mc_def_conn_seq_labels_global_ids_     MC_DEF_CONN_ARB_LABELS_GLOBAL_IDS
#define mc_def_conn_labels_         MC_DEF_CONN_LABELS
#define mc_def_global_ids_          MC_DEF_GLOBAL_IDS
#define mc_def_conn_arb_            MC_DEF_CONN_ARB
#define mc_def_conn_arb_labels_     MC_DEF_CONN_ARB_LABELS
#define mc_def_conn_arb_labels_global_ids_     MC_DEF_CONN_ARB_LABELS_GLOBAL_IDS
#define mc_def_conn_surface_        MC_DEF_CONN_SURFACE
#define mc_load_nodes_              MC_LOAD_NODES
#define mc_load_conns_              MC_LOAD_CONNS
#define mc_load_surface_            MC_LOAD_SURFACE
#define mc_load_conn_labels_        MC_LOAD_CONN_LABELS
#define mc_load_node_labels_        MC_LOAD_NODE_LABELS
#define mc_get_node_label_info_     MC_GET_NODE_LABEL_INFO
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
#define mc_get_svar_size_           MC_GET_SVAR_SIZE
#define mc_get_svar_mo_ids_on_class_ MC_GET_SVAR_MO_IDS_ON_CLASS
#define mc_get_svar_on_class_       MC_GET_SVAR_ON_CLASS
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
#define mc_ti_undefclass_          MC_TI_UNDEF_CLASS
#define mc_ti_def_class_           MC_TI_DEF_CLASS
#define mc_ti_set_class_           MC_TI_SET_CLASS
#define mc_ti_get_class_           MC_TI_GET_CLASS
#define mc_ti_get_data_attribites_ MC_TI_GET_DATA_ATTRIBUTES
#define mc_ti_enable_              MC_TI_ENABLE
#define mc_ti_disable              MC_TI_DISABLE
#define mc_ti_make_var_name_       MC_TI_MAKE_VAR_NAME
#define mc_ti_wrt_scalar_          MC_TI_WRT_SCALAR
#define mc_ti_read_scalar_         MC_TI_READ_SCALAR
#define mc_ti_wrt_string_          MC_TI_WRT_STRING
#define mc_ti_read_string_         MC_TI_READ_STRING
#define mc_ti_wrt_array_           MC_TI_WRT_ARRAY
#define mc_ti_read_array_          MC_TI_READ_ARRAY
#define mc_ti_get_metadata_        MC_TI_GET_METADATA
#define mc_ti_check_arch_          MC_TI_CHECK_ARCH

#endif

#if defined(_WIN32) || defined(WIN32)  /* #JAL */
#define mc_end_state_               MC_END_STATE_
#define mc_open_                    MC_OPEN_
#define mc_close_                   MC_CLOSE_
#define mc_filelock_enable_         MC_FILELOCK_ENABLE_
#define mc_filelock_disable_        MC_FILELOCK_DISABLE_
#define mc_delete_family_           MC_DELETE_FAMILY_
#define mc_partition_state_data_    MC_PARTITION_STATE_DATA_
#define mc_wrt_scalar_              MC_WRT_SCALAR_
#define mc_read_scalar_             MC_READ_SCALAR_
#define mc_wrt_array_               MC_WRT_ARRAY_
#define mc_wrt_string_              MC_WRT_STRING_
#define mc_read_string_             MC_READ_STRING_
#define mc_make_umesh_              MC_MAKE_UMESH_
#define mc_def_class_               MC_DEF_CLASS_
#define mc_def_class_idents_        MC_DEF_CLASS_IDENTS_
#define mc_def_nodes_               MC_DEF_NODES_
#define mc_def_conn_seq_            MC_DEF_CONN_SEQ_
#define mc_def_conn_arb_            MC_DEF_CONN_ARB_
#define mc_def_conn_surface_        MC_DEF_CONN_SURFACE_
#define mc_load_nodes_              MC_LOAD_NODES_
#define mc_load_conns_              MC_LOAD_CONNS_
#define mc_load_surface_            MC_LOAD_SURFACE_
#define mc_get_mesh_id_             MC_GET_MESH_ID_
#define mc_def_svars_               MC_DEF_SVARS_
#define mc_def_vec_svar_            MC_DEF_VEC_SVAR_
#define mc_def_arr_svar_            MC_DEF_ARR_SVAR_
#define mc_def_vec_arr_svar_        MC_DEF_VEC_ARR_SVAR_
#define mc_open_srec_               MC_OPEN_SREC_
#define mc_def_subrec_              MC_DEF_SUBREC_
#define mc_def_surf_subrec_         MC_DEF_SURF_SUBREC_  /* JAL */
#define mc_close_srec_              MC_CLOSE_SREC_
#define mc_flush_                   MC_FLUSH_
#define mc_new_state_               MC_NEW_STATE_
#define mc_restart_at_state_        MC_RESTART_AT_STATE_
#define mc_restart_at_file_         MC_RESTART_AT_FILE_
#define mc_wrt_stream_              MC_WRT_STREAM_
#define mc_wrt_subrec_              MC_WRT_SUBREC_
#define mc_read_results_            MC_READ_RESULTS_
#define mc_get_svar_size_           MC_GET_SVAR_SIZE_
#define mc_get_svar_mo_ids_on_class_ MC_GET_SVAR_MO_IDS_ON_CLASS_
#define mc_get_svar_on_class_       MC_GET_SVAR_ON_CLASS_
#define mc_limit_states_            MC_LIMIT_STATES_
#define mc_limit_filesize_          MC_LIMIT_FILESIZE_
#define mc_suffix_width_            MC_SUFFIX_WIDTH_
#define mc_print_error_             MC_PRINT_ERROR_
#define mc_query_family_            MC_QUERY_FAMILY_
#define mc_get_class_info_          MC_GET_CLASS_INFO_
#define mc_get_simple_class_info_   MC_GET_SIMPLE_CLASS_INFO_
#define mc_set_buffer_qty_          MC_SET_BUFFER_QTY_
#define mc_read_param_array_        MC_READ_PARAM_ARRAY_
#define mc_get_metadata_            MC_GET_METADATA_   /* JAL */

#define mc_ti_undef_class_         MC_TI_UNDEF_CLASS_
#define mc_ti_def_class_           MC_TI_DEF_CLASS_
#define mc_ti_set_class_           MC_TI_SET_CLASS_
#define mc_ti_get_class_           MC_TI_GET_CLASS_
#define mc_ti_enable_              MC_TI_ENABLE_
#define mc_ti_disable_             MC_TI_DISABLE_
#define mc_ti_make_var_name_       MC_TI_MAKE_VAR_NAME_
#define mc_ti_wrt_scalar_          MC_TI_WRT_SCALAR_
#define mc_ti_read_scalar_         MC_TI_READ_SCALAR_
#define mc_ti_wrt_string_          MC_TI_WRT_STRING_
#define mc_ti_read_string_         MC_TI_READ_STRING_
#define mc_ti_wrt_array_           MC_TI_WRT_ARRAY_
#define mc_ti_read_array_          MC_TI_READ_ARRAY_
#define mc_ti_get_metadata_        MC_TI_GET_METADATA_
#define mc_ti_check_arch_          MC_TI_CHECK_ARCH_
#define mc_ti_check_if_data_found_ MC_TI_CHECK_IF_DATA_FOUND_  /* JAL */
#define mc_ti_get_data_attribites_ MC_TI_GET_DATA_ATTRIBUTES_

#define mc_def_conn_labels_         MC_DEF_CONN_LABELS_
#define mc_def_node_labels_         MC_DEF_NODE_LABELS_
#define mc_load_conn_labels_        MC_LOAD_CONN_LABELS_
#define mc_load_node_labels_        MC_LOAD_NODE_LABELS_
#define mc_get_node_label_info_     MC_GET_NODE_LABEL_INFO_
#define mc_def_conn_arb_labels_     MC_DEF_CONN_ARB_LABELS_
#define mc_def_conn_seq_labels_     MC_DEF_CONN_SEQ_LABELS_

/* JAL */
#define mc_set_global_class_count_             MC_SET_GLOBAL_CLASS_COUNT_
#define mc_get_global_class_count_             MC_GET_GLOBAL_CLASS_COUNT_
#define mc_def_conn_arb_labels_global_ids_     MC_DEF_CONN_ARB_LABELS_GLOBAL_IDS_
#define mc_def_conn_seq_labels_global_ids_     MC_DEF_CONN_SEQ_LABELS_GLOBAL_IDS_
#define mc_def_seq_labels_                     MC_DEF_SEQ_LABELS_
#define mc_def_global_ids_                     MC_DEF_GLOBAL_IDS_

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

Return_value
mc_end_state_( Famid *fam_id , int *srec_id )
{
   return mc_end_state(*fam_id, *srec_id);
}

         
Return_value
mc_open_( CHAR_DESCR param_root, CHAR_DESCR param_path,
          CHAR_DESCR param_mode, Famid *fam_id )
{
   char *c_param_root, *c_param_path, *c_param_mode;

   c_param_root = CHAR_CONV_F2C( param_root );
   c_param_path = CHAR_CONV_F2C( param_path );
   c_param_mode = CHAR_CONV_F2C( param_mode );

   return mc_open( c_param_root, c_param_path, c_param_mode, fam_id );
}


Return_value
mc_close_( Famid *fam_id )
{
   return mc_close( *fam_id );
}


void
mc_filelock_enable_( void )
{
   mc_filelock_enable( );
   return;
}


void
mc_filelock_disable_( void )
{
   mc_filelock_disable( );
   return;
}



Return_value
mc_delete_family_( CHAR_DESCR param_root, CHAR_DESCR param_path )
{
   char *c_param_root, *c_param_path;

   c_param_root = CHAR_CONV_F2C( param_root );
   c_param_path = CHAR_CONV_F2C( param_path );

   return mc_delete_family( c_param_root, c_param_path );
}


Return_value
mc_partition_state_data_( Famid *fam_id )
{
   return mc_partition_state_data( *fam_id );
}


Return_value
mc_wrt_scalar_( Famid *fam_id, int *type, CHAR_DESCR name, void *p_value )
{
   char *c_name;

   c_name = CHAR_CONV_F2C( name );

   return mc_wrt_scalar( *fam_id, *type, c_name, p_value );
}

Return_value
mc_set_global_class_count_( Famid *fam_id, CHAR_DESCR name, int *p_value )
{
   char *c_name;

   c_name = CHAR_CONV_F2C( name );

   return mc_set_global_class_count( *fam_id, c_name, *p_value );
}


Return_value
mc_read_scalar_( Famid *fam_id, CHAR_DESCR name, void *p_value )
{
   char *c_name;

   c_name = CHAR_CONV_F2C( name );

   return mc_read_scalar( *fam_id, c_name, p_value );
}

Return_value
mc_get_global_class_count_( Famid *fam_id, CHAR_DESCR name, void *p_value )
{
   char *c_name;

   c_name = CHAR_CONV_F2C( name );

   return mc_get_global_class_count( *fam_id, c_name, p_value );
}


Return_value
mc_wrt_array_( Famid *fam_id, int *type, CHAR_DESCR name, int *rank,
               int *dimensions, void *data )
{
   char *c_name;

   c_name = CHAR_CONV_F2C( name );

   return mc_wrt_array( *fam_id, *type, c_name, *rank, dimensions, data );
}


Return_value
mc_wrt_string_( Famid *fam_id, CHAR_DESCR name, CHAR_DESCR value )
{
   char *c_name, *c_value;

   c_name = CHAR_CONV_F2C( name );
   c_value = CHAR_CONV_F2C( value );

   return mc_wrt_string( *fam_id, c_name, c_value );
}


Return_value
mc_read_string_( Famid *fam_id, CHAR_DESCR name, CHAR_DESCR value )
{
   char *c_name;
   static char c_value[M_MAX_STRING_LEN + 1];
   Return_value rval;
   char *p_value;

   c_name = CHAR_CONV_F2C( name );

   ASSIGN_STRING_IN( p_value, c_value, value );

   rval = mc_read_string( *fam_id, c_name, p_value );

   COPY_STRING_OUT( value, c_value );

   return rval;
}


Return_value
mc_make_umesh_( Famid *fam_id, CHAR_DESCR mesh_name,  int *p_dim,
                int *p_mesh_id )
{
   char *c_mesh_name;

   c_mesh_name = CHAR_CONV_F2C( mesh_name );

   return mc_make_umesh( *fam_id, c_mesh_name, *p_dim, p_mesh_id );
}


Return_value
mc_def_class_( Famid *fam_id, int *mesh_id, int *superclass,
               CHAR_DESCR short_name, CHAR_DESCR long_name )
{
   char *c_short_name, *c_long_name;

   c_short_name = CHAR_CONV_F2C( short_name );
   c_long_name = CHAR_CONV_F2C( long_name );

   return mc_def_class( *fam_id, *mesh_id, *superclass, c_short_name,
                        c_long_name );
}


Return_value
mc_def_class_idents_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                      int *start, int *stop )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_def_class_idents( *fam_id, *mesh_id, c_class_name, *start,
                               *stop );
}


Return_value
mc_def_nodes_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
               int *start_node, int *stop_node, float *coords )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_def_nodes( *fam_id, *mesh_id, c_class_name, *start_node,
                        *stop_node, coords );
}


Return_value
mc_def_node_labels_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                     int *qty, int *labels )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_def_node_labels( *fam_id, *mesh_id, c_class_name, *qty,
                              labels );
}


Return_value
mc_def_conn_arb_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                  int *qty, int *elem_ids, int *data )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_def_conn_arb( *fam_id, *mesh_id, c_class_name, *qty, elem_ids,
                           data );
}

Return_value
mc_def_conn_arb_labels_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                         int *qty, int *elem_ids, int *labels, int *data )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_def_conn_arb_labels( *fam_id, *mesh_id, c_class_name, *qty,
                                  elem_ids,  labels, data );
}

Return_value
mc_def_conn_arb_labels_global_ids_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                         int *qty, int *elem_ids, int *labels, int *data,
                         int *global_ids )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_def_conn_arb_labels_global_ids( *fam_id, *mesh_id, c_class_name, 
                                  *qty, elem_ids,  labels, data, global_ids );
}

Return_value
mc_def_conn_seq_labels_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                         int *start_el, int *stop_el, int *labels, int *data )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_def_conn_seq_labels( *fam_id, *mesh_id, c_class_name, *start_el,
                                  *stop_el, labels, data );
}

Return_value
mc_def_seq_labels_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                         int *start_el, int *stop_el, int *labels )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_def_seq_labels( *fam_id, *mesh_id, c_class_name, *start_el,
                                  *stop_el, labels);
}

Return_value
mc_def_conn_seq_labels_global_ids_( Famid *fam_id, int *mesh_id, 
                                    CHAR_DESCR class_name, int *start_el, 
                                    int *stop_el, int *labels, int *data, 
                                    int *global_ids)
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_def_conn_seq_labels_global_ids( *fam_id, *mesh_id, c_class_name, 
                                  *start_el,*stop_el, labels, data, global_ids);
}


Return_value
mc_def_conn_seq_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                  int *start_el, int *stop_el, int *data )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_def_conn_seq( *fam_id, *mesh_id, c_class_name, *start_el,
                           *stop_el, data );
}

Return_value
mc_def_conn_labels_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                     int *qty, int *idents, int *labels )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_def_conn_labels( *fam_id, *mesh_id, c_class_name, *qty,
                              idents, labels );
}

Return_value
mc_def_global_ids_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                     int *qty, int *idents, int *global_ids )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_def_global_ids( *fam_id, *mesh_id, c_class_name, *qty,
                              idents, global_ids );
}

Return_value
mc_load_conn_labels_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                      int *qty,
                      int *num_blocks, int *block_range,
                      int *element_ids, int *labels )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_load_conn_labels( *fam_id, *mesh_id, c_class_name, *qty,
                               num_blocks, &block_range,
                               element_ids, labels );
}


Return_value
mc_def_conn_surface_( Famid *fam_id, int *mesh_id, CHAR_DESCR short_name,
                      int *qty_of_facets, int *data, int *surface_id )
{
   char *c_short_name;

   c_short_name = CHAR_CONV_F2C( short_name );

   return mc_def_conn_surface( *fam_id, *mesh_id, c_short_name,
                               *qty_of_facets, data, surface_id );
}


Return_value
mc_load_nodes_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                void *coords )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_load_nodes( *fam_id, *mesh_id, c_class_name, coords );
}

Return_value
mc_load_node_labels_(Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                     int *num_blocks, int *block_range,
                     int *labels)
{
   char *c_class_name;
   Return_value rval;
   int *c_block_range = NULL;
   int i;

   c_class_name = CHAR_CONV_F2C(class_name);

   rval = mc_load_node_labels(*fam_id, *mesh_id, c_class_name,
                              num_blocks, &c_block_range, labels);

   for (i = 0; i < 2*(*num_blocks); ++i) {
      block_range[i] = c_block_range[i];
   }

   if (c_block_range) {
      free(c_block_range);
   }

   return rval;
}

Return_value
mc_get_node_label_info_(Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                        int *num_blocks, int *num_labels)
{
   char *c_class_name;
	Return_value rval;
   c_class_name = CHAR_CONV_F2C( class_name );

   rval = mc_get_node_label_info(*fam_id, *mesh_id, c_class_name,
                                 num_blocks, num_labels);
	return rval;
}

Return_value
mc_load_conns_( Famid *fam_id, int *mesh_id, CHAR_DESCR class_name,
                int *conns, int *mats, int *parts )
{
   char *c_class_name;
	Return_value rval;
   c_class_name = CHAR_CONV_F2C( class_name );

   rval = mc_load_conns( *fam_id, *mesh_id, c_class_name, conns,
                         mats, parts );
	return rval;
}


Return_value
mc_load_surface_( Famid *fam_id, int *mesh_id, int surf, CHAR_DESCR class_name,
                  int *conns )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_load_surface( *fam_id, *mesh_id, surf, c_class_name, conns );
}


Return_value
mc_get_mesh_id_( Famid *fam_id, CHAR_DESCR mesh_name, int *p_mesh_id )
{
   char *c_mesh_name;

   c_mesh_name = CHAR_CONV_F2C( mesh_name );

   return mc_get_mesh_id( *fam_id, c_mesh_name, p_mesh_id );
}


Return_value
mc_def_svars_( Famid *fam_id, int *qty_svars, CHAR_DESCR names,
               int *name_stride, CHAR_DESCR titles, int *title_stride,
               int types[] )
{
   char *c_names, *c_titles;

   c_names = CHAR_CONV_F2C( names );
   c_titles = CHAR_CONV_F2C( titles );

   return mc_def_svars( *fam_id, *qty_svars, c_names, *name_stride,
                        c_titles, *title_stride, types );
}


Return_value
mc_def_vec_svar_( Famid *fam_id, int *type, int *qty_fields, CHAR_DESCR name,
                  CHAR_DESCR title, CHAR_DESCR field_names, int *name_stride,
                  CHAR_DESCR field_titles, int *title_stride )
{
   char *c_name, *c_title, *c_field_names, *c_field_titles;
   char **field_names_array, **field_titles_array;
   int i;
   int qty, dtype;
   int n_stride, t_stride;
   Return_value rval = OK;

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
      if (field_names_array[i] == NULL || field_titles_array[i] == NULL)
      {
         rval = ALLOC_FAILED;
         break;
      }
   }

   if (rval == OK)
   {
      rval = mc_def_vec_svar( *fam_id, dtype, qty, c_name, c_title,
                              field_names_array, field_titles_array );
   }

   for ( i = 0; i < qty; i++ )
   {
      free( field_names_array[i] );
      free( field_titles_array[i] );
   }
   free( field_names_array );
   free( field_titles_array );

   return rval;
}


Return_value
mc_def_arr_svar_( Famid *fam_id, int *rank, int dims[], CHAR_DESCR name,
                  CHAR_DESCR title, int *type )
{
   char *c_name, *c_title;

   c_name = CHAR_CONV_F2C( name );
   c_title = CHAR_CONV_F2C( title );

   return mc_def_arr_svar( *fam_id, *rank, dims, c_name, c_title, *type );
}


Return_value
mc_def_vec_arr_svar_( Famid *fam_id, int *rank, int dims[], CHAR_DESCR name,
                      CHAR_DESCR title, int *qty_fields,
                      CHAR_DESCR field_names, int *name_stride,
                      CHAR_DESCR field_titles,  int *title_stride, int *type )
{
   char *c_name, *c_title, *c_field_names, *c_field_titles;
   char **field_names_array, **field_titles_array;
   int qty, i;
   int n_stride, t_stride;
   Return_value rval = OK;

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
      if (field_names_array[i] == NULL || field_titles_array[i] == NULL)
      {
         rval = ALLOC_FAILED;
         break;
      }
   }

   if (rval == OK)
   {
      rval = mc_def_vec_arr_svar( *fam_id, *rank, dims, c_name, c_title, qty,
                                  field_names_array, field_titles_array,
                                  *type );
   }

   for ( i = 0; i < qty; i++ )
   {
      free( field_names_array[i] );
      free( field_titles_array[i] );
   }
   free( field_names_array );
   free( field_titles_array );

   return rval;
}


Return_value
mc_open_srec_( Famid *fam_id, int *mesh_id, int *p_srec_id )
{
   return mc_open_srec( *fam_id, *mesh_id, p_srec_id );
}


int
mc_close_srec_( Famid *fam_id, int *p_srec_id )
{
   return mc_close_srec( *fam_id, *p_srec_id );
}


Return_value
mc_def_subrec_( Famid *fam_id, int *srec_id, CHAR_DESCR subrec_name,
                int *data_org, int *qty_svars, CHAR_DESCR svar_names,
                int *name_stride, CHAR_DESCR subclass, int *format,
                int *qty, int *mo_ids, int *flag )
{
   Return_value rval;
   char *c_subrec_name, *c_svar_names, *c_subclass;

   c_subrec_name = CHAR_CONV_F2C( subrec_name );
   c_svar_names = CHAR_CONV_F2C( svar_names );
   c_subclass = CHAR_CONV_F2C( subclass );

   fortran_api = 1;

   rval =  mc_def_subrec( *fam_id, *srec_id, c_subrec_name, *data_org,
                          *qty_svars, c_svar_names, *name_stride,
                          c_subclass, *format, *qty, mo_ids, flag );

   fortran_api = 0;

   return rval;
}


Return_value
mc_flush_( Famid *fam_id, int *data_type )
{
   return mc_flush( *fam_id, *data_type );
}


Return_value
mc_new_state_( Famid *fam_id, int *srec_id, float *time, int *p_file_suffix,
               int *p_file_state_index )
{
   return mc_new_state( *fam_id, *srec_id, *time, p_file_suffix,
                        p_file_state_index );
}


Return_value
mc_restart_at_state_( Famid *fam_id, int *p_file_suffix, int *p_state_index )
{
   return mc_restart_at_state( *fam_id, *p_file_suffix, *p_state_index );
}


Return_value
mc_restart_at_file_( Famid *fam_id, int *file_name_index )
{
   return mc_restart_at_file( *fam_id, *file_name_index );
}


Return_value
mc_wrt_stream_( Famid *fam_id, int *type, int *qty, void *data )
{
   return mc_wrt_stream( *fam_id, *type, *qty, data );
}


Return_value
mc_wrt_subrec_( Famid *fam_id, CHAR_DESCR subrec_name, int *p_start,
                int *p_stop, void *data )
{
   char *c_subrec_name;

   c_subrec_name = CHAR_CONV_F2C( subrec_name );

   return mc_wrt_subrec( *fam_id, c_subrec_name, *p_start, *p_stop, data );
}


Return_value
mc_read_results_( Famid *fam_id, int *p_state, int *p_subrec_id, int *p_qty,
                  CHAR_DESCR res_names, int *name_stride, void *p_data )
{
   char *c_res_names;
   char **specs, **c_specs;
   char *spec_buf, *p_c;
   int i, j;
   int qty;
   int stride;
   Return_value rval;
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
      if (specs[i] == NULL)
      {
         for ( j = 0; j < i; j++ )
         {
            free( specs[j] );
         }
         free( specs );
         return ALLOC_FAILED;
      }
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
      if (p_name == NULL)
      {
         for ( j = 0; j < qty; j++ )
         {
            free( specs[j] );
         }
         free(specs);
         free(spec_buf);
         free(c_specs);

         return BAD_NAME_READ;
      }

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
            {
               return MALFORMED_SUBSET;
            }
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
                  {
                     *p_c++ = ',';
                  }
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
   {
      free( specs[i] );
   }
   free( specs );

   rval = mc_read_results( *fam_id, *p_state, *p_subrec_id, qty, c_specs,
                           p_data );

   free( spec_buf );
   free( c_specs );

   return rval;
}


Return_value
mc_get_svar_size_(Famid *p_fam_id, CHAR_DESCR class_name, CHAR_DESCR var_name,
                  int *p_num_blocks, int *p_size, int *p_type)
{
   Return_value rval;
   char *c_class_name, *c_var_name;

   c_class_name = CHAR_CONV_F2C(class_name);
   c_var_name   = CHAR_CONV_F2C(var_name);

   rval = mc_get_svar_size(*p_fam_id, c_class_name, var_name,
                           p_num_blocks, p_size, p_type);

   return rval;
}


Return_value
mc_get_svar_mo_ids_on_class_(Famid *p_fam_id, CHAR_DESCR class_name,
                             CHAR_DESCR var_name, int *blocks)
{
   Return_value rval;
   char *c_class_name, *c_var_name;

   c_class_name = CHAR_CONV_F2C(class_name);
   c_var_name   = CHAR_CONV_F2C(var_name);

   rval = mc_get_svar_mo_ids_on_class(*p_fam_id, c_class_name,
                                      c_var_name, blocks);

   return rval;
}


Return_value
mc_get_svar_on_class_(Famid *p_fam_id, int *p_state, CHAR_DESCR class_name,
                      CHAR_DESCR var_name, void *result)
{
   Return_value rval;
   char *c_class_name, *c_var_name;

   c_class_name = CHAR_CONV_F2C(class_name);
   c_var_name   = CHAR_CONV_F2C(var_name);

   rval = mc_get_svar_on_class(*p_fam_id, *p_state,
                               c_class_name, c_var_name, result);

   return rval;
}


Return_value
mc_limit_states_( Famid *fam_id, int *states_per_file )
{
   return mc_limit_states( *fam_id, *states_per_file );
}


Return_value
mc_limit_filesize_( Famid *fam_id, int *filesize )
{
   return mc_limit_filesize( *fam_id, *filesize );
}


Return_value
mc_suffix_width_( Famid *fam_id, int *suffix_width )
{
   return mc_suffix_width( *fam_id, *suffix_width );
}


void
mc_print_error_( CHAR_DESCR preamble, int *return_value )
{
   char *c_preamble;

   c_preamble = CHAR_CONV_F2C( preamble );

   mc_print_error( c_preamble, *return_value );

   return;
}


Return_value
mc_query_family_( Famid *fam_id, int *req_type, void *num_args,
                  CHAR_DESCR char_args, int *p_info )
{
   Return_value rval;
   char *c_char_args;
   void *p_void;
   int rtype;
   static char char_info[M_MAX_STRING_LEN + 1];
   Bool_type string_output;

   c_char_args = CHAR_CONV_F2C( char_args );
   rtype = *req_type;

   string_output = FALSE;

   if ( rtype == SUBREC_CLASS || rtype == LIB_VERSION || rtype == MESH_NAME )
   {
      p_void = (void *) char_info;
      string_output = TRUE;
   }
   else
   {
      p_void = (void *) p_info;
   }

   rval = mc_query_family( *fam_id, rtype, num_args, c_char_args, p_void );

   if ( string_output )
   {
      COPY_STRING_OUT( (CHAR_DESCR) p_info, char_info );
   }

   return rval;
}


Return_value
mc_get_class_info_( Famid *famid, int *mesh_id, int *superclass,
                    int *class_index, CHAR_DESCR short_name,
                    CHAR_DESCR long_name, int *object_qty )
{
   static char c_short_name[M_MAX_NAME_LEN + 1];
   static char c_long_name[M_MAX_NAME_LEN + 1];
   Return_value rval;
   char *p_short_name, *p_long_name;

   ASSIGN_STRING_IN( p_short_name, c_short_name, short_name )
   ASSIGN_STRING_IN( p_long_name, c_long_name, long_name )

   rval = mc_get_class_info( *famid, *mesh_id, *superclass, *class_index,
                             p_short_name, p_long_name, object_qty );

   COPY_STRING_OUT( short_name, c_short_name );
   COPY_STRING_OUT( long_name, c_long_name );

   return rval;
}


Return_value
mc_get_simple_class_info_( Famid *famid, int *mesh_id, int *superclass,
                           int *class_index, CHAR_DESCR short_name,
                           CHAR_DESCR long_name, int *start_ident,
                           int *stop_ident )
{
   static char c_short_name[M_MAX_NAME_LEN + 1];
   static char c_long_name[M_MAX_NAME_LEN + 1];
   Return_value rval;
   char *p_short_name, *p_long_name;

   ASSIGN_STRING_IN( p_short_name, c_short_name, short_name );
   ASSIGN_STRING_IN( p_long_name, c_long_name, long_name );

   rval = mc_get_simple_class_info( *famid, *mesh_id, *superclass,
                                    *class_index, p_short_name, p_long_name,
                                    start_ident, stop_ident );

   COPY_STRING_OUT( short_name, c_short_name );
   COPY_STRING_OUT( long_name, c_long_name );

   return rval;
}


Return_value
mc_set_buffer_qty_( Famid *famid, int *mesh_id, CHAR_DESCR class_name,
                    int *buf_qty )
{
   char *c_class_name;

   c_class_name = CHAR_CONV_F2C( class_name );

   return mc_set_buffer_qty( *famid, *mesh_id, c_class_name, *buf_qty );
}

/*************************************************************************
 *
 *  New TI functions - added October 2006 EMP
 *
 ************************************************************************/
Return_value
mc_ti_undef_class_( Famid *famid )
{
   return mc_ti_undef_class( *famid );
}


Return_value
mc_ti_def_class_( Famid *famid, int *meshid, int *state, int  *matid,
                  CHAR_DESCR superclass,    int *meshvar, int *nodal,
                  CHAR_DESCR short_name, CHAR_DESCR long_name )
{
   char *c_superclass, *c_short_name, *c_long_name;
   Bool_type meshvar_flag=FALSE, nodal_flag=FALSE;

   c_superclass = CHAR_CONV_F2C( superclass );
   c_short_name = CHAR_CONV_F2C( short_name );
   c_long_name = CHAR_CONV_F2C( long_name );

   if ( *nodal )
   {
      nodal_flag = TRUE;
   }

   if ( *meshvar )
   {
      meshvar_flag = TRUE;
   }

   return mc_ti_set_class( *famid, *meshid, *state, *matid, c_superclass,
                           meshvar_flag, nodal_flag,
                           c_short_name, c_long_name );
}


Return_value
mc_ti_set_class_( Famid *famid, int *meshid, int *state, int *matid,
                  CHAR_DESCR superclass, int *meshvar, int *nodal,
                  CHAR_DESCR short_name, CHAR_DESCR long_name )
{
   char *c_superclass, *c_short_name, *c_long_name;

   Bool_type meshvar_flag=FALSE, nodal_flag=FALSE;

   c_superclass = CHAR_CONV_F2C( superclass );
   c_short_name = CHAR_CONV_F2C( short_name );
   c_long_name = CHAR_CONV_F2C( long_name );

   if ( *nodal )
   {
      nodal_flag = TRUE;
   }

   if ( *meshvar )
   {
      meshvar_flag = TRUE;
   }

   return mc_ti_set_class( *famid, *meshid, *state, *matid, c_superclass,
                           meshvar_flag, nodal_flag,
                           c_short_name, c_long_name );
}


void
mc_ti_enable_( Famid famid )
{
   mc_ti_enable( famid );
}


void
mc_ti_disable_( Famid famid )
{
   mc_ti_disable( famid );
}


Return_value
mc_ti_make_var_name_( Famid *famid, CHAR_DESCR name, CHAR_DESCR class,
                      CHAR_DESCR new_name )
{
   char *c_class;
   char *c_name;
   char *p_name;
   Return_value status;

   c_name  = CHAR_CONV_F2C( name );
   c_class = CHAR_CONV_F2C( class );

   ASSIGN_STRING_IN( p_name, c_name, new_name );

   status = mc_ti_make_var_name( *famid, c_name, c_class, p_name );

   COPY_STRING_OUT( new_name, c_name );

   return status;
}


Return_value
mc_ti_wrt_scalar_( Famid *fam_id, int *type, CHAR_DESCR name, void *value )
{
   char *c_name;

   c_name = CHAR_CONV_F2C( name );

   return mc_ti_wrt_scalar( *fam_id, *type, c_name, value );
}


Return_value
mc_ti_read_scalar_( Famid *famid, CHAR_DESCR name, void *value )
{
   char *c_name;
   Return_value rval;

   c_name = CHAR_CONV_F2C( name );

   rval = mc_ti_read_scalar( *famid, c_name, value );

   return rval;
}


Return_value
mc_ti_wrt_string_( Famid *fam_id, CHAR_DESCR name, CHAR_DESCR value )
{
   char *c_name, *c_value;

   c_name = CHAR_CONV_F2C( name );
   c_value = CHAR_CONV_F2C( value );

   return mc_ti_wrt_string( *fam_id, c_name, c_value );
}


Return_value
mc_ti_read_string_( Famid *famid, CHAR_DESCR name, CHAR_DESCR value )
{
   char *c_name;
   char *p_value;
   static char c_value[M_MAX_STRING_LEN + 1];
   Return_value rval;

   c_name = CHAR_CONV_F2C( name );

   ASSIGN_STRING_IN( p_value, c_value, value );

   rval = mc_ti_read_string( *famid, c_name, p_value );

   COPY_STRING_OUT( value, c_value );

   return rval;
}


Return_value
mc_ti_wrt_array_( Famid *famid, int *type, CHAR_DESCR name,
                  int *order, int *dims, void *data )
{
   char *c_name;

   c_name = CHAR_CONV_F2C( name );

   return mc_ti_wrt_array( *famid, *type, c_name, *order, dims, data );
}


Return_value
mc_ti_read_array_( Famid *famid, CHAR_DESCR name, void *value,
                   int *num_items_read )
{
   char *c_name;

   c_name = CHAR_CONV_F2C( name );

   return mc_ti_read_array( *famid, c_name, value, num_items_read );

}

Return_value
mc_ti_get_metadata_( Famid *fam_id, CHAR_DESCR mili_version, CHAR_DESCR host,
                     CHAR_DESCR arch, CHAR_DESCR timestamp,
                     CHAR_DESCR username,
                     CHAR_DESCR xmilics_version, CHAR_DESCR code_name )
{
   char *c_mili_version, *c_host, *c_arch, *c_timestamp, *c_username,
        *c_xmilics_version, *c_code_name;

   c_mili_version    = CHAR_CONV_F2C( mili_version );
   c_host            = CHAR_CONV_F2C( host );
   c_arch            = CHAR_CONV_F2C( arch );
   c_timestamp       = CHAR_CONV_F2C( timestamp );
   c_username        = CHAR_CONV_F2C( username );
   c_xmilics_version = CHAR_CONV_F2C( xmilics_version );
   c_code_name       = CHAR_CONV_F2C( code_name );

   return mc_ti_get_metadata( *fam_id, c_mili_version, c_host, c_arch,
                              c_timestamp, c_username, c_xmilics_version,
                              c_code_name );
}

Return_value
mc_get_metadata_( Famid *fam_id, CHAR_DESCR mili_version, CHAR_DESCR host,
                  CHAR_DESCR arch, CHAR_DESCR timestamp,
                  CHAR_DESCR xmilics_version )
{
   char *c_mili_version, *c_host, *c_arch, *c_timestamp, *c_xmilics_version;

   c_mili_version = CHAR_CONV_F2C( mili_version );
   c_host         = CHAR_CONV_F2C( host );
   c_arch         = CHAR_CONV_F2C( arch );
   c_timestamp    = CHAR_CONV_F2C( timestamp );
   c_xmilics_version = CHAR_CONV_F2C( xmilics_version );

   return mc_get_metadata( *fam_id, c_mili_version, c_host, c_arch,
                           c_timestamp, c_xmilics_version );
}

Return_value
mc_ti_check_arch_( Famid fam_id )
{
   return mc_ti_check_arch( fam_id );
}

#endif
