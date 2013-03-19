#!/usr/bin/python

##############################################################################
# Application: GrizIt - Griz CLI interface to the VisIt rendering
#
# grizIt_Mesh.py - Classes for creating and operating on Meshes.
#
#      
#      Lawrence Livermore National Laboratory
#      Jan 2, 1992
#
#############################################################################
# Modifications:
#
#  I. R. Corey - March 25, 2010: Created.
#
#############################################################################

import pdb, os, datetime

#############################################################################
# Enums
#############################################################################
class superclass:
    M_UNIT, M_NODE, M_TRUSS, M_BEAM, M_TRI, M_QUAD, M_TET, M_PYRAMID, M_WEDGE, M_HEX, M_MAT, M_MESH, M_SURFACE, M_PARTICLE = range(14)

class num_type:
     M_STRING, M_CHAR, M_FLOAT, M_FLOAT4, M_FLOAT8, M_INT,M_INT4, M_INT8 = range() 

#############################################################################
# Env Classes
#############################################################################
class Mesh:
    def __init__(self):
        self.material_qty = 0
        self.surface_qty = 0
        self.translate_material = False

        self.hide_material    = array("I", [0])
        self.disable_material = array("I", [0])
        self.exclude_material = array("I", [0])
        self.mtl_trans[0.0, 0.0, 0.0];
        # Bricks
        self.hide_brick_by_mat    = True
        self.hide_brick_by_result = False
        self.disable_brick_by_mat = True
        self.exclude_brick_by_mat = True
        
        self.hide_brick    = array("I", [0])
        self.disable_brick = array("I", [0])
        self.exclude_brick = array("I", [0])
        
        # Shells
        self.hide_shell_by_mat    = True
        self.hide_shell_by_result = False
        self.disable_shell_by_mat = True
        self.exclude_shell_by_mat = True
        
        self.hide_shell    = array("I", [0])
        self.disable_shell = array("I", [0])
        self.exclude_shell = array("I", [0])

        # Beams
        self.hide_beam_by_mat    = True
        self.hide_beam_by_result = False
        self.disable_beam_by_mat = True
        self.exclude_beam_by_mat = True
        
        self.hide_beam    = array("I", [0])
        self.disable_beam = array("I", [0])
        self.exclude_beam = array("I", [0])

        # Truss
        self.hide_truss_by_mat    = True
        self.hide_truss_by_result = False
        self.disable_truss_by_mat = True
        self.exclude_truss_by_mat = True
        
        self.hide_truss    = array("I", [0])
        self.disable_truss = array("I", [0])
        self.exclude_truss = array("I", [0])

         # Tets
        self.hide_tet_by_mat    = True
        self.hide_tet_by_result = False
        self.disable_tet_by_mat = True
        self.exclude_tet_by_mat = True
        
        self.hide_tet    = array("I", [0])
        self.disable_tet = array("I", [0])
        self.exclude_tet = array("I", [0])

         # Nodes
        self.hide_tet    = array("I", [0])
        self.disable_tet = array("I", [0])
        self.exclude_tet = array("I", [0])

        
      #end def

    def setenv(self):
        systype = os.getenv( "SYS_TYPE" )
    #end def    

#-----------------------------------------------------------------------------
