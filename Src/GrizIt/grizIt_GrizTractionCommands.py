############################################################################
# grizIt_GrizTractionCommands.py - Griz Functions - Traction Commands.
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      May 07, 2010
#
# 
############################################################################
# Modifications:
#  I. R. Corey - May 07, 2010: Created.
#
############################################################################
#

import pdb
import os, sys, re, bisect, math
import grizIt_VisitInfo

import visit
import grizIt_Main
import grizIt_Session as S

session = S.session
env     = S.env

############################################################################################
 

####################
# Traction Commands
####################

class traction(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
