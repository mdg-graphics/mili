#!/usr/bin/env python
#
############################################################################
# grizIt_GuiMain.py - Griz Main GUI Functions.
# 
#!/usr/gapps/visit/python/2.6.4/linux-x86_64_gcc-4.1/bin/python
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      May 07, 2010
#
############################################################################
# Modifications:
#  I. R. Corey - August 23, 2010: Created.
#
############################################################################
#
import os, sys, re, bisect, math, copy
import sys, os, datetime
sys.path.append("/usr/local/lib/python2.6/site-packages")
sys.path.append("/usr/gapps/visit/python/2.6.4/linux-x86_64_gcc-4.1/lib/python2.6/site-packages")

from PySide.QtCore import *
from PySide.QtGui import *
from PySide import *

import grizIt_VisitInfo as VI
from grizIt_ParseGrizCommand import *
from grizIt_Main import *
from grizIt_GuiCB import *
import grizIt_Session as GS

session = GS.session
env     = GS.env

def setAction(action, slot, shortcut, icon,
              tip, checkable, signal):

        if shortcut is not None:
            action.setShortcut(shortcut)
            
        action.setIcon(QIcon("%s.png" % icon))
        if icon is not None:
            action.setIcon(QIcon("%s.png" % icon))

        if slot is not None:
            action.connect(action, SIGNAL(signal), slot)
                        
        if checkable is not None:
            action.setCheckable(True)

        if tip is not None:
            action.setStatusTip(tip)      
            action.setToolTip(tip)

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

######################################
# CMD(Main) Window
######################################
class CMDWindow(QtGui.QMainWindow): 
    def __init__(self, *args): 
        QtGui.QMainWindow.__init__(self)
        self.fontName="Courior"
	self.colorName="blue"
        self.size=9
	self.ital=False
	self.bold=False
	self.uline=False
        self.align=Qt.AlignLeft 
 	self.xpos=100
	self.ypos=100
	self.height=800
	self.width=500
        
        self.setGeometry( self.xpos, self.ypos, self.height, self.width )
	       
        self.create_main_window()
        self.create_toplevel_menus()
        self.setWindowIcon(QtGui.QIcon('myicon.png'))
        self.show()
	
    def create_main_window(self):
        splitter = QSplitter(Qt.Vertical, self);
        centralwidget = QtGui.QWidget(self)
        frame = QtGui.QFrame(centralwidget)
 
        self.te1 = QtGui.QTextEdit(frame)
        self.te1.setReadOnly(True)
        self.te1.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        splitter.addWidget(self.te1)

        self.te2 = QtGui.QTextEdit(frame)
        self.te2.setReadOnly(True)
        self.te2.installEventFilter(self)

        splitter.addWidget(self.te2)
        label = QLabel(self.tr("Command:"))
        splitter.addWidget(label)

        self.input = QtGui.QLineEdit(frame)
        self.input.installEventFilter(self)
	
        splitter.addWidget(self.input)
        
        self.setWindowTitle(grizItVersion+" Control:")
	                
        self.setCentralWidget(splitter)
	
        # create connections
        self.connect(self.input, SIGNAL("returnPressed(void)"),
                     self.return_pressed)

        # add code to allow capture of commands in text box
        self.connect(self.te2, SIGNAL("selectionChanged(void)"), self.selectTxt );

    def selectTxt(self):
        cursor = self.te2.textCursor();		
        cursor.movePosition(QTextCursor.StartOfBlock)
        cursor.movePosition(QTextCursor.EndOfBlock, QTextCursor.KeepAnchor);
        cmdLine = cursor.selectedText();
        env.cmdSelected=True
        env.selectedCmd=cmdLine
        env.returnPressed=False
        self.run_command()
	
    def MsgFont( self, fontName, colorName, size, ital, bold, uline, align ):
        self.fontName  = fontName
        self.size      = size
        self.colorName = colorName	
	self.bold      = bold
	self.ital      = ital
	self.align     = align
        self.uline     = uline
	
    def MsgText( self, text, msgBoxNum ):
        if ( msgBoxNum==1 ):
	     mb=self.te1
	else:
	     mb=self.te2
	     
	mb.moveCursor(QtGui.QTextCursor.End)
        font = QFont( self.fontName )
        font.setPointSize( self.size )
	if ( self.bold==True):
	     font.setWeight( QFont.Bold )
	font.setItalic( self.ital )
        color = QColor( self.colorName )
	
        mb.setAlignment( self.align )
	mb.setTextColor( color )
        mb.setCurrentFont( font )
        mb.setFontUnderline( self.uline )
        mb.setTextColor( color )
        mb.append( text )
	mb.moveCursor(QtGui.QTextCursor.End)

    def MsgFile( self, filename ):
        file = open(filename)
	while 1:
	     line = file.readline()
	     if not line:
			break
	     line = line.replace("\n", "")
	     line = line.replace("\t", "       ")
	     self.MsgText( line, 1 );
				
    def MsgClear( self ):
   	self.te1.clear()
       
    def return_pressed(self):
        env.returnPressed=True
	self.run_command()
	
    def run_command(self):
        # Maintain list of plots
        env.VisitPlots.Visit_UpdatePlotList()

	if ( env.cmdSelected==True and env.returnPressed==False ):
             self.input.setText("GrizIt:"+env.selectedCmd)          
	     return
     
	if ( env.cmdSelected==True and env.returnPressed==True ):
	     self.input.setText("GrizIt:")
             ParseCommand(str(env.selectedCmd))
	     env.cmdSelected=False
             env.selectedCmd=""
             env.returnPressed=False
	     return
                  
        cmd = str(self.input.text())
        cmd = cmd.replace('GrizIt:', '')
	
	self.updateCmdLine( "" )
	
       	ParseCommand("-copyrt clear") 
	
        ParseCommand(str(cmd))
	if ( env.exit == True ):
	     sys.exit(1)
	     
        env.returnPressed=False
	
    def updateCmdLine( self, cmd ):
        self.input.setText( "GrizIt:"+cmd )
		    
    def create_toplevel_menus(self):
	numClasses = len(session.MiliClassNames)

        menubar = self.menuBar()
        
