#
# basic2_default_combine command file
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

# All state variables for states 10, 59, and 101
State_Variables_All 10
State_Variables_All 59
State_Variables_All 101

State_Variable_State_Output 1,11,21,31,41,51,61,71,81,91,101
State_Variable node_12_ux
State_Variable node_12_uy
State_Variable node_12_uz
State_Variable brick_1_sx
State_Variable brick_1_sz
State_Variable brick_12_sx
State_Variable brick_12_sz
