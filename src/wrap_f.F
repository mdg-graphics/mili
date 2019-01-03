c.....MILI Fortran wrappers

c ************************************************************************
c Modifications:
c  02/01/06: IRC - Fixed a typo.
c     
c  I. R. Corey - August 18, 2006: Added new file type for time invarient
c                data (TI). This data will be written to a new file
c                See SCR #298.
c
c  Time Independent file functions:
c  Added May 2006 by I.R. Corey
c
c  I. R. Corey - October 24, 2007: Added element and nodal labels.
c                See SCR #418.
c 
c  I. R. Corey - February 27, 2012: New functions to write strings for
c                stream and subrecord output.
c                See TeamForge #17490.
c 
c ************************************************************************
      
c.....MILI C API

c    mc_open
c    mc_close
c    mc_filelock_enable
c    mc_filelock_disable
c    mc_delete_family
c    mc_partition_state_data
c    mc_wrt_scalar
c    mc_read_scalar
c    mc_wrt_array
c    mc_wrt_string
c    mc_read_string
c    mc_set_global_class_count  *****
c    mc_get_global_class_count  *****
c    mc_make_umesh
c    mc_def_class
c    mc_def_class_idents
c    mc_def_nodes
c    mc_def_global_ids
c    mc_def_conn_labels
c    mc_def_conn_seq
c    mc_def_conn_seq_global_ids
c    mc_def_conn_arb
c    mc_def_conn_arb_labels
c    mc_def_conn_arb_labels_global_ids
c    mc_def_conn_surface
c    mc_load_nodes
c    mc_load_conns
c    mc_load_surface
c    mc_load_conn_labels
c    mc_load_node_labels
c    mc_get_node_label_info
c    mc_get_mesh_id
c    mc_get_metadata
c    mc_def_svars
c    mc_def_vec_svar
c    mc_def_arr_svar
c    mc_def_vec_arr_svar
c    mc_open_srec
c    mc_def_subrec
c    mc_def_surf_subrec
c    mc_flush
c    mc_new_state
c    mc_restart_at_state
c    mc_restart_at_file
c    mc_wrt_stream
c    mc_wrt_subrec
c    mc_read_results
c    mc_get_svar_size
c    mc_get_svar_mo_ids_on_class
c    mc_get_svar_on_class
c    mc_limit_states
c    mc_suffix_width
c    mc_print_error
c    mc_query_family
c    mc_get_class_info
c    mc_get_simple_class_info
c    mc_set_buffer_qty


c.....MILI Fortran API

c     mf_open                   Open a family
c     mf_close                  Close a family
c     mf_partition_state_data   Create a file partition in state data branch
c     mf_wrt_scalar             Write a named scalar value to the family
c     mf_read_scalar            Read a named scalar value from the family
c     mf_wrt_array              Write a named array of values to the family
c     mf_wrt_string             Write a named string to the family
c     mf_read_string            Read a named string from the family
c     mf_make_umesh             Prepare to define an unstructured mesh
c     mf_def_class              Define a class belonging to a superclass
c     mf_def_class_idents       Define object idents for a mesh object class
c     mf_def_nodes              Define node coordinates
c     mf_def_conn_seq           Define conn's for sequential list of elements
c     mf_def_conn_arb           Define conn's for arbitrary list of elements
c     mf_def_conn_seq_labels    Define conn's for sequential list of elements with labels
c     mf_def_conn_arb_labels    Define conn's for arbitrary list of elements with labels
c     mf_def_conn_labels        Define element labels
c     mf_load_nodes             Load nodal coordinates into memory
c     mf_def_node_labels        Define nodal labels
c     mf_load_node_labels       Load nodal labels
c     mf_get_node_label_info    Return info needed to make call to mf_get_node_labels
c     mf_load_conns             Load element connectivity data into memory
c     mf_load_conn_labels       Load element labels
c     mf_load_surface           Load connectivities for nodes of a surface class
c     mf_get_mesh_id            Return the numeric ident for the named mesh
c     mf_get_metadata           Read metadata entries from a Mili file
c     mf_def_svars              Define one or more state variables
c     mf_def_vec_svar           Define a vector state variable
c     mf_def_arr_svar           Define an array state variable
c     mf_def_vec_arr_svar       Define a vector array state variable
c     mf_open_srec              Prepare for state record definition
c     mf_close_srec             Close a state record definition from input
c     mf_make_srec              DEPRECATED - CALLS mc_open_srec
c     mf_def_subrec             Define a state subrecord
c     mf_def_surf_subrec        Define a state subrecord on surface class
c     mf_flush                  Flush cached info for state or non-state file
c     mf_new_state              Prepare to receive data for a new state
c     mf_restart_at_state       Prepare to overwrite at an existing state
c     mf_restart_at_file        Prepare to overwrite at an existing file
c     mf_wrt_stream             Write state data as a word stream
c     mf_wrt_stream_char        Write state data as a char stream
c     mf_wrt_subrec             Write one or more lumps of a state subrecord
c     mf_wrt_subrec_string      Write one lumps of a state subrecord of char type
c     mf_read_results           Read state var result arrays from a subrecord
c     mf_get_svar_size          Returns amount and type of data for svar
c     mf_get_svar_mo_ids_on_class Read and return mesh object ids for specified class/var
c     mf_get_svar_on_class      Read and return results for specified class/var
c     mf_limit_states           Set max number of states per state-file
c     mf_suffix_width           Set min suffix width for state-file names
c     mf_print_error            Print diagnostic for a function return value
c     mf_query_family           Query information about contents of a family
c     mf_get_class_info         Get name and size of an mesh object class
c     mf_get_simple_class_info  Get name and start/stop idents of a mesh class
c     mf_set_buffer_qty         Set quantity of input buffers for object classes
c 
c TI Functions
c
c     mf_ti_undef_class
c     mf_ti_def_class
c     mf_ti_set_class
c     mf_ti_enable
c     mf_ti_disable
c     mf_ti_make_var_name
c     mf_ti_wrt_scalar
c     mf_ti_read_scalar
c     mf_ti_wrt_string
c     mf_ti_read_string
c     mf_ti_wrt_array
c     mf_ti_read_array
c     mc_ti_check_arch_c 
c     mc_ti_check_if_data_found
c     mc_ti_get_metadata_  

c  FORTRAN/C calling convention stuff...  More in wrap_c.c.

#if defined(_WIN32) || defined(WIN32) || defined(AIX) || defined(__hpux)
! one underscore after routine name
#define US_1
#endif

#ifdef US_1
#define mc_end_state mc_end_state_
#define mc_open mc_open_
#define mc_close mc_close_
#define mc_filelock_enable mc_filelock_enable_
#define mc_filelock_disable mc_filelock_disable_
#define mc_delete_family mc_delete_family_
#define mc_partition_state_data mc_partition_state_data_
#define mc_wrt_scalar mc_wrt_scalar_
#define mc_read_scalar mc_read_scalar_
#define mc_wrt_array mc_wrt_array_
#define mc_wrt_string mc_wrt_string_
#define mc_read_string mc_read_string_
#define mc_make_umesh mc_make_umesh_
#define mc_def_class mc_def_class_
#define mc_def_class_idents mc_def_class_idents_
#define mc_def_nodes mc_def_nodes_
#define mc_def_conn_seq mc_def_conn_seq_
#define mc_def_conn_arb mc_def_conn_arb_
#define mc_def_conn_surface mc_def_conn_surface_
#define mc_load_nodes mc_load_nodes_
#define mc_load_conns mc_load_conns_
#define mc_load_surface mc_load_surface_
#define mc_get_mesh_id mc_get_mesh_id_
#define mc_def_svars mc_def_svars_
#define mc_def_vec_svar mc_def_vec_svar_
#define mc_def_arr_svar mc_def_arr_svar_
#define mc_def_vec_arr_svar mc_def_vec_arr_svar_
#define mc_open_srec mc_open_srec_
#define mc_def_subrec mc_def_subrec_
#define mc_def_surf_subrec mc_def_surf_subrec_
#define mc_close_srec mc_close_srec_
#define mc_flush mc_flush_
#define mc_new_state mc_new_state_
#define mc_restart_at_state mc_restart_at_state_
#define mc_restart_at_file mc_restart_at_file_
#define mc_wrt_stream mc_wrt_stream_
#define mc_wrt_subrec mc_wrt_subrec_
#define mc_read_results mc_read_results_
#define mc_get_svar_size mc_get_svar_size_
#define mc_get_svar_mo_ids_on_class mc_get_svar_mo_ids_on_class_
#define mc_get_svar_on_class mc_get_svar_on_class_
#define mc_limit_states mc_limit_states_
#define mc_limit_filesize mc_limit_filesize_
#define mc_suffix_width mc_suffix_width_
#define mc_print_error mc_print_error_
#define mc_query_family mc_query_family_
#define mc_get_class_info mc_get_class_info_
#define mc_get_simple_class_info mc_get_simple_class_info_
#define mc_set_buffer_qty mc_set_buffer_qty_
#define mc_get_metadata mc_get_metadata_