# FILE MENU
	##########################
        # Create File Sub-menu 
	##########################
        fileMenu = menubar.addMenu('&File')
        fileMenu.setTearOffEnabled(True)
        fileMenu.setWindowTitle("File Menu")

        openAction = QAction("O&pen", self)
        setAction(openAction, openFile_CB, "Ctrl+o", None, "Open File",
                  False, "triggered()")
        openAction.setIcon(QIcon(QPixmap("./Icons/FileOpen.png").scaled(20,20)))
        openAction.setIconVisibleInMenu(True)
        fileMenu.addAction(openAction)

        exitAction = QAction("Q&uit", self)
        setAction(exitAction, Quit_CB, "Ctrl+q", None, "Exit application",
                  False, "triggered()")

	exitAction.setIcon(QIcon(QPixmap("./Icons/Exit.png").scaled(20,20)))
        exitAction.setIconVisibleInMenu(True)
	exitAction.setIcon(QIcon(QBitmap("./Icons/Bitmaps/GrizCheck").scaled(20,20)))
        exitAction.setIconVisibleInMenu(True)
        fileMenu.addAction(exitAction)

# CONTROL MENU
	##########################
        # Create Control Sub-menu 
	##########################
        controlMenu = menubar.addMenu('&Control') 
        controlMenu.setTearOffEnabled(True)
        controlMenu.setWindowTitle("Control Menu")
       
        copyRightAction = QAction("&Copyright", self)
        setAction(copyRightAction, Copyright_CB, None, None, "Display Copyrite Notice",
                  True, "triggered()")
        copyRightAction.setIcon(QIcon(QPixmap("./Icons/Editor.png").scaled(20,20)))
        copyRightAction.setIconVisibleInMenu(True)

        controlMenu.addAction(copyRightAction)
        
 	utilityPanelAction = QAction("&Utility Panel", self)
        setAction(utilityPanelAction, UtilityPanel_CB, "Ctrl+u", None, "Popup Utility Panel",
                  True, "triggered()")
	utilityPanelAction.setIcon(QIcon(QPixmap("./Icons/Pencil.png").scaled(20,20)))
        utilityPanelAction.setIconVisibleInMenu(True)
        controlMenu.addAction(utilityPanelAction)

	materialPanelAction = QAction("&Material Manager", self)
        setAction(materialPanelAction, MaterialPanel_CB, "Ctrl+m", None, "Popup Material Panel",
                  True, "triggered()")
	materialPanelAction.setIcon(QIcon(QPixmap("./Icons/Exit.png").scaled(20,20)))
        materialPanelAction.setIconVisibleInMenu(True)
        controlMenu.addAction(materialPanelAction)

        controlMenu.addSeparator() # -------------------------------------------------------
 
        saveSessionGlobalAction = QAction("S&ave Session (Global)", self)
	saveSessionGlobalAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/save_session_global.png").scaled(20,20)))
        saveSessionGlobalAction.setIconVisibleInMenu(True)
        setAction(saveSessionGlobalAction, SaveSessionGlobal_CB, None, None, "Save global session data to a file",
                  True, "triggered()")
        controlMenu.addAction(saveSessionGlobalAction)

        saveSessionLocalAction = QAction("S&ave Session (Local)", self)
	saveSessionLocalAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/save_session_local.png").scaled(20,20)))
        saveSessionLocalAction.setIconVisibleInMenu(True)
        setAction(saveSessionLocalAction, SaveSessionLocal_CB, None, None, "Save local (plot) session data to a file",
                  True, "triggered()")
        controlMenu.addAction(saveSessionLocalAction)

        controlMenu.addSeparator() # -------------------------------------------------------
	
        loadSessionGlobalAction = QAction("L&oad Session (Global)", self)
	loadSessionGlobalAction.setIcon(QIcon(QPixmap("./Icons/VisitIcons/db_open2.xpm").scaled(40,40)))
        loadSessionGlobalAction.setIconVisibleInMenu(True)
        setAction(loadSessionGlobalAction, LoadSessionGlobal_CB, None, None, "Load global session data from a file",
                  True, "triggered()")
        controlMenu.addAction(loadSessionGlobalAction)

        loadSessionLocalAction = QAction("L&oad Session (Local)", self)
	loadSessionLocalAction.setIcon(QIcon(QPixmap("./Icons/VisitIcons/db_open.xpm").scaled(40,40)))
        loadSessionLocalAction.setIconVisibleInMenu(True)
        setAction(loadSessionLocalAction, LoadSessionLocal_CB, None, None, "Load local (plot) session data from a file",
                  True, "triggered()")
        controlMenu.addAction(loadSessionLocalAction)

        controlMenu.addSeparator() # -------------------------------------------------------
 
        controlMenu.addAction(exitAction)

