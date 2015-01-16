#! /usr/local/bin/python

import string,os,sys,fpformat,math
import parser
sys.path.append("%s/scripts" % os.getcwd())

class CurveParser(parser.parser):
	def __init__(self):
		pass
	
	def isEqual(self, other_parser,results,analyzer):
		passed = self.validateMatching(self.data,other_parser.data,results)
		return self.compare(analyzer,other_parser.data,results)
	
	def validateMatching(self,curves1,curves2,results):
		passed = True
		
		for curve in curves1.keys():
			if not curve in curves2.keys():
				passed = False
				results[curve]= None
			else:
				if type(curves1[curve]) == dict:
					if curve not in results.keys():
						results[curve] = {}
					passed = self.validateMatching(curves1[curve],curves2[curve],results[curve])
					if passed:
						del results[curve]
		return passed
	
	def compare(	self,analyzer,curves2,results):
		curves1 = self.data
		test_passed = True
		analyzer.resetAnalysis()
		for key in curves1.keys():
			if not key in curves2.keys():
				test_passed = False
				m= {"message":"Unknown curve generated","baseline":"None","test_value":None}
				
				if not key in results.keys():
					results[key] = {"unkowncurve":m}
				else:
					print key
					results[key]["unkowncurve"] = m
			else:
				for timestep in curves1[key].keys():
					if key in curves2.keys() and not timestep in curves2[key].keys():
						test_passed = False
						m= {"message":"is unkown timestep to baseline","baseline":"None","test_value":None,"validtimestep":False}
						if not key in analysis_results.keys():
							results[key] = {timestep:m}
						else:
							results[key][timestep] = m
					else:
						a = curves2[key][timestep][0]
						base = curves1[key][timestep][0]
						if not analyzer.analyze(a,base):
							error = math.fabs(base-a)
							test_passed = False
							if math.fabs(base)>0.0:
								perc_diff = math.fabs((error/base)*100)
							else:
								perc_diff = 1.0 #math.fabs((a-1.0e-18))
							
							if  error > analyzer.max_error:
								analyzer.max_error = error
								analyzer.max_error_array[0] = error
								analyzer.max_error_array[1] = perc_diff
								analyzer.max_err_info[0] = key
								analyzer.max_err_info[1] =  timestep
									
							if perc_diff > analyzer.max_percent_error:
								analyzer.max_percent_error = perc_diff
								analyzer.max_perc_array[0] = perc_diff
								analyzer	.max_perc_array[1] = error
								analyzer.max_perc_info[0] = key
								analyzer.max_perc_info[1] =  timestep
							
							m= {"message":"Curve analysis failed",
								"baseline":fpformat.sci(base,analyzer.sig_digs),
								"test_value":fpformat.sci(a,analyzer.sig_digs),
								"perc":fpformat.sci(perc_diff,2),
								"validtimestep":True}
													
							if not key in results.keys():
								results[key] = {timestep:m}
							else:
								if results[key] == None:
									results[key] = {timestep:m}
								else:
									results[key][timestep] = m
		if test_passed:
			analyzer.max_error = 0.0
			
		return test_passed

	
	def writeResults(self,message,test,brief):
		error = fpformat.sci(float(test["error"]),4)
		a_error = fpformat.sci(float(test["assoc_error"]),4)
		pmcurve= test["error_perc_info"][0]
		pmts= test["error_perc_info"][1]
		emcurve= test["error_err_info"][0]
		emts= test["error_err_info"][1]
		message = "%s%s\n" % (message,test["status"])
		message = "%s%s" % (message,"%5s%-23s\n" % ("", "Max percent Error: %s%%" % fpformat.sci(float(test["perc_error"]),3)))
		message = "%s%s" % (message,"%5s%-22s Curve: %-23s Step: %s\n" % ("","Error: %s" % a_error,pmcurve,pmts))
		message = "%s%s" % (message,"%5s%-23s\n" % ("","Max Error: %s" % error))
		message = "%s%s" % (message,"%5s%-22s Curve: %-23s Step: %s\n\n" % ("","Perc Error: %s%%" % fpformat.sci(float(test["assoc_perc"]),3),emcurve,emts))

		info = test["info"]
		
		if not brief:
			for key in info.keys():
				message = "%s%s" % (message,"%4sCurve: %s\n" % ("",key))
				if not info[key] == None:
					if "unkowncurve" in info[key].keys():
						message = "%s%s" % (message,"%6sCurve not in baselines \n" %"")
					else:
						message = "%s%s" % (message,"%6sTimestep   %16s%22s%20s\n" %("","Baseline","Test","% Error"))
						for step in sorted(info[key].keys()):
							d = info[key][step]
							if d == None:
								message = "%s%s" % (message,"%6s%s has no matching test result\n" % ("",fpformat.sci(step,1)))
							elif not d["validtimestep"]:
								message = "%s%s" % (message,"%6s%-10s has no corresponding baseline value\n" % ("",fpformat.sci(step,1)))
							else:
								if not "perc" in d.keys():
									message = "%s%s" % (message,"%6s%-10s%24s%24s \n" % ("",fpformat.sci(step,1),d["baseline"]+(" "*((24-len(d["baseline"]))/2)),d["test_value"]+(" "*((24-len(d["test_value"]))/2))))
								else:
									message = "%s%s" % (message,"%6s%-10s%24s%24s%10s%% \n" % ("",fpformat.sci(step,1),d["baseline"]+(" "*((24-len(d["baseline"]))/2)),d["test_value"]+(" "*((24-len(d["test_value"]))/2)),d["perc"]))
				else:
					message = "%s%s" % (message,"%6sBaselines curve %s not generated in test\n" % ("",key))
		return message
			
	def generateAnswerFileList(self,base_dir,directory,test,files):
		home = os.getcwd()
		for pfile in os.listdir(home):
			if os.path.isdir(pfile):
				os.chdir(pfile)
				self.generateAnswerFileList(base_dir,directory,test,files)
				os.chdir(home)
			elif os.path.isfile(pfile):
				if pfile == test and not home == base_dir:
					files.append("%s/%s" % (home,test))
					
	def parseFile(self, test_answ):
		self.data = {}
		f = open(test_answ,'r')
		lines = f.readlines()
		length = len(lines)
		i = 0
		if not lines[i].startswith("#"):
			## We need to get passed some header garbage
			while not lines[i].startswith("#") or len(lines[i].strip()) == 0:
				i = i+1
		while i < length:
			if lines[i].startswith("#") or lines[i].startswith("end") or len(lines[i].strip()) == 0:
				i = i+1
			else:
				i,curvename,curve = self.readCurve(i,lines)
				if not curvename in self.data.keys():
					self.data[curvename] = curve
				else:
					for key in curve.keys():
						if not key in self.data[curvename].keys():
							self.data[curvename][key]= curve[key]
						else:
							self.data[curvename][key].append(curve[key][0])
		f.close()
		
	def parseAnswerFiles(self,directory,test_answ):
		home = os.getcwd()
		os.chdir(directory)
		files = []
		data = {}
		
		self.generateAnswerFileList(os.getcwd(),directory,test_answ,files)
		for file in files:
			self.parseAnswerFile(file,data)
		
		os.chdir(home)
		return data
		
	def readCurve(self,pos,lines):
		# we need to try and step back to locate the curve name
		max_length = len(lines)
		if pos >= max_length:
			return pos,None, None
		i = pos
		start = pos
		i = i-1
		while not lines[i].startswith("end") and i >= 0:
			testline =  lines[i].strip()
			if testline.find("rel ") > 0 or  testline.find("abs ") > 0 or testline == "#":
				pass
			elif testline.startswith("#"):
				# we assume we have found the name
				if len(testline.split()) >1:
					curname = " ".join(testline.split()[1:])
				else:
					curname = testline[1:]
				break
			i = i -1
		if i <0:
			## We had an issue and need to abort the process
			sys.exit(100)
		i = start
		curve = {}
		while not lines[i].strip().startswith("end") and not lines[i].strip().startswith("#") and len(lines[i].strip()) >1:
			vals = lines[i].strip().split()
			time = vals[0]
			value = fpformat.sci(float(vals[1]),17)
		
			curve[float(time)] = [float(value)]
			i = i+1
			if i >= max_length: 
				break
		return i+1, curname, curve

if __name__ == "__main__":
	pass
