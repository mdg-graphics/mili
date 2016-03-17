#! /usr/local/bin/python

import string,os,sys,fpformat

sys.path.append("%s/scripts" % os.getcwd())

class StatParser:
	def __init__(self):
		pass

	def parseStats(self,suite,test):
		data = {}
		if os.path.exists("%s%s%s.stats" % (suite,os.path.sep,test)):
			file = open("%s%s%s.stats" % (suite,os.path.sep,test))
			for line in file:
				tdata = {}
				nodes = line.strip().split(",")
				
				for n in nodes:
					if n.find("=")>0:
						n1 = n.split("=")
						tdata[n1[0]]=float(n1[1])
					else:
						n1 = nodes[0].split(":")
						curve = n1[0]
						timestep = fpformat.sci(float(n1[1]),1)
				if not curve in data.keys():
					data[curve] = {timestep:tdata}
				else:
					data[curve][timestep] = tdata

		return data
		
		
if __name__ == "__main__":
	if len(sys.argv) > 2:
		suite = sys.argv[1]
		test = sys.argv[2]
	
	parser = StatParser()
	data = parser.parseStats(suite,test)
	print data
