c $Id: partre.f,v 1.9 2002/12/11 22:16:27 speck Exp $

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


      program partre
c.....Creates an initial db ("prest...") intended as input to partre_b for
c.....testing Mili restart functionality.
      
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
     +    mf_def_vec_svar, mf_restart_at_state, mf_restart_at_file,
     +    mf_partition_state_data, mf_close_srec
     
      integer fid, mesh_id, srec_id, suffix, st_idx
      integer iiargs(2), oiargs(2)
      integer stat
      real time
c     integer obj_blocks(2,1)
      integer obj_blocks(2,5)

c.....Initialize object-ordered state records
      real state_record1(54)
      data state_record1 /
     +    -0.50, 1.25, 1.25,  1250.0, 50.0, 90.0,  
     +    -0.50, 5.00, 1.25,   900.0, 50.0, 85.0, 
     +    -0.50, 8.75, 1.25,   790.0, 50.0, 75.0,  
     +    -0.50, 1.25, 5.00,   900.0, 40.0, 80.0, 
     +    -0.50, 5.00, 5.00,   800.0, 40.0, 75.0,  
     +    -0.50, 8.75, 5.00,   670.0, 40.0, 70.0, 
     +    -0.50, 1.25, 8.75,   760.0, 20.0, 75.0,  
     +    -0.50, 5.00, 8.75,   650.0, 20.0, 70.0,
     +    -0.50, 8.75, 8.75,   520.0, 20.0, 60.0/
      real state_record2(54)
      data state_record2 /
     +    1.10, 1.25, 1.25,  1250.0, 50.0, 90.0,  
     +    0.925, 5.00, 1.25,   900.0, 50.0, 85.0, 
     +    0.83, 8.75, 1.25,   790.0, 50.0, 75.0,  
     +    0.87, 1.25, 5.00,   900.0, 40.0, 80.0, 
     +    0.77, 5.00, 5.00,   800.0, 40.0, 75.0,  
     +    0.70, 8.75, 5.00,   670.0, 40.0, 70.0, 
     +    0.72, 1.25, 8.75,   760.0, 20.0, 75.0,  
     +    0.63, 5.00, 8.75,   650.0, 20.0, 70.0,
     +    0.55, 8.75, 8.75,   520.0, 20.0, 60.0/
      real state_record3(54)
      data state_record3 /
     +    2.20, 1.25, 1.25,  1250.0, 50.0, 90.0,  
     +    1.85, 5.00, 1.25,   900.0, 50.0, 85.0, 
     +    1.66, 8.75, 1.25,   790.0, 50.0, 75.0,  
     +    1.74, 1.25, 5.00,   900.0, 40.0, 80.0, 
     +    1.54, 5.00, 5.00,   800.0, 40.0, 75.0,  
     +    1.40, 8.75, 5.00,   670.0, 40.0, 70.0, 
     +    1.44, 1.25, 8.75,   760.0, 20.0, 75.0,  
     +    1.26, 5.00, 8.75,   650.0, 20.0, 70.0,
     +    1.10, 8.75, 8.75,   520.0, 20.0, 60.0/
      real state_record4(54)
      data state_record4 /
     +    3.30, 1.25, 1.25,  1250.0, 50.0, 90.0,  
     +    2.775, 5.00, 1.25,   900.0, 50.0, 85.0, 
     +    2.49, 8.75, 1.25,   790.0, 50.0, 75.0,  
     +    2.61, 1.25, 5.00,   900.0, 40.0, 80.0, 
     +    2.31, 5.00, 5.00,   800.0, 40.0, 75.0,  
     +    2.10, 8.75, 5.00,   670.0, 40.0, 70.0, 
     +    2.16, 1.25, 8.75,   760.0, 20.0, 75.0,  
     +    1.89, 5.00, 8.75,   650.0, 20.0, 70.0,
     +    1.65, 8.75, 8.75,   520.0, 20.0, 60.0/
      real state_record5(54)
      data state_record5 /
     +    4.40, 1.25, 1.25,  1250.0, 50.0, 90.0,  
     +    3.70, 5.00, 1.25,   900.0, 50.0, 85.0, 
     +    3.32, 8.75, 1.25,   790.0, 50.0, 75.0,  
     +    3.48, 1.25, 5.00,   900.0, 40.0, 80.0, 
     +    3.08, 5.00, 5.00,   800.0, 40.0, 75.0,  
     +    2.80, 8.75, 5.00,   670.0, 40.0, 70.0, 
     +    2.88, 1.25, 8.75,   760.0, 20.0, 75.0,  
     +    2.52, 5.00, 8.75,   650.0, 20.0, 70.0,
     +    2.20, 8.75, 8.75,   520.0, 20.0, 60.0/
      real state_record6(54)
      data state_record6 /
     +    5.50, 1.25, 1.25,  1250.0, 50.0, 90.0,  
     +    4.625, 5.00, 1.25,   900.0, 50.0, 85.0, 
     +    4.15, 8.75, 1.25,   790.0, 50.0, 75.0,  
     +    4.35, 1.25, 5.00,   900.0, 40.0, 80.0, 
     +    3.85, 5.00, 5.00,   800.0, 40.0, 75.0,  
     +    3.50, 8.75, 5.00,   670.0, 40.0, 70.0, 
     +    3.60, 1.25, 8.75,   760.0, 20.0, 75.0,  
     +    3.15, 5.00, 8.75,   650.0, 20.0, 70.0,
     +    2.75, 8.75, 8.75,   520.0, 20.0, 60.0/
      real state_record7(54)
      data state_record7 /
     +    6.60, 1.25, 1.25,  1250.0, 50.0, 90.0,  
     +    5.55, 5.00, 1.25,   900.0, 50.0, 85.0, 
     +    4.98, 8.75, 1.25,   790.0, 50.0, 75.0,  
     +    5.22, 1.25, 5.00,   900.0, 40.0, 80.0, 
     +    4.62, 5.00, 5.00,   800.0, 40.0, 75.0,  
     +    4.20, 8.75, 5.00,   670.0, 40.0, 70.0, 
     +    4.32, 1.25, 8.75,   760.0, 20.0, 75.0,  
     +    3.78, 5.00, 8.75,   650.0, 20.0, 70.0,
     +    3.30, 8.75, 8.75,   520.0, 20.0, 60.0/
      real state_record8(54)
      data state_record8 /
     +    7.70, 1.25, 1.25,  1250.0, 50.0, 90.0,  
     +    6.475, 5.00, 1.25,   900.0, 50.0, 85.0, 
     +    5.81, 8.75, 1.25,   790.0, 50.0, 75.0,  
     +    6.09, 1.25, 5.00,   900.0, 40.0, 80.0, 
     +    5.39, 5.00, 5.00,   800.0, 40.0, 75.0,  
     +    4.90, 8.75, 5.00,   670.0, 40.0, 70.0, 
     +    5.04, 1.25, 8.75,   760.0, 20.0, 75.0,  
     +    4.41, 5.00, 8.75,   650.0, 20.0, 70.0,
     +    3.85, 8.75, 8.75,   520.0, 20.0, 60.0/
      real state_record9(54)
      data state_record9 /
     +    8.80, 1.25, 1.25,  1250.0, 50.0, 90.0,  
     +    7.40, 5.00, 1.25,   900.0, 50.0, 85.0, 
     +    6.64, 8.75, 1.25,   790.0, 50.0, 75.0,  
     +    6.96, 1.25, 5.00,   900.0, 40.0, 80.0, 
     +    6.16, 5.00, 5.00,   800.0, 40.0, 75.0,  
     +    5.60, 8.75, 5.00,   670.0, 40.0, 70.0, 
     +    5.76, 1.25, 8.75,   760.0, 20.0, 75.0,  
     +    5.04, 5.00, 8.75,   650.0, 20.0, 70.0,
     +    4.40, 8.75, 8.75,   520.0, 20.0, 60.0/
      real state_record10(54)
      data state_record10 /
     +    9.90, 1.25, 1.25,  1250.0, 50.0, 90.0,  
     +    8.325, 5.00, 1.25,   900.0, 50.0, 85.0, 
     +    7.47, 8.75, 1.25,   790.0, 50.0, 75.0,  
     +    7.83, 1.25, 5.00,   900.0, 40.0, 80.0, 
     +    6.93, 5.00, 5.00,   800.0, 40.0, 75.0,  
     +    6.30, 8.75, 5.00,   670.0, 40.0, 70.0, 
     +    6.48, 1.25, 8.75,   760.0, 20.0, 75.0,  
     +    5.67, 5.00, 8.75,   650.0, 20.0, 70.0,
     +    4.95, 8.75, 8.75,   520.0, 20.0, 60.0/
      real state_record11(54)
      data state_record11 /
     +    11.00, 1.25, 1.25,  1250.0, 50.0, 90.0,  
     +    9.25, 5.00, 1.25,   900.0, 50.0, 85.0, 
     +    8.30, 8.75, 1.25,   790.0, 50.0, 75.0,  
     +    8.70, 1.25, 5.00,   900.0, 40.0, 80.0, 
     +    7.70, 5.00, 5.00,   800.0, 40.0, 75.0,  
     +    7.00, 8.75, 5.00,   670.0, 40.0, 70.0, 
     +    7.20, 1.25, 8.75,   760.0, 20.0, 75.0,  
     +    6.30, 5.00, 8.75,   650.0, 20.0, 70.0,
     +    5.50, 8.75, 8.75,   520.0, 20.0, 60.0/


