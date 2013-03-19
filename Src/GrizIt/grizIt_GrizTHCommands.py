############################################################################
# grizIt_GrizTHCommands.py - Griz Functions - Time History Commands.
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

from grizIt_Main    import *
import grizIt_Session as S
# from grizIt_Plot    import *

import grizIt_VisitInfo as VISITINFO
# import grizIt_GuiPlot   as PLT

session = S.session
env     = S.env

############################################################################################
 

#############################
# Time History (TH) Commands
#############################
class GRIZ_plot(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)
        self.args = args
        angle = float(self.args[0])
    #end def

class swVertGlyph(S.GrizFunction):
    """ SWITCH: Set vertical alignment of glyphs """
    def __init__(self, key):
        self.key = key
    def do(self):
        w = S.WarningMsg('This command is not yet implemented')
        if self.key == 'glyphstag':
            pass
        elif self.key == 'glyphalign':
            pass
        else: w = S.WarningMsg('GrizIt could not parse the item specified after "switch"')
    #end def
#end class

class glyphqty(S.GrizFunction):
    def __init__(self):
        S.GrizFuction.__init__(self)

class glyphscl(S.GrizFunction):
    def __init__(self):
        S.GrizFuction.__init__(self)

class plot(S.GrizFunction):
    def __init__(self):
        S.GrizFuction.__init__(self)

class oplot(S.GrizFunction):
    def __init__(self):
        S.GrizFuction.__init__(self)
 
class outth(S.GrizFunction):
    def __init__(self):
        S.GrizFuction.__init__(self)

class gather(S.GrizFunction):
    def __init__(self):
        S.GrizFuction.__init__(self)

class delth(S.GrizFunction):
    def __init__(self):
        S.GrizFuction.__init__(self)

class mmloc(S.GrizFunction):
    def __init__(self):
        S.GrizFuction.__init__(self)

class timhis(S.GrizFunction):
    def __init__(self):
        env.THWin = PLT.PlotWin()
        xdata = [0.01*x for x in range(1000)] #That's 0 to 10 in steps of 0.01
        ydata = [math.sin(x) for x in xdata]    
        id    = env.THWin.add_curve("Curve1", xdata, ydata, 4, "blue")

###################
# On/Off Functions
###################

class OnOffGlyphs(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        # The key to these types of functions is annotation attributes: visit.GetAnnotationAttributes() and set annotation attributes. See some of the other on off functions, like on box

class OnOffGlyphcol(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)


class OnOffMinmax(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)


class OnOffPlotcoords(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)

class OnOffPlotcol(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)


class OnOffPlotgrid(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)


class OnOffLeglines(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)


