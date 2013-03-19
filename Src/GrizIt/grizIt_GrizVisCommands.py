############################################################################
# grizIt_GrizVisCommands.py - Griz Functions - Visualization Commands.
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

import grizIt_inputHelper as InputHelper
import visit

import grizIt_Main
import grizIt_Session as S

session = S.session
env     = S.env

############################################################################################
 

#########################
# Visualization Commands
#########################

class swVecSrc(S.GrizFunction):
    """ SWITCH: Set vector Source """
    def __init__(self, key):
        self.key = key
    #end def
    def do(self):
        w = S.WarningMsg('THis command is not yet implemented')
        if self.key == 'nodvec':
            pass
        elif self.key == 'grdvec':
            pass
        else: w = S.WarningMsg('GrizIt could not parse the item specified after "switch"')
    #end def
#end class        

class cutpln(S.GrizFunction):
    """ Define a cutting plane.
    The cutting plane is defined by a point on the plane and a vector normal to the plane.
    The cutting plane is added to any previously defined cutting planes
    """
    def __init__(self, coordsList):
        self.inputHelper = InputHelper.InputParser()
        self.coordsList = self.inputHelper.inputToList(coordsList)
        [p_x, p_y, p_z, n_x, n_y, n_z] = self.coordsList
        self.p_x = p_x
        self.p_y = p_y
        self.p_z = p_z
        self.n_x = n_x
        self.n_y = n_y
        self.n_z = n_z
    def do(self):
        visit.AddOperator("Slice",1)
        numPlots = visit.GetNumPlots() #session.NumPlots
        visit.SetActivePlots(numPlots)
        session.cutPlanes.append(numPlots)
        SliceAtts = visit.SliceAttributes()
        SliceAtts.originPoint = (self.p_x, self.p_y, self.p_z)
        SliceAtts.normal = (self.n_x, self.n_y, self.n_z)
        SliceAtts.project2d = 0 #0: False, 1: True
        SliceAtts.interactive = 1 #0: False, 1: True
        visit.SetOperatorOptions(SliceAtts, 1)
        visit.DrawPlots()
    #end def
#end class

class cutrpln(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class clrcut(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class conwid(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class ison(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class isop(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class isov(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class clriso(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class vgrid1(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class vgrid2(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class vgrid3(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
class clrvgr(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class veccm(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class vecscl(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class vhdscl(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class outvec(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class ptrace(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class aptrace(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class ptstat(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class prake(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class clrpar(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class ptlim(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class ptwid(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class ptdis(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class outpt(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class inpt(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

##################
# On-Off Commands
##################

class OnOffCut(S.GrizFunction):
    """ Turns off display of the currently defined cutting planes.

        # Still to do: make a continuous loop that checks for these
    """
    def __init__(self, flag):
        d = S.DebugMsg('This command still needs a little work')
        self.flag = flag
    def do(self):
        if self.flag == 'on':
            session.OnOffCutDisplay = True
            visit.DrawPlots()
        elif self.flag == 'off':
            session.OnOffCutDisplay = False
            visit.RemoveOperator(session.cutPlanes.pop())
            visit.DrawPlots()
            
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+self.flag)
    #end def
#end class        
        
class OnOffRough(S.GrizFunction):
    """ Turn on/off the rough cutting plane display. The 'rough cut' option deletes all elements that intersect the cutting plane or are on the
    side of the cutting plane in which the plane normal points. This commandi suseful in checking for degenerately shaped elements in the interior
    of a volume or for selecting nodes in the interior of a mesh

    """
    def __init__(self, flag):
        d = S.DebugMsg('This command still needs a little work')
        self.flag = flag
    def do(self):
        if self.flag == 'on':
            session.RoughDisplay = True #Need to actually change something in visit
        elif self.flag == 'off':
            session.RoughDisplay = False
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+self.flag)
    #end def
#end class       

class OnOffCon(S.GrizFunction):
    """ Turns off ???
    
    """
    def __init__(self, flag):
        d = S.DebugMsg('This command still needs a little work')
        self.flag = flag
    def do(self):
        if self.flag == 'on':
            session.OnOffCutDisplay == True
        elif self.flag == 'off':
            session.OnOffCutDisplay == False
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+self.flag)
    #end def
#end class        

class OnOffIso(S.GrizFunction):
    """ Turns off ???
    
    """
    def __init__(self, flag):
        d = S.DebugMsg('This command still needs a little work')
        self.flag = flag
    def do(self):
        if self.flag == 'on':
            session.OnOffCutDisplay == True
        elif self.flag == 'off':
            session.OnOffCutDisplay == False
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+self.flag)
    #end def
#end class        
 
class OnOffVec(S.GrizFunction):
    """ Turns off ???
    
    """
    def __init__(self, flag):
        d = S.DebugMsg('This command still needs a little work')
        self.flag = flag
    def do(self):
        if self.flag == 'on':
            session.OnOffCutDisplay == True
        elif self.flag == 'off':
            session.OnOffCutDisplay == False
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+self.flag)
    #end def
#end class        

class OnOffSphere(S.GrizFunction):
    """ Turns off ???
    
    """
    def __init__(self, flag):
        d = S.DebugMsg('This command still needs a little work')
        self.flag = flag
    def do(self):
        if self.flag == 'on':
            session.OnOffCutDisplay == True
        elif self.flag == 'off':
            session.OnOffCutDisplay == False
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+self.flag)
    #end def
#end class        

class OnOffPn(S.GrizFunction):
    """ Turns off ???
    
    """
    def __init__(self, flag):
        d = S.DebugMsg('This command still needs a little work')
        self.flag = flag
    def do(self):
        if self.flag == 'on':
            session.OnOffCutDisplay == True
        elif self.flag == 'off':
            session.OnOffCutDisplay == False
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+self.flag)
    #end def
#end class        

class OnOffFn(S.GrizFunction):
    """ Turns off ???
    
    """
    def __init__(self, flag):
        d = S.DebugMsg('This command still needs a little work')
        self.flag = flag
    def do(self):
        if self.flag == 'on':
            session.OnOffCutDisplay == True
        elif self.flag == 'off':
            session.OnOffCutDisplay == False
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+self.flag)
    #end def
#end class        
