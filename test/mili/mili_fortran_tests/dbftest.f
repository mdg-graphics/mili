c $Id: dbftest.f,v 1.1 2021/07/23 16:03:02 rhathaw Exp $

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


      program dbftest
      
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
      integer rdims(3), idims(2)
      integer idum
      real real_array(3,3,3)
c     integer obj_blocks(2,1)
      integer obj_blocks(2,5)
      integer obj_list(5)
      data 
     +    rdims /3, 3, 3/,
     +    real_array /
     +        1.0, 1.0, 1.0, 2.0, 2.0, 2.0, 3.0, 3.0, 3.0,
     +        4.0, 4.0, 4.0, 5.0, 5.0, 5.0, 6.0, 6.0, 6.0,
     +        7.0, 7.0, 7.0, 8.0, 8.0, 8.0, 9.0, 9.0, 9.0/
      
      integer int_array(2,3)
      data 
     +    idims /2, 3/,
     +    int_array /10, 10, 20, 20, 30, 30/
      
      character*48 char_string, char_name
      data
     +    char_string /'This is a test, this is only a test!'/

c     character*(m_max_name_len) ver_string
      character*(32) ver_string

c.....Initialize a object_ordered state record with numbers that will
c     distinguish each subrecord and each "lump".  Size to fit format
c     specified below.
      real state_record(660)
      data state_record /
     +    6 * 1001.0,                                         ! Mesh vars
     +    4 * 2001.0, 4 * 2002.0, 4 * 2003.0, 3 * 2004.0,     ! Material vars
     +    3 * 2005.0, 3 * 2006.0, 3 * 2007.0,
     +    2 * 3001.0,                                         ! Rigid wall vars
     +    9 * 4001.0, 9 * 4002.0, 9 * 4003.0, 9 * 4004.0,     ! Nodal vars
     +    9 * 4005.0, 9 * 4006.0, 9 * 4007.0, 9 * 4008.0, 
     +    9 * 4009.0, 9 * 4010.0, 9 * 4011.0, 9 * 4012.0, 
     +    9 * 4013.0, 9 * 4014.0, 9 * 4015.0, 9 * 4016.0, 
     +    9 * 4017.0, 9 * 4018.0, 9 * 4019.0, 9 * 4020.0, 
     +    9 * 4021.0, 9 * 4022.0, 9 * 4023.0, 9 * 4024.0, 
     +    9 * 4025.0, 9 * 4026.0, 9 * 4027.0, 9 * 4028.0, 
     +    9 * 4029.0, 9 * 4030.0, 9 * 4031.0, 9 * 4032.0, 
     +    9 * 4033.0, 9 * 4034.0, 9 * 4035.0, 9 * 4036.0, 
     +    9 * 4037.0, 9 * 4038.0, 9 * 4039.0, 9 * 4040.0, 
     +    9 * 4041.0, 
     +    7 * 5001.0, 7 * 5002.0, 7 * 5003.0, 7 * 5004.0,      ! Brick vars
     +    7 * 5005.0, 
     +    7 * 6001.0, 7 * 6002.0, 7 * 6003.0, 7 * 6004.0,      ! Thick shells
     +    7 * 6005.0, 
     +    6 * 7001.0, 6 * 7002.0, 6 * 7003.0, 6 * 7004.0,      ! Beam vars
     +    33 * 8001.0, 33 * 8002.0, 33 * 8003.0, 33 * 8004.0,  ! Shell vars
     +    33 * 8005.0 /


c.....State variables per mesh object type

c.....All short names MUST be unique

c.....Global state variables
c     Note that although all short names are packed into one array,
c     they will be referenced through different calls when the
c     state variables are actually defined.  Later, when subrecords
c     are defined, it will be convenient to have them all in one
c     array.  This is a convenience afforded by this hard-coded
c     example.  In general, a flexible database would have to
c     build up the string arrays dynamically as required.
      character*(m_max_name_len) global_short(14), global_long(14)
      data 
     +    global_short /
     +        'ke', 'pe', 'te', 'rigxv', 'rigyv', 'rigzv', 'matpe', 
     +        'matke', 'matvel', 'matma', 'wf', 'xvel', 'yvel', 
     +        'zvel' /,
     +    global_long /
     +        'Kinetic energy', 'Potential energy', 'Total energy',
     +        'Rigid body x-velocity', 'Rigid body y-velocity',
     +        'Rigid body z-velocity', 'Mat Potential Energy', 
     +        'Mat Kinetic Energy', 'Mat Velocity', 'Mat Mass', 
     +        'Rigid Wall Force', 'Mat X Velocity', 'Mat Y Velocity', 
     +        'Mat Z Velocity' /
      
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

