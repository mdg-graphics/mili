#!/usr/bin/env /grdev/regrtest/bin/python


#import os,re,string
from rtest_support import *

"""This routine does some basic initialization for the
regression testing.

A griz binary is called to generate an ascii file with
*/.info* about the sample file. The file is then parsed
to determine the relevant elements.

***This only needs to be run once for each sample***
"""

generate=1

GRIZBIN=TEST['bin'][0]
DB='d3plot'
for sample in CASE['SAMP']+CASE['SND']:
   print("Analyzing data for "+sample+"...")
   os.chdir(RHOME+sample+"/")
   if generate:
      g=open('grizinit','w+')    #create grizinit file 
      g.write(HEADER)
      g.write("savtxt .info\n")
      g.write("info\n")
      g.write("endtxt\n")
      g.write("savtxt done\n")
      g.write("endtxt\n")
      g.write("quit\n")
      g.close()
      err=os.system(GRIZBIN+" -i "+DB)   #run griz with new grizinit file
      if not os.path.exists('done'):
         print("waiting ...")
      while not os.path.exists('done'):
         pass
      os.remove('done')
      os.remove('grizinit')
   #Now parse .info file
   if not os.path.exists('.info'):
      print("ERROR: No info file found.")
   info=open('.info','r+')
   raw = info.read() 
   info.close()
   #
   for key in IS.keys():
      etest = re.compile(IS[key][0])
      if etest.search(raw):
         np=string.replace(IS[key][0],':','')
         print(">>> "+np+" detected in "+sample)

