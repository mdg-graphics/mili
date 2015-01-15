#! /usr/local/bin/python

import sys,string,os,math,fpformat,popen2
from string import Template


file = popen2.Popen4("uname -p")
outfile = file.fromchild
processor = outfile.readline().strip()
outfile.close()

file = popen2.Popen4("uname -s")
outfile = file.fromchild
ostype = outfile.readline().strip().lower()
outfile.close()
username = None
if "USER" in os.environ.keys():
	 username = os.environ["USER"]
if "SYS_TYPE" in os.environ.keys():
	osname = os.environ["SYS_TYPE"]
else:
	osname = ostype
file = popen2.Popen4("uname -n")
outfile = file.fromchild
nodename = outfile.readline().strip().lower()

if osname == "sles_10_ppc64":
	ostype = "linux3"
par_system = ostype

preawk = ""
if ostype == "sunos":
	preawk = "n"
if ostype == "aix":
	node_count = "4"
dir_prefix = "%s" % (osname)
absolute_tol_value = "15"
default_platform = "chaos_3_x86_64"
#################################################################################
#
#   Define the parallel drivers for the specific platform
#   the array following the os is as follows
#   [0] executable name
#   [1] A template defining the parameters passed to the code It include
#            $processors -- then number of processors
#            $partition -- nodes or partitions
#            $CODE -- comes from the defintion provided in the mapping
#   [2] Array of the pools [0] is the debug pool for interactive [1] is the batch pool these are determined automatically by
#       the code querying the environment.
#

parallel_drivers = {"linux":["srun",  Template(" -N $nodes -n $processors $partition $CODE"),["-ppdebug",""]],
		   "linux2":["mpirun",Template(" -np $processors $partition $CODE"),["",""]],
		   "linux3":["mpirun",Template(" -np $processors $partition -exe $CODE"),["",""]],
		      "aix":["poe",   Template(" $CODE -procs $processors -nodes $nodes $partition" ),["-rmpool pdebug"," "]]}

#
################################################################################
