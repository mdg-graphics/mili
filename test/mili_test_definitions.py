from string import Template

code_name = "mili"

mapping = {
    "mili/version_3_mili_C_tests": {
        "keep_on_fail_file_expr": ["plt", "answ", "jansw", ".c", ".o", ".out"],
        "keep_on_pass_file_expr": [".c", ".out"],
        "test_type": "mililib",
        "quicklist": ["mixdb_wrt_stream_v3",
                      "mixdb_repair_v3",
                      "mixdb_wrt_subrec_v3",
                      "restart_v3",         
                      "restart_zero_v3",    
                      "restart_statelimit_a_v3",
                      "restart_statelimit_b_v3",
                      "restart_statelimit_c_v3",
                      "restart_past_last_state_v3",
                      "state_write_check_v3",
                      "value_change_v3",    
                      "del_test_v3"],
        "file_suffix": ".c",
        "baseline_suffix": "answ",
        "output_suffix": "jansw",
        "awkfiles": [],
        "awk_commands": [],
        "tests": {
            "del_test_v3": {
                "test_type": "stdout",
                "exec_command": [Template("./${prob}.out 2>&1 | tee ${prob}.jansw")]
            },
            "restart_past_last_state_v3": {
                "test_type": "stdout",
                "exec_command": [Template("./${prob}.out 2>&1 | tee ${prob}.jansw")],
            },
            "state_write_check_v3": {
                "test_type": "stdout",
                "exec_command": [Template("./${prob}.out 2>&1 | tee ${prob}.jansw")],
            }
        },
        "compile_commands": {"icc": [Template("icc -g -I ${include_path} -c ${prob}.c"),
                                     Template("icc -o ${prob}.out -g -I ${include_path} ${prob}.o -L ${library_path} -lmili -ltaurus")],
                             "gcc": [Template("gcc -g -I ${include_path} -c ${prob}.c"),
                                     Template("gcc -o ${prob}.out -g -I ${include_path} ${prob}.o -L ${library_path} -lmili -ltaurus -lm")],
                            },
        "exec_command": [Template("./${prob}.out")]
    },"mili/mili_C_tests": {
        "keep_on_fail_file_expr": ["plt", "answ", "jansw", ".c", ".o", ".out"],
        "keep_on_pass_file_expr": [".c", ".out"],
        "test_type": "mililib",
        "quicklist": ["mixdb_wrt_stream",
                      "mixdb_wrt_subrec",
                      "mixdb_mixed_subrecords",
                      "restart",         
                      "restart_zero",    
                      "restart_statelimit_a",
                      "restart_statelimit_b",
                      "restart_statelimit_c",
                      "restart_past_last_state",
                      "state_write_check",
                      "value_change",    
                      "del_test"],
        "file_suffix": ".c",
        "baseline_suffix": "answ",
        "output_suffix": "jansw",
        "awkfiles": [],
        "awk_commands": [],
        "tests": {
            "del_test": {
                "test_type": "stdout",
                "exec_command": [Template("./${prob}.out 2>&1 | tee ${prob}.jansw")]
            },
            "restart_past_last_state": {
                "test_type": "stdout",
                "exec_command": [Template("./${prob}.out 2>&1 | tee ${prob}.jansw")],
            },
            "state_write_check": {
                "test_type": "stdout",
                "exec_command": [Template("./${prob}.out 2>&1 | tee ${prob}.jansw")],
            }
        },
        "compile_commands": {"icc": [Template("icc -g -I ${include_path} -c ${prob}.c"),
                                     Template("icc -o ${prob}.out -g -I ${include_path} ${prob}.o -L ${library_path} -lmili -ltaurus")],
                             "gcc": [Template("gcc -g -I ${include_path} -c ${prob}.c"),
                                     Template("gcc -o ${prob}.out -g -I ${include_path} ${prob}.o -L ${library_path} -lmili -ltaurus -lm")],
                            },
        "exec_command": [Template("./${prob}.out")]
    },
    "mili/mili_fortran_tests": {
        "keep_on_fail_file_expr": ["plt", "answ", "jansw", ".f", ".o", ".out"],
        "keep_on_pass_file_expr": [".f", ".out"],
        "test_type": "mililib",
        "quicklist": ["hexdb", "hexdb2", "hexdb2p2", "hvadb",
                      "nodev1", "partdb", "partdb2", # "multfmt", 
                      "qmenu", "qnost", "quaddb", "shelldb",
                      "tridb"],
        "file_suffix": ".f",
        "baseline_suffix": "answ",
        "output_suffix": "jansw",
        "awkfiles": [],
        "awk_commands": [],
        "tests": None,
        "compile_commands": {"icc": [Template("ifort -g -I ${include_path} -c ${prob}.f"),
                                     Template("ifort -o ${prob}.out -g -I ${include_path} ${prob}.o -L ${library_path} -lmili -ltaurus")],
                             "gcc": [Template("gfortran -g -I ${include_path} -c ${prob}.f"),
                                     Template("gfortran -o ${prob}.out -g -I ${include_path} ${prob}.o -L ${library_path} -lmili -ltaurus")],
                            },
        "exec_command": [Template("./${prob}.out")]
    },
}