c Labels
#define mc_def_conn_labels  mc_def_conn_labels_
#define mc_def_node_labels  mc_def_node_labels_
#define mc_load_conn_labels mc_load_conn_labels_
#define mc_load_node_labels mc_load_node_labels_
#define mc_get_node_label_info mc_get_node_label_info_
#define mc_def_conn_arb_labels mc_def_conn_arb_labels_
#define mc_def_conn_seq_labels mc_def_conn_seq_labels_
#define mc_def_seq_labels mc_def_seq_labels_

      
c ************************************************************************
c
c   New TI functions - added August 2006: IRC
c
c ************************************************************************

#define mc_ti_undef_class        mc_ti_undef_class_
#define mc_ti_def_class          mc_ti_def_class_          
#define mc_ti_set_class          mc_ti_set_class_       
#define mc_ti_enable             mc_ti_enable
#define mc_ti_disable            mc_ti_disable
#define mc_ti_make_var_name      mc_ti_make_var_name_   
#define mc_ti_wrt_scalar         mc_ti_wrt_scalar_       
#define mc_ti_read_scalar        mc_ti_read_scalar_      
#define mc_ti_wrt_string         mc_ti_wrt_string_       
#define mc_ti_read_string        mc_ti_read_string_ 
#define mc_ti_wrt_array          mc_ti_wrt_array_    
#define mc_ti_read_array         mc_ti_read_array_       
#define mc_ti_get_metadata       mc_ti_get_metadata_  
#define mc_ti_check_arch         mc_ti_check_arch_
#define mc_ti_check_if_data_found mc_ti_check_if_data_found_

