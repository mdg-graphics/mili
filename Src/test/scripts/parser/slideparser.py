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

		#filelist.append(open("%s/%s" % (dir,f)))
		filelist.append(open("%s.hsp" % test))
		if len(filelist) == 0:
			return None
		
		for file in filelist:
			print "file",file.name
			self.readFile(file)
			
		self.closeFiles(filelist)
		return self.storage
	
	def readFile(self, file):
		line = file.readline()
		lastnodestep = 0
		lastelementstep = 0
		while not line == "":
			line =  line.strip()
			if line.startswith("problem time"):
				self.storage["problemLine"] = line
				node = line.split()
				self.storage["time"]=node[3]
				self.storage["timesteps"]=node[-1]
			elif line.startswith("n o r m a l"):
				print line
				self.storage["termString"] = line
			elif file.name.endswith(".frc"):
				
				nodes = line.split()
				self.readElements(nodes,float(nodes[4]),self.convertScientificNotation(nodes[4]))
			line = file.readline()
			
	def readElements(self,nodes,step,time):
		time_step = step
		
		current_nodes = self.storage["elements"]
		#mapping = self.environment["elements"]
		#nodes= line.split()
		yforce = nodes[8]
		xforce = nodes[6]
		xvarname = "%s_force_x" % nodes[2]
		yvarname = "%s_force_y" % nodes[2]
		face = int(nodes[1])

		if not face in current_nodes.keys():
			current_nodes[face] = {xvarname:{"timesteps":{time_step:[time_step,xforce]}},
								   yvarname:{"timesteps":{time_step:[time_step,yforce]}}
								   }
		else:
			if not xvarname in current_nodes[face].keys():
				current_nodes[face][xvarname] = {"timesteps":{time_step:[time_step,xforce]}}
			else:
				if not time_step in current_nodes[face][xvarname]["timesteps"].keys():
					current_nodes[face][xvarname]["timesteps"][time_step] = [time_step,xforce]
			if not yvarname in current_nodes[face].keys():
				current_nodes[face][yvarname] = {"timesteps":{time_step:[time_step,yforce]}}
			else:
				if not time_step in current_nodes[face][yvarname]["timesteps"].keys():
					current_nodes[face][yvarname]["timesteps"][time_step] = [time_step,xforce]

					
		