c.....Beam state variables
      character*(m_max_name_len) beam_short(6), beam_long(6)
      data
     +    beam_short / 'axf', 'sfs', 'sft', 'ms', 'mt', 'tor' /,
     +    beam_long /
     +        'Axial Force', 'Shear Resultant - s', 
     +        'Shear Resultant - t', 'Bending Moment - s', 
     +        'Bending Moment - t', 'Torsional Resultant' /

c.....Shell state variables
      character*(m_max_name_len) shell_short(33), shell_long(33)
      data
     +    shell_short /
     +        'sxxmid', 'syymid', 'szzmid', 'sxymid', 'syzmid', 
     +        'szxmid', 'eeffmid', 'sxxin', 'syyin', 'szzin', 'sxyin',
     +        'syzin', 'szxin', 'eeffin', 'sxxout', 'syyout', 'szzout', 
     +        'sxyout', 'syzout', 'szxout', 'eeffout', 'mxx', 'myy',
     +        'mxy', 'qxx', 'qyy', 'nxx', 'nyy', 'nxy', 'th', 'edv1',
     +        'edv2', 'ie' /
      data
     +    shell_long /
     +        'Sigma-xx Mid', 'Sigma-yy Mid', 'Sigma-zz Mid', 
     +        'Sigma-xy Mid', 'Sigma-yz Mid', 'Sigma-zx Mid',
     +        'Eff plastic strain Mid', 'Sigma-xx In', 'Sigma-yy In', 
     +        'Sigma-zz In', 'Sigma-xy In', 'Sigma-yz In', 
     +        'Sigma-zx In', 'Eff plastic strain In', 'Sigma-xx Out',
     +        'Sigma-yy Out', 'Sigma-zz Out', 'Sigma-xy Out',
     +        'Sigma-yz Out', 'Sigma-zx Out', 'Eff plastic strain Out',
     +        'Bending Moment - mxx', 'Bending Moment - myy',
     +        'Bending Moment - mxy', 'Shear Resultant - qxx',
     +        'Shear Resultant - qyy', 'Normal Resultant - nxx',
     +        'Normal Resultant - nyy', 'Normal Resultant - nxy',
     +        'Thickness', 'Element variable 1', 'Element variable 2',
     +        'Internal Energy' /

c.....Hex state variables
      character*(m_max_name_len) hex_short(7), hex_long(7)
      data
     +    hex_short / 
     +        'sxx', 'syy', 'szz', 'sxy', 'syz', 'szx', 'eps' /,
     +    hex_long /
     +        'Sigma-xx', 'Sigma-yy', 'Sigma-zz', 'Sigma-xy', 
     +        'Sigma-yz', 'Sigma-zx', 'Eff plastic strain' /

c.....State var types; they're all floats, so just define one array as big
c.....as the largest block of svars that will be defined in one call
      integer svar_types(33)
      data svar_types / 33*m_real /

c.....Short and long group names, to be used when defining connectivities 
c.....and when defining subrecords
      character*(m_max_name_len) global_s, global_l, nodal_s, nodal_l, 
     +                           beams_s, beams_l, shells_s, shells_l, 
     +                           hexs_s, hexs_l, thick_shells_s, 
     +                           thick_shells_l, material_s, 
     +                           material_l, rigid_wall_s, rigid_wall_l
      data global_s, global_l, nodal_s, nodal_l, beams_s, beams_l, 
     +     shells_s, shells_l, hexs_s, hexs_l, thick_shells_s, 
     +     thick_shells_l, material_s, material_l, rigid_wall_s, 
     +     rigid_wall_l /
     +        'g', 'Global', 'node', 'Nodal', 'b', 'Beams', 's', 
     +        'Shells', 'h', 'Bricks', 't', 'Thick Shells', 'm', 
     +        'Material', 'w', 'Rigid Wall' /


c.....MESH GEOMETRY


