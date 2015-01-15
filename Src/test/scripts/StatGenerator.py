#! /usr/local/bin/python

import string,os,sys,fpformat,math

"""
import scipy,numpy
from scipy import array

from scipy import stats
"""

import dyna_env,dyna_testsuites
from parser.answerparser import AnswerParser
"""
suite = "SLIDE"
test = "sslide10lr"

suite = "PLATE"
test = "plate1"

suite = "COUNT"
test = "d3samp1"
"""
suite = "BAR"
test = "bar11"
class StatAnalyzer:
	def __init__(self,verbose=False):
		self.verbose = verbose
		
	def analyze(self,test,suite,base_dir="."):
		parser = AnswerParser()
		if self.verbose:
			print suite,test
		test_file = "%s.answ" % test
		home = os.getcwd()
		os.chdir(base_dir)
		data = parser.parseAnswerFiles(suite,test_file)
		
		os.chdir(home)
		self.generateStats(data,test)
		
			
	def generateStats(self,data,test):
		outfile = open("%s.stats" % test,'w')
		for key in data.keys():
			
			steps = sorted(data[key].keys())
			length = len(steps)
			mins =[]#scipy.arange(length,dtype=scipy.Float)
			maxs = []#scipy.arange(length,dtype=scipy.Float)
			stddevs =[] #scipy.arange(length,dtype=scipy.Float)
			means =  []#scipy.arange(length,dtype=scipy.Float)
			i = 0
			if self.verbose:
				print "Curve name: %s" % key
				print "Standard deviation\nTimestep\tstd dev"
			for i in range(len(steps)):
				
				step =  steps[i]
				if self.verbose:
					print key,step
					print data[key][step]
				
				mins.append(self.smin(data[key][step]))#mins[i]=stats.tmin(data[key][step])
				maxs.append(self.smax(data[key][step]))#maxs[i]=stats.tmax(data[key][step],None)
				stddevs.append(self.stddev(data[key][step]))#stddevs[i] = stats.std(data[key][step])
				means.append(self.mean(data[key][step]))#means[i] = stats.mean(data[key][step])
				
				if self.verbose:
					print mins[i],maxs[i],stddevs[i],means[i]
				outfile.write("%s:%s,max=%s,min=%s,stddev=%s,mean=%s\n" % (key,fpformat.sci(step,1),fpformat.sci(maxs[i],16),fpformat.sci(mins[i],16),fpformat.sci(stddevs[i],16),fpformat.sci(means[i],16)))
				if self.verbose:
					print "%s\t%s" % (fpformat.sci(step,1),fpformat.sci(stddevs[i],15))
		
			#differences = maxs-mins
			#dmax = stats.tmax(differences,None)
			#dmin = stats.tmin(differences)
			#if self.verbose:
			#	print "\nMax difference of min and max: %s\nMin difference of min and max: %s\n" % (fpformat.sci(dmax,10),fpformat.sci(dmin,10))
		outfile.close()
	def parseAnswerFiles(self,files):
		data = {}
		for file in files:
			
			f = open(file,'r')
			lines = f.readlines()
			length = len(lines)
			i = 0
			if not lines[i].startswith("#"):
				## We need to get passed some header garbage
				while not lines[i].startswith("#"):
					i = i+1
			while i < length:
				if lines[i].startswith("#") or lines[i].startswith("end"):
					i = i+1
				else:
					i,curvename,curve = self.readCurve(i,lines)
					if not curvename in data.keys():
						data[curvename] = curve
					else:
						for key in curve.keys():
							if not key in data[curvename].keys():
								data[curvename][key]= curve[key]
							else:
								data[curvename][key].append(curve[key][0])
			f.close()
		return data
		
	def readCurve(self,pos,lines):
		# we need to try and step back to locate the curve name
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
					curname = testline.split()[1]
				else:
					curname = testline[1:]
				break
			i = i -1
		if i <0:
			## We had an issue and need to abort the process
			sys.exit(100)
		i = start
		curve = {}
		
		while not lines[i].strip().startswith("end"):
			vals = lines[i].strip().split()
			time = vals[0]
			value = fpformat.sci(float(vals[1]),17)
		
			curve[float(time)] = [float(value)]
			i = i+1

		return i+1, curname, curve
	
	def printDictionary(self,dictionary,level):
		
		tab = "\t"
		tabs=""
		for i in range(level):
			tabs += tab
		for key in sorted(dictionary.keys()):
			if isinstance( dictionary[key], dict):
				print tabs,key, " :"
				self.printDictionary(dictionary[key],level+1)
			else:
				print tabs, key," : " , dictionary[key]	
	def smin(self, in_array):
		
		if len(in_array) == 0: 
			return None
		if len(in_array) == 1: 
			return in_array[0]
			
		ret_val = in_array[0]
		index = 1
		while index < len(in_array):
			if ret_val>in_array[index]:
				ret_val = in_array[index]
			index = index+1
			
		
		return ret_val
		
	def smax(self, in_array):
		if len(in_array) == 0: 
			return None
		if len(in_array) == 1: 
			return in_array[0]
		ret_val = in_array[0]
		index = 1
		while index < len(in_array):
			if ret_val<in_array[index]:
				ret_val = in_array[index]
			index = index+1
			
		
		return ret_val
		
	def stddev(self, in_array):
		if len(in_array) == 1:
			return 0.0
		mean = self.mean(in_array)
		mean_squared_sum=0.0
		for val in in_array:
			mean_squared_sum = mean_squared_sum + math.pow(val-mean,2)
		return math.sqrt(mean_squared_sum/(len(in_array)-1))
		
	def mean(self, in_array):
		sum = 0.0
		for val in in_array:
			sum = sum + val
		return sum/len(in_array)
				
if __name__ == "__main__":
	if len(sys.argv) > 2:
		suite = sys.argv[1]
		test = sys.argv[2]
	verbose = False
	if "-v" in sys.argv:
		verbose = True
	stat_analyzer = StatAnalyzer(verbose)
	stat_analyzer.analyze(test,suite)
	
