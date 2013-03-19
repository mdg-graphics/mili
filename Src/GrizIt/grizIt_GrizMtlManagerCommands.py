############################################################################
# grizIt_GrizMtlManagerCommands.py - Griz Functions: Mtl Manager Commands
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
import grizIt_Session as S
import grizIt_VisitInfo

session = S.session

import grizIt_inputHelper as inputHelper
import grizIt_GrizMeshControlCommands as MeshControl

############################################################################################
 
        
#TO DO: Perhpas these should all inherit from GrizFunction, but right now they all are independent objects

class mtl(object):
    """ This is the precursor for the classes defined below. As in, the Griz
        command should be of the form: mtl amb .3 .4 .5

    """
    def __init__(self, command_tokens):
        """ Creates the appropriate class and performs the appropriate do function

            class_key is the name of the class (see classes below)

            args are the arguments listed after the keyword
        """
        self.class_key = command_tokens[0]
        self.args = command_tokens[1:]
        d = S.debugMsg('(prnt stmt in mtl) Command Arg: %s ' % (self.class_key))
        self.command_dict = {'amb': amb, 'diff':diff, 'spec':spec,  'emis':emis, 'shine':shine, 'alpha':alpha, 'gslevel':gslevel}
    def do(self):
        PyCommand= self.command_dict[self.class_key]
        newInstance = PyCommand(*self.args)
        newInstance.do()
        
        

# This was the info from the dict:


#   "mtl amb":		(MTL.amb,		3,	3,      "Changes the color of the ambient light"),

#mtl: "Modify visibility and/or enable-state of multiple materials: mtl function~ { all | n~} [continue] | Modify, or preview modification of, material color properties for multiple materials: mtl [preview] mat {all | n~} color_property | Keep or reject previewed material color property change: mtl {apply | cancel} | Reset material color properties for multiple materials to their default values: mtl default {all | n~} [continue]"),
                         
#    "mtl diff":         (MTL.diff,              3,      3,      "Changes the color of the diffuse light"),
#    "mtl spec":         (MTL.spec,              3,      3,      "Changes the color of the specular light"),
#    "mtl specStrength": (MTL.specStrength,      1,      1,      "This is not actually a fcn, but is awesome"),
#    "mtl specSharpness": (MTL.specSharpness,    1,      1,      "Also not a fcn for griz, but awesome"),

#    "mtl emis":         (MTL.emis,              3,      3,      "Emissive light color"),

#    "mtl shine":        (MTL.shine,             1,      1,      "Shininess"),

#    "mtl alpha":        (MTL.alpha,             1,      1,      "Opacity"),

#    "mtl gslevel":      (MTL.gslevel,           1,      1,      "Gray Scale Level"),









class mat(object):
    """ A helper object
    """

    def rgbScale(self, r, g, b):
        r=math.floor(float(r)*255) # Not sure if the floor is necessary
        g=math.floor(float(g)*255)
        b=math.floor(float(b)*255)
        return r, g, b

    def opacityScale(self, v):
        v=math.floor(float(v)*255)
        return v





class amb(object):
    def __init__(self, r, g, b):
        ''' Ambient light color

            r,g,b from 0-1
        '''
        self.mat = mat()
        self.r = r
        self.g = g
        self.b = b
    def do(self):
        r, g, b = self.mat.rgbScale(self.r,self.g,self.b)
        light0 = visit.LightAttributes()
        light0.type = light0.Ambient
        light0.color = (r, g, b, 255)
        visit.SetLight(0, light0)
        # goes through 7 iterations of the exact same thing, except their first
        # one was to set (255, 255, 0, 255)* this is the one that makes the
        # color change. Other ex: (51, 153, 102, 255), as opposed to the rest
        # which were (255, 255, 255, 255)
        #return materials_list

        # ** other things you can change: direction, brightness of light)



class diff(mat):
    def __init__(self, r, g, b):
        ''' Diffuse light color

            r,g,b from 0-1
        '''
        self.mat = mat()
        self.r = r
        self.g = g
        self.b = b
    def do(self):
        # is camera diffuse or specular?
        r,g,b=self.mat.rgbScale(self.r,self.g,self.b)
        light0 = visit.LightAttributes()
        light0.type = light0.Camera
        light0.color = (r, g, b, 200) #maybe diffuse light is camera, less opaque?
        visit.SetLight(0, light0)
        

class spec(mat):
    def __init__(self, r, g, b):
        ''' Specular light color

            r,g,b from 0-1
        '''
        self.r = r
        self.g = g
        self.b = b
    def do(self):
        r,g,b=self.rgbScale(self.r,self.g,self.b)
        RA = visit.RenderingAttributes()
        RA.specularFlag = 1
        RA.specularColor=(r, g, b, 255)
        visit.SetRenderingAttributes(RA)
        #return materials_list


