############################################################################
# grizIt_GUI.py - GUI
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

import sys, os, datetime
sys.path.append("/usr/local/lib/python2.6/site-packages")

from PySide.QtCore import *
from PySide.QtGui import *
from PySide import *

def setAction(action, slot, shortcut, icon,
              tip, checkable, signal):

        if shortcut is not None:
            action.setShortcut(shortcut)
            
#        if icon is not None:
#            action.setIcon(QIcon(":/%s.png" % icon))

        if slot is not None:
            action.connect(action, SIGNAL(signal), slot)
                        
        if checkable is not None:
            action.setCheckable(True)

        if tip is not None:
            action.setStatusTip(tip)      
            action.setToolTip(tip)

#"toggled(bool)" 

def DrawSolid():
        print "DrawSolid...............\n\n"
def CoordSystem():
        print "Coord System...............\n\n"
                              
                                        
#-----------------------------------------------------------------------------
class color_te(QtGui.QTextEdit):
    def __init__(self):
        QtGui.QTextEdit.__init__(self)
        pal = QtGui.QPalette()
        bgc = QtGui.QColor(0, 0, 0)
        pal.setColor(QtGui.QPalette.Base, bgc)
        textc = QtGui.QColor(255, 255, 255)
        pal.setColor(QtGui.QPalette.Text, textc)
        self.setPalette(pal)

#-----------------------------------------------------------------------------
class CMDWindow(QtGui.QMainWindow): 
    def action_test(self):
        print "Action test\n"

    def run_command(self):
        cmd = str(self.input.text())
        cmd = cmd.replace('GrizIt:', '')
        color = QColor("red") 
        self.te2.setTextColor( color );

        font = QFont( "Times" )
        font.setPointSize( 14 )
        font.setWeight( QFont.Bold )
        font.setItalic( True )
        
        self.te2.setCurrentFont( font );
        self.te2.append(cmd)
        color = QColor("green")
        self.te2.setTextColor( color );        
        self.te2.append(cmd)
        self.input.setText("GrizIt:")  
        print "\nCmd=", cmd
	
    def test(self):
        print "Function called......................."
        
    def create_toplevel_menus(self):
        menubar = self.menuBar()
        
        # Create File Sub-menu
        fileMenu = menubar.addMenu('&File')

        exitAction = QAction("E&xit", self)
        setAction(exitAction, exit, None, None, "Exit application",
                  False, "triggered()")
        fileMenu.addAction(exitAction)

        # Create Control Sub-menu
        controlMenu = menubar.addMenu('&Control')
        controlMenu.setTearOffEnabled(True)
        
        drawSolidAction = QAction("&Draw Solid", self)
        setAction(drawSolidAction, DrawSolid, 'Ctrl+S', None, "Draw Solid - no mesh",
                  False, "triggered()")
        controlMenu.addAction(drawSolidAction)
        
        controlMenu.addSeparator()

        coordAction = QAction("&Coord System Toggle", self)
        setAction(coordAction, CoordSystem, None, None, "Coord System On/Off",
                  True, "triggered()")
        controlMenu.addAction(coordAction)
        
        # Create Rendering Sub-menu
        rendering = menubar.addMenu('&Rendering')
        rendering.setTearOffEnabled(True)
        rendering.addAction(coordAction)
        
        picking = menubar.addMenu('&Picking')
        picking.setTearOffEnabled(True)
        picking.addAction(coordAction)
        
        derived = menubar.addMenu('&Derived')
        derived.setTearOffEnabled(True)
        derived.addAction(coordAction)
        
        primal = menubar.addMenu('&Primal')
        primal.setTearOffEnabled(True)
        primal.addAction(coordAction)
        
        plot = menubar.addMenu('&Plot')
        plot.addAction(coordAction)
        
        help = menubar.addMenu('&Help')
        help.setTearOffEnabled(True)
        help.addAction(coordAction)


######################################
# Main Window
######################################
    def create_main_window(self):
        centralwidget = QtGui.QWidget(self)
        frame = QtGui.QFrame(centralwidget)

        verticalLayout = QtGui.QVBoxLayout(centralwidget)

        self.te1 = QtGui.QTextEdit(centralwidget)
        verticalLayout.addWidget(self.te1)

        self.te2 = QtGui.QTextEdit(centralwidget)
        self.te2.setReadOnly(True)
        verticalLayout.addWidget(self.te2)
        label = QLabel(self.tr("Command:"))
        verticalLayout.addWidget(label)

        self.input = QtGui.QLineEdit(centralwidget)
        verticalLayout.addWidget(self.input)
        
        self.setWindowTitle("GrizIt Version 9.1 Control:")
                
        self.setCentralWidget(centralwidget)

        # create connection
        self.connect(self.input, SIGNAL("returnPressed(void)"),
                     self.run_command)

    
    def __init__(self, *args): 
        QtGui.QMainWindow.__init__(self)

        self.setGeometry(100,100,750,550)
       
        self.create_main_window()
        self.create_toplevel_menus()
                
        self.show()

