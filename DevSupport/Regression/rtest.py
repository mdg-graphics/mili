#!/usr/bin/env /grdev/regrtest/bin/python
   

import os,sys,re,string

verbose=1
RHOME="/grdev/regrtest/"
GRIZ2=RHOME+"bin/griz2"
GRIZ4=RHOME+"bin/griz4"
TAURUS="../d3plot"
MILI="../m_plot"
H1="# This grizinit file was automatically generated\n"
H2="# by regrtest.py for regression testing of Griz4\n"
H3="# -Vic Castillo\n#\n"
HEADER=H1+H2+H3

TEST={}
TEST['bin']=GRIZ2,GRIZ4,GRIZ4
TEST['db']=TAURUS,TAURUS,MILI
TEST['dir']='G2T','G4T','G4M'

CASE={}
CASE['SAMP']='SAMP1','SAMP2','SAMP4','SAMP6','SAMP8'
CASE['SND']='SND1','SND2','SND3'

# Derived Results Dictionary
DR={}
drnd1='dispx','dispy','dispz','dispmag'
drnd2='velx','vely','velz','velmag'
drnd3='accx','accy','accz','accmag','pvmag'
DR['Nodal']=drnd1+drnd2+drnd3
drshr1='sx','sy','sz','sxy','syz','szx'
drshr2='press','seff','pdev1','pdev2','pdev3'
drshr3='maxshr','prin1','prin2','prin3'
DR['Share']=drshr1+drshr2+drshr3
drshl1='surf1','surf2','surf3','surf4','surf5','surf6'
drshl2='eff1','eff2','effmax'
DR['Shell']=drshl1+drshl2
drbrk1='ex','ey','ez','exy','eyz','ezx'
drbrk2='pdstrn1','pdstrn2','pdstrn3','pshrstr'
drbrk3='pstrn1','pstrn2','pstrn3','relvol','evol'
DR['Brick']=drbrk1+drbrk2+drbrk3

# Primal Results Dictionary
PR={}
PR['Nodal']='nodpos[ux]',
PR['Global']='ke',
PR['Mat']='matpe',
PR['Brick']='eeff',
PR['Shell']='eeff_mid',

# Time History Dictionary
# These are the results selected for the time histories
TH={}
TH['Nodal']='dispmag',
TH['Global']='ke',
TH['Mat']='matpe',
TH['Brick']='prin1',
TH['Shell']='effmax',

# Info String Dictionary
# These are the string patterns from the griz 'info' command.
# (Why does it not work without the commas?)
IS={}
IS['Nodal']='Nodes:',
IS['Brick']='Hex elements:',
IS['Shell']='Shell elements:',
# IS['Beam']='Beam elements:',

# These are the strings describing result types
RS={}
RS['DR']='Derived Results'
RS['PR']='Primative Results'
RS['TH']='Time History Results'



#from rtest_support import * #Not used for NoWeb version
SHORTLIST=CASE['SAMP'][0],
LONGLIST=CASE['SAMP']+CASE['SND']
CASELIST=SHORTLIST
verbose=1
clean=0


   

def main():
   elem_type_list=[]
   for sample in CASELIST:
      os.chdir(RHOME+sample+"/")
      elem_type_list=read_info()
      print sample,": ",elem_type_list 
      #ok=griz_it(sample,'DR',elem_type_list)
      ok=griz_it(sample,'TH',elem_type_list)
      #ok=griz_it(sample,'PR',elem_type_list)
   return 0


   

def read_info(): 
   "Parse ./.info file to determine relevant element types"
   if not os.path.exists('.info'):
      print("ERROR: No info file found. Run rtest_init.py.")
      return -1
   info=open('.info','r+')
   raw = info.read() 
   info.close()
   keylist=[]
   for key in IS.keys():
      etest = re.compile(IS[key][0])
      if etest.search(raw):
         keylist.append(key)
   return keylist


   

