#!/usr/gapps/visit/python/2.6.4/linux-x86_64_gcc-4.1/bin/python
#
############################################################################
# grizIt_GuiUtil.py - GrizIt GUI Utility Functions.
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

import pdb

import sys, os, datetime
sys.path.append("/usr/local/lib/python2.6/site-packages")

from PySide.QtCore import *
from PySide.QtCore import *
from PySide.QtGui import *
from PySide import *

from grizIt_Main import *
import grizIt_ParseGrizCommand as PC
from grizIt_Env import *
import grizIt_Session as S

session = S.session
env     = S.env
#------------------------------------------------------------------------------

class UtilWindow(QtGui.QWidget):
    def __init__(self, parent=None):
        super(UtilWindow, self).__init__(parent)

        centralwidget = QtGui.QWidget(self)
        frame = QtGui.QFrame()
        frameLayout = QVBoxLayout( frame )
        
        frame.setFrameShape( QFrame.StyledPanel );
        frame.setFrameShadow( QFrame.Raised );
        
        grid = QtGui.QGridLayout(self)

        self.setWindowTitle(self.tr(grizItVersion+" Utility Panel:"))

        self.resize(480, 420)
        self.show()
        f = QFont()
        f.setFamily( "Helvetica [Adobe]" )

        grid.addWidget( self.layoutUpper() )
        grid.addWidget( self.layoutLower() )
        self.setLayout(grid)

        self.show()
        self.setWindowTitle(self.tr(grizItVersion+" Utility Panel:"))
        self.resize(480, 420)

    def layoutUpper(self):
        groupBox = QtGui.QGroupBox()
        groupBox.setCheckable(False)
        groupBox.setChecked(False)

        hbox = QtGui.QHBoxLayout()
        hbox.addStretch(1)

        label = QLabel(self.tr("Step"))
        label.setSizePolicy( QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed )
        hbox.addWidget( label )

        btnStep1 = QPushButton();
        btnStep1.setIcon(QIcon(QPixmap('./Icons/Bitmaps/GrizLeftStop')))
        btnStep1.setMinimumSize( QSize( 20, 0 ) );
        btnStep1.setAutoDefault( 0 );
        self.connect(btnStep1, SIGNAL("clicked()"),self.btnStep1Clicked)
        hbox.addWidget( btnStep1 );
        
        btnStep2 = QPushButton();
        btnStep2.setIcon(QIcon(QPixmap('./Icons/Bitmaps/GrizLeft')))
        btnStep2.setMinimumSize( QSize( 20, 0 ) );
        btnStep2.setAutoDefault( 0 );
        self.connect(btnStep2, SIGNAL("clicked()"),self.btnStep2Clicked)
        hbox.addWidget( btnStep2 );

        btnStep3 = QPushButton();
        btnStep3.setIcon(QIcon(QPixmap('./Icons/Bitmaps/GrizRight')))
        btnStep3.setMinimumSize( QSize( 20, 0 ) );
        btnStep3.setAutoDefault( 0 );
        self.connect(btnStep3, SIGNAL("clicked()"),self.btnStep3Clicked)
        hbox.addWidget( btnStep3 );

        btnStep4 = QPushButton();
        btnStep4.setIcon(QIcon(QPixmap('./Icons/Bitmaps/GrizRightStop')))
        btnStep4.setMinimumSize( QSize( 20, 0 ) );
        btnStep4.setAutoDefault( 0 );
        self.connect(btnStep4, SIGNAL("clicked()"),self.btnStep4Clicked)
        hbox.addWidget( btnStep4 );

        label = QLabel(self.tr("Stride:"))
        label.setSizePolicy( QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed )
        hbox.addWidget( label )

        name = 'Stride'
        s = QSpinBox(self)
        s.setMaximum(1000)
        s.setMinimum(0)
        QObject.connect(s, SIGNAL('valueChanged(int)'),
                        self.valueChanged)           
        setattr(self, name, s)
        hbox.addWidget(s)     

        label = QLabel(self.tr("Animate"))
        label.setSizePolicy( QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed )
        hbox.addWidget( label )
        
        btnAnim1 = QPushButton();
        btnAnim1.setIcon(QIcon(QPixmap('./Icons/Bitmaps/GrizLeftStop')))
        btnAnim1.setMinimumSize( QSize( 20, 0 ) );
        btnAnim1.setAutoDefault( 0 );
        self.connect(btnAnim1, SIGNAL("clicked()"),self.btnAnim1Clicked)
        hbox.addWidget( btnAnim1 );
        
        btnAnim2 = QPushButton();
        btnAnim2.setIcon(QIcon(QPixmap('./Icons/Bitmaps/GrizLeft')))
        btnAnim2.setMinimumSize( QSize( 20, 0 ) );
        btnAnim2.setAutoDefault( 0 );
        self.connect(btnAnim2, SIGNAL("clicked()"),self.btnAnim2Clicked)
        hbox.addWidget( btnAnim2 );

        btnAnim3 = QPushButton();
        btnAnim3.setIcon(QIcon(QPixmap('./Icons/Bitmaps/GrizRight')))
        btnAnim3.setMinimumSize( QSize( 20, 0 ) );
        btnAnim3.setAutoDefault( 0 );
        self.connect(btnAnim3, SIGNAL("clicked()"),self.btnAnim3Clicked)
        hbox.addWidget( btnAnim3 );

        groupBox.setLayout(hbox)
        return groupBox
    
    def valueChanged(self, sbvalue):
        newStride = "stride "+str(sbvalue)
        PC.ParseCommand(newStride)
        d = S.DebugMsg("\nValue="+sbvalue)

    # Step button callbacks
    def btnStep1Clicked(self):
        print "B1 pressed"
        PC.ParseCommand('f')
        d = S.DebugMsg("\nB1!")

    def btnStep2Clicked(self):
        PC.ParseCommand('p')
        d = S.DebugMsg("\nB2!")

    def btnStep3Clicked(self):
        PC.ParseCommand('n')
        d = S.DebugMsg("\nB3!")

    def btnStep4Clicked(self):
        PC.ParseCommand('l')
        d = S.DebugMsg("\nB3!")

    # Animate button callbacks
    def btnAnim1Clicked(self):
        PC.ParseCommand('anim')
        d = S.DebugMsg("\nB1!")

    def btnAnim2Clicked(self):
        PC.ParseCommand('stopan')
        d = S.DebugMsg("\nB2!")

    def btnAnim3Clicked(self):
        PC.ParseCommand('animc')

    def layoutLower(self):
        groupBox = QtGui.QGroupBox()
        hbox = QtGui.QHBoxLayout()
        hbox.addStretch(1)

        hbox.addWidget(self.createMeshViewGroup())
        hbox.addWidget(self.createPickModeGroup())
        hbox.addWidget(self.createPickClassGroup())
        hbox.addWidget(self.createCleanupGroup())

        groupBox.setLayout(hbox)
        return groupBox

    ####################
    # Mesh View Buttons
    ####################
    def createMeshViewGroup(self):
        groupBox = QtGui.QGroupBox()
        vbox = QtGui.QVBoxLayout()
        vbox.addStretch(1)
        groupBox.setLayout(vbox)

        label = QLabel(self.tr("Mesh View"))
        label.setSizePolicy( QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed )
        vbox.addWidget( label )

        # first group

        # Create an exclusive button group
        # grp1=QButtonGroup()
        # grp1.setColumnLayout(Qt.Vertical )
        # grp1.setExclusive(TRUE)
        
        # insert 3 radiobuttons
        # rb11=QRadioButton("&Radiobutton 1", grp1)
        # rb11.setChecked(TRUE)
        # QRadioButton("R&adiobutton 2", grp1)
        # QRadioButton("Ra&diobutton 3", grp1)
        # vbox.addWidget( grp1 )        
        
        return groupBox

    ####################
    # Pick Mode Buttons
    ####################
    def createPickModeGroup(self):
        groupBox = QtGui.QGroupBox()
        vbox = QtGui.QVBoxLayout()

        vbox.addStretch(1)

        groupBox.setLayout(vbox)          

        label = QLabel(self.tr("Pick Mode"))
        label.setSizePolicy( QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed )
        vbox.addWidget( label )
        combo = QComboBox();     

	for className in session.MiliClassNames:
            d = S.DebugMsg( "\nClassName="+ className)

        combo.addItem('1')
        combo.addItem('2')

        # Connect the activated signal on the combo box to our handler.
        self.connect(combo, QtCore.SIGNAL('activated(QString)'), self.combo1Pick)

        vbox.addWidget( combo )
        return groupBox
    
    def combo1Pick(self, value): 
        d = S.DebugMsg( "\nComboValue="+ value)
            
    ####################
    # Pick Class Buttons
    ####################
    def createPickClassGroup(self):
        groupBox = QtGui.QGroupBox()
        vbox = QtGui.QVBoxLayout()

        vbox.addStretch(1)
        groupBox.setLayout(vbox)          

        label = QLabel(self.tr("Btn Pick Class"))
        label.setSizePolicy( QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed )
        vbox.addWidget( label )
        combo = QComboBox();     

	for className in session.MiliClassNames:
            d = S.DebugMsg( "\nClassName="+ className)

        combo.addItem('1')
        combo.addItem('2')

        # Connect the activated signal on the combo box to our handler.
        self.connect(combo, QtCore.SIGNAL('activated(QString)'), self.combo1Pick)

        vbox.addWidget( combo )
        return groupBox

    ####################
    # Clean-up Buttons
    ####################
    def createCleanupGroup(self):
        groupBox = QtGui.QGroupBox()
        vbox = QtGui.QVBoxLayout()

        vbox.addStretch(1)

        groupBox.setLayout(vbox)          

        label = QLabel(self.tr("Clean-up"))
        label.setSizePolicy( QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Fixed )
        vbox.addWidget( label )
        return groupBox

    def btnAnim4Clicked(self):
        d = S.DebugMsg("\nB3!")

if __name__ == "__main__":
    app = QtGui.QApplication(sys.argv)
    w = UtilWindow() 
    w.show()
    

