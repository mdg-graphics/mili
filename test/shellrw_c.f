c $Id: shellrw_c.f,v 1.6 2002/12/11 22:16:28 speck Exp $

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


      program shellrw_c
c.....Tests re-write on nodal coordinates.  Intended to be called after
c.....shellrw_b to change "original" nodes 6 and 8 back to their modified
c.....positions by over-writing an entire existing definition block.  

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

c.....Nodes 5:8 with modified 6 and 8
      real node_5_8(3,4)
      data node_5_8/
     +    0.0, 1.0, 0.0,  1.0, 1.5, 0.0,  3.0, 1.0, 0.0,  6.0, 1.5, 0.0/


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
    
    
c.....Re-write nodes 5-8
      call mf_def_nodes( fid, mesh_id, nodal_s, 5, 8, node_5_8, 
     +                   stat )
      call mf_print_error( 'mf_def_nodes re-write', stat )

c.....MUST close the family when finished

      call mf_close( fid, stat )
      call mf_print_error( 'mf_close', stat )
      
      end
