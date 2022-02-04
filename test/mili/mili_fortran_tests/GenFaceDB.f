c $Id

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


      program genfacedb
      
      implicit none

      include "mili_fparam.h"
      
      external
     +    mf_open, mf_close, mf_wrt_scalar, mf_wrt_array, 
     +    mf_wrt_string, mf_make_umesh, mf_def_class, mf_def_nodes, 
     +    mf_def_class_idents, mf_def_conn_seq, mf_def_conn_arb, 
     +    mf_get_mesh_id, mf_def_svars, mf_def_arr_svar, 
     +    mf_make_srec, mf_def_subrec, mf_flush, mf_new_state, 
     +    mf_wrt_stream, mf_wrt_subrec, mf_limit_states, 
     +    mf_suffix_width, mf_print_error, mf_def_vec_arr_svar,
     +    mf_def_vec_svar
     
      integer fid, mesh_id, srec_id
      integer stat
      real time
c     integer obj_blocks(2,1)
      integer obj_blocks(2,5)
      integer obj_list(5)

c.....Initialize a sample object-ordered state record with load curve
c     numbers (in floating point form).
      real state_record1(3)
      data state_record1 / 1.0, 2.0, 3.0 /

c.....State variables per mesh object type

c.....All short names MUST be unique

c.....Quad state variable
      character*(m_max_name_len) face_short, face_long
      data
     +    face_short, face_long / 'lc', 'Load Curve' /

c.....Group names, to be used when defining connectivities and when
c.....defining subrecords
      character*(m_max_name_len) global, nodal, face, material
      data global, nodal, face, material / 
     +    'Global', 'Nodal', 'Face', 'Material' /


c.....MESH GEOMETRY


c.....Nodes 1:8
      real node_1_8(3,8)
      data node_1_8/
     +    0.0, 0.0, 0.0,  1.0, 0.0, 0.0,  3.0, 0.0, 0.0,  6.0, 0.0, 0.0, 
     +    0.0, 1.0, 0.0,  1.0, 1.0, 0.0,  3.0, 1.0, 0.0,  6.0, 1.0, 0.0/

c.....Face 1:3 connectivities; 4 nodes, 1 material, 1 part
      integer face_1_3(6,3)
      data face_1_3 /
     +    1, 2, 6, 5, 1, 1,
     +    2, 3, 7, 6, 2, 1,
     +    3, 4, 8, 7, 3, 1/

c.....Total quantity of materials
      integer qty_materials
      data qty_materials / 3 /


c.....And away we go...


c.....MUST open the family

      call mf_open( 'GenFaceDB.plt', '.', 'AwPs', fid, stat )
      if ( stat .ne. 0 ) then
          call mf_print_error( 'mf_open', stat )
          stop
      end if

c.....Define title for Mili Database
      call mf_wrt_string(fid, "title", "GenFaceDB Test", stat)
      
c.....MUST define state variables prior to binding subrecord definitions

      call mf_def_svars( fid, 1, m_real, face_short, 
     +                   face_long, stat )
      call mf_print_error( 'mf_def_svars', stat )

      
c.....MUST create & define the mesh prior to binding subrecord definitions

c.....Initialize the mesh
      call mf_make_umesh( fid, 'Face mesh', 3, mesh_id, stat )
      call mf_print_error( 'mf_make_umesh', stat )

c.....Create the mesh object classes by deriving the classes from
c     superclasses, then instantiate the classes by naming the object
c     identifiers and supplying any class-specific attributes.

c.....Create simple mesh object classes; these objects have no
c     special attributes beyond an identifier and membership in a class.
c     The mesh itself is the simplest of all since a single mesh can
c     have (be) only one mesh.

c.....Define a class for the mesh itself
      call mf_def_class( fid, mesh_id, m_mesh, global, stat )
      call mf_print_error( 'mf_def_class (global)', stat )
    
      call mf_def_class_idents( fid, mesh_id, global, 1, 1, stat )
      call mf_print_error( 'mf_def_class_idents (global)', stat )

c.....Define the material class.
      call mf_def_class( fid, mesh_id, m_mat, material, stat )
      call mf_print_error( 'mf_def_class (material)', stat )
    
      call mf_def_class_idents( fid, mesh_id, material, 1, 3, stat )
      call mf_print_error( 'mf_def_class_idents (material)', stat )

c.....Create complex mesh object classes.  First create each class
c     from a superclass as above, then use special calls to supply
c     appropriate attributes (i.e., coordinates for nodes and
c     connectivities for elements).

c.....Define a node class.
      call mf_def_class( fid, mesh_id, m_node, nodal, stat )
      call mf_print_error( 'mf_def_class (nodal)', stat )
    
      call mf_def_nodes( fid, mesh_id, nodal, 1, 8, node_1_8, stat )
      call mf_print_error( 'mf_def_nodes', stat )

c.....Define element classes

c.....Define a quad class for the faces
      call mf_def_class( fid, mesh_id, m_quad, face, stat )
      call mf_print_error( 'mf_def_class (face)', stat )

      call mf_def_conn_seq( fid, mesh_id, face, 1, 3, 
     +                      face_1_3, stat )
      call mf_print_error( 'mf_def_conn_seq', stat )


c.....Define state record format(s)

c.....Create a state record format descriptor
      call mf_make_srec( fid, mesh_id, srec_id, stat )
      call mf_print_error( 'mf_make_srec', stat )

c.....Load Curve data, Face class
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 3
      call mf_def_subrec( fid, srec_id, face, m_object_ordered, 1, 
     +                    face_short, face, m_block_obj_fmt, 1, 
     +                    obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )


c.....SHOULD commit before writing state data so the family will be 
c     readable if a crash occurs during the analysis.
   
      call mf_flush( fid, m_non_state_data, stat )
      call mf_print_error( 'mf_commit', stat )


c     call mf_limit_states( fid, 500, stat )
c     call mf_print_error( 'mf_limit_states', stat )


c.....Write a state record to dump out the load curve values.
      time = 0.0
      call mf_new_state( fid, srec_id, time, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 3, state_record1, stat )
      call mf_print_error( 'mf_wrt_stream', stat )


c.....MUST close the family when finished

      call mf_close( fid, stat )
      call mf_print_error( 'mf_close', stat )
      
      end
