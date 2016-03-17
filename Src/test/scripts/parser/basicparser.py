#! /usr/local/bin/python

import sys,string,os,math,fpformat

import parser

class localparser(parser.parser):
	def __init__(self,env,absolute):
		#parser.parser(env,absolute)
		self.environment=env
		self.absolute = absolute
		self.storage={"time":"","timesteps":"","nodes":{},"elements":{},"problemLine":"","termString":"a b n o r m a l  t e r m i n a t i o n"}
		
	def endNode(self, line):
		return (line.find("Model") >=0 or line.find("Restart") >=0)
	def endElement(self, line):
		return not line.find("Problem") == -1 or not line.find("Restart") == -1 or not line.find("restart") == -1
	def elementBlockStart(self, line):
		return line.startswith("Hex Block Print Out -")
	def elementStart(self, line):
		return line.startswith("Hex:")
	def nodeBlockStart(self,line):
		return line.startswith("Nodal Point Print Out -")
	def nodeStart(self,line):
		return line.startswith("Node:")

	 		