#define mc_set_global_class_count             mc_set_global_class_count_
#define mc_get_global_class_count             mc_get_global_class_count_
#define mc_def_conn_arb_labels_global_ids     mc_def_conn_arb_labels_global_ids_
#define mc_def_conn_seq_labels_global_ids     mc_def_conn_seq_labels_global_ids_
#define mc_def_global_ids                     mc_def_global_ids_
#endif

      subroutine mf_end_state( fam_id, srec_id, rval )
      
      implicit none
      
      integer fam_id
      integer srec_id
      integer rval 
      include "mili_fparam.h"
      
      external mc_end_state
      integer mc_end_state 
      
      rval = mc_end_state( fam_id, srec_id)
      
      return
      end
      
      subroutine mf_open( root_name, path, mode, fam_id, rval )

      implicit none

      character*(*) root_name
      character*(*) path
      character*(*) mode
      integer fam_id
      integer rval
      
      include "mili_fparam.h"
      
      external mc_open, EFF_LEN
      integer mc_open, EFF_LEN

      character*(m_max_name_len + 1) param_root
      character*(m_max_path_len + 1) param_path
      character*(m_max_name_len + 1) param_mode
      integer slen

      slen = EFF_LEN( root_name, m_max_name_len ) + 1
      param_root = root_name
      param_root(slen:slen) = CHAR(0)

      slen = EFF_LEN( path, m_max_path_len ) + 1
      param_path = path
      param_path(slen:slen) = CHAR(0)

      slen = EFF_LEN( mode, m_max_name_len ) + 1
      param_mode = mode
      param_mode(slen:slen) = CHAR(0)

      rval = mc_open( param_root, param_path, param_mode, 
     +                fam_id )

      return
      end




      subroutine mf_close( fam_id, rval )

      implicit none
      
      integer fam_id
      integer rval
      
      external mc_close
      integer mc_close
      
      rval = mc_close( fam_id )
      
      return
      end



      subroutine mf_filelock_enable()

      implicit none

      include "mili_fparam.h"

      external mc_filelock_enable

      call mc_filelock_enable()

      return
      end



      subroutine mf_filelock_disable()

      implicit none

      include "mili_fparam.h"

      external mc_filelock_disable

      call mc_filelock_disable()

      return
      end



      subroutine mf_delete_family( root_name, path, rval )

      implicit none

      character*(*) root_name
      character*(*) path
      integer rval
      
      include "mili_fparam.h"
      
      external mc_delete_family, EFF_LEN
      integer mc_delete_family, EFF_LEN

      character*(m_max_name_len + 1) param_root
      character*(m_max_path_len + 1) param_path
      integer slen

      slen = EFF_LEN( root_name, m_max_name_len ) + 1
      param_root = root_name
      param_root(slen:slen) = CHAR(0)

      slen = EFF_LEN( path, m_max_path_len ) + 1
      param_path = path
      param_path(slen:slen) = CHAR(0)

      rval = mc_delete_family( param_root, param_path )

      return
      end




      subroutine mf_partition_state_data( fam_id, rval )

      implicit none
      
      integer fam_id
      integer rval
      
      external mc_partition_state_data
      integer mc_partition_state_data
      
      rval = mc_partition_state_data( fam_id )
      
      return
      end
     

      
      
      subroutine mf_wrt_scalar( fam_id, type, name, value, rval )

      implicit none
 
      integer fam_id
      integer type
      character*(*) name
      integer value  ! Type of scalar is arbitrary, just need the reference
      integer rval
      
      include "mili_fparam.h"
      
      external mc_wrt_scalar, EFF_LEN
      integer mc_wrt_scalar, EFF_LEN

      character*(m_max_name_len + 1) param_name
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)

      rval = mc_wrt_scalar( fam_id, type, param_name, value )

      return
      end
     

      subroutine mf_set_global_class_count( fam_id, name, value, 
     +                                      rval )

      implicit none
 
      integer fam_id
      character*(*) name
      integer value  ! Type of scalar is arbitrary, just need the reference
      integer rval
      
      include "mili_fparam.h"
      
      external mc_set_global_class_count, EFF_LEN
      integer mc_set_global_class_count, EFF_LEN

      character*(m_max_name_len + 1) param_name
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)

      rval = mc_set_global_class_count( fam_id, param_name, value )

      return
      end
      
      
      subroutine mf_read_scalar( fam_id, name, value, rval )

      implicit none

      integer fam_id
      character*(*) name
      integer value  ! Type of scalar is arbitrary, just need the reference
      integer rval
      
      include "mili_fparam.h"
      
      external mc_read_scalar, EFF_LEN
      integer mc_read_scalar, EFF_LEN

      character*(m_max_name_len + 1) param_name
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)

      rval = mc_read_scalar( fam_id, param_name, value )

      return
      end

      subroutine mf_get_global_class_count( fam_id, name, value, rval )

      implicit none

      integer fam_id
      character*(*) name
      integer value  ! Type of scalar is arbitrary, just need the reference
      integer rval
      
      include "mili_fparam.h"
      
      external mc_get_global_class_count, EFF_LEN
      integer mc_get_global_class_count, EFF_LEN

      character*(m_max_name_len + 1) param_name
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)

      rval = mc_get_global_class_count( fam_id, param_name, value )

      return
      end




      subroutine mf_wrt_array( fam_id, type, name, order, dims, 
     +                         values, rval )

      implicit none
      
      integer fam_id
      integer type
      character*(*) name
      integer order
      integer dims(order)
      integer values
      integer rval

      include "mili_fparam.h"
            
      external mc_wrt_array, EFF_LEN
      integer mc_wrt_array, EFF_LEN

      integer c_dims(m_max_array_dims)
      integer i
      character*(m_max_name_len + 1) param_name
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)
      
      do 10 i = 1, order
          c_dims(i) = dims(order - i + 1)
 10   continue

      rval = mc_wrt_array( fam_id, type, param_name, order, 
     +                     c_dims, values )

      return
      end


      
      
      subroutine mf_wrt_string( fam_id, name, string, rval )

      implicit none

      integer fam_id
      character*(*) name
      character*(*) string
      integer rval
      
      include "mili_fparam.h"
      
      external mc_wrt_string, EFF_LEN
      integer mc_wrt_string, EFF_LEN

      character*(m_max_name_len + 1) param_name
      character*(m_max_string_len + 1) param_string
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)

      slen = EFF_LEN( string, m_max_string_len  ) + 1
      param_string = string
      param_string(slen:slen) = CHAR(0)
      
      rval = mc_wrt_string( fam_id, param_name, param_string )
      
      return
      end


      
      
      subroutine mf_read_string( fam_id, name, string, rval )

      implicit none

      integer fam_id
      character*(*) name
      character*(*) string
      integer rval
      
      include "mili_fparam.h"
      
      external mc_read_string, EFF_LEN
      integer mc_read_string, EFF_LEN

      character*(m_max_name_len + 1) param_name
      character*(m_max_string_len + 1) param_string
      integer slen, i

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)
      
      rval = mc_read_string( fam_id, param_name, param_string )
      
      i = INDEX( param_string, CHAR(0) ) - 1
      string(1:i) = param_string(1:i)
      
      slen = LEN( string )
      
      do while ( i .lt. slen )
          i = i + 1
          string(i:i) = ' '
      end do
      
      return
      end
      
      
      
      subroutine mf_make_umesh( fam_id, name, dims, mesh_id, rval )
      
      implicit none
      
      integer fam_id
      character*(*) name
      integer dims
      integer mesh_id
      integer rval
      
      include "mili_fparam.h"
      
      external mc_make_umesh, EFF_LEN
      integer mc_make_umesh, EFF_LEN

      character*(m_max_name_len + 1) param_name
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)
      
      rval = mc_make_umesh( fam_id, param_name, dims, mesh_id )
      
      return
      end
      
      
      
      subroutine mf_def_class( fam_id, mesh_id, superclass,
     +                         short_name, long_name, rval )
      
      implicit none
      
      integer fam_id
      integer mesh_id
      integer superclass
      character*(*) short_name
      character*(*) long_name
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_class, EFF_LEN
      integer mc_def_class, EFF_LEN      

      character*(m_max_name_len + 1) param_short_name
      character*(m_max_name_len + 1) param_long_name
      integer slen

      slen = EFF_LEN( short_name, m_max_name_len  ) + 1
      param_short_name = short_name
      param_short_name(slen:slen) = CHAR(0)

      slen = EFF_LEN( long_name, m_max_name_len  ) + 1
      param_long_name = long_name
      param_long_name(slen:slen) = CHAR(0)
      
      rval = mc_def_class( fam_id, mesh_id, superclass,
     +                     param_short_name, param_long_name )
      
      return
      end
      
      
      
      subroutine mf_def_class_idents( fam_id, mesh_id, class, 
     +                                start, stop, rval )

      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class
      integer start
      integer stop
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_class_idents, EFF_LEN
      integer mc_def_class_idents, EFF_LEN
      
      character*(m_max_name_len + 1) param_class
      integer slen

      slen = EFF_LEN( class, m_max_name_len  ) + 1
      param_class = class
      param_class(slen:slen) = CHAR(0)
      
      rval = mc_def_class_idents( fam_id, mesh_id, param_class, start, 
     +                            stop )
      
      return
      end
      
      
      
      subroutine mf_def_nodes( fam_id, mesh_id, class, start_node, 
     +                         stop_node, coords, rval )
      
      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class
      integer start_node
      integer stop_node
      real coords
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_nodes, EFF_LEN
      integer mc_def_nodes, EFF_LEN
      
      character*(m_max_name_len + 1) param_class
      integer slen

      slen = EFF_LEN( class, m_max_name_len  ) + 1
      param_class = class
      param_class(slen:slen) = CHAR(0)
      
      rval = mc_def_nodes( fam_id, mesh_id, param_class, 
     +                     start_node, stop_node, coords )
      
      return
      end
      
      
      subroutine mf_def_node_labels( fam_id, mesh_id, class_name, 
     +                               qty, labels, rval )

      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer qty
      integer labels
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_node_labels, EFF_LEN
      integer  mc_def_node_labels, EFF_LEN
      
      character*(m_max_name_len + 1) param_cname
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len  ) + 1
      param_cname = class_name
      param_cname(slen:slen) = CHAR(0)
      
      rval = mc_def_node_labels( fam_id, mesh_id, param_cname, 
     +                           qty, labels )
      
      return
      end
      
      
      subroutine mf_def_conn_arb( fam_id, mesh_id, class_name, 
     +                            qty, elems, conns, rval )

      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer qty
      integer elems
      integer conns
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_conn_arb, EFF_LEN
      integer mc_def_conn_arb, EFF_LEN
      
      character*(m_max_name_len + 1) param_cname
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len  ) + 1
      param_cname = class_name
      param_cname(slen:slen) = CHAR(0)
      
      rval = mc_def_conn_arb( fam_id, mesh_id, param_cname, 
     +                        qty, elems, conns )
      
      return
      end
      
      subroutine mf_def_conn_arb_labels_global_ids( fam_id, mesh_id, 
     +                            class_name, qty, elems, labels, 
     +                            conns, global_ids, rval )

      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer qty
      integer elems
      integer labels
      integer conns
      integer global_ids
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_conn_arb_labels_global_ids, EFF_LEN
      integer mc_def_conn_arb_labels_global_ids, EFF_LEN
      
      character*(m_max_name_len + 1) param_cname
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len  ) + 1
      param_cname = class_name
      param_cname(slen:slen) = CHAR(0)
      
      rval = mc_def_conn_arb_labels_global_ids( fam_id, mesh_id, 
     +                        param_cname,qty, elems, labels, 
     +                        global_ids,  conns )
      
      return
      end

      subroutine mf_def_conn_arb_labels( fam_id, mesh_id, class_name, 
     +                            qty, elems, labels, conns, rval )

      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer qty
      integer elems
      integer labels
      integer conns
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_conn_arb_labels, EFF_LEN
      integer mc_def_conn_arb_labels, EFF_LEN
      
      character*(m_max_name_len + 1) param_cname
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len  ) + 1
      param_cname = class_name
      param_cname(slen:slen) = CHAR(0)
      
      rval = mc_def_conn_arb_labels( fam_id, mesh_id, param_cname, 
     +                        qty, elems, labels, conns )
      
      return
      end
 
      subroutine mf_def_conn_seq_labels_global_ids( fam_id, mesh_id,  
     +                       class_name,start_el, stop_el, labels, 
     +                       conns, global_ids, rval )
      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer start_el
      integer stop_el
      integer labels
      integer conns
      integer global_ids
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_conn_seq_labels_global_ids, EFF_LEN
      integer mc_def_conn_seq_labels_global_ids, EFF_LEN
      
      character*(m_max_name_len + 1) param_class_name
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len  ) + 1
      param_class_name = class_name
      param_class_name(slen:slen) = CHAR(0)
      
      rval = mc_def_conn_seq_labels_global_ids( fam_id, mesh_id,  
     +                        param_class_name, start_el, stop_el,  
     +                        labels, conns , global_ids)
      
      return
      end

      subroutine mf_def_conn_seq_labels( fam_id, mesh_id, class_name, 
     +                       start_el, stop_el, labels, conns, rval )

      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer start_el
      integer stop_el
      integer labels
      integer conns
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_conn_seq_labels, EFF_LEN
      integer mc_def_conn_seq_labels, EFF_LEN
      
      character*(m_max_name_len + 1) param_class_name
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len  ) + 1
      param_class_name = class_name
      param_class_name(slen:slen) = CHAR(0)
      
      rval = mc_def_conn_seq_labels( fam_id, mesh_id, param_class_name, 
     +                        start_el, stop_el, labels, conns )
      
      return
      end

      subroutine mf_def_seq_labels( fam_id, mesh_id, class_name, 
     +                       start_el, stop_el, labels, rval )

      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer start_el
      integer stop_el
      integer labels
      integer conns
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_seq_labels, EFF_LEN
      integer mc_def_seq_labels, EFF_LEN
      
      character*(m_max_name_len + 1) param_class_name
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len  ) + 1
      param_class_name = class_name
      param_class_name(slen:slen) = CHAR(0)
      
      rval = mc_def_seq_labels( fam_id, mesh_id, param_class_name, 
     +                        start_el, stop_el, labels)
      
      return
      end

      subroutine mf_def_conn_seq( fam_id, mesh_id, class_name, 
     +                            start_el, stop_el, conns, rval )

      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer start_el
      integer stop_el
      integer conns
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_conn_seq, EFF_LEN
      integer mc_def_conn_seq, EFF_LEN
      
      character*(m_max_name_len + 1) param_class_name
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len  ) + 1
      param_class_name = class_name
      param_class_name(slen:slen) = CHAR(0)
      
      rval = mc_def_conn_seq( fam_id, mesh_id, param_class_name, 
     +                        start_el, stop_el, conns )
      
      return
      end
        
      
      subroutine mf_def_global_ids( fam_id, mesh_id, class_name, 
     +                               qty, idents, global_ids, rval )

      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer qty
		integer idents
      integer global_ids
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_global_ids, EFF_LEN
      integer  mc_def_global_ids, EFF_LEN
      
      character*(m_max_name_len + 1) param_cname
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len  ) + 1
      param_cname = class_name
      param_cname(slen:slen) = CHAR(0)
      
      rval = mc_def_global_ids( fam_id, mesh_id, param_cname, 
     +                           qty, idents, global_ids )
      
      return
      end
      
      subroutine mf_def_conn_labels( fam_id, mesh_id, class_name, 
     +                               qty, idents, labels, rval )

      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer qty
		integer idents
      integer labels
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_conn_labels, EFF_LEN
      integer  mc_def_conn_labels, EFF_LEN
      
      character*(m_max_name_len + 1) param_cname
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len  ) + 1
      param_cname = class_name
      param_cname(slen:slen) = CHAR(0)
      
      rval = mc_def_conn_labels( fam_id, mesh_id, param_cname, 
     +                           qty, idents, labels )
      
      return
      end
      

      subroutine mf_load_conn_labels( fam_id, mesh_id, class_name, 
     +                                qty, num_blocks, block_range,
     +                                elem_ids, labels, rval )

      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer qty
      integer num_blocks
      integer block_range(*)
      integer elem_ids(*)
      integer labels(*)
      integer rval
      
      include "mili_fparam.h"
      
      external mc_load_conn_labels, EFF_LEN
      integer  mc_load_conn_labels, EFF_LEN

      character*(m_max_name_len + 1) param_cname
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len  ) + 1
      param_cname = class_name
      param_cname(slen:slen) = CHAR(0)
      
      rval = mc_load_conn_labels( fam_id, mesh_id, param_cname, 
     +                            qty, num_blocks, block_range,
     +                            elem_ids, labels )
      
      return
      end
      

      subroutine mf_def_conn_surface( fam_id, mesh_id, short_name,
     +                                qty_of_facets, conns, surface_id,
     +                                rval )

      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) short_name
      integer qty_of_facets
      integer conns
      integer surface_id
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_conn_surface, EFF_LEN
      integer mc_def_conn_surface, EFF_LEN
      
      character*(m_max_name_len + 1) param_cname
      integer slen

      slen = EFF_LEN( short_name, m_max_name_len  ) + 1
      param_cname = short_name
      param_cname(slen:slen) = CHAR(0)
      
      rval = mc_def_conn_surface( fam_id, mesh_id, param_cname,
     +                            qty_of_facets, conns, surface_id )
      
      return
      end



      subroutine mf_load_nodes( fam_id, mesh_id, class_name, coords,
     +                          rval )
      
      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      real coords
      integer rval
      
      include "mili_fparam.h"
      
      external mc_load_nodes, EFF_LEN
      integer mc_load_nodes, EFF_LEN
            
      character*(m_max_name_len + 1) param_cname
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len  ) + 1
      param_cname = class_name
      param_cname(slen:slen) = CHAR(0)
      
      rval = mc_load_nodes( fam_id, mesh_id, param_cname, coords )
      
      return
      end


      subroutine mf_load_node_labels(fam_id, mesh_id, class_name,
     +                               block_range, labels, rval)

      implicit none

      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer block_range(*)
      integer labels(*)
      integer rval

      include "mili_fparam.h"

      external mc_load_node_labels, EFF_LEN
      integer  mc_load_node_labels, EFF_LEN

      character*(m_max_name_len + 1) param_cname
      integer slen
      integer num_blocks

      slen = EFF_LEN(class_name, m_max_name_len) + 1
      param_cname = class_name
      param_cname(slen:slen) = CHAR(0)

      rval = mc_load_node_labels(fam_id, mesh_id, param_cname,
     +                           num_blocks, block_range, labels)

      return
      end



      subroutine mf_get_node_label_info(fam_id, mesh_id, class_name,
     +                                  num_blocks, num_labels, rval)
      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer num_blocks
      integer num_labels
      integer rval
      
      include "mili_fparam.h"
      
      external mc_get_node_label_info, EFF_LEN
      integer  mc_get_node_label_info, EFF_LEN
      
      character*(m_max_name_len + 1) param_cname
      integer slen

      slen = EFF_LEN(class_name, m_max_name_len) + 1
      param_cname = class_name
      param_cname(slen:slen) = CHAR(0)

      rval = mc_get_node_label_info(fam_id, mesh_id, param_cname,
     +                              num_blocks, num_labels);

      return
      end



      subroutine mf_load_conns( fam_id, mesh_id, class_name, conns, 
     +                          mats, parts, rval )
      
      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer conns(*)
      integer mats(*)
      integer parts(*)
      integer rval
      
      include "mili_fparam.h"
      
      external mc_load_conns, EFF_LEN
      integer mc_load_conns, EFF_LEN
            
      character*(m_max_name_len + 1) param_cname
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len  ) + 1
      param_cname = class_name
      param_cname(slen:slen) = CHAR(0)
      
      rval = mc_load_conns( fam_id, mesh_id, param_cname, conns, 
     +                      mats, parts )
      
      return
      end
      


      subroutine mf_load_surface( fam_id, mesh_id, class_name,
     +                            surface_id, conns, rval ) 
      
      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer surface_id
      integer conns(*)
      integer rval
      
      include "mili_fparam.h"
      
      external mc_load_surface, EFF_LEN
      integer mc_load_surface, EFF_LEN
            
      character*(m_max_name_len + 1) param_cname
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len ) + 1
      param_cname = class_name
      param_cname(slen:slen) = CHAR(0)
      
      rval = mc_load_surface( fam_id, mesh_id, surface_id, param_cname,
     +                        conns ) 
      
      return
      end
      
      
      
      subroutine mf_get_mesh_id( fam_id, name, mesh_id, rval )
      
      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) name
      integer rval
      
      include "mili_fparam.h"
      
      external mc_get_mesh_id, EFF_LEN
      integer mc_get_mesh_id, EFF_LEN

      character*(m_max_name_len + 1) param_name
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)
      
      rval = mc_get_mesh_id( fam_id, param_name, mesh_id )
      
      return
      end
      
      
      
      subroutine mf_def_svars( fam_id, qty, types, names, titles, 
     +                         rval )
      
      implicit none
      
      integer fam_id
      integer qty
      integer types(qty)
      character*(*) names
      character*(*) titles
      integer rval
      
      dimension names(qty), titles(qty)
      
      include "mili_fparam.h"
      
      external mc_def_svars, EFF_LEN
      integer mc_def_svars, EFF_LEN
      
      character*(m_max_name_len + 1) param_names(m_max_svars), 
     +                               param_titles(m_max_svars)
      integer slen
      integer i
      
      if ( qty .gt. m_max_svars ) then
          rval = m_too_many_scalars
          return
      end if
      
      do 10 i = 1, qty
      
          slen = EFF_LEN( names(i), m_max_name_len  ) + 1
          param_names(i) = names(i)
          param_names(i)(slen:slen) = CHAR(0)
          
          slen = EFF_LEN( titles(i), m_max_name_len  ) + 1
          param_titles(i) = titles(i)
          param_titles(i)(slen:slen) = CHAR(0)
          
 10   continue
 
      slen = m_max_name_len + 1
      
      rval = mc_def_svars( fam_id, qty, param_names, 
     +                     slen, param_titles, slen, types )
      
      return
      end



      subroutine mf_def_vec_svar( fam_id, type, field_qty, name, title, 
     +                            field_names, field_titles, rval )
      
      implicit none
      
      integer fam_id
      integer type
      integer field_qty
      character*(*) name
      character*(*) title
      character*(*) field_names
      character*(*) field_titles
      integer rval
      
      dimension field_names(field_qty), field_titles(field_qty)
      
      include "mili_fparam.h"

      external mc_def_vec_svar, EFF_LEN
      integer mc_def_vec_svar, EFF_LEN
      
      character*(m_max_name_len + 1) param_name
      character*(m_max_name_len + 1) param_title
      integer nlen, tlen, slen
      
      if ( field_qty .gt. m_max_svars ) then
          rval = m_too_many_fields
          return
      end if
      
      nlen = LEN( field_names(1) )
      tlen = LEN( field_titles(1) )

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)

      slen = EFF_LEN( title, m_max_name_len  ) + 1
      param_title = title
      param_title(slen:slen) = CHAR(0)
 
      slen = m_max_name_len + 1
      
      rval = mc_def_vec_svar( fam_id, type, field_qty,  
     +                        param_name, param_title, 
     +                        field_names, nlen, field_titles, tlen )

      return
      end
      
      
      
      subroutine mf_def_arr_svar( fam_id, type, order, dims,
     +                            name, title, rval )
      
      implicit none
      
      integer fam_id
      integer type
      integer order
      integer dims(order)
      character*(*) name
      character*(*) title
      integer rval
      
      include "mili_fparam.h"

      external mc_def_arr_svar, EFF_LEN
      integer mc_def_arr_svar, EFF_LEN
      
      integer c_dims(m_max_array_dims)
      integer i
      character*(m_max_name_len + 1) param_name
      character*(m_max_name_len + 1) param_title
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)

      slen = EFF_LEN( title, m_max_name_len  ) + 1
      param_title = title
      param_title(slen:slen) = CHAR(0)
      
      do 10 i = 1, order
          c_dims(i) = dims(order - i + 1)
 10   continue
 
      rval = mc_def_arr_svar( fam_id, order, c_dims, param_name, 
     +                        param_title, type )
      
      return
      end



      subroutine mf_def_vec_arr_svar( fam_id, type, order, dims,
     +                                field_qty, name, title, 
     +                                field_names, field_titles, 
     +                                rval )
      
      implicit none
      
      integer fam_id
      integer type
      integer order
      integer dims(order)
      integer field_qty
      character*(*) name
      character*(*) title
      character*(*) field_names
      character*(*) field_titles
      integer rval
      
      dimension field_names(field_qty), field_titles(field_qty)
      
      include "mili_fparam.h"

      external mc_def_vec_arr_svar, EFF_LEN
      integer mc_def_vec_arr_svar, EFF_LEN
      
      integer c_dims(m_max_array_dims)
      integer i
      character*(m_max_name_len + 1) param_name
      character*(m_max_name_len + 1) param_title
      character*(m_max_name_len + 1) param_fnames(m_max_svars), 
     +                               param_ftitles(m_max_svars)
      integer slen
      
      if ( field_qty .gt. m_max_svars ) then
          rval = m_too_many_fields
          return
      end if
      
      do 10 i = 1, field_qty
      
          slen = EFF_LEN( field_names(i), m_max_name_len  ) + 1
          param_fnames(i) = field_names(i)
          param_fnames(i)(slen:slen) = CHAR(0)
          
          slen = EFF_LEN( field_titles(i), m_max_name_len  ) + 1
          param_ftitles(i) = field_titles(i)
          param_ftitles(i)(slen:slen) = CHAR(0)
          
 10   continue

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)

      slen = EFF_LEN( title, m_max_name_len  ) + 1
      param_title = title
      param_title(slen:slen) = CHAR(0)
      
      do 20 i = 1, order
          c_dims(i) = dims(order - i + 1)
 20   continue
 
      slen = m_max_name_len + 1
      
      rval = mc_def_vec_arr_svar( fam_id, order, c_dims, 
     +                            param_name, param_title, 
     +                            field_qty, param_fnames, slen, 
     +                            param_ftitles,  slen, type )

      return
      end
            
      
          
      subroutine mf_open_srec( fam_id, mesh_id, srec_id, rval )
      
      implicit none
      
      integer fam_id
      integer mesh_id
      integer srec_id
      integer rval
      
      external mc_open_srec
      integer mc_open_srec
      
      rval = mc_open_srec( fam_id, mesh_id, srec_id )
      
      return
      end
            
       
      
      
      subroutine mf_close_srec( fam_id, srec_id, rval )
      
      implicit none
      
      integer fam_id
      integer srec_id
      integer rval
      
      external mc_close_srec
      integer mc_close_srec
      
      rval = mc_close_srec( fam_id, srec_id )
      
      return
      end
            
      
      