# RENDERING MENU
	############################
        # Create Rendering Sub-menu
	############################
        renderingMenu = menubar.addMenu('&Rendering')
        renderingMenu.setTearOffEnabled(True)
        renderingMenu.setWindowTitle("Rendering Menu")

	# Draw Solid
        drawSolidAction = QAction("&Draw Solid", self)
        setAction(drawSolidAction, DrawSolid_CB, 'Ctrl+S', None, "Draw Solid - no mesh",
                  False, "triggered()")
        drawSolidAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/draw_solid.png").scaled(40,40)))
        drawSolidAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(drawSolidAction)

	# Draw Hidden    
        drawHiddenAction = QAction("&Draw Hidden", self)
        setAction(drawHiddenAction, DrawHidden_CB, 'Ctrl+H', None, "Draw Hidden - with mesh",
                  False, "triggered()")
        drawHiddenAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/ghost.png").scaled(40,40))) # There is also a draw hidden icon
        drawHiddenAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(drawHiddenAction)

	# Draw Wireframe
        drawWfAction = QAction("&Draw Wireframe", self)
        setAction(drawWfAction, DrawWireframe_CB, None, None, "Draw Wireframe",
                  False, "triggered()")
        drawWfAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/wireframe.png").scaled(40,40)))
        drawWfAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(drawWfAction)

	renderingMenu.addSeparator() # -------------------------------------------------------

	# Coord System
        coordAction = QAction("&Coord System (On/Off)", self)
        setAction(drawWfAction, CoordSystem_CB,  'Ctrl+C,Ctrl+S', None, "Show Plot Coord System",
                  False, "triggered()")
        coordAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/button-blue.png").scaled(40,40)))
        coordAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(coordAction)

	# Title
        titleAction = QAction("&Title (On/Off)", self)
        setAction(titleAction, Title_CB,  'Ctrl+C,Ctrl+T', None, "Show Plot Title",
                  False, "triggered()")
        titleAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/title.png").scaled(40,40)))
        titleAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(titleAction)

	# Time
        timeAction = QAction("&Time (On/Off)", self)
        setAction(timeAction, Time_CB, None, None, "Show Plot Time",
                  False, "triggered()")
        timeAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/time.png").scaled(40,40)))
        timeAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(timeAction)

	# Colormap
        colormapAction = QAction("&Colormap (On/Off)", self)
        setAction(colormapAction, Colormap_CB, None, None, "Show Plot Colormap",
                  False, "triggered()")
        colormapAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/colormap.png").scaled(40,40)))
        colormapAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(colormapAction)

	# Min/Max
        minMaxAction = QAction("&Min/Max (On/Off)", self)
        setAction(minMaxAction, MinMax_CB, None, None, "Show Plot MinMax",
                  False, "triggered()")
        minMaxAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/button-blue.png").scaled(40,40)))
        minMaxAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(minMaxAction)

	# Disp Scale
        dispAction = QAction("&Disp Scale (On/Off)", self)
        setAction(dispAction, DispScale_CB, None, None, "Show Plot Disp Scale",
                  False, "triggered()")
        dispAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/disp_scale.png").scaled(40,40)))
        dispAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(dispAction)

	# Error Indicator
        eiAction = QAction("&Error Indicator (On/Off)", self)
        setAction(eiAction, EI_CB, None, None, "Show Error Indicator Plot",
                  False, "triggered()")
        eiAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/error_indicator.png").scaled(40,40)))
        eiAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(eiAction)

	# All
        allAction = QAction("&All (On/Off)", self)
        setAction(allAction, All_CB, None, None, "Show All",
                  False, "triggered()")
        allAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/button-blue.png").scaled(40,40)))
        allAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(allAction)

	# Bound Box
        bboxAction = QAction("&Bound Box (On/Off)", self)
        setAction(bboxAction, Bbox_CB, None, None, "Show Bounding Box",
                  False, "triggered()")
        bboxAction.setIcon(QIcon(QPixmap("./Icons/VisitIcons/boxtool.xpm").scaled(40,40)))
        bboxAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(bboxAction)

	# Edges
        edgesAction = QAction("&Edges (On/Off)", self)
        setAction(edgesAction, Edges_CB, None, None, "Show Edges",
                  False, "triggered()")
        edgesAction.setIcon(QIcon(QPixmap("./Icons/VisitIcons/perspectiveoff.xpm").scaled(40,40)))
        edgesAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(edgesAction)

	# Greyscale
        gsAction = QAction("&Greyscale (On/Off)", self)
        setAction(gsAction, GS_CB, None, None, "Show Greyscale",
                  False, "triggered()")
        gsAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/greyscale.png").scaled(40,40)))
        gsAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(gsAction)

	renderingMenu.addSeparator() # -------------------------------------------------------

	# Perspective
        perspAction = QAction("&Perspective (On/Off)", self)
        setAction(perspAction, Persp_CB, None, None, "Show Perspective",
                  False, "triggered()")
        perspAction.setIcon(QIcon(QPixmap("./Icons/VisitIcons/perspectiveon.xpm").scaled(40,40)))
        perspAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(perspAction)

	# Orthographic
        orthoAction = QAction("&Orthographic (On/Off)", self)
        setAction(orthoAction, Ortho_CB, None, None, "Show Orthographic",
                  False, "triggered()")
        orthoAction.setIcon(QIcon(QPixmap("./Icons/Pencil.png").scaled(40,40)))
        orthoAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(orthoAction)

	renderingMenu.addSeparator() # -------------------------------------------------------

	# Near Far
        rnfAction = QAction("&Adjust Near/Far", self)
        setAction(rnfAction, Rnf_CB, None, None, "Adjust Near/Far",
                  False, "triggered()")
        rnfAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/nearfar.png").scaled(40,40)))
        rnfAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(rnfAction)

	renderingMenu.addSeparator() # -------------------------------------------------------

	# Reset
        rvAction = QAction("R&eset View (On/Off)", self)
        setAction(rvAction, ResetView_CB, None, None, "Reset View",
                  False, "triggered()")
        rvAction.setIcon(QIcon(QPixmap("./Icons/VisitIcons/db_reopen.xpm").scaled(40,40)))
        rvAction.setIconVisibleInMenu(True)
       	renderingMenu.addAction(rvAction)

	renderingMenu.addSeparator() # -------------------------------------------------------

	# Set Colormap
        cmMenu = renderingMenu.addMenu('&Set Colormap')
        cmMenu.setTearOffEnabled(True)

	cmAction = QAction("Inverse Colormap", self)
        setAction(cmAction, lambda mapName="invmap": SetCm_CB(mapName), None, None, "invMap",
		      False, "triggered()")
        cmAction.setIcon(QIcon(QPixmap("./Icons/Mouse.png").scaled(100,100)))
        cmAction.setIconVisibleInMenu(True)
        cmMenu.addAction(cmAction) 

