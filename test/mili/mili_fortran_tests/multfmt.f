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


      program multfmt
      
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
     
      integer fid, mesh_id, srec0_id, srec1_id, srec2_id, suffix, st_idx
      integer stat
      real time
c     integer obj_blocks(2,1)
      integer obj_blocks(2,5)

c.....Initialize a object_ordered state record with numbers that will
c     distinguish each subrecord and each "lump".  Size to fit format
c     specified below.
      real state_record1(64)
      data state_record1 /
c                                                           Node Positions
     +    0.0, 0.0, 0.0,  1.0, 0.0, 0.0,  3.0, 0.0, 0.0,  6.0, 0.0, 0.0, 
     +    0.0, 1.0, 0.0,  1.0, 1.0, 0.0,  3.0, 1.0, 0.0,  6.0, 1.0, 0.0, 
     +    0.0, 0.0, 1.0,  1.0, 0.0, 1.0,  3.0, 0.0, 1.0,  6.0, 0.0, 1.0, 
     +    0.0, 1.0, 1.0,  1.0, 1.0, 1.0,  3.0, 1.0, 1.0,  6.0, 1.0, 1.0,
c                                                          Node Temperatures
     +    200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 
     +    200, 200, 200, 200 /

      real state_record2(64)
      data state_record2 /
c                                                           Node Positions
     +  0.0, 0.0, 0.0,  .9667, 0.0, 0.0,  2.9, 0.0, 0.0,  5.8, 0.0, 0.0,
     +  0.0, 1.0, 0.0,  .9667, 1.0, 0.0,  2.9, 1.0, 0.0,  5.8, 1.0, 0.0,
     +  0.0, 0.0, 1.0,  .9667, 0.0, 1.0,  2.9, 0.0, 1.0,  5.8, 0.0, 1.0,
     +  0.0, 1.0, 1.0,  .9667, 1.0, 1.0,  2.9, 1.0, 1.0,  5.8, 1.0, 1.0,
c                                                          Node Temperatures
     +    250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 
     +    250, 250, 250, 250 /

      real state_record3(64)
      data state_record3 /
c                                                           Node Positions
     +  0.0, 0.0, 0.0,  .9333, 0.0, 0.0,  2.8, 0.0, 0.0,  5.6, 0.0, 0.0,
     +  0.0, 1.0, 0.0,  .9333, 1.0, 0.0,  2.8, 1.0, 0.0,  5.6, 1.0, 0.0,
     +  0.0, 0.0, 1.0,  .9333, 0.0, 1.0,  2.8, 0.0, 1.0,  5.6, 0.0, 1.0,
     +  0.0, 1.0, 1.0,  .9333, 1.0, 1.0,  2.8, 1.0, 1.0,  5.6, 1.0, 1.0,
c                                                          Node Temperatures
     +    300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 
     +    300, 300, 300, 300 /

      real state_record4(64)
      data state_record4 /
c                                                           Node Positions
     +    0.0, 0.0, 0.0,  0.9, 0.0, 0.0,  2.7, 0.0, 0.0,  5.4, 0.0, 0.0, 
     +    0.0, 1.0, 0.0,  0.9, 1.0, 0.0,  2.7, 1.0, 0.0,  5.4, 1.0, 0.0, 
     +    0.0, 0.0, 1.0,  0.9, 0.0, 1.0,  2.7, 0.0, 1.0,  5.4, 0.0, 1.0, 
     +    0.0, 1.0, 1.0,  0.9, 1.0, 1.0,  2.7, 1.0, 1.0,  5.4, 1.0, 1.0,
c                                                          Node Temperatures
     +    350, 350, 350, 350, 350, 350, 350, 350, 350, 350, 350, 350, 
     +    350, 350, 350, 350 /

      real state_record5(64)
      data state_record5 /
c                                                           Node Positions
     +  0.0, 0.0, 0.0,  .8667, 0.0, 0.0,  2.6, 0.0, 0.0,  5.2, 0.0, 0.0,
     +  0.0, 1.0, 0.0,  .8667, 1.0, 0.0,  2.6, 1.0, 0.0,  5.2, 1.0, 0.0,
     +  0.0, 0.0, 1.0,  .8667, 0.0, 1.0,  2.6, 0.0, 1.0,  5.2, 0.0, 1.0,
     +  0.0, 1.0, 1.0,  .8667, 1.0, 1.0,  2.6, 1.0, 1.0,  5.2, 1.0, 1.0,
c                                                          Node Temperatures
     +    400, 400, 400, 400, 400, 400, 400, 400, 400, 400, 400, 400, 
     +    400, 400, 400, 400 /


      real state_record6(98)
      data state_record6 /
c                                                           Node Positions
     +  0.0, 0.0, 0.0,  .8333, 0.0, 0.0,  2.5, 0.0, 0.0,  5.0, 0.0, 0.0,
     +  0.0, 1.0, 0.0,  .8333, 1.0, 0.0,  2.5, 1.0, 0.0,  5.0, 1.0, 0.0,
     +  0.0, 0.0, 1.0,  .8333, 0.0, 1.0,  2.5, 0.0, 1.0,  5.0, 0.0, 1.0,
     +  0.0, 1.0, 1.0,  .8333, 1.0, 1.0,  2.5, 1.0, 1.0,  5.0, 1.0, 1.0,
c                                                          Node Temperatures
     +    450, 450, 450, 450, 450, 450, 450, 450, 450, 450, 450, 450, 
     +    450, 450, 450, 450,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          HIVar1-2 (2-3)
     +    20.0, 22.0,  20.0, 22.0,