# These were functions I created below. However, they didn't work in the griz environment, even though
# they worked in the visit that I had running from my mac. But they are not part of the
# griz commands, so I have abandoned them.

#class specStrength(mat):
#    def __init__(self, v):
#        ''' v is a float between 0 and 1 '''
#        self.v = v
#    def do(self):
#        RA = visit.RenderingAttributes()
#        RA.specularFlag = 1
#        RA.specularCoeff = self.v
#        visit.SetRenderingAttributes(RA)
        

#class specSharpness(mat):
#    def __init__(self, v):
#        ''' Controls sharpness or power of the specular light.
#            v is a value between 0 and 1
#        '''
#        self.v = v
#    def do(self):
#        RA = visit.RenderingAttributes()
#        RA.specularFlag = 1
#        RA.specularPower = 100*self.v
#        visit.SetRenderingAttributes(RA)

class emis(mat):
    def __init__(self, r, g, b):
        ''' Emissive light color, object gives off light

            r,g,b from 0-1
        '''
        self.r = r
        self.g = g
        self.b = b
    def do(self):
        r,g,b=self.rgbScale(self.r, self.g, self.b)
        light0 = visit.LightAttributes()
        light0.type = light0.Object #Ambient, Object, Camera
        light0.color = (r, g, b, 255)
        visit.SetLight(0, light0)
        #return materials_list

class shine(mat):
    def __init__(self, v):
        ''' Shininess.

            v from 0-128; a value f 0 disables specularity #*? Should it be 255?
            * Higher values give smaller, more tightly focused specular
                highlights
        '''
        self.v = v
    def do(self):
        light0 = visit.LightAttributes()
        current_r = light0.color[0]
        current_g = light0.color[1]
        current_b = light0.color[2]
        light0.type = light0.Camera
        d = S.DebugMsg(light0.color+ 'does this work? CHECK SHINE ')
        light0.color=(current_r, current_g, current_b, self.v)#Should v range to 255?
        visit.SetLight(0, light0)
        

class alpha(mat):
    def __init__(self, *input):
        ''' Opacity

            v from 0-1, where 0=black, 1=white

            *Note, inputList will have a list of materials followed by the opacity value, v
        '''
        self.inptHlpr = inputHelper.InputParser()
        self.materials_list = list(input[:-1])
        self.v = input[-1]
        self.attHandler = MeshControl.AttributeHandler()
    def do(self):
        # TO DO: fix these to utilize self.changeAttributeColor(args...)
        v=self.opacityScale(self.v)
        materials_list = self.inptHlpr.inputToList(self.materials_list)
        attributeList = self.attHandler.getAttributeType()
        for A in attributeList:
            for k in materials_list:
                if attributeList.index(A) in [0,1,3,14]:
                    if k not in session.true_colors.keys():
                        session.true_colors[k] = A.GetMultiColor()[k-1]
                    current_color_tup = A.GetMultiColor()[k-1][0:3]
                    current_color_tup = current_color_tup + (v,) #The opacity
                    A.SetMultiColor(k-1, current_color_tup)
                elif attributeList.index(A) in [6,10,13,14]:
                    if k not in session.true_colors.keys():
                        session.true_colors[k] = A.GetOpacity()[k-1]
                    A.SetOpacity(k-1, v)
                else: d = S.DebugMsg('the attribute,'+A+',has not been accounted for')
            visit.SetPlotOptions(A)
        return materials_list

class gslevel(mat):
    def __init__(self, *input):
        ''' Gray Scale Level

            v from 0-1, where 0=black, 1=white

            *Note, inputList will have a list of materials followed by the gslevel, v
        '''
        self.inptHlpr = inputHelper.InputParser()
        self.materials_list = list(input[:-1])
        self.v = input[-1]
        self.attHandler = MeshControl.AttributeHandler()
    def do(self):
        # To DO: fix to utilize self.changeAttributeColor(args...)
        materials_list = self.inptHlpr.inputToList(self.materials_list)
        v=self.opacityScale(self.v)
        attributeList = self.attHandler.getAttributeType()
        for A in attributeList:
            for k in materials_list:
                if attributeList.index(A) in [0,1,3,14]:
                    if k not in session.true_colors.keys():
                        session.true_colors[k] = A.GetMultiColor()[k-1]
                    current_opacity = A.GetMultiColor()[k-1][3]
                    A.SetMultiColor(k-1, (v, v, v, current_opacity))
                elif attributeList.index(A) in [6,10,13,14]:
                    # Don't know how to change greyscale for these types
                    d = S.DebugMsg('Unsure how to change gresycale from the command line. To fix')
                else: d = S.DebugMsg('the attribute,'+A+',has not been accounted for')
            visit.SetPlotOptions(A)                               
        return materials_list # need to change the variable within the env!


