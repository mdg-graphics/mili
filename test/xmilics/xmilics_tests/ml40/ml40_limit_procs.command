#
# ml40_limit_procs command file
#
# This file contains a list of commands that describe what data should be tested between the
# uncombined plot files and the combined plot files produced by Xmilics. This command file is
# parsed by the test suite to produce a .jansw file for the combined plot database and a .answ file
# for the uncombined plot database. These files are then compared.
#

Initial_Nodal_Positions all

Element_Labels all

Element_Connectivity brick all
 
Materials all

# Write all state variables for state 34, 56, and 81
State_Variables_All 34
State_Variables_All 56
State_Variables_All 81

State_Variable_State_Output 1,9,17,25,33,41,49,57,65,73,81

State_Variable node_667_uy
State_Variable node_667_uz
State_Variable node_667_vy
State_Variable node_667_vz

State_Variable brick_217_sx
State_Variable brick_217_sy
State_Variable brick_217_sz
State_Variable brick_217_sxy
State_Variable brick_217_syz
State_Variable brick_217_szx