c                                                          HIVar3-5 (1)
     +    30.0, 32.0, 34.0, 
c                                                          SPVar1-3 (all)
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4 /

      real state_record7(98)
      data state_record7 /
c                                                           Node Positions
     +  0.0, 0.0, 0.0,  .8, 0.0, 0.0,  2.4, 0.0, 0.0,  4.8, 0.0, 0.0,
     +  0.0, 1.0, 0.0,  .8, 1.0, 0.0,  2.4, 1.0, 0.0,  4.8, 1.0, 0.0,
     +  0.0, 0.0, 1.0,  .8, 0.0, 1.0,  2.4, 0.0, 1.0,  4.8, 0.0, 1.0,
     +  0.0, 1.0, 1.0,  .8, 1.0, 1.0,  2.4, 1.0, 1.0,  4.8, 1.0, 1.0,
c                                                          Node Temperatures
     +    500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 
     +    500, 500, 500, 500,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          HIVar1-2 (2-3)
     +    20.0, 22.0,  20.0, 22.0,
c                                                          HIVar3-5 (1)
     +    30.0, 32.0, 34.0, 
c                                                          SPVar1-3 (all)
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4 /

      real state_record8(98)
      data state_record8 /
c                                                           Node Positions
     +  0.0, 0.0, 0.0,  .7667, 0.0, 0.0,  2.3, 0.0, 0.0,  4.6, 0.0, 0.0,
     +  0.0, 1.0, 0.0,  .7667, 1.0, 0.0,  2.3, 1.0, 0.0,  4.6, 1.0, 0.0,
     +  0.0, 0.0, 1.0,  .7667, 0.0, 1.0,  2.3, 0.0, 1.0,  4.6, 0.0, 1.0,
     +  0.0, 1.0, 1.0,  .7667, 1.0, 1.0,  2.3, 1.0, 1.0,  4.6, 1.0, 1.0,
c                                                          Node Temperatures
     +    550, 550, 550, 550, 550, 550, 550, 550, 550, 550, 550, 550, 
     +    550, 550, 550, 550,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          HIVar1-2 (2-3)
     +    20.0, 22.0,  20.0, 22.0,
c                                                          HIVar3-5 (1)
     +    30.0, 32.0, 34.0, 
c                                                          SPVar1-3 (all)
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4 /

      real state_record9(98)
      data state_record9 /
c                                                           Node Positions
     +  0.0, 0.0, 0.0,  .7333, 0.0, 0.0,  2.2, 0.0, 0.0,  4.4, 0.0, 0.0,
     +  0.0, 1.0, 0.0,  .7333, 1.0, 0.0,  2.2, 1.0, 0.0,  4.4, 1.0, 0.0,
     +  0.0, 0.0, 1.0,  .7333, 0.0, 1.0,  2.2, 0.0, 1.0,  4.4, 0.0, 1.0,
     +  0.0, 1.0, 1.0,  .7333, 1.0, 1.0,  2.2, 1.0, 1.0,  4.4, 1.0, 1.0,
c                                                          Node Temperatures
     +    600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 
     +    600, 600, 600, 600,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          HIVar1-2 (2-3)
     +    20.0, 22.0,  20.0, 22.0,
c                                                          HIVar3-5 (1)
     +    30.0, 32.0, 34.0, 
c                                                          SPVar1-3 (all)
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4 /

      real state_record10(98)
      data state_record10 /
c                                                           Node Positions
     +  0.0, 0.0, 0.0,  .7, 0.0, 0.0,  2.1, 0.0, 0.0,  4.2, 0.0, 0.0,
     +  0.0, 1.0, 0.0,  .7, 1.0, 0.0,  2.1, 1.0, 0.0,  4.2, 1.0, 0.0,
     +  0.0, 0.0, 1.0,  .7, 0.0, 1.0,  2.1, 0.0, 1.0,  4.2, 0.0, 1.0,
     +  0.0, 1.0, 1.0,  .7, 1.0, 1.0,  2.1, 1.0, 1.0,  4.2, 1.0, 1.0,
c                                                          Node Temperatures
     +    650, 650, 650, 650, 650, 650, 650, 650, 650, 650, 650, 650, 
     +    650, 650, 650, 650,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          HIVar1-2 (2-3)
     +    20.0, 22.0,  20.0, 22.0,
c                                                          HIVar3-5 (1)
     +    30.0, 32.0, 34.0, 
c                                                          SPVar1-3 (all)
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4 /


      real state_record11(87)
      data state_record11 /
c                                                           Node Positions
     +  0.0, 0.0, 0.0,  .6667, 0.0, 0.0,  2.0, 0.0, 0.0,  4.0, 0.0, 0.0,
     +  0.0, 1.0, 0.0,  .6667, 1.0, 0.0,  2.0, 1.0, 0.0,  4.0, 1.0, 0.0,
     +  0.0, 0.0, 1.0,  .6667, 0.0, 1.0,  2.0, 0.0, 1.0,  4.0, 0.0, 1.0,
     +  0.0, 1.0, 1.0,  .6667, 1.0, 1.0,  2.0, 1.0, 1.0,  4.0, 1.0, 1.0,
c                                                          Node Temperatures
     +    700, 700, 700, 700, 700, 700, 700, 700, 700, 700, 700, 700, 
     +    700, 700, 700, 700,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          SPVar1-2 (all)
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,
c                                                          SIVar1-3 (1)
     +    60.0, 62.0, 64.0 /

      real state_record12(87)
      data state_record12 /
