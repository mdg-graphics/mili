#! /usr/local/bin/python

import sys,string,os,math,fpformat,popen2
from string import Template
from  dyna_env import *
                                                               
cur_driver = parallel_drivers[par_system]

default_tolerance = 1.0e-04
default_tol_type = "rel"
# define any additional suite combinations
# define as follows:
#
#  "suite_name" = ["testname[0]",...,"testname[n]"],
#  "next_suite" = ...
#  There must be a comma between each suite declaration
#  Do not use 'all' or a suite name from mapping they will be ignored

defined_suites ={"count_regular" : {"COUNT":["d3samp1",  "d3samp3",  "d3samp6",  "d3samp7",
                                             "d3samp8",  "d3samp9",  "d3samp10", "d3samp17",
                                             "d3samp18", "d3samp19", "d3samp20", "d3samp22",
                                             "d3snd1",   "d3snd4",   "sbi",      "sbp",
                                             "nrbc_norm", "nrbc_shear"]
                                    },
                 
                 "count_long" :{"COUNT":["d3snd2", "d3snd3", "d3snd3n", "d3snd5",  "d3snd6","d3samp2",  
                                         "d3samp4","d3samp5","d3samp21","mk24"]},
                 "test" :{"BAR":["bar1","bar1_9"],
                          "BEAM": ["vrt_HL0","bending-24"],
                          "PLATE":["plate3"],
                          "SLIDE":["sslide9ps", "sslide9pt", "sslide10lr"]},
			   "bad_bar": {"BAR": ["bar62"]},
                 "smoke":{}
                }