c.....SUPERSEDED      
      subroutine mf_make_srec( fam_id, mesh_id, srec_id, rval )
      
      implicit none
      
      integer fam_id
      integer mesh_id
      integer srec_id
      integer rval
      
      external mc_open_srec
      integer mc_open_srec
      
      rval = mc_open_srec( fam_id, mesh_id, srec_id )
      
      return
      end
      
      subroutine mf_def_subrec( fam_id, srec_id, name, org, 
     +                          qty_svars, svar_names, class, 
     +                          idformat, qty, obj_ids, rval )
      
      implicit none
      
      integer fam_id
      integer srec_id
      character*(*) name
      integer org
      integer qty_svars
      character*(*) svar_names(qty_svars)
      character*(*) class
      integer idformat
      integer qty
      integer obj_ids(*)
      integer flag
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_subrec, EFF_LEN
      integer mc_def_subrec, EFF_LEN
            
      character*(m_max_name_len + 1) param_name, param_class
      character*(m_max_name_len + 1) param_sv_names(m_max_svars)
      integer slen
      integer i
      integer svar_max

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)

      slen = EFF_LEN( class, m_max_name_len  ) + 1
      param_class = class
      param_class(slen:slen) = CHAR(0)
      
      svar_max = MIN( qty_svars, m_max_svars )
      
      do 10 i = 1, svar_max
          slen = EFF_LEN( svar_names(i), m_max_name_len  ) + 1
          param_sv_names(i) = svar_names(i)
          param_sv_names(i)(slen:slen) = CHAR(0)
 10   continue
 
      slen = m_max_name_len + 1

      flag = 0
      
      rval = mc_def_subrec( fam_id, srec_id, param_name, org, 
     +                      qty_svars, param_sv_names, slen,
     +                      param_class, idformat, qty, obj_ids, flag )
      
      return
      end
      
      
      subroutine mf_def_surf_subrec( fam_id, srec_id, name, org, 
     +                               qty_svars, svar_names, class, 
     +                               idformat, qty, obj_ids, flag,
     +                               rval )
      
      implicit none
      
      integer fam_id
      integer srec_id
      character*(*) name
      integer org
      integer qty_svars
      character*(*) svar_names(qty_svars)
      character*(*) class
      integer idformat
      integer qty
      integer obj_ids(*)
      integer flag(*)
      integer rval
      
      include "mili_fparam.h"
      
      external mc_def_subrec, EFF_LEN
      integer mc_def_subrec, EFF_LEN
            
      character*(m_max_name_len + 1) param_name, param_class
      character*(m_max_name_len + 1) param_sv_names(m_max_svars)
      integer slen
      integer i
      integer svar_max

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)

      slen = EFF_LEN( class, m_max_name_len  ) + 1
      param_class = class
      param_class(slen:slen) = CHAR(0)
      
      svar_max = MIN( qty_svars, m_max_svars )
      
      do 10 i = 1, svar_max
          slen = EFF_LEN( svar_names(i), m_max_name_len  ) + 1
          param_sv_names(i) = svar_names(i)
          param_sv_names(i)(slen:slen) = CHAR(0)
 10   continue
 
      slen = m_max_name_len + 1

      rval = mc_def_subrec( fam_id, srec_id, param_name, org, 
     +                      qty_svars, param_sv_names, slen,
     +                      param_class, idformat, qty, obj_ids, flag )
      
      return
      end
      
      subroutine mf_flush( fam_id, file_type, rval )
      
      implicit none
      
      integer fam_id
      integer file_type
      integer rval
      
      external mc_flush
      integer mc_flush
      
      rval = mc_flush( fam_id, file_type )
      
      return
      end
      
      
      
      subroutine mf_new_state( fam_id, srec_id, time, file_suffix, 
     +                         file_state_index, rval )
      
      implicit none
      
      integer fam_id
      integer srec_id
      real time
      integer file_suffix
      integer file_state_index
      integer rval
      
      external mc_new_state
      integer mc_new_state
      
      rval = mc_new_state( fam_id, srec_id, time, file_suffix,
     +                     file_state_index )
      
      return
      end
      
      
      
      subroutine mf_restart_at_state( fam_id, file_suffix, state_index,
     +                                rval )
      
      implicit none
      
      integer fam_id
      integer file_suffix
      integer state_index
      integer rval
      
      external mc_restart_at_state
      integer mc_restart_at_state
      
      rval = mc_restart_at_state( fam_id, file_suffix, state_index )
      
      return
      end
      
      
      
      subroutine mf_restart_at_file( fam_id, file_name_index, rval )
      
      implicit none
      
      integer fam_id
      integer file_name_index
      integer rval
      
      external mc_restart_at_file
      integer mc_restart_at_file
      
      rval = mc_restart_at_file( fam_id, file_name_index )
      
      return
      end
      
      
      
      subroutine mf_wrt_stream( fam_id, type, qty, data, rval )
      
      implicit none
      
      integer fam_id
      integer type
      integer qty
      real data(*)
      integer rval
      
      external mc_wrt_stream
      integer mc_wrt_stream
      
      rval = mc_wrt_stream( fam_id, type, qty, data )
      
      return
      end
      
      subroutine mf_wrt_stream_char( fam_id, qty, string, rval )
      
      implicit none
      
      integer fam_id
      integer qty
      character*(*) string
      integer rval
      
      include "mili_fparam.h"

      external mc_wrt_stream, EFF_LEN
      integer mc_wrt_stream,  EFF_LEN

      character*(m_max_name_len + 1) string_c
      integer slen

      slen = EFF_LEN( string, m_max_name_len  ) + 1
      string_c = string
      string_c(slen:slen) = CHAR(0)
      
      rval = mc_wrt_stream( fam_id, m_string, qty, string_c )
      
      return
      end
        
      
      subroutine mf_wrt_subrec( fam_id, name, start, stop, data,
     +                          rval )
      
      implicit none
      
      integer fam_id
      character*(*) name
      integer start
      integer stop
      real data(*)
      integer rval
      
      include "mili_fparam.h"
      
      external mc_wrt_subrec, EFF_LEN
      integer mc_wrt_subrec, EFF_LEN
      
      character*(m_max_name_len + 1) param_name
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)
      
      rval = mc_wrt_subrec( fam_id, param_name, start, stop, data )
      
      return
      end
      
      subroutine mf_wrt_subrec_char( fam_id, name, start, stop, string,
     +                               rval )
      
      implicit none
      
      integer fam_id
      character*(*) name
      integer start
      integer stop
      character*(*) string
      integer rval
      
      include "mili_fparam.h"
      
      external mc_wrt_subrec, EFF_LEN
      integer mc_wrt_subrec, EFF_LEN
      
      character*(m_max_name_len + 1) param_name
      integer slen

      character*(m_max_name_len + 1) string_c

      slen = EFF_LEN( string, m_max_name_len  ) + 1
      string_c = string
      string_c(slen:slen) = CHAR(0)

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      param_name = name
      param_name(slen:slen) = CHAR(0)
      
      rval = mc_wrt_subrec( fam_id, param_name, start, stop, string_c )
      
      return
      end
      
      subroutine mf_read_results( fam_id, state, subrec_id, qty,
     +                            res_names, data, rval )
      
      implicit none
      
      integer fam_id
      integer state
      integer qty
      integer subrec_id
      character*(*) res_names(qty)
      real data(*)
      integer rval
      
      include "mili_fparam.h"
      
      external mc_read_results
      integer mc_read_results
      
      integer slen

      slen = LEN( res_names(1) )
      
      rval = mc_read_results( fam_id, state, subrec_id, qty, 
     +                        res_names, slen, data )
      
      return
      end



      subroutine mf_get_svar_size(fam_id, class_name, var_name,
     +                            num_blocks, size, type, rval)

      implicit none

      integer fam_id
      character*(*) class_name
      character*(*) var_name
      integer num_blocks
      integer size
      integer type
      integer rval

      include "mili_fparam.h"

      external mc_get_svar_size, EFF_LEN
      integer mc_get_svar_size, EFF_LEN

      character*(m_max_name_len + 1) class_name_c
      character*(m_max_name_len + 1) var_name_c
      integer slen

      slen = EFF_LEN(class_name, m_max_name_len) + 1
      class_name_c = class_name
      class_name_c(slen:slen) = CHAR(0)

      slen = EFF_LEN(var_name, m_max_name_len) + 1
      var_name_c = var_name
      var_name_c(slen:slen) = CHAR(0)

      rval = mc_get_svar_size(fam_id, class_name_c, var_name_c,
     +                        num_blocks, size, type)

      return
      end



      subroutine mf_get_svar_mo_ids_on_class(fam_id, class_name,
     +                                       var_name, blocks, rval)

      implicit none

      integer fam_id
      character*(*) class_name
      character*(*) var_name
      integer blocks(*)
      integer rval

      include "mili_fparam.h"

      external mc_get_svar_mo_ids_on_class, EFF_LEN
      integer mc_get_svar_mo_ids_on_class, EFF_LEN

      character*(m_max_name_len + 1) class_name_c
      character*(m_max_name_len + 1) var_name_c
      integer slen

      slen = EFF_LEN(class_name, m_max_name_len  ) + 1
      class_name_c = class_name
      class_name_c(slen:slen) = CHAR(0)

      slen = EFF_LEN(var_name, m_max_name_len  ) + 1
      var_name_c = var_name
      var_name_c(slen:slen) = CHAR(0)

      rval = mc_get_svar_mo_ids_on_class(fam_id, class_name_c,
     +                                   var_name_c, blocks)

      return
      end



      subroutine mf_get_svar_on_class(fam_id, state, class_name,
     +                                var_name, data, rval)

      implicit none

      integer fam_id
      integer state
      character*(*) class_name
      character*(*) var_name
      real data(*)
      integer rval

      include "mili_fparam.h"

      external mc_get_svar_on_class, EFF_LEN
      integer mc_get_svar_on_class, EFF_LEN

      character*(m_max_name_len + 1) class_name_c
      character*(m_max_name_len + 1) var_name_c
      integer slen

      slen = EFF_LEN(class_name, m_max_name_len  ) + 1
      class_name_c = class_name
      class_name_c(slen:slen) = CHAR(0)

      slen = EFF_LEN(var_name, m_max_name_len  ) + 1
      var_name_c = var_name
      var_name_c(slen:slen) = CHAR(0)

      rval = mc_get_svar_on_class(fam_id, state, class_name_c,
     +                            var_name_c, data)

      return
      end
      
      
      
      subroutine mf_limit_states( fam_id, st_limit, rval )
      
      implicit none
      
      integer fam_id
      integer st_limit
      integer rval
      
      external mc_limit_states
      integer mc_limit_states
      
      rval = mc_limit_states( fam_id, st_limit )
      
      return
      end
      

      subroutine mf_limit_filesize( fam_id, filesize, rval )
      
      implicit none
      
      integer fam_id
      integer filesize
      integer rval
      
      external mc_limit_filesize
      integer mc_limit_filesize
      
      rval = mc_limit_filesize( fam_id, filesize )
      
      return
      end
       
      
      subroutine mf_suffix_width( fam_id, width, rval )
      
      implicit none
      
      integer fam_id
      integer width
      integer rval
      
      external mc_suffix_width
      integer mc_suffix_width
      
      rval = mc_suffix_width( fam_id, width )
      
      return
      end
      
      
      
      subroutine mf_print_error( preamble, err_val )
      
      implicit none
      
      character*(*) preamble
      integer err_val
      
      include "mili_fparam.h"
      
      external mc_print_error, EFF_LEN
      integer EFF_LEN
      
      character*(m_max_preamble_len + 1) param_pre
      integer slen
      
      slen = EFF_LEN( preamble, m_max_preamble_len  ) + 1
      param_pre = preamble
      param_pre(slen:slen) = CHAR(0)
      
      call mc_print_error( param_pre, err_val )
      
      return
      end


      subroutine mf_query_family( fam_id, req_type, int_args, 
     +                            char_arg, data, rval )
      
      implicit none
      
      integer fam_id
      integer req_type
      integer int_args
      character*(*) char_arg
      integer data
      integer rval
      
      include "mili_fparam.h"
      
      external mc_query_family, EFF_LEN, GET_LEN
      integer mc_query_family, EFF_LEN, GET_LEN
            
      character*(m_max_name_len + 1) param_carg
      integer slen

      slen = EFF_LEN( char_arg, m_max_name_len  ) + 1
      param_carg = char_arg
      param_carg(slen:slen) = CHAR(0)
      
      rval = mc_query_family( fam_id, req_type, int_args, 
     +                        param_carg, data )
      
      return
      end


      subroutine mf_get_class_info( fam_id, mesh_id, superclass, 
     +                              class_index, short_name, long_name, 
     +                              object_qty, rval )
      
      implicit none
      
      integer fam_id
      integer mesh_id
      integer superclass
      integer class_index
      character*(*) short_name
      character*(*) long_name
      integer object_qty
      integer rval
      
      include "mili_fparam.h"
      
      external mc_get_class_info, EFF_LEN
      integer mc_get_class_info, EFF_LEN
            
      character*(m_max_name_len + 1) param_sname
      character*(m_max_name_len + 1) param_lname
      integer slen, i
      
      rval = mc_get_class_info( fam_id, mesh_id, superclass,
     +                          class_index, param_sname, param_lname, 
     +                          object_qty )
      
      i = INDEX( param_sname, CHAR(0) ) - 1
      short_name(1:i) = param_sname(1:i)
      
      slen = LEN( short_name )
      
      do while ( i .lt. slen )
          i = i + 1
          short_name(i:i) = ' '
      end do
      
      i = INDEX( param_lname, CHAR(0) ) - 1
      long_name(1:i) = param_lname(1:i)
      
      slen = LEN( long_name )
      
      do while ( i .lt. slen )
          i = i + 1
          long_name(i:i) = ' '
      end do
      
      return
      end
      


      subroutine mf_get_simple_class_info( fam_id, mesh_id, 
     +                                     superclass, 
     +                                     class_index, 
     +                                     short_name, long_name, 
     +                                     start_ident, stop_ident, 
     +                                     rval )
      
      implicit none
      
      integer fam_id
      integer mesh_id
      integer superclass
      integer class_index
      character*(*) short_name
      character*(*) long_name
      integer start_ident
      integer stop_ident
      integer rval
      
      include "mili_fparam.h"
      
      external mc_get_simple_class_info, EFF_LEN
      integer mc_get_simple_class_info, EFF_LEN
            
      character*(m_max_name_len + 1) param_sname
      character*(m_max_name_len + 1) param_lname
      integer slen, i
      
      rval = mc_get_simple_class_info( fam_id, mesh_id, superclass,
     +                                 class_index, 
     +                                 param_sname, param_lname, 
     +                                 start_ident, stop_ident )
      
      i = INDEX( param_sname, CHAR(0) ) - 1
      short_name(1:i) = param_sname(1:i)
      
      slen = LEN( short_name )
      
      do while ( i .lt. slen )
          i = i + 1
          short_name(i:i) = ' '
      end do
      
      i = INDEX( param_lname, CHAR(0) ) - 1
      long_name(1:i) = param_lname(1:i)
      
      slen = LEN( long_name )
      
      do while ( i .lt. slen )
          i = i + 1
          long_name(i:i) = ' '
      end do
      
      return
      end      
      
      
      
      subroutine mf_set_buffer_qty( fam_id, mesh_id, class_name, 
     +                              buf_qty, rval )
      
      implicit none
      
      integer fam_id
      integer mesh_id
      character*(*) class_name
      integer buf_qty
      integer rval
      
      include "mili_fparam.h"
      
      external mc_set_buffer_qty, EFF_LEN
      integer mc_set_buffer_qty, EFF_LEN      

      character*(m_max_name_len + 1) param_name
      integer slen

      slen = EFF_LEN( class_name, m_max_name_len  ) + 1
      param_name = class_name
      param_name(slen:slen) = CHAR(0)
      
      rval = mc_set_buffer_qty( fam_id, mesh_id, param_name, buf_qty )
      
      return
      end


