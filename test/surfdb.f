c $Id: surfdb.f,v 1.3 2006/04/24 21:02:20 pierce5 Exp $

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


      program surfdbftest
      
      implicit none

c.....SHOULD include this, or extract necessary values somehow/somewhere      
      include "mili_fparam.h"
      
      external
     +    mf_open, mf_close, mf_wrt_scalar, mf_wrt_array, 
     +    mf_wrt_string, mf_make_umesh, mf_def_class, mf_def_nodes, 
     +    mf_def_class_idents, mf_def_conn_seq, mf_def_conn_arb, 
     +    mf_get_mesh_id, mf_def_svars, mf_def_arr_svar, 
     +    mf_open_srec, mf_def_subrec, mf_def_surf_subrec,
     +    mf_flush, mf_new_state, 
     +    mf_wrt_stream, mf_wrt_subrec, mf_limit_states, 
     +    mf_suffix_width, mf_print_error, mf_def_vec_arr_svar,
     +    mf_def_vec_svar, mf_close_srec
     
      integer fid, mesh_id, srec_id, surf_id, suffix, st_idx
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
      real state_record(663)
      data state_record /
     +    6 * 1001.0,                                         ! Mesh vars
     +    3 * 2001.0, 3 * 2002.0, 3 * 2003.0,                 ! Material vars
     +    3 * 2004.0, 3 * 2005.0, 3 * 2006.0,
     +    2 * 3001.0,                                         ! Rigid wall vars
     +    6 * 4001.0, 6 * 4002.0, 6 * 4003.0, 6 * 4004.0,     ! Nodal vars
     +    6 * 4005.0, 6 * 4006.0, 6 * 4007.0, 6 * 4008.0, 
     +    6 * 4009.0, 6 * 4010.0, 6 * 4011.0, 6 * 4012.0, 
     +    6 * 4013.0, 6 * 4014.0, 6 * 4015.0, 6 * 4016.0, 
     +    6 * 4017.0, 6 * 4018.0, 6 * 4019.0, 6 * 4020.0, 
     +    6 * 4021.0, 6 * 4022.0, 6 * 4023.0, 6 * 4024.0, 
     +    6 * 4025.0, 6 * 4026.0, 6 * 4027.0, 6 * 4028.0, 
     +    6 * 4029.0, 6 * 4030.0, 6 * 4031.0, 6 * 4032.0, 
     +    6 * 4033.0, 6 * 4034.0, 6 * 4035.0, 6 * 4036.0, 
     +    6 * 4037.0, 6 * 4038.0, 6 * 4039.0, 6 * 4040.0, 
     +    6 * 4041.0, 
     +    7 * 5001.0, 7 * 5002.0, 7 * 5003.0, 7 * 5004.0,      ! Brick vars
     +    7 * 5005.0, 
     +    7 * 6001.0, 7 * 6002.0, 7 * 6003.0, 7 * 6004.0,      ! Thick shells
     +    7 * 6005.0, 
     +    6 * 7001.0, 6 * 7002.0, 6 * 7003.0, 6 * 7004.0,      ! Beam vars
     +    33 * 8001.0, 33 * 8002.0, 33 * 8003.0, 33 * 8004.0,  ! Surf vars
     +    33 * 8005.0,
     +    33 * 9001.0, 33 * 9002.0, 33 * 9003.0, 33 * 9004.0 / ! Surf vars

