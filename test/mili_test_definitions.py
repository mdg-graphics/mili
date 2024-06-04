config_file = "mili_config.py"

mapping = {

    "mili_C_tests": {
        "suite_dir": "mili/mili_C_tests",
        "testnames": ["mixdb_wrt_stream",
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
        "file_suffix": "c",
        "num_procs": 1,
        "restart_past_last_state": { "expected_failures": ["run"] },
    },

    "version_3_mili_C_tests": {
        "suite_dir": "mili/version_3_mili_C_tests",
        "testnames": ["mixdb_wrt_stream_v3",
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
        "file_suffix": "c",
        "num_procs": 1,
        "restart_past_last_state_v3": { "expected_failures": ["run"] },
    },

    "mili_fortran_tests": {
        "suite_dir": "mili/mili_fortran_tests",
        "testnames": ["hexdb", "hexdb2", "hexdb2p2", "hvadb",
                      "nodev1", "partdb", "partdb2", # "multfmt",
                      "qmenu", "qnost", "quaddb", "shelldb",
                      "tridb"],
        "file_suffix": "f",
        "num_procs": 1,
    },
}