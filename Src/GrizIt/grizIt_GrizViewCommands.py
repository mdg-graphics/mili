############################################################################
# grizIt_GrizViewCommands.py - Griz Functions - View Commands.
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
import os, sys, re, bisect, math, time
import grizIt_VisitInfo as VI
import visit

from grizIt_ParseGrizCommand import *
import grizIt_Mili           as MILI

import grizIt_Session as GS

session = GS.session

############################################################################################

#############################
# View Commands
#############################
class rx(GS.GrizFunction):
    def __init__(self, args):
        self.args = args
        self.angle = float(self.args[0])
        d = GS.DebugMsg("Angle=", self.angle)

    def do(self):
        if session.CurDatabase == "":
           w = session.WarningMsg("No database is currently loaded.")
        else:
            rotRadians = self.angle/180.0 * math.pi
            rotCos = math.cos(rotRadians)
            rotSin = math.sin(rotRadians)

            view3D = visit.GetView3D()
            
            yNormal = view3D.viewNormal[1]
            zNormal = view3D.viewNormal[2]
            newYNormal =  rotCos*yNormal + rotSin*zNormal
            newZNormal = -rotSin*yNormal + rotCos*zNormal

            yViewUp = view3D.viewUp[1]
            zViewUp = view3D.viewUp[2]
            newYViewUp =  rotCos*yViewUp + rotSin*zViewUp
            newZViewUp = -rotSin*yViewUp + rotCos*zViewUp

            view3D.SetViewNormal(view3D.viewNormal[0], newYNormal, newZNormal)
            view3D.SetViewUp    (view3D.viewUp[0],     newYViewUp, newZViewUp)

            visit.SetView3D(view3D)
        #end else
    #end def

class ry(GS.GrizFunction):
    def __init__(self, args):
        self.args = args
        angle = ' '.join(self.args)
        self.angle = float(angle)
    def do(self):
        if session.CurDatabase == "":
           w = session.WarningMsg("No database is currently loaded-1.")
        else:
           rotRadians = self.angle/180.0 * math.pi
           rotCos = math.cos(rotRadians)
           rotSin = math.sin(rotRadians)
           
           view3D = visit.GetView3D()
           
           xNormal = view3D.viewNormal[0]
           zNormal = view3D.viewNormal[2]
           newXNormal = rotCos*xNormal - rotSin*zNormal
           newZNormal = rotSin*xNormal + rotCos*zNormal
           
           xViewUp = view3D.viewUp[0]
           zViewUp = view3D.viewUp[2]
           newXViewUp = rotCos*xViewUp - rotSin*zViewUp
           newZViewUp = rotSin*xViewUp + rotCos*zViewUp
           
           view3D.SetViewNormal(newXNormal, view3D.viewNormal[1], newZNormal)
           view3D.SetViewUp    (newXViewUp, view3D.viewUp[1],     newZViewUp)
           
           visit.SetView3D(view3D)
    #end def


class rz(GS.GrizFunction):
    def __init__(self, args):
        self.args = args
        angle = ' '.join(self.args)
        self.angle = float(angle)
    def do(self):
        if session.CurDatabase == "":
           w = session.WarningMsg("No database is currently loaded-2.")
        else:
           rotRadians = self.angle/180.0 * math.pi
           rotCos = math.cos(rotRadians)
           rotSin = math.sin(rotRadians)
           
           view3D = visit.GetView3D()
           
           xNormal = view3D.viewNormal[0]
           yNormal = view3D.viewNormal[1]
           newXNormal =  rotCos*xNormal + rotSin*yNormal
           newYNormal = -rotSin*xNormal + rotCos*yNormal
           
           xViewUp = view3D.viewUp[0]
           yViewUp = view3D.viewUp[1]
           newXViewUp =  rotCos*xViewUp + rotSin*yViewUp
           newYViewUp = -rotSin*xViewUp + rotCos*yViewUp
           
           view3D.SetViewNormal(newXNormal, newYNormal, view3D.viewNormal[2])
           view3D.SetViewUp    (newXViewUp, newYViewUp, view3D.viewUp[2]    )
           
           visit.SetView3D(view3D)
    #end def

