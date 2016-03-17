#! /usr/local/bin/python
"""
A simple program to generate patitioned dyna files for the tests
to run

dynapartGen.py directory num processors[ num processors  ...]
example dynapartGen.py BAR 2 4 8

"""
import sys,string,os,popen2

class dynapart:
	def __init__(self, dir, file=None):
		self.init(dir, file)
	def init(self, dir, file):
		if os.environ['PATH'].find("/usr/gapps/mdg/bin") <0:
			os.environ['PATH'] = "%s:%s" % ("/usr/gapps/mdg/bin",os.environ['PATH'])
		self.dir = dir
		self.file = file
		
	def runPartition(self, file, processors):
		self.startfiles.append(file)

		if not self.validDynaFile(file):
			if not file.endswith("dyn"):
				if os.path.exists(os.getcwd()+os.path.sep+file+".dyn"):
					file = file+".dyn"
		if self.validDynaFile(file):
			print file
			for p in processors:
				if not p.isdigit():
					continue
				self.startfiles.append("%s.%s" % (file,p))
				nfile = popen2.Popen4("dynapart.beta %s %s" % (file,p))
				outfile = nfile.fromchild
				for line in outfile.readlines():
					pass
				
	def run(self, processors):
		
		home = os.getcwd()
		os.chdir(self.dir)
		self.startfiles = os.listdir(".")
		if self.file == None:
			files = os.listdir(".")
			for f in files:
				self.runPartition(f,processors)
		else:
			self.runPartition(self.file,processors)
		self.cleanup()
		os.chdir(home)
		return
	
	def cleanup(self):
		files = os.listdir(".")
		for f in files:
			if not f in self.startfiles:
				os.remove(f)
				
	def validDynaFile(self, filename):
		if filename.endswith(".dyn"):
			return True
	
		return False
		
if __name__ == "__main__":
	dir = sys.argv[1]
	if sys.argv[2].isdigit():
		dp = dynapart(dir)
		dp.run(sys.argv[2:])
	else:
		file = sys.argv[2]
		dp = dynapart(dir,file=file)
		dp.run(sys.argv[3:])
	
	
	

