#! /usr/local/bin/python

import sys,string,os,math,fpformat

import dyna_env,parser#,barparser,basicparser,beamparser,slideparser,countparser,plateparser,parser
from parser import *
class Join:
	def __init__(self,suite):
		self.suite = suite
		if suite == "BAR":
			name = "parser.barparser"
		mod =  __import__(name)
		
		components = name.split('.')
		for comp in components[1:]:
			mod = getattr(mod, comp)
		
		self.parser =mod.localparser(None,None)
		
	def processFiles(self, dir):
		print dir
		print self.parser
		return
		files = os.listdir(dir)
		filelist=[]
		suffix = "hsp"
		
		for f in files:
			if f.endswith(suffix):
				basename =  f
				origfile = "%s.orig" % f
				os.rename(f,origfile)
				
				base = open("%s/%s" % (dir,origfile))
				filelist.append(base)
			elif f.find(suffix) > 0:
				filelist.append(open("%s/%s" % (dir,f)))
		
		timesteps = self.joinFiles(filelist)
		
		outfile = open("%s/%s" % (dir,basename))
		
		
		self.writeFile(outfile,timesteps)
		
		outfile.close()
				
		self.closeFiles(filelist)
		
	def closeFiles(self,filelist):
		
		for f in filelist:
			f.close()
			
	def joinFiles(self,filelist):
		timesteps = {}
		for file in filelist:
			line = file.readline()
			while not line == "":
				testline = line.strip()
				if self.elementBlockStart(testline):					
					self.parseElementBlock(file,line,timesteps)
				if self.nodeBlockStart(testline):
					self.parseNodeBlock(file,line,timesteps)
				if self.shellBlockStart(testline):
					print line
					pass
				line = file.readline()
		#self.printDictionary(timesteps,0)
		return timesteps 
		
	def writeFile(self, outfile, timesteps ):
		for step in timesteps.keys():
			for line in timesteps[step]["blockstart"]:
				outfile.write(line)
			for section in timesteps[step]["pieces"].keys():
				for line in timesteps[step]["pieces"][section]["blockstart"]:
					outfile.write(line)
				for subsection in timesteps[step]["pieces"][section]["pieces"].keys():
					for line in  timesteps[step]["pieces"][section]["pieces"][subsection]:
						outfile.write(line)
	
			
			
	def parseElementBlock(self,file,line,timesteps):
		
		nodes = line.split()
		timestep =  float(nodes[-2])
		
		if not timestep in timesteps.keys():
			timesteps[timestep] = {"elements":{}}
		if not "elements" in timesteps[timestep].keys():
			timesteps[timestep]["elements"] = {}

		line = file.readline()
		
		while not self.elementStart(line):
			line = file.readline()
		beamfile = False
		while not self.endElement(line.strip()):
			nodes = line.strip().split()
			
			if line.startswith("beam/truss"):
				beamfile = True
				element = int(nodes[3])
				print element,line,timestep
				timesteps[timestep]["elements"][element]= [line]
				line = file.readline()
				tline = line.strip()
				
				while not tline.startswith("resultant"):
					timesteps[timestep]["elements"][element].append("\n")
					line = file.readline()
					tline = line.strip()
					
				timesteps[timestep]["elements"][element].append(line)
				line = file.readline()
				timesteps[timestep]["elements"][element].append(line)
				line = file.readline()
				tline = line.strip()
				while not tline.startswith("integration"):
					timesteps[timestep]["elements"][element].append("\n")
					line = file.readline()
					tline = line.strip()
				timesteps[timestep]["elements"][element].append(line)
				line = file.readline()
				tline = line.strip()
				while not tline == "":
					timesteps[timestep]["elements"][element].append(line)
					line = file.readline()
					tline = line.strip()
				timesteps[timestep]["elements"][element].append("\n")
				
				while not (tline.startswith("Dyna") or tline.startswith("beam/truss") or tline.startswith("Problem") or tline.startswith("Node")):
					line = file.readline()
					tline = line.strip()
			elif not beamfile:
				element = int(nodes[0].replace("-",""))
				nextline = file.readline()
				timesteps[timestep]["elements"][element]= [line,nextline]
				line = file.readline()
			else:
				line = file.readline()

			
	def parseNodeBlock(self,file,line,timesteps):
		nodes = line.split()
		timestep =  float(nodes[-2])
		
		if not timestep in timesteps.keys():
			timesteps[timestep] = {"nodes":{}}
		elif not "nodes" in timesteps[timestep].keys():
			timesteps[timestep]["nodes"] = {}
		
		line = file.readline()

		while not self.nodeStart(line.strip()):	
			line = file.readline()
		
		while not self.endNode(line.strip()):
			nodes = line.strip().split()
			node = int(nodes[0])
			timesteps[timestep]["nodes"][node]= [line]
			line = file.readline()
		
			
	def printDictionary(self,dictionary,level):
		
		tab = "\t"
		tabs=""
		for i in range(level):
			tabs += tab
		for key in sorted(dictionary.keys()):
			if isinstance( dictionary[key], dict):
				print tabs,key, " :"
				self.printDictionary(dictionary[key],level+1)
			else:
				print tabs, key," : " , dictionary[key]	
		
	def shellBlockStart(self, line):
		return False

	def shellStart(self,line):
		return (len(self.environment["elements"].keys())>0 and line.startswith("Shell:"))		

	
	def elementBlockStart(self, line):
		return line.startswith("e l e m e n t") or line.startswith("r e s u l t a n t s")
	
	def elementStart(self, line):		
		nodes = line.split()
		if line.startswith("beam/truss"):
			return True
		if len(nodes) == 0:
			return False
		node = nodes[0]
		return node.find("-") >= 0

	def endElement(self, line):
		testline = line.strip()
		return (testline == "" or  line.find("odel") >=0)

	def nodeBlockStart(self,line):
		return line.startswith("n o d a l   p r i n t   o u t ")

	def nodeStart(self,line):
		nodes = line.split()
		if len(nodes) == 0:
			return False
		return nodes[0].isdigit()
	
	def endNode(self, line):
		return (line.find("odel") >=0 or line.find("Restart") >=0)

		
if __name__ == '__main__':
	
	dir = sys.argv[1]
	suite = sys.argv[2]
	os.chdir(dir)
	joiner = Join(suite)
	joiner.processFiles(os.getcwd())
	
