#!/usr/bin/env python
#
# recompiles all Make-based projects within the repository
# excluding any that are located within the specified excluded
# directory list
#
import sys
import os

filter_dirs = ["libDaisy",
               "DaisySP",
               ".github",
               ".vscode",
               ".git",
               "ci",
               "cube",
               "dist",
               "utils",
               "stmlib",
               "libdaisy",
               "MyProjects"]

dirs_to_search = list(
    filter(lambda x: x not in filter_dirs and os.path.isdir(x), os.listdir('.')))

# recursively go through each directory in dirs_to_search
# and attempt to compile each example
for dir in dirs_to_search:
    example_dirs = []
    for root, dirs, files in os.walk(dir):
        if 'Makefile' in files:
            example_dirs.append(root)
    cwd = os.path.abspath(os.getcwd())
    for ex in example_dirs:
        dest = os.path.join(cwd, ex)
        os.chdir(dest)
        os.system("echo Building: {}".format(ex))
        exit_code = os.system('make -s clean')
        exit_code = os.system('make -s')
        if exit_code != 0:
            os.chdir(cwd)
            sys.exit(1)
    os.chdir(cwd)
# exit successfully
print("done")
sys.exit(0)