c.....Nodes 
      real node_pos(3,41)
      data node_pos/
     +    0.0, 0.0, 0.0,  0.0, 1.0, 0.0,  0.0, 1.0, 1.0,  0.0, 0.0, 1.0, 
     +    1.0, 0.0, 0.0,  1.0, 1.0, 0.0,  1.0, 1.0, 1.0,  1.0, 0.0, 1.0, 
     +    2.0, 0.0, 0.0,  2.0, 1.0, 0.0,  2.0, 1.0, 1.0,  2.0, 0.0, 1.0, 
     +    3.0, 0.0, 0.0,  3.0, 1.0, 0.0,  3.0, 1.0, 1.0,  3.0, 0.0, 1.0, 
     +    4.0, 0.0, 0.0,  4.0, 1.0, 0.0,  4.0, 1.0, 1.0,  4.0, 0.0, 1.0, 
     +    5.0, 0.0, 0.0,  5.0, 1.0, 0.0,  5.0, 1.0, 1.0,  5.0, 0.0, 1.0,
     +    0.5, 0.5, 2.0,  1.5, 0.5, 2.0,  2.5, 0.5, 2.0,  3.5, 0.5, 2.0,
     +    4.5, 0.5, 2.0,
     +    0.0, -1.0, 0.0,  1.0, -1.0, 0.0,  2.0, -1.0, 0.0,
     +    3.0, -1.0, 0.0,  4.0, -1.0, 0.0,  5.0, -1.0, 0.0,
     +    0.0, -1.0, 1.0,  1.0, -1.0, 1.0,  2.0, -1.0, 1.0,
     +    3.0, -1.0, 1.0,  4.0, -1.0, 1.0,  5.0, -1.0, 1.0 /


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

c.....Surf state variables
      character*(m_max_name_len) surf_short(33), surf_long(33)
      data
     +    surf_short /
     +        'sxxmid', 'syymid', 'szzmid', 'sxymid', 'syzmid', 
     +        'szxmid', 'eeffmid', 'sxxin', 'syyin', 'szzin', 'sxyin',
     +        'syzin', 'szxin', 'eeffin', 'sxxout', 'syyout', 'szzout', 
     +        'sxyout', 'syzout', 'szxout', 'eeffout', 'mxx', 'myy',
     +        'mxy', 'qxx', 'qyy', 'nxx', 'nyy', 'nxy', 'th', 'edv1',
     +        'edv2', 'ie' /
      data
     +    surf_long /
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

      integer surf_flags(33)
      data surf_flags / 33*m_per_facet /

c.....Short and long group names, to be used when defining connectivities 
c.....and when defining subrecords
      character*(m_max_name_len) global_s, global_l, nodal_s, nodal_l, 
     +                           beams_s, beams_l, surfs_s, surfs_l, 
     +                           hexs_s, hexs_l, thick_shells_s, 
     +                           thick_shells_l, material_s, 
     +                           material_l, rigid_wall_s, rigid_wall_l
      data global_s, global_l, nodal_s, nodal_l, beams_s, beams_l, 
     +     surfs_s, surfs_l, hexs_s, hexs_l, thick_shells_s, 
     +     thick_shells_l, material_s, material_l, rigid_wall_s, 
     +     rigid_wall_l /
     +      'glob', 'Global', 'node', 'Nodal', 'beam', 'Beams', 'surf', 
     +      'Surfs', 'hex', 'Bricks', 'thick', 'Thick Shells', 'mat', 
     +      'Material', 'wall', 'Rigid Wall' /


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

c.....Surf 1:5 facets; 4 nodes
      integer surf_1_5(4,5)
      data surf_1_5 /
     +    1, 5, 6, 2,
     +    5, 9, 10, 6,
     +    9, 13, 14, 10,
     +    13, 17, 18, 14,
     +    17, 21, 22, 18 /

c.....Surf 1:5 facets; 4 nodes
      integer surf_2_4(4,4)
      data surf_2_4 /
     +    34, 35, 21, 17,
     +    33, 34, 17, 13,
     +    32, 33, 13, 9,
     +    31, 32, 9, 5 /

c.....Hex 1:5 connectivities; 8 nodes, 1 material, 1 part
      integer hex_1_5(10,5)
      data hex_1_5 /
     +    30, 31, 5, 1, 36, 37, 8, 4, 3, 1,
     +    1, 5, 6, 2, 4, 8, 7, 3, 3, 1,
     +    5, 9, 10, 6, 8, 12, 11, 7, 3, 1,
     +    9, 13, 14, 10, 12, 16, 15, 11, 3, 1,
     +    13, 17, 18, 14, 16, 20, 19, 15, 3, 1/

