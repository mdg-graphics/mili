This is a brief readme describing how to run the Mili and Xmilics Test suites.

To run the Mili Test Suite:
	All Tests:
		mdgtest/Test.py -e mililib_env -c <path_to_code> -I <include_dir> -p1 -s all

	Single Suite:
		mdgtest/Test.py -e mililib_env -c <path_to_code> -I <include_dir> -p1 -s mili/<suite_name>
		mdgtest/Test.py -e mililib_env -c <path_to_code> -I <include_dir> -p1 -s mili/mili_C_tests

	Single Test:
		mdgtest/Test.py -e mililib_env -c <path_to_code> -I <include_dir> -p1 -t mixdb_wrt_stream


To run the Xmilics test suite:
	All Tests:
		mdgtest/Test.py -e xmilics_env -c <path_to_code> -p1 -s all

	Single Suite:
		mdgtest/Test.py -e xmilics_env -c <path_to_code> -p1 -s xmilics/<suite_name>
		mdgtest/Test.py -e xmilics_env -c <path_to_code> -p1 -s xmilics/bar1
		mdgtest/Test.py -e xmilics_env -c <path_to_code> -p1 -s xmilics/d3samp6

	Single Test:
		mdgtest/Test.py -e xmilics_env -c <path_to_code> -p1 -t <test_name>
		mdgtest/Test.py -e xmilics_env -c <path_to_code> -p1 -t bar1_default_combine
		mdgtest/Test.py -e xmilics_env -c <path_to_code> -p1 -t d3samp6_default_combine

    NOTE: For Xmilics tests, there is a default limit of 50 incorrect results that will be written to
          the log file for each test. A message is also written that displays the number of
          results that are not written. This is done because Xmilics tests have the possibility
          to produce a very large amount or errors.

          The flag -limit_mili_output <integer> can be used to set a different limit
