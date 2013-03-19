############################################################################
# grizIt_GrizParticleCommands.py - Griz Functions - Particle Commands.
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

#########################
# Particle Commands
#########################

class pres(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
class pscale(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class fres(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class fscale(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
