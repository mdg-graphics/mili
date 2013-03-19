############################################################################
# grizIt_GrizIndepSurfaceCommands.py - Griz Functions -
#                                      Indep Surface Commands.
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
import grizIt_Session as S
import grizIt_VisitInfo as VISITINFO

session = S.session
env     = S.env

############################################################################################
 
#########################
# Indep Surface Commands
######################### 

class rect(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class spot(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class ring(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class tube(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class poly(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class inref(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class outref(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class clrref(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class inslp(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

##################
# On-Off Commands
##################
class OnOffRefsrf(S.GrizFunction):
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
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: ',self.flag)
    #end def
#end class        
 
class OnOffRefres(S.GrizFunction):
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
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: ',self.flag)
    #end def
#end class        

class OnOffExtsrf(S.GrizFunction):
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
        else: w = S.WarningMsg('Flag should be (on) or (off). You entered: ',self.flag)
    #end def
#end class        
