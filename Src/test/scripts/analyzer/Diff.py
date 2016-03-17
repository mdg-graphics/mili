#! /usr/local/bin/python

import sys,string,popen2,os,math,fpformat

default_tolerance = 1.0e-3
default_sig_digits = -1
default_tolerance_type = "rel"
def usage():
	print "error"
class Diff:
	def __init__(self,  test_file, base_file,tolerance, sig_digits,type):
		self.base_file = base_file
		self.test_file = test_file
		if tolerance == None:
			self.tolerance = default_tolerance
		else:
			self.tolerance = tolerance
		if sig_digits == None:
			self.sig_digits=default_sig_digits
		else:
			self.sig_digits=sig_digits
		if type == None:
			self.tol_type = default_tolerance_type
		else:
			self.tol_type = type
		self.max_error = 0.0
	
	def diff_files(self,output_file):
		self.max_error = 0.0
		bf = open( self.base_file, 'r')
   		tf = open(self.test_file,'r')
		logger = open(output_file,'w')
		test_passed = True
		
		blines=[]
		tlines = []
		for line in bf:
			blines.append(line)
		for line in tf:
			tlines.append(line)
		if len(blines) == len(tlines):
			for i in range(len(blines)):
				if not blines[i] == tlines[i]:
					if not (blines[i].startswith("#") and tlines[i].startswith("#")) and blines[i].strip()[0].isdigit():
						info = blines[i].strip().split("#")[1]
						bnodes = blines[i].strip().split()
						tnodes = tlines[i].strip().split()
						if not tnodes[0] == bnodes[0]:
							logger.write( "time error: line - %d :%s %s for %s" % (i,tnodes[0],bnodes[0],info))
						if not tnodes[1] == bnodes[1]:
							if self.sig_digits >0:
								sfval = fpformat.sci(tnodes[1],self.sig_digits)
								tfloat = float(sfval)
								bfloat = float(fpformat.sci(float(bnodes[1]),self.sig_digits))
							else:
								tfloat = float(tnodes[1])
								bfloat = float(bnodes[1])
							"""
							if self.tol_type == "rel":
								if bfloat == 0.0:
									error = math.fabs(bfloat-tfloat)
								else:
									error = math.fabs(tfloat/bfloat)
							else: #abs
							"""

							
							error = math.fabs(bfloat-tfloat)
							if error > self.tolerance:
								test_passed= False
								if math.fabs(error) > math.fabs(self.max_error):
									self.max_error = error
								logger.write( "Error was %s \n" % fpformat.sci(error,5))
								#logger.write("%f" % math.fabs((bfloat-tfloat)))
								logger.write("line %d %s" % (i,tlines[i]))
								logger.write("line %d %s" % (i,blines[i]))
		else:
			logger.write("Line count mismatch running standard diff over files" )
			file =popen2.Popen4("diff %s %s" % (self.base_file,self.test_file))
			outfile = file.fromchild
					
			for line in outfile:
				logger.write(line)
			test_passed = False
				
			
		bf.close()
		tf.close()
		logger.close()
		return test_passed
		
	
	def findNextMatch(self, bf, tf, bline, tline):
		
		if self.bline_num > self.tline_num:
			pass
			
			
		
		
if __name__ == "__main__":
	if len(sys.argv) <3:
		usage()
		sys.exit(10)
	testfile = sys.argv[-1]
	baseline = sys.argv[-2]
	tol = default_tolerance 
	sig_digs = default_sig_digits
	tol_type = default_tolerance_type
	if "-v" in sys.argv:
		for i in range(len(sys.argv)):
			if sys.argv[i] == "-v":
				tol = float(sys.argv[i+1])
	if "-t" in sys.argv:
		for i in range(len(sys.argv)):
			if sys.argv[i] == "-t":
				tol_type = sys.argv[i+1]
	if "-d" in sys.argv:
		for i in range(len(sys.argv)):
			if sys.argv[i] == "-d":
				sig_digs = int(sys.argv[i+1])
	if os.path.exists(testfile) and os.path.exists(baseline):
		
		diff = Diff(testfile,baseline,tol,sig_digs,tol_type)
		diff.diff_files("diff.output")
	else:
		print "Invalid files: either the baseline file\n or the test file does not exist"
	
	
