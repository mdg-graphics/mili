#
# bar5_limit_procs command file
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

State_Variable node_391_uy
State_Variable node_391_uz
State_Variable node_391_vy
State_Variable node_391_vz

State_Variable brick_201_sx
State_Variable brick_201_sy
State_Variable brick_201_sz
State_Variable brick_852_sxy
State_Variable brick_852_syz
State_Variable brick_852_szx
