############################################################################
# grizIt_TestSuite.py 
# 
#      Ivan R. Corey
#      Lawrence Livermore National Laboratory
#      
#
# 
############################################################################
# Modifications:
#  S.E.Drakeley - July 2010: Created.
#
############################################################################

from grizIt_ParseGrizCommand import * #ParseCommandx
import visit
import grizIt_Session as S
session = S.session
import grizIt_Mili as M


class NullCommandMaker(object):
    def __init__(self):
        self.runTest = False

class CommandList(object):
    def __init__(self):
        self.commandList = []
    def getCommandList(self, openFile):
        commands = openFile.readlines() # a list
        for command in commands:
            command = command.strip('\n')
            if '//' in command: # This is a comment
                comment_ind = command.index('/')
                command = command[:comment_ind]
                self.commandList.append(command)
            elif 'OpenDatabase' in command:# Personal preference for formatting
                database = command.strip('OpenDatabase=')
                d1 = S.DebugMsg('Current Database ='+ database)
                visit.OpenDatabase(database,0)
                visit.AddPlot("FilledBoundary","materials1",1,1)
                visit.DrawPlots()
                session.CurDatabase=database
            elif '#include ' in command: # should not take includes after //
                nextFile = open(command.strip('#include '), 'r')
                d2 = S.DebugMsg('opening test file'+command.strip('#include '))
                self.getCommandList(nextFile)
            elif command == ' ':
                pass
            else:
                self.commandList.append(command)
        return self.commandList

class getCommand(object):
    def __init__(self):
        self.runTest = True
        self.openFile = open(S.env.tests_file)
        self.list_creator = CommandList()
        self.commandList = self.list_creator.getCommandList(self.openFile)
        self.commandIndex = 0
        
    def get_command(self):
        try:
            finalCommand = self.commandList[self.commandIndex]
            self.commandIndex += 1
            return finalCommand
        except IndexError:
            self.runTest = False
    def get_command_list(self):
        return self.commandList

        
        
        
class ExtraTests(): # the actual CLI command for this function is "debug"
    def __init__(self):
        d1 = S.DebugMsg('Initializing extra tests')
#    session.checkVar(session.ei_mode)# This is not printing out the proper things
#    ParseCommand('ei on')
#    session.checkVar(session.ei_mode)
#    session.checkVar(session.gs_mode)
#    ParseCommand('gs on')
#    session.checkVar(session.gs_mode)
#    session.checkVar(session.pn_mode)
#    ParseCommand('pn on')
#    session.checkVar(session.pn_mode)
#    session.checkVar(session.fn_mode)
#    ParseCommand('fn on')
#    session.checkVar(session.fn_mode)
    def do(self):
        pass
        #'this should be the directory of visit'
        #dir(visit)

        # Still To Do: Write the functions below:
        #Populator = S.PopulateFunctions()
        #Populator.checkVariables()

class VariableChecker(object):
    def __init__(self):
        pass
    def do(self):
        d1 = S.DebugMsg("session.CurDatabase ="+ session.CurDatabase)
        d2 = S.DebugMsg( "session.materials = {0}".format(session.materials))
        d3 = S.DebugMsg("session.materials_visible = {0}".format(session.materials_visible))
        d4 = S.DebugMsg("session.enabled_mats = {0}".format(session.enabled_mats))
       
        

    
###############################################
# And then Enters main command input control loop (for additional testing by hand)
###############################################












