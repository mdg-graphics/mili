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
		suffix = "frc"
		
		for f in files:
			if f.startswith("%s.%s" % (test,suffix)):
				filelist.append(open("%s/%s" % (dir,f)))
		
		#filelist.append(open("%s.%s" % (test,suffix)))
		if len(filelist) == 0:
			return None
		
		for file in filelist:
			self.readFile(file)
			
		self.closeFiles(filelist)
		return self.storage
	
	def readFile(self, file):
		line = file.readline()
		lastnodestep = 0
		lastelementstep = 0
		while not line == "":
			line =  line.strip()
			nodes = line.split()
			self.readElements(file,float(nodes[4],self.convertScientificNotation(nodes[4])))
			line = file.readline()
			
	def readElements(self,file,step,time):
		time_step = step
		
		current_nodes = self.storage["elements"]
		mapping = self.environment["elements"]
		nodes= line.split()
		yforce = nodes[8]
		xforce = nodes[6]
		xvarname = "%s_x_force" % nodes[2]
		yvarname = "%s_y_force" % nodes[2]
		face = nodes[1]
		print "face",face 
		print yvarname,"=",yforce
		print xvarname,"=",xforce
		
