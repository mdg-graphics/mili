#!/bin/env python3

import sys
import os
import shutil
import argparse
import subprocess

def main( args ):

  sys_type = os.environ.get('SYS_TYPE', 'unknown')
  print( f"Detected system type: {sys_type}" )
  # Use system information to determine default host-config file
  default_host_config_by_system = {
    "toss_3_x86_64_ib": "host-configs/toss_3_x86_64_ib-intel@2021.4.cmake",
    "toss_4_x86_64_ib": "host-configs/toss_4_x86_64_ib-intel_classic@2021.6.0.cmake",
    "unknown": None
  }
  host_config_default = default_host_config_by_system[sys_type]

  # Set up argument parser
  parser = argparse.ArgumentParser( description = "Configure mili library"  )
  parser.add_argument('-hc', "--hostconfig", type = str, help = "A host-config file to initialize the cmake cache.", default=host_config_default )
  build_type_options = ["Debug","Release","RelWithDebInfo","RelCleanup"]
  parser.add_argument('-bt', "--buildtype", type = str, help = "Build type", choices = build_type_options, default = "RelWithDebInfo" )
  parser.add_argument('-id', '--install-dir', type = str, help = "Installation directory", default = None)
  parser.add_argument('-so','--suppress-output', action="store_true", default=False )
  parser.add_argument('--show-build-commands', help = "Generate a verbose Makefile: shows the build commands (default=false)", default = False, action='store_true')

  args, unknown = parser.parse_known_args( args )
  if unknown:
    print( f"Unknown arguments being appended to cmake call: {unknown}" )

  if args.hostconfig is None:
    print( f"ERROR: A host config must be provided." )
    sys.exit( -1 )
  if not ( os.path.exists( args.hostconfig ) and os.path.isfile( args.hostconfig ) ):
    print( f"ERROR: cannot find file {args.hostconfig}, or it is not a file." )
    sys.exit( -1 )

  setup_blt_path = os.path.join( os.getcwd(), 'cmake', 'blt', 'SetupBLT.cmake' )
  if not ( os.path.exists( setup_blt_path ) ):
    print( f"ERROR: SetupBLT.cmake not in expected location: {setup_blt_path}\n       Have git submodules been initialized and updated?" )
    sys.exit( -1 )

  hostconfig_filename = os.path.split(args.hostconfig)[1]
  if hostconfig_filename.endswith(".cmake"):
    hostconfig_filename = hostconfig_filename[:-6]
  compiler_name = hostconfig_filename.split("-")[1]

  full_host_config = os.path.realpath( args.hostconfig )

  install_dir_format = [ "install", sys_type, compiler_name, args.buildtype.lower() ]
  if args.install_dir == None:
    install_dir = "-".join( install_dir_format )
  else:
    install_dir = args.install_dir

  install_dir = os.path.abspath( install_dir )
  if os.path.exists( install_dir ):
    shutil.rmtree( install_dir )
  os.makedirs( install_dir )

  config_dir_list = [ "build", sys_type, compiler_name, args.buildtype.lower() ]
  config_dir = "-".join( config_dir_list )

  config_dir = os.path.abspath( config_dir )
  if os.path.exists( config_dir ):
    shutil.rmtree( config_dir )
    if os.path.exists( config_dir ):
      raise FileExistsError( f"Failed to remove the build directory {config_dir}. Is there a locked/open file?")
  os.makedirs( config_dir )
  print( "Changing to build directory.." )
  cwd = os.getcwd()
  os.chdir( config_dir )

  config_cmd = [ "cmake",
                 "..",
                 *unknown,
                 "-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON",
                f"-DCMAKE_BUILD_TYPE:STRING={args.buildtype}",
                f"-DCMAKE_INSTALL_PREFIX:PATH={install_dir}",
                f"-DCMAKE_VERBOSE_MAKEFILE:BOOL={'ON' if args.show_build_commands else 'OFF'}",
                f"-C {full_host_config}" ]

  # Define full configure command so that it can be output in Mili
  config_cmd.append( f"-DMILI_CONFIG_CMD:STRING=\"{' '.join(config_cmd)}\"" )

  sep = "="*80
  print(sep)
  print("configuring build with command:")
  print("")
  print(" ".join(config_cmd))
  print("")
  print(sep)
  config_result = subprocess.run( " ".join(config_cmd), shell=True, text=True, capture_output=args.suppress_output, check=True )
  return config_result

if __name__ == "__main__":
  sys.dont_write_bytecode = True #prevent __pycache__ and .pyc file generation
  main( sys.argv[1:] )
