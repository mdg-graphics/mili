# Mili

### Description

Binary Database format used by MDG codes for mesh simulations.

### Table of Contents
- [Developer Guide](#developer-guide)
- [Developer Workflow](#developer-workflow)

## GitLab and LC Setup

This section includes required steps for setting up your GitLab and LC Account

### GitLab Setup

1. Add an ssh key to RZ GitLab
    - Create the ssh key on the machine you will use to access gitlab, such as an RZ machine, laptop, etc.
    - Create an ssh key: `ssh-keygen -t rsa -b 4096`
        - Use the standard location. (`~/.ssh/id_rsa` and `~/.ssh/id_rsa.pub`)
        - Most people press enter and don't have a password.
        - Keys should be 4096 bits or longer.
        - Use RSA
    - Add key to gitlab here: https://rzlc.llnl.gov/gitlab/-/profile/keys

2. CI Pipeline/Testing Setup
    - How CI pipelines works on GitLab
        - There is something called gitlab runners (managed by LC, not the team)
        - CI pipelines are run in the user's LC account
        - CI pipelines are run by default in your home directory in `~/.jacamar-ci/`
            - Don’t do this. Your quota is small there. 24 GB.
        - CI pipelines are triggered whenever:
            - A user pushes to a branch with an open merge request
            - manually triggered through the GitLab web interface
    - Modify your home directory such that tests are run on workspace instead:
        - Make a directory in your workspace: `mkdir /usr/workspace/<your-lc-username>/gitlab_testing`
        - Symlink test location to this location: `ln -s /usr/workspace/<your-lc-username>/gitlab_testing ~/.jacamar-ci`

### Local Repo Setup

Once you have complete the above steps you can clone the repository. To clone the repository into your current location run:
```
git clone ssh://git@rzgitlab.llnl.gov:7999/mdg/mili/mili.git
```

### Submodules

Mili contains the following git submodules:
- mdgtest (MDG's generic testing scripts)

When checking out the repository for the first time, submodules should be updated/initialized via:
```
git submodule update --init --recursive
```

If you don't want to have to remember to update submodules when pulling, you can configure git (globally for your user on RZ, so it will effect all git repos you operate on) to automatically update submodules when you pull:
```
git config --global submodule.recurse true
```

## Configuration

Mili's BLT/Cmake-based build system is largely modeled after Diablo's build system, providing a `configure.py` script to automate the configuration and setup of build and install directories based upon an input host-config file. A handful of example host-config files are provided for reference, consistent with the standard build options used by Mili on TOSS3 and TOSS4 machines.

Mili requires CMake version 3.18 or later. On LLNL machines, the user's local version of CMake can be updated using the module load command, e.g.:
```
module load cmake/3.18.0
```

> For convenience, the `module load` command above may be added to the user's local `.cshrc` or `.bashrc` file to avoid having to re-load the correct CMake module after login.

Specific builds are configured via:
```
configure.py -hc host-configs/<host_config_file_name>.cmake -bt <build_type>
```
which generates corresponding `build-<host_config_file_name>-<build_type>` and `install-<host_config_file_name>-<build_type>` subdirectories, and uses CMake to generate a Makefile in the `build` directory.

The `<build_type>` can be any one of the following:
|Build Type     |Compiler Flags|
|---------------|--------------|
|Debug          |`-g`          |
|Release        |`-O3`         |
|RelWithDebInfo |`-g -O3`      |

The default build type is currently set to `RelWithDebInfo`.

## Style/Formatting

Mili uses `clang-format` to maintain a consistant style and format throughout the code. The style rules can be found in the file `clang-format.yaml` in the directory `griz/scripts/style/`. To use clang-format you must configure using the toss3 host-config file which sets up clang-format for you. To style your code run `make clangformat_style` and to check if you code matches the style guide run `clangformat_check`.

## Compilation

Once a particular build directory has been generated using the `configure.py` script, the code is compiled within this new directory using the standard `make` utility, e.g. to compile the code:
```
make
```

## Testing  (**LLNL Specific**)

The test suite for mili and xmilics reside in the directories: `mili/test/mili` and `mili/test/xmilics`. The scripts that run the tests can be found in the git submodule `mdgtest` in the `mili/test` directory.

You shouldn't run the test suite on a login node, so you will have to get access to a debug/batch node. This can be done by running the command:
```
sxterm 1 36 60 -p pdebug -A wbronze
```

You can now run the test suite as shown below:

### Mili Library
```
mdgtest/Test.py -e mililib_env -c <path-to-code> -I <include-dir> -n1 -p1 -q -s all
```

### Xmilics
```
mdgtest/Test.py -e xmilics_env -c <path-to-code> -n1 -p1 -q -s all
```

## Deployment on OCF (**LLNL Specific**)

To do deploy Mili on the OCF perform the following steps

### 1. Update Version

In the directory `mili`, edit the top level `CMakeLists.txt` and change the following

- If we are changing the version, change appropriately
```
# Build version
set( major_version 23 )
set( minor_version 02 )
set( patch_version 03 )
string( CONCAT PACKAGE_VERSION "${major_version}_${minor_version}_${patch_version}" )
```

### 2. Configure and Build

Build the release version of Mili:

```
configure.py -bt Release
cd <build-directory>
make -j
```

This will build:
- The Mili library files in the directory `<build-dir>/lib`
- The Mili include files in `<build-dir>/include`
- Xmilics and other tools in `<build-dir>/bin`

### 3. Give files to mdgadmin

```
give mdgadmin lib/libmili.a lib/libtaurus.a lib/libeprtf.a
```

### 4. Login as mdgadmin

```
xsu mdgadmin
```

### If creating a NEW version

If this is a new version then we need to create a new directory. Follow the steps below:

5. `cd /collab/usr/apps/mdg/archive/milidir`
6. `cp -p -r [Last Version Directory name (i.e V16_01)] [New version name]`
7. `rm lib.alpha`
8. `ln -sf [New version name] lib.alpha`
9. `cd lib.alpha/lib`
10. `take -f <your username>`
11. `chmod g+rx libeprtf.a libmili.a libtaurus.a`

### NOT a new version

5. `cd /collab/usr/apps/mdg/archive/milidir/[lib.alpha|lib.production|V23_02]/lib`
6. `mkdir mili-MM.DD.YYYY` (MM.DD.YYYY should be date of version being moved)
7. `mv libmili.a libtaurus.a lib eprtf.a mili-MM.DD.YYYY/`
8. `take -f <your username>`
9. `chmod g+rx libeprtf.a libmili.a libtaurus.a`

### Install new include files

Any new include files should be added in the directory `/collab/usr/apps/mdg/archive/milidir/<version-dir>/include`.

### Installing Xmilics

Similar steps can be followed to install new version of Xmilics or makemili in `/collab/usr/apps/mdg/archive/militools`.

### Licences

SPDX-License-Identifier: LGPL-2.1-or-later