c.....State variables per mesh object type

c.....All short names MUST be unique
      
c.....Particle state variables
c
c.....NOTE - Particle position vector name is "partpos" - Griz4 looks
c.....for this name
      character*(m_max_name_len) part_short(7), part_long(7)
      data 
     +    part_short /
     +        'partpos', 'temp', 'ke', 'pe', 'ux', 'uy', 'uz'/ 
     +    part_long /
     +        'Particle Position', 'Temperature', 'Kinetic Energy',
     +        'Potential Energy', 
     +        'X Position', 'Y Position', 'Z Position'/

c.....State var types; they're all floats, so just define one array as big
c.....as the largest block of svars that will be defined in one call
      integer svar_types(7)
      data svar_types / 7*m_real /

c.....Class names, to be used when defining connectivities and when
c.....defining subrecords
c
c.....NOTE - Particle class short name is "part" - Griz4 looks for this
c.....name to distinguish from other M_UNIT classes.
      character*(m_max_name_len) particle_s, particle_l, nodal_s, 
     +    nodal_l, hexs_s, hexs_l, global_s, global_l,
     +    material_s, material_l
      data particle_s, particle_l, nodal_s, nodal_l, hexs_s, hexs_l,
     +    global_s, global_l, material_s, material_l/
     +        'part', 'Particle', 'node', 'Node', 'h', 'Brick',
     +        'mesh', 'Mesh', 'mat', 'Material'/


