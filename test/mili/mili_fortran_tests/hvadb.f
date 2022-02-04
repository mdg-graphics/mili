c $Id: hvadb.f,v 1.1 2021/07/23 16:03:03 rhathaw Exp $

c This work was produced at the University of California, Lawrence 
c Livermore National Laboratory (UC LLNL) under contract no. 
c W-7405-ENG-48 (Contract 48) between the U.S. Department of Energy 
c (DOE) and The Regents of the University of California (University) 
c for the operation of UC LLNL. Copyright is reserved to the University 
c for purposes of controlled dissemination, commercialization through 
c formal licensing, or other disposition under terms of Contract 48; 
c DOE policies, regulations and orders; and U.S. statutes. The rights 
c of the Federal Government are reserved under Contract 48 subject to 
c the restrictions agreed upon by the DOE and University as allowed 
c under DOE Acquisition Letter 97-1.
c 
c 
c DISCLAIMER
c 
c This work was prepared as an account of work sponsored by an agency 
c of the United States Government. Neither the United States Government 
c nor the University of California nor any of their employees, makes 
c any warranty, express or implied, or assumes any liability or 
c responsibility for the accuracy, completeness, or usefulness of any 
c information, apparatus, product, or process disclosed, or represents 
c that its use would not infringe privately-owned rights.  Reference 
c herein to any specific commercial products, process, or service by 
c trade name, trademark, manufacturer or otherwise does not necessarily 
c constitute or imply its endorsement, recommendation, or favoring by 
c the United States Government or the University of California. The 
c views and opinions of authors expressed herein do not necessarily 
c state or reflect those of the United States Government or the 
c University of California, and shall not be used for advertising or 
c product endorsement purposes.


      program hvadb
      
      implicit none

c.....SHOULD include this, or extract necessary values somehow/somewhere      
      include "mili_fparam.h"
      
      external
     +    mf_open, mf_close, mf_wrt_scalar, mf_wrt_array, 
     +    mf_wrt_string, mf_make_umesh, mf_def_class, mf_def_nodes, 
     +    mf_def_class_idents, mf_def_conn_seq, mf_def_conn_arb, 
     +    mf_get_mesh_id, mf_def_svars, mf_def_arr_svar, 
     +    mf_open_srec, mf_def_subrec, mf_flush, mf_new_state, 
     +    mf_wrt_stream, mf_wrt_subrec, mf_limit_states, 
     +    mf_suffix_width, mf_print_error, mf_def_vec_arr_svar,
     +    mf_def_vec_svar, mf_close_srec
     
      integer fid, mesh_id, srec_id, suffix, st_idx
      integer stat
      real time
c     integer obj_blocks(2,1)
      integer obj_blocks(2,5)

c.....Initialize a object_ordered state record with numbers that will
c     distinguish each subrecord and each "lump".  Size to fit format
c     specified below.
      real state_record1(210)
      data state_record1 /
c                                                           Node Positions
     +    0.0, 0.0, 0.0,  1.0, 0.0, 0.0,  3.0, 0.0, 0.0,  6.0, 0.0, 0.0, 
     +    0.0, 1.0, 0.0,  1.0, 1.0, 0.0,  3.0, 1.0, 0.0,  6.0, 1.0, 0.0, 
     +    0.0, 0.0, 1.0,  1.0, 0.0, 1.0,  3.0, 0.0, 1.0,  6.0, 0.0, 1.0, 
     +    0.0, 1.0, 1.0,  1.0, 1.0, 1.0,  3.0, 1.0, 1.0,  6.0, 1.0, 1.0,
c                                                          Node Velocities
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2,
c                                                       Node Accelerations
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,
c                                                              Brick vars
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0,
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0,
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0 /
      real state_record2(210)
      data state_record2 /
c                                                           Node Positions
     +    0.0, 0.0, 0.2,  1.0, 0.0, 0.0,  3.0, 0.0, 0.0,  6.0, 0.0, 0.2, 
     +    0.0, 1.0, 0.2,  1.0, 1.0, 0.0,  3.0, 1.0, 0.0,  6.0, 1.0, 0.2, 
     +    0.0, 0.0, 1.2,  1.0, 0.0, 1.0,  3.0, 0.0, 1.0,  6.0, 0.0, 1.2, 
     +    0.0, 1.0, 1.2,  1.0, 1.0, 1.0,  3.0, 1.0, 1.0,  6.0, 1.0, 1.2,
c                                                          Node Velocities
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2,
c                                                       Node Accelerations
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,
c                                                              Brick vars
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0,
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0,
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0 /
      real state_record3(210)
      data state_record3 /
