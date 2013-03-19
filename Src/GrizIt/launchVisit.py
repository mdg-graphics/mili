#!/usr/gapps/visit/python/2.6.4/linux-x86_64_gcc-4.1/bin/python
# !/usr/local/bin/python
#
############################################################################
# launchVisit.py - Launches Visit
#
# Ivan R. Corey
# Lawrence Livermore National Laboratory
# May 07, 2010
#
#
############################################################################
# Modifications:
# I. R. Corey - May 07, 2010: Created.
#
############################################################################

import os, sys, re, bisect, math
sys.path.append("/usr/local/lib/python2.6/site-packages")
sys.path.append("/usr/gapps/visit/2.5.2/linux-x86_64/lib/site-packages")
import visit

def When_Point(atts):
    print "\nPick event\n\n"
#    a = visit.GetPickOutput()
#    print "\nPick=", a
#end def

def When_TimeSliderNextState(atts):
    print "\n\n\nNext State\n\n"
#end def

visit.AddArgument("-v 2.5.2")
visit.AddArgument("-pyside_viewer")
visit.AddArgument("-pyside")
visit.Launch()
visit.ShowAllWindows()

visit.RegisterCallback("PickAttributes",         When_Point)
visit.RegisterCallback("TimeSliderNextStateRPC", When_TimeSliderNextState)
visit.OpenGUI()
visit.SuppressQueryOutputOn()

command = raw_input("\n\nGrizIt> ")
sys.exit(1)
