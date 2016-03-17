#!/usr/bin/env python 
import warnings
warnings.simplefilter("ignore", DeprecationWarning)

import os,sys,string,popen2,time,fpformat,glob,math
from parser import *

import dyna_optparser,ReportGenerator
from string import Template
time_stamp_format="%m-%d-%Y_%H:%M:%S"
runid = time.strftime("%m%d%Y%H%M%S",time.localtime())
default_log = "log.%s" % time.strftime(time_stamp_format,time.localtime())
run_path = "%s%s" %(os.getcwd(), os.path.sep)
run_log = "run.%s" % default_log

actualpath = ""

class Test:
	def __init__(self):

		file_search = os.path.abspath(__file__)

		self.test_base_dir = file_search[:file_search.rfind("/")+1]
		self.gnuplot = "%sscripts%sgnuplot%sgnuplot.sh" % (self.test_base_dir,os.path.sep,os.path.sep)
		self.gnudat = "gnu.dat" 
		self.version_set = False
		self.tests = {}
		self.code_version=""
		self.code_revision=""
		self.altSuite = None
		self.home = os.getcwd()
		#self.logfilename = "%s/%s" % (home,default_log)
		
	def loadEnvironment(self,environ):
		self.dyna_env =  __import__(environ)
		

	def generateImage(self,test):
		os.system("%s %s %s" % (self.gnuplot,test,self.gnudat))
		
	def start(self,params,suites):
		self.loadEnvironment( params["environment"])
		self.getVersion(self.home,params)
		
		sys.stdout.write("Log File: %s\n" % default_log)
		sys.stdout.flush()
		if params["quiet"]:
			sys.stdout.write("Run File: %s\n\n" %  run_log)
			sys.stdout.flush()
			f = open(run_log,'a')
			f.close()
		self.rg = ReportGenerator.ReportGenerator(params,
											 self.code_version,
											 self.code_revision,
											 self.altSuite,
											 self.dyna_env,
											 "%s/%s" % (self.home,default_log))
		self.rg.writeLogFileHeader()
		
		self.runTest(params,suites)
		
		self.rg.writeLogSummary()
		
		sys.stdout.write("Log File: %s\n" %  default_log)
		sys.stdout.flush()
		#self.rg.writeReports()
		
	def getSuffix(self, params, suite) :
		if "base_suffix" in self.dyna_env.mapping[suite].keys():
			params["base_suffix"]=self.dyna_env.mapping[suite]["base_suffix"]
		else :
			params["base_suffix"]="answ"
		if "output_suffix" in self.dyna_env.mapping[suite].keys():
			params["output_suffix"]=self.dyna_env.mapping[suite]["output_suffix"]
		else :
			params["output_suffix"]="jansw"
		
	def runTest(self , params,suites):
		"""
		Run the test(s) based on the params dictionary
		"""
				
		if not params["baseline"] and not params["evaluate"]:
			home = os.getcwd()
			self.getVersion(home,params)

		#decide which scenario we are doing
		if ("all" in suites): 
			suites = self.dyna_env.mapping.keys()
			params["test"] = None
			self.runTest(params,suites)
		elif len(suites) == 0: 
			# search for the test. pain but necessary
			testdir = self.findTest(params["test"])
			if testdir== None:
				print "Failed to locate suite for test %s." % params["test"]
				return
			
			self.rg.writeSuiteHeader(testdir)
			params["suite"]= testdir
			
			self.getSuffix(params,testdir)
			self.loadScripts(params)
			self.runATest(params)
		elif params["test"] == None:
			#run all the tests in the suite(s)
			for suite in suites:
				if suite in self.dyna_env.mapping.keys():
					params["suite"] = suite
					self.getSuffix(params,suite)
					self.rg.writeSuiteHeader(suite)
					tests = self.findSuite(suite)
					for test in tests:
						params["test"] = test
						self.loadScripts(params)
						self.runATest(params)
				elif suite in self.dyna_env.defined_suites.keys():
					temp_suites = self.dyna_env.defined_suites[suite]
					
					self.altSuite = temp_suites.keys()
					for this_suite in temp_suites.keys():
						tests = temp_suites[this_suite]
						params["suite"] = this_suite
						self.getSuffix(params,this_suite)
						self.rg.writeSuiteHeader(this_suite)
						for test in tests:
							params["test"] = test
							self.loadScripts(params)
							self.runATest(params)
					
				else:
					print "Failed to locate suite %s." % suite
					temp_suite = self.findTest(suite)
					if temp_suite == None:
						return
					print "Found individual test with this name. Running this"
					self.rg.writeSuiteHeader(temp_suite)
					params["suite"] = temp_suite
					params["test"] = suite
					self.loadScripts(params)
					self.runATest(params)
			
		else:
			for suite in suites:
				params["suite"] = suite
				self.getSuffix(params,suite)
				self.rg.writeSuiteHeader(suite)
				self.loadScripts(params)
				self.runATest(params)
			
	
		
	def runCode(self, params):
		run_test = False
		run_only_dyna =False
		cpu_timeout = False
		if params["print_added"]:
			self.printSystemUserInfo(params["full_info"],params["quiet"]);
			
		execs = []
		
		if not self.dyna_env.mapping[params["suite"]]["tests"] == None:
			if params["test"] in self.dyna_env.mapping[params["suite"]]["tests"].keys():
				if "exec_command" in self.dyna_env.mapping[params["suite"]]["tests"][params["test"]].keys():
					execs = self.dyna_env.mapping[params["suite"]]["tests"][params["test"]]["exec_command"]
		if len(execs) == 0:
			execs = self.dyna_env.mapping[params["suite"]]["exec_command"]
		index = 0
		for template in execs:
			
			code = template.substitute({"CODE":params["code"],"prob":params["test"]})
			if params["run_dynapart"]:
				code += " dynapart ";
			if params["parallel"]:
			
				command = self.dyna_env.cur_driver[0]
				if os.environ["ENVIRONMENT"] == "INTERACTIVE":
					partition = self.dyna_env.cur_driver[2][0]
				else:
					partition = self.dyna_env.cur_driver[2][1]
				if params["run_dynapart"]:
					code = command +" "+self.dyna_env.cur_driver[1].substitute({"processors":1 ,
																	"nodes":1,
																	"partition":partition,
																	"CODE":code})				
				else:			
					code = command +" "+self.dyna_env.cur_driver[1].substitute({"processors":params["processors"] ,
																	"nodes":params["nodes"],
																	"partition":partition,
																	"CODE":code})
			if self.dyna_env.ostype == "linux3":
				code = code.replace("i=","-args i=")

			if params["quiet"]:
				os.system(code +">> %s%s 2>&1" % (run_path,run_log))
			else:
				os.system(code)
			index = index +1
			if index == len(execs):
				if params["run_dynapart"]:
					if os.path.exists("%s%s" % (params["test"],".hsp")):
						file = open("%s%s" % (params["test"],".hsp"))
						for line in file.readlines():
							if line.find("n o r m a l")>=0:
								run_test = True					
				elif "run_validation_suffix" in self.dyna_env.mapping[params["suite"]].keys():
					if os.path.exists("%s%s" % (params["test"],self.dyna_env.mapping[params["suite"]]["run_validation_suffix"])):
						file = open("%s%s" % (params["test"],self.dyna_env.mapping[params["suite"]]["run_validation_suffix"]))
						for line in file.readlines():
							if line.find("not supported in parallel")>=0:
								run_only_dyna = True
								params["serial_message"]=line.strip()
							if line.find("n o r m a l")>=0:
								run_test = True
				else:
					run_test=True
				if not run_test:
					break
          
		if params["print_added"]:
			self.printSystemUserInfo(params["full_info"],params["quiet"]);

		return run_test,run_only_dyna
		
	def printSystemUserInfo(self,full,quiet):
		q=""
		if(quiet):
			q = ">> %s%s 2>&1" % (run_path,run_log)
		if(full):
			os.system("ps -ealf "+q)
		else:
			os.system("ps -lf -U root -u root -N "+q)
		os.system("finger " +q)

	def getVersion(self,home,params):

		if not self.version_set:
			file =popen2.Popen4("strings %s | grep -i revision" % params["code"])

			lines = file.fromchild.readlines()
			for line in lines:
				if len(line) >0 and line.count("Revision:")>0:
					nodes = line.split()
					self.version_set = True
					if nodes[0].count("Revision") > 0:
						continue 
					self.code_version = nodes[0]
					self.code_revision = nodes[3]
					

	def removeDirFiles(self):
		try:
			cur_dir = os.getcwd()
		
			files = os.listdir(cur_dir)
			for file in files:
				if os.path.isdir(file):
					os.chdir(file)
					self.removeDirFiles()
					os.chdir(cur_dir)
					os.rmdir(file)
				elif not file.startswith(".nfs"):
					os.remove(file)
		except OSError:
			pass
				
	def 	cleanPriorRun(self,keep_list):
		home_dir = os.getcwd()
		rfiles = os.listdir(os.getcwd())
		for jfile in rfiles:
			if os.path.isdir(jfile):
				os.chdir(jfile)
				self.removeDirFiles()
				os.chdir(home_dir)
			else:
				if jfile in keep_list: #.find("answ") >= 0  or jfile.find(".elem") >= 0 or jfile.find(".dyn") >= 0 or  jfile.find("awk") >= 0:
					pass
				else:
					try:
						os.remove(jfile)
					except OSError:
						pass
					
	def cannotRunParallel(self,test,suite):
		return False

	def runATest(self, params):
		
		if params["parallel"] and self.cannotRunParallel(params["test"],params["suite"]):
			output={"status":"run_error","message":"Cannot run this test in Parallel"}

		elif self.isTest(params["suite"],params["test"]):
			
			run_serial = False
			if params["mode"] == "smoke":
				sys.stdout.write("Starting Smoke Test of %s %s\n" % (params["suite"],params["test"]))
			elif params["evaluate"]:
				sys.stdout.write("Starting Evaluation of %s %s\n" % (params["suite"],params["test"]))
			else:
				sys.stdout.write("Starting %s %s\n" % (params["suite"],params["test"]))
			sys.stdout.flush()
			home = os.getcwd()
			test_dir = self.generateTestDir(params)
			
			if not os.path.exists(test_dir):
				os.makedirs(test_dir)
				
			map = self.dyna_env.mapping[params["suite"]]
			found_in_tests =False

			if not map["tests"] == None and params["test"] in map["tests"].keys():
				if "test_type" in map["tests"][params["test"]]:
					if map["tests"][params["test"]]["test_type"] != None:
						params["test_type"] = map["tests"][params["test"]]["test_type"]
						found_in_tests = True
				
			if not found_in_tests and "test_type" in map.keys():
				if map["test_type"] != None:
					params["test_type"] = map["test_type"]
				
			
			#make links required for running problem and validating results
			links_made, keep_list,link_messages = self.createLinks(params,test_dir,home)
			
			
			output={"status":"test_environment_error","message":"failed to create test"}
				
			#This is a switch generated by using the -b option.
			if params["baseline"]:
				os.chdir(test_dir)
				code_ran = True
				script_ran= True
				if not os.path.exists("%s%s%s.%s" % (test_dir,os.path.sep,params["test"],params["output_suffix"])):				
					self.cleanPriorRun(keep_list)
					
					code_ran,run_serial = self.runCode(params)
					
					if code_ran:
						script_ran = self.runScripts(params);
						
				if code_ran and script_ran:
					if not self.installBaseline(params):
						output["status"]="install_failed."
						output["message"]="Failed to install baseline"
					else:
						output["status"]="install_success"
						output["message"]="Install successfull"
				else:
					output["status"]="Install_failed"
					if not code_ran:
						if run_serial:
							output["message"]=params["serial_message"]+": Try running in serial for installation."
						else:
							output["message"]="Code failed to run correctly for install."
					else:
						output["message"]="Awk scripts failed to run correctly for install."
						
						
				for rfile in os.listdir(os.getcwd()):
					try:
						os.remove(rfile)
					except OSError:
						## There are some NSF mount issues which this will ignore
						pass
				
				os.chdir(home)
			elif params["evaluate"]:
				os.chdir(test_dir)
				if os.path.exists("%s.%s" %(params["test"],params["output_suffix"])) and os.path.exists("%s.%s" %(params["test"],params["base_suffix"])):
					status ,output= self.runAnalysis(home,params)
					if params["plot"] and not status == "passed":
						self.generateImage(params["test"])
			elif not links_made:
				output={"status":"setup_failure","message":link_messages}
			else:
				os.chdir(test_dir)
				self.cleanPriorRun(keep_list)
				exec_count = len(self.dyna_env.mapping[params["suite"]]["exec_command"])
				cur_count = 1;
				run_test,run_serial = self.runCode(params)
				if run_test:
					status = "passed"
					if params["mode"] == "smoke":
						output["status"] = "passed"
						output["message"] = "Code completed without an error"
					elif params["run_dynapart"]:
						output = {"status":"passed",
						          "message":"Dynapart code completed without an error"}
					else:
						scripts_run = self.runScripts(params)
						if scripts_run:
							status , output= self.runAnalysis(home,params)
							if params["plot"] and not status == "passed":
								self.generateImage(params["test"])
						else:
							output = {"status":"test_environment_error",
								  			"message":"scripts did not run correctly"}
							status = "failed"
				else:
					status = "failed"
					if run_serial:
						output = {"status":"run_error",
							  "message":params["serial_message"]+" : Try running in serial."}
					elif params["run_dynapart"]:
						output = {"status":"failed",
							      "message":"Dynapart failed."}
					else:
						output = {"status":"run_error",
							      "message":"Code failed to run correctly"}
				
				rfiles = os.listdir(os.getcwd())
				for jfile in rfiles:
					found = False
					if os.path.isdir(jfile):
						os.chdir(jfile)
						self.removeDirFiles()
						os.chdir(test_dir)
					elif status == "failed":
						if jfile.find(params["output_suffix"]) >= 0 or  jfile.find(params["base_suffix"]) >= 0:
							found = True
						elif not self.dyna_env.mapping[params["suite"]]["keep_on_fail_file_expr"] == None:
							for expr in self.dyna_env.mapping[params["suite"]]["keep_on_fail_file_expr"]:
								if jfile.find(expr) >=0:
									found = True
									break
						if not found:
							try:
								os.remove(jfile)
							except OSError:
								pass
					else:
						if jfile.find(params["output_suffix"]) >= 0 or  jfile.find(params["base_suffix"]) >= 0:
							found = True
						elif not self.dyna_env.mapping[params["suite"]]["keep_on_pass_file_expr"] == None:
							for expr in self.dyna_env.mapping[params["suite"]]["keep_on_pass_file_expr"]:
								if jfile.find(expr) >=0:
									found = True
									break
						if not found:
							try:
								os.remove(jfile)
							except OSError:
								pass
						
							
			os.chdir(home)
			sys.stdout.write("Finished with status: %s\n" % output["status"])
			sys.stdout.flush()

		else:
			message = "Suite/test combination does not exist %s: %s" % (params["suite"],params["test"])

			output={"status":"invalid_test","message":message}

		
		self.rg.writeTestOutput(params,output,self.getParser(params["test_type"]))
	
	
	def generateTestDir(self, params):
		testdir = params["testdir"]
		if testdir == None:
			if params["parallel"]:
				test_dir = "%s.p.%s/%s/%s" % (self.dyna_env.osname,params["processors"],params["suite"],params["test"])
			else:
				test_dir = "%s.serial/%s/%s" % (self.dyna_env.osname,params["suite"],params["test"])
		else:
			test_dir = "%s/%s/%s" % (testdir,params["suite"],params["test"])
		if not test_dir.startswith("/"):
			test_dir = "%s/%s" % (os.getcwd(),test_dir)
		if not os.path.exists(test_dir):
				os.makedirs(test_dir)
		return test_dir
	
	def getParser(self,parser_type):
		if parser_type.startswith("curve"):
			return curveparser.CurveParser();
		elif parser_type.startswith("image"):
			return imageparser.ImageParser();
		else :
			return None;
		
	def runAnalysis(self,home,params):
		error = 0.0
		assoc_error_perc = 0.0
		perc_error = 0.0
		assoc_error = 0.0
		analysis_results = {}
		status="passed"
		
		base_parser = self.getParser(params["test_type"]);
		baseline = "%s.%s" % (params["test"],params["base_suffix"])
		base_parser.parseFile(baseline)
		
		output_parser = self.getParser(params["test_type"]);
		baseline = "%s.%s" % (params["test"],params["output_suffix"])
		output_parser.parseFile(baseline)
		
		params["analyzer"].setAnalysis(params["sigdig"],params["stddev"])
		
		analyzer = params["analyzer"]
		
		passed = base_parser.isEqual(output_parser,analysis_results, analyzer)		
				
		if not passed:
			status = "failed"
			error = fpformat.sci(analyzer.max_error_array[0],15)
			assoc_error_perc = analyzer.max_error_array[1]
			perc_error = analyzer.max_perc_array[0]
			assoc_error = fpformat.sci(analyzer.max_perc_array[1],15)
		
		output= {"status":status,
				 "error":error,
				 "assoc_perc":assoc_error_perc,
				 "info":analysis_results,
				 "perc_error":perc_error,
				 "assoc_error":assoc_error,
				 "error_perc_info":analyzer.max_perc_info,
				 "error_err_info":analyzer.max_err_info
				 }
		return status,output
	
	def createLinks(self, params,directory,wbase):
		if not params["basedir"] == None:
			base = params["basedir"]
		else:
			base = wbase
		#ok lets try and determine where the baseline is
		
		answ_suffix=params["base_suffix"]
		
		suite_dir = params["suite"]		
		base_file = "%s.%s" % (params["test"],answ_suffix)
		awk_dir = params["suite"]
		
		baseline_dir = ""
		keepers = []
		found = False
		messages =[]
		alt_required_files_dir = None
    
		path_base = "%s%s" %(base,os.path.sep)
		if params["plot"]:
			os.system("rm -f %s%sgnu.dat" % (directory,os.path.sep))
			os.system("ln -s %sscripts%sgnuplot%sgnu.dat %s%sgnu.dat" % (self.test_base_dir,os.path.sep,os.path.sep,directory,os.path.sep))
			#os.symlink("%sgnuplot%sgnu.dat" % (self.test_base_dir,os.path.sep),"%s%sgnu.dat" % (directory,os.path.sep))
			if not os.path.exists("%s%sgnu.dat" % (directory,os.path.sep)):
				sys.exit(200)
			
		suffix = self.dyna_env.mapping[params["suite"]]["file_suffix"]
		
		## check to make sure we do not need to use an alternate test suite result ##
		if not self.dyna_env.mapping[params["suite"]]["tests"] == None:
			if params["test"] in self.dyna_env.mapping[params["suite"]]["tests"].keys():
				if "alt_base" in self.dyna_env.mapping[params["suite"]]["tests"][params["test"]].keys():
					## we will be using an alternate baseline from where our dyn files are located
					suite_dir = self.dyna_env.mapping[params["suite"]]["tests"][params["test"]]["alt_base"]["suite"]
					base_file = "%s.%s" % (self.dyna_env.mapping[params["suite"]]["tests"][params["test"]]["alt_base"]["baseline"],answ_suffix)
				if "awk_dir" in self.dyna_env.mapping[params["suite"]]["tests"][params["test"]].keys():
					awk_dir = self.dyna_env.mapping[params["suite"]]["tests"][params["test"]]["awk_dir"]
				if "required_files_alt_dir" in self.dyna_env.mapping[params["suite"]]["tests"][params["test"]].keys():
					alt_required_files_dir = self.dyna_env.mapping[params["suite"]]["tests"][params["test"]]["required_files_alt_dir"]
				if "file_suffix" in self.dyna_env.mapping[params["suite"]]["tests"][params["test"]].keys():
					suffix = self.dyna_env.mapping[params["suite"]]["tests"][params["test"]]["file_suffix"]
		if os.path.exists("%s%s" %(path_base,suite_dir)):
			if os.path.exists("%s%s%s%s" %(path_base, suite_dir, os.path.sep, self.dyna_env.default_platform)):
				if os.path.exists("%s%s%s%s%s%s" %(path_base, suite_dir, os.path.sep, self.dyna_env.default_platform, os.path.sep, base_file)):
					baseline_dir ="%s%s%s%s%s" %(base,os.path.sep,suite_dir,os.path.sep,self.dyna_env.default_platform)
					found= True
			if not found:
				if os.path.exists("%s%s%s%s" %(path_base,suite_dir,os.path.sep,base_file)):
					baseline_dir ="%s%s" %(path_base,suite_dir)
					found = True
			if not found:
				messages.append("Failed to find baselines %s" % base_file)
		else:
			messages.append("Failed to find suite directory %s for baseline %s " % (suite_dir,base_file))

		if found:
			self.baseline_dir = baseline_dir

			os.system("rm -f %s/%s.%s" % (directory,params["test"],answ_suffix))
			os.symlink("%s/%s.%s" % (baseline_dir,params["test"], answ_suffix),"%s/%s.%s" % (directory,params["test"], answ_suffix))

			keepers.append("%s.%s" % (params["test"], answ_suffix))
			
		dyna_file = "%s%s" % (params["test"],suffix)
		if os.path.exists("%s/%s/%s" % (base,params["suite"],dyna_file)):
			keepers.append(dyna_file)
			os.system("rm -f %s/%s" % (directory, dyna_file))
			os.symlink("%s/%s/%s" % (base,params["suite"],dyna_file), "%s/%s" % (directory,dyna_file))
		else:
			messages.append("Failed to find input file %s%s" % (params["test"],suffix))

		if params["parallel"] and not "no_partition" in self.dyna_env.mapping[params["suite"]].keys():
			if not params["processors"] == "1":
				part_file = "%s%s.%s" % (params["test"],suffix, params["processors"])

				if os.path.exists("%s/%s/%s" % (base,params["suite"],part_file)):
					keepers.append(part_file)
					os.system("rm -f %s/%s" % (directory,part_file))
					os.symlink("%s/%s/%s" % (base,params["suite"],part_file), "%s/%s" % (directory,part_file))
				else:
					messages.append("Failed to find partition file %s.%s.%s" % (params["test"],suffix,params["processors"]))

		
		for script in params["awk_scripts"]:
			file_name = Template(script).substitute({"prob":params["test"]})
			if os.path.exists("%s/%s/%s" % (base,awk_dir,file_name)):
				keepers.append(file_name)
				os.system("rm -f %s/%s" % (directory,file_name))
				os.symlink("%s/%s/%s" % (base,awk_dir,file_name),"%s/%s" % (directory,file_name))
				found = True
			else:
				messages.append("Failed to find script file %s/%s" % (awk_dir,file_name))
					
		# Needed to setup ability for additional file to be linked. This is done through the "required_files" array in the mapping
		# at the suite level or in the tests level
		
		if (alt_required_files_dir == None) and "required_files" in self.dyna_env.mapping[params["suite"]].keys():
			for rf in self.dyna_env.mapping[params["suite"]]["required_files"]:
				if os.path.exists("%s/%s/%s" % (base,params["suite"],rf)):
					keepers.append(rf)
					os.system("rm -f %s/%s" % (directory,rf))
					os.symlink("%s/%s/%s" % (base,params["suite"],rf),"%s/%s" % (directory,rf))
				else:
					messages.append("Failed to find required file %s/%s for suite %s" % (params["suite"],rf,params["suite"]))
		elif not alt_required_files_dir == None:
			if "required_files" in self.dyna_env.mapping[params["suite"]]["tests"][params["test"]].keys():
				for rf in self.dyna_env.mapping[params["suite"]]["tests"][params["test"]]["required_files"]:
					if os.path.exists("%s/%s/%s" % (base,alt_required_files_dir,rf)):
						keepers.append(rf)
						os.system("rm -f %s/%s" % (directory,rf))
						os.symlink("%s/%s/%s" % (base,alt_required_files_dir,rf),"%s/%s" % (directory,rf))
					else:
						messages.append("Failed to find alternate required file %s/%s for suite %s" % (alt_required_files_dir,rf,params["suite"]))
		elif (not self.dyna_env.mapping[params["suite"]]["tests"] == None) and params["test"] in self.dyna_env.mapping[params["suite"]]["tests"].keys():
			if "required_files" in self.dyna_env.mapping[params["suite"]]["tests"][params["test"]].keys() :
				for rf in self.dyna_env.mapping[params["suite"]]["tests"][params["test"]]["required_files"]:
					if os.path.exists("%s/%s/%s" % (base,params["suite"],rf)):
						keepers.append(rf)
						os.system("rm -f %s/%s" % (directory,rf))
						os.symlink("%s/%s/%s" % (base,params["suite"],rf),"%s/%s" % (directory,rf))
					else:
						messages.append("Failed to find required file %s/%s for suite %s" % (params["suite"],rf,params["suite"]))

		
		self.answ_suffix = answ_suffix
		self.baseline_dir = baseline_dir
		self.test_from = "%s/%s.jansw" % (directory,params["test"])
		self.test_to = "%s%s%s.%s" % (baseline_dir,os.path.sep,params["test"], answ_suffix)
		self.answ_file = "%s/%s.answ" % (directory,params["test"])
		return (len(messages) ==0),keepers,messages

		
	def loadScripts(self,params):
		
		if not self.dyna_env.mapping[params["suite"]]["tests"] == None and (params["test"] in self.dyna_env.mapping[params["suite"]]["tests"].keys() and "awkfiles" in self.dyna_env.mapping[params["suite"]]["tests"][params["test"]].keys()):
			params["awk_scripts"] = self.dyna_env.mapping[params["suite"]]["tests"][params["test"]]["awkfiles"]
		else:
			params["awk_scripts"] = self.dyna_env.mapping[params["suite"]]["awkfiles"]
		if not self.dyna_env.mapping[params["suite"]]["tests"] == None and (params["test"] in self.dyna_env.mapping[params["suite"]]["tests"].keys() and "awk_commands" in self.dyna_env.mapping[params["suite"]]["tests"][params["test"]].keys()):
			params["awk_commands"] = self.dyna_env.mapping[params["suite"]]["tests"][params["test"]]["awk_commands"]
		else:
			params["awk_commands"] = self.dyna_env.mapping[params["suite"]]["awk_commands"]

			
	def runScripts(self,params):
		# We found on the aix system the NFS mount took to long to register new files so we touch the directory
		# to force regitration of the files
		os.system("touch .")
		if(len(params["awk_commands"]) ==0):
			return True
		if not os.path.exists( "%s.hsp" % params["test"]):
			return False

		answerfile = open("%s.jansw" % params["test"],'w')

		for script in params["awk_commands"]:
			
			#digs =(int(params["sigdig"]))-1
			command = script.substitute({"preawk":self.dyna_env.preawk,"sig":15,"prob":params["test"]})

			file = popen2.Popen4(command)
			
			outfile = file.fromchild
			for line in outfile:
				answerfile.write(line)
		answerfile.close()
		return True

	
	def installBaseline(self,params):
		if self.baseline_dir =="":
			self.baseline_dir = "%s%s%s" % (self.home, os.path.sep, params["suite"])
		if not os.path.exists("%s" % (self.baseline_dir)):
			os.makedirs("%s" % (self.baseline_dir))
		if os.path.exists("%s%s%s.%s" % (os.getcwd(),os.path.sep,params["test"],params["output_suffix"])):
			os.rename("%s%s%s.%s" % (os.getcwd(),os.path.sep,params["test"],params["output_suffix"]),"%s%s%s.%s" % ( self.baseline_dir, os.path.sep, params["test"],params["base_suffix"]))
			return True
		return False
		
	def doAnalysis(self, params):
		pass
		
		
	def findSuite(self, suite):
		if suite in self.dyna_env.mapping.keys():
			return self.dyna_env.mapping[suite]["quicklist"]
		return None
	
	def findTest(self, test):
		for key  in self.dyna_env.mapping.keys():
			if test in self.dyna_env.mapping[key]["quicklist"]:
				return key
		return None
	
	def isTest(self, dir, test):
		if dir in self.dyna_env.mapping.keys():
			return (test in self.dyna_env.mapping[dir]["quicklist"])
		return False
	
if __name__ == "__main__":
	
	parameters,suites = dyna_optparser.parseParameters(sys.argv[1:])
	
	if parameters == None:
		sys.exit(100)
	test = Test()
	test.start(parameters,suites)