c                                                           Node Positions
     +  0.0, 0.0, 0.0,  .6333, 0.0, 0.0,  1.9, 0.0, 0.0,  3.8, 0.0, 0.0,
     +  0.0, 1.0, 0.0,  .6333, 1.0, 0.0,  1.9, 1.0, 0.0,  3.8, 1.0, 0.0,
     +  0.0, 0.0, 1.0,  .6333, 0.0, 1.0,  1.9, 0.0, 1.0,  3.8, 0.0, 1.0,
     +  0.0, 1.0, 1.0,  .6333, 1.0, 1.0,  1.9, 1.0, 1.0,  3.8, 1.0, 1.0,
c                                                          Node Temperatures
     +    750, 750, 750, 750, 750, 750, 750, 750, 750, 750, 750, 750, 
     +    750, 750, 750, 750,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          SPVar1-2 (all)
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,
c                                                          SIVar1-3 (1)
     +    60.0, 62.0, 64.0 /

      real state_record13(87)
      data state_record13 /
c                                                           Node Positions
     +  0.0, 0.0, 0.0,  .6, 0.0, 0.0,  1.8, 0.0, 0.0,  3.6, 0.0, 0.0,
     +  0.0, 1.0, 0.0,  .6, 1.0, 0.0,  1.8, 1.0, 0.0,  3.6, 1.0, 0.0,
     +  0.0, 0.0, 1.0,  .6, 0.0, 1.0,  1.8, 0.0, 1.0,  3.6, 0.0, 1.0,
     +  0.0, 1.0, 1.0,  .6, 1.0, 1.0,  1.8, 1.0, 1.0,  3.6, 1.0, 1.0,
c                                                          Node Temperatures
     +    800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 
     +    800, 800, 800, 800,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          SPVar1-2 (all)
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,
c                                                          SIVar1-3 (1)
     +    60.0, 62.0, 64.0 /

      real state_record14(87)
      data state_record14 /
c                                                           Node Positions
     +  0.0, 0.0, 0.0,  .5667, 0.0, 0.0,  1.7, 0.0, 0.0,  3.4, 0.0, 0.0,
     +  0.0, 1.0, 0.0,  .5667, 1.0, 0.0,  1.7, 1.0, 0.0,  3.4, 1.0, 0.0,
     +  0.0, 0.0, 1.0,  .5667, 0.0, 1.0,  1.7, 0.0, 1.0,  3.4, 0.0, 1.0,
     +  0.0, 1.0, 1.0,  .5667, 1.0, 1.0,  1.7, 1.0, 1.0,  3.4, 1.0, 1.0,
c                                                          Node Temperatures
     +    850, 850, 850, 850, 850, 850, 850, 850, 850, 850, 850, 850, 
     +    850, 850, 850, 850,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          SPVar1-2 (all)
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,
c                                                          SIVar1-3 (1)
     +    60.0, 62.0, 64.0 /

      real state_record15(87)
      data state_record15 /
c                                                           Node Positions
     +  0.0, 0.0, 0.0,  .5333, 0.0, 0.0,  1.6, 0.0, 0.0,  3.2, 0.0, 0.0,
     +  0.0, 1.0, 0.0,  .5333, 1.0, 0.0,  1.6, 1.0, 0.0,  3.2, 1.0, 0.0,
     +  0.0, 0.0, 1.0,  .5333, 0.0, 1.0,  1.6, 0.0, 1.0,  3.2, 0.0, 1.0,
     +  0.0, 1.0, 1.0,  .5333, 1.0, 1.0,  1.6, 1.0, 1.0,  3.2, 1.0, 1.0,
c                                                          Node Temperatures
     +    900, 900, 900, 900, 900, 900, 900, 900, 900, 900, 900, 900, 
     +    900, 900, 900, 900,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          SPVar1-2 (all)
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,
c                                                          SIVar1-3 (1)
     +    60.0, 62.0, 64.0 /

      real state_record16(87)
      data state_record16 /
c                                                           Node Positions
     +  0.0, 0.0, 0.0,  .5, 0.0, 0.0,  1.5, 0.0, 0.0,  3.0, 0.0, 0.0,
     +  0.0, 1.0, 0.0,  .5, 1.0, 0.0,  1.5, 1.0, 0.0,  3.0, 1.0, 0.0,
     +  0.0, 0.0, 1.0,  .5, 0.0, 1.0,  1.5, 0.0, 1.0,  3.0, 0.0, 1.0,
     +  0.0, 1.0, 1.0,  .5, 1.0, 1.0,  1.5, 1.0, 1.0,  3.0, 1.0, 1.0,
c                                                          Node Temperatures
     +    950, 950, 950, 950, 950, 950, 950, 950, 950, 950, 950, 950, 
     +    950, 950, 950, 950,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          SPVar1-2 (all)
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,
c                                                          SIVar1-3 (1)
     +    60.0, 62.0, 64.0 /

      real state_record17(87)
      data state_record17 /
c                                                           Node Positions
     + 0.0, 0.0, 0.0,  .5267, 0.0, 0.0,  1.58, 0.0, 0.0, 3.16, 0.0, 0.0,
     + 0.0, 1.0, 0.0,  .5267, 1.0, 0.0,  1.58, 1.0, 0.0, 3.16, 1.0, 0.0,
     + 0.0, 0.0, 1.0,  .5267, 0.0, 1.0,  1.58, 0.0, 1.0, 3.16, 0.0, 1.0,
     + 0.0, 1.0, 1.0,  .5267, 1.0, 1.0,  1.58, 1.0, 1.0, 3.16, 1.0, 1.0,