c                                                           Node Positions
     +    0.0, 0.0, 0.4,  1.0, 0.0, 0.0,  3.0, 0.0, 0.0,  6.0, 0.0, 0.4, 
     +    0.0, 1.0, 0.4,  1.0, 1.0, 0.0,  3.0, 1.0, 0.0,  6.0, 1.0, 0.4, 
     +    0.0, 0.0, 1.4,  1.0, 0.0, 1.0,  3.0, 0.0, 1.0,  6.0, 0.0, 1.4, 
     +    0.0, 1.0, 1.4,  1.0, 1.0, 1.0,  3.0, 1.0, 1.0,  6.0, 1.0, 1.4,
c                                                          Node Velocities
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2,
c                                                       Node Accelerations
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,
c                                                              Brick vars
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0,
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0,
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0 /
      real state_record4(210)
      data state_record4 /
c                                                           Node Positions
     +    0.0, 0.0, 0.6,  1.0, 0.0, 0.0,  3.0, 0.0, 0.0,  6.0, 0.0, 0.6, 
     +    0.0, 1.0, 0.6,  1.0, 1.0, 0.0,  3.0, 1.0, 0.0,  6.0, 1.0, 0.6, 
     +    0.0, 0.0, 1.6,  1.0, 0.0, 1.0,  3.0, 0.0, 1.0,  6.0, 0.0, 1.6, 
     +    0.0, 1.0, 1.6,  1.0, 1.0, 1.0,  3.0, 1.0, 1.0,  6.0, 1.0, 1.6,
c                                                          Node Velocities
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2,
c                                                       Node Accelerations
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,
c                                                              Brick vars
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0,
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0,
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0 /
      real state_record5(210)
      data state_record5 /
c                                                           Node Positions
     +    0.0, 0.0, 0.8,  1.0, 0.0, 0.0,  3.0, 0.0, 0.0,  6.0, 0.0, 0.8, 
     +    0.0, 1.0, 0.8,  1.0, 1.0, 0.0,  3.0, 1.0, 0.0,  6.0, 1.0, 0.8, 
     +    0.0, 0.0, 1.8,  1.0, 0.0, 1.0,  3.0, 0.0, 1.0,  6.0, 0.0, 1.8, 
     +    0.0, 1.0, 1.8,  1.0, 1.0, 1.0,  3.0, 1.0, 1.0,  6.0, 1.0, 1.8,
c                                                          Node Velocities
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2, 
     +    0.0, 0.0, 0.2,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.2,
c                                                       Node Accelerations
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,
c                                                              Brick vars
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0,
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0,
     +    7 * 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
     +    11.0, 12.0, 13.0, 14.0, 15.0 /


c.....State variables per mesh object type

c.....All short names MUST be unique
      
c.....Nodal state variables
      character*(m_max_name_len) nodal_short(12), nodal_long(12)
      data 
     +    nodal_short /
     +        'nodpos', 'nodvel', 'nodacc', 'ux', 'uy', 'uz', 
     +        'vx', 'vy', 'vz', 'ax', 'ay', 'az' /,
     +    nodal_long /
     +        'Node Position', 'Node Velocity', 'Node Acceleration', 
     +        'X Position', 'Y Position', 'Z Position',
     +        'X Velocity', 'Y Velocity', 'Z Velocity',
     +        'X Acceleration', 'Y Acceleration', 'Z Acceleration' /

c.....Hex state variables
      character*(m_max_name_len) hex_short(9), hex_long(9)
      data
     +    hex_short / 
     +        'stress', 'eeff', 'va', 'sx', 'sy', 'sz', 'sxy', 'syz', 
     +        'szx' /,
     +    hex_long /
     +        'Stress', 'Eff plastic strain', 'VectorArray', 
     +        'Sigma-xx', 'Sigma-yy', 'Sigma-zz', 'Sigma-xy', 
     +        'Sigma-yz', 'Sigma-zx' /

c.....Vector array components
      character*(m_max_name_len) va_short(5), va_long(5)
      data
     +    va_short / 
     +        'vac1', 'vac2', 'vac3', 'vac4', 'vac5' /,
     +    va_long /
     +        'VAComp1', 'VAComp2', 'VAComp3', 'VAComp4', 
     +        'VAComp5' /

c.....State var types; they're all floats, so just define one array as big
c.....as the largest block of svars that will be defined in one call
      integer svar_types(7)
      data svar_types / 7*m_real /