c.....Hex 6:10 connectivities; 8 nodes, 1 material, 1 part
      integer hex_6_10(10,5)
      data hex_6_10 /
     +    17, 21, 22, 18, 20, 24, 23, 19, 2, 1,
     +    34, 35, 21, 17, 40, 41, 24, 20, 2, 1,
     +    33, 34, 17, 13, 39, 40, 20, 16, 2, 1,
     +    32, 33, 13, 9, 38, 39, 16, 12, 2, 1,
     +    31, 32, 9, 5, 37, 38, 12, 8, 2, 1/

c.....Beam 1:4 connectivities; 3 nodes, 1 material, 1 part
      integer beam_1_4(5,4), beam_list(4)
      data 
     +    beam_1_4 /
     +        25, 26, 1, 1, 2,    26, 27, 1, 1, 2,   27, 28, 1, 1, 2, 
     +        28, 29, 1, 1, 2 /,
     +    beam_list / 1, 2, 3, 4 /

c.....Total quantity of materials, rigid walls
      integer qty_materials, qty_rwalls
      data qty_materials, qty_rwalls / 3, 2 /


c.....And away we go...


c.....MUST open the family

      call mf_open( 'surfo', 'data', 'AwPs', fid, stat )
      if ( stat .ne. 0 ) then
          call mf_print_error( 'mf_open', stat )
          stop
      end if

c.....Write a title
      call mf_wrt_string( fid, 'title', "Surface Class Test", 
     +                    stat )
      call mf_print_error( 'mf_wrt_string', stat )

c.....Query the library version
      ver_string = m_blank
      call mf_query_family( fid, m_lib_version, m_dont_care, m_blank,
     +                      ver_string, stat )
      call mf_print_error( 'mf_query_family (m_lib_version)', stat )
      write( *, 122 ) ver_string
122   format( 'Mili library version is ', a )
      
c.....MUST define state variables prior to binding subrecord definitions

c     Note: 1) Re-use of svar_types array since everything is of type svar_types
c           2) Variables are defined in groups by object type only so that
c              the "short" name arrays can be re-used during subrecord
c              definitions.  They could be defined in any order over one
c              or any number of calls to mf_def_svars() as definition
c              is completely independent of formatting the state record.
    
      call mf_def_svars( fid, 8, svar_types, global_short, 
     +                   global_long, stat )
      call mf_print_error( 'mf_def_svars', stat )
      
      call mf_def_vec_svar( fid, svar_types, 3, 
     +                      global_short(9), global_long(9), 
     +                      global_short(12), global_long(12), 
     +                      stat )
      call mf_print_error( 'mf_def_vec_svar', stat )
      
      call mf_def_svars( fid, 2, svar_types, 
     +                   global_short(10), global_long(10), stat )
      call mf_print_error( 'mf_def_svars', stat )
      
      call mf_def_vec_svar( fid, svar_types, 3, nodal_short(1), 
     +                      nodal_long(1), nodal_short(4), 
     +                      nodal_long(4), stat )
      call mf_print_error( 'mf_def_vec_svar', stat )
      
      call mf_def_vec_svar( fid, svar_types, 3, nodal_short(2), 
     +                      nodal_long(2), nodal_short(7), 
     +                      nodal_long(7), stat )
      call mf_print_error( 'mf_def_vec_svar', stat )
      
      call mf_def_vec_svar( fid, svar_types, 3, nodal_short(3), 
     +                      nodal_long(3), nodal_short(10), 
     +                      nodal_long(10), stat )
      call mf_print_error( 'mf_def_vec_svar', stat )
    
      call mf_def_svars( fid, 6, svar_types, beam_short, beam_long, 
     +                   stat )
      call mf_print_error( 'mf_def_svars', stat )
    
      call mf_def_svars( fid, 33, svar_types, surf_short, surf_long, 
     +                   stat )
      call mf_print_error( 'mf_def_svars', stat )
    
      call mf_def_svars( fid, 7, svar_types, hex_short, hex_long, stat )
      call mf_print_error( 'mf_def_svars', stat )
      
