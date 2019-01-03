c *
c * Program to test time-dep stream and sub-record string output.
c *
c ************************************************************************
c * Modifications:
c *  I. R. Corey - February 27, 2012. Created
c *
c ************************************************************************

      program timedepstringdb

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
      integer i, j
      integer stat
      real time, state_val
      integer int_time

      integer obj_blocks(2,5)

      character(4)  mech_string_count
      character(20) mech_string

c.....Initialize object-ordered state records
      real state_record(54)
      data state_record /
     +    -0.50, 1.25, 1.25,  1250.0, 50.0, 90.0,  
     +    -0.50, 5.00, 1.25,   900.0, 50.0, 85.0, 
     +    -0.50, 8.75, 1.25,   790.0, 50.0, 75.0,  
     +    -0.50, 1.25, 5.00,   900.0, 40.0, 80.0, 
     +    -0.50, 5.00, 5.00,   800.0, 40.0, 75.0,  
     +    -0.50, 8.75, 5.00,   670.0, 40.0, 70.0, 
     +    -0.50, 1.25, 8.75,   760.0, 20.0, 75.0,  
     +    -0.50, 5.00, 8.75,   650.0, 20.0, 70.0,
     +    -0.50, 8.75, 8.75,   520.0, 20.0, 60.0/

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

      character*(m_max_name_len) mech_short(1)
      data 
     +    mech_short /
     +        'MechVar'/ 

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

      character*(m_max_name_len) mech(1)
      data mech/'mech'/

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

c
c*******************************
c Write a db using stream output
c*******************************
c

c.....MUST open the family

      call mf_open( 'timedepstringdb_stream', 'data', 'AwPs', fid, stat)
      if ( stat .ne. 0 ) then
          call mf_print_error( 'mf_open', stat )
          stop
      end if