c                                                          Node Temperatures
     +    910, 910, 910, 910, 910, 910, 910, 910, 910, 910, 910, 910, 
     +    910, 910, 910, 910,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          SPVar1-2 (all)
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,
c                                                          SIVar1-3 (1)
     +    60.0, 62.0, 64.0 /

      real state_record18(87)
      data state_record18 /
c                                                           Node Positions
     + 0.0, 0.0, 0.0,  .5533, 0.0, 0.0,  1.66, 0.0, 0.0, 3.32, 0.0, 0.0,
     + 0.0, 1.0, 0.0,  .5533, 1.0, 0.0,  1.66, 1.0, 0.0, 3.32, 1.0, 0.0,
     + 0.0, 0.0, 1.0,  .5533, 0.0, 1.0,  1.66, 0.0, 1.0, 3.32, 0.0, 1.0,
     + 0.0, 1.0, 1.0,  .5533, 1.0, 1.0,  1.66, 1.0, 1.0, 3.32, 1.0, 1.0,
c                                                          Node Temperatures
     +    870, 870, 870, 870, 870, 870, 870, 870, 870, 870, 870, 870, 
     +    870, 870, 870, 870,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          SPVar1-2 (all)
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,
c                                                          SIVar1-3 (1)
     +    60.0, 62.0, 64.0 /

      real state_record19(87)
      data state_record19 /
c                                                           Node Positions
     + 0.0, 0.0, 0.0,  .58, 0.0, 0.0,  1.74, 0.0, 0.0, 3.48, 0.0, 0.0,
     + 0.0, 1.0, 0.0,  .58, 1.0, 0.0,  1.74, 1.0, 0.0, 3.48, 1.0, 0.0,
     + 0.0, 0.0, 1.0,  .58, 0.0, 1.0,  1.74, 0.0, 1.0, 3.48, 0.0, 1.0,
     + 0.0, 1.0, 1.0,  .58, 1.0, 1.0,  1.74, 1.0, 1.0, 3.48, 1.0, 1.0,
c                                                          Node Temperatures
     +    830, 830, 830, 830, 830, 830, 830, 830, 830, 830, 830, 830, 
     +    830, 830, 830, 830,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          SPVar1-2 (all)
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,
c                                                          SIVar1-3 (1)
     +    60.0, 62.0, 64.0 /

      real state_record20(87)
      data state_record20 /
c                                                           Node Positions
     + 0.0, 0.0, 0.0,  .6067, 0.0, 0.0,  1.82, 0.0, 0.0, 3.64, 0.0, 0.0,
     + 0.0, 1.0, 0.0,  .6067, 1.0, 0.0,  1.82, 1.0, 0.0, 3.64, 1.0, 0.0,
     + 0.0, 0.0, 1.0,  .6067, 0.0, 1.0,  1.82, 0.0, 1.0, 3.64, 0.0, 1.0,
     + 0.0, 1.0, 1.0,  .6067, 1.0, 1.0,  1.82, 1.0, 1.0, 3.64, 1.0, 1.0,
c                                                          Node Temperatures
     +    790, 790, 790, 790, 790, 790, 790, 790, 790, 790, 790, 790, 
     +    790, 790, 790, 790,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          SPVar1-2 (all)
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,  5.0, 5.2,  5.0, 5.2,  
     +    5.0, 5.2,
c                                                          SIVar1-3 (1)
     +    60.0, 62.0, 64.0 /


      real state_record21(98)
      data state_record21 /
c                                                           Node Positions
     + 0.0, 0.0, 0.0,  .6333, 0.0, 0.0,  1.9, 0.0, 0.0, 3.8, 0.0, 0.0,
     + 0.0, 1.0, 0.0,  .6333, 1.0, 0.0,  1.9, 1.0, 0.0, 3.8, 1.0, 0.0,
     + 0.0, 0.0, 1.0,  .6333, 0.0, 1.0,  1.9, 0.0, 1.0, 3.8, 0.0, 1.0,
     + 0.0, 1.0, 1.0,  .6333, 1.0, 1.0,  1.9, 1.0, 1.0, 3.8, 1.0, 1.0,
c                                                          Node Temperatures
     +    750, 750, 750, 750, 750, 750, 750, 750, 750, 750, 750, 750, 
     +    750, 750, 750, 750,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          HIVar1-2 (2-3)
     +    20.0, 22.0,  20.0, 22.0,
c                                                          HIVar3-5 (1)
     +    30.0, 32.0, 34.0, 
c                                                          SPVar1-3 (all)
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4 /

      real state_record22(98)
      data state_record22 /
c                                                           Node Positions
     + 0.0, 0.0, 0.0,  .66, 0.0, 0.0,  1.98, 0.0, 0.0, 3.96, 0.0, 0.0,
     + 0.0, 1.0, 0.0,  .66, 1.0, 0.0,  1.98, 1.0, 0.0, 3.96, 1.0, 0.0,
     + 0.0, 0.0, 1.0,  .66, 0.0, 1.0,  1.98, 0.0, 1.0, 3.96, 0.0, 1.0,
     + 0.0, 1.0, 1.0,  .66, 1.0, 1.0,  1.98, 1.0, 1.0, 3.96, 1.0, 1.0,
c                                                          Node Temperatures
     +    710, 710, 710, 710, 710, 710, 710, 710, 710, 710, 710, 710, 
     +    710, 710, 710, 710,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          HIVar1-2 (2-3)
     +    20.0, 22.0,  20.0, 22.0,
