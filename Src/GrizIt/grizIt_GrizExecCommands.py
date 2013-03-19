############################################################################
# grizIt_GrizExecCommands.py - Griz Functions - Executive Commands.
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      May 07, 2010
#
# 
############################################################################
# Modifications:
#  I. R. Corey - May 07, 2010: Created.
#
############################################################################
#

import pdb
import os, sys, re, bisect, math
import visit

import grizIt_GuiMain as GUI
import grizIt_Mili as MILI
import grizIt_VisitInfo as VISITINFO

import grizIt_Session as S

env     = S.env
session = S.session

def ParseImportHelper():
    # Helps resolve the circular import issue
    from grizIt_ParseGrizCommand import GrizCommands, GRIZ_null#, GrizNoOp
    return GrizCommands, GRIZ_null, None#, GRIZ_NoOp

def FactoryImportHelper():
    from grizIt_GrizOnOffFactory import on_off_commands
    from grizIt_GrizSwitchFactory import switch_commands
    # from grizIt_GrizMtlFactory import mtl_commands --- TO DO: currently not organized in proper factory fashion with dictionary and help text. Go to grizIt_GrizMtlManagerCommands.py to see the old dictionary in a comment
    return on_off_commands, switch_commands

class refresh(S.OnOffFunction):
    """ Turn on (off) free node rendering mode"""
    ## No visible change in Griz. Will create flag.
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
    def on(self):
        session.refresh=True
    def off(self):
        session.refresh=False

class OnOffSu(S.OnOffFunction):
    """ Turn on (off) free node rendering mode"""
    ## No visible change in Griz. Will create flag.
    def __init__(self, flag):
        S.OnOffFunction.__init__(self, flag)
        self.flag = flag
    def on(self):
        session.refresh=True
    def off(self):
        session.refresh=False

#########################
# Executive Commands
#########################

class load(S.GrizFunction):
    def __init__(self,args):
        self.args = args
    def do(self):
        database = ' '.join(self.args)
        session.miliDB.openDB(database)
        env.populateVars()

class quit(S.GrizFunction):
    def __init__(self, args):
       self.args = args

    def do(self):
        env.exit = True
        GUI.closeGUI()

class info():
    """ This shouldn't be hard - this should just retrieve the info from the parse dictionary in GrizIt_ParseGrizCommand.py and return the info text"""
    def __init__(self):
        print "Warning: This command is not yet implemented"
    def do(self):
        pass        
    #end def

class r():
    def __init__(self):
        print "Warning: This command is not yet implemented"
    def do(self):
        pass
    #end def

class alias(S.GrizFunction):
    def __init__(self,args):
        self.args = args
        cmd = args[0]
        string = args[1]
    def do(self):
        self.aliasCmds[cmd] = string
    #end def

class Exec(S.GrizFunction):
    def __init__(self, args):
        self.args = args

    def do(self):
        options = ' '.join(self.args)
        p = os.popen(options)
        for line in p.readlines():
            print line.rstrip("\n")
    #end def

class commands(S.GrizFunction):
    """ Prints out all implemented commands - however, also prints if only halfway-implemented. Also prints out help text """
    def __init__(self):
        self.GrizCommands, self.GRIZ_null, self.GRIZ_NoOp = ParseImportHelper() #imports necessary functions
        self.on_off_commands, self.switch_commands = FactoryImportHelper()
    def do(self):
        for command in self.GrizCommands.keys():
            if self.GrizCommands[command][0] != self.GRIZ_null and self.GrizCommands[command][0] != self.GRIZ_NoOp:
                print command, '\t\t', self.GrizCommands[command][3]
        # from command factories


class run_tests(S.GrizFunction):
    """ This function runs the tests from the given test file.

        Future work on this function would be adding a return to default
        at the end
    """
    def __init__(self, filename):
        
        from grizIt_TestSuite import getCommand # Maybe this should be put at the top, but these only need to be imported for this function
        from grizIt_ParseGrizCommand import ParseCommand
        
        print "NOW RUNNING TESTS FROM TEST FILE %s" % (filename)
        
        self.filename = filename
        gritIt_Session.env.tests_file = filename
        self.commandMaker = getCommand()

    def do(self):
        commandList = commandMaker.get_command_list()
        for command in commandList:    
            if not command:
                continue
            else:
                ParseCommand(command)
                
        # return to default would be nice to have here 

        print 'Tests Complete'

###################
# On/Off Functions
###################

       
