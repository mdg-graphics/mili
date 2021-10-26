c $Id: shellrw_b.f,v 1.1 2021/07/23 16:03:03 rhathaw Exp $

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


      program shellrw_b
c.....Tests re-write on nodal coordinates.  Intended to be called after
c.....shellrw to over-write modified nodes back to their original
c.....positions.  The db should effectively be identical to shelldb
c.....output at that point (although with a different distribution
c.....among the files).

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
     
      integer fid, mesh_id, qty_meshes
      integer stat

c.....Group names, to be used when defining connectivities and when
c.....defining subrecords
      character*(m_max_name_len) global_s, global_l, nodal_s, nodal_l, 
     +                           quads_s, quads_l, material_s, 
     +                           material_l
      data global_s, global_l, nodal_s, nodal_l, quads_s, quads_l, 
     +     material_s, material_l /
     +        'g', 'Global', 'node', 'Nodal', 's', 'Shells', 'm', 
     +        'Material' /


c.....MESH GEOMETRY

c.....Modified coordinates for nodes 2, 4, 6, and 8 for re-write.
      real node_2_orig(3,1)
      data node_2_orig/ 1.0, 0.0, 0.0/

      real node_6_orig(3,1)
      data node_6_orig/ 1.0, 1.0, 0.0/

      real node_4_orig(3,1)
      data node_4_orig/ 6.0, 0.0, 0.0/

      real node_8_orig(3,1)
      data node_8_orig/ 6.0, 1.0, 0.0/

c.....Nodes 3:6 (for re-writing with error condition)
      real node_3_6(3,4)
      data node_3_6/
     +    0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.0/


c.....And away we go...


c.....MUST open the family

      call mf_open( 'shrw', 'data', 'AaPs', fid, stat )
      if ( stat .ne. 0 ) then
          call mf_print_error( 'mf_open', stat )
          stop
      end if
    
c.....Query the quantity of meshes to illustrate determining available id's
      call mf_query_family( fid, m_qty_meshes, m_dont_care, 
     +                      m_blank, qty_meshes, stat )
      call mf_print_error( 'mf_query_family', stat )
    
      mesh_id = 0
    
    
c.....First node re-write, node 2 (re-write to currently open non-state file)
      call mf_def_nodes( fid, mesh_id, nodal_s, 2, 2, node_2_orig, 
     +                   stat )
      call mf_print_error( 'mf_def_nodes re-write', stat )

c.....First node re-write, node 4 (re-write to closed non-state file)
      call mf_def_nodes( fid, mesh_id, nodal_s, 4, 4, node_4_orig, 
     +                   stat )
      call mf_print_error( 'mf_def_nodes re-write', stat )

c.....First node re-write node 6 (re-write to 2nd open non-state file)
      call mf_def_nodes( fid, mesh_id, nodal_s, 6, 6, node_6_orig, 
     +                   stat )
      call mf_print_error( 'mf_def_nodes re-write', stat )
    
c.....First node re-write, node 8 (re-write to 2nd closed file)
      call mf_def_nodes( fid, mesh_id, nodal_s, 8, 8, node_8_orig, 
     +                   stat )
      call mf_print_error( 'mf_def_nodes re-write', stat )
    
c.....Re-write nodes 3-6 (block straddles extant blocks, should err) 
      call mf_def_nodes( fid, mesh_id, nodal_s, 3, 6, node_3_6, 
     +                   stat )
      call mf_print_error( 'mf_def_nodes re-write', stat )

c.....MUST close the family when finished

      call mf_close( fid, stat )
      call mf_print_error( 'mf_close', stat )
      
      end
