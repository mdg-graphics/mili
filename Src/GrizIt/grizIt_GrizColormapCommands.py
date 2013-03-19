############################################################################
# grizIt_ColormapCommands.py - 
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      June 10, 2010
#
# 
############################################################################
# Modifications:
#  I. R. Corey - June 10, 2010: Created.
#
############################################################################
#

import pdb
import os, sys, re, bisect, math

import visit
import grizIt_Main

import grizIt_Session as S
import grizIt_VisitInfo as VISITINFO

session = S.session

############################################################################################


####################
# ColorMap Commands
####################

class cgmap(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)

class chmap(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)

class conmap(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)

class grmap(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)

class igmap(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)
        
class invmap(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)
        
class ldmap(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)

class posmap(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)
 
class hotmap(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)


###################
# On/Off Functions
###################

# None at this time

