############################################################################
# grizIt_GrizOutputCommands.py - Griz Functions - Output Commands.
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
import os, sys, re, bisect, math
import grizIt_VisitInfo

import grizIt_Main

import grizIt_Session
session = grizIt_Session.session

############################################################################################


###################
# Output Commands
###################

class outps(grizIt_Session.GrizFunction):
    def __init__(self, args):
        self.args = args
        self.filename = self.args[0]
    def do(self):
        a = visit.SaveWindowAttributes() 
        a.fileName = self.filename
        a.format = a.POSTSCRIPT
        visit.SetSaveWindowAttributes(a)
        visit.SaveWindow()
    #end def
#end class

class outjpeg(grizIt_Session.GrizFunction):
    def __init__(self, args):
        self.args = args
        self.filename = self.args[0]
    def do(self):
        a = visit.SaveWindowAttributes() 
        a.fileName = self.filename
        a.format   = a.JPEG
        a.quality   = a.session.jpeg_quality
        visit.SetSaveWindowAttributes(a)
        visit.SaveWindow()
    #end def
#end class

class outrgb(grizIt_Session.GrizFunction):
    def __init__(self, args):
        self.args = args
        self.filename = self.args[0]
    def do(self):
        a = visit.SaveWindowAttributes() 
        a.fileName = self.filename
        a.format   = a.RGB
        visit.SetSaveWindowAttributes(a)
        visit.SaveWindow()
    #end def
#end class

class jpegqual(grizIt_Session.GrizFunction):
    def __init__(self, args):
        self.args = args
        session.jpeg_quality = int(self.args[0])
    def do(self):
        pass # (just initializes it and sets the quality)
    #end def
#end class

class outrgba(grizIt_Session.GrizFunction):
    def __init__(self, args):
        grizIt_Session.GrizFunction.__init__(self)

class outpng(grizIt_Session.GrizFunction):
    def __init__(self, args):
        grizIt_Session.GrizFunction.__init__(self)

class outmm(grizIt_Session.GrizFunction):
    def __init__(self, args):
        grizIt_Session.GrizFunction.__init__(self)

class outobj(grizIt_Session.GrizFunction):
    def __init__(self, args):
        grizIt_Session.GrizFunction.__init__(self)

class outview(grizIt_Session.GrizFunction):
    def __init__(self, args):
        grizIt_Session.GrizFunction.__init__(self)

class outhid(grizIt_Session.GrizFunction):
    def __init__(self, args):
        grizIt_Session.GrizFunction.__init__(self)

class outth(grizIt_Session.GrizFunction):
    def __init__(self, args):
        grizIt_Session.GrizFunction.__init__(self)

class outvec(grizIt_Session.GrizFunction):
    def __init__(self, args):
        grizIt_Session.GrizFunction.__init__(self)

class outpt(grizIt_Session.GrizFunction):
    def __init__(self, args):
        grizIt_Session.GrizFunction.__init__(self)