###############################################################################################
#
#  This is the default mapping of the tests to the required information for running them
#  It has the following layout
#
#  mapping = {"suite_name":"quicklist":["test1","test2",....,"testn"],##used for quick validation of
#                                                                       test suite combo
#                          "file_suffix":"suffix for the file read in by dyna3d/paradyn",
#                          "tests": None | a dictionary of tests which do not use the suite defaults
#                          "awkfiles": ["awkfile1", "awkfile2", ...]  list of all the awk files needed
#                                                                     to post process the run
#                          "awk_commands":  this is an array of python Templates which can include the
#                                           variable substitution strings
#                                           ${preawk}(prefix for awk in case of SunOS is 'n'), 
#                                           ${sig}(significant digits) and ${prob}(test name)
#                          "exec command": command to use when executing dyna/paradyn. It does the 
#                                          following variable substitution
#                                          $CODE = name of code
#                                          ${prob} = test name
#                          "default_tolerance": unused at this point
#
# Generic suite
# "suite_name":"quicklist":[""],
#               "file_suffix":".dyn",
#               "tests": None,
#               "awkfiles": [""],
#               "awk_commands": [Template("${preawk}awk -f XXX.awk -v sig_dig=${sig} ${prob}.hsp")],
#               "exec command": [Template("$CODE i=${prob}.dyn ")],
#               "default_tolerance": {"type":"rel","tolerance":"0.0"}
###############################################################################################
mapping = {"derived":{"keep_on_fail_file_expr":None,
                      "keep_on_pass_file_expr":None,
                      "test_type":"curve",
                      "quicklist":["accmag","accx","accx_array","accy","accy_array",
                                   "accz","accz_array","dispmag","dispx",
                                   "dispy","dispz","velmag","velx","velx_array","vely",
                                   "vely_array","velz","velz_array","ex_shell","exy_shell",
                                   "ey_shell","eyz_shell","ez_shell","ezx_shell",
                                   "pressure_shell","prin1_shell","prin2_shell","prin3_shell",
                                   "seff_shell","sx_shell","sxy_shell","sy_shell","syz_shell",
                                   "sz_shell","szx_shell","xy_engr_strain_shell",
                                   "yz_engr_strain_shell","zx_engr_strain_shell",
                                   "bar71_th",
                                   "bending-71_th",
                                   "bolt_th",
                                   "clamped_plate_th",
                                   "d3samp6_th",
                                   "d3snd1_th",
                                   "diablodbc_th",
                                   "discrete_th",
                                   "ml40_th",
                                   "nif_th",
                                   "ntet20_th",
                                   "plate71_th",
                                   "thickshells_th",
                                   "tplate1_th",
                                   "damage_th"
                                   ],
                      
				    "test_input_mode":"curve",
				    "file_suffix":".command",
                      
				    "tests":{
                                              "bar71_th":{"required_files_alt_dir":"image/bar71",
                                                         "file_suffix":".gsf",
                                                         "required_files":["bar71.plt00","bar71.plt_TI_A","bar71.pltA"],
                                                         "exec_command":[Template("$CODE  -q -i bar71.plt -b ${prob}.gsf ")]},
                                             
                                              "bending-71_th":{"required_files_alt_dir":"image/bending-71",
                                                         "file_suffix":".gsf",
                                                         "required_files":["bending-71.plt00","bending-71.plt_TI_A","bending-71.pltA"],
                                                         "exec_command":[Template("$CODE  -q -i bending-71.plt -b ${prob}.gsf ")]},

                                              "bolt_th":{"required_files_alt_dir":"image/bolt",
                                                         "file_suffix":".gsf",
                                                         "required_files":["boltA", "bolt00","bolt01","bolt02","bolt03","bolt04"],
                                                         "exec_command":[Template("$CODE  -q -i bolt -b ${prob}.gsf ")]},

                                              "clamped_plate_th":{"required_files_alt_dir":"image/clamped_plate",
                                                         "file_suffix":".gsf",
                                                          "required_files":["clamped_plate.th00",  "clamped_plate.thA",  "clamped_plate.th_TI_A"],
                                                          "exec_command":[Template("$CODE  -q -i clamped_plate.plt -b ${prob}.gsf ")]},

                                              "d3samp6_th":{"required_files_alt_dir":"image/d3samp6",
                                                         "file_suffix":".gsf",
                                                          "required_files":["d3samp6.plt00", "d3samp6.pltA", "d3samp6.plt_TI_A"],
                                                          "exec_command":[Template("$CODE  -q -i d3samp6.plt -b ${prob}.gsf ")]},
                                              

                                              "d3snd1_th":{"required_files_alt_dir":"image/d3snd1",
                                                           "file_suffix":".gsf",
                                                           "required_files":["d3snd1.plt00","d3snd1.pltA", "d3snd1.plt_TI_A"],
                                                           "exec_command":[Template("$CODE  -q -i d3snd1.plt -b ${prob}.gsf ")]},
                                               
                                              "diablodbc_th":{"required_files_alt_dir":"image/diablodbc",
                                                              "file_suffix":".gsf",
                                                              "required_files":["dblplt00000","dblplt000A","dblplt000_TI_A"],
                                                              "exec_command":[Template("$CODE  -q -i dblplt000 -b ${prob}.gsf ")]},
                      
                                              "discrete_th":{"required_files_alt_dir":"image/discrete",
                                                            "file_suffix":".gsf",
                                                            "required_files":["discrete.plt00","discrete.pltA","discrete.plt_TI_A"],
                                                            "exec_command":[Template("$CODE  -q -i discrete.plt -b ${prob}.gsf ")]},

                                              "ml40_th":{"required_files_alt_dir":"image/ml40",
                                              "file_suffix":".gsf",
                                                         "required_files":["ml40.plt00", "ml40.pltA", "ml40.plt_TI_A"],
                                                         "exec_command":[Template("$CODE  -q -i ml40.plt -b ${prob}.gsf ")]},

                                              "nif_th":{"required_files_alt_dir":"image/nif",
                                                         "file_suffix":".gsf",
                                                         "required_files":["nif00", "nifA"],
                                                         "exec_command":[Template("$CODE  -q -i nif -b ${prob}.gsf ")]},

                                              "ntet20_th":{"required_files_alt_dir":"image/ntet20",
                                                         "file_suffix":".gsf",
                                                         "required_files":["ntet20.plt00",  "ntet20.pltA", "ntet20.pltB", "ntet20.plt_TI_A"],
                                                         "exec_command":[Template("$CODE  -q -i ntet20.plt -b ${prob}.gsf ")]},

                                              "partition_th":{"required_files_alt_dir":"image/partition",
                                                         "file_suffix":".gsf",
                                                         "required_files":["partitionA"],
                                                         "exec_command":[Template("$CODE  -q -i partition -b ${prob}.gsf ")]},

                                              "plate71_th":{"required_files_alt_dir":"image/plate71",
                                                         "file_suffix":".gsf",
                                                         "required_files":["plate71.plt00", "plate71.pltA", "plate71.plt_TI_A"],
                                                         "exec_command":[Template("$CODE  -q -i plate71.plt -b ${prob}.gsf ")]},

                                              "thickshells_th":{"required_files_alt_dir":"image/thickshells",
                                                         "file_suffix":".gsf",
                                                         "required_files":["thickshells_c_TI_A", "thickshells_c00", "thickshells_cA"],
                                                         "exec_command":[Template("$CODE  -q -i thickshells_c -b ${prob}.gsf ")]},

                                              "tplate1_th":{"required_files_alt_dir":"image/tplate1",
                                                         "file_suffix":".gsf",
                                                         "required_files":["tplate1.plt_TI_A", "tplate1.plt00", "tplate1.pltA"],
                                                         "exec_command":[Template("$CODE  -q -i tplate1.plt -b ${prob}.gsf ")]},
                          
                                              "damage_th":{"required_files_alt_dir":"image/damage",
                                                         "file_suffix":".gsf",
                                                         "required_files":["bar45.plt_TI_A", "bar45.plt00", "bar45.pltA"],
                                                         "exec_command":[Template("$CODE  -q -i bar45.plt -b ${prob}.gsf ")]},
                          },


                                    "awkfiles":[],
				    "awk_commands":[],
				    "required_files":["d3plot","d3plot01","d3plot02","d3plot03",
							        "d3plot04","d3plot05","d3plot06","d3plot07",
								   "d3plot08","d3plot09","d3plot10","d3plot11",
								   "d3plot12","d3plot13","d3plot14","d3plot15",
								   "d3plot16","d3plot17","d3plot18"],
				    "exec_command":[Template("$CODE -i d3plot -b ${prob}.command ")]
				},
           
  		   "image/bar71":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg", "jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "bar71_image_dispmag_plot",    "bar71_image_dispmag_th",
                                                               "bar71_image_eps_plot",        "bar71_image_eps_th",
                                                               "bar71_image_nodposux_plot",   "bar71_image_nodposux_th",
                                                               "bar71_image_nodposuz_plot",   "bar71_image_nodposuz_th",
                                                               "bar71_image_press_rmin_plot" ,"bar71_image_press_rmin_th", 
                                                               "bar71_image_sx_plot",         "bar71_image_sx_th",
                                                               "bar71_image_sxy_plot",        "bar71_image_sxy_th",
                                                               "bar71_image_sz_plot",         "bar71_image_sz_th",
                                                               "bar71_image_szx_plot",        "bar71_image_szx_th",
                                                               "bar71_image_szx_cutpln1_plot","bar71_image_szx_cutpln2_plot",
                                                               "bar71_image_szx_plot",        "bar71_image_szx_th"
                                                               ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
                                                   "required_files":["bar71.plt00",  "bar71.pltA",  "bar71.plt_TI_A",
                                                                     "bar71_V1.gsf"],
						  "exec_command":[Template("$CODE  -w 1200 1200 -i bar71.plt -b ${prob}.gsf ")]
						  },

 		   "image/bending-71":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg", ".jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "bending-71_image_accy_plot",     "bending-71_image_accy_th",
                                                               "bending-71_image_dispr_plot",    "bending-71_image_dispr_th",
                                                               "bending-71_image_dispx_plot",    "bending-71_image_dispx_th",
                                                               "bending-71_image_mt_plot",       "bending-71_image_mt_th",
                                                               "bending-71_image_nodposux_plot", "bending-71_image_nodposux_th",
                                                               "bending-71_image_svecz_plot",    "bending-71_image_svecz_th",
                                                               "bending-71_image_tor_plot",      "bending-71_image_tor_th",
                                                               "bending-71_image_velmag_plot",   "bending-71_image_velmag_th"
                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None, 
						  "awkfiles":[],
						  "awk_commands":[],
                                                  "required_files_alt_dir":"image",
						  "required_files":["bending-71.plt00",  "bending-71.pltA",  "bending-71.plt_TI_A"],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i bending-71.plt -b ${prob}.gsf ")]
                                  },

		   "image/bolt":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg", "jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "bolt_image_gamxy_plot",    "bolt_image_gamxy_th",
                                                               "bolt_image_ez_plot",       "bolt_image_ez_th",
                                                               "bolt_image_sz_plot",       "bolt_image_sz_th",
                                                               "bolt_image_nodposuy_plot", "bolt_image_nodposuy_th",
                                                               "bolt_image_pdev1_plot",    "bolt_image_pdev1_th",
                                                               "bolt_image_press_plot",    "bolt_image_press_th",
                                                               "bolt_image_slide_plot",    "bolt_image_slide_th",
                                                               "bolt_image_maxshr_plot",   "bolt_image_maxshr_th",
                                                               "bolt_image_szx_cutpln1_plot" , "bolt_image_dispy_cutpln1_plot"
                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
                                                  "required_files_alt_dir":"image",
						  "required_files":["boltA", "bolt00",  "bolt01",  "bolt02", "bolt03", "bolt04", "bolt_V1.gsf", "bolt_V2.gsf", "bolt_V3.gsf", "bolt_V4.gsf"],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i bolt -b ${prob}.gsf ")]
                                                   },
           
 		   "image/clamped_plate":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg", ".jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "clamped_plate_image_eps_plot"
                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
						  "required_files":["clamped_plate.th00",  "clamped_plate.thA",  "clamped_plate.th_TI_A"],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i clamped_plate.th -b ${prob}.gsf ")]
                                    },
            
 		   "image/d3samp6":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg",".jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "d3samp6_image_dispz_plot", "d3samp6_image_dispz_th",
                                                               "d3samp6_image_gamxy_plot", "d3samp6_image_gamxy_th",
                                                               "d3samp6_image_press_rmin_plot", "d3samp6_image_press_rmin_th",
                                                               "d3samp6_image_pshrstr_cutpln1_plot", "d3samp6_image_szx_cutpln2_plot"
                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
						  "required_files":["d3samp6.plt00",  "d3samp6.pltA",  "d3samp6.plt_TI_A",
                                                                    "d3samp6_V1.gsf"],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i d3samp6.plt -b ${prob}.gsf ")]
                              },
           
 		   "image/d3snd1":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg",".jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "d3snd1_eps"
                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
						  "required_files":["d3snd1.plt00",  "d3snd1.pltA",  "d3snd1.plt_TI_A"],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i d3snd1.plt -b ${prob}.gsf ")]
                              },
           
 		   "image/diablodbc":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg",".jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "diablodbc_image_nodposux_plot", "diablodbc_image_nodposux_th"
                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
						  "required_files":["dblplt00000",  "dblplt000A", "dblplt000_TI_A", "diablodbc_V1.gsf"],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i dblplt000 -b ${prob}.gsf ")]
                              },
           
 		   "image/discrete":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "discrete_image_discfx_plot", "discrete_image_discfx_th"
                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
						  "required_files":["discrete.plt00",  "discrete.pltA",  "discrete.plt_TI_A"],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i discrete.plt -b ${prob}.gsf ")]
                              },
           
 		   "image/ml40":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg",".jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "ml40_image_epl1x_plot", "ml40_image_epl1x_th", "ml40_image_epl1x_all_th",
                                                               "ml40_image_eps_plot",   "ml40_image_eps_th",
                                                               "ml40_image_sx_plot",    "ml40_image_sx_th",
                                                               "ml40_image_sz_plot",    "ml40_image_sz_th"

                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
                                                  "required_files_alt_dir":"image",
						  "required_files":["ml40.plt00",  "ml40.pltA",  "ml40.plt_TI_A",
                                                                    "ml40_V1.gsf", "ml40_V2.gsf" ],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i ml40.plt -b ${prob}.gsf ")]
                              },

 		   "image/nif":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg",".jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "nif_eps"
                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
						  "required_files":["nif00", "nifA",
                                                                    "nif_V1.gsf", "nif_V2.gsf" ],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i nif -b ${prob}.gsf ")]
                              },
  
		   "image/ntet20":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg",".jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "ntet20_eps"
                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
						  "required_files":["ntet20.plt00",  "ntet20.pltA", "ntet20.pltB", "ntet20.plt_TI_A",
                                                                    "ntet20_V1.gsf"],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i ntet20.plt -b ${prob}.gsf ")]
                              },

 		   "image/partition":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg",".jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "partition_image_mat_plot"
                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
						  "required_files":["partitionA",
                                                                    "partition_V1.gsf"],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i partition -b ${prob}.gsf ")]
                              },

 		   "image/plate71":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg",".jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "plate71_image_sx_plot", "plate71_image_sx_th"
                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
						  "required_files":["plate71.plt00",  "plate71.pltA",  "plate71.plt_TI_A",
                                                                    "plate71_V1.gsf"],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i plate71.plt -b ${prob}.gsf ")]
                              },

 		   "image/thickshells":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg",".jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[ 
                                                               "thickshells_image_eyz_plot", "thickshells_image_eyz_th"
                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
						  "required_files":["thickshells_c00", "thickshells_cA", "thickshells_c_TI_A",
                                                                    "thickshells_V1.gsf" ],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i thickshells_c -b ${prob}.gsf ")]
                                  },
           
 		   "image/tplate1":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg",".jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "tplate1_velx_plot", "tplate1_velx_th",
                                                               "tplate1_sx_plot",   "tplate1_sx_th" 
                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
						  "required_files":["tplate1.plt00",  "tplate1.pltA",  "tplate1.plt_TI_A",
                                                                    "tplate1_V1.gsf" ],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i tplate1.plt -b ${prob}.gsf ")]
                                    },

  		   "image/damage":{"keep_on_fail_file_expr":[".gsf",".plt",".jpg",".jpeg"],
						  "keep_on_pass_file_expr":[".jpg",".gsf",".plt"],
						  "quicklist":[
                                                               "damage_damage_plot",  "damage_damage_th",
                                                               "damage_seff_plot",    "damage_seff_th",
                                                               "damage_evol_plot",    "damage_evol_th",
                                                               "damage_dispmag_plot", "damage_dispmag_th",
							       "damage_pvmag_th"
                                                              ],
						  "test_type":"image",
						  "file_suffix":".gsf",
						  "base_suffix":"jpg",
						  "output_suffix":"answ.jpg",
						  "tests":None,
						  "awkfiles":[],
						  "awk_commands":[],
						  "required_files":["bar45.plt00",  "bar45.pltA",  "bar45.plt_TI_A",
                                                                    "bar45_V1.gsf", "bar45_V2.gsf" ],
						  "exec_command":[Template("$CODE  -q -w 1200 1200 -i bar45.plt -b ${prob}.gsf ")]
                                    }
             }
		   

