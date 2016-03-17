#! /usr/local/bin/python

import sys,string,popen2,os,math,fpformat
import parser.answerparser
import Analyzer
default_tolerance = 1.0e-3
default_sig_digits = 16
default_tolerance_type = "abs"

class AbsoluteAnalyzer(Analyzer.Analyzer):
	def __init__(self):
		self.max_percent_error = -1.0
		self.max_error_array = [0.0, 0.0]
		self.max_perc_array = [0.0, 0.0]
		self.max_err_info = ["",""]
		self.max_perc_info= ["",""]
		self.max_error=0.0

	def resetAnalysis(self):
		self.max_percent_error = -1.0
		self.max_error_array = [0.0, 0.0]
		self.max_perc_array = [0.0, 0.0]
		self.max_err_info = ["",""]
		self.max_perc_info= ["",""]
		self.max_error=0.0
		
	def setAnalysis(self,sig_digs=16,stddev=2):
				
		digs = int(sig_digs)
		if digs>0:
			self.sig_digs = digs-1
		else:
			self.sig_digs=0
	
		
	#def analyze(self,tol=0.0):
	def analyze(self,base_answer,answer,tol=0.0):
		if type(base_answer) == float and type (answer) == float :
			base_answer = round(base_answer,self.getDecimalPlaces(base_answer))
			answer = round(answer,self.getDecimalPlaces(answer))
						
		return base_answer == answer
	
		
		
if __name__ == "__main__":
	analyzer = AbsoluteAnalyzer()
	analyzer.setAnalysis(None,None,None)