c                                                          HIVar3-5 (1)
     +    30.0, 32.0, 34.0, 
c                                                          SPVar1-3 (all)
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4 /

      real state_record23(98)
      data state_record23 /
c                                                           Node Positions
     + 0.0, 0.0, 0.0,  .6867, 0.0, 0.0,  2.06, 0.0, 0.0, 4.12, 0.0, 0.0,
     + 0.0, 1.0, 0.0,  .6867, 1.0, 0.0,  2.06, 1.0, 0.0, 4.12, 1.0, 0.0,
     + 0.0, 0.0, 1.0,  .6867, 0.0, 1.0,  2.06, 0.0, 1.0, 4.12, 0.0, 1.0,
     + 0.0, 1.0, 1.0,  .6867, 1.0, 1.0,  2.06, 1.0, 1.0, 4.12, 1.0, 1.0,
c                                                          Node Temperatures
     +    670, 670, 670, 670, 670, 670, 670, 670, 670, 670, 670, 670, 
     +    670, 670, 670, 670,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          HIVar1-2 (2-3)
     +    20.0, 22.0,  20.0, 22.0,
c                                                          HIVar3-5 (1)
     +    30.0, 32.0, 34.0, 
c                                                          SPVar1-3 (all)
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4 /

      real state_record24(98)
      data state_record24 /
c                                                           Node Positions
     + 0.0, 0.0, 0.0,  .7133, 0.0, 0.0,  2.14, 0.0, 0.0, 4.28, 0.0, 0.0,
     + 0.0, 1.0, 0.0,  .7133, 1.0, 0.0,  2.14, 1.0, 0.0, 4.28, 1.0, 0.0,
     + 0.0, 0.0, 1.0,  .7133, 0.0, 1.0,  2.14, 0.0, 1.0, 4.28, 0.0, 1.0,
     + 0.0, 1.0, 1.0,  .7133, 1.0, 1.0,  2.14, 1.0, 1.0, 4.28, 1.0, 1.0,
c                                                          Node Temperatures
     +    630, 630, 630, 630, 630, 630, 630, 630, 630, 630, 630, 630, 
     +    630, 630, 630, 630,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          HIVar1-2 (2-3)
     +    20.0, 22.0,  20.0, 22.0,
c                                                          HIVar3-5 (1)
     +    30.0, 32.0, 34.0, 
c                                                          SPVar1-3 (all)
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4 /

      real state_record25(98)
      data state_record25 /
c                                                           Node Positions
     + 0.0, 0.0, 0.0,  .74, 0.0, 0.0,  2.22, 0.0, 0.0, 4.44, 0.0, 0.0,
     + 0.0, 1.0, 0.0,  .74, 1.0, 0.0,  2.22, 1.0, 0.0, 4.44, 1.0, 0.0,
     + 0.0, 0.0, 1.0,  .74, 0.0, 1.0,  2.22, 0.0, 1.0, 4.44, 0.0, 1.0,
     + 0.0, 1.0, 1.0,  .74, 1.0, 1.0,  2.22, 1.0, 1.0, 4.44, 1.0, 1.0,
c                                                          Node Temperatures
     +    610, 610, 610, 610, 610, 610, 610, 610, 610, 610, 610, 610, 
     +    610, 610, 610, 610,
c                                                          HPVar1-2 (all)
     +    1.0, 1.2,  1.0, 1.2,  1.0, 1.2, 
c                                                          HIVar1-2 (2-3)
     +    20.0, 22.0,  20.0, 22.0,
c                                                          HIVar3-5 (1)
     +    30.0, 32.0, 34.0, 
c                                                          SPVar1-3 (all)
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  5.0, 5.2, 5.4,  
     +    5.0, 5.2, 5.4 /


      real state_record26(64)
      data state_record26 /
c                                                           Node Positions
     + 0.0, 0.0, 0.0,  .7667, 0.0, 0.0,  2.3, 0.0, 0.0, 4.6, 0.0, 0.0,
     + 0.0, 1.0, 0.0,  .7667, 1.0, 0.0,  2.3, 1.0, 0.0, 4.6, 1.0, 0.0,
     + 0.0, 0.0, 1.0,  .7667, 0.0, 1.0,  2.3, 0.0, 1.0, 4.6, 0.0, 1.0,
     + 0.0, 1.0, 1.0,  .7667, 1.0, 1.0,  2.3, 1.0, 1.0, 4.6, 1.0, 1.0,
c                                                          Node Temperatures
     +    570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 
     +    570, 570, 570, 570 /

      real state_record27(64)
      data state_record27 /
c                                                           Node Positions
     + .16, 0.0, 0.0,  .9267, 0.0, 0.0,  2.46, 0.0, 0.0, 4.76, 0.0, 0.0,
     + .16, 1.0, 0.0,  .9267, 1.0, 0.0,  2.46, 1.0, 0.0, 4.76, 1.0, 0.0,
     + .16, 0.0, 1.0,  .9267, 0.0, 1.0,  2.46, 0.0, 1.0, 4.76, 0.0, 1.0,
     + .16, 1.0, 1.0,  .9267, 1.0, 1.0,  2.46, 1.0, 1.0, 4.76, 1.0, 1.0,
c                                                          Node Temperatures
     +    570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 
     +    570, 570, 570, 570 /

      real state_record28(64)
      data state_record28 /
