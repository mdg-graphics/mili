#!/usr/gapps/visit/python/2.6.4/linux-x86_64_gcc-4.1/bin/python
#
############################################################################
# grizIt_VisitInfo.py - Prints various Visit internal data structures.
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      August 24, 2010
#
# 
############################################################################
# Modifications:
#  I. R. Corey - August 24, 2010: Created.
#
############################################################################
#
visitPath="/usr/gapps/visit/2.5.2/linux-x86_64/lib/site-packages"
visitVersion="2.5.2"

import pdb, sys
import os, sys, re, bisect, math, copy
sys.path.append(visitPath)
import visit

import grizIt_Session as S

if __name__ == "__main__":
    visit.AddArgument("-v 2.5.2")
    visit.AddArgument("-nosplash")
    visit.AddArgument("-nowin")
            
    visit.Launch()

    dirInfo = dir(visit)
    for s in dirInfo:
        p = S.PrintMsg(s)

    names = visit.GetCallbackNames()
    p = S.PrintMsg("\n\n\n\nCallback names = "+names)