c.....MUST create & define the mesh prior to binding subrecord definitions

c.....Initialize the mesh
      call mf_make_umesh( fid, 'Surface Mesh', 3, mesh_id, stat )
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

      call mf_def_conn_seq( fid, mesh_id, hexs_s, 4, 5, 
     +                      hex_1_5(1,4), stat )
      call mf_print_error( 'mf_def_conn_seq', stat )

c.....Define a surf class
      call mf_def_class( fid, mesh_id, m_surf, surfs_s, surfs_l, 
     +                   stat )
      call mf_print_error( 'mf_def_class (surfs)', stat )

      call mf_def_conn_surface( fid, mesh_id, surfs_s, 5, 
     +                          surf_1_5, surf_id, stat )
      call mf_print_error( 'mf_def_conn_surface', stat )
    
      call mf_def_conn_surface( fid, mesh_id, surfs_s, 4, 
     +                          surf_2_4, surf_id, stat )
      call mf_print_error( 'mf_def_conn_surface', stat )
    
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
c    +                    global_short, m_blank, global, 
c    +                    m_block_obj_fmt, 1, obj_blocks, stat )
      call mf_def_subrec( fid, srec_id, global_l, m_object_ordered, 6, 
     +                    global_short, global_s, m_dont_care, 
     +                    1, m_dont_care, stat )
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
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 41
      call mf_def_subrec( fid, srec_id, nodal_long, m_object_ordered,
     +                    1, nodal_short, nodal_s, m_block_obj_fmt,
     +                    1, obj_blocks, stat )
