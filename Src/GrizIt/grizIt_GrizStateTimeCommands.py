############################################################################
# grizIt_GrizStateTimeCommands.py - Griz Functions - State and Time Commands.
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
import grizIt_VisitInfo

import grizIt_Session as S
import grizIt_ParseGrizCommand as P
import grizIt_TestSuite as T
from time import sleep

from threading import Thread

session = S.session
env     = S.env

############################################################################################


#############################
# State and Time Commands
#############################

class minst(S.GrizFunction):
    def __init__(self, statenumber):
        S.GrizFunction.__init__(self)
        self.state = statenumber
    def do(self):
        if self.state > session.maxst:
            w = S.WarningMsg("Specified minimum state exceeds current maximum state.")
        elif self.state < 1:
            w = S.WarningMsg("Specified minimum state is zero or negative.")
        else:
            session.minst = self.state
            
            if session.curst < session.minst:
                GRIZ_set_state(session.minst)
	    #end if
        #end else
    #end def

class maxst(S.GrizFunction):
    def __init__(self, statenumber):
        S.GrizFunction.__init__(self)
        self.statenumber = statenumber
    def do(self):
        if self.statenumber < session.min_state:
            w = S.WarningMsg("Specified maximum state is less than the current minimum state.")
        elif self.statenumber > session_maxst:
            w = S.WarningMsg("Specified maximum state exceeds the number of states in the current database (%d)." % (session_maxst))
        else:
            session.maxst = self.state
	
            if session.curst > session.maxst:
                GRIZ_set_state(session.maxst)
	     #end if
        #end else
    #end def

class stride(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)
        self.args = args
    def do(self):
        if statecount < 1:
            w = S.WarningMsg("Stride must be a positive integer.")
        elif statecount >= session.maxst:
            w = S.WarningMsg("Stride must be less than the number of states in the current database (%d)." % (DBStateCount))
        else:
            session.cur_stride = statecount
        #end else
    #end def

class state(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)
        self.args = args
    def do(self):
        if state < session.minst:
            w = S.WarningMsg("Current state is %d; minimum possible state is %d." % (session.curst, session.minst))
        elif state > session.maxst:
            w = S.WarningMsg("Current state is %d; maximum possible state is %d." % (session.curst, session.maxst))
        elif session.ViewCenteringOn:
            DisableRedraw()

            visit.SetTimeSliderState(state-1)
            visit.RecenterView()
	
            visit.RedrawWindow()
	
            session.cur_state = statenumber
        else:
            visit.SetTimeSliderState(state-1)

            visit.curView = GetView3D()
            DefaultViewCenter = curView.focus
	
            session.cur_stat = statenumber
        #end else
    #end def

class time(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)
        time = args[0]
    def do(self):
        state = -1
        # Check for valid time
        
#        for i in range(1, session.maxst):
#              if ( print "\nTimes=", session.state_times[i-1]
            
        if state < session.minst:
            w = S.WarningMsg("Current state is %d; minimum possible state is %d." % (session.curst, session.minst))
        elif state > session.maxst:
            w = S.WarningMsg("Current state is %d; maximum possible state is %d." % (session.curst, session.maxst))
        elif session.ViewCenteringOn:
            DisableRedraw()

            visit.SetTimeSliderState(state-1)
            visit.RecenterView()
	
            visit.RedrawWindow()
	
            session.cur_state = statenumber
        else:
            visit.SetTimeSliderState(state-1)

            visit.curView = GetView3D()
            DefaultViewCenter = curView.focus
	
            session.cur_stat = statenumber
        #end else
    #end def

