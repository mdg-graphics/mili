############################################################################
# grizIt_GrizAddtlCommands.py - Griz Functions - Additional Commands.
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
import sys
import visit

import grizIt_Session as S
import grizIt_VisitInfo as VISITINFO

session = S.session
env     = S.env

class swRefl(S.GrizFunction):
    """ SWITCH: Set reflections to cumulative or not """
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key
    def do(self):
        S.GrizFunction.do(self)
        if self.key == 'symcu':
            pass
        elif self.key == 'symor':
            pass        
        else: w = S.WarningMsg('GrizIt could not parse the item specified after "switch"')

class swProj(S.GrizFunction):
    """ SWITCH: Set projection type """
    def __init__(self, key):
        self.key = key
    def do(self):
        d = S.DebugMsg("Double check that this switch Proj command is working")
        v = visit.GetView3D()
        if self.key == 'persp':
           v.perspective = 1
        elif self.key == 'ortho':
           v.perspective = 0
        visit.SetView3D(v)

class swPoly(S.GrizFunction):
    """ SWITCH: Set polygon shading type """
    def __init__(self, key):
        self.key = key
    #end  def
    def do(self):
        S.GrizFunction.do(self)
        if self.key == 'smooth':
            pass
        elif self.key == 'flat':
            pass        
        else: w = S.WarningMsg('GrizIt could not parse the item specified after "switch"')
    #end  def
#end class


############################
# Set Color Command Options
############################
class setcolText(S.GrizFunction):
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key
    

class setcolMesh(S.GrizFunction):
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key

class setcolEdges(S.GrizFunction):
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key
    

class setcolFg(S.GrizFunction):
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key
    

class setcolBg(S.GrizFunction):
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key
    

class setcolCon(S.GrizFunction):
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key
    

class setcolHilite(S.GrizFunction):
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key
    

class setcolPlot(S.GrizFunction):
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key
    

class setcolRmax(S.GrizFunction):
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key
    

class setcolRmin(S.GrizFunction):
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key
    

class setcolSelect(S.GrizFunction):
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key
    

class setcolVec(S.GrizFunction):
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key
    

class setcolDefault(S.GrizFunction):
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key
    

##################
# On Off Commands
##################
class OnOffSym(S.OnOffFunction):
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
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+str(self.flag))
    #end def
#end class

class OnOffParticles(S.OnOffFunction):
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
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+str(self.flag))
    #end def
#end class

class OnOffLighting(S.OnOffFunction):
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
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+str(self.flag))
    #end def
#end class

class OnOffAutosz(S.OnOffFunction):
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
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+str(self.flag))
    #end def
#end class

class OnOffBbmax(S.OnOffFunction):
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
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+str(self.flag))
    #end def
#end class

class OnOffShrfac(S.OnOffFunction):
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
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+str(self.flag))
    #end def
#end class

class OnOffHexoverlap(S.OnOffFunction):
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
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: '+str(self.flag))
    #end def
#end class
