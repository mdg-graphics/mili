############################################################################
# grizIt_GrizSurfaceFactory.py - Function Factory: Surface Commands
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#
# 
############################################################################
# Modifications:
#  I.R. Corey - September 2010: Created
#
############################################################################

## For Surface Factories:

import grizIt_GrizIndepSurfaceCommands as SC
import grizIt_Session as S

##########################
# The surface dictionary:
##########################

surface_commands = {
#   Keyword             Function          # Help Text
# Results Commands
    "rect": (SC.rect,  "Define rectangular surface for traction calculation"),
    "spot": (SC.spot,  "Define circular surface for traction calculation"),
    "ring": (SC.ring,  "Define annular surface for traction calculation"),
    "tube": (SC.tube,  "Define (conical) tubular surface for traction calculation"),
    "poly": (SC.poly,  "Read surface definition from file for traction calculation"),
    }

#################################################################################

class surface(S.GrizFunction):
    """ This is the Surface Factory. """

    def __init__(self, command_tokens):
        self.class_key = command_tokens[0] # There should only ever be one arg
        self.command_dict = self.surface_commands
        self.PyCommand    = self.command_dict[self.class_key]
        self.newInstance  = self.PyCommand(self.class_key)
    #end def
    
    def do(self):
        self.newInstance.do()
    #end def
#end class
