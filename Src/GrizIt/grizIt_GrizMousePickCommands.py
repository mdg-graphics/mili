############################################################################
# grizIt_GrizMousePickCommands.py - Griz Functions: Mouse Pick Commands.
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

import os, sys, re, bisect, math

import visit
import grizIt_Main
import grizIt_Session as S
import grizIt_VisitInfo as VISITINFO

session = S.session

############################################################################################
 
######################
# Mouse Pick Commands
######################

class setpick(S.GrizFunction):
    def __init__(self, args):
        self.button     = int(args[0])
        self.className  = args[1]
    def do(self):
        if self.button==1:
           session.btn1Pick = self.className
        elif self.button==2:
           session.btn2Pick = self.className
        else: session.btn3Pick = self.className
        d = S.DebugMsg("\nIn set pick"+ self.className)
#end def


class minmove(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)
        session.minmove = int(args[0])
   
class swPick(object):
    """ SWITCH: Set pick mode """
    def __init__(self, key):
        self.key = key

    def do(self):
        if self.key == 'pichil':
            session.pickMode = self.key 
        elif self.key == 'picsel':
            session.pickMode = self.key
        else: w = S.WarningMsg('GrizIt could not parse the item specified after "switch"')

###################
# On/Off Functions
###################

# None at thos time 
