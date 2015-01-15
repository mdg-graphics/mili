import os,sys,string,popen2,time,fpformat,glob,webbrowser
from xml.dom.minidom import parse
class ReportGenerator:
	def __init__(self, params,version,revision,altSuite,environment,tests,logfile):
		self.init_params(params,version,revision,altSuite,environment,tests,logfile)
		
	def __init__(self, params,version,revision,altSuite,environment,logfile):
		self.init_params(params,version,revision,altSuite,environment,None,logfile)

	def init_params(self,params,version,revision,altSuite,environment,tests,logfile):
		self.tests = tests
		self.summary = {"total":int(0),
						"passed":int(0),
						"failed":int(0),
						"invalid":int(0)}
		self.params = params
		self.code_version = version
		self.code_revision = revision
		self.altSuite = altSuite
		self.dyna_env = environment
		self.tests = None
		self.default_log = logfile
		self.testfiles = {}
		
	def genTimeStamps(self):
		YM_format = "%Y_%m"
		fileNameFormat = "%Y_%m_%d_%H%M%S"
		self.basedir = time.strftime(YM_format,time.localtime())
		self.fileNamePrefix = time.strftime(fileNameFormat,time.localtime())
		self.month = time.strftime("%B",time.localtime())
		self.year = time.strftime("%Y",time.localtime())
		self.day = time.strftime("%d",time.localtime())
		self.hour = time.strftime("%H",time.localtime())
		self.minute = time.strftime("%M",time.localtime())
		self.second = time.strftime("%S",time.localtime())
		
	def writeReports(self):
		for type in self.params["types"]:
			if type == "html":
				self.genTimeStamps()
				self.writeHTMLreport()
			elif type == "log":
				self.writeLogFileReport()
				
	def writeHTMLreport(self):
		
		if not os.path.exists(self.basedir):
			os.mkdir(self.basedir)
		
		self.writeHTMLTestFiles()
		
		if not os.path.exists("index.html"):
			self.createNewFiles()
		else:
			itable = self.generateIndexTable()
			self.appendToIndexFile()
		print "done"
		
	def createNewFiles(self):
		table = self.generateTestingTable()
		f = open("index.html",'w')
		f.write("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n")
		f.write("<html>\n")
		f.write("\t<head><title>Paradyn Testing Suite</title></head>\n")
		f.write("\t<body>\n")
		f.write("<center><h2><strong>ParaDyn Testing</strong><p>%s, %s %s</p></h2></center>" % (self.year,self.month,self.day))
		f.write("\t\t<table border=\"1\" cellpadding=\"3\" cellspacing=\"2\">\n")
		f.write("<tr><th colspan=\"3\">Past Results</th></tr>\n")
		f.write("<tr><th>Year</th><th>Month</th><th>Date</th></tr>\n")
		f.write("<tr><th>%s</th><th>%s</th><th><a href=\"%s%s%s_%s.html\">%s</a></th></tr>"% (self.year,self.month,self.basedir,os.path.sep,self.basedir,self.day,self.day))
		f.write("\t\t</table>\n")
		f.write(table)
		f.write("\t\t<table border=\"1\" cellpadding=\"3\" cellspacing=\"2\">\n")
		f.write("<tr><th colspan=\"3\">Past Results</th></tr>\n")
		f.write("<tr><th>Year</th><th>Month</th><th>Date</th></tr>\n")
		f.write("<tr><th>%s</th><th>%s</th><th><a href=\"%s%s%s_%s.html\">%s</a></th></tr>"% (self.year,self.month,self.basedir,os.path.sep,self.basedir,self.day,self.day))
		f.write("\t\t</table>\n")
		f.write("\t</body>\n")
		f.write("</html>\n")
		
		f.close()

	
		f = open("%s/%s_%s.html" % (self.basedir,self.basedir,self.day),'w+')
		f.write("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n")
		f.write("<html>\n")
		f.write("\t<head><title>Paradyn Testing Suite</title></head>\n")
		f.write("\t<body>\n")
		f.write("<center><h2><strong>ParaDyn Testing</strong><p>%s, %s %s</p></h2></center>" % (self.year,self.month,self.day))
		f.write("\t\t<table border=\"1\" cellpadding=\"3\" cellspacing=\"2\">\n")
		f.write("<tr><th colspan=\"3\">Past Results</th></tr>\n")
		f.write("<tr><th>Year</th><th>Month</th><th>Date</th></tr>\n")
		f.write("<tr><th>%s</th><th>%s</th><th><a href=\"%s_%s.html\">%s</a></th></tr>"% (self.year,self.month,self.basedir,self.day,self.day))
		f.write("\t\t</table>\n")
		f.write(table)
		f.write("\t\t<table border=\"1\" cellpadding=\"3\" cellspacing=\"2\">\n")
		f.write("<tr><th colspan=\"3\">Past Results</th></tr>\n")
		f.write("<tr><th>Year</th><th>Month</th><th>Date</th></tr>\n")
		f.write("<tr><th>%s</th><th>%s</th><th><a href=\"%s_%s.html\">%s</a></th></tr>"% (self.year,self.month,self.basedir,self.day,self.day))
		f.write("\t\t</table>\n")
		f.write("\t</body>\n")
		f.write("</html>\n")
		
		f.close()
		webbrowser.open_new("///%s/index.html" % os.getcwd())
		
	def generateIndexTable(self):
		ifile = open("index.html")
		domtree = parse(ifile)
		
		table = domtree.getElementsByTagName("table")[0]
		print table.toprettyxml()
		
	def generateTestingTable(self):
		table = "<table border=\"1\" cellpadding=\"3\" cellspacing=\"2\"><tr><th>Machine</th><th>Task</th><th>Result</th><th>Report Time</th><th>Report Links</th><th>Error Images</th></tr>"
		table = table + "<tr><td>%s</td><td>report</td><td>" % self.dyna_env.nodename
		failure = False
		failed_tests = []
		for suite in self.tests.keys():
			for test in self.tests[suite].keys():
				if not self.tests[suite][test]["status"] == "passed":
					failed_tests.append("%s:%s" % (suite,test))
					failure = True
		if failure:
			table = table + "Failed <br />\n"
			for result in failed_tests:
				table = table + "%s <br />\n" % result
		table = table + "</td>"
		table = table + "<td>%s/%s/%s %s:%s:%s</td>" % (self.year,self.month,self.day,self.hour,self.minute,self.second)
		table = table + "</tr>"
		table = table + "</table>"
			
		return table
	
	def appendToIndexFile(self):
		pass
	
	def writeHTMLTestFiles(self):
		if not os.path.exists("%s/%s_%s.html" % (self.basedir,self.basedir,self.day)):
			f = open("%s/%s_%s.html" % (self.basedir,self.basedir,self.day),'a')
			f.close()	
		for suite in self.tests.keys():
			print suite
			for test in self.tests[suite].keys():
				print "keys for suite",test
				self.testfiles["%s_%s_%s.html" % (self.fileNamePrefix, suite, test)] = self.tests[suite][test]
				for key in self.tests[suite][test].keys():
					print key
					if key == "status":
						print self.tests[suite][test]["status"]
		
	def writeLogFileHeader(self):

		f = open("%s" % self.default_log,'a')
		params = self.params
		f.write("Summary for Testing\n\n")
		f.write("System Summary:\n")
		f.write("%2sMachine: %s\n" % ("",self.dyna_env.nodename))
		f.write("%2sOS type: %s \n%2sProcessor: %s\n\n" % ("",self.dyna_env.osname,"",self.dyna_env.processor))
		f.write("Global Run Information\n")
		
		if params["parallel"]:
			f.write("Paradyn code version %s Revision: %s\n" % (self.code_version,self.code_revision));
			f.write("%2sProcessors: %s\n" % ("",params["processors"]));
			f.write("%2sPartitions(nodes): %s\n\n" % ("",params["nodes"]));
			
		else:
			f.write("%2sDyna3d serial code version %s Revision: %s\n\n" % ("",self.code_version,self.code_revision))
		if params["basedir"] == None:
			f.write("%2sBaseline directory: %s\n" %("",os.getcwd()));
		else:
			f.write("%2sBaseline directory: %s\n" %("",params["basedir"]));
		if params["testdir"] == None:
			f.write("%2sTesting directory: %s\n" %("",self.dyna_env.osname));
		else:
			f.write("%2sTesting directory: %s\n" %("",params["testdir"]));
			
		if not self.altSuite == None:
			f.write("\tUsing user defined suite %s\n" % self.altSuite)
		f.write("%2sTest mode: %s\n" % ("",params["mode"]))
		f.write("%2sPath to code used in testing is: \n\t%s\n" % ("",params["path"]))
		f.write("%2sTolerance type: %s\n%2sTolerance value: %s\n%2sSignificant Digits: %s\n"%("",params["tol_type"],"",params["tolerance"],"",params["sigdig"]))
		f.close()
		
	def writeSuiteHeader(self,suitename):

		f = open("%s" % self.default_log,'a')
		
		if not suitename in self.summary.keys():
			self.summary[suitename] = {"total":int(0),
							  "passed":int(0),
							  "failed":int(0)}
		f.write("\nSuite: %s\n" % suitename)
		f.close()

	def writeTestOutput(self, params,test,parser):
		suite=params["suite"]
		testname=params["test"]
		quiet=params["quiet"]
		
		f = open("%s" % self.default_log,'a')
		self.summary[suite]["total"] = (self.summary[suite]["total"])+1
		self.summary["total"] = (self.summary["total"])+1
		status = test["status"]
		message = "%2s**%s status: " % ("",testname)
		
		if status=="passed":
			message = message+status+"\n"
			self.summary[suite]["passed"] = (self.summary[suite]["passed"])+1
			self.summary["passed"] = (self.summary["passed"])+1
		elif status=="failed":
			self.summary[suite]["failed"] = (self.summary[suite]["failed"])+1
			self.summary["failed"] =(self.summary["failed"])+1
		
			message = parser.writeResults(message,test,self.params["brief"])
			
		elif status == "setup_failure":
			message = message+"Test environment failure\n"
			for m in test["message"]:
				message = message+"%4s%s \n"% ("",m)
		else:
			message = message+test["message"]+"\n"
			if status=="install_success":
				self.summary[suite]["passed"] = (self.summary[suite]["passed"])+1
				self.summary["passed"] = (self.summary["passed"])+1
			elif status=="install_failed":
				self.summary[suite]["failed"] = (self.summary[suite]["failed"])+1
				self.summary["failed"] =(self.summary["failed"])+1
			elif status == "invalid_test":
				self.summary["invalid"] = self.summary["invalid"]+1
				

		f.write(message)
		if not quiet and not status=="passed":
			os.system("echo \"%s\"" % message)
		f.close()
												

	def writeLogSummary(self):

		f = open("%s" % self.default_log,'a')
		f.write("*********************** Summary ***********************\n")
		f.write("Tests run:  %d\n" % self.summary["total"])
		f.write("Total Tests passed: %d\n" % self.summary["passed"])
		f.write("Total Tests failed: %d\n" % self.summary["failed"])
		f.write("Abnormal termination: %d\n" % (self.summary["total"]-self.summary["passed"]-self.summary["failed"]-self.summary["invalid"]))
		f.write("Invalid Tests: %d\n" % self.summary["invalid"])
		f.write("\nSuite summaries\n")
		for key in self.summary.keys():
			if key == "total" or key == "passed" or key == "failed"or key == "invalid":
				pass
			else:
				d = self.summary[key]

				f.write("%2sSuite %s  \n" % ("",key))
				f.write("%4sTotal tests run: %d\n" % ("",d["total"]))
				f.write("%4sTotal Test passed: %d\n" % ("",d["passed"]))
				f.write("%4sTotal Test failed: %d\n" % ("",d["failed"]))
				f.write("%4s%d failed to run\n" % ("",d["total"]-d["passed"]-d["failed"]))
		f.close()


			
	def writeLogFileReport(self):
		home = os.getcwd()
		f = open("%s" % (home,self.default_log),'a')
		self.writeLogFileHeader(f,self.params,self.code_version,self.code_revision,self.dyna_env.osname,self.altSuite)
		params = self.params
		if params["baseline"]:
			f.write("Just generating baselines\n")
		elif len(self.tests.keys()) == 0:
			f.write("No suites were recorded\n")
		else:
			summary = {"total":int(0),
					   "passed":int(0),
					   "failed":int(0),
					   "invalid":int(0)}
			for suite in self.tests.keys():
				summary[suite] = {"total":int(0),
								  "passed":int(0),
								  "failed":int(0)}
				f.write("\nSuite: %s\n" % suite)
				if len(self.tests[suite].keys())==0:
					f.write("No tests were recorded for suite %s\n" % suite)
				else:
					for test in self.tests[suite].keys():
						summary[suite]["total"] = (summary[suite]["total"])+1
						summary["total"] = (summary["total"])+1
						if self.tests[suite][test]["status"] == "run_error":							
							f.write("%2s**%s status: %s.\n"% ("",test,self.tests[suite][test]["message"]))
						elif self.tests[suite][test]["status"] == "setup_failure":
							f.write("%2s**%s status: Test environment failure.\n"% ("",test))
							for message in self.tests[suite][test]["message"]:
								f.write("%4s%s \n"% ("",message))
						elif self.tests[suite][test]["status"] == "invalid_test":							
							f.write("%2s**%s status: %s.\n"% ("",test,self.tests[suite][test]["message"]))
							summary["invalid"] = summary["invalid"]+1
						elif self.tests[suite][test]["status"] == "test_environment_error":
							f.write("%2s**%s status: Testing suite error -- %s\n"% ("",test,self.tests[suite][test]["message"]))
						else:
							status = self.tests[suite][test]["status"]
							error = fpformat.sci(float(self.tests[suite][test]["error"]),4)
							a_error = fpformat.sci(float(self.tests[suite][test]["assoc_error"]),4)
							if status == "passed":
								f.write("%2s**%s status: %s Passed\n" % ("",test,status))
								summary[suite]["passed"] = (summary[suite]["passed"])+1
								summary["passed"] = (summary["passed"])+1
							else:
								summary[suite]["failed"] = (summary[suite]["failed"])+1
								summary["failed"] =(summary["failed"])+1
								pmcurve= self.tests[suite][test]["error_perc_info"][0]
								pmts= self.tests[suite][test]["error_perc_info"][1]
								emcurve= self.tests[suite][test]["error_err_info"][0]
								emts= self.tests[suite][test]["error_err_info"][1]
								f.write("%2s%-12s %-14s\n" % ("","** %s" % test,"Status: %s" %status))
								f.write("%5s%-23s\n" % ("", "Max percent Error: %s%%" % self.tests[suite][test]["perc_error"]))
								f.write("%5s%-22s Curve: %-23s Step: %s\n" % ("","Error: %s" % a_error,pmcurve,pmts))
								f.write("%5s%-23s\n" % ("","Max Error: %s" % error))
								f.write("%5s%-22s Curve: %-23s Step: %s\n\n" % ("","Perc Error: %s%%" % self.tests[suite][test]["assoc_perc"],emcurve,emts))
								info = self.tests[suite][test]["info"]
								
								for key in info.keys():
									f.write("%4sCurve: %s\n" % ("",key))
								
									if not info[key] == None:
										if "unkowncurve" in info[key].keys():
											f.write("%6sCurve not in baselines \n" %"")
										else:
											f.write("%6sTimestep   %16s%22s%20s\n" %("","Baseline","Test","% Error"))
											for step in sorted(info[key].keys()):
												d = info[key][step]
												if d == None:
													f.write("%6s%s has no matching test result\n" % ("",fpformat.sci(step,1)))
												elif not d["validtimestep"]:
													f.write("%6s%-10s has no corresponding baseline value\n" % ("",fpformat.sci(step,1)))
												else:
													if not "perc" in d.keys():
														f.write("%6s%-10s%24s%24s \n" % ("",fpformat.sci(step,1),d["baseline"],d["test_value"]))
													else:
														f.write("%6s%-10s%24s%24s%10s%% \n" % ("",fpformat.sci(step,1),d["baseline"],d["test_value"],d["perc"]))
									else:
										f.write("%6sBaselines curve %s not generated in test\n" % ("",key))
												
										
							
							
			f.write("****************************************************************************\nSummary\n")
			f.write("Tests run:  %d\nTotal Tests passed: %d\nTotal Tests failed: %d\nAbnormal termination: %d\nInvalid Tests: %d\n" % (summary["total"],summary["passed"],summary["failed"],summary["total"]-summary["passed"]-summary["failed"]-summary["invalid"],summary["invalid"]))
			f.write("\nSuite summaries\n")
			for key in summary.keys():
				if key == "total" or key == "passed" or key == "failed"or key == "invalid":
					pass
				else:
					d = summary[key]

					f.write("%2sSuite %s  \n" % ("",key))
					f.write("%4sTotal tests run: %d\n" % ("",d["total"]))
					f.write("%4sTotal Test passed: %d\n" % ("",d["passed"]))
					f.write("%4sTotal Test failed: %d\n" % ("",d["failed"]))
					f.write("%4s%d failed to run\n" % ("",d["total"]-d["passed"]-d["failed"]))
			f.close()
