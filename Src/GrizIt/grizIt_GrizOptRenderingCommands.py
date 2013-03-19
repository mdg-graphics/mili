############################################################################
# grizIt_GrizOptRenderingCommands.py - Griz Functions - Optional Rendering
#      commands.
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      June 10, 2010
#
# 
############################################################################
# Modifications:
#  I. R. Corey - June 10, 2010: Created.
#  S. Drakeley - June 30, 2010: 
#
############################################################################
#

import pdb
import os, sys, re, bisect, math

import visit
import grizIt_Main
import grizIt_Session as S
import grizIt_VisitInfo as VISITINFO

session = S.session

############################################################################################


##############################
# Optional Rendering Commands
##############################

class GRIZ_numclass(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)
    

import grizIt_GrizMeshControlCommands 


class PlotHandler(object):
    def __init__(self):
        self.meshController = grizIt_GrizMeshControlCommands.AttributeHandler()

    def getMaterialsName(self):
        plotlist = visit.GetPlotList()
        materials_name = plotlist.GetPlots(0).plotVar # What shoudl the GetPlots
        #                                               index be if multiple
        #                                               plots? (not always 0?)
        return materials_name

    def getNumPlots(self):
        plotlist = visit.GetPlotList()
        numPlots = plotlist.GetNumPlots()
        return numPlots
        
class edgwid(S.GrizFunction):
    def __init__(self, width):
        ''' Set the width in pixels of mesh edges renderd when "on edges" is
            in effect. Available line width granularity and maximum width
            are OenGL implementation-dependent.

            width is a value from 0 to 9 (parser gives it a list)
        '''
        self.pHandler = PlotHandler()
        self.width = int(width[0]) # The way this thing is parsing, the value is always a list
        # Follow up: is the 0 to 9 value a pixel value?
    def do(self):
        materials_name = self.pHandler.getMaterialsName()
        numPlots = self.pHandler.getNumPlots()
        if numPlots >= 2 and self.width < 10 and self.width >= 0: #need to know how to determine that last plot is
            #               the edges plot!!
            visit.SetActivePlots(numPlots-1) #This should be more specific to
            #                                   the actual edges plot
            FBA = visit.FilledBoundaryAttributes()
            self.pHandler.meshController.attributes[3]=(FBA)
            FBA.lineWidth=self.width
            visit.SetPlotOptions(FBA)
            visit.DrawPlots()
            # need to write some error catching here # TO DO
        elif numPlots <2:
            p = S.PrintMsg('Edges are not turned on. Use "on edges" to turn on edges.')
        else:
            w = S.WarningMsg('width value must be an integer from 0 to 9')
            
            
        

#######################################################################

