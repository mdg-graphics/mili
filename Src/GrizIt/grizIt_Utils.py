
##############################################################################
# Application: GrizIt - Griz CLI interface to the VisIt rendering
#
# grizIt_Utils.py - Classes for creatin.
#
#      
#      Lawrence Livermore National Laboratory
#      March, 2010
#
#############################################################################
# Modifications:
#
#  I. R. Corey - March 25, 2010: Created.
#  I. R. Corey - May    7, 2010: Added recursive hierarchiel dictionay
#        builder developed by Kevin Durrenburger.
#
#############################################################################

import pdb, datetime
import os, sys, re, bisect, math
from array import *

import fileinput
 
test = ["1 1","4","1 1 1", "2 1", "3","1 1 2", "2 2"]
test2 = ["strain x", "strain y", "strain x 1", "strain x 2", "strain x 2 a", "strain x 2 b", "strain y 1 a"]
isLeaf = "isLeaf"

def list_unique(input):
  output = []
  for i in input:
      if i not in output:
         output.append(i)
  return output
#end def
 
def buildHierDict(in_array):
        return_dictionary = {}
        for val in in_array:
            target = val.split()
            if len(target) >0:
               addHierDict(return_dictionary, target[0],target[1:])
        return return_dictionary


def addHierDict(dictionary, key, sub_list):
        if not key in dictionary.keys():
                if len(sub_list) == 0:
                        dictionary[key] = {isLeaf: True}
                        return
                else:
                        dictionary[key] = {isLeaf: False}

        if len(sub_list) >0:
                if dictionary[key][isLeaf]:
                        dictionary[key][isLeaf] = False
                addHierDict(dictionary[key], sub_list[0], sub_list[1:])

def printHierDict(dictionary, space):
        for key in dictionary.keys():
                if key == isLeaf:
                   continue
                # print space,key,"isLeaf: ",dictionary[key][isLeaf]
                # if not dictionary[key][isLeaf]:
                   # printHierDict(dictionary[key]," "*(len(space)+2))


if __name__ == "__main__":
    printHierDict( buildHierDict(test), "" )
    printHierDict( buildHierDict(test2), "" )