def make_g_file(result_type,elem_type):
   g=open('grizinit','w+')    #create grizinit file 
   g.write(HEADER)
   if result_type=='DR':
      for result in DR[elem_type]: 
         hisfile='DR'+elem_type+result+".his"
         g.write("show "+result+"\n")
         g.write("savtxt "+hisfile+"\n")
         g.write("tellmm\n")
         g.write("endtxt\n")
      g.write("savtxt done\n")
      g.write("endtxt\n")
   elif result_type=='PR':
      for result in PR[elem_type]: 
         hisfile='PR'+elem_type+result+".his"
         g.write("show "+result+"\n")
         g.write("savtxt "+hisfile+"\n")
         g.write("tellmm\n")
         g.write("endtxt\n")
      g.write("savtxt done\n")
      g.write("endtxt\n")
   elif result_type=='TH':
      for result in TH[elem_type]: 
         hisfile='TH'+elem_type+result+".his"
         g.write("select node 1\n")
         g.write("select beam 1\n")
         g.write("select shell 1\n")
         g.write("select brick 1\n")
         g.write("show "+result+"\n")
         g.write("timhis \n")
         g.write("outth "+hisfile+"\n")
      g.write("savtxt done\n")
      g.write("endtxt\n")
   else:
      g.write("quit\n")
      g.close()
      return 0
   g.write("quit\n")
   g.close()
   return 1


   

def griz_it(sample,result_type,elem_type_list):
   result_type_string=RS[result_type]
   os.chdir(RHOME+sample)
   for elem_type in elem_type_list:
      rprint("Testing "+elem_type+" "+result_type_string+" with "+sample)
      for i in range(len(TEST['bin'])):
         GRIZBIN=TEST['bin'][i]   #selects the binary, Griz2 or Griz4
         DB=TEST['db'][i]         #selects the database, Taurus or Mili
         DIR=TEST['dir'][i]       #selects the appropriate directory for results
         if not os.path.exists(DIR):
            # if appropriate directory does not exist, create it
            if verbose:print "creating directory: "+DIR
            os.mkdir(DIR)
         os.chdir(DIR)              #change to appropriate directory
         ok=make_g_file(result_type, elem_type)   #create grizinit file 
         err=os.system(GRIZBIN+" -i "+DB)   #run griz with new grizinit file
         os.chdir('..')
      #force all runs to finish before comparing results
      for i in [0,1,2]:
         DIR=TEST['dir'][i]
         if not os.path.exists(DIR):
            print("ERROR: Directory "+DIR+"was not created")
         if not os.path.exists(DIR+'/done'):
            print("waiting for "+DIR+"...")
         while not os.path.exists(DIR+'/done'):
            pass
      for i in [0,1,2]:
         os.remove(TEST['dir'][i]+'/done')
      ok=diff_it(result_type,elem_type)
   rprint("fini")
   return 1


   

def onespace(filename):
   tempfilename='tempfile'
   tempfile=open(tempfilename,'w+')
   datafile=open(filename,'r+')
   datin=datafile.readlines()
   for i in range(len(datin)):
      j=string.join(string.split(datin[i]))
      tempfile.write(j+"\n")
   datafile.close()
   tempfile.close()
   err=os.system('mv '+tempfilename+' '+filename)
   return 1


   

def diff_it(result_type,elem_type):
   # compare results for G2T and G4T and G4M
   for i in [0,1]:
      DIR1=TEST['dir'][i]
      DIR2=TEST['dir'][i+1]
      rprint("Comparing results: "+DIR1+" and "+DIR2)
      ext="."+string.lower(result_type)
      TEMP=DIR1+DIR2+elem_type+ext
      err=os.system('echo  >'+TEMP)
      for result in eval(result_type)[elem_type]:
         TARG1=DIR1+'/'+result_type+elem_type+result+".his"
         TARG2=DIR2+'/'+result_type+elem_type+result+".his"
         if result_type =='TH':
            ok=onespace(TARG1)
            ok=onespace(TARG2)
         err=os.system('diff '+TARG1+' '+TARG2+'>>'+TEMP)
   for i in [0,1,2]:
      for result in eval(result_type)[elem_type]:
         hisfile=TEST['dir'][i]+'/'+result_type+elem_type+result+".his"
         if not os.path.exists(hisfile):
            msg="File not created: "+hisfile
            rprint(msg)
            err=os.system('echo '+msg+' >>'+TEMP)
         else:
            if clean:
               err=os.remove(hisfile)
   return 1


   

def rprint(text):
   "Reports information to report file and console if verbose"
   # r.write(text+"\n")
   if verbose: print(text) 
   return 0


   

if __name__ == '__main__':
   sys.exit(main())




