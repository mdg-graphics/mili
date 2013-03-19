
##############################################################################
# Application: GrizIt - Griz CLI interface to the VisIt rendering
#
# grizIt_VisitCommands.py - Classes for performing functions specific
# to VisIt (no counterpart in Griz)
#
#      
#      Lawrence Livermore National Laboratory
#      March, 2010
#
#############################################################################
# Modifications:
#
#  I. R. Corey - August 25, 2010: Created.
#############################################################################

import pdb, datetime
import os, sys, re, bisect, math
from array import *

import fileinput

import grizIt_Session as GS

#########################
# Executive Commands
#########################

class VisitFunction(object):
    def __init__(self):
        pass
    def do(self):
        pass

class opengui(VisitFunction):
    def __init__(self,args):
        self.args = args
    def do(self):
        if ( not GS.env.batch_mode and GS.env.visitGUI ):
             visit.OpenGUI()

if __name__ == "__main__":
    pass


