#! /usr/local/bin/python
#
############################################################################
# InstallVisitPlugin.py - Installs links to MDG Visit-Mili plugin into
# users home .visit directory.
# 
#      Peter Tonner
#      Lawrence Livermore National Laboratory
#      June 10, 2011
#
############################################################################
# Modifications:
#  I. R. Corey - June 16, 2011: 
#
############################################################################
#

import os,subprocess

path = os.environ['HOME'] + "/.visit/"
libPath = "/usr/apps/mdg/archive/visitdir/"
pluginPath = "plugins/databases/"

linkFiles = ["libEMiliDatabase_par.so",
	     "libEMiliDatabase_ser.so",
	     "libIMiliDatabase.so",
	     "libMMiliDatabase.so"]

# Checks to see if a directory exists, returns True if it is found, False otherwise
def checkForDirectory(p):
	try:
		os.listdir(p)
	except OSError:
		return False
	return True


##########################
# USAGE
##########################
def usage():
	print "python visitPlugins.py {-u | -a | -l | -h} -p <plugin path> -f <file list>"
	print "Options:"
	print "\t-u : undo plugin linking, -a and -f are still valid parameters"
	print "\t-a : operate on all files found in the plugin path"
	print "\t-l : list plugin files found and exit"
	print "\t-h, --help : print this menu and exit"
	print "\t-p : specify the plugin path (/usr/apps/mdg/bin/visit/<architecture>/ by default)"
	print "\t-f : specify plugins to link using a comma separated list"
	print "\t--alpha : install alpha version of plugin"
	print "\t--beta : install alpha version of plugin"
	
##########################
# MAIN
##########################
if __name__ == "__main__":
	# Read command line arguments
	import getopt,sys
	undo = False
	copyAll = False
	List = False
	changeLib = False
	overwrite = False
	
        version="V11_01"
        alpha=False
	beta=False
	
	try:
	  opts,args = getopt.getopt(sys.argv[1:], "up:af:lho", ["help", "alpha", "beta"])
	except getopt.GetoptError:
	  usage()
	  sys.exit()
		
	for opt,val in opts:
		if opt == "-u":
			undo = True
		elif opt == "-p":
			libPath = val
			changeLib = True
			version = val
		elif opt == "-a":
			copyAll = True
		elif opt == "-f":
			linkFiles = val.split(',')
		elif opt == "-l":
			List = True
		elif opt in ("-h", "-help"):
                        usage()
			sys.exit()
		elif opt == "-o":
			overwrite = True
		if opt == "--alpha":
			version = "alpha"
			print "\n** Installing Alpha version of Mili Plugin **\n"
			alpha = True
		if opt == "--beta":
			version = "beta"
			print "\n** Installing Beta version of Mili Plugin **\n"
	                beta = True
			
        if ( alpha==False and beta==False ):
	      print "\n** Installing Production version of Mili Plugin **\n"
		
	# Setup string representing system architecture
	osType = os.environ['OSTYPE']
	arch = osType
	hostType = os.environ['HOSTTYPE']
	if hostType == "x86_64-linux":
		arch = "linux-x86_64"	

	print "Checking system type... " + arch
	
	# Update the path locations
	path = path + arch + "/" + pluginPath
	if not changeLib:
	   libPath = libPath + version + "/" + arch + "/" + pluginPath
		
	# List all of the plugin files existing in the home directory
	if List:
		print "\nListing plugins in home library... "
		linkFiles = os.listdir(path)
		for file in linkFiles:
		    print "\t" + path + file
			
	# Move plugin files in the home directory
	# List is specified in the initial linkFile list (default),
	# comma separated list from -f option, or all files (-a option)		
	elif undo:
		print "Checking for local .visit directory... " + path
		if checkForDirectory(path):
			if copyAll:
				linkFiles = os.listdir(path)
				
			# Keep only files that exist and are links
			linkFiles = [x for x in linkFiles if os.path.exists(path + x)]
			for file in linkFiles:
				if os.path.islink(path + file):
					print "Removing link... " + path + file
					os.unlink(path + file)
		else:
			print "Folder " + path + " does not exist! Nothing to remove..."
			
	# Link files from the library directory ("/usr/apps/mdg/bin/visit/" by default)
	else:
		print "Checking for local visit directory... " + path
		if not checkForDirectory(path):
			print "Directory " + path + " does not exist, creating directory."
			os.makedirs(path)
			
		if copyAll:
		   linkFiles = os.listdir(libPath)	
			
		if not overwrite and os.path.exists(path + linkFiles[3]):
	             input = raw_input("\nOverwrite local Mili Plugin [y|n]?")
		     if ( input=="y" ):
		          overwrite=True
		     else:
			 print "\nExiting - No links to make" 
	
		for file in linkFiles:
			if os.path.exists(libPath + file):
				if overwrite and os.path.exists(path + file):
				   os.rename(path + file, path + file + "-SAVE")
				   
				if not os.path.exists(path + file):
					print "Create link... " + libPath + file + " to " + path + file
					os.symlink(libPath + file,path + file)
			else:
				print libPath + file + " does not exist, no link created."

