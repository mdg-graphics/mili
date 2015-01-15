#! /usr/local/bin/python

import sys,string,popen2,os,math,fpformat
cur = sys.path[0]
i = cur.rfind(os.path.sep)
sys.path[:0] = [cur[:i]]

import parser.answerparser,parser.statparser,StatGenerator,Analyzer
default_tolerance = 1.0e-3
default_sig_digits = -1
default_tolerance_type = "rel"

class RangeAnalyzer(Analyzer.Analyzer):
	def __init__(self):
		self.parser = parser.answerparser.AnswerParser()
		self.stats = StatGenerator.StatAnalyzer()
		self.statsparser = parser.statparser.StatParser()
		
	def setAnalysis(self,base_dir,suite,test,sig_digs=4,stddev=2):
		self.test = test
		self.results = "%s.jansw" % test
		self.sig_digs = sig_digs
		self.base_dir = base_dir
		self.suite = suite
		self.statfile = "%s.stats" % test
		self.max_error = 0.0
		
	def analyze(self,output_file):
		analysis_results = {}
		self.max_error = -1.0e18
		logger = open(output_file,'w')
		test_passed = True
		if os.path.exists(self.results):
		
			self.stats.analyze(self.test,self.suite,base_dir=self.base_dir)
			statDict = self.statsparser.parseStats(".",self.test)
			answers = {}
			self.parser.parseAnswerFile(self.results,answers)
			test_passed = self.validateMatchingCurves(statDict,answers,analysis_results)
			for key in answers.keys():
				for timestep in answers[key].keys():
					if not timestep in statDict[key].keys():
						test_passed = False
						m= {"message":"is unkown timestep to baseline","baseline":"None","test_value":a}
						if not key in analysis_results.keys():
							analysis_results[key] = {timestep:m}
						else:
							analysis_results[key][timestep] = m	
					else:
						amin = round(statDict[key][timestep]["min"],self.getDecimalPlaces( statDict[key][timestep]["min"], int(self.sig_digs)))
						amax = round(statDict[key][timestep]["max"],self.getDecimalPlaces( statDict[key][timestep]["max"], int(self.sig_digs)))
						a = round(answers[key][timestep][0],self.getDecimalPlaces( answers[key][timestep][0], int(self.sig_digs)))
						
						if (a<amin and a>amax):
							
							if a < amin:
								message = "below minimum range"
								base = amin
							else:#a>amax
								message = "above maximum range"
								base = amax
							if math.fabs(base-a) > self.max_error:
								self.max_error = math.fabs(base-a)
							m= {"message":message,"baseline":base,"test_value":a}
							if not key in analysis_results.keys():
								analysis_results[key] = {timestep:m}
							else:
								analysis_results[key][timestep] = m						
							test_passed = False
			
		else:
			logger.write("%s file does not exist\n" % self.results)
			test_passed = False
		#print "results:",self.analysis_results		
		logger.close()
		if test_passed and self.max_error == -1.0e18:
			self.max_error = 0.0
		return test_passed,analysis_results
		
	
	
			
			
		
		
if __name__ == "__main__":
	analyzer = RangeAnalyzer()
	analyzer.setAnalysis(".","BAR","bar1")
	passed , results =analyzer.analyze("testlog.txt")
	
	print passed, results
