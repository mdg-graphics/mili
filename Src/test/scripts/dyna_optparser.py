#! /usr/local/bin/python

import os,string,sys
from analyzer import AnalyzerFactory

parameters = ["-eval","-s","-suite","-t","-test","-p","-b","-n","-c","-e","-d","-v","-k","-ra","-rb","-q","-m","-delete_mili","-html","-basedir","-rundir","-lcinfo","-full_lcinfo"]

defaultEnvironment = "dyna_testsuites"

def usage(message):
	if message == None:
		print "  Help:"
	else:
		print "  Error: %s \n\tusage:" % message
	print """
    scripts/Test.py -c codename [-t testname | -s suitename] [-p #]
                                   [-n #] [-a] [-e #] [-v #] [-k #]
                                   [-d #] [-m (absolute|stddev #)]
                                   [-eval][-basdir dir][-rundir dir]
      -c            Name of code
      -t  -test     Test name 
      -s  -suite    Suite name
      -d            # set the significant digits
      -p            # number of processors integer value(giving just the -p with no value 
                    sets the default -p 8 -n 1)"
      -n            # number of node or partitions integer value
      -b            Generate baselines moves the generated results into the baseline
      -v            Tolerance value
      -e            Set a different environment (example dyna_v6)
      -k            Tolerance type either rel or abs 
      -m            Mode of analysis default absolute. also includes smoke to validate code runs
      -delete_mili  On failing delete the mili plot files generated.
      -eval         Tells the testing suite to evaluate an existing answer. Ignores the -c option
      -q            Put Dyna output to run file instead of standard out
      -plot         Plot the difference using gnuplot creating a jpeg image
      -brief        Print out only pass fail for test and not for each curve
      -basdir dir   Pirectory to locate the baselines. default is current working directory
      -rundir dir   Name of directory ro which you wish run tests in. default is SYS_TYPE
      -lcinfo       Upon failing due to a run error prints out some Livermore Computing info
	  
	  """
#    The following have not yet been implemented
#      -f  load the following file and run this set of problems.
#      -ra remove after all test results and directories  for this SYS_TYPE.
#      -rm remove before all test results and directories  for this SYS_TYPE.
#		
#	"""
	