c ************************************************************************
c
c   New TI function  wrappers - added October 2006: EMP
c
c ************************************************************************
      subroutine mf_ti_undef_class( fam_id, rval )

      implicit none
      
      integer fam_id
      integer rval

      include "mili_fparam.h"

      external mc_ti_undef_class
      integer  mc_ti_undef_class
      
      rval = mc_ti_undef_class( fam_id )

      return
      end


c ************************************************************************
      subroutine mf_ti_def_class( fam_id, meshid, state, matid,
     +                            superclass, meshvar, nodal,
     +                            short_name, long_name, rval ) 

      implicit none
      
      integer fam_id
      integer meshid
      integer state
      integer matid
      character*(*) superclass
      integer meshvar
      integer nodal
      character*(*) short_name
      character*(*) long_name
      integer rval

      include "mili_fparam.h"

      external mc_ti_def_class, EFF_LEN
      integer  mc_ti_def_class, EFF_LEN

      character*(m_max_name_len + 1) c_superclass
      character*(m_max_name_len + 1) c_short_name
      character*(m_max_name_len + 1) c_long_name
      integer slen

      slen = EFF_LEN( superclass, m_max_name_len  ) + 1
      c_superclass = superclass
      c_superclass(slen:slen) = CHAR(0)

      slen = EFF_LEN( short_name, m_max_name_len  ) + 1
      c_short_name = short_name
      c_short_name(slen:slen) = CHAR(0)

      slen = EFF_LEN( long_name, m_max_name_len  ) + 1
      c_long_name = long_name
      c_long_name(slen:slen) = CHAR(0)

      rval = mc_ti_def_class(fam_id, meshid, state, matid,
     +                       c_superclass, meshvar, nodal,
     +                       c_short_name, c_long_name )

      return
      end


