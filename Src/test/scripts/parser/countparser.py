#! /usr/local/bin/python

import sys,string,os,math,fpformat

import parser

class localparser(parser.parser):
	def __init__(self,env,absolute):
		self.environment=env
		
		self.current_parse_piece=""
		self.current_type = "element"
		
		self.absolute = absolute
		self.storage={"time":"","timesteps":"","nodes":{},"elements":{},"problemLine":"","termString":"a b n o r m a l  t e r m i n a t i o n"}

	def readFile(self, file):
		line = file.readline()
		lastnodestep = 0
		lastelementstep = 0
		while not line == "":
			line =  line.strip()
			if self.nodeBlockStart(line):
				nodes = line.split()				
				self.readNodes(file,float(nodes[-1]),self.convertScientificNotation(nodes[-1]))					
			elif self.elementBlockStart(line):
				nodes = line.split()
				self.readElements(file, float(nodes[-1]),self.convertScientificNotation(nodes[-1]))
			elif self.shellBlockStart(line):
				print line
				nodes = line.split()
				self.readShells(file, float(nodes[-1]),self.convertScientificNotation(nodes[-1]))
			elif line.startswith("problem time"):
				self.storage["problemLine"] = line
				node = line.split()
				self.storage["time"]=node[3]
				self.storage["timesteps"]=node[8]
				if len(self.environment["nodes"].keys()) ==0 and len(self.environment["elements"].keys()) ==0 :
					self.storage["nodes"]={"":{"":{"timesteps":{1:[node[3],int(node[8])]}}}}																
			elif line.startswith("n o r m a l"):
				self.storage["termString"] = line
			line = file.readline()
			
	def readShells(self,file,step,time):
		time_step = step
		
		current_nodes = self.storage["elements"]
		mapping = self.environment["elements"]
		
		notStart = True
		parameters = []
		line = file.readline().strip()
		while not self.endElement(line):
			if self.shellStart(line):
				shell_line = line
				nodes= line.split()
				face = nodes[4]
				cur_node = int(nodes[1])
				
				if not cur_node in current_nodes.keys():
					current_nodes[cur_node]={}
				
				line = file.readline().strip()
				
				
				while not line == "":
					nodes = line.split()
					
					for var in mapping.keys():
						mapping[var].keys()
						if "parse_piece" in mapping[var].keys():
							parse_piece = mapping[var]["parse_piece"]
						else:
							parse_piece =""
						if face == parse_piece:
							
							if nodes[0] in mapping[var]["parse_start"]:
							
								if cur_node in mapping[var]["elements"].keys() or "all" in mapping[var]["elements"].keys():
								
									self.eval(time_step,time,var,nodes,mapping[var],current_nodes[cur_node],cur_node,"elements")
									if "all" in mapping[var]["elements"].keys():
										current_nodes[cur_node][var]["tolerance"] = mapping[var]["elements"]["all"]["tolerance"]
										current_nodes[cur_node][var]["tol_type"] = mapping[var]["elements"]["all"]["which"]
									else:
										current_nodes[cur_node][var]["tolerance"] = mapping[var]["elements"][cur_node]["tolerance"]
										current_nodes[cur_node][var]["tol_type"] = mapping[var]["elements"][cur_node]["which"]
						
					line = file.readline().strip()
				
			else:
				line = file.readline().strip()
				
	def shellBlockStart(self, line):
		return line.startswith("Shell Block Print")
	def shellStart(self,line):
		return (len(self.environment["elements"].keys())>0 and line.startswith("Shell:"))
	def elementBlockStart(self, line):
		return ( len(self.environment["elements"].keys())>0 and line.startswith("Hex Block Print" ))
	def elementStart(self, line):
		return line.startswith("Hex:")
	def nodeBlockStart(self,line):
		return (len(self.environment["nodes"].keys()) >0 and line.startswith("Nodal Point Print Out -"))
	def nodeStart(self,line):
		return line.startswith("Node:")
	def endNode(self, line):
		return (line.find("Problem") >=0 or line.find("Simply") >=0 or line.find("Restart") >=0 or line.find("Boundary") >=0 )
	def endElement(self, line):
		return (line.find("Boundary") >=0 or line.find("Problem") >=0 or line.find("Restart") >=0 )
