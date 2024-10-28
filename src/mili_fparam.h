!.....MILI parameters include file

! Copyright (c) 2016, Lawrence Livermore National Security, LLC. 
! Produced at the Lawrence Livermore National Laboratory. Written 
! by Kevin Durrenberger: durrenberger1@llnl.gov. CODE-OCEC-16-056. 
! All rights reserved.

! This file is part of Mili. For details, see <URL describing code 
! and how to download source>.

! Please also read this link-- Our Notice and GNU Lesser General 
! Public License.

! This program is free software; you can redistribute it and/or modify 
! it under the terms of the GNU General Public License (as published by 
! the Free Software Foundation) version 2.1 dated February 1999.

! This program is distributed in the hope that it will be useful, but 
! WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF 
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms 
! and conditions of the GNU General Public License for more details.
 
! You should have received a copy of the GNU Lesser General Public License 
! along with this program; if not, write to the Free Software Foundation, 
! Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 
! This work was produced at the University of California, Lawrence 
! Livermore National Laboratory (UC LLNL) under contract no. 
! W-7405-ENG-48 (Contract 48) between the U.S. Department of Energy 
! (DOE) and The Regents of the University of California (University) 
! for the operation of UC LLNL. Copyright is reserved to the University 
! for purposes of controlled dissemination, commercialization through 
! formal licensing, or other disposition under terms of Contract 48; 
! DOE policies, regulations and orders; and U.S. statutes. The rights 
! of the Federal Government are reserved under Contract 48 subject to 
! the restrictions agreed upon by the DOE and University as allowed 
! under DOE Acquisition Letter 97-1.
! 
! 
! DISCLAIMER
! 
! This work was prepared as an account of work sponsored by an agency 
! of the United States Government. Neither the United States Government 
! nor the University of California nor any of their employees, makes 
! any warranty, express or implied, or assumes any liability or 
! responsibility for the accuracy, completeness, or usefulness of any 
! information, apparatus, product, or process disclosed, or represents 
! that its use would not infringe privately-owned rights.  Reference 
! herein to any specific commercial products, process, or service by 
! trade name, trademark, manufacturer or otherwise does not necessarily 
! constitute or imply its endorsement, recommendation, or favoring by 
! the United States Government or the University of California. The 
! views and opinions of authors expressed herein do not necessarily 
! state or reflect those of the United States Government or the 
! University of California, and shall not be used for advertising or 
! product endorsement purposes.


     
!.....MILI data types

!     m_string           Character string
!     m_real             Real (default size)
!     m_real4            Real, 32-bit
!     m_real8            Real, 64-bit
!     m_int              Integer (default size)
!     m_int4             Integer, 32-bit
!     m_int8             Integer, 64-bit
   
!.....MILI object data superclasses.

!     m_unit             Generic object
!     m_node             1 node
!     m_truss            2 nodes
!     m_beam             3 nodes
!     m_tri              3 nodes
!     m_quad             4 nodes
!     m_tet              4 nodes
!     m_pyramid          5 nodes
!     m_wedge            6 nodes
!     m_hex              8 nodes
!     m_surf             4 nodes
!     m_particle         1 node
!     m_inode            1 node
!     m_mat              Material
!     m_mesh             Class for global state data (all nodes)

!.....MILI state data organization options

!     m_result_ordered   All instances of one result contiguous
!     m_object_ordered   All results for one object contiguous (default)

!.....MILI information categories; the two branches of a MILI family

!     m_state_data       State record data
!     m_non_state_data   Everything else

!.....MILI family information request types

!     m_qty_states          Current quantity of state records in family
!     m_qty_dimensions      Dimensionality of mesh(es) in family
!     m_qty_meshes          Quantity of meshes defined in family
!     m_qty_srec_fmts       Quantity of state record formats defined
!     m_qty_subrecs         Quantity of subrecords in a state rec format
!     m_qty_subrec_svars    Quantity of state variables in a subrecord
!     m_qty_svars           Quantity of state variables defined in a family
!     m_qty_node_blks       Quantity of nodal coordinate blocks for a mesh
!     m_qty_nodes_in_blk    Quantity of nodes defined in a particular block
!     m_qty_class_in_sclass Quantity of classes in a particular superclass
!     m_qty_elem_conn_defs  Quantity of element conn def's in a class
!     m_qty_elems_in_def    Quantity of elements defined in a single conn def
!     m_srec_fmt_id         State record format ident of given state record
!     m_series_srec_fmts    State record format idents of a series of records
!     m_subrec_class        Mesh object class of a subrecord
!     m_srec_mesh           Ident of mesh a state record format belongs to
!     m_class_superclass    Superclass of a mesh object class
!     m_state_time          Time value for a particular state record
!     m_series_times        Time values for a continuous series of records
!     m_multiple_times      Time values for an arbitrary sequence of records
!     m_state_of_time       Indices of states bounding a time value
!     m_next_state_loc      Location indices of state after input state
!     m_class_exists        Logical test if named class exists
!     m_lib_version         Value of "mili_version" string
!     m_state_size          Size (in bytes) of state record
!     m_qty_facets_in_surface

!.....MILI limits

