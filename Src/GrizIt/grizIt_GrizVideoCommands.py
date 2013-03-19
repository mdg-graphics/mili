############################################################################
# grizIt_GrizVideoCommands.py - Griz Functions - Video Commands.
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
import grizIt_VisitInfo
import grizIt_Session as S

session = S.session

############################################################################################


#######################
# Video Commands
#######################

class vidti(S.GrizFunction):
    def __init__(self, line, title):
        S.GrizFunction.__init__(self)
        self.line = line
        self.title = title
        

class vidttl(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
 


class swAsp(S.GrizFunction):
    """ SWITCH: Set image aspect ratio """
    def __init__(self, key):
        self.key = key
    def do(self):
        w = S.WarningMsg( 'This command is not yet implemented')
        if self.key == 'norasp':
            pass
        elif self.key == 'vdasp':
            pass        
        else: w = S.WarningMsg('GrizIt could not parse the item specified after "switch"')


##################
# On Off Commands
##################
class OnOffSafe(S.OnOffFunction):
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
