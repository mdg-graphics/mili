############################################################################
# grizIt_GrizViewingModeCommands.py - Griz Functions - Viewing Mode Commands.
# 
#      Sara Drakeley
#      Lawrence Livermore National Laboratory
#      July 9, 2010
#
# 
############################################################################
# Modifications:
#  S. Drakeley - 7/9/10 Created
#
############################################################################
#

import os, sys, re, bisect, math

import visit
import grizIt_Main
import grizIt_Session as S
import grizIt_VisitInfo

session = S.session
env     = S.env

############################################################################################

###################
# On/Off Functions
###################

class OnOffEi(S.OnOffFunction):
    """ Turn on (off) Error Indicator rendering mode"""
    ## Just turns the background grey. Will create flag, but probably requires
    ##     feedback somewhere to know to indicate errors
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag

    def on(self):
        session.ei_mode=True
    def off(self):
        session.ei_mode=False     
            
class OnOffGs(S.OnOffFunction):
    """ Turn on (off) greyscale rendering of disabled materials"""
    ## Turns the object greyscale. Doesn't seem to have feedback with enable/disable
    ##     as far as Griz goes.
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag

    def on(self):
        session.gs_mode=True
    def off(self):
        session.gs_mode=False
        
class OnOffPn(S.OnOffFunction):
    """ Turn on (off) particle rendering mode"""
    ## No visible change in Griz. Will create flag.
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
    #end def
    def on(self):
        session.pn_mode = True
    def on(self):
        session.pn_mode = False
        
class OnOffFn(S.OnOffFunction):
    """ Turn on (off) free node rendering mode"""
    ## No visible change in Griz. Will create flag.
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
    def on(self):
        session.fn_mode=True
    def off(self):
        session.fn_mode=False