c ************************************************************************
      subroutine mf_ti_set_class( fam_id, meshid, state, matid,
     +                            superclass, meshvar, nodal,
     +                            short_name, long_name, rval ) 

      implicit none
      
      integer fam_id
      integer meshid
      integer state
      integer matid
      character*(*) superclass
      integer meshvar
      integer nodal
      character*(*) short_name
      character*(*) long_name
      integer rval

      include "mili_fparam.h"

      external mc_ti_set_class, EFF_LEN
      integer  mc_ti_set_class, EFF_LEN

      character*(m_max_name_len + 1) c_superclass
      character*(m_max_name_len + 1) c_short_name
      character*(m_max_name_len + 1) c_long_name
      integer slen

      slen = EFF_LEN( superclass, m_max_name_len  ) + 1
      c_superclass = superclass
      c_superclass(slen:slen) = CHAR(0)

      slen = EFF_LEN( short_name, m_max_name_len  ) + 1
      c_short_name = short_name
      c_short_name(slen:slen) = CHAR(0)

      slen = EFF_LEN( long_name, m_max_name_len  ) + 1
      c_long_name = long_name
      c_long_name(slen:slen) = CHAR(0)

      rval = mc_ti_set_class(fam_id, meshid, state, matid,
     +                       c_superclass, meshvar, nodal,
     +                       c_short_name, c_long_name )

      return
      end