c.....Write a title
      call mf_wrt_string( fid, 'title', "3D Particle db", 
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
      
      call mf_def_svars( fid, 1, m_string, mech_short(1), 
     +                   mech_short(1), stat )
      call mf_print_error( 'mf_def_svars', stat )

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
      call mf_print_error( 'mf_def_class (mesh)', stat )
    
      call mf_def_class_idents( fid, mesh_id, global_s, 1, 1, stat )
      call mf_print_error( 'mf_def_class_idents (mesh)', stat )

c.....Define the material class.
      call mf_def_class( fid, mesh_id, m_mat, material_s, material_l, 
     +                   stat )
      call mf_print_error( 'mf_def_class (material)', stat )
    
      call mf_def_class_idents( fid, mesh_id, material_s, 1, 1, stat )
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

      call mf_def_class( fid, mesh_id, m_unit, mech, mech, stat )

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

      call mf_def_conn_seq( fid, mesh_id, hexs_s, 1, 1, 
     +                      hex_1_1, stat )
      call mf_print_error( 'mf_def_conn_seq', stat )

c.....Define a particle class
      call mf_def_class( fid, mesh_id, m_unit, particle_s, 
     +                   particle_l, stat )
      call mf_print_error( 'mf_def_class (part)', stat )

      call mf_def_class_idents( fid, mesh_id, particle_s, 1, 9, 
     +                          stat )
      call mf_print_error( 'mf_def_class_idents (part)', stat )

      call mf_def_class_idents( fid, mesh_id, mech, 1, 20, 
     +                          stat )

c.....MUST create and define the state record format

c.....Create a state record format descriptor
      call mf_open_srec( fid, mesh_id, srec_id, stat )
      call mf_print_error( 'mf_open_srec', stat )

c.....Particle data, part class
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 9
      call mf_def_subrec( fid, srec_id, particle_l, m_object_ordered,
     +                    4, part_short, particle_s, 
     +                    m_block_obj_fmt, 1, obj_blocks, stat )

c.....MechMode data, mech class
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 20
      call mf_def_subrec( fid, srec_id, mech, m_object_ordered,
     +                    1, mech_short, mech, 
     +                    m_block_obj_fmt, 1, obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

      call mf_close_srec( fid, srec_id, stat )
      call mf_print_error( 'mf_close_srec', stat )


c.....SHOULD commit before writing state data so the family will be 
c     readable if a crash occurs during the analysis.
   
      call mf_flush( fid, m_non_state_data, stat )
      call mf_print_error( 'mf_commit', stat )

c.....Write ten records, each with one call.
      time = 0.0
      do 100 i=1, 100
          int_time = time
          write( mech_string_count, '(i4)' ) int_time
          write( mech_string, * ) 'MechMode: ' // mech_string_count 

          time = time+1
          call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
          call mf_print_error( 'mf_new_state', stat )
      
          call mf_wrt_stream( fid, m_real, 54, state_record, stat )
          call mf_print_error( 'mf_wrt_stream', stat )

          call mf_wrt_stream_char( fid, 20, mech_string, stat)
          call mf_print_error( 'mf_wrt_stream', stat )

c      call mf_wrt_subrec( fid, particle_l, 1, 7, state_record2, stat )
c      call mf_wrt_subrec( fid, mech, 1, 20, mech_data2, stat )
          
          do 20 j=1, 54
             state_val = state_record(j)
             state_val = state_val+(state_val*0.1)
             state_record(j) = state_val
             
20       continue
 100  continue

c.....MUST close the family when finished

      call mf_close( fid, stat )
      call mf_print_error( 'mf_close', stat )

c
c*****************************************
c Write a db using subrecord output method
c*****************************************
c

c.....MUST open the family

      call mf_open( 'timedepstringdb_subrec', 'data', 'AwPs', fid, stat)
      if ( stat .ne. 0 ) then
          call mf_print_error( 'mf_open', stat )
          stop
      end if

c.....Write a title
      call mf_wrt_string( fid, 'title', "3D Particle db", 
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
      
      call mf_def_svars( fid, 1, m_string, mech_short(1), 
     +                   mech_short(1), stat )
      call mf_print_error( 'mf_def_svars', stat )

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
      call mf_print_error( 'mf_def_class (mesh)', stat )
    
      call mf_def_class_idents( fid, mesh_id, global_s, 1, 1, stat )
      call mf_print_error( 'mf_def_class_idents (mesh)', stat )

c.....Define the material class.
      call mf_def_class( fid, mesh_id, m_mat, material_s, material_l, 
     +                   stat )
      call mf_print_error( 'mf_def_class (material)', stat )
    
      call mf_def_class_idents( fid, mesh_id, material_s, 1, 1, stat )
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

      call mf_def_class( fid, mesh_id, m_unit, mech, mech, stat )

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

      call mf_def_conn_seq( fid, mesh_id, hexs_s, 1, 1, 
     +                      hex_1_1, stat )
      call mf_print_error( 'mf_def_conn_seq', stat )

c.....Define a particle class
      call mf_def_class( fid, mesh_id, m_unit, particle_s, 
     +                   particle_l, stat )
      call mf_print_error( 'mf_def_class (part)', stat )

      call mf_def_class_idents( fid, mesh_id, particle_s, 1, 9, 
     +                          stat )
      call mf_print_error( 'mf_def_class_idents (part)', stat )

      call mf_def_class_idents( fid, mesh_id, mech, 1, 20, 
     +                          stat )

c.....MUST create and define the state record format

c.....Create a state record format descriptor
      call mf_open_srec( fid, mesh_id, srec_id, stat )
      call mf_print_error( 'mf_open_srec', stat )

c.....Particle data, part class
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 9
      call mf_def_subrec( fid, srec_id, particle_l, m_object_ordered,
     +                    4, part_short, particle_s, 
     +                    m_block_obj_fmt, 1, obj_blocks, stat )

c.....MechMode data, mech class
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 20
      call mf_def_subrec( fid, srec_id, mech, m_object_ordered,
     +                    1, mech_short, mech, 
     +                    m_block_obj_fmt, 1, obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

      call mf_close_srec( fid, srec_id, stat )
      call mf_print_error( 'mf_close_srec', stat )


c.....SHOULD commit before writing state data so the family will be 
c     readable if a crash occurs during the analysis.
   
      call mf_flush( fid, m_non_state_data, stat )
      call mf_print_error( 'mf_commit', stat )

c.....Write ten records, each with one call.
      time = 0.0
      do 200 i=1, 100
          int_time = time
          write( mech_string_count, '(i4)' ) int_time
          write( mech_string, * ) 'MechMode: ' // mech_string_count 

          time = time+1
          call mf_new_state( fid, srec_id, time, suffix, st_idx, stat )
          call mf_print_error( 'mf_new_state', stat )
      
          call mf_wrt_subrec( fid, particle_l, 1, 7, state_record, stat)
          call mf_print_error( 'mf_wrt_stream', stat )

          call mf_wrt_subrec( fid, mech, 1, 20, mech_string, stat )
          call mf_print_error( 'mf_wrt_stream', stat )
          
          do 120 j=1, 54
             state_val = state_record(j)
             state_val = state_val+(state_val*0.1)
             state_record(j) = state_val
             
 120      continue
 200  continue

c.....MUST close the family when finished

      call mf_close( fid, stat )
      call mf_print_error( 'mf_close', stat )
      end
