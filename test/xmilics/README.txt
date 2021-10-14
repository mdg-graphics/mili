This is a brief readme describing how to run the Xmilics test suite.

Test.py and xmilics_env.py live in Dyna3dx/CheckOut/scripts/ repository
To checkout/update locally do the following:
	cvs co -d scripts Dyna3dx/CheckOut/scripts


To run the test suite:
	All Tests:
		scripts/Test.py -e xmilics_env -c <path_to_code> -p1 -s all

	Single Suite:
		scripts/Test.py -e xmilics_env -c <path_to_code> -p1 -s xmilics_tests/<suite_name>
		scripts/Test.py -e xmilics_env -c <path_to_code> -p1 -s xmilics_tests/bar1
		scripts/Test.py -e xmilics_env -c <path_to_code> -p1 -s xmilics_tests/d3samp6

	Single Test:
		scripts/Test.py -e xmilics_env -c <path_to_code> -p1 -t <test_name>
		scripts/Test.py -e xmilics_env -c <path_to_code> -p1 -t bar1_default_combine
		scripts/Test.py -e xmilics_env -c <path_to_code> -p1 -t d3samp6_default_combine


Tests can be added by adding the required files in xmilics_tests/ and updating xmilics_env.py
