############################################################################
# Griz_GrizResultsCommands.py - Griz Functions - Results Commands.
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


#######################
# Results Commands
#######################

class show(S.GrizFunction):
    def __init__(self, resultname):
        self.resultname = resultname
    def do(self):
        Set_Material_Mode_Is_On(True)
        if self.resultname == "mat":
            if not session.mat_mode_on:
                d = S.DebugMsg("\nSet Material Mode")
                Set_Material_Mode_Is_On(True)
	    #end if
        else:
            resultExists = False

            for resultsVar in session.DBPrimalVars:
                if resultsVar == self.resultname:
                    resultExists = True
                    break
	        #end if
	    #end for
	
            if not resultExists:
                for resultsVar in session.DBDerivedVars:
                    if resultsVar == self.resultname:
                        resultExists = True
                        break
		    #end if
	        #end for
	    #end if
	
            if not resultExists:
                w = S.WarningMsg("No result named '%s' exists in database currently loaded." % (self.resultname))
            else:
                if resultsVar != CurResultsVar:
                    visit.SetActivePlots(2)
                    visit.ChangeActivePlotsVar(resultsVar)
		
                    session.cur_result = resultsVar
	        #end if
	    
                if  session.mat_mode_on:
                    visit.Set_Material_Mode_Is_On(False)
	        #end if
	    #end else
        #end else
    #end def
#end class

class rzero(S.GrizFunction):
    def __init__(self, args):
        self.args = args
    def do(self):
        n = ' '.join(self.args)
        n = float(n)
        env.rzero=n
        env.rzero_set=True
    #end def
#end class

class rmin(S.GrizFunction):
    def __init__(self, args):
        self.args = args
    def do(self):
        n = ' '.join(self.args)
        n = int(n)
        env.rmin=n
        env.rmin_set=True
    #end def
#end class

class rmax(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)

class clrthr(S.GrizFunction):
    def __init__(self, args):
        self.args = args
    def do(self):
        env.rzero_set=False
        env.rmin_set=False        
        env.rmax_set=False
   #end def
#end class

class refstate(S.GrizFunction):
    def __init__(self, args):
        self.args = args
    def do(self):
        n = ' '.join(self.args)
        n = int(n)
        env.ref_state=n
    #end def
#end class

class globmm(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class resetmm(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class resetmm(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)
        self.args = args
        arg_list = ' '.join(self.args)
        arg_list = arg_list.split()
        scale    = float(arg_list[0])
        offset   = float(arg_list[1])
    #end def
#end class

class coordxf(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
class dirv(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class dirv(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class dir3n(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class dir3p(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class clrconv(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class pref(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class dia(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
 
class ym(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

########################################################
## THESE FUNCTIONS BELOW ARE ALL FOR THE SWITCH FACTORY
########################################################
    
class swTerpMode(S.GrizFunction):
    """ SWITCH: Set result color interpolation mode"""
    def __init__(self, key):
        self.key = key
    def do(self):
        w = S.WarningMsg('This command is not yet implemented')
        if self.key == 'interp':
            pass
        elif self.key == 'gterp':
            pass
        elif self.key == 'noterp':
            pass
        else:
            w = S.WarningMsg('GrizIt could not parse the item after "switch"')
            
#end class

class swStrainType(S.GrizFunction):
    """ SWITCH: Set strain type """
    def __init__(self, key):
        self.key = key
    def do(self):
        w = S.WarningMsg('This command is not yet implemented')
        if self.key == 'infin':
            pass
        elif self.key == 'alman':
            pass
        elif self.key == 'grn':
            pass
        elif self.key == 'rate':
            pass
        else:
            w = S.WarningMsg('GrizIt could not parse the item after "switch"')
#end class

class swElementReference(S.GrizFunction):
    """ SWITCH: Set element reference frame """
    def __init__(self, key):
        self.key = key
    def do(self):
        w = S.WarningMsg('This command is not yet implemented')
        if self.key == 'rglob':
            pass
        elif self.key == 'rloc':
            pass
        else:
            w = S.WarningMsg('GrizIt could not parse the item after "switch"')
#end class

class swShellSurface(S.GrizFunction):
    """ SWITCH: Set shell result surface """
    def __init__(self, key):
        self.key = key
    def do(self):
        w = S.WarningMsg('This command is not yet implemented')
        if self.key == 'middle':
            pass
        elif self.key == 'inner':
            pass
        elif self.key == 'outer':
            pass
        else:
            w = S.WarningMsg('GrizIt could not parse the item after "switch"')
#end class

class swResultMinMax(S.GrizFunction):
    def __init__(self, key):
        self.key = key
    def do(self):
        w = S.WarningMsg('This command is not yet implemented')
        if self.key == 'mglob':
            pass
        elif self.key == 'mstat':
            pass
        else:
            w = S.WarningMsg('GrizIt could not parse the item after "switch"')
#end class

###################
# On/Off Functions
###################

class OnOffDerived(S.OnOffFunction):
    """ Turn on (off) free node rendering mode"""
    ## No visible change in Griz. Will create flag.
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
    #end def
    def on(self):
        session.OnOffDerived=True
    def off(self):
        session.OnOffDerived=False
#end class

class OnOffPrimal(S.OnOffFunction):
    """ Turn on (off) free node rendering mode"""
    ## No visible change in Griz. Will create flag.
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
    def on(self):
        session.OnOffPrimal=True
    def off(self):
        session.OnOffPrimal=False

class OnOffConv(S.OnOffFunction):
    """ Turn on (off) free node rendering mode"""
    ## No visible change in Griz. Will create flag.
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
    def on(self):
        session.OnOffConv=True
    def off(self):
        session.OnOffConv=False