#
# Translate View in X, Y, Z
#
class tx(GS.GrizFunction):
    def __init__(self, args):
        self.args = args
        distance = ' '.join(self.args)
        self.distance = -float(distance)
        
    def do(self):
        if (self.distance==0.0):
            return
        if session.CurDatabase == "":
           w = session.WarningMsg("No database is currently loaded.-3")
        else:
           view3D = visit.GetView3D()

           focusX = view3D.focus[0]+self.distance
           focusY = view3D.focus[1]
           focusZ = view3D.focus[2]
           view3D.SetFocus(focusX, focusY, focusZ)
           visit.SetView3D(view3D)
    #end def

class ty(GS.GrizFunction):
    def __init__(self, args):
        self.args = args
        distance = ' '.join(self.args)
        self.distance = -float(distance)
        
    def do(self):
        if (self.distance==0.0):
            return
        if session.CurDatabase == "":
           w = session.WarningMsg("No database is currently loaded.")
        else:
           view3D = visit.GetView3D()

           focusX = view3D.focus[0]
           focusY = view3D.focus[1]+self.distance
           focusZ = view3D.focus[2]
           view3D.SetFocus(focusX, focusY, focusZ)
           visit.SetView3D(view3D)
    #end def

class tz(GS.GrizFunction):
    def __init__(self, args):
        self.args = args
        distance = ' '.join(self.args)
        self.distance = -float(distance)
        
    def do(self):
        if (self.distance==0.0):
            return
        if session.CurDatabase == "":
           w = session.WarningMsg("No database is currently loaded.")
        else:
           view3D = visit.GetView3D()

           focusX = view3D.focus[0]
           focusY = view3D.focus[1]
           focusZ = view3D.focus[2]+self.distance
           view3D.SetFocus(focusX, focusY, focusZ)
           visit.SetView3D(view3D)
    #end def


class rview(GS.GrizFunction):
    def __init__(self):
        pass
    def do(self):
        visit.ResetView()
        
    #end def

class rnf(GS.GrizFunction):
    def __init__(self):
        GS.GrizFunction.__init__(self)
    # def do(self):
    #end def
    
class scale(GS.GrizFunction):
    def __init__(self, args):
        self.args = args
        factor = ' '.join(self.args)
        self.factor = float(factor)
    def do(self):
        
        ## Get the current view
        
        view3D = visit.GetView3D()
        
        zoomFactor = view3D.imageZoom
        zoomFactor = zoomFactor*self.factor
        view3D.imageZoom = zoomFactor
        visit.SetView3D(view3D)
    #end def

class scalax(GS.GrizFunction):
    def __init__(self, args):
        self.args = args
        arg_list = ' '.join(self.args)
        self.arg_list = arg_list.split()
    def do(self):
        vx = float(self.arg_list[0])
        vy = float(self.arg_list[1])
        vz = float(self.arg_list[2])

        visit.Query("SpatialExtents", 1, 0, "default")
        extents = visit.GetQueryOutputValue()
        print "\nExtents=", extents
        
        visit.AddOperator("Transform", 1)
        status = visit.AddOperator("Transform", 1)
            
        transformAtts = visit.TransformAttributes()       

        visit.SetOperatorOptions(transformAtts, 1)
        visit.RemoveLastOperator()

        ## Get the current view
        if GS.TransOpDefined:
            visit.AddOperator("Transform", 1)
            GS.TransOpDefined = True
            status = visit.AddOperator("Transform", 1)
            
            
        transformAtts = visit.TransformAttributes()       
        transformAtts.doScale = 1
        transformAtts.scaleX = vx
        transformAtts.scaleY = vy
        transformAtts.scaleZ = vz
        visit.SetOperatorOptions(transformAtts, 1)
        
    #end def

