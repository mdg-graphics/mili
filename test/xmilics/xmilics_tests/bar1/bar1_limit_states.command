#
# bar1_limit_states command file
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

State_Variables_All 40

State_Variable node_667_uy
State_Variable node_667_uz
State_Variable node_667_vy
State_Variable node_667_vz

State_Variable brick_601_sx
State_Variable brick_601_sz
State_Variable brick_601_sxy
State_Variable brick_601_szx
