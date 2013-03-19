#!/usr/bin/python
#
############################################################################
# grizIt_CommandFactory.py -
#
# Sara Drakeley
# Lawrence Livermore National Laboratory
# July 6, 2010
#
#
############################################################################
# Modifications:
# S. Drakeley - July 6, 2010 - Created
#
############################################################################

import grizIt_ParseGrizCommand as P
import traceback
import grizIt_Session as S

class GrizCommand(object):
    """ This is the 'Creation Requestor.' It works by first getting a
        PyCommand object from the PyCommandFactoryIF object. It then
        accomplishes the do() method using the Filter pattern, so that
        it extends the do() methods from the particular PyCommand object
    """
    def __init__(self, key, args):
        """ key is the keyword for the Parse dictionary from the module
            grizIt_ParseGrizCommands.py

            args are the arguments listed after the griz command keyword

            PyCommandFactory is an instance of the factory interface class
        """
        self.key = key
        self.args = args
        self.PyCommandFactory = PyCommandFactory(key, args)

    def do(self):
        PyCommand = self.PyCommandFactory.createPyCommand(self.args)
        PyCommand.do()


class PyCommandFactory(object):
    """ This class implements the appropriate factory interface and has a
        method to create concrete PyCommand objects
    """
    def __init__(self, key, args):
        self.key = key
        self.ParseDict = P.GrizCommands
        self.args = args
        print "args=", args
        
    def createPyCommand(self, key):
        PyCommand = self.ParseDict[self.key][0] #Pointer to the class
        try:
            return PyCommand(self.args) #Creates a new instance of that class

        except TypeError: #There are no arguments
            if len(self.args)==0: return PyCommand()
            else:
                w = S.WarningMsg("This error occurred when creating the command:\n"+traceback.format_exc())
        else:
            m = S.WarningMsg(str("The arguments "+self.args+" are not compatible with the command "+key), 1)
            

        
