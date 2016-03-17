#! /usr/local/bin/python

import string,os,sys,fpformat

sys.path.append("%s/scripts" % os.getcwd())

class ImageParser:
	def __init__(self):
		pass
	
class CurveParser:
	def __init__(self):
		pass
	
	def generateAnswerFileList(self,base_dir,directory,test,files):
		home = os.getcwd()
		for pfile in os.listdir(home):
			if os.path.isdir(pfile):
				os.chdir(pfile)
				self.generateAnswerFileList(base_dir,directory,test,files)
				os.chdir(home)
			elif os.path.isfile(pfile):
				if pfile == test and not home == base_dir:
					files.append("%s/%s" % (home,test))
					
	def parseAnswerFile(self, test_answ, data):
		
		f = open(test_answ,'r')
		lines = f.readlines()
		length = len(lines)
		i = 0
		if not lines[i].startswith("#"):
			## We need to get passed some header garbage
			while not lines[i].startswith("#"):
				i = i+1
		while i < length:
			if lines[i].startswith("#") or lines[i].startswith("end"):
				i = i+1
			else:
				i,curvename,curve = self.readCurve(i,lines)
				if not curvename in data.keys():
					data[curvename] = curve
				else:
					for key in curve.keys():
						if not key in data[curvename].keys():
							data[curvename][key]= curve[key]
						else:
							data[curvename][key].append(curve[key][0])
		f.close()			
	def parseAnswerFiles(self,directory,test_answ):
		home = os.getcwd()
		os.chdir(directory)
		files = []
		data = {}
		
		self.generateAnswerFileList(os.getcwd(),directory,test_answ,files)
		for file in files:
			self.parseAnswerFile(file,data)
		
		os.chdir(home)
		return data
		
	def readCurve(self,pos,lines):
		# we need to try and step back to locate the curve name
		i = pos
		start = pos
		i = i-1
		while not lines[i].startswith("end") and i >= 0:
			testline =  lines[i].strip()
			if testline.find("rel ") > 0 or  testline.find("abs ") > 0 or testline == "#":
				pass
			elif testline.startswith("#"):
				# we assume we have found the name
				if len(testline.split()) >1: 
					curname = testline.split()[1]
				else:
					curname = testline[1:]
				break
			i = i -1
		if i <0:
			## We had an issue and need to abort the process
			sys.exit(100)
		i = start
		curve = {}
		
		while not lines[i].strip().startswith("end") and len(lines[i].strip()) >1:
			vals = lines[i].strip().split()
			time = vals[0]
			value = fpformat.sci(float(vals[1]),17)
		
			curve[float(time)] = [float(value)]
			i = i+1

		return i+1, curname, curve

if __name__ == "__main__":
	pass