class Window(QtGui.QWidget):
    def __init__(self, parent=None):
        super(Window, self).__init__(parent)

        grid = QtGui.QGridLayout()
        grid.addWidget(self.createFirstExclusiveGroup(), 0, 0)
        grid.addWidget(self.createSecondExclusiveGroup(), 1, 0)
        grid.addWidget(self.createNonExclusiveGroup(), 0, 1)
        grid.addWidget(self.createPushButtonGroup(), 1, 1)
        self.setLayout(grid)

        self.setWindowTitle(self.tr("Group Box"))
        self.resize(480, 420)

    def createFirstExclusiveGroup(self):
        groupBox = QtGui.QGroupBox(self.tr("Exclusive Radio Buttons"))

        radio1 = QtGui.QRadioButton(self.tr("&Radio button 1"))
        radio2 = QtGui.QRadioButton(self.tr("R&adio button 2"))
        radio3 = QtGui.QRadioButton(self.tr("Ra&dio button 3"))

        radio1.setChecked(True)

        vbox = QtGui.QVBoxLayout()
        vbox.addWidget(radio1)
        vbox.addWidget(radio2)
        vbox.addWidget(radio3)
        vbox.addStretch(1)
        groupBox.setLayout(hbox)

        return groupBox

    def createSecondExclusiveGroup(self):
        groupBox = QtGui.QGroupBox(self.tr("E&xclusive Radio Buttons"))
        groupBox.setCheckable(True)
        groupBox.setChecked(False)

        radio1 = QtGui.QRadioButton(self.tr("Rad&io button 1"))
        radio2 = QtGui.QRadioButton(self.tr("Radi&o button 2"))
        radio3 = QtGui.QRadioButton(self.tr("Radio &button 3"))
        radio1.setChecked(True)
        checkBox = QtGui.QCheckBox(self.tr("Ind&ependent checkbox"))
        checkBox.setChecked(True)

        vbox = QtGui.QVBoxLayout()
        vbox.addWidget(radio1)
        vbox.addWidget(radio2)
        vbox.addWidget(radio3)
        vbox.addWidget(checkBox)
        vbox.addStretch(1)
        groupBox.setLayout(vbox)

        return groupBox

    def createNonExclusiveGroup(self):
        groupBox = QtGui.QGroupBox(self.tr("Non-Exclusive Checkboxes"))
        groupBox.setFlat(True)

        checkBox1 = QtGui.QCheckBox(self.tr("&Checkbox 1"))
        checkBox2 = QtGui.QCheckBox(self.tr("C&heckbox 2"))
        checkBox2.setChecked(True)
        tristateBox = QtGui.QCheckBox(self.tr("Tri-&state button"))
        tristateBox.setTristate(True)
        tristateBox.setCheckState(QtCore.Qt.PartiallyChecked)

        vbox = QtGui.QVBoxLayout()
        vbox.addWidget(checkBox1)
        vbox.addWidget(checkBox2)
        vbox.addWidget(tristateBox)
        vbox.addStretch(1)
        groupBox.setLayout(vbox)

        return groupBox

    def createPushButtonGroup(self):
        groupBox = QtGui.QGroupBox(self.tr("&Push Buttons"))
        groupBox.setCheckable(True)
        groupBox.setChecked(True)

        pushButton = QtGui.QPushButton(self.tr("&Normal Button"))
        toggleButton = QtGui.QPushButton(self.tr("&Toggle Button"))
        toggleButton.setCheckable(True)
        toggleButton.setChecked(True)
        flatButton = QtGui.QPushButton(self.tr("&Flat Button"))
        flatButton.setFlat(True)

        popupButton = QtGui.QPushButton(self.tr("Pop&up Button"))
        menu = QtGui.QMenu(self)
        menu.addAction(self.tr("&First Item"))
        menu.addAction(self.tr("&Second Item"))
        menu.addAction(self.tr("&Third Item"))
        menu.addAction(self.tr("F&ourth Item"))
        popupButton.setMenu(menu)

        newAction = menu.addAction(self.tr("Submenu"))
        subMenu = QtGui.QMenu(self.tr("Popup Submenu"), self)
        subMenu.addAction(self.tr("Item 1"))
        subMenu.addAction(self.tr("Item 2"))
        subMenu.addAction(self.tr("Item 3"))
        newAction.setMenu(subMenu)

        vbox = QtGui.QVBoxLayout()
        vbox.addWidget(pushButton)
        vbox.addWidget(toggleButton)
        vbox.addWidget(flatButton)
        vbox.addWidget(popupButton)
        vbox.addStretch(1)
        groupBox.setLayout(vbox)

        return groupBox

if __name__ == "__main__":
    app = QtGui.QApplication(sys.argv)
    w = CMDWindow() 
    w.show()
