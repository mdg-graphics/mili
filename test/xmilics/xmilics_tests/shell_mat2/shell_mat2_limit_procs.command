#
# shell_mat2_limit_states command file
#
# This file contains a list of commands that describe what data should be tested between the
# uncombined plot files and the combined plot files produced by Xmilics. This command file is
# parsed by the test suite to produce a .jansw file for the combined plot database and a .answ file
# for the uncombined plot database. These files are then compared.
#

Initial_Nodal_Positions all

Element_Labels all

Element_Connectivity shell all

Materials all

# Write out all state variables for states 2, 8, and 11
State_Variables_All 2
State_Variables_All 8
State_Variables_All 11

# Add specific state variables later
State_Variable shell_11_ipt_1_sx
State_Variable shell_11_ipt_1_sy
State_Variable shell_11_ipt_1_sz

State_Variable shell_10_ipt_1_sx
State_Variable shell_10_ipt_1_sy
State_Variable shell_10_ipt_1_sz