c.....Nodes 1:24
      real node_1_24(3,24)
      data node_1_24/
     +    0.0, 0.0, 0.0,  0.0, 1.0, 0.0,  0.0, 1.0, 1.0,  0.0, 0.0, 1.0, 
     +    1.0, 0.0, 0.0,  1.0, 1.0, 0.0,  1.0, 1.0, 1.0,  1.0, 0.0, 1.0, 
     +    2.0, 0.0, 0.0,  2.0, 1.0, 0.0,  2.0, 1.0, 1.0,  2.0, 0.0, 1.0, 
     +    3.0, 0.0, 0.0,  3.0, 1.0, 0.0,  3.0, 1.0, 1.0,  3.0, 0.0, 1.0, 
     +    4.0, 0.0, 0.0,  4.0, 1.0, 0.0,  4.0, 1.0, 1.0,  4.0, 0.0, 1.0, 
     +    5.0, 0.0, 0.0,  5.0, 1.0, 0.0,  5.0, 1.0, 1.0,  5.0, 0.0, 1.0/
     
c.....Nodes 25:29
      real node_25_29(3,5)
      data node_25_29/
     +    0.5, 0.5, 2.0,  1.5, 0.5, 2.0,  2.5, 0.5, 2.0,  3.5, 0.5, 2.0,
     +    4.5, 0.5, 2.0 /
     
c.....Nodes 30:35
      real node_30_35(3,6)
      data node_30_35/
     +    0.0, -1.0, 0.0,  1.0, -1.0, 0.0,  2.0, -1.0, 0.0,
     +    3.0, -1.0, 0.0,  4.0, -1.0, 0.0,  5.0, -1.0, 0.0 /
     
c.....Nodes 36:41
      real node_36_41(3,6)
      data node_36_41/
     +    0.0, -1.0, 1.0,  1.0, -1.0, 1.0,  2.0, -1.0, 1.0,
     +    3.0, -1.0, 1.0,  4.0, -1.0, 1.0,  5.0, -1.0, 1.0 /

c.....Shell 1:5 connectivities; 4 nodes, 1 material, 1 part
      integer shell_1_5(6,5), shell_list(5)
      data shell_1_5 /
     +    1, 5, 4, 2, 2, 1,
     +    5, 9, 8, 6, 2, 1,
     +    9, 13, 14, 10, 2, 1,
     +    13, 17, 18, 14, 2, 1,
     +    17, 21, 22, 18, 2, 1 /,
     +    shell_list / 1, 2, 3, 4, 5 /

c.....Hex 1:5 connectivities; 8 nodes, 1 material, 1 part
      integer hex_1_5(10,5), hex_list(5)
      data hex_1_5 /
     +    30, 31, 5, 1, 36, 37, 8, 4, 3, 1,
     +    1, 5, 6, 2, 4, 8, 7, 3, 3, 1,
     +    5, 9, 10, 6, 8, 12, 11, 7, 3, 1,
     +    9, 13, 14, 10, 12, 16, 15, 11, 3, 1,
     +    13, 17, 18, 14, 16, 20, 19, 15, 3, 1/
     +    hex_list / 1, 3, 5, 2, 4 /

c.....Hex 6:10 connectivities; 8 nodes, 1 material, 1 part
      integer hex_6_10(10,5)
      data hex_6_10 /
     +    17, 21, 22, 18, 20, 24, 23, 19, 4, 1,
     +    34, 35, 2, 17, 40, 41, 24, 20, 4, 1,
     +    33, 34, 17, 13, 39, 40, 20, 16, 4, 1,
     +    32, 33, 13, 9, 38, 39, 16, 12, 4, 1,
     +    31, 32, 9, 5, 37, 38, 12, 8, 4, 1/

c.....Beam 1:4 connectivities; 3 nodes, 1 material, 1 part
      integer beam_1_4(5,4), beam_list(4)
      data 
     +    beam_1_4 /
     +        25, 26, 1, 1, 2,    26, 27, 1, 1, 2,   27, 28, 1, 1, 2, 
     +        28, 29, 1, 1, 2 /,
     +    beam_list / 1, 2, 3, 4 /

c.....Total quantity of materials, rigid walls
      integer qty_materials, qty_rwalls
      data qty_materials, qty_rwalls / 4, 2 /


c.....And away we go...


c.....MUST open the family

      call mf_open( 'dbftest.plt', '.', 'AwPs', fid, stat )
      if ( stat .ne. 0 ) then
          call mf_print_error( 'mf_open', stat )
          stop
      end if

