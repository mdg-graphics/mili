############################################################################
# grizIt_GrizHelpCommands.py - Griz Functions - Help Commands.
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


import visit
import grizIt_Main
import grizIt_Env
import grizIt_VisitInfo as VISITINFO

import grizIt_Session as S

############################################################################################

#########################
# Help Commands
#########################

class GRIZ_help(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class GRIZ_man(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class GRIZ_list(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class GRIZ_visitmessages(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class GRIZ_visit(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
