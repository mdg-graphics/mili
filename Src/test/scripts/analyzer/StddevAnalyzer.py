#! /usr/local/bin/python

import sys,string,popen2,os,math,fpformat
cur = sys.path[0]
i = cur.rfind(os.path.sep)
sys.path[:0] = [cur[:i]]

import parser.answerparser,parser.statparser,StatGenerator,Analyzer
default_tolerance = 1.0e-3
default_sig_digits = -1
default_tolerance_type = "rel"

class StddevAnalyzer(Analyzer.Analyzer):
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
		self.stddevs = stddev
		self.statfile = "%s.stats" % test
		
		
	def analyze(self,output_file):
		self.max_error = -1.0e18
		analysis_results = {}
		logger = open(output_file,'w')
		test_passed = True
		if os.path.exists(self.results):
			self.stats.analyze(self.test,self.suite,base_dir=self.base_dir)
			statDict = self.statsparser.parseStats(".",self.test)
			answers = {}
			
			self.parser.parseAnswerFile(self.results,answers)
			test_passed = self.validateMatchingCurves(statDict,answers,analysis_results)
			for key in answers.keys():
				#print "curve: ",key
				for timestep in answers[key].keys():
					#print "timestep: ",timestep
					if not timestep in base_answers[key].keys():
						test_passed = False
						m= {"message":"is unkown timestep to baseline","baseline":"None","test_value":a}
						if not key in analysis_results.keys():
							analysis_results[key] = {timestep:m}
						else:
							analysis_results[key][timestep] = m	
					else:
						a = answers[key][timestep][0]
						mean = statDict[key][timestep]["mean"]
						stddev = statDict[key][timestep]["stddev"]
						variance = self.stddevs*stddev
						if not (a>=mean-variance and a<=mean+variance):
							message = "Outside %d standard deviations" % self.stddevs
							if a < mean-variance:
								#message = "below minimum range"
								base = mean-variance
							else:#a > mean+variance
								#message = "above maximum range"
								base = mean+variance
							if math.fabs((a-mean)/stddev) > self.max_error:
								self.max_error = math.fabs((a-mean)/stddev)
							m= {"message":message,"baseline":base,"test_value":a}
							if not key in analysis_results.keys():
								analysis_results[key] = {timestep:m}
							else:
								analysis_results[key][timestep] = m						
							test_passed = False
			
		else:
			logger.write("%s file does not exist\n" % self.results)
			test_passed = False
		logger.close()
		if test_passed and self.max_error == -1.0e18:
			self.max_error = 0.0
		return test_passed,analysis_results
		
	
	
			
			
		
		
if __name__ == "__main__":
	analyzer = StddevAnalyzer()
	analyzer.setAnalysis(".","BAR","bar1")
	passed , results =analyzer.analyze("testlog.txt")
	
	print analyzer.max_error,passed, results
