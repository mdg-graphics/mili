This is a very brief readme on how to run the testing suite for Griz.


################################################################
                    Checkout Scripts Directory

Test.py lives in  Dyna3dx/Checkout/scripts/Test.py

griz_env lives there also

To checkout locally do the following: 

cvs co -d scripts Dyna3dx/CheckOut/scripts


################################################################
                    Running Tests

Tests are run using the script Test.py. The argument "-e griz_env" must 
be added to run the griz tests

To find all the options just run: scripts/Test.py -h

The tests results were baselined using the "bin_batch_opt" version of Griz.
You should be using this version when running the tests otherwise some
of the tests will fail


To run the Griz Test Suite:
	All Tests:
		scripts/Test.py -e griz_env -c <path_to_code> -p1 -s all

	Single Suite:
		scripts/Test.py -e griz_env -c <path_to_code> -p1 -s <suite_name>
		scripts/Test.py -e griz_env -c <path_to_code> -p1 -s derived

	Single Test:
		scripts/Test.py -e griz_env -c <path_to_code> -p1 -t <testname>
		scripts/Test.py -e griz_env -c <path_to_code> -p1 -t bar71_image_press_rmin_th

################################################################
                    Adding a New Test Case

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





