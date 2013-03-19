#!/usr/bin/env tcsh

echo "Now updating from repository"

cvsupdate

echo "Now committing the files you have modified"

#cvscommit -m # can do a whole list here, or do them individually below -- include commit message

#cvscommit -m # Commit message here # grizIt_ParseGrizCommand.py 
#cvscommit -m grizIt_Main.py
#cvscommit -m master_test.txt
#cvscommit -m grizIt_Env.py
#cvscommit -m grizIt_TestSuite.py
#cvscommit -m closeout.sh

echo "Good bye $USER!"
