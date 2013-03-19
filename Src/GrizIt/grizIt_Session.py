
############################################################################
# grizIt_Session.py - Holds global session state and environment variables.
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
from grizIt_Env import *
from PySide import QtGui, QtCore
import traceback

session      = Analy()
save_session = session
env          = Env()

# Visit state variables

class GrizFunction(object):
    """ This is the master class so that in the future when typing the class
        it will be clear that every class is a GrizFunction. Therefore all
        of the classes will inherit from GrizFunction, and this way they
        will all have the do() function regardless of whether that do() has
        been implemented.

        It's a little tricky to type functions in python, so here is what
        my research revealed: (I'll use 'anim' as the class. anim is a Griz
        function which has an __init__ method and a do() method. It inherits
        from the GrizFunction class. The particular instance of anim will be
        called 'animInstance')

        >>> type(animInstance)
        <class '__main__.anim'>
        >>> animInstance.__class__
        <class '__main__.anim'>
        >>> animInstance.__class__()
        <__main__.anim object at 0x10048ced0>
        >>> type(animInstance) == anim
        True
        >>> isinstance(animInstance, GrizFunction)
        True
        >>> issubclass(anim, GrizFunction)
        True

        **Note** In order for each class to check if the database is currently
        loaded, they would have to initialize GrizFunction -- (only if the
        class has its own __init__ function. Without it, it defaults to its
        superclass's init function):

        class anim(GrizFunction):
            def __init__(self):
                GrizFunction.__init__(self)

        But this is useful in the future if you want to change the
        initialization of every class, you just have to change GrizFunction.


        The message function prints out a message to the screen. All griz functions
        will inherit this functionality :D (puts it in green)

        The warning function outputs a message in warning colors (red)

    """

    def message(self, messageString):
        m = Message( messageString, 1 )
        m.display()
    def warning(self, warningString):
        w = WarningMsg(warningString)
    def do(self):
        self.warning("Warning: This command is not yet implemented")


class NoGrizItEquivalent(GrizFunction):
    def do(self):
        self.warning("There is no GrizIt equivalent for this function")
        

class OnOffFunction(GrizFunction):
    """ This is the class for the on/off functions.

        Each on/off function checks for a flag and has the on function and the off function

        So every on/off command function should have an 'on' function and an 'off'   function,
        and should not have a 'do' function because the do is taken care of by the inheritance

    """
    def __init__(self, flag):
        self.flag = flag
    def on(self):
        pass
    def off(self):
        pass
    def do(self):
        if self.flag == "on":
            self.on()
        elif self.flag == "off":
            self.off()
        else:
            self.warning("Inappropriate argument. Use 'on' or 'off'")
               
 ## It would be nice to have the same sort of function inheritance for the switch functions

class SwitchFunction(GrizFunction):
    """ This is the class for the switch functions.

        Each switch function switches a flag
    """
    def __init__(self, flag):
        pass

class Message:
    """ This is a class which prints a message to the screen. It can either
        receive a string or a text file as the message.

        Using the set methods, you can set the following:

        setFont ( a string of the fontname. default = 'Couriour')
        setColor (a string of the colorname. default = 'green')
        setFontsize (int. default = 11)
        setItal (bool. default = False)
        setBold (bool. default = True
        setUline (bool. default = False)
        setAlignment (Qt -> default = Qt.AlignLeft)
    """
    def __init__(self, message, screen_number=1):
        self.screen_number = screen_number
        # Font attributes with default settings
        self.font = "Courior"
        self.color = "green"
        self.fontsize = 11
        self.ital = False
        self.bold = True
        self.uline = False
        self.alignment = QtCore.Qt.AlignLeft

        if type(message) == str:
            self.message = message
        else: # message is  file
            f = open(message, 'r')
            self.message = "\n".join(f.readlines()) # turns it into a long string formatted by lines

            
    def display(self):
        try:
            env.mainWin.MsgFont(self.font, self.color, self.fontsize, self.ital, self.bold, self.uline, self.alignment)
            env.mainWin.MsgText( self.message, self.screen_number )
        except:
            if env.mainWin == None:
                print "\n"
            else:
                print "Could not display message in GUI. The following error occurred:"
                traceback.print_stack()
                error = traceback.format_exc()
                print error
                print "Message = ", self.message

    def setScreenNumber(self, screen_number):
        self.screen_number = screen_number
    def setFont(self, font):
        self.font = font
    def setColor(self, color):
        self.color = color
    def setFontsize(self, fontsize):
        self.fontsize = fontsize
    def setItal(self, ital):
        self.ital = ital
    def setBold(self, bold):
        self.bold = bold
    def setUline(self, uline):
        self.uline = uline
    def setAlignment(self, alignment):
        self.alignment = alignment

    def setParams(self, font, color, fontsize, ital, bold, uline, alignment):
        self.setFont(font)
        self.setColor(color)
        self.setFontsize(fontsize)
        self.setItal(ital)
        self.setBold(bold)
        self.setUline(uline)
        self.setAlignment(alignment)

class DebugMsg(Message):
    def __init__(self, debugTxt, screen_number = 1):
        if env.debugMode: #Prints the debug messages to the screen
            Message.__init__(self, "**Debug**  "+debugTxt, screen_number)
            self.setColor("purple")
            self.display()
            
        if ( not env.show_gui ):
             print "DebugMsg: "+debugTxt

class PrintMsg(Message):
    def __init__(self, printTxt, screen_number = 1):
        Message.__init__(self, printTxt, screen_number)
        print printTxt # also prints to the screen
       
class WarningMsg(Message):
    def __init__(self, warningTxt, screen_number=1):
        Message.__init__(self, warningTxt, screen_number)
        self.setColor("red")
        self.display()

        if ( not env.show_gui ):
             print "WarningMsg: "+warningTxt
     
class InfoMsg(Message):
    def __init__(self, infoTxt, screen_number=1):
        Message.__init__(self, infoTxt, screen_number)
        self.setColor("green")
        self.display()
        
        if ( not env.show_gui ):
             print "Info: "+infoTxt

#
# This class is used to create and manage a log
# of commands that have been executed.
#
class CmdLog:
    def __init__(self):
        session.lastCommand=""
        session.historyLogIndex=0
        
    def add(self, cmd):
        session.log.append(cmd)
        session.logIndex = len(session.log)

    def clear(self):
        session.log = []

    def push(self):
        session.logIndex += 1
        if ( session.logIndex>len(session.log) ):
             session.logIndex = len(session.log)
        return session.log[session.logIndex-1]

    def pop(self):
        if len(session.log)==0:
            return
            
        session.logIndex -= 1
        if ( session.logIndex<1 ):
             session.logIndex = 1
        return session.log[session.logIndex-1]

    