c.....MESH GEOMETRY


c.....Nodes 1:8
      real node_1_8(3,8)
      data node_1_8/
     +    0.0, 0.0, 0.0,  10.0, 0.0, 0.0,  10.0, 10.0, 0.0,  
     +    0.0, 10.0, 0.0,  0.0, 0.0, 10.0,  10.0, 0.0, 10.0,  
     +    10.0, 10.0, 10.0,  0.0, 10.0, 10.0/

c.....Hex 1:1 connectivities; 8 nodes, 1 material, 1 part
      integer hex_1_1(10,1)
      data hex_1_1 /
     +    1, 2, 3, 4, 5, 6, 7, 8, 1, 1/

c.....Total quantity of materials
      integer qty_materials
      data qty_materials / 1 /


c.....And away we go...


c.....MUST open the family

      call mf_open( 'prest', 'data', 'AwPs', fid, stat )
      if ( stat .ne. 0 ) then
          call mf_print_error( 'mf_open', stat )
          stop
      end if

c.....Write a title
      call mf_wrt_string( fid, 'title', "Restart operations test db", 
     +                    stat )
      call mf_print_error( 'mf_wrt_string', stat )
      
      
c.....MUST define state variables prior to binding subrecord definitions

c     Note: 1) Re-use of svar_types array since everything is of type m_real
c           2) Variables are defined in groups by object type only so that
c              the "short" name arrays can be re-used during subrecord
c              definitions.  They could be defined in any order over one
c              or any number of calls to mf_def_svars() as definition
c              is completely independent of formatting the state record.
      
      call mf_def_vec_svar( fid, m_real, 3, part_short(1), 
     +                      part_long(1), part_short(5), 
     +                      part_long(5), stat )
      call mf_print_error( 'mf_def_vec_svar', stat )
      
      call mf_def_svars( fid, 3, svar_types, part_short(2), 
     +                   part_long(2), stat )
      call mf_print_error( 'mf_def_svars', stat )

c.....Test flushing
      call mf_flush( fid, m_non_state_data, stat )
      call mf_print_error( 'mf_flush', stat )
      
c.....MUST create & define the mesh prior to binding subrecord definitions

c.....Initialize the mesh
      call mf_make_umesh( fid, 'Hex mesh', 3, mesh_id, stat )
      call mf_print_error( 'mf_make_umesh', stat )

c.....Test flushing
      call mf_flush( fid, m_non_state_data, stat )
      call mf_print_error( 'mf_flush', stat )

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
      call mf_print_error( 'mf_def_class (mesh)', stat )
    
      call mf_def_class_idents( fid, mesh_id, global_s, 1, 1, stat )
      call mf_print_error( 'mf_def_class_idents (mesh)', stat )

c.....Define the material class.
      call mf_def_class( fid, mesh_id, m_mat, material_s, material_l, 
     +                   stat )
      call mf_print_error( 'mf_def_class (material)', stat )
    
      call mf_def_class_idents( fid, mesh_id, material_s, 1, 1, stat )
      call mf_print_error( 'mf_def_class_idents (material)', stat )

c.....Test flushing
      call mf_flush( fid, m_non_state_data, stat )
      call mf_print_error( 'mf_flush', stat )

