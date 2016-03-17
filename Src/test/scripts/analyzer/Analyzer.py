#! /usr/local/bin/python

import string,fpformat,math,sys,os
from parser import *

class Analyzer:
	def __init__(self):
		self.sig_digs = 15

	def getDecimalPlaces(self, number ):
		num = fpformat.sci(number,self.sig_digs+1)
		
		nodes = num.split("e")
		
		val = int(nodes[1])
		orig =  val
		if val <= 0:
			val = abs(val)+self.sig_digs
		else:
			if val < self.sig_digs:
				val = self.sig_digs-val
			elif val > self.sig_digs:
				val = self.sig_digs-val-1
			else:
				val = 0
		
		return val
	
	def setBasicParams(self,base_dir,suite,test,summary,logfile):
		self.summary = summary
		self.logfile=logfile
		self.test = test
		self.base_dir = base_dir
		self.suite = suite
		
	def setAnalysis(self,sig_digs=16,stddev=2):
		raise Exception,"Error abstract method must be implemented"
	def analyze(self,base_answers,answers,analysis_results,tol=0.0):
		raise Exception, "Error abstract method must be implemented"
	def writeTestOutput(self, suite, testname,test,quiet,brief):
		raise Exception, "Error abstract method must be implemented"


	
if __name__ == "__main__":
	a = Analyzer()
	a.setAnalysis(None,None,None)