class vcent(GS.GrizFunction):
    def __init__(self, args):
        self.args = args
        x = float(x)
        y = float(y)
        z = float(z)
    #end def

def GRIZ_vcent_x_y_z(x, y, z):
    global ViewCenteringOn, GenericCentering

    x = float(x)
    y = float(y)
    z = float(z)
    
    view3D = GetView3D()
    view3D.SetFocus(x, y, z)
    SetView3D(view3D)

    ViewCenteringOn  = True
    GenericCentering = False
#end def


def GRIZ_vcent_N_nodenumber(nodenumber):
    global ViewCenteringOn, GenericCentering

    
    if nodenumber < 0:
	w = session.WarningMsg("Node number is negative.")
    elif CurNodeCount == 0:
	w = session.WarningMsg("Node numbers in the currently displayed mesh are unknown.")
    elif nodenumber >= CurNodeCount:
	w = session.WarningMsg("Node number equals or exceeds number of nodes in mesh currently displayed.")
    else:
	Query("Node Coords", nodenumber)
	nodeCoords = GetQueryOutputValue()
    
	view3D = GetView3D()
	view3D.SetFocus(nodeCoords[0], nodeCoords[1], nodeCoords[2])
	SetView3D(view3D)

	ViewCenteringOn  = True
	GenericCentering = False
    #end else
#end def


def GRIZ_vcent_onoroff(onoroff):
    global ViewCenteringOn, GenericCentering
    
    if (onoroff == "on") or (onoroff == "ON"):
	RecenterView()
	
	ViewCenteringOn  = True
	GenericCentering = True
    elif (onoroff == "off") or (onoroff == "OFF"):
	view3D = GetView3D()
	view3D.SetFocus(DefaultViewCenter[0], DefaultViewCenter[1], DefaultViewCenter[2])
	SetView3D(view3D)
	
	RecenterView()
	
	ViewCenteringOn = False
    elif (onoroff == "hi") or (onoroff == "HI"):
	w = session.WarningMsg("'vcent hi' is not yet available in GrizIt.  Sorry.")
    else:
	w = session.WarningMsg("A single parameter following 'vcent' must be 'on', 'off', or 'hi'.")
    #end else
#end def

#
# Zoom Commands
#
class zb(GS.GrizFunction):
    def __init__(self, args):
        self.args = args
        factor = ' '.join(self.args)
        self.factor = float(factor)
    def do(self):
        
        ## Get the current view
        
        view3D = visit.GetView3D()

        zoomFactor = view3D.imageZoom
        zoomFactor = zoomFactor/self.factor
        view3D.imageZoom = zoomFactor
        
        visit.SetView3D(view3D)
        
    #end def

class zf(GS.GrizFunction):
    def __init__(self, args):
        self.args = args
        factor = ' '.join(self.args)
        self.factor = float(factor)
    def do(self):
        ## Get the current view
        
        view3D = visit.GetView3D()

        zoomFactor = view3D.imageZoom
        zoomFactor = zoomFactor*self.factor
        view3D.imageZoom = zoomFactor
        
        visit.SetView3D(view3D)
    #end def


#############
# Test Driver
#############
if __name__ == "__main__":
    INIT.Initialize_GrizIt_Session(session)
    session.miliDB = MILI.Mili()    

    visit.AddArgument("-v "+VI.visitVersion)
    visit.AddArgument("-nosplash")
    visit.Launch()
    visit.OpenGUI()
    time.sleep(2)
    
    filename = raw_input("\nEnter Visit filename to process:")
    print "\nOpening file: ", filename
    visit.OpenDatabase(filename)
    time.sleep(2)
    cmd = "load "+filename
    ParseCommand(cmd)
    ParseCommand("rx 20")

    testing = False
    i=0
    while testing:
     i=i+1    
    