c                                                           Node Positions
     + .32, 0.0, 0.0, 1.0867, 0.0, 0.0,  2.62, 0.0, 0.0, 4.92, 0.0, 0.0,
     + .32, 1.0, 0.0, 1.0867, 1.0, 0.0,  2.62, 1.0, 0.0, 4.92, 1.0, 0.0,
     + .32, 0.0, 1.0, 1.0867, 0.0, 1.0,  2.62, 0.0, 1.0, 4.92, 0.0, 1.0,
     + .32, 1.0, 1.0, 1.0867, 1.0, 1.0,  2.62, 1.0, 1.0, 4.92, 1.0, 1.0,
c                                                          Node Temperatures
     +    570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 
     +    570, 570, 570, 570 /

      real state_record29(64)
      data state_record29 /
c                                                           Node Positions
     + .48, 0.0, 0.0, 1.2467, 0.0, 0.0,  2.78, 0.0, 0.0, 5.08, 0.0, 0.0,
     + .48, 1.0, 0.0, 1.2467, 1.0, 0.0,  2.78, 1.0, 0.0, 5.08, 1.0, 0.0,
     + .48, 0.0, 1.0, 1.2467, 0.0, 1.0,  2.78, 0.0, 1.0, 5.08, 0.0, 1.0,
     + .48, 1.0, 1.0, 1.2467, 1.0, 1.0,  2.78, 1.0, 1.0, 5.08, 1.0, 1.0,
c                                                          Node Temperatures
     +    570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 
     +    570, 570, 570, 570 /

      real state_record30(64)
      data state_record30 /
c                                                           Node Positions
     + .64, 0.0, 0.0, 1.4067, 0.0, 0.0,  2.94, 0.0, 0.0, 5.24, 0.0, 0.0,
     + .64, 1.0, 0.0, 1.4067, 1.0, 0.0,  2.94, 1.0, 0.0, 5.24, 1.0, 0.0,
     + .64, 0.0, 1.0, 1.4067, 0.0, 1.0,  2.94, 0.0, 1.0, 5.24, 0.0, 1.0,
     + .64, 1.0, 1.0, 1.4067, 1.0, 1.0,  2.94, 1.0, 1.0, 5.24, 1.0, 1.0,
c                                                          Node Temperatures
     +    570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 570, 
     +    570, 570, 570, 570 /


c.....State variables per mesh object type

c.....All short names MUST be unique
      
c.....Nodal state variables
      character*(m_max_name_len) nodal_short(5), nodal_long(5)
      data 
     +    nodal_short /
     +        'nodpos', 'temp', 'ux', 'uy', 'uz' /,
     +    nodal_long /
     +        'Node Position', 'Temperature', 
     +        'X Position', 'Y Position', 'Z Position' /

c.....Hex state variables
      character*(m_max_name_len) hex_short(7), hex_long(7)
      data
     +    hex_short / 
     +        'hpvar1', 'hpvar2', 'hivar1', 'hivar2', 'hivar3', 
     +        'hivar4', 'hivar5' /,
     +    hex_long /
     +        'HPVar1', 'HPVar2', 'HIVar1', 'HIVar2', 'HIVar3', 
     +        'HIVar4', 'HIVar5' /

c.....Shell state variables
      character*(m_max_name_len) shell_short(6), shell_long(6)
      data
     +    shell_short / 
     +        'spvar1', 'spvar2', 'spvar3', 'sivar1', 'sivar2', 
     +        'sivar3' /,
     +    shell_long /
     +        'SPVar1', 'SPVar2', 'SPVar3', 'SIVar1', 'SIVar2', 
     +        'SIVar3' /

c.....State var types; they're all floats, so just define one array as big
c.....as the largest block of svars that will be defined in one call
      integer svar_types(7)
      data svar_types / 7*m_real /

c.....Class names, short and long
      character*(m_max_name_len) global_s, global_l, nodal_s, nodal_l, 
     +                           hexs_s, hexs_l, shell_s, shell_l,
     +                           material_s, material_l
      data global_s, global_l, nodal_s, nodal_l, hexs_s, hexs_l, 
     +     shell_s, shell_l, material_s, material_l /
     +        'g', 'Global', 'node', 'Node', 'b', 'Brick', 's', 'Shell',
     +        'm', 'Material' /


c.....MESH GEOMETRY


c.....Nodes 1:16
      real node_1_16(3,16)
      data node_1_16/
     +    0.0, 0.0, 0.0,  1.0, 0.0, 0.0,  3.0, 0.0, 0.0,  6.0, 0.0, 0.0, 
     +    0.0, 1.0, 0.0,  1.0, 1.0, 0.0,  3.0, 1.0, 0.0,  6.0, 1.0, 0.0, 
     +    0.0, 0.0, 1.0,  1.0, 0.0, 1.0,  3.0, 0.0, 1.0,  6.0, 0.0, 1.0, 
     +    0.0, 1.0, 1.0,  1.0, 1.0, 1.0,  3.0, 1.0, 1.0,  6.0, 1.0, 1.0/
c.....Node labels 1:16
      integer n_labels(16)
      data n_labels / 
     +    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 /

c.....Hex 1:3 connectivities; 8 nodes, 1 material, 1 part
      integer hex_1_3(10,3)
      data hex_1_3 /
     +    1, 2, 6, 5, 9, 10, 14, 13, 1, 1,
     +    2, 3, 7, 6, 10, 11, 15, 14, 2, 1,
     +    3, 4, 8, 7, 11, 12, 16, 15, 3, 1/
c.....Hex Labels 1:3
      integer hex_labels(3)
      data hex_labels / 1, 2, 3 /

