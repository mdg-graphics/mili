#!/usr/bin/env python
#
############################################################################
# grizIt_GuiMaterialMgr.py - Griz Material Manager GUI Functions.
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      August 27, 2010
#
############################################################################
# Modifications:
#  I. R. Corey - August 27, 2010: Created.
#
############################################################################
#
import os, sys, re, bisect, math, copy
import sys, os, datetime
sys.path.append("/usr/local/lib/python2.6/site-packages")

from PySide.QtCore import *
from PySide.QtGui import *
from PySide import *

import grizIt_ParseGrizCommand as PC
from grizIt_GuiCB import *
import grizIt_Session

##########################################################################
class MaterialMegPanel(QtGui.QWidget):
    def __init__(self, parent=None):
        super(MaterialMegPanel, self).__init__(parent)

        self.mtlStates = {"vis":True, "enable":True, "color":None} # Still to do: Figure out how to get the color!
        self.mtlsChecked = []

        layout = QHBoxLayout()
        spacTopLeft = QSpacerItem( 50, 20, QSizePolicy.Expanding, QSizePolicy.Minimum );

        grid = QtGui.QGridLayout()
	
        
        grid.addWidget(self.createFunctionPanel(), 0, 0)
        grid.addWidget(self.createSliderPanel(), 1, 0)
        grid.addWidget(self.createMaterialsPanel(), 0, 1)
        grid.addWidget(self.createActionPanel(), 1, 1)
        self.setLayout(grid)

        self.setWindowTitle(self.tr("GrizIt: Material Manager"))
        self.resize(480, 420)
        self.show()
	
    def createFunctionPanel(self):
        bg = QtGui.QButtonGroup()
        groupBox = QtGui.QGroupBox(self.tr("Functions"))
        
        radio1 = QtGui.QPushButton(self.tr("&Visible"))
        radio2 = QtGui.QPushButton(self.tr("&Invisible"))
        radio3 = QtGui.QPushButton(self.tr("&Enable"))
        radio4 = QtGui.QPushButton(self.tr("&Disable"))
        radio5 = QtGui.QPushButton(self.tr("&Color"))

        radio1.setChecked(True)

        hbox = QtGui.QHBoxLayout()
        hbox.addWidget(radio1)
        hbox.addWidget(radio2)
        hbox.addWidget(radio3)
        hbox.addWidget(radio4)
        hbox.addWidget(radio5)
        hbox.addStretch(1)
        groupBox.setLayout(hbox)

        return groupBox

    def createSliderPanel(self):
        groupBox = QtGui.QGroupBox(self.tr("Sliders"))
        groupBox.setCheckable(False)

        Rslider = QtGui.QSlider(QtCore.Qt.Horizontal, self)
        self.label = QtGui.QLabel("R")
        self.connect(Rslider, QtCore.SIGNAL('valueChanged(int)'),
                     self.changeValue)

        Gslider = QtGui.QSlider(QtCore.Qt.Horizontal, self)
        self.label = QtGui.QLabel("G")
        self.connect(Gslider, QtCore.SIGNAL('valueChanged(int)'),
                     self.changeValue)

        Bslider = QtGui.QSlider(QtCore.Qt.Horizontal, self)
        self.label = QtGui.QLabel("B")
        self.connect(Bslider, QtCore.SIGNAL('valueChanged(int)'),
                     self.changeValue)
      
        vbox = QtGui.QVBoxLayout()
        vbox.addWidget(Rslider)
        vbox.addWidget(Gslider)
        vbox.addWidget(Bslider)
        vbox.addStretch(1)
        groupBox.setLayout(vbox)

        return groupBox

    def createMaterialsPanel(self):
        groupBox = QtGui.QGroupBox(self.tr("Materials"))
        groupBox.setFlat(True)

        mtlButtons = []
        print "\n@\n@\n@\n@\n@\n@\n@\nThese are the materials: ", session.DBMatSetNames
        for material in session.DBMatSetNames:
            mtlButtons.append(QtGui.QCheckBox(self.tr(material)))
            
        if mtlButtons: mtlButtons[0].setChecked(True)

        vbox = QtGui.QVBoxLayout()
        for checkbox in mtlButtons:
            vbox.addWidget(checkbox)

        vbox.addStretch(1)
        groupBox.setLayout(vbox)

        return groupBox

    def createActionPanel(self):
        groupBox = QtGui.QGroupBox(self.tr("Action"))
        #groupBox.setCheckable(True)
        #groupBox.setChecked(True)

        previewButton = QtGui.QPushButton(self.tr("&Preview"))
        cancelButton = QtGui.QPushButton(self.tr("&Cancel"))
        
        applyButton = QtGui.QPushButton(self.tr("&Apply"))
        self.connect(applyButton, SIGNAL("clicked()"),self.applyClicked)
        

        defaultButton = QtGui.QPushButton(self.tr("Default"))
        #menu = QtGui.QMenu(self)
        #menu.addAction(self.tr("&First Item"))
        #menu.addAction(self.tr("&Second Item"))
        #menu.addAction(self.tr("&Third Item"))
        #menu.addAction(self.tr("F&ourth Item"))
        #popupButton.setMenu(menu)

        #newAction = menu.addAction(self.tr("Submenu"))
        #subMenu = QtGui.QMenu(self.tr("Popup Submenu"), self)
        #subMenu.addAction(self.tr("Item 1"))
        #subMenu.addAction(self.tr("Item 2"))
        #subMenu.addAction(self.tr("Item 3"))
        #newAction.setMenu(subMenu)

        vbox = QtGui.QVBoxLayout()
        vbox.addWidget(previewButton)
        vbox.addWidget(defaultButton)
        vbox.addWidget(applyButton)
        vbox.addWidget(cancelButton)
        vbox.addStretch(1)
        groupBox.setLayout(vbox)

        return groupBox



    def applyClicked(self):
        print "Apply clicked"

        if self.mtlStates["vis"] == True:
            PC.ParseCommand("vis" + " ".join(self.mtlsChecked))
        else:
            PC.ParseCommand("invis" + " ".join(self.mtlsChecked))

        if self.mtlStates["enable"] == True:
            PC.ParseCommand("enable" + " ".join(self.mtlsChecked))
        else:
            PC.ParseCommand("disable" + " ".join(self.mtlsChecked))

        if self.mtlStates["color"]:
            pass # Still to figure out


    def changeValue(self):
        print "Current slider position = ..."

        
