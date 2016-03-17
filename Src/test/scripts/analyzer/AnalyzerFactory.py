#! /usr/local/bin/python

import string,fpformat,math,sys,os

from AbsoluteAnalyzer import AbsoluteAnalyzer
from RunAnalyzer import RunAnalyzer
def getAnalyzer(test_mode):
	if test_mode == "range":
		return RangeAnalyzer.RangeAnalyzer()
	elif test_mode == "stddev":
		return StddevAnalyzer.StddevAnalyzer()
	elif test_mode == "run" or test_mode == "smoke":
		return RunAnalyzer()
	else:
		return AbsoluteAnalyzer()