# PICKING MENU
	################
        # Pick Sub-Menu
	################
        pickingMenu = menubar.addMenu('&Picking')
        pickingMenu.setTearOffEnabled(True)
        pickingMenu.setWindowTitle("Picking Menu")

	# Mouse Hilite
        mouseHiliteAction = QAction("&Mouse Hilite", self)

        setAction(mouseHiliteAction, MouseHilite_CB, None, None, "Mouse Hilite",
                  False, "triggered()")

        mouseHiliteAction.setIcon(QIcon("./Icons/VisitIcons/pick.xpm"))
        mouseHiliteAction.setIconVisibleInMenu(True)

	pickingMenu.addAction(mouseHiliteAction)

	# Mouse Select
        mouseSelectAction = QAction("&Mouse Select", self)
        setAction(mouseSelectAction, MouseSelect_CB, None, None, "Mouse Select",
                  False, "triggered()")

        mouseSelectAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/button-red.png").scaled(100,100)))
        mouseSelectAction.setIconVisibleInMenu(True)
	pickingMenu.addAction(mouseSelectAction)

	pickingMenu.addSeparator() # -------------------------------------------------------

	# Clear Hilite
        clearHiliteAction = QAction("&Clear Hilite", self)
        setAction(clearHiliteAction, ClrHilite_CB, None, None, "Clear Hilite",
                  False, "triggered()")

        clearHiliteAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/clear.png").scaled(100,100)))
        clearHiliteAction.setIconVisibleInMenu(True)
	pickingMenu.addAction(clearHiliteAction)

	# Clear Select
        clearSelectAction = QAction("&Clear Select", self)
        setAction(clearSelectAction, ClrSelect_CB, None, None, "Clear Select",
                  False, "triggered()")

        clearSelectAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/clear.png").scaled(100,100)))
        clearSelectAction.setIconVisibleInMenu(True)
	pickingMenu.addAction(clearSelectAction)

	pickingMenu.addSeparator() # -------------------------------------------------------

	# Set Btn1 Pick
        btn1Menu = pickingMenu.addMenu('&Set Btn 1 Pick')
        btn1Menu.setTearOffEnabled(True)
	
	d = GS.DebugMsg("\nSessionMiliClassNames=")
	for name in session.MiliClassNames:
	    d = GS.DebugMsg(session.MiliClassNames)
	    
	for className in session.MiliClassNames:
            d = GS.DebugMsg( "\nClassName="+ className)
            classAction = QAction("&"+className, self)
	    setAction(classAction, lambda node=className: SetBtn1Pick_CB(node), None, None, className,
		      False, "triggered()")
            classAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/button-seagreen.png").scaled(100,100)))
            classAction.setIconVisibleInMenu(True)
	    btn1Menu.addAction(classAction) 

	# Set Btn2 Pick
        btn2Menu = pickingMenu.addMenu('&Set Btn 2 Pick')
        btn2Menu.setTearOffEnabled(True)
	for className in session.MiliClassNames:
            classAction = QAction("&"+className, self)
	    setAction(classAction, lambda node=className: SetBtn2Pick_CB(node), None, None, className,
		      False, "triggered()")
            classAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/button-purple.png").scaled(100,100)))
            classAction.setIconVisibleInMenu(True)
	    btn2Menu.addAction(classAction) 

	# Set Btn3 Pick
        btn3Menu = pickingMenu.addMenu('&Set Btn 3 Pick')
        btn3Menu.setTearOffEnabled(True)
	for className in session.MiliClassNames:
            classAction = QAction("&"+className, self)
	    setAction(classAction, lambda node=className: SetBtn3Pick_CB(node), None, None, className,
		      False, "triggered()")
            classAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/button-green.png").scaled(100,100)))
            classAction.setIconVisibleInMenu(True)
	    btn3Menu.addAction(classAction) 

	pickingMenu.addSeparator() # -------------------------------------------------------

	# Center Hilite On
        ctrOnAction = QAction("&Center Hilite On", self)
        setAction(ctrOnAction, CtrHiliteOn_CB, None, None, "Center Hilite On",
                  False, "triggered()")

        ctrOnAction.setIcon(QIcon(QPixmap("./Icons/Mouse.png").scaled(100,100)))
        ctrOnAction.setIconVisibleInMenu(True)
	pickingMenu.addAction(ctrOnAction)
	
