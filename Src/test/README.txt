This is a very brief readme on how to run the testing suite for Griz.

A very simple run would consist of running the entire derived suite


Test.py lives in  Dyna3dx/Checkout/scripts/Test.py

griz_env lives there also





scripts/Test.py -e griz_env -c path to the code" -s derived


To find all the options just run 

scripts/Test.py -h


You must use the "-e griz_env" flag

You can all the test by doing an "ls derived/*.answ"

To run a single test such as accz 
 
scripts/Test.py -e griz_env -c "path to the griz batch executable (directory
bin_batch_opt or bin_batch_debug" -t accz

Steps to add a test case to an existing suite:

As an example here is how the test .../GrizDist/test/image/damage/damage_pvmag
was added.

1.  create a .gsf file in .../griz/Src/test/image/damage called damage_pvmag_th.gsf
    where the .gsf file is a set of griz commands.  There is header
    information in the form of comments.  This was taken from another gsf file
    and modified for the pvmag test case. 
2.  The test suite is called image/damage.  so cd into
    ...griz/Src/test/scripts and edit griz_env.py in the quicklist section for
    image/damage add damage_pvmag_th
3.  Run griz in batch mode with the .gsf file as input to make sure it is
    producing the correct results.
4.  Add the base line.  From the .../griz/Src/test directory type the
    following command
    "scripts/Test.py -e griz_env -c <path to griz batch executable> -b -t
     damage_pvmag_th"
5.  You should get a message indicating the baseline was added/updated
6.  Run the test by typing the command in step 4. without the -b option to 
    verify it passed.
7.  cd into .../griz/Src/test/image/damage and run git add to the 
    damage_pvmag_th.gsf and damage_pvmag_th.jpg files
8.  from the .../griz/Src directory run git commit

To checkout/update the scripts directory run:

cvs co -d scripts Dyna3dx/CheckOut/scripts





