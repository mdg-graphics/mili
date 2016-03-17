#! /usr/local/bin/python

import sys,string,os,math,fpformat

import parser

class localparser(parser.parser):
	def __init__(self,env,absolute):
		#parser.parser(env,absolute)
		self.environment=env
		
		self.absolute = absolute
		self.storage={"time":"","timesteps":"","nodes":{},"elements":{},"problemLine":"","termString":"a b n o r m a l  t e r m i n a t i o n"}

	def parseFiles(self, dir , test):
		files = os.listdir(dir)
		filelist=[]
		suffix = "hsp"
		for f in files:
			if f.startswith("%s.%s" % (test,suffix)):
				filelist.append(open("%s/%s" % (dir,f)))
			if f == "forrct":
				filelist.append(open("forrct"))
		
		#filelist.append(open("%s.%s" % (test,suffix)))
		if len(filelist) == 0:
			return None
		for file in filelist:
			self.readFile(file)
				
		self.closeFiles(filelist)
		return self.storage
	
	
	def elementBlockStart(self, line):
		return (line.startswith("Beam Block Print") and len(self.environment["elements"].keys()) >0)
	def elementStart(self, line):
		return line.startswith("Beam:")
	def nodeBlockStart(self,line):
		return (line.startswith("Nodal Point Print Out -") or line.startswith("Reaction Forces"))
	def nodeStart(self,line):
		return line.startswith("Node:")
	def endNode(self, line):
		return (line.find("model") >=0 or line.find("Restart") >=0 or line.find("Analysis") >=0 or line == "")
	def endElement(self, line):
		return (line.find("Problem") >= 0  or line.find("Restart") >= 0 )
