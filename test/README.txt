This is a brief readme describing how to run the Mili and Xmilics Test suites.


Test.py, mililib_env.py, xmilics_env.py live in Dyna3dx/CheckOut/scripts/ repository
To checkout/update locally do the following:
	cvs co -d scripts Dyna3dx/CheckOut/scripts


To run the Mili Test Suite:
	All Tests:
		scripts/Test.py -e mililib_env -c <path_to_code> -I <include_dir> -p1 -s all

	Single Suite:
		scripts/Test.py -e mililib_env -c <path_to_code> -I <include_dir> -p1 -s mili/<suite_name>
		scripts/Test.py -e mililib_env -c <path_to_code> -I <include_dir> -p1 -s mili/mili_C_tests

	Single Test:
		scripts/Test.py -e mililib_env -c <path_to_code> -I <include_dir> -p1 -t mixdb_wrt_stream


To run the Xmilics test suite:
	All Tests:
		scripts/Test.py -e xmilics_env -c <path_to_code> -p1 -s all

	Single Suite:
		scripts/Test.py -e xmilics_env -c <path_to_code> -p1 -s xmilics/<suite_name>
		scripts/Test.py -e xmilics_env -c <path_to_code> -p1 -s xmilics/bar1
		scripts/Test.py -e xmilics_env -c <path_to_code> -p1 -s xmilics/d3samp6

	Single Test:
		scripts/Test.py -e xmilics_env -c <path_to_code> -p1 -t <test_name>
		scripts/Test.py -e xmilics_env -c <path_to_code> -p1 -t bar1_default_combine
		scripts/Test.py -e xmilics_env -c <path_to_code> -p1 -t d3samp6_default_combine

    NOTE: For Xmilics tests, there is a default limit of 50 incorrect results that will be written to
          the log file for each test. A message is also written that displays the number of
          results that are not written. This is done because Xmilics tests have the possibility
          to produce a very large amount or errors.

          The flag -limit_mili_output <integer> can be used to set a different limit
