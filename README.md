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

To configure mili use the command:
```
cd mili/
configure
```
This will generate a new build directory starting with `MILI`, for example: `MILI-toss_3_x86_64_ib-RZGENIE`

## Compilation

Once the build directory has been generated using `configure`, the code is compiled within this new directory using the `gmake` utility.

There are currently 4 standard builds that are commonly made:

|Build Type     |Description|
|---------------|--------------|
|debug          |Debug version of Mili Library        |
|opt            |Optimized version of Mili Library    |
|utilsdebug     |Debug version of Mili utilities      |
|utilsopt       |Optimized version of Mili utilities  |

These are compiled by running the command:

```
gmake <build>
```

For example:

```
gmake debug; gmake opt; gmake utilsdebug; gmake utilsopt
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

### Licences

SPDX-License-Identifier: LGPL-2.1-or-later
