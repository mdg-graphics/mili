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


      program qnost
c.....Defines a quad mesh but no state variables or state record formats and
c.....writes no states.
      
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
     
      integer fid, mesh_id
      integer stat

c.....Initialize a object_ordered state record with numbers that will
c     distinguish each subrecord and each "lump".  Size to fit format
c     specified below.
      real state_record1(77)
      data state_record1 /
c                                                           Node Positions
     +    0.0, 0.0,  1.0, 0.0,  3.0, 0.0,  6.0, 0.0, 
     +    0.0, 1.0,  1.0, 1.0,  3.0, 1.0,  6.0, 1.0, 
c                                                          Node Velocities
     +    0.0, 0.2,  0.0, 0.0,  0.0, 0.0,  0.0, 0.2, 
     +    0.0, 0.2,  0.0, 0.0,  0.0, 0.0,  0.0, 0.2, 
c                                                       Node Accelerations
     +    0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0, 
     +    0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0, 
c                                                       Node Temperatures
     +    487.3, 487.4, 489.1, 494.0, 480.0, 481.1, 483.7, 488.7,
c
     +    7 * 0.0, 7 * 0.0, 7 * 0.0 /                         ! Quad vars
      real state_record2(77)
      data state_record2 /
c                                                           Node Positions
     +    0.0, 0.2,  1.0, 0.0,  3.0, 0.0,  6.0, 0.2, 
     +    0.0, 1.2,  1.0, 1.0,  3.0, 1.0,  6.0, 1.2, 
c                                                          Node Velocities
     +    0.0, 0.2,  0.0, 0.0,  0.0, 0.0,  0.0, 0.2, 
     +    0.0, 0.2,  0.0, 0.0,  0.0, 0.0,  0.0, 0.2, 
c                                                       Node Accelerations
     +    0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0, 
     +    0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0, 
c                                                       Node Temperatures
     +    488.3, 488.4, 490.1, 495.0, 481.0, 482.1, 484.7, 489.7,
c
     +    7 * 0.0, 7 * 0.0, 7 * 0.0 /                         ! Quad vars
      real state_record3(77)
      data state_record3 /
c                                                           Node Positions
     +    0.0, 0.4,  1.0, 0.0,  3.0, 0.0,  6.0, 0.4, 
     +    0.0, 1.4,  1.0, 1.0,  3.0, 1.0,  6.0, 1.4, 
c                                                          Node Velocities
     +    0.0, 0.2,  0.0, 0.0,  0.0, 0.0,  0.0, 0.2, 
     +    0.0, 0.2,  0.0, 0.0,  0.0, 0.0,  0.0, 0.2, 
c                                                       Node Accelerations
     +    0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0, 
     +    0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0, 
c                                                       Node Temperatures
     +    489.3, 489.4, 491.1, 496.0, 482.0, 483.1, 485.7, 490.7,
c
     +    7 * 0.0, 7 * 0.0, 7 * 0.0 /                         ! Quad vars
      real state_record4(77)
      data state_record4 /
c                                                           Node Positions
     +    0.0, 0.6,  1.0, 0.0,  3.0, 0.0,  6.0, 0.6, 
     +    0.0, 1.6,  1.0, 1.0,  3.0, 1.0,  6.0, 1.6, 
c                                                          Node Velocities
     +    0.0, 0.2,  0.0, 0.0,  0.0, 0.0,  0.0, 0.2, 
     +    0.0, 0.2,  0.0, 0.0,  0.0, 0.0,  0.0, 0.2, 
c                                                       Node Accelerations
     +    0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0, 
     +    0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0, 
c                                                       Node Temperatures
     +    490.3, 490.4, 492.1, 497.0, 483.0, 484.1, 486.7, 491.7,
c
     +    7 * 0.0, 7 * 0.0, 7 * 0.0 /                         ! Quad vars
      real state_record5(77)
      data state_record5 /
c                                                           Node Positions
     +    0.0, 0.8,  1.0, 0.0,  3.0, 0.0,  6.0, 0.8, 
     +    0.0, 1.8,  1.0, 1.0,  3.0, 1.0,  6.0, 1.8, 
c                                                          Node Velocities
     +    0.0, 0.2,  0.0, 0.0,  0.0, 0.0,  0.0, 0.2, 
     +    0.0, 0.2,  0.0, 0.0,  0.0, 0.0,  0.0, 0.2, 
c                                                       Node Accelerations
     +    0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0, 
     +    0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0, 
c                                                       Node Temperatures
     +    491.3, 491.4, 493.1, 498.0, 484.0, 485.1, 487.7, 492.7,
c
     +    7 * 0.0, 7 * 0.0, 7 * 0.0 /                         ! Quad vars