c ************************************************************************
      subroutine mf_ti_enable( fam_id )

      implicit none
      integer fam_id
      
      include "mili_fparam.h"

      external mc_ti_enable

      call mc_ti_enable( fam_id )

      return
      end


c ************************************************************************
      subroutine mf_ti_disable( fam_id )

      implicit none
      integer fam_id
    
      include "mili_fparam.h"

      external mc_ti_disable

      call mc_ti_disable( fam_id )

      return
      end


c ************************************************************************
      subroutine mf_ti_make_var_name( fam_id, name, class,
     +                                new_name, rval )
      implicit none
      
      integer fam_id
      character*(*) name
      character*(*) class
      character*(*) new_name
      integer rval

      include "mili_fparam.h"

      external mc_ti_make_var_name, EFF_LEN
      integer  mc_ti_make_var_name, EFF_LEN

      character*(m_max_name_len + 1) c_name
      character*(m_max_name_len + 1) c_class
      character*(m_max_name_len + 1) string
      integer i
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      c_name = name
      c_name(slen:slen) = CHAR(0)

      slen = EFF_LEN( class, m_max_name_len  ) + 1
      c_class = class
      c_class(slen:slen) = CHAR(0)

      rval = mc_ti_make_var_name( fam_id, c_name, c_class, string )

      i = INDEX( string, CHAR(0) ) - 1
      new_name(1:i) = string(1:i)

      slen = LEN( new_name )

      do while ( i .lt. slen )
          i = i + 1
          new_name(i:i) = ' '
      end do

      return
      end


c ************************************************************************
      subroutine mf_ti_wrt_scalar( fam_id, type, name, value, rval )

      implicit none
      
      integer fam_id
      integer type
      character*(*) name
      integer value
      integer rval

      include "mili_fparam.h"

      external mc_ti_wrt_scalar, EFF_LEN
      integer  mc_ti_wrt_scalar, EFF_LEN

      character*(m_max_name_len + 1) c_name
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      c_name = name
      c_name(slen:slen) = CHAR(0)

      rval = mc_ti_wrt_scalar( fam_id, type, c_name, value )

      return
      end


c ************************************************************************
      subroutine mf_ti_read_scalar( fam_id, name, value, rval)

      implicit none
      
      integer fam_id
      integer value
      character*(*) name
      integer rval

      include "mili_fparam.h"

      external mc_ti_read_scalar, EFF_LEN
      integer  mc_ti_read_scalar, EFF_LEN

      character*(m_max_name_len + 1) c_name
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      c_name = name
      c_name(slen:slen) = CHAR(0)

      rval = mc_ti_read_scalar( fam_id, c_name, value )

      return
      end