# DERIVED VARIABLES MENU
	###################
        # Derived Sub-Menu
	###################
        derivedMenu = menubar.addMenu('&Derived')
        derivedMenu.setTearOffEnabled(True)
        derivedMenu.setWindowTitle("Derived Menu")

	# Result Off
        derivedAction = QAction("&Result Off", self)
        setAction(derivedAction, ResultOff_CB, None, None, "Result Off",
                  False, "triggered()")

        derivedAction.setIcon(QIcon(QPixmap("./Icons/Mouse.png").scaled(100,100)))
        derivedAction.setIconVisibleInMenu(True)
	derivedMenu.addAction(derivedAction)
        
# PRIMAL VARIABLES MENU
	##################
        # Primal Sub-Menu
	##################
        primalMenu = menubar.addMenu('&Primal')
        primalMenu.setTearOffEnabled(True)
        primalMenu.setWindowTitle("Primal Menu")
        
	# Result Off
        primalAction = QAction("&Result Off", self)
        setAction(primalAction, ResultOff_CB, None, None, "Result Off",
                  False, "triggered()")

        primalAction.setIcon(QIcon(QPixmap("./Icons/Mouse.png").scaled(100,100)))
        primalAction.setIconVisibleInMenu(True)
	primalMenu.addAction(primalAction)

