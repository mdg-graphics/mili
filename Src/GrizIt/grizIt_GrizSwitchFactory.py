############################################################################
# grizIt_GrizSwitchFactory.py - Function Factory: Switch Commands
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      
#
# 
############################################################################
# Modifications:
#  S. E. Drakeley - July 2010: Created
#
############################################################################

## For switch factories:
import grizIt_GrizResultsCommands as RC
import grizIt_GrizMeshControlCommands as MC
import grizIt_GrizTHCommands as TH
import grizIt_GrizVisCommands as VC
import grizIt_GrizAddtlCommands as AC
import grizIt_GrizMousePickCommands as MP
import grizIt_GrizVideoCommands as VID
import grizIt_Session as S

def get_null():
    #from grizIt_ParseGrizCommand import GRIZ_null
    return None #grizNull

GRIZ_null = get_null()
VAR = 100

############################
# The switch dictionary:
############################

switch_commands = {
#   Keyword             Function          # Help Text
# Results Commands
    "interp":             RC.swTerpMode,  #Set result color interpolation mode
    "gterp":              RC.swTerpMode,
    "noterp":             RC.swTerpMode,

    "infin":              RC.swStrainType, # Set strain type
    "alman":              RC.swStrainType,
    "grn":                RC.swStrainType,
    "rate":               RC.swStrainType,

    "rglob":              RC.swElementReference, # Set element reference frame
    "rloc":               RC.swElementReference,

    "middle":             RC.swShellSurface, #Set Shell Result surface
    "inner":              RC.swShellSurface,
    "outer":              RC.swShellSurface,

    "mglob":              RC.swResultMinMax, #Set result min/max scope
    "mstat":              RC.swResultMinMax,


# Mesh Control COmmands
    "hidden":             MC.swMeshRend, #Set mesh rendering style
    "solid":              MC.swMeshRend,
    "cloud":              MC.swMeshRend,
    "none":               MC.swMeshRend,
    "wf":                 MC.swMeshRend,
    "wft":                MC.swMeshRend,

# Time History Commands
#    "glyphstag":          TH.swVertGlyph, # Set vertical alignment of glyphs
#    "glyphalign":         TH.swVertGlyph,

# Visualization Commands
    "nodvec":             VC.swVecSrc, # Set vector source
    "grdvec":             VC.swVecSrc,

# Additional View, Rendering, and Mesh Control Commands
    "symcu":              AC.swRefl, # Set reflections to cumulative or not
    "symor":              AC.swRefl,

    "persp":              AC.swProj, # Set Projection Type
    "ortho":              AC.swProj,

    "smooth":             AC.swPoly, #Set polygon shading type
    "flat":               AC.swPoly,
    
# Mouse Picking COmmands
    "pichil":             MP.swPick, # Set Pick mode
    "picsel":             MP.swPick,

# Video Commands
    "norasp":             VID.swAsp, # Set image aspect ratio
    "vidasp":             VID.swAsp

    }

#################################################################################

# This is the switch FACTORY. The skeleton of the switch function is defined
# in grizIt_Session.py


class switch(S.GrizFunction):
    """ This is the switch factory. """

    def __init__(self, command_tokens):      
        self.class_key = command_tokens[0] # There should only ever be one arg
        self.command_dict = grizIt_GrizSwitchFactory.switch_commands
        self.PyCommand    = self.command_dict[self.class_key]
        self.newInstance  = self.PyCommand(self.class_key)
        
    def do(self):      
        self.newInstance.do()

