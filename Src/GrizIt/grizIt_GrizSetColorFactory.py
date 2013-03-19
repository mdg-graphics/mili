############################################################################
# grizIt_GrizSetColorFactory.py - Function Factory: Set Color Commands
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#
# 
############################################################################
# Modifications:
#  I. R. Corey - September  2010: Created
#
############################################################################

## For switch factories:

import grizIt_GrizAddtlCommands as AC
import grizIt_Session as S



VAR = 100

#######################
# The color dictionary:
#######################

setcol_commands = {
#   Keyword             Function          # Help Text
    "text":             (AC.setcolText,    "Set text color"),
    "mesh":	        (AC.setcolMesh,    "Set mesh line color"),
    "edges":            (AC.setcolEdges,   "Set edge line color"),
    "fg":	        (AC.setcolFg,  	   "Set foreground color"),
    "bg":		(AC.setcolBg,	   "Set background color"),
    "con":       	(AC.setcolCon,	   "Set contour color"),
    "hilite":    	(AC.setcolHilite,  "Set hilite color"),
    "plot":      	(AC.setcolPlot,	   "Set color for plot curve n"),
    "rmax":      	(AC.setcolRmax,	   "Set rmax threshold color"),
    "rmin":      	(AC.setcolRmin,	   "Set rmin threshold color"),
    "select":    	(AC.setcolSelect,  "Set select color"),
    "vec":       	(AC.setcolVec,	   "Set vector color"),
    "default":   	(AC.setcolDefault, "Reset color for target to default"),
    }

#################################################################################

class setcol(S.GrizFunction):
    """ This is the set color factory. """

    def __init__(self, command_tokens):
        self.class_key = command_tokens[0] # There should only ever be one arg
        self.command_dict = setcol_commands
        self.PyCommand    = self.command_dict[self.class_key]
        self.newInstance  = self.PyCommand(self.class_key)
        
    def do(self):
        self.newInstance.do()