class lts(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
    def do(self):
        
        visit.Query("Time")
        time_array = visit.GetQueryOutputValue() # I think this just gives you the current state

        self.message("\n\n Database state times:" +"\n State Num \t State Time \t Delta Time"
                   + "\n ---------------\t ---------------\t ---------------")

        self.message(str(time_array))

class AnimHelper(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
        session.ViewCenteringOn = session.ViewCenteringOn

    def stateNumber(self, statenumber):
        if session.ViewCenteringOn:
            visit.DisableRedraw()
            visit.SetTimeSliderState(statenumber)
            visit.RecenterView()

            visit.RedrawWindow()

            session.curst = statenumber
        else:
            visit.SetTimeSliderState(statenumber)

            curView = visit.GetView3D()
            DefaultViewCenter = curView.focus

            session.curst = statenumber

class AnimThread(Thread):
    """ Does the animation work as a separate thread so that Griz can still be
        responsive while it is animating.
    """
    def __init__(self, animMinst, animMaxst):
        Thread.__init__(self)

        self.animMinst = animMinst
        self.animMaxst = animMaxst
        self.AnimHelper = AnimHelper()

        self.currentState = animMinst
        self.animating = False

    def run(self):
        self.animating = True
        for i in range(self.animMinst, self.animMaxst+1, session.stride):
            try:
                if self.animating:
                    self.AnimHelper.stateNumber(i)
                    self.currentState = i
                    sleep(1)
            except:
                #Some error prevented it from continuing with i, so i must be beyond the scope of the possible states
                w = S.WarningMsg("Possibly some error in animate max and min states")
                session.updateVar(session.maxst, i-1)

    def pause(self):
        self.animating = False
        

    def cont(self): # continues the thread
        self.animating = True
        self.animMinst = self.currentState
        self.run()
        


class anim(S.GrizFunction):
    """ Animate states in database. Has optional number of args: n, t_s, t_e.

        anim           -- Animate states in database
        anim n         -- Animate between 'f' and 'l' with interpolation
        anim n t_s t_e -- Animate between t_s and t_e with interpolation
    """

    def __init__(self, *args):
        S.GrizFunction.__init__(self)
        # These variables come from Env. DBStateCount is populated in load_metadata
        self.minst = session.minst
        self.maxst = session.maxst # or session.DBStateCount
        self.CurStride = session.curstride

        self.animMinst = session.minst
        self.animMaxst = session.maxst
       
        if len(args) > 0:
            # Animate between states with interpolation
            self.CurStride = args[0]
        if len(args) == 3:
            # Animate between t_s and t_e with interpolation
            self.animMinst = args[1]
            self.animMaxst = args[2]
            
    def do(self):

        session.currentAnimThread = AnimThread(self.animMinst, self.animMaxst)
        session.currentAnimThread.start()

    #end def


class anim_frames(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)
        self.args = args
        for i in range(visit.TimeSliderGetNStates()):
            visit.SetTimeSliderState(i)
            session.curst = session.curst+1
            if ( session.animStop==True ):
                 session.animStop=False
                 break
                 
                       
class anim_interp(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)
        self.args = args

class stopan(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
    def do(self):
        session.currentAnimThread.pause()

class animc(S.GrizFunction):
    def __init__(self):
        if session.currentAnimThread: session.currentAnimThread.cont()
        else:
            f = anim()
            f.do()

class autoimg(S.GrizFunction):
    def __init__(self, root_name, file_type):
        S.GrizFunction.__init__(self)
        self.root_name = root_name
        self.file_type = file_type

class resetimg(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class state_changers(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
        self.set_state = SetState()

class f(state_changers):
    def __init__(self):
        state_changers.__init__(self)
    def do(self):
        self.set_state.set(1)
    #end def

class l(state_changers):
    def __init__(self):
        state_changers.__init__(self)
    def do(self):
        self.set_state.set(session.maxst)
    #end def

class n(state_changers):
    def __init__(self):
        state_changers.__init__(self)
    def do(self):
        self.set_state.set(session.curst+1)
    #end def

class p(state_changers):
    def __init__(self):
        state_changers.__init__(self)
    def do(self):
        self.set_state.set(session.curst-1)
    #end def

class SetState(object):
    def __init__(self):
        pass
    def set(self, statenumber):
        if statenumber < session.minst:
            w = S.WarningMsg("Current state is %d; minimum possible state is %d." % (session.curst, session.minst))
        elif statenumber > session.maxst:
            w = S.WarningMsg("Current state is %d; maximum possible state is %d." % (session.curst, session.maxst))
        elif session.ViewCenteringOn:
            DisableRedraw()

            visit.SetTimeSliderState(statenumber)
            visit.RecenterView()
	
            visit.RedrawWindow()
	
            session.curst = statenumber
        else:
            visit.SetTimeSliderState(statenumber)

            curView = visit.GetView3D()
            DefaultViewCenter = curView.focus
            session.curst = statenumber
        #end else
    #end def


###################
# On/Off Functions
###################

class OnOffAutoimg(S.OnOffFunction):
    """ Turn on (off) free node rendering mode"""
    ## No visible change in Griz. Will create flag.
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
    def on(self):
        session.OnOff_autoimg=True
    def off(self):
        session.OnOff_autoimg=False

