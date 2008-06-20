This is a very brief readme on how to run the testing suite for Griz.

A very simple run would consist of running the entire derived suite

scripts/Test.py -e griz_env -c path to the code" -s derived


To find all the options just run 

scripts/Test.py -h


You must use the "-e griz_env" flag

You can all the test by doing an "ls derived/*.answ"

To run a single test such as accz 
 
scripts/Test.py -e griz_env -c path to the code" -t accz
