############################################################################
# grizIt_GrizMeshControlCommands.py - Griz Functions - Mesh Control
# Commands.
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      May 07, 2010
#
# 
############################################################################
# Modifications:
#  I. R. Corey - May 18, 2010: Created.
#  S. Drakeley - Jun 29, 2010: Classes created and modified
#
############################################################################
#

import pdb
import os, sys, re, bisect, math

import visit
import grizIt_Main

import grizIt_Session as S
import grizIt_inputHelper as HelperInput
import grizIt_VisitInfo as VISITINFO


session = S.session
env     = S.env

############################################################################################


#########################
# Mesh Control Commands
#########################


class hilite(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)
        self.args = args
        name = self.args[0]
        n = int(self.args[1])
    
        
class clrhil(S.GrizFunction):
    def __init__(self, args):
        S.GrizFunction.__init__(self)
        self.args = args
        name = self.args[0]
        n = int(self.args[1])
        

class AttributeHandler(object):
    ''' A helper object that allows access to attribute types
    '''
    def __init__(self):
      
      self.attributes = [visit.BoundaryAttributes(),
                         visit.ContourAttributes(),
                         visit.CurveAttributes(),
                         visit.FilledBoundaryAttributes(),
                         visit.HistogramAttributes(),
                         visit.LabelAttributes(),
                         visit.MeshAttributes(),
                         visit.MoleculeAttributes(),
                         visit.ParallelCoordinatesAttributes(),
                         visit.PoincareAttributes(),
                         visit.PseudocolorAttributes(),
                         visit.ScatterAttributes(),
                         visit.SpreadsheetAttributes(),
                        # visit.StreamlineAttributes(),
                         visit.SubsetAttributes(),
                         visit.TensorAttributes(),
                         visit.TruecolorAttributes(),
                         visit.VectorAttributes(),
                         visit.VolumeAttributes(),

                         #Below this line were not thought to be needed, but
                         # I put them here for debugging

                         visit.AnimationAttributes(),
                         visit.AnnotationAttributes(),
                         visit.AxisAttributes(),
                         visit.BoundaryOpAttributes(),
                         visit.BoxAttributes(),
                         visit.ClipAttributes(),
                         visit.ColorAttribute(),
                         visit.ConeAttributes(),
                         visit.ConstructDDFAttributes(),
                         visit.CoordSwapAttributes(),
                         visit.CreateBondsAttributes(),
                         visit.CylinderAttributes(),
                         visit.DeferExpressionAttributes(),
                         visit.DisplaceAttributes(),
                         visit.DualMeshAttributes(),
                         visit.EdgeAttributes(),
                         visit.ElevateAttributes(),
                         visit.ExportDBAttributes(),
                         visit.ExternalSurfaceAttributes(),
                         visit.FontAttributes(),
                         
                         
                         # Lots of other "gets" that end in attributes. not included here
                         visit.GlobalAttributes(),
                         visit.IndexSelectAttributes(),
                         visit.InverseGhostZoneAttributes(),
                         visit.IsosurfaceAttributes(),
                         visit.IsovolumeAttributes(),
                         visit.LightAttributes(),
                         visit.LineoutAttributes(),
                      
                         visit.MaterialAttributes(),
                         visit.MeshManagementAttributes(),
                         visit.OnionPeelAttributes(),
                         visit.PersistentParticlesAttributes(),
                         visit.PrinterAttributes(),
                         visit.ProcessAttributes(),
                         visit.ProjectAttributes(),
                         visit.ReflectAttributes(),
                         visit.RenderingAttributes(),
                         visit.ReplicateAttributes(),
                         visit.ResampleAttributes(),
                         visit.ResetPickAttributes(),
                         visit.ReplicateAttributes(),
                         visit.ResampleAttributes(),
                         visit.RevolveAttributes(),
                         visit.SaveWindowAttributes(),
                         visit.ScatterAttributes(),
                         
                         # Lots of other Set functions here
                         
                         visit.SliceAttributes(),
                         visit.SmoothOperatorAttributes(),
                         visit.SphereSliceAttributes(),
                         visit.SpreadsheetAttributes(),
                         visit.StreamlineAttributes(),
                         visit.ThreeSliceAttributes(),
                         visit.ThresholdAttributes(),
                         visit.TransformAttributes(),
                         visit.TriangulateRegularPointsAttributes(),
                         
                         visit.TubeAttributes(),
                         
                         visit.View2DAttributes(),
                         visit.View3DAttributes(),
                         visit.ViewAttributes(),
                         visit.ViewAxisArrayAttributes(),
                         visit.ViewCurveAttributes(),
                         ]
      
    def getAttributeType(self):
        ''' Returns the class containing the attributes for a plot.
            e.g. FilledBoundaryAttributes()

            Returns attributeList, a list of all the current attribute types active in Visit,
            as determined by the plotlist.
        '''
        plotlist = visit.GetPlotList()

        # numPlots = self.getNumPlots()

        plots = []    
        morePlots = True
        plotIndex = 0
        while morePlots == True:
            try:
                plotType = plotlist.GetPlots(plotIndex).plotType # an integer value in[0,18]
                plots.append(plotType)
                
                plotIndex+=1
            except IndexError:
                morePlots = False

        # numPlots = len(plots)

        attributeList = []

        for plotType in plots:

            # There are 19 possible plot types, not all of which would necessary
            # call the enable function, but I listed some of hte important ones
            # as well as a bit of information about retrieiving color values from
            # the other ones. To see a list of possible color-retrieving functions,
            # type, for example, dir(VolumeAttributes())

            A = self.attributes[plotType]
            # 0=Boundary -> SetMultiColor
            # 1=Contour -> SetMultiColor
            # 2=Curve -> GetCurveColor, SetCurveColor
            # 3=FilledBoundary -> SetMultiColor
            # 4=Histogram -> GetColor, SetColor
            # 5=Label -> GetTextColor1, GetTextColor2, SetTextColor1, SetTextColor2
            # 6= Mesh -> GetOpacity, GetOpaqueColor, SetOpacity, SetOpaqueColor
            # 7=Molecule,
            # 8=ParralelCoordinates
            # 9=PoinCare
            # 10= Pseudocolor -> GetOpacity, SetOpacity, GetColorTableName, SetColorTableName
            # 11=Scatter -> GetSingleColor, SetSingleColor
            # 12=Spreadsheet -> GetColorTableName, GetTracerColor, SetColorTableName, SetTracerColor
            # 13=Streamline -> GetColoringVariable, GetOpacity, GetOpacityType,..
            # 14=Subset -> GetMulticolor, SetMulticolor
            # 15=Tensor -> GetTensorColor, GetColorByEigenvalues
            # 16=Truecolor -> GetOpacity
            # 17=Vector -> GetVectorColor
            # 18=Volume -> GetColorVarMin, GetColorVarMax...
                
            attributeList.append(A)

        return attributeList
        
   
    def changeAttributeColor(self, material, colorTup, attributeType):
        ''' Depending on the type of attribute (FilledBoundary etc), goes
            through and changes the color per material in materials_list

            materials = a single material number (int)
            colorTup = the tuple with rgb and opacity values (r, g, b, opacity)
                where r, g, b, and opacity range from 0-255
                ** Unfortunately, colorTup could also be an opacity value
                    in the case where an opacity value is desired, it just
                    takes the last value of the colorTup
                
            attributeType = FilledBoundary, Pseudocolor, etc. (see self.attributes)
        '''
       
        try:
            if self.attributes.index(attributeType) in [0, 1, 3, 14]:
                attributeType.SetMultiColor(material-1, colorTup)
            elif self.attributes.index(attributeType) in [6, 10, 13, 16]:
                attributeType.SetOpacity(material-1, colorTup[3])
            else: d = S.DebugMsg('The attribute requested ({0}) has not been accounted for.'.format(attributeType))
            visit.SetPlotOptions(attributeType)
        except ValueError:
            w = S.DebugMsg("Value Error: attribute Type = {0}".format(attributeType))
            
 
