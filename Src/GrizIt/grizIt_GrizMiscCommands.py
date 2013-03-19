#!/usr/gapps/visit/python/2.6.4/linux-x86_64_gcc-4.1/bin/python
############################################################################
# grizIt_GrizMiscCommands.py - Griz Functions - Misc Commands.
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      July 28, 2010
#
# 
############################################################################
# Modifications:
#  I. R. Corey - July 28, 2010: Created.
#
############################################################################
#

import pdb
import os, sys, re, bisect, math

from PySide.QtCore import *
from PySide.QtGui import *
from PySide import *

import grizIt_Session as S
import grizIt_GuiMain as GUI
import grizIt_GuiDialog as DIALOG
import grizIt_VisitInfo as VISITINFO

session = S.session
env     = S.env

################
# Misc Commands
################

class copyrt(S.GrizFunction):
    def __init__(self, args):
        self.options = ' '.join(args)
    #end def

    #
    # Cleanup PopUp message windows
    #
    def CRdelete(self):
        if ( env.CRWin != None ):
             env.CRWin.hide()
             env.CRWin.destroy()
             env.CRWin=None
        #end if

    def do(self):
        if len(self.options)>0:
           if ( self.options=="clear" ):
                self.CRdelete()
        else: 
            color="green"
            env.CRWin = DIALOG.msgWindow()
            env.CRWin.msgFont( "Courior", color, 24, False, True, False, Qt.AlignCenter )
            #                                        ital   bold  uline
            env.CRWin.msgText( "GrizIt\n" )
            
            env.CRWin.msgFont( "Courior", color, 12, False, False, False, Qt.AlignCenter )
            
            env.CRWin.msgText( "Visualization of Finite Element Analysis Results\n" )
            env.CRWin.msgText( "Methods Development Group" )
            env.CRWin.msgText( "Lawrence Livermore National Laboratory\n" )
            
            env.CRWin.msgFont( "Courior", color, 14, True, False, True, Qt.AlignCenter )
            env.CRWin.msgText( "Authors" )
            
            env.CRWin.msgFont( "Courior", color, 12, False, False, False, Qt.AlignCenter )
            env.CRWin.msgText( "Ivan R. Corey" )
            env.CRWin.msgText( "Sara Drakeley" )
            env.CRWin.msgText( "Kevin Durrenberger\n" )
            env.CRWin.msgText( "Copyright (c) 2010 Lawrence Livermore National Laboratory");
            
            env.CRWin.show()
#end class

    
class noOp(S.GrizFunction):
    def __init__(self, args):
        self.args = args
    #end def
    def do(self):
        m = S.Message("This Griz Command [" + session.lastCommand + "] has no equivalant in Visit\n", 2 )
        m.setParams( "Courior", "red", 11, False, True, False, Qt.AlignLeft )
        #                                  ital   bold  uline
    #end def
#end class


################
# Misc Commands
################
class clrsym(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
    #end def
    
    
#end class

class tmx(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class tmy(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class tmz(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class clrtm(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class partrad(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class em(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class emsc(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class partrad(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class emcsph(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class emcyl(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__() 
#end class

class emax(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class emrm(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class clrem(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class tellem(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class crease(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class getedg(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class emrm(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class edgbias(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class inrgb(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class mat(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class light(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class tlx(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class tly(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class tlz(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class dellet(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class camang(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class lookfr(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class lookat(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class lookup(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class tfx(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class tfy(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class tfz(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class tax(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class tay(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class taz(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class near(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class far(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class fracsz(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class bbsrc(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class bbox(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class hidwid(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class

class bufqty(S.GrizFunction):
    def __init__(self):
        S.GrizFunction.__init__()
#end class
