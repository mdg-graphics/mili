#! /usr/local/bin/python

import sys,string,os,math,fpformat
import dyna_env

class parser:
	def __init__(self):
		pass
	
	def parseFile(self, file,storage):
		raise Exception,"Error abstract method must be implemented"
	def writeResults(self,message,results,brief):
		print message
	
	