class TrueColorHandler(object):
    ''' These are commands which all involve the visibility of materials
    '''
    def __init__(self, attHandler):
        self.attHandler = attHandler
        self.inptHlpr = HelperInput.InputParser() # This is an object which contains some parsing capabilities. This may be overkill as we already have a powerful parser
        if session.CurDatabase == "": # There is a bug here!!
            self.silr = visit.SILRestriction()  #delete
            self.silr.SuspendCorrectnessChecking() #delete
        
        else:
            self.silr = visit.SILRestriction()
            self.silr.SuspendCorrectnessChecking()

    def getTrueColor(self, material_index):
        true_color = session.true_colors[material_index]
        return true_color

    def saveTrueColor(self, material, attributeType):
        ''' Stores the original color/opacity of the material in the
            true_colors dict
        '''
        currentTrueColors = session.true_colors
        try:
             
            if self.attHandler.attributes.index(attributeType) in [0, 1, 3, 14]:
                currentTrueColors[material] = attributeType.GetMultiColor()[material-1]
                env.updateVar(session.true_colors, currentTrueColors)
            elif self.attributes.index(attributeType) in [6, 10, 13, 16]:
                currentTrueColors[material] = attributeType.GetOpacity()[material-1]
                env.updateVar(session.true_colors, currentTrueColors)
            else: d = S.DebugMsg('Did not account for attribute type {0}'.format(attributeType))
        except ValueError:
            d = S.DebugMsg("The Attribute Type {0} is not in 'attributes'".format(attributeType))


#############################################################
# Mesh Control Commands
#############################################################