c.....Group names, to be used when defining connectivities and when
c.....defining subrecords
      character*(m_max_name_len) global_s, global_l, nodal_s, nodal_l, 
     +                           hexs_s, hexs_l, material_s, 
     +                           material_l
      data global_s, global_l, nodal_s, nodal_l, hexs_s, hexs_l, 
     +     material_s, material_l /
     +        'g', 'Global', 'node', 'Nodal', 'h', 'Bricks', 'm', 
     +        'Material' /


c.....MESH GEOMETRY


c.....Nodes 1:16
      real node_1_16(3,16)
      data node_1_16/
     +    0.0, 0.0, 0.0,  1.0, 0.0, 0.0,  3.0, 0.0, 0.0,  6.0, 0.0, 0.0, 
     +    0.0, 1.0, 0.0,  1.0, 1.0, 0.0,  3.0, 1.0, 0.0,  6.0, 1.0, 0.0, 
     +    0.0, 0.0, 1.0,  1.0, 0.0, 1.0,  3.0, 0.0, 1.0,  6.0, 0.0, 1.0, 
     +    0.0, 1.0, 1.0,  1.0, 1.0, 1.0,  3.0, 1.0, 1.0,  6.0, 1.0, 1.0/
c.....Node Labels 1:16
      integer n_labels(16)
      data n_labels / 
     +    10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110,
     +    120, 130, 140, 150, 160 /

c.....Hex 1:3 connectivities; 8 nodes, 1 material, 1 part
      integer hex_1_3(10,3)
      data hex_1_3 /
     +    1, 2, 6, 5, 9, 10, 14, 13, 1, 1,
     +    2, 3, 7, 6, 10, 11, 15, 14, 2, 1,
     +    3, 4, 8, 7, 11, 12, 16, 15, 3, 1/
c.....Hex Labels 1:3
      integer hex_labels(3)
      data hex_labels / 20, 25, 30 /

c.....Total quantity of materials, rigid walls
      integer qty_materials
      data qty_materials / 3 /


c.....And away we go...


c.....MUST open the family

      call mf_open( 'hvadb.plt', '.', 'AwPs', fid, stat )
      if ( stat .ne. 0 ) then
          call mf_print_error( 'mf_open', stat )
          stop
      end if

c.....Write a title
      call mf_wrt_string( fid, 'title', "Vector Array variable test", 
     +                    stat )
      call mf_print_error( 'mf_wrt_string', stat )
      
      
c.....MUST define state variables prior to binding subrecord definitions

c     Note: 1) Re-use of svar_types array since everything is of type m_real
c           2) Variables are defined in groups by object type only so that
c              the "short" name arrays can be re-used during subrecord
c              definitions.  They could be defined in any order over one
c              or any number of calls to mf_def_svars() as definition
c              is completely independent of formatting the state record.
      
c.....Node Positions
      call mf_def_vec_svar( fid, m_real, 3, nodal_short(1), 
     +                      nodal_long(1), nodal_short(4), 
     +                      nodal_long(4), stat )
      call mf_print_error( 'mf_def_vec_svar', stat )
      
c.....Node Velocities
      call mf_def_vec_svar( fid, m_real, 3, nodal_short(2), 
     +                      nodal_long(2), nodal_short(7), 
     +                      nodal_long(7), stat )
      call mf_print_error( 'mf_def_vec_svar', stat )
      
c.....Node Accelerations
      call mf_def_vec_svar( fid, m_real, 3, nodal_short(3), 
     +                      nodal_long(3), nodal_short(10), 
     +                      nodal_long(10), stat )
      call mf_print_error( 'mf_def_vec_svar', stat )
      
c.....Hex Stress Vector
      call mf_def_vec_svar( fid, m_real, 6, hex_short(1), 
     +                      hex_long(1), hex_short(4), 
     +                      hex_long(4), stat )
      call mf_print_error( 'mf_def_vec_svar', stat )
    
c.....Hex EEFF
      call mf_def_svars( fid, 1, svar_types, hex_short(2), 
     +                   hex_long(2), stat )
      call mf_print_error( 'mf_def_svars', stat )
    
c.....Hex VA variable
      call mf_def_vec_arr_svar( fid, m_real, 1, 3, 5, hex_short(3), 
     +                   hex_long(3), va_short, va_long, stat )
      call mf_print_error( 'mf_def_vec_arr_svar', stat )

      
c.....MUST create & define the mesh prior to binding subrecord definitions

c.....Initialize the mesh
      call mf_make_umesh( fid, 'Hex mesh', 3, mesh_id, stat )
      call mf_print_error( 'mf_make_umesh', stat )

