#!/usr/bin/python
############################################################################
# grizIt_Callbacks.py - Define and register all callback functions
#      commands.
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      June 10, 2010
#
############################################################################
# Modifications:
#  I. R. Corey - August 20, 2010: Created.
#
############################################################################
#

import pdb

import os, sys, re, bisect, math, copy
import grizIt_VisitInfo

from grizIt_Main import *
from grizIt_Env import *

import grizIt_Session as S

def Initialize_GrizIt_Callbacks():

    # Register all callback functions
    paResponder = pickAttsResponder(handle_pick, 0)

    visit.RegisterCallback("AnimationStopRPC",           When_AnimationStop)
    visit.RegisterCallback("PickAttributes",             When_PickAtts)
    visit.RegisterCallback("TimeSliderNextStateRPC",     When_TimeSliderNextState)
    visit.RegisterCallback("TimeSliderPreviousStateRPC", When_TimeSliderPreviousState)
    visit.RegisterCallback("SetTimeSliderStateRPC",      When_SetTimeSliderState)
#end def
 
#
# This class lets get pick attributes routed to handle_pick so that
# changes to pickAtts not caused by the user picking are ignored. 
#
class pickAttsResponder:
    def __init__(self, cb, cbdata):
        self.count = 0
        self.cb = cb
        self.cbdata = cbdata
    def reset(self, c):
        self.count = c
    def respond(self, atts):
        if self.count == 0:
            self.count = 0
            self.cb(atts, self.cbdata)
        else:
            self.count = self.count - 1

def handle_pick(atts, userdata):
    p = S.DebugMsg("We received a real user-caused pick event"+ atts+ userdata)
#end def

########################################################################
# Viewing Callback functions
########################################################################

def When_PickAtts(atts, obj):
    obj.respond(atts)
    d = S.DebugMsg("\nWhen_PickAtts", atts)
#end def

def When_TimeSliderNextState(atts):
    pass
#end def

def When_TimeSliderPreviousState(atts):
    pass
#end def

def When_SetTimeSliderState(atts):
    pass
#end def

def When_TimeSliderNextState(atts):
    if session.cur_state < DBStateCount:
       session.cur_state += 1
    #end if

    if ViewCenteringOn:
       RecenterView()
    #end if
#end def

def When_TimeSliderPreviousState(atts):
    if session.cur_state > 1:
       session.cur_state -= 1
    #end if

    if ViewCenteringOn:
	RecenterView()
    #end if
#end def

def When_AnimationStop(atts):
    windowInfo = GetWindowInformation()
    
    session.cur_state = windowInfo.timeSliderCurrentStates[0] + 1
    
    if session.cur_state < session.min_state:
	GRIZ_state_statenumber(session.min_state)
    elif session.cur_state > session.max_state:
	GRIZ_state_statenumber(ession.max_state)
    #end elif

    if ViewCenteringOn:
	RecenterView()
    #end if

#-----------------------------------------------------------------------------

if __name__ == "__main__":
    visit.Launch()
    visit.Initialize_GrizIt()   