# TIME MENU
	##################
        # Time Sub-Menu
	##################
        timeMenu = menubar.addMenu('&Time')
        timeMenu.setTearOffEnabled(True)
        timeMenu.setWindowTitle("Time Menu")

	# Next State
        timeAction = QAction("&Next State", self)
        setAction(timeAction, NextState_CB, None, None, "Next State",
                  False, "triggered()")

        timeAction.setIcon(QIcon(QPixmap("./Icons/VisitIcons/animationforwardstep.xpm").scaled(70,70)))
        timeAction.setIconVisibleInMenu(True)
	timeMenu.addAction(timeAction)

	# Previous State
        timeAction = QAction("&Previous State", self)
        setAction(timeAction, PreviousState_CB, None, None, "Previous State",
                  False, "triggered()")

        timeAction.setIcon(QIcon(QPixmap("./Icons/VisitIcons/animationreversestep.xpm").scaled(70,70)))
        timeAction.setIconVisibleInMenu(True)
	timeMenu.addAction(timeAction)

	# First State
        timeAction = QAction("&First State", self)
        setAction(timeAction, FirstState_CB, None, None, "First State",
                  False, "triggered()")

        timeAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/first.png").scaled(20,20)))
        timeAction.setIconVisibleInMenu(True)
	timeMenu.addAction(timeAction)

	# Last State
        timeAction = QAction("&Last State", self)
        setAction(timeAction, LastState_CB, None, None, "Last State",
                  False, "triggered()")

        timeAction.setIcon(QIcon(QPixmap("./Icons/OpenSource/last.png").scaled(20,20)))
        timeAction.setIconVisibleInMenu(True)
	timeMenu.addAction(timeAction)

	# Animate States
        timeAction = QAction("&Animate States", self)
        setAction(timeAction, AnimStates_CB, None, None, "Animate States",
                  False, "triggered()")

        timeAction.setIcon(QIcon(QPixmap("./Icons/VisitIcons/animationplayoff.xpm").scaled(70,70)))
        timeAction.setIconVisibleInMenu(True)
	timeMenu.addAction(timeAction)

	# Stop Animate
        timeAction = QAction("&Stop Animate", self)
        setAction(timeAction, StopAnim_CB, None, None, "Stop Animate",
                  False, "triggered()")

        timeAction.setIcon(QIcon(QPixmap("./Icons/VisitIcons/animationstopoff.xpm").scaled(70,70)))
        timeAction.setIconVisibleInMenu(True)
	timeMenu.addAction(timeAction)

	# Continue Animate
        timeAction = QAction("&Continue Animate", self)
        setAction(timeAction, ContAnim_CB, None, None, "Continue Animate",
                  False, "triggered()")

        timeAction.setIcon(QIcon(QPixmap("./Icons/VisitIcons/animationplayon.xpm").scaled(100,100)))
        timeAction.setIconVisibleInMenu(True)
	timeMenu.addAction(timeAction)
	