c.....Write a title
      call mf_wrt_string( fid, 'title', "General Taurus-db-like test", 
     +                    stat )
      call mf_print_error( 'mf_wrt_string', stat )

c.....Query the library version
      ver_string = m_blank
      call mf_query_family( fid, m_lib_version, m_dont_care, m_blank,
     +                      ver_string, stat )
      call mf_print_error( 'mf_query_family (m_lib_version)', stat )
      write( *, 122 ) ver_string
122   format( 'Mili library version is ', a )
      
c.....Write a few named parameters (just to show how - not required
c     in the database)

      call mf_wrt_scalar( fid, m_real, 'real num', 7.5, stat )
      call mf_print_error( 'mf_wrt_scalar real', stat )
      
      call mf_wrt_scalar( fid, m_int, 'integer num', 10, stat )
      call mf_print_error( 'mf_wrt_scalar int', stat )
      
      call mf_wrt_array( fid, m_real, 'real array', 3, rdims, 
     +                   real_array, stat )
      call mf_print_error( 'mf_wrt_array real', stat )
      
      call mf_wrt_array( fid, m_int, 'int array', 2, idims, 
     +                   int_array, stat )
      call mf_print_error( 'mf_wrt_array int', stat )
      
      char_name = 'string'
      call mf_wrt_string( fid, char_name, char_string, stat )
      call mf_print_error( 'mf_wrt_string', stat )
      
      
c.....MUST define state variables prior to binding subrecord definitions

c     Note: 1) Re-use of svar_types array since everything is of type m_real
c           2) Variables are defined in groups by object type only so that
c              the "short" name arrays can be re-used during subrecord
c              definitions.  They could be defined in any order over one
c              or any number of calls to mf_def_svars() as definition
c              is completely independent of formatting the state record.
    
      call mf_def_svars( fid, 8, svar_types, global_short, 
     +                   global_long, stat )
      call mf_print_error( 'mf_def_svars', stat )
      
      call mf_def_vec_svar( fid, m_real, 3, 
     +                      global_short(9), global_long(9), 
     +                      global_short(12), global_long(12), 
     +                      stat )
      call mf_print_error( 'mf_def_vec_svar', stat )
      
      call mf_def_svars( fid, 2, svar_types, 
     +                   global_short(10), global_long(10), stat )
      call mf_print_error( 'mf_def_svars', stat )
      
      call mf_def_vec_svar( fid, m_real, 3, nodal_short(1), 
     +                      nodal_long(1), nodal_short(4), 
     +                      nodal_long(4), stat )
      call mf_print_error( 'mf_def_vec_svar', stat )
      
      call mf_def_vec_svar( fid, m_real, 3, nodal_short(2), 
     +                      nodal_long(2), nodal_short(7), 
     +                      nodal_long(7), stat )
      call mf_print_error( 'mf_def_vec_svar', stat )
      
      call mf_def_vec_svar( fid, m_real, 3, nodal_short(3), 
     +                      nodal_long(3), nodal_short(10), 
     +                      nodal_long(10), stat )
      call mf_print_error( 'mf_def_vec_svar', stat )
    
      call mf_def_svars( fid, 6, svar_types, beam_short, beam_long, 
     +                   stat )
      call mf_print_error( 'mf_def_svars', stat )
    
      call mf_def_svars( fid, 33, svar_types, shell_short, shell_long, 
     +                   stat )
      call mf_print_error( 'mf_def_svars', stat )
    
      call mf_def_svars( fid, 7, svar_types, hex_short, hex_long, stat )
      call mf_print_error( 'mf_def_svars', stat )

c.....Just a test - try to redefine an extant state variable.  Should
c     generate a warning.
      call mf_def_svars( fid, 1, svar_types, 'sxx', 'Don''t matter', 
     +                   stat )
      call mf_print_error( 'mf_def_svars', stat )

      
c.....MUST create & define the mesh prior to binding subrecord definitions

c.....Initialize the mesh
      call mf_make_umesh( fid, 'Test mesh', 3, mesh_id, stat )
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
    
      call mf_def_class_idents( fid, mesh_id, material_s, 1, 4, stat )
      call mf_print_error( 'mf_def_class_idents (material)', stat )
    
