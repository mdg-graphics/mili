#!/usr/local/bin/python

import string,  time,os, sys, Image, ImageChops, ImageStat, ImageOps
#import string,  time,os, sys

import parser

class ImageParser(parser.parser):
	
	def __init__(self):
		self.image = None
		pass

	def isEqual(self, other_parser,results,analyzer):
		rval = True
		if(other_parser.image == None):
			results["status"]="setup_failure";
			results["message"]="The file %s does not exist." %other_parser.input_file;
			return False
		if(self.image == None):
			results["status"]="setup_failure";
			results["message"]="The file %s does not exist." %self.input_file;
			return False
		
		if(self.image.size != other_parser.image.size):
			other_parser.image = other_parser.image.resize(self.image.size, Image.BICUBIC);

		im_data = self.image.getdata();
		im2_data = other_parser.image.getdata();
		diff_count = 0;
    
		for im_pixel, im2_pixel in zip(im_data,im2_data):
			if not analyzer.analyze(im_pixel, im2_pixel):
				diff_count = diff_count+1;
        
			dcount = 1.0 * (self.image.size[0]*self.image.size[1]);
    
		perc = (diff_count / dcount) * 100.0;
		if perc != 0.0:
			rval = False
			index = self.input_file.rfind(".")
			file_name = "%s_difference" %self.input_file[:index]
			
			diffImage = self.createDifferenceImage(self.image, other_parser.image)
			size = 128,128;
			diffImage.save( file_name+".jpeg","JPEG");
			diffImage.thumbnail(size);
			th_file = file_name+".thumbnail.jpeg";
			
			diffImage.save(th_file,"JPEG")
			
			results["message"]="Image analysis failed. Difference image file is %s"% file_name+".jpeg";
		return rval
		
	def parseFile(self, test_answ):
		self.input_file = test_answ
		if(os.path.isfile(test_answ)):
			self.image = Image.open(test_answ);

	def writeResults(self,message,test,brief):
		message = "%4s%s\n" % (message,test["status"]);
		
		message = "%s%s" % (message,"%4s%s\n"%("",test["info"]["message"]));
		
		return message
			
	def createDifferenceImage(self,image1, image2):
    
		im_diff = ImageChops.difference(image1,image2).convert("L");
		im_diff = ImageOps.invert(im_diff);
		return (im_diff);
    
	def main(baseImage, testImage, resultOutputDir, filename, percent_threshold):

		outputTestFile = open(resultOutputDir+"/"+filename,"w");
		outputTestFile.truncate(0);

		if(os.path.isfile(baseImage)):
			im = Image.open(baseImage);
		else:
			print "Warning: No baseline image: ",baseImage;
			outputTestFile.write("Test Failure: No Base Test\n");
			outputTestFile.close();
			return;
    
		if(os.path.isfile(testImage)):
			im2 = Image.open(testImage);
		else:
			print "Warning: Could not locate test image: ",testImage;
			outputTestFile.write("Test Failure: Could Not Open Test Image \n");
			outputTestFile.close();
			return;
		
		if(im.size != im2.size):
			im2 = im2.resize(im.size, Image.BICUBIC);

		im_data = im.getdata();
		im2_data = im2.getdata();
		diff_count = 0;
    
		for im_pixel, im2_pixel in zip(im_data,im2_data):
			if(im_pixel != im2_pixel):
				diff_count = diff_count+1;
        
			dcount = 1.0 * (im.size[0]*im.size[1]);
    
		perc = (diff_count / dcount) * 100.0;
    
        
		if perc > float(percent_threshold):
			outputTestFile.write("passed false\n");
        
			diffImage = createDifferenceImage(im, im2);
                
        
			dstat   = ImageStat.Stat(diffImage)
			dmin    = dstat.extrema[0][0];
			dmax    = dstat.extrema[0][1];
			dmean   = dstat.mean[0];
			dmedian = dstat.median[0];
			drms    = dstat.rms[0];
			dstddev = dstat.stddev[0];
        
			size = 128,128;
			diffImage.save( resultOutputDir+"/"+filename+".jpeg","JPEG");
			diffImage.thumbnail(size);
			th_file = resultOutputDir+"/"+filename+".thumbnail.jpeg";
			diffImage.save(th_file,"JPEG")
        
		else:
			outputTestFile.write("passed true\n");


		outputTestFile.write("percent %s \n" % perc);

		if perc > float(percent_threshold):
			outputTestFile.write("minimum %s \n" % dmin);
			outputTestFile.write("maximum %s \n" % dmax);
			outputTestFile.write("mean %s \n" % dmean);
			outputTestFile.write("median %s \n" % dmedian);
			outputTestFile.write("root_mean_square %s \n" % drms);
			outputTestFile.write("standard_deviation %s \n" % dstddev);
        

		outputTestFile.close();
    

def parseArgs(args):
	valid = True
	inFileName =None
	baseFileName = None
	resultFileName = None
	dirName = None
	tolerance = 0.0
	i=0
	while(i<len(args)):
		if args[i].startswith("-i"):
			if len(args[i]) == 2:
				i = i+1
				inFileName = args[i]
			else:
				inFileName = args[i][2:]
			print "found input",inFileName
		if args[i].startswith("-b"):
			if len(args[i]) == 2:
				i = i+1
				baseFileName = args[i]
			else:
				baseFileName = args[i][2:]
			print "found base",baseFileName
		if args[i].startswith("-r"):
			if len(args[i]) == 2:
				i = i+1
				resultFileName = args[i]
			else:
				resultFileName = args[i][2:]
			print "found result Name",resultFileName
		if args[i].startswith("-d"):
			if len(args[i]) == 2:
				i = i+1
				dirName = args[i]
			else:
				dirName = args[i][2:]
			print "found outdirectory",dirName
		if args[i].startswith("-t"):
			temp_tolerance = None
			if len(args[i]) == 2:
				i = i+1
				temp_tolerance = args[i]
			else:
				temp_tolerance = args[i][2:]
			try:
				tolerance = float(temp_tolerance)
			except:
				print "Invalid tolerance setting tolerance to zero."
			
			print "found tolerance",tolerance
		i = i+1
	if 	inFileName == None:
		print "No file to compare with: Exiting."
		valid = False;
	if baseFileName == None:
		print "No Base file given: Exiting."
		valid = False
	if valid == False:
		sys.exit(0)
	if dirName == None:
		dirName = "."
	if  resultFileName == None:
		resultFileName = inFileName+".result"
	return baseFileName,inFileName,dirName,resultFileName,tolerance	

if __name__=="__main__":
	args = parseArgs(sys.argv)
	ia = ImageAnalyzer();
	#sys.exit()
	#main(parseArgs(sys.argv))
	ia.main(args[0],args[1],args[2],args[3],args[4])