c.....Shell 1:7 connectivities; 4 nodes, 1 material, 1 part
      integer shell_1_7(6,7)
      data shell_1_7 /
     +    4, 3, 7, 8, 4, 1, 
     +    3, 2, 6, 7, 4, 1, 
     +    2, 1, 5, 6, 4, 1,  
     +    1, 9, 13, 5, 5, 1, 
     +    9, 10, 14, 13, 6, 1, 
     +    10, 11, 15, 14, 6, 1, 
     +    11, 12, 16, 15, 6, 1 /
c.....Shell Labels 1:7
      integer shell_labels(7)
      data shell_labels / 1, 2, 3, 4, 5, 6, 7 /

c.....Total quantity of materials
      integer qty_materials
      data qty_materials / 6 /


c.....And away we go...


c.....MUST open the family

      call mf_open( 'multfmt.plt', '.', 'AwPs', fid, stat )
      if ( stat .ne. 0 ) then
          call mf_print_error( 'mf_open', stat )
          stop
      end if

c.....Write a title
      call mf_wrt_string( fid, 'title', "Multiple srec format test", 
     +                    stat )
      call mf_print_error( 'mf_wrt_string', stat )
      
      
c.....MUST define state variables prior to binding subrecord definitions
      
      call mf_def_vec_svar( fid, m_real, 3, nodal_short(1), 
     +                      nodal_long(1), nodal_short(3), 
     +                      nodal_long(3), stat )
      call mf_print_error( 'mf_def_vec_svar', stat )
    
      call mf_def_svars( fid, 1, svar_types, nodal_short(2), 
     +                   nodal_long(2), stat )
      call mf_print_error( 'mf_def_svars', stat )
    
      call mf_def_svars( fid, 7, svar_types, hex_short(1), 
     +                   hex_long(1), stat )
      call mf_print_error( 'mf_def_svars', stat )
    
      call mf_def_svars( fid, 6, svar_types, shell_short(1), 
     +                   shell_long(1), stat )
      call mf_print_error( 'mf_def_svars', stat )

      
c.....MUST create & define the mesh prior to binding subrecord definitions

c.....Initialize the mesh
      call mf_make_umesh( fid, 'HexShell mesh', 3, mesh_id, stat )
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
    
      call mf_def_class_idents( fid, mesh_id, material_s, 1, 
     +                          qty_materials, stat )
      call mf_print_error( 'mf_def_class_idents (material)', stat )

c.....Create complex mesh object classes.  First create each class
c     from a superclass as above, then use special calls to supply
c     appropriate attributes (i.e., coordinates for nodes and
c     connectivities for elements).

c.....Define a node class.
      call mf_def_class( fid, mesh_id, m_node, nodal_s, nodal_l, stat )
      call mf_print_error( 'mf_def_class (node)', stat )
    
      call mf_def_nodes( fid, mesh_id, nodal_s, 1, 16, node_1_16, stat )
      call mf_print_error( 'mf_def_nodes', stat )

      call mf_def_node_labels(fid, mesh_id, nodal_s, 16, n_labels, stat)
      call mf_print_error( 'mf_def_node_labels', stat )

c.....Define element classes

c.....Define a hex class
      call mf_def_class( fid, mesh_id, m_hex, hexs_s, hexs_l, stat )
      call mf_print_error( 'mf_def_class (hex)', stat )

      call mf_def_conn_seq_labels( fid, mesh_id, hexs_s, 1, 3, 
     +                             hex_labels, hex_1_3, stat )
      call mf_print_error( 'mf_def_conn_seq', stat )

c.....Define a shell class
      call mf_def_class( fid, mesh_id, m_quad, shell_s, shell_l, stat )
      call mf_print_error( 'mf_def_class (shell)', stat )

      call mf_def_conn_seq_labels( fid, mesh_id, shell_s, 1, 7, 
     +                             shell_labels, shell_1_7, stat )
      call mf_print_error( 'mf_def_conn_seq', stat )


c.....Create and define the state record formats

c.....Open the first state record format descriptor
      call mf_open_srec( fid, mesh_id, srec0_id, stat )
      call mf_print_error( 'mf_open_srec', stat )