class vis(S.GrizFunction):
    def __init__(self, materials_list):
        """ Make specified materials visible
    
            materials_list: a list of material numbers that can be
            in the form of '[1 2 3-5 11]' or as a list [1, 2, 3, 4, 5, 11]
    
            * Also, do you need to return a list of what is visible or invisible? IT
                does not appear that is necessary...
        """
        self.attHandler = AttributeHandler()
        self.tcHandler = TrueColorHandler(self.attHandler)
        self.mats = materials_list

    def do(self):
        materials_list = self.tcHandler.inptHlpr.inputToList(self.mats)
        for k in materials_list:
            self.tcHandler.silr.TurnOnSet(k)
        self.tcHandler.silr.EnableCorrectnessChecking() # What is a better way to construct this?
        visit.SetPlotSILRestriction(self.tcHandler.silr ,1)

        #update environment materials
        for mat in materials_list:
            session.materials_visible[mat] = True

class invis(S.GrizFunction):
    '''Make specified materials invisible'''
    def __init__(self, materials_list):
        self.attHandler = AttributeHandler()
        self.tcHandler = TrueColorHandler(self.attHandler)
        self.mats = materials_list

    def do(self):
        materials_list = self.tcHandler.inptHlpr.inputToList(self.mats)
        for k in materials_list:
            self.tcHandler.silr.TurnOffSet(k)
        self.tcHandler.silr.EnableCorrectnessChecking() # What is a better way to construct this envelope?
        visit.SetPlotSILRestriction(self.tcHandler.silr ,1)

        #update environment materials
        for mat in materials_list:
            session.materials_visible[mat] = False
        

class enable(S.GrizFunction):
    def __init__(self, materials_list):
        ''' Enable result display on specified materials

            * Returns the list of materials that were enabled
        '''
        self.attHandler = AttributeHandler()
        self.tcHandler = TrueColorHandler(self.attHandler)
        self.mats = materials_list

    def do(self):
        materials_list = self.tcHandler.inptHlpr.inputToList(self.mats)
        attributeList = self.attHandler.getAttributeType()

        for A in attributeList:
            for k in materials_list:
                if k in session.true_colors.keys():
                    colorTup = session.true_colors[k]
                    self.attHandler.changeAttributeColor(k, colorTup, A)
                else: p = S.WarningMsg('Material'+str(k)+'is already enabled')
        visit.DrawPlots()

        #update environment materials
        for mat in materials_list:
            session.enabled_mats[mat] = True
            
    
class disable(S.GrizFunction):
    def __init__(self, materials_list):
        ''' Disable result display on specified materials

            * Returns the list of materials that were disabled
        '''
        self.attHandler = AttributeHandler()
        self.tcHandler = TrueColorHandler(self.attHandler)
        self.mats = materials_list
        self.materialUpdater = HelperInput.UpdateEnabled()

    def do(self):
        materials_list = self.tcHandler.inptHlpr.inputToList(self.mats)
        attributeList = self.attHandler.getAttributeType()
        
        for A in attributeList:
            # Here is what sets the color of the materials to grey opaque
            for k in materials_list:
                if k not in session.true_colors.keys(): #otherwise will turn it grey without saving its true color
                    self.tcHandler.saveTrueColor(k, A)
                self.attHandler.changeAttributeColor(k, (192, 192, 192, 129), A)
        visit.DrawPlots()

        # update environment materials
        for mat in materials_list: 
            session.enabled_mats[mat] = False

class include(S.GrizFunction):
    def __init__(self, materials_list):
        '''Combined "vis" and "enable" on specified materials'''
        # ?Question? Is this supposed to have more functionality? Instead of
        # assuming that another part of the program will take care of keeping
        # track of what materials are enabled or disabled for calculation?
        self.attHandler = AttributeHandler()
        self.tcHandler = TrueColorHandler(self.attHandler)
        self.vis = vis(materials_list)
        self.enable = enable(materials_list)
        self.materials_list = materials_list
        self.materialUpdater = HelperInput.UpdateEnabled()

    def do(self):
        materials_list = self.tcHandler.inptHlpr.inputToList(self.materials_list)
        self.vis.do()
        self.enable.do()

            

class exclude(S.GrizFunction):
    def __init__(self, materials_list):
        '''Combined "invis" and "disable" on specified materials'''
        self.attHandler = AttributeHandler()
        self.tcHandler = TrueColorHandler(self.attHandler)
        self.invis = invis(materials_list)
        self.disable = disable(materials_list)                       
        self.materials_list = materials_list
        self.materialUpdater = HelperInput.UpdateEnabled()
    def do(self):
        materials_list = self.tcHandler.inptHlpr.inputToList(self.materials_list)
        self.invis.do()
        self.disable.do()

class swMeshRend(S.GrizFunction):
    """ SWITCH: Set mesh rendering style """
    def __init__(self, key):
        S.GrizFunction.__init__(self)
        self.key = key
    def do(self):
        w = S.WarningMsg('This command is not yet implemented')
        if self.key == 'hidden':
            pass
        elif self.key == 'solid':
            pass
        elif self.key == 'cloud':
            pass
        elif self.key == 'none':
            pass
        elif self.key == 'wf':
            pass
        elif self.key == 'wft':
            pass
        else: w = S.WarningMsg('GrizIt could not parse the item specified after "switch"')
#end class

class select(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class clrsel(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class dscal(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class dscalx(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class dscaly(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

class dscalz(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__(self)

