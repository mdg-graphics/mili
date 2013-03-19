############################################################################
# grizIt_GrizTractionCommands.py - Griz Functions - Traction Commands.
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

import time

import visit
import grizIt_Main
import grizIt_Session
import grizIt_ParseGrizCommand 
from grizIt_TestSuite import CommandList
import grizIt_VisitInfo as VISITINFO

session = grizIt_Session.session

############################################################################################
 

#############################
# Time History (TH) Commands
#############################
def GRIZ_surface(args):
    surface_type = string(args[0])
#end def

#****** I THINK THE ABOVE IS IN THE WRONG FILE

##  I'm going ot need an environment variable that saves the history. It will be a text file.  Something like session.history

class savhis(grizIt_Session.GrizFunction): 
    """ Begin saving commands to file """
    def __init__(self, filename):
        self.filename = filename[0] # delivers filename in an argument list
    def do(self):
        session.historyFile = open(self.filename, 'a')
        session.saveHistory = True #now go into main and add conditional

class endhis(grizIt_Session.GrizFunction):
    """ Close command history file """
    def __init__(self):
        pass
    def do(self):
        session.saveHistory = False
        session.historyFile.close()


class rdhis(grizIt_Session.GrizFunction):
    """ Read command history file and execute commands """
    def __init__(self, filename):
        self.filename = open(filename[0]) # parser gives a list with one element
    def do(self):
        commands = self.filename.readlines()
        for command in commands:
            grizIt_ParseGrizCommand.ParseCommand(command)


class pause(grizIt_Session.GrizFunction):
    """ Pauses for n seconds """
    def __init__(self, n):
        self.n = n
    def do(self):
        time.sleep(n)


class echo(grizIt_Session.GrizFunction):
    """ Print string to history

        ## This function is broken because of the parsing. Should be able to recognize that the string is one argument despite any number of spaces between the quotes.
        ## Also, should we make it a comment when it is added to the history file? Just in case they are writing some string that is not a valid command? Or should it assume that the string is going to be a command?

    """
    def __init__(self, some_string):
        self.some_string = some_string
    def do(self):
        if session.historyFile:
            session.historyFile.write('\n'+some_string+'\n') #maybe don't want the newline sandwich?
        else: print 'History is not being saved. Use savhis to write history to file'


class loop(grizIt_Session.GrizFunction):
    """ Loop endlessly over commands in file """
    def __init__(self, filename):
        self.filename = filename[0]
        print "**&&\n\n**&&\n\n**&&\n\n THIS IS THE FILENAME:",filename
    def do(self):
        while True:
            loop_executer = rdhis(self.filename)
            loop_executer.do()


class savtxt(grizIt_Session.GrizFunction):
    """ Begin saving feedback window text to file """
    def __init__(self, filename):
        grizIt_Session.GrizFunction.__init__(self)
        self.filename = filename

class endtxt(grizIt_Session.GrizFunction):
    """ Stop saving feedback text and close file """
    def __init__(self):
        grizIt_Session.GrizFunction.__init__(self)
        self.filename = filename

class savhislog(grizIt_Session.GrizFunction):
    """ Save all of the history up to that point to a file """
    def __init__(self, filename):
        grizIt_Session.GrizFunction.__init__(self)
        self.filename = filename

class savesg(object):
    """ Save global session data to a file """
    def __init__(self, filename):
        grizIt_Session.GrizFunction.__init__(self)
        self.filename = filename

class savesl(object):
    """ Save local (plot) session data to a file """
    def __init__(self, filename):
        grizIt_Session.GrizFunction.__init__(self)
        self.filename = filename

class loadsg(object):
    """ Save global session data to a file """
    def __init__(self, filename):
        grizIt_Session.GrizFunction.__init__(self)
        self.filename = filename
        

class loadsl(object):
    """ Save global session data to a file """
    def __init__(self, filename):
        grizIt_Session.GrizFunction.__init__(self)
        self.filename = filename
    
