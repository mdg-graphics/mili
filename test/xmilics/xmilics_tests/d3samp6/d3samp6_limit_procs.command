#
# d3samp6_limit_proc command file
#
# This file contains a list of commands that describe what data should be tested between the
# uncombined plot files and the combined plot files produced by Xmilics. This command file is
# parsed by the test suite to produce a .jansw file for the combined plot database and a .answ file
# for the uncombined plot database. These files are then compared.
#

Initial_Nodal_Positions all

Element_Labels all

Element_Connectivity brick all
Element_Connectivity shell all
Element_Connectivity beam all
Element_Connectivity cseg all

Materials all

# All state variables for states 10, 59, and 101
State_Variables_All 10
State_Variables_All 59
State_Variables_All 101

# All states are output for these
State_Variable node_12_ux
State_Variable node_97_ux
State_Variable node_97_uz
State_Variable brick_1_sx
State_Variable shell_1_ipt_1_sx
State_Variable shell_1_ipt_2_sx