c     In the above case, the existing state variable definitions of
c     three vector variables (position, velocity, and acceleration)
c     should be replaced with one call to mf_def_svars() to define all
c     nine scalars, although the existing definitions as vectors would
c     actually work since each vector component is in fact created as a
c     scalar variable and can be used as such independently of the
c     vector definition that created it.
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 41
      call mf_def_subrec( fid, srec_id, nodal_l, m_result_ordered, 2, 
     +                    nodal_short(2), nodal_s, 
     +                    m_block_obj_fmt, 1, obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Hex data, brick class
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 5
      call mf_def_subrec( fid, srec_id, hexs_l, m_object_ordered, 7, 
     +                    hex_short, hexs_s, m_block_obj_fmt, 
     +                    1, obj_blocks, stat )
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
     +                    beam_short, beams_s, 
     +                    m_block_obj_fmt, 1, obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Surf data
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 2
      call mf_def_surf_subrec( fid, srec_id, surfs_l, m_object_ordered,
     +                    33, surf_short, surfs_s, 
     +                    m_block_obj_fmt, 1, obj_blocks, surf_flags,
     +                    stat )
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
c     doesn't know that).  The state record has 654 real elements in it.

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
      
      call mf_wrt_stream( fid, svar_types,  26, state_record, stat )
      call mf_print_error( 'mf_wrt_stream', stat )

      call mf_wrt_stream( fid, svar_types, 123, node_pos, stat )
      call mf_print_error( 'mf_wrt_stream', stat )

      call mf_wrt_stream( fid, svar_types, 637, state_record(27), stat )
      call mf_print_error( 'mf_wrt_stream', stat )

c.....Write another record in arbitrary (though sequentially correct)
c     pieces.
      time = time + 0.01
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, svar_types,  26, state_record, stat )
      call mf_print_error( 'mf_wrt_stream', stat )

      call mf_wrt_stream( fid, svar_types, 123, node_pos, stat )
      call mf_print_error( 'mf_wrt_stream', stat )

      call mf_wrt_stream( fid, svar_types, 200, state_record(27), stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      call mf_wrt_stream( fid, svar_types, 200, state_record(227), stat)
      call mf_print_error( 'mf_wrt_stream', stat )
      
      call mf_wrt_stream( fid, svar_types, 237, state_record(427), stat)
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

      time = time + 0.01
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
c.....In the statements below, remember that it is just an artifact of this
c     hard-coded example that all of the state data is in one big array,
c     allowing me to access each subrecord by knowing the correct index.
c     A real application might re-use the same buffer over and over or 
c     assemble each subrecord into a different buffer.
      
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
      call mf_wrt_subrec( fid, rigid_wall_l, 1, 2, state_record(25), 
     +                    stat )
      call mf_print_error( 'mf_wrt_subrec', stat )

c.....Note that as a practical matter for current Dyna and Nike data,
c     it would be much more efficient to have the above mesh, material,
c     and wall data in a buffer and write it all at once using
c     mf_wrt_stream().  These types of data will generally have a few
c     tens or at most hundreds of words so it would be less efficient
c     to perform three small writes rather than one "less-small" write.

c.....Nodal data, all at once.
      call mf_wrt_subrec( fid, nodal_l, 1, 2, state_record(27), stat )
      call mf_print_error( 'mf_wrt_subrec', stat )

      call mf_wrt_subrec( fid, nodal_long, 1, 41, node_pos, stat )
      call mf_print_error( 'mf_wrt_subrec', stat )
      
c.....Beam data, all at once.
      call mf_wrt_subrec( fid, beams_l, 1, 4, state_record(343), stat )
      call mf_print_error( 'mf_wrt_subrec', stat )

c.....Thick Shells, all at once.
      call mf_wrt_subrec( fid, thick_shells_l, 1, 5, state_record(308), 
     +                    stat )
      call mf_print_error( 'mf_wrt_subrec', stat )

c.....Hex data, all at once.
      call mf_wrt_subrec( fid, hexs_l, 1, 5, state_record(273), stat )
      call mf_print_error( 'mf_wrt_subrec', stat )

c.....Surface data, all at once.
      call mf_wrt_subrec( fid, surfs_l, 1, 2, state_record(367), stat )
      call mf_print_error( 'mf_wrt_subrec', stat )

c.....If one were so inclined, both mf_wrt_subrec() and mf_wrt_stream()
c     could be used to write data for one state.  Just be aware that
c     (1) mf_wrt_subrec() seeks the file position to the correct position
c     for writing, while mf_wrt_stream() writes at the current position,
c     and (2) both calls leave the file position where it falls after 
c     writing the data.

      if(0.EQ.1)then
c.....Query for class existence
      idims(1) = 2
      call mf_query_family( fid, m_class_exists, idims, surfs_s, idum,
     +                      stat )
      call mf_print_error( 'mf_query_family (m_class_exists)', stat )
      idims(1) = mesh_id
      call mf_query_family( fid, m_class_exists, idims, '', idum,
     +                      stat )
      call mf_print_error( 'mf_query_family (m_class_exists)', stat )
      idims(1) = mesh_id
      call mf_query_family( fid, m_class_exists, idims, surfs_s, idum,
     +                      stat )
      call mf_print_error( 'mf_query_family (m_class_exists)', stat )
      if ( idum.ne.0 ) print *, 'Class "', surfs_s(1:1), '" found.'

      call mf_query_family( fid, m_class_exists, idims, 'noclass', idum,
     +                      stat )
      call mf_print_error( 'mf_query_family (m_class_exists)', stat )
      if ( idum.eq.0 ) print *, 'Class "noclass" not found.'
      endif

c.....MUST close the family when finished

      call mf_close( fid, stat )
      call mf_print_error( 'mf_close', stat )
      
      end
