#!/usr/gapps/visit/python/2.6.4/linux-x86_64_gcc-4.1/bin/python
#
############################################################################
# grizIt_VisitPlots.py - Classes to manage Vist Plots and Windows.
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      September 17, 2010
#
# 
############################################################################
# Modifications:
#  I. R. Corey - August 17, 2010: Created.
#
############################################################################
#
import pdb, sys

import os, sys, re, bisect, math, copy
import visit
import grizIt_VisitInfo
import grizIt_Session as S

#-------------------------------------------------------------------------
# Visit Window Management Class
#-------------------------------------------------------------------------

class VisitWindows:
    def __init__(self):
        self.numWindows = 0
        self.edgesPlot = None # should be int, 0 is first
        self.Visit_PlotList = None
        self.Grizit_NumPlots = 0
        self.Grizit_PlotList = None
        self.Grizit_PlotNames = []
        self.Grizit_PlotTypes = []
        self.Grizit_PlotIds   = []
    #end def

    #-------------------------------------------------------------------------
    # Window Management Functions
    #-------------------------------------------------------------------------
    def AddWindow(self):
        visit.AddWindow()
        self.numWindows = self.numWindows+1
    def DeleteWindow(self, num):
        visit.SetActiveWindow(num)
        visit.DeleteWindow()
        self.numWindows = self.numWindows-1
    #end def
    
    def LockView(self, num):
        lock = visit.GetWindowInformation().lockView
        if lock==0:
           visit.ToggleLockViewMode()
    #end def
    
    def UnlockView(self, num):
        lock = visit.GetWindowInformation().lockView
        if lock==1:
           visit.ToggleLockViewMode()
    #end def
#end class

#-------------------------------------------------------------------------
# Visit Plot Management Class
#-------------------------------------------------------------------------

class VisitPlots:
    def __init__(self):
        self.Visit_NumPlots = 0
        self.edgesPlot = None # should be int, 0 is first
        self.Visit_PlotList = None
        self.Grizit_NumPlots = 0
        self.Grizit_PlotList = None
        self.Grizit_PlotNames = []
        self.Grizit_PlotTypes = []
        self.Grizit_PlotIds   = []
    #end def

    #-------------------------------------------------------------------------
    # Plot Management Functions
    #-------------------------------------------------------------------------
    def Visit_UpdatePlotList(self):
        """ This function maintains the variables within the while loop of main that does the parsing."""
#        self.Visit_NumPlots = visit.GetNumPlots()
#        self.Visit_PlotList = visit.GetPlotList()

    def Visit_PlotListSearch(self, name ):
        for i in range(self.NumPlots ):
            if ( self.PlotList.GetPlots(i).plotVar==name ):
                 return(True)
        return(False)
    #end def
    def Visit_GetActivePlotId(self, name ):
        for i in range(self.Visit_NumPlots ):
            if ( self.PlotList.GetPlots(i).plotVar==name and self.PlotList.GetPlots(i).activeFlag==1 ):
                 return(self.PlotList.GetPlots(i).id)
        return(-1)
    #end def
      
    def Grizit_AddPlot( self, name, unique ):
        return(False)
        plotId = Visit_GetActivePlotId(name)
        
    #end def    

    def Grizit_DeletePlot( self, name ):
        d = S.DebugMsg("\nIn DeletePlots")
        if name=="*":
           for i in range(self.NumPlots ):
               plotId = self.Grizit_PlotIds[i]
               visit.SetActivePlots(plotId)
               visit.DeleteActivePlots()
               visit.DrawPlots()
        else:
               index  = self.Grizit_PlotIds.index(name)
               plotId = self.Grizit_PlotIds[i]
               visit.SetActivePlots(plotId)
               visit.DeleteActivePlots()
               visit.DrawPlots()
     #end def    
    
    def Grizit_HidePlot( self, name ):
        return(False)
    #end def    

    def Grizit_ShowPlot( self, name ):
        return(False)
    #end def    
#end class

if __name__ == "__main__":
    pass