!     m_max_svars        200 state vars defined in one mf_def_svars() call
!     m_max_array_dims   6 dimensions max in an array
!     m_max_name_len     256 characters in any name (state var, etc.)
!     m_max_path_len     128 characters in directory path specification
!     m_max_preamble_len 128 characters in mf_print_err() preamble text
!     m_max_string_len   512 characters in a named string parameter

!.....MILI formats for specifying elements in subrec definition

!     m_list_obj_fmt     All mesh objects listed explicitly
!     m_block_obj_fmt    Mesh objects referenced as list of first/last pairs

!.....MILI formats for specifying elements in subrec definition

!     m_per_object
!     m_per_facet

!.....MILI miscellaneous

!     m_dont_care        Numeric placeholder
!     m_blank            Character placeholder

!.....MILI Fortran-API detected error conditions

!     m_too_many_scalars     Attempt to exceed maximum allowable number of
!                            scalar state variable definitions in one call
!                            to mf_def_svars().
!     m_too_many_fields      Attempt to define a list array state variable
!                            with more than m_max_svars fields.


      integer                                                           &
     &    m_string, m_int, m_real, m_int4, m_int8, m_real4,             &
     &    m_real8, m_unit, m_node, m_truss, m_beam, m_tri, m_quad,      &
     &    m_tet, m_pyramid, m_wedge, m_hex, m_mat, m_mesh, m_surf,      &
     &    m_particle, m_tet10, m_inode, m_class_count, m_state_data,    &
     &    m_non_state_data,                                             &
     &    m_result_ordered, m_object_ordered,                           &
     &    m_qty_states, m_qty_dimensions, m_qty_meshes,                 &
     &    m_qty_srec_fmts, m_qty_subrecs, m_qty_subrec_svars,           &
     &    m_qty_svars, m_qty_node_blks, m_qty_nodes_in_blk,             &
     &    m_qty_class_in_sclass, m_qty_elem_conn_defs,                  &
     &    m_qty_elems_in_def, m_srec_fmt_id, m_series_srec_fmts,        &
     &    m_subrec_class, m_srec_mesh, m_class_superclass,              &
     &    m_state_time, m_series_times, m_multiple_times,               &
     &    m_state_of_time, m_next_state_loc,                            &
     &    m_class_exists, m_lib_version,                                &
     &    m_state_size, m_qty_facets_in_surface,                        &
     &    m_max_svars, m_max_array_dims, m_max_name_len,                &
     &    m_max_path_len, m_max_preamble_len, m_max_string_len,         & 
     &    m_too_many_scalars, m_too_many_fields, m_list_obj_fmt,        &
     &    m_block_obj_fmt, m_per_object, m_per_facet, m_dont_care,      &
     &    m_time_of_state, m_mesh_name

      character*(1) m_blank


!.....Read 'em if you need to, but don't mess with these values!

      parameter (                                                       &
     &    m_string = 1, m_real = 2, m_real4 = 3, m_real8 = 4,           &
     &    m_int = 5, m_int4 = 6, m_int8 = 7,                            &

     &    m_unit = 0, m_node = 1, m_truss = 2, m_beam = 3, m_tri = 4,   &
     &    m_quad = 5, m_tet = 6, m_pyramid = 7, m_wedge = 8, m_hex = 9, &
     &    m_mat = 10, m_mesh = 11, m_surf = 12, m_particle = 13,        &
     &    m_tet10 = 14, m_inode = 15, m_class_count = 16  ,             &

     &    m_result_ordered = 0, m_object_ordered = 1,                   &

     &    m_state_data = 0, m_non_state_data = 1,                       &

     &    m_qty_states = 0, m_qty_dimensions = 1, m_qty_meshes = 2,     &
     &    m_qty_srec_fmts = 3, m_qty_subrecs = 4,                       &
     &    m_qty_subrec_svars = 5, m_qty_svars = 6, m_qty_node_blks = 7, &
     &    m_qty_nodes_in_blk = 8, m_qty_class_in_sclass = 9,            &
     &    m_qty_elem_conn_defs = 10, m_qty_elems_in_def = 11,           &
     &    m_srec_fmt_id = 12, m_series_srec_fmts = 13,                  &
     &    m_subrec_class = 14, m_srec_mesh = 15,                        &
     &    m_class_superclass = 16, m_state_time = 17,                   &
     &    m_series_times = 18, m_multiple_times = 19,                   &
     &    m_state_of_time = 20, m_next_state_loc = 21,                  &
     &    m_class_exists = 22, m_lib_version = 23,                      &
     &    m_state_size = 24, m_qty_facets_in_surface = 25,              &
     &    m_time_of_state = 26, m_mesh_name = 27,                       &

     &    m_max_svars = 200, m_max_array_dims = 6, m_max_name_len=256,  &
     &    m_max_path_len = 128, m_max_preamble_len = 128,               &
     &    m_max_string_len = 512,                                       &

     &    m_list_obj_fmt = 1, m_block_obj_fmt = 2,                      &
     &    m_per_object = 0, m_per_facet = 1,                            &

     &    m_dont_care = 0, m_blank = ' ',                               &
      
     &    m_too_many_scalars = 1, m_too_many_fields = 2 )