c.....Nodal data
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 16
      call mf_def_subrec( fid, srec0_id, 'S1-Node', m_result_ordered, 2,
     +                    nodal_short, nodal_s, m_block_obj_fmt, 1, 
     +                    obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Close the first state record format descriptor    
      call mf_close_srec( fid, srec0_id, stat )
      call mf_print_error( 'mf_close_srec', stat )

c.....Open the second state record format descriptor
      call mf_open_srec( fid, mesh_id, srec1_id, stat )
      call mf_print_error( 'mf_open_srec', stat )

c.....Nodal data
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 16
      call mf_def_subrec( fid, srec1_id, 'S2-Node', m_result_ordered, 2,
     +                    nodal_short, nodal_s, m_block_obj_fmt, 1, 
     +                    obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Hex data, brick class, proper subrecord
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 3
      call mf_def_subrec( fid, srec1_id, 'S2-Hex1', m_object_ordered, 2,
     +                    hex_short(1), hexs_s, m_block_obj_fmt, 1, 
     +                    obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Hex data, brick class, non-proper subrecord
      obj_blocks(1,1) = 2
      obj_blocks(2,1) = 3
      call mf_def_subrec( fid, srec1_id, 'S2-Hex2', m_object_ordered, 2,
     +                    hex_short(3), hexs_s, m_block_obj_fmt, 1, 
     +                    obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Hex data, brick class, non-proper subrecord
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 1
      call mf_def_subrec( fid, srec1_id, 'S2-Hex3', m_object_ordered, 3,
     +                    hex_short(5), hexs_s, m_block_obj_fmt, 1, 
     +                    obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Quad data, shell class, proper subrecord
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 7
      call mf_def_subrec( fid, srec1_id, 'S2-Shell1', m_object_ordered,
     +                    3, shell_short(1), shell_s, m_block_obj_fmt, 
     +                    1, obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )
    
c.....Close the second state record format descriptor    
      call mf_close_srec( fid, srec1_id, stat )
      call mf_print_error( 'mf_close_srec', stat )

c.....Open the third state record format descriptor
      call mf_open_srec( fid, mesh_id, srec2_id, stat )
      call mf_print_error( 'mf_open_srec', stat )

c.....Nodal data
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 16
      call mf_def_subrec( fid, srec2_id, 'S3-Node', m_result_ordered, 2,
     +                    nodal_short, nodal_s, m_block_obj_fmt, 1, 
     +                    obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Hex data, brick class, proper subrecord
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 3
      call mf_def_subrec( fid, srec2_id, 'S3-Hex1', m_object_ordered, 2,
     +                    hex_short(1), hexs_s, m_block_obj_fmt, 1, 
     +                    obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Quad data, shell class, proper subrecord
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 7
      call mf_def_subrec( fid, srec2_id, 'S3-Shell1', m_object_ordered,
     +                    2, shell_short(1), shell_s, m_block_obj_fmt, 
     +                    1, obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )

c.....Quad data, shell class, non-proper subrecord
      obj_blocks(1,1) = 1
      obj_blocks(2,1) = 1
      call mf_def_subrec( fid, srec2_id, 'S3-Shell2', m_object_ordered,
     +                    3, shell_short(4), shell_s, m_block_obj_fmt, 
     +                    1, obj_blocks, stat )
      call mf_print_error( 'mf_def_subrec', stat )
    
c.....Close the third state record format descriptor    
      call mf_close_srec( fid, srec2_id, stat )
      call mf_print_error( 'mf_close_srec', stat )


c.....SHOULD commit before writing state data so the family will be 
c     readable if a crash occurs during the analysis.
   
      call mf_flush( fid, m_non_state_data, stat )
      call mf_print_error( 'mf_commit', stat )


c     call mf_limit_states( fid, 500, stat )
c     call mf_print_error( 'mf_limit_states', stat )


c.....Write thirty records, each with one call.
      time = 0.0
      call mf_new_state( fid, srec0_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 64, state_record1, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 1.0
      call mf_new_state( fid, srec0_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 64, state_record2, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 2.0
      call mf_new_state( fid, srec0_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 64, state_record3, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 3.0
      call mf_new_state( fid, srec0_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 64, state_record4, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 4.0
      call mf_new_state( fid, srec0_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 64, state_record5, stat )
      call mf_print_error( 'mf_wrt_stream', stat )

      time = 5.0
      call mf_new_state( fid, srec1_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 98, state_record6, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 6.0
      call mf_new_state( fid, srec1_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 98, state_record7, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 7.0
      call mf_new_state( fid, srec1_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 98, state_record8, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 8.0
      call mf_new_state( fid, srec1_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 98, state_record9, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 9.0
      call mf_new_state( fid, srec1_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 98, state_record10, stat )
      call mf_print_error( 'mf_wrt_stream', stat )

      time = 10.0
      call mf_new_state( fid, srec2_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 87, state_record11, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 11.0
      call mf_new_state( fid, srec2_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 87, state_record12, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 12.0
      call mf_new_state( fid, srec2_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 87, state_record13, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 13.0
      call mf_new_state( fid, srec2_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 87, state_record14, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 14.0
      call mf_new_state( fid, srec2_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 87, state_record15, stat )
      call mf_print_error( 'mf_wrt_stream', stat )

      time = 15.0
      call mf_new_state( fid, srec2_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 87, state_record16, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 16.0
      call mf_new_state( fid, srec2_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 87, state_record17, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 17.0
      call mf_new_state( fid, srec2_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 87, state_record18, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 18.0
      call mf_new_state( fid, srec2_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 87, state_record19, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 19.0
      call mf_new_state( fid, srec2_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 87, state_record20, stat )
      call mf_print_error( 'mf_wrt_stream', stat )

      time = 20.0
      call mf_new_state( fid, srec1_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 98, state_record21, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 21.0
      call mf_new_state( fid, srec1_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 98, state_record22, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 22.0
      call mf_new_state( fid, srec1_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 98, state_record23, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 23.0
      call mf_new_state( fid, srec1_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 98, state_record24, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 24.0
      call mf_new_state( fid, srec1_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 98, state_record25, stat )
      call mf_print_error( 'mf_wrt_stream', stat )

      time = 25.0
      call mf_new_state( fid, srec0_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 64, state_record26, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 26.0
      call mf_new_state( fid, srec0_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 64, state_record27, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 27.0
      call mf_new_state( fid, srec0_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 64, state_record28, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 28.0
      call mf_new_state( fid, srec0_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 64, state_record29, stat )
      call mf_print_error( 'mf_wrt_stream', stat )
      
      time = 29.0
      call mf_new_state( fid, srec0_id, time, suffix, st_idx, stat )
      call mf_print_error( 'mf_new_state', stat )
      
      call mf_wrt_stream( fid, m_real, 64, state_record30, stat )
      call mf_print_error( 'mf_wrt_stream', stat )


c.....MUST close the family when finished

      call mf_close( fid, stat )
      call mf_print_error( 'mf_close', stat )
      
      end
