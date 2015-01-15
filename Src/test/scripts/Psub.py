#! /usr/local/bin/python

import os,string,popen2,sys
from string import Template
import dyna_optparser,dyna_env
user = os.getlogin()
if "SYS_TYPE" in os.environ.keys():
	systype = os.environ["SYS_TYPE"]
file = popen2.Popen4("uname -n")
outfile = file.fromchild
line = outfile.readline()
line = line.strip()
machine = line[:line.find("[0-9]")]

defaultTime = "4h"
psubfilename = "dyna3d.psub"
parallel_info = Template("")
prefix = "#! /usr/local/bin/bash\n\n"
lin1 = Template("#PSUB -N ${nodes} \n#PSUB -n ${processors}\n")
lin2 = Template("#PSUB -np ${processors}\n")
aix = Template("#PSUB -ln ${nodes} \n#PSUB -g ${processors}\n")
parallel_info= {"yana":lin2,
				"zeus":lin2,
				"thunder":lin1,
				"alc":lin1,
				"pengra":lin1,
				"up":aix,
				"ubgl":aix,
				"berg":aix,
				"newberg":aix}
#parallel_info_yana = Template("#PSUB -np ${processors}\n")
#parallel_info = Template("#PSUB -ln ${nodes} \n#PSUB -g ${processors}\n")

allruns = "#PSUB -eo\n#PSUB -nr\n#PSUB -ro\n#PSUB -mn\nSYS_TYPE=`grep SYS_TYPE /etc/home.config | cut -d\" \" -f2`\nexport SYS_TYPE\n"
parallel_runs = ""
machineName = Template("#PSUB -c ${machine}\n")
time = Template("#PSUB -tM ${time}\n")
bank = Template("#PSUB -b ${bank}\n")
path_load = "PATH=/usr/local/bin:$PATH\nexport PATH\n"
commands = Template("cd ${testdir}\nscripts/Test.py ${commands}\n") 
command = Template("psub -c ${machine} ${psubfilename}.${machine}")
class Psub:
	def __init__(self,args):
		self.parallel = False
		self.machine_runs = {}
		self.codename = None
		self.parsePsubCommands(args)
		#self.params,self.suites = dyna_optparser.parseParameters(args[1:])
		if not self.codename == None:
			self.commandline = string.join(args[1:])
			if len(self.machine_runs.keys())>1:
				nodes = self.codename.split(os.path.sep)
				index = self.commandline.find(nodes[-1])
				self.commandline = self.commandline[0:index+len(nodes[-1])]+".${SYS_TYPE} "+self.commandline[index+len(nodes[-1]):]
			else:
				#lets do a sanity check at this point to find out if the code exists for the single list
				if not os.path.exists(self.codename):
					#lets see if they followed our naming convention
					if os.path.exists("%s.%s" % (self.codename,systype)):
						#ah, we are only running an a single machine but are using our naming convention
						nodes = self.codename.split(os.path.sep)
						index = self.commandline.find(nodes[-1])
						self.commandline = self.commandline[0:index+len(nodes[-1])]+".${SYS_TYPE} "+self.commandline[index+len(nodes[-1]):]
					else:
						#hmmm, no code found might as well exit
						print "No code found to execute"
						sys.exit(99)
		else:
			print "Error running code"
			sys.exit(99)
	def printUsage(self,error,message):
		if error:
			print "Error: %s" % message
		
		print"Psub.py \n  Usage:"
		print"%4s%-25s\n%12s%-15s\n" % ("","Psub.py machines=machine_name:bank[,machine_name:bank]* [no_run]","","[time=lcrm_run_time] [help]")

		print"%6s%-10s%-30s" % ("","help","prints this message")
		print"%6s%-10s%-30s" % ("","machines" ,"a pairing of machine name and bank seperated by a \':\'")
		print"%6s%-10s%-30s" % ("","","this be submitted as a comma seperated list.")
		print"%6s%-10s%-30s" % ("","no_run", "This generates the Psub file then exits.")
		print"%6s%-10s%-30s" % ("","time ", "default 4 hours(4h): Allows the user to set the run")
		print"%6s%-10s%-30s\n" % ("","","time for the job in LCRM notation (ie 1h or 1:00).")
		#print"Test Suite"
		
	def parsePsubCommands(self, args):
		this_args = [len(args)]
		this_args[0:] = args
		prev_arg = None
		self.time = defaultTime
		self.norun = False
		error_message = ""
		error = False
		for arg in this_args:
			if not error:
				if arg.startswith("machines"):
					ms = arg.split("=")
					if len(ms) <2:
						error = True
						error_message = "machines parameter not correct \"%s\""  % arg
						break
					if len(ms[1].split(",")[0].split(":")) < 2:
						error = True
						error_message = "machines parameter not correct \"%s\""  % arg
						break
					machines = arg.split("=")[1].split(",")
					for machine in machines:
						ma = machine.split(":")
						self.machine_runs[ma[0]] = {"bank":ma[1]}
					args.remove(arg)
				elif arg.startswith("time"):
					self.time = arg.split("=")[1]
					args.remove(arg)
				elif arg.startswith("no_run"):
					self.norun = True
					args.remove(arg)
				elif arg.startswith("help"):
					self.printUsage(error,None)
					dyna_optparser.usage(None)
					sys.exit(0)
				elif arg.startswith("-c"):			
					if dyna_optparser.isParameter(arg):
						prev_arg = arg
					else:
						pr = dyna_optparser.getString(len("-c"),arg)
						if not len(pr) == 0:
							if arg.startswith(os.path.sep):
								self.codename = pr
							else:
								self.codename = "%s/%s" % (os.getcwd(),pr)
						
				elif not prev_arg == None:
					if prev_arg == "-c":
						if arg.startswith(os.path.sep):
							self.codename = arg
						else:
							self.codename = "%s/%s" % (os.getcwd(),arg)
					prev_arg = None
		if error:
			self.printUsage(error,error_message)
			dyna_optparser.usage("Psub error")
			sys.exit(99)
		if not len(self.machine_runs.keys()) >0:
			self.printUsage(True,"Need at least one machine to generate psub")
			sys.exit(99)
		lib,nothing = dyna_optparser.parseParameters(args[1:])

		if lib == None:
			sys.exit(9)
		elif lib["parallel"]:
			self.parallel = True
			self.processors = lib["processors"]
			self.nodes = lib["nodes"]
				
			
	def createPsubScriptFile(self):
		for machine in self.machine_runs.keys():
			psubfile = open("%s.%s" % (psubfilename,machine),'w')
			psubfile.write(prefix)
			if self.parallel:
				psubfile.write(parallel_info[machine.lower()].substitute({"processors":self.processors,"nodes":self.nodes}))
			psubfile.write(machineName.substitute({"machine":machine}))
			psubfile.write(bank.substitute({"bank":self.machine_runs[machine]["bank"]}))
			psubfile.write(time.substitute({"time":self.time}))
			psubfile.write(allruns)			
			psubfile.write(path_load)
			psubfile.write(commands.substitute({"testdir":os.getcwd(),"commands":self.commandline}))
			psubfile.close()
		
	
	def runPsubScript(self):
		if not self.norun:
			for machine in self.machine_runs.keys():
				file = popen2.Popen4(command.substitute({"dir":os.getcwd(),"machine":machine,"psubfilename":psubfilename}))
				outfile = file.fromchild
				for line in outfile:
					print line
		
		

if __name__ == "__main__":
	psub = Psub(sys.argv)
	psub.createPsubScriptFile()
	
	psub.runPsubScript()