c.....Create complex mesh object classes.  First create each class
c     from a superclass as above, then use special calls to supply
c     appropriate attributes (i.e., coordinates for nodes and
c     connectivities for elements).

c.....Define a node class.
      call mf_def_class( fid, mesh_id, m_node, nodal_s, nodal_l, stat )
      call mf_print_error( 'mf_def_class (nodal)', stat )
    
      call mf_def_nodes( fid, mesh_id, nodal_s, 1, 8, node_1_8, stat )
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

c.....Define a hex class
      call mf_def_class( fid, mesh_id, m_hex, hexs_s, hexs_l, stat )
      call mf_print_error( 'mf_def_class (hexs)', stat )

c.....Test flushing
      call mf_flush( fid, m_non_state_data, stat )
      call mf_print_error( 'mf_flush', stat )

      call mf_def_conn_seq( fid, mesh_id, hexs_s, 1, 1, 
     +                      hex_1_1, stat )
      call mf_print_error( 'mf_def_conn_seq', stat )

c.....Define a particle class
      call mf_def_class( fid, mesh_id, m_unit, particle_s, 
     +                   particle_l, stat )
      call mf_print_error( 'mf_def_class (part)', stat )

c.....Test flushing
      call mf_flush( fid, m_non_state_data, stat )
      call mf_print_error( 'mf_flush', stat )

      call mf_def_class_idents( fid, mesh_id, particle_s, 1, 9, 
     +                          stat )
      call mf_print_error( 'mf_def_class_idents (part)', stat )


c.....Test flushing
      call mf_flush( fid, m_non_state_data, stat )
      call mf_print_error( 'mf_flush', stat )

c.....MUST create and define the state record format

c.....Create a state record format descriptor
      call mf_open_srec( fid, mesh_id, srec_id, stat )
      call mf_print_error( 'mf_open_srec', stat )

c.....Test flushing
      call mf_flush( fid, m_non_state_data, stat )
      call mf_print_error( 'mf_flush', stat )

c.....Particle data, part class
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 9
      call mf_def_subrec( fid, srec_id, particle_l, m_object_ordered,
     +                    4, part_short, particle_s, 
     +                    m_block_obj_fmt, 1, obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

      call mf_close_srec( fid, srec_id, stat )
      call mf_print_error( 'mf_close_srec', stat )


c.....SHOULD commit before writing state data so the family will be 
c     readable if a crash occurs during the analysis.
   
      call mf_flush( fid, m_non_state_data, stat )
      call mf_print_error( 'mf_flush', stat )

      call mf_limit_states( fid, 2, stat )
      call mf_print_error( 'mf_limit_states', stat )


c.....Write ten records, each with one call.
      time = 0.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 54, state_record1, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
     

c.....Test m_next_state_loc query
      iiargs(1) = suffix
      iiargs(2) = st_idx
      call mf_query_family( fid, m_next_state_loc, iiargs, 
     +                      m_blank, oiargs, stat )
      call mf_print_error( 'mf_query_family', stat )
      print 100, iiargs(1), iiargs(2), oiargs(1), oiargs(2) 
      
      
      time = 1.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 54, state_record2, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
     

c.....Test m_next_state_loc query
      iiargs(1) = suffix
      iiargs(2) = st_idx
      call mf_query_family( fid, m_next_state_loc, iiargs, 
     +                      m_blank, oiargs, stat )
      call mf_print_error( 'mf_query_family', stat )
      print 100, iiargs(1), iiargs(2), oiargs(1), oiargs(2) 
      
    
c     call mf_partition_state_data( fid, stat )
c     call mf_print_error( 'mf_partition_state_data', stat )
      
      time = 2.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 54, state_record3, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 3.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 54, state_record4, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
    
c     call mf_partition_state_data( fid, stat )
c     call mf_print_error( 'mf_partition_state_data', stat )
      
      time = 4.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 54, state_record5, stat )
      call mf_print_error( 'mf_wrt_stream', stat )

      time = 5.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 54, state_record6, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
    
c     call mf_partition_state_data( fid, stat )
c     call mf_print_error( 'mf_partition_state_data', stat )
      
      time = 6.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 54, state_record7, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 7.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 54, state_record8, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
    
c     call mf_partition_state_data( fid, stat )
c     call mf_print_error( 'mf_partition_state_data', stat )
      
      time = 8.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 54, state_record9, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 9.0
      call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 54, state_record10, stat )
      call mf_print_error( 'mf_wrt_stream', stat )


c.....MUST close the family when finished

      call mf_close( fid, stat )
      call mf_print_error( 'mf_close', stat )
    
    
c.....Format test outputs
100   format( 'After state at (', I2, ', ', I2, '), next state at (', 
     +        I2, ', ', I2, ')' )
      
      end
