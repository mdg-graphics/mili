#!/usr/local/bin/python
#
#  !/usr/gapps/visit/python/2.6.4/linux-x86_64_gcc-4.1/bin/python
#
############################################################################
# grizIt_Main.py - Griz Main Driver.
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
import os, sys, re, bisect, math, copy
sys.path.append("/usr/local/lib/python2.6/site-packages")
sys.path.append("/usr/gapps/visit/python/2.6.4/linux-x86_64_gcc-4.1/lib/python2.6/site-packages")
sys.path.append("/usr/gapps/visit/2.5.2/linux-x86_64/lib/site-packages")

import pdb, random

import grizIt_VisitInfo as VI

from PySide.QtCore import *
from PySide.QtGui import *
from PySide import *  

import grizIt_GuiMain          as GUI
import grizIt_GrizMiscCommands as MISC
import grizIt_GuiDialog
import grizIt_GuiUtil          as UTIL
import grizIt_GuiMaterialMgr   as MATL

import grizIt_ParseArgs      as PA
# import grizIt_GuiPlot        as PLT
from grizIt_Env              import *
import grizIt_Init           as INIT
from grizIt_Utils            import *
import grizIt_TestSuite      as TS 
import grizIt_Mili           as MILI 
import grizIt_VisitPlots     as VP

from grizIt_ParseGrizCommand import *
import grizIt_Session        as S

# session = S.session
# env     = S.env

def main():
    progname = os.path.basename(sys.argv[0])

    commandMaker= TS.NullCommandMaker()

    S.env.setenv()
    PA.Parse_Args( env )
    
    p1 = S.PrintMsg('\nStarting GrizIt Version 1.0')
        
    if ( S.env.batch_mode ):
         visit.AddArgument("-nowin")

    if ( S.env.visitSerial ):
         visit.AddArgument("-noconfig")

    visit.AddArgument("-v "+VI.visitVersion)
    visit.AddArgument("-nosplash")
    
    #
    # This argument is used to debug the
    # Mili Plug-in
    #
    if ( S.env.debug_mode ):
         visit.AddArgument("-totalview engine_ser")

    visit.AddArgument("-pyside")
    visit.AddArgument("-pysideviewer")
    visit.Launch()
    visit.ShowAllWindows()
    
    if ( not S.env.batch_mode and S.env.visitGUI ):
         visit.OpenGUI()
         
    if ( not S.env.batch_mode ):
         #
         # Initialize the GrizIt GUI if interactive mode
         #
         S.env.app = QtGui.QApplication(sys.argv)
         if ( S.env.show_util_panel ): 
              S.env.mainWin = GUI.CMDWindow()
              S.env.msgWin  = grizIt_GuiDialog.msgBox()
              S.env.CR      = grizIt_GuiDialog.msgWindow()
              S.env.CRWin   = grizIt_GuiDialog.msgWindow()
                     
         S.env.VisitPlots=VP.VisitPlots()
         S.env.VisitPlots.Visit_UpdatePlotList()
         S.env.VisitWindows=VP.VisitWindows()
         
         if ( S.env.show_util_panel ):  
              S.env.utilWin = UTIL.UtilWindow()
         if ( S.env.show_mat_panel ):
              S.env.mtlWin  = MATL.MaterialMegPanel()

    #end if
    
    INIT.Initialize_GrizIt_Session(session)

    S.session.miliDB = MILI.Mili()

#
# Display the Release Notes in the message window
#
    # S.env.mainWin.MsgFont( "Courior", "blue", 9, False, True, False, Qt.AlignLeft ) 
    try:
        S.env.mainWin.MsgFile( "Doc/grizIt_start_text" )
    except:
        w = S.WarningMsg("Unable to open grizIt_start_text")

    if ( S.env.tests_file ):
         m = S.PrintMsg('RUNNING TEST FILES')
         commandMaker = TS.getCommand() # Initiates the class and the command list
    
    if ( len(S.env.input_filename)>0):
         visit.OpenDatabase(S.env.input_filename )
         cmd = "load "+S.env.input_filename
         ParseCommand(cmd)
         if ( not S.env.batch_mode ):
              S.env.mainWin.updateCmdLine("")

#    if ( S.env.batch_mode ):
#         visit.OpenComputeEngine()
    
    p = S.PrintMsg(visit.Version())

    # print dir(visit)

    PA.print_grizIt_Header()
    PA.print_grizIt_Usage()
    
#    S.env.THWin   = PLT.PlotWin()

###############################################
# Main command input control loop
###############################################

    #
    # If CLI mode is enabled allow input
    # from shell.
    #
    parseCommands=True

    if S.env.CLImode == False or :
       S.env.app.exec_()
       parseCommands=False
        
    while parseCommands:
        if S.env.exit == True:
            S.env.exit =  False
            parseCommands = False
            continue
    
        # Maintain list of plots
        S.env.VisitPlots.Visit_UpdatePlotList()

        if commandMaker.runTest:
           command = commandMaker.get_command()
        else:
           command = raw_input("GrizIt>")

        if not command:
            continue
        else:
            ParseCommand(command)

        if S.env.exit == True:
           S.env.exit =  False
           parseCommands = False
        #end else
    #end while
 
if __name__ == "__main__":
    main()

###############################################
def perfTest():
#
# Performance test of large list
#
    size=2000000
    numbers=[]
    for i in range(size):
        numbers.append(random.randint(1, size-1))

    p3 = S.PrintMsg("\nStart loading list\n")
    list=[]
    for i in range(size):
        list.append(numbers[i])

    p4 = S.PrintMsg("\nAfter append to list\n\n\n\n")
    i = list.index(size-1000)
    p5 = S.PrintMsg("\ni=\n\n\n\n"+ i)

    p6 = S.PrintMsg("\nStart loading dictionary\n")
    dict={}
    for i in range(size):
        key=numbers[i]
        dict[numbers[i]] = i

    p7 = S.PrintMsg("\nAfter append to disctionary\n\n\n\n")
    i = dict[key]
    p8 = S.PrintMsg("\ni=\n\n\n\n"+ i+ " key="+ key)
    


