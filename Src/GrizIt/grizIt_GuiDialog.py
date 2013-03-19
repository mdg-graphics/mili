#!/usr/bin/env python
##############################################################################
# Application: GrizIt - Griz CLI interface to the VisIt rendering
#
# grizIt_GuiDialog.py - Classes for creating messages and dialogs.
#
#      Lawrence Livermore National Laboratory
#
#############################################################################
# Modifications:
#
#  I. R. Corey - August 25, 2010: Created.
#
#############################################################################

import os, sys, re, bisect, math, copy
import sys, os, datetime
sys.path.append("/usr/local/lib/python2.6/site-packages")

from PySide.QtCore import *
from PySide.QtGui import *
from PySide import *
from grizIt_ParseGrizCommand import *
from grizIt_Main import *

#################
# Message Window
#################
class msgWindow(QtGui.QWidget):
      def __init__(self, parent=None):
        self.fontName=None
	self.colorName=None
        self.size=None
	self.ital=False
	self.bold=False
	self.uline=False
        self.title="None"
	
 	self.height=700
	self.width=400
	
        super(msgWindow, self).__init__(parent)
        self.msg = QTextEdit(self)
        self.msg.setReadOnly(True)
	
	layout = QVBoxLayout(self)
        layout.addWidget(self.msg)
	
        self.setWindowTitle(self.title)
        self.resize(self.height, self.width)
        self.setAttribute(Qt.WA_DeleteOnClose, True)
	
      def msgText(self, text ):
        font = QFont( self.fontName )
        font.setPointSize( self.size )
	if ( self.bold==True):
	     font.setWeight( QFont.Bold )
	font.setItalic( self.ital )
        color = QColor( self.colorName )
	
        self.msg.setAlignment( self.align )
	self.msg.setTextColor( color )
        self.msg.setCurrentFont( font )
        self.msg.setFontUnderline( self.uline )
        self.msg.setTextColor( color )
	self.msg.append( text )

      def msgFont( self, fontName, colorName, size, ital, bold, uline, align ):
        self.fontName  = fontName
        self.size      = size
        self.colorName = colorName	
	self.bold      = bold
	self.ital      = ital
	self.uline     = uline
	self.align     = align

      def msgWinSize( self, height, width ):
        self.height = height
        self.width  = width

      def msgWinTitle( self, title ):
        self.title = title

class msgBox(QtGui.QMessageBox):
    def __init__(self, parent=None):
        QtGui.QMessageBox.__init__(self, parent)
        self.setGeometry(300, 300, 250, 150)

    def warning(self, msgTxt):
        reply = QtGui.QMessageBox.warning(self, 'Warning', msgTxt + "                                                  ")
    def info(self, msgTxt):
        reply = QtGui.QMessageBox.information(self, 'Information', msgTxt + "                                                  ")
    def critical(self, msgTxt):
        reply = QtGui.QMessageBox.critical(self, 'Error', msgTxt + "                                                  ")
	