c.....Create the mesh object classes by deriving the classes from
c     superclasses, then instantiate the classes by naming the object
c     identifiers and supplying any class-specific attributes.

c.....Create simple mesh object classes; these objects have no
c     special attributes beyond an identifier and membership in a class.
c     The mesh itself is the simplest of all since a single mesh can
c     have (be) only one mesh.  It's unnecessary to specify an
c     identifier as there can be no ambiguity.

c.....Define a class for the mesh itself
      call mf_def_class( fid, mesh_id, m_mesh, global_s, global_l, 
     +                   stat )
      call mf_print_error( 'mf_def_class (global)', stat )
    
      call mf_def_class_idents( fid, mesh_id, global_s, 1, 1, stat )
      call mf_print_error( 'mf_def_class_idents (global)', stat )

c.....Define the material class.
      call mf_def_class( fid, mesh_id, m_mat, material_s, material_l, 
     +                   stat )
      call mf_print_error( 'mf_def_class (material)', stat )
    
      call mf_def_class_idents( fid, mesh_id, material_s, 1, 3, stat )
      call mf_print_error( 'mf_def_class_idents (material)', stat )

c.....Create complex mesh object classes.  First create each class
c     from a superclass as above, then use special calls to supply
c     appropriate attributes (i.e., coordinates for nodes and
c     connectivities for elements).

c.....Define a node class.
      call mf_def_class( fid, mesh_id, m_node, nodal_s, nodal_l, stat )
      call mf_print_error( 'mf_def_class (nodal)', stat )
    
      call mf_def_nodes( fid, mesh_id, nodal_s, 1, 16, node_1_16, stat )
      call mf_print_error( 'mf_def_nodes', stat )

      call mf_def_node_labels(fid, mesh_id, nodal_s, 16, n_labels, stat)
      call mf_print_error( 'mf_def_node_labels', stat)

c.....Define element classes

c.....Connectivities are defined using either of two interfaces,
c     mf_def_conn_arb() or mf_def_conn_seq().  The first references
c     element identifiers explicitly in a 1D array (in any order); 
c     the second references them implicitly by giving the first and last 
c     element identifiers of a continuous sequence.
c
c     Note that all connectivities for a given element type do not have
c     to be defined in one call (for ex., hexs).
c
c     NO checking is performed to verify that nodes referenced in
c     element connectivities have actually been defined in the mesh.

c.....Define a hex class
      call mf_def_class( fid, mesh_id, m_hex, hexs_s, hexs_l, stat )
      call mf_print_error( 'mf_def_class (hexs)', stat )

      call mf_def_conn_seq_labels( fid, mesh_id, hexs_s, 1, 3, 
     +                             hex_labels, hex_1_3, stat )
      call mf_print_error( 'mf_def_conn_seq', stat )


c.....MUST create and define the state record format

c.....Create a state record format descriptor
      call mf_open_srec( fid, mesh_id, srec_id, stat )
      call mf_print_error( 'mf_open_srec', stat )

c.....Nodal data
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 16
      call mf_def_subrec( fid, srec_id, nodal_l, m_result_ordered, 3, 
     +                    nodal_short, nodal_s, m_block_obj_fmt, 1, 
     +                    obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Hex data, brick class
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 3
c.....call mf_def_subrec( fid, srec_id, hexs, m_object_ordered, 7, 
c....+                    hex_short, hexs, m_block_obj_fmt, 1, 
c....+                    obj_blocks, stat )
      call mf_def_subrec( fid, srec_id, hexs_l, m_object_ordered, 3, 
     +                    hex_short(1), hexs_s, m_block_obj_fmt, 1, 
     +                    obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

      call mf_close_srec( fid, srec_id, stat )
      call mf_print_error( 'mf_close_srec', stat )


c.....SHOULD commit before writing state data so the family will be 
c     readable if a crash occurs during the analysis.
   
      call mf_flush( fid, m_non_state_data, stat )
      call mf_print_error( 'mf_commit', stat )


c     call mf_limit_states( fid, 500, stat )
c     call mf_print_error( 'mf_limit_states', stat )


c.....Write five records, each with one call.
      time = 0.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 210, state_record1, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 1.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 210, state_record2, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 2.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 210, state_record3, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 3.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 210, state_record4, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 4.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 210, state_record5, stat )
      call mf_print_error( 'mf_wrt_stream', stat )


c.....MUST close the family when finished

      call mf_close( fid, stat )
      call mf_print_error( 'mf_close', stat )
      
      end