c.....State variables per mesh object type

c.....All short names MUST be unique
      
c.....Nodal state variables
      character*(m_max_name_len) nodal_short(10), nodal_long(10)
      data 
     +    nodal_short /
     +        'nodpos', 'nodvel', 'nodacc', 'temp', 'ux', 'uy', 
     +        'vx', 'vy', 'ax', 'ay' /,
     +    nodal_long /
     +        'Node Position', 'Node Velocity', 'Node Acceleration', 
     +        'Node Temperature', 
     +        'X Position', 'Y Position',
     +        'X Velocity', 'Y Velocity',
     +        'X Acceleration', 'Y Acceleration' /

c.....Quad state variables
      character*(m_max_name_len) quad_short(8), quad_long(8)
      data
     +    quad_short / 
     +        'stress', 'eps', 'sx', 'sy', 'sz', 'sxy', 'syz', 
     +        'szx' /,
     +    quad_long /
     +        'Stress', 'Eff plastic strain', 'Sigma-xx', 'Sigma-yy', 
     +        'Sigma-zz', 'Sigma-xy', 'Sigma-yz', 'Sigma-zx' /

c.....State var types; they're all floats, so just define one array as big
c.....as the largest block of svars that will be defined in one call
      integer svar_types(7)
      data svar_types / 7*m_real /

c.....Group names, to be used when defining connectivities and when
c.....defining subrecords
      character*(m_max_name_len) global_s, global_l, nodal_s, nodal_l, 
     +                           quads_s, quads_l, material_s, 
     +                           material_l
      data global_s, global_l, nodal_s, nodal_l, quads_s, quads_l, 
     +     material_s, material_l /
     +        'g', 'Global', 'node', 'Nodal', 's', 'Shell', 'm', 
     +        'Material' /


c.....MESH GEOMETRY


c.....Nodes 1:8
      real node_1_8(2,8)
      data node_1_8/
     +    0.0, 0.0,  1.0, 0.0,  3.0, 0.0,  6.0, 0.0, 
     +    0.0, 1.0,  1.0, 1.0,  3.0, 1.0,  6.0, 1.0/
      integer n_labels(8)
      data n_labels / 1, 2, 3, 4, 5, 6, 7, 8 /

c.....Quad 1:3 connectivities; 4 nodes, 1 material, 1 part
      integer quad_1_3(6,3)
      data quad_1_3 /
     +    1, 2, 6, 5, 1, 1,
     +    2, 3, 7, 6, 2, 1,
     +    3, 4, 8, 7, 3, 1/
      integer quad_labels(3)
      data quad_labels / 1, 2, 3 /

c.....Total quantity of materials, rigid walls
      integer qty_materials
      data qty_materials / 3 /


c.....And away we go...


c.....MUST open the family

      call mf_open( 'qnost.plt', '.', 'AwPs', fid, stat )
      if ( stat .ne. 0 ) then
          call mf_print_error( 'mf_open', stat )
          stop
      end if

c.....Write a title
      call mf_wrt_string( fid, 'title', "No states or var's test db", 
     +                    stat )
      call mf_print_error( 'mf_wrt_string', stat )
      
      
c.....MUST create & define the mesh prior to binding subrecord definitions

c.....Initialize the mesh
      call mf_make_umesh( fid, 'Quad mesh', 2, mesh_id, stat )
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
    
      call mf_def_nodes( fid, mesh_id, nodal_s, 1, 8, node_1_8, stat )
      call mf_print_error( 'mf_def_nodes', stat )

      call mf_def_node_labels(fid, mesh_id, nodal_s, 8, n_labels, stat)
      call mf_print_error( 'mf_def_node_labels', stat )

c.....Define element classes

c.....Connectivities are defined using either of two interfaces,
c     mf_def_conn_arb() or mf_def_conn_seq().  The first references
c     element identifiers explicitly in a 1D array (in any order); 
c     the second references them implicitly by giving the first and last 
c     element identifiers of a continuous sequence.
c
c     Note that all connectivities for a given element type do not have
c     to be defined in one call (for ex., quads).
c
c     NO checking is performed to verify that nodes referenced in
c     element connectivities have actually been defined in the mesh.

c.....Define a quad class
      call mf_def_class( fid, mesh_id, m_quad, quads_s, quads_l, stat )
      call mf_print_error( 'mf_def_class (quads)', stat )

      call mf_def_conn_seq_labels( fid, mesh_id, quads_s, 1, 3, 
     +                             quad_labels, quad_1_3, stat )
      call mf_print_error( 'mf_def_conn_seq', stat )

c.....MUST close the family when finished

      call mf_close( fid, stat )
      call mf_print_error( 'mf_close', stat )
      
      end
