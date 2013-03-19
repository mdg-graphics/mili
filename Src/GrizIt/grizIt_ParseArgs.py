#
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

import pdb;
import optparse
import sys
import visit

from grizIt_Main import *
from grizIt_Env  import *
import grizIt_Session as S

#***********************************************************************************************************************


#***********************************************************************************************************************

def Parse_Args( env ):
    parser = optparse.OptionParser()

    parser.add_option('-i',                                     dest="filename" )
    parser.add_option('-f',                action="store_true", dest="run_foreground", default=False )
    parser.add_option('-b', '--batch',     action="store_true", dest="run_batch",      default=False )
    parser.add_option('-c', '--climode',   action="store_true", dest="cli_mode",       default=False )
    parser.add_option('-v', '--visitgui',  action="store_true", dest="visit_gui",      default=False )
    parser.add_option(      '--visitbeta', action="store_true", dest="visit_beta",     default=False )
    parser.add_option(      '--visitver',                       dest="visit_version" )
    parser.add_option('-s', '--serial',    action="store_true",  dest="visit_serial",  default=False )
    parser.add_option('-p', '--partition',                      dest="part_file" )
    parser.add_option('-g', '--gid',                            dest="gid" )
    parser.add_option('-V', '--version',   action="store_true", dest="show_version",   default=False )
    parser.add_option('-t', '--test',                           dest="tests_file")
    parser.add_option('-d', '--debug',     action="store_true", dest="print_debug_messages", default=False )
    parser.add_option('',   '--tv',        action="store_true",   dest="debug_mode",   default=False )
    parser.add_option('-u', '--util',      action="store_true", dest="util_panel",   default=False )
    parser.add_option('-m', '--mat',       action="store_true", dest="mat_panel",   default=False )

    (options, rem) = parser.parse_args()
  
    # Assign env variables
    
    S.env.visitGUI     = options.visit_gui
    S.env.visitBeta    = options.visit_beta
    S.env.visitVersion = options.visit_version
    S.env.visitSerial  = options.visit_serial
    S.env.CLImode      = options.cli_mode
    S.env.show_utilpanel = options.util_panel
    S.env.show_matpanel  = options.mat_panel
    S.env.debugMode      = options.print_debug_messages

    if ( options.filename != None ):
         S.env.input_filename = options.filename

    if ( options.gid != None ):
         S.env.gid = options.gid

    if ( options.tests_file != None ):
         S.env.tests_file = options.tests_file

    if ( options.tests_file != None ):
         S.env.tests_file = options.tests_file
#end def

def print_grizIt_Usage():
    p = S.PrintMsg("\nUsage[] = grizit -i input_base_name [options]\n")
    p = S.PrintMsg( "\t  [-gid <n>]                     # GrizIt session id #")
    p = S.PrintMsg( "\t  [-f]                           # run in foreground          #")
    p = S.PrintMsg( "\t  [--beta | --alpha]             # run a alpha or beta version of GrizIt #")
    p = S.PrintMsg( "\t  [--batch <command_file>]       # run GrizIt off-line with no render window #")
    p = S.PrintMsg( "\t  [--visitgui]                   # include VisIt Gui #")
    p = S.PrintMsg( "\t  [--visitbeta]                  # run Beta version of Visit #")
    p = S.PrintMsg( "\t  [--visitver]                   # run specific version of Visit (such as 2.0.2 #")
    p = S.PrintMsg( "\t  [-s]                           # run visit in Serial Mode #")
    p = S.PrintMsg( "\t  [-tv | debug]                  # run GrizIt in debug mode #")
    p = S.PrintMsg( "\t  [-w <width_pix> <height_pix>]  # set exact render window size #")
    p = S.PrintMsg( "\t  [-V]                           # print GrizIt version info #")
    p = S.PrintMsg( "\t  [-t]                           # run the GrizIt test suite #\n\n")
#end def

def print_grizIt_Header():
    p = S.PrintMsg("\nRunning GrizIt Version 12.1")
    p = S.PrintMsg( "\tMili Version: 12.1")
    p = S.PrintMsg( "\tVisIt Version:"+visit.Version())
#end def
