#!/usr/bin/env python
#
############################################################################
# grizIt_GuiCB.py - Griz GUI Callback (CB) Functions.
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      August 23, 2010
#
# 
############################################################################
# Modifications:
#  I. R. Corey - August 23, 2010: Created.
#
############################################################################
#
import os, sys, re, bisect, math, copy
import sys, os, datetime
sys.path.append("/usr/local/lib/python2.6/site-packages")

from PySide.QtCore import *
from PySide.QtGui import *
from PySide import *

from grizIt_Main import *
import grizIt_ParseGrizCommand as PC
import grizIt_Session as S

import grizIt_GuiUtil          as UTIL
import grizIt_GuiMaterialMgr   as MATL
# import grizIt_GuiPlot          as PLT

session = S.session
env     = S.env

#-----------------------------------------------------------------------------
# Menu Callbacks
#-----------------------------------------------------------------------------
# 

# File Menu Callbacks

def openFile_CB():
        filename = QtGui.QFileDialog.getOpenFileName(None, 'Open file',
                   "")
	cmd = " "
	cmd = "load " + str(filename)

        PC.ParseCommand(str(cmd))

###########################
# Control Menu Callbacks
###########################

def Copyright_CB():
        PC.ParseCommand("copyrt")	

def UtilityPanel_CB():
	env.utilWin = UTIL.UtilWindow()

def MaterialPanel_CB():
	env.mtlWin  = MATL.MaterialMegPanel()

def SaveSessionGlobal_CB():
        PC.ParseCommand(str("savesg"))	

def SaveSessionLocal_CB():
        PC.ParseCommand(str("savesl"))	

def LoadSessionGlobal_CB():
        PC.ParseCommand(str("loadsg"))	

def LoadSessionLocal_CB():
        PC.ParseCommand(str("loadsl"))	

def Quit_CB():
        PC.ParseCommand("quit")	

###########################
# Rendering Menu Callbacks
###########################

def DrawSolid_CB():
        PC.ParseCommand(str("sw solid"))

def DrawHidden_CB():
        PC.ParseCommand(str("sw hidden"))

def DrawWireframe_CB():
        d = S.DebugMsg("DrawWireframe...............\n\n")

def CoordSystem_CB():
	if session.show_coord:
	   PC.ParseCommand(str("off coord"))
	else:
           PC.ParseCommand(str("on coord"))
        #end if
	
def Title_CB():
	if session.show_title:
	   PC.ParseCommand(str("off title"))
	else:
           PC.ParseCommand(str("on title"))
        #end if
                               
def Time_CB():
	if session.show_time:
	   PC.ParseCommand(str("off time"))
	else:
           PC.ParseCommand(str("on time"))
        #end if

def Colormap_CB():
	if session.show_cmap:
	   PC.ParseCommand(str("off cmap"))
	else:
           PC.ParseCommand(str("on cmap"))
        #end if

def MinMax_CB():
	if session.show_minmax:
	   PC.ParseCommand(str("off minmax"))
	else:
           PC.ParseCommand(str("on minmax"))
        #end if

def DispScale_CB():
	if session.show_scale:
	   PC.ParseCommand(str("off scale"))
	else:
           PC.ParseCommand(str("on scale"))
        #end if

def EI_CB():
	if session.OnOff_ei_mode:
	   PC.ParseCommand(str("off ei"))
	else:
           PC.ParseCommand(str("on ei"))
        #end if

def All_CB():
	if session.show_all:
	   PC.ParseCommand(str("off all"))
	else:
           PC.ParseCommand(str("on all"))
        #end if

def Bbox_CB():
	if session.show_bbox:
	   PC.ParseCommand(str("off box"))
	else:
           PC.ParseCommand(str("on box"))
        #end if

def Edges_CB():
	if session.OnOff_edges:
	   PC.ParseCommand(str("off edges"))
	else:
           PC.ParseCommand(str("on edges"))
        #end if

def GS_CB():
	if session.OnOff_gs_mode:
	   PC.ParseCommand(str("off gs"))
	else:
           PC.ParseCommand(str("on gs"))
        #end if

def Persp_CB():
        PC.ParseCommand(str("switch presp"))
        #end if

def Ortho_CB():
        PC.ParseCommand(str("switch presp"))
        #end if

def Rnf_CB():
        PC.ParseCommand(str("rnf"))
        #end if


def SetCm_CB(MapName):
        PC.ParseCommand(MapName)

def LoadCm_CB(mapName):
        PC.ParseCommand("ldmap"+mapName)

def ResetView_CB():
        PC.ParseCommand("rview" )

#########################
# Picking Menu Callbacks
#########################

def MouseHilite_CB():
        PC.ParseCommand(str("switch pichil"))
	                                        
def MouseSelect_CB():
        PC.ParseCommand(str("switch picsel"))

def ClrHilite_CB():
        PC.ParseCommand(str("switch pichil"))
	                                        
def ClrSelect_CB():
        PC.ParseCommand(str("switch picsel"))

def SetBtn1Pick_CB(className):
        PC.ParseCommand(str("setpick 1 "+className))

def SetBtn2Pick_CB(className):
        PC.ParseCommand(str("setpick 2 "+className))

def SetBtn3Pick_CB(className):
        PC.ParseCommand(str("setpick 2 "+className))

def CtrHiliteOn_CB():
        PC.ParseCommand(str("vcent hi"))
        
def CtrHiliteOff_CB():
        PC.ParseCommand(str("vcent off"))


#########################
# Derived Callbacks
#########################

def Show_CB(varName):
        PC.ParseCommand(str("show "+varName))

def ResultOff_CB():
        PC.ParseCommand('show mat')

#########################
# Primal Callbacks
#########################

# Same functions as Derived (Show and ResultOff

#########################
# Time Callbacks
#########################

def NextState_CB():
        PC.ParseCommand('n')
def PreviousState_CB():
        PC.ParseCommand('p')
def FirstState_CB():
        PC.ParseCommand('f')
def LastState_CB():
        PC.ParseCommand('l')
def AnimStates_CB():
        PC.ParseCommand('anim')
def StopAnim_CB():
        PC.ParseCommand('stopan')
def ContAnim_CB():
        PC.ParseCommand('animc')
	    	    	    	    
#########################
# Plot Callbacks
#########################

def TH_CB():
        PC.ParseCommand('timhis')
	    	    	    	    
#########################
# Help Callbacks
#########################

def Help_CB():
        PC.ParseCommand('help')
	    	    	    	    
