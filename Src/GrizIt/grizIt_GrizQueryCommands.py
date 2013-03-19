############################################################################
# grizIt_GrizQueryCommands.py - Griz Functions - Query Commands.
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
import grizIt_VisitInfo

import grizIt_Session as S
session = S.session
env     = S.env


############################################################################################


#################
# Query Commands
#################

class tell(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class info(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class lts(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
        
class tellmm(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class telliso(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class tellpos(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class tellem(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
