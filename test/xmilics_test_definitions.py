# Specifies the config file to be used for these tests
config_file = "xmilics_config.py"

"""
NOTES:

For the exec_command field for a given test, the -o flag should be used to specify a name for the
combined plot file produced by Xmilics.

Each test is required to have the key:value pair "xmilics_args":str, where str is a string containing
the arguments that were passed to Xmilics. This is used to get the name of the combined plot file
and handle tests where only a range of states are combine, only certain processors are combined, etc.
"""

mapping = {
    "xmilics/d3samp6": {
        "testnames": ["d3samp6_default_combine", "d3samp6_limit_states",
                      "d3samp6_limit_procs"],
        "additional_files": [
            "d3samp6.plt00000", "d3samp6.plt000A",
            "d3samp6.plt00100", "d3samp6.plt001A",
            "d3samp6.plt00200", "d3samp6.plt002A",
            "d3samp6.plt00300", "d3samp6.plt003A",
            "d3samp6.plt00400", "d3samp6.plt004A",
            "d3samp6.plt00500", "d3samp6.plt005A",
            "d3samp6.plt00600", "d3samp6.plt006A",
            "d3samp6.plt00700", "d3samp6.plt007A",
        ],
        "num_procs": 1,

        "d3samp6_default_combine": {
            "xmilics_args": "-i d3samp6.plt -o d3samp6_combined.plt"
        },
        "d3samp6_limit_states": {
            "xmilics_args": "-i d3samp6.plt -o d3samp6_limit_states.plt -start 20 -stop 40"
        },
        "d3samp6_limit_procs": {
            "xmilics_args": "-i d3samp6.plt -o d3samp6_limit_procs.plt -proc 4-8"
        },
        "d3samp6_pstart_pstop": {
            "xmilics_args": "-i d3samp6.plt -o d3samp6_pstart_pstop.plt -pstart 4 -pstop 8"
        }
    },

    "xmilics/bar1": {
        "testnames": ["bar1_default_combine", "bar1_limit_states",
                      "bar1_limit_procs"],
        "additional_files": [
            "bar1.plt00000", "bar1.plt000A",
            "bar1.plt00100", "bar1.plt001A",
            "bar1.plt00200", "bar1.plt002A",
            "bar1.plt00300", "bar1.plt003A",
            "bar1.plt00400", "bar1.plt004A",
            "bar1.plt00500", "bar1.plt005A",
            "bar1.plt00600", "bar1.plt006A",
            "bar1.plt00700", "bar1.plt007A",
        ],
        "num_procs": 1,

        "bar1_default_combine": {
            "xmilics_args": "-i bar1.plt -o bar1_combined.plt",
        },
        "bar1_limit_states": {
            "xmilics_args": "-i bar1.plt -o bar1_limit_states.plt -start 20 -stop 40",
        },
        "bar1_limit_procs": {
            "xmilics_args": "-i bar1.plt -o bar1_limit_procs.plt -proc 1,2,3",
        },
        "bar1_pstart_pstop": {
            "xmilics_args": "-i bar1.plt -o bar1_limit_procs.plt -pstart 1 -pstop 4",
        }
    },

    "xmilics/shell_mat2": {
        "testnames": ["shell_mat2_default_combine", "shell_mat2_limit_states",
                      "shell_mat2_limit_procs"],
        "additional_files": [
            "shell_mat2.plt00000", "shell_mat2.plt000A",
            "shell_mat2.plt00100", "shell_mat2.plt001A",
            "shell_mat2.plt00200", "shell_mat2.plt002A",
            "shell_mat2.plt00300", "shell_mat2.plt003A",
            "shell_mat2.plt00400", "shell_mat2.plt004A",
            "shell_mat2.plt00500", "shell_mat2.plt005A",
            "shell_mat2.plt00600", "shell_mat2.plt006A",
            "shell_mat2.plt00700", "shell_mat2.plt007A",
            "shell_mat2.plt00800", "shell_mat2.plt008A",
            "shell_mat2.plt00900", "shell_mat2.plt009A",
            "shell_mat2.plt01000", "shell_mat2.plt010A",
        ],
        "num_procs": 1,

        "shell_mat2_default_combine": {
            "xmilics_args": "-i shell_mat2.plt -o shell_mat2_combined.plt",
        },
        "shell_mat2_limit_states": {
            "xmilics_args": "-i shell_mat2.plt -o shell_mat2_combined.plt -start 4 -stop 8",
        },
        "shell_mat2_limit_procs": {
            "xmilics_args": "-i shell_mat2.plt -o shell_mat2_combined.plt -procs 5-10",
        },
        "shell_mat2_pstart_pstop": {
            "xmilics_args": "-i shell_mat2.plt -o shell_mat2_combined.plt -pstart 5 -pstop 10",
        },
    },

    "xmilics/bar5": {
        "testnames": ["bar5_default_combine", "bar5_limit_states",
                      "bar5_limit_procs"],
        "additional_files": [
            "bar5.plt00000", "bar5.plt000A",
            "bar5.plt00100", "bar5.plt001A",
            "bar5.plt00200", "bar5.plt002A",
            "bar5.plt00300", "bar5.plt003A",
            "bar5.plt00400", "bar5.plt004A",
            "bar5.plt00500", "bar5.plt005A",
            "bar5.plt00600", "bar5.plt006A",
            "bar5.plt00700", "bar5.plt007A",
            "bar5.plt00800", "bar5.plt008A",
            "bar5.plt00900", "bar5.plt009A",
            "bar5.plt01000", "bar5.plt010A",
            "bar5.plt01100", "bar5.plt011A",
            "bar5.plt01200", "bar5.plt012A",
            "bar5.plt01300", "bar5.plt013A",
            "bar5.plt01400", "bar5.plt014A",
            "bar5.plt01500", "bar5.plt015A",
            "bar5.plt01600", "bar5.plt016A",
            "bar5.plt01700", "bar5.plt017A",
            "bar5.plt01800", "bar5.plt018A",
            "bar5.plt01900", "bar5.plt019A",
            "bar5.plt02000", "bar5.plt020A",
            "bar5.plt02100", "bar5.plt021A",
            "bar5.plt02200", "bar5.plt022A",
            "bar5.plt02300", "bar5.plt023A",
            "bar5.plt02400", "bar5.plt024A",
            "bar5.plt02500", "bar5.plt025A",
            "bar5.plt02600", "bar5.plt026A",
            "bar5.plt02700", "bar5.plt027A",
            "bar5.plt02800", "bar5.plt028A",
            "bar5.plt02900", "bar5.plt029A",
            "bar5.plt03000", "bar5.plt030A",
            "bar5.plt03100", "bar5.plt031A",
            "bar5.plt03200", "bar5.plt032A",
            "bar5.plt03300", "bar5.plt033A",
            "bar5.plt03400", "bar5.plt034A",
            "bar5.plt03500", "bar5.plt035A",
        ],
        "num_procs": 1,

        "bar5_default_combine": {
            "xmilics_args": "-i bar5.plt -o bar5_combined.plt"
        },
        "bar5_limit_states": {
            "xmilics_args": "-i bar5.plt -o bar5_limit_states.plt -start 20 -stop 60"
        },
        "bar5_limit_procs": {
            "xmilics_args": "-i bar5.plt -o bar5_limit_procs.plt -proc 13-23"
        },
        "bar5_pstart_pstop": {
            "xmilics_args": "-i bar5.plt -o bar5_limit_procs.plt -pstart 13 -pstop 23"
        }
    },

    "xmilics/ml40": {
        "testnames": ["ml40_default_combine", "ml40_limit_states",
                      "ml40_limit_procs"],
        "additional_files": [
            "ml40.plt00000", "ml40.plt000A",
            "ml40.plt00100", "ml40.plt001A",
            "ml40.plt00200", "ml40.plt002A",
            "ml40.plt00300", "ml40.plt003A",
            "ml40.plt00400", "ml40.plt004A",
            "ml40.plt00500", "ml40.plt005A",
            "ml40.plt00600", "ml40.plt006A",
            "ml40.plt00700", "ml40.plt007A",
        ],
        "num_procs": 1,

        "ml40_default_combine": {
            "xmilics_args": "-i ml40.plt -o ml40_combined.plt"
        },
        "ml40_limit_states": {
            "xmilics_args": "-i ml40.plt -o ml40_limit_states.plt -start 20 -stop 40"
        },
        "ml40_limit_procs": {
            "xmilics_args": "-i ml40.plt -o ml40_limit_procs.plt -proc 1,2,3"
        },
    },

    "xmilics/basic2": {
        "testnames": ["basic2_default_combine", "basic2_limit_states",
                      "basic2_limit_procs"],
        "additional_files": [
            "basic2.plt00000", "basic2.plt000A",
            "basic2.plt00100", "basic2.plt001A",
            "basic2.plt00200", "basic2.plt002A",
            "basic2.plt00300", "basic2.plt003A",
            "basic2.plt00400", "basic2.plt004A",
            "basic2.plt00500", "basic2.plt005A",
            "basic2.plt00600", "basic2.plt006A",
            "basic2.plt00700", "basic2.plt007A",
        ],
        "num_procs": 1,

        "basic2_default_combine": {
            "xmilics_args": "-i basic2.plt -o basic2_combined.plt"
        },
        "basic2_limit_states": {
            "xmilics_args": "-i basic2.plt -o basic2_limit_states.plt -start 20 -stop 40"
        },
        "basic2_limit_procs": {
            "xmilics_args": "-i basic2.plt -o basic2_limit_procs.plt -proc 1,2,3"
        },
    },
}
