#!/usr/bin/python
############################################################################
# grizIt_Init.py - Init
#      commands.
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      June 10, 2010
#
############################################################################
# Modifications:
#  I. R. Corey - June 10, 2010: Created.
#
############################################################################
#

import pdb

import os, sys, re, bisect, math, copy
import grizIt_VisitInfo

from grizIt_Main import *
from grizIt_Env import *
from grizIt_GrizViewCommands import *
from grizIt_GuiCB import *
from grizIt_VisitCB import *

def Initialize_GrizIt_Session(session):
    visit.SuppressMessages(1)
    visit.SuppressQueryOutputOn()

    VisItMessagesOn = False
    
    MatModeOn = True

    ViewCenteringOn  = False
    GenericCentering = False
    
    annotAtts = visit.GetAnnotationAttributes()
    annotAtts.axesFlag         = 0
    annotAtts.legendInfoFlag   = 0
    annotAtts.bboxFlag         = 0
    annotAtts.databaseInfoFlag = 0
    annotAtts.triadFlag        = 1
    annotAtts.userInfoFlag     = 0
    visit.SetAnnotationAttributes(annotAtts)
    
    BoundingBoxOn = False
    LegendOn      = False
    CoordAxesOn   = False
    TitleOn       = False
    TriadOn       = True
    UserInfoOn    = False

    # Register all callback functions
    Initialize_GrizIt_Callbacks()


#end def
 
#-----------------------------------------------------------------------------
if __name__ == "__main__":
    Launch()
    Initialize_GrizIt()   
    