def parseParameters(params):
	if "-h" in params:
		usage(None)
		sys.exit(0)
	if len(params) <3:
		usage("Too few arguments")
		sys.exit(101)
	
	suites = []	
	length = len(params)
	i = 0
	suite = None
	test = None
	processors = None
	nodes = None
	parallel = False
	code = None
	absolute_tolerance = False
	genBaseline = False
	envir = defaultEnvironment
	sig_dig = "16"
	tolerance = 0.0#dyna_env.default_tolerance
	tol_type = "abs"#dyna_env.default_tol_type
	do_sigs = True
	removeAllBefore = False
	removeAllAfter = False
	test_mode = "absolute"
	test_mode_set = False
	stddev = "2"
	kmili = True
	types = ["log"]
	evaluate = False
	quiet = False
	basedir = None
	testdir = None
	defaultProcessors="8"
	defaultNodes="1"
	print_added=False
	full_info = False
	brief=False
	plot=False
	test_type="curve"
	
	while i < length:
		if params[i].startswith("-s") or params[i].startswith("-suite"):
			if isParameter(params[i]):
				if i+1 < length and notParameter(params[i+1]): 
					suites.append(params[i+1])
					i = i+1
				else:
					usage("you must give a name with the suite option")
					return None,None
			else:
				if params[i].startswith("-suite"):
					suites.append(getString(len("-suite"),params[i]))
				else:
					suites.append(getString(len("-s"),params[i]))

			while (i< length-1) and not isParameter(params[i+1]) and not params[i+1].startswith("-") :
				## assume a list of suites
				suites.append(params[i+1])
				i = i+1
				
		elif params[i].startswith("-plot"):
			plot = True
		elif params[i].startswith("-lcinfo"):
			print_added = True
		elif params[i].startswith("-full_lcinfo"):
			print_added = True
			full_info = True
		elif params[i].startswith("-basedir"):
			if isParameter(params[i]):
				if i+1 < length and notParameter(params[i+1]): 
					basedir = params[i+1]
					i = i+1
				else:
					usage("you must give a directory name with the -basedir option")
					return None,None
			else:
				if params[i].startswith("-basedir"):
					basedir = getString(len("-basedir"),params[i])
			if not basedir == None:
				if os.path.exists(basedir):
					if not basedir.startswith("/"):
						basedir = "%s%s%s" % (os.getcwd(),os.path.sep,basedir)
				else:
					usage("%s does not exist." % basedir)
					return None,None
		elif params[i].startswith("-rundir"):
			if isParameter(params[i]):
				if i+1 < length and notParameter(params[i+1]): 
					testdir = params[i+1]
					i = i+1
				else:
					usage("you must give a directory name with the -testdir option")
					return None,None
			else:
				if params[i].startswith("-rundir"):
					testdir = getString(len("-rundir"),params[i])
				
		elif params[i].startswith("-eval"):
			evaluate = True
		elif params[i].startswith("-delete_mili"):
			kmili = False
		elif params[i].startswith("-html"):
			types.append("html")
		elif params[i].startswith("-t") or params[i].startswith("-test"):
			if isParameter(params[i]):
				if i+1 < length and notParameter(params[i+1]): 
					test = params[i+1]
					i = i+1
				else:
					usage("you must give a name with the test option")
					return None,None
			else:
				if params[i].startswith("-test"):
					test = getString(len("-test"),params[i])
				else:
					test = getString(len("-t"),params[i])
		elif params[i].startswith("-m"):
			if isParameter(params[i]):
				if i+1 < length and notParameter(params[i+1]): 
					test_mode = params[i+1]
					i = i+1
				else:
					usage("you must give a name with the -m option")
					return None,None
			else:
				test_mode = getString(len("-m"),params[i])
			test_mode_set = True	
			if test_mode.startswith("stddev"):
				if test_mode.find("=") > 0:
					test_mode = test_mode.split("=")[0]
					stddev = test_mode.split("=")[1]
								
		elif params[i].startswith("-e"):
			if isParameter(params[i]):
				if i+1 < length and notParameter(params[i+1]): 
					envir = params[i+1]
					i = i+1
				else:
					usage("you must give a name with the -e option")
					return None,None
			else:
				envir = getString(len("-e"),params[i])		
		elif params[i].startswith("-p"):
			if isParameter(params[i]):
				if i+1 < length and notParameter(params[i+1]) and params[i+1].isdigit(): 
					processors = params[i+1]
					i = i+1
				else:
					processors=defaultProcessors;
			else:
				pr = getString(len("-p"),params[i])
				if len(pr) == 0 or not pr.isdigit():
					usage("the number of processors is incorrect: %s" % pr)
					return None,None
				processors = pr
		elif params[i].startswith("-n"):
			if isParameter(params[i]):
				if i+1 < length and notParameter(params[i+1]) and params[i+1].isdigit(): 
					nodes = params[i+1]
					i = i+1
				else:
					usage("Number of nodes not given or invalid")
					return None,None
			else:
				pr = getString(len("-n"),params[i])
				if len(pr) == 0 or not pr.isdigit():
					usage("the number of nodes is incorrect: %s" % pr)
					return None,None
				nodes = pr
		elif params[i].startswith("-d"):
			if isParameter(params[i]):
				if i+1 < length and notParameter(params[i+1]) and params[i+1].isdigit():
					if do_sigs:
						sig_dig = params[i+1]
					i = i+1
				else:
					usage("Number of significant digits not given or invalid")
					return None,None
			else:
				pr = getString(len("-d"),params[i])
				if len(pr) == 0 or not pr.isdigit():
					usage("the significant digits is not a number: %s" % pr)
					return None,None
				if do_sigs:
					sig_dig = pr
			if not sig_dig == "4":
				absolute_tolerance = True
		elif params[i].startswith("-c"):
			if isParameter(params[i]):
				if i+1 < length and notParameter(params[i+1]):
					if os.path.exists(params[i+1]):
						if params[i+1].startswith(os.path.sep):
							code =  params[i+1]
						else:
							code = "%s/%s" % (os.getcwd(),params[i+1])
						i = i+1
					else:
						usage("Code %s does not exist." % params[i+1])
						return None,None
				else:
					usage("No code -- code parameter not given")
					return None,None
			else:
				pr = getString(len("-c"),params[i])
				if len(pr) == 0 or not os.path.exists(pr):
					usage("No code given or it does not exist: %s" % pr)
					return None,None
				if pr.startswith(os.path.sep):
					code = pr
				else:
					code = "%s/%s" % (os.getcwd(),pr)
			actualpath = os.path.realpath(code)
		elif params[i] == "-q":
			quiet = True
		elif params[i] == "-brief":
			brief=True
		elif params[i] == "-b":
			genBaseline = True
		elif params[i] == "-rb":
			removeAllBefore = True
		elif params[i] == "-ra":
			removeAllAfter = True	
		elif params[i].startswith("-v"):
			if isParameter(params[i]):
				if i+1 < length and notParameter(params[i+1]):
					try:
						
						tolerance = float(params[i+1])
						break
					except ValueError:
						tolerance = dyna_env.default_tolerance
					
					i = i+1
				else:
					usage("You must give a numeric value for the tolerance")
					return None,None
			else:
				pr = getString(len("-v"),params[i])
				if len(pr) == 0 or not pr.isdigit():
					usage("You must give a numeric value for the tolerance")
					return None,None
				try:
					tolerance = float(pr)
					break
				except ValueError:
					tolerance = dyna_env.default_tolerance
		elif params[i].startswith("-k"):
			if isParameter(params[i]):
				if i+1 < length and notParameter(params[i+1]): 
					tol_type = params[i+1]
					i = i+1
				else:
					tol_type = dyna_env.default_tol_type
			else:
				tol_type = getString(len("-k"),params[i])
		else:
			usage("Invalid parameter %s" % params[i])
			return None,None
		i = i+1
	if code == None and not evaluate:
		usage("You must enter the code to use")
		return None,None
	if len(suites) == 0 and test == None:
		usage("Must define suite(s) or a test")
		return None,None
	
	if evaluate:
		code = "Evaluation only"
		actualpath="None"
	if (not processors == None):
		if nodes == None:
			nodes = defaultNodes
		parallel = True
		
	#if (processors == None and not nodes == None) or (not processors == None and nodes == None) :
	#	usage ("You must define the number of processors and nodes")
	#elif not ( processors == None):
	#	parallel = True
		
	
	an = AnalyzerFactory.getAnalyzer(test_mode)
	
	return {"suite":"",
			"evaluate":evaluate,
			"test":test,
			"parallel": parallel,
			"processors": processors,
			"nodes":nodes ,
			"code":code,
			"abs_tol":absolute_tolerance,
			"tolerance":tolerance,
			"tol_type":tol_type,
			"baseline":genBaseline,
			"environment":envir,
			"sigdig":sig_dig,
			"path":actualpath,
			"removeAllBefore":removeAllBefore,
			"removeAllAfter":removeAllAfter,
			"analyzer":an,
			"mode":test_mode,
			"test_type":test_type,
			"test_mode_set":test_mode_set,
			"stddev":stddev,
			"keepmili":kmili,
			"types":types,
			"quiet":quiet,
			"basedir":basedir,
			"testdir":testdir,
			"print_added":print_added,
			"full_info":full_info,
			"plot":plot,
			"brief":brief},suites
	
def getString(pos,param):
	return param[pos:]

def isParameter(p):
	return (p in parameters)

def notParameter(p):
	return (not p in parameters)

def parseList(inPosition, length, array):
	
	if inPosition+1 == length:
		return inPosition

	if array[inPosition+1].startsWith("-"):
		return inPosition

	