c ************************************************************************
      subroutine mf_ti_wrt_string( fam_id, name, string, rval )
      implicit none
      
      integer fam_id
      character*(*) name
      character*(*) string
      integer rval

      external mc_ti_wrt_string, EFF_LEN
      integer mc_ti_wrt_string, EFF_LEN

      include "mili_fparam.h"

      character*(m_max_name_len + 1) c_name
      character*(m_max_string_len + 1) c_string
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      c_name = name
      c_name(slen:slen) = CHAR(0)

      slen = EFF_LEN( string, m_max_string_len  ) + 1
      c_string = string
      c_string(slen:slen) = CHAR(0)
      
      rval = mc_ti_wrt_string( fam_id, c_name, c_string )
      
      return
      end


c ************************************************************************
      subroutine mf_ti_read_string( fam_id, name, string, rval )

      implicit none
      
      integer fam_id
      character*(*) name
      character*(*)string
      integer rval

      include "mili_fparam.h"

      external mc_ti_read_string, EFF_LEN
      integer mc_ti_read_string, EFF_LEN

      character*(m_max_name_len + 1) c_name
      character*(m_max_string_len + 1) c_string
      integer slen, i

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      c_name = name
      c_name(slen:slen) = CHAR(0)

      rval = mc_ti_read_string( fam_id, c_name, c_string )

      i = INDEX( c_string, CHAR(0) ) - 1
      string(1:i) = c_string(1:i)

      slen = LEN( string )

      do while ( i .lt. slen )
          i = i + 1
          string(i:i) = ' '
      end do

      return
      end


c ************************************************************************
      subroutine mf_ti_wrt_array( fam_id, type, name, order, dims,
     +                            values, rval )

      implicit none
      
      integer fam_id
      integer type
      character*(*) name
      integer order
      integer dims(order)
      integer values
      integer rval

      include "mili_fparam.h"
            
      external mc_ti_wrt_array, EFF_LEN
      integer mc_ti_wrt_array, EFF_LEN

      integer c_dims(m_max_array_dims)
      integer i
      character*(m_max_name_len + 1) c_name
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      c_name = name
      c_name(slen:slen) = CHAR(0)
      
      do 10 i = 1, order
          c_dims(i) = dims(order - i + 1)
 10   continue

      rval = mc_ti_wrt_array( fam_id, type, c_name, order, 
     +                        c_dims, values )

      return
      end


c ************************************************************************
      subroutine mf_ti_read_array( fam_id, name, values, rval,
     +                             num_items_read )
      implicit none
      
      integer fam_id
      character*(*) name
      integer values(*)
      integer rval
      integer num_items_read

      include "mili_fparam.h"
            
      external mc_ti_read_array, EFF_LEN
      integer mc_ti_read_array, EFF_LEN

      character*(m_max_name_len + 1) c_name
      integer slen

      slen = EFF_LEN( name, m_max_name_len  ) + 1
      c_name = name
      c_name(slen:slen) = CHAR(0)

      rval = mc_ti_read_array( fam_id, c_name, values, num_items_read )

      return
      end

c ************************************************************************
      subroutine mf_get_metadata( fam_id,  mili_version, host, arch,
     +                            timestamp, xmilics_version, rval )

      implicit none

      integer fam_id
      character*(*) mili_version
      character*(*) host
      character*(*) arch
      character*(*) timestamp
      character*(*) xmilics_version
      integer rval
      
      include "mili_fparam.h"
      
      external mc_get_metadata, EFF_LEN
      integer  mc_get_metadata, EFF_LEN

      character*(m_max_string_len + 1) param_mili_version
      character*(m_max_string_len + 1) param_host
      character*(m_max_string_len + 1) param_arch
      character*(m_max_string_len + 1) param_timestamp
      character*(m_max_string_len + 1) param_xmilics_version
      integer slen
      integer i

      rval = mc_get_metadata( fam_id, param_mili_version,
     +                        param_host,
     +                        param_arch, param_timestamp,
     +                        param_xmilics_version)

      i = INDEX( param_mili_version, CHAR(0) ) - 1
      mili_version(1:i) = param_mili_version(1:i)

      slen = LEN( mili_version )

      do while ( i .lt. slen )
          i = i + 1
          mili_version(i:i) = ' '
      end do

      i = INDEX( param_host, CHAR(0) ) - 1
      host(1:i) = param_host(1:i)

      slen = LEN( host )

      do while ( i .lt. slen )
          i = i + 1
          host(i:i) = ' '
      end do

      i = INDEX( param_arch, CHAR(0) ) - 1
      arch(1:i) = param_arch(1:i)

      slen = LEN( arch )

      do while ( i .lt. slen )
          i = i + 1
          arch(i:i) = ' '
      end do

      i = INDEX( param_timestamp, CHAR(0) ) - 1
      timestamp(1:i) = param_timestamp(1:i)

      slen = LEN( timestamp )

      do while ( i .lt. slen )
          i = i + 1
          timestamp(i:i) = ' '
      end do

      i = INDEX( param_xmilics_version, CHAR(0) ) - 1
      xmilics_version(1:i) = param_xmilics_version(1:i)

      slen = LEN( xmilics_version )

      do while ( i .lt. slen )
          i = i + 1
          xmilics_version(i:i) = ' '
      end do

      return
      end

c ************************************************************************
      subroutine mf_ti_get_metadata( fam_id,  mili_version, host, arch,
     +                               timestamp, username,
     +                               xmilics_version, code_name, rval )

      implicit none

      integer fam_id
      character*(*) mili_version
      character*(*) host
      character*(*) arch
      character*(*) timestamp
      character*(*) username
      character*(*) xmilics_version
      character*(*) code_name
      integer rval
      
      include "mili_fparam.h"
      
      external mc_ti_get_metadata, EFF_LEN
      integer  mc_ti_get_metadata, EFF_LEN

      character*(m_max_string_len + 1) param_mili_version
      character*(m_max_string_len + 1) param_host
      character*(m_max_string_len + 1) param_arch
      character*(m_max_string_len + 1) param_timestamp
      character*(m_max_string_len + 1) param_username
      character*(m_max_string_len + 1) param_xmilics_version
      character*(m_max_string_len + 1) param_code_name
      integer slen
      integer i

      rval = mc_ti_get_metadata( fam_id, param_mili_version,
     +                           param_host, param_arch,
     +                           param_timestamp, param_username,
     +                           param_xmilics_version,
     +                           param_code_name)

      i = INDEX( param_mili_version, CHAR(0) ) - 1
      mili_version(1:i) = param_mili_version(1:i)

      slen = LEN( mili_version )

      do while ( i .lt. slen )
          i = i + 1
          mili_version(i:i) = ' '
      end do

      i = INDEX( param_host, CHAR(0) ) - 1
      host(1:i) = param_host(1:i)

      slen = LEN( host )

      do while ( i .lt. slen )
          i = i + 1
          host(i:i) = ' '
      end do

      i = INDEX( param_arch, CHAR(0) ) - 1
      arch(1:i) = param_arch(1:i)

      slen = LEN( arch )

      do while ( i .lt. slen )
          i = i + 1
          arch(i:i) = ' '
      end do

      i = INDEX( param_timestamp, CHAR(0) ) - 1
      timestamp(1:i) = param_timestamp(1:i)

      slen = LEN( timestamp )

      do while ( i .lt. slen )
          i = i + 1
          timestamp(i:i) = ' '
      end do

      i = INDEX( param_username, CHAR(0) ) - 1
      username(1:i) = param_username(1:i)

      slen = LEN( username )

      do while ( i .lt. slen )
          i = i + 1
          username(i:i) = ' '
      end do

      i = INDEX( param_xmilics_version, CHAR(0) ) - 1
      xmilics_version(1:i) = param_xmilics_version(1:i)

      slen = LEN( xmilics_version )

      do while ( i .lt. slen )
          i = i + 1
          xmilics_version(i:i) = ' '
      end do

      i = INDEX( param_code_name, CHAR(0) ) - 1
      code_name(1:i) = param_code_name(1:i)

      slen = LEN( code_name )

      do while ( i .lt. slen )
          i = i + 1
          code_name(i:i) = ' '
      end do

      return
      end


c ************************************************************************
      subroutine mf_ti_check_arch( fam_id, rval )
      implicit none

      integer fam_id

      integer rval
      
      include "mili_fparam.h"
      
      external mc_ti_check_arch, EFF_LEN
      integer  mc_ti_check_arch, EFF_LEN

      rval = mc_ti_check_arch( fam_id )
      
      return
      end