class OnOffAll(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
        self.annotationAtts = visit.GetAnnotationAttributes()
    #end def
    def on(self):
        self.annotationAtts.axes3D.bboxFlag  = 1
        self.annotationAtts.legendInfoFlag   = 1
        self.annotationAtts.axes3D.visible   = 1
        self.annotationAtts.databaseInfoFlag = 1
        self.annotationAtts.axes3D.triadFlag = 1
        self.annotationAtts.userInfoFlag     = 1
            
        S.session.show_triad  = True
        S.session.show_bbox   = True
        S.session.show_triad  = True
        S.session.show_title  = True
        S.session.show_user   = True        
        S.session.show_coord  = True
        S.session.show_legend = True

        visit.SetAnnotationAttributes(self.annotationAtts)
        
    def off(self):
        self.annotationAtts.axes3D.bboxFlag  = 0
        self.annotationAtts.legendInfoFlag   = 0
        self.annotationAtts.axes3D.visible   = 0
        self.annotationAtts.databaseInfoFlag = 0
        self.annotationAtts.axes3D.triadFlag = 0
        self.annotationAtts.userInfoFlag     = 0

        S.session.show_triad  = False
        S.session.show_bbox   = False
        S.session.show_triad  = False
        S.session.show_title  = False
        S.session.show_user   = False        
        S.session.show_coord  = False
        S.session.show_legend = False
        
        visit.SetAnnotationAttributes(self.annotationAtts)
    #end def
#end class

class OnOffBgimage(S.OnOffFunction):
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

class OnOffBox(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
        self.annotationAtts = visit.GetAnnotationAttributes()
    #end def
    def on(self):
        self.annotationAtts.axes3D.bboxFlag = 1
        S.session.show_bbox = True

        visit.SetAnnotationAttributes(self.annotationAtts)
        
    def off(self):
        self.annotationAtts.axes3D.bboxFlag = 0
        S.session.show_coord = False
        
        visit.SetAnnotationAttributes(self.annotationAtts)
    #end def
#end class

class OnOffCmap(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
        self.annotationAtts = visit.GetAnnotationAttributes()
    #end def
    def on(self):
        self.annotationAtts.legendInfoFlag = 1
        LegendOn = True
        visit.SetAnnotationAttributes(self.annotationAtts)
    def off(self):
        self.annotationAtts.legendInfoFlag = 0
        S.session.show_legend = False
        visit.SetAnnotationAttributes(self.annotationAtts)
    #end def
#end class

class OnOffCoord(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
        self.annotationAtts = visit.GetAnnotationAttributes()
    #end def
    def on(self):
        self.annotationAtts.axes3D.visible = 1
        CoordAxesOn = True
        visit.SetAnnotationAttributes(self.annotationAtts)
    def off(self):
        self.annotationAtts.axes3D.visible = 0
        S.session.show_coord = False
        visit.SetAnnotationAttributes(self.annotationAtts)
    #end def
#end class

class OnOffCscale(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.annotationAtts = visit.GetAnnotationAttributes()        
        self.flag = flag
    #end def
    def on(self):
        pass #TO DO
        #end if
    def off(self):
        pass #TO DO
    #end def
#end class

class OnOffDscal(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
        self.annotationAtts = visit.GetAnnotationAttributes()
    #end def
    def on(self):
        pass #TO DO

    def off(self):
        pass #TO DO
    #end def
#end class

class OnOffLocref(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
        self.annotationAtts = visit.GetAnnotationAttributes()
    #end def
    def on(self):
        pass # TO DO
        
    def off(self):
        pass #TO DO
    #end def
#end class

class OnOffMinmax(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
        self.annotationAtts = visit.GetAnnotationAttributes()
    #end def
    def on(self):
        pass # TO DO
    def off(self):
        pass #TO DO
#end class

class OnOffNum(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
        self.annotationAtts = visit.GetAnnotationAttributes()
    #end def
    def on(self):
        pass # TO DO
    def off(self):
        pass # TO DO
    #end def
#end class

class OnOffTime(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
        self.annotationAtts = visit.GetAnnotationAttributes()
    #end def
    def on(self):
        pass
    def off(self):
        pass
    #end def
#end class

class OnOffTitle(S.OnOffFunction):
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
        self.annotationAtts = visit.GetAnnotationAttributes()
    #end def
    def on(self):
        self.annotationAtts.databaseInfoFlag = 1
        TitleOn = True
        visit.SetAnnotationAttributes(self.annotationAtts)
    def off(self):
        self.annotationAtts.databaseInfoFlag = 0
        S.session.show_title = False
        visit.SetAnnotationAttributes(self.annotationAtts)
    #end def
#end class

class OnOffEdges(S.OnOffFunction):
    def __init__(self, flag):
        ''' Turn on (off) mesh edges '''
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag # this is a potential for bugs. But it gives a list
        self.pHandler = PlotHandler()

    def on(self):
        materials_name = self.pHandler.getMaterialsName()
        numPlots = self.pHandler.getNumPlots()

        visit.AddPlot("FilledBoundary", materials_name, 1, 1)
        edgesPlotNum = visit.GetNumPlots()
        session.edgesPlot = edgesPlotNum-1 #On a scale with 0 as the first
        visit.SetActivePlots(session.edgesPlot)

        FBA = visit.FilledBoundaryAttributes()
        self.pHandler.meshController.attributes[3]=(FBA)
        
        # This is so wrong. I have to fix this issue :(
            # The problem with this is that the list would then have to change
            # every time you added a plot, and that just doesn't seem right.
            # Maybe a solution is in the GetPlotList() function. 
        for k in range(0, len(FBA.boundaryNames)+1):
            self.pHandler.meshController.changeAttributeColor(k, (0, 0, 0, 255), FBA)
        FBA.lineWidth = 5
        FBA.wireframe = 1
        visit.SetPlotOptions(FBA)
        visit.DrawPlots()

            
    def off(self):
        if session.edgesPlot: #edgesPlot is None if edges off
            visit.SetActivePlots(session.edgesPlot)
            visit.DeleteActivePlots()

            session.edgesPlot = None