c.....Define a unit class to accommodate rigid wall data
      call mf_def_class( fid, mesh_id, m_unit, rigid_wall_s,
     +                   rigid_wall_l, stat )
      call mf_print_error( 'mf_def_class (rigid wall)', stat )
    
      call mf_def_class_idents( fid, mesh_id, rigid_wall_s, 1, 2, stat )
      call mf_print_error( 'mf_def_class_idents (rigid wall)', stat )

c.....Create complex mesh object classes.  First create each class
c     from a superclass as above, then use special calls to supply
c     appropriate attributes (i.e., coordinates for nodes and
c     connectivities for elements).

c.....Define a node class.
      call mf_def_class( fid, mesh_id, m_node, nodal_s, nodal_l, stat )
      call mf_print_error( 'mf_def_class (nodal)', stat )
    
      call mf_def_nodes( fid, mesh_id, nodal_s, 30, 35, node_30_35, 
     +                   stat )
      call mf_print_error( 'mf_def_nodes', stat )

      call mf_def_nodes( fid, mesh_id, nodal_s, 1, 24, node_1_24, stat )
      call mf_print_error( 'mf_def_nodes', stat )

      call mf_def_nodes( fid, mesh_id, nodal_s, 25, 29, node_25_29, 
     +                   stat )
      call mf_print_error( 'mf_def_nodes', stat )

      call mf_def_nodes( fid, mesh_id, nodal_s, 36, 41, node_36_41, 
     +                   stat )
      call mf_print_error( 'mf_def_nodes', stat )

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

c.....Define a beam class
      call mf_def_class( fid, mesh_id, m_beam, beams_s, beams_l, stat )
      call mf_print_error( 'mf_def_class (beams)', stat )

      call mf_def_conn_arb( fid, mesh_id, beams_s, 4, 
     +                      beam_list, beam_1_4, stat )
      call mf_print_error( 'mf_def_lst_conn', stat )

c.....The equivalent mf_def_conn_seq() call for above:
c     call mf_def_conn_seq( fid, mesh_id, beams, 1, 
c    +                      4, beam_1_4, stat )

c.....Define a hex class
      call mf_def_class( fid, mesh_id, m_hex, hexs_s, hexs_l, stat )
      call mf_print_error( 'mf_def_class (hexs)', stat )

      call mf_def_conn_arb( fid, mesh_id, hexs_s, 5, 
     +                      hex_list, hex_1_5, stat )
      call mf_print_error( 'mf_def_conn_arb', stat )

c.....The equivalent mf_def_conn_seq() call for above:
c     call mf_def_conn_seq( fid, mesh_id, hexs_s, 4, 5, 
c    +                      hex_1_5(1,4), stat )
c     call mf_print_error( 'mf_def_conn_seq', stat )

c.....Define a quad class
      call mf_def_class( fid, mesh_id, m_quad, shells_s, shells_l, 
     +                   stat )
      call mf_print_error( 'mf_def_class (shells)', stat )

      call mf_def_conn_arb( fid, mesh_id, shells_s, 5, 
     +                      shell_list, shell_1_5, stat )
      call mf_print_error( 'mf_def_conn_arb', stat )
    
c.....The equivalent mf_def_conn_seq() call for above:
c     call mf_def_conn_seq( fid, mesh_id, shells, 1, 
c    +                      5, shell_1_5, stat )

c.....Add to the hex class created above (just to demonstrate that
c     order is unimportant within a particular class)
      call mf_def_conn_seq( fid, mesh_id, hexs_s, 1, 3, 
     +                      hex_1_5, stat )
      call mf_print_error( 'mf_def_conn_seq', stat )

c.....Define a thick shell class
      call mf_def_class( fid, mesh_id, m_hex, thick_shells_s,
     +                   thick_shells_l, stat )
      call mf_print_error( 'mf_def_class (thick_shells)', stat )

      call mf_def_conn_seq( fid, mesh_id, thick_shells_s, 1, 5, 
     +                      hex_6_10, stat )
      call mf_print_error( 'mf_def_conn_seq', stat )


c.....MUST create and define the state record format
    
c.....A state record is defined by defining each subrecord that will make
c     up the state record.  "Defining" a subrecord entails binding one or 
c     more class objects (nodes or elements), 
c     to a set of state variables, indicated by their short names.  The 
c     order in which objects are referenced in the calling array indicates 
c     the order in which their data should be found in actual state data.  The 
c     order in which the state variable names appear indicates the order in 
c     which those values should appear for each object in actual state data, 
c     assuming the database organization is m_object_ordered.  If the 
c     database organization is m_result_ordered, the order of state variables 
c     indicates the order in which result arrays should appear within the 
c     subrecord's actual state data.  The order in which subrecords are 
c     defined similarly dictates the order in which their data should be 
c     found in the actual state data record.  Are we confused yet?