# PLOTTING MENU
        ################
        # Plot Sub-Menu
	################
        plotMenu = menubar.addMenu('&Plot')
        plotMenu.setTearOffEnabled(True)
        plotMenu.setWindowTitle("Plot Menu")
       
	# Time History Plot
        plotAction = QAction("&Time Hist Plot", self)
        setAction(plotAction, TH_CB, None, None, "Time Hist Plot",
                  False, "triggered()")

        plotAction.setIcon(QIcon(QPixmap("./Icons/Mouse.png").scaled(100,100)))
        plotAction.setIconVisibleInMenu(True)
	plotMenu.addAction(plotAction)
          
# HELP MENU
	################
        # Help Sub-Menu
	################
        helpMenu = menubar.addMenu('&Help')
        helpMenu.setTearOffEnabled(True)
        helpMenu.setWindowTitle("Help Menu")
       
	# Display Griz Manual
        helpAction = QAction("&Time Hist Plo", self)
        setAction(helpAction, TH_CB, None, None, "Time Hist Plot",
                  False, "triggered()")

        helpAction.setIcon(QIcon(QPixmap("./Icons/VisitIcons/help.xpm").scaled(100,100)))
        helpAction.setIconVisibleInMenu(True)
	helpMenu.addAction(helpAction)

########################################
# Event Handlers
########################################
    def eventFilter(self, obj, event):
        log = GS.CmdLog()
	if ( event.type()==QEvent.KeyPress) and (event.key()==Qt.Key_Up):
	     cmd = log.pop()
             self.updateCmdLine(cmd)      

	if ( event.type()==QEvent.KeyPress) and (event.key()==Qt.Key_Down):
	     cmd = log.push()
	     self.updateCmdLine(cmd)

 	if (not self.te2):
	    return False
    
	if ( obj == self.te2 ):
	     if ( event.type()==QEvent.KeyPress) and (event.key()==Qt.Key_Return):
	          self.return_pressed() 
	return False

def closeGUI():
    deleteWin( env.mainWin )
    deleteWin( env.CRWin )
    deleteWin( env.msg )
     
def deleteWin( win ):
    if ( win != None ):
	 win.hide()
	 env.mainWin.destroy()
	 env.mainWin.hide()

def main():
    app = QtGui.QApplication(sys.argv)
    ex = CMDWindow()
    app.exec_()
    a=True
    i=0
    while a:
     i = i+1
    

if __name__ == '__main__':
    main()