c.....Create a state record format descriptor
      call mf_open_srec( fid, mesh_id, srec_id, stat )
      call mf_print_error( 'mf_open_srec', stat )

c.....In this example, the definitions below format a state record with
c     global data first, followed by nodal data, hex data, beam data, 
c     and shell data.  As this example purports to format a record
c     similar to that used in the existing Taurus plotfile, there is only
c     one set of state variables for each object type, and so each call
c     includes only one object ident block which encompasses all of the
c     nodes or elements for the type.  In general, there can be any number
c     of discontiguous mesh objects for a subrecord, and, conversely, an
c     arbitrary number of subrecords formatting data for a single object 
c     type (each subrecord binding a subset of the mesh objects of that type).
c     In this way, different materials within one element type can have
c     different state variables output.

c.....In each of the non-comment examples below, the same string is 
c     used for both the class name and the subrecord name.  This is a
c     convenience afforded by the fact that each subrecord being defined
c     is binding all of the mesh objects for the associated subclass, so only
c     one subrecord name is needed.  If multiple subrecords were being 
c     defined for a class, for example if it were desired to write 
c     different state variables for solid 8-node elements of different 
c     materials, then each subrecord would require a unique name.
      
c.....Global (mesh) data  
c     Notes:
c      1. Only referencing the first 6 entries in array global_short.
c      2. Mesh data implicitly binds to only one mesh object, so the 
c         format and object id arguments are m_dont_care (and the object
c         quantity, the ninth argument, is not actually evaluated).
c     obj_blocks(1,1) = 1
c     obj_blocks(2,1) = 1
c     call mf_def_subrec( fid, srec_id, global, m_object_ordered, 6, 
c    +                    global_short, global, m_block_obj_fmt, 1, 
c    +                    obj_blocks, stat )
      call mf_def_subrec( fid, srec_id, global_l, m_object_ordered, 6, 
     +                    global_short, global_s, m_dont_care, 1, 
     +                    m_dont_care, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Material data
c     Note that this data will be written out result-ordered.
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = qty_materials
      call mf_def_subrec( fid, srec_id, material_l, m_result_ordered, 
     +                    4, global_short(7), material_s, 
     +                    m_block_obj_fmt, 1, obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Rigid wall data (time-history database only?)
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = qty_rwalls
      call mf_def_subrec( fid, srec_id, rigid_wall_l, m_object_ordered,
     +                    1, global_short(11), rigid_wall_s, 
     +                    m_block_obj_fmt, 1, obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Nodal data
c     Note specification as three result-ordered arrays of vectors.
c     Could alternatively be specified as three subrecords of object-
c     ordered groups of three scalars (i.e., posx, posy, posz; velx,
c     vely, velz; and accx, accy, accz) and utilize the same physical
c     ordering of the actual data.  One such subrecord definition for
c     the positions (for example) would be:
c          call mf_def_subrec( fid, srec_id, nodal, m_object_ordered,
c         +                    3, nodal_short(4), nodal, m_block_obj_fmt,
c         +                    1, obj_blocks, stat )
c     In the above case, the existing state variable definitions of
c     three vector variables (position, velocity, and acceleration)
c     should be replaced with one call to mf_def_svars() to define all
c     nine scalars, although the existing definitions as vectors would
c     actually work since each vector component is in fact created as a
c     scalar variable and can be used as such independently of the
c     vector definition that created it.
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 41
      call mf_def_subrec( fid, srec_id, nodal_l, m_result_ordered, 3, 
     +                    nodal_short, nodal_s, m_block_obj_fmt, 1, 
     +                    obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Hex data, brick class
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 5
      call mf_def_subrec( fid, srec_id, hexs_l, m_object_ordered, 7, 
     +                    hex_short, hexs_s, m_block_obj_fmt, 1, 
     +                    obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....As an example of varying the set of state variables for a subset of
c     an element class, suppose there were 10 hex elements instead of 
c     just the five referenced above (and forgetting for the moment the
c     thick_shell hex elements, since they're referenced below), but that 
c     the second five were of a different material for which we wanted to 
c     output 12 different state variables.  Assuming that the 12 state 
c     variables had all been defined and their short names were loaded 
c     into an array hexmat2_short, the subrecord definition would be as 
c     follows (note primarily the new subrecord name [third parameter], 
c     the same class name [sixth parameter], and the new mesh object block 
c     [ninth parameter]):
c     obj_blocks(1,1) = 6
c     obj_blocks(2,1) = 10
c     call mf_def_subrec( fid, srec_id, 'Hex Mat 2', m_object_ordered, 
c    +                    7, hexmat2_short, hexs, m_block_obj_fmt, 1, 
c    +                    obj_blocks, stat )
c     call mf_print_error( 'mf_def_subrec', stat )

c.....Hex data, thick shell class
c.....After creating a class to allow for a new set of 8-node elements, 
c     I'm just putting out the same seven state variables as for the 
c     8-node brick elements (since I haven't a clue as to what state 
c     variables would be desirable for real thick shell elements...).
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 5
      call mf_def_subrec( fid, srec_id, thick_shells_l, 
     +                    m_object_ordered, 7, hex_short, 
     +                    thick_shells_s, m_block_obj_fmt, 
     +                    1, obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Beam data
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 4
      call mf_def_subrec( fid, srec_id, beams_l, m_object_ordered, 6, 
     +                    beam_short, beams_s, m_block_obj_fmt, 1, 
     +                    obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Shell data
c.....Note the alternative specification showing use of the m_list_obj_fmt
c     element format.  Mili does not care what order the elements are in, but
c     the specified order must be followed when actual state data is output.
c     Internally, Mili converts element lists to element block lists, so it
c     is desirable that an element list actually contain contiguous ranges of
c     ascending element numbers which can be advantageously compressed to 
c     first:last pairs.
      obj_list(1) = 1
      obj_list(2) = 2
      obj_list(3) = 3
      obj_list(4) = 4
      obj_list(5) = 5
      call mf_def_subrec( fid, srec_id, shells_l, m_object_ordered, 33, 
     +                    shell_short, shells_s, m_list_obj_fmt, 5, 
     +                    obj_list, stat )
      call mf_print_error( 'mf_def_subrec', stat )
    
      call mf_close_srec( fid, srec_id, stat )
      call mf_print_error( 'mf_close_srec', stat )


c.....SHOULD commit before writing state data so the family will be 
c     readable if a crash occurs during the analysis.
   
      call mf_flush( fid, m_non_state_data, stat )
      call mf_print_error( 'mf_commit', stat )


c.....SHOULD set number of states to write per file before writing
c     any state data, or take the default (100 states per file).
c     Technically, this can be changed at any time and will affect
c     all states initiated after the call.  As a matter of good
c     practice (and possibly a future requirement), it should be set
c     once for a family before state data is written and not changed
c     thereafter.

c     call mf_limit_states( fid, 500, stat )
c     call mf_print_error( 'mf_limit_states', stat )


c.....Now write some state data.  MUST call mf_new_state() prior to writing
c     data for each state.  Write state data with mf_wrt_stream() or 
c     mf_wrt_subrec().  Note that in this contrived example, I've already
c     created a state record full of data, and the examples below just
c     access the pertinent portion of the array (but of course Mili
c     doesn't know that).  The state record has 660 real elements in it.

c.....First write a couple records with the "stream" interface.  This has 
c     the lowest overhead within the I/O library because the application
c     bears responsibility for writing the correct amount of data and
c     sequencing it properly.  All data passed in a single call must be of 
c     the same base type since only one conversion is applied (if a
c     translation is necessary, i.e., Cray to Mili Standard).

c.....Write one record all at once.
      time = 0.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 660, state_record, stat )
      call mf_print_error( 'mf_wrt_stream', stat )

c.....Write another record in arbitrary (though sequentially correct)
c     pieces.
c     time = time + 0.01
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 200, state_record, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      call mf_wrt_stream( fid, m_real, 300, state_record(201), stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      call mf_wrt_stream( fid, m_real, 160, state_record(501), stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
c.....Now write a record with mf_wrt_subrec().  This interface allows the
c     application to randomly access a subrecord's data with a resolution
c     of one mesh object's data (for m_object_ordered file families) or one
c     result array (for m_result_ordered file families).  The "start" and
c     "stop" parameters indicate the relative position of the data being
c     written within the sequence of object "vectors" (when m_object_ordered)
c     or result arrays (when m_result_ordered) for the subrecord.

c.....All data is assumed to have the same type, so if a subrecord has both
c     real and integer state variables (requiring the file family to be
c     m_result_ordered), the data passed in one call to mf_wrt_subrec() must
c     not contain both real and integer result arrays, even if they will be 
c     adjacent on disk.
      
c     time = time + 0.01
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
c.....In the statements below, remember that it is just an artifact of this
c     hard-coded example that all of the state data is in one big array,
c     allowing me to access each subrecord by knowing the correct index.
c     A real application might re-use the same buffer over and over or 
c     assemble each subrecord into a different buffer.
      
c.....Write the shell data first, in two steps and non-sequential order
c     (just to show that it can be done).
      call mf_wrt_subrec( fid, shells_l, 3, 5, state_record(562), stat )
      call mf_print_error( 'mf_wrt_subrec', stat )

      call mf_wrt_subrec( fid, shells_l, 1, 2, state_record(496), stat )
      call mf_print_error( 'mf_wrt_subrec', stat )

c.....Global mesh data.
c.....Note that if this file family were m_result_ordered, the "stop"
c     parameter would need to be 11, and each result array
c     would only have one entry.
      call mf_wrt_subrec( fid, global_l, 1, 1, state_record, stat )
      call mf_print_error( 'mf_wrt_subrec', stat )

c.....Material data.
      call mf_wrt_subrec( fid, material_l, 1, 4, state_record(7), stat )
      call mf_print_error( 'mf_wrt_subrec', stat )

c.....Rigid wall data.
      call mf_wrt_subrec( fid, rigid_wall_l, 1, 2, state_record(31), 
     +                    stat )
      call mf_print_error( 'mf_wrt_subrec', stat )

c.....Note that as a practical matter for current Dyna and Nike data,
c     it would be much more efficient to have the above mesh, material,
c     and wall data in a buffer and write it all at once using
c     mf_wrt_stream().  These types of data will generally have a few
c     tens or at most hundreds of words so it would be less efficient
c     to perform three small writes rather than one "less-small" write.

c.....Nodal data, all at once.
      call mf_wrt_subrec( fid, nodal_l, 1, 3, state_record(33), stat )
      call mf_print_error( 'mf_wrt_subrec', stat )
      
c.....Beam data, all at once.
      call mf_wrt_subrec( fid, beams_l, 1, 4, state_record(472), stat )
      call mf_print_error( 'mf_wrt_subrec', stat )

c.....Hex data, second subclass, all at once.
      call mf_wrt_subrec( fid, thick_shells_l, 1, 5, state_record(437), 
     +                    stat )
      call mf_print_error( 'mf_wrt_subrec', stat )

c.....Hex data, first subclass, all at once.
      call mf_wrt_subrec( fid, hexs_l, 1, 5, state_record(402), stat )
      call mf_print_error( 'mf_wrt_subrec', stat )
      

c.....If one were so inclined, both mf_wrt_subrec() and mf_wrt_stream()
c     could be used to write data for one state.  Just be aware that
c     (1) mf_wrt_subrec() seeks the file position to the correct position
c     for writing, while mf_wrt_stream() writes at the current position,
c     and (2) both calls leave the file position where it falls after 
c     writing the data.

c.....Query for class existence
      idims(1) = 2
      call mf_query_family( fid, m_class_exists, idims, shells_s, idum,
     +                      stat )
      call mf_print_error( 'mf_query_family (m_class_exists)', stat )
      idims(1) = mesh_id
      call mf_query_family( fid, m_class_exists, idims, '', idum,
     +                      stat )
      call mf_print_error( 'mf_query_family (m_class_exists)', stat )
      idims(1) = mesh_id
      call mf_query_family( fid, m_class_exists, idims, shells_s, idum,
     +                      stat )
      call mf_print_error( 'mf_query_family (m_class_exists)', stat )
      if ( idum.ne.0 ) print *, 'Class "', shells_s(1:1), '" found.'

      call mf_query_family( fid, m_class_exists, idims, 'noclass', idum,
     +                      stat )
      call mf_print_error( 'mf_query_family (m_class_exists)', stat )
      if ( idum.eq.0 ) print *, 'Class "noclass" not found.'


c.....MUST close the family when finished

      call mf_close( fid, stat )
      call mf_print_error( 'mf_close', stat )
      
      end
